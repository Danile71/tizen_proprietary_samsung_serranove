/*
 * audiosplitter
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
 * SECTION:element-audiosplitter
 *
 * Splits the voice pcm data into two separate Tx & Rx streams
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fdsrc ! audiosplitter name=s ! s.tx_src ! queue ! filesink location=txdata.pcm
 *                          s.rx_src ! queue ! filesink location=rxdata.pcm
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>

#include "gstaudiosplitter.h"

GST_DEBUG_CATEGORY_STATIC (gst_audio_splitter_debug);
#define GST_CAT_DEFAULT gst_audio_splitter_debug
#define _do_init(bla)   GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "audiosplitter", 0, "Audio splitter"); \
	    GST_DEBUG("Audio splitter is registered");

GST_BOILERPLATE_FULL(GstAudioSplitter, gst_audio_splitter, GstElement, GST_TYPE_ELEMENT, _do_init);

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_SILENT,
  PROP_FRAMESIZE,
  PROP_DATATYPE
};

#define VOICE_DATA_HEADER_SIZE 4

#define VOICE_DATATYPE_NARROWBAND         0x01
#define VOICE_DATATYPE_WIDEBAND           0x02

#define VOICE_DATA_NARROWBAND_FRAMESIZE   640
#define VOICE_DATA_NARROWBAND_DATASIZE    (VOICE_DATA_NARROWBAND_FRAMESIZE >> 1)
#define VOICE_DATA_WIDEBAND_FRAMESIZE     1280
#define VOICE_DATA_WIDEBAND_DATASIZE      (VOICE_DATA_WIDEBAND_FRAMESIZE >> 1)

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate audio_splitter_sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("ANY")
    );

static GstStaticPadTemplate audio_splitter_txsrc_template = GST_STATIC_PAD_TEMPLATE ("txsrc",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "audio/x-raw-int, "
        "endianness = (int) " G_STRINGIFY (G_BYTE_ORDER) ", "
        "signed = (boolean) true, "
        "width = (int) 16, "
        "depth = (int) 16"
      )
    );

static GstStaticPadTemplate audio_splitter_rxsrc_template = GST_STATIC_PAD_TEMPLATE ("rxsrc",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "audio/x-raw-int, "
        "endianness = (int) " G_STRINGIFY (G_BYTE_ORDER) ", "
        "signed = (boolean) true, "
        "width = (int) 16, "
        "depth = (int) 16"
      )
    );


static void gst_audio_splitter_finalize(GObject * object);
static void gst_audio_splitter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_audio_splitter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static gboolean gst_audio_splitter_set_caps (GstPad * pad, GstCaps * caps);
static GstStateChangeReturn gst_audio_splitter_change_state(GstElement *element, GstStateChange transition);
static GstFlowReturn gst_audio_splitter_chain (GstPad * pad, GstBuffer * buf);

/* GObject vmethod implementations */

static void
gst_audio_splitter_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "AudioSplitter",
    "FIXME:Generic",
    "FIXME:Generic Template Element",
    "Prashanth Kumar <<prashanth.kd@samsung.com>>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&audio_splitter_txsrc_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&audio_splitter_rxsrc_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&audio_splitter_sink_template));
}

/* initialize the audiosplitter's class */
static void
gst_audio_splitter_class_init (GstAudioSplitterClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->finalize = gst_audio_splitter_finalize;
  gobject_class->set_property = gst_audio_splitter_set_property;
  gobject_class->get_property = gst_audio_splitter_get_property;
  gstelement_class->change_state = GST_DEBUG_FUNCPTR(gst_audio_splitter_change_state);

  g_object_class_install_property (gobject_class, PROP_SILENT,
      g_param_spec_boolean ("silent", "Silent", "Produce verbose output ?",
          FALSE, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FRAMESIZE,
      g_param_spec_int ("framesize", "frame-size", "size of (tx + rx) frame size",
	   640, 1280, VOICE_DATA_NARROWBAND_FRAMESIZE, G_PARAM_READABLE));

  g_object_class_install_property (gobject_class, PROP_DATATYPE,
      g_param_spec_int ("datatype", "data-type", "voice data format: 1=>narrow-band, 2=>wide-band",
	   1, 2, VOICE_DATATYPE_NARROWBAND, G_PARAM_READABLE));
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_audio_splitter_init (GstAudioSplitter * filter,
    GstAudioSplitterClass * gclass)
{
  filter->sinkpad = gst_pad_new_from_static_template (&audio_splitter_sink_template, "sink");
  gst_pad_set_setcaps_function (filter->sinkpad,
                                GST_DEBUG_FUNCPTR(gst_audio_splitter_set_caps));
  gst_pad_set_chain_function (filter->sinkpad,
                              GST_DEBUG_FUNCPTR(gst_audio_splitter_chain));

  filter->srcpad_tx = gst_pad_new_from_static_template (&audio_splitter_txsrc_template, "txsrc");
  filter->srcpad_rx = gst_pad_new_from_static_template (&audio_splitter_rxsrc_template, "rxsrc");

  gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad_tx);
  gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad_rx);

  filter->silent = FALSE;
  filter->frame_size = VOICE_DATA_NARROWBAND_FRAMESIZE;
  filter->frame_type = VOICE_DATATYPE_NARROWBAND;

  filter->outcaps = gst_caps_new_simple(
        "audio/x-raw-int",
        "endianness", G_TYPE_INT, G_BYTE_ORDER,
        "signed" , G_TYPE_BOOLEAN, TRUE,
        "width", G_TYPE_INT, 16,
        "depth" , G_TYPE_INT, 16,
	"channels" , G_TYPE_INT, 1,
        "rate" , G_TYPE_INT, 8000, NULL);
  if (!filter->outcaps)
  {
	  GST_ERROR("failed to create out-caps\n");
	  return;
  }

  filter->buff_adapter = gst_adapter_new();
  if(!filter->buff_adapter)
  {
	  GST_ERROR("Unable to create buff_adapter");
	  return;
  }

  GST_LOG("init done");
}


static void
gst_audio_splitter_finalize(GObject * object)
{
  GstAudioSplitter *filter = GST_AUDIOSPLITTER (object);

  if(filter->buff_adapter)
  {
	  gst_adapter_clear(filter->buff_adapter);
	  g_object_unref(filter->buff_adapter);
	  filter->buff_adapter = NULL;
  }

  GST_LOG("finalize done");
}

static void
gst_audio_splitter_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstAudioSplitter *filter = GST_AUDIOSPLITTER (object);

  switch (prop_id) {
    case PROP_SILENT:
      filter->silent = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_audio_splitter_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstAudioSplitter *filter = GST_AUDIOSPLITTER (object);

  switch (prop_id) {
    case PROP_SILENT:
      g_value_set_boolean (value, filter->silent);
      break;
    case PROP_FRAMESIZE:
      g_value_set_int (value, filter->frame_size);
      break;
    case PROP_DATATYPE:
      g_value_set_int (value, filter->frame_type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

/* GstElement vmethod implementations */

/* this function handles the link with other elements */
static gboolean
gst_audio_splitter_set_caps (GstPad * pad, GstCaps * caps)
{
	GstAudioSplitter *filter;
	gboolean result_tx, result_rx;
	GstCaps *newcaps = NULL;
	gchar *str = NULL;

	GST_LOG("setcaps=>");
	filter = GST_AUDIOSPLITTER (gst_pad_get_parent (pad));

	str = gst_caps_to_string(filter->outcaps);
	if (str == NULL) {
		GST_ERROR("gst_caps_to_string(filter->outcaps) failed");
		return FALSE;
	}

	GST_DEBUG("setting caps to srcpads = %s", str);
	result_tx = gst_pad_set_caps(filter->srcpad_tx, filter->outcaps);
	result_rx = gst_pad_set_caps(filter->srcpad_rx, filter->outcaps);

	gst_object_unref (filter);
	g_free(str);
	GST_LOG("setcaps done");
	return (result_tx && result_rx);
}


static GstStateChangeReturn
gst_audio_splitter_change_state(GstElement *element, GstStateChange transition)
{
	GstStateChangeReturn res = GST_FLOW_ERROR;


	switch (transition)
	{
		case GST_STATE_CHANGE_NULL_TO_READY:
			GST_LOG("GST_STATE_CHANGE_NULL_TO_READY");
			break;

		case GST_STATE_CHANGE_READY_TO_PAUSED:
			GST_LOG("GST_STATE_CHANGE_READY_TO_PAUSED");
			break;

		case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
			GST_LOG("GST_STATE_CHANGE_PAUSED_TO_PLAYING");
			break;

		default:
			break;
	}

	res = parent_class->change_state(element, transition);
	if ( res != GST_STATE_CHANGE_SUCCESS )
	{
		GST_ERROR ("chane state error in parent class\n");
		return GST_STATE_CHANGE_FAILURE;
	}

	switch (transition)
	{
		case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
			GST_LOG("GST_STATE_CHANGE_PLAYING_TO_PAUSED");
			break;

		case GST_STATE_CHANGE_PAUSED_TO_READY:
			GST_LOG("GST_STATE_CHANGE_PAUSED_TO_READY");
			break;

		case GST_STATE_CHANGE_READY_TO_NULL:
			GST_LOG("GST_STATE_CHANGE_READY_TO_NULL");
			break;

		default:
			break;
	}

	return res;
}



/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_audio_splitter_chain (GstPad * pad, GstBuffer * buf)
{
  GstAudioSplitter *filter = NULL;
  GstBuffer* rx_buf = NULL;
  GstBuffer* tx_buf = NULL;
  GstBuffer* temp = NULL;
  guint8* inbuff = NULL;
  guint read_bytes = 0;
  guint split_size = 0;
  guint temp_size = 0;
  GstFlowReturn ret = GST_FLOW_OK;

  GST_LOG("IN-BUFFER: TS=%" GST_TIME_FORMAT ", DUR=%" GST_TIME_FORMAT ", SIZE=%d\n",
          GST_TIME_ARGS(GST_BUFFER_TIMESTAMP(buf)),
          GST_TIME_ARGS(GST_BUFFER_DURATION(buf)),
          GST_BUFFER_SIZE(buf));

  filter = GST_AUDIOSPLITTER (GST_OBJECT_PARENT (pad));

  gst_adapter_push(filter->buff_adapter, buf);

  GST_LOG("adapter size = %d", gst_adapter_available(filter->buff_adapter));
  if(gst_adapter_available(filter->buff_adapter) < VOICE_DATA_HEADER_SIZE)
  {
    GST_DEBUG("Need more data");
    ret = GST_FLOW_OK;
    goto chain_exit;
  }

  GST_LOG("adapter size = %d", gst_adapter_available(filter->buff_adapter));
  inbuff = gst_adapter_peek(filter->buff_adapter, VOICE_DATA_HEADER_SIZE);
  if(!inbuff)
  {
    GST_ERROR("Unable to peek data from buff_adapter");
    ret = GST_FLOW_ERROR;
    goto chain_exit;
  }

  read_bytes += VOICE_DATA_HEADER_SIZE;
  filter->frame_size = (*(guint16*)inbuff) - VOICE_DATA_HEADER_SIZE;
  GST_DEBUG("frame_size = %d", filter->frame_size);

  filter->frame_type = (*(guint16*)(inbuff + 2));
  GST_DEBUG("frame_type = %d", filter->frame_type);

  if(gst_adapter_available(filter->buff_adapter) < (filter->frame_size + VOICE_DATA_HEADER_SIZE))
  {
    GST_DEBUG("Need more data");
    ret = GST_FLOW_OK;
    goto chain_exit;
  }

  gst_adapter_flush(filter->buff_adapter, read_bytes);
  read_bytes = 0;

  split_size = filter->frame_size / 2;
  if (filter->frame_type == VOICE_DATATYPE_NARROWBAND) {
    read_bytes = VOICE_DATA_NARROWBAND_DATASIZE;
  } else if (filter->frame_type == VOICE_DATATYPE_WIDEBAND ) {
    read_bytes = VOICE_DATA_WIDEBAND_DATASIZE;
  } else {
    read_bytes = VOICE_DATA_NARROWBAND_DATASIZE;
  }

  GST_LOG_OBJECT(filter, "read_bytes:%d, split_size:%d", read_bytes, split_size);

  if (read_bytes > split_size) {
    read_bytes = split_size;
    GST_WARNING_OBJECT(filter, "read_bytes is bigger than split_size. modify read_bytes[%d]", read_bytes);
  }

  GST_LOG("adapter size = %d", gst_adapter_available(filter->buff_adapter));
  temp = gst_adapter_take_buffer(filter->buff_adapter, split_size);
  if(!temp)
  {
    GST_ERROR("Unable to take buffer[%d bytes] from buff_adapter", split_size);
    ret = GST_FLOW_ERROR;
    goto chain_exit;
  }
  /* copy meaningful data only */
  temp_size = GST_BUFFER_SIZE(temp);
  GST_BUFFER_SIZE(temp) = read_bytes;
  tx_buf = gst_buffer_copy(temp);
  GST_BUFFER_SIZE(temp) = temp_size;
  gst_buffer_unref(temp);

  temp = gst_adapter_take_buffer(filter->buff_adapter, split_size);
  if(!temp)
  {
    GST_ERROR("Unable to take buffer[%d bytes] from buff_adapter", split_size);
    ret = GST_FLOW_ERROR;
    goto chain_exit;
  }
  /* copy meaningful data only */
  temp_size = GST_BUFFER_SIZE(temp);
  GST_BUFFER_SIZE(temp) = read_bytes;
  rx_buf = gst_buffer_copy(temp);
  GST_BUFFER_SIZE(temp) = temp_size;
  gst_buffer_unref(temp);

  GST_BUFFER_TIMESTAMP(tx_buf) = GST_BUFFER_TIMESTAMP(buf);
  GST_BUFFER_TIMESTAMP(rx_buf) = GST_BUFFER_TIMESTAMP(buf);

  gst_buffer_set_caps(tx_buf, filter->outcaps);
  gst_buffer_set_caps(rx_buf, filter->outcaps);

  GST_LOG("pushing tx=%d", GST_BUFFER_SIZE(tx_buf));
  ret = gst_pad_push (filter->srcpad_tx, tx_buf);
  if(ret != GST_FLOW_OK)
  {
    GST_ERROR("Error in pushing tx data\n");
    goto chain_exit;
  }
  GST_LOG("push to TX success", split_size);

  GST_LOG("pushing rx=%d", GST_BUFFER_SIZE(rx_buf));
  ret = gst_pad_push (filter->srcpad_rx, rx_buf);
  if(ret != GST_FLOW_OK)
  {
    GST_ERROR("Error in pushing rx data\n");
    goto chain_exit;
  }
  GST_LOG("push to RX success", split_size);

chain_exit:
  return ret;
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
audiosplitter_init (GstPlugin * audiosplitter)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template audiosplitter' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_audio_splitter_debug, "audiosplitter",
      0, "Template audiosplitter");

  return gst_element_register (audiosplitter, "audiosplitter", GST_RANK_NONE,
      GST_TYPE_AUDIOSPLITTER);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "myfirstaudiosplitter"
#endif

/* gstreamer looks for this structure to register audiosplitters
 *
 * exchange the string 'Template audiosplitter' with your audiosplitter description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "audiosplitter",
    "Template audiosplitter",
    audiosplitter_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
