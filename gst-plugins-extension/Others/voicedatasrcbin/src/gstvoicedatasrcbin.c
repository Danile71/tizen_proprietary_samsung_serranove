/*
 * voicedatasrcbin
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
 * SECTION:element-plugin
 *
 * A src bin for extracting PCM data from VOICEDATA
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m voicedatasrcbin ! filesink location=voicedump.pcm
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "gstvoicedatasrcbin.h"

GST_DEBUG_CATEGORY_STATIC (gst_voicedatasrcbin_debug);
#define GST_CAT_DEFAULT gst_voicedatasrcbin_debug
#define _do_init(bla)   GST_DEBUG_CATEGORY_INIT(GST_CAT_DEFAULT, "voicedatasrcbin", 0, "VOICEDATA source bin"); \
            GST_DEBUG("VOICEDATA source bin is registered");

GST_BOILERPLATE_FULL(GstVoicedataSrcBin, gst_voicedatasrcbin, GstBin, GST_TYPE_BIN, _do_init);

/* Filter signals and args */
enum
{
  /* FILL ME */
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_LOCATION,
  PROP_FD,
  PROP_BLOCKSIZE,
  PROP_DATATYPE
};

#define VOICE_DATATYPE_NARROWBAND 0x01
#define VOICE_DATATYPE_WIDEBAND 0x02

#define VOICE_DATA_HEADER_SIZE 4
#define VOICE_DATA_BLOCK_SIZE_DEFAULT 644

static guint format_signal_id;

static void gst_voicedatasrcbin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_voicedatasrcbin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static GstStateChangeReturn gst_voicedatasrcbin_change_state (GstElement * element,
		    GstStateChange transition);
static void gst_voicedatasrcbin_on_format_detected(GstElement *element, guint format, gpointer data);
/* GObject vmethod implementations */

static void
gst_voicedatasrcbin_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "voicedatasrcbin",
    "source/voicedatasrc",
    "source bin to extract voice call pcm data from VOICEDATA",
    "Prashanth Kumar D <prashanth.kd@samsung.com>");
}

/* initialize the plugin's class */
static void
gst_voicedatasrcbin_class_init (GstVoicedataSrcBinClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;

  gobject_class->set_property = gst_voicedatasrcbin_set_property;
  gobject_class->get_property = gst_voicedatasrcbin_get_property;

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "Location", "Location of the device file",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS |  GST_PARAM_MUTABLE_READY));
  g_object_class_install_property (gobject_class, PROP_FD,
      g_param_spec_int ("fd", "file-descriptor", "file descriptor of an opened file",
          0, G_MAXINT, 0, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class, PROP_BLOCKSIZE,
      g_param_spec_long ("blocksize", "block-size", "Size in bytes to read per buffer",
          1, G_MAXINT, 640, G_PARAM_READWRITE));
  g_object_class_install_property (gobject_class, PROP_DATATYPE,
      g_param_spec_int ("datatype", "data-type", "voice data format: 1=>narrow-band, 2=>wide-band",
	  1, 2, VOICE_DATATYPE_NARROWBAND, G_PARAM_READABLE));

  format_signal_id = g_signal_new("format-detected", G_TYPE_FROM_CLASS (klass), G_SIGNAL_RUN_LAST,
		  		G_STRUCT_OFFSET(GstVoicedataSrcBinClass, format_detected), NULL, NULL,
				g_cclosure_marshal_VOID__UINT, G_TYPE_NONE, 1, G_TYPE_UINT);

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_voicedatasrcbin_change_state);
}

/* initialize the new element
 * instantiate pads and add them to element
 * set pad calback functions
 * initialize instance structure
 */
static void
gst_voicedatasrcbin_init (GstVoicedataSrcBin * voicedatasrcbin,
    GstVoicedataSrcBinClass * gclass)
{
  GstElement *source, *splitter, *adder, *queue1, *queue2;
  GstPad *srcpad, *sinkpad;
  GstCaps *target_caps;
  gchar *str = NULL;
  source   = gst_element_factory_make ("socketsrc", "socket-source");
  splitter = gst_element_factory_make ("audiosplitter", "audio splitter");
  adder = gst_element_factory_make ("adder", "adder");
  queue1 = gst_element_factory_make ("queue", "queue-1");
  queue2 = gst_element_factory_make ("queue", "queue-2");

  if(!source || !splitter || !adder || !queue1 || !queue2)
  {
	  GST_ERROR("Unable to create one of the child elements");
	  goto error_exit;
  }

  GST_LOG("Created child elements: socketsrc, audiosplitter, adder, queue1, queue2");

  voicedatasrcbin->source_element = source;
  voicedatasrcbin->splitter_element = splitter;

  target_caps = gst_caps_new_simple(
		  "audio/x-raw-int",
		  "endianness", G_TYPE_INT, G_BYTE_ORDER,
		  "signed" , G_TYPE_BOOLEAN, TRUE,
		  "width", G_TYPE_INT, 16,
		  "depth" , G_TYPE_INT, 16,
		  "channels" , G_TYPE_INT, 1,
		  "rate", G_TYPE_INT, 8000, NULL);
  if (!target_caps)
  {
	  GST_ERROR("fail to create new target-caps");
	  goto error_exit;
  }

  str = gst_caps_to_string(target_caps);
  if (str == NULL) {
	  GST_ERROR("gst_caps_to_string(target_caps) failed");
	  goto error_exit;
  } else {
	  GST_DEBUG("setting new target caps[%s] to adder", str);
	  g_free(str);
	  str = NULL;
  }
  g_object_set(G_OBJECT(adder), "caps", target_caps, NULL);

  gst_bin_add_many (GST_BIN(voicedatasrcbin), source, splitter, adder, queue1, queue2, NULL);
  gst_element_link (source, splitter);

  sinkpad = gst_element_get_static_pad (queue1, "sink");
  srcpad = gst_element_get_static_pad (splitter, "txsrc");

  if(gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK)
  {
	  GST_ERROR("not able to link queue1 & txsrc");
	  goto error_exit;
  }
  else
	  GST_LOG("linking queue1 & txsrc success");

  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  sinkpad = gst_element_get_static_pad (queue2, "sink");
  srcpad = gst_element_get_static_pad (splitter, "rxsrc");

  if(gst_pad_link (srcpad, sinkpad) != GST_PAD_LINK_OK)
  {
	  GST_ERROR("Not able to link queue2 & rxsrc");
	  goto error_exit;
  }
  else
	  GST_DEBUG("linking queue2 & rxsrc success");

  gst_object_unref (sinkpad);
  gst_object_unref (srcpad);

  gst_element_link (queue1, adder);
  gst_element_link (queue2, adder);

  srcpad = gst_element_get_static_pad (adder, "src");
  voicedatasrcbin->srcpad = gst_ghost_pad_new ("ghost-pad", srcpad);
  gst_object_unref (srcpad);

  gst_pad_set_active (voicedatasrcbin->srcpad, TRUE);
  gst_element_add_pad (GST_ELEMENT (voicedatasrcbin), voicedatasrcbin->srcpad);

  /* Register a signal with splitter plugin to recieve the detected format of the
   * voice data (either Narrow-band/Wide-band) */
  g_signal_connect(splitter, "format-detected", G_CALLBACK(gst_voicedatasrcbin_on_format_detected), voicedatasrcbin);

error_exit:
  voicedatasrcbin->location = NULL;
  voicedatasrcbin->fd = -1;
  voicedatasrcbin->blocksize = VOICE_DATA_BLOCK_SIZE_DEFAULT;
  voicedatasrcbin->datatype = VOICE_DATATYPE_NARROWBAND;
}

static void
gst_voicedatasrcbin_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstVoicedataSrcBin *voicedatasrcbin = GST_VOICEDATASRCBIN (object);

  switch (prop_id) {
    case PROP_LOCATION:

      GST_INFO("setting property \"location\" = %s", g_value_get_string (value));
      voicedatasrcbin->location = g_strdup(g_value_get_string (value));

      voicedatasrcbin->fd = open(voicedatasrcbin->location, O_RDONLY | O_NOCTTY);
      if(voicedatasrcbin->fd < 0)
      {
	      GST_ERROR("error: %d, cannot open device %s \n", voicedatasrcbin->fd, voicedatasrcbin->location);
	      return;
      }

      GST_DEBUG("setting fd-source fd=%d", voicedatasrcbin->fd);
      g_object_set (G_OBJECT (voicedatasrcbin->source_element), "fd", voicedatasrcbin->fd, NULL);
      GST_DEBUG("setting fd-source blocksize=%d", voicedatasrcbin->blocksize);
      g_object_set (G_OBJECT (voicedatasrcbin->source_element), "blocksize", voicedatasrcbin->blocksize, NULL);
      break;

    case PROP_FD:
      GST_INFO("setting property \"fd\" = %d", g_value_get_int(value));
      voicedatasrcbin->fd = g_value_get_int(value);
      g_object_set (G_OBJECT (voicedatasrcbin->source_element), "fd", voicedatasrcbin->fd, NULL);
      GST_DEBUG("setting fd-source blocksize=%d", voicedatasrcbin->blocksize);
      g_object_set (G_OBJECT (voicedatasrcbin->source_element), "blocksize", voicedatasrcbin->blocksize, NULL);
      break;

    case PROP_BLOCKSIZE:
      GST_INFO("setting property \"blocksize\" = %d", g_value_get_long(value));
      voicedatasrcbin->blocksize = g_value_get_long(value);
      GST_DEBUG("setting fd-source blocksize=%d", voicedatasrcbin->blocksize);
      g_object_set (G_OBJECT (voicedatasrcbin->source_element), "blocksize", voicedatasrcbin->blocksize, NULL);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_voicedatasrcbin_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstVoicedataSrcBin *voicedatasrcbin = GST_VOICEDATASRCBIN (object);

  switch (prop_id) {
    case PROP_LOCATION:
      g_value_set_string (value, voicedatasrcbin->location);
      break;
    case PROP_FD:
      g_value_set_int (value, voicedatasrcbin->fd);
      break;
    case PROP_BLOCKSIZE:
      g_value_set_long(value, voicedatasrcbin->blocksize);
      break;
    case PROP_DATATYPE:
      g_value_set_int(value, voicedatasrcbin->datatype);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_voicedatasrcbin_change_state (GstElement * element, GstStateChange transition)
{
  GstStateChangeReturn ret;
  GstVoicedataSrcBin *voicedatasrcbin = GST_VOICEDATASRCBIN (element);
  gint frame_size = 0;

  switch (transition) {
	  case GST_STATE_CHANGE_NULL_TO_READY:
		  GST_LOG("GST_STATE_CHANGE_NULL_TO_READY");
		  break;
	  case GST_STATE_CHANGE_READY_TO_PAUSED:
		  GST_LOG("GST_STATE_CHANGE_READY_TO_PAUSED");
		  break;
	  case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
		  GST_LOG("GST_STATE_CHANGE_PAUSED_TO_PLAYING");
		  g_object_get (G_OBJECT (voicedatasrcbin->splitter_element), "datatype", &voicedatasrcbin->datatype, NULL);
		  GST_DEBUG("Data-type read from splitter=%d", voicedatasrcbin->datatype);
		  g_object_get (G_OBJECT (voicedatasrcbin->splitter_element), "framesize", &frame_size, NULL);
		  GST_DEBUG("Frame-size read from splitter=%d", frame_size);
		  voicedatasrcbin->blocksize = frame_size + VOICE_DATA_HEADER_SIZE;
		  GST_INFO("setting fd-source blocksize=%d", voicedatasrcbin->blocksize);
		  g_object_set (G_OBJECT (voicedatasrcbin->source_element), "blocksize", voicedatasrcbin->blocksize, NULL);
		  break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
	  return ret;

  switch (transition) {
	  case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
		  GST_LOG("GST_STATE_CHANGE_PLAYING_TO_PAUSED");
		  break;
	  case GST_STATE_CHANGE_PAUSED_TO_READY:
		  GST_LOG("GST_STATE_CHANGE_PAUSED_TO_READY");
		  break;
	  case GST_STATE_CHANGE_READY_TO_NULL:
		  GST_LOG("GST_STATE_CHANGE_READY_TO_NULL");
		  break;
  }

  return ret;

}


static void gst_voicedatasrcbin_on_format_detected(GstElement *element, guint format, gpointer data)
{

  GstVoicedataSrcBin *voicedatasrcbin = (GstVoicedataSrcBin*) data;

  GST_INFO("<<Recieved signal from splitter>>");
  GST_DEBUG("format type detected = %d", format);
  GST_DEBUG("Regenerating the signal: \"format-detected\" from here");

  voicedatasrcbin->datatype = format;
  g_signal_emit(voicedatasrcbin, format_signal_id, 0, voicedatasrcbin->datatype);
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
plugin_init (GstPlugin * plugin)
{
  /* debug category for fltering log messages
   *
   * exchange the string 'Template plugin' with your description
   */
  GST_DEBUG_CATEGORY_INIT (gst_voicedatasrcbin_debug, "voicedatasrcbin",
      0, "VOICEDATA source plugin");

  return gst_element_register (plugin, "voicedatasrcbin", GST_RANK_NONE,
      GST_TYPE_VOICEDATASRCBIN);
}

/* gstreamer looks for this structure to register plugins
 *
 * exchange the string 'Template plugin' with your plugin description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "voicedatasrcbin",
    "Template voicedatasrcbin",
    plugin_init,
    VERSION,
    "LGPL",
    "GStreamer",
    "http://gstreamer.net/"
)
