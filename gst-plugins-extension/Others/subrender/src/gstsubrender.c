/* GStreamer
 * Copyright (C) <1999> Erik Walthinsen <omega@cse.ogi.edu>
 * Copyright (C) <2003> David Schleef <ds@schleef.org>
 * Copyright (C) <2009> Young-Ho Cha <ganadist@gmail.com>
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Nam Jeong-yoon <just.nam@samsung.com>
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
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/**
 * SECTION:element-gstsubrender
 *
 * The subrender element can render text string to image.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v fakesrc ! subrender ! fakesink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include "gstsubrender.h"
#include <string.h>
#include <gst/video/video.h>

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
# define CAIRO_ARGB_A 3
# define CAIRO_ARGB_R 2
# define CAIRO_ARGB_G 1
# define CAIRO_ARGB_B 0
#else
# define CAIRO_ARGB_A 0
# define CAIRO_ARGB_R 1
# define CAIRO_ARGB_G 2
# define CAIRO_ARGB_B 3
#endif

#define MINIMUM_OUTLINE_OFFSET 1.0

#define DEFAULT_PROP_VALIGNMENT GST_SUBRENDER_VALIGN_BASELINE
#define DEFAULT_PROP_HALIGNMENT GST_SUBRENDER_HALIGN_CENTER
#define DEFAULT_PROP_LINE_ALIGNMENT GST_SUBRENDER_LINE_ALIGN_CENTER
#define DEFAULT_PROP_XPAD       25
#define DEFAULT_PROP_YPAD       25

#define DEFAULT_RENDER_WIDTH 720
#define DEFAULT_RENDER_HEIGHT 526
#define DEFAULT_FONT_COLOR      0xffffffff
#define DEFAULT_CUSTOM_EDGE_OFFSET 2

GST_DEBUG_CATEGORY_STATIC (gst_subrender_debug_category);
#define GST_CAT_DEFAULT gst_subrender_debug_category

/* properties */
enum {
  PROP_0,
  PROP_HALIGNMENT,
  PROP_VALIGNMENT,
  PROP_LINE_ALIGNMENT,
  PROP_XPAD,
  PROP_YPAD,
  PROP_FONT_DESC,
  PROP_FONT_COLOR,
  PROP_FONT_BG_COLOR,
  PROP_EXTERNAL_WIDTH,
  PROP_EXTERNAL_HEIGHT,
  PROP_IGNORE_MARKUP,
  PROP_EDGE_MODE
};

#define GST_TYPE_SUBRENDER_VALIGN (gst_subrender_valign_get_type())
static GType
gst_subrender_valign_get_type (void)
{
  static GType subrender_valign_type = 0;
  static const GEnumValue subrender_valign[] = {
    {GST_SUBRENDER_VALIGN_BASELINE, "baseline", "baseline"},
    {GST_SUBRENDER_VALIGN_BOTTOM, "bottom", "bottom"},
    {GST_SUBRENDER_VALIGN_TOP, "top", "top"},
    {0, NULL, NULL},
  };

  if (!subrender_valign_type) {
    subrender_valign_type =
        g_enum_register_static ("GstSubrenderVAlign", subrender_valign);
  }
  return subrender_valign_type;
}

#define GST_TYPE_SUBRENDER_HALIGN (gst_subrender_halign_get_type())
static GType
gst_subrender_halign_get_type (void)
{
  static GType subrender_halign_type = 0;
  static const GEnumValue subrender_halign[] = {
    {GST_SUBRENDER_HALIGN_LEFT, "left", "left"},
    {GST_SUBRENDER_HALIGN_CENTER, "center", "center"},
    {GST_SUBRENDER_HALIGN_RIGHT, "right", "right"},
    {0, NULL, NULL},
  };

  if (!subrender_halign_type) {
    subrender_halign_type =
        g_enum_register_static ("GstSubrenderHAlign", subrender_halign);
  }
  return subrender_halign_type;
}

#define GST_TYPE_SUBRENDER_LINE_ALIGN (gst_subrender_line_align_get_type())
static GType
gst_subrender_line_align_get_type (void)
{
  static GType subrender_line_align_type = 0;
  static const GEnumValue subrender_line_align[] = {
    {GST_SUBRENDER_LINE_ALIGN_LEFT, "left", "left"},
    {GST_SUBRENDER_LINE_ALIGN_CENTER, "center", "center"},
    {GST_SUBRENDER_LINE_ALIGN_RIGHT, "right", "right"},
    {0, NULL, NULL}
  };

  if (!subrender_line_align_type) {
    subrender_line_align_type =
        g_enum_register_static ("GstSubrenderLineAlign",
        subrender_line_align);
  }
  return subrender_line_align_type;
}
#ifdef DUMP_IMG
  int util_write_rawdata (const char *file, const void *data, unsigned int size);
#endif

typedef struct _BufferInfo
{
    Display *dpy;
    Pixmap   pixmap;
    int      width;
    int      height;

    /* Dri2 */
    int         drm_fd;
    tbm_bufmgr  bufmgr;
    void       *virtual;
    DRI2Buffer *dri2_buffers;
    tbm_bo bo;
} BufferInfo;

static BufferInfo *bufinfo;
#ifdef DUMP_IMG
int cnt;
#endif

void * gst_subrender_get_buffer (Display *dpy, Pixmap pixmap, int width, int height);
void gst_subrender_free_buffer (void);
static Bool gst_subrender_init_dri2 (BufferInfo *bufinfo);
static Bool gst_subrender_set_edge (GstSubrender * subrender);

#if 0
static GstStateChangeReturn
gst_subrender_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GstSubrender *subrender;
  subrender = GST_SUBRENDER (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      GST_WARNING("NULL_TO_READY done");
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_WARNING("READY_TO_PAUSED done");
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      GST_WARNING("PAUSED_TO_PLAYING done");
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_WARNING("PAUSED_TO_READY done");
      break;
    default:
      break;
  }

  if (ret == GST_STATE_CHANGE_FAILURE)
     return ret;

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  if (ret == GST_STATE_CHANGE_FAILURE)
     return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      GST_WARNING("PLAYING_TO_PAUSED done");
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_WARNING("PAUSED_TO_READY done");
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      GST_WARNING("READY_TO_NULL done");
      break;
    default:
      break;
  }
  return ret;
}
#endif
static void gst_subrender_adjust_values_with_fontdesc (GstSubrender *
    subrender, PangoFontDescription * desc)
{
  gint font_size = pango_font_description_get_size (desc) / PANGO_SCALE;

  subrender->shadow_offset = (double) (font_size) / 13.0;
  subrender->outline_offset = (double) (font_size) / 15.0;
  if (subrender->outline_offset < MINIMUM_OUTLINE_OFFSET)
    subrender->outline_offset = MINIMUM_OUTLINE_OFFSET;
}

static void
gst_subrender_render_pangocairo (GstSubrender * subrender)
{
  cairo_t *cr;
  cairo_surface_t *surface;
  cairo_t *cr_shadow;
  cairo_surface_t *surface_shadow;
  PangoRectangle ink_rect, logical_rect;
  guint width, height;
  double a, r, g, b;

  if((subrender->external_w==0 || subrender->external_h==0) && (subrender->pixmap || bufinfo))
  {
    GST_WARNING("no width or height -> FreePixmap %p", subrender->pixmap);
    if(bufinfo)
      XFreePixmap(bufinfo->dpy, subrender->pixmap);
    subrender->pixmap = 0;
    subrender->pixmap_addr = NULL;
    gst_subrender_free_buffer();
    subrender->is_buffer = FALSE;
    subrender->skip_frame = TRUE;
    return;
  }

  subrender->skip_frame = FALSE;
  pango_layout_get_pixel_extents (subrender->layout, &ink_rect, &logical_rect);
  width = logical_rect.width + subrender->shadow_offset;
  height = logical_rect.height + logical_rect.y + subrender->shadow_offset;

  if(gst_subrender_set_edge(subrender))
    GST_LOG("set edge finish");

  if(subrender->edge_color == EDGE_COLOR_SHADOW || subrender->edge_color == EDGE_COLOR_BLACK)
    surface_shadow = cairo_image_surface_create (CAIRO_FORMAT_A8, width, height);
  else
    surface_shadow = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr_shadow = cairo_create (surface_shadow);

  /* clear shadow surface */
  cairo_set_operator (cr_shadow, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr_shadow);
  cairo_set_operator (cr_shadow, CAIRO_OPERATOR_OVER);

  /* draw shadow text */
  cairo_save (cr_shadow);
  if(subrender->edge_color == EDGE_COLOR_BLACK)
  {
    a = 1;
    r = g = b = 0;
  }
  else if (subrender->edge_color == EDGE_COLOR_WHITE)
  {
    a = r = g = b = 1;
  }
  else if(subrender->edge_color == EDGE_COLOR_SHADOW)
  {
    a = 0.6;
    r = g = b = 0;
  }
  else
  {
    GST_LOG("undefined edge color");
    a = r = g = b = 0;
  }
  cairo_set_source_rgba (cr_shadow, r, g, b, a); // if you want to draw edge by other colors, you can control in this API.
  // using internal default - offset
  if(!subrender->use_custom_offset)
    cairo_translate (cr_shadow, subrender->shadow_offset, subrender->shadow_offset);
  // using custom setting - offset
  else
    cairo_translate (cr_shadow, subrender->custom_edge_offset_x, subrender->custom_edge_offset_y);
  pango_cairo_show_layout (cr_shadow, subrender->layout);
  cairo_restore (cr_shadow);

  /* draw outline text */
  cairo_save (cr_shadow);
  if (subrender->edge_color == EDGE_COLOR_WHITE)
    cairo_set_source_rgb (cr_shadow, 1.0, 1.0, 1.0);
  else
    cairo_set_source_rgb (cr_shadow, 0.0, 0.0, 0.0);
  // using internal default - outline
  if(!subrender->custom_edge_outline)
    cairo_set_line_width (cr_shadow, subrender->outline_offset);
  // using custom setting - outline
  else
    cairo_set_line_width (cr_shadow, subrender->custom_edge_outline);
  pango_cairo_layout_path (cr_shadow, subrender->layout);
  cairo_stroke (cr_shadow);
  cairo_restore (cr_shadow);

  cairo_destroy (cr_shadow);

  subrender->text_image = g_realloc (subrender->text_image, 4 * width * height);

  if(subrender->is_changed_size && subrender->pixmap && bufinfo)
  {
    GST_INFO("FreePixmap %p", subrender->pixmap);
    XFreePixmap(bufinfo->dpy, subrender->pixmap);
    subrender->pixmap = 0;
    subrender->pixmap_addr = NULL;
    gst_subrender_free_buffer();
    subrender->is_buffer = FALSE;
  }

  if((!subrender->pixmap && !subrender->is_buffer) || subrender->is_changed_size)
  {
    /* create pixmap */
    Pixmap pixmap;

    subrender->width = subrender->external_w;
    subrender->height = subrender->external_h;

    GST_WARNING("external width / height : %d * %d", subrender->external_w, subrender->external_h);

    pixmap = XCreatePixmap (subrender->dpy, DefaultRootWindow (subrender->dpy), subrender->width, subrender->height,
      DefaultDepth (subrender->dpy, DefaultScreen (subrender->dpy)));
    subrender->pixmap_addr = gst_subrender_get_buffer(subrender->dpy, pixmap, subrender->width, subrender->height);
    subrender->pixmap = pixmap;

    GST_WARNING("pixmap id : %p, pixmap vaddr : %p", subrender->pixmap, subrender->pixmap_addr);

    subrender->is_buffer = TRUE;
    subrender->is_changed_size = FALSE;
  }

  surface = cairo_image_surface_create_for_data (subrender->text_image,
      CAIRO_FORMAT_ARGB32, width, height, width * 4);

  cr = cairo_create (surface);
  cairo_set_operator (cr, CAIRO_OPERATOR_CLEAR);
  cairo_paint (cr);
  cairo_set_operator (cr, CAIRO_OPERATOR_OVER);

#if 0
  /* set default color */
  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);

  cairo_save (cr);
  /* draw text */
  pango_cairo_show_layout (cr, subrender->layout);
  cairo_restore (cr);

  /* composite shadow with offset */
  cairo_set_operator (cr, CAIRO_OPERATOR_DEST_OVER);
  cairo_set_source_surface (cr, surface_shadow, 0.0, 0.0);
  cairo_paint (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (surface_shadow);
  cairo_surface_destroy (surface);
  subrender->image_width = width;
  subrender->image_height = height;
#endif

  /* set background color */
  if(subrender->is_nbsp) {
      a = b = r = g = 0;
  } else {
    a = (subrender->font_bg_color >> 24) & 0xff;
    r = (subrender->font_bg_color >> 16) & 0xff;
    g = (subrender->font_bg_color >> 8) & 0xff;
    b = (subrender->font_bg_color >> 0) & 0xff;
  }
  GST_DEBUG("[background color] a : %lf, r : %lf, g : %lf, b : %lf", a, r, g, b);

  cairo_save (cr);

  /* fill background */
  cairo_set_source_rgba (cr, r / 255.0, g / 255.0, b / 255.0, a / 255.0);
  cairo_rectangle (cr, 0, 0, subrender->external_w, subrender->external_h);
  cairo_fill (cr);
  /* composite shadow with offset */
  cairo_set_source_surface (cr, surface_shadow, 0.0, 0.0);
  cairo_paint (cr);

  /* set text color */
  if(subrender->is_nbsp) {
    a = b = r = g = 0;
  } else {
    a = (subrender->font_color >> 24) & 0xff;
    r = (subrender->font_color >> 16) & 0xff;
    g = (subrender->font_color >> 8) & 0xff;
    b = (subrender->font_color >> 0) & 0xff;
  }
  GST_DEBUG("[font color] a : %lf, r : %lf, g : %lf, b : %lf", a, r, g, b);

  /* fill text */
  pango_cairo_update_layout (cr, subrender->layout);
  cairo_set_source_rgba (cr, r / 255.0, g / 255.0, b / 255.0, a / 255.0);
  pango_cairo_show_layout (cr, subrender->layout);

  cairo_restore (cr);
  cairo_destroy (cr);
  cairo_surface_destroy (surface_shadow);
  cairo_surface_destroy (surface);
  subrender->image_width = width;
  subrender->image_height = height;

  if(subrender->is_nbsp)
    subrender->is_nbsp = FALSE;
}

static void
gst_subrender_check_argb (GstSubrender * subrender)
{
  GstCaps *peer_caps;
  gboolean subtitle = FALSE;
  peer_caps = gst_pad_get_allowed_caps (subrender->srcpad);
  if (G_LIKELY (peer_caps)) {
    guint i = 0, n = 0;

    n = gst_caps_get_size (peer_caps);
    GST_DEBUG_OBJECT (subrender, "peer allowed caps (%u structure(s)) are %"
        GST_PTR_FORMAT, n, peer_caps);

  guint fourcc;
    /* Check if AYUV or ARGB is first */
    for (i = 0; i < n; i++) {
      GstStructure *s = gst_caps_get_structure (peer_caps, i);
      if (gst_structure_has_name (s, "video/x-raw-rgb")) {
        if(gst_structure_get_boolean (s, "subtitle", &subtitle)) {
          subrender->use_ARGB = TRUE;
          subrender->is_subtitle_format = TRUE;
          break;
        } else if(gst_structure_has_field (s, "alpha_mask")) {
          subrender->use_ARGB = TRUE;
          break;
        }
      } else if (gst_structure_has_name (s, "video/x-raw-yuv")) {
        if (gst_structure_get_fourcc (s, "format", &fourcc) &&
            fourcc == GST_MAKE_FOURCC ('A', 'Y', 'U', 'V')) {
          subrender->use_ARGB = FALSE;
          break;
        }
      }
    }
    gst_caps_unref (peer_caps);
  }
}

GST_BOILERPLATE (GstSubrender, gst_subrender, GstElement, GST_TYPE_ELEMENT);

static void gst_subrender_set_property (GObject * object,
    guint property_id, const GValue * value, GParamSpec * pspec);
static void gst_subrender_get_property (GObject * object,
    guint property_id, GValue * value, GParamSpec * pspec);
static void gst_subrender_finalize (GObject * object);
static GstFlowReturn gst_subrender_sink_chain (GstPad *pad, GstBuffer *buffer);
static gboolean gst_subrender_src_setcaps (GstPad *pad, GstCaps *caps);
static void gst_subrender_src_fixatecaps (GstPad *pad, GstCaps *caps);
static void gst_subrender_image_to_ayuv (GstSubrender * subrender, guchar * pixbuf, int xpos, int ypos, int stride);
static void gst_subrender_image_to_argb (GstSubrender * subrender, guchar * pixbuf, int xpos, int ypos, int stride);


/* pad templates */

static GstStaticPadTemplate gst_subrender_sink_template =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("text/x-pango-markup; text/plain")
    );

static GstStaticPadTemplate gst_subrender_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-rgb, "
    "bpp = (int) 32, "
    "depth = (int) 32, "
    "endianness = (int) BIG_ENDIAN, "
    "red_mask =   (int) 0x00FF0000, "
    "green_mask = (int) 0x0000FF00, "
    "blue_mask =  (int) 0x000000FF, "
    "alpha_mask = (int) 0xFF000000, "
    "width = (int) [ 1, MAX ], "
    "height = (int) [ 1, MAX ], "
    "framerate = (fraction) [ 0, MAX ], "
    "subtitle = (boolean) true")
    );

/* class initialization */
#define DEBUG_INIT(bla) \
  GST_DEBUG_CATEGORY_INIT (gst_subrender_debug_category, "subrender", 0, \
      "debug category for subrender element");


static void
gst_subrender_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_static_pad_template (element_class,
      &gst_subrender_sink_template);
  gst_element_class_add_static_pad_template (element_class,
      &gst_subrender_src_template);

  gst_element_class_set_details_simple (element_class, "Text renderer supporting pixmap for subtitle",
      "Generic", "Render text string to images", "Nam Jeong-yoon <just.nam@samsung.com>");

  GST_DEBUG_CATEGORY_INIT (gst_subrender_debug_category, "subrender", 0, "subrender plugin");
}

static void
gst_subrender_class_init (GstSubrenderClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  PangoFontMap *fontmap;
  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = gst_subrender_finalize;
  // define virtual function pointers
  gobject_class->set_property = gst_subrender_set_property;
  gobject_class->get_property = gst_subrender_get_property;
  fontmap = pango_cairo_font_map_get_default ();
  //gobject_class->change_state = GST_DEBUG_FUNCPTR (gst_subrender_change_state);

  klass->pango_context = pango_font_map_create_context(PANGO_FONT_MAP (fontmap));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_FONT_DESC,
      g_param_spec_string ("font-desc", "font description",
          "Pango font description of font "
          "to be used for rendering. "
          "See documentation of "
          "pango_font_description_from_string"
          " for syntax.", "Tizen 42", G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_FONT_COLOR,
      g_param_spec_uint ("font-color", "font color",
          "Color to use for subtitle (big-endian ARGB)", 0, G_MAXUINT32,
          DEFAULT_FONT_COLOR, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_FONT_BG_COLOR,
      g_param_spec_uint ("font-bg-color", "font background color",
          "Color of rectangle surrounding text for subtitle (big-endian ARGB)", 0, G_MAXUINT32,
          0, G_PARAM_READWRITE | GST_PARAM_CONTROLLABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_VALIGNMENT,
      g_param_spec_enum ("valignment", "vertical alignment",
          "Vertical alignment of the text", GST_TYPE_SUBRENDER_VALIGN,
          DEFAULT_PROP_VALIGNMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_HALIGNMENT,
      g_param_spec_enum ("halignment", "horizontal alignment",
          "Horizontal alignment of the text", GST_TYPE_SUBRENDER_HALIGN,
          DEFAULT_PROP_HALIGNMENT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_XPAD,
      g_param_spec_int ("xpad", "horizontal paddding",
          "Horizontal paddding when using left/right alignment", 0, G_MAXINT,
          DEFAULT_PROP_XPAD, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_YPAD,
      g_param_spec_int ("ypad", "vertical padding",
          "Vertical padding when using top/bottom alignment", 0, G_MAXINT,
          DEFAULT_PROP_YPAD, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_LINE_ALIGNMENT,
      g_param_spec_enum ("line-alignment", "line alignment",
          "Alignment of text lines relative to each other.",
          GST_TYPE_SUBRENDER_LINE_ALIGN, DEFAULT_PROP_LINE_ALIGNMENT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_EXTERNAL_WIDTH,
     g_param_spec_int ("external-width", "external width",
          "Width of external display", 0, G_MAXINT,
          DEFAULT_RENDER_WIDTH, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_EXTERNAL_HEIGHT,
      g_param_spec_int ("external-height", "external height",
          "Height of external display", 0, G_MAXINT,
          DEFAULT_RENDER_HEIGHT, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_IGNORE_MARKUP,
      g_param_spec_boolean ("ignore-markup", "ignore markup",
          "Ignore markup tag of pango text", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property(gobject_class, PROP_EDGE_MODE,
      g_param_spec_enum("edge-mode", "Edge Mode",
        "Edge mode of subtitle",
        GST_TYPE_SUBRENDER_EDGE_MODE, GST_SUBRENDER_EDGE_NO,
        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

static void
gst_subrender_init (GstSubrender * subrender, GstSubrenderClass * subrender_class)
{
  GstPadTemplate *template;

  /* sink */
  template = gst_static_pad_template_get (&gst_subrender_sink_template);
  subrender->sinkpad = gst_pad_new_from_template (template, "sink");
  gst_object_unref (template);
  gst_pad_set_chain_function (subrender->sinkpad,
      GST_DEBUG_FUNCPTR (gst_subrender_sink_chain));
  gst_element_add_pad (GST_ELEMENT (subrender), subrender->sinkpad);

  /* source */
  template = gst_static_pad_template_get (&gst_subrender_src_template);
  subrender->srcpad = gst_pad_new_from_template (template, "src");
  gst_object_unref (template);
  gst_pad_set_fixatecaps_function (subrender->srcpad,
      GST_DEBUG_FUNCPTR (gst_subrender_src_fixatecaps));
  gst_pad_set_setcaps_function (subrender->srcpad,
      GST_DEBUG_FUNCPTR (gst_subrender_src_setcaps));

  gst_element_add_pad (GST_ELEMENT (subrender), subrender->srcpad);

  subrender->line_align = DEFAULT_PROP_LINE_ALIGNMENT;
  subrender->layout =
      pango_layout_new (GST_SUBRENDER_GET_CLASS (subrender)->pango_context);
  pango_layout_set_alignment (subrender->layout,
      (PangoAlignment) subrender->line_align);

  subrender->halign = DEFAULT_PROP_HALIGNMENT;
  subrender->valign = DEFAULT_PROP_VALIGNMENT;
  subrender->xpad = DEFAULT_PROP_XPAD;
  subrender->ypad = DEFAULT_PROP_YPAD;

  subrender->width = DEFAULT_RENDER_WIDTH;
  subrender->height = DEFAULT_RENDER_HEIGHT;

  subrender->text_image = NULL;

  subrender->is_subtitle_format = FALSE;
  subrender->is_buffer = FALSE;

  subrender->dpy = NULL;
  subrender->pixmap = 0;
  subrender->pixmap_addr = NULL;

  subrender->external_w = DEFAULT_RENDER_WIDTH;
  subrender->external_h = DEFAULT_RENDER_HEIGHT;
  subrender->is_changed_size = FALSE;
  subrender->is_nbsp = FALSE;
  subrender->skip_frame = FALSE;

  bufinfo = NULL;
#ifdef DUMP_IMG
  cnt = 0;
#endif
  subrender->font_color = DEFAULT_FONT_COLOR;
  subrender->font_bg_color = 0;
  subrender->ignore_markup = FALSE;
  subrender->edge_mode = 0;
  subrender->custom_edge_outline = 0;
  subrender->use_custom_offset = FALSE;
  subrender->custom_edge_offset_x = 0;
  subrender->custom_edge_offset_y = 0;
  subrender->edge_color = EDGE_COLOR_BLACK;

  subrender->dpy = XOpenDisplay (NULL);
  exit_if_fail (subrender->dpy != NULL);
  GST_WARNING("XOpenDisplay -> dpy [%p]", subrender->dpy);

}

void
gst_subrender_set_property (GObject * object, guint property_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSubrender *subrender = GST_SUBRENDER (object);

  switch (property_id) {
    case PROP_VALIGNMENT:
      subrender->valign = g_value_get_enum (value);
      break;
    case PROP_HALIGNMENT:
      subrender->halign = g_value_get_enum (value);
      break;
    case PROP_LINE_ALIGNMENT:
      subrender->line_align = g_value_get_enum (value);
      pango_layout_set_alignment (subrender->layout,
          (PangoAlignment) subrender->line_align);
      break;
    case PROP_XPAD:
      subrender->xpad = g_value_get_int (value);
      break;
    case PROP_YPAD:
      subrender->ypad = g_value_get_int (value);
      break;
    case PROP_FONT_DESC:
    {
      PangoFontDescription *desc;

      desc = pango_font_description_from_string (g_value_get_string (value));
      //ex) pango_font_description_from_string("Sans Bold 12");
      //it contains family-name, style, size in an orderly manner
      if (desc) {
        GST_WARNING ("font description set: %s", g_value_get_string (value));
        GST_OBJECT_LOCK (subrender);
        pango_layout_set_font_description (subrender->layout, desc);
        gst_subrender_adjust_values_with_fontdesc (subrender, desc);
        pango_font_description_free (desc);
        gst_subrender_render_pangocairo (subrender);
        GST_OBJECT_UNLOCK (subrender);
      } else {
        GST_WARNING ("font description parse failed: %s",
        g_value_get_string (value));
      }
      break;
    }
    case PROP_FONT_COLOR :
    {
      subrender->font_color = g_value_get_uint (value);
      break;
    }
    case PROP_FONT_BG_COLOR :
    {
      subrender->font_bg_color = g_value_get_uint (value);
      break;
    }
    case PROP_EXTERNAL_WIDTH:
    {
      if(subrender->external_w != g_value_get_int (value))
      {
        subrender->external_w = g_value_get_int (value);
        subrender->is_changed_size = TRUE;
        GST_WARNING("[set property] subrender->external_w : %d", subrender->external_w);
      }
      break;
    }
    case PROP_EXTERNAL_HEIGHT:
    {
      if(subrender->external_h != g_value_get_int (value))
      {
        subrender->external_h = g_value_get_int (value);
        subrender->is_changed_size = TRUE;
        GST_WARNING("[set property] subrender->external_h : %d", subrender->external_h);
      }
      break;
    }
    case PROP_IGNORE_MARKUP:
    {
      subrender->ignore_markup = g_value_get_boolean (value);
      GST_WARNING("[set property] subrender->ignore_markup : %d", subrender->ignore_markup);
      break;
    }
    case PROP_EDGE_MODE:
    {
      subrender->edge_mode = g_value_get_enum(value);
      GST_WARNING("[set property] subrender->edge_mode : %d", subrender->edge_mode);
      break;
    }
    default:
      break;
  }
}

void
gst_subrender_get_property (GObject * object, guint property_id,
    GValue * value, GParamSpec * pspec)
{
  GstSubrender *subrender = GST_SUBRENDER (object);

  switch (property_id) {
    case PROP_VALIGNMENT:
      g_value_set_enum (value, subrender->valign);
      break;
    case PROP_HALIGNMENT:
      g_value_set_enum (value, subrender->halign);
      break;
    case PROP_LINE_ALIGNMENT:
      g_value_set_enum (value, subrender->line_align);
      break;
    case PROP_XPAD:
      g_value_set_int (value, subrender->xpad);
      break;
    case PROP_YPAD:
      g_value_set_int (value, subrender->ypad);
      break;
    case PROP_FONT_COLOR :
      g_value_set_uint (value, subrender->font_color);
      break;
    case PROP_FONT_BG_COLOR :
      g_value_set_uint (value, subrender->font_bg_color);
      break;
    case PROP_EXTERNAL_WIDTH:
      g_value_set_int (value, subrender->external_w);
      break;
    case PROP_EXTERNAL_HEIGHT:
      g_value_set_int (value, subrender->external_h);
      break;
    case PROP_IGNORE_MARKUP:
      g_value_set_boolean (value, subrender->ignore_markup);
      break;
    case PROP_EDGE_MODE:
      g_value_set_enum (value, subrender->edge_mode);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
gst_subrender_finalize (GObject * object)
{
  GstSubrender *subrender = GST_SUBRENDER (object);

  g_return_if_fail (GST_IS_SUBRENDER (subrender));
  /* clean up object here */
  g_free (subrender->text_image);

  if (subrender->layout)
    g_object_unref (subrender->layout);
  if (subrender->pixmap && bufinfo) {
    XFreePixmap(bufinfo->dpy, subrender->pixmap);
    GST_WARNING("FreePixmap %p", subrender->pixmap);
  }
  subrender->pixmap = 0;
  subrender->pixmap_addr = NULL;
  subrender->is_buffer = FALSE;
  subrender->is_subtitle_format = FALSE;
  subrender->skip_frame = FALSE;
  if (bufinfo)
    gst_subrender_free_buffer ();
  if(subrender->dpy)
  {
    GST_WARNING("XCloseDisplay -> dpy [%p]", subrender->dpy);
    XCloseDisplay(subrender->dpy);
  }
  subrender->dpy = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static GstFlowReturn
gst_subrender_sink_chain (GstPad *pad, GstBuffer *buffer)
{
  GstSubrender *subrender;
  subrender = GST_SUBRENDER (gst_pad_get_parent (pad));

  GstFlowReturn ret;
  GstBuffer *outbuf;
  GstCaps *caps = NULL, *padcaps, *peercaps;
  guint8 *data = GST_BUFFER_DATA (buffer);
  guint size = GST_BUFFER_SIZE (buffer);
  gint n;
  gint xpos, ypos;

  /* drop &nbsp; */
  if (size == 2 &&
      data[0] == 194 &&
      data[1] == 160 )
  {
    GST_INFO("it is non-breaking space..");
    subrender->is_nbsp = TRUE;
  }

  /* somehow pango barfs over "\0" buffers... */
  while (size > 0 &&
      (data[size - 1] == '\r' ||
          data[size - 1] == '\n' || data[size - 1] == '\0')) {
    size--;
  }
  /* render text */
  GST_DEBUG ("rendering '%*s'", size, data);
  pango_layout_set_markup (subrender->layout, (gchar *) data, size);

  if(subrender->ignore_markup)
    pango_layout_set_attributes(subrender->layout, NULL);
  gst_subrender_render_pangocairo (subrender);

  gst_subrender_check_argb (subrender);

  // negotiate with downstream
  peercaps = gst_pad_peer_get_caps (subrender->srcpad);
  padcaps = gst_pad_get_caps (subrender->srcpad);
  caps = gst_caps_intersect (padcaps, peercaps);
  gst_caps_unref (padcaps);
  gst_caps_unref (peercaps);

  if (!caps || gst_caps_is_empty (caps)) {
    GST_ELEMENT_ERROR (subrender, CORE, NEGOTIATION, (NULL), (NULL));
    ret = GST_FLOW_ERROR;
    goto done;
  }
  gst_caps_truncate (caps);
  gst_pad_fixate_caps (subrender->srcpad, caps);

  if (!gst_pad_set_caps (subrender->srcpad, caps)) {
    GST_ELEMENT_ERROR (subrender, CORE, NEGOTIATION, (NULL), (NULL));
    ret = GST_FLOW_ERROR;
    goto done;
  }
  GST_DEBUG ("Allocating buffer WxH = %dx%d", subrender->width, subrender->height);

  if(subrender->skip_frame) {
    GST_ERROR("no width or height -> skip pad push");
    ret = GST_FLOW_OK;
    goto done;
  }

  if (subrender->is_subtitle_format) {
    outbuf = gst_buffer_new_and_alloc (sizeof(int));
    gst_buffer_set_caps(outbuf, caps);
    ret = GST_FLOW_OK;  // warning : value of ret is setted by hand. It needs another API.
  } else {
    ret =
      gst_pad_alloc_buffer_and_set_caps (subrender->srcpad, GST_BUFFER_OFFSET_NONE,
      subrender->width * subrender->height * 4, caps, &outbuf);
  }
  if (subrender->is_subtitle_format) {
    GST_BUFFER_DATA (outbuf) = (void *) subrender->pixmap;
  }
  if (ret != GST_FLOW_OK)
    goto done;
  gst_buffer_copy_metadata (outbuf, buffer, GST_BUFFER_COPY_TIMESTAMPS);

  // initialize the background for ARGB
  if (subrender->use_ARGB) {
    if (subrender->is_subtitle_format) {
      data =  subrender->pixmap_addr;
      GST_DEBUG ("connected with virtual address : %p", subrender->pixmap_addr);
    }
    memset (data, 0, subrender->width * subrender->height * 4);
  } else {
    for (n = 0; n < subrender->width * subrender->height; n++) {
        data[n * 4] = data[n * 4 + 1] = 0;
        data[n * 4 + 2] = data[n * 4 + 3] = 128;
    }
  }

  switch (subrender->halign) {
    case GST_SUBRENDER_HALIGN_LEFT:
      xpos = subrender->xpad;
      break;
    case GST_SUBRENDER_HALIGN_CENTER:
      xpos = (subrender->width - subrender->image_width) / 2;
      break;
    case GST_SUBRENDER_HALIGN_RIGHT:
      xpos = subrender->width - subrender->image_width - subrender->xpad;
      break;
    default:
      xpos = 0;
  }

  switch (subrender->valign) {
    case GST_SUBRENDER_VALIGN_BOTTOM:
      ypos = subrender->height - subrender->image_height - subrender->ypad;
      break;
    case GST_SUBRENDER_VALIGN_BASELINE:
      ypos = subrender->height - (subrender->image_height + subrender->ypad);
      break;
    case GST_SUBRENDER_VALIGN_TOP:
      ypos = subrender->ypad;
      break;
    default:
      ypos = subrender->ypad;
      break;
  }

  if (subrender->text_image) {
    if (subrender->use_ARGB) {
      int offset =  (bufinfo->dri2_buffers->pitch / bufinfo->dri2_buffers->cpp) - subrender->width;
      gst_subrender_image_to_argb (subrender, data, xpos, ypos, (subrender->width + offset) * 4);
    } else {
      gst_subrender_image_to_ayuv (subrender, data, xpos, ypos, subrender->width * 4);
    }
  }
#ifdef DUMP_IMG
  int ret2 = 0;
  char file_name[128];
  char *dump_data;
  GST_INFO ("DUMP IMG_%3.3d : buffer size(%d)", cnt, size);
  sprintf(file_name, "DUMP_IMG_%3.3d.dump", cnt++);

  dump_data = g_malloc(subrender->width * subrender->height * 4);
  memcpy (dump_data, subrender->pixmap_addr, subrender->width * subrender->height * 4);


  ret2 = util_write_rawdata(file_name, dump_data, subrender->width * subrender->height * 4);
  if (ret2) {
    GST_ERROR_OBJECT (subrender, "util_write_rawdata() failed");
  }
  g_free(dump_data);
#endif
  GST_DEBUG_OBJECT(subrender, "pushing pixmap id : %p", GST_BUFFER_DATA (outbuf));
  ret = gst_pad_push (subrender->srcpad, outbuf);

done:
  if (caps)
    gst_caps_unref (caps);
  gst_buffer_unref (buffer);
  gst_object_unref (subrender);

  return ret;

}

static Bool
gst_subrender_set_edge (GstSubrender * subrender)
{
  switch (subrender->edge_mode)
  {
    case GST_SUBRENDER_EDGE_NO:
      subrender->custom_edge_outline = (subrender->outline_offset/2);
      subrender->use_custom_offset = TRUE;
      subrender->custom_edge_offset_x = 1;
      subrender->custom_edge_offset_y = 1;
      subrender->edge_color = EDGE_COLOR_BLACK;
      break;
    case GST_SUBRENDER_EDGE_RAISED:
      subrender->custom_edge_outline = 0;
      subrender->use_custom_offset = TRUE;
      subrender->custom_edge_offset_x = -DEFAULT_CUSTOM_EDGE_OFFSET;
      subrender->custom_edge_offset_y = -DEFAULT_CUSTOM_EDGE_OFFSET;
      subrender->edge_color = EDGE_COLOR_WHITE;
      break;
    case GST_SUBRENDER_EDGE_DEPRESSED:
      subrender->custom_edge_outline = 0;
      subrender->use_custom_offset = TRUE;
      subrender->custom_edge_offset_x = DEFAULT_CUSTOM_EDGE_OFFSET;
      subrender->custom_edge_offset_y = DEFAULT_CUSTOM_EDGE_OFFSET;
      subrender->edge_color = EDGE_COLOR_WHITE;
      break;
    case GST_SUBRENDER_EDGE_UNIFORM:
      subrender->custom_edge_outline = (subrender->outline_offset*1.5);
      subrender->use_custom_offset = TRUE;
      subrender->custom_edge_offset_x = subrender->custom_edge_offset_y = 0;
      subrender->edge_color = EDGE_COLOR_WHITE;
      break;
    case GST_SUBRENDER_EDGE_DROPSHADOW:
      subrender->custom_edge_outline = (subrender->outline_offset/2);
      subrender->use_custom_offset = FALSE;
      subrender->edge_color = EDGE_COLOR_SHADOW;
      break;
    default:
      goto EXIT;
      break;
  }

  return TRUE;

  EXIT:
    GST_WARNING("unsupported edge mode");
    return FALSE;
}

static gboolean
gst_subrender_src_setcaps (GstPad *pad, GstCaps *caps)
{
  GstSubrender *subrender;

  subrender = GST_SUBRENDER (gst_pad_get_parent (pad));

  GST_DEBUG_OBJECT(subrender, "setcaps");
  gst_object_unref (subrender);
  return TRUE;
}

static void
gst_subrender_src_fixatecaps (GstPad *pad, GstCaps *caps)
{
  GstSubrender *subrender;
  subrender = GST_SUBRENDER (gst_pad_get_parent (pad));

  GstStructure *structure = gst_caps_get_structure (caps, 0);

  GST_DEBUG ("Fixating caps %" GST_PTR_FORMAT, caps);
  gst_structure_fixate_field_nearest_int (structure, "width", MAX (subrender->external_w,
          DEFAULT_RENDER_WIDTH));
  gst_structure_fixate_field_nearest_int (structure, "height",
      MAX (subrender->external_h, DEFAULT_RENDER_HEIGHT));
  GST_DEBUG ("Fixated to    %" GST_PTR_FORMAT, caps);

  gst_object_unref (subrender);
}

#define CAIRO_UNPREMULTIPLY(a,r,g,b) G_STMT_START { \
  b = (a > 0) ? MIN ((b * 255 + a / 2) / a, 255) : 0; \
  g = (a > 0) ? MIN ((g * 255 + a / 2) / a, 255) : 0; \
  r = (a > 0) ? MIN ((r * 255 + a / 2) / a, 255) : 0; \
} G_STMT_END

static void
gst_subrender_image_to_ayuv (GstSubrender * subrender, guchar * pixbuf,
    int xpos, int ypos, int stride)
{
  int y;                        /* text bitmap coordinates */
  guchar *p, *bitp;
  guchar a, r, g, b;
  int width, height;

  width = subrender->image_width;
  height = subrender->image_height;

  for (y = 0; y < height && ypos + y < subrender->height; y++) {
    int n;
    p = pixbuf + (ypos + y) * stride + xpos * 4;
    bitp = subrender->text_image + y * width * 4;
    for (n = 0; n < width && n < subrender->width; n++) {
      b = bitp[CAIRO_ARGB_B];
      g = bitp[CAIRO_ARGB_G];
      r = bitp[CAIRO_ARGB_R];
      a = bitp[CAIRO_ARGB_A];
      bitp += 4;

      /* Cairo uses pre-multiplied ARGB, unpremultiply it */
      CAIRO_UNPREMULTIPLY (a, r, g, b);

      *p++ = a;
      *p++ = CLAMP ((int) (((19595 * r) >> 16) + ((38470 * g) >> 16) +
              ((7471 * b) >> 16)), 0, 255);
      *p++ = CLAMP ((int) (-((11059 * r) >> 16) - ((21709 * g) >> 16) +
              ((32768 * b) >> 16) + 128), 0, 255);
      *p++ = CLAMP ((int) (((32768 * r) >> 16) - ((27439 * g) >> 16) -
              ((5329 * b) >> 16) + 128), 0, 255);
    }
  }
}

static void
gst_subrender_image_to_argb (GstSubrender * subrender, guchar * pixbuf,
    int xpos, int ypos, int stride)
{
  int i, j;
  guchar *p, *bitp;
  int width, height;

  width = subrender->image_width;
  height = subrender->image_height;

  for (i = 0; i < height && ypos + i < subrender->height; i++) {
    p = pixbuf + (ypos + i) * stride + xpos * 4;
    bitp = subrender->text_image + i * width * 4;
    for (j = 0; j < width && j < subrender->width; j++) {

      /* we draw the image to ARGB format, but xvimagesink use BGRA format
        so we will copy data reversely in buffer */
      p[0] = bitp[CAIRO_ARGB_B];
      p[1] = bitp[CAIRO_ARGB_G];
      p[2] = bitp[CAIRO_ARGB_R];
      p[3] = bitp[CAIRO_ARGB_A];

      /* Cairo don't use pre-multiplied ARGB, so don't need unpremultiply it */
      //CAIRO_UNPREMULTIPLY (p[0], p[1], p[2], p[3]);

      bitp += 4;    //move to next address
      p += 4;       //move to next address
    }
  }
}

static Bool gst_subrender_init_dri2 (BufferInfo *bufinfo)
{
  int screen;
  int dri2_base = 0;
  int dri2_err_base = 0;
  int dri2Major, dri2Minor;
  char *driverName = NULL, *deviceName = NULL;
  unsigned int attachments[1];
  int dri2_count, dri2_out_count;
  int dri2_width, dri2_height;
  drm_magic_t magic;
  tbm_bo_handle bo_handle;
  screen = DefaultScreen(bufinfo->dpy);
  if (!DRI2QueryExtension (bufinfo->dpy, &dri2_base, &dri2_err_base))
  {
    GST_ERROR ("no DRI2 extension. !!\n");
    goto fail_init_dri2;
  }
  if (!DRI2QueryVersion (bufinfo->dpy, &dri2Major, &dri2Minor))
  {
    GST_ERROR ("fail : DRI2QueryVersion !!\n");
    goto fail_init_dri2;
  }
  if (!DRI2Connect (bufinfo->dpy, RootWindow(bufinfo->dpy, screen), &driverName, &deviceName))
  {
    GST_ERROR ("fail : DRI2Connect !!\n");
    goto fail_init_dri2;
  }    /* drm_fd */
  bufinfo->drm_fd = open (deviceName, O_RDWR);
  if (bufinfo->drm_fd < 0)
  {
    GST_ERROR ("fail : open drm device (%s)\n", deviceName);
    goto fail_init_dri2;
  }
  /* get the drm magic */
  drmGetMagic(bufinfo->drm_fd, &magic);
  if (!DRI2Authenticate(bufinfo->dpy, RootWindow(bufinfo->dpy, screen), magic))
  {
    GST_ERROR ("fail : DRI2Authenticate (%d)\n", magic);
    goto fail_init_dri2;
  }
  /* bufmgr */
  bufinfo->bufmgr = tbm_bufmgr_init (bufinfo->drm_fd);
  if (!bufinfo->bufmgr)
  {
    GST_ERROR ("fail : init buffer manager \n");
    goto fail_init_dri2;
  }
  DRI2CreateDrawable (bufinfo->dpy, bufinfo->pixmap);
  attachments[0] = DRI2BufferFrontLeft;
  dri2_count = 1;
  bufinfo->dri2_buffers = DRI2GetBuffers (bufinfo->dpy, bufinfo->pixmap, &dri2_width, &dri2_height,
                                       attachments, dri2_count, &dri2_out_count);
  if (!bufinfo->dri2_buffers)
  {
    GST_ERROR ("fail : get buffers\n");
    goto fail_init_dri2;
  }
  if (!bufinfo->dri2_buffers[0].name)
  {
    GST_ERROR ("fail : a handle of the dri2 buffer is null \n ");
    goto fail_init_dri2;
  }
  bufinfo->bo = tbm_bo_import (bufinfo->bufmgr, bufinfo->dri2_buffers[0].name);
  if (!bufinfo->bo)
  {
    GST_ERROR ("fail : import bo (key:%d)\n", bufinfo->dri2_buffers[0].name);
    goto fail_init_dri2;
  }
  /* virtual */
  bo_handle = tbm_bo_map(bufinfo->bo, TBM_DEVICE_CPU, TBM_OPTION_WRITE);
  if(!bo_handle.ptr)
  {
    GST_ERROR ("fail : tbm_bo_map");
    goto fail_init_dri2;
  }
  tbm_bo_unmap (bufinfo->bo);

  bufinfo->virtual = (void *)bo_handle.ptr;
  if (!bufinfo->virtual)
  {
    GST_ERROR ("fail : map \n");
    goto fail_init_dri2;
  }

  free (driverName);
  free (deviceName);

  return True;

  fail_init_dri2:
  if (driverName)
    free (driverName);
  if (deviceName)
    free (deviceName);
  if (bufinfo->bo)
    tbm_bo_unref(bufinfo->bo);
  if (bufinfo->dri2_buffers)
    free(bufinfo->dri2_buffers);
  if (bufinfo->bufmgr)
    tbm_bufmgr_deinit (bufinfo->bufmgr);
  if (bufinfo->drm_fd >= 0)
    close (bufinfo->drm_fd);
  return False;
}

void *
gst_subrender_get_buffer (Display *dpy, Pixmap pixmap, int width, int height)
{
  exit_if_fail (bufinfo == NULL);

  bufinfo = calloc (1, sizeof (BufferInfo));

  bufinfo->dpy = dpy;
  bufinfo->pixmap = pixmap;
  bufinfo->width = width;
  bufinfo->height = height;

  if (!gst_subrender_init_dri2 (bufinfo))
  {
    free (bufinfo);
    bufinfo = NULL;
    return NULL;
  }
  exit_if_fail (bufinfo->virtual != NULL);

  return bufinfo->virtual;
}

void
gst_subrender_free_buffer (void)
{
  exit_if_fail (bufinfo != NULL);

  if (bufinfo)
  {
    if (bufinfo->bo)
      tbm_bo_unref(bufinfo->bo);
    if (bufinfo->dri2_buffers)
      free(bufinfo->dri2_buffers);
    if (bufinfo->bufmgr)
      tbm_bufmgr_deinit (bufinfo->bufmgr);
    if (bufinfo->drm_fd >= 0)
      close (bufinfo->drm_fd);
  }

  XSync (bufinfo->dpy, 0);
  free (bufinfo);
  bufinfo = NULL;
}


#ifdef DUMP_IMG
int util_write_rawdata(const char *file, const void *data, unsigned int size)
{
  FILE *fp;

  fp = fopen(file, "wb");
  if (fp == NULL)
    return -1;
  fwrite((char*)data, sizeof(char), size, fp);
  fclose(fp);

  return 0;
}
#endif

static gboolean
plugin_init (GstPlugin * plugin)
{

  return gst_element_register (plugin, "subrender", GST_RANK_NONE,
      GST_TYPE_SUBRENDER);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "subrender",
    "textrender plugin",
    plugin_init, "1.0", "LGPL", "Samsung Electronics Co., Ltd.", "http://www.samsung.com/")
