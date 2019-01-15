/*
 * gstmshttpsrc
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
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
 * SECTION:element-mshttpsrc
 *
 * Multi session HTTP Streaming src element.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch mshttpsrc location=http://devimages.apple.com/iphone/samples/bipbop/gear4/prog_index.3gp ! queue2 ! qtdemux ! decodebin2 ! ffmpegcolorspace ! videoscale ! autovideosink
 * ]|
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif


#include <string.h>
#include <gst/base/gsttypefindhelper.h>
#include "gstmshttpsrc.h"

static GstStaticPadTemplate srctemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

static GstStaticPadTemplate fetchertemplate = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC (gst_mshttpsrc_debug);
#define GST_CAT_DEFAULT gst_mshttpsrc_debug

enum
{
  PROP_0=0,
  PROP_LOCATION,
  PROP_IS_LIVE,
  PROP_USER_AGENT,
  PROP_AUTOMATIC_REDIRECT,
  PROP_PROXY,
  PROP_USER_ID,
  PROP_USER_PW,
  PROP_PROXY_ID,
  PROP_PROXY_PW,
  PROP_COOKIES,
  PROP_IRADIO_MODE,
  PROP_TIMEOUT,
  PROP_EXTRA_HEADERS,
  PROP_LAST,
};

#define DEFAULT_CONNECTION_COUNT 3
#define DEFAULT_MAX_CONNECTION_COUNT 5
#define DEFAULT_SIZE_OF_RANGE 3072000
#define DEFAULT_MAX_SIZE_OF_RANGE 5120000
#define DEFAULT_MSHTTPSRC_THRESHOLD 1024000

#define DEFAULT_USER_AGENT           "GStreamer mshttpsrc "

static void gst_mshttpsrc_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void gst_mshttpsrc_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void gst_mshttpsrc_dispose (GObject * gobject);
static void gst_mshttpsrc_finalize (GObject * gobject);
static GstStateChangeReturn gst_mshttpsrc_change_state (GstElement * element, GstStateChange transition);

static GstBusSyncReply gst_mshttpsrc_fetcher_bus_handler (GstBus * bus, GstMessage * message, gpointer data);
static gboolean gst_mshttpsrc_src_event (GstPad * pad, GstEvent * event);
static gboolean gst_mshttpsrc_src_query (GstPad * pad, GstQuery * query);
static GstFlowReturn gst_mshttpsrc_fetcher_chain (GstPad * pad, GstBuffer * buf);
static gboolean gst_mshttpsrc_fetcher_sink_event (GstPad * pad, GstEvent * event);
static void gst_mshttpsrc_loop (GstMSHTTPSrc * src);
static void gst_mshttpsrc_stop (GstMSHTTPSrc * src);
static void gst_mshttpsrc_stop_fetcher (GstMSHTTPFetcher* fetcher, gboolean cancelled);
static gboolean gst_mshttpsrc_start_thread (GstMSHTTPSrc * src);
static gboolean gst_mshttpsrc_update_thread (GstMSHTTPSrc * src);
static void gst_mshttpsrc_reset (GstMSHTTPSrc * src, gboolean dispose);

static gboolean gst_mshttpsrc_make_fetcher (GstMSHTTPFetcher* fetch, const gchar * uri, GstMSHTTPSrc * src);
static gint GMSHTTPCompareDataFunc(gconstpointer a, gconstpointer b, gpointer user_data);

static void
_do_init (GType type)
{
  GST_DEBUG_CATEGORY_INIT (gst_mshttpsrc_debug, "mshttpsrc", 0,
      "mshttpsrc element");
}

GST_BOILERPLATE_FULL (GstMSHTTPSrc, gst_mshttpsrc, GstElement, GST_TYPE_ELEMENT, _do_init);

static void
gst_mshttpsrc_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&srctemplate));

  gst_element_class_set_details_simple (element_class,
      "Multi Session HTTP source",
      "Source/Network",
      "Multi session HTTP Streaming src element using soup",
      "www.samsung.com");
}

static void
gst_mshttpsrc_class_init (GstMSHTTPSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->set_property = gst_mshttpsrc_set_property;
  gobject_class->get_property = gst_mshttpsrc_get_property;
  gobject_class->dispose = gst_mshttpsrc_dispose;
  gobject_class->finalize = gst_mshttpsrc_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location",
          "Location to read from", "",
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_USER_AGENT,
      g_param_spec_string ("user-agent", "User-Agent",
          "Value of the User-Agent HTTP request header field", DEFAULT_USER_AGENT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_AUTOMATIC_REDIRECT,
      g_param_spec_boolean ("automatic-redirect", "automatic-redirect",
          "Automatically follow HTTP redirects (HTTP Status Code 3xx)", TRUE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PROXY,
      g_param_spec_string ("proxy", "Proxy",
          "HTTP proxy server URI", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_USER_ID,
      g_param_spec_string ("user-id", "user-id",
          "HTTP location URI user id for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_USER_PW,
      g_param_spec_string ("user-pw", "user-pw",
          "HTTP location URI user password for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PROXY_ID,
      g_param_spec_string ("proxy-id", "proxy-id",
          "HTTP proxy URI user id for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_PROXY_PW,
      g_param_spec_string ("proxy-pw", "proxy-pw",
          "HTTP proxy URI user password for authentication", "",
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_COOKIES,
      g_param_spec_boxed ("cookies", "Cookies",
          "HTTP request cookies", G_TYPE_STRV,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_IS_LIVE,
      g_param_spec_boolean ("is-live", "is-live",
          "Act like a live source", FALSE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_TIMEOUT,
      g_param_spec_uint ("timeout", "timeout",
          "Value in seconds to timeout a blocking I/O (0 = No timeout).", 0,
          3600, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_EXTRA_HEADERS,
      g_param_spec_boxed ("extra-headers", "Extra Headers",
          "Extra headers to append to the HTTP request",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* icecast stuff */
  g_object_class_install_property (gobject_class,
      PROP_IRADIO_MODE,
      g_param_spec_boolean ("iradio-mode", "iradio-mode",
          "Enable internet radio mode (extraction of shoutcast/icecast metadata)",
          FALSE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_mshttpsrc_change_state);
}

static void
gst_mshttpsrc_init (GstMSHTTPSrc * src, GstMSHTTPSrcClass * klass)
{
  /* src pad */
  src->srcpad = gst_pad_new_from_static_template (&srctemplate, "src");
  gst_pad_set_event_function (src->srcpad, GST_DEBUG_FUNCPTR (gst_mshttpsrc_src_event));
  gst_pad_set_query_function (src->srcpad, GST_DEBUG_FUNCPTR (gst_mshttpsrc_src_query));

  gst_pad_set_element_private (src->srcpad, src);

  gst_element_add_pad (GST_ELEMENT (src), src->srcpad);

  /* Properties */
  src->uri_list = g_list_alloc ();
  src->uri_count = 0;
  src->is_live = FALSE;
  src->automatic_redirect = TRUE;
  src->user_agent = g_strdup (DEFAULT_USER_AGENT);
  src->user_id = NULL;
  src->user_pw = NULL;
  src->proxy_id = NULL;
  src->proxy_pw = NULL;
  src->cookies = NULL;
  src->iradio_mode = FALSE;
  src->end_of_playlist = FALSE;
  src->fetcher = NULL;
  src->session_count = 1;
  src->range_size = -1;
  src->total_file_size = 0;
  src->total_download = 0;
  src->seek_event = NULL;
  src->buf_index_push = 0;
  src->buf_index_req = -1;
  src->eos_count = 0;
  src->is_flushed = TRUE;

  src->thread_cond = g_cond_new ();
  src->thread_lock = g_mutex_new ();
  src->queue_lock = g_mutex_new ();
  src->index_lock = g_mutex_new ();
  src->queue = g_queue_new ();
  g_static_rec_mutex_init (&src->task_lock);

  src->task = gst_task_create ((GstTaskFunction) gst_mshttpsrc_loop, src);
  gst_task_set_lock (src->task, &src->task_lock);
}

static void
gst_mshttpsrc_finalize (GObject * gobject)
{
  GstMSHTTPSrc *src = GST_MSHTTP_SRC (gobject);

  GST_DEBUG_OBJECT (src, "finalize");

  g_list_free (src->uri_list);
  g_free (src->user_agent);
  g_free (src->proxy);
  g_free (src->user_id);
  g_free (src->user_pw);
  g_free (src->proxy_id);
  g_free (src->proxy_pw);
  g_strfreev (src->cookies);

  G_OBJECT_CLASS (parent_class)->finalize (gobject);
}

static void
gst_mshttpsrc_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
  GstMSHTTPSrc *src = GST_MSHTTP_SRC (object);
  GST_DEBUG ("set property with prop ID %d", prop_id);

  switch (prop_id) {
    case PROP_LOCATION:
    {
      const gchar *location,*temp;
      location = g_value_get_string (value);
      if (location == NULL) {
        GST_WARNING ("location property cannot be NULL");
        break;
      }
      temp = g_strdup(location);
      src->uri_list = g_list_append(src->uri_list, (gpointer)temp);
      GST_DEBUG ("set property with URI :%s", temp);
      src->uri_count++;
    }
    break;
    case PROP_USER_AGENT:
      if (src->user_agent) g_free (src->user_agent);
      src->user_agent = g_value_dup_string (value);
    break;
    case PROP_IRADIO_MODE:
      src->iradio_mode = g_value_get_boolean (value);
    break;
    case PROP_AUTOMATIC_REDIRECT:
      src->automatic_redirect = g_value_get_boolean (value);
    break;
    case PROP_PROXY:
      if (src->proxy) g_free (src->proxy);
      src->proxy = g_value_dup_string (value);
    break;
    case PROP_COOKIES:
      g_strfreev (src->cookies);
      src->cookies = g_strdupv (g_value_get_boxed (value));
    break;
    case PROP_IS_LIVE:
      src->is_live = g_value_get_boolean (value);
    break;
    case PROP_USER_ID:
      if (src->user_id) g_free (src->user_id);
      src->user_id = g_value_dup_string (value);
    break;
    case PROP_USER_PW:
      if (src->user_pw) g_free (src->user_pw);
      src->user_pw = g_value_dup_string (value);
    break;
    case PROP_PROXY_ID:
      if (src->proxy_id) g_free (src->proxy_id);
      src->proxy_id = g_value_dup_string (value);
    break;
    case PROP_PROXY_PW:
      if (src->proxy_pw) g_free (src->proxy_pw);
      src->proxy_pw = g_value_dup_string (value);
    break;
    case PROP_TIMEOUT:
      src->timeout = g_value_get_uint (value);
    break;
    case PROP_EXTRA_HEADERS:
    {
      const GstStructure *s = gst_value_get_structure (value);
      if (src->extra_headers) gst_structure_free (src->extra_headers);
      src->extra_headers = s ? gst_structure_copy (s) : NULL;
    }
    break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
gst_mshttpsrc_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
  GstMSHTTPSrc *src = GST_MSHTTP_SRC (object);
  GST_DEBUG ("get property for prop ID %d", prop_id);
  switch (prop_id) {
    case PROP_USER_AGENT:
      g_value_set_string (value, src->user_agent);
    break;
    case PROP_AUTOMATIC_REDIRECT:
      g_value_set_boolean (value, src->automatic_redirect);
    break;
    case PROP_PROXY:
      g_value_set_string (value, src->proxy);
    break;
    case PROP_COOKIES:
      g_value_set_boxed (value, g_strdupv (src->cookies));
    break;
    case PROP_IS_LIVE:
      g_value_set_boolean (value, src->is_live);
    break;
    case PROP_IRADIO_MODE:
      g_value_set_boolean (value, src->iradio_mode);
    break;
    case PROP_USER_ID:
      g_value_set_string (value, src->user_id);
    break;
    case PROP_USER_PW:
      g_value_set_string (value, src->user_pw);
    break;
    case PROP_PROXY_ID:
      g_value_set_string (value, src->proxy_id);
    break;
    case PROP_PROXY_PW:
      g_value_set_string (value, src->proxy_pw);
    break;
    case PROP_TIMEOUT:
      g_value_set_uint (value, src->timeout);
    break;
    case PROP_EXTRA_HEADERS:
      gst_value_set_structure (value, src->extra_headers);
    break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}
static void gst_mshttpsrc_set_rangesize(GstMSHTTPSrc *src)
{
  if(src->total_file_size < (DEFAULT_SIZE_OF_RANGE * DEFAULT_CONNECTION_COUNT)) src->range_size = -1;
  else if(src->total_file_size < (DEFAULT_MAX_SIZE_OF_RANGE * DEFAULT_MAX_CONNECTION_COUNT)) src->range_size = DEFAULT_SIZE_OF_RANGE;
  else src->range_size = DEFAULT_MAX_SIZE_OF_RANGE;
  return;
}
static void gst_mshttpsrc_set_sessioncount(GstMSHTTPSrc *src)
{
  if(src->total_file_size < (DEFAULT_SIZE_OF_RANGE * DEFAULT_CONNECTION_COUNT)) src->session_count = 1;
  else if(src->total_file_size < (DEFAULT_MAX_SIZE_OF_RANGE * DEFAULT_MAX_CONNECTION_COUNT)) src->session_count = DEFAULT_CONNECTION_COUNT;
  else src->session_count = DEFAULT_CONNECTION_COUNT;
  return;
}
static GstStateChangeReturn
gst_mshttpsrc_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstMSHTTPSrc *src = GST_MSHTTP_SRC (element);
  GST_INFO ("Change state  = %d ", transition);
  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
    {
      guint fetchercount = 0;
      guint i=0;
      GstState cur, pending;
      gboolean status = FALSE;
      if(src->uri_count > 1) fetchercount = src->uri_count;
      else fetchercount = DEFAULT_MAX_CONNECTION_COUNT;
      GST_DEBUG ("change state NULL to READY");
      src->fetcher = g_new0(GstMSHTTPFetcher, fetchercount);
      // Only one session created currently. After knowing the complete file size number of sessions to open will be decided
      {
        /* fetcher pad */
        src->fetcher[i].fetcherpad = gst_pad_new_from_static_template (&fetchertemplate, "sink");
        gst_pad_set_chain_function (src->fetcher[i].fetcherpad, GST_DEBUG_FUNCPTR (gst_mshttpsrc_fetcher_chain));
        gst_pad_set_event_function (src->fetcher[i].fetcherpad,GST_DEBUG_FUNCPTR (gst_mshttpsrc_fetcher_sink_event));
        gst_pad_set_element_private (src->fetcher[i].fetcherpad, &src->fetcher[i]);
        gst_pad_activate_push (src->fetcher[i].fetcherpad, TRUE);

        src->fetcher[i].download = gst_adapter_new ();
        src->fetcher[i].fetcher_bus = gst_bus_new ();
        gst_bus_set_sync_handler (src->fetcher[i].fetcher_bus, gst_mshttpsrc_fetcher_bus_handler, &src->fetcher[i]);
        src->fetcher[i].fetcher_cond = g_cond_new ();
        src->fetcher[i].cond_lock = g_mutex_new ();
        src->fetcher[i].fetcher_lock = g_mutex_new ();
        src->fetcher[i].parent = (gpointer)src;
        src->fetcher[i].buf_index_prev = 0;
        src->fetcher[i].buf_index_cur = 0;
        src->fetcher[i].is_active = FALSE;
        if(src->uri_count == fetchercount) status = gst_mshttpsrc_make_fetcher (&src->fetcher[i], g_list_nth_data(src->uri_list,i+1), src);
        else status = gst_mshttpsrc_make_fetcher (&src->fetcher[i], g_list_nth_data(src->uri_list,1), src);
        if (!status) GST_ERROR ("Error creating fetcher element.");
        ret = gst_element_set_state (GST_ELEMENT (src->fetcher[i].fetcher), GST_STATE_PLAYING);
        if (ret == GST_STATE_CHANGE_FAILURE) GST_ELEMENT_ERROR (src, CORE, STATE_CHANGE,("Error changing state of the fetcher element."), NULL);
        gst_element_get_state (GST_ELEMENT (src->fetcher[i].fetcher), &cur, &pending, -1);
        GST_WARNING ("Cur state  = %d and pending = %d", cur, pending);
      }
      /* we can start now the updates thread */
      gst_mshttpsrc_start_thread (src);
      gst_task_start (src->task);
    }
    break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
    {
      guint i=1;
      GstState cur, pending;
      gboolean status = FALSE;
      GST_WARNING ("change state PAUSED to PLAYING");
      if(src->session_count==1) {
        gst_mshttpsrc_set_sessioncount(src);
        gst_mshttpsrc_set_rangesize(src);
        GST_WARNING ("Creating new fetcherpads with connection-count %d range-size %d", src->session_count, src->range_size);
        for(;i<src->session_count;i++) {
          /* fetcher pad */
          src->fetcher[i].fetcherpad = gst_pad_new_from_static_template (&fetchertemplate, "sink");
          gst_pad_set_chain_function (src->fetcher[i].fetcherpad, GST_DEBUG_FUNCPTR (gst_mshttpsrc_fetcher_chain));
          gst_pad_set_event_function (src->fetcher[i].fetcherpad, GST_DEBUG_FUNCPTR (gst_mshttpsrc_fetcher_sink_event));
          gst_pad_set_element_private (src->fetcher[i].fetcherpad, &src->fetcher[i]);
          gst_pad_activate_push (src->fetcher[i].fetcherpad, TRUE);

          src->fetcher[i].download = gst_adapter_new ();
          src->fetcher[i].fetcher_bus = gst_bus_new ();
          gst_bus_set_sync_handler (src->fetcher[i].fetcher_bus, gst_mshttpsrc_fetcher_bus_handler, &src->fetcher[i]);
          src->fetcher[i].fetcher_cond = g_cond_new ();
          src->fetcher[i].cond_lock = g_mutex_new ();
          src->fetcher[i].fetcher_lock = g_mutex_new ();
          src->fetcher[i].parent = (gpointer)src;
          src->fetcher[i].buf_index_prev = 0;
          src->fetcher[i].buf_index_cur = 0;
          src->fetcher[i].is_active = FALSE;
          if(src->uri_count == src->session_count) status = gst_mshttpsrc_make_fetcher (&src->fetcher[i], g_list_nth_data(src->uri_list,i+1), src);
          else status = gst_mshttpsrc_make_fetcher (&src->fetcher[i], g_list_nth_data(src->uri_list,1), src);
          if (!status) GST_WARNING ("Error creating fetcher element.");
          src->fetcher[i].range_size = src->range_size;
          ret = gst_element_set_state (GST_ELEMENT (src->fetcher[i].fetcher), GST_STATE_PLAYING);
          if (ret == GST_STATE_CHANGE_FAILURE) GST_ELEMENT_ERROR (src, CORE, STATE_CHANGE,("Error changing state of the fetcher element."), NULL);
          gst_element_get_state (GST_ELEMENT (src->fetcher[i].fetcher), &cur, &pending, -1);
          GST_WARNING ("Cur state  = %d and pending = %d", cur, pending);
        }
      }
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
      if(src->eos_count == src->session_count)
      {
        src->end_of_playlist = TRUE;
        GST_DEBUG ("Got EOS end_of_playlist set");
        g_cond_signal (src->thread_cond);
      }
    break;
    default:
    break;
  }
  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  return ret;
}

static gboolean gst_mshttpsrc_perform_seek (GstMSHTTPSrc *src, GstPad * pad, GstEvent * event)
{
  gdouble rate;
  GstFormat format = GST_FORMAT_UNDEFINED;
  GstSeekFlags flags;
  GstSeekType cur_type = GST_SEEK_TYPE_NONE, stop_type;
  gint64 cur = 0, stop;
  gboolean flush;
  gint i=src->session_count-1;
  GstEvent * newsegment;
  if (event) {
    gint64 start = 0;
    if(src->seek_event) gst_event_parse_seek(src->seek_event, NULL, NULL, NULL, NULL, &start, NULL, NULL);
    GST_DEBUG_OBJECT (src, "doing seek with event");
    gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur, &stop_type, &stop);
    if(start == cur) {
      GST_DEBUG_OBJECT (src, "duplicate seek event");
      return TRUE;
    } else {
      gst_event_unref (src->seek_event);
      src->seek_event = gst_event_ref (event);
    }
  } else {
    GST_DEBUG_OBJECT (src, "doing seek without event");
    flags = 0;
    rate = 1.0;
  }
  /* save flush flag */
  flush = flags & GST_SEEK_FLAG_FLUSH;
  if (flush) {
    GstEvent *fevent = gst_event_new_flush_start ();
    /* for a flushing seek, we send a flush_start on all pads. This will
     * eventually stop streaming with a WRONG_STATE. We can thus eventually
     * take the STREAM_LOCK. */
    GST_DEBUG_OBJECT (src, "sending flush start");
    gst_pad_push_event (src->srcpad, fevent);
  } else {
    /* a non-flushing seek, we PAUSE the task so that we can take the
     * STREAM_LOCK */
    GST_DEBUG_OBJECT (src, "non flushing seek, pausing task");
    gst_pad_pause_task (src->srcpad);
  }
  /* wait for streaming to stop */
  GST_DEBUG_OBJECT (src, "wait for streaming to stop");
  GST_PAD_STREAM_LOCK (src->srcpad);
  g_mutex_lock (src->index_lock);
  src->buf_index_req = (cur/src->range_size)-1;
  g_mutex_unlock (src->index_lock);
  src->eos_count = 0;
  for(;i>=0;i--) {
    GstEvent* event = NULL;
    GstPad *pad = NULL;
    src->fetcher[i].stopping_fetcher = FALSE;
    src->fetcher[i].range_size = src->range_size;
    if(i==0){
      g_mutex_lock (src->fetcher[i].fetcher_lock);
      gst_adapter_flush (src->fetcher[i].download, gst_adapter_available (src->fetcher[i].download));
      src->fetcher[i].range_downloaded = 0;
      src->fetcher[i].is_flushed = FALSE;
      src->fetcher[i].is_active = TRUE;
      g_mutex_unlock (src->fetcher[i].fetcher_lock);

      g_mutex_lock (src->index_lock);
      src->buf_index_req++;
      src->fetcher[i].buf_index_cur = src->buf_index_req;
      g_mutex_unlock (src->index_lock);

      g_object_set (G_OBJECT (src->fetcher[i].fetcher), "rangesize", (gint64)(src->range_size - (cur%src->range_size)), NULL);
      if(src->range_size != -1) src->fetcher[i].range_size = ((src->total_file_size-cur)<(src->range_size-(cur%src->range_size)))? (src->total_file_size-cur): (src->range_size-(cur%src->range_size));
      else src->fetcher[i].range_size = (src->total_file_size-cur);
	    GST_LOG_OBJECT (src,"Fetcher range size is %lli source range size is %lli", src->fetcher[i].range_size, src->range_size);
      event = gst_event_new_seek (1.0, GST_FORMAT_BYTES, GST_SEEK_FLAG_ACCURATE|GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, cur, GST_SEEK_TYPE_SET, -1);
      pad = gst_element_get_static_pad (src->fetcher[i].fetcher, "src");
      if (pad) {
        gst_pad_send_event(pad, event );
        gst_object_unref (pad);
      }
    }else{
      g_mutex_lock (src->fetcher[i].fetcher_lock);
      gst_adapter_flush (src->fetcher[i].download, gst_adapter_available (src->fetcher[i].download));
      src->fetcher[i].is_active = FALSE;
      src->fetcher[i].is_flushed = TRUE;
      src->fetcher[i].range_downloaded = 0;
      g_mutex_unlock (src->fetcher[i].fetcher_lock);
      GST_DEBUG_OBJECT(src,"setting the other two sessions into blocked state after seek");
    }

  }
  src->is_flushed = TRUE;
  if(src->queue) {
    g_mutex_lock (src->queue_lock);
    while (!g_queue_is_empty (src->queue)) {
      GstMSHTTPBuffer *mc_buf = (GstMSHTTPBuffer *)g_queue_peek_head (src->queue);
      gst_buffer_unref (mc_buf->buffer);
      g_queue_remove(src->queue, mc_buf);
      g_free(mc_buf);
    }
    src->buf_index_push = (cur/src->range_size);
    g_mutex_unlock (src->queue_lock);
  }
  if (flush) {
    GstEvent *fevent = gst_event_new_flush_stop ();
    GST_DEBUG_OBJECT (src, "sending flush stop");
    gst_pad_push_event (src->srcpad, fevent);
  }
  newsegment = gst_event_new_new_segment_full (FALSE, rate, rate, format, cur, -1, cur);
  gst_pad_push_event (src->srcpad, newsegment);
  /* reset the last flow and mark discont, seek is always DISCONT */
  GST_PAD_STREAM_UNLOCK (src->srcpad);
  return TRUE;
}

static gboolean
gst_mshttpsrc_src_event (GstPad * pad, GstEvent * event)
{
  gboolean res = TRUE;
  GstMSHTTPSrc *src = GST_MSHTTP_SRC (gst_pad_get_parent (pad));
  GST_DEBUG_OBJECT (src, "handle event: %" GST_PTR_FORMAT, event);
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      res = gst_mshttpsrc_perform_seek (src, pad, event);
      gst_event_unref (event);
    break;
    case GST_EVENT_QOS:
    case GST_EVENT_NAVIGATION:
      res = FALSE;
      gst_event_unref (event);
    break;
    default:
      res = gst_pad_event_default (pad, event);
    break;
  }
  gst_object_unref (src);
  return res;
}

static gboolean gst_mshttpsrc_src_query (GstPad * pad, GstQuery * query)
{
  GstMSHTTPSrc *src;
  gboolean ret = FALSE;
  if (query == NULL) return FALSE;

  src = GST_MSHTTP_SRC (gst_pad_get_element_private (pad));
  GST_DEBUG("query type %d", query->type);
  switch (query->type) {
    case GST_QUERY_DURATION:{
      GstFormat format;
      gst_query_parse_duration (query, &format, NULL);
      GST_DEBUG_OBJECT (src, "duration query in format %s",
      gst_format_get_name (format));
      switch (format) {
        case GST_FORMAT_PERCENT:
          gst_query_set_duration (query, GST_FORMAT_PERCENT,
          GST_FORMAT_PERCENT_MAX);
          ret = TRUE;
        break;
        default: {
          gint64 duration = -1;
          /* this is the duration */
          if(src->total_file_size) duration = src->total_file_size;
          GST_LOG_OBJECT (src, "duration %" G_GINT64_FORMAT ", format %s", duration, gst_format_get_name(format));
          if (duration != -1) ret = gst_pad_query_convert (src->srcpad, GST_FORMAT_BYTES, duration, &format, &duration);
          else ret = TRUE;
          gst_query_set_duration (query, format, duration);
        }
        break;
      }
    }
    break;
    case GST_QUERY_URI:
      gst_query_set_uri (query, g_list_nth_data(src->uri_list,1));
      GST_DEBUG("query URI type set %s", g_list_nth_data(src->uri_list,1));
      ret = TRUE;
    break;
    case GST_QUERY_BUFFERING: {
      GstFormat format;
      gint64 start, stop, estimated, percent;

      gst_query_parse_buffering_range (query, &format, NULL, NULL, NULL);
      GST_DEBUG("buffering query in format %s", gst_format_get_name (format));
      estimated = src->total_file_size;
      start = 0;
      if (format == GST_FORMAT_PERCENT) stop = GST_FORMAT_PERCENT_MAX * src->total_download /src->total_file_size;
      else stop = src->total_download;
      ret = TRUE;
      percent = (src->total_download/(src->range_size/8.0))*1.0;
      if(percent > 1) percent = 100;
      else percent *= 100;
      GST_INFO("buffering query in format %s and start %lli stop %lli buffering percent %lli",
                 gst_format_get_name (format), start, stop, percent);
      gst_query_set_buffering_percent (query, TRUE, percent);
      gst_query_set_buffering_range (query, format, start, stop, estimated);
    }
    break;
    case GST_QUERY_SEEKING:
    {
      GstFormat format;
      gst_query_parse_seeking (query, &format, NULL, NULL, NULL);
      gst_query_set_seeking (query, format, TRUE, 0, src->total_file_size);
      ret = TRUE;
      break;
    }
    default:
    break;
  }
  return ret;
}

static gboolean
gst_mshttpsrc_make_fetcher (GstMSHTTPFetcher* fetch, const gchar * uri, GstMSHTTPSrc * src)
{
  GstPad *pad;
  GstPadLinkReturn link_ret ;
  if (!gst_uri_is_valid (uri)) return FALSE;
  GST_DEBUG("Creating fetcher for the URI:%s", uri);
  fetch->fetcher = gst_element_make_from_uri (GST_URI_SRC, uri, NULL);
  if (!fetch->fetcher) return FALSE;
  fetch->range_size = -1;
  fetch->fetcher_error = FALSE;
  fetch->stopping_fetcher = FALSE;
  fetch->is_flushed = TRUE;
  fetch->cancelled = FALSE;
  fetch->end_of_sequence = FALSE;
  fetch->is_active = FALSE;
  gst_element_set_bus (GST_ELEMENT (fetch->fetcher), fetch->fetcher_bus);

  g_object_set (G_OBJECT (fetch->fetcher), "location", uri, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "rangesize", (gint64)src->range_size, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "user-agent", src->user_agent, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "automatic-redirect", src->automatic_redirect, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "proxy", src->proxy, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "user-id", src->user_id, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "user-pw", src->user_pw, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "proxy-id", src->proxy_id, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "proxy-pw", src->proxy_pw, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "cookies", src->cookies, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "is-live", src->is_live, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "timeout", src->timeout, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "extra-headers", src->extra_headers, NULL);
  g_object_set (G_OBJECT (fetch->fetcher), "iradio-mode", src->iradio_mode, NULL);
  pad = gst_element_get_static_pad (fetch->fetcher, "src");
  if (pad) {
    link_ret = gst_pad_link (pad, fetch->fetcherpad);
    GST_DEBUG ("Link ret = %d", link_ret);
    gst_object_unref (pad);
  }
  GST_INFO("Fetcher created");
  return TRUE;
}

static GstFlowReturn
gst_mshttpsrc_fetcher_chain (GstPad * apad, GstBuffer * buf)
{
  GstMSHTTPFetcher *fetch = (GstMSHTTPFetcher *)gst_pad_get_element_private (apad);
  GstMSHTTPSrc *src = (GstMSHTTPSrc *)fetch->parent;
  guint avail = 0;

  if (fetch->fetcher_error) {
    goto done;
  }
  if(fetch->is_flushed)
  {
    GstEvent* event = NULL;
    gboolean result = FALSE;
    GstPad *pad = NULL;
    GstFormat format = GST_FORMAT_BYTES;
    GstSeekFlags flags = GST_SEEK_FLAG_ACCURATE|GST_SEEK_FLAG_FLUSH;
    GstSeekType type = GST_SEEK_TYPE_SET;
    gint64 cur = 0;
    gint64 stop = -1;
    GstFormat peer_fmt;
    g_mutex_lock (src->index_lock);
    src->buf_index_req++;
    fetch->buf_index_cur = src->buf_index_req;
    g_mutex_unlock (src->index_lock);

    cur = fetch->buf_index_cur*src->range_size;
    if(src->is_flushed) {
      fetch->is_active = TRUE;
      src->is_flushed = FALSE;
      g_cond_signal(src->thread_cond);
    }
    GST_LOG_OBJECT (fetch, "The sink thread func before seek to index %d ", fetch->buf_index_cur);
    if(fetch->buf_index_cur) {
      event = gst_event_new_seek (1.0, format, flags, type, cur, type, stop);
      pad = gst_element_get_static_pad (fetch->fetcher, "src");
      if (pad) {
        peer_fmt = GST_FORMAT_BYTES;
        gst_pad_query_duration (pad, &peer_fmt, (gint64*)&src->total_file_size);
        GST_DEBUG("The complete file size is %llu ", src->total_file_size);
        result = gst_pad_send_event(pad, event );
        gst_object_unref (pad);
      }
      GST_LOG_OBJECT (fetch, "The sink thread func seek to index %d file position %llu, result %d", fetch->buf_index_cur, cur, result);
      fetch->range_downloaded = 0;
      fetch->is_flushed = FALSE;
      goto done;
    }
    pad = gst_element_get_static_pad (fetch->fetcher, "src");
    if (pad) {
      peer_fmt = GST_FORMAT_BYTES;
      gst_pad_query_duration (pad, &peer_fmt, (gint64*)&src->total_file_size);
      GST_DEBUG("The complete file size is %llu ", src->total_file_size);
      gst_object_unref (pad);
    }
    fetch->range_downloaded = 0;
    fetch->is_flushed = FALSE;
    gst_mshttpsrc_set_rangesize(src);
    fetch->range_size = src->range_size;

    if(src->range_size != -1) {
      g_object_set (G_OBJECT (fetch->fetcher), "rangesize", (gint64)src->range_size, NULL);
      cur = fetch->buf_index_cur*src->range_size;
      event = gst_event_new_seek (1.0, format, flags, type, cur, type, stop);
      result = gst_pad_send_event(pad, event );
      GST_LOG_OBJECT (fetch, "The sink thread func seek to index start %d file position %llu, result %d", fetch->buf_index_cur, cur, result);
      goto done;
    }
  }
  GST_LOG_OBJECT (fetch, "The fetcher received a new buffer of size %u",
  GST_BUFFER_SIZE (buf));
  g_mutex_lock (fetch->fetcher_lock);
  gst_adapter_push (fetch->download, buf);
  GST_LOG_OBJECT (fetch, "The fetcher pushed a new buffer of size %u",GST_BUFFER_SIZE (buf));
  fetch->range_downloaded += GST_BUFFER_SIZE (buf);
  avail = gst_adapter_available (fetch->download);
  if(avail) fetch->buf_index_prev = fetch->buf_index_cur;
  if((fetch->range_downloaded >= fetch->range_size)|| fetch->stopping_fetcher) {
    fetch->end_of_sequence = TRUE;
    GST_LOG_OBJECT (fetch, "The fetcher end of sequence set");
  }
  g_mutex_unlock (fetch->fetcher_lock);
  if((fetch->range_downloaded >= fetch->range_size)|| fetch->stopping_fetcher) {
    GST_WARNING ("The fetcher signal sending to thread %d",fetch->buf_index_cur);
retry_signal:
    g_cond_signal(src->thread_cond);
    GST_LOG_OBJECT (fetch, "The fetcher signal sent to thread");
    {
      GTimeVal abs_time;
      g_get_current_time (&abs_time);
      g_time_val_add (&abs_time, 1000);
      g_cond_timed_wait(fetch->fetcher_cond, fetch->cond_lock, &abs_time);
      g_mutex_lock (fetch->fetcher_lock);
      avail = gst_adapter_available (fetch->download);
      g_mutex_unlock (fetch->fetcher_lock);
      if(avail) {
        GST_LOG_OBJECT (fetch, "The fetcher signal waiting to get buffer consumed %d", avail);
        goto retry_signal;
      }
    }
    GST_LOG_OBJECT (fetch, "The fetcher signal out of timed wait %lli %lli",fetch->range_downloaded, fetch->range_size);
    if(fetch->range_downloaded >= fetch->range_size) {
      GstEvent* event = NULL;
      gboolean result = FALSE;
      GstPad *pad = NULL;
      GstFormat format = GST_FORMAT_BYTES;
      GstSeekFlags flags = GST_SEEK_FLAG_ACCURATE|GST_SEEK_FLAG_FLUSH;
      GstSeekType type = GST_SEEK_TYPE_SET;
      gint64 cur = 0;
      gint64 stop = -1;
      g_mutex_lock (src->index_lock);
      src->buf_index_req++;
      fetch->buf_index_cur = src->buf_index_req;
      g_mutex_unlock (src->index_lock);
      cur = fetch->buf_index_cur*src->range_size;
      GST_LOG_OBJECT (fetch, "The sink thread func before seek to index %d ", fetch->buf_index_cur);
	    if(fetch->range_size != src->range_size) {
	      fetch->range_size = src->range_size;
        g_object_set (G_OBJECT (fetch->fetcher), "rangesize", (gint64)src->range_size, NULL);
	    }
      event = gst_event_new_seek (1.0, format, flags, type, cur, type, stop);
      pad = gst_element_get_static_pad (fetch->fetcher, "src");
      if (pad) {
        result = gst_pad_send_event(pad, event );
        gst_object_unref (pad);
      }
      GST_LOG_OBJECT (fetch, "The sink thread func seek to index %d file position %llu, result %d", fetch->buf_index_cur, cur, result);
      fetch->range_downloaded = 0;
      fetch->end_of_sequence = FALSE;
    }
  }
done:
  {
    return GST_FLOW_OK;
  }
}

static GstBusSyncReply
gst_mshttpsrc_fetcher_bus_handler (GstBus * bus, GstMessage * message, gpointer data)
{
  GstMSHTTPFetcher *fetch = (GstMSHTTPFetcher *)data;
  if (GST_MESSAGE_TYPE (message) == GST_MESSAGE_ERROR) {
    fetch->fetcher_error = TRUE;
	GST_ERROR ("gst_mshttpsrc_fetcher_bus_handler error notification");
    g_cond_signal (fetch->fetcher_cond);
  }
  gst_message_unref (message);
  return GST_BUS_DROP;
}

static gboolean
gst_mshttpsrc_fetcher_sink_event (GstPad * pad, GstEvent * event)
{
  GstMSHTTPFetcher *fetch = (GstMSHTTPFetcher *)gst_pad_get_element_private (pad);
  GstMSHTTPSrc *src = (GstMSHTTPSrc *)fetch->parent;

  switch (event->type) {
    case GST_EVENT_EOS: {
      if(!fetch->stopping_fetcher) {
        GST_WARNING ("Got EOS on the fetcher pad");
        if (!fetch->cancelled) g_cond_signal (fetch->fetcher_cond);
        fetch->stopping_fetcher = TRUE;
        src->eos_count++;
        fetch->buf_index_prev = fetch->buf_index_cur;
        g_cond_signal (src->thread_cond);
      }
    }
    break;
    default:
    break;
  }
  gst_event_unref (event);
  return FALSE;
}

static void
gst_mshttpsrc_loop (GstMSHTTPSrc * src)
{
  GstFlowReturn ret;
  GST_LOG_OBJECT (src, "The sink loop start");
  if (src->thread_return)
  {
    g_mutex_lock (src->queue_lock);
    if(g_queue_is_empty (src->queue))
    {
      g_mutex_unlock (src->queue_lock);
      GST_LOG_OBJECT (src, "The sink loop goto end_of_playlist");
      goto end_of_playlist;
    }
    g_mutex_unlock (src->queue_lock);
  }

  GST_TASK_WAIT (src->task);

  GST_LOG_OBJECT (src, "The sink loop woke up from task wait");
  g_mutex_lock (src->queue_lock);
  /* If the queue is still empty check again if it's the end of the
   * sequence in case we reached it after being waken up */
  if (g_queue_is_empty (src->queue) && src->thread_return) {
    g_mutex_unlock (src->queue_lock);
    GST_LOG_OBJECT (src, "The sink loop goto end_of_playlist");
    goto end_of_playlist;
  }
  while(1) {
    GstMSHTTPBuffer *mc_buf = NULL;
    mc_buf = (GstMSHTTPBuffer *)g_queue_peek_head(src->queue);
    if(mc_buf == NULL) break;
    GST_LOG_OBJECT (src, "The sink loop current buffer index %d %d", src->buf_index_push, mc_buf->index);
    {
      ret = gst_pad_push (src->srcpad, mc_buf->buffer);
      if (ret != GST_FLOW_OK) {
        g_mutex_unlock (src->queue_lock);
        goto error;
      }
      GST_DEBUG ("The sink loop pad push buffer index %d %x", mc_buf->index, mc_buf);
      src->buf_index_push = mc_buf->index;
      src->total_download += mc_buf->buffer->size;
      g_queue_remove(src->queue, mc_buf);
    }
  }
  g_mutex_unlock (src->queue_lock);
  return;

end_of_playlist:
  {
    GST_DEBUG_OBJECT (src, "Reached end of playlist, sending EOS");
    gst_pad_push_event (src->srcpad, gst_event_new_eos ());
    GST_OBJECT_UNLOCK (src->task);
    gst_task_pause(src->task);
    GST_LOG_OBJECT (src, "The sink loop exit final");
    return;
  }
error:
  {
    /* FIXME: handle error */
    GST_ERROR ("The sink loop pad push buffer error %d",ret);
    return;
  }
}

static gint GMSHTTPCompareDataFunc(gconstpointer a, gconstpointer b, gpointer user_data)
{
  GstMSHTTPBuffer *a_buf = (GstMSHTTPBuffer *)a;
  GstMSHTTPBuffer *b_buf = (GstMSHTTPBuffer *)b;
  GST_DEBUG ("GMSHTTPCompareDataFunc %d %d %x %x", a_buf->index, b_buf->index, a_buf, b_buf);
  if(a_buf->index < b_buf->index) return -1;
  else if(a_buf->index == b_buf->index) return 0;
  else return 1;
}

static gboolean
gst_mshttpsrc_start_thread (GstMSHTTPSrc * src)
{
  GError *error;
  /* creates a new thread for the updates */
  src->updates_thread = g_thread_create ((GThreadFunc) gst_mshttpsrc_update_thread, src, TRUE, &error);
  return (error != NULL);
}

static gboolean
gst_mshttpsrc_update_thread (GstMSHTTPSrc * src)
{
  GST_LOG_OBJECT (src, "The thread function start");

  while (1) {
    guint i=0;
    guint avail;
    /* block until the next scheduled update or the signal to quit this thread */
    if(src->end_of_playlist) {
      guint bufferleft = 0;
      for(i=0;i<src->session_count;i++) {
        guint avail = 0;
        g_mutex_lock (src->fetcher[i].fetcher_lock);
        avail = gst_adapter_available (src->fetcher[i].download);
        g_mutex_unlock (src->fetcher[i].fetcher_lock);
        GST_LOG_OBJECT (src, "The thread func end of playlist %d",avail);
        bufferleft+=avail;
      }
      if(!bufferleft) goto quit;
    }
    {
      GTimeVal abs_time;
      g_get_current_time (&abs_time);
      g_time_val_add (&abs_time, 30000);
      GST_LOG_OBJECT (src, "The thread func going on cond timed lock");
      g_cond_timed_wait(src->thread_cond, src->thread_lock, &abs_time);
    }
    GST_LOG_OBJECT (src, "The thread func woke up from lock");
    for(i=0;i<src->session_count;i++) {
      if(src->fetcher[i].is_active) break;
    }
    if(i==src->session_count) continue;
    g_mutex_lock (src->fetcher[i].fetcher_lock);
    avail = gst_adapter_available (src->fetcher[i].download);
    if(avail) {
      GstMSHTTPBuffer *buf = g_new0(GstMSHTTPBuffer, 1);
      GST_LOG_OBJECT (src, "The sink thread func fetcher %d buffer available %d", i, avail);
      GST_LOG_OBJECT (src, "The sink thread func index %d %d", src->fetcher[i].buf_index_cur, src->buf_index_req);
      buf->index = src->fetcher[i].buf_index_cur;
      buf->buffer = gst_adapter_take_buffer (src->fetcher[i].download, avail);

      if(src->fetcher[i].end_of_sequence) {
        guint j = 0;
        GST_LOG_OBJECT (src, "The thread func end of sequence");
        for(j=0;j<src->session_count;j++) {
          if(j == i) continue;
            g_mutex_lock (src->fetcher[j].fetcher_lock);
            if(src->fetcher[j].buf_index_cur == (src->fetcher[i].buf_index_cur+1)) {
              src->fetcher[j].is_active = TRUE;
              GST_LOG_OBJECT (src, "The fetcher[%d] activated with index ID %d", j, src->fetcher[j].buf_index_cur);
              g_mutex_unlock (src->fetcher[j].fetcher_lock);
              break;
            }
            g_mutex_unlock (src->fetcher[j].fetcher_lock);
          }
          if(j<src->session_count) src->fetcher[i].is_active = FALSE;
        }
        g_mutex_unlock (src->fetcher[i].fetcher_lock);
        g_cond_signal(src->fetcher[i].fetcher_cond);
        g_mutex_lock (src->queue_lock);
        g_queue_push_tail(src->queue, buf);
        g_mutex_unlock (src->queue_lock);
        GST_TASK_SIGNAL (src->task);
        GST_LOG_OBJECT (src, "The sink thread func buffer available and sorted insert index %d %d", avail, buf->index);
      }
      else g_mutex_unlock (src->fetcher[i].fetcher_lock);
    }
quit:
  {
    GST_LOG_OBJECT (src, "The thread function stop");
    src->thread_return = TRUE;
    GST_TASK_SIGNAL (src->task);
    return TRUE;
  }
}

static void
gst_mshttpsrc_stop_fetcher (GstMSHTTPFetcher* fetch, gboolean cancelled)
{
  GstPad *pad;

  g_return_if_fail (fetch != NULL);
  g_return_if_fail (fetch->fetcher != NULL);

  GST_DEBUG_OBJECT (fetch, "Stopping fetcher.");
  fetch->stopping_fetcher = TRUE;
  /* set the element state to NULL */
  gst_element_set_state (fetch->fetcher, GST_STATE_NULL);
  gst_element_get_state (fetch->fetcher, NULL, NULL, GST_CLOCK_TIME_NONE);
  /* unlink it from the internal pad */
  pad = gst_pad_get_peer (fetch->fetcherpad);
  if (pad) {
    gst_pad_unlink (pad, fetch->fetcherpad);
    gst_object_unref (pad);
  }

  /* if we stopped it to cancel a download, free the cached buffer */
  g_mutex_lock (fetch->fetcher_lock);
  if (cancelled && !gst_adapter_available (fetch->download)) {
    gst_adapter_clear (fetch->download);
    /* signal the fetcher thread that the download has finished/cancelled */
    g_cond_signal (fetch->fetcher_cond);
  }
  g_mutex_unlock (fetch->fetcher_lock);
  GST_DEBUG_OBJECT (fetch, "Stopped fetcher.");
}

static void
gst_mshttpsrc_stop (GstMSHTTPSrc * src)
{
  guint i=0;
  GST_LOG_OBJECT (src, "stopping fetchers");
  if(src->fetcher) {
    for(;i<src->session_count;i++) {
      GST_LOG_OBJECT (src, "stopping fetcher %d",i);
      gst_mshttpsrc_stop_fetcher (&src->fetcher[i], TRUE);
      g_mutex_lock (src->fetcher[i].fetcher_lock);
      if(gst_adapter_available (src->fetcher[i].download)) {
        gst_adapter_clear (src->fetcher[i].download);
        /* signal the fetcher thread that the download has finished/cancelled */
        g_cond_signal (src->fetcher[i].fetcher_cond);
      }
      g_mutex_unlock (src->fetcher[i].fetcher_lock);
    }
  }
  GST_LOG_OBJECT (src, "flushing queue");
  g_mutex_lock (src->queue_lock);
  while (!g_queue_is_empty (src->queue)) {
    GstMSHTTPBuffer *mc_buf = (GstMSHTTPBuffer *)g_queue_peek_head (src->queue);
    gst_buffer_unref (mc_buf->buffer);
    g_queue_remove(src->queue, mc_buf);
    g_free(mc_buf);
  }
  g_queue_clear(src->queue);
  g_mutex_unlock (src->queue_lock);

  GST_LOG_OBJECT (src, "stopping thread function");
  src->end_of_playlist = TRUE;
  g_cond_signal (src->thread_cond);
  if(src->updates_thread)
    g_thread_join(src->updates_thread);

  GST_LOG_OBJECT (src, "stopping task function");
  if (GST_TASK_STATE (src->task) != GST_TASK_STOPPED) {
    gst_task_stop (src->task);
    gst_task_join (src->task);
  }
  GST_LOG_OBJECT (src, "fetchers, thread, task stopped");
}

static void
gst_mshttpsrc_reset (GstMSHTTPSrc * src, gboolean dispose)
{
  guint i = 0;
  src->thread_return = FALSE;
  src->end_of_playlist = FALSE;
  GST_DEBUG ("gst_mshttpsrc_reset in");

  if(src->fetcher) {
    for(;i<src->session_count;i++) {
      if(src->fetcher[i].download) {
        gst_adapter_clear (src->fetcher[i].download);
        g_object_unref (src->fetcher[i].download);
        src->fetcher[i].download = NULL;
      }
      if(src->fetcher[i].fetcher_cond) {
        g_cond_free (src->fetcher[i].fetcher_cond);
        src->fetcher[i].fetcher_cond = NULL;
      }
      if(src->fetcher[i].fetcher_lock) {
        g_mutex_free (src->fetcher[i].fetcher_lock);
        src->fetcher[i].fetcher_lock = NULL;
      }
      if(src->fetcher[i].cond_lock) {
        g_mutex_free (src->fetcher[i].cond_lock);
        src->fetcher[i].cond_lock = NULL;
      }
      if(src->fetcher[i].fetcher_bus) {
        gst_object_unref (src->fetcher[i].fetcher_bus);
        src->fetcher[i].fetcher_bus = NULL;
      }
      if(src->fetcher[i].fetcherpad) {
        gst_object_unref (src->fetcher[i].fetcherpad);
        src->fetcher[i].fetcherpad = NULL;
      }
      if(src->fetcher[i].fetcher) {
        gst_object_unref (src->fetcher[i].fetcher);
        src->fetcher[i].fetcher = NULL;
      }
    }
    g_free(src->fetcher);
    src->fetcher = NULL;
    GST_DEBUG ("fetcher elements freed");
  }

  if(src->user_agent) {
    g_free (src->user_agent);
    src->user_agent = NULL;
  }
  if(src->proxy) {
    g_free (src->proxy);
    src->proxy = NULL;
  }
  if(src->user_id) {
    g_free (src->user_id);
    src->user_id = NULL;
  }
  if(src->user_pw) {
    g_free (src->user_pw);
    src->user_pw = NULL;
  }
  if(src->proxy_id) {
    g_free (src->proxy_id);
    src->proxy_id = NULL;
  }
  if(src->proxy_pw) {
    g_free (src->proxy_pw);
    src->proxy_pw = NULL;
  }
  if(src->cookies) {
    g_strfreev (src->cookies);
    src->cookies = NULL;
  }
  if(src->extra_headers) {
    gst_structure_free (src->extra_headers);
    src->extra_headers = NULL;
  }
  if(src->uri_list) {
    g_list_free (src->uri_list);
    src->uri_list = NULL;
  }
  GST_DEBUG ("src properties freed");

  if(src->updates_thread) {
    src->updates_thread = NULL;
  }
  if(src->thread_lock) {
    g_mutex_free (src->thread_lock);
    src->thread_lock = NULL;
  }
  if(src->thread_cond) {
    g_cond_free (src->thread_cond);
    src->thread_cond = NULL;
  }
  if(src->index_lock) {
    g_mutex_free (src->index_lock);
    src->index_lock = NULL;
  }
  GST_DEBUG ("Thread elements freed");

  if(src->task) {
    src->task = NULL;
  }
  g_static_rec_mutex_free (&src->task_lock);
  if(src->queue) {
    g_queue_free (src->queue);
    src->queue = NULL;
  }
  if(src->queue_lock) {
    g_mutex_free (src->queue_lock);
    src->queue_lock = NULL;
  }
  GST_DEBUG ("Queue elements freed");

  if(src->seek_event) {
    gst_event_unref (src->seek_event);
    src->seek_event = NULL;
  }
  GST_DEBUG ("gst_mshttpsrc_reset out");
}

static void
gst_mshttpsrc_dispose (GObject * obj)
{
  GstMSHTTPSrc *src;
  if (obj == NULL)
    return;
  GST_DEBUG ("gst_mshttpsrc_dispose in");

  src = GST_MSHTTP_SRC (obj);
  gst_mshttpsrc_stop (src);
  gst_mshttpsrc_reset (src, TRUE);

  G_OBJECT_CLASS (parent_class)->dispose (obj);
  GST_DEBUG ("gst_mshttpsrc_dispose out");
}

////////////////////////////////////////////////////////
//        GStreamer plugin register                   //
////////////////////////////////////////////////////////
/**
 * plugin_init
 * This function initialises the plugin
 *
 * @param              GstPlugin
 * @return             gboolean
 * @remark             This function is called while loading the plugin file from the gstreamer application
 * Initial coding done on 29th September 2009
 */
static gboolean plugin_init(GstPlugin* plugin)
{
  return gst_element_register(plugin, "mshttpsrc", GST_RANK_PRIMARY, GST_TYPE_MSHTTP_SRC);
}

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
        GST_VERSION_MINOR,
        "mshttpsrc",
        "Multi Session HTTP Src",
        plugin_init,
        VERSION,
        "Proprietary",
        "Samsung Electronics Co",
        "http://www.samsung.com")
