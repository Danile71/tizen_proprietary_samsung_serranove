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

/**
* SECTION:element-wfdrtspsrc
*
* Makes a connection to an RTSP server and read the data.
* Device recognition is through wifi direct.
* wfdrtspsrc strictly follows Wifi display specification.
*
* RTSP supports transport over TCP or UDP in unicast or multicast mode. By
* default wfdrtspsrc will negotiate a connection in the following order:
* UDP unicast/UDP multicast/TCP. The order cannot be changed but the allowed
* protocols can be controlled with the #GstWFDRTSPSrc:protocols property.
*
* wfdrtspsrc currently understands WFD capability negotiation messages
* For each stream listed in the wfd-presentation-url a new rtp_stream%d pad
* will be created.
*
* wfdrtspsrc will internally instantiate an RTP session manager element
* that will handle the RTCP messages to and from the server, jitter removal,
* packet reordering along with providing a clock for the pipeline.
* This feature is implemented using the gstrtpbin element.
*
* wfdrtspsrc acts like a live source and will therefore only generate data in the
* PLAYING state.
*
* <refsect2>
* <title>Example launch line</title>
* |[
* gst-launch wfdrtspsrc location=rtsp://some.server/url ! fakesink
* ]| Establish a connection to an RTSP server and send the raw RTP packets to a
* fakesink.
* </refsect2>
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
#include <vconf/vconf.h>

#include "gstwfdrtspsrc.h"
#include <mmf/wfdconfigmessage.h>
#include "wfdhalconfig.h"

#ifdef G_OS_WIN32
#include <winsock2.h>
#endif

GST_DEBUG_CATEGORY_STATIC (wfdrtspsrc_debug);
#define GST_CAT_DEFAULT (wfdrtspsrc_debug)

static GstStaticPadTemplate rtptemplate = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-rtp; application/x-rdt"));

enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum _GstWfdRtspSrcBufferMode
{
  BUFFER_MODE_NONE,
  BUFFER_MODE_SLAVE,
  BUFFER_MODE_BUFFER,
  BUFFER_MODE_AUTO
};

#define GST_TYPE_WFD_RTSP_SRC_BUFFER_MODE (gst_wfd_rtsp_src_buffer_mode_get_type())
static GType
gst_wfd_rtsp_src_buffer_mode_get_type (void)
{
  static GType buffer_mode_type = 0;
  static const GEnumValue buffer_modes[] = {
    {BUFFER_MODE_NONE, "Only use RTP timestamps", "none"},
    {BUFFER_MODE_SLAVE, "Slave receiver to sender clock", "slave"},
    {BUFFER_MODE_BUFFER, "Do low/high watermark buffering", "buffer"},
    {BUFFER_MODE_AUTO, "Choose mode depending on stream live", "auto"},
    {0, NULL, NULL},
  };

  if (!buffer_mode_type) {
    buffer_mode_type =
        g_enum_register_static ("GstWFDRTSPSrcBufferMode", buffer_modes);
  }
  return buffer_mode_type;
}

#define GST_TYPE_WFD_RTSP_NAT_METHOD (gst_wfd_rtsp_nat_method_get_type())
static GType
gst_wfd_rtsp_nat_method_get_type (void)
{
  static GType wfd_rtsp_nat_method_type = 0;
  static const GEnumValue wfd_rtsp_nat_method[] = {
    {GST_WFD_RTSP_NAT_NONE, "None", "none"},
    {GST_WFD_RTSP_NAT_DUMMY, "Send Dummy packets", "dummy"},
    {0, NULL, NULL},
  };

  if (!wfd_rtsp_nat_method_type) {
    wfd_rtsp_nat_method_type =
        g_enum_register_static ("GstWFDRTSPNatMethod", wfd_rtsp_nat_method);
  }
  return wfd_rtsp_nat_method_type;
}

/* properties default values */
#define DEFAULT_LOCATION         NULL
#define DEFAULT_PROTOCOLS        GST_RTSP_LOWER_TRANS_UDP |GST_RTSP_LOWER_TRANS_TCP
#define DEFAULT_DEBUG            FALSE
#define DEFAULT_RETRY            20
#define DEFAULT_TIMEOUT          5000000
#define DEFAULT_UDP_BUFFER_SIZE  0x80000
#define DEFAULT_TCP_TIMEOUT      20000000
#define DEFAULT_LATENCY_MS       2000
#define DEFAULT_CONNECTION_SPEED 0
#define DEFAULT_NAT_METHOD       GST_WFD_RTSP_NAT_DUMMY
#define DEFAULT_DO_RTCP          TRUE
#define DEFAULT_PROXY            NULL
#define DEFAULT_RTP_BLOCKSIZE    0
#define DEFAULT_USER_ID          NULL
#define DEFAULT_USER_PW          NULL
#define DEFAULT_BUFFER_MODE      BUFFER_MODE_AUTO
#define DEFAULT_PORT_RANGE       NULL
#define DEFAULT_USER_AGENT       "SEC-WDH/WFD-SINK/2.0/MI30"
#ifdef ENABLE_WFD_EXTENDED_FEATURES
#define DEFAULT_DEVICE_NAME      "Sink"
#define DEFAULT_BUF_ANIMATION    FALSE
#define DEFAULT_DO_REQUEST       TRUE
#endif
#define DEFAULT_UIBC_GEN_CAPABILITY 15
#define DEFAULT_UIBC_HIDC_CAP_COUNT 4

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_PROTOCOLS,
  PROP_DEBUG,
  PROP_RETRY,
  PROP_TIMEOUT,
  PROP_TCP_TIMEOUT,
  PROP_LATENCY,
  PROP_CONNECTION_SPEED,
  PROP_NAT_METHOD,
  PROP_DO_RTCP,
  PROP_PROXY,
  PROP_RTP_BLOCKSIZE,
  PROP_USER_ID,
  PROP_USER_PW,
  PROP_BUFFER_MODE,
  PROP_PORT_RANGE,
  PROP_UDP_BUFFER_SIZE,
  PROP_USER_AGENT,
#ifdef ENABLE_WFD_EXTENDED_FEATURES
  PROP_DEVICE_NAME,
  PROP_BUF_ANIMATION,
  PROP_DO_REQUEST,
  PROP_DEMUX_HANDLE,
  PROP_AUDIO_QUEUE_HANDLE,
#endif
  PROP_USER_INPUT_EVENT,
  PROP_UIBC_STATE_CB,
  PROP_AUDIO_PARAM,
  PROP_VIDEO_PARAM,
  PROP_HDCP_PARAM,
  PROP_LAST
};



static void gst_wfdrtspsrc_finalize (GObject * object);

static void gst_wfdrtspsrc_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_wfdrtspsrc_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void gst_wfdrtspsrc_uri_handler_init (gpointer g_iface,
    gpointer iface_data);

static gboolean gst_wfdrtspsrc_set_proxy (GstWFDRTSPSrc * rtsp, const gchar * proxy);
static void gst_wfdrtspsrc_set_tcp_timeout (GstWFDRTSPSrc * wfdrtspsrc, guint64 timeout);

static GstStateChangeReturn gst_wfdrtspsrc_change_state (GstElement * element,
    GstStateChange transition);
static gboolean gst_wfdrtspsrc_send_event (GstElement * element, GstEvent * event);
static void gst_wfdrtspsrc_handle_message (GstBin * bin, GstMessage * message);

static gboolean gst_wfdrtspsrc_setup_auth (GstWFDRTSPSrc * src,
    GstRTSPMessage * response);

static void gst_wfdrtspsrc_loop_send_cmd (GstWFDRTSPSrc * src, gint cmd,
    gboolean flush);
static GstRTSPResult gst_wfdrtspsrc_send_cb (GstRTSPExtension * ext,
    GstRTSPMessage * request, GstRTSPMessage * response, GstWFDRTSPSrc * src);

static GstRTSPResult gst_wfdrtspsrc_open (GstWFDRTSPSrc * src);
static GstRTSPResult gst_wfdrtspsrc_play (GstWFDRTSPSrc * src, GstSegment * segment);
static GstRTSPResult gst_wfdrtspsrc_pause (GstWFDRTSPSrc * src, gboolean idle);
static GstRTSPResult gst_wfdrtspsrc_close (GstWFDRTSPSrc * src, gboolean only_close);

static gboolean gst_wfdrtspsrc_uri_set_uri (GstURIHandler * handler,
    const gchar * uri);

static gboolean gst_wfdrtspsrc_activate (GstWFDRTSPSrc * src);
static gboolean gst_wfdrtspsrc_loop (GstWFDRTSPSrc * src);
static gboolean gst_wfdrtspsrc_stream_push_event (GstWFDRTSPSrc * src,
    WFDRTSPStream * stream, GstEvent * event, gboolean source);
static gboolean gst_wfdrtspsrc_push_event (GstWFDRTSPSrc * src, GstEvent * event,
    gboolean source);

static WFDRTSPStream * gst_wfdrtspsrc_create_stream (GstWFDRTSPSrc * src);
static gboolean gst_wfdrtspsrc_setup (GstWFDRTSPSrc * src);

static GstRTSPResult
gst_wfdrtspsrc_send (GstWFDRTSPSrc * src, GstRTSPConnection * conn,
    GstRTSPMessage * request, GstRTSPMessage * response,
    GstRTSPStatusCode * code);

static gboolean gst_wfdrtspsrc_parse_methods (GstWFDRTSPSrc * src, GstRTSPMessage * response);

static GstFlowReturn gst_wfdrtspsrc_send_sink_status_report (GstWFDRTSPSrc * src);

static gchar* gst_wfdrtspsrc_parse_parametr(gchar* data, const gchar* delim);

static GstRTSPResult gst_wfdrtspsrc_send_keep_alive (GstWFDRTSPSrc * src);

static void gst_wfdrtspsrc_connection_flush (GstWFDRTSPSrc * src, gboolean flush);

static GstRTSPResult gst_wfdrtspsrc_get_video_parametr(GstWFDRTSPSrc * src, WFDMessage *msg);

static void uibc_new_tcp (GstWFDRTSPSrc * src);

static gboolean
gst_wfdrtspsrc_handle_src_event (GstPad * pad, GstEvent * event);
static gboolean
gst_wfdrtspsrc_handle_src_query(GstPad * pad, GstQuery * query);


/* commands we send to out loop to notify it of events */
#define WFD_CMD_OPEN	0
#define WFD_CMD_PLAY	1
#define WFD_CMD_PAUSE	2
#define WFD_CMD_CLOSE	3
#define WFD_CMD_WAIT	4
#define WFD_CMD_LOOP	5

#define GST_ELEMENT_PROGRESS(el, type, code, text)      \
G_STMT_START {                                          \
  gchar *__txt = _gst_element_error_printf text;        \
  gst_element_post_message (GST_ELEMENT_CAST (el),      \
      gst_message_new_progress (GST_OBJECT_CAST (el),   \
          GST_PROGRESS_TYPE_ ##type, code, __txt));     \
  g_free (__txt);                                       \
} G_STMT_END


/*static guint gst_wfdrtspsrc_signals[LAST_SIGNAL] = { 0 }; */

static void
_do_init (GType wfdrtspsrc_type)
{
  static const GInterfaceInfo urihandler_info = {
    gst_wfdrtspsrc_uri_handler_init,
    NULL,
    NULL
  };

  GST_DEBUG_CATEGORY_INIT (wfdrtspsrc_debug, "wfdrtspsrc", 0, "WFD RTSP src");

  g_type_add_interface_static (wfdrtspsrc_type, GST_TYPE_URI_HANDLER,
      &urihandler_info);
}

GST_BOILERPLATE_FULL (GstWFDRTSPSrc, gst_wfdrtspsrc, GstBin, GST_TYPE_BIN, _do_init);

static void
gst_wfdrtspsrc_base_init (gpointer g_class)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (g_class);

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&rtptemplate));

  gst_element_class_set_details_simple (element_class, "Wifi direct src element",
      "Source/Network",
      "Receive data over the Wifi direct network via RTSP",
      "Santhoshi KS <santhoshi.ks@samsung.com>");
}

static void
gst_wfdrtspsrc_class_init (GstWFDRTSPSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBinClass *gstbin_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbin_class = (GstBinClass *) klass;

  gobject_class->set_property = gst_wfdrtspsrc_set_property;
  gobject_class->get_property = gst_wfdrtspsrc_get_property;

  gobject_class->finalize = gst_wfdrtspsrc_finalize;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "RTSP Location",
          "Location of the RTSP url to read",
          DEFAULT_LOCATION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_PROTOCOLS,
      g_param_spec_flags ("protocols", "Protocols",
          "Allowed lower transport protocols", GST_TYPE_RTSP_LOWER_TRANS,
          DEFAULT_PROTOCOLS, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEBUG,
      g_param_spec_boolean ("debug", "Debug",
          "Dump request and response messages to stdout",
          DEFAULT_DEBUG, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_RETRY,
      g_param_spec_uint ("retry", "Retry",
          "Max number of retries when allocating RTP ports.",
          0, G_MAXUINT16, DEFAULT_RETRY,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TIMEOUT,
      g_param_spec_uint64 ("timeout", "Timeout",
          "Retry TCP transport after UDP timeout microseconds (0 = disabled)",
          0, G_MAXUINT64, DEFAULT_TIMEOUT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_TCP_TIMEOUT,
      g_param_spec_uint64 ("tcp-timeout", "TCP Timeout",
          "Fail after timeout microseconds on TCP connections (0 = disabled)",
          0, G_MAXUINT64, DEFAULT_TCP_TIMEOUT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_LATENCY,
      g_param_spec_uint ("latency", "Buffer latency in ms",
          "Amount of ms to buffer", 0, G_MAXUINT, DEFAULT_LATENCY_MS,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_CONNECTION_SPEED,
      g_param_spec_uint ("connection-speed", "Connection Speed",
          "Network connection speed in kbps (0 = unknown)",
          0, G_MAXINT / 1000, DEFAULT_CONNECTION_SPEED,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NAT_METHOD,
      g_param_spec_enum ("nat-method", "NAT Method",
          "Method to use for traversing firewalls and NAT",
          GST_TYPE_WFD_RTSP_NAT_METHOD, DEFAULT_NAT_METHOD,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstWFDRTSPSrc::do-rtcp
   *
   * Enable RTCP support. Some old server don't like RTCP and then this property
   * needs to be set to FALSE.
   *
   * Since: 0.10.15
   */
  g_object_class_install_property (gobject_class, PROP_DO_RTCP,
      g_param_spec_boolean ("do-rtcp", "Do RTCP",
          "Send RTCP packets, disable for old incompatible server.",
          DEFAULT_DO_RTCP, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstWFDRTSPSrc::proxy
   *
   * Set the proxy parameters. This has to be a string of the format
   * [http://][user:passwd@]host[:port].
   *
   * Since: 0.10.15
   */
  g_object_class_install_property (gobject_class, PROP_PROXY,
      g_param_spec_string ("proxy", "Proxy",
          "Proxy settings for HTTP tunneling. Format: [http://][user:passwd@]host[:port]",
          DEFAULT_PROXY, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstWFDRTSPSrc::rtp_blocksize
   *
   * RTP package size to suggest to server.
   *
   * Since: 0.10.16
   */
  g_object_class_install_property (gobject_class, PROP_RTP_BLOCKSIZE,
      g_param_spec_uint ("rtp-blocksize", "RTP Blocksize",
          "RTP package size to suggest to server (0 = disabled)",
          0, 65536, DEFAULT_RTP_BLOCKSIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_USER_ID,
      g_param_spec_string ("user-id", "user-id",
          "RTSP location URI user id for authentication", DEFAULT_USER_ID,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_USER_PW,
      g_param_spec_string ("user-pw", "user-pw",
          "RTSP location URI user password for authentication", DEFAULT_USER_PW,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstWFDRTSPSrc::buffer-mode:
   *
   * Control the buffering and timestamping mode used by the jitterbuffer.
   *
   * Since: 0.10.22
   */
  g_object_class_install_property (gobject_class, PROP_BUFFER_MODE,
      g_param_spec_enum ("buffer-mode", "Buffer Mode",
          "Control the buffering algorithm in use",
          GST_TYPE_WFD_RTSP_SRC_BUFFER_MODE, DEFAULT_BUFFER_MODE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstWFDRTSPSrc::port-range:
   *
   * Configure the client port numbers that can be used to recieve RTP and
   * RTCP.
   *
   * Since: 0.10.25
   */
  g_object_class_install_property (gobject_class, PROP_PORT_RANGE,
      g_param_spec_string ("port-range", "Port range",
          "Client port range that can be used to receive RTP and RTCP data, "
          "eg. 3000-3005 (NULL = no restrictions)", DEFAULT_PORT_RANGE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstWFDRTSPSrc::udp-buffer-size:
   *
   * Size of the kernel UDP receive buffer in bytes.
   *
   * Since: 0.10.26
   */
  g_object_class_install_property (gobject_class, PROP_UDP_BUFFER_SIZE,
      g_param_spec_int ("udp-buffer-size", "UDP Buffer Size",
          "Size of the kernel UDP receive buffer in bytes, 0=default",
          0, G_MAXINT, DEFAULT_UDP_BUFFER_SIZE,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /* property for user-agent */
   g_object_class_install_property (gobject_class, PROP_USER_AGENT,
      g_param_spec_string ("user-agent", "User Agent",
          "User agent specified string", DEFAULT_USER_AGENT,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

#ifdef ENABLE_WFD_EXTENDED_FEATURES
  g_object_class_install_property (gobject_class, PROP_DEVICE_NAME,
      g_param_spec_string ("device-name", "Device Name",
          "Device Name of the WFD sink",
          DEFAULT_LOCATION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  /**
   * GstWFDRTSPSrc::buffering-animation
   *
   * Enable buffering animation.
   *
   */
  g_object_class_install_property (gobject_class, PROP_BUF_ANIMATION,
      g_param_spec_boolean ("buffering-animation", "Buffering Animation",
          "State of buffering animation",
          DEFAULT_BUF_ANIMATION, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  /**
   * GstWFDRTSPSrc::do-request
   *
   * Enable RTP retransmission.
   *
   */
  g_object_class_install_property (gobject_class, PROP_DO_REQUEST,
      g_param_spec_boolean ("do-request", "Enable RTP Retransmission Request",
          "Send RTCP FB packets and handel retransmitted RTP packets.",
          DEFAULT_DO_REQUEST, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DEMUX_HANDLE,
        g_param_spec_pointer ("demux-handle", "Demux Handle",
            "To get demux pointer from application", G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_AUDIO_QUEUE_HANDLE,
          g_param_spec_pointer ("audio-queue-handle", "Audio Queue Handle",
              "To get audio queue pointer from application", G_PARAM_READWRITE));
#endif
//property for getting audio parameter from sink ini
  g_object_class_install_property (gobject_class, PROP_AUDIO_PARAM,
      g_param_spec_boxed ("audio-param", "audio parameters",
          "A GstStructure specifies the mapping for audio parameters",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE));

//property for getting video parameter from sink ini
  g_object_class_install_property (gobject_class, PROP_VIDEO_PARAM,
      g_param_spec_boxed ("video-param", "video parameters",
          "A GstStructure specifies the mapping for video parameters",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE));

//property for getting HDCP parameter from sink ini
  g_object_class_install_property (gobject_class, PROP_HDCP_PARAM,
      g_param_spec_boxed ("hdcp-param", "HDCP parameters",
          "A GstStructure specifies the mapping for HDCP parameters",
          GST_TYPE_STRUCTURE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_USER_INPUT_EVENT,
      g_param_spec_pointer ("user-input-event", "User Input Event",
          "UIBC related User Input Events", G_PARAM_WRITABLE));

  g_object_class_install_property (gobject_class, PROP_UIBC_STATE_CB,
        g_param_spec_pointer ("uibc-state-cb", "UIBC State Callback",
            "Notifying UIBC state to application", G_PARAM_WRITABLE));

  gstelement_class->send_event = gst_wfdrtspsrc_send_event;
  gstelement_class->change_state = gst_wfdrtspsrc_change_state;

  gstbin_class->handle_message = gst_wfdrtspsrc_handle_message;

  gst_wfd_rtsp_ext_list_init ();
}


static void
gst_wfdrtspsrc_init (GstWFDRTSPSrc * src, GstWFDRTSPSrcClass * g_class)
{
#ifdef G_OS_WIN32
  WSADATA wsa_data;

  if (WSAStartup (MAKEWORD (2, 2), &wsa_data) != 0) {
    GST_ERROR_OBJECT (src, "WSAStartup failed: 0x%08x", WSAGetLastError ());
  }
#endif

  src->conninfo.location = g_strdup (DEFAULT_LOCATION);
  src->conninfo.url_str = NULL;
  src->protocols = DEFAULT_PROTOCOLS;
  src->debug = DEFAULT_DEBUG;
  src->retry = DEFAULT_RETRY;
  src->udp_timeout = DEFAULT_TIMEOUT;
  gst_wfdrtspsrc_set_tcp_timeout (src, DEFAULT_TCP_TIMEOUT);
  src->latency = DEFAULT_LATENCY_MS;
  src->connection_speed = DEFAULT_CONNECTION_SPEED;
  src->nat_method = DEFAULT_NAT_METHOD;
  src->do_rtcp = DEFAULT_DO_RTCP;
  gst_wfdrtspsrc_set_proxy (src, DEFAULT_PROXY);
  src->rtp_blocksize = DEFAULT_RTP_BLOCKSIZE;
  src->user_id = g_strdup (DEFAULT_USER_ID);
  src->user_pw = g_strdup (DEFAULT_USER_PW);
  src->user_agent = g_strdup (DEFAULT_USER_AGENT);
  src->buffer_mode = DEFAULT_BUFFER_MODE;
  src->client_port_range.min = 0;
  src->client_port_range.max = 0;
  src->udp_buffer_size = DEFAULT_UDP_BUFFER_SIZE;
  src->ssr_timeout.tv_sec = 0;
  src->ssr_timeout.tv_usec = 0;
  src->is_paused = TRUE;
  src->uibc = (UIBC *)malloc(sizeof(UIBC));
  memset (src->uibc, 0x00, sizeof(UIBC));
  src->audio_param = NULL;
  src->video_param = NULL;
  src->hdcp_param = NULL;

  /* create stream */
  src->stream = gst_wfdrtspsrc_create_stream (src);

  /* get a list of all extensions */
  src->extensions = gst_wfd_rtsp_ext_list_get ();

  /* connect to send signal */
  gst_wfd_rtsp_ext_list_connect (src->extensions, "send",
      (GCallback) gst_wfdrtspsrc_send_cb, src);

  /* protects the streaming thread in interleaved mode or the polling
   * thread in UDP mode. */
  src->stream_rec_lock = g_new (GStaticRecMutex, 1);
  g_static_rec_mutex_init (src->stream_rec_lock);

  /* protects our state changes from multiple invocations */
  src->state_rec_lock = g_new (GStaticRecMutex, 1);
  g_static_rec_mutex_init (src->state_rec_lock);

  /* protects access to the server connection */
  src->conn_rec_lock = g_new (GStaticRecMutex, 1);
  g_static_rec_mutex_init (src->conn_rec_lock);
  src->conn_rec_write_lock = g_new (GStaticRecMutex, 1);
  g_static_rec_mutex_init (src->conn_rec_write_lock);

  src->state = GST_RTSP_STATE_INVALID;

  /* set jitterbuffer src pad  as ghost pad we will use to stream to */
  GstPadTemplate *template;
  GstPad *outpad;

  outpad = gst_element_get_static_pad (src->stream->jitterbuffer, "src");

  template = gst_static_pad_template_get (&rtptemplate);
  src->stream->srcpad = gst_ghost_pad_new_from_template ("src", outpad, template);
  gst_object_unref (template);


  src->stream->added = TRUE;
  gst_pad_set_event_function (src->stream->srcpad,
      GST_DEBUG_FUNCPTR (gst_wfdrtspsrc_handle_src_event));
  gst_pad_set_query_function (src->stream->srcpad,
      GST_DEBUG_FUNCPTR (gst_wfdrtspsrc_handle_src_query));

  gst_element_add_pad (GST_ELEMENT_CAST (src), src->stream->srcpad);

  GST_OBJECT_FLAG_SET (src, GST_ELEMENT_IS_SOURCE);
}

static void
gst_wfdrtspsrc_finalize (GObject * object)
{
  GstWFDRTSPSrc *wfdrtspsrc;

  wfdrtspsrc = GST_WFDRTSPSRC (object);

  gst_wfd_rtsp_ext_list_free (wfdrtspsrc->extensions);
  gst_rtsp_url_free (wfdrtspsrc->conninfo.url);
  if (wfdrtspsrc->conninfo.location)
    g_free (wfdrtspsrc->conninfo.location);
  wfdrtspsrc->conninfo.location = NULL;
  if (wfdrtspsrc->conninfo.url_str)
    g_free (wfdrtspsrc->conninfo.url_str);
  wfdrtspsrc->conninfo.url_str = NULL;
  if (wfdrtspsrc->user_id)
    g_free (wfdrtspsrc->user_id);
  wfdrtspsrc->user_id = NULL;
  if (wfdrtspsrc->user_pw)
    g_free (wfdrtspsrc->user_pw);
  wfdrtspsrc->user_pw = NULL;
  if (wfdrtspsrc->user_agent)
    g_free (wfdrtspsrc->user_agent);
  wfdrtspsrc->user_agent = NULL;

  if(wfdrtspsrc->audio_param) {
    gst_structure_free(wfdrtspsrc->audio_param);
    wfdrtspsrc->audio_param = NULL;
  }
  if(wfdrtspsrc->video_param) {
    gst_structure_free(wfdrtspsrc->video_param);
    wfdrtspsrc->video_param = NULL;
  }
  if(wfdrtspsrc->hdcp_param) {
    gst_structure_free(wfdrtspsrc->hdcp_param);
    wfdrtspsrc->hdcp_param = NULL;
  }

  /* free locks */
  g_static_rec_mutex_free (wfdrtspsrc->stream_rec_lock);
  g_free (wfdrtspsrc->stream_rec_lock);
  g_static_rec_mutex_free (wfdrtspsrc->state_rec_lock);
  g_free (wfdrtspsrc->state_rec_lock);
  g_static_rec_mutex_free (wfdrtspsrc->conn_rec_lock);
  g_free (wfdrtspsrc->conn_rec_lock);
  g_static_rec_mutex_free (wfdrtspsrc->conn_rec_write_lock);
  g_free (wfdrtspsrc->conn_rec_write_lock);

#ifdef G_OS_WIN32
  WSACleanup ();
#endif

  G_OBJECT_CLASS (parent_class)->finalize (object);
}


/* a proxy string of the format [user:passwd@]host[:port] */
static gboolean
gst_wfdrtspsrc_set_proxy (GstWFDRTSPSrc * rtsp, const gchar * proxy)
{
  gchar *p, *at, *col;

  g_free (rtsp->proxy_user);
  rtsp->proxy_user = NULL;
  g_free (rtsp->proxy_passwd);
  rtsp->proxy_passwd = NULL;
  g_free (rtsp->proxy_host);
  rtsp->proxy_host = NULL;
  rtsp->proxy_port = 0;

  p = (gchar *) proxy;

  if (p == NULL)
    return TRUE;

  /* we allow http:// in front but ignore it */
  if (g_str_has_prefix (p, "http://"))
    p += 7;

  at = strchr (p, '@');
  if (at) {
    /* look for user:passwd */
    col = strchr (proxy, ':');
    if (col == NULL || col > at)
      return FALSE;

    rtsp->proxy_user = g_strndup (p, col - p);
    col++;
    rtsp->proxy_passwd = g_strndup (col, at - col);

    /* move to host */
    p = at + 1;
  }
  col = strchr (p, ':');

  if (col) {
    /* everything before the colon is the hostname */
    rtsp->proxy_host = g_strndup (p, col - p);
    p = col + 1;
    rtsp->proxy_port = strtoul (p, (char **) &p, 10);
  } else {
    rtsp->proxy_host = g_strdup (p);
    rtsp->proxy_port = 8080;
  }
  return TRUE;
}

static void
gst_wfdrtspsrc_set_tcp_timeout (GstWFDRTSPSrc * wfdrtspsrc, guint64 timeout)
{
  wfdrtspsrc->tcp_timeout.tv_sec = timeout / G_USEC_PER_SEC;
  wfdrtspsrc->tcp_timeout.tv_usec = timeout % G_USEC_PER_SEC;

  if (timeout != 0)
    wfdrtspsrc->ptcp_timeout = &wfdrtspsrc->tcp_timeout;
  else
    wfdrtspsrc->ptcp_timeout = NULL;
}

static void
gst_wfdrtspsrc_set_property (GObject * object, guint prop_id, const GValue * value,
    GParamSpec * pspec)
{
  GstWFDRTSPSrc *wfdrtspsrc;

  wfdrtspsrc = GST_WFDRTSPSRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      gst_wfdrtspsrc_uri_set_uri (GST_URI_HANDLER (wfdrtspsrc),
          g_value_get_string (value));
      break;
    case PROP_PROTOCOLS:
      wfdrtspsrc->protocols = g_value_get_flags (value);
      break;
    case PROP_DEBUG:
      wfdrtspsrc->debug = g_value_get_boolean (value);
      break;
    case PROP_RETRY:
      wfdrtspsrc->retry = g_value_get_uint (value);
      break;
    case PROP_TIMEOUT:
      wfdrtspsrc->udp_timeout = g_value_get_uint64 (value);
      break;
    case PROP_TCP_TIMEOUT:
      gst_wfdrtspsrc_set_tcp_timeout (wfdrtspsrc, g_value_get_uint64 (value));
      break;
    case PROP_LATENCY:
      wfdrtspsrc->latency = g_value_get_uint (value);
      break;
    case PROP_CONNECTION_SPEED:
      wfdrtspsrc->connection_speed = g_value_get_uint (value);
      break;
    case PROP_NAT_METHOD:
      wfdrtspsrc->nat_method = g_value_get_enum (value);
      break;
    case PROP_DO_RTCP:
      wfdrtspsrc->do_rtcp = g_value_get_boolean (value);
      break;
    case PROP_PROXY:
      gst_wfdrtspsrc_set_proxy (wfdrtspsrc, g_value_get_string (value));
      break;
    case PROP_RTP_BLOCKSIZE:
      wfdrtspsrc->rtp_blocksize = g_value_get_uint (value);
      break;
    case PROP_USER_ID:
      if (wfdrtspsrc->user_id)
        g_free (wfdrtspsrc->user_id);
      wfdrtspsrc->user_id = g_value_dup_string (value);
      break;
    case PROP_USER_PW:
      if (wfdrtspsrc->user_pw)
        g_free (wfdrtspsrc->user_pw);
      wfdrtspsrc->user_pw = g_value_dup_string (value);
      break;
    case PROP_BUFFER_MODE:
      wfdrtspsrc->buffer_mode = g_value_get_enum (value);
      break;
    case PROP_PORT_RANGE:
    {
      const gchar *str;

      str = g_value_get_string (value);
      if (str) {
        sscanf (str, "%u-%u",
            &wfdrtspsrc->client_port_range.min, &wfdrtspsrc->client_port_range.max);
      } else {
        wfdrtspsrc->client_port_range.min = 0;
        wfdrtspsrc->client_port_range.max = 0;
      }
      break;
    }
    case PROP_UDP_BUFFER_SIZE:
      wfdrtspsrc->udp_buffer_size = g_value_get_int (value);
      break;
    case PROP_USER_AGENT:
      if (wfdrtspsrc->user_agent)
        g_free(wfdrtspsrc->user_agent);
      wfdrtspsrc->user_agent = g_value_dup_string (value);
      break;
#ifdef ENABLE_WFD_EXTENDED_FEATURES
    case PROP_DEVICE_NAME:
      if (wfdrtspsrc->stream) {
        if (wfdrtspsrc->stream->device_name)
          g_free (wfdrtspsrc->stream->device_name);
        wfdrtspsrc->stream->device_name = g_value_dup_string (value);
      }
      break;
    case PROP_BUF_ANIMATION:
      if (wfdrtspsrc->stream)
        wfdrtspsrc->stream->buffering_animation = g_value_get_boolean (value);
      break;
    case PROP_DO_REQUEST:
      if (wfdrtspsrc->stream)
        wfdrtspsrc->stream->do_request = g_value_get_boolean (value);
      break;
    case PROP_DEMUX_HANDLE:
    {
      void *param = NULL;
      param = g_value_get_pointer(value);
      wfdrtspsrc->stream->demux_handle = (GstElement*)param;
      break;
    }
    case PROP_AUDIO_QUEUE_HANDLE:
    {
      void *param = NULL;
      param = g_value_get_pointer(value);
      wfdrtspsrc->stream->audio_queue_handle = (GstElement*)param;
      break;
    }
#endif
    case PROP_AUDIO_PARAM:
    {
      const GstStructure *s = gst_value_get_structure (value);
      if (wfdrtspsrc->audio_param) {
        gst_structure_free (wfdrtspsrc->audio_param);
      }
      if (s)
        wfdrtspsrc->audio_param = gst_structure_copy (s);
      else
        wfdrtspsrc->audio_param = NULL;
      break;
    }
    case PROP_VIDEO_PARAM:
    {
      const GstStructure *s = gst_value_get_structure (value);
      if (wfdrtspsrc->video_param) {
        gst_structure_free (wfdrtspsrc->video_param);
      }
      if (s)
        wfdrtspsrc->video_param = gst_structure_copy (s);
      else
        wfdrtspsrc->video_param = NULL;
      break;
    }
    case PROP_HDCP_PARAM:
    {
      const GstStructure *s = gst_value_get_structure (value);
      if (wfdrtspsrc->hdcp_param) {
        gst_structure_free (wfdrtspsrc->hdcp_param);
      }
      if (s)
        wfdrtspsrc->hdcp_param = gst_structure_copy (s);
      else
        wfdrtspsrc->hdcp_param = NULL;
      break;
    }
    case PROP_USER_INPUT_EVENT:
    {
      void *user_event = NULL;
      char *wbuf = NULL;
      user_event = g_value_get_pointer(value);
      if (user_event) {
        WFDUIBCMessage * msg = (WFDUIBCMessage *)user_event;
        wfd_uibc_message_dump (msg);
        wbuf = wfd_uibc_message_as_text(msg);
        if(wbuf) {
          if(wfdrtspsrc->uibc->sockfd > 0)
            write(wfdrtspsrc->uibc->sockfd, wbuf, msg->length);
          free(wbuf);
        }
      }
      break;
    }
    case PROP_UIBC_STATE_CB:
    {
      void ** params = NULL;
      params = (void **)g_value_get_pointer(value);
      wfdrtspsrc->uibc->wfd_sink_handle = (void *)params[1];
      wfdrtspsrc->uibc->state_cb = (wfd_uibc_state_cb)params[0];
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_wfdrtspsrc_get_property (GObject * object, guint prop_id, GValue * value,
    GParamSpec * pspec)
{
  GstWFDRTSPSrc *wfdrtspsrc;

  wfdrtspsrc = GST_WFDRTSPSRC (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_set_string (value, wfdrtspsrc->conninfo.location);
      break;
    case PROP_PROTOCOLS:
      g_value_set_flags (value, wfdrtspsrc->protocols);
      break;
    case PROP_DEBUG:
      g_value_set_boolean (value, wfdrtspsrc->debug);
      break;
    case PROP_RETRY:
      g_value_set_uint (value, wfdrtspsrc->retry);
      break;
    case PROP_TIMEOUT:
      g_value_set_uint64 (value, wfdrtspsrc->udp_timeout);
      break;
    case PROP_TCP_TIMEOUT:
    {
      guint64 timeout;

      timeout = wfdrtspsrc->tcp_timeout.tv_sec * G_USEC_PER_SEC +
          wfdrtspsrc->tcp_timeout.tv_usec;
      g_value_set_uint64 (value, timeout);
      break;
    }
    case PROP_LATENCY:
      g_value_set_uint (value, wfdrtspsrc->latency);
      break;
    case PROP_CONNECTION_SPEED:
      g_value_set_uint (value, wfdrtspsrc->connection_speed);
      break;
    case PROP_NAT_METHOD:
      g_value_set_enum (value, wfdrtspsrc->nat_method);
      break;
    case PROP_DO_RTCP:
      g_value_set_boolean (value, wfdrtspsrc->do_rtcp);
      break;
    case PROP_PROXY:
    {
      gchar *str;

      if (wfdrtspsrc->proxy_host) {
        str =
            g_strdup_printf ("%s:%d", wfdrtspsrc->proxy_host, wfdrtspsrc->proxy_port);
      } else {
        str = NULL;
      }
      g_value_take_string (value, str);
      break;
    }
    case PROP_RTP_BLOCKSIZE:
      g_value_set_uint (value, wfdrtspsrc->rtp_blocksize);
      break;
    case PROP_USER_ID:
      g_value_set_string (value, wfdrtspsrc->user_id);
      break;
    case PROP_USER_PW:
      g_value_set_string (value, wfdrtspsrc->user_pw);
      break;
    case PROP_BUFFER_MODE:
      g_value_set_enum (value, wfdrtspsrc->buffer_mode);
      break;
    case PROP_PORT_RANGE:
    {
      gchar *str;

      if (wfdrtspsrc->client_port_range.min != 0) {
        str = g_strdup_printf ("%u-%u", wfdrtspsrc->client_port_range.min,
            wfdrtspsrc->client_port_range.max);
      } else {
        str = NULL;
      }
      g_value_take_string (value, str);
      break;
    }
    case PROP_UDP_BUFFER_SIZE:
      g_value_set_int (value, wfdrtspsrc->udp_buffer_size);
      break;
    case PROP_USER_AGENT:
      g_value_set_string (value, wfdrtspsrc->user_agent);
      break;
#ifdef ENABLE_WFD_EXTENDED_FEATURES
    case PROP_DEVICE_NAME:
      if (wfdrtspsrc->stream)
        g_value_set_string (value, wfdrtspsrc->stream->device_name);
      break;
    case PROP_BUF_ANIMATION:
      if (wfdrtspsrc->stream)
        g_value_set_boolean (value, wfdrtspsrc->stream->buffering_animation);
      break;
    case PROP_DO_REQUEST:
      if (wfdrtspsrc->stream)
        g_value_set_boolean (value, wfdrtspsrc->stream->do_request);
      break;
    case PROP_DEMUX_HANDLE:
      if (wfdrtspsrc->stream)
        g_value_set_pointer (value, wfdrtspsrc->stream->demux_handle);
      break;
    case PROP_AUDIO_QUEUE_HANDLE:
      if (wfdrtspsrc->stream)
        g_value_set_pointer (value, wfdrtspsrc->stream->audio_queue_handle);
      break;
#endif
    case PROP_AUDIO_PARAM:
      gst_value_set_structure (value, wfdrtspsrc->audio_param);
      break;
    case PROP_VIDEO_PARAM:
      gst_value_set_structure (value, wfdrtspsrc->video_param);
      break;
    case PROP_HDCP_PARAM:
      gst_value_set_structure (value, wfdrtspsrc->hdcp_param);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* free the UDP sources allocated when negotiating a transport.
 * This function is called when the server negotiated to a transport where the
 * UDP sources are not needed anymore, such as TCP or multicast. */
static void
gst_wfdrtspsrc_stream_free_udp (WFDRTSPStream * stream)
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

static void
gst_wfdrtspsrc_stream_free (GstWFDRTSPSrc * src)
{
  WFDRTSPStream *stream;
  gint i;

  g_return_if_fail (src->stream);

  stream = src->stream;
  GST_DEBUG_OBJECT (src, "free stream %p", stream);

  if (stream->caps)
    gst_caps_unref (stream->caps);
  stream->caps = NULL;

  if (stream->destination)
    g_free (stream->destination);
  stream->destination = NULL;

  if (stream->conninfo.location)
    g_free (stream->conninfo.location);
  stream->conninfo.location = NULL;

#ifdef ENABLE_WFD_EXTENDED_FEATURES
  wfd_stream_ext_free_tcp (stream);
#endif

  for (i=0; i <WFD_PORT_NUM; i++) {
    if (stream->udpsrc[i]) {
      gst_element_set_state (stream->udpsrc[i], GST_STATE_NULL);
      gst_bin_remove (GST_BIN_CAST (src), stream->udpsrc[i]);
      gst_object_unref (stream->udpsrc[i]);
      stream->udpsrc[i] = NULL;
    }
    if (stream->channelpad[i]) {
      gst_object_unref (stream->channelpad[i]);
      stream->channelpad[i] = NULL;
    }
    if (stream->udpsink[i]) {
      gst_element_set_state (stream->udpsink[i], GST_STATE_NULL);
      gst_bin_remove (GST_BIN_CAST (src), stream->udpsink[i]);
      gst_object_unref (stream->udpsink[i]);
      stream->udpsink[i] = NULL;
    }
  }
  if (stream->requester) {
    gst_element_set_state (stream->requester, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (src), stream->requester);
    gst_object_unref (stream->requester);
    stream->requester = NULL;
  }
  if (stream->session) {
    gst_element_set_state (stream->session, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (src), stream->session);
    gst_object_unref (stream->session);
    stream->session = NULL;
  }
  if (stream->jitterbuffer) {
    gst_element_set_state (stream->jitterbuffer, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (src), stream->jitterbuffer);
    gst_object_unref (stream->jitterbuffer);
    stream->jitterbuffer = NULL;
  }
  if (stream->fakesrc) {
    gst_element_set_state (stream->fakesrc, GST_STATE_NULL);
    gst_bin_remove (GST_BIN_CAST (src), stream->fakesrc);
    gst_object_unref (stream->fakesrc);
    stream->fakesrc = NULL;
  }
  if (stream->srcpad) {
    gst_pad_set_active (stream->srcpad, FALSE);
    if (stream->added) {
      gst_element_remove_pad (GST_ELEMENT_CAST (src), stream->srcpad);
      stream->added = FALSE;
    }
    stream->srcpad = NULL;
  }
  if (stream->rtcppad) {
    gst_object_unref (stream->rtcppad);
    stream->rtcppad = NULL;
  }
  g_free (stream);
}

static void
gst_wfdrtspsrc_cleanup (GstWFDRTSPSrc * src)
{
  GST_DEBUG_OBJECT (src, "cleanup");

  if (src->uibc && (src->uibc->sockfd > 0)) {
    src->uibc->state_cb( 0, NULL, 0, src->video_width, src->video_height, src->uibc->wfd_sink_handle);
    shutdown (src->uibc->sockfd, SHUT_RDWR);
    src->uibc->sockfd = -1;
  }

  if(src->uibc)
    g_free (src->uibc);
  src->uibc = NULL;

  if (src->stream)
    gst_wfdrtspsrc_stream_free (src);
  src->stream = NULL;
}

static gboolean
gst_wfdrtspsrc_alloc_udp_ports (WFDRTSPStream * stream,
    gint * rtpport, gint * rtcpport)
{
  GstWFDRTSPSrc *src;
  GstStateChangeReturn ret;
  GstElement *udpsrc0 = NULL, *udpsrc1 = NULL;
  gint tmp_rtp = 0, tmp_rtcp = 0;
  const gchar *host;
  GstCaps *caps;
#ifdef ENABLE_WFD_EXTENDED_FEATURES
  GstElement *udpsrc2 = NULL;
  gint tmp_retransmitted_rtp = 0;
#endif

  src = GST_WFDRTSPSRC(stream->parent);

  /* Start at next port */
  tmp_rtp = src->next_port_num;

  if (stream->is_ipv6)
    host = "udp://[::0]";
  else
    host = "udp://0.0.0.0";

  /* try to allocate 2 UDP ports, the RTP port should be an even
   * number and the RTCP port should be the next (uneven) port */
  if (tmp_rtp != 0 && src->client_port_range.max > 0 &&
      tmp_rtp >= src->client_port_range.max)
    goto no_ports;

  udpsrc0 = gst_element_make_from_uri (GST_URI_SRC, host, NULL);
  if (udpsrc0 == NULL)
    goto no_udp_protocol;
  if(!src->primary_rtpport)
    g_object_set (G_OBJECT (udpsrc0), "port", tmp_rtp, "reuse", FALSE, NULL);
  else
    g_object_set (G_OBJECT (udpsrc0), "port", src->primary_rtpport, "reuse", FALSE, NULL);

  if (src->udp_buffer_size != 0)
    g_object_set (G_OBJECT (udpsrc0), "buffer-size", src->udp_buffer_size,
        NULL);

  caps = gst_caps_new_simple ("application/x-rtp",
                    "media", G_TYPE_STRING, "video", "payload", G_TYPE_INT, 33,
                    "clock-rate", G_TYPE_INT, 90000, "encoding-params", G_TYPE_STRING, "MP2T-ES", NULL);

  g_object_set (udpsrc0, "caps", caps, NULL);
  gst_caps_unref (caps);

  ret = gst_element_set_state (udpsrc0, GST_STATE_PAUSED);
  if (ret == GST_STATE_CHANGE_FAILURE)  {
    GST_DEBUG_OBJECT (src, "fail to change state udpsrc for RTP");
    goto no_udp_protocol;
  }

  g_object_get (G_OBJECT (udpsrc0), "port", &tmp_rtp, NULL);
  GST_DEBUG_OBJECT (src, "got RTP port %d", tmp_rtp);

  /* check if port is even */
  if ((tmp_rtp & 0x01) != 0) {
    GST_DEBUG_OBJECT (src, "RTP port not even");
    goto no_ports;
  }

  /* allocate port+1 for RTCP now */
  udpsrc1 = gst_element_make_from_uri (GST_URI_SRC, host, NULL);
  if (udpsrc1 == NULL)
    goto no_udp_rtcp_protocol;

  /* set port */
  tmp_rtcp = src->primary_rtpport + 1;
  if (src->client_port_range.max > 0 && tmp_rtcp >= src->client_port_range.max)
    goto no_ports;

  g_object_set (G_OBJECT (udpsrc1), "port", tmp_rtcp, "reuse", FALSE, NULL);

  GST_DEBUG_OBJECT (src, "starting RTCP on port %d", tmp_rtcp);
  ret = gst_element_set_state (udpsrc1, GST_STATE_PAUSED);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    GST_DEBUG_OBJECT (src, "fail to change state udpsrc for RTCP");
    goto no_udp_rtcp_protocol;
  }

  /* all fine, do port check */
  g_object_get (G_OBJECT (udpsrc0), "port", rtpport, NULL);
  g_object_get (G_OBJECT (udpsrc1), "port", rtcpport, NULL);

  /* this should not happen... */
  if (*rtpport != tmp_rtp || *rtcpport != tmp_rtcp)
    goto port_error;

  /* we keep these elements, we configure all in configure_transport when the
   * server told us to really use the UDP ports. */
  stream->udpsrc[0] = gst_object_ref (udpsrc0);
  stream->udpsrc[1] = gst_object_ref (udpsrc1);

  /* keep track of next available port number when we have a range
   * configured */
  if (src->next_port_num != 0)
    src->next_port_num = tmp_rtcp + 1;

  /* they are ours now */
  gst_object_sink (udpsrc0);
  gst_object_sink (udpsrc1);

#ifdef ENABLE_WFD_EXTENDED_FEATURES
  if (stream->do_request) {
    /* allocate port #19120 for retransmitted RTP now */
    udpsrc2 = gst_element_make_from_uri (GST_URI_SRC, host, NULL);
    if (udpsrc2 == NULL)
      goto no_udp_rtcp_protocol;

    /* set port */
    tmp_retransmitted_rtp = RETRANSMITTED_RTP_PORT;
    g_object_set (G_OBJECT (udpsrc2), "port", tmp_retransmitted_rtp, "reuse", FALSE, NULL);

    if (src->udp_buffer_size != 0)
      g_object_set (G_OBJECT (udpsrc2), "buffer-size", src->udp_buffer_size,
          NULL);

    caps = gst_caps_new_simple ("application/x-rtp",
                      "media", G_TYPE_STRING, "video", "payload", G_TYPE_INT, 33,
                      "clock-rate", G_TYPE_INT, 90000, "encoding-params", G_TYPE_STRING, "MP2T-ES", NULL);

    g_object_set (udpsrc2, "caps", caps, NULL);
    gst_caps_unref (caps);

    GST_DEBUG_OBJECT (src, "starting Requested RTP  on port %d", tmp_retransmitted_rtp);
    ret = gst_element_set_state (udpsrc2, GST_STATE_PAUSED);
    if (ret == GST_STATE_CHANGE_FAILURE) {
      GST_DEBUG_OBJECT (src, "Unable to make udpsrc from Retransmitted port %d", tmp_retransmitted_rtp);

      GST_DEBUG_OBJECT (src, "free Retransmitted udpsrc");
      gst_element_set_state (udpsrc2, GST_STATE_NULL);
      gst_object_unref (udpsrc2);
      udpsrc2 = NULL;

      GST_DEBUG_OBJECT (src, "turn off rtp retransmission");
      stream->do_request = FALSE;
    }
  }

  if (udpsrc2)
    stream->udpsrc[2] = gst_object_ref (udpsrc2);

  if (udpsrc2)
    gst_object_sink (udpsrc2);
#endif


  return TRUE;

  /* ERRORS */
no_udp_protocol:
  {
    GST_DEBUG_OBJECT (src, "could not get UDP source");
    goto cleanup;
  }
no_ports:
  {
    GST_DEBUG_OBJECT (src, "could not allocate UDP port pair");
    goto cleanup;
  }
no_udp_rtcp_protocol:
  {
    GST_DEBUG_OBJECT (src, "could not get UDP source for RTCP");
    goto cleanup;
  }
port_error:
  {
    GST_DEBUG_OBJECT (src, "ports don't match rtp: %d<->%d, rtcp: %d<->%d",
        tmp_rtp, *rtpport, tmp_rtcp, *rtcpport);
    goto cleanup;
  }
cleanup:
  {
    if (udpsrc0) {
      gst_element_set_state (udpsrc0, GST_STATE_NULL);
      gst_object_unref (udpsrc0);
    }
    if (udpsrc1) {
      gst_element_set_state (udpsrc1, GST_STATE_NULL);
      gst_object_unref (udpsrc1);
    }
#ifdef ENABLE_WFD_EXTENDED_FEATURES
    if (udpsrc2) {
      gst_element_set_state (udpsrc2, GST_STATE_NULL);
      gst_object_unref (udpsrc2);
    }
#endif
    return FALSE;
  }
}

static GstRTSPResult
gst_wfdrtspsrc_connection_send (GstWFDRTSPSrc * src, GstRTSPConnection * conn,
    GstRTSPMessage * message, GTimeVal * timeout)
{
  GstRTSPResult ret;

  if (conn)
    ret = gst_rtsp_connection_send (conn, message, timeout);
  else
    ret = GST_RTSP_ERROR;

  return ret;
}

static GstRTSPResult
gst_wfdrtspsrc_connection_receive (GstWFDRTSPSrc * src, GstRTSPConnection * conn,
    GstRTSPMessage * message, GTimeVal * timeout)
{
  GstRTSPResult ret;

  if (conn)
    ret = gst_rtsp_connection_receive (conn, message, timeout);
  else
    ret = GST_RTSP_ERROR;

  return ret;
}

static gboolean
gst_wfdrtspsrc_prepare_set_param (GstWFDRTSPSrc * src, GstWFDParam param)
{
  WFDRTSPStream *stream = NULL;
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPResult res = GST_RTSP_OK;
  WFDResult wfd_res = WFD_OK;
  WFDMessage *msg_no= NULL;
  gchar *msg;
  gchar *ua;
  guint msglen = 0;
  GString *msglength;

  res = gst_rtsp_message_init_request (&request, GST_RTSP_SET_PARAMETER,
    src->conninfo.url_str);
  if (res < 0)
    goto error;

  /* Create set_parameter body to be sent in the request */

  wfd_res = wfdconfig_message_new (&msg_no);
  if (wfd_res != WFD_OK) {
    GST_ERROR_OBJECT (src, "Failed to create wfd message...");
    res = GST_RTSP_ERROR;
    goto error;
  }

  wfd_res = wfdconfig_message_init(msg_no);
  if (wfd_res != WFD_OK) {
    GST_ERROR_OBJECT (src, "Failed to init wfd message...");
    res = GST_RTSP_ERROR;
    goto error;
  }

  switch(param) {

    case WFD_ROUTE: {
      wfd_res = wfdconfig_set_audio_sink_type(msg_no,WFD_SECONDARY_SINK);
      if (wfd_res != WFD_OK) {
        GST_ERROR_OBJECT (src, "Failed to set audio sink type...");
        res = GST_RTSP_ERROR;
        goto error;
      }
      break;
    }

    case WFD_CONNECTOR_TYPE: {
      wfd_res = wfdconfig_set_connector_type(msg_no,WFD_CONNECTOR_VGA);
      if (wfd_res != WFD_OK) {
        GST_ERROR_OBJECT (src, "Failed to set connector type...");
        res = GST_RTSP_ERROR;
        goto error;
      }
      break;
    }

    case WFD_STANDBY: {
      wfd_res = wfdconfig_set_standby(msg_no,TRUE);
      if (wfd_res != WFD_OK) {
        GST_ERROR_OBJECT (src, "Failed to set standby...");
        res = GST_RTSP_ERROR;
        goto error;
      }
      break;
    }

    case WFD_IDR_REQUEST: {
      wfd_res = wfdconfig_set_idr_request(msg_no);
      if (wfd_res != WFD_OK) {
        GST_ERROR_OBJECT (src, "Failed to set IDR request...");
        res = GST_RTSP_ERROR;
        goto error;
      }
      break;
    }

    case WFD_UIBC_CAPABILITY: {
      wfd_res = wfdconfig_set_uibc_capability(msg_no, WFD_UIBC_INPUT_CAT_UNKNOWN, 0, NULL, 0, 0);
      if (wfd_res != WFD_OK) {
        GST_ERROR_OBJECT (src, "Failed to set UIBC capability...");
        res = GST_RTSP_ERROR;
        goto error;
      }
      break;
    }

    case WFD_UIBC_SETTING: {
      wfd_res = wfdconfig_set_uibc_status(msg_no, TRUE);
      if (wfd_res != WFD_OK) {
        GST_ERROR_OBJECT (src, "Failed to set UIBC setting...");
        res = GST_RTSP_ERROR;
        goto error;
      }
      break;
    }

    default: {
      GST_ERROR_OBJECT (src, "Unhandled WFD message type...");
      goto not_supported;
    }
  }

  wfd_res = wfdconfig_message_dump(msg_no);
  if (wfd_res != WFD_OK) {
    GST_ERROR_OBJECT (src, "Failed to dump wfd message...");
    res = GST_RTSP_ERROR;
    goto error;
  }

  msg = wfdconfig_message_as_text(msg_no);
  if (msg == NULL) {
    GST_ERROR_OBJECT (src, "Failed to get wfd message as text...");
    res = GST_RTSP_ERROR;
    goto error;
  }

  msglen = strlen(msg);
  msglength = g_string_new ("");
  g_string_append_printf (msglength,"%d",msglen);

  ua = g_strdup (src->user_agent);
  res = gst_rtsp_message_add_header (&request, GST_RTSP_HDR_USER_AGENT, ua);
  if (ua)
    g_free(ua);

  GST_DEBUG ("WFD message body: %s", msg);
  /* add content-length type */
  res = gst_rtsp_message_add_header (&request, GST_RTSP_HDR_CONTENT_LENGTH, g_string_free (msglength, FALSE));
  if (res != GST_RTSP_OK) {
    GST_ERROR_OBJECT (src, "Failed to add header to rtsp request...");
    goto error;
  }

  /* adding wfdconfig data to request */
  res = gst_rtsp_message_set_body (&request,(guint8 *)msg, msglen);
  if (res != GST_RTSP_OK) {
    GST_ERROR_OBJECT (src, "Failed to set body to rtsp request...");
    goto error;
  }

  wfdconfig_message_free(msg_no);
  msg_no = NULL;

  /* send SET_PARAM */
  GST_DEBUG_OBJECT (src, "send SET_PARAM...");

  if (gst_wfdrtspsrc_send (src, src->conninfo.connection, &request, &response,
      NULL) < 0)
    goto error;

  if (src->debug)
    gst_rtsp_message_dump (&response);

  return TRUE;

/* ERRORS */
not_supported:
  {
    GST_DEBUG_OBJECT (src, "SET_PARAMETER is not supported.");
    if(msg_no) {
      wfdconfig_message_free(msg_no);
      msg_no = NULL;
    }
    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);
    return GST_RTSP_EINVAL;
  }
error:
 {
    if(msg_no)
      wfdconfig_message_free(msg_no);
    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);
    return FALSE;
 }
}

static gboolean
gst_wfdrtspsrc_handle_src_event (GstPad * pad, GstEvent * event)
{
  GstWFDRTSPSrc *src;
  gboolean res = TRUE;
  gboolean forward = FALSE;
  const GstStructure *s;
  /*static gdouble elapsed = 0.0;
  static int count = 0;*/

  src = GST_WFDRTSPSRC_CAST (gst_pad_get_parent (pad));

  //GST_DEBUG_OBJECT (src, "pad %s:%s received event %s",
  //    GST_DEBUG_PAD_NAME (pad), GST_EVENT_TYPE_NAME (event));

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_QOS:
    case GST_EVENT_NAVIGATION:
    case GST_EVENT_LATENCY:
      break;
    case GST_EVENT_CUSTOM_UPSTREAM:
      s = gst_event_get_structure (event);
      if (gst_structure_has_name (s, "WFD_FORCE_IDR")) {
        /* Send IDR request */
        res = gst_wfdrtspsrc_prepare_set_param(src, WFD_IDR_REQUEST);
      }
      break;
    default:
      forward = TRUE;
      break;
  }
  if (forward) {
    GstPad *target;

    if ((target = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad)))) {
      res = gst_pad_send_event (target, event);
      gst_object_unref (target);
    } else {
      gst_event_unref (event);
    }
  } else {
    gst_event_unref (event);
  }
  gst_object_unref (src);

  return res;
}

/* this query is executed on the ghost source pad exposed on wfdrtspsrc. */
static gboolean
gst_wfdrtspsrc_handle_src_query (GstPad * pad, GstQuery * query)
{
  GstWFDRTSPSrc *src;
  gboolean res = FALSE;

  src = GST_WFDRTSPSRC_CAST (gst_pad_get_parent (pad));

  //GST_DEBUG_OBJECT (src, "pad %s:%s received query %s",
  //    GST_DEBUG_PAD_NAME (pad), GST_QUERY_TYPE_NAME (query));

  switch (GST_QUERY_TYPE (query)) {
    case GST_QUERY_DURATION:
    {
      GstFormat format;

      gst_query_parse_duration (query, &format, NULL);

      switch (format) {
        case GST_FORMAT_TIME:
          gst_query_set_duration (query, format, src->segment.duration);
          res = TRUE;
          break;
        default:
          break;
      }
      break;
    }
    case GST_QUERY_URI:
    {
      if (src->conninfo.location == NULL) {
        res = FALSE;
      } else {
        gst_query_set_uri( query, src->conninfo.location);
        res = TRUE;
      }
      break;
    }
    default:
    {
      GstPad *target = gst_ghost_pad_get_target (GST_GHOST_PAD_CAST (pad));

      /* forward the query to the proxy target pad */
      if (target) {
        res = gst_pad_query (target, query);
        gst_object_unref (target);
      }
      break;
    }
  }
  gst_object_unref (src);

  return res;
}

static void
pad_unblocked (GstPad * pad, gboolean blocked, GstWFDRTSPSrc * src)
{
  GST_DEBUG_OBJECT (src, "pad %s:%s unblocked", GST_DEBUG_PAD_NAME (pad));
}

static void
pad_blocked (GstPad * pad, gboolean blocked, GstWFDRTSPSrc * src)
{
  GST_DEBUG_OBJECT (src, "pad %s:%s blocked, activating streams",
      GST_DEBUG_PAD_NAME (pad));

  /* activate the streams */
  GST_OBJECT_LOCK (src);
  if (!src->need_activate)
    goto was_ok;

  src->need_activate = FALSE;
  GST_OBJECT_UNLOCK (src);

  gst_wfdrtspsrc_activate (src);

  return;

was_ok:
  {
    GST_OBJECT_UNLOCK (src);
    return;
  }
}


static void
gst_wfdrtspsrc_do_stream_eos (GstWFDRTSPSrc * src, WFDRTSPStream * stream)
{
  if(src->is_paused) {
    GST_DEBUG_OBJECT (src, "Skipping EOS while on pause.");
    return;
  }

  GST_DEBUG_OBJECT (src, "setting stream for session %u to EOS", stream->id);

  if (stream->eos)
    goto was_eos;

  stream->eos = TRUE;
  gst_wfdrtspsrc_stream_push_event (src, stream, gst_event_new_eos (), TRUE);
  return;

  /* ERRORS */
was_eos:
  {
    GST_DEBUG_OBJECT (src, "stream for session %u was already EOS", stream->id);
    return;
  }
}

static void
on_bye_ssrc (GObject * session, GObject * source, WFDRTSPStream * stream)
{
  GstWFDRTSPSrc *src = GST_WFDRTSPSRC(stream->parent);

  GST_DEBUG_OBJECT (src, "source in session %u received BYE", stream->id);

  gst_wfdrtspsrc_do_stream_eos (src, stream);
}

static void
on_new_ssrc (GObject * session, GObject * source, WFDRTSPStream * stream)
{
  GstWFDRTSPSrc *src = GST_WFDRTSPSRC(stream->parent);

  GST_DEBUG_OBJECT (src, "source in session received NEW");
}

static void
on_timeout (GObject * session, GObject * source, WFDRTSPStream * stream)
{
  GstWFDRTSPSrc *src = GST_WFDRTSPSRC(stream->parent);

  GST_DEBUG_OBJECT (src, "source in session %u timed out", stream->id);

  //gst_wfdrtspsrc_do_stream_eos (src, stream);
}

static void
on_ssrc_active (GObject * session, GObject * source, WFDRTSPStream * stream)
{
  GST_DEBUG_OBJECT (stream->parent, "source in session %u is active",
      stream->id);
}

static GstCaps *
request_pt_map_for_session (GstElement * session, guint session_id, guint pt, GstWFDRTSPSrc * src)
{
  WFDRTSPStream *stream;
  GstCaps *caps;

  GST_DEBUG_OBJECT (src, "getting pt map for pt %d in session %d", pt, session_id);

  GST_WFD_RTSP_STATE_LOCK (src);
  stream = src->stream;
  if (!stream)
    goto unknown_stream;

  caps = stream->caps;
  if (caps)
    gst_caps_ref (caps);
  GST_WFD_RTSP_STATE_UNLOCK (src);

  return caps;

unknown_stream:
  {
    GST_DEBUG_OBJECT (src, "unknown stream %d", session_id);
    GST_WFD_RTSP_STATE_UNLOCK (src);
    return NULL;
  }
}


static GstCaps *
request_pt_map_for_jitterbuffer (GstElement * jitterbuffer, guint pt, GstWFDRTSPSrc * src)
{
  WFDRTSPStream *stream;
  GstCaps *caps;

  GST_DEBUG_OBJECT (src, "getting pt map for pt %d", pt);

  GST_WFD_RTSP_STATE_LOCK (src);
  stream = src->stream;
  if (!stream)
    goto unknown_stream;

  caps = stream->caps;
  if (caps)
    gst_caps_ref (caps);
  GST_WFD_RTSP_STATE_UNLOCK (src);

  return caps;

unknown_stream:
  {
    GST_DEBUG_OBJECT (src, "unknown stream %p", stream);
    GST_WFD_RTSP_STATE_UNLOCK (src);
    return NULL;
  }
}

static void
request_timeout_by_requester (GstElement * requester, GstWFDRTSPSrc *src)
{
  GST_DEBUG_OBJECT (src, "rtp retransmission is timed out, request IDR");

  /* Send IDR request */
  //gst_wfdrtspsrc_prepare_set_param (src, WFD_IDR_REQUEST);
}

static gboolean
gst_wfdrtspsrc_configure_manager (GstWFDRTSPSrc * src, WFDRTSPStream * stream)
{
  GstStateChangeReturn ret;
  GstState target;
  GstPad *sinkpad, *srcpad;
  GstPadLinkReturn lres;
  GstPadTemplate *template;
  gchar *name;
  GstPad *outpad;

  GST_DEBUG_OBJECT (src, "create session");
  if (stream->session == NULL) {
    if (!(stream->session = gst_element_factory_make ("gstrtpsession", NULL)))
	goto no_session;

    /* we manage this element */
    if (!gst_bin_add (GST_BIN_CAST (src), stream->session))
	goto no_session;

    g_signal_connect (stream->session, "on-bye-ssrc", (GCallback) on_bye_ssrc,
        stream);
    g_signal_connect (stream->session, "on-bye-timeout", (GCallback) on_timeout,
        stream);
    g_signal_connect (stream->session, "on-timeout", (GCallback) on_timeout,
        stream);
    g_signal_connect (stream->session, "on-ssrc-active",
        (GCallback) on_ssrc_active, stream);
    g_signal_connect (stream->session, "on-new-ssrc", (GCallback) on_new_ssrc,
        stream);
    g_signal_connect (stream->session, "request-pt-map",
        (GCallback) request_pt_map_for_session, src);
  }

  if (stream->requester == NULL) {
    GST_DEBUG_OBJECT (src, "create requester");
    if (!(stream->requester = gst_element_factory_make ("wfdrtprequester", NULL)))
      goto no_requester;

    /* we manage this element */
    gst_bin_add (GST_BIN_CAST (src), stream->requester);
  }

  GST_DEBUG_OBJECT (src, "create jitterbuffer");
  if (stream->jitterbuffer == NULL) {
    if (!(stream->jitterbuffer = gst_element_factory_make ("gstrtpjitterbuffer", NULL)))
	goto no_buffer;

    /* we manage this element */
    gst_bin_add (GST_BIN_CAST (src), stream->jitterbuffer);

    /* configure latency and packet lost */
    g_object_set (stream->jitterbuffer, "latency", src->latency, NULL);
    if (src->buffer_mode != BUFFER_MODE_AUTO)
      g_object_set (stream->jitterbuffer, "mode", src->buffer_mode, NULL);
      g_signal_connect (stream->jitterbuffer, "request-pt-map",
	 (GCallback) request_pt_map_for_jitterbuffer, src);
  }

  /* get channel pads */
  GST_DEBUG_OBJECT (src, "getting RTP sink pad of gstrtpsession");
  stream->channelpad[0] = gst_element_get_request_pad (stream->session, "recv_rtp_sink");

  GST_DEBUG_OBJECT (src, "getting RTCP sink pad of gstrtpsession");
  stream->channelpad[1] = gst_element_get_request_pad (stream->session, "recv_rtcp_sink");

  /* link session and requester*/
  srcpad = gst_element_get_static_pad (stream->session, "recv_rtp_src");
  sinkpad = gst_element_get_static_pad (stream->requester, "rtp_sink");
  lres = gst_pad_link (srcpad, sinkpad);
  if (lres != GST_PAD_LINK_OK)
    goto link_failed;
  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  /* link requester and jitterbuffer*/
  srcpad = gst_element_get_static_pad (stream->requester, "rtp_src");
  sinkpad = gst_element_get_static_pad (stream->jitterbuffer, "sink");
  lres = gst_pad_link (srcpad, sinkpad);
  if (lres != GST_PAD_LINK_OK)
    goto link_failed;
  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  /* set session, requester and jitterbuffer state */
  GST_OBJECT_LOCK (src);
  target = GST_STATE_TARGET (src);
  GST_OBJECT_UNLOCK (src);

  /* change state only to what's needed */
  ret = gst_element_set_state (stream->session, target);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto start_failure;
  ret = gst_element_set_state (stream->requester, target);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto start_failure;
  ret = gst_element_set_state (stream->jitterbuffer, target);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto start_failure;

  return TRUE;

  /* ERRORS */

no_session:
  {
    GST_DEBUG_OBJECT (src, "no session");
    return FALSE;
  }
no_requester:
  {
    GST_DEBUG_OBJECT (src, "no requester");
    return FALSE;
  }
no_buffer:
  {
    GST_DEBUG_OBJECT (src, "no buffer");
    return FALSE;
  }
link_failed:
  {
    if (sinkpad)
	gst_object_unref (sinkpad);
    if (srcpad)
      gst_object_unref (srcpad);

    GST_DEBUG_OBJECT (src, "link failure");
    return FALSE;
  }
start_failure:
  {
    GST_DEBUG_OBJECT (src, "start failure");
    return FALSE;
  }
}

static gboolean
gst_wfdrtspsrc_set_manager (GstWFDRTSPSrc * src, WFDRTSPStream * stream)
{
  g_return_val_if_fail (src, FALSE);
  g_return_val_if_fail (stream, FALSE);

#ifdef ENABLE_WFD_EXTENDED_FEATURES
  if (stream->requester != NULL) {
    g_signal_connect (stream->requester, "request-timeout",
        (GCallback) request_timeout_by_requester, src);

    g_object_set (stream->requester, "do-request", stream->do_request, NULL);
  }
#endif

  if (stream->jitterbuffer != NULL) {
    /* configure latency and packet lost */
    g_object_set (stream->jitterbuffer, "latency", src->latency, NULL);

    if (src->buffer_mode != BUFFER_MODE_AUTO)
      g_object_set (stream->jitterbuffer, "mode", src->buffer_mode, NULL);

    g_signal_connect (stream->jitterbuffer, "request-pt-map",
	 (GCallback) request_pt_map_for_jitterbuffer, src);
  }

#ifdef ENABLE_WFD_EXTENDED_FEATURES
  if (stream->do_request) {
    GST_DEBUG_OBJECT (src, "getting retransmitted RTP sink pad of gstrtprequester");
    stream->channelpad[2] = gst_element_get_request_pad (stream->requester, "retransmitted_rtp_sink");
    if (!stream->channelpad[2]) {
      GST_DEBUG_OBJECT (src, "fail to get retransmitted RTP sink pad of gstrtprequester, coutld not request retransmission.");
      stream->do_request = FALSE;
    }
  }
#endif

  return TRUE;
}


static void
gst_wfdrtspsrc_get_transport_info (GstWFDRTSPSrc * src, WFDRTSPStream * stream,
    GstRTSPTransport * transport, const gchar ** destination, gint * min, gint * max)
{
  g_return_if_fail (transport);
  g_return_if_fail (transport->lower_transport == GST_RTSP_LOWER_TRANS_UDP);

  if (destination) {
    /* first take the source, then the endpoint to figure out where to send
     * the RTCP. */
    if (!(*destination = transport->source)) {
      if (src->conninfo.connection)
        *destination = gst_rtsp_connection_get_ip (src->conninfo.connection);
      else if (stream->conninfo.connection)
        *destination =
            gst_rtsp_connection_get_ip (stream->conninfo.connection);
    }
  }
  if (min && max) {
    /* for unicast we only expect the ports here */
    *min = transport->server_port.min;
    *max = transport->server_port.max;
  }
}


/* configure the remainder of the UDP ports */
static gboolean
gst_wfdrtspsrc_configure_udp (GstWFDRTSPSrc * src, WFDRTSPStream * stream,
    GstRTSPTransport * transport, GstPad ** outpad)
{
  /* we manage the UDP elements now. For unicast, the UDP sources where
   * allocated in the stream when we suggested a transport. */
  if (stream->udpsrc[0]) {
    gst_bin_add (GST_BIN_CAST (src), stream->udpsrc[0]);

    GST_DEBUG_OBJECT (src, "setting up UDP source");

    /* configure a timeout on the UDP port. When the timeout message is
     * posted, we assume UDP transport is not possible. We reconnect using TCP
     * if we can. */
    g_object_set (G_OBJECT (stream->udpsrc[0]), "timeout", src->udp_timeout,
        NULL);

    GST_DEBUG_OBJECT (src, "got outpad from udpsrc");
    /* get output pad of the UDP source. */
    *outpad = gst_element_get_static_pad (stream->udpsrc[0], "src");

    /* save it so we can unblock */
    stream->blockedpad = *outpad;

    /* configure pad block on the pad. As soon as there is dataflow on the
     * UDP source, we know that UDP is not blocked by a firewall and we can
     * configure all the streams to let the application autoplug decoders. */
    gst_pad_set_blocked_async (stream->blockedpad, TRUE,
        (GstPadBlockCallback) pad_blocked, src);

    /* RTP port */
    if (stream->channelpad[0]) {
      GST_DEBUG_OBJECT (src, "connecting to session");
      /* configure for UDP delivery, we need to connect the UDP pads to
       * the session plugin. */
      gst_pad_link (*outpad, stream->channelpad[0]);
      gst_object_unref (*outpad);
      *outpad = NULL;
      /* we connected to pad-added signal to get pads from the manager */
    } else {
      GST_DEBUG_OBJECT (src, "using UDP src pad as output");
    }
  }

  /* RTCP port */
  if (stream->udpsrc[1]) {
    gst_bin_add (GST_BIN_CAST (src), stream->udpsrc[1]);

    if (stream->channelpad[1]) {
      GstPad *pad;

      GST_DEBUG_OBJECT (src, "connecting UDP source 1 to manager");

      pad = gst_element_get_static_pad (stream->udpsrc[1], "src");
      gst_pad_link (pad, stream->channelpad[1]);
      gst_object_unref (pad);
    } else {
      /* leave unlinked */
    }
  }

  /* Retransmitted RTP port */
  if (stream->udpsrc[2]) {
    gst_bin_add (GST_BIN_CAST (src), stream->udpsrc[2]);

    if (stream->channelpad[2]) {
      GstPad *pad;

      GST_DEBUG_OBJECT (src, "connecting UDP source 2 to requester");
      pad = gst_element_get_static_pad (stream->udpsrc[2], "src");
      gst_pad_link (pad, stream->channelpad[2]);
      gst_object_unref (pad);
    } else {
      /* leave unlinked */
    }
  }

  return TRUE;
}

/* configure the UDP sink back to the server for status reports */
static gboolean
gst_wfdrtspsrc_configure_udp_sinks (GstWFDRTSPSrc * src,
    WFDRTSPStream * stream, GstRTSPTransport * transport)
{
  GstPad *pad;
  gint rtp_port, rtcp_port, sockfd = -1;
  gboolean do_rtp, do_rtcp;
  const gchar *destination =  NULL;
  gchar *uri;
#ifdef ENABLE_WFD_EXTENDED_FEATURES
  GstPad *rtcp_fb_pad;
  gint rtcp_fb_port;
  gboolean do_rtcp_fb;
#endif

  /* get transport info */
  gst_wfdrtspsrc_get_transport_info (src, stream, transport, &destination,
      &rtp_port, &rtcp_port);

#ifdef ENABLE_WFD_EXTENDED_FEATURES
   rtcp_fb_port = RTCP_FB_PORT;
#endif

  /* see what we need to do */
  do_rtp = (rtp_port != -1);
  /* it's possible that the server does not want us to send RTCP in which case
   * the port is -1 */
  do_rtcp = (rtcp_port != -1 && stream->session != NULL && src->do_rtcp);
#ifdef ENABLE_WFD_EXTENDED_FEATURES
  do_rtcp_fb = (rtcp_fb_port != -1 && stream->do_request);
#endif

  /* we need a destination when we have RTP or RTCP ports */
  if (destination == NULL && (do_rtp || do_rtcp))
    goto no_destination;

  /* try to construct the fakesrc to the RTP port of the server to open up any
   * NAT firewalls */
  if (do_rtp) {
    GST_DEBUG_OBJECT (src, "configure RTP UDP sink for %s:%d", destination,
        rtp_port);

    uri = g_strdup_printf ("udp://%s:%d", destination, rtp_port);
    stream->udpsink[0] = gst_element_make_from_uri (GST_URI_SINK, uri, NULL);
    g_free (uri);
    if (stream->udpsink[0] == NULL)
      goto no_sink_element;

    /* don't join multicast group, we will have the source socket do that */
    /* no sync or async state changes needed */
    g_object_set (G_OBJECT (stream->udpsink[0]), "auto-multicast", FALSE,
        "loop", FALSE, "sync", FALSE, "async", FALSE, NULL);

    if (stream->udpsrc[0]) {
      /* configure socket, we give it the same UDP socket as the udpsrc for RTP
       * so that NAT firewalls will open a hole for us */
      g_object_get (G_OBJECT (stream->udpsrc[0]), "sock", &sockfd, NULL);
      GST_DEBUG_OBJECT (src, "RTP UDP src has sock %d", sockfd);
      /* configure socket and make sure udpsink does not close it when shutting
       * down, it belongs to udpsrc after all. */
      g_object_set (G_OBJECT (stream->udpsink[0]), "sockfd", sockfd,
          "closefd", FALSE, NULL);
    }

    /* the source for the dummy packets to open up NAT */
    stream->fakesrc = gst_element_factory_make ("fakesrc", NULL);
    if (stream->fakesrc == NULL)
      goto no_fakesrc_element;

    /* random data in 5 buffers, a size of 200 bytes should be fine */
    g_object_set (G_OBJECT (stream->fakesrc), "filltype", 3, "num-buffers", 5,
        "sizetype", 2, "sizemax", 200, "silent", TRUE, NULL);

    /* we don't want to consider this a sink */
    GST_OBJECT_FLAG_UNSET (stream->udpsink[0], GST_ELEMENT_IS_SINK);

    /* keep everything locked */
    gst_element_set_locked_state (stream->udpsink[0], TRUE);
    gst_element_set_locked_state (stream->fakesrc, TRUE);

    gst_object_ref (stream->udpsink[0]);
    gst_bin_add (GST_BIN_CAST (src), stream->udpsink[0]);
    gst_object_ref (stream->fakesrc);
    gst_bin_add (GST_BIN_CAST (src), stream->fakesrc);

    gst_element_link (stream->fakesrc, stream->udpsink[0]);
  }
  if (do_rtcp) {
    GST_DEBUG_OBJECT (src, "configure RTCP UDP sink for %s:%d", destination,
        rtcp_port);

    uri = g_strdup_printf ("udp://%s:%d", destination, rtcp_port);
    stream->udpsink[1] = gst_element_make_from_uri (GST_URI_SINK, uri, NULL);
    g_free (uri);
    if (stream->udpsink[1] == NULL)
      goto no_sink_element;

    /* don't join multicast group, we will have the source socket do that */
    /* no sync or async state changes needed */
    g_object_set (G_OBJECT (stream->udpsink[1]), "auto-multicast", FALSE,
        "loop", FALSE, "sync", FALSE, "async", FALSE, NULL);

    if (stream->udpsrc[1]) {
      /* configure socket, we give it the same UDP socket as the udpsrc for RTCP
       * because some servers check the port number of where it sends RTCP to identify
       * the RTCP packets it receives */
      g_object_get (G_OBJECT (stream->udpsrc[1]), "sock", &sockfd, NULL);
      GST_DEBUG_OBJECT (src, "RTCP UDP src has sock %d", sockfd);
      /* configure socket and make sure udpsink does not close it when shutting
       * down, it belongs to udpsrc after all. */
      g_object_set (G_OBJECT (stream->udpsink[1]), "sockfd", sockfd,
          "closefd", FALSE, NULL);
    }

    /* we don't want to consider this a sink */
    GST_OBJECT_FLAG_UNSET (stream->udpsink[1], GST_ELEMENT_IS_SINK);

    /* we keep this playing always */
    gst_element_set_locked_state (stream->udpsink[1], TRUE);
    gst_element_set_state (stream->udpsink[1], GST_STATE_PLAYING);

    gst_object_ref (stream->udpsink[1]);
    gst_bin_add (GST_BIN_CAST (src), stream->udpsink[1]);

    stream->rtcppad = gst_element_get_static_pad (stream->udpsink[1], "sink");

    /* get session RTCP pad */
    pad = gst_element_get_request_pad (stream->session, "send_rtcp_src");

    /* and link */
    if (pad) {
      gst_pad_link (pad, stream->rtcppad);
      gst_object_unref (pad);
    }
  }

#ifdef ENABLE_WFD_EXTENDED_FEATURES
  if (do_rtcp_fb) {
    GST_DEBUG_OBJECT (src, "configure RTCP FB sink for %s:%d", destination,
        rtcp_fb_port);

    uri = g_strdup_printf ("udp://%s:%d", destination, rtcp_fb_port);
    stream->udpsink[2] = gst_element_make_from_uri (GST_URI_SINK, uri, NULL);
    g_free (uri);
    if (stream->udpsink[2] == NULL)
      goto no_sink_element;

    /* don't join multicast group, we will have the source socket do that */
    /* no sync or async state changes needed */
    g_object_set (G_OBJECT (stream->udpsink[2]), "auto-multicast", FALSE,
        "loop", FALSE, "sync", FALSE, "async", FALSE, NULL);

    if (stream->udpsrc[2]) {
      /* configure socket, we give it the same UDP socket as the udpsrc for RTCP
       * because some servers check the port number of where it sends RTCP to identify
       * the RTCP packets it receives */
      g_object_get (G_OBJECT (stream->udpsrc[2]), "sock", &sockfd, NULL);
      GST_DEBUG_OBJECT (src, "RTCP UDP src has sock %d", sockfd);
      /* configure socket and make sure udpsink does not close it when shutting
       * down, it belongs to udpsrc after all. */
      g_object_set (G_OBJECT (stream->udpsink[2]), "sockfd", sockfd,
          "closefd", FALSE, NULL);
    }

    /* we don't want to consider this a sink */
    GST_OBJECT_FLAG_UNSET (stream->udpsink[2], GST_ELEMENT_IS_SINK);

    /* we keep this playing always */
    gst_element_set_locked_state (stream->udpsink[2], TRUE);
    gst_element_set_state (stream->udpsink[2], GST_STATE_PLAYING);

    gst_object_ref (stream->udpsink[2]);
    gst_bin_add (GST_BIN_CAST (src), stream->udpsink[2]);

    /* get RTCP FB sink pad */
    rtcp_fb_pad = gst_element_get_static_pad (stream->udpsink[2], "sink");

    /* get requester RTCP pad */
    pad = gst_element_get_static_pad (stream->requester, "rtcp_src");

    /* and link */
    if (rtcp_fb_pad && pad) {
      gst_pad_link (pad, rtcp_fb_pad);
      gst_object_unref (pad);
      gst_object_unref (rtcp_fb_pad);
    }
  }
#endif

  return TRUE;

  /* ERRORS */
no_destination:
  {
    GST_DEBUG_OBJECT (src, "no destination address specified");
    return FALSE;
  }
no_sink_element:
  {
    GST_DEBUG_OBJECT (src, "no UDP sink element found");
    return FALSE;
  }
no_fakesrc_element:
  {
    GST_DEBUG_OBJECT (src, "no fakesrc element found");
    return FALSE;
  }
}

/* sets up all elements needed for streaming over the specified transport.
 * Does not yet expose the element pads, this will be done when there is actuall
 * dataflow detected, which might never happen when UDP is blocked in a
 * firewall, for example.
 */
static gboolean
gst_wfdrtspsrc_configure_transport (WFDRTSPStream * stream,
    GstRTSPTransport * transport)
{
  GstWFDRTSPSrc *src;
  GstPad *outpad = NULL;
  GstPadTemplate *template;
  gchar *name;
  GstStructure *s;
  const gchar *mime;

  src = GST_WFDRTSPSRC (stream->parent);

  GST_DEBUG_OBJECT (src, "configuring transport for stream %p", stream);

  s = gst_caps_get_structure (stream->caps, 0);

  /* get the proper mime type for this stream now */
  if (gst_rtsp_transport_get_mime (transport->trans, &mime) < 0)
    goto unknown_transport;
  if (!mime)
    goto unknown_transport;

  /* configure the final mime type */
  GST_DEBUG_OBJECT (src, "setting mime to %s", mime);
  gst_structure_set_name (s, mime);

  /* try to get and configure a manager, channelpad[0-2] will be configured with
   * the pads for the manager, or NULL when no manager is needed. */
  if (!gst_wfdrtspsrc_set_manager (src, stream))
    goto no_manager;

  switch (transport->lower_transport) {
    case GST_RTSP_LOWER_TRANS_TCP:
    case GST_RTSP_LOWER_TRANS_UDP_MCAST:
     goto transport_failed;
    case GST_RTSP_LOWER_TRANS_UDP:
      if (!gst_wfdrtspsrc_configure_udp (src, stream, transport, &outpad))
        goto transport_failed;
      /* configure udpsinks back to the server for RTCP messages and for the
       * dummy RTP messages to open NAT. */
      if (!gst_wfdrtspsrc_configure_udp_sinks (src, stream, transport))
        goto transport_failed;
      break;
    default:
      goto unknown_transport;
  }

  if (outpad) {
    /* if stream->channelpad[0] is null, use src pad of udpsrc[0] as ghostpad of wfdrtspsrc */
    GST_DEBUG_OBJECT (src, "creating ghostpad");

    gst_pad_use_fixed_caps (outpad);

    /* create ghostpad, don't add just yet, this will be done when we activate
     * the stream. */
    name = g_strdup_printf ("stream%d", stream->id);
    template = gst_static_pad_template_get (&rtptemplate);
    stream->srcpad = gst_ghost_pad_new_from_template (name, outpad, template);
    gst_pad_set_event_function (stream->srcpad, gst_wfdrtspsrc_handle_src_event);
    gst_pad_set_query_function (stream->srcpad, gst_wfdrtspsrc_handle_src_query);
    gst_object_unref (template);
    g_free (name);

    gst_object_unref (outpad);
  }
  stream->protocol = transport->lower_transport;
  /* mark pad as ok */
  stream->last_ret = GST_FLOW_OK;

  return TRUE;

  /* ERRORS */
transport_failed:
  {
    GST_DEBUG_OBJECT (src, "failed to configure transport");
    return FALSE;
  }
unknown_transport:
  {
    GST_DEBUG_OBJECT (src, "unknown transport");
    return FALSE;
  }
no_manager:
  {
    GST_DEBUG_OBJECT (src, "cannot get a session manager");
    return FALSE;
  }
}

/* send a couple of dummy random packets on the receiver RTP port to the server,
 * this should make a firewall think we initiated the data transfer and
 * hopefully allow packets to go from the sender port to our RTP receiver port */
static gboolean
gst_wfdrtspsrc_send_dummy_packets (GstWFDRTSPSrc * src)
{
  WFDRTSPStream *stream;

  if (src->nat_method != GST_WFD_RTSP_NAT_DUMMY)
    return TRUE;

  if (src->stream) {
    stream = src->stream;
    if (stream->fakesrc && stream->udpsink[0]) {
      GST_DEBUG_OBJECT (src, "sending dummy packet to stream %p", stream);
      gst_element_set_state (stream->udpsink[0], GST_STATE_NULL);
      gst_element_set_state (stream->fakesrc, GST_STATE_NULL);
      gst_element_set_state (stream->udpsink[0], GST_STATE_PLAYING);
      gst_element_set_state (stream->fakesrc, GST_STATE_PLAYING);
    }
  }

  return TRUE;
}

/* Adds the source pads of all configured streams to the element.
 * This code is performed when we detected dataflow.
 *
 * We detect dataflow from either the _loop function or with pad probes on the
 * udp sources.
 */
static gboolean
gst_wfdrtspsrc_activate (GstWFDRTSPSrc * src)
{
  WFDRTSPStream *stream;

  GST_DEBUG_OBJECT (src, "activating streams");

  stream = src->stream;
  if (stream->udpsrc[0]) {
    /* remove timeout, we are streaming now and timeouts will be handled by
     * the session manager and jitter buffer */
    g_object_set (G_OBJECT (stream->udpsrc[0]), "timeout", (guint64) 0, NULL);
  }
#ifdef ENABLE_WFD_EXTENDED_FEATURES
  if (stream->udpsrc[2]) {
    /* remove timeout, we are streaming now and timeouts will be handled by
     * the session manager and jitter buffer */
    g_object_set (G_OBJECT (stream->udpsrc[2]), "timeout", (guint64) 0, NULL);
  }
#endif
  if (stream->srcpad) {
    GST_DEBUG_OBJECT (src, "setting pad caps for stream %p", stream);
    gst_pad_set_caps (stream->srcpad, stream->caps);

    GST_DEBUG_OBJECT (src, "activating stream pad %p", stream);
    gst_pad_set_active (stream->srcpad, TRUE);
    /* add the pad */
    if (!stream->added) {
      GST_DEBUG_OBJECT (src, "adding stream pad %p", stream);
      gst_element_add_pad (GST_ELEMENT_CAST (src), stream->srcpad);
      stream->added = TRUE;
    }
  }

  /* unblock all pads */
  if (stream->blockedpad) {
    GST_DEBUG_OBJECT (src, "unblocking stream pad %p", stream);
    gst_pad_set_blocked_async (stream->blockedpad, FALSE,
        (GstPadBlockCallback) pad_unblocked, src);
    stream->blockedpad = NULL;
  }

  return TRUE;
}

static void
gst_wfdrtspsrc_configure_caps (GstWFDRTSPSrc * src, GstSegment * segment)
{
  WFDRTSPStream *stream;
  GstCaps *caps;
  guint64 start, stop;
  gdouble play_speed, play_scale;
  GstStructure *structure;

  GST_DEBUG_OBJECT (src, "configuring stream caps");

  segment->duration = GST_CLOCK_TIME_NONE;
  segment->applied_rate = 1.000;

  start = segment->last_stop;
  stop = segment->duration;
  play_speed = segment->rate;
  play_scale = segment->applied_rate;
  stop = GST_CLOCK_TIME_NONE;
  play_scale = 1.000;

  stream = src->stream;

  if ((caps = stream->caps)) {
    caps = gst_caps_make_writable (caps);
    structure = gst_caps_get_structure (caps, 0);
    /* update caps */
    if (stream->timebase != -1)
      gst_structure_set (structure, "clock-base", G_TYPE_UINT,
          (guint) stream->timebase, NULL);
    if (stream->seqbase != -1)
      gst_structure_set (structure, "seqnum-base", G_TYPE_UINT,
          (guint) stream->seqbase, NULL);
    gst_structure_set (structure, "npt-start", G_TYPE_UINT64, start, NULL);
    if (stop != -1)
      gst_structure_set (structure, "npt-stop", G_TYPE_UINT64, stop, NULL);
    gst_structure_set (structure, "play-speed", G_TYPE_DOUBLE, play_speed, NULL);
    gst_structure_set (structure, "play-scale", G_TYPE_DOUBLE, play_scale, NULL);
    gst_structure_set (structure, "height", G_TYPE_INT, src->video_height, NULL);
    gst_structure_set (structure, "width", G_TYPE_INT, src->video_width, NULL);
    gst_structure_set (structure, "video-framerate", G_TYPE_INT, src->video_framerate, NULL);

    stream->caps = caps;
  }
  GST_DEBUG_OBJECT (src, "stream %p, caps %" GST_PTR_FORMAT, stream, caps);

  if (stream->srcpad) {
    GST_DEBUG_OBJECT (src, "set caps for srcpad");
    gst_pad_set_caps(stream->srcpad, stream->caps);
  }
#if 0
  if (stream->session) {
    GST_DEBUG_OBJECT (src, "clear session");
    g_signal_emit_by_name (stream->session, "clear-pt-map", NULL);
  }
  if (stream->jitterbuffer) {
    GST_DEBUG_OBJECT (src, "clear jitterbuffer");
    g_signal_emit_by_name (stream->jitterbuffer, "clear-pt-map", NULL);
  }
#endif
}

static GstFlowReturn
gst_wfdrtspsrc_combine_flows (GstWFDRTSPSrc * src, WFDRTSPStream * stream,
    GstFlowReturn ret)
{
  WFDRTSPStream *ostream;

  ostream = src->stream;

  /* store the value */
  stream->last_ret = ret;

  /* if it's success we can return the value right away */
  if (ret == GST_FLOW_OK)
    goto done;

  /* any other error that is not-linked can be returned right
   * away */
  if (ret != GST_FLOW_NOT_LINKED)
    goto done;

  /* only return NOT_LINKED if all other pads returned NOT_LINKED */
  ret = ostream->last_ret;
  /* some other return value (must be SUCCESS but we can return
   * other values as well) */
  if (ret != GST_FLOW_NOT_LINKED)
    goto done;
  /* if we get here, all other pads were unlinked and we return
   * NOT_LINKED then */
done:
  return ret;
}

static gboolean
gst_wfdrtspsrc_stream_push_event (GstWFDRTSPSrc * src, WFDRTSPStream * stream,
    GstEvent * event, gboolean source)
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

  if (source && stream->udpsrc[1]) {
    gst_event_ref (event);
    res &= gst_element_send_event (stream->udpsrc[1], event);
  } else if (stream->channelpad[1]) {
    gst_event_ref (event);
    if (GST_PAD_IS_SRC (stream->channelpad[1]))
      res &= gst_pad_push_event (stream->channelpad[1], event);
    else
      res &= gst_pad_send_event (stream->channelpad[1], event);
  }

  if (source && stream->udpsrc[2]) {
    gst_event_ref (event);
    res &= gst_element_send_event (stream->udpsrc[2], event);
  } else if (stream->channelpad[2]) {
    gst_event_ref (event);
    if (GST_PAD_IS_SRC (stream->channelpad[2]))
      res &= gst_pad_push_event (stream->channelpad[2], event);
    else
      res &= gst_pad_send_event (stream->channelpad[2], event);
  }

done:
  gst_event_unref (event);

  return res;
}

static gboolean
gst_wfdrtspsrc_push_event (GstWFDRTSPSrc * src, GstEvent * event, gboolean source)
{
  gboolean res = TRUE;

  if (src->stream) {
    gst_event_ref (event);
    res &= gst_wfdrtspsrc_stream_push_event (src, src->stream, event, source);
    gst_event_unref (event);
  }

  return res;
}

static GstRTSPResult
gst_wfdrtspsrc_conninfo_connect (GstWFDRTSPSrc * src, GstWFDRTSPConnInfo * info)
{
  GstRTSPResult res;

  if (info->connection == NULL) {
    if (info->url == NULL) {
      GST_DEBUG_OBJECT (src, "parsing uri (%s)...", info->location);
      if ((res = gst_rtsp_url_parse (info->location, &info->url)) < 0)
        goto parse_error;
    }

    /* create connection */
    GST_DEBUG_OBJECT (src, "creating connection (%s)...", info->location);
    if ((res = gst_rtsp_connection_create (info->url, &info->connection)) < 0)
      goto could_not_create;

    if (info->url_str)
      g_free (info->url_str);
    info->url_str = gst_rtsp_url_get_request_uri (info->url);

    GST_DEBUG_OBJECT (src, "sanitized uri %s", info->url_str);

    if (info->url->transports & GST_RTSP_LOWER_TRANS_HTTP)
      gst_rtsp_connection_set_tunneled (info->connection, TRUE);

    if (src->proxy_host) {
      GST_DEBUG_OBJECT (src, "setting proxy %s:%d", src->proxy_host,
          src->proxy_port);
      gst_rtsp_connection_set_proxy (info->connection, src->proxy_host,
          src->proxy_port);
    }
  }

  if (!info->connected) {
    /* connect */
    GST_DEBUG_OBJECT (src, "connecting (%s)...", info->location);
    if ((res =
            gst_rtsp_connection_connect (info->connection,
                src->ptcp_timeout)) < 0)
      goto could_not_connect;

    info->connected = TRUE;
  }
  return GST_RTSP_OK;

  /* ERRORS */
parse_error:
  {
    GST_ERROR_OBJECT (src, "No valid RTSP URL was provided");
    return res;
  }
could_not_create:
  {
    gchar *str = gst_rtsp_strresult (res);
    GST_ERROR_OBJECT (src, "Could not create connection. (%s)", str);
    g_free (str);
    return res;
  }
could_not_connect:
  {
    gchar *str = gst_rtsp_strresult (res);
    GST_ERROR_OBJECT (src, "Could not connect to server. (%s)", str);
    g_free (str);
    return res;
  }
}

static GstRTSPResult
gst_wfdrtspsrc_conninfo_close (GstWFDRTSPSrc * src, GstWFDRTSPConnInfo * info,
    gboolean free)
{
  g_return_val_if_fail (info, GST_RTSP_EINVAL);

  if (info->connected) {
    GST_DEBUG_OBJECT (src, "closing connection...");
    gst_rtsp_connection_close (info->connection);
    info->connected = FALSE;
  }
  if (free && info->connection) {
    /* free connection */
    GST_DEBUG_OBJECT (src, "freeing connection...");
    gst_rtsp_connection_free (info->connection);
    info->connection = NULL;
  }
  return GST_RTSP_OK;
}

static GstRTSPResult
gst_wfdrtspsrc_conninfo_reconnect (GstWFDRTSPSrc * src, GstWFDRTSPConnInfo * info)
{
  GstRTSPResult res;

  GST_DEBUG_OBJECT (src, "reconnecting connection...");
  gst_wfdrtspsrc_conninfo_close (src, info, FALSE);
  res = gst_wfdrtspsrc_conninfo_connect (src, info);

  return res;
}

static void
gst_wfdrtspsrc_connection_flush (GstWFDRTSPSrc * src, gboolean flush)
{
  GST_DEBUG_OBJECT (src, "set flushing %d", flush);
  if (src->conninfo.connection) {
    GST_DEBUG_OBJECT (src, "connection flush %d", flush);
    gst_rtsp_connection_flush (src->conninfo.connection, flush);
  }

  if (src->stream) {
    WFDRTSPStream *stream = src->stream;
    GST_DEBUG_OBJECT (src, "stream %p flush %d", stream, flush);
    if (stream->conninfo.connection)
      gst_rtsp_connection_flush (stream->conninfo.connection, flush);
  }
  GST_DEBUG_OBJECT (src, "flushing %d is done", flush);
}

static WFDRTSPStream *
gst_wfdrtspsrc_create_stream (GstWFDRTSPSrc * src)
{
  WFDRTSPStream *stream;
  GstStructure *s;

  stream = g_new0 (WFDRTSPStream, 1);
  stream->parent = GST_ELEMENT_CAST(src);
  /* we mark the pad as not linked, we will mark it as OK when we add the pad to
  * the element. */
  stream->last_ret = GST_FLOW_NOT_LINKED;
  stream->added = FALSE;
  stream->disabled = FALSE;
  stream->id = 0;
  stream->eos = FALSE;
  stream->discont = TRUE;
  stream->seqbase = -1;
  stream->timebase = -1;
#ifdef ENABLE_WFD_EXTENDED_FEATURES
  stream->do_request = DEFAULT_DO_REQUEST;
  stream->buffering_animation = DEFAULT_BUF_ANIMATION;
  stream->demux_handle = NULL;
  stream->audio_queue_handle = NULL;
  stream->audio_data_per_sec = 0;
  stream->tcp_task = NULL;
  stream->tcp_status_report_task = NULL;
#endif

  /* Payload type for mpegts stream is 33 */
  stream->pt = 33;
  stream->caps = gst_caps_new_simple ("application/x-rtp",
                  "media", G_TYPE_STRING, "video", "payload", G_TYPE_INT, 33, NULL);

  s = gst_caps_get_structure (stream->caps, 0);
  gst_structure_set (s, "clock-rate", G_TYPE_INT, 90000, NULL);
  gst_structure_set (s, "encoding-params", G_TYPE_STRING, "MP2T-ES", NULL);

  gst_wfdrtspsrc_configure_manager (src, stream);

  return stream;
}

static GstRTSPResult
gst_wfdrtspsrc_handle_request (GstWFDRTSPSrc * src, GstRTSPConnection * conn,
    GstRTSPMessage * request)
{
  GstRTSPMessage response = { 0 };
  GstRTSPResult res;
  GstRTSPMethod method;
  const gchar *uristr;
  GstRTSPVersion version;
  GstRTSPMethod options;
  gchar *str;
  gchar *ua;
  guint8 *data;
  guint size;

  GST_DEBUG_OBJECT (src, "got server request message");

  res = gst_wfd_rtsp_ext_list_receive_request (src->extensions, request);

  gst_rtsp_message_parse_request (request, &method, &uristr, &version);

  if (version != GST_RTSP_VERSION_1_0) {
    /* we can only handle 1.0 requests */
    res = GST_RTSP_ENOTIMPL;
  }

  if (src->debug)
    gst_rtsp_message_dump (request);

  switch(method) {
    case GST_RTSP_OPTIONS:
    {
      gchar *tmp = NULL;
      char *options_str = NULL;

      options = GST_RTSP_GET_PARAMETER |GST_RTSP_SET_PARAMETER;

      str = gst_rtsp_options_as_text (options);

      tmp = g_strdup (", org.wfa.wfd1.0");

      GST_LOG ("tmp = %s\n\n", tmp);

      options_str = (char *) malloc (strlen(tmp) + strlen (str) + 1);
      if (!options_str) {
        GST_ERROR ("Failed to allocate memory...");
        res = GST_RTSP_ENOMEM;
        goto send_error;
      } else {
        strncpy (options_str, str, strlen (str));
        strncpy (options_str+strlen(str), tmp, strlen (tmp));
        options_str [strlen(tmp) + strlen (str)] = '\0';

        GST_LOG ("\n\noptions_str = %s\n\n", options_str);
      }
      GST_DEBUG_OBJECT (src, "Creating OPTIONS response");

      res = gst_rtsp_message_init_response (&response, GST_RTSP_STS_OK,
          gst_rtsp_status_as_text (GST_RTSP_STS_OK), request);
      if(res < 0) {
        if(options_str)
          g_free (options_str);
        goto send_error;
      }
      ua = g_strdup(src->user_agent);
      gst_rtsp_message_add_header (&response, GST_RTSP_HDR_PUBLIC, options_str);
      gst_rtsp_message_add_header (&response, GST_RTSP_HDR_USER_AGENT, ua);
      if(options_str)
        g_free (options_str);
      if(ua)
        g_free (ua);
      break;
    }

    case GST_RTSP_GET_PARAMETER:
    {
      gchar *msg;
      guint msglen = 0;
      GString *msglength;
      WFDMessage *msg3rep;
      WFDResult wfd_res = WFD_OK;

      GST_DEBUG_OBJECT (src, "Got GET_PARAMETER request");

      res = gst_rtsp_message_init_response (&response, GST_RTSP_STS_OK, gst_rtsp_status_as_text (GST_RTSP_STS_OK), request);
      if(res < 0)
        goto send_error;

      gst_rtsp_message_add_header (&response, GST_RTSP_HDR_CONTENT_TYPE, "text/parameters");

      ua = g_strdup(src->user_agent);
      gst_rtsp_message_add_header (&response, GST_RTSP_HDR_USER_AGENT, ua);

      gst_rtsp_message_get_body (request, &data, &size);

      /* Check if message is keep-alive if body is empty*/
      if(size==0) {
        res = gst_rtsp_message_init_response (&response, GST_RTSP_STS_OK, "OK",
              request);
        gst_rtsp_connection_reset_timeout (src->conninfo.connection);
        if (res < 0)
          goto send_error;
        break;
      }

      /* Creating WFD parameters supported to send as response for
        * GET_PARAMETER request from server*/

      wfdconfig_message_new(&msg3rep);
      if(msg3rep == NULL) {
        res = GST_RTSP_ERROR;
        goto send_error;
      }

      wfd_res = wfdconfig_message_init(msg3rep);

      wfd_res = wfdconfig_message_parse_buffer(data,size,msg3rep);

      if(wfd_res != WFD_OK) {
        res = GST_RTSP_ERROR;
        goto send_error;
      }
      wfdconfig_message_dump(msg3rep);

      /* Check if the request message has "wfd_audio_codecs" in it */
      if(msg3rep->audio_codecs) {
        guint audio_codec = 0;
        guint audio_sampling_frequency = 0;
        guint audio_channels = 0;
        guint audio_latency = 0;
        //reading audio parameters which are set from sink ini file
        if(src->audio_param != NULL) {
          if (gst_structure_has_field (src->audio_param, "audio_codec")) {
            gst_structure_get_uint (src->audio_param, "audio_codec", &audio_codec);
          }
          if (gst_structure_has_field (src->audio_param, "audio_latency")) {
            gst_structure_get_uint (src->audio_param, "audio_latency", &audio_latency);
          }
          if (gst_structure_has_field (src->audio_param, "audio_channels")) {
            gst_structure_get_uint (src->audio_param, "audio_channels", &audio_channels);
          }
          if (gst_structure_has_field (src->audio_param, "audio_sampling_frequency")) {
            gst_structure_get_uint (src->audio_param, "audio_sampling_frequency", &audio_sampling_frequency);
          }
        }

        wfd_res = wfdconfig_set_supported_audio_format(msg3rep, audio_codec,
            audio_sampling_frequency, audio_channels, 16, audio_latency);
      }

      /* Check if the request message has "wfd_video_formats" in it */
      if(msg3rep->video_formats) {
        guint video_codec = 0;
        guint video_native_resolution = 0;
        guint video_cea_support = 0;
        guint video_vesa_support = 0;
        guint video_hh_support = 0;
        guint video_profile = 0;
        guint video_level = 0;
        guint video_latency = 0;
        gint video_vertical_resolution = 0;
        gint video_horizontal_resolution = 0;
        gint video_minimum_slicing = 0;
        gint video_slice_enc_param = 0;
        gint video_framerate_control_support = 0;

        //reading video parameters which are set from sink ini file
        if (src->video_param != NULL) {
          if (gst_structure_has_field (src->video_param, "video_codec")) {
            gst_structure_get_uint (src->video_param, "video_codec", &video_codec);
          }
          if (gst_structure_has_field (src->video_param, "video_native_resolution")) {
            gst_structure_get_uint (src->video_param, "video_native_resolution", &video_native_resolution);
          }
          if (gst_structure_has_field (src->video_param, "video_cea_support")) {
            gst_structure_get_uint (src->video_param, "video_cea_support", &video_cea_support);
          }
          if (gst_structure_has_field (src->video_param, "video_vesa_support")) {
            gst_structure_get_uint (src->video_param, "video_vesa_support", &video_vesa_support);
          }
          if (gst_structure_has_field (src->video_param, "video_hh_support")) {
            gst_structure_get_uint (src->video_param, "video_hh_support", &video_hh_support);
          }
          if (gst_structure_has_field (src->video_param, "video_profile")) {
            gst_structure_get_uint (src->video_param, "video_profile", &video_profile);
          }
          if (gst_structure_has_field (src->video_param, "video_level")) {
            gst_structure_get_uint (src->video_param, "video_level", &video_level);
          }
          if (gst_structure_has_field (src->video_param, "video_latency")) {
            gst_structure_get_uint (src->video_param, "video_latency", &video_latency);
          }
          if (gst_structure_has_field (src->video_param, "video_vertical_resolution")) {
            gst_structure_get_int (src->video_param, "video_vertical_resolution", &video_vertical_resolution);
          }
          if (gst_structure_has_field (src->video_param, "video_horizontal_resolution")) {
            gst_structure_get_int (src->video_param, "video_horizontal_resolution", &video_horizontal_resolution);
          }
          if (gst_structure_has_field (src->video_param, "video_minimum_slicing")) {
            gst_structure_get_int (src->video_param, "video_minimum_slicing", &video_minimum_slicing);
          }
          if (gst_structure_has_field (src->video_param, "video_slice_enc_param")) {
            gst_structure_get_int (src->video_param, "video_slice_enc_param", &video_slice_enc_param);
          }
          if (gst_structure_has_field (src->video_param, "video_framerate_control_support")) {
            gst_structure_get_int (src->video_param, "video_framerate_control_support", &video_framerate_control_support);
          }
        }


        wfd_res = wfdconfig_set_supported_video_format(msg3rep, video_codec,
            WFD_VIDEO_CEA_RESOLUTION, video_native_resolution,
            video_cea_support,
            video_vesa_support,
            video_hh_support,
            video_profile, video_level, video_latency, video_vertical_resolution, video_horizontal_resolution, video_minimum_slicing, video_slice_enc_param, video_framerate_control_support);
      }

      /* Check if the request has "wfd_client_rtp_ports" in it */
      if(msg3rep->client_rtp_ports) {
        /* Hardcoded as of now. This is to comply with dongle port settings.
        This should be derived from gst_wfdrtspsrc_alloc_udp_ports */
        src->primary_rtpport = 19000;
        src->secondary_rtpport = 0;
        wfd_res = wfdconfig_set_prefered_RTP_ports(msg3rep, WFD_RTSP_TRANS_RTP, WFD_RTSP_PROFILE_AVP,
                      WFD_RTSP_LOWER_TRANS_UDP, src->primary_rtpport, src->secondary_rtpport);
      }

      if(wfd_res != WFD_OK) {
        res = GST_RTSP_ERROR;
        goto send_error;
      }

      /* Check if the request has "wfd_3d_video_formats " in it */
      if(msg3rep->video_3d_formats) {
        /* TODO: Set preferred 3d_video_formats */
        wfd_res = WFD_OK;
      }

      /* When set preferred 3d_video_formats is done, following block need to be uncommented */
      /*if(wfd_res != WFD_OK) {
      res = GST_RTSP_ERROR;
      goto send_error;
      }*/

      /* Check if the request has "wfd_content_protection " in it */
      if(msg3rep->content_protection) {
        hdcp_config_t* hdcp_config = NULL;
        //get hdcp parameters from HAL
        wfd_hal_get_hdcp_config(&hdcp_config);
        wfd_res = wfdconfig_set_contentprotection_type(msg3rep, hdcp_config->hdcp_version, hdcp_config->hdcp_port);
        if(wfd_res != WFD_OK) {
          res = GST_RTSP_ERROR;
          goto send_error;
        }
      }

      if(msg3rep->uibc_capability) {
        src->uibc->sockfd = -1;
        src->uibc->gen_capability = DEFAULT_UIBC_GEN_CAPABILITY;
        src->uibc->hidc_cap_list = (WFDHIDCTypePathPair *)malloc (sizeof(WFDHIDCTypePathPair) * 4);
        src->uibc->hidc_cap_list[0].inp_type = WFD_UIBC_INPUT_TYPE_KEYBOARD;
        src->uibc->hidc_cap_list[0].inp_path = WFD_UIBC_INPUT_PATH_BT;
        src->uibc->hidc_cap_list[1].inp_type = WFD_UIBC_INPUT_TYPE_KEYBOARD;
        src->uibc->hidc_cap_list[1].inp_path = WFD_UIBC_INPUT_PATH_USB;
        src->uibc->hidc_cap_list[2].inp_type = WFD_UIBC_INPUT_TYPE_MOUSE;
        src->uibc->hidc_cap_list[2].inp_path = WFD_UIBC_INPUT_PATH_BT;
        src->uibc->hidc_cap_list[3].inp_type = WFD_UIBC_INPUT_TYPE_MOUSE;
        src->uibc->hidc_cap_list[3].inp_path = WFD_UIBC_INPUT_PATH_USB;
        wfd_res = wfdconfig_set_uibc_capability (msg3rep, WFD_UIBC_INPUT_CAT_GENERIC, src->uibc->gen_capability,
          src->uibc->hidc_cap_list, DEFAULT_UIBC_HIDC_CAP_COUNT, 0);
      }
      if(wfd_res  != WFD_OK) {
        GST_ERROR("failed to set_uibc_capability.");
        res = GST_RTSP_ERROR;
        goto send_error;
      }
      /* Check if the request has "wfd_display_edid" in it */
      if(msg3rep->display_edid) {
        /* TODO: Set preferred display_edid */
        wfd_res = WFD_OK;
      }

      /* when set preferred display_edid following block need to be uncommented. */
      /*if(wfd_res != WFD_OK) {
      res = GST_RTSP_ERROR;
      goto send_error;
      }*/

      /* Check if the request has "wfd_coupled_sink" in it */
      if(msg3rep->coupled_sink) {
        //wfd_res = wfdconfig_set_coupled_sink(msg3rep, WFD_SINK_NOT_COUPLED, NULL);
        /* To test with dummy coupled sink address */
        wfd_res = wfdconfig_set_coupled_sink(msg3rep,WFD_SINK_COUPLED,"1.0.0.1:435");
      }

      if(wfd_res != WFD_OK) {
        res = GST_RTSP_ERROR;
        goto send_error;
      }
      msg = wfdconfig_message_as_text(msg3rep);

      msglen = strlen(msg);
      msglength = g_string_new ("");
      g_string_append_printf (msglength,"%d",msglen);

      GST_DEBUG_OBJECT (src, "GET_PARAMETER response message body: %s\n\n", msg);

      gst_rtsp_message_set_body (&response, (guint8*) msg, msglen);

      wfdconfig_message_free(msg3rep);

      if(ua)
        g_free(ua);

      break;
      }

    case GST_RTSP_SET_PARAMETER:
    {
      WFDMessage *msg = NULL;
      WFDResult wfd_res = WFD_OK;
      gchar *url0, *url1;

      GST_DEBUG_OBJECT (src, "Got SET_PARAMETER request");

      res = gst_rtsp_message_init_response (&response, GST_RTSP_STS_OK,
            gst_rtsp_status_as_text (GST_RTSP_STS_OK), request);
      if (res < 0)
        goto send_error;

      ua = g_strdup (src->user_agent);
      gst_rtsp_message_add_header (&response, GST_RTSP_HDR_USER_AGENT, ua);

      gst_rtsp_message_get_body (request, &data, &size);

      wfdconfig_message_new(&msg);
      if(msg == NULL) {
        res = GST_RTSP_ERROR;
        goto send_error;
      }

      wfd_res = wfdconfig_message_init(msg);
      if(wfd_res != WFD_OK) {
        res = GST_RTSP_ERROR;
        goto send_error;
      }
      wfd_res = wfdconfig_message_parse_buffer(data,size,msg);
      if(wfd_res != WFD_OK) {
        res = GST_RTSP_ERROR;
        goto send_error;
      }

      if(msg->video_formats || msg->audio_codecs) {
        GString *result;
        GstStructure *struct_message;
        GstBus *bus;

        if(g_strrstr((gchar*)data,"wfd_audio_codecs")) {
          /*Tells application which format of audio to use*/

          gint audio_format = WFD_AUDIO_UNKNOWN;
          gint audio_latency = 0;
          gint audio_channels = WFD_CHANNEL_UNKNOWN;
          gint audio_bitwidth = 0;
          gint audio_frequency = WFD_FREQ_UNKNOWN;


          result = g_string_new("audiotype_");
          g_string_append_printf(result,"%s", msg->audio_codecs->list->audio_format);
          wfdconfig_get_prefered_audio_format(msg, &audio_format, &audio_frequency, &audio_channels, &audio_bitwidth, &audio_latency);

          if(audio_frequency == WFD_FREQ_48000)
            audio_frequency = 48000;
          else if(audio_frequency == WFD_FREQ_44100)
            audio_frequency = 44100;

          if(audio_channels == WFD_CHANNEL_2)
            audio_channels = 2;
          else if(audio_channels == WFD_CHANNEL_4)
            audio_channels = 4;
          else if(audio_channels == WFD_CHANNEL_6)
            audio_channels = 6;
          else if(audio_channels == WFD_CHANNEL_8)
            audio_channels = 8;

#ifdef ENABLE_WFD_EXTENDED_FEATURES
          src->stream->audio_data_per_sec = audio_frequency * audio_channels * (audio_bitwidth/8);
#endif

          struct_message = gst_structure_empty_new(g_string_free(result, FALSE));
          bus = gst_element_get_bus (GST_ELEMENT_CAST(src));
          gst_bus_post(bus, gst_message_new_application(GST_OBJECT_CAST(src), struct_message));
          gst_object_unref(bus);
        } else {
          /*Supporting only video streaming*/
          result = g_string_new ("video_only");

          struct_message = gst_structure_empty_new(g_string_free(result, FALSE));
          bus = gst_element_get_bus (GST_ELEMENT_CAST(src));
          gst_bus_post(bus, gst_message_new_application (GST_OBJECT_CAST(src), struct_message));
          gst_object_unref(bus);
        }

        if(!g_strrstr((gchar*)data,"wfd_video_formats")) {
            /*Supporting only audio streaming*/
          result = g_string_new("audio_only");

          struct_message = gst_structure_empty_new(g_string_free(result, FALSE));
          bus = gst_element_get_bus(GST_ELEMENT_CAST(src));
          gst_bus_post(bus, gst_message_new_application(GST_OBJECT_CAST(src), struct_message));
          gst_object_unref(bus);
        } else {
            wfd_res = gst_wfdrtspsrc_get_video_parametr(src, msg);
        }

        wfd_res = wfdconfig_get_presentation_url(msg, &url0, &url1);
        if(wfd_res == WFD_OK) {
          g_free (src->conninfo.location);
          src->conninfo.location = g_strdup (url0);
          /* url1 is ignored as of now */
        } else {
          res = GST_RTSP_ERROR;
          goto send_error;
        }
        wfdconfig_message_dump(msg);
      }

      if(msg->standby) {
        gboolean standby_enable;
        wfd_res = wfdconfig_get_standby(msg, &standby_enable);
        if(wfd_res == WFD_OK)
          GST_DEBUG("M12 server set param request STANDBY %s", standby_enable?"ENABLE":"DISABLE");
      }

      if(msg->uibc_capability) {
        guint32 input_category;
        guint32 inp_type;
        guint32 i = 0;
        wfd_res = wfdconfig_message_dump (msg);
        wfd_res = wfdconfig_get_uibc_capability(msg, &input_category, &inp_type, &src->uibc->hidc_cap_list_src, &src->uibc->hidc_cap_count_src, &src->uibc->port);
        GST_DEBUG ("uibc_port read is %u", src->uibc->port);
        if(wfd_res == WFD_OK) {
          GST_DEBUG("M14 server set param UIBC input category: %s %s", input_category&WFD_UIBC_INPUT_CAT_GENERIC?"GENERIC":"", input_category&WFD_UIBC_INPUT_CAT_HIDC?"HIDC":"");
          GST_DEBUG("UIBC input type: %s %s %s %s %s %s %s %s", inp_type&WFD_UIBC_INPUT_TYPE_KEYBOARD?"KEYBOARD":"", inp_type&WFD_UIBC_INPUT_TYPE_MOUSE?"MOUSE":"",
           inp_type&WFD_UIBC_INPUT_TYPE_SINGLETOUCH?"SINGLETOUCH":"", inp_type&WFD_UIBC_INPUT_TYPE_MULTITOUCH?"MULTITOUCH":"",
           inp_type&WFD_UIBC_INPUT_TYPE_JOYSTICK?"JOYSTICK":"", inp_type&WFD_UIBC_INPUT_TYPE_CAMERA?"CAMERA":"",
           inp_type&WFD_UIBC_INPUT_TYPE_GESTURE?"GESTURE":"", inp_type&WFD_UIBC_INPUT_TYPE_REMOTECONTROL?"REMOTECONTROL":"");
          for(;i<src->uibc->hidc_cap_count_src;i++) {
            switch (src->uibc->hidc_cap_list_src[i].inp_type) {
              case WFD_UIBC_INPUT_TYPE_KEYBOARD:
                GST_DEBUG("UIBC HIDC %d type KEYBOARD", i);
                break;
              case WFD_UIBC_INPUT_TYPE_MOUSE:
                GST_DEBUG("UIBC HIDC %d type MOUSE", i);
                break;
              case WFD_UIBC_INPUT_TYPE_SINGLETOUCH:
                GST_DEBUG("UIBC HIDC %d type SINGLETOUCH", i);
                break;
              case WFD_UIBC_INPUT_TYPE_MULTITOUCH:
                GST_DEBUG("UIBC HIDC %d type MULTITOUCH", i);
                break;
              case WFD_UIBC_INPUT_TYPE_JOYSTICK:
                GST_DEBUG("UIBC HIDC %d type JOYSTICK", i);
                break;
              case WFD_UIBC_INPUT_TYPE_CAMERA:
                GST_DEBUG("UIBC HIDC %d type CAMERA", i);
                break;
              case WFD_UIBC_INPUT_TYPE_GESTURE:
                GST_DEBUG("UIBC HIDC %d type GESTURE", i);
                break;
              case WFD_UIBC_INPUT_TYPE_REMOTECONTROL:
                GST_DEBUG("UIBC HIDC %d type REMOTECONTROL", i);
                break;
              default :
                GST_ERROR ("UIBC HIDC %d invalid type", i);
                break;
            }
            switch (src->uibc->hidc_cap_list_src[i].inp_path) {
              case WFD_UIBC_INPUT_PATH_INFRARED:
                GST_DEBUG("UIBC HIDC %d path INFRARED", i);
                break;
              case WFD_UIBC_INPUT_PATH_USB:
                GST_DEBUG("UIBC HIDC %d path USB", i);
                break;
              case WFD_UIBC_INPUT_PATH_BT:
                GST_DEBUG("UIBC HIDC %d path BT", i);
                break;
              case WFD_UIBC_INPUT_PATH_ZIGBEE:
                GST_DEBUG("UIBC HIDC %d path ZIGBEE", i);
                break;
              case WFD_UIBC_INPUT_PATH_WIFI:
                GST_DEBUG("UIBC HIDC %d path WIFI", i);
                break;
              case WFD_UIBC_INPUT_PATH_NOSP:
                GST_DEBUG("UIBC HIDC %d path NOSP", i);
                break;
              default:
                GST_ERROR ("UIBC HIDC %d invalid path", i);
                break;
            }
          }
        }
      }

      if(g_strrstr((gchar*)data,"wfd_uibc_setting")) {
        wfdconfig_message_dump (msg);
        gboolean uibc_enable = FALSE;
        wfd_res = wfdconfig_get_uibc_status(msg, &uibc_enable);
        if(wfd_res == WFD_OK) {
          if(uibc_enable == TRUE) {
            GST_DEBUG ("uibc_enable is TRUE");
            uibc_new_tcp (src);
          } else {
            src->uibc->state_cb (0, NULL, 0, src->video_width, src->video_height, src->uibc->wfd_sink_handle);
            shutdown (src->uibc->sockfd, SHUT_RDWR);
            src->uibc->sockfd = -1;
          }
        }
      }

#ifdef ENABLE_WFD_EXTENDED_FEATURES
    gboolean check_result = FALSE;
    gboolean need_idr = FALSE;
    /* make sure it is not running */
    res = wfd_stream_ext_hadle_request (src->stream, data, request, &check_result, &need_idr);
    if (check_result) {
      if (res != GST_RTSP_OK)
        goto send_error;
      else if (need_idr)
        gst_wfdrtspsrc_prepare_set_param (src, WFD_IDR_REQUEST);

      goto exit;
    }
#endif
      /* DEAD CODE It will be used for UIBC implementation
      else if(g_strrstr((gchar*)data,"wfd_standby")) {
        gboolean uibc_enable;
        wfd_res = wfdconfig_get_uibc_status(msg, &uibc_enable);
        if(wfd_res == WFD_OK)
          GST_DEBUG("M15 server set param request for UIBC %s", uibc_enable?"ENABLE":"DISABLE");
      }*/
      if (ua)
        g_free(ua);
      break;
    }

    default:
    {
      res = gst_rtsp_message_init_response (&response, GST_RTSP_STS_OK, "OK",
            request);
      if (res < 0)
        goto send_error;
      break;
    }
  }

  if (src->debug)
    gst_rtsp_message_dump (&response);

  res = gst_wfdrtspsrc_connection_send (src, conn, &response, NULL);
  if (res < 0)
    goto send_error;

  /* Handling wfd_trigger_method: PAUSE */
  if(method == GST_RTSP_SET_PARAMETER && g_strrstr((gchar*)data,"PAUSE")) {
    GST_DEBUG_OBJECT (src, "trigger PAUSE \n");
    gst_wfdrtspsrc_loop_send_cmd(src, WFD_CMD_PAUSE, FALSE);
  }

  /* Handling wfd_trigger_method: RESUME */
  if(method == GST_RTSP_SET_PARAMETER && g_strrstr((gchar*)data,"PLAY")) {
    GST_DEBUG_OBJECT (src, "trigger RESUME \n");
    gst_wfdrtspsrc_loop_send_cmd(src, WFD_CMD_PLAY, FALSE);
  }

  /* Handling wfd_trigger_method: TEARDOWN */
  if(method == GST_RTSP_SET_PARAMETER && g_strrstr((gchar*)data,"TEARDOWN")) {
    GstBus *bus;

    GST_DEBUG_OBJECT (src, "trigger TEARDOWN \n");

    bus = gst_element_get_bus(GST_ELEMENT_CAST(src));
    if(!gst_bus_post(bus, gst_message_new_application(GST_OBJECT_CAST(src), gst_structure_empty_new("TEARDOWN")))){
      GST_ERROR_OBJECT(src, "Failed to send TEARDOWN message\n");
    }
    gst_object_unref(bus);

    goto exit;
  }

  /* Handling wfd_trigger_method: setup */
  if(method == GST_RTSP_SET_PARAMETER && g_strrstr((gchar*)data,"SETUP")) {
    src->state = GST_RTSP_STATE_INIT;

    /* setup streams */
    if (!gst_wfdrtspsrc_setup (src))
      goto setup_failed;

    src->state = GST_RTSP_STATE_READY;
  }

exit:
  gst_rtsp_message_unset (request);
  gst_rtsp_message_unset (&response);

  return GST_RTSP_OK;

setup_failed:
  return GST_RTSP_ERROR;

  /* ERRORS */
send_error:
  {
    gst_rtsp_message_unset (request);
    gst_rtsp_message_unset (&response);
    return res;
  }

}

/* send server keep-alive */
static GstRTSPResult
gst_wfdrtspsrc_send_keep_alive (GstWFDRTSPSrc * src)
{
  GstRTSPMessage request = { 0 };
  GstRTSPResult res;
  GstRTSPMethod method;
  gchar *control;

  GST_INFO_OBJECT (src, "creating server keep-alive");

  /* find a method to use for keep-alive */
  if (src->methods & GST_RTSP_GET_PARAMETER)
    method = GST_RTSP_GET_PARAMETER;
  else
    method = GST_RTSP_OPTIONS;

  if (src->control)
    control = src->control;
  else
    control = src->conninfo.url_str;

  if (control == NULL)
    goto no_control;

  res = gst_rtsp_message_init_request (&request, method, control);
  if (res < 0)
    goto send_error;

  if (src->debug && request.body != NULL)
    gst_rtsp_message_dump (&request);

  res =
      gst_wfdrtspsrc_connection_send (src, src->conninfo.connection, &request,
      NULL);
  if (res < 0)
    goto send_error;

  gst_rtsp_connection_reset_timeout (src->conninfo.connection);
  gst_rtsp_message_unset (&request);

  return GST_RTSP_OK;

  /* ERRORS */
no_control:
  {
    GST_WARNING_OBJECT (src, "no control url to send keepalive");
    return GST_RTSP_OK;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    gst_rtsp_message_unset (&request);
    GST_ELEMENT_WARNING (src, RESOURCE, WRITE, (NULL),
        ("Could not send keep-alive. (%s)", str));
    g_free (str);
    return res;
  }
}



static GstFlowReturn
gst_wfdrtspsrc_loop_udp (GstWFDRTSPSrc * src)
{
  GstRTSPResult res;
  GstRTSPMessage message = { 0 };
  gint retry = 0;

  while (TRUE) {
    GTimeVal tv_timeout;

    /* get the next timeout interval */
    gst_rtsp_connection_next_timeout (src->conninfo.connection, &tv_timeout);

    GST_DEBUG_OBJECT (src, "doing receive with timeout %d seconds",
        (gint) tv_timeout.tv_sec);

    gst_rtsp_message_unset (&message);

    /* we should continue reading the TCP socket because the server might
     * send us requests. When the session timeout expires, we need to send a
     * keep-alive request to keep the session open. */
    res = gst_wfdrtspsrc_connection_receive (src, src->conninfo.connection,
        &message, &tv_timeout);

    switch (res) {
      case GST_RTSP_OK:
        GST_DEBUG_OBJECT (src, "we received a server message");
        break;
      case GST_RTSP_EINTR:
        /* we got interrupted, see what we have to do */
        goto interrupt;
      case GST_RTSP_ETIMEOUT:
        /* send keep-alive, ignore the result, a warning will be posted. */
        GST_DEBUG_OBJECT (src, "timeout, sending keep-alive");
        if ((res = gst_wfdrtspsrc_send_keep_alive (src)) == GST_RTSP_EINTR)
          goto interrupt;
	break;
      case GST_RTSP_EEOF:
        /* server closed the connection. not very fatal for UDP, reconnect and
         * see what happens. */
        GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
            ("The server closed the connection."));
        goto connect_error;
      default:
        goto receive_error;
    }

    switch (message.type) {
      case GST_RTSP_MESSAGE_REQUEST:
        /* server sends us a request message, handle it */
        res =
            gst_wfdrtspsrc_handle_request (src, src->conninfo.connection,
            &message);
        if (res == GST_RTSP_EEOF)
          goto server_eof;
        else if (res < 0)
          goto handle_request_failed;
        break;
      case GST_RTSP_MESSAGE_RESPONSE:
        /* we ignore response and data messages */
        GST_DEBUG_OBJECT (src, "ignoring response message");
        if (src->debug)
          gst_rtsp_message_dump (&message);
        if (message.type_data.response.code == GST_RTSP_STS_UNAUTHORIZED) {
          GST_DEBUG_OBJECT (src, "but is Unauthorized response ...");
          if (gst_wfdrtspsrc_setup_auth (src, &message) && !(retry++)) {
            GST_DEBUG_OBJECT (src, "so retrying keep-alive");
            if ((res = gst_wfdrtspsrc_send_keep_alive (src)) == GST_RTSP_EINTR)
              goto interrupt;
          }
        } else {
          retry = 0;
        }
        break;
      case GST_RTSP_MESSAGE_DATA:
        /* we ignore response and data messages */
        GST_DEBUG_OBJECT (src, "ignoring data message");
        break;
      default:
        GST_WARNING_OBJECT (src, "ignoring unknown message type %d",
            message.type);
        break;
    }
  }

  /* we get here when the connection got interrupted */
interrupt:
  {
    gst_rtsp_message_unset (&message);
    GST_DEBUG_OBJECT (src, "got interrupted: stop connection flush");
    gst_wfdrtspsrc_connection_flush (src, FALSE);
    return GST_FLOW_WRONG_STATE;
  }
connect_error:
  {
    gchar *str = gst_rtsp_strresult (res);
    GstFlowReturn ret;

    src->conninfo.connected = FALSE;
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ_WRITE, (NULL),
        ("Could not connect to server. (%s)", str));
    g_free (str);
    ret = GST_FLOW_ERROR;

    return ret;
  }
receive_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("Could not receive message. (%s)", str));
    g_free (str);
    return GST_FLOW_ERROR;
  }
handle_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);
    GstFlowReturn ret;

    gst_rtsp_message_unset (&message);
    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
          ("Could not handle server message. (%s)", str));
      g_free (str);
      ret = GST_FLOW_ERROR;
    } else {
      ret = GST_FLOW_WRONG_STATE;
    }
    return ret;
  }
server_eof:
  {
    GST_DEBUG_OBJECT (src, "we got an eof from the server");
    GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
        ("The server closed the connection."));
    src->conninfo.connected = FALSE;
    gst_rtsp_message_unset (&message);
    return GST_FLOW_UNEXPECTED;
  }
}

static void
gst_wfdrtspsrc_loop_start_cmd (GstWFDRTSPSrc * src, gint cmd)
{
  GST_DEBUG_OBJECT (src, "start cmd %d", cmd);

  switch (cmd) {
    case WFD_CMD_OPEN:
      GST_ELEMENT_PROGRESS (src, START, "open", ("Opening Stream"));
      break;
    case WFD_CMD_PLAY:
      GST_ELEMENT_PROGRESS (src, START, "request", ("Sending PLAY request"));
      break;
    case WFD_CMD_PAUSE:
      GST_ELEMENT_PROGRESS (src, START, "request", ("Sending PAUSE request"));
      break;
    case WFD_CMD_CLOSE:
      GST_ELEMENT_PROGRESS (src, START, "close", ("Closing Stream"));
      break;
    default:
      break;
  }
}

static void
gst_wfdrtspsrc_loop_complete_cmd (GstWFDRTSPSrc * src, gint cmd)
{
  GST_DEBUG_OBJECT (src, "complete cmd %d", cmd);

  switch (cmd) {
    case WFD_CMD_OPEN:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "open", ("Opened Stream"));
      break;
    case WFD_CMD_PLAY:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "request", ("Sent PLAY request"));
      break;
    case WFD_CMD_PAUSE:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "request", ("Sent PAUSE request"));
      break;
    case WFD_CMD_CLOSE:
      GST_ELEMENT_PROGRESS (src, COMPLETE, "close", ("Closed Stream"));
      break;
    default:
      break;
  }
}

static void
gst_wfdrtspsrc_loop_cancel_cmd (GstWFDRTSPSrc * src, gint cmd)
{
  GST_DEBUG_OBJECT (src, "cancel cmd %d", cmd);

  switch (cmd) {
    case WFD_CMD_OPEN:
      GST_ELEMENT_PROGRESS (src, CANCELED, "open", ("Open canceled"));
      break;
    case WFD_CMD_PLAY:
      GST_ELEMENT_PROGRESS (src, CANCELED, "request", ("PLAY canceled"));
      break;
    case WFD_CMD_PAUSE:
      GST_ELEMENT_PROGRESS (src, CANCELED, "request", ("PAUSE canceled"));
      break;
    case WFD_CMD_CLOSE:
      GST_ELEMENT_PROGRESS (src, CANCELED, "close", ("Close canceled"));
      break;
    default:
      break;
  }
}

static void
gst_wfdrtspsrc_loop_error_cmd (GstWFDRTSPSrc * src, gint cmd)
{
  GST_DEBUG_OBJECT (src, "error cmd %d", cmd);

  switch (cmd) {
    case WFD_CMD_OPEN:
      GST_ELEMENT_PROGRESS (src, ERROR, "open", ("Open failed"));
      break;
    case WFD_CMD_PLAY:
      GST_ELEMENT_PROGRESS (src, ERROR, "request", ("PLAY failed"));
      break;
    case WFD_CMD_PAUSE:
      GST_ELEMENT_PROGRESS (src, ERROR, "request", ("PAUSE failed"));
      break;
    case WFD_CMD_CLOSE:
      GST_ELEMENT_PROGRESS (src, ERROR, "close", ("Close failed"));
      break;
    default:
      break;
  }
}

static void
gst_wfdrtspsrc_loop_end_cmd (GstWFDRTSPSrc * src, gint cmd, GstRTSPResult ret)
{
  if (ret == GST_RTSP_OK)
    gst_wfdrtspsrc_loop_complete_cmd (src, cmd);
  else if (ret == GST_RTSP_EINTR)
    gst_wfdrtspsrc_loop_cancel_cmd (src, cmd);
  else
    gst_wfdrtspsrc_loop_error_cmd (src, cmd);
}

static void
gst_wfdrtspsrc_loop_send_cmd (GstWFDRTSPSrc * src, gint cmd, gboolean flush)
{
  gint old;

  /* FIXME flush param mute; remove at discretion */

  /* start new request */
  gst_wfdrtspsrc_loop_start_cmd (src, cmd);

  GST_DEBUG_OBJECT (src, "sending cmd %d", cmd);

  GST_OBJECT_LOCK (src);
  old = src->loop_cmd;
  if (old != WFD_CMD_WAIT) {
    src->loop_cmd = WFD_CMD_WAIT;
    GST_OBJECT_UNLOCK (src);
    /* cancel previous request */
    gst_wfdrtspsrc_loop_cancel_cmd (src, old);
    GST_OBJECT_LOCK (src);
  }
  src->loop_cmd = cmd;
  /* interrupt if allowed */
  if (src->waiting && flush) {
    GST_DEBUG_OBJECT (src, "start connection flush");
    gst_wfdrtspsrc_connection_flush (src, TRUE);
  }
  if (src->task)
    gst_task_start (src->task);

  GST_OBJECT_UNLOCK (src);
}

static gboolean
gst_wfdrtspsrc_loop (GstWFDRTSPSrc * src)
{
  GstFlowReturn ret;

  if (!src->conninfo.connection || !src->conninfo.connected)
    goto no_connection;

  ret = gst_wfdrtspsrc_loop_udp (src);

  if (ret != GST_FLOW_OK)
    goto pause;

  return TRUE;

  /* ERRORS */
no_connection:
  {
    GST_WARNING_OBJECT (src, "we are not connected");
    ret = GST_FLOW_WRONG_STATE;
    goto pause;
  }
pause:
  {
    const gchar *reason = gst_flow_get_name (ret);

    GST_DEBUG_OBJECT (src, "pausing task, reason %s", reason);
    src->running = FALSE;
    if (ret == GST_FLOW_UNEXPECTED) {
      /* perform EOS logic */
      if (src->segment.flags & GST_SEEK_FLAG_SEGMENT) {
        gst_element_post_message (GST_ELEMENT_CAST (src),
            gst_message_new_segment_done (GST_OBJECT_CAST (src),
                src->segment.format, src->segment.last_stop));
      } else {
        gst_wfdrtspsrc_push_event (src, gst_event_new_eos (), FALSE);
      }
    } else if (ret == GST_FLOW_NOT_LINKED || ret < GST_FLOW_UNEXPECTED) {
      /* for fatal errors we post an error message, post the error before the
       * EOS so the app knows about the error first. */
      GST_ELEMENT_ERROR (src, STREAM, FAILED,
          ("Internal data flow error."),
          ("streaming task paused, reason %s (%d)", reason, ret));
      gst_wfdrtspsrc_push_event (src, gst_event_new_eos (), FALSE);
    }
    return FALSE;
  }
}

#ifndef GST_DISABLE_GST_DEBUG
static const gchar *
gst_rtsp_auth_method_to_string (GstRTSPAuthMethod method)
{
  gint index = 0;

  while (method != 0) {
    index++;
    method >>= 1;
  }
  switch (index) {
    case 0:
      return "None";
    case 1:
      return "Basic";
    case 2:
      return "Digest";
  }

  return "Unknown";
}
#endif

static const gchar *
gst_wfdrtspsrc_skip_lws (const gchar * s)
{
  while (g_ascii_isspace (*s))
    s++;
  return s;
}

static const gchar *
gst_wfdrtspsrc_unskip_lws (const gchar * s, const gchar * start)
{
  while (s > start && g_ascii_isspace (*(s - 1)))
    s--;
  return s;
}

static const gchar *
gst_wfdrtspsrc_skip_commas (const gchar * s)
{
  /* The grammar allows for multiple commas */
  while (g_ascii_isspace (*s) || *s == ',')
    s++;
  return s;
}

static const gchar *
gst_wfdrtspsrc_skip_item (const gchar * s)
{
  gboolean quoted = FALSE;
  const gchar *start = s;

  /* A list item ends at the last non-whitespace character
   * before a comma which is not inside a quoted-string. Or at
   * the end of the string.
   */
  while (*s) {
    if (*s == '"')
      quoted = !quoted;
    else if (quoted) {
      if (*s == '\\' && *(s + 1))
        s++;
    } else {
      if (*s == ',')
        break;
    }
    s++;
  }

  return gst_wfdrtspsrc_unskip_lws (s, start);
}

static void
gst_rtsp_decode_quoted_string (gchar * quoted_string)
{
  gchar *src, *dst;

  src = quoted_string + 1;
  dst = quoted_string;
  while (*src && *src != '"') {
    if (*src == '\\' && *(src + 1))
      src++;
    *dst++ = *src++;
  }
  *dst = '\0';
}

/* Extract the authentication tokens that the server provided for each method
 * into an array of structures and give those to the connection object.
 */
static void
gst_wfdrtspsrc_parse_digest_challenge (GstRTSPConnection * conn,
    const gchar * header, gboolean * stale)
{
  GSList *list = NULL, *iter;
  const gchar *end;
  gchar *item, *eq, *name_end, *value;

  g_return_if_fail (stale != NULL);

  gst_rtsp_connection_clear_auth_params (conn);
  *stale = FALSE;

  /* Parse a header whose content is described by RFC2616 as
   * "#something", where "something" does not itself contain commas,
   * except as part of quoted-strings, into a list of allocated strings.
   */
  header = gst_wfdrtspsrc_skip_commas (header);
  while (*header) {
    end = gst_wfdrtspsrc_skip_item (header);
    list = g_slist_prepend (list, g_strndup (header, end - header));
    header = gst_wfdrtspsrc_skip_commas (end);
  }
  if (!list)
    return;

  list = g_slist_reverse (list);
  for (iter = list; iter; iter = iter->next) {
    item = iter->data;

    if (item == NULL) continue;

    eq = strchr (item, '=');
    if (eq) {
      name_end = (gchar *) gst_wfdrtspsrc_unskip_lws (eq, item);
      if (name_end == item) {
        /* That's no good... */
        g_free (item);
        item = NULL;
        continue;
      }

      *name_end = '\0';

      value = (gchar *) gst_wfdrtspsrc_skip_lws (eq + 1);
      if (*value == '"')
        gst_rtsp_decode_quoted_string (value);
    } else
        value = NULL;

    if(value && item)
      if ((strcmp (item, "stale") == 0) && (strcmp (value, "TRUE") == 0))
        *stale = TRUE;
    gst_rtsp_connection_set_auth_param (conn, item, value);
    g_free (item);
  }

  g_slist_free (list);
}

/* Parse a WWW-Authenticate Response header and determine the
 * available authentication methods
 *
 * This code should also cope with the fact that each WWW-Authenticate
 * header can contain multiple challenge methods + tokens
 *
 * At the moment, for Basic auth, we just do a minimal check and don't
 * even parse out the realm */
static void
gst_wfdrtspsrc_parse_auth_hdr (gchar * hdr, GstRTSPAuthMethod * methods,
    GstRTSPConnection * conn, gboolean * stale)
{
  gchar *start;

  g_return_if_fail (hdr != NULL);
  g_return_if_fail (methods != NULL);
  g_return_if_fail (stale != NULL);

  /* Skip whitespace at the start of the string */
  for (start = hdr; start[0] != '\0' && g_ascii_isspace (start[0]); start++);

  if (g_ascii_strncasecmp (start, "basic", 5) == 0)
    *methods |= GST_RTSP_AUTH_BASIC;
  else if (g_ascii_strncasecmp (start, "digest ", 7) == 0) {
    *methods |= GST_RTSP_AUTH_DIGEST;
    gst_wfdrtspsrc_parse_digest_challenge (conn, &start[7], stale);
  }
}

/**
 * gst_wfdrtspsrc_setup_auth:
 * @src: the rtsp source
 *
 * Configure a username and password and auth method on the
 * connection object based on a response we received from the
 * peer.
 *
 * Currently, this requires that a username and password were supplied
 * in the uri. In the future, they may be requested on demand by sending
 * a message up the bus.
 *
 * Returns: TRUE if authentication information could be set up correctly.
 */
static gboolean
gst_wfdrtspsrc_setup_auth (GstWFDRTSPSrc * src, GstRTSPMessage * response)
{
  gchar *user = NULL;
  gchar *pass = NULL;
  GstRTSPAuthMethod avail_methods = GST_RTSP_AUTH_NONE;
  GstRTSPAuthMethod method;
  GstRTSPResult auth_result;
  GstRTSPUrl *url;
  GstRTSPConnection *conn;
  gchar *hdr;
  gboolean stale = FALSE;

  conn = src->conninfo.connection;

  /* Identify the available auth methods and see if any are supported */
  if (gst_rtsp_message_get_header (response, GST_RTSP_HDR_WWW_AUTHENTICATE,
          &hdr, 0) == GST_RTSP_OK) {
    gst_wfdrtspsrc_parse_auth_hdr (hdr, &avail_methods, conn, &stale);
  }

  if (avail_methods == GST_RTSP_AUTH_NONE)
    goto no_auth_available;

  /* For digest auth, if the response indicates that the session
   * data are stale, we just update them in the connection object and
   * return TRUE to retry the request */
  if (stale)
    src->tried_url_auth = FALSE;

  url = gst_rtsp_connection_get_url (conn);

  /* Do we have username and password available? */
  if (url != NULL && !src->tried_url_auth && url->user != NULL
      && url->passwd != NULL) {
    user = url->user;
    pass = url->passwd;
    src->tried_url_auth = TRUE;
    GST_DEBUG_OBJECT (src,
        "Attempting authentication using credentials from the URL");
  } else {
    user = src->user_id;
    pass = src->user_pw;
    GST_DEBUG_OBJECT (src,
        "Attempting authentication using credentials from the properties");
  }

  /* FIXME: If the url didn't contain username and password or we tried them
   * already, request a username and passwd from the application via some kind
   * of credentials request message */

  /* If we don't have a username and passwd at this point, bail out. */
  if (user == NULL || pass == NULL)
    goto no_user_pass;

  /* Try to configure for each available authentication method, strongest to
   * weakest */
  for (method = GST_RTSP_AUTH_MAX; method != GST_RTSP_AUTH_NONE; method >>= 1) {
    /* Check if this method is available on the server */
    if ((method & avail_methods) == 0)
      continue;

    /* Pass the credentials to the connection to try on the next request */
    auth_result = gst_rtsp_connection_set_auth (conn, method, user, pass);
    /* INVAL indicates an invalid username/passwd were supplied, so we'll just
     * ignore it and end up retrying later */
    if (auth_result == GST_RTSP_OK || auth_result == GST_RTSP_EINVAL) {
      GST_DEBUG_OBJECT (src, "Attempting %s authentication",
          gst_rtsp_auth_method_to_string (method));
      break;
    }
  }

  if (method == GST_RTSP_AUTH_NONE)
    goto no_auth_available;

  return TRUE;

no_auth_available:
  {
    /* Output an error indicating that we couldn't connect because there were
     * no supported authentication protocols */
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
        ("No supported authentication protocol was found"));
    return FALSE;
  }
no_user_pass:
  {
    /* We don't fire an error message, we just return FALSE and let the
     * normal NOT_AUTHORIZED error be propagated */
    return FALSE;
  }
}

static GstRTSPResult
gst_wfdrtspsrc_try_send (GstWFDRTSPSrc * src, GstRTSPConnection * conn,
    GstRTSPMessage * request, GstRTSPMessage * response,
    GstRTSPStatusCode * code)
{
  GstRTSPResult res;
  GstRTSPStatusCode thecode;
  gchar *content_base = NULL;
  gint try = 0;

again:
  gst_wfd_rtsp_ext_list_before_send (src->extensions, request);

  GST_DEBUG_OBJECT (src, "sending message");

  if (src->debug)
    gst_rtsp_message_dump (request);

  res = gst_wfdrtspsrc_connection_send (src, conn, request, src->ptcp_timeout);
  if (res < 0)
    goto send_error;

  gst_rtsp_connection_reset_timeout (conn);

next:
  res = gst_wfdrtspsrc_connection_receive (src, conn, response, src->ptcp_timeout);
  if (res < 0)
    goto receive_error;

  if (src->debug)
    gst_rtsp_message_dump (response);

  switch (response->type) {
    case GST_RTSP_MESSAGE_REQUEST:
      res = gst_wfdrtspsrc_handle_request (src, conn, response);
      if (res == GST_RTSP_EEOF)
        goto server_eof;
      else if (res < 0)
        goto handle_request_failed;
      goto next;
    case GST_RTSP_MESSAGE_RESPONSE:
      /* ok, a response is good */
      GST_DEBUG_OBJECT (src, "received response message");
      break;
    default:
    case GST_RTSP_MESSAGE_DATA:
      /* get next response */
      GST_DEBUG_OBJECT (src, "ignoring data response message");
      goto next;
  }

  thecode = response->type_data.response.code;

  GST_DEBUG_OBJECT (src, "got response message %d", thecode);

  /* if the caller wanted the result code, we store it. */
  if (code)
    *code = thecode;

  /* If the request didn't succeed, bail out before doing any more */
  if (thecode != GST_RTSP_STS_OK)
    return GST_RTSP_OK;

  /* store new content base if any */
  gst_rtsp_message_get_header (response, GST_RTSP_HDR_CONTENT_BASE,
      &content_base, 0);
  if (content_base) {
    g_free (src->content_base);
    src->content_base = g_strdup (content_base);
  }
  gst_wfd_rtsp_ext_list_after_send (src->extensions, request, response);

  return GST_RTSP_OK;

  /* ERRORS */
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
        ("Could not send message. (%s)", str));
    g_free (str);
    return res;
  }
receive_error:
  {

    switch (res) {
      case GST_RTSP_EEOF:
        GST_WARNING_OBJECT (src, "server closed connection, doing reconnect");
        if (try == 0) {
          try++;
          /* if reconnect succeeds, try again */
          if ((res = gst_wfdrtspsrc_conninfo_reconnect (src, &src->conninfo)) == 0)
            goto again;
        }
        /* only try once after reconnect, then fallthrough and error out */
      default:
      {
        gchar *str = gst_rtsp_strresult (res);

        GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
            ("Could not receive message. (%s)", str));
        g_free (str);
        break;
      }
    }
    return res;
  }
handle_request_failed:
  {
    /* ERROR was posted */
    gst_rtsp_message_unset (response);
    return res;
  }
server_eof:
  {
    GST_DEBUG_OBJECT (src, "we got an eof from the server");
    GST_ELEMENT_WARNING (src, RESOURCE, READ, (NULL),
        ("The server closed the connection."));
    gst_rtsp_message_unset (response);
    return res;
  }
}

/**
 * gst_wfdrtspsrc_send:
 * @src: the rtsp source
 * @conn: the connection to send on
 * @request: must point to a valid request
 * @response: must point to an empty #GstRTSPMessage
 * @code: an optional code result
 *
 * send @request and retrieve the response in @response. optionally @code can be
 * non-NULL in which case it will contain the status code of the response.
 *
 * If This function returns #GST_RTSP_OK, @response will contain a valid response
 * message that should be cleaned with gst_rtsp_message_unset() after usage.
 *
 * If @code is NULL, this function will return #GST_RTSP_ERROR (with an invalid
 * @response message) if the response code was not 200 (OK).
 *
 * If the attempt results in an authentication failure, then this will attempt
 * to retrieve authentication credentials via gst_wfdrtspsrc_setup_auth and retry
 * the request.
 *
 * Returns: #GST_RTSP_OK if the processing was successful.
 */

static GstRTSPResult
gst_wfdrtspsrc_send (GstWFDRTSPSrc * src, GstRTSPConnection * conn,
    GstRTSPMessage * request, GstRTSPMessage * response,
    GstRTSPStatusCode * code)
{
  GstRTSPStatusCode int_code = GST_RTSP_STS_OK;
  GstRTSPResult res = GST_RTSP_ERROR;
  gint count;
  gboolean retry;
  GstRTSPMethod method = GST_RTSP_INVALID;

  count = 0;
  do {
    retry = FALSE;

    /* make sure we don't loop forever */
    if (count++ > 8)
      break;

    /* save method so we can disable it when the server complains */
    method = request->type_data.request.method;

    if ((res =
            gst_wfdrtspsrc_try_send (src, conn, request, response, &int_code)) < 0)
      goto error;

    switch (int_code) {
      case GST_RTSP_STS_UNAUTHORIZED:
        if (gst_wfdrtspsrc_setup_auth (src, response)) {
          /* Try the request/response again after configuring the auth info
           * and loop again */
          retry = TRUE;
        }
        break;
      default:
        break;
    }
  } while (retry == TRUE);

  /* If the user requested the code, let them handle errors, otherwise
   * post an error below */
  if (code != NULL)
    *code = int_code;
  else if (int_code != GST_RTSP_STS_OK)
    goto error_response;

  return res;

  /* ERRORS */
error:
  {
    GST_DEBUG_OBJECT (src, "got error %d", res);
    return res;
  }
error_response:
  {
    res = GST_RTSP_ERROR;

    switch (response->type_data.response.code) {
      case GST_RTSP_STS_NOT_FOUND:
        GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (NULL), ("%s",
                response->type_data.response.reason));
        break;
      case GST_RTSP_STS_MOVED_PERMANENTLY:
      case GST_RTSP_STS_MOVE_TEMPORARILY:
      {
        gchar *new_location;
        GstRTSPLowerTrans transports;

        GST_DEBUG_OBJECT (src, "got redirection");
        /* if we don't have a Location Header, we must error */
        if (gst_rtsp_message_get_header (response, GST_RTSP_HDR_LOCATION,
                &new_location, 0) < 0)
          break;

        /* When we receive a redirect result, we go back to the INIT state after
         * parsing the new URI. The caller should do the needed steps to issue
         * a new setup when it detects this state change. */
        GST_DEBUG_OBJECT (src, "redirection to %s", new_location);

        /* save current transports */
        if (src->conninfo.url)
          transports = src->conninfo.url->transports;
        else
          transports = GST_RTSP_LOWER_TRANS_UNKNOWN;

        gst_wfdrtspsrc_uri_set_uri (GST_URI_HANDLER (src), new_location);

        /* set old transports */
        if (src->conninfo.url && transports != GST_RTSP_LOWER_TRANS_UNKNOWN)
          src->conninfo.url->transports = transports;

        src->need_redirect = TRUE;
        src->state = GST_RTSP_STATE_INIT;
        res = GST_RTSP_OK;
        break;
      }
      case GST_RTSP_STS_NOT_ACCEPTABLE:
      case GST_RTSP_STS_NOT_IMPLEMENTED:
      case GST_RTSP_STS_METHOD_NOT_ALLOWED:
        GST_WARNING_OBJECT (src, "got NOT IMPLEMENTED, disable method %s",
            gst_rtsp_method_as_text (method));
        src->methods &= ~method;
        res = GST_RTSP_OK;
        break;
      default:
        GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
            ("Got error response: %d (%s).", response->type_data.response.code,
                response->type_data.response.reason));
        break;
    }
    /* if we return ERROR we should unset the response ourselves */
    if (res == GST_RTSP_ERROR)
      gst_rtsp_message_unset (response);

    return res;
  }
}

static GstRTSPResult
gst_wfdrtspsrc_send_cb (GstRTSPExtension * ext, GstRTSPMessage * request,
    GstRTSPMessage * response, GstWFDRTSPSrc * src)
{
  return gst_wfdrtspsrc_send (src, src->conninfo.connection, request, response,
      NULL);
}


/* parse the response and collect all the supported methods. We need this
 * information so that we don't try to send an unsupported request to the
 * server.
 */
static gboolean
gst_wfdrtspsrc_parse_methods (GstWFDRTSPSrc * src, GstRTSPMessage * response)
{
  GstRTSPHeaderField field;
  gchar *respoptions;
  gchar **options;
  gint indx = 0;
  gint i;

  /* reset supported methods */
  src->methods = 0;

  /* Try Allow Header first */
  field = GST_RTSP_HDR_ALLOW;
  while (TRUE) {
    respoptions = NULL;
    gst_rtsp_message_get_header (response, field, &respoptions, indx);
    if (indx == 0 && !respoptions) {
      /* if no Allow header was found then try the Public header... */
      field = GST_RTSP_HDR_PUBLIC;
      gst_rtsp_message_get_header (response, field, &respoptions, indx);
    }
    if (!respoptions)
      break;

    /* If we get here, the server gave a list of supported methods, parse
     * them here. The string is like:
     *
     * OPTIONS, DESCRIBE, ANNOUNCE, PLAY, SETUP, ...
     */
    options = g_strsplit (respoptions, ",", 0);

    for (i = 0; options[i]; i++) {
      gchar *stripped;
      gint method;

      stripped = g_strstrip (options[i]);
      method = gst_rtsp_find_method (stripped);

      /* keep bitfield of supported methods */
      if (method != GST_RTSP_INVALID)
        src->methods |= method;
    }
    g_strfreev (options);

    indx++;
  }

  if (src->methods == 0) {
    /* neither Allow nor Public are required, assume the server supports
     * at least DESCRIBE, SETUP, we always assume it supports PLAY as
     * well. */
    GST_DEBUG_OBJECT (src, "could not get OPTIONS");
    src->methods = GST_RTSP_DESCRIBE | GST_RTSP_SETUP;
  }
  /* always assume PLAY, FIXME, extensions should be able to override
   * this */
  src->methods |= GST_RTSP_PLAY;

  if (!(src->methods & GST_RTSP_SETUP))
    goto no_setup;

  return TRUE;

no_setup:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ, (NULL),
        ("Server does not support SETUP."));
    return FALSE;
  }
}

static GstRTSPResult
gst_wfdrtspsrc_create_transports_string (GstWFDRTSPSrc * src,
    GstRTSPLowerTrans protocols, gchar ** transports)
{
  GstRTSPResult res;
  GString *result;

  *transports = NULL;

  res =
      gst_wfd_rtsp_ext_list_get_transports (src->extensions, protocols, transports);

  if (res < 0)
    goto failed;

  GST_DEBUG_OBJECT (src, "got transports %s", GST_STR_NULL (*transports));

  /* extension listed transports, use those */
  if (*transports != NULL)
    return GST_RTSP_OK;

  /* the default RTSP transports */
  result = g_string_new ("");
  if (protocols & GST_RTSP_LOWER_TRANS_UDP) {
    GST_DEBUG_OBJECT (src, "adding UDP unicast");

    g_string_append (result, "RTP/AVP");
    /*if (add_udp_str)
      g_string_append (result, "/UDP");*/
    g_string_append (result, ";unicast;client_port=%%u1-%%u2");
  } else if (protocols & GST_RTSP_LOWER_TRANS_UDP_MCAST) {
    GST_DEBUG_OBJECT (src, "adding UDP multicast");

    /* we don't have to allocate any UDP ports yet, if the selected transport
     * turns out to be multicast we can create them and join the multicast
     * group indicated in the transport reply */
    if (result->len > 0)
      g_string_append (result, ",");
    g_string_append (result, "RTP/AVP");
    /*if (add_udp_str)
      g_string_append (result, "/UDP");*/
    g_string_append (result, ";multicast");
  } else if (protocols & GST_RTSP_LOWER_TRANS_TCP) {
    GST_DEBUG_OBJECT (src, "adding TCP");

    if (result->len > 0)
      g_string_append (result, ",");
    g_string_append (result, "RTP/AVP/TCP;unicast;interleaved=%%i1-%%i2");
  }
  *transports = g_string_free (result, FALSE);

  GST_DEBUG_OBJECT (src, "prepared transports %s", GST_STR_NULL (*transports));

  return GST_RTSP_OK;

  /* ERRORS */
failed:
  {
    return res;
  }
}

static GstRTSPResult
gst_wfdrtspsrc_prepare_transports (WFDRTSPStream * stream, gchar ** transports,
    gint orig_rtpport, gint orig_rtcpport)
{
  GstWFDRTSPSrc *src;
  gint nr_udp, nr_int;
  gchar *next, *p;
  gint rtpport = 0, rtcpport = 0;
  GString *str;

  src = GST_WFDRTSPSRC(stream->parent);

  /* find number of placeholders first */
  if (strstr (*transports, "%%i2"))
    nr_int = 2;
  else if (strstr (*transports, "%%i1"))
    nr_int = 1;
  else
    nr_int = 0;

  if (strstr (*transports, "%%u2"))
    nr_udp = 2;
  else if (strstr (*transports, "%%u1"))
    nr_udp = 1;
  else
    nr_udp = 0;

  if (nr_udp == 0 && nr_int == 0)
    goto done;

  if (nr_udp > 0) {
    if (!orig_rtpport || !orig_rtcpport) {
      if (!gst_wfdrtspsrc_alloc_udp_ports (stream, &rtpport, &rtcpport))
        goto failed;
    } else {
      gst_wfdrtspsrc_alloc_udp_ports (stream, &orig_rtpport, &orig_rtcpport);
      rtpport = orig_rtpport;
      rtcpport = orig_rtcpport;
    }
  }

  str = g_string_new ("");
  p = *transports;
  while ((next = strstr (p, "%%"))) {
    g_string_append_len (str, p, next - p);
    if (next[2] == 'u') {
      if (next[3] == '1')
        g_string_append_printf (str, "%d", rtpport);
      else if (next[3] == '2')
        g_string_append_printf (str, "%d", rtcpport);
    }
    if (next[2] == 'i') {
      if (next[3] == '1')
        g_string_append_printf (str, "%d", src->free_channel);
      else if (next[3] == '2')
        g_string_append_printf (str, "%d", src->free_channel + 1);
    }

    p = next + 4;
  }
  /* append final part */
  g_string_append (str, p);

  g_free (*transports);
  *transports = g_string_free (str, FALSE);

done:
  return GST_RTSP_OK;

  /* ERRORS */
failed:
  {
    return GST_RTSP_ERROR;
  }
}

/* Perform the SETUP request for all the streams.
 *
 * We ask the server for a specific transport, which initially includes all the
 * ones we can support (UDP/TCP/MULTICAST). For the UDP transport we allocate
 * two local UDP ports that we send to the server.
 *
 * Once the server replied with a transport, we configure the other streams
 * with the same transport.
 *
 * This function will also configure the stream for the selected transport,
 * which basically means creating the pipeline.
 */
static gboolean
gst_wfdrtspsrc_setup (GstWFDRTSPSrc * src)
{
  GstRTSPResult res;
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  WFDRTSPStream *stream = NULL;
  GstRTSPLowerTrans protocols = GST_RTSP_LOWER_TRANS_UNKNOWN;
  GstRTSPStatusCode code;
  gint rtpport, rtcpport;
  GstRTSPUrl *url;
  gchar *hval;
  GstRTSPConnection *conn;
  gchar *transports;
  gint retry = 0;

  if (src->conninfo.connection) {
    url = gst_rtsp_connection_get_url (src->conninfo.connection);
    /* we initially allow all configured lower transports. based on the URL
     * transports and the replies from the server we narrow them down. */
    protocols = url->transports & src->protocols;
  } else {
    url = NULL;
    protocols = src->protocols;
  }

  if (protocols == 0)
    goto no_protocols;

  /* reset some state */
  src->free_channel = 0;
  src->need_activate = FALSE;
  /* keep track of next port number, 0 is random */
  src->next_port_num = src->client_port_range.min;
  rtpport = rtcpport = 0;

  stream = src->stream;

  stream->conninfo.location = src->conninfo.location;
  GST_DEBUG_OBJECT (src, " setup: %s",
    GST_STR_NULL (stream->conninfo.location));

#ifdef ENABLE_WFD_EXTENDED_FEATURES
  stream->control_url = g_strdup (src->conninfo.url_str);
  GST_DEBUG_OBJECT (src, " setup: %s",
    GST_STR_NULL (stream->control_url));

  stream->control_connection = src->conninfo.connection;
  GST_DEBUG_OBJECT (src, " setup: %p", stream->control_connection);
#endif

  /* see if we need to configure this stream */
  if (!gst_wfd_rtsp_ext_list_configure_stream (src->extensions, stream->caps)) {
    GST_DEBUG_OBJECT (src, "skipping stream %p, disabled by extension",
        stream);
    stream->disabled = TRUE;
    goto skip;
  }

  /* skip setup if we have no URL for it */
  if (stream->conninfo.location == NULL) {
    GST_DEBUG_OBJECT (src, "skipping stream %p, no setup", stream);
    goto skip;
  }

  if (src->conninfo.connection == NULL) {
    if (!gst_wfdrtspsrc_conninfo_connect (src, &stream->conninfo)) {
      GST_DEBUG_OBJECT (src, "skipping stream %p, failed to connect", stream);
      goto skip;
    }
    conn = stream->conninfo.connection;
  } else {
    conn = src->conninfo.connection;
  }
  GST_DEBUG_OBJECT (src, "doing setup of stream %p with %s", stream,
      stream->conninfo.location);

  /* wfd use only UDP protocol for setup */
  if (!(protocols & GST_RTSP_LOWER_TRANS_UDP))
    goto no_protocols;

  GST_DEBUG_OBJECT (src, "protocols = 0x%x", protocols);
  /* create a string with first transport in line */
  transports = NULL;
  res = gst_wfdrtspsrc_create_transports_string (src,
      protocols & GST_RTSP_LOWER_TRANS_UDP, &transports);
  if (res < 0 || transports == NULL)
    goto setup_transport_failed;

  if (strlen (transports) == 0) {
    g_free (transports);
    GST_DEBUG_OBJECT (src, "no transports found");
    goto setup_transport_failed;
  }
  GST_DEBUG_OBJECT (src, "replace ports in %s", GST_STR_NULL (transports));

  /* replace placeholders with real values, this function will optionally
   * allocate UDP ports and other info needed to execute the setup request */
  res = gst_wfdrtspsrc_prepare_transports (stream, &transports, 0, 0);
  if (res < 0) {
    g_free (transports);
    goto setup_transport_failed;
  }

  GST_DEBUG_OBJECT (src, "transport is now %s", GST_STR_NULL (transports));

  /* create SETUP request */
  res =
      gst_rtsp_message_init_request (&request, GST_RTSP_SETUP,
      stream->conninfo.location);
  if (res < 0) {
    g_free (transports);
    goto create_request_failed;
  }

  gst_rtsp_message_add_header (&request, GST_RTSP_HDR_USER_AGENT, (const gchar *)src->user_agent);
  /* select transport, copy is made when adding to header so we can free it. */
  gst_rtsp_message_add_header (&request, GST_RTSP_HDR_TRANSPORT, transports);
  g_free (transports);

  /* if the user wants a non default RTP packet size we add the blocksize
   * parameter */
  if (src->rtp_blocksize > 0) {
    hval = g_strdup_printf ("%d", src->rtp_blocksize);
    gst_rtsp_message_add_header (&request, GST_RTSP_HDR_BLOCKSIZE, hval);
    g_free (hval);
  }

  /* handle the code ourselves */
  if ((res = gst_wfdrtspsrc_send (src, conn, &request, &response, &code) < 0))
    goto send_error;

  switch (code) {
    case GST_RTSP_STS_OK:
      break;
    case GST_RTSP_STS_UNSUPPORTED_TRANSPORT:
      gst_rtsp_message_unset (&request);
      gst_rtsp_message_unset (&response);
      /* cleanup of leftover transport */
      gst_wfdrtspsrc_stream_free_udp (stream);
      goto no_protocols;
      break;
    default:
      /* cleanup of leftover transport and move to the next stream */
      gst_wfdrtspsrc_stream_free_udp (stream);
      goto response_error;
  }

  /* parse response transport */
  {
    gchar *resptrans = NULL;
    GstRTSPTransport transport = { 0 };

    gst_rtsp_message_get_header (&response, GST_RTSP_HDR_TRANSPORT,
        &resptrans, 0);
    if (!resptrans) {
      gst_wfdrtspsrc_stream_free_udp (stream);
      goto no_transport;
    }

    /* parse transport, go to next stream on parse error */
    if (gst_rtsp_transport_parse (resptrans, &transport) != GST_RTSP_OK) {
      GST_WARNING_OBJECT (src, "failed to parse transport %s", resptrans);
      goto next;
    }

    /* update allowed transports for other streams. once the transport of
     * one stream has been determined, we make sure that all other streams
     * are configured in the same way */
    switch (transport.lower_transport) {
      case GST_RTSP_LOWER_TRANS_UDP:
        /* only allow unicast for other streams */
        GST_DEBUG_OBJECT (src, "stream %p as UDP unicast", stream);
        protocols = GST_RTSP_LOWER_TRANS_UDP;
        break;
      case GST_RTSP_LOWER_TRANS_TCP:
      case GST_RTSP_LOWER_TRANS_UDP_MCAST:
        GST_DEBUG_OBJECT (src, "stream %p transport %d is not supported", stream,
            transport.lower_transport);
	  break;
      default:
        GST_DEBUG_OBJECT (src, "stream %p unknown transport %d", stream,
            transport.lower_transport);
        break;
    }

    /* now configure the stream with the selected transport */
    if (!gst_wfdrtspsrc_configure_transport (stream, &transport)) {
      GST_DEBUG_OBJECT (src,
          "could not configure stream %p transport, skipping stream",
          stream);
      goto next;
    } else if (stream->udpsrc[0] && stream->udpsrc[1]) {
      /* retain the first allocated UDP port pair */
      g_object_get (G_OBJECT (stream->udpsrc[0]), "port", &rtpport, NULL);
      g_object_get (G_OBJECT (stream->udpsrc[1]), "port", &rtcpport, NULL);
    }
    /* we need to activate at least one streams when we detect activity */
    src->need_activate = TRUE;
  next:
    /* clean up our transport struct */
    gst_rtsp_transport_init (&transport);
    /* clean up used RTSP messages */
    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);
  }
skip:
  /* store the transport protocol that was configured */
  stream->protocol = protocols;

  gst_wfd_rtsp_ext_list_stream_select (src->extensions, url);

  /* if there is nothing to activate, error out */
  if (!src->need_activate)
    goto nothing_to_activate;

  return TRUE;

  /* ERRORS */
no_protocols:
  {
    /* no transport possible, post an error and stop */
    GST_ELEMENT_ERROR (src, RESOURCE, READ, (NULL),
        ("Could not connect to server, no protocols left"));
    return FALSE;
  }
create_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request. (%s)", str));
    g_free (str);
    goto cleanup_error;
  }
setup_transport_failed:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("Could not setup transport."));
    goto cleanup_error;
  }
response_error:
  {
    const gchar *str = gst_rtsp_status_as_text (code);

    GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
        ("Error (%d): %s", code, GST_STR_NULL (str)));
    goto cleanup_error;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
        ("Could not send message. (%s)", str));
    g_free (str);
    goto cleanup_error;
  }
no_transport:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, SETTINGS, (NULL),
        ("Server did not select transport."));
    goto cleanup_error;
  }
nothing_to_activate:
  {
    /* none of the available error codes is really right .. */
    GST_ELEMENT_ERROR (src, STREAM, CODEC_NOT_FOUND, (NULL),
        ("No supported stream was found. You might need to allow "
                "more transport protocols or may otherwise be missing "
                "the right GStreamer RTSP extension plugin."));
    return FALSE;
  }
cleanup_error:
  {
    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);
    return FALSE;
  }
}


/* Parse all the wifi related messages.
 *
 * Uses wfdconfig utility to parse the wfd parameters sent from the server
 */

static GstRTSPResult
gst_wfdrtspsrc_retrieve_wifi_parameters (GstWFDRTSPSrc * src)
{
  GstRTSPResult res = GST_RTSP_OK;
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPMessage message = { 0 };
  GstRTSPMethod method = { 0 };
  gchar *str = NULL;
  gchar *ua = NULL;
  guint8 *data;
  guint size;

  /* can't continue without a valid url */
  if (G_UNLIKELY (src->conninfo.url == NULL))
    goto no_url;
  src->tried_url_auth = FALSE;

  if ((res = gst_wfdrtspsrc_conninfo_connect (src, &src->conninfo)) < 0)
    goto connect_failed;

  res = gst_wfdrtspsrc_connection_receive (src, src->conninfo.connection, &message,
    src->ptcp_timeout);
  if(res != GST_RTSP_OK)
    goto connect_failed;

  if(message.type == GST_RTSP_MESSAGE_REQUEST) {
    method = message.type_data.request.method;
    if(method == GST_RTSP_OPTIONS)
      gst_wfdrtspsrc_handle_request (src, src->conninfo.connection, &message);
    else
      goto methods_error;
  }

  /* create OPTIONS */
  GST_DEBUG_OBJECT (src, "create options...");
  res = gst_rtsp_message_init_request (&request, GST_RTSP_OPTIONS,
      src->conninfo.url_str);
  if (res < 0)
    goto create_request_failed;

  str = g_strdup ("org.wfa.wfd1.0");
  ua = g_strdup (src->user_agent);

  gst_rtsp_message_add_header (&request, GST_RTSP_HDR_REQUIRE, str);
  gst_rtsp_message_add_header (&request, GST_RTSP_HDR_USER_AGENT, ua);
  /* send OPTIONS */
  GST_DEBUG_OBJECT (src, "send options...");
  if (gst_wfdrtspsrc_send (src, src->conninfo.connection, &request, &response,
          NULL) < 0)
    goto send_error;

  /* parse OPTIONS */
  if (!gst_wfdrtspsrc_parse_methods (src, &response))
    goto methods_error;

  /* Receive GET_PARAMETER request from server */
  res = gst_wfdrtspsrc_connection_receive (src, src->conninfo.connection, &message,
    src->ptcp_timeout);
  if(res != GST_RTSP_OK)
    goto connect_failed;

  if(message.type == GST_RTSP_MESSAGE_REQUEST) {
    method = message.type_data.request.method;
    if(method == GST_RTSP_GET_PARAMETER)
      gst_wfdrtspsrc_handle_request (src, src->conninfo.connection, &message);
    else
      goto methods_error;
  }

  /* Receive SET_PARAMETER request from server */
  res = gst_wfdrtspsrc_connection_receive (src, src->conninfo.connection, &message,
        src->ptcp_timeout);
  if(res != GST_RTSP_OK)
    goto connect_failed;

  if(message.type == GST_RTSP_MESSAGE_REQUEST) {
    method = message.type_data.request.method;
    if (method == GST_RTSP_SET_PARAMETER) {
      res = gst_wfdrtspsrc_handle_request (src, src->conninfo.connection, &message);
      if(res != GST_RTSP_OK)
        goto methods_error;
      GST_DEBUG_OBJECT (src, "waiting for second SET_PARAMETER ");
      res = gst_wfdrtspsrc_connection_receive (src, src->conninfo.connection, &message,
              src->ptcp_timeout);
      gst_rtsp_message_get_body (&message, &data, &size);

      method = message.type_data.request.method;

      /* Handle WFD_TRIGGER_METHOD:SETUP from server */
      if(method == GST_RTSP_SET_PARAMETER && g_strrstr((gchar*)data,"SETUP")) {
        res = gst_wfdrtspsrc_handle_request (src, src->conninfo.connection, &message);
        if(res != GST_RTSP_OK)
          goto methods_error;
        }
      }
    }

  /* clean up any messages */
  gst_rtsp_message_unset (&request);
  gst_rtsp_message_unset (&response);

  return res;

  /* ERRORS */
no_url:
  {
    GST_ELEMENT_ERROR (src, RESOURCE, NOT_FOUND, (NULL),
        ("No valid RTSP URL was provided"));
    goto cleanup_error;
  }
connect_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, RESOURCE, OPEN_READ_WRITE, (NULL),
        ("Failed to connect. (%s)", str));
    g_free (str);
    goto cleanup_error;
  }
create_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request. (%s)", str));
    g_free (str);
    goto cleanup_error;
  }

send_error:
  {
    /* Don't post a message - the rtsp_send method will have
     * taken care of it because we passed NULL for the response code */
    goto cleanup_error;
  }
methods_error:
  {
    /* error was posted */
    goto cleanup_error;
  }

cleanup_error:
  {
    if (src->conninfo.connection) {
      GST_DEBUG_OBJECT (src, "free connection");
      gst_wfdrtspsrc_conninfo_close (src, &src->conninfo, TRUE);
    }
    gst_rtsp_message_unset (&request);
    gst_rtsp_message_unset (&response);
    return res;
  }
}



static GstRTSPResult
gst_wfdrtspsrc_open (GstWFDRTSPSrc * src)
{
  GstRTSPResult res;

  src->methods =
      GST_RTSP_SETUP | GST_RTSP_PLAY | GST_RTSP_PAUSE | GST_RTSP_TEARDOWN;

  if ((res = gst_wfdrtspsrc_retrieve_wifi_parameters (src)) < 0)
    goto open_failed;

done:
  return res;

  /* ERRORS */
open_failed:
  {
    GST_WARNING_OBJECT (src, "can't setup streaming");
    src->open_error = TRUE;
    goto done;
  }
}


static GstRTSPResult
gst_wfdrtspsrc_close (GstWFDRTSPSrc * src, gboolean only_close)
{
  WFDRTSPStream *stream = NULL;
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPResult res = GST_RTSP_OK;
  gchar *control;
  gchar *setup_url;
  GstWFDRTSPConnInfo *info;

  GST_DEBUG_OBJECT (src, "TEARDOWN...");

  if (src->state < GST_RTSP_STATE_READY) {
    GST_DEBUG_OBJECT (src, "not ready, doing cleanup");
    goto close;
  }

  if (only_close)
    goto close;

  /* construct a control url */
  if (src->control)
    control = src->control;
  else
    control = src->conninfo.url_str;

  if (!(src->methods & (GST_RTSP_PLAY | GST_RTSP_TEARDOWN)))
    goto not_supported;

  stream = src->stream;

  /* try aggregate control first but do non-aggregate control otherwise */
  if (control)
    setup_url = control;
  else if ((setup_url = stream->conninfo.location) == NULL)
    goto  close;

  if (src->conninfo.connection) {
    info = &src->conninfo;
  } else if (stream->conninfo.connection) {
    info = &stream->conninfo;
  } else {
    goto close;
  }
  if (!info->connected)
    goto close;

  /* do TEARDOWN */
  res =
      gst_rtsp_message_init_request (&request, GST_RTSP_TEARDOWN, setup_url);
  if (res < 0)
    goto create_request_failed;

  if ((res =
          gst_wfdrtspsrc_send (src, info->connection, &request, &response,
              NULL)) < 0)
    goto send_error;

  /* FIXME, parse result? */
  gst_rtsp_message_unset (&request);
  gst_rtsp_message_unset (&response);

close:
  /* close connections */
  GST_DEBUG_OBJECT (src, "closing connection...");
  gst_wfdrtspsrc_conninfo_close (src, &src->conninfo, TRUE);
  if (stream) {
    GST_DEBUG_OBJECT (src, "closing stream %p connection...", stream);
    gst_wfdrtspsrc_conninfo_close (src, &stream->conninfo, TRUE);
  }

  /* cleanup */
  gst_wfdrtspsrc_cleanup (src);

  src->state = GST_RTSP_STATE_INVALID;

  return res;

  /* ERRORS */
create_request_failed:
  {
    gchar *str = gst_rtsp_strresult (res);

    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request. (%s)", str));
    g_free (str);
    goto close;
  }
send_error:
  {
    gchar *str = gst_rtsp_strresult (res);

    gst_rtsp_message_unset (&request);
    if (res != GST_RTSP_EINTR) {
      GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
          ("Could not send message. (%s)", str));
    } else {
      GST_WARNING_OBJECT (src, "TEARDOWN interrupted");
    }
    g_free (str);
    goto close;
  }
not_supported:
  {
    GST_DEBUG_OBJECT (src,
        "TEARDOWN and PLAY not supported, can't do TEARDOWN");
    goto close;
  }
}

/* RTP-Info is of the format:
 *
 * url=<URL>;[seq=<seqbase>;rtptime=<timebase>] [, url=...]
 *
 * rtptime corresponds to the timestamp for the NPT time given in the header
 * seqbase corresponds to the next sequence number we received. This number
 * indicates the first seqnum after the seek and should be used to discard
 * packets that are from before the seek.
 */
static gboolean
gst_wfdrtspsrc_parse_rtpinfo (GstWFDRTSPSrc * src, gchar * rtpinfo)
{
  gchar **infos;
  gint i, j;

  GST_DEBUG_OBJECT (src, "parsing RTP-Info %s", rtpinfo);

  infos = g_strsplit (rtpinfo, ",", 0);
  for (i = 0; infos[i]; i++) {
    gchar **fields;
    WFDRTSPStream *stream;
    gint32 seqbase;
    gint64 timebase;

    GST_DEBUG_OBJECT (src, "parsing info %s", infos[i]);

    /* init values, types of seqbase and timebase are bigger than needed so we
     * can store -1 as uninitialized values */
    stream = NULL;
    seqbase = -1;
    timebase = -1;

    /* parse url, find stream for url.
     * parse seq and rtptime. The seq number should be configured in the rtp
     * depayloader or session manager to detect gaps. Same for the rtptime, it
     * should be used to create an initial time newsegment. */
    fields = g_strsplit (infos[i], ";", 0);
    for (j = 0; fields[j]; j++) {
      GST_DEBUG_OBJECT (src, "parsing field %s", fields[j]);
      /* remove leading whitespace */
      fields[j] = g_strchug (fields[j]);
      if (g_str_has_prefix (fields[j], "url=")) {
        /* get the url and the stream */
        stream = src->stream;
      } else if (g_str_has_prefix (fields[j], "seq=")) {
        seqbase = atoi (fields[j] + 4);
      } else if (g_str_has_prefix (fields[j], "rtptime=")) {
        timebase = g_ascii_strtoll (fields[j] + 8, NULL, 10);
      }
    }
    g_strfreev (fields);
    /* now we need to store the values for the caps of the stream */
    if (stream != NULL) {
      GST_DEBUG_OBJECT (src,
          "found stream %p, setting: seqbase %d, timebase %" G_GINT64_FORMAT,
          stream, seqbase, timebase);

      /* we have a stream, configure detected params */
      stream->seqbase = seqbase;
      stream->timebase = timebase;
    }
  }
  g_strfreev (infos);

  return TRUE;
}

static gdouble
gst_wfdrtspsrc_get_float (const gchar * dstr)
{
  gchar s[G_ASCII_DTOSTR_BUF_SIZE] = { 0, };

  /* canonicalise floating point string so we can handle float strings
   * in the form "24.930" or "24,930" irrespective of the current locale */
  g_strlcpy (s, dstr, sizeof (s));
  g_strdelimit (s, ",", '.');
  return g_ascii_strtod (s, NULL);
}

static void
clear_rtp_base (GstWFDRTSPSrc * src, WFDRTSPStream * stream)
{
  stream->timebase = -1;
  stream->seqbase = -1;
  if (stream->caps) {
    GstStructure *s;

    stream->caps = gst_caps_make_writable (stream->caps);
    s = gst_caps_get_structure (stream->caps, 0);
    gst_structure_remove_fields (s, "clock-base", "seqnum-base", NULL);
  }
}

static GstRTSPResult
gst_wfdrtspsrc_ensure_open (GstWFDRTSPSrc * src)
{
  GstRTSPResult res = GST_RTSP_OK;

  if (src->state < GST_RTSP_STATE_READY) {
    res = GST_RTSP_ERROR;
    if (src->open_error) {
      GST_DEBUG_OBJECT (src, "the stream was in error");
      goto done;
    }

    if ((res = gst_wfdrtspsrc_open (src)) < 0) {
      GST_DEBUG_OBJECT (src, "failed to open stream");
      goto done;
    }
  }

done:
  return res;
}

static GstRTSPResult
gst_wfdrtspsrc_play (GstWFDRTSPSrc * src, GstSegment * segment)
{
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  GstRTSPResult res;
  gchar *hval;
  gint hval_idx;
  gchar *control;
  WFDRTSPStream *stream;
  gchar *setup_url;
  GstRTSPConnection *conn;

  src->is_paused = FALSE;


  GST_DEBUG_OBJECT (src, "PLAY...");

  if ((res = gst_wfdrtspsrc_ensure_open (src) < 0))
    goto open_failed;

  if (!(src->methods & GST_RTSP_PLAY))
    goto not_supported;

  if (src->state == GST_RTSP_STATE_PLAYING)
    goto was_playing;

  if (!src->conninfo.connection || !src->conninfo.connected)
    goto done;

  /* send some dummy packets before we activate the receive in the
   * udp sources */
  gst_wfdrtspsrc_send_dummy_packets (src);

  /* construct a control url */
  if (src->control)
    control = src->control;
  else
    control = src->conninfo.url_str;

  stream = src->stream;
  /* try aggregate control first but do non-aggregate control otherwise */
  if (control)
    setup_url = control;
  else if ((setup_url = stream->conninfo.location) == NULL)
    goto skip;

  if (src->conninfo.connection) {
    conn = src->conninfo.connection;
  } else if (stream->conninfo.connection) {
    conn = stream->conninfo.connection;
  } else {
    goto skip;
  }

  /* do play */
  res = gst_rtsp_message_init_request (&request, GST_RTSP_PLAY, setup_url);
  if (res < 0)
    goto create_request_failed;

  gst_rtsp_message_add_header (&request, GST_RTSP_HDR_USER_AGENT, (const gchar*)src->user_agent);

  if (segment->rate != 1.0) {
    gchar hval[G_ASCII_DTOSTR_BUF_SIZE];

    g_ascii_dtostr (hval, sizeof (hval), segment->rate);
    if (src->skip)
      gst_rtsp_message_add_header (&request, GST_RTSP_HDR_SCALE, hval);
    else
      gst_rtsp_message_add_header (&request, GST_RTSP_HDR_SPEED, hval);
  }

  if (res = gst_wfdrtspsrc_send (src, conn, &request, &response, NULL) < 0)
    goto send_error;

  /* seek may have silently failed as it is not supported */
  if (!(src->methods & GST_RTSP_PLAY)) {
    GST_DEBUG_OBJECT (src, "PLAY Range not supported; re-enable PLAY");
    /* obviously it is supported as we made it here */
    src->methods |= GST_RTSP_PLAY;
    /* but there is nothing to parse in the response,
     * so convey we have no idea and not to expect anything particular */
    clear_rtp_base (src, stream);
    /* NOTE the above also disables npt based eos detection */
    /* and below forces position to 0,
      * which is visible feedback we lost the plot */
    segment->start = segment->last_stop = 0;
  }

  gst_rtsp_message_unset (&request);

  /* assume 1.0 rate now, overwrite when the SCALE or SPEED headers are present. */
  segment->rate = 1.0;

  /* parse Speed header. This is the intended playback rate of the stream
   * and should be put in the NEWSEGMENT rate field. */
  if (gst_rtsp_message_get_header (&response, GST_RTSP_HDR_SPEED, &hval,
          0) == GST_RTSP_OK) {
    segment->rate = gst_wfdrtspsrc_get_float (hval);
  } else if (gst_rtsp_message_get_header (&response, GST_RTSP_HDR_SCALE,
          &hval, 0) == GST_RTSP_OK) {
    segment->rate = gst_wfdrtspsrc_get_float (hval);
  }

  /* parse the RTP-Info header field (if ANY) to get the base seqnum and timestamp
   * for the RTP packets. If this is not present, we assume all starts from 0...
   * This is info for the RTP session manager that we pass to it in caps. */
  hval_idx = 0;
  while (gst_rtsp_message_get_header (&response, GST_RTSP_HDR_RTP_INFO,
          &hval, hval_idx++) == GST_RTSP_OK)
    gst_wfdrtspsrc_parse_rtpinfo (src, hval);

  gst_rtsp_message_unset (&response);

skip:
  /* configure the caps of the streams after we parsed all headers. */
  gst_wfdrtspsrc_configure_caps (src, segment);

  src->running = TRUE;
  src->base_time = -1;
  src->state = GST_RTSP_STATE_PLAYING;

  /* mark discont */
  GST_DEBUG_OBJECT (src, "mark DISCONT");
  stream->discont = TRUE;

done:
  return res;

  /* ERRORS */
open_failed:
  {
    GST_DEBUG_OBJECT (src, "failed to open stream");
    goto done;
  }
not_supported:
  {
    GST_DEBUG_OBJECT (src, "PLAY is not supported");
    goto done;
  }
was_playing:
  {
    GST_DEBUG_OBJECT (src, "we were already PLAYING");
    goto done;
  }
create_request_failed:
  {
    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request."));
    return FALSE;
  }
send_error:
  {
    gst_rtsp_message_unset (&request);
    GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
        ("Could not send message."));
    return FALSE;
  }
}

static GstRTSPResult
gst_wfdrtspsrc_pause (GstWFDRTSPSrc * src, gboolean idle)
{
  GstRTSPResult res = GST_RTSP_OK;
  GstRTSPMessage request = { 0 };
  GstRTSPMessage response = { 0 };
  WFDRTSPStream *stream;
  GstRTSPConnection *conn;
  gchar *setup_url;
  gchar *control;

  GST_DEBUG_OBJECT (src, "PAUSE...");

  if ((res = gst_wfdrtspsrc_ensure_open (src) < 0))
    goto open_failed;

  if (!(src->methods & GST_RTSP_PAUSE))
    goto not_supported;

  if (src->state == GST_RTSP_STATE_READY)
    goto was_paused;

  if (!src->conninfo.connection || !src->conninfo.connected)
    goto no_connection;

  /* construct a control url */
  if (src->control)
    control = src->control;
  else
    control = src->conninfo.url_str;

  stream = src->stream;

  /* try aggregate control first but do non-aggregate control otherwise */
  if (control)
    setup_url = control;
  else if ((setup_url = stream->conninfo.location) == NULL)
    goto no_connection;

  if (src->conninfo.connection) {
    conn = src->conninfo.connection;
  } else if (stream->conninfo.connection) {
    conn = stream->conninfo.connection;
  } else {
    goto no_connection;
  }

  if (gst_rtsp_message_init_request (&request, GST_RTSP_PAUSE, setup_url) < 0)
    goto create_request_failed;

  gst_rtsp_message_add_header (&request, GST_RTSP_HDR_USER_AGENT, (const gchar *)src->user_agent);

  if ((res = gst_wfdrtspsrc_send (src, conn, &request, &response, NULL)) < 0)
    goto send_error;

  gst_rtsp_message_unset (&request);
  gst_rtsp_message_unset (&response);

  src->is_paused = TRUE;

no_connection:
  src->state = GST_RTSP_STATE_READY;

done:
  return res;

  /* ERRORS */
open_failed:
  {
    GST_DEBUG_OBJECT (src, "failed to open stream");
    goto done;
  }
not_supported:
  {
    GST_DEBUG_OBJECT (src, "PAUSE is not supported");
    goto done;
  }
was_paused:
  {
    GST_DEBUG_OBJECT (src, "we were already PAUSED");
    goto done;
  }
create_request_failed:
  {
    GST_ELEMENT_ERROR (src, LIBRARY, INIT, (NULL),
        ("Could not create request."));
    goto done;
  }
send_error:
  {
    gst_rtsp_message_unset (&request);
    GST_ELEMENT_ERROR (src, RESOURCE, WRITE, (NULL),
        ("Could not send message."));
    goto done;
  }
}

static void
gst_wfdrtspsrc_handle_message (GstBin * bin, GstMessage * message)
{
  GstWFDRTSPSrc *wfdrtspsrc;

  wfdrtspsrc = GST_WFDRTSPSRC (bin);

  GST_DEBUG_OBJECT (wfdrtspsrc, "got %s message from %s",
      GST_MESSAGE_TYPE_NAME (message), GST_MESSAGE_SRC_NAME (message));

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      gst_message_unref (message);
      break;
    case GST_MESSAGE_ELEMENT:
    {
      const GstStructure *s = gst_message_get_structure (message);
      if (gst_structure_has_name (s, "GstUDPSrcTimeout"))
        GST_DEBUG_OBJECT (bin, "timeout on UDP port");

      GST_BIN_CLASS (parent_class)->handle_message (bin, message);
      break;
    }
    case GST_MESSAGE_ERROR:
    {
      GstFlowReturn ret = GST_FLOW_OK;
      WFDRTSPStream *stream;
      GstObject *message_src;
      gchar* debug = NULL;
      GError* error = NULL;

      message_src = GST_MESSAGE_SRC (message);

      gst_message_parse_error (message, &error, &debug);
      GST_DEBUG_OBJECT (wfdrtspsrc, "error : %s", error->message);
      GST_DEBUG_OBJECT (wfdrtspsrc, "debug : %s", debug);

      if (debug)
        g_free (debug);
      debug = NULL;
      g_error_free (error);

      stream = wfdrtspsrc->stream;
      if (!stream)
        goto forward;

      /* we ignore the RTCP udpsrc */
      if (stream->udpsrc[1] == GST_ELEMENT_CAST (message_src))
        goto done;

#ifdef ENABLE_WFD_EXTENDED_FEATURES
      /* we ignore the Retransmitted RTP udpsrc */
      if (stream->udpsrc[2] == GST_ELEMENT_CAST (message_src))
        goto done;
#endif

      /* if we get error messages from the udp sources, that's not a problem as
       * long as not all of them error out. We also don't really know what the
       * problem is, the message does not give enough detail... */
      ret = gst_wfdrtspsrc_combine_flows (wfdrtspsrc, stream, GST_FLOW_NOT_LINKED);
      GST_DEBUG_OBJECT (wfdrtspsrc, "combined flows: %s", gst_flow_get_name (ret));
      if (ret != GST_FLOW_OK)
        goto forward;

    done:
      gst_message_unref (message);
      break;

    forward:
      /* fatal but not our message, forward */
      GST_BIN_CLASS (parent_class)->handle_message (bin, message);
      break;
    }
    default:
    {
      GST_BIN_CLASS (parent_class)->handle_message (bin, message);
      break;
    }
  }
}

/* the thread where everything happens */
static void
gst_wfdrtspsrc_thread (GstWFDRTSPSrc * src)
{
  gint cmd;
  GstRTSPResult ret;
  gboolean running = FALSE;

  GST_OBJECT_LOCK (src);
  cmd = src->loop_cmd;
  src->loop_cmd = WFD_CMD_WAIT;
  GST_DEBUG_OBJECT (src, "got command %d", cmd);

  /* we got the message command, so ensure communication is possible again */
  gst_wfdrtspsrc_connection_flush (src, FALSE);

  /* we allow these to be interrupted */
  if (cmd == WFD_CMD_LOOP)
    src->waiting = TRUE;
  GST_OBJECT_UNLOCK (src);

  switch (cmd) {
    case WFD_CMD_OPEN:
      ret = gst_wfdrtspsrc_open (src);
      break;
    case WFD_CMD_PLAY:
      ret = gst_wfdrtspsrc_play (src, &src->segment);
      if (ret == GST_RTSP_OK)
        running = TRUE;
      break;
    case WFD_CMD_PAUSE:
      ret = gst_wfdrtspsrc_pause (src, TRUE);
      if (ret == GST_RTSP_OK)
        running = TRUE;
      break;
    case WFD_CMD_CLOSE:
      ret = gst_wfdrtspsrc_close (src, FALSE);
      break;
    case WFD_CMD_LOOP:
      running = gst_wfdrtspsrc_loop (src);
      break;
    default:
      break;
  }
  GST_OBJECT_LOCK (src);

  /* and go back to sleep */
  if (src->loop_cmd == WFD_CMD_WAIT) {
    if (running)
      src->loop_cmd = WFD_CMD_LOOP;
    else if (src->task)
      gst_task_pause (src->task);
  }
  /* reset waiting */
  src->waiting = FALSE;
  GST_OBJECT_UNLOCK (src);
}

static gboolean
gst_wfdrtspsrc_start (GstWFDRTSPSrc * src)
{
  GST_DEBUG_OBJECT (src, "starting");

  GST_OBJECT_LOCK (src);

  src->loop_cmd = WFD_CMD_WAIT;

  if (src->task == NULL) {
    src->task = gst_task_create ((GstTaskFunction) gst_wfdrtspsrc_thread, src);
    if (src->task == NULL)
      goto task_error;

    gst_task_set_lock (src->task, GST_WFD_RTSP_STREAM_GET_LOCK (src));
  }
  GST_OBJECT_UNLOCK (src);

  return TRUE;

  /* ERRORS */
task_error:
  {
    GST_ERROR_OBJECT (src, "failed to create task");
    return FALSE;
  }
}

static gboolean
gst_wfdrtspsrc_stop (GstWFDRTSPSrc * src)
{
  GstTask *task;

  GST_DEBUG_OBJECT (src, "stopping");

  /* also cancels pending task */
  gst_wfdrtspsrc_loop_send_cmd (src, WFD_CMD_WAIT, TRUE);

  GST_OBJECT_LOCK (src);
  if ((task = src->task)) {
    src->task = NULL;
    GST_OBJECT_UNLOCK (src);

    gst_task_stop (task);

    /* make sure it is not running */
    GST_WFD_RTSP_STREAM_LOCK (src);
    GST_WFD_RTSP_STREAM_UNLOCK (src);

    /* now wait for the task to finish */
    gst_task_join (task);

    /* and free the task */
    gst_object_unref (GST_OBJECT (task));

    GST_OBJECT_LOCK (src);
  }
  GST_OBJECT_UNLOCK (src);

  /* ensure synchronously all is closed and clean */
  gst_wfdrtspsrc_close (src, TRUE);

  return TRUE;
}

static GstStateChangeReturn
gst_wfdrtspsrc_change_state (GstElement * element, GstStateChange transition)
{
  GstWFDRTSPSrc *wfdrtspsrc;
  GstStateChangeReturn ret;

  wfdrtspsrc = GST_WFDRTSPSRC (element);

  switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
      GST_DEBUG_OBJECT (wfdrtspsrc, "NULL->READY");
      if (!gst_wfdrtspsrc_start (wfdrtspsrc))
        goto start_failed;
      break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      GST_DEBUG_OBJECT (wfdrtspsrc, "READY->PAUSED");
      /* init some state */
      wfdrtspsrc->open_error = FALSE;
      if (GST_RTSP_OK != gst_wfdrtspsrc_open (wfdrtspsrc))
        ret = GST_STATE_CHANGE_FAILURE;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      GST_DEBUG_OBJECT(wfdrtspsrc, "PAUSED->PLAYING");
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      GST_DEBUG_OBJECT(wfdrtspsrc, "PLAYING->PAUSED");
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    default:
      break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    goto done;

  switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
      if (wfdrtspsrc->state != GST_RTSP_STATE_PLAYING)
        gst_wfdrtspsrc_loop_send_cmd (wfdrtspsrc, WFD_CMD_PLAY, FALSE);
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
    case GST_STATE_CHANGE_READY_TO_PAUSED:
      ret = GST_STATE_CHANGE_NO_PREROLL;
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      gst_wfdrtspsrc_loop_send_cmd (wfdrtspsrc, WFD_CMD_CLOSE, TRUE);
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      gst_wfdrtspsrc_stop (wfdrtspsrc);
      break;
    default:
      break;
  }

done:
  return ret;

start_failed:
  {
    GST_DEBUG_OBJECT (wfdrtspsrc, "start failed");
    return GST_STATE_CHANGE_FAILURE;
  }
}

static gboolean
gst_wfdrtspsrc_send_event (GstElement * element, GstEvent * event)
{
  gboolean res;
  GstWFDRTSPSrc *wfdrtspsrc;

  wfdrtspsrc = GST_WFDRTSPSRC (element);

  if (GST_EVENT_IS_DOWNSTREAM (event)) {
    res = gst_wfdrtspsrc_push_event (wfdrtspsrc, event, TRUE);
  } else {
    res = GST_ELEMENT_CLASS (parent_class)->send_event (element, event);
  }

  return res;
}


/*** GSTURIHANDLER INTERFACE *************************************************/
static GstURIType
gst_wfdrtspsrc_uri_get_type (void)
{
  return GST_URI_SRC;
}

static gchar **
gst_wfdrtspsrc_uri_get_protocols (void)
{
  static const gchar *protocols[] =
      { "rtsp", "rtspu", "rtspt", "rtsph", "rtsp-sdp", NULL };

  return (gchar **) protocols;
}

static const gchar *
gst_wfdrtspsrc_uri_get_uri (GstURIHandler * handler)
{
  GstWFDRTSPSrc *src = GST_WFDRTSPSRC (handler);

  /* should not dup */
  return src->conninfo.location;
}

static gboolean
gst_wfdrtspsrc_uri_set_uri (GstURIHandler * handler, const gchar * uri)
{
  GstWFDRTSPSrc *src;
  GstRTSPResult res;
  GstRTSPUrl *newurl = NULL;

  src = GST_WFDRTSPSRC (handler);

  /* same URI, we're fine */
  if (src->conninfo.location && uri && !strcmp (uri, src->conninfo.location))
    goto was_ok;

    /* try to parse */
    GST_DEBUG_OBJECT (src, "parsing URI");
    if ((res = gst_rtsp_url_parse (uri, &newurl)) < 0)
      goto parse_error;


  /* if worked, free previous and store new url object along with the original
   * location. */
  GST_DEBUG_OBJECT (src, "configuring URI");
  g_free (src->conninfo.location);
  src->conninfo.location = g_strdup (uri);
  gst_rtsp_url_free (src->conninfo.url);
  src->conninfo.url = newurl;
  g_free (src->conninfo.url_str);
  if (newurl)
    src->conninfo.url_str = gst_rtsp_url_get_request_uri (src->conninfo.url);
  else
    src->conninfo.url_str = NULL;

  GST_DEBUG_OBJECT (src, "set uri: %s", GST_STR_NULL (uri));
  GST_DEBUG_OBJECT (src, "request uri is: %s",
      GST_STR_NULL (src->conninfo.url_str));

  return TRUE;

  /* Special cases */
was_ok:
  {
    GST_DEBUG_OBJECT (src, "URI was ok: '%s'", GST_STR_NULL (uri));
    return TRUE;
  }
parse_error:
  {
    GST_ERROR_OBJECT (src, "Not a valid RTSP url '%s' (%d)",
        GST_STR_NULL (uri), res);
    return FALSE;
  }
}

static void
gst_wfdrtspsrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
  GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

  iface->get_type = gst_wfdrtspsrc_uri_get_type;
  iface->get_protocols = gst_wfdrtspsrc_uri_get_protocols;
  iface->get_uri = gst_wfdrtspsrc_uri_get_uri;
  iface->set_uri = gst_wfdrtspsrc_uri_set_uri;
}

static gboolean
gst_wfdrtspsrc_plugin_init (GstPlugin *plugin)
{
  if (!gst_element_register (plugin, "wfdrtspsrc", GST_RANK_PRIMARY, gst_wfdrtspsrc_get_type())) {
    return FALSE;
  }
  return TRUE;
}

static gchar* gst_wfdrtspsrc_parse_parametr(gchar* data, const gchar* delim)
{
  gchar **splited_message;
  gchar *res;
  splited_message = g_strsplit((gchar*)data, delim, 2);
  res = g_strdup(splited_message[1]);
  g_strfreev (splited_message);
  return res;
}

static GstRTSPResult _get_cea_resolution_and_set_to_src(GstWFDRTSPSrc *src, WFDVideoCEAResolution Resolution)
{
  WFDVideoCEAResolution CEARes = Resolution;
  switch(CEARes)
  {
    case WFD_CEA_UNKNOWN:
      break;
    case WFD_CEA_640x480P60:
      src->video_width=640;
      src->video_height=480;
      src->video_framerate=60;
      break;
    case WFD_CEA_720x480P60:
      src->video_width=720;
      src->video_height=480;
      src->video_framerate=60;
      break;
    case WFD_CEA_720x480I60:
      src->video_width=720;
      src->video_height=480;
      src->video_framerate=60;
      break;
    case WFD_CEA_720x576P50:
      src->video_width=720;
      src->video_height=576;
      src->video_framerate=50;
      break;
    case WFD_CEA_720x576I50:
       src->video_width=720;
      src->video_height=576;
      src->video_framerate=50;
      break;
    case WFD_CEA_1280x720P30:
      src->video_width=1280;
      src->video_height=720;
      src->video_framerate=30;
      break;
    case WFD_CEA_1280x720P60:
      src->video_width=1280;
      src->video_height=720;
      src->video_framerate=60;
      break;
    case WFD_CEA_1920x1080P30:
      src->video_width=1920;
      src->video_height=1080;
      src->video_framerate=30;
      break;
    case WFD_CEA_1920x1080P60:
      src->video_width=1920;
      src->video_height=1080;
      src->video_framerate=60;
      break;
    case WFD_CEA_1920x1080I60:
      src->video_width=1920;
      src->video_height=1080;
      src->video_framerate=60;
      break;
    case WFD_CEA_1280x720P25:
      src->video_width=1280;
      src->video_height=720;
      src->video_framerate=25;
      break;
    case WFD_CEA_1280x720P50:
      src->video_width=1280;
      src->video_height=720;
      src->video_framerate=50;
      break;
    case WFD_CEA_1920x1080P25:
      src->video_width=1920;
      src->video_height=1080;
      src->video_framerate=25;
      break;
    case WFD_CEA_1920x1080P50:
      src->video_width=1920;
      src->video_height=1080;
      src->video_framerate=50;
      break;
    case WFD_CEA_1920x1080I50:
      src->video_width=1920;
      src->video_height=1080;
      src->video_framerate=50;
      break;
    case WFD_CEA_1280x720P24:
      src->video_width=1280;
      src->video_height=720;
      src->video_framerate=24;
      break;
    case WFD_CEA_1920x1080P24:
      src->video_width=1920;
      src->video_height=1080;
      src->video_framerate=24;
      break;
  }
  return GST_RTSP_OK;
}

static GstRTSPResult _get_vesa_resolution_and_set_to_src(GstWFDRTSPSrc *src, WFDVideoVESAResolution Resolution)
{
  WFDVideoVESAResolution VESARes = Resolution;
  switch(VESARes)
  {
    case WFD_VESA_UNKNOWN:
      break;
    case WFD_VESA_800x600P30:
      src->video_width=800;
      src->video_height=600;
      src->video_framerate=30;
      break;
    case WFD_VESA_800x600P60:
      src->video_width=800;
      src->video_height=600;
      src->video_framerate=60;
      break;
    case WFD_VESA_1024x768P30:
      src->video_width=1024;
      src->video_height=768;
      src->video_framerate=30;
      break;
    case WFD_VESA_1024x768P60:
      src->video_width=1024;
      src->video_height=768;
      src->video_framerate=60;
      break;
    case WFD_VESA_1152x864P30:
      src->video_width=1152;
      src->video_height=864;
      src->video_framerate=30;
      break;
    case WFD_VESA_1152x864P60:
      src->video_width=1152;
      src->video_height=864;
      src->video_framerate=60;
      break;
    case WFD_VESA_1280x768P30:
      src->video_width=1280;
      src->video_height=768;
      src->video_framerate=30;
      break;
    case WFD_VESA_1280x768P60:
      src->video_width=1280;
      src->video_height=768;
      src->video_framerate=60;
      break;
    case WFD_VESA_1280x800P30:
      src->video_width=1280;
      src->video_height=800;
      src->video_framerate=30;
      break;
    case WFD_VESA_1280x800P60:
      src->video_width=1280;
      src->video_height=800;
      src->video_framerate=60;
      break;
    case WFD_VESA_1360x768P30:
      src->video_width=1360;
      src->video_height=768;
      src->video_framerate=30;
      break;
    case WFD_VESA_1360x768P60:
      src->video_width=1360;
      src->video_height=768;
      src->video_framerate=60;
      break;
    case WFD_VESA_1366x768P30:
      src->video_width=1366;
      src->video_height=768;
      src->video_framerate=30;
      break;
    case WFD_VESA_1366x768P60:
      src->video_width=1366;
      src->video_height=768;
      src->video_framerate=60;
      break;
    case WFD_VESA_1280x1024P30:
      src->video_width=1280;
      src->video_height=1024;
      src->video_framerate=30;
      break;
    case WFD_VESA_1280x1024P60:
      src->video_width=1280;
      src->video_height=1024;
      src->video_framerate=60;
      break;
    case WFD_VESA_1400x1050P30:
      src->video_width=1400;
      src->video_height=1050;
      src->video_framerate=30;
      break;
    case WFD_VESA_1400x1050P60:
      src->video_width=1400;
      src->video_height=1050;
      src->video_framerate=60;
      break;
    case WFD_VESA_1440x900P30:
      src->video_width=1440;
      src->video_height=900;
      src->video_framerate=30;
      break;
    case WFD_VESA_1440x900P60:
      src->video_width=1440;
      src->video_height=900;
      src->video_framerate=60;
      break;
    case WFD_VESA_1600x900P30:
      src->video_width=1600;
      src->video_height=900;
      src->video_framerate=30;
      break;
    case WFD_VESA_1600x900P60:
      src->video_width=1600;
      src->video_height=900;
      src->video_framerate=60;
      break;
    case WFD_VESA_1600x1200P30:
      src->video_width=1600;
      src->video_height=1200;
      src->video_framerate=30;
      break;
    case WFD_VESA_1600x1200P60:
      src->video_width=1600;
      src->video_height=1200;
      src->video_framerate=60;
      break;
    case WFD_VESA_1680x1024P30:
      src->video_width=1680;
      src->video_height=1024;
      src->video_framerate=30;
      break;
    case WFD_VESA_1680x1024P60:
      src->video_width=1680;
      src->video_height=1024;
      src->video_framerate=60;
      break;
    case WFD_VESA_1680x1050P30:
      src->video_width=1680;
      src->video_height=1050;
      src->video_framerate=30;
      break;
    case WFD_VESA_1680x1050P60:
      src->video_width=1680;
      src->video_height=1050;
      src->video_framerate=60;
      break;
    case WFD_VESA_1920x1200P30:
      src->video_width=1920;
      src->video_height=1200;
      src->video_framerate=30;
      break;
    case WFD_VESA_1920x1200P60:
      src->video_width=1920;
      src->video_height=1200;
      src->video_framerate=60;
      break;
  }
  return GST_RTSP_OK;
}

static GstRTSPResult _get_hh_resolution_and_set_to_src(GstWFDRTSPSrc *src, WFDVideoHHResolution Resolution)
{
  WFDVideoHHResolution HHRes = Resolution;
  switch(HHRes)
  {
    case WFD_HH_UNKNOWN:
      break;
    case WFD_HH_800x480P30:
      src->video_width=800;
      src->video_height=480;
      src->video_framerate=30;
      break;
    case WFD_HH_800x480P60:
      src->video_width=800;
      src->video_height=480;
      src->video_framerate=60;
      break;
    case WFD_HH_854x480P30:
      src->video_width=854;
      src->video_height=480;
      src->video_framerate=30;
      break;
    case WFD_HH_854x480P60:
      src->video_width=854;
      src->video_height=480;
      src->video_framerate=60;
      break;
    case WFD_HH_864x480P30:
      src->video_width=864;
      src->video_height=480;
      src->video_framerate=30;
      break;
    case WFD_HH_864x480P60:
      src->video_width=864;
      src->video_height=480;
      src->video_framerate=60;
      break;
    case WFD_HH_640x360P30:
      src->video_width=640;
      src->video_height=360;
      src->video_framerate=30;
      break;
    case WFD_HH_640x360P60:
      src->video_width=640;
      src->video_height=360;
      src->video_framerate=60;
      break;
    case WFD_HH_960x540P30:
      src->video_width=960;
      src->video_height=540;
      src->video_framerate=30;
      break;
    case WFD_HH_960x540P60:
      src->video_width=960;
      src->video_height=540;
      src->video_framerate=60;
      break;
    case WFD_HH_848x480P30:
      src->video_width=848;
      src->video_height=480;
      src->video_framerate=30;
      break;
    case WFD_HH_848x480P60:
      src->video_width=848;
      src->video_height=480;
      src->video_framerate=60;
      break;
  }
  return GST_RTSP_OK;
}
  static GstRTSPResult
gst_wfdrtspsrc_get_video_parametr(GstWFDRTSPSrc * src, WFDMessage * msg)
{
  WFDVideoCodecs cvCodec = WFD_VIDEO_UNKNOWN;
  WFDVideoNativeResolution cNative = WFD_VIDEO_CEA_RESOLUTION;
  guint64 cNativeResolution = 0;
  WFDVideoCEAResolution cCEAResolution = WFD_CEA_UNKNOWN;
  WFDVideoVESAResolution cVESAResolution = WFD_VESA_UNKNOWN;
  WFDVideoHHResolution cHHResolution = WFD_HH_UNKNOWN;
  WFDVideoH264Profile cProfile = WFD_H264_UNKNOWN_PROFILE;
  WFDVideoH264Level cLevel = WFD_H264_LEVEL_UNKNOWN;
  guint32 cMaxHeight = 0;
  guint32 cMaxWidth = 0;
  guint32 cmin_slice_size = 0;
  guint32 cslice_enc_params = 0;
  guint cframe_rate_control = 0;
  guint cvLatency = 0;
  wfdconfig_get_prefered_video_format(msg, &cvCodec, &cNative, &cNativeResolution,
      &cCEAResolution, &cVESAResolution, &cHHResolution,
      &cProfile, &cLevel, &cvLatency, &cMaxHeight,
      &cMaxWidth, &cmin_slice_size, &cslice_enc_params, &cframe_rate_control);
  switch(cNative)
  {
    case WFD_VIDEO_CEA_RESOLUTION:
      _get_cea_resolution_and_set_to_src(src, cCEAResolution);
      break;
    case WFD_VIDEO_VESA_RESOLUTION:
      _get_vesa_resolution_and_set_to_src(src, cVESAResolution);
      break;
    case WFD_VIDEO_HH_RESOLUTION:
      _get_hh_resolution_and_set_to_src(src, cHHResolution);
      break;
  }
  return GST_RTSP_OK;
}

static void
uibc_new_tcp (GstWFDRTSPSrc * src)
{
  struct sockaddr_in serv_addr;

  GST_DEBUG ("Calling uibc_state_cb %x", (void *)src->uibc->state_cb);
  src->uibc->state_cb(1, (void *)src->uibc->hidc_cap_list_src, src->uibc->hidc_cap_count_src, src->video_width, src->video_height, src->uibc->wfd_sink_handle);
  src->uibc->sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (src->uibc->sockfd < 0) {
    GST_ERROR("ERROR opening socket\n");
  }

  memset(&serv_addr, 0x00, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(src->uibc->port);
  serv_addr.sin_addr.s_addr = inet_addr(gst_rtsp_connection_get_ip (src->conninfo.connection));
  GST_DEBUG("uibc get socket before connect\n");

  int retry = 0;
  int rc = 0;
try:
  rc = connect(src->uibc->sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr));
  if (rc < 0) {
    GST_ERROR("uibc trying to connect failed %d",rc);
    if (retry <5) {
      GST_ERROR("uibc trying to connect again..");
      retry++;
      usleep(10000);
      goto try;
    }
  }
  if (retry < 5)
    GST_LOG("WFD server initiated and connected %u\n", src->uibc->port);
  else {
    GST_ERROR ("WFD server not connected");
    src->uibc->state_cb(0, NULL, 0, src->video_width, src->video_height, src->uibc->wfd_sink_handle);
    shutdown (src->uibc->sockfd, SHUT_RDWR);
    src->uibc->sockfd = -1;
  }
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
                   GST_VERSION_MINOR,
                   "wfdrtspsrc",
                   "Wifi direct src plugin",
                   gst_wfdrtspsrc_plugin_init,
                   VERSION,
                   "LGPL",
                   "Samsung Electronics Co",
                   "http://www.samsung.com")

