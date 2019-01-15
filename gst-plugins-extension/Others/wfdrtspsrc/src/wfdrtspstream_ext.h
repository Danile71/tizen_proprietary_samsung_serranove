/*
 * wfdrtspstream
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

#ifndef __GST_WFDSAMSUNGEXT_H__
#define __GST_WFDSAMSUNGEXT_H__

#include <gst/gst.h>
#include <gst/rtsp/gstrtspconnection.h>
#include <gst/rtsp/gstrtspurl.h>
#include <sys/socket.h>


G_BEGIN_DECLS

#define WFD_PORT_NUM 3

#define RETRANSMITTED_RTP_PORT 19120
#define RTCP_FB_PORT 19121

#define T1_REQUEST_STRING "wfd_vnd_sec_max_buffer_length"
#define T2_REQUEST_STRING "wfd_vnd_sec_control_playback"

typedef struct _GstWFDRTSPConnInfo GstWFDRTSPConnInfo;

struct _GstWFDRTSPConnInfo {
  gchar              *location;
  GstRTSPUrl         *url;
  gchar              *url_str;
  GstRTSPConnection  *connection;
  gboolean            connected;
};

typedef struct _WFDRTSPStream WFDRTSPStream;

struct _WFDRTSPStream {
  gint          id;

  GstElement   *parent; /* parent, no extra ref to parent is taken */

  /* pad we expose or NULL when it does not have an actual pad */
  GstPad       *srcpad;
  GstFlowReturn last_ret;
  gboolean      added;
  gboolean      disabled;
  gboolean      eos;
  gboolean      discont;

  /* for interleaved mode */
  GstCaps      *caps;
  GstPad       *channelpad[WFD_PORT_NUM];

  /* our udp sources */
  GstElement   *udpsrc[WFD_PORT_NUM];
  GstPad       *blockedpad;
  gboolean      is_ipv6;

  /* our udp sinks back to the server */
  GstElement   *udpsink[WFD_PORT_NUM];
  GstPad       *rtcppad;

  /* fakesrc for sending dummy data */
  GstElement   *fakesrc;

  /* state */
  gint          pt;
  guint         port;
  guint32       ssrc;
  /* original control url */
  gchar        *control_url;
  GstRTSPConnection *control_connection;

  guint32       seqbase;
  guint64       timebase;

  /* per stream connection */
  GstWFDRTSPConnInfo  conninfo;

  /* pipeline */
  GstElement      *requester;
  GstElement      *session;
  GstElement      *jitterbuffer;

  /* destination */
  gchar        *destination;

  /*For tcp*/
  GStaticRecMutex tcp_task_lock;
  GstTask *tcp_task;
  GStaticRecMutex tcp_status_task_lock;
  GstTask *tcp_status_report_task;
  int tcp_socket;

  /* samsung specific */
  gchar *device_name;
  gboolean do_request;

  /* samsung specific : T1 */
  GstRTSPLowerTrans  protocol;

  /* samsung specific : T2 */
  gboolean buffering_animation;

  /* samsung specific : T3 */
  GstElement *demux_handle;
  GstElement *audio_queue_handle;
  gint32 audio_data_per_sec;
};

GstRTSPResult
wfd_stream_ext_hadle_request (WFDRTSPStream * stream, guint8 *data, GstRTSPMessage * request, gboolean *check_result, gboolean *need_idr);

void wfd_samsung_ext_free_tcp (WFDRTSPStream *stream);


G_END_DECLS

#endif /* __GST_WFDSAMSUNGEXT_H__ */

