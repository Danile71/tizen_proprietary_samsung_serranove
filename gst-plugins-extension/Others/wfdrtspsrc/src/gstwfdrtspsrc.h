/*
 * wfdrtspsrc
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: JongHyuk Choi <jhchoi.choi@samsung.com>
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



#ifndef __GST_WFDRTSPSRC_H__
#define __GST_WFDRTSPSRC_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#include <gst/rtsp/gstrtspconnection.h>
#include <gst/rtsp/gstrtspmessage.h>
#include <gst/rtsp/gstrtspurl.h>
#include <gst/rtsp/gstrtsprange.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <mmf/uibcmessage.h>
#include <mmf/wfdconfigmessage.h>

#include "gstwfdrtspext.h"
#ifdef ENABLE_WFD_EXTENDED_FEATURES
#include "wfdrtspstream_ext.h"
#else
#include "wfdrtspstream.h"
#endif

#define GST_TYPE_WFDRTSPSRC \
  (gst_wfdrtspsrc_get_type())
#define GST_WFDRTSPSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_WFDRTSPSRC,GstWFDRTSPSrc))
#define GST_WFDRTSPSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_WFDRTSPSRC,GstWFDRTSPSrcClass))
#define GST_IS_WFDRTSPSRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_WFDRTSPSRC))
#define GST_IS_WFDRTSPSRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_WFDRTSPSRC))
#define GST_WFDRTSPSRC_CAST(obj) \
  ((GstWFDRTSPSrc *)(obj))

typedef struct _GstWFDRTSPSrc GstWFDRTSPSrc;
typedef struct _GstWFDRTSPSrcClass GstWFDRTSPSrcClass;

#define GST_WFD_RTSP_STATE_GET_LOCK(wfd)    (GST_WFDRTSPSRC_CAST(wfd)->state_rec_lock)
#define GST_WFD_RTSP_STATE_LOCK(wfd)        (g_static_rec_mutex_lock (GST_WFD_RTSP_STATE_GET_LOCK(wfd)))
#define GST_WFD_RTSP_STATE_UNLOCK(wfd)      (g_static_rec_mutex_unlock (GST_WFD_RTSP_STATE_GET_LOCK(wfd)))

#define GST_WFD_RTSP_STREAM_GET_LOCK(wfd)   (GST_WFDRTSPSRC_CAST(wfd)->stream_rec_lock)
#define GST_WFD_RTSP_STREAM_LOCK(wfd)       (g_static_rec_mutex_lock (GST_WFD_RTSP_STREAM_GET_LOCK(wfd)))
#define GST_WFD_RTSP_STREAM_UNLOCK(wfd)     (g_static_rec_mutex_unlock (GST_WFD_RTSP_STREAM_GET_LOCK(wfd)))

#define GST_WFD_RTSP_CONN_GET_LOCK(wfd)     (GST_WFDRTSPSRC_CAST(wfd)->conn_rec_lock)
#define GST_WFD_RTSP_CONN_LOCK(wfd)         (g_static_rec_mutex_lock (GST_WFD_RTSP_CONN_GET_LOCK(wfd)))
#define GST_WFD_RTSP_CONN_UNLOCK(wfd)       (g_static_rec_mutex_unlock (GST_WFD_RTSP_CONN_GET_LOCK(wfd)))
#define GST_WFD_RTSP_CONN_WRITE_LOCK(wfd)         (g_static_rec_mutex_lock ((wfd)->conn_rec_write_lock))
#define GST_WFD_RTSP_CONN_WRITE_UNLOCK(wfd)         (g_static_rec_mutex_unlock ((wfd)->conn_rec_write_lock))



/**
 * GstWFDRTSPNatMethod:
 * @GST_WFD_RTSP_NAT_NONE: none
 * @GST_WFD_RTSP_NAT_DUMMY: send dummy packets
 *
 * Different methods for trying to traverse firewalls.
 */
typedef enum
{
  GST_WFD_RTSP_NAT_NONE,
  GST_WFD_RTSP_NAT_DUMMY
} GstWFDRTSPNatMethod;

typedef enum
{
  WFD_ROUTE,
  WFD_CONNECTOR_TYPE,
  WFD_STANDBY,
  WFD_IDR_REQUEST,
  WFD_UIBC_CAPABILITY,
  WFD_UIBC_SETTING,
  WFD_SINK_STATUS
} GstWFDParam;

typedef struct _UIBC UIBC;

struct _UIBC {
  gint gen_capability;
  WFDHIDCTypePathPair *hidc_cap_list;
  WFDHIDCTypePathPair *hidc_cap_list_src;
  gint hidc_cap_count_src;
  gint sockfd;
  guint32 port;
  void *wfd_sink_handle;
  wfd_uibc_state_cb state_cb;
};

struct _GstWFDRTSPSrc {
  GstBin           parent;

  /* task and mutex for TCP mode */
  GstTask         *task;
  GStaticRecMutex *stream_rec_lock;
  GstSegment       segment;
  gboolean         running;
  gboolean         skip;
  gint             free_channel;
  GstClockTime     base_time;

  /* UDP mode loop */
  gint             loop_cmd;
  gboolean         waiting;
  gboolean         open_error;

  /* mutex for protecting state changes */
  GStaticRecMutex *state_rec_lock;

  /* mutex for protecting the connection */
  GStaticRecMutex *conn_rec_lock;

  /*mutex for protecting writing to the connection*/
  GStaticRecMutex *conn_rec_write_lock;

  WFDRTSPStream           *stream;
  gboolean         need_activate;

  /* properties */
  GstRTSPLowerTrans protocols;
  gboolean          debug;
  guint             retry;
  guint64           udp_timeout;
  GTimeVal          tcp_timeout;
  GTimeVal         *ptcp_timeout;
  guint             latency;
  guint             connection_speed;
  GstWFDRTSPNatMethod  nat_method;
  gboolean          do_rtcp;
  gchar            *proxy_host;
  guint             proxy_port;
  gchar            *proxy_user;
  gchar            *proxy_passwd;
  guint             rtp_blocksize;
  gchar            *user_id;
  gchar            *user_pw;
  gint              buffer_mode;
  GstRTSPRange      client_port_range;
  gint              udp_buffer_size;
  /* Set User-agent */
  gchar *user_agent;
  /* Set RTP port */
  gint primary_rtpport;
  gint secondary_rtpport;

  /* state */
  GstRTSPState       state;
  gchar             *content_base;
  gboolean           tried_url_auth;
  gchar             *addr;
  gboolean           need_redirect;
  gchar             *control;
  guint              next_port_num;

  /* supported methods */
  gint               methods;

  GstWFDRTSPConnInfo  conninfo;

  /* a list of RTSP extensions as GstElement */
  GstWFDRTSPExtensionList  *extensions;


  GTimeVal ssr_timeout;
  guint video_height;
  guint video_width;
  guint video_framerate;
  gboolean is_paused;
  UIBC *uibc;
  GstStructure *audio_param;
  GstStructure *video_param;
  GstStructure *hdcp_param;
};

struct _GstWFDRTSPSrcClass {
  GstBinClass parent_class;
};

GType gst_wfdrtspsrc_get_type(void);

G_END_DECLS

#endif /* __GST_WFDRTSPSRC_H__ */
