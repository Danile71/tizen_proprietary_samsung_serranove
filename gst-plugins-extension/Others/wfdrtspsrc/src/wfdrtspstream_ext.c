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


 #ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <stdio.h>
#include <stdarg.h>

#include <sys/ioctl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>

#include "wfdrtspstream_ext.h"

#ifdef G_OS_WIN32
#include <winsock2.h>
#endif

static gboolean
wfd_stream_ext_push_event (WFDRTSPStream * stream, GstEvent * event, gboolean source)
{
  gboolean res = TRUE;

  /* only streams that have a connection to the outside world */
  if (stream->srcpad == NULL)
    goto done;

  if (source && stream->udpsrc[0]) {
    gst_event_ref (event);
    res = gst_element_send_event (stream->udpsrc[0], event);
  } else if (stream->channelpad[0]) {
    gst_event_ref (event);
    if (GST_PAD_IS_SRC (stream->channelpad[0]))
      res = gst_pad_push_event (stream->channelpad[0], event);
    else
      res = gst_pad_send_event (stream->channelpad[0], event);
  }

done:
  gst_event_unref (event);

  return res;
}

static void
wfd_stream_ext_flush (WFDRTSPStream * stream, gboolean flush)
{
  GstEvent *event;
  GstClock *clock;
  GstClockTime base_time = GST_CLOCK_TIME_NONE;
  gint i;

  if (flush) {
    event = gst_event_new_flush_start ();
    GST_ERROR ("start flush");
  } else {
    event = gst_event_new_flush_stop ();
    GST_ERROR ("stop flush");
    clock = gst_element_get_clock (GST_ELEMENT_CAST (stream->parent));
    if (clock) {
      base_time = gst_clock_get_time (clock);
      gst_object_unref (clock);
    }
  }
  wfd_stream_ext_push_event (stream, event, FALSE);

  if (stream->session) {
    gst_element_set_base_time (GST_ELEMENT_CAST (stream->session), base_time);
  }
  if (stream->requester) {
    gst_element_set_base_time (GST_ELEMENT_CAST (stream->requester), base_time);
  }
  if (stream->jitterbuffer) {
    gst_element_set_base_time (GST_ELEMENT_CAST (stream->jitterbuffer), base_time);
  }

  /* make running time start start at 0 again */
  for (i = 0; i < 3; i++) {
    /* for udp case */
    if (stream->udpsrc[i]) {
      if (base_time != -1)
        gst_element_set_base_time (stream->udpsrc[i], base_time);
    }
  }

  /* for tcp interleaved case */
  if (base_time != -1)
    gst_element_set_base_time (GST_ELEMENT_CAST (stream->parent), base_time);
}


static gchar* wfd_stream_ext_parse_parametr (gchar* data, const gchar* delim)
{
  gchar **splited_message;
  gchar *res;
  splited_message = g_strsplit ((gchar*)data, delim, 2);
  res = g_strdup (splited_message[1]);
  g_strfreev (splited_message);

  return res;
}


static GstElement*
_create_udpsrc (WFDRTSPStream *stream, gint port)
{
  const gchar *host;
  GstElement *udpsrc;
  GstStateChangeReturn ret;

  if (stream->is_ipv6)
    host = "udp://[::0]";
  else
    host = "udp://0.0.0.0";

  udpsrc = gst_element_make_from_uri (GST_URI_SRC, host, NULL);
  if (udpsrc == NULL)
    return NULL;
  g_object_set (G_OBJECT (udpsrc), "port", port, "reuse", FALSE, NULL);

  ret = gst_element_set_state (udpsrc, GST_STATE_PAUSED);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    gst_object_unref (udpsrc);
    return NULL;
  }

  /* we keep these elements, we configure all in configure_transport when the
   * server told us to really use the UDP ports. */
  gst_object_ref (udpsrc);

  /* they are ours now */
  gst_object_sink (udpsrc);
  gst_bin_add (GST_BIN_CAST (stream->parent), udpsrc);
  return udpsrc;
}

void
wfd_stream_ext_free_tcp (WFDRTSPStream *stream)
{
  if (stream->tcp_task) {
    GST_ERROR ("Closing tcp loop");
    gst_task_stop (stream->tcp_task);
    if (stream->conninfo.connection)
      gst_rtsp_connection_flush (stream->conninfo.connection, TRUE);
    gst_task_join (stream->tcp_task);
    gst_object_unref (stream->tcp_task);
    g_static_rec_mutex_free (&stream->tcp_task_lock);
    stream->tcp_task = NULL;
    if(stream->tcp_socket) {
      close (stream->tcp_socket);
      stream->tcp_socket = 0;
    }
    GST_ERROR ("Tcp connection closed\n");
  }

  if(stream->tcp_status_report_task) {
    gst_task_stop (stream->tcp_status_report_task);
    gst_task_join (stream->tcp_status_report_task);
    gst_object_unref (stream->tcp_status_report_task);
    g_static_rec_mutex_free (&stream->tcp_status_task_lock);
    stream->tcp_status_report_task = NULL;
  }
}


static GstRTSPResult
wfd_stream_ext_switch_to_udp (WFDRTSPStream * stream, gint64 rtp_port, gint64 rtcp_port, GstRTSPMessage response)
{
  GstRTSPResult res = GST_RTSP_OK;
  GstEvent *event = NULL;
  GstBus *bus;
  GstPad *pad;

  g_return_val_if_fail (stream, GST_RTSP_EINVAL);
  g_return_val_if_fail (stream->control_connection, GST_RTSP_EINVAL);

  if (stream->protocol == GST_RTSP_LOWER_TRANS_UDP) {
    GST_ERROR  ("Src transport is already UDP");
    return GST_RTSP_OK;
  }

  wfd_stream_ext_flush (stream, TRUE);

  wfd_stream_ext_free_tcp (stream);
  if (stream->conninfo.connection) {
    GST_ERROR ("freeing connection...");
    gst_rtsp_connection_free (stream->conninfo.connection);
    stream->conninfo.connection = NULL;
  }

  /*Allocate udpsrc and link them.*/
  if ( (stream->udpsrc[0] = _create_udpsrc (stream, rtp_port)) == NULL
   || ((stream->udpsrc[1] = _create_udpsrc (stream, rtcp_port)) == NULL)){
    return GST_RTSP_ERROR;
  }
  if (stream->do_request) {
    if ( (stream->udpsrc[2] = _create_udpsrc (stream, 19121)) == NULL)
      return GST_RTSP_ERROR;
  }

  pad = gst_element_get_static_pad (stream->udpsrc[0], "src");
  if (gst_pad_link(pad,stream->channelpad[0]) != GST_PAD_LINK_OK) {
    GST_ERROR ("Can not link udpsrc%d to channel pad.\n", 0);
    return GST_RTSP_ERROR;
  }
  pad = gst_element_get_static_pad (stream->udpsrc[1], "src");
  if (gst_pad_link (pad,stream->channelpad[1]) != GST_PAD_LINK_OK) {
    GST_ERROR ("Can not link udpsrc%d to channel pad.\n", 1);
    return GST_RTSP_ERROR;
  }
  if (stream->do_request) {
    pad = gst_element_get_static_pad (stream->udpsrc[2], "src");
    if (gst_pad_link (pad,stream->channelpad[2]) != GST_PAD_LINK_OK) {
      GST_ERROR ("Can not link udpsrc%d to channel pad.\n", 2);
      return GST_RTSP_ERROR;
    }
  }
  if (stream->requester)
    g_object_set (stream->requester, "do-request", stream->do_request, NULL);

  /* flush stop and send custon event */
  wfd_stream_ext_flush (stream, FALSE);

  event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
                                       gst_structure_new ("GstWFDRequest", "need_segment", G_TYPE_BOOLEAN, TRUE, NULL));
  if (!wfd_stream_ext_push_event (stream, event, FALSE))
    GST_ERROR ("fail to push custom event for tsdemux to send new segment event");


  gst_element_set_state (stream->udpsrc[0], GST_STATE_PLAYING);
  gst_element_set_state (stream->udpsrc[1], GST_STATE_PLAYING);
  if (stream->do_request)
    gst_element_set_state (stream->udpsrc[2], GST_STATE_PLAYING);


  res = gst_rtsp_connection_send (stream->control_connection, &response, NULL);
  gst_rtsp_message_unset (&response);
  if(res != GST_RTSP_OK)
   return res;

  /*Informing application for changing transport.*/
  bus = gst_element_get_bus (GST_ELEMENT_CAST (stream->parent));
  gst_bus_post (bus, gst_message_new_application (GST_OBJECT_CAST(stream->parent),gst_structure_empty_new ("SWITCH_TO_UDP")));
  gst_object_unref (bus);

  stream->protocol = GST_RTSP_LOWER_TRANS_UDP;

  GST_ERROR ("Transport change to UDP");

  return GST_RTSP_OK;
}

static GstRTSPResult
wfd_stream_ext_sink_status_report (WFDRTSPStream * stream)
{
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPResult res = GST_RTSP_OK;
  GstRTSPMethod method;
  GString *msg = NULL;
  GString *msglength = NULL;
  gint msglen;
  gchar * temp = NULL;
  guint64 pts = 0;
  gint percent = 0;
  gint32 buffer_left = 0;
  gint32 audio_buffer_left = 0;
  gint32 audio_queue_data = 0;
  gint jitter_latency = 0;

  /* set a method to use for sink status report */
  method = GST_RTSP_SET_PARAMETER;

  if (!stream->control_url)
    goto no_control;

  msg = g_string_new("");
  if (msg == NULL)
    goto send_error;

  if (stream->demux_handle)
    g_object_get (stream->demux_handle, "current-PTS", &pts, NULL);
  if (stream->audio_queue_handle)
    g_object_get (stream->audio_queue_handle, "current-level-bytes", &audio_queue_data, NULL);
  if (stream->jitterbuffer) {
    g_object_get (stream->jitterbuffer, "percent", &percent, NULL);
    g_object_get (stream->jitterbuffer, "latency", &jitter_latency, NULL);
  }

  audio_buffer_left = (jitter_latency * percent)/100;
  buffer_left = (stream->audio_data_per_sec * audio_buffer_left)/1000;
  buffer_left = buffer_left + audio_queue_data;

  GST_ERROR ("buffer left %d and audio buffer left %d", buffer_left, audio_buffer_left);

  g_string_append_printf (msg, "wfd_vnd_sec_current_audio_buffer_size:");
  g_string_append_printf (msg, " %d\r\n", buffer_left);
  g_string_append_printf (msg, "wfd_vnd_sec_current_audio_decoded_pts:");
  g_string_append_printf (msg, " %lld\r\n", pts);
  msglen = strlen (msg->str);
  msglength = g_string_new ("");
  if (msglength == NULL) {
    goto send_error;
  }
  g_string_append_printf (msglength, "%d", msglen);

  res = gst_rtsp_message_init_request (&request, method, stream->control_url);
  if (res < 0)
    goto send_error;

  temp = g_string_free (msglength, FALSE);
  res = gst_rtsp_message_add_header (&request, GST_RTSP_HDR_CONTENT_LENGTH, temp);
  if (res != GST_RTSP_OK)
  {
    GST_ERROR ("Failed to set body to rtsp request...");
    goto send_error;
  }
  g_free (temp);
  temp = g_string_free (msg, FALSE);
  res = gst_rtsp_message_set_body (&request, (guint8 *)temp, msglen);
  if (res != GST_RTSP_OK)
  {
    GST_ERROR ( "Failed to set body to rtsp request...");
    goto send_error;
  }

  GST_ERROR ("send sink status report with timestamp : %lld", pts);
  if (gst_rtsp_connection_send (stream->control_connection, &request, NULL) < 0){
    goto send_error;
  }

  gst_rtsp_message_dump (&request);
  gst_rtsp_message_unset (&request);

  g_free (temp);

  sleep (1);

  return res;

  /* ERRORS */
no_control:
  {
    GST_ERROR ("no control url to send sink status report");
    res = GST_FLOW_WRONG_STATE;

    if (stream->tcp_status_report_task)
      gst_task_pause (stream->tcp_status_report_task);

    return res;
  }
/*no_connection:
  {
    GST_ERROR ("we are not connected");
    res = GST_FLOW_WRONG_STATE;

    if (stream->tcp_status_report_task)
      gst_task_pause (stream->tcp_status_report_task);

    return res;
  }*/
send_error:
  {
    GST_ERROR ("Could not send sink status report. (%s)", gst_rtsp_strresult (res));

    g_free (temp);

    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);
    return res;
  }

}

static GstFlowReturn
wfd_stream_ext_loop_tcp (WFDRTSPStream *stream)
{
  GstRTSPResult res;
  GstPad *outpad = NULL;
  GstFlowReturn ret = GST_FLOW_OK;
  guint8 *sizedata, *datatmp;
  gint message_size_length = 2, message_size;
  GstBuffer *buf;
  GTimeVal tv_timeout;

  if (!stream->conninfo.connection)
    goto no_connection;

  /* get the next timeout interval */
  gst_rtsp_connection_next_timeout (stream->conninfo.connection, &tv_timeout);
  if (tv_timeout.tv_sec == 0) {
    gst_rtsp_connection_reset_timeout (stream->conninfo.connection);
    gst_rtsp_connection_next_timeout (stream->conninfo.connection, &tv_timeout);
    GST_ERROR ("doing receive with timeout %ld seconds, %ld usec",
        tv_timeout.tv_sec, tv_timeout.tv_usec);
  }
  /*In rtp message over TCP the first 2 bytes are message size.
   * So firtstly read rtp message size.*/
  sizedata = (guint8 *)malloc (message_size_length);
  if((res = gst_rtsp_connection_read (stream->conninfo.connection,sizedata, message_size_length, &tv_timeout))
      !=GST_RTSP_OK){
    ret = GST_FLOW_ERROR;
    switch (res) {
      case GST_RTSP_EINTR:
        {
          GST_ERROR ("Got interrupted\n");
          if (stream->conninfo.connection)
            gst_rtsp_connection_flush (stream->conninfo.connection, FALSE);
          break;
        }
      default:
        {
          GST_ERROR ("Got error %d\n", res);
          break;
        }
    }
    g_free(sizedata);
    return ret;
  }
  message_size = ((guint)sizedata[0] << 8) | sizedata[1];
  datatmp = (guint8 *) malloc (message_size);
  g_free(sizedata);
  if((res = gst_rtsp_connection_read (stream->conninfo.connection,datatmp,message_size, &tv_timeout))
      !=GST_RTSP_OK){
    ret = GST_FLOW_ERROR;
    switch (res) {
      case GST_RTSP_EINTR:
        {
          GST_ERROR ("Got interrupted\n");
          if (stream->conninfo.connection)
            gst_rtsp_connection_flush (stream->conninfo.connection, FALSE);
          break;
        }
      default:
        {
          GST_ERROR ("Got error %d\n", res);
          break;
        }
    }
    g_free(datatmp);
    return ret;
  }
  /*first byte of data is type of payload
   * 200 is rtcp type then we need other pad*/
  if(datatmp[0] == 200)
    outpad = stream->channelpad[1];
  else
    outpad = stream->channelpad[0];

  buf = gst_buffer_new ();
  GST_BUFFER_DATA (buf) = datatmp;
  GST_BUFFER_MALLOCDATA (buf) = datatmp;
  GST_BUFFER_SIZE (buf) = message_size;

  if (stream->discont) {
    /* mark first RTP buffer as discont */
    GST_BUFFER_FLAG_SET (buf, GST_BUFFER_FLAG_DISCONT);
    stream->discont = FALSE;
  }

  if (GST_PAD_IS_SINK (outpad))
    ret = gst_pad_chain (outpad, buf);
  else
    ret = gst_pad_push (outpad, buf);

  return ret;

  /* ERRORS */
no_connection:
  {
    GST_ERROR ("we are not connected");
    ret = GST_FLOW_WRONG_STATE;

    if (stream->tcp_task)
      gst_task_pause (stream->tcp_task);

    return ret;
  }
}

static GstRTSPResult
wfd_stream_ext_create_socket (gint port, gint *tcp_socket)
{
  /* length of address structure */
  struct sockaddr_in my_addr;
  /* client's address */
  gint sockoptval = 1;
  int res;

  /* create a TCP/IP socket */
  if ((*tcp_socket = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
    GST_ERROR ("cannot create socket");
    return GST_RTSP_ERROR;
  }
  /* allow immediate reuse of the port */
  res = setsockopt (*tcp_socket, SOL_SOCKET, SO_REUSEADDR, &sockoptval, sizeof(int));
  if(res != 0)
  {
    GST_ERROR ("cannot change socket options");
    return GST_RTSP_ERROR;
  }
  /* bind the socket to our source address */
  memset ((char*)&my_addr, 0, sizeof(my_addr));
  /* 0 out the structure */
  my_addr.sin_family = AF_INET;
  /* address family */
  my_addr.sin_port = htons (port);
  if (bind (*tcp_socket, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0) {
    GST_ERROR ("cannot bind socket");
    close (*tcp_socket);
    return GST_RTSP_ERROR;
  }
  /* set the socket for listening (queue backlog of 5) */
  if (listen(*tcp_socket, 5) < 0) {
    close (*tcp_socket);
    GST_ERROR ("error while listening socket");
    return GST_RTSP_ERROR;
  }
  return GST_RTSP_OK;
}

static void
wfd_stream_ext_free_udp (WFDRTSPStream * stream)
{
  gint i;

  for (i = 0; i < 3; i++) {
    if (stream->udpsrc[i]) {
      gst_bin_remove (GST_BIN_CAST(stream->parent), stream->udpsrc[i]);
      gst_element_set_state (stream->udpsrc[i], GST_STATE_NULL);
      gst_object_unref (stream->udpsrc[i]);
      stream->udpsrc[i] = NULL;
    }
  }
}

static GstRTSPResult
wfd_stream_ext_switch_to_tcp (WFDRTSPStream * stream, gint64 port, GstRTSPMessage response)
{
  GstRTSPResult res;
  int tcp_socket;
  GstBus *bus;
  GstPad *udp_pad;

  g_return_val_if_fail (stream, GST_RTSP_EINVAL);
  g_return_val_if_fail (stream->control_connection, GST_RTSP_EINVAL);

  if (stream->protocol == GST_RTSP_LOWER_TRANS_TCP) {
    GST_ERROR ("Src transport is already TCP");
    return GST_RTSP_OK;
  }

  /* flush start */
  wfd_stream_ext_flush (stream, TRUE);

  if (stream->udpsrc[0]) {
    udp_pad = gst_element_get_static_pad (stream->udpsrc[0],"src");
    gst_pad_unlink (udp_pad, stream->channelpad[0]);
    gst_object_unref (udp_pad);
  }
  if (stream->udpsrc[1]) {
    udp_pad = gst_element_get_static_pad (stream->udpsrc[1],"src");
    gst_pad_unlink (udp_pad, stream->channelpad[1]);
    gst_object_unref (udp_pad);
  }
  if (stream->udpsrc[2] && stream->do_request) {
    udp_pad = gst_element_get_static_pad(stream->udpsrc[2],"src");
    gst_pad_unlink (udp_pad, stream->channelpad[2]);
    gst_object_unref (udp_pad);
  }

  wfd_stream_ext_free_udp (stream);
  if (stream->conninfo.connection) {
    GST_ERROR ("freeing connection...");
    gst_rtsp_connection_free (stream->conninfo.connection);
    stream->conninfo.connection = NULL;
  }

  if (stream->requester)
    g_object_set (stream->requester, "do-request", FALSE, NULL);

  /* Open tcp socket*/
  res = wfd_stream_ext_create_socket (port, &tcp_socket);
  if(res != GST_RTSP_OK) {
    GST_ERROR ("fail to create tcp socket");
    return res;
  }

   /* sending message after changing transport */
   res = gst_rtsp_connection_send (stream->control_connection, &response, NULL);
   gst_rtsp_message_unset (&response);
   if(res != GST_RTSP_OK) {
    close (tcp_socket);
    return res;
   }

  if (GST_RTSP_OK != gst_rtsp_connection_accept (tcp_socket, &stream->conninfo.connection)) {
    close (tcp_socket);
    GST_ERROR ("Server failed to accept\n");
    return GST_RTSP_ERROR;
  } else {
    GstEvent *event;

    /* flush stop and send custon event */
    wfd_stream_ext_flush (stream, FALSE);

    event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
                                         gst_structure_new ("GstWFDRequest", "need_segment", G_TYPE_BOOLEAN, TRUE, NULL));
    if (!wfd_stream_ext_push_event (stream, event, FALSE))
      GST_ERROR ("fail to push custom event for tsdemux to send new segment event");

    stream->discont = TRUE;
    stream->tcp_socket = tcp_socket;
    stream->tcp_task = gst_task_create ((GstTaskFunction) wfd_stream_ext_loop_tcp, stream);
    g_static_rec_mutex_init (&stream->tcp_task_lock);
    gst_task_set_lock (stream->tcp_task, &stream->tcp_task_lock);
    gst_task_start (stream->tcp_task);

    /* Start new thread to send status report to wfd src */
    stream->tcp_status_report_task = gst_task_create ((GstTaskFunction) wfd_stream_ext_sink_status_report, stream);
    g_static_rec_mutex_init (&stream->tcp_status_task_lock);
    gst_task_set_lock (stream->tcp_status_report_task, &stream->tcp_status_task_lock);
    gst_task_start (stream->tcp_status_report_task);
  }

   /*Informing application for changing transport.*/
   bus = gst_element_get_bus (GST_ELEMENT_CAST (stream->parent));
   gst_bus_post (bus, gst_message_new_application (GST_OBJECT_CAST(stream->parent), gst_structure_empty_new ("SWITCH_TO_TCP")));
   gst_object_unref (bus);

   stream->protocol = GST_RTSP_LOWER_TRANS_TCP;

   GST_ERROR ("Transport change to TCP");

   return GST_RTSP_OK;
}

static gboolean
wfd_stream_ext_rename_sink (WFDRTSPStream *stream, guint8 *data)
{
  gchar *p = (gchar*)data;
  gchar value[8192]= {0};
  guint idx = 0;

  while (*p != ':' && *p != '\0')
    p++;

  if (*p && g_ascii_isspace(*p))
    p++;

  while (*p != '\0' && *p != '\r' && *p != '\n')  {
    if (idx < 8191)
      value[idx++] = *p;
    p++;
  }

  value[idx] = '\0';

  if (stream->device_name)
    g_free (stream->device_name);

  stream->device_name = g_strdup(value);

  if (stream->device_name)
    return TRUE;

  return FALSE;
}


static GstRTSPResult
wfd_stream_ext_handle_rename_request (WFDRTSPStream * stream, guint8 *data, GstRTSPMessage *request)
{
  GstRTSPMessage response = { 0 };
  GstRTSPResult res = GST_RTSP_OK;
  gboolean rename = FALSE;

  g_return_val_if_fail (stream, GST_RTSP_EINVAL);
  g_return_val_if_fail (data, GST_RTSP_EINVAL);
  g_return_val_if_fail (request, GST_RTSP_EINVAL);

  rename = wfd_stream_ext_rename_sink (stream, data);
  if (rename /*&& vconf_set_str ("VCONFKEY_MIRACAST_WFD_SINK_NAME", stream->device_name)*/) {
    GST_ERROR ("Sink device renamed. Name is %s", stream->device_name);
  } else {
    res = GST_RTSP_ERROR;
    goto send_error;
  }

  res = gst_rtsp_message_init_response (&response, GST_RTSP_STS_OK, "OK",
       request);
  if (res < 0)
    goto send_error;

  res = gst_rtsp_connection_send (stream->control_connection, &response, NULL);
  if (res < 0)
    goto send_error;

  return res;

/* ERRORS */
send_error:
  {
    gst_rtsp_message_unset (request);
    gst_rtsp_message_unset (&response);
    return res;
  }
}

static GstRTSPResult
wfd_stream_ext_handle_T1_request (WFDRTSPStream * stream, guint8 *data, GstRTSPMessage *request, gboolean *need_idr)
{
  GstRTSPMessage response = { 0 };
  GstRTSPResult res = GST_RTSP_OK;
  gchar **splited_param;
  gchar * msg = (gchar *)data;
  gchar * parametr;
  gint64 max_buf_length, rtp_port, rtcp_port;
  gchar * resp;

  g_return_val_if_fail (stream, GST_RTSP_EINVAL);
  g_return_val_if_fail (data, GST_RTSP_EINVAL);
  g_return_val_if_fail (request, GST_RTSP_EINVAL);

  parametr = wfd_stream_ext_parse_parametr(msg, "length:");
  max_buf_length = g_ascii_strtoll(parametr, NULL, 10);
  g_free (parametr);

  parametr = wfd_stream_ext_parse_parametr(msg, "ports:");
  splited_param = g_strsplit(parametr, " ", 4);

  rtp_port = g_ascii_strtoll (splited_param[2], NULL, 10);
  rtcp_port = g_ascii_strtoll (splited_param[3] , NULL, 10);
  GST_ERROR ("Switching transport. Buffer max size - %lli, ports are - %lli %lli\n", max_buf_length, rtp_port, rtcp_port);

  resp = g_strdup_printf ("wfd_vnd_sec_max_buffer_length: %d \r\nwfd_client_rtp_ports: %s %d %d mode=play\r\n",
     (gint)max_buf_length, splited_param[1], (gint)rtp_port, (gint)rtcp_port );
  g_strfreev (splited_param);

  res = gst_rtsp_message_init_response (&response, GST_RTSP_STS_OK, "OK", request);
  if (res < 0)
    goto send_error;

  res = gst_rtsp_message_set_body (&response, (guint8 *)resp, strlen(resp));
  g_free(resp);

  gst_rtsp_message_dump (&response);

  if (res < 0)
    goto send_error;

  if (g_strrstr (parametr, "TCP"))
    res = wfd_stream_ext_switch_to_tcp (stream, rtp_port, response);
  else if (g_strrstr(parametr, "UDP")) {
    res =  wfd_stream_ext_switch_to_udp (stream, rtp_port, rtcp_port, response);
   *need_idr	= TRUE;
  }

  g_free (parametr);

  return res;

/* ERRORS */
send_error:
  {
    gst_rtsp_message_unset (request);
    gst_rtsp_message_unset (&response);
    return res;
  }
}

static GstRTSPResult
wfd_stream_ext_handle_T2_request (WFDRTSPStream * stream, guint8 *data, GstRTSPMessage *request)
{
  GstRTSPMessage response = { 0 };
  GstRTSPResult res = GST_RTSP_OK;
  GstEvent* event = NULL;
  gchar *msg;
  gchar* parametr;

  g_return_val_if_fail (stream, GST_RTSP_EINVAL);
  g_return_val_if_fail (data, GST_RTSP_EINVAL);
  g_return_val_if_fail (request, GST_RTSP_EINVAL);

  res = gst_rtsp_message_init_response (&response, GST_RTSP_STS_OK, "OK",
          request);
  if (res < 0)
    goto send_error;

  res = gst_rtsp_connection_send (stream->control_connection, &response, NULL);
  if (res < 0)
    goto send_error;

  msg = (gchar *)data;

  /* FLUSH PLAY */
  if (g_strrstr (msg, "flush_play")) {
    GST_ERROR ("got flush_play");
    parametr = wfd_stream_ext_parse_parametr (msg, "flush_timing=");

    wfd_stream_ext_flush (stream, TRUE);
    wfd_stream_ext_flush (stream, FALSE);

    event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
                                   gst_structure_new ("GstWFDRequest", "need_segment", G_TYPE_BOOLEAN, TRUE,
                                   "pts", G_TYPE_STRING, parametr, NULL));

    if (!wfd_stream_ext_push_event (stream, event, FALSE))
      GST_ERROR ("failed to push custom GstWFDRequest event for tsdemux to send newsement event");

  /* FLUSH PAUSE */
  } else if (g_strrstr (msg, "flush_pause")) {
    GST_ERROR ("got flush_pause");
    parametr = wfd_stream_ext_parse_parametr (msg, "flush_timing=");

    wfd_stream_ext_flush (stream, TRUE);
    wfd_stream_ext_flush (stream, FALSE);

    event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
                                 gst_structure_new ("GstWFDRequest", "need_segment", G_TYPE_BOOLEAN, TRUE,
                                 "pts", G_TYPE_STRING, parametr, NULL));

    if (!wfd_stream_ext_push_event (stream, event, FALSE))
      GST_ERROR ("failed to push custom GstWFDRequest event for tsdemux to send newsement event");

  } else if (!g_strrstr (msg, "flush")) {
    /* SET VOLUME */
    if (g_strrstr (msg, "set_volume")) {
      GstBus *bus;
      gchar **splited_parametr;
      gchar *result;

      bus = gst_element_get_bus (GST_ELEMENT_CAST(stream->parent));
      parametr = wfd_stream_ext_parse_parametr (msg, "volume=");
      splited_parametr = g_strsplit (parametr, "\r\n", 2);
      g_free(parametr);
      parametr = g_strstrip (splited_parametr[0]);
      result = g_strdup_printf("volume_%s", parametr);

      if (!gst_bus_post (bus, gst_message_new_application (GST_OBJECT_CAST(stream->parent), gst_structure_empty_new (result))))
        GST_LOG("Failed to send volume message\n");
        g_free(result);
        gst_object_unref(bus);
        g_strfreev (splited_parametr);

    /* BUFFERING ANIMATION */
    } else if (g_strrstr (msg, "buffering_animation")) {
      parametr = wfd_stream_ext_parse_parametr (msg, "buffering_animation=");
      if(g_strrstr(parametr, "on")) {
        GST_ERROR("Buffering animation ON");
        stream->buffering_animation = TRUE;

        //vconf_set_bool ("VCONFKEY_MIRACAST_BUFFERING_ANIMATION", stream->buffering_animation);
      } else if (g_strrstr (parametr, "off")) {
        GST_ERROR ("Buffering animation OFF");

        stream->buffering_animation = FALSE;
        //vconf_set_bool ("VCONFKEY_MIRACAST_BUFFERING_ANIMATION", stream->buffering_animation);
      }
      g_free (parametr);

    /* PAUSE */
    } else if (g_strrstr (msg, "pause")) {
      GST_ERROR ("got pause");
      /* Fix : Do somthing */

    /* PLAY */
    } else if (g_strrstr (msg, "play")) {
      event = gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM,
                                 gst_structure_new ("GstWFDRequest", NULL));
      if (!wfd_stream_ext_push_event (stream, event, FALSE))
        GST_ERROR ("failed to push custom GstWFDRequest event for flushing tsdemux");
    }
  }


  return res;

/* ERRORS */
send_error:
  {
    gst_rtsp_message_unset (request);
    gst_rtsp_message_unset (&response);
    return res;
  }
}


GstRTSPResult
wfd_stream_ext_hadle_request (WFDRTSPStream * stream, guint8 *data, GstRTSPMessage * request, gboolean *check_result, gboolean *need_idr)
{
  GstRTSPResult res = GST_RTSP_OK;

  *check_result = TRUE;
  *need_idr = FALSE;

  if (g_strrstr ((gchar*)data,"wfd_vnd_sec_rename_dongle")) {
    res = wfd_stream_ext_handle_rename_request (stream, data, request);
  } else if (g_strrstr ((gchar*)data,"wfd_vnd_sec_max_buffer_length")) {
    res = wfd_stream_ext_handle_T1_request (stream, data, request, need_idr);
  } else if (g_strrstr ((gchar*)data,"wfd_vnd_sec_control_playback")) {
    res = wfd_stream_ext_handle_T2_request (stream, data, request);
  } else {
    *check_result = FALSE;
  }

  return res;
}

