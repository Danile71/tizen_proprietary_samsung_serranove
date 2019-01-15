/*
 * evastextoverlay
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jawahir Abul <jawahir.abul@samsung.com>,
 *                Manoj Kumar K <manojkumar.k@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

/**
 * SECTION:element-evastextoverlay
 *
 * FIXME:Describe evastextoverlay here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m videotestsrc ! evastextoverlay ! fakesink silent=TRUE
 * ]|
 * </refsect2>
 */
#include <Evas.h>
#include <Evas_Engine_Buffer.h>
#include <stdio.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>

#include "gstevastextoverlay.h"

#define DEFAULT_PROP_XPOS         110
#define DEFAULT_PROP_YPOS         110
#define DEFAULT_PROP_FONT         "DejaVu"
#define DEFAULT_RENDER_WIDTH      800
#define DEFAULT_RENDER_HEIGHT     480
#define DEFAULT_PROP_COLOR        0xffffffff
#define DEFAULT_PROP_FONT_SIZE    30

#define DEFAULT_PROP_TEXT   ""
#define GST_EVASTEXTOVERLAY_CAPS         \
  "video/x-raw, "                        \
  "format =(string) ARGB, "              \
  "width  =(int)  [16, 4096], "          \
  "height =(int)  [16, 4096], "          \
  "framerate = (fraction) [0/1,4096/1];"


/* Filter signals and args */
enum
{
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_TEXT,
  PROP_SCILENTPROP_0,
  PROP_HALIGNMENT,
  PROP_VALIGNMENT,
  PROP_LINE_ALIGNMENT,
  PROP_XPOS,
  PROP_YPOS,
  PROP_FONT,
  PROP_FONT_SIZE,
  PROP_FONT_STYLE,
  PROP_BUFFER_DEPTH,
  PROP_COLOR,
  PROP_ENABLE_OVERLAY
};

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_EVASTEXTOVERLAY_CAPS)
    );

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (GST_EVASTEXTOVERLAY_CAPS)
    );

GST_DEBUG_CATEGORY_STATIC(gst_evastextoverlay_debug);
#define GST_CAT_DEFAULT gst_evastextoverlay_debug

static void _do_init(GType type)
{
 GST_DEBUG_CATEGORY_INIT(gst_evastextoverlay_debug, "evastextoverlay", 0, "Evas Text Overlay plugin");
}
#define gst_evastextoverlay_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(Gstevastextoverlay, gst_evastextoverlay, GST_TYPE_ELEMENT, _do_init(G_TYPE_INVALID));

static void                 gst_evastextoverlay_set_property (GObject * object, guint prop_id,const GValue * value, GParamSpec * pspec);
static void                 gst_evastextoverlay_get_property (GObject * object, guint prop_id,GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_evastextoverlay_change_state (GstElement * element, GstStateChange transition);
static void                 gst_evastextoverlay_finalize (GObject * object);
static gboolean             gst_evastextoverlay_set_caps (GstPad *pad, GstCaps *caps);
static GstFlowReturn        gst_evastextoverlay_chain(GstPad *pad, GstObject *parent, GstBuffer *buf);
static gboolean             gst_evastextoverlay_event(GstPad *pad, GstObject *parent, GstEvent *event);

static Evas *create_canvas(Gstevastextoverlay *overlay);
static void draw_scene(Evas *canvas);
static void destroy_canvas(Evas *canvas);
/* GObject vmethod implementations */

/* initialize the evastextoverlay's class */
static void
gst_evastextoverlay_class_init (GstevastextoverlayClass * klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);

  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_evastextoverlay_finalize);
  gobject_class->set_property = gst_evastextoverlay_set_property;
  gobject_class->get_property = gst_evastextoverlay_get_property;

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_evastextoverlay_change_state);

  gst_element_class_set_static_metadata(gstelement_class, "Evas text overlay plugin",
                                              "Utility/Video",
                                              "Utility for overlaying text on video frames",
                                              "Samsung Electronics <www.samsung.com>");
  gst_element_class_add_pad_template (gstelement_class,gst_static_pad_template_get (&src_template));
  gst_element_class_add_pad_template (gstelement_class,gst_static_pad_template_get (&sink_template));

  g_object_class_install_property (gobject_class, PROP_FONT,
      g_param_spec_string ("font", "font name","Evas Canvas font name to be used for rendering. eg.DejaVu, courier, utopia evas for syntax.",
      DEFAULT_PROP_FONT, G_PARAM_WRITABLE ));

  g_object_class_install_property (gobject_class, PROP_XPOS,
      g_param_spec_int ("xpos", "horizontal position","Horizontal position when using position alignment", 0, 4096, DEFAULT_PROP_XPOS,
      G_PARAM_READWRITE ));

  g_object_class_install_property (gobject_class, PROP_YPOS,
      g_param_spec_int ("ypos", "vertical position","Vertical position when using position alignment", 0, 4096, DEFAULT_PROP_YPOS,
      G_PARAM_READWRITE ));

  g_object_class_install_property (gobject_class, PROP_TEXT,
      g_param_spec_string ("text", "text","Text to be display.", DEFAULT_PROP_TEXT,
      G_PARAM_READWRITE ));

  g_object_class_install_property (gobject_class, PROP_FONT_SIZE,
      g_param_spec_int("size", "font size","Font size of the text to be display.", 0, 200, DEFAULT_PROP_FONT_SIZE,
      G_PARAM_READWRITE ));

  g_object_class_install_property (gobject_class, PROP_FONT_STYLE,
      g_param_spec_int ("style", "font style","Font style of the text to be display.eg, 0 -> EVAS_TEXT_STYLE_PLAIN refer enum _Evas_Text_Style_Type", 0, 112, 0,
      G_PARAM_READWRITE ));

  g_object_class_install_property (gobject_class, PROP_COLOR,
      g_param_spec_uint("color", "font color","Font color of the text to be display eg: ARGB 65280(ffff0000).", 0, DEFAULT_PROP_COLOR, DEFAULT_PROP_COLOR,
      G_PARAM_READWRITE ));

  g_object_class_install_property (gobject_class, PROP_ENABLE_OVERLAY,
      g_param_spec_int("enableoverlay", "enable overlay", "enable or disable text overlay", 0, 1, 1,
      G_PARAM_READWRITE ));
  GST_DEBUG("Evas text overlay class init");
}

static void
gst_evastextoverlay_init (Gstevastextoverlay *overlay)
{
  overlay->sinkpad = gst_pad_new_from_static_template(&sink_template, "sink");
  overlay->srcpad = gst_pad_new_from_static_template(&src_template, "src");
  gst_pad_set_chain_function (overlay->sinkpad, GST_DEBUG_FUNCPTR(gst_evastextoverlay_chain));
  gst_pad_set_event_function (overlay->sinkpad, GST_DEBUG_FUNCPTR(gst_evastextoverlay_event));
  gst_pad_use_fixed_caps(overlay->srcpad);
  gst_element_add_pad(GST_ELEMENT (overlay), overlay->sinkpad);
  gst_element_add_pad(GST_ELEMENT(overlay), overlay->srcpad);
  overlay->buffer_depth = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
  overlay->colorspace =   EVAS_COLORSPACE_ARGB8888;
  overlay->enable_overlay = TRUE;
  overlay->color = DEFAULT_PROP_COLOR;
  overlay->font = DEFAULT_PROP_FONT;
  overlay->style = EVAS_TEXT_STYLE_PLAIN;
  overlay->size = DEFAULT_PROP_FONT_SIZE;

  evas_init();
  GST_DEBUG("Evas text overlay init");
}

static gboolean gst_evastextoverlay_event(GstPad *pad, GstObject *parent, GstEvent *event)
{
  Gstevastextoverlay *overlay = NULL;
  gboolean res = FALSE;
  overlay = GST_EVASTEXTOVERLAY(parent);

  GST_WARNING("event: %s : %"GST_PTR_FORMAT, GST_EVENT_TYPE_NAME (event), event);
  switch (GST_EVENT_TYPE(event))
  {
    case GST_EVENT_CAPS:
    {
      GstCaps *caps;
      gst_event_parse_caps (event, &caps);
      GST_DEBUG("caps: %" GST_PTR_FORMAT, caps);
      res = gst_evastextoverlay_set_caps(pad, caps);
      /* will send our own caps downstream */
      gst_event_unref (event);
      event = NULL;
      break;
    }
    case GST_EVENT_EOS:
      break;
    default:
      break;
  }
  if (event)
    res = gst_pad_push_event(overlay->srcpad, event);
  return res;
}

static gboolean
gst_evastextoverlay_set_caps (GstPad *pad, GstCaps *caps)
{
  Gstevastextoverlay *overlay = GST_EVASTEXTOVERLAY(GST_PAD_PARENT(pad));
  GstStructure *s_in = NULL;
  int in_width=0, in_height=0, in_fps_num=0, in_fps_den=0;
  const GValue *framerate;
  GstCaps *newcaps = NULL;
  GST_DEBUG("Evas text overlay set caps");
  s_in = gst_caps_get_structure(caps, 0);
  if(s_in) {
    gst_structure_get_int(s_in, "width", &in_width);
    gst_structure_get_int(s_in, "height", &in_height);
    framerate = gst_structure_get_value (s_in, "framerate");
    in_fps_num = gst_value_get_fraction_numerator (framerate);
    in_fps_den = gst_value_get_fraction_denominator (framerate);
    overlay->width = in_width;
    overlay->height = in_height;
    GST_INFO("width and Height from parsed info is %d %d", overlay->width, overlay->height);
  }
  newcaps = gst_caps_new_simple(
    "video/x-raw",
    "format", G_TYPE_STRING, "ARGB",
    "width", G_TYPE_INT, overlay->width,
    "height", G_TYPE_INT, overlay->height,
    "framerate", GST_TYPE_FRACTION, in_fps_num, in_fps_den,
    NULL);
  if(!newcaps)
  {
    GST_ERROR("fail to create new-caps");
    return FALSE;
  }
  gst_pad_set_caps(overlay->srcpad, newcaps);
  overlay->canvas = create_canvas(overlay);
  if (!overlay->canvas)
  {
    GST_ERROR("fail to create canvas");
    return FALSE;
  }
  return TRUE;
}

static void
gst_evastextoverlay_set_property (GObject * object, guint prop_id,  const GValue * value, GParamSpec * pspec)
{
  Gstevastextoverlay *overlay = GST_EVASTEXTOVERLAY (object);
  GST_DEBUG("set property called for ID: %d", prop_id);
   switch (prop_id) {
    case PROP_XPOS:
      overlay->xpos = g_value_get_int (value);
      break;
    case PROP_YPOS:
      overlay->ypos = g_value_get_int (value);
      break;
    case PROP_TEXT:
      //if(overlay->text) g_free (overlay->text);
      overlay->text = g_value_dup_string (value);
      break;
    case PROP_FONT_SIZE:
      overlay->size = g_value_get_int (value);
      break;
    case PROP_FONT_STYLE:
      overlay->style = g_value_get_int (value);
      break;
    case PROP_FONT:
      //if(overlay->font) g_free (overlay->font);
      overlay->font = g_value_dup_string (value);
      break;
    case PROP_COLOR:
      overlay->color = g_value_get_uint (value);
      break;
    case PROP_ENABLE_OVERLAY:
      overlay->enable_overlay = g_value_get_int (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_evastextoverlay_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  Gstevastextoverlay *overlay = GST_EVASTEXTOVERLAY (object);
  switch (prop_id) {
    case PROP_XPOS:
      g_value_set_int (value, overlay->xpos);
      break;
    case PROP_YPOS:
      g_value_set_int (value, overlay->ypos);
      break;
    case PROP_TEXT:
      g_value_set_string (value, overlay->text);
      break;
    case PROP_FONT_SIZE:
      g_value_set_int (value, overlay->size);
      break;
    case PROP_FONT_STYLE:
      g_value_set_int (value, overlay->style);
      break;
    case PROP_FONT:
      g_value_set_string (value, overlay->font);
      break;
    case PROP_COLOR:
      g_value_set_uint (value, overlay->color );
      break;
    case PROP_ENABLE_OVERLAY:
      g_value_set_int (value, overlay->enable_overlay);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstFlowReturn gst_evastextoverlay_chain(GstPad *pad, GstObject *parent, GstBuffer *inbuf)
{
  Evas_Object *text;
  Gstevastextoverlay *overlay;
  int ret = 0;
  Evas_Engine_Info_Buffer *einfo;
  GstMapInfo buf_info = GST_MAP_INFO_INIT;
  GstMemory *mem;
  overlay = GST_EVASTEXTOVERLAY (parent);
  double a, r, g, b;

  //adding image to the canvas
  if(!overlay->enable_overlay) {
    /* just push out the incoming buffer*/
    ret = gst_pad_push(overlay->srcpad, inbuf);
    if (ret != GST_FLOW_OK) {
      GST_WARNING("failed to push buffer. reason: %s\n", gst_flow_get_name (ret));
    }
    return GST_FLOW_OK;
  }

  Evas_Object *image =  evas_object_image_add(overlay->canvas);
  evas_object_image_colorspace_set(image, overlay->colorspace);
  evas_object_image_size_set(image, overlay->width, overlay->height);
  evas_object_image_fill_set(image, 0, 0, overlay->width, overlay->height);
  evas_object_resize (image, overlay->width, overlay->height);
  mem = gst_buffer_peek_memory (inbuf, 0);
  gst_memory_map (mem, &buf_info, GST_MAP_READ);
  if(buf_info.data) {
    evas_object_image_data_set(image, buf_info.data);//set the data got from buf
  } else {
    GST_ERROR("invalid buffer input");
  }
  evas_object_image_filled_set(image, EINA_TRUE);
  evas_object_show(image);
  //draw_scene(overlay->canvas);
  //adding text to the canvas
  text = evas_object_text_add(overlay->canvas);
  evas_object_text_style_set(text, overlay->style);
  a = 255;//(overlay->color >> 24) & 0xff;
  r = (overlay->color >> 16) & 0xff;
  g = (overlay->color >> 8) & 0xff;
  b = (overlay->color >> 0) & 0xff;
  evas_object_color_set(text, r, g, b, a);
  evas_object_text_font_set(text, overlay->font, overlay->size);
  evas_object_text_text_set(text, overlay->text);
  evas_object_resize(text, (3 * overlay->width) / 4, overlay->height / 4);
  evas_object_move(text, overlay->xpos, overlay->ypos);
  evas_object_show(text);
  draw_scene(overlay->canvas);
  einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(overlay->canvas);
  if (!einfo)
  {
    GST_ERROR("ERROR: could not get evas engine info!");
    goto EXIT;
  }
  GstBuffer *outbuf = gst_buffer_new_allocate (NULL, overlay->width*overlay->height*4, NULL);
  gst_buffer_copy_into (outbuf, inbuf,
                        GST_BUFFER_COPY_FLAGS|GST_BUFFER_COPY_TIMESTAMPS|GST_BUFFER_COPY_META,
                        0,
                        -1);
  gst_buffer_fill (outbuf, 0, einfo->info.dest_buffer, overlay->width*overlay->height*4);
  /* just push out the incoming buffer with added text */
  GST_DEBUG("buffer ts = %" GST_TIME_FORMAT", dur = %"GST_TIME_FORMAT, GST_TIME_ARGS(GST_BUFFER_PTS(outbuf)), GST_TIME_ARGS(GST_BUFFER_DURATION(outbuf)));
  ret = gst_pad_push(overlay->srcpad, outbuf);
  if (ret != GST_FLOW_OK) {
    GST_WARNING("failed to push buffer. reason: %s\n", gst_flow_get_name (ret));
  }
  gst_memory_unmap (mem, &buf_info);
  gst_buffer_unref (inbuf);
  return GST_FLOW_OK;
EXIT:
  if(inbuf)
  {
    gst_buffer_unref (inbuf);//decrease the count by one , and free if count reaches 0
  }
  GST_DEBUG("transform exit");
  return GST_FLOW_ERROR;
}

static Evas *create_canvas(Gstevastextoverlay *overlay)
{
  Evas *canvas;
  Evas_Engine_Info_Buffer *einfo;
  int method;
  void *pixels = NULL;
  method = evas_render_method_lookup("buffer");
  if (method <= 0)
  {
    GST_ERROR("ERROR: evas was not compiled with 'buffer' engine!");
    return NULL;
  }
  canvas = evas_new();
  if (!canvas)
  {
    GST_ERROR("ERROR: could not instantiate new evas canvas");
    return NULL;
  }
  evas_output_method_set(canvas, method);
  evas_output_size_set(canvas, overlay->width, overlay->height);
  evas_output_viewport_set(canvas, 0, 0, overlay->width, overlay->height);
  einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(canvas);
  if (!einfo)
  {
    GST_ERROR("ERROR: could not get evas engine info!");
    evas_free(canvas);
    return NULL;
  }
  pixels = g_malloc(overlay->width * overlay->height * 4);
  if (!pixels)
  {
    GST_ERROR("ERROR: could not allocate canvas pixels! %d, %d", overlay->width, overlay->height);
    evas_free(canvas);
    return NULL;
  }
  einfo->info.depth_type = overlay->buffer_depth;
  einfo->info.dest_buffer = pixels;
  einfo->info.dest_buffer_row_bytes = overlay->width * 4;
  einfo->info.use_color_key = 0;
  einfo->info.alpha_threshold = 0;
  einfo->info.func.new_update_region = NULL;
  einfo->info.func.free_update_region = NULL;
  evas_engine_info_set(canvas, (Evas_Engine_Info *)einfo);
  GST_DEBUG("canvas create end");
  return canvas;
}

static void destroy_canvas(Evas *canvas)
{
  GST_DEBUG("destroy canvas start");
  Evas_Engine_Info_Buffer *einfo;
  einfo = (Evas_Engine_Info_Buffer *)evas_engine_info_get(canvas);
  if (!einfo)
  {
    fputs("ERROR: could not get evas engine info!", stderr);
    evas_free(canvas);
    return;
  }
  g_free(einfo->info.dest_buffer);
  GST_DEBUG("destroy canvas end");
  evas_free(canvas);
}

static void draw_scene(Evas *canvas)
{
  Eina_List *updates, *n;
  Eina_Rectangle *update;
  updates = evas_render_updates(canvas);
  EINA_LIST_FOREACH(updates, n, update)

    //GST_DEBUG("");
    //GST_DEBUG("UPDATED REGION: pos: %3d, %3d    size: %3dx%3d", update->x, update->y, update->w, update->h);
  evas_render_updates_free(updates);
}

static void
gst_evastextoverlay_finalize (GObject * object)
{
  GST_DEBUG("finalize start");
  evas_shutdown();
  Gstevastextoverlay *overlay = GST_EVASTEXTOVERLAY (object);
  if(overlay->text) g_free(overlay->text);
  G_OBJECT_CLASS(parent_class)->finalize (object);
  GST_DEBUG(" finalize end");
}

static GstStateChangeReturn gst_evastextoverlay_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  Gstevastextoverlay *overlay  = GST_EVASTEXTOVERLAY (element);
  GST_INFO ("Change state  = %d ", transition);
  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      GST_DEBUG ("change state NULL TO READY");
    break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
    {
      GST_DEBUG ("change state PAUSED_TO_PLAYING");
      GST_DEBUG ("Nothing to be done");
    }
    break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_DEBUG ("change state READY to PAUSED");
      GST_DEBUG ("Nothing to be done");
    break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      GST_DEBUG ("change state PLAYING to PAUSED");
      GST_DEBUG ("Nothing to be done");
    break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_DEBUG ("change state PAUSED to READY");
    break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      GST_DEBUG ("change state READY to NULL");
      destroy_canvas(overlay->canvas);
      overlay->canvas = NULL;
    break;
    default:
    break;
  }
  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  return ret;
}

#ifndef PACKAGE
#define PACKAGE "evastextoverlay"
#endif

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean evastextoverlay_init (GstPlugin * evastextoverlay)
{
  GST_DEBUG_CATEGORY_INIT (gst_evastextoverlay_debug, "evastextoverlay",0, "Template evastextoverlay");
  return gst_element_register (evastextoverlay, "evastextoverlay", GST_RANK_PRIMARY, GST_TYPE_EVASTEXTOVERLAY);
}

/* gstreamer looks for this structure to register evastextoverlays
 *
 * exchange the string 'Template evastextoverlay' with your evastextoverlay description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    evastextoverlay,
    "Template evastextoverlay",
    evastextoverlay_init,
    VERSION,
    "Proprietary",
    "Samsung Electronics Co",
    "http://www.samsung.com"
)
