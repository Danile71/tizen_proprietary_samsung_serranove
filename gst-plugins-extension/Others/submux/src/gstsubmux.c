/*
 * submux
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Heechul jeon <heechul.jeon@samsung.com>
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

#include "config.h"
#include "gstsubmux.h"
#include <gst/base/gstadapter.h>
#include <glib/gstdio.h>
#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <gst/tag/tag.h>
#include <fcntl.h>
#include <unistd.h>
#include <gst/gst.h>

static GstStaticPadTemplate gst_submux_sink_template = GST_STATIC_PAD_TEMPLATE(
    "sink_%u",
    GST_PAD_SINK,
    GST_PAD_REQUEST,
    GST_STATIC_CAPS("text/x-raw, format = { pango-markup, utf8 }")
);
static GstStaticPadTemplate gst_submux_src_template = GST_STATIC_PAD_TEMPLATE(
    "src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("text/x-raw, format = { pango-markup, utf8 }")
);

GST_DEBUG_CATEGORY_STATIC (gst_submux_debug);
#define GST_CAT_DEFAULT gst_submux_debug
#define _do_init(bla) \
    GST_DEBUG_CATEGORY_INIT (gst_submux_debug, "submux", 0, "submux");
////////////////////////////////////////////////////////
//        Gstreamer Base Prototype                    //
////////////////////////////////////////////////////////

G_DEFINE_TYPE_WITH_CODE(Gstsubmux, gst_submux, GST_TYPE_ELEMENT, GST_DEBUG_CATEGORY_INIT (gst_submux_debug, "submux", 0, "submux"));

#define GST_SUBMUX_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GST_TYPE_SUBMUX, GstsubmuxPrivate))
#define MAX_LANGUAGE 10
static gboolean gst_submux_create_pipelines(Gstsubmux *self,GstPad * pad);
static GstPad*  gst_submux_request_new_pad (GstElement * element,
       GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps);
static void gst_submux_release_pad (GstElement * element, GstPad * pad);
static gchar * gst_submux_get_external_file_path(Gstsubmux *submux);
static FILE * gst_submux_get_external_file_handle(Gstsubmux *submux);
static gint gst_submux_external_file_status(Gstsubmux *submux);
static gchar* gst_submux_extract_data (Gstsubmux *submux);
static gboolean gst_create_own_language_list (Gstsubmux *submux) ;
static GstSubMuxFormat gst_submux_data_format_autodetect (gchar * match_str);
static gpointer gst_submux_data_format_autodetect_regex_once (GstSubMuxRegex regtype);
static gboolean gst_submux_format_autodetect (Gstsubmux * self);
static void gst_submux_class_init(GstsubmuxClass *klass);
static GstStateChangeReturn gst_submux_change_state (GstElement * element, GstStateChange transition);
static void gst_submux_init(Gstsubmux *submux);
static GstFlowReturn gst_submux_chain (GstPad *pad, GstObject * parent, GstBuffer *buffer);
static void gst_submux_dispose(GObject *object);
static void gst_submux_loop (Gstsubmux * submux);
static gboolean gst_submux_stream_init(GstSubmuxStream * stream);
static void gst_submux_stream_deinit(GstSubmuxStream * stream,Gstsubmux * submux);
static void gst_submux_stream_deinit_queue (GstSubmuxStream *stream,Gstsubmux *submux);
static GstFlowReturn gst_submux_on_new_sample (GstElement *appsink, void *data);
static gboolean gst_submux_handle_src_event (GstPad * pad, GstObject * parent, GstEvent * event);
static gboolean gst_submux_handle_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
////////////////////////////////////////////////////////
//        Plugin Utility Prototype                    //
////////////////////////////////////////////////////////
static void gst_submux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_submux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static gboolean gst_submux_deinit_private_values(Gstsubmux *submux,gboolean clear_lang_list);
static gchar *convert_to_utf8 (const gchar * str, gsize len, const gchar * encoding,
    gsize * consumed, GError ** err, Gstsubmux * self);
static gchar *detect_encoding (const gchar * str, gsize len);
static gchar * convert_encoding (Gstsubmux * self, const gchar * str, gsize len, gsize * consumed);
#define DEFAULT_ENCODING           NULL
#define DEFAULT_CURRENT_LANGUAGE   NULL
static GstElementClass *parent_class = NULL;
/*
**
**  Description    : Initilizes the Gstsubmux's class
**  Params        : @ klass instance of submux plugin's class
**  return        : None
**  Comments    : Declaring properties and over-writing function pointers
**
*/
static void
gst_submux_class_init(GstsubmuxClass *klass)
{
    GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);
    GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
    g_type_class_add_private (klass, sizeof (GstsubmuxPrivate));
    gobject_class->set_property = gst_submux_set_property;
    gobject_class->get_property = gst_submux_get_property;

    gst_element_class_set_static_metadata(gstelement_class,
                                                                      "submux",
                                                                      "Codec/Parser/Subtitle",
                                                                      "muxing of different subtitle stream",
                                                                      "Samsung Electronics <www.samsung.com>");
    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&gst_submux_sink_template));
    gst_element_class_add_pad_template(gstelement_class, gst_static_pad_template_get(&gst_submux_src_template));

    g_object_class_install_property (gobject_class, PROP_ENCODING,
        g_param_spec_string ("subtitle-encoding", "subtitle charset encoding",
            "Encoding to assume if input subtitles are not in UTF-8 or any other "
            "Unicode encoding. If not set, the GST_SUBTITLE_ENCODING environment "
            "variable will be checked for an encoding to use. If that is not set "
            "either, ISO-8859-15 will be assumed.", DEFAULT_ENCODING,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_VIDEOFPS,
        gst_param_spec_fraction ("video-fps", "Video framerate",
            "Framerate of the video stream. This is needed by some subtitle "
            "formats to synchronize subtitles and video properly. If not set "
            "and the subtitle format requires it subtitles may be out of sync.",
            0, 1, G_MAXINT, 1, 24000, 1001,
            G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property (gobject_class, PROP_EXTSUB_CURRENT_LANGUAGE,
          g_param_spec_string ("current-language", "Current language",
                "Current language of the subtitle in external subtitle case.",
                DEFAULT_CURRENT_LANGUAGE,
                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property (gobject_class, PROP_IS_INTERNAL,
          g_param_spec_boolean ("is-internal", "is internal",
              "TRUE for internal subtitle case",
              FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    g_object_class_install_property (gobject_class, PROP_LANG_LIST,
          g_param_spec_pointer ("lang-list", "language list", "List of languages selected/not selected",
               G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    parent_class = g_type_class_peek_parent (klass);
    gstelement_class->request_new_pad = GST_DEBUG_FUNCPTR(gst_submux_request_new_pad);
    gobject_class->dispose = GST_DEBUG_FUNCPTR(gst_submux_dispose);
    gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_submux_change_state);
    gstelement_class->release_pad = GST_DEBUG_FUNCPTR (gst_submux_release_pad);
}

/*
**
**  Description    : Initilizes the submux element
**  Params        : (1)instance of submux (2) instance of submux class
**  return        : None
**  Comments    : instantiate pads and add them to element, set pad calback functions
**
*/
static void
gst_submux_init(Gstsubmux *submux)
{
  SUBMUX_FENTER (submux);
  submux->priv = GST_SUBMUX_GET_PRIVATE(submux);
  submux->srcpad = gst_pad_new_from_static_template(&gst_submux_src_template, "src");
  gst_pad_set_event_function (submux->srcpad,
            GST_DEBUG_FUNCPTR (gst_submux_handle_src_event));
  submux->priv->first_buffer = FALSE;
  gst_segment_init (&submux->segment, GST_FORMAT_TIME);
  submux->flushing = FALSE;
  submux->msl_streams = NULL;
  submux->stop_loop = FALSE;
  submux->need_segment = TRUE;
  submux->pipeline_made = FALSE;
  submux->external_sinkpad = FALSE;
  submux->detected_encoding = NULL;
  submux->fps_n = 30;
  submux->fps_d = 1;
  submux->encoding = NULL;
  submux->seek_came = FALSE;
  submux->sinkpads_count = 0;
  submux->langlist_msg_posted = FALSE;
  submux->cur_buf_array = NULL;
  submux->priv->is_internal = FALSE;
  submux->external_filepath = NULL;
  submux->seek_new_file = FALSE;
  submux->detected_encoding = NULL;
  submux->srcpad_loop_running = FALSE;;
  g_mutex_init (&submux->srcpad_loop_lock);
  g_cond_init (&submux->srcpad_loop_cond);
  gst_element_add_pad (GST_ELEMENT (submux), submux->srcpad);
  SUBMUX_FLEAVE (submux);
}

/*
**
**  Description    : for setting the property of submux
**  return        : None
**  Comments    : To set the various properties of submux
**
*/
static void
gst_submux_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  Gstsubmux *submux = GST_SUBMUX (object);
  guint length = 0;
  gint i = 0;
  GstLangStruct *cur_language=NULL;
  GstSubmuxStream *cur_stream = NULL;
  GST_OBJECT_LOCK (submux);
  length = g_list_length(submux->priv->lang_list);
  switch (prop_id) {
    case PROP_ENCODING:
    {
      SAFE_FREE (submux->encoding);
      submux->encoding = g_value_dup_string (value);
      GST_DEBUG_OBJECT (submux, "Subtitle encoding set to %s",GST_STR_NULL (submux->encoding));
      for(i = 0;i < length;i++) {
        cur_stream = g_list_nth_data(submux->streams,i);
        g_object_set (G_OBJECT (cur_stream->pipe_struc.parser), "subtitle-encoding", submux->encoding, NULL);
      }
    break;
    }
    case PROP_VIDEOFPS:
    {
      submux->fps_n = gst_value_get_fraction_numerator (value);
      submux->fps_d = gst_value_get_fraction_denominator (value);
      GST_DEBUG_OBJECT (submux, "Video framerate set to %d/%d", submux->fps_n, submux->fps_d);
      break;
    }
    case PROP_EXTSUB_CURRENT_LANGUAGE:
    {
      for (i = 0; i < length; i++) {
        cur_stream = g_list_nth_data(submux->streams, i);
        cur_language = g_list_nth_data(submux->priv->lang_list, i);
        GST_DEBUG_OBJECT (submux, "Value of current-language key is %s", cur_language->language_key);
        g_object_set (G_OBJECT (cur_stream->pipe_struc.parser), "current-language",cur_language->language_key, NULL);
      }
      break;
    }
    case PROP_IS_INTERNAL:
    {
      submux->priv->is_internal = g_value_get_boolean (value);
      GST_DEBUG_OBJECT (submux, "Setting the is_internal prop to %d", submux->priv->is_internal);
      break;
    }
    case PROP_LANG_LIST:
    {
      submux->priv->lang_list = (GList*) g_value_get_pointer (value);
      GST_DEBUG_OBJECT (submux, "Updating the languages list and length is %d", g_list_length (submux->priv->lang_list));
      submux->msl_streams = g_list_copy (submux->priv->lang_list);
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (submux);
}

/*
**
**  Description    : for getting the property of submux
**  return        : None
**  Comments    : To get the various properties of submux in case called by MSL
**
*/
static void
gst_submux_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  Gstsubmux *submux = GST_SUBMUX (object);
  GST_OBJECT_LOCK (submux);
  switch (prop_id) {
    case PROP_ENCODING:
      g_value_set_string (value, submux->encoding);
      break;
    case PROP_VIDEOFPS:
      gst_value_set_fraction (value, submux->fps_n, submux->fps_d);
      break;
    case PROP_EXTSUB_CURRENT_LANGUAGE:
      GST_DEBUG_OBJECT (submux, "Getting the current language");
      break;
    case PROP_IS_INTERNAL:
      g_value_set_boolean(value,submux->priv->is_internal);
      break;
    case PROP_LANG_LIST:
      g_value_set_pointer(value,(gpointer)(submux->priv->lang_list));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
  GST_OBJECT_UNLOCK (submux);
}

static void
gst_submux_dispose (GObject *object)
{
  Gstsubmux *submux = GST_SUBMUX(object);
  SUBMUX_FENTER (submux);

  if (submux && GST_PAD_TASK(submux->srcpad)) {
    gst_pad_stop_task (submux->srcpad);
    GST_INFO_OBJECT (submux, "Stopped %s pad task", gst_pad_get_name (submux->srcpad));
  }

  if (submux->srcpad) {
    gst_element_remove_pad (GST_ELEMENT_CAST (submux), submux->srcpad);
    submux->srcpad = NULL;
  }

  if (submux->priv->lang_list && !submux->priv->is_internal) {
    g_list_free (submux->priv->lang_list);
    submux->priv->lang_list = NULL;
  }

  if (submux->srcpad_loop_lock.p) {
    g_cond_broadcast (&submux->srcpad_loop_cond);
    g_mutex_clear (&submux->srcpad_loop_lock);
    submux->srcpad_loop_lock.p = NULL;
  }

  g_cond_clear (&submux->srcpad_loop_cond);

  if (submux->sinkpad) {
    g_list_free (submux->sinkpad);
    submux->sinkpad = NULL;
  }

  SAFE_FREE (submux->external_filepath);
  gst_submux_deinit_private_values (submux,TRUE);

  GST_CALL_PARENT (G_OBJECT_CLASS, dispose, (object));
  SUBMUX_FLEAVE (submux);
}

static void
gst_submux_stop (Gstsubmux* submux)
{
  GstSubmuxStream *new_stream = NULL;
  guint i = 0;
  guint length =0;
  SUBMUX_FENTER (submux);

  submux->stop_loop = TRUE;
  if (submux->priv->is_internal) {
    for (i = 0; i < (submux->sinkpads_count); i++) {
      new_stream =  g_list_nth_data (submux->streams, i);
      if (new_stream) {
        g_mutex_lock (&new_stream->queue_lock);
        g_cond_signal (&new_stream->queue_empty);
        g_mutex_unlock (&new_stream->queue_lock);
      }
    }
  } else {
    if(submux->priv->parser_type == GST_SUB_PARSE_FORMAT_SAMI)
     length = g_list_length (submux->priv->lang_list);
    else
     length =1;
    for (i = 0; i < length ; i++) {
      new_stream =  g_list_nth_data (submux->streams, i);
      if (new_stream) {
        g_mutex_lock (&new_stream->queue_lock);
        g_cond_signal (&new_stream->queue_empty);
        g_mutex_unlock (&new_stream->queue_lock);
      }
    }
  }
  /*Wait for loop task to exit before proceeding*/
  g_mutex_lock (&submux->srcpad_loop_lock);
  if (submux->srcpad_loop_running) {
    GST_WARNING_OBJECT (submux, "Waiting for loop exit");
    g_cond_wait (&submux->srcpad_loop_cond, &submux->srcpad_loop_lock);
  }
  g_mutex_unlock (&submux->srcpad_loop_lock);
  SUBMUX_FLEAVE (submux);
  return;
}

static GstStateChangeReturn
gst_submux_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  Gstsubmux *submux = GST_SUBMUX (element);
  gint i = 0;

  switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      GST_INFO_OBJECT (submux,"PAUSED->PLAYING");
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_INFO_OBJECT (submux,"PAUSED->READY");
      gst_submux_stop (submux);
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      GST_INFO_OBJECT (submux,"PLAYING->PAUSED");
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      GST_INFO_OBJECT (submux,"PAUSED->READY");
      if(submux->msl_streams) {
        g_list_free(submux->msl_streams);
        submux->msl_streams = NULL;
      }

      if (submux->priv->is_internal) {
        for (i = 0; i < (submux->sinkpads_count); i++){
          gst_submux_stream_deinit_queue (g_list_nth_data (submux->streams, i), submux);
        }
      } else {
           for (i = 0; i < submux->priv->stream_count; i++) {
             gst_submux_stream_deinit(g_list_nth_data (submux->streams, i),submux);
           }
           g_list_free(submux->streams);
           submux->streams = NULL;
           gst_submux_deinit_private_values (submux,TRUE);

           submux->stop_loop = FALSE;
           submux->langlist_msg_posted = FALSE;
           submux->pipeline_made = FALSE;
      }
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      GST_INFO_OBJECT (submux,"READY->NULL");
      if (submux->priv->is_internal) {
        for (i = 0; i < (submux->sinkpads_count); i++) {
          gst_submux_stream_deinit (g_list_nth_data (submux->streams, i), submux);
        }
      } else {
          for (i = 0; i < submux->priv->stream_count; i++) {
            gst_submux_stream_deinit (g_list_nth_data (submux->streams, i), submux);
        }
      }
      break;
    default:
      break;
  }
  return ret;
}

static gchar* gst_submux_get_external_file_path(Gstsubmux *submux){
  GstQuery *cquery = NULL;
  GstStructure *structure = NULL;
  const GstStructure *query_structure = NULL;
  const GValue *value = NULL;
  GstPad *sinkpad = NULL;
  gchar * file_path_full = NULL;
  SUBMUX_FENTER(submux);

  sinkpad = (GstPad *)g_list_nth_data (submux->sinkpad, 0);
  structure = gst_structure_new ("FileSrcURI",
                                 "file-uri", G_TYPE_STRING, NULL, NULL);

  cquery = gst_query_new_custom (GST_QUERY_CUSTOM, structure);
  gst_structure_free (structure);

  if (!gst_pad_peer_query (sinkpad, cquery)){
    GST_ERROR_OBJECT (submux, "Failed to query SMI file path");
    gst_query_unref (cquery);
    return NULL;
  }
  query_structure = gst_query_get_structure (cquery);
  value = gst_structure_get_value (query_structure, "file-uri");

  file_path_full = g_strdup (g_value_get_string (value));
  gst_query_unref (cquery);

  SUBMUX_FLEAVE(submux);
  return file_path_full;
}

static FILE * gst_submux_get_external_file_handle(Gstsubmux *submux){
  FILE  * fp = NULL;
  gchar * file_path_type = NULL;
  gchar * file_path = NULL;
  SUBMUX_FENTER(submux);
  file_path = submux->external_filepath;
  file_path_type = g_strndup ((gchar *) submux->external_filepath, 4);

  if (!g_strcmp0(file_path_type, "file")){
    file_path += 7;
    fp = fopen (file_path, "r");
    if (!fp){
      GST_ERROR_OBJECT (submux, "Failed to open file");
      SAFE_FREE(file_path_type);
      return NULL;
    }
  }else{
    GST_ERROR_OBJECT (submux, "File is not local");
    SAFE_FREE(file_path_type);
    return NULL;
  }
  SUBMUX_FLEAVE(submux);
  return fp;
}

static gint gst_submux_external_file_status(Gstsubmux *submux){
  gchar * file_path_full = NULL;
  gint ret = EXTERNAL_FILE_PATH_UNKNOWN;
  SUBMUX_FENTER(submux);
  file_path_full = gst_submux_get_external_file_path(submux);
  if(!submux->external_filepath){
    submux->external_filepath = g_strdup(file_path_full);
    GST_INFO_OBJECT (submux, "External File path initialized to:%s",submux->external_filepath);
    submux->seek_new_file = TRUE;
    ret = EXTERNAL_FILE_PATH_INITIALIZED;
  }else if(!g_strcmp0 (file_path_full, submux->external_filepath)) {
    GST_INFO_OBJECT (submux, "Same external file URI");
    ret = EXTERNAL_FILE_PATH_UNCHANGED;
  }else{
    SAFE_FREE (submux->external_filepath);
    submux->external_filepath = g_strdup(file_path_full);
    GST_INFO_OBJECT (submux, "External File path changed to:%s",submux->external_filepath);
    submux->seek_new_file = TRUE;
    ret = EXTERNAL_FILE_PATH_CHANGED;
  }
  SAFE_FREE (file_path_full);
  return ret;
}

/*extracting data for file format detection*/
static gchar* gst_submux_extract_data (Gstsubmux *submux){
  gchar  *line = NULL;
  gboolean is_converted = FALSE;
  gchar *converted = NULL;
  FILE  * fp = NULL;
  guint charCount = 0;
  gsize consumed = 0;

  SUBMUX_FENTER (submux);

  fp = gst_submux_get_external_file_handle(submux);
  if (!fp){
    GST_ERROR_OBJECT (submux, "Could not get External subtitle handle");
    return NULL;
  }

  line = (gchar*)g_malloc (2049);
  charCount = fread (line, sizeof(char), 2048, fp);
  line[charCount] = '\0';

  if (!charCount) {
    GST_WARNING_OBJECT (submux, "fread returned zero bytes");
    fclose (fp);
    SAFE_FREE(line);
    return NULL;
  }

  if (!submux->detected_encoding)
    submux->detected_encoding = detect_encoding (line, (gsize)charCount);

  converted = convert_encoding (submux, line, charCount, &consumed);

  if (converted){
    GST_INFO("returned from conversion and length of converted string is[%d]", strlen(converted));
    is_converted = TRUE;
  }

  fclose (fp);

  if(is_converted) {
    SAFE_FREE(line);
    return converted;
  }

  SUBMUX_FLEAVE (submux);
  return line;
}

static gpointer
gst_submux_data_format_autodetect_regex_once (GstSubMuxRegex regtype)
{
  gpointer result = NULL;
  GError *gerr = NULL;
  switch (regtype) {
    case GST_SUB_PARSE_REGEX_MDVDSUB:
      result =
          (gpointer) g_regex_new ("^\\{[0-9]+\\}\\{[0-9]+\\}",
          G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, &gerr);
      if (result == NULL) {
        g_warning ("Compilation of mdvd regex failed: %s", gerr->message);
        g_error_free (gerr);
      }
      break;
    case GST_SUB_PARSE_REGEX_SUBRIP:
      result = (gpointer)
          g_regex_new ("^[\\s\\n]*[\\n]? {0,3}[ 0-9]{1,4}\\s*(\x0d)?\x0a"
          " ?[0-9]{1,2}: ?[0-9]{1,2}: ?[0-9]{1,2}[,.] {0,2}[0-9]{1,3}"
          " +--> +[0-9]{1,2}: ?[0-9]{1,2}: ?[0-9]{1,2}[,.] {0,2}[0-9]{1,2}",
          G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, &gerr);
      if (result == NULL) {
        g_warning ("Compilation of subrip regex failed: %s", gerr->message);
        g_error_free (gerr);
      }
      break;
    case GST_SUB_PARSE_REGEX_DKS:
      result = (gpointer) g_regex_new ("^\\[[0-9]+:[0-9]+:[0-9]+\\].*",
          G_REGEX_RAW | G_REGEX_OPTIMIZE, 0, &gerr);
      if (result == NULL) {
        g_warning ("Compilation of dks regex failed: %s", gerr->message);
        g_error_free (gerr);
      }
      break;
    default:
      GST_WARNING ("Trying to allocate regex of unknown type %u", regtype);
  }
  return result;
}

static GstSubMuxFormat
gst_submux_data_format_autodetect (gchar * match_str)
{
  guint n1, n2, n3;

  static GOnce mdvd_rx_once = G_ONCE_INIT;
  static GOnce subrip_rx_once = G_ONCE_INIT;
  static GOnce dks_rx_once = G_ONCE_INIT;

  GRegex *mdvd_grx;
  GRegex *subrip_grx;
  GRegex *dks_grx;

  g_once (&mdvd_rx_once,
      (GThreadFunc) gst_submux_data_format_autodetect_regex_once,
      (gpointer) GST_SUB_PARSE_REGEX_MDVDSUB);
  g_once (&subrip_rx_once,
      (GThreadFunc) gst_submux_data_format_autodetect_regex_once,
      (gpointer) GST_SUB_PARSE_REGEX_SUBRIP);
  g_once (&dks_rx_once,
      (GThreadFunc) gst_submux_data_format_autodetect_regex_once,
      (gpointer) GST_SUB_PARSE_REGEX_DKS);

  mdvd_grx = (GRegex *) mdvd_rx_once.retval;
  subrip_grx = (GRegex *) subrip_rx_once.retval;
  dks_grx = (GRegex *) dks_rx_once.retval;

  if (g_regex_match (mdvd_grx, match_str, 0, NULL) == TRUE) {
    GST_LOG ("MicroDVD (frame based) format detected");
    return GST_SUB_PARSE_FORMAT_MDVDSUB;
  }
  if (g_regex_match (subrip_grx, match_str, 0, NULL) == TRUE) {
    GST_LOG ("SubRip (time based) format detected");
    return GST_SUB_PARSE_FORMAT_SUBRIP;
  }
  if (g_regex_match (dks_grx, match_str, 0, NULL) == TRUE) {
    GST_LOG ("DKS (time based) format detected");
    return GST_SUB_PARSE_FORMAT_DKS;
  }

  if (!strncmp (match_str, "FORMAT=TIME", 11)) {
    GST_LOG ("MPSub (time based) format detected");
    return GST_SUB_PARSE_FORMAT_MPSUB;
  }
  if (strstr (match_str, "<SAMI>") != NULL ||
      strstr (match_str, "<sami>") != NULL) {
    GST_LOG ("SAMI (time based) format detected");
    return GST_SUB_PARSE_FORMAT_SAMI;
  }
  /* we're boldly assuming the first subtitle appears within the first hour */
  if (sscanf (match_str, "0:%02u:%02u:", &n1, &n2) == 2 ||
      sscanf (match_str, "0:%02u:%02u=", &n1, &n2) == 2 ||
      sscanf (match_str, "00:%02u:%02u:", &n1, &n2) == 2 ||
      sscanf (match_str, "00:%02u:%02u=", &n1, &n2) == 2 ||
      sscanf (match_str, "00:%02u:%02u,%u=", &n1, &n2, &n3) == 3) {
    GST_LOG ("TMPlayer (time based) format detected");
    return GST_SUB_PARSE_FORMAT_TMPLAYER;
  }
  if (sscanf (match_str, "[%u][%u]", &n1, &n2) == 2) {
    GST_LOG ("MPL2 (time based) format detected");
    return GST_SUB_PARSE_FORMAT_MPL2;
  }
  if (strstr (match_str, "[INFORMATION]") != NULL) {
    GST_LOG ("SubViewer (time based) format detected");
    return GST_SUB_PARSE_FORMAT_SUBVIEWER;
  }
  if (strstr (match_str, "{QTtext}") != NULL) {
    GST_LOG ("QTtext (time based) format detected");
    return GST_SUB_PARSE_FORMAT_QTTEXT;
  }
  /* We assume the LRC file starts immediately */
  if (match_str[0] == '[') {
    gboolean all_lines_good = TRUE;
    gchar **split;
    gchar **ptr;

    ptr = split = g_strsplit (match_str, "\n", -1);
    while (*ptr && *(ptr + 1)) {
      gchar *str = *ptr;
      gint len = strlen (str);

      if (sscanf (str, "[%u:%02u.%02u]", &n1, &n2, &n3) == 3 ||
          sscanf (str, "[%u:%02u.%03u]", &n1, &n2, &n3) == 3) {
        all_lines_good = TRUE;
      } else if (str[len - 1] == ']' && strchr (str, ':') != NULL) {
        all_lines_good = TRUE;
      } else {
        all_lines_good = FALSE;
        break;
      }

      ptr++;
    }
    g_strfreev (split);

    if (all_lines_good)
      return GST_SUB_PARSE_FORMAT_LRC;
  }

  GST_WARNING ("no subtitle format detected");
  return GST_SUB_PARSE_FORMAT_UNKNOWN;
}

/*checking the type of subtitle*/
static gboolean
gst_submux_format_autodetect (Gstsubmux *self)
{
  gchar *data;
  GstSubMuxFormat format;
  gchar * line = NULL;

  SUBMUX_FENTER(self);
  if (self->priv->is_internal) {
    GST_DEBUG_OBJECT (self, "File is of internal type");
    return TRUE;
  }
  line = gst_submux_extract_data (self);
  if (!line)
    return FALSE;
  if (strlen (line) < 30) {
    GST_ERROR_OBJECT (self, "File too small to be a subtitles file");
    SAFE_FREE(line);
    return FALSE;
  }

  data = g_strndup (line, FORMAT_DETECTION_TEXT_STRING);
  format = gst_submux_data_format_autodetect (data);
  SAFE_FREE (data);
  SAFE_FREE(line);

  SUBMUX_FLEAVE(self);
  if(GST_SUB_PARSE_FORMAT_UNKNOWN!=format){
    self->priv->parser_type = format;
    return TRUE;
  }else{
    return FALSE;
  }
}

/*to validate the number of languages in case of sami files*/
static gboolean
gst_calculate_number_languages(Gstsubmux *self) {
  gchar* text=NULL;
  gchar *start = NULL;
  gchar *end = NULL;
  gint count = 0;
  gchar* found = NULL;
  gchar * name_temp = NULL;
  int i = 0, j = 0;

  SUBMUX_FENTER (self);

  if ((self->priv->parser_type != GST_SUB_PARSE_FORMAT_SAMI) || self->priv->is_internal)
    return TRUE;

  text = gst_submux_extract_data (self);
  start = g_strstr_len (text, strlen (text), "!--");
  if (!start) {
    GST_ERROR_OBJECT (self, "Could not find the language start code in smi file");
    return gst_create_own_language_list(self);
  }
  end =  g_strstr_len (start, strlen (start), "-->");
  if (!end){
    GST_ERROR_OBJECT (self, "Could not find the language end code in smi file");
    goto error;
  }

  found = start + 1;

  while (TRUE) {
    found = (gchar*)strcasestr (found, "lang:");
    if (!found)
       break;
    found++;
    count++;
  }

  if (!count){
    return gst_create_own_language_list(self);
  }

  for (i = 0; i < count; i++) {
    gchar *attr_name = NULL, *attr_value = NULL;
    GstLangStruct *new = NULL;

    start = (gchar*)strcasestr (start, "lang:");
    attr_value = (gchar*)malloc (3);
    if (!attr_value) {
      GST_ERROR_OBJECT (self, "memory could not be allocated through malloc call");
      goto error;
    }
    start = start + 5;
    strncpy (attr_value, start, 2);
    attr_value[2] = '\0';
    GST_DEBUG_OBJECT (self, "Language value comes as %s", attr_value);
    name_temp = start;
    while (TRUE) {
      if (*name_temp == '{') {
        int character_count = 0;
        while (TRUE) {
          name_temp--;
          if (*name_temp == '.') {
            attr_name = (gchar*) malloc (character_count + 1);
            break;
          } else if (*name_temp != ' ')
             character_count++;
        }
        break;
      }
      name_temp--;
    }
    if (!attr_name) {
      GST_ERROR_OBJECT (self, "Could not find the languages field in the file");
      free(attr_value);
      goto error;
    }
    name_temp++;
    for (j = 0; *(name_temp + j) != ' '; j++) {
      attr_name[j] = *(name_temp + j);
    }
    attr_name[j] = '\0';
    new = g_new0 (GstLangStruct, 1);
    new->language_code = (gchar*) malloc (strlen (attr_value) + 1);
    if (new->language_code && attr_value)
      strcpy (new->language_code, attr_value);
    new->language_key = (gchar*) malloc (strlen (attr_name) + 1);
    if (new->language_key && attr_name)
      strcpy (new->language_key, attr_name);
    free (attr_name);
    free (attr_value);
    self->priv->lang_list = g_list_append (self->priv->lang_list, new);
  }
  SAFE_FREE(text);
  SUBMUX_FLEAVE (self);
  return TRUE;
error:
  SAFE_FREE(text);
  SUBMUX_FLEAVE (self);
  return FALSE;
}

/*to initialize stream*/
static gboolean gst_submux_stream_init(GstSubmuxStream * stream)
{
  GST_DEBUG ("%s E", __FUNCTION__);
  stream->duration = 0;
  stream->need_segment = TRUE;
  stream->flushing = FALSE;
  stream->eos_sent = FALSE;
  stream->eos_came = FALSE;
  stream->discont_came = FALSE;
  stream->eos_ts = -1;
  stream->last_ts = -1;
  stream->queue = g_queue_new ();
  g_cond_init (&stream->queue_empty);
  g_mutex_init (&stream->queue_lock);
  stream->flush_done = FALSE;
  GST_DEBUG ("%s X", __FUNCTION__);
  return TRUE;
}

/*to create pipelines according to internal and external subtitle*/
gboolean gst_submux_create_pipelines(Gstsubmux *self, GstPad * pad)
{
  int i = 0;
  GstStateChangeReturn ret;
  GstSubmuxStream *new_stream;
  guint length = 0;
  SUBMUX_FENTER (self);

  if (!self->priv->is_internal) {
    GstLangStruct *cur_language=NULL;

    if (self->priv->parser_type == GST_SUB_PARSE_FORMAT_SAMI) {
      if (!self->priv->lang_list) {
        GST_ERROR_OBJECT(self, "Failed to get the lang list");
        return FALSE;
      }
      length = g_list_length (self->priv->lang_list);
    } else {
      length = 1;
    }

    GST_DEBUG_OBJECT (self, "External Subtitle Case, tentative language[%d]",length);

    for (i = 0; i < length; i++) {
      new_stream = g_new0 (GstSubmuxStream, 1);
      if (!gst_submux_stream_init(new_stream)) {
        GST_ERROR_OBJECT (self, "Stream init is failed");
        return FALSE;
      }

      new_stream->pipe_struc.pipe = gst_pipeline_new ("subtitle-pipeline");
      if (!new_stream->pipe_struc.pipe) {
        GST_ERROR_OBJECT (self, "Failed to create pipeline");
        return FALSE;
      }

      /* creating source element */
      new_stream->pipe_struc.appsrc = gst_element_factory_make ("appsrc", "pipe_appsrc");
      if (!new_stream->pipe_struc.appsrc) {
        GST_ERROR_OBJECT (self, "Failed to create appsrc");
        return FALSE;
      }
      g_object_set (G_OBJECT (new_stream->pipe_struc.appsrc), "block", 1, NULL);
      g_object_set (G_OBJECT (new_stream->pipe_struc.appsrc), "max-bytes", (guint64)1, NULL);

      /* create sink element */
      new_stream->pipe_struc.appsink =  gst_element_factory_make ("appsink", "pipe_appsink");
      if (!new_stream->pipe_struc.appsink) {
        GST_ERROR_OBJECT (self, "Failed to create appsink");
        return FALSE;
      }
      g_object_set (G_OBJECT (new_stream->pipe_struc.appsink), "sync", FALSE, "emit-signals", TRUE, NULL);
      g_object_set(G_OBJECT (new_stream->pipe_struc.appsrc),"emit-signals", TRUE, NULL);


      /* create parsing element */
      new_stream->pipe_struc.parser = gst_element_factory_make("subparse","pipe_parser");
      if (!new_stream->pipe_struc.parser) {
        GST_ERROR_OBJECT (self, "Failed to create parser");
        return FALSE;
      }
      if (self->priv->parser_type == GST_SUB_PARSE_FORMAT_SAMI) {
        cur_language = g_list_nth_data(self->priv->lang_list, i);
        g_object_set (G_OBJECT (new_stream->pipe_struc.parser), "current-language",cur_language->language_key, NULL);
      }
      g_object_set (G_OBJECT (new_stream->pipe_struc.parser), "subtitle-encoding", self->encoding, NULL);
      g_object_set (G_OBJECT (new_stream->pipe_struc.appsrc), "stream-type",0,"format",GST_FORMAT_TIME, NULL);
      gst_bin_add_many (GST_BIN ( new_stream->pipe_struc.pipe), new_stream->pipe_struc.appsrc, new_stream->pipe_struc.parser,new_stream->pipe_struc.appsink, NULL);
      if (!gst_element_link_many (new_stream->pipe_struc.appsrc, new_stream->pipe_struc.parser,new_stream->pipe_struc.appsink, NULL)) {
        GST_ERROR_OBJECT (self, "Failed to link elements");
        return FALSE;
      }

      ret = gst_element_set_state (new_stream->pipe_struc.pipe, GST_STATE_PLAYING);
      if (ret == GST_STATE_CHANGE_FAILURE) {
        GST_ERROR_OBJECT (self, "Failed to set state to PLAYING");
        return FALSE;
      }
      self->streams = g_list_append(self->streams, new_stream);
      self->priv->stream_count++;
      g_signal_connect (new_stream->pipe_struc.appsink, "new-sample",  G_CALLBACK (gst_submux_on_new_sample), g_list_nth_data(self->streams,i) );
    }
  } else {
    length = self->sinkpads_count;
    GST_DEBUG_OBJECT (self, "Internal Subtitle Case, tentative language[%d]",length);
    for (i = 0; i < length; i++) {
      new_stream = g_new0 (GstSubmuxStream, 1);
      if (!gst_submux_stream_init (new_stream)) {
        GST_ERROR_OBJECT (self, "stream init is failed");
        return FALSE;
     }

      self->streams=g_list_append(self->streams,new_stream);
      self->priv->stream_count++;
    }
    self->pipeline_made  = TRUE;
  }

  self->cur_buf_array = g_malloc0 (self->priv->stream_count * (sizeof (GstBuffer *)));
  if (!self->cur_buf_array) {
    GST_ERROR_OBJECT (self, "failed to allocate memory..");
    return FALSE;
  }
  SUBMUX_FLEAVE (self);
  return TRUE;
}

/* call back on recieving the new buffer in appsink pad */
static GstFlowReturn
gst_submux_on_new_sample (GstElement *appsink, void *data)
{
  GstSubmuxStream *stream = (GstSubmuxStream  *)data;
  GstBuffer *inbuf = NULL;
  GstSample *sample;
  GstMemory *buf_memory = NULL;
  GstMapInfo buf_info = GST_MAP_INFO_INIT;

  if (!stream) {
    GST_WARNING("Stream not available");
    return GST_FLOW_OK;
  }
  g_mutex_lock (&stream->queue_lock);
  sample = gst_app_sink_pull_sample (GST_APP_SINK(appsink));
  inbuf = gst_sample_get_buffer (sample);
  if (!inbuf) {
    GST_WARNING_OBJECT (stream, "Input buffer not available");
    g_mutex_unlock (&stream->queue_lock);
    return GST_FLOW_OK;
  }
  if(stream->eos_ts == -1) {
    buf_memory = gst_buffer_peek_memory(inbuf, 0);
    gst_memory_map(buf_memory, &buf_info, GST_MAP_READ);
    unsigned char *data = (unsigned char *)buf_info.data;
    gst_memory_unmap(buf_memory, &buf_info);
    if (!strcmp ((const char*)data, "eos") && GST_BUFFER_FLAG_IS_SET(inbuf,GST_BUFFER_FLAG_GAP))
    {
      stream->eos_ts = stream->last_ts;
      if (stream->eos_ts <= stream->seek_ts) {
        g_queue_push_tail (stream->queue, inbuf);
        g_cond_signal (&stream->queue_empty);
        g_mutex_unlock (&stream->queue_lock);
        GST_INFO_OBJECT (stream, "signaling queue empty signal as we are seeking beyond last subtitle");
        return GST_FLOW_OK;
      }
      gst_buffer_unref(inbuf);
    } else {
      stream->last_ts = GST_BUFFER_DURATION(inbuf) + GST_BUFFER_PTS(inbuf);
    }
  } else if (stream->eos_ts <= stream->seek_ts) {
    gst_buffer_unref(inbuf);
    GstBuffer *buf = gst_buffer_new_and_alloc (3 + 1);
    GST_DEBUG_OBJECT(stream, "sending EOS buffer to chain\n");
    GST_DEBUG_OBJECT (stream, "EOS. Pushing remaining text (if any)");
    gst_buffer_map (buf, &buf_info, GST_MAP_READWRITE);
    buf_info.data[0] = 'e';
    buf_info.data[1] = 'o';
    buf_info.data[2] = 's';
    buf_info.data[3] = '\0';
    gst_buffer_unmap (buf, &buf_info);
    GST_BUFFER_FLAG_SET(buf,GST_BUFFER_FLAG_GAP);
    g_queue_push_tail (stream->queue, buf);
    g_cond_signal (&stream->queue_empty);
    g_mutex_unlock (&stream->queue_lock);
    GST_INFO_OBJECT (stream,"signaling queue empty signal as we are seeking beyond last subtitle");
    return GST_FLOW_OK;
  }
  if (!stream->discont_came) {
    stream->discont_came = GST_BUFFER_IS_DISCONT (inbuf);
    if (stream->discont_came) {
      GST_DEBUG_OBJECT (stream, "first buffer with discont on new_buffer for stream with ts = %"
                        GST_TIME_FORMAT", dur = %"GST_TIME_FORMAT, GST_TIME_ARGS(GST_BUFFER_PTS(inbuf)),
                        GST_TIME_ARGS(GST_BUFFER_DURATION(inbuf)));
    }
  }

  if (!stream->discont_came) {
    GST_DEBUG_OBJECT (stream, "rejecting the buffer in appsink on new_buffer for stream with ts = %"
                      GST_TIME_FORMAT", dur = %"GST_TIME_FORMAT, GST_TIME_ARGS(GST_BUFFER_PTS(inbuf)),
                      GST_TIME_ARGS(GST_BUFFER_DURATION(inbuf)));
    gst_buffer_unref(inbuf);
    g_mutex_unlock (&stream->queue_lock);
    return GST_FLOW_OK;
  }
  g_queue_push_tail (stream->queue, inbuf);
  g_cond_signal (&stream->queue_empty);
  g_mutex_unlock (&stream->queue_lock);
  return GST_FLOW_OK;
}

gchar *
convert_to_utf8 (const gchar * str, gsize len, const gchar * encoding,
    gsize * consumed, GError ** err, Gstsubmux * self)
{
  gchar *ret = NULL;

  /* The char cast is necessary in glib < 2.24 */
  ret =
      g_convert_with_fallback (str, len, "UTF-8", encoding, (char *) "*",
      consumed, NULL, err);

  if (ret == NULL)
  {
    GST_DEBUG_OBJECT (self, "g_convert_with_fallback returns NULL");
    return ret;
  }

  /* + 3 to skip UTF-8 BOM if it was added */
  len = strlen (ret);
  if (len >= 3 && (guint8) ret[0] == 0xEF && (guint8) ret[1] == 0xBB
      && (guint8) ret[2] == 0xBF)
    g_memmove (ret, ret + 3, len + 1 - 3);

  return ret;
}

gchar *
convert_encoding (Gstsubmux * self, const gchar * str, gsize len, gsize * consumed)
{
  const gchar *encoding;
  gchar *ret = NULL;
  GError *err = NULL;

  GST_INFO_OBJECT (self, "Detected encoding: %s Set encoding: %s",self->detected_encoding,self->encoding);
  *consumed = 0;

  /* First try any detected encoding */
  if (self->detected_encoding) {
    ret = convert_to_utf8 (str, len, self->detected_encoding, consumed, &err,self);

    if (!err)
      return ret;

    GST_WARNING_OBJECT (self, "could not convert string from '%s' to UTF-8: %s",self->detected_encoding, err->message);
    SAFE_FREE (self->detected_encoding);
    g_error_free (err);
    err = NULL;
  }

  /* Otherwise check if it's UTF8 */
  if (g_utf8_validate (str, len, NULL)) {
    GST_LOG_OBJECT (self, "valid UTF-8, no conversion needed");
    *consumed = len;
    return g_strndup (str, len);
  }
  GST_INFO_OBJECT (self, "invalid UTF-8!");

  /* Else try fallback */
  encoding = self->encoding;
  if (encoding == NULL || *encoding == '\0') {
    encoding = g_getenv ("GST_SUBTITLE_ENCODING");
  }
  if (encoding == NULL || *encoding == '\0') {
    /* if local encoding is UTF-8 and no encoding specified
     * via the environment variable, assume ISO-8859-15 */
    if (g_get_charset (&encoding)) {
      encoding = "ISO-8859-15";
    }
  }

  ret = convert_to_utf8 (str, len, encoding, consumed, &err,self);

  if (err) {
    GST_WARNING_OBJECT (self, "could not convert string from '%s' to UTF-8: %s",encoding, err->message);
    g_error_free (err);
    err = NULL;
    if(!strcmp(self->encoding,"EUC-KR")) {
      GST_WARNING("failback case is occure with EUC-KR,so going with CP949 ");
      SAFE_FREE(self->encoding);
      self->encoding = g_strdup("CP949");
      encoding = self->encoding;
      ret = convert_to_utf8 (str, len, encoding, consumed, &err,self);
    } else {
      /* invalid input encoding, fall back to ISO-8859-15 (always succeeds) */
      ret = convert_to_utf8 (str, len, "ISO-8859-15", consumed, NULL,self);
    }
  }

  GST_LOG_OBJECT (self,
      "successfully converted %" G_GSIZE_FORMAT " characters from %s to UTF-8"
      "%s", len, encoding, (err) ? " , using ISO-8859-15 as fallback" : "");

  return ret;
}

static gchar *
detect_encoding (const gchar * str, gsize len)
{
  if (len >= 3 && (guint8) str[0] == 0xEF && (guint8) str[1] == 0xBB
      && (guint8) str[2] == 0xBF)
    return g_strdup ("UTF-8");

  if (len >= 2 && (guint8) str[0] == 0xFE && (guint8) str[1] == 0xFF)
    return g_strdup ("UTF-16BE");

  if (len >= 2 && (guint8) str[0] == 0xFF && (guint8) str[1] == 0xFE)
    return g_strdup ("UTF-16LE");

  if (len >= 4 && (guint8) str[0] == 0x00 && (guint8) str[1] == 0x00
      && (guint8) str[2] == 0xFE && (guint8) str[3] == 0xFF)
    return g_strdup ("UTF-32BE");

  if (len >= 4 && (guint8) str[0] == 0xFF && (guint8) str[1] == 0xFE
      && (guint8) str[2] == 0x00 && (guint8) str[3] == 0x00)
    return g_strdup ("UTF-32LE");

  return NULL;
}

/* If language list is not present in smi file, check the body and create our own list */
static gboolean
gst_create_own_language_list (Gstsubmux *self)
{
  gsize consumed = 0;
  guint keyCount = 0;
  gchar* langkey[MAX_LANG];
  gint langKey_length[MAX_LANG];
  FILE *fp=NULL;
  gint i=0;

  SUBMUX_FENTER(self);

  fp = gst_submux_get_external_file_handle(self);
  if (!fp){
    GST_ERROR_OBJECT (self, "Could not get External subtitle handle");
    return FALSE;
  }

  for( i=0;i<MAX_LANG;i++){
    langkey[i]=NULL;
    langKey_length[i]=0;
  }
  gboolean lang_found= FALSE;
  while (!feof (fp) ){
    gchar line[1025];
    guint charCount = 0;
    gboolean conversion = TRUE;
    gchar *result = NULL;
    gchar *con_temp = NULL;
    gchar *delimiter = NULL;
    guint keyLength = 0;

    charCount = fread (line, sizeof(char), 1024, fp);
    line[charCount] = '\0';
    if (!charCount) {
      GST_WARNING_OBJECT (self, "fread returned zero bytes");
      continue;
    }

    if (!self->detected_encoding)
      self->detected_encoding = detect_encoding (line, (gsize)charCount);
      GST_DEBUG_OBJECT (self, "value of detected encoding is %s and self encoding is %s",
        self->detected_encoding,self->encoding);
    if (conversion)
      result = convert_encoding (self, line, charCount, &consumed);

    if (result == NULL) {
      result = line;
      conversion =  FALSE;
    }
    con_temp =  result;

    while (con_temp){
      gchar* tempKey = NULL;

      con_temp =  (gchar*)strcasestr(con_temp,"class=");
      if(con_temp)
        delimiter =  (gchar*)strcasestr(con_temp, ">");

      if (con_temp && (delimiter!=NULL)){
        gchar* tempChar = con_temp + 6;
        while (*tempChar != '>'){
          keyLength++;
          tempChar++;
        }

        tempChar -= keyLength;
        tempKey = (gchar*) g_malloc (keyLength + 1);
        if(!tempKey){
          GST_DEBUG_OBJECT (self, "Failed to get Key");
          goto error;
        }
        gchar* temp1 =tempKey;
        while (*tempChar != '>'){
          *tempKey = *tempChar;
          tempKey++;
          tempChar++;
        }
        tempKey =temp1;
        tempKey[keyLength]='\0';
        int k =0;
        for (k = 0; k < keyCount; k++){
          if(langkey[k]){
            if (!strcasecmp (tempKey,langkey[k])){
              lang_found = TRUE;
              break;
            }
          }
        }
        if(lang_found == FALSE){
          langkey[keyCount] = (gchar*) g_malloc (keyLength);
          if(! langkey[keyCount])
            goto error;
          strcpy(langkey[keyCount],tempKey);
          langKey_length[keyCount]=keyLength;
          keyCount++;
        }
        lang_found =FALSE;
        keyLength =0;
        SAFE_FREE(tempKey);
      } else {
        keyLength =0;
        lang_found =FALSE;
        break;
      }
      con_temp+=6;
    }
  }

  for(i=0;i<keyCount;i++) {
    if(langkey[i]) {
      GstLangStruct *new = g_new0 (GstLangStruct, 1);
      GST_DEBUG_OBJECT (self, "Adding ign case to the langKey keyCount %d and lang %s ",i, langkey[i]);
      new->language_code = (gchar*)malloc (3);
      if(!(new->language_code)){
        GST_DEBUG_OBJECT (self, "Failed to allocate language code");
        goto error;
      }
      gchar *attr_val=new->language_code ;
      strcpy (attr_val, "un");
      attr_val[2]='\0';

      new->language_key = (gchar*) malloc (langKey_length[i] + 1);
      if(!(new->language_key)){
        GST_DEBUG_OBJECT (self, "Failed to allocate language key");
        goto error;
      }
      strcpy (new->language_key, langkey[i]);
      self->priv->lang_list = g_list_append (self->priv->lang_list, new);
      SAFE_FREE(langkey[i]);
    }
  }
  if (fp){
    fclose(fp);
  }
  SUBMUX_FLEAVE(self);
  return TRUE;
  error:
  GST_DEBUG_OBJECT (self, "Returning in error case");
  if (fp){
    fclose(fp);
  }
  return FALSE;
}

gboolean
validate_langlist_body(GList * lang_list, Gstsubmux * self)
{
  gchar   line[1025];
  FILE  * fp = NULL;
  guint i = 0, found_count = 0;
  const guint list_len = g_list_length(lang_list);
  gboolean counter[MAX_LANGUAGE];
  struct LangStruct
  {
      gchar *language_code;
      gchar *language_key;
  } * lang;

  SUBMUX_FENTER(self);

  fp = gst_submux_get_external_file_handle(self);
  if (!fp){
    GST_ERROR_OBJECT (self, "Could not get External subtitle handle");
    return FALSE;
  }

  for (i = 0; i < list_len; i++){
    counter[i] = FALSE;
  }

  while (!feof (fp) && found_count < list_len){
    gsize consumed = 0;
    gint gap = 0;
    guint charCount = 0;
    gchar* result = NULL;
    gchar* temp = NULL;
    gchar* temp_lang = NULL;
    gchar * temp1 = NULL;
    gchar *con_temp_lang = NULL;
    gchar *con_temp = NULL;
    gboolean conversion = TRUE;
    charCount = fread (line, sizeof(char), 1024, fp);
    line[charCount] = '\0';
    if (!charCount) {
      GST_WARNING_OBJECT (self, "fread returned zero bytes");
      continue;
    }

    if (!self->detected_encoding)
      self->detected_encoding = detect_encoding (line, (gsize)charCount);
      GST_DEBUG_OBJECT (self, "Value of detected encoding is %s and self encoding is %s",
                           self->detected_encoding,self->encoding);
    if (conversion){
      result = convert_encoding (self, line, charCount, &consumed);
    }
    if (result == NULL) {
       result = line;
       conversion =  FALSE;
    }
    con_temp = g_utf8_strdown (result, strlen (result));
    temp = con_temp;
    while (con_temp) {
      con_temp = g_strstr_len(con_temp, strlen (con_temp), "class=");
      if (con_temp) {
        temp1 = g_strstr_len(con_temp+1, strlen (con_temp), "class=");
      }
      if (temp1 && con_temp){
        gap = strlen (con_temp) - strlen (temp1);
      } else if (con_temp) {
        gap = strlen (con_temp);
      } else {
        continue;
      }
      if (con_temp){
        for (i = 0; i < list_len; i++){
          if (counter[i] == TRUE) {
            con_temp = con_temp + 1;
            continue;
          }
          lang = (struct LangStruct *) g_list_nth_data (lang_list, i);
          if (lang && lang->language_key) {
            temp_lang = (gchar*)g_malloc (strlen (lang->language_key) + 1);
            strcpy (temp_lang, lang->language_key);
            con_temp_lang = g_utf8_strdown (temp_lang, strlen (temp_lang));
            if (g_strstr_len (con_temp, gap, con_temp_lang)) {
              found_count++;
              counter[i] = TRUE;
              con_temp = con_temp + 1;
            }
/* Fix Me: Cases where there is no body for a specific language
* inside a single language .smi file */
#if 0
            else {
              con_temp_start = con_temp;
              con_temp_end = con_temp;
              while(con_temp_end) {
                if(*con_temp_end == '=') {
                  con_temp_start = con_temp_end+1;
                  con_temp_end++;
                }else if(*con_temp_end == '>') {
                  con_temp_end = con_temp_end;
                  new_key_found = TRUE;
                  break;
                }else {
                  con_temp_end++;
                  new_key_found = FALSE;
                }
              }
              if(new_key_found) {
                new_key_length = strlen(con_temp_start)-strlen(con_temp_end);
                new_key = g_malloc(new_key_length +1);
                for(k=0;k<new_key_length;k++){
                  *(new_key+k)=*(con_temp_start+k);
                }
                *(new_key+new_key_length)='\0';
                GST_INFO("new lang key is %s",lang->language_key);
                g_free(new_key);
                found_count++;
                counter[i] = TRUE;
                con_temp = con_temp + 1;
              }
            }
#endif
            SAFE_FREE (temp_lang);
            SAFE_FREE (con_temp_lang);
          }
        }
      }
    }
    if (conversion){
      SAFE_FREE (result);
    }
    SAFE_FREE (temp);
  }

  if (found_count < list_len) {
    for (i = 0; i < list_len; i++) {
      if (counter[i] == FALSE) {
        lang_list = g_list_delete_link (lang_list, g_list_nth (lang_list, i));
      }
    }
  }

  fclose (fp);
  SUBMUX_FLEAVE(self);
  return TRUE;
}

/*
**
**  Description    : Chain function used to push the subtitle buffer to internal pipelines of submux element
**  Params        : (1) sink pad on which buffer is arriving (2) the buffer itself
**  return        : GST_FLOW_OK on successfully pushing subtitle buffer to next element
**
*/
static GstFlowReturn
gst_submux_chain(GstPad *pad, GstObject * parent, GstBuffer *buffer)
{
  guint length = 0;
  guint i=0;
  GstPad *checkpad = NULL;
  Gstsubmux *submux = GST_SUBMUX(GST_PAD_PARENT(pad));
  gboolean ret = FALSE;
  GstFlowReturn fret = GST_FLOW_ERROR;
  GstSubmuxStream *stream = NULL;
  GstMessage *m = NULL;
  GstMemory *buf_memory = NULL;
  GstMapInfo buf_info = GST_MAP_INFO_INIT;
  if (GST_BUFFER_IS_DISCONT (buffer)){
    GST_DEBUG_OBJECT(submux, "Discont buffer came in chain function");
  }

    buf_memory = gst_buffer_peek_memory(buffer, 0);
    gst_memory_map(buf_memory, &buf_info, GST_MAP_READ);
    unsigned char *data = (unsigned char *)buf_info.data;
    gst_memory_unmap(buf_memory, &buf_info);

  if (!submux->priv->is_internal) {
    if (!submux->priv->first_buffer) {
      if (!submux->detected_encoding) {
        submux->detected_encoding = detect_encoding ((gchar*)data, gst_buffer_get_size (buffer));
      }
    }

    if (!submux->langlist_msg_posted && submux->priv->lang_list) {
      GList* temp_list_to_post = NULL;
      temp_list_to_post = g_list_copy (submux->priv->lang_list);
      m = gst_message_new_element (GST_OBJECT_CAST (submux), gst_structure_new("Ext_Sub_Language_List",
                                  "lang_list", G_TYPE_POINTER, temp_list_to_post, NULL));
      gst_element_post_message (GST_ELEMENT_CAST (submux), m);
      submux->langlist_msg_posted = TRUE;
      GST_WARNING_OBJECT(submux, "Language List Posted");
    }

    if (submux->need_segment) {
      ret = gst_pad_push_event (submux->srcpad, gst_event_new_segment (&submux->segment));
      GST_DEBUG_OBJECT (submux, "pushing newsegment event with %" GST_SEGMENT_FORMAT, &submux->segment);
      if (!ret) {
        GST_WARNING_OBJECT (submux, "Sending newsegment to next element is failed");
        return GST_FLOW_FLUSHING;
      }
      GST_DEBUG_OBJECT (submux, "Starting the loop again");
      if (!gst_pad_start_task (submux->srcpad, (GstTaskFunction) gst_submux_loop, submux,NULL))
      {
         GST_ERROR_OBJECT (submux, "failed to start srcpad task...");
         GST_ELEMENT_ERROR (submux, RESOURCE, FAILED, ("failed to create  push loop"), (NULL));
         return GST_FLOW_ERROR;
      }else{
        submux->srcpad_loop_running = TRUE;
      }
      submux->need_segment = FALSE;
    }

    if (!submux->priv->lang_list && submux->priv->parser_type == GST_SUB_PARSE_FORMAT_SAMI) {
      GST_WARNING_OBJECT (submux, "Language list not available for SAMI file");
      return GST_FLOW_FLUSHING;
    }

    if (submux->priv->is_internal) {
      length = submux->sinkpads_count;
      if(!length){
        GST_WARNING_OBJECT (submux, "Streams are not innitialized yet");
        return GST_FLOW_FLUSHING;
      }
    }else{
      length = submux->priv->stream_count;
      if(submux->priv->parser_type != GST_SUB_PARSE_FORMAT_UNKNOWN && !length){
        GST_WARNING_OBJECT (submux, "Streams are not innitialized yet");
        return GST_FLOW_FLUSHING;
      }
    }

    for (i = 0; i < length; i++) {
      stream = g_list_nth_data(submux->streams, i);
      if (stream){
        if ((submux->priv->parser_type == GST_SUB_PARSE_FORMAT_SAMI) && !submux->priv->first_buffer){
          GstLangStruct *lang = g_list_nth_data(submux->priv->lang_list, i);
          if(submux->msl_streams){
            GstLangStruct *lang1 = g_list_nth_data(submux->msl_streams, i);
            lang->active = lang1->active;
          } else {
            if (i == 0)
              lang->active = TRUE;
            else
              lang->active = FALSE;
          }
        }
        stream->need_segment = FALSE;
      }else{
        /*Seems like we failed to create the stream or deleted it somewhere-in any case ignore this index*/
        GST_WARNING_OBJECT (submux, "stream[%d] not found",i);
        return GST_FLOW_FLUSHING;
      }
    }

    for (i = 0; i < length; i++) {
      stream = g_list_nth_data(submux->streams, i);
      if(stream && stream->pipe_struc.appsrc){
        if (i < (length - 1)){
          gst_buffer_ref (buffer);
        }

        fret = gst_app_src_push_buffer ((GstAppSrc*)stream->pipe_struc.appsrc, buffer);
        if (fret != GST_FLOW_OK) {
          GST_ERROR_OBJECT (submux, "push buffer failed with fret is %d", fret);
          return fret;
        }
      }else{
        /*Seems like we failed to create the stream or deleted it somewhere-in any case ignore this index*/
        GST_WARNING_OBJECT (submux, "stream[%d] not found",i);
        return GST_FLOW_FLUSHING;
      }
    }
  } else {
    length = submux->sinkpads_count;
    checkpad = (GstPad *) g_list_nth_data (submux->sinkpad, 0);
    if (checkpad == pad) {
      if (submux->need_segment) {
        ret = gst_pad_push_event (submux->srcpad, gst_event_new_segment (&submux->segment));
        GST_DEBUG_OBJECT (submux, "pushing newsegment event with %" GST_SEGMENT_FORMAT, &submux->segment);
        if (!ret) {
          GST_ERROR_OBJECT (submux, "Sending newsegment to next element is failed");
          return GST_FLOW_ERROR;
        }
        GST_DEBUG_OBJECT (submux, "Starting the loop again");
        if (!gst_pad_start_task (submux->srcpad, (GstTaskFunction) gst_submux_loop, submux,NULL))
        {
          GST_ERROR_OBJECT (submux, "failed to start srcpad task...");
          GST_ELEMENT_ERROR (submux, RESOURCE, FAILED, ("failed to create  push loop"), (NULL));
          return GST_FLOW_ERROR;
        }else{
          submux->srcpad_loop_running = TRUE;
        }
        submux->need_segment = FALSE;
      }
    }
    for (i = 0; i < length; i++) {
      checkpad = (GstPad *) g_list_nth_data(submux->sinkpad, i);
      if (checkpad == pad) {
        stream = g_list_nth_data (submux->streams, i);
        if (!stream) {
          GST_ERROR_OBJECT (submux, "Stream not available...");
          return GST_FLOW_ERROR;
        }
        if (stream->flushing){
          GST_DEBUG_OBJECT (submux, "flushing going on in appsink");
          return GST_FLOW_OK ;
        }

        g_mutex_lock (&stream->queue_lock);
        g_queue_push_tail (stream->queue, buffer);
        g_cond_signal (&stream->queue_empty);
        g_mutex_unlock (&stream->queue_lock);
        fret = GST_FLOW_OK;
        break;
      }
    }
  }

  if (!submux->priv->first_buffer) {
    GST_DEBUG_OBJECT (submux, "Got the first buffer");
    submux->priv->first_buffer = TRUE;
  }
  return fret;
}

static void
gst_submux_stream_deinit_queue (GstSubmuxStream *stream,Gstsubmux *submux)
{
  GstBuffer *buf = NULL;
  SUBMUX_FENTER(submux);
  if (stream) {
    if (stream->queue) {
      while (!g_queue_is_empty (stream->queue)) {
        buf = g_queue_pop_head (stream->queue);
        gst_buffer_unref (buf);
        buf = NULL;
      }
      g_queue_free (stream->queue);
      stream->queue = NULL;
    }
  }
  SUBMUX_FLEAVE(submux);
}

/* stream_denit */
static void
gst_submux_stream_deinit (GstSubmuxStream *stream,Gstsubmux *submux)
{
  GstBuffer *buf = NULL;
  SUBMUX_FENTER(submux);
  if (stream) {
    if (stream->queue) {
      while (!g_queue_is_empty (stream->queue)) {
        buf = g_queue_pop_head (stream->queue);
        gst_buffer_unref (buf);
        buf = NULL;
      }
      g_queue_free (stream->queue);
      stream->queue = NULL;
    }

    if (stream->pipe_struc.pipe) {
      gst_element_set_state (stream->pipe_struc.pipe, GST_STATE_NULL);
      gst_element_get_state (stream->pipe_struc.pipe, NULL, NULL, GST_CLOCK_TIME_NONE);
      gst_object_unref (stream->pipe_struc.pipe);
      stream->pipe_struc.pipe = NULL;
    }

    if (stream->queue_lock.p) {
      g_cond_broadcast (&stream->queue_empty);
      g_mutex_clear (&stream->queue_lock);
      stream->queue_lock.p = NULL;
    }

    g_cond_clear (&stream->queue_empty);

    SAFE_FREE (stream);
  }
  SUBMUX_FLEAVE(submux);
  return;
}

/* releasing the requested pad */
static void
gst_submux_release_pad (GstElement * element, GstPad * pad)
{
  Gstsubmux *submux = GST_SUBMUX_CAST (element);
  GstPad *check_pad;
  int i=0;
  guint length;
  SUBMUX_FENTER(submux);
  length = g_list_length(submux->sinkpad);
  GST_DEBUG_OBJECT (element, "Releasing %s:%s", GST_DEBUG_PAD_NAME (pad));

  for (i=1;i<=length;i++) {
    check_pad = (GstPad *) g_list_nth_data(submux->sinkpad,i);
    if (check_pad == pad) {
      /* this is it, remove */
      submux->sinkpad = g_list_delete_link (submux->sinkpad, g_list_nth_data(submux->sinkpad,i));
      gst_element_remove_pad (element, pad);
      break;
    }
  }
  SUBMUX_FLEAVE(submux);
  return;
}

/* request new pad */
static GstPad *
gst_submux_request_new_pad (GstElement * element,
       GstPadTemplate * templ, const gchar * req_name, const GstCaps * caps)
{
  GstElementClass *klass = GST_ELEMENT_GET_CLASS (element);
  Gstsubmux *submux = GST_SUBMUX_CAST (element);
  GstPad *newpad = NULL;
  gchar *name = NULL;

  if (templ->direction != GST_PAD_SINK) {
    GST_ERROR_OBJECT (submux, "templ direction is not sinkpad, returning from here");
    goto wrong_direction;
  }

  if (templ == gst_element_class_get_pad_template (klass, "sink_%u")) {
    name = g_strdup_printf ("sink_%u", submux->sinkpads_count++);
  }

  GST_DEBUG_OBJECT (submux, "Requested pad: %s", name);
  /* create pad and add to collections */
  newpad = gst_pad_new_from_template (templ, name);
  SAFE_FREE (name);
  if(!submux->priv->is_internal) {
    submux->external_sinkpad = TRUE;
  }
  submux->sinkpad = g_list_append (submux->sinkpad, newpad);
  /* set up pad */

  gst_pad_set_event_function (newpad, GST_DEBUG_FUNCPTR (gst_submux_handle_sink_event));
  gst_pad_set_chain_function(newpad, GST_DEBUG_FUNCPTR (gst_submux_chain));
  gst_pad_set_active (newpad, TRUE);
  gst_element_add_pad (element, newpad);

  return newpad;

/* ERRORS */
wrong_direction:
  GST_WARNING_OBJECT (submux, "Request pad that is not a SINK pad.");
  return NULL;
}

static gboolean
gst_submux_handle_src_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
  Gstsubmux *submux = GST_SUBMUX(GST_PAD_PARENT(pad));
  gboolean ret = FALSE;
  gboolean update;

  GST_DEBUG_OBJECT (submux, "Handling %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    /* this event indicates speed change or seek */
    case GST_EVENT_SEEK:
    {
      GstFormat format;
      GstSeekType start_type, stop_type;
      gint64 start, stop;
      gdouble rate;
      GstPad *sinkpad = (GstPad *) g_list_nth_data (submux->sinkpad, 0);
      gst_event_parse_seek (event, &rate, &format, &submux->segment_flags,
                             &start_type, &start, &stop_type, &stop);

#ifdef DISABLE_FAST_SEEK
      if((submux->segment_flags & GST_SEEK_FLAG_ACCURATE) ||
          submux->seek_new_file){
        GST_DEBUG_OBJECT (submux, "Subtitle seek happens seek_new_file(%d)",submux->seek_new_file);
        submux->seek_new_file = FALSE;
      }else{
        GST_DEBUG_OBJECT (submux, "Ignore seek event !!");
        gst_event_unref (event);
        return TRUE ;
      }
#endif
      gst_segment_do_seek (&submux->segment, rate, format, submux->segment_flags,
                             start_type, start, stop_type, stop, &update);
      if (!submux->priv->is_internal) {
        ret = gst_pad_push_event (sinkpad, gst_event_new_seek (rate, GST_FORMAT_BYTES, submux->segment_flags,
                                  GST_SEEK_TYPE_SET, 0, GST_SEEK_TYPE_NONE, 0));
        gst_event_unref (event);
      } else {
        GST_DEBUG_OBJECT (submux, "Handling seek in case of internal");
        ret = gst_pad_event_default (pad, parent, event);
      }

      if (!ret) {
        GST_ERROR_OBJECT (submux, "Sending seek event to sink pad failed");
        break;
      }
      break;
    }
    default:
    {
      ret = gst_pad_event_default (pad, parent, event);
      break;
    }
  }
  return ret;
}

static gboolean
gst_submux_handle_sink_event (GstPad * pad , GstObject * parent, GstEvent * event)
{
  Gstsubmux *submux = GST_SUBMUX (GST_PAD_PARENT (pad));
  gboolean ret = TRUE;
  guint length = 0;
  GstBuffer *buf = NULL;
  GstPad *checkpad = NULL;
  gint i = 0;
  GstSubmuxStream *cur_stream = NULL;
  GstMapInfo buf_info = GST_MAP_INFO_INIT;

  GST_DEBUG_OBJECT (submux, "Handling %s event", GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
    {
      length = g_list_length (submux->sinkpad);
      GST_OBJECT_LOCK (submux);
      submux->need_segment = FALSE;
      for (i = 0; i < length; i++) {
        GST_DEBUG_OBJECT(submux, "Inside the handling of EOS event for stream [%d]",i);
        cur_stream = g_list_nth_data(submux->streams,i);
        if (cur_stream && !cur_stream->eos_sent) {
          cur_stream->need_segment = FALSE;
          GstBuffer *buf = gst_buffer_new_and_alloc (3 + 1);
          GST_DEBUG_OBJECT(submux, "Sending EOS buffer to chain");
          gst_buffer_map (buf, &buf_info, GST_MAP_READWRITE);
          buf_info.data[0] = 'e';
          buf_info.data[1] = 'o';
          buf_info.data[2] = 's';
          buf_info.data[3] = '\0';
          gst_buffer_unmap (buf, &buf_info);
          GST_BUFFER_FLAG_SET(buf,GST_BUFFER_FLAG_GAP);
          gst_submux_chain (g_list_nth_data(submux->sinkpad,i),parent,  buf);
          cur_stream->eos_sent = TRUE;
        }else{
          GST_DEBUG_OBJECT(submux, "No stream is present so stop the loop");
          submux->stop_loop = TRUE;
        }
      }
      GST_OBJECT_UNLOCK (submux);
      gst_event_unref(event);
      break;
    }
    case GST_EVENT_SEGMENT:
    {
      const GstSegment *segment;
      gint external_file_status = EXTERNAL_FILE_PATH_UNKNOWN;

      GST_OBJECT_LOCK (submux);

      if (!submux->pipeline_made) {
        if (!submux->priv->is_internal) {
          external_file_status = gst_submux_external_file_status(submux);
          if (external_file_status != EXTERNAL_FILE_PATH_UNCHANGED) {
            if (!gst_submux_format_autodetect (submux)) {
              GST_ERROR_OBJECT (submux, "Auto Format detection failed");
              GST_OBJECT_UNLOCK (submux);
              return FALSE;
            }
            if (!gst_calculate_number_languages(submux)) {
              GST_ERROR_OBJECT (submux, "Failed to calculate number of languages");
              GST_OBJECT_UNLOCK (submux);
              return FALSE;
            }
            if (!validate_langlist_body (submux->priv->lang_list, submux)) {
              GST_WARNING_OBJECT(submux, "Error occured while validating language list. Will post without validation");
            }
          }
          if (!submux->priv->is_internal) {
            if (submux->priv->parser_type == GST_SUB_PARSE_FORMAT_UNKNOWN) {
              GST_ERROR_OBJECT (submux, "Parser Type not detected");
              GST_OBJECT_UNLOCK (submux);
              return FALSE;
            }
            if (submux->priv->parser_type == GST_SUB_PARSE_FORMAT_SAMI && !submux->priv->lang_list) {
              GST_ERROR_OBJECT (submux, "Language list not innitialized");
              GST_OBJECT_UNLOCK (submux);
              return FALSE;
            }
          }
        }
        if (external_file_status != EXTERNAL_FILE_PATH_CHANGED || submux->priv->is_internal) {
          /*Not creating submux pipeline until seek to new file is completed*/
          if (!gst_submux_create_pipelines (submux, pad)) {
            GST_ERROR_OBJECT (submux, "Failed to create pipelines");
            GST_OBJECT_UNLOCK (submux);
            return FALSE;
          }
        }
      }

      if (!submux->priv->is_internal) {
        if (submux->pipeline_made) {
          gst_event_unref(event);
          length = g_list_length(submux->streams);
          for (i = 0; i < length; i++) {
            cur_stream = g_list_nth_data(submux->streams,i);
            if (!cur_stream->pipe_struc.pipe) {
              GST_ERROR_OBJECT (submux, "Pipeline is null");
              GST_OBJECT_UNLOCK (submux);
              return FALSE;
            }
            cur_stream = g_list_nth_data(submux->streams,i);
            cur_stream->seek_ts = submux->segment.start;
            if(!gst_pad_push_event (submux->srcpad, gst_event_new_segment (&submux->segment)))
            {
              GST_ERROR_OBJECT(submux, "Sending %s event for stream[%d] failed",GST_EVENT_TYPE_NAME (event), i);
              GST_OBJECT_UNLOCK (submux);
              ret = FALSE;
              break;
            }
          }
          submux->need_segment = TRUE;
        }
      } else {
        length = g_list_length (submux->sinkpad);
        if (length ==  g_list_length (submux->streams) && submux->need_segment) {
          for (i = 0; i < length; i++) {
            cur_stream = g_list_nth_data(submux->streams, i);
            if (cur_stream->need_segment) {
              gst_event_parse_segment (event, &segment);
              GST_DEBUG_OBJECT (submux, "Got NEWSEGMENT event %" GST_SEGMENT_FORMAT,segment);
              gst_segment_copy_into (segment, &submux->segment);
              cur_stream->need_segment = FALSE;
            }
          }
          submux->need_segment = TRUE;
          gst_event_unref(event);
        }
      }
      GST_OBJECT_UNLOCK (submux);
      break;
    }
    case GST_EVENT_FLUSH_START:
    {
      length = g_list_length(submux->streams);
      if (!submux->priv->is_internal) {
        gst_event_unref(event);
        if(!gst_pad_event_default (pad, parent, gst_event_new_flush_start ()))
        {
            GST_ERROR_OBJECT(submux, "Sending %s event for stream[%d] failed",GST_EVENT_TYPE_NAME (event), i);
            ret = FALSE;
        }

        for (i = 0;i < length;i++) {
          cur_stream = g_list_nth_data(submux->streams,i);
          cur_stream->flushing = TRUE;
          cur_stream->discont_came = FALSE;
          cur_stream->eos_came = FALSE;
          cur_stream->eos_sent = FALSE;
        }

        for (i = 0; i < length; i++) {
          cur_stream = g_list_nth_data(submux->streams,i);
          if(!gst_element_send_event(cur_stream->pipe_struc.appsrc,gst_event_new_flush_start ())){
            GST_ERROR_OBJECT(submux, "Sending flush start event to appsrc for stream[%d] failed", i);
            ret = FALSE;
          }
          if(!gst_element_send_event(cur_stream->pipe_struc.appsrc,gst_event_new_flush_stop (TRUE)))
          {
            GST_ERROR_OBJECT(submux, "Sending flush stop event to appsrc for stream[%d] failed", i);
            ret = FALSE;
          }
          if(!gst_element_send_event(cur_stream->pipe_struc.appsrc,gst_event_new_eos ())){
            GST_ERROR_OBJECT(submux, "Sending eos event to appsrc for stream[%d] failed", i);
            ret = FALSE;
          }
          submux->flushing =TRUE;
          g_mutex_lock (&cur_stream->queue_lock);
          g_cond_signal (&cur_stream->queue_empty);
          g_mutex_unlock (&cur_stream->queue_lock);
          cur_stream->flush_done = TRUE;
        }

        if (submux && GST_PAD_TASK(submux->srcpad)) {
          GST_PAD_STREAM_LOCK (submux->srcpad);
          GST_PAD_STREAM_UNLOCK (submux->srcpad);
        }

        /*changes for new design*/
        for (i = 0; i < submux->priv->stream_count; i++) {
          cur_stream = g_list_nth_data (submux->streams, i);
          gst_submux_stream_deinit(cur_stream,submux);
        }
        g_list_free(submux->streams);
        submux->streams = NULL;
        gst_submux_deinit_private_values (submux,FALSE);

        submux->stop_loop = FALSE;
        submux->need_segment = TRUE;
        submux->langlist_msg_posted = FALSE;
      } else {
        gst_event_unref (event);
        submux->flushing = TRUE;
        checkpad = (GstPad *) g_list_nth_data (submux->sinkpad, length - 1);
        if (checkpad == pad) {
          if(!gst_pad_event_default (pad, parent, gst_event_new_flush_start ()))
          {
            GST_ERROR_OBJECT(submux, "Sending flush start event to sinkpad failed");
            ret = FALSE;
          }

          for (i = 0; i < length; i++) {
            cur_stream = g_list_nth_data(submux->streams, i);
            cur_stream->flushing = TRUE;
            submux->flushing = TRUE;
            g_mutex_lock (&cur_stream->queue_lock);
            while (!g_queue_is_empty (cur_stream->queue)) {
              buf = g_queue_pop_head (cur_stream->queue);
              gst_buffer_unref (buf);
            }
            g_queue_clear (cur_stream->queue);
            g_cond_signal (&cur_stream->queue_empty);
            g_mutex_unlock (&cur_stream->queue_lock);
            cur_stream->eos_came = FALSE;
            cur_stream->eos_sent = FALSE;
          }

          if (submux && GST_PAD_TASK (submux->srcpad)) {
            GST_PAD_STREAM_LOCK (submux->srcpad);
            GST_PAD_STREAM_UNLOCK (submux->srcpad);
          }
        }
      }
      break;
    }
    case GST_EVENT_FLUSH_STOP:
    {
      gst_event_unref(event);
      if (!submux->priv->is_internal) {
        guint idx = 0;
        submux->flushing = FALSE;
        if(!gst_pad_event_default (pad, parent, gst_event_new_flush_stop (TRUE)))
        {
          GST_ERROR_OBJECT (submux, "sending flush-stop event to sinkpad pad failed");
          ret = FALSE;
        }
        for (idx = 0; idx < submux->priv->stream_count; idx++) {
          submux->cur_buf_array[idx] = NULL;
        }
      } else {
        length = g_list_length(submux->streams);
        checkpad = (GstPad *) g_list_nth_data (submux->sinkpad, length - 1);
        if (checkpad == pad) {
          for (i = 0; i < length; i++) {
            cur_stream = g_list_nth_data(submux->streams, i);
            cur_stream->need_segment = TRUE;
            submux->cur_buf_array[i] = NULL;
            submux->need_segment = TRUE;
            submux->stop_loop = FALSE;
            submux->flushing = FALSE;
            cur_stream->flushing = FALSE;
          }
          if(!gst_pad_event_default (pad, parent, gst_event_new_flush_stop (TRUE)))
          {
            GST_ERROR_OBJECT (submux, "sending flush-stop event to sinkpad failed");
            ret = FALSE;
          }
        }
      }
      break;
    }
    default:{
      if (!submux->priv->is_internal) {
        if(!gst_pad_event_default (pad, parent, event))
        {
            GST_ERROR_OBJECT (submux, "sending %s event to sinkpad pad failed",GST_EVENT_TYPE_NAME (event));
            ret = FALSE;
        }
      } else {
        checkpad = (GstPad *) g_list_nth_data (submux->sinkpad, length - 1);
        if (checkpad == pad) {
          if(!gst_pad_event_default (pad, parent, event))
          {
              GST_ERROR_OBJECT (submux, "sending %s event to sinkpad pad failed",GST_EVENT_TYPE_NAME (event));
              ret = FALSE;
          }
        }
      }
      break;
    }
  }
  GST_INFO_OBJECT (submux, "Event result: %d", ret);
  return ret;
}

static gboolean
gst_submux_is_muxing_needed (GstBuffer *ref_buffer, GstBuffer *cur_buf)
{
  GstClockTime ref_start = GST_BUFFER_PTS(ref_buffer);
  GstClockTime ref_stop = GST_BUFFER_PTS(ref_buffer) + GST_BUFFER_DURATION(ref_buffer);
  GstClockTime start = GST_BUFFER_PTS(cur_buf);
  GstClockTime stop = GST_BUFFER_PTS(cur_buf) + GST_BUFFER_DURATION(cur_buf);
  /* if we have a stop position and a valid start and start is bigger,
   * we're outside of the segment */
  if (G_UNLIKELY (ref_stop != -1 && start != -1 && start >= ref_stop))
    return FALSE;

  /* if a stop position is given and is before the segment start,
   * we're outside of the segment. Special case is were start
   * and stop are equal to the segment start. In that case we
   * are inside the segment. */
  if (G_UNLIKELY (stop != -1 && (stop < ref_start || (start != stop && stop == ref_start))))
    return FALSE;

  return TRUE;
}

/* This function do the actual muxing of buffer on the basis of timestamps */
static GList*
gst_submux_muxing (Gstsubmux *submux)
{
  GstClockTime min_timestamp = 0;
  int min_stream = 0;
  int overlap = 0;
  GstClockTime next_min_time = 0;
  int idx = 0;
  GList *push_list = NULL;
  GstMemory *buf_memory = NULL;
  GstMapInfo buf_info = GST_MAP_INFO_INIT;
  /* Finding least timestamp of all streams and their stream ID */
  for (idx = 0; idx < submux->priv->stream_count; idx++) {
    if(submux->cur_buf_array[idx] && !min_timestamp) {
      min_timestamp = GST_BUFFER_PTS(submux->cur_buf_array[idx]);
      min_stream = idx;
    }
    if(submux->cur_buf_array[idx] && (GST_BUFFER_PTS(submux->cur_buf_array[idx]) < min_timestamp)){
      min_timestamp = GST_BUFFER_PTS(submux->cur_buf_array[idx]);
      min_stream = idx;
    }
  }

  /* Finding overlap buffers and next least timestamp */
  for (idx = 0; idx < submux->priv->stream_count; idx++) {
    if(submux->cur_buf_array[idx] && (idx != min_stream) && (!next_min_time)) {
      next_min_time = GST_BUFFER_PTS(submux->cur_buf_array[idx]);
    }
    if(submux->cur_buf_array[idx] && (idx != min_stream)) {
      if(gst_submux_is_muxing_needed (submux->cur_buf_array[min_stream], submux->cur_buf_array[idx])) {
        overlap = overlap | (1<<idx);      // bit setting of overlap variable with stream ID
        if(GST_BUFFER_PTS(submux->cur_buf_array[idx]) < next_min_time)
          next_min_time = GST_BUFFER_PTS(submux->cur_buf_array[idx]);
      }
    }
  }

  /* If no overlap send buffer as it is */
  if(!overlap) {
    buf_memory = gst_buffer_peek_memory(submux->cur_buf_array[min_stream], 0);
    gst_memory_map(buf_memory, &buf_info, GST_MAP_READ);
    unsigned char *data = (unsigned char *)buf_info.data;
    gst_memory_unmap(buf_memory, &buf_info);
    if(submux->cur_buf_array[min_stream] && data)
    {
       push_list = g_list_append(push_list, submux->cur_buf_array[min_stream]);
       submux->cur_buf_array[min_stream] = NULL;
     }else{
       GST_ERROR_OBJECT (submux, "not pushing string: as null buffer encounteredl");
     }
  }else{
    GstBuffer *push_buf = NULL;
    GstClockTime stop_time = 0;
    int stop_idx = 0;
    GstBuffer *overlap_buf = NULL;
    guint overlap_buf_length = 0;
    GString *overlap_text = NULL;
    gchar *text = NULL;
    if(next_min_time > GST_BUFFER_PTS(submux->cur_buf_array[min_stream])) {
      push_buf = gst_buffer_copy (submux->cur_buf_array[min_stream]);
      push_buf->duration = next_min_time - GST_BUFFER_PTS(submux->cur_buf_array[min_stream]);
      GST_BUFFER_PTS(submux->cur_buf_array[min_stream]) = next_min_time;
      GST_BUFFER_DURATION(submux->cur_buf_array[min_stream]) -= push_buf->duration;
      min_timestamp = next_min_time;
      push_list = g_list_append(push_list, push_buf);
    }

    for (idx = 0; idx < submux->priv->stream_count; idx++) {
      if(submux->cur_buf_array[idx] && !stop_time) {
        stop_time = GST_BUFFER_PTS(submux->cur_buf_array[idx]) + GST_BUFFER_DURATION(submux->cur_buf_array[idx]);
        stop_idx = idx;
      }
      if(submux->cur_buf_array[idx] &&
          ((GST_BUFFER_PTS(submux->cur_buf_array[idx])+ GST_BUFFER_DURATION(submux->cur_buf_array[idx])) < stop_time)) {
        stop_time = GST_BUFFER_PTS(submux->cur_buf_array[idx]) + GST_BUFFER_DURATION(submux->cur_buf_array[idx]);
        stop_idx = idx;
      }
    }
    overlap_text = g_string_new ("");
    overlap = overlap | (1<<min_stream);
    for (idx = 0; idx < submux->priv->stream_count; idx++) {
      int finder = 1<<idx;
      if(overlap & finder) {
        buf_memory = gst_buffer_peek_memory(submux->cur_buf_array[idx], 0);
        gst_memory_map(buf_memory, &buf_info, GST_MAP_READ);
        unsigned char *data = (unsigned char *)buf_info.data;
        gst_memory_unmap(buf_memory, &buf_info);
        g_string_append (overlap_text, (gchar*)data);
        GST_BUFFER_PTS(submux->cur_buf_array[idx])+= (stop_time - min_timestamp);
        GST_BUFFER_DURATION(submux->cur_buf_array[idx])-= (stop_time - min_timestamp);
        if(overlap > (1<<(idx+1))) g_string_append_c (overlap_text, '\n');
      }
    }
    text = g_string_free (overlap_text, FALSE);
    overlap_buf_length = strlen(text);
    overlap_buf = gst_buffer_new_and_alloc (overlap_buf_length + 1);
    gst_buffer_map (overlap_buf, &buf_info, GST_MAP_READWRITE);
    memcpy (buf_info.data, text, overlap_buf_length + 1);
    gst_buffer_unmap (overlap_buf, &buf_info);
    overlap_buf->pts = min_timestamp;
    overlap_buf->duration = stop_time - min_timestamp;
    SAFE_FREE (text);
    submux->cur_buf_array[stop_idx] = NULL;
    push_list = g_list_append(push_list, overlap_buf);
    for (idx = 0; idx < submux->priv->stream_count; idx++) {
      if(submux->cur_buf_array[idx] &&
           ((GST_BUFFER_PTS(submux->cur_buf_array[idx])+ GST_BUFFER_DURATION(submux->cur_buf_array[idx])) <= stop_time))
      {
        submux->cur_buf_array[idx] = NULL;
      }
    }
  }
  return push_list;
}

static void gst_submux_loop (Gstsubmux *submux)
{
  guint length = 0;
  GstBuffer *check_buffer = NULL;
  GstSubmuxStream *cur_stream = NULL;
  GstSubmuxStream *check_stream = NULL;
  GstFlowReturn fret = GST_FLOW_OK;
  gboolean eos = TRUE;
  guint i= 0,k = 0;
  GList *push_list = NULL;
  GstBuffer *push_buf = NULL;
  GstMemory *buf_memory = NULL;
  GstMapInfo buf_info = GST_MAP_INFO_INIT;
  if (!submux->priv->first_buffer) {
    if(submux->stop_loop || submux->flushing){
      GST_ERROR_OBJECT (submux, "Loop stopped before first buffer stop_loop[%d] flushing[%d]",submux->stop_loop,submux->flushing);
      goto error;
    }
    return;
  }

  if (submux->priv->is_internal) {
    length = submux->sinkpads_count;
  }else{
    length = submux->priv->stream_count;
  }

  if(!length){
    GST_ERROR_OBJECT (submux, "No streams found cancel loop");
    goto error;
  }

  for (i = 0; i < length; i++) {

re_pop:
    cur_stream = g_list_nth_data (submux->streams, i);
    g_mutex_lock (&cur_stream->queue_lock);

    if (g_queue_is_empty (cur_stream->queue) && !submux->flushing) {
      g_cond_wait (&cur_stream->queue_empty, &cur_stream->queue_lock);
    }

    if (submux->flushing || submux->stop_loop) {
      g_mutex_unlock (&cur_stream->queue_lock);
      goto error;
    }

    check_buffer = g_queue_peek_head (cur_stream->queue);
    buf_memory = gst_buffer_peek_memory(check_buffer, 0);
    gst_memory_map(buf_memory, &buf_info, GST_MAP_READ);
    char *data = (char *)buf_info.data;
    gst_memory_unmap(buf_memory, &buf_info);
    if (!strcmp ((const char*)data, "eos"))
    {
      cur_stream->eos_came = TRUE;
      GST_DEBUG_OBJECT (submux, "Eos recieved for stream");
    }
    for (k = 0; k < length; k++)  {
      check_stream = g_list_nth_data(submux->streams, k);
      if (!check_stream->eos_came) {
        eos = FALSE;
        break;
      } else {
        eos = TRUE;
      }
    }
    if (eos) {
      if(!gst_pad_push_event(submux->srcpad, gst_event_new_eos ())){
        GST_ERROR_OBJECT (submux, "Sending EOS to submux srcpad failed");
      }
      g_mutex_unlock (&cur_stream->queue_lock);
      goto eos_sent;
    }

    if (!cur_stream->eos_came && (submux->priv->parser_type == GST_SUB_PARSE_FORMAT_SAMI ||
                                  submux->priv->is_internal)) {
      GstLangStruct *lang = NULL;
      if (submux->priv->lang_list) {
        if (submux->cur_buf_array[i] == NULL) {
          check_buffer = g_queue_pop_head (cur_stream->queue);
          lang = g_list_nth_data(submux->priv->lang_list, i);
          if (!lang->active) {
            if(check_buffer) {
              gst_buffer_unref(check_buffer);
              check_buffer = NULL;
            }
            submux->cur_buf_array[i] = NULL;
            g_mutex_unlock (&cur_stream->queue_lock);
            continue;
          } else {
            if (!check_buffer) {
              g_mutex_unlock (&cur_stream->queue_lock);
              goto re_pop;
            }
            if (!GST_BUFFER_DURATION(check_buffer)) {
              gst_buffer_unref (check_buffer);
              g_mutex_unlock (&cur_stream->queue_lock);
              goto re_pop;
            }
            submux->cur_buf_array[i] = check_buffer;
          }
        }
      }
      else if(submux->priv->lang_list==NULL && submux->priv->is_internal){
        if (submux->cur_buf_array[i] == NULL){
          check_buffer = g_queue_pop_head (cur_stream->queue);
          if(i==0){
            if (!check_buffer){
              g_mutex_unlock (&cur_stream->queue_lock);
              goto re_pop;
            }
            if (!GST_BUFFER_DURATION(check_buffer)){
              gst_buffer_unref (check_buffer);
              g_mutex_unlock (&cur_stream->queue_lock);
              goto re_pop;
            }
            submux->cur_buf_array[i] = check_buffer;
          }else{
          if(check_buffer){
            gst_buffer_unref(check_buffer);
            check_buffer = NULL;
          }
          submux->cur_buf_array[i] = NULL;
          g_mutex_unlock (&cur_stream->queue_lock);
          continue;
          }
        }
      }
      else {
        g_mutex_unlock (&cur_stream->queue_lock);
        continue;
      }
    } else if (!cur_stream->eos_came) {
      /* External subtitle format other than smi */
      if (submux->sinkpad) {
        if (submux->cur_buf_array[i] == NULL) {
          check_buffer = g_queue_pop_head (cur_stream->queue);
          if (!check_buffer) {
            g_mutex_unlock (&cur_stream->queue_lock);
            goto re_pop;
          }
          if (!GST_BUFFER_DURATION (check_buffer)) {
            gst_buffer_unref (check_buffer);
            g_mutex_unlock (&cur_stream->queue_lock);
            goto re_pop;
          }
          submux->cur_buf_array[i] = check_buffer;
        }
      } else {
        g_mutex_unlock (&cur_stream->queue_lock);
        continue;
      }
    } else {
      submux->cur_buf_array[i] = NULL;
    }
    g_mutex_unlock (&cur_stream->queue_lock);
  }

  push_list = gst_submux_muxing (submux);
  if (push_list) {
    guint idx = 0;

    for (idx = 0; idx < g_list_length (push_list); idx++) {
      push_buf = g_list_nth_data (push_list, idx);

      if (push_buf) {
        fret = gst_pad_push (submux->srcpad, push_buf);
        if (fret != GST_FLOW_OK) {
          GST_ERROR_OBJECT (submux, "failed to push buffer. reason : %s", gst_flow_get_name (fret));
          /* clean any left buffers in push_list */
          idx++;
          for (; idx < g_list_length (push_list); idx++) {
            push_buf = g_list_nth_data (push_list, idx);
            gst_buffer_unref (push_buf);
          }
          g_list_free (push_list);
          goto error;
        }
      }
    }

    g_list_free (push_list);
  }

  return;

eos_sent:
error:
  GST_WARNING_OBJECT (submux->srcpad, "Pausing the push task...");
  if (fret < GST_FLOW_EOS)
  {
    GST_ERROR_OBJECT (submux, "Crtical error in push loop....");
    GST_ELEMENT_ERROR (submux, CORE, PAD, ("failed to push. reason - %s", gst_flow_get_name (fret)), (NULL));
  }
  gst_pad_pause_task (submux->srcpad);

  g_mutex_lock (&submux->srcpad_loop_lock);
  submux->srcpad_loop_running = FALSE;
  g_cond_signal (&submux->srcpad_loop_cond);
  g_mutex_unlock (&submux->srcpad_loop_lock);

  GST_WARNING_OBJECT (submux, "Exiting from lopp in eos/error");
  return;
}

////////////////////////////////////////////////////////
//        Plugin Utility Functions                    //
////////////////////////////////////////////////////////
/*
**
**  Description    : De-Initializing the submux private structure
**  Params        : (1) submux instance
**  return        : TRUE
**  Comments    :
**
*/
static gboolean
gst_submux_deinit_private_values(Gstsubmux *submux,gboolean clear_lang_list)
{
  guint idx = 0;
  SUBMUX_FENTER(submux);

  submux->priv->first_buffer = FALSE;

  if(clear_lang_list){
    GST_DEBUG_OBJECT (submux, "Clearing language list");
    submux->priv->parser_type = GST_SUB_PARSE_FORMAT_UNKNOWN;
    if (submux->priv->lang_list && !submux->priv->is_internal) {
      g_list_free (submux->priv->lang_list);
      submux->priv->lang_list = NULL;
    }
  }
  for (idx = 0; idx < submux->priv->stream_count; idx++) {
    submux->cur_buf_array[idx] = NULL;
  }
  SAFE_FREE(submux->cur_buf_array);
  SAFE_FREE(submux->detected_encoding);
  submux->priv->is_internal = FALSE;
  submux->priv->stream_count = 0;

  SUBMUX_FLEAVE(submux);
  return TRUE;
}

static gboolean
gst_submux_plugin_init (GstPlugin * plugin)
{
  return gst_element_register (plugin, "submux", GST_RANK_PRIMARY, GST_TYPE_SUBMUX);
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,GST_VERSION_MINOR,submux,"submux",gst_submux_plugin_init,"0.10.36","Proprietary","Samsung Electronics Co","http://www.samsung.com")
