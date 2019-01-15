/* GStreamer
 * Copyright (C) <2005> Julien Moutte <julien@moutte.net>
 *               <2009>,<2010> Stefan Kost <stefan.kost@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/* for developers: there are two useful tools : xvinfo and xvattr */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* Object header */
#include "xvcontext.h"

/* Debugging category */
#include <gst/gstinfo.h>

/* for XkbKeycodeToKeysym */
#include <X11/XKBlib.h>

GST_DEBUG_CATEGORY_EXTERN (gst_debug_xvcontext);
GST_DEBUG_CATEGORY_EXTERN (GST_CAT_PERFORMANCE);
#define GST_CAT_DEFAULT gst_debug_xvcontext

#ifdef GST_EXT_XV_ENHANCEMENT
#define _BUFFER_WAIT_TIMEOUT            200000
#define _CHECK_DISPLAYED_BUFFER_COUNT   30

/* headers for drm */
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>
#include <X11/Xmd.h>
#include <dri2/dri2.h>
#include <libdrm/drm.h>

static XImage *make_transparent_image (Display * d, Window win, int w, int h);
static unsigned int drm_convert_dmabuf_gemname (GstXvContext * context,
    unsigned int dmabuf_fd, unsigned int *gem_handle);
static void drm_close_gem (GstXvContext * context, unsigned int *gem_handle);
#endif /* GST_EXT_XV_ENHANCEMENT */

void
gst_xvcontext_config_clear (GstXvContextConfig * config)
{
  if (config->display_name) {
    g_free (config->display_name);
    config->display_name = NULL;
  }
  config->adaptor_nr = -1;
}

GST_DEFINE_MINI_OBJECT_TYPE (GstXvContext, gst_xvcontext);

typedef struct
{
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
  long input_mode;
  unsigned long status;
}
MotifWmHints, MwmHints;

#define MWM_HINTS_DECORATIONS   (1L << 1)


static void
gst_lookup_xv_port_from_adaptor (GstXvContext * context,
    XvAdaptorInfo * adaptors, guint adaptor_nr)
{
  gint j;
  gint res;

  /* Do we support XvImageMask ? */
  if (!(adaptors[adaptor_nr].type & XvImageMask)) {
    GST_DEBUG ("XV Adaptor %s has no support for XvImageMask",
        adaptors[adaptor_nr].name);
    return;
  }

  /* We found such an adaptor, looking for an available port */
  for (j = 0; j < adaptors[adaptor_nr].num_ports && !context->xv_port_id; j++) {
    /* We try to grab the port */
    res = XvGrabPort (context->disp, adaptors[adaptor_nr].base_id + j, 0);
    if (Success == res) {
      context->xv_port_id = adaptors[adaptor_nr].base_id + j;
      GST_DEBUG ("XV Adaptor %s with %ld ports", adaptors[adaptor_nr].name,
          adaptors[adaptor_nr].num_ports);
    } else {
      GST_DEBUG ("GrabPort %d for XV Adaptor %s failed: %d", j,
          adaptors[adaptor_nr].name, res);
    }
  }
}

#ifdef GST_EXT_XV_ENHANCEMENT
#define FOURCC(a,b,c,d) (((unsigned)d&0xff)<<24 | ((unsigned)c&0xff)<<16 | ((unsigned)b&0xff)<<8 | ((unsigned)a&0xff))
#endif /* GST_EXT_XV_ENHANCEMENT */

/* This function generates a caps with all supported format by the first
   Xv grabable port we find. We store each one of the supported formats in a
   format list and append the format to a newly created caps that we return
   If this function does not return NULL because of an error, it also grabs
   the port via XvGrabPort */
static GstCaps *
gst_xvcontext_get_xv_support (GstXvContext * context,
    const GstXvContextConfig * config, GError ** error)
{
  gint i;
  XvAdaptorInfo *adaptors;
  gint nb_formats;
  XvImageFormatValues *formats = NULL;
  guint nb_encodings;
  XvEncodingInfo *encodings = NULL;
  gulong max_w = G_MAXINT, max_h = G_MAXINT;
  GstCaps *caps = NULL;
  GstCaps *rgb_caps = NULL;

  g_return_val_if_fail (context != NULL, NULL);

  /* First let's check that XVideo extension is available */
  if (!XQueryExtension (context->disp, "XVideo", &i, &i, &i))
    goto no_xv;

  /* Then we get adaptors list */
  if (Success != XvQueryAdaptors (context->disp, context->root,
          &context->nb_adaptors, &adaptors))
    goto no_adaptors;

  context->xv_port_id = 0;

  GST_DEBUG ("Found %u XV adaptor(s)", context->nb_adaptors);

  context->adaptors =
      (gchar **) g_malloc0 (context->nb_adaptors * sizeof (gchar *));

  /* Now fill up our adaptor name array */
  for (i = 0; i < context->nb_adaptors; i++) {
    context->adaptors[i] = g_strdup (adaptors[i].name);
  }

  if (config->adaptor_nr != -1 && config->adaptor_nr < context->nb_adaptors) {
    /* Find xv port from user defined adaptor */
    gst_lookup_xv_port_from_adaptor (context, adaptors, config->adaptor_nr);
  }

  if (!context->xv_port_id) {
    /* Now search for an adaptor that supports XvImageMask */
    for (i = 0; i < context->nb_adaptors && !context->xv_port_id; i++) {
      gst_lookup_xv_port_from_adaptor (context, adaptors, i);
      context->adaptor_nr = i;
    }
  }

  XvFreeAdaptorInfo (adaptors);

  if (!context->xv_port_id)
    goto no_ports;

  /* Set XV_AUTOPAINT_COLORKEY and XV_DOUBLE_BUFFER and XV_COLORKEY */
  {
    int count, todo = 4;
    XvAttribute *const attr = XvQueryPortAttributes (context->disp,
        context->xv_port_id, &count);
    static const char autopaint[] = "XV_AUTOPAINT_COLORKEY";
    static const char dbl_buffer[] = "XV_DOUBLE_BUFFER";
    static const char colorkey[] = "XV_COLORKEY";
    static const char iturbt709[] = "XV_ITURBT_709";

    GST_DEBUG ("Checking %d Xv port attributes", count);

    context->have_autopaint_colorkey = FALSE;
    context->have_double_buffer = FALSE;
    context->have_colorkey = FALSE;
    context->have_iturbt709 = FALSE;

    for (i = 0; ((i < count) && todo); i++) {
      GST_DEBUG ("Got attribute %s", attr[i].name);

      if (!strcmp (attr[i].name, autopaint)) {
        const Atom atom = XInternAtom (context->disp, autopaint, False);

        /* turn on autopaint colorkey */
        XvSetPortAttribute (context->disp, context->xv_port_id, atom,
            (config->autopaint_colorkey ? 1 : 0));
        todo--;
        context->have_autopaint_colorkey = TRUE;
      } else if (!strcmp (attr[i].name, dbl_buffer)) {
        const Atom atom = XInternAtom (context->disp, dbl_buffer, False);

        XvSetPortAttribute (context->disp, context->xv_port_id, atom,
            (config->double_buffer ? 1 : 0));
        todo--;
        context->have_double_buffer = TRUE;
      } else if (!strcmp (attr[i].name, colorkey)) {
        /* Set the colorkey, default is something that is dark but hopefully
         * won't randomly appear on the screen elsewhere (ie not black or greys)
         * can be overridden by setting "colorkey" property
         */
        const Atom atom = XInternAtom (context->disp, colorkey, False);
        guint32 ckey = 0;
        gboolean set_attr = TRUE;
        guint cr, cg, cb;

        /* set a colorkey in the right format RGB565/RGB888
         * We only handle these 2 cases, because they're the only types of
         * devices we've encountered. If we don't recognise it, leave it alone
         */
        cr = (config->colorkey >> 16);
        cg = (config->colorkey >> 8) & 0xFF;
        cb = (config->colorkey) & 0xFF;
        switch (context->depth) {
          case 16:             /* RGB 565 */
            cr >>= 3;
            cg >>= 2;
            cb >>= 3;
            ckey = (cr << 11) | (cg << 5) | cb;
            break;
          case 24:
          case 32:             /* RGB 888 / ARGB 8888 */
            ckey = (cr << 16) | (cg << 8) | cb;
            break;
          default:
            GST_DEBUG ("Unknown bit depth %d for Xv Colorkey - not adjusting",
                context->depth);
            set_attr = FALSE;
            break;
        }

        if (set_attr) {
          ckey = CLAMP (ckey, (guint32) attr[i].min_value,
              (guint32) attr[i].max_value);
          GST_LOG ("Setting color key for display depth %d to 0x%x",
              context->depth, ckey);

          XvSetPortAttribute (context->disp, context->xv_port_id, atom,
              (gint) ckey);
        }
        todo--;
        context->have_colorkey = TRUE;
      } else if (!strcmp (attr[i].name, iturbt709)) {
        todo--;
        context->have_iturbt709 = TRUE;
      }
    }

    XFree (attr);
  }

  /* Get the list of encodings supported by the adapter and look for the
   * XV_IMAGE encoding so we can determine the maximum width and height
   * supported */
  XvQueryEncodings (context->disp, context->xv_port_id, &nb_encodings,
      &encodings);

  for (i = 0; i < nb_encodings; i++) {
    GST_LOG ("Encoding %d, name %s, max wxh %lux%lu rate %d/%d",
        i, encodings[i].name, encodings[i].width, encodings[i].height,
        encodings[i].rate.numerator, encodings[i].rate.denominator);
    if (strcmp (encodings[i].name, "XV_IMAGE") == 0) {
      max_w = encodings[i].width;
      max_h = encodings[i].height;
    }
  }

  XvFreeEncodingInfo (encodings);

  /* We get all image formats supported by our port */
  formats = XvListImageFormats (context->disp,
      context->xv_port_id, &nb_formats);
  caps = gst_caps_new_empty ();
  for (i = 0; i < nb_formats; i++) {
    GstCaps *format_caps = NULL;
    gboolean is_rgb_format = FALSE;
    GstVideoFormat vformat;

    /* We set the image format of the context to an existing one. This
       is just some valid image format for making our xshm calls check before
       caps negotiation really happens. */
    context->im_format = formats[i].id;

    switch (formats[i].type) {
      case XvRGB:
      {
        XvImageFormatValues *fmt = &(formats[i]);
        gint endianness;

        endianness =
            (fmt->byte_order == LSBFirst ? G_LITTLE_ENDIAN : G_BIG_ENDIAN);
#ifdef GST_EXT_XV_ENHANCEMENT
#ifndef _TIZEN_PUBLIC_
        if (formats[i].id == FOURCC('S','R','3','2')) {
          vformat = gst_video_format_from_fourcc (formats[i].id);

          format_caps = gst_caps_new_simple ("video/x-raw",
            "format", G_TYPE_STRING, "SR32",
            "width", GST_TYPE_INT_RANGE, 1, max_w,
            "height", GST_TYPE_INT_RANGE, 1, max_h,
            "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);
        } else
#endif /* _TIZEN_PUBLIC_ */
#endif /* GST_EXT_XV_ENHANCEMENT */
        {
          vformat = gst_video_format_from_masks (fmt->depth, fmt->bits_per_pixel,
            endianness, fmt->red_mask, fmt->green_mask, fmt->blue_mask, 0);
          if (vformat == GST_VIDEO_FORMAT_UNKNOWN)
            break;

          format_caps = gst_caps_new_simple ("video/x-raw",
              "format", G_TYPE_STRING, gst_video_format_to_string (vformat),
              "width", GST_TYPE_INT_RANGE, 1, max_w,
              "height", GST_TYPE_INT_RANGE, 1, max_h,
              "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);
        }

        is_rgb_format = TRUE;
        break;
      }
      case XvYUV:
      {
        vformat = gst_video_format_from_fourcc (formats[i].id);
        if (vformat == GST_VIDEO_FORMAT_UNKNOWN)
          break;

        format_caps = gst_caps_new_simple ("video/x-raw",
            "format", G_TYPE_STRING, gst_video_format_to_string (vformat),
            "width", GST_TYPE_INT_RANGE, 1, max_w,
            "height", GST_TYPE_INT_RANGE, 1, max_h,
            "framerate", GST_TYPE_FRACTION_RANGE, 0, 1, G_MAXINT, 1, NULL);
        break;
      }
      default:
        vformat = GST_VIDEO_FORMAT_UNKNOWN;
        g_assert_not_reached ();
        break;
    }

    if (format_caps) {
      GstXvImageFormat *format = NULL;

      format = g_new0 (GstXvImageFormat, 1);
      if (format) {
        format->format = formats[i].id;
        format->vformat = vformat;
        format->caps = gst_caps_copy (format_caps);
        context->formats_list = g_list_append (context->formats_list, format);
      }

      if (is_rgb_format) {
        if (rgb_caps == NULL)
          rgb_caps = format_caps;
        else
          gst_caps_append (rgb_caps, format_caps);
      } else
        gst_caps_append (caps, format_caps);
    }
  }

  /* Collected all caps into either the caps or rgb_caps structures.
   * Append rgb_caps on the end of YUV, so that YUV is always preferred */
  if (rgb_caps)
    gst_caps_append (caps, rgb_caps);

  if (formats)
    XFree (formats);

  GST_DEBUG ("Generated the following caps: %" GST_PTR_FORMAT, caps);

  if (gst_caps_is_empty (caps))
    goto no_caps;

  return caps;

  /* ERRORS */
no_xv:
  {
    g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_SETTINGS,
        ("XVideo extension is not available"));
    return NULL;
  }
no_adaptors:
  {
    g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_SETTINGS,
        ("Failed getting XV adaptors list"));
    return NULL;
  }
no_ports:
  {
    context->adaptor_nr = -1;
    g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_BUSY,
        ("No Xv Port available"));
    return NULL;
  }
no_caps:
  {
    gst_caps_unref (caps);
    g_set_error (error, GST_STREAM_ERROR, GST_STREAM_ERROR_WRONG_TYPE,
        ("No supported format found"));
    return NULL;
  }
}

/* This function calculates the pixel aspect ratio based on the properties
 * in the context structure and stores it there. */
static void
gst_xvcontext_calculate_pixel_aspect_ratio (GstXvContext * context)
{
  static const gint par[][2] = {
    {1, 1},                     /* regular screen */
    {16, 15},                   /* PAL TV */
    {11, 10},                   /* 525 line Rec.601 video */
#ifdef GST_EXT_XV_ENHANCEMENT
    {44, 46},                   /* Curved display */
#endif
    {54, 59},                   /* 625 line Rec.601 video */
    {64, 45},                   /* 1280x1024 on 16:9 display */
    {5, 3},                     /* 1280x1024 on 4:3 display */
    {4, 3}                      /*  800x600 on 16:9 display */
  };
  gint i;
  gint index;
  gdouble ratio;
  gdouble delta;

#define DELTA(idx) (ABS (ratio - ((gdouble) par[idx][0] / par[idx][1])))

  /* first calculate the "real" ratio based on the X values;
   * which is the "physical" w/h divided by the w/h in pixels of the display */
  ratio = (gdouble) (context->widthmm * context->height)
      / (context->heightmm * context->width);

  /* DirectFB's X in 720x576 reports the physical dimensions wrong, so
   * override here */
  if (context->width == 720 && context->height == 576) {
    ratio = 4.0 * 576 / (3.0 * 720);
  }
  GST_DEBUG ("calculated pixel aspect ratio: %f", ratio);

  /* now find the one from par[][2] with the lowest delta to the real one */
  delta = DELTA (0);
  index = 0;

  for (i = 1; i < sizeof (par) / (sizeof (gint) * 2); ++i) {
    gdouble this_delta = DELTA (i);

    if (this_delta < delta) {
      index = i;
      delta = this_delta;
    }
  }

  GST_DEBUG ("Decided on index %d (%d/%d)", index,
      par[index][0], par[index][1]);

  g_free (context->par);
  context->par = g_new0 (GValue, 1);
  g_value_init (context->par, GST_TYPE_FRACTION);
  gst_value_set_fraction (context->par, par[index][0], par[index][1]);
  GST_DEBUG ("set context PAR to %d/%d",
      gst_value_get_fraction_numerator (context->par),
      gst_value_get_fraction_denominator (context->par));
}

#ifdef HAVE_XSHM
/* X11 stuff */
static gboolean error_caught = FALSE;

static int
gst_xvimage_handle_xerror (Display * display, XErrorEvent * xevent)
{
  char error_msg[1024];

#ifdef GST_EXT_XV_ENHANCEMENT
  if (xevent) {
    XGetErrorText (display, xevent->error_code, error_msg, 1024);
    error_msg[1023] = '\0';
    GST_DEBUG ("context triggered an XError. error: %s", error_msg);
  } else {
    GST_ERROR ("CAUTION:xevent is NULL");
  }
#else /* GST_EXT_XV_ENHANCEMENT */
  XGetErrorText (display, xevent->error_code, error_msg, 1024);
  GST_DEBUG ("xvimage triggered an XError. error: %s", error_msg);
#endif /* GST_EXT_XV_ENHANCEMENT */
  error_caught = TRUE;
  return 0;
}

/* This function checks that it is actually really possible to create an image
   using XShm */
static gboolean
gst_xvcontext_check_xshm_calls (GstXvContext * context)
{
  XvImage *xvimage;
  XShmSegmentInfo SHMInfo;
  size_t size;
  int (*handler) (Display *, XErrorEvent *);
  gboolean result = FALSE;
  gboolean did_attach = FALSE;

  g_return_val_if_fail (context != NULL, FALSE);

  /* Sync to ensure any older errors are already processed */
  XSync (context->disp, FALSE);

  /* Set defaults so we don't free these later unnecessarily */
  SHMInfo.shmaddr = ((void *) -1);
  SHMInfo.shmid = -1;

  /* Setting an error handler to catch failure */
  error_caught = FALSE;
  handler = XSetErrorHandler (gst_xvimage_handle_xerror);

  /* Trying to create a 1x1 picture */
  GST_DEBUG ("XvShmCreateImage of 1x1");
  xvimage = XvShmCreateImage (context->disp, context->xv_port_id,
      context->im_format, NULL, 1, 1, &SHMInfo);

  /* Might cause an error, sync to ensure it is noticed */
  XSync (context->disp, FALSE);
  if (!xvimage || error_caught) {
    GST_WARNING ("could not XvShmCreateImage a 1x1 image");
    goto beach;
  }
  size = xvimage->data_size;
  SHMInfo.shmid = shmget (IPC_PRIVATE, size, IPC_CREAT | 0777);
  if (SHMInfo.shmid == -1) {
    GST_WARNING ("could not get shared memory of %" G_GSIZE_FORMAT " bytes",
        size);
    goto beach;
  }

  SHMInfo.shmaddr = shmat (SHMInfo.shmid, NULL, 0);
  if (SHMInfo.shmaddr == ((void *) -1)) {
    GST_WARNING ("Failed to shmat: %s", g_strerror (errno));
    /* Clean up the shared memory segment */
    shmctl (SHMInfo.shmid, IPC_RMID, NULL);
    goto beach;
  }

  xvimage->data = SHMInfo.shmaddr;
  SHMInfo.readOnly = FALSE;

  if (XShmAttach (context->disp, &SHMInfo) == 0) {
    GST_WARNING ("Failed to XShmAttach");
    /* Clean up the shared memory segment */
    shmctl (SHMInfo.shmid, IPC_RMID, NULL);
    goto beach;
  }

  /* Sync to ensure we see any errors we caused */
  XSync (context->disp, FALSE);

  /* Delete the shared memory segment as soon as everyone is attached.
   * This way, it will be deleted as soon as we detach later, and not
   * leaked if we crash. */
  shmctl (SHMInfo.shmid, IPC_RMID, NULL);

  if (!error_caught) {
    GST_DEBUG ("XServer ShmAttached to 0x%x, id 0x%lx", SHMInfo.shmid,
        SHMInfo.shmseg);

    did_attach = TRUE;
    /* store whether we succeeded in result */
    result = TRUE;
  } else {
    GST_WARNING ("MIT-SHM extension check failed at XShmAttach. "
        "Not using shared memory.");
  }

beach:
  /* Sync to ensure we swallow any errors we caused and reset error_caught */
  XSync (context->disp, FALSE);

  error_caught = FALSE;
  XSetErrorHandler (handler);

  if (did_attach) {
    GST_DEBUG ("XServer ShmDetaching from 0x%x id 0x%lx",
        SHMInfo.shmid, SHMInfo.shmseg);
    XShmDetach (context->disp, &SHMInfo);
    XSync (context->disp, FALSE);
  }
  if (SHMInfo.shmaddr != ((void *) -1))
    shmdt (SHMInfo.shmaddr);
  if (xvimage)
    XFree (xvimage);
  return result;
}
#endif /* HAVE_XSHM */

static GstXvContext *
gst_xvcontext_copy (GstXvContext * context)
{
  return NULL;
}

static void
gst_xvcontext_free (GstXvContext * context)
{
  GList *formats_list, *channels_list;
  gint i = 0;

  GST_LOG ("free %p", context);

  formats_list = context->formats_list;

  while (formats_list) {
    GstXvImageFormat *format = formats_list->data;

    gst_caps_unref (format->caps);
    g_free (format);
    formats_list = g_list_next (formats_list);
  }

  if (context->formats_list)
    g_list_free (context->formats_list);

  channels_list = context->channels_list;

  while (channels_list) {
    GstColorBalanceChannel *channel = channels_list->data;

    g_object_unref (channel);
    channels_list = g_list_next (channels_list);
  }

  if (context->channels_list)
    g_list_free (context->channels_list);

  if (context->caps)
    gst_caps_unref (context->caps);
  if (context->last_caps)
    gst_caps_unref (context->last_caps);

  for (i = 0; i < context->nb_adaptors; i++) {
    g_free (context->adaptors[i]);
  }

  g_free (context->adaptors);

  g_free (context->par);

  GST_DEBUG ("Closing display and freeing X Context");

  if (context->xv_port_id)
    XvUngrabPort (context->disp, context->xv_port_id, 0);

  if (context->disp)
    XCloseDisplay (context->disp);

#ifdef GST_EXT_XV_ENHANCEMENT
  if (context->display_buffer_lock) {
    g_mutex_free (context->display_buffer_lock);
    context->display_buffer_lock = NULL;
  }
  if (context->display_buffer_cond) {
    g_cond_free (context->display_buffer_cond);
    context->display_buffer_cond = NULL;
  }
#endif /* GST_EXT_XV_ENHANCEMENT */

  g_mutex_clear (&context->lock);

  g_slice_free1 (sizeof (GstXvContext), context);
}

#ifdef GST_EXT_XV_ENHANCEMENT
gboolean
gst_xvcontext_set_display_mode (GstXvContext * context, int set_mode)
{
  int ret = 0;
  static gboolean is_exist = FALSE;
  static XvPortID current_port_id = -1;
  Atom atom_output = None;

  if (context == NULL) {
    GST_WARNING ("context is NULL");
    return FALSE;
  }

  /* check once per one xv_port_id */
  if (current_port_id != context->xv_port_id) {
    /* check whether _USER_WM_PORT_ATTRIBUTE_OUTPUT attribute is existed */
    int i = 0;
    int count = 0;
    XvAttribute *const attr = XvQueryPortAttributes (context->disp,
        context->xv_port_id, &count);
    if (attr) {
      current_port_id = context->xv_port_id;
      for (i = 0; i < count; i++) {
        if (!strcmp (attr[i].name, "_USER_WM_PORT_ATTRIBUTE_OUTPUT")) {
          is_exist = TRUE;
          GST_INFO ("_USER_WM_PORT_ATTRIBUTE_OUTPUT[index %d] found", i);
          break;
        }
      }
      XFree (attr);
    } else {
      GST_WARNING ("XvQueryPortAttributes disp:%d, port_id:%d failed",
          context->disp, context->xv_port_id);
    }
  }

  if (is_exist) {
    GST_INFO ("set display mode %d", set_mode);
    atom_output = XInternAtom (context->disp,
        "_USER_WM_PORT_ATTRIBUTE_OUTPUT", False);
    ret = XvSetPortAttribute (context->disp, context->xv_port_id,
        atom_output, set_mode);
    if (ret == Success) {
      return TRUE;
    } else {
      GST_WARNING ("display mode[%d] set failed.", set_mode);
    }
  } else {
    GST_WARNING ("_USER_WM_PORT_ATTRIBUTE_OUTPUT is not existed");
  }

  return FALSE;
}
#endif /* GST_EXT_XV_ENHANCEMENT */

/* This function gets the X Display and global info about it. Everything is
   stored in our object and will be cleaned when the object is disposed. Note
   here that caps for supported format are generated without any window or
   image creation */
GstXvContext *
gst_xvcontext_new (GstXvContextConfig * config, GError ** error)
{
  GstXvContext *context = NULL;
  XPixmapFormatValues *px_formats = NULL;
  gint nb_formats = 0, i, j, N_attr;
  XvAttribute *xv_attr;
  Atom prop_atom;
  const char *channels[4] = { "XV_HUE", "XV_SATURATION",
    "XV_BRIGHTNESS", "XV_CONTRAST"
  };

  g_return_val_if_fail (config != NULL, NULL);

  context = g_slice_new0 (GstXvContext);

  gst_mini_object_init (GST_MINI_OBJECT_CAST (context), 0,
      gst_xvcontext_get_type (),
      (GstMiniObjectCopyFunction) gst_xvcontext_copy,
      (GstMiniObjectDisposeFunction) NULL,
      (GstMiniObjectFreeFunction) gst_xvcontext_free);

  g_mutex_init (&context->lock);
  context->im_format = 0;
  context->adaptor_nr = -1;

  if (!(context->disp = XOpenDisplay (config->display_name)))
    goto no_display;

  context->screen = DefaultScreenOfDisplay (context->disp);
  context->screen_num = DefaultScreen (context->disp);
  context->visual = DefaultVisual (context->disp, context->screen_num);
  context->root = DefaultRootWindow (context->disp);
  context->white = XWhitePixel (context->disp, context->screen_num);
  context->black = XBlackPixel (context->disp, context->screen_num);
  context->depth = DefaultDepthOfScreen (context->screen);

  context->width = DisplayWidth (context->disp, context->screen_num);
  context->height = DisplayHeight (context->disp, context->screen_num);
  context->widthmm = DisplayWidthMM (context->disp, context->screen_num);
  context->heightmm = DisplayHeightMM (context->disp, context->screen_num);

  GST_DEBUG ("X reports %dx%d pixels and %d mm x %d mm",
      context->width, context->height, context->widthmm, context->heightmm);

  gst_xvcontext_calculate_pixel_aspect_ratio (context);
  /* We get supported pixmap formats at supported depth */
  px_formats = XListPixmapFormats (context->disp, &nb_formats);

  if (!px_formats)
    goto no_pixel_formats;

  /* We get bpp value corresponding to our running depth */
  for (i = 0; i < nb_formats; i++) {
    if (px_formats[i].depth == context->depth)
      context->bpp = px_formats[i].bits_per_pixel;
  }

  XFree (px_formats);

  context->endianness =
      (ImageByteOrder (context->disp) ==
      LSBFirst) ? G_LITTLE_ENDIAN : G_BIG_ENDIAN;

  /* our caps system handles 24/32bpp RGB as big-endian. */
  if ((context->bpp == 24 || context->bpp == 32) &&
      context->endianness == G_LITTLE_ENDIAN) {
    context->endianness = G_BIG_ENDIAN;
    context->visual->red_mask = GUINT32_TO_BE (context->visual->red_mask);
    context->visual->green_mask = GUINT32_TO_BE (context->visual->green_mask);
    context->visual->blue_mask = GUINT32_TO_BE (context->visual->blue_mask);
    if (context->bpp == 24) {
      context->visual->red_mask >>= 8;
      context->visual->green_mask >>= 8;
      context->visual->blue_mask >>= 8;
    }
  }

  if (!(context->caps = gst_xvcontext_get_xv_support (context, config, error)))
    goto no_caps;

  /* Search for XShm extension support */
#ifdef HAVE_XSHM
  if (XShmQueryExtension (context->disp) &&
      gst_xvcontext_check_xshm_calls (context)) {
    context->use_xshm = TRUE;
    GST_DEBUG ("context is using XShm extension");
  } else
#endif /* HAVE_XSHM */
  {
    context->use_xshm = FALSE;
    GST_DEBUG ("context is not using XShm extension");
  }

  xv_attr = XvQueryPortAttributes (context->disp, context->xv_port_id, &N_attr);

  /* Generate the channels list */
  for (i = 0; i < (sizeof (channels) / sizeof (char *)); i++) {
    XvAttribute *matching_attr = NULL;

    /* Retrieve the property atom if it exists. If it doesn't exist,
     * the attribute itself must not either, so we can skip */
    prop_atom = XInternAtom (context->disp, channels[i], True);
    if (prop_atom == None)
      continue;

    if (xv_attr != NULL) {
      for (j = 0; j < N_attr && matching_attr == NULL; ++j)
        if (!g_ascii_strcasecmp (channels[i], xv_attr[j].name))
          matching_attr = xv_attr + j;
    }

    if (matching_attr) {
      GstColorBalanceChannel *channel;

      channel = g_object_new (GST_TYPE_COLOR_BALANCE_CHANNEL, NULL);
      channel->label = g_strdup (channels[i]);
      channel->min_value = matching_attr->min_value;
      channel->max_value = matching_attr->max_value;

      context->channels_list = g_list_append (context->channels_list, channel);

      /* If the colorbalance settings have not been touched we get Xv values
         as defaults and update our internal variables */
      if (!config->cb_changed) {
        gint val;

        XvGetPortAttribute (context->disp, context->xv_port_id,
            prop_atom, &val);
        /* Normalize val to [-1000, 1000] */
        val = floor (0.5 + -1000 + 2000 * (val - channel->min_value) /
            (double) (channel->max_value - channel->min_value));

        if (!g_ascii_strcasecmp (channels[i], "XV_HUE"))
          config->hue = val;
        else if (!g_ascii_strcasecmp (channels[i], "XV_SATURATION"))
          config->saturation = val;
        else if (!g_ascii_strcasecmp (channels[i], "XV_BRIGHTNESS"))
          config->brightness = val;
        else if (!g_ascii_strcasecmp (channels[i], "XV_CONTRAST"))
          config->contrast = val;
      }
    }
  }

  if (xv_attr)
    XFree (xv_attr);
#ifdef GST_EXT_XV_ENHANCEMENT
  context->xim_transparenter = NULL;
  gst_xvcontext_set_display_mode (context, config->display_mode);
  context->dsp_test=FALSE;
  context->drm_fd = -1;
#ifdef USE_FLUSH_BUFFER
  context->tbm = NULL;
  context->last_added_buffer_index = -1;
  context->temp_added_buffer_index = -1;
#endif
  for (i = 0; i < DISPLAYING_BUFFERS_MAX_NUM; i++) {
    context->displaying_buffers[i].buffer = NULL;
    for (j = 0; j < XV_BUF_PLANE_NUM; j++) {
      context->displaying_buffers[i].gem_name[j] = 0;
      context->displaying_buffers[i].gem_handle[j] = 0;
      context->displaying_buffers[i].dmabuf_fd[j] = 0;
      context->displaying_buffers[i].ref_count = 0;
    }
  }

  context->display_buffer_lock = g_mutex_new ();
  context->display_buffer_cond = g_cond_new ();

  context->displayed_buffer_count = 0;
  context->displaying_buffer_count = 0;

#endif /* GST_EXT_XV_ENHANCEMENT */
  return context;

  /* ERRORS */
no_display:
  {
    gst_xvcontext_unref (context);
    g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_WRITE,
        "Could not open display %s", config->display_name);
    return NULL;
  }
no_pixel_formats:
  {
    gst_xvcontext_unref (context);
    g_set_error (error, GST_RESOURCE_ERROR, GST_RESOURCE_ERROR_SETTINGS,
        ("Could not get pixel formats"));
    return NULL;
  }
no_caps:
  {
    gst_xvcontext_unref (context);
    return NULL;
  }
}

void
gst_xvcontext_set_synchronous (GstXvContext * context, gboolean synchronous)
{
  /* call XSynchronize with the current value of synchronous */
  GST_DEBUG ("XSynchronize called with %s", synchronous ? "TRUE" : "FALSE");
  g_mutex_lock (&context->lock);
  XSynchronize (context->disp, synchronous);
  g_mutex_unlock (&context->lock);
}

void
gst_xvcontext_update_colorbalance (GstXvContext * context,
    GstXvContextConfig * config)
{
  GList *channels = NULL;

  /* Don't set the attributes if they haven't been changed, to avoid
   * rounding errors changing the values */
  if (!config->cb_changed)
    return;

  /* For each channel of the colorbalance we calculate the correct value
     doing range conversion and then set the Xv port attribute to match our
     values. */
  channels = context->channels_list;

  while (channels) {
    if (channels->data && GST_IS_COLOR_BALANCE_CHANNEL (channels->data)) {
      GstColorBalanceChannel *channel = NULL;
      Atom prop_atom;
      gint value = 0;
      gdouble convert_coef;

      channel = GST_COLOR_BALANCE_CHANNEL (channels->data);
      g_object_ref (channel);

      /* Our range conversion coef */
      convert_coef = (channel->max_value - channel->min_value) / 2000.0;

      if (g_ascii_strcasecmp (channel->label, "XV_HUE") == 0) {
        value = config->hue;
      } else if (g_ascii_strcasecmp (channel->label, "XV_SATURATION") == 0) {
        value = config->saturation;
      } else if (g_ascii_strcasecmp (channel->label, "XV_CONTRAST") == 0) {
        value = config->contrast;
      } else if (g_ascii_strcasecmp (channel->label, "XV_BRIGHTNESS") == 0) {
        value = config->brightness;
      } else {
        g_warning ("got an unknown channel %s", channel->label);
        g_object_unref (channel);
        return;
      }

      /* Committing to Xv port */
      g_mutex_lock (&context->lock);
      prop_atom = XInternAtom (context->disp, channel->label, True);
      if (prop_atom != None) {
        int xv_value;
        xv_value =
            floor (0.5 + (value + 1000) * convert_coef + channel->min_value);
        XvSetPortAttribute (context->disp,
            context->xv_port_id, prop_atom, xv_value);
      }
      g_mutex_unlock (&context->lock);

      g_object_unref (channel);
    }
    channels = g_list_next (channels);
  }
}

/* This function tries to get a format matching with a given caps in the
   supported list of formats we generated in gst_context_get_xv_support */
gint
gst_xvcontext_get_format_from_info (GstXvContext * context, GstVideoInfo * info)
{
  GList *list = NULL;

  list = context->formats_list;

  while (list) {
    GstXvImageFormat *format = list->data;

    if (format && format->vformat == GST_VIDEO_INFO_FORMAT (info))
      return format->format;

    list = g_list_next (list);
  }
  return -1;
}

void
gst_xvcontext_set_colorimetry (GstXvContext * context,
    GstVideoColorimetry * colorimetry)
{
  Atom prop_atom;
  int xv_value;

  if (!context->have_iturbt709)
    return;

  switch (colorimetry->matrix) {
    case GST_VIDEO_COLOR_MATRIX_SMPTE240M:
    case GST_VIDEO_COLOR_MATRIX_BT709:
      xv_value = 1;
      break;
    default:
      xv_value = 0;
      break;
  }

  g_mutex_lock (&context->lock);
  prop_atom = XInternAtom (context->disp, "XV_ITURBT_709", True);
  if (prop_atom != None) {
    XvSetPortAttribute (context->disp,
        context->xv_port_id, prop_atom, xv_value);
  }
  g_mutex_unlock (&context->lock);
}

#ifdef GST_EXT_XV_ENHANCEMENT
static XImage *
make_transparent_image (Display * d, Window win, int w, int h)
{
  XImage *xim;

  /* create a normal ximage */
  xim =
      XCreateImage (d, DefaultVisualOfScreen (DefaultScreenOfDisplay (d)), 24,
      ZPixmap, 0, NULL, w, h, 32, 0);

  GST_INFO ("ximage %p", xim);

  /* allocate data for it */
  if (xim) {
    xim->data = (char *) malloc (xim->bytes_per_line * xim->height);
    if (xim->data) {
      memset (xim->data, 0x00, xim->bytes_per_line * xim->height);
      return xim;
    } else {
      GST_ERROR ("failed to alloc data - size %d",
          xim->bytes_per_line * xim->height);
    }

    XDestroyImage (xim);
  }

  GST_ERROR ("failed to create Ximage");

  return NULL;
}
#endif /* GST_EXT_XV_ENHANCEMENT */

GstXWindow *
gst_xvcontext_create_xwindow (GstXvContext * context, gint width, gint height)
{
  GstXWindow *window;
  Atom wm_delete;
  Atom hints_atom = None;
#ifdef GST_EXT_XV_ENHANCEMENT
  XSetWindowAttributes win_attr;
  XWindowAttributes root_attr;
#endif /* GST_EXT_XV_ENHANCEMENT */

  g_return_val_if_fail (GST_IS_XVCONTEXT (context), NULL);

  window = g_slice_new0 (GstXWindow);

  window->context = gst_xvcontext_ref (context);
  window->render_rect.x = window->render_rect.y = 0;
  window->render_rect.w = width;
  window->render_rect.h = height;
  window->have_render_rect = FALSE;

  window->width = width;
  window->height = height;
#ifdef GST_EXT_XV_ENHANCEMENT
  XGetWindowAttributes (context->disp, context->root, &root_attr);

  if (window->width > root_attr.width) {
    GST_INFO_OBJECT (context,
        "Width[%d] is bigger than Max width. Set Max[%d].", window->width,
        root_attr.width);
    window->render_rect.w = window->width = root_attr.width;
  }
  if (window->height > root_attr.height) {
    GST_INFO_OBJECT (context,
        "Height[%d] is bigger than Max Height. Set Max[%d].", window->height,
        root_attr.height);
    window->render_rect.h = window->height = root_attr.height;
  }
  window->internal = TRUE;

  g_mutex_lock (&context->lock);

  GST_DEBUG_OBJECT (context, "window create [%dx%d] and context->dsp_test=%d", window->width,
      window->height, context->dsp_test);

  window->win = XCreateSimpleWindow (context->disp,
      context->root, 0, 0, window->width, window->height, 0, 0, 0);

  context->xim_transparenter = make_transparent_image (context->disp,
      context->root, window->width, window->height);

  /* Make window manager not to change window size as Full screen */
  if(context->xim_transparenter && context->dsp_test)
    win_attr.override_redirect = False;
  else
    win_attr.override_redirect = True;

  XChangeWindowAttributes (context->disp, window->win, CWOverrideRedirect,
      &win_attr);
#else /* GST_EXT_XV_ENHANCEMENT */
  window->internal = TRUE;

  g_mutex_lock (&context->lock);

  window->win = XCreateSimpleWindow (context->disp,
      context->root, 0, 0, width, height, 0, 0, context->black);
#endif /* GST_EXT_XV_ENHANCEMENT */

  /* We have to do that to prevent X from redrawing the background on
   * ConfigureNotify. This takes away flickering of video when resizing. */
  XSetWindowBackgroundPixmap (context->disp, window->win, None);

  /* Tell the window manager we'd like delete client messages instead of
   * being killed */
  wm_delete = XInternAtom (context->disp, "WM_DELETE_WINDOW", True);
  if (wm_delete != None) {
    (void) XSetWMProtocols (context->disp, window->win, &wm_delete, 1);
  }

  hints_atom = XInternAtom (context->disp, "_MOTIF_WM_HINTS", True);
  if (hints_atom != None) {
    MotifWmHints *hints;

    hints = g_malloc0 (sizeof (MotifWmHints));

    hints->flags |= MWM_HINTS_DECORATIONS;
    hints->decorations = 1 << 0;

    XChangeProperty (context->disp, window->win,
        hints_atom, hints_atom, 32, PropModeReplace,
        (guchar *) hints, sizeof (MotifWmHints) / sizeof (long));

    XSync (context->disp, FALSE);

    g_free (hints);
  }

  window->gc = XCreateGC (context->disp, window->win, 0, NULL);

  XMapRaised (context->disp, window->win);

  XSync (context->disp, FALSE);

  g_mutex_unlock (&context->lock);

#ifdef GST_EXT_XV_ENHANCEMENT
  if(context->xim_transparenter && context->dsp_test)
    DRI2CreateDrawable(context->disp, window->win);
#endif

  return window;
}

GstXWindow *
gst_xvcontext_create_xwindow_from_xid (GstXvContext * context, XID xid)
{
  GstXWindow *window;
  XWindowAttributes attr;

  window = g_slice_new0 (GstXWindow);
  window->win = xid;
  window->context = gst_xvcontext_ref (context);

  /* Set the event we want to receive and create a GC */
  g_mutex_lock (&context->lock);

  XGetWindowAttributes (context->disp, window->win, &attr);

  window->width = attr.width;
  window->height = attr.height;
  window->internal = FALSE;

  window->have_render_rect = FALSE;
  window->render_rect.x = window->render_rect.y = 0;
  window->render_rect.w = attr.width;
  window->render_rect.h = attr.height;

  window->gc = XCreateGC (context->disp, window->win, 0, NULL);
  g_mutex_unlock (&context->lock);

  return window;
}

void
gst_xwindow_destroy (GstXWindow * window)
{
  GstXvContext *context;

  g_return_if_fail (window != NULL);

  context = window->context;

  g_mutex_lock (&context->lock);

  /* If we did not create that window we just free the GC and let it live */
  if (window->internal) {
    XDestroyWindow (context->disp, window->win);
#ifdef GST_EXT_XV_ENHANCEMENT
    if (context->xim_transparenter) {
      XDestroyImage (context->xim_transparenter);
      context->xim_transparenter = NULL;
    }
#endif /* GST_EXT_XV_ENHANCEMENT */
  } else
    XSelectInput (context->disp, window->win, 0);

  XFreeGC (context->disp, window->gc);

  XSync (context->disp, FALSE);

  g_mutex_unlock (&context->lock);

  gst_xvcontext_unref (context);

  g_slice_free1 (sizeof (GstXWindow), window);
}

void
gst_xwindow_set_event_handling (GstXWindow * window, gboolean handle_events)
{
  GstXvContext *context;

  g_return_if_fail (window != NULL);

  context = window->context;

  g_mutex_lock (&context->lock);
  if (handle_events) {
    if (window->internal) {
      XSelectInput (context->disp, window->win,
          ExposureMask | StructureNotifyMask | PointerMotionMask |
          KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask);
    } else {
      XSelectInput (context->disp, window->win,
          ExposureMask | StructureNotifyMask | PointerMotionMask |
          KeyPressMask | KeyReleaseMask);
    }
  } else {
    XSelectInput (context->disp, window->win, 0);
  }
  g_mutex_unlock (&context->lock);
}

void
gst_xwindow_set_title (GstXWindow * window, const gchar * title)
{
  GstXvContext *context;

  g_return_if_fail (window != NULL);

  context = window->context;

  /* we have a window */
  if (window->internal && title) {
    XTextProperty xproperty;

    if ((XStringListToTextProperty (((char **) &title), 1, &xproperty)) != 0) {
      XSetWMName (context->disp, window->win, &xproperty);
      XFree (xproperty.value);
    }
  }
}

void
gst_xwindow_update_geometry (GstXWindow * window)
{
#ifdef GST_EXT_XV_ENHANCEMENT
  Window root_window, child_window;
  XWindowAttributes root_attr;

  int cur_win_x = 0;
  int cur_win_y = 0;
  unsigned int cur_win_width = 0;
  unsigned int cur_win_height = 0;
  unsigned int cur_win_border_width = 0;
  unsigned int cur_win_depth = 0;
#else /* GST_EXT_XV_ENHANCEMENT */
  XWindowAttributes attr;
#endif /* GST_EXT_XV_ENHANCEMENT */
  GstXvContext *context;

  g_return_if_fail (window != NULL);

  context = window->context;

  /* Update the window geometry */
  g_mutex_lock (&context->lock);
#ifdef GST_EXT_XV_ENHANCEMENT
  /* Get root window and size of current window */
  XGetGeometry (context->disp, window->win, &root_window, &cur_win_x, &cur_win_y,       /* relative x, y */
      &cur_win_width, &cur_win_height, &cur_win_border_width, &cur_win_depth);

  window->width = cur_win_width;
  window->height = cur_win_height;

  /* Get absolute coordinates of current window */
  XTranslateCoordinates (context->disp, window->win, root_window, 0, 0, &cur_win_x, &cur_win_y, // relative x, y to root window == absolute x, y
      &child_window);

  /* Get size of root window == size of screen */
  XGetWindowAttributes (context->disp, root_window, &root_attr);

  if (!window->have_render_rect) {
    window->render_rect.x = window->render_rect.y = 0;
    window->render_rect.w = cur_win_width;
    window->render_rect.h = cur_win_height;
  }

  GST_LOG_OBJECT (window,
      "screen size %dx%d, current window geometry %dx%d, render_rect %d,%d,%dx%d",
      root_attr.width, root_attr.height, window->width, window->height,
      window->render_rect.x, window->render_rect.y, window->render_rect.w,
      window->render_rect.h);
#else /* GST_EXT_XV_ENHANCEMENT */
  XGetWindowAttributes (context->disp, window->win, &attr);

  window->width = attr.width;
  window->height = attr.height;

  if (!window->have_render_rect) {
    window->render_rect.x = window->render_rect.y = 0;
    window->render_rect.w = attr.width;
    window->render_rect.h = attr.height;
  }
#endif /* GST_EXT_XV_ENHANCEMENT */

  g_mutex_unlock (&context->lock);
}


void
gst_xwindow_clear (GstXWindow * window)
{
  GstXvContext *context;

  g_return_if_fail (window != NULL);

  context = window->context;

  g_mutex_lock (&context->lock);
#ifdef GST_EXT_XV_ENHANCEMENT
  GST_WARNING_OBJECT (context, "CALL XvStopVideo");
#endif /* GST_EXT_XV_ENHANCEMENT */

  XvStopVideo (context->disp, context->xv_port_id, window->win);

  XSync (context->disp, FALSE);

  g_mutex_unlock (&context->lock);
}

void
gst_xwindow_set_render_rectangle (GstXWindow * window,
    gint x, gint y, gint width, gint height)
{
  g_return_if_fail (window != NULL);

  if (width >= 0 && height >= 0) {
    window->render_rect.x = x;
    window->render_rect.y = y;
    window->render_rect.w = width;
    window->render_rect.h = height;
    window->have_render_rect = TRUE;
  } else {
    window->render_rect.x = 0;
    window->render_rect.y = 0;
    window->render_rect.w = window->width;
    window->render_rect.h = window->height;
    window->have_render_rect = FALSE;
  }
}

#ifdef GST_EXT_XV_ENHANCEMENT
#ifdef USE_FLUSH_BUFFER
static void
_release_flush_buffer (GstXvContext * context)
{
  int i = 0;

  if (context == NULL || context->flush_buffer == NULL) {
    GST_WARNING ("handle is NULL");
    return;
  }

  GST_WARNING_OBJECT (context, "release FLUSH BUFFER");

  for (i = 0; i < XV_BUF_PLANE_NUM; i++) {
    if (context->flush_buffer->bo[i]) {
      tbm_bo_unref (context->flush_buffer->bo[i]);
      context->flush_buffer->bo[i] = NULL;
    }
  }

  GST_WARNING_OBJECT (context, "release FLUSH BUFFER done");

  free (context->flush_buffer);
  context->flush_buffer = NULL;

  return;
}
#endif
void
gst_xvcontext_drm_init (GstXvContext * context)
{
  Display *dpy;
  int eventBase = 0;
  int errorBase = 0;
  int dri2Major = 0;
  int dri2Minor = 0;
  char *driverName = NULL;
  char *deviceName = NULL;
  struct drm_auth auth_arg = { 0 };

  context->drm_fd = -1;

  dpy = XOpenDisplay (0);
  if (!dpy) {
    GST_ERROR ("XOpenDisplay failed errno:%d", errno);
    return;
  }

  GST_INFO ("START");

  /* DRI2 */
  if (!DRI2QueryExtension (dpy, &eventBase, &errorBase)) {
    GST_ERROR ("DRI2QueryExtension !!");
    goto DRM_INIT_ERROR;
  }

  if (!DRI2QueryVersion (dpy, &dri2Major, &dri2Minor)) {
    GST_ERROR ("DRI2QueryVersion !!");
    goto DRM_INIT_ERROR;
  }

  if (!DRI2Connect (dpy, RootWindow (dpy, DefaultScreen (dpy)), &driverName,
          &deviceName)) {
    GST_ERROR ("DRI2Connect !!");
    goto DRM_INIT_ERROR;
  }

  if (!driverName || !deviceName) {
    GST_ERROR ("driverName or deviceName is not valid");
    goto DRM_INIT_ERROR;
  }

  GST_INFO ("Open drm device : %s", deviceName);

  /* get the drm_fd though opening the deviceName */
  context->drm_fd = open (deviceName, O_RDWR);
  if (context->drm_fd < 0) {
    GST_ERROR ("cannot open drm device (%s)", deviceName);
    goto DRM_INIT_ERROR;
  }

  /* get magic from drm to authentication */
  if (ioctl (context->drm_fd, DRM_IOCTL_GET_MAGIC, &auth_arg)) {
    GST_ERROR ("cannot get drm auth magic");
    goto DRM_INIT_ERROR;
  }

  if (!DRI2Authenticate (dpy, RootWindow (dpy, DefaultScreen (dpy)),
          auth_arg.magic)) {
    GST_ERROR ("cannot get drm authentication from X");
    goto DRM_INIT_ERROR;
  }

  XCloseDisplay (dpy);
  free (driverName);
  free (deviceName);
#ifdef USE_FLUSH_BUFFER
  context->tbm = tbm_bufmgr_init (context->drm_fd);
  if (context->tbm == NULL) {
    GST_ERROR_OBJECT (context, "tbm_bufmgr_init failed");
    goto DRM_INIT_ERROR;
  }
#endif
  GST_INFO ("DONE");

  return;

DRM_INIT_ERROR:
  if (context->drm_fd >= 0) {
    close (context->drm_fd);
    context->drm_fd = -1;
  }
  if (dpy) {
    XCloseDisplay (dpy);
  }
  if (driverName) {
    free (driverName);
  }
  if (deviceName) {
    free (deviceName);
  }

  return;
}

void
gst_xvcontext_drm_fini (GstXvContext * context)
{
  GST_INFO ("START");

  if (context->drm_fd >= 0) {
    int i;
    int j;
    gboolean is_timeout = FALSE;

    /* close remained gem handle */
    g_mutex_lock (context->display_buffer_lock);
    for (i = 0; i < DISPLAYING_BUFFERS_MAX_NUM; i++) {
      if (context->displaying_buffers[i].buffer) {
        GTimeVal abstimeout;

        GST_WARNING ("remained buffer %p, name %u %u %u, handle %u %u %u",
            context->displaying_buffers[i].buffer,
            context->displaying_buffers[i].gem_name[0],
            context->displaying_buffers[i].gem_name[1],
            context->displaying_buffers[i].gem_name[2],
            context->displaying_buffers[i].gem_handle[0],
            context->displaying_buffers[i].gem_handle[1],
            context->displaying_buffers[i].gem_handle[2]);

        g_get_current_time (&abstimeout);
        g_time_val_add (&abstimeout, _BUFFER_WAIT_TIMEOUT);

        if (is_timeout ||
            !g_cond_timed_wait (context->display_buffer_cond,
                context->display_buffer_lock, &abstimeout)) {
          GST_ERROR
              ("Buffer wait timeout[%d usec] or is_timeout[%d]. Force Unref buffer",
              _BUFFER_WAIT_TIMEOUT, is_timeout);

          /* set flag not to wait next time */
          is_timeout = TRUE;

#ifdef USE_FLUSH_BUFFER
          /*release flush buffer */
          if (context->flush_buffer) {
            if (context->flush_buffer->gem_name[0] ==
                context->displaying_buffers[i].gem_name[0]
                && context->flush_buffer->gem_name[1] ==
                context->displaying_buffers[i].gem_name[1]
                && context->flush_buffer->gem_name[2] ==
                context->displaying_buffers[i].gem_name[2]) {
              _release_flush_buffer (context);
            }
          } else {
            GST_WARNING_OBJECT (context, "Force Unref buffer");
          }
#endif
          for (j = 0; j < XV_BUF_PLANE_NUM; j++) {
            if (context->displaying_buffers[i].gem_handle[j] > 0) {
              drm_close_gem (context,
                  &(context->displaying_buffers[i].gem_handle[j]));
            }
            context->displaying_buffers[i].gem_name[j] = 0;
            context->displaying_buffers[i].dmabuf_fd[j] = 0;
            context->displaying_buffers[i].bo[j] = NULL;
          }

          gst_buffer_unref (context->displaying_buffers[i].buffer);
          context->displaying_buffers[i].buffer = NULL;
        } else {
          GST_WARNING ("Signal received. check again...");
        }

        /* init index and check again from first */
        i = -1;
      }
    }
    g_mutex_unlock (context->display_buffer_lock);
#ifdef USE_FLUSH_BUFFER
    GST_WARNING_OBJECT (context, "destroy tbm buffer manager");
    tbm_bufmgr_deinit (context->tbm);
    context->tbm = NULL;
#endif
    GST_INFO ("close drm_fd %d", context->drm_fd);
    close (context->drm_fd);
    context->drm_fd = -1;
  } else {
    GST_INFO ("DRM device is NOT opened");
  }

  /* init displaying_buffer_count */
  context->displaying_buffer_count = 0;

  GST_INFO ("DONE");
}

static unsigned int
drm_convert_dmabuf_gemname (GstXvContext * context, unsigned int dmabuf_fd,
    unsigned int *gem_handle)
{
  int ret = 0;

  struct drm_prime_handle prime_arg = { 0, };
  struct drm_gem_flink flink_arg = { 0, };

  if (!context || !gem_handle) {
    GST_ERROR ("handle[%p,%p] is NULL", context, gem_handle);
    return 0;
  }

  if (context->drm_fd <= 0) {
    GST_ERROR ("DRM is not opened");
    return 0;
  }

  if (dmabuf_fd <= 0) {
    GST_LOG ("Ignore wrong dmabuf fd [%u]", dmabuf_fd);
    return 0;
  }

  prime_arg.fd = dmabuf_fd;
  ret = ioctl (context->drm_fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime_arg);
  if (ret) {
    GST_ERROR ("DRM_IOCTL_PRIME_FD_TO_HANDLE failed. ret %d, dmabuf fd : %u",
        ret, dmabuf_fd);
    return 0;
  }

  *gem_handle = prime_arg.handle;
  flink_arg.handle = prime_arg.handle;
  ret = ioctl (context->drm_fd, DRM_IOCTL_GEM_FLINK, &flink_arg);
  if (ret) {
    GST_ERROR ("DRM_IOCTL_GEM_FLINK failed. ret %d, gem_handle %u, gem_name %u",
        ret, *gem_handle, flink_arg.name);
    return 0;
  }

  return flink_arg.name;
}

static void
drm_close_gem (GstXvContext * context, unsigned int *gem_handle)
{
  struct drm_gem_close close_arg = { 0, };

  if (context->drm_fd < 0 || !gem_handle) {
    GST_ERROR ("DRM is not opened");
    return;
  }

  if (*gem_handle <= 0) {
    GST_DEBUG ("invalid gem handle %u", *gem_handle);
    return;
  }

  GST_LOG ("Call DRM_IOCTL_GEM_CLOSE - handle %u", *gem_handle);

  close_arg.handle = *gem_handle;
  if (ioctl (context->drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg)) {
    GST_ERROR ("cannot close drm gem handle %u", gem_handle);
    return;
  }

  *gem_handle = 0;

  return;
}

#ifdef USE_FLUSH_BUFFER
gboolean
gst_xvimagesink_make_flush_buffer (GstXvContext * context)
{
  GstXvImageFlushBuffer *flush_buffer = NULL;
  GstXvImageDisplayingBuffer *display_buffer = NULL;
  tbm_bo bo = NULL;
  int size = 0;
  int i = 0;
  int ret = 0;

  if (context == NULL) {
    GST_ERROR ("handle is NULL");
    return FALSE;
  }

  if (context->last_added_buffer_index == -1) {
    GST_WARNING_OBJECT (context, "there is no remained buffer");
    return FALSE;
  }

  if (context->drm_fd < 0 || context->tbm == NULL) {
    GST_ERROR_OBJECT (context, "drm fd[%d] or bufmgr[%p] is invalid",
        context->drm_fd, context->tbm);
    return FALSE;
  }

  flush_buffer =
      (GstXvImageFlushBuffer *) malloc (sizeof (GstXvImageFlushBuffer));
  if (flush_buffer == NULL) {
    GST_ERROR_OBJECT (context, "GstXvImageFlushBuffer alloc failed");
    return FALSE;
  }

  memset (flush_buffer, 0x0, sizeof (GstXvImageFlushBuffer));

  display_buffer =
      &(context->displaying_buffers[context->last_added_buffer_index]);
  GST_ERROR ("display_buffer address is %p", display_buffer);
  GST_ERROR ("context->displaying_buffers[%d] is %p",
      context->last_added_buffer_index,
      &(context->displaying_buffers[context->last_added_buffer_index]));
  GST_WARNING_OBJECT (context, "last_added_buffer_index [%d]",
      context->last_added_buffer_index);

  GST_ERROR ("XV_BUF_PLANE_NUM is %d", XV_BUF_PLANE_NUM);

  for (i = 0; i < XV_BUF_PLANE_NUM; i++) {
    GST_ERROR ("display_buffer->bo[%d] is %p ", i, display_buffer->bo[i]);
    if (display_buffer->bo[i]) {
      tbm_bo_handle vaddr_src;
      tbm_bo_handle vaddr_dst;

      /* get bo size */
      size = tbm_bo_size (display_buffer->bo[i]);
      GST_ERROR ("size is %d", size);

      /* alloc bo */
      bo = tbm_bo_alloc (context->tbm, size, TBM_BO_DEFAULT);
      if (bo == NULL) {
        GST_ERROR_OBJECT (context, "bo alloc[%d] failed", size);
        goto FLUSH_BUFFER_FAILED;
      }

      GST_WARNING_OBJECT (context, "[%d] bo %p, size %d alloc done", i, bo,
          size);

      flush_buffer->gem_name[i] = tbm_bo_export (bo);
      flush_buffer->bo[i] = bo;

      /* get virtual address */
      vaddr_src =
          tbm_bo_map (display_buffer->bo[i], TBM_DEVICE_CPU,
          TBM_OPTION_READ | TBM_OPTION_WRITE);
      vaddr_dst =
          tbm_bo_map (bo, TBM_DEVICE_CPU, TBM_OPTION_READ | TBM_OPTION_WRITE);
      if (vaddr_src.ptr == NULL || vaddr_dst.ptr == NULL) {
        GST_WARNING_OBJECT (context, "get vaddr failed src %p, dst %p",
            vaddr_src.ptr, vaddr_dst.ptr);
        if (vaddr_src.ptr) {
          tbm_bo_unmap (display_buffer->bo[i]);
        }
        if (vaddr_dst.ptr) {
          tbm_bo_unmap (bo);
        }
        goto FLUSH_BUFFER_FAILED;
      }

      /* copy buffer */
      memcpy (vaddr_dst.ptr, vaddr_src.ptr, size);

      tbm_bo_unmap (display_buffer->bo[i]);
      tbm_bo_unmap (bo);

      GST_WARNING_OBJECT (context, "[%d] copy done", i);

      context->flush_buffer = flush_buffer;

      ret = TRUE;
    }
  }

  return ret;

FLUSH_BUFFER_FAILED:
  if (flush_buffer) {
    for (i = 0; i < XV_BUF_PLANE_NUM; i++) {
      if (flush_buffer->bo[i]) {
        tbm_bo_unref (flush_buffer->bo[i]);
        flush_buffer->bo[i] = NULL;
      }
    }
    free (flush_buffer);
    flush_buffer = NULL;
  }

  return FALSE;
}
#endif
void
gst_xvcontext_add_displaying_buffer (GstXvContext * context,
    XV_DATA_PTR img_data, GstBuffer * buffer)
{
  int i = 0;
  int j = 0;

  if (!context || !img_data) {
    GST_ERROR ("handle is NULL %p, %p", context, img_data);
    return;
  }

  /* lock display buffer mutex */
  g_mutex_lock (context->display_buffer_lock);

  /* increase displaying buffer count */
  context->displaying_buffer_count++;

  /* check duplicated */
  for (i = 0; i < DISPLAYING_BUFFERS_MAX_NUM; i++) {
    if (context->displaying_buffers[i].gem_name[0] > 0) {
      if ((img_data->dmabuf_fd[0] > 0 &&
              context->displaying_buffers[i].dmabuf_fd[0] ==
              img_data->dmabuf_fd[0]
              && context->displaying_buffers[i].dmabuf_fd[1] ==
              img_data->dmabuf_fd[1]
              && context->displaying_buffers[i].dmabuf_fd[2] ==
              img_data->dmabuf_fd[2]) || (img_data->bo[0]
              && context->displaying_buffers[i].bo[0] == img_data->bo[0]
              && context->displaying_buffers[i].bo[1] == img_data->bo[1]
              && context->displaying_buffers[i].bo[2] == img_data->bo[2])) {
        /* increase ref count */
        context->displaying_buffers[i].ref_count++;

        /* set buffer info */
        img_data->YBuf = context->displaying_buffers[i].gem_name[0];
        img_data->CbBuf = context->displaying_buffers[i].gem_name[1];
        img_data->CrBuf = context->displaying_buffers[i].gem_name[2];

        if (img_data->dmabuf_fd[0] > 0) {
          GST_WARNING ("already converted fd [%u %u %u] name [%u %u %u]",
              img_data->dmabuf_fd[0], img_data->dmabuf_fd[1],
              img_data->dmabuf_fd[2], img_data->YBuf, img_data->CbBuf,
              img_data->CrBuf);
        } else {
          GST_WARNING ("already exported bo [%p %p %p] gem name [%u %u %u]",
              img_data->bo[0], img_data->bo[1], img_data->bo[2],
              img_data->YBuf, img_data->CbBuf, img_data->CrBuf);
        }

        /* unlock display buffer mutex */
        g_mutex_unlock (context->display_buffer_lock);
        return;
      }
    }
  }

  /* store buffer temporarily */
  for (i = 0; i < DISPLAYING_BUFFERS_MAX_NUM; i++) {
    if (context->displaying_buffers[i].gem_name[0] == 0) {
      if (buffer) {
        /* increase ref count of buffer */
        gst_buffer_ref (buffer);
        context->displaying_buffers[i].buffer = buffer;
      }

      if (img_data->dmabuf_fd[0] > 0) {
        /* convert fd to name */
        img_data->YBuf =
            drm_convert_dmabuf_gemname (context, img_data->dmabuf_fd[0],
            &img_data->gem_handle[0]);
        img_data->CbBuf =
            drm_convert_dmabuf_gemname (context, img_data->dmabuf_fd[1],
            &img_data->gem_handle[1]);
        img_data->CrBuf =
            drm_convert_dmabuf_gemname (context, img_data->dmabuf_fd[2],
            &img_data->gem_handle[2]);
      } else {
        /* export bo */
        if (img_data->bo[0]) {
          img_data->YBuf = tbm_bo_export (img_data->bo[0]);
        }
        if (img_data->bo[1]) {
          img_data->CbBuf = tbm_bo_export (img_data->bo[1]);
        }
        if (img_data->bo[2]) {
          img_data->CrBuf = tbm_bo_export (img_data->bo[2]);
        }
      }

      for (j = 0; j < XV_BUF_PLANE_NUM; j++) {
        context->displaying_buffers[i].dmabuf_fd[j] = img_data->dmabuf_fd[j];
        context->displaying_buffers[i].gem_handle[j] = img_data->gem_handle[j];
        context->displaying_buffers[i].bo[j] = img_data->bo[j];
      }

      /* set buffer info */
      context->displaying_buffers[i].gem_name[0] = img_data->YBuf;
      context->displaying_buffers[i].gem_name[1] = img_data->CbBuf;
      context->displaying_buffers[i].gem_name[2] = img_data->CrBuf;

      /* set ref count */
      context->displaying_buffers[i].ref_count = 1;

      if (context->displayed_buffer_count < _CHECK_DISPLAYED_BUFFER_COUNT) {
        GST_WARNING_OBJECT (context,
            "cnt %d - add idx %d, buf %p, fd [%u %u %u], handle [%u %u %u], name [%u %u %u]",
            context->displayed_buffer_count, i,
            context->displaying_buffers[i].buffer,
            context->displaying_buffers[i].dmabuf_fd[0],
            context->displaying_buffers[i].dmabuf_fd[1],
            context->displaying_buffers[i].dmabuf_fd[2],
            context->displaying_buffers[i].gem_handle[0],
            context->displaying_buffers[i].gem_handle[1],
            context->displaying_buffers[i].gem_handle[2],
            context->displaying_buffers[i].gem_name[0],
            context->displaying_buffers[i].gem_name[1],
            context->displaying_buffers[i].gem_name[2]);
      } else {
        GST_DEBUG_OBJECT (context,
            "add idx %d, buf %p, fd [%u %u %u], handle [%u %u %u], name [%u %u %u]",
            i, context->displaying_buffers[i].buffer,
            context->displaying_buffers[i].dmabuf_fd[0],
            context->displaying_buffers[i].dmabuf_fd[1],
            context->displaying_buffers[i].dmabuf_fd[2],
            context->displaying_buffers[i].gem_handle[0],
            context->displaying_buffers[i].gem_handle[1],
            context->displaying_buffers[i].gem_handle[2],
            context->displaying_buffers[i].gem_name[0],
            context->displaying_buffers[i].gem_name[1],
            context->displaying_buffers[i].gem_name[2]);
      }
#ifdef USE_FLUSH_BUFFER
      /*set last added buffer index */
      context->temp_added_buffer_index = context->last_added_buffer_index ;
      context->last_added_buffer_index = i;
      GST_DEBUG_OBJECT (context, "last_added_buffer_index %d", i);
#endif
      /* unlock display buffer mutex */
      g_mutex_unlock (context->display_buffer_lock);

      /* get current time */
      gettimeofday (&context->request_time[i], NULL);
      return;
    }
  }

  /* decrease displaying buffer count */
  context->displaying_buffer_count--;

  /* unlock display buffer mutex */
  g_mutex_unlock (context->display_buffer_lock);

  GST_ERROR ("should not be reached here. buffer slot is FULL...");
}

void
gst_xvcontext_remove_displaying_buffer (GstXvContext * context,
    unsigned int *gem_name)
{
  int i = 0;
  int j = 0;

  if (!context || !gem_name) {
    GST_ERROR ("handle is NULL %p, %p", context, gem_name);
    return;
  }

  /* lock display buffer mutex */
  g_mutex_lock (context->display_buffer_lock);

  if (context->displaying_buffer_count == 0) {
    GST_WARNING ("there is no displaying buffer");
    /* unlock display buffer mutex */
    g_mutex_unlock (context->display_buffer_lock);
    return;
  }

  GST_DEBUG ("gem name [%u %u %u], displaying buffer count %d",
      gem_name[0], gem_name[1], gem_name[2], context->displaying_buffer_count);

  for (i = 0; i < DISPLAYING_BUFFERS_MAX_NUM; i++) {
    if (context->displaying_buffers[i].gem_name[0] == gem_name[0] &&
        context->displaying_buffers[i].gem_name[1] == gem_name[1] &&
        context->displaying_buffers[i].gem_name[2] == gem_name[2]) {
      struct timeval current_time;

      /* get current time to calculate displaying time */
      gettimeofday (&current_time, NULL);

      GST_DEBUG_OBJECT (context, "buffer return time %8d us",
          (current_time.tv_sec - context->request_time[i].tv_sec) * 1000000 +
          (current_time.tv_usec - context->request_time[i].tv_usec));

      if (context->displayed_buffer_count < _CHECK_DISPLAYED_BUFFER_COUNT) {
        context->displayed_buffer_count++;
        GST_WARNING_OBJECT (context,
            "cnt %d - remove idx %d, buf %p, handle [%u %u %u], name [%u %u %u]",
            context->displayed_buffer_count, i,
            context->displaying_buffers[i].buffer,
            context->displaying_buffers[i].gem_handle[0],
            context->displaying_buffers[i].gem_handle[1],
            context->displaying_buffers[i].gem_handle[2],
            context->displaying_buffers[i].gem_name[0],
            context->displaying_buffers[i].gem_name[1],
            context->displaying_buffers[i].gem_name[2]);
      } else {
        GST_DEBUG_OBJECT (context,
            "remove idx %d, buf %p, handle [%u %u %u], name [%u %u %u]", i,
            context->displaying_buffers[i].buffer,
            context->displaying_buffers[i].gem_handle[0],
            context->displaying_buffers[i].gem_handle[1],
            context->displaying_buffers[i].gem_handle[2],
            context->displaying_buffers[i].gem_name[0],
            context->displaying_buffers[i].gem_name[1],
            context->displaying_buffers[i].gem_name[2]);
      }

      /* decrease displaying buffer count */
      context->displaying_buffer_count--;

      /* decrease ref count */
      context->displaying_buffers[i].ref_count--;

      if (context->displaying_buffers[i].ref_count > 0) {
        GST_WARNING ("ref count not zero[%d], skip close gem handle",
            context->displaying_buffers[i].ref_count);
        break;
      }
#ifdef USE_FLUSH_BUFFER
      /* release flush buffer */
      if (context->flush_buffer) {
        if (context->flush_buffer->gem_name[0] == gem_name[0] &&
            context->flush_buffer->gem_name[1] == gem_name[1] &&
            context->flush_buffer->gem_name[2] == gem_name[2]) {
          _release_flush_buffer (context);
        }
      }
#endif
      for (j = 0; j < XV_BUF_PLANE_NUM; j++) {
        if (context->displaying_buffers[i].gem_handle[j] > 0) {
          drm_close_gem (context,
              &(context->displaying_buffers[i].gem_handle[j]));
        }
        context->displaying_buffers[i].gem_name[j] = 0;
        context->displaying_buffers[i].dmabuf_fd[j] = 0;
        context->displaying_buffers[i].bo[j] = NULL;
      }
#ifdef USE_FLUSH_BUFFER
      /* reset last_added_buffer_index */
      if (context->displaying_buffer_count < 1) {
        context->last_added_buffer_index = -1;
        context->temp_added_buffer_index = -1;
        GST_DEBUG("displaying_buffer_count %d", context->displaying_buffer_count);
      }
#endif
      if (context->displaying_buffers[i].buffer) {
        gst_buffer_unref (context->displaying_buffers[i].buffer);
        context->displaying_buffers[i].buffer = NULL;
      } else {
        GST_WARNING ("no buffer to unref");
      }
      break;
    }
  }

  /* send signal to wait display_buffer_cond */
  g_cond_signal (context->display_buffer_cond);

  /* unlock display buffer mutex */
  g_mutex_unlock (context->display_buffer_lock);
}
#endif /* GST_EXT_XV_ENHANCEMENT */
