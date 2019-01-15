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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <string.h>
#include <gst/gst.h>
#include <gst/gsterror.h>
#include <glib.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/base/gstadapter.h>
G_BEGIN_DECLS

#define GST_TYPE_SUBMUX        (gst_submux_get_type())
#define GST_SUBMUX(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SUBMUX,Gstsubmux))
#define GST_SUBMUX_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SUBMUX,GstsubmuxClass))
#define GST_SUBMUX_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_SUBMUX,GstsubmuxClass))
#define GST_IS_SUBMUX(obj)     (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SUBMUX))
#define GST_IS_SUBMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SUBMUX))
#define GST_SUBMUX_CAST(obj)    ((Gstsubmux *)(obj))
#define MAX_LANG 10
#define DISABLE_FAST_SEEK
#define FORMAT_DETECTION_TEXT_STRING 128

#define SUBMUX_FENTER(submux)      GST_DEBUG_OBJECT (submux, "%s E",__FUNCTION__);
#define SUBMUX_FLEAVE(submux)      GST_DEBUG_OBJECT (submux, "%s X",__FUNCTION__);
#define SAFE_FREE(memory) \
do \
{ \
  if (memory) { \
    g_free (memory); \
    memory = NULL; \
  } \
} while (0);

typedef struct _Gstsubmux Gstsubmux;
typedef struct _GstsubmuxClass GstsubmuxClass;
typedef struct _GstsubmuxPrivate GstsubmuxPrivate;
typedef struct _LanguageStruct  GstLangStruct;
typedef struct _GstSubMuxStream GstSubmuxStream;
typedef struct _GstSubMuxPipe SubMuxPipe;

typedef enum
{
  GST_SUB_PARSE_FORMAT_UNKNOWN = 0,
  GST_SUB_PARSE_FORMAT_MDVDSUB = 1,
  GST_SUB_PARSE_FORMAT_SUBRIP = 2,
  GST_SUB_PARSE_FORMAT_MPSUB = 3,
  GST_SUB_PARSE_FORMAT_SAMI = 4,
  GST_SUB_PARSE_FORMAT_TMPLAYER = 5,
  GST_SUB_PARSE_FORMAT_MPL2 = 6,
  GST_SUB_PARSE_FORMAT_SUBVIEWER = 7,
  GST_SUB_PARSE_FORMAT_DKS = 8,
  GST_SUB_PARSE_FORMAT_QTTEXT = 9,
  GST_SUB_PARSE_FORMAT_LRC = 10
} GstSubMuxFormat;

typedef enum
{
  GST_SUB_PARSE_REGEX_UNKNOWN = 0,
  GST_SUB_PARSE_REGEX_MDVDSUB = 1,
  GST_SUB_PARSE_REGEX_SUBRIP = 2,
  GST_SUB_PARSE_REGEX_DKS = 3,
} GstSubMuxRegex;

typedef enum
{
  PROP_0,
  PROP_ENCODING,
  PROP_VIDEOFPS,
  PROP_EXTSUB_CURRENT_LANGUAGE,
  PROP_IS_INTERNAL,
  PROP_LANG_LIST
}GstSubMuxProperties;

typedef enum
{
  EXTERNAL_FILE_PATH_UNKNOWN,
  EXTERNAL_FILE_PATH_INITIALIZED,
  EXTERNAL_FILE_PATH_CHANGED,
  EXTERNAL_FILE_PATH_UNCHANGED
}GstSubMuxExtPath;

struct _GstSubMuxPipe
{
  GstElement *pipe;
  GstElement *appsrc;
  GstElement *appsink;
  GstElement *parser;
  GstPad *app_sinkpad;
};

struct _GstSubMuxStream
{
  void *parent;
  gboolean need_segment;
  gboolean discont_came;
  gboolean flushing;
  SubMuxPipe pipe_struc;
  gboolean eos_sent;
  gboolean eos_came;
  GstClockTime duration;
  GstClockTime last_ts; /* last timestamp of subtitle present in subtitle file*/
  GstClockTime eos_ts;
  GstClockTime seek_ts;
  GstBuffer *buffer;
  GQueue *queue;
  GMutex queue_lock;
  GCond queue_empty;
  gboolean flush_done;
};

struct _Gstsubmux
{
    GstElement     element;

    /*< private >*/
    GstsubmuxPrivate *priv;

    /* pads */
    guint sinkpads_count;
    gboolean external_sinkpad;
    gboolean stop_loop;
    GstPad *srcpad;
    GList *sinkpad;
    gboolean pipeline_made;
    //GstPad *sinkpad;
    GList *buffer_list;
    gboolean init_done;
    GList *msl_streams;
    GList *streams;
    gboolean flushing;
    gboolean need_segment;
    GstSubmuxStream muxed_stream;
    gchar   *encoding;
    gchar   *detected_encoding;
    gint fps_n, fps_d;
    GstSegment    segment;
    GstTask *loop_task;
    GstSeekFlags  segment_flags;
    gboolean seek_came ;
    gboolean finalize;
    gboolean langlist_msg_posted;
    GstBuffer **cur_buf_array;
    gchar* external_filepath;
    gboolean seek_new_file;
    gboolean srcpad_loop_running;
    GMutex srcpad_loop_lock;
    GCond srcpad_loop_cond;
};

struct _GstsubmuxPrivate
{
    gboolean first_buffer;
    GstSubMuxFormat parser_type;
    GList *lang_list;
    gboolean is_internal;
    guint stream_count;
};

struct _LanguageStruct
{
    gchar *language_code;
    gchar *language_key;
    gboolean active;
};

struct _GstsubmuxClass
{
  GstElementClass parent_class;
};

GType           gst_submux_get_type         (void);

G_END_DECLS


