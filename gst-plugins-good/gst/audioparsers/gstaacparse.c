/* GStreamer AAC parser plugin
 * Copyright (C) 2008 Nokia Corporation. All rights reserved.
 *
 * Contact: Stefan Kost <stefan.kost@nokia.com>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:element-aacparse
 * @short_description: AAC parser
 * @see_also: #GstAmrParse
 *
 * This is an AAC parser which handles both ADIF and ADTS stream formats.
 *
 * As ADIF format is not framed, it is not seekable and stream duration cannot
 * be determined either. However, ADTS format AAC clips can be seeked, and parser
 * can also estimate playback position and clip duration.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 filesrc location=abc.aac ! aacparse ! faad ! audioresample ! audioconvert ! alsasink
 * ]|
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>

#include <gst/base/gstbitreader.h>
#include <gst/pbutils/pbutils.h>
#include "gstaacparse.h"

static GstStaticPadTemplate src_template = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg, "
        "framed = (boolean) true, " "mpegversion = (int) { 2, 4 }, "
        "stream-format = (string) { raw, adts, adif, loas };"));

static GstStaticPadTemplate sink_template = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("audio/mpeg, mpegversion = (int) { 2, 4 };"));

#define ADIF_MAX_SIZE 40        /* Should be enough */
#define ADTS_MAX_SIZE 10        /* Should be enough */
#define LOAS_MAX_SIZE 3         /* Should be enough */

#ifdef GST_EXT_AACPARSER_MODIFICATION /* to get more accurate duration */
#define AAC_MAX_ESTIMATE_DURATION_BUF (1024 * 1024) /* use first 1 Mbyte */
#define AAC_SAMPLE_PER_FRAME 1024
#endif


#define ADTS_HEADERS_LENGTH 7UL /* Total byte-length of fixed and variable
                                   headers prepended during raw to ADTS
                                   conversion */

#define AAC_FRAME_DURATION(parse) (GST_SECOND/parse->frames_per_sec)

static const gint loas_sample_rate_table[32] = {
  96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050,
  16000, 12000, 11025, 8000, 7350, 0, 0, 0
};

static const gint loas_channels_table[32] = {
  0, 1, 2, 3, 4, 5, 6, 8,
  0, 0, 0, 0, 0, 0, 0, 0
};

static gboolean gst_aac_parse_start (GstBaseParse * parse);
static gboolean gst_aac_parse_stop (GstBaseParse * parse);

static gboolean gst_aac_parse_sink_setcaps (GstBaseParse * parse,
    GstCaps * caps);
static GstCaps *gst_aac_parse_sink_getcaps (GstBaseParse * parse,
    GstCaps * filter);

static GstFlowReturn gst_aac_parse_handle_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, gint * skipsize);
static GstFlowReturn gst_aac_parse_pre_push_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame);

#ifdef GST_EXT_AACPARSER_MODIFICATION /* make full aac(adts) index table when seek */
  static guint gst_aac_parse_adts_get_fast_frame_len (const guint8 * data);
  static gboolean gst_aac_parse_src_eventfunc(GstBaseParse * parse, GstEvent * event); // evil
  static gboolean gst_aac_parse_adts_src_eventfunc (GstBaseParse * parse, GstEvent * event);
  static gboolean gst_aac_parse_adif_src_eventfunc (GstBaseParse * parse, GstEvent * event); // evil

  int get_aac_parse_get_adts_frame_length (const unsigned char* data, gint64 offset);
  static gboolean gst_aac_parse_estimate_duration (GstBaseParse * parse);

  #define AAC_MAX_PULL_RANGE_BUF  (1 * 1024 * 1024)   /* 1 MByte */
  #define AAC_LARGE_FILE_SIZE     (2 * 1024 * 1024)    /* 2 MByte */
#endif


GST_DEBUG_CATEGORY_STATIC (aacparse_debug);
#define GST_CAT_DEFAULT aacparse_debug

#define _do_init \
    GST_DEBUG_CATEGORY_INIT (aacparse_debug, "aacparse",0, "AAC audio stream parser."); \
    GST_DEBUG("aacparse is registered");
#define gst_aac_parse_parent_class parent_class

G_DEFINE_TYPE_WITH_CODE(GstAacParse, gst_aac_parse, GST_TYPE_BASE_PARSE, _do_init);


#ifdef GST_EXT_AACPARSER_MODIFICATION
static inline gint
gst_aac_parse_get_sample_rate_from_index (guint sr_idx)
{
  static const guint aac_sample_rates[] = { 96000, 88200, 64000, 48000, 44100,
    32000, 24000, 22050, 16000, 12000, 11025, 8000
  };

  if (sr_idx < G_N_ELEMENTS (aac_sample_rates))
    return aac_sample_rates[sr_idx];
  GST_WARNING ("Invalid sample rate index %u", sr_idx);
  return 0;
}
#endif

/**
 * gst_aac_parse_class_init:
 * @klass: #GstAacParseClass.
 *
 */
static void
gst_aac_parse_class_init (GstAacParseClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstBaseParseClass *parse_class = GST_BASE_PARSE_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (aacparse_debug, "aacparse", 0,
      "AAC audio stream parser");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));

  gst_element_class_set_static_metadata (element_class,
      "AAC audio stream parser", "Codec/Parser/Audio",
      "Advanced Audio Coding parser", "Stefan Kost <stefan.kost@nokia.com>");

  parse_class->start = GST_DEBUG_FUNCPTR (gst_aac_parse_start);
  parse_class->stop = GST_DEBUG_FUNCPTR (gst_aac_parse_stop);
  parse_class->set_sink_caps = GST_DEBUG_FUNCPTR (gst_aac_parse_sink_setcaps);
  parse_class->get_sink_caps = GST_DEBUG_FUNCPTR (gst_aac_parse_sink_getcaps);
  parse_class->handle_frame = GST_DEBUG_FUNCPTR (gst_aac_parse_handle_frame);
  parse_class->pre_push_frame =
      GST_DEBUG_FUNCPTR (gst_aac_parse_pre_push_frame);

#ifdef GST_EXT_AACPARSER_MODIFICATION /* make full aac(adts) index table when seek */
  parse_class->src_event = GST_DEBUG_FUNCPTR (gst_aac_parse_src_eventfunc);
#endif
}


/**
 * gst_aac_parse_init:
 * @aacparse: #GstAacParse.
 * @klass: #GstAacParseClass.
 *
 */
static void
gst_aac_parse_init (GstAacParse * aacparse)
{
  GST_DEBUG ("initialized");
  GST_PAD_SET_ACCEPT_INTERSECT (GST_BASE_PARSE_SINK_PAD (aacparse));

#ifdef GST_EXT_AACPARSER_MODIFICATION /* to get more correct duration */
  aacparse->first_frame = TRUE;
#endif
}


/**
 * gst_aac_parse_set_src_caps:
 * @aacparse: #GstAacParse.
 * @sink_caps: (proposed) caps of sink pad
 *
 * Set source pad caps according to current knowledge about the
 * audio stream.
 *
 * Returns: TRUE if caps were successfully set.
 */
static gboolean
gst_aac_parse_set_src_caps (GstAacParse * aacparse, GstCaps * sink_caps)
{
  GstStructure *s;
  GstCaps *src_caps = NULL, *allowed;
  gboolean res = FALSE;
  const gchar *stream_format;
  guint8 codec_data[2];
  guint16 codec_data_data;
  gint sample_rate_idx;

  GST_DEBUG_OBJECT (aacparse, "sink caps: %" GST_PTR_FORMAT, sink_caps);
  if (sink_caps)
    src_caps = gst_caps_copy (sink_caps);
  else
    src_caps = gst_caps_new_empty_simple ("audio/mpeg");

  gst_caps_set_simple (src_caps, "framed", G_TYPE_BOOLEAN, TRUE,
      "mpegversion", G_TYPE_INT, aacparse->mpegversion, NULL);

  aacparse->output_header_type = aacparse->header_type;
  switch (aacparse->header_type) {
    case DSPAAC_HEADER_NONE:
      stream_format = "raw";
      break;
    case DSPAAC_HEADER_ADTS:
      stream_format = "adts";
      break;
    case DSPAAC_HEADER_ADIF:
      stream_format = "adif";
      break;
    case DSPAAC_HEADER_LOAS:
      stream_format = "loas";
      break;
    default:
      stream_format = NULL;
  }

  /* Generate codec data to be able to set profile/level on the caps */
  sample_rate_idx =
      gst_codec_utils_aac_get_index_from_sample_rate (aacparse->sample_rate);
  if (sample_rate_idx < 0)
    goto not_a_known_rate;
  codec_data_data =
      (aacparse->object_type << 11) |
      (sample_rate_idx << 7) | (aacparse->channels << 3);
  GST_WRITE_UINT16_BE (codec_data, codec_data_data);
  gst_codec_utils_aac_caps_set_level_and_profile (src_caps, codec_data, 2);

  s = gst_caps_get_structure (src_caps, 0);
  if (aacparse->sample_rate > 0)
    gst_structure_set (s, "rate", G_TYPE_INT, aacparse->sample_rate, NULL);
  if (aacparse->channels > 0)
    gst_structure_set (s, "channels", G_TYPE_INT, aacparse->channels, NULL);
  if (stream_format)
    gst_structure_set (s, "stream-format", G_TYPE_STRING, stream_format, NULL);

  allowed = gst_pad_get_allowed_caps (GST_BASE_PARSE (aacparse)->srcpad);
  if (allowed && !gst_caps_can_intersect (src_caps, allowed)) {
    GST_DEBUG_OBJECT (GST_BASE_PARSE (aacparse)->srcpad,
        "Caps can not intersect");
    if (aacparse->header_type == DSPAAC_HEADER_ADTS) {
      GST_DEBUG_OBJECT (GST_BASE_PARSE (aacparse)->srcpad,
          "Input is ADTS, trying raw");
      gst_caps_set_simple (src_caps, "stream-format", G_TYPE_STRING, "raw",
          NULL);
      if (gst_caps_can_intersect (src_caps, allowed)) {
        GstBuffer *codec_data_buffer;

        GST_DEBUG_OBJECT (GST_BASE_PARSE (aacparse)->srcpad,
            "Caps can intersect, we will drop the ADTS layer");
        aacparse->output_header_type = DSPAAC_HEADER_NONE;

        /* The codec_data data is according to AudioSpecificConfig,
           ISO/IEC 14496-3, 1.6.2.1 */
        codec_data_buffer = gst_buffer_new_and_alloc (2);
        gst_buffer_fill (codec_data_buffer, 0, codec_data, 2);
        gst_caps_set_simple (src_caps, "codec_data", GST_TYPE_BUFFER,
            codec_data_buffer, NULL);
      }
    } else if (aacparse->header_type == DSPAAC_HEADER_NONE) {
      GST_DEBUG_OBJECT (GST_BASE_PARSE (aacparse)->srcpad,
          "Input is raw, trying ADTS");
      gst_caps_set_simple (src_caps, "stream-format", G_TYPE_STRING, "adts",
          NULL);
      if (gst_caps_can_intersect (src_caps, allowed)) {
        GST_DEBUG_OBJECT (GST_BASE_PARSE (aacparse)->srcpad,
            "Caps can intersect, we will prepend ADTS headers");
        aacparse->output_header_type = DSPAAC_HEADER_ADTS;
      }
    }
  }
  if (allowed)
    gst_caps_unref (allowed);

  GST_DEBUG_OBJECT (aacparse, "setting src caps: %" GST_PTR_FORMAT, src_caps);

  res = gst_pad_set_caps (GST_BASE_PARSE (aacparse)->srcpad, src_caps);
  gst_caps_unref (src_caps);
  return res;

not_a_known_rate:
  GST_ERROR_OBJECT (aacparse, "Not a known sample rate: %d",
      aacparse->sample_rate);
  gst_caps_unref (src_caps);
  return FALSE;
}


/**
 * gst_aac_parse_sink_setcaps:
 * @sinkpad: GstPad
 * @caps: GstCaps
 *
 * Implementation of "set_sink_caps" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_aac_parse_sink_setcaps (GstBaseParse * parse, GstCaps * caps)
{
  GstAacParse *aacparse;
  GstStructure *structure;
  gchar *caps_str;
  const GValue *value;

  aacparse = GST_AAC_PARSE (parse);
  structure = gst_caps_get_structure (caps, 0);
  caps_str = gst_caps_to_string (caps);

  GST_DEBUG_OBJECT (aacparse, "setcaps: %s", caps_str);
  g_free (caps_str);

  /* This is needed at least in case of RTP
   * Parses the codec_data information to get ObjectType,
   * number of channels and samplerate */
  value = gst_structure_get_value (structure, "codec_data");
  if (value) {
    GstBuffer *buf = gst_value_get_buffer (value);

    if (buf) {
      GstMapInfo map;
      guint sr_idx;

      gst_buffer_map (buf, &map, GST_MAP_READ);

      sr_idx = ((map.data[0] & 0x07) << 1) | ((map.data[1] & 0x80) >> 7);
      aacparse->object_type = (map.data[0] & 0xf8) >> 3;
      aacparse->sample_rate =
          gst_codec_utils_aac_get_sample_rate_from_index (sr_idx);
      aacparse->channels = (map.data[1] & 0x78) >> 3;
      aacparse->header_type = DSPAAC_HEADER_NONE;
      aacparse->mpegversion = 4;
      aacparse->frame_samples = (map.data[1] & 4) ? 960 : 1024;
      gst_buffer_unmap (buf, &map);

      GST_DEBUG ("codec_data: object_type=%d, sample_rate=%d, channels=%d, "
          "samples=%d", aacparse->object_type, aacparse->sample_rate,
          aacparse->channels, aacparse->frame_samples);

      /* arrange for metadata and get out of the way */
      gst_aac_parse_set_src_caps (aacparse, caps);
      if (aacparse->header_type == aacparse->output_header_type)
        gst_base_parse_set_passthrough (parse, TRUE);
    } else
      return FALSE;

    /* caps info overrides */
    gst_structure_get_int (structure, "rate", &aacparse->sample_rate);
    gst_structure_get_int (structure, "channels", &aacparse->channels);
  } else {
    aacparse->sample_rate = 0;
    aacparse->channels = 0;
    aacparse->header_type = DSPAAC_HEADER_NOT_PARSED;
    gst_base_parse_set_passthrough (parse, FALSE);
  }

  return TRUE;
}


/**
 * gst_aac_parse_adts_get_frame_len:
 * @data: block of data containing an ADTS header.
 *
 * This function calculates ADTS frame length from the given header.
 *
 * Returns: size of the ADTS frame.
 */
static inline guint
gst_aac_parse_adts_get_frame_len (const guint8 * data)
{
  return ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] & 0xe0) >> 5);
}


/**
 * gst_aac_parse_check_adts_frame:
 * @aacparse: #GstAacParse.
 * @data: Data to be checked.
 * @avail: Amount of data passed.
 * @framesize: If valid ADTS frame was found, this will be set to tell the
 *             found frame size in bytes.
 * @needed_data: If frame was not found, this may be set to tell how much
 *               more data is needed in the next round to detect the frame
 *               reliably. This may happen when a frame header candidate
 *               is found but it cannot be guaranteed to be the header without
 *               peeking the following data.
 *
 * Check if the given data contains contains ADTS frame. The algorithm
 * will examine ADTS frame header and calculate the frame size. Also, another
 * consecutive ADTS frame header need to be present after the found frame.
 * Otherwise the data is not considered as a valid ADTS frame. However, this
 * "extra check" is omitted when EOS has been received. In this case it is
 * enough when data[0] contains a valid ADTS header.
 *
 * This function may set the #needed_data to indicate that a possible frame
 * candidate has been found, but more data (#needed_data bytes) is needed to
 * be absolutely sure. When this situation occurs, FALSE will be returned.
 *
 * When a valid frame is detected, this function will use
 * gst_base_parse_set_min_frame_size() function from #GstBaseParse class
 * to set the needed bytes for next frame.This way next data chunk is already
 * of correct size.
 *
 * Returns: TRUE if the given data contains a valid ADTS header.
 */
static gboolean
gst_aac_parse_check_adts_frame (GstAacParse * aacparse,
    const guint8 * data, const guint avail, gboolean drain,
    guint * framesize, guint * needed_data)
{
  guint crc_size;

  *needed_data = 0;

  /* Absolute minimum to perform the ADTS syncword,
     layer and sampling frequency tests */
  if (G_UNLIKELY (avail < 3)) {
    *needed_data = 3;
    return FALSE;
  }

  /* Syncword and layer tests */
  if ((data[0] == 0xff) && ((data[1] & 0xf6) == 0xf0)) {

    /* Sampling frequency test */
    if (G_UNLIKELY ((data[2] & 0x3C) >> 2 == 15))
      return FALSE;

    /* This looks like an ADTS frame header but
       we need at least 6 bytes to proceed */
    if (G_UNLIKELY (avail < 6)) {
      *needed_data = 6;
      return FALSE;
    }

    *framesize = gst_aac_parse_adts_get_frame_len (data);

    /* If frame has CRC, it needs 2 bytes
       for it at the end of the header */
    crc_size = (data[1] & 0x01) ? 0 : 2;

    /* CRC size test */
    if (*framesize < 7 + crc_size) {
      *needed_data = 7 + crc_size;
      return FALSE;
    }

    /* In EOS mode this is enough. No need to examine the data further.
       We also relax the check when we have sync, on the assumption that
       if we're not looking at random data, we have a much higher chance
       to get the correct sync, and this avoids losing two frames when
       a single bit corruption happens. */
    if (drain || !GST_BASE_PARSE_LOST_SYNC (aacparse)) {
      return TRUE;
    }

    if (*framesize + ADTS_MAX_SIZE > avail) {
      /* We have found a possible frame header candidate, but can't be
         sure since we don't have enough data to check the next frame */
      GST_DEBUG ("NEED MORE DATA: we need %d, available %d",
          *framesize + ADTS_MAX_SIZE, avail);
      *needed_data = *framesize + ADTS_MAX_SIZE;
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
          *framesize + ADTS_MAX_SIZE);
      return FALSE;
    }

    if ((data[*framesize] == 0xff) && ((data[*framesize + 1] & 0xf6) == 0xf0)) {
      guint nextlen = gst_aac_parse_adts_get_frame_len (data + (*framesize));

      GST_LOG ("ADTS frame found, len: %d bytes", *framesize);
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
          nextlen + ADTS_MAX_SIZE);
      return TRUE;
    }
  }
  return FALSE;
}

static gboolean
gst_aac_parse_latm_get_value (GstAacParse * aacparse, GstBitReader * br,
    guint32 * value)
{
  guint8 bytes, i, byte;

  *value = 0;
  if (!gst_bit_reader_get_bits_uint8 (br, &bytes, 2))
    return FALSE;
  for (i = 0; i < bytes; ++i) {
    *value <<= 8;
    if (!gst_bit_reader_get_bits_uint8 (br, &byte, 8))
      return FALSE;
    *value += byte;
  }
  return TRUE;
}

static gboolean
gst_aac_parse_get_audio_object_type (GstAacParse * aacparse, GstBitReader * br,
    guint8 * audio_object_type)
{
  if (!gst_bit_reader_get_bits_uint8 (br, audio_object_type, 5))
    return FALSE;
  if (*audio_object_type == 31) {
    if (!gst_bit_reader_get_bits_uint8 (br, audio_object_type, 6))
      return FALSE;
    *audio_object_type += 32;
  }
  GST_LOG_OBJECT (aacparse, "audio object type %u", *audio_object_type);
  return TRUE;
}

static gboolean
gst_aac_parse_get_audio_sample_rate (GstAacParse * aacparse, GstBitReader * br,
    gint * sample_rate)
{
  guint8 sampling_frequency_index;
  if (!gst_bit_reader_get_bits_uint8 (br, &sampling_frequency_index, 4))
    return FALSE;
  GST_LOG_OBJECT (aacparse, "sampling_frequency_index: %u",
      sampling_frequency_index);
  if (sampling_frequency_index == 0xf) {
    guint32 sampling_rate;
    if (!gst_bit_reader_get_bits_uint32 (br, &sampling_rate, 24))
      return FALSE;
    *sample_rate = sampling_rate;
  } else {
    *sample_rate = loas_sample_rate_table[sampling_frequency_index];
    if (!*sample_rate)
      return FALSE;
  }
  return TRUE;
}

/* See table 1.13 in ISO/IEC 14496-3 */
static gboolean
gst_aac_parse_read_loas_audio_specific_config (GstAacParse * aacparse,
    GstBitReader * br, gint * sample_rate, gint * channels, guint32 * bits)
{
  guint8 audio_object_type, channel_configuration;

  if (!gst_aac_parse_get_audio_object_type (aacparse, br, &audio_object_type))
    return FALSE;

  if (!gst_aac_parse_get_audio_sample_rate (aacparse, br, sample_rate))
    return FALSE;

  if (!gst_bit_reader_get_bits_uint8 (br, &channel_configuration, 4))
    return FALSE;
  GST_LOG_OBJECT (aacparse, "channel_configuration: %d", channel_configuration);
  *channels = loas_channels_table[channel_configuration];
  if (!*channels)
    return FALSE;

  if (audio_object_type == 5) {
    GST_LOG_OBJECT (aacparse,
        "Audio object type 5, so rereading sampling rate...");
    if (!gst_aac_parse_get_audio_sample_rate (aacparse, br, sample_rate))
      return FALSE;
  }

  GST_INFO_OBJECT (aacparse, "Found LOAS config: %d Hz, %d channels",
      *sample_rate, *channels);

  /* There's LOTS of stuff next, but we ignore it for now as we have
     what we want (sample rate and number of channels */
  GST_DEBUG_OBJECT (aacparse,
      "Need more code to parse humongous LOAS data, currently ignored");
  if (bits)
    *bits = 0;
  return TRUE;
}


static gboolean
gst_aac_parse_read_loas_config (GstAacParse * aacparse, const guint8 * data,
    guint avail, gint * sample_rate, gint * channels, gint * version)
{
  GstBitReader br;
  guint8 u8, v, vA;

  /* No version in the bitstream, but the spec has LOAS in the MPEG-4 section */
  if (version)
    *version = 4;

  gst_bit_reader_init (&br, data, avail);

  /* skip sync word (11 bits) and size (13 bits) */
  if (!gst_bit_reader_skip (&br, 11 + 13))
    return FALSE;

  /* First bit is "use last config" */
  if (!gst_bit_reader_get_bits_uint8 (&br, &u8, 1))
    return FALSE;
  if (u8) {
    GST_DEBUG_OBJECT (aacparse, "Frame uses previous config");
    if (!aacparse->sample_rate || !aacparse->channels) {
      GST_WARNING_OBJECT (aacparse, "No previous config to use");
    }
    *sample_rate = aacparse->sample_rate;
    *channels = aacparse->channels;
    return TRUE;
  }

  GST_DEBUG_OBJECT (aacparse, "Frame contains new config");

  if (!gst_bit_reader_get_bits_uint8 (&br, &v, 1))
    return FALSE;
  if (v) {
    if (!gst_bit_reader_get_bits_uint8 (&br, &vA, 1))
      return FALSE;
  } else
    vA = 0;

  GST_LOG_OBJECT (aacparse, "v %d, vA %d", v, vA);
  if (vA == 0) {
    guint8 same_time, subframes, num_program, prog;
    if (v == 1) {
      guint32 value;
      if (!gst_aac_parse_latm_get_value (aacparse, &br, &value))
        return FALSE;
    }
    if (!gst_bit_reader_get_bits_uint8 (&br, &same_time, 1))
      return FALSE;
    if (!gst_bit_reader_get_bits_uint8 (&br, &subframes, 6))
      return FALSE;
    if (!gst_bit_reader_get_bits_uint8 (&br, &num_program, 4))
      return FALSE;
    GST_LOG_OBJECT (aacparse, "same_time %d, subframes %d, num_program %d",
        same_time, subframes, num_program);

    for (prog = 0; prog <= num_program; ++prog) {
      guint8 num_layer, layer;
      if (!gst_bit_reader_get_bits_uint8 (&br, &num_layer, 3))
        return FALSE;
      GST_LOG_OBJECT (aacparse, "Program %d: %d layers", prog, num_layer);

      for (layer = 0; layer <= num_layer; ++layer) {
        guint8 use_same_config;
        if (prog == 0 && layer == 0) {
          use_same_config = 0;
        } else {
          if (!gst_bit_reader_get_bits_uint8 (&br, &use_same_config, 1))
            return FALSE;
        }
        if (!use_same_config) {
          if (v == 0) {
            if (!gst_aac_parse_read_loas_audio_specific_config (aacparse, &br,
                    sample_rate, channels, NULL))
              return FALSE;
          } else {
            guint32 bits, asc_len;
            if (!gst_aac_parse_latm_get_value (aacparse, &br, &asc_len))
              return FALSE;
            if (!gst_aac_parse_read_loas_audio_specific_config (aacparse, &br,
                    sample_rate, channels, &bits))
              return FALSE;
            asc_len -= bits;
            if (!gst_bit_reader_skip (&br, asc_len))
              return FALSE;
          }
        }
      }
    }
    GST_LOG_OBJECT (aacparse, "More data ignored");
  } else {
    GST_WARNING_OBJECT (aacparse, "Spec says \"TBD\"...");
  }
  return TRUE;
}

/**
 * gst_aac_parse_loas_get_frame_len:
 * @data: block of data containing a LOAS header.
 *
 * This function calculates LOAS frame length from the given header.
 *
 * Returns: size of the LOAS frame.
 */
static inline guint
gst_aac_parse_loas_get_frame_len (const guint8 * data)
{
  return (((data[1] & 0x1f) << 8) | data[2]) + 3;
}


/**
 * gst_aac_parse_check_loas_frame:
 * @aacparse: #GstAacParse.
 * @data: Data to be checked.
 * @avail: Amount of data passed.
 * @framesize: If valid LOAS frame was found, this will be set to tell the
 *             found frame size in bytes.
 * @needed_data: If frame was not found, this may be set to tell how much
 *               more data is needed in the next round to detect the frame
 *               reliably. This may happen when a frame header candidate
 *               is found but it cannot be guaranteed to be the header without
 *               peeking the following data.
 *
 * Check if the given data contains contains LOAS frame. The algorithm
 * will examine LOAS frame header and calculate the frame size. Also, another
 * consecutive LOAS frame header need to be present after the found frame.
 * Otherwise the data is not considered as a valid LOAS frame. However, this
 * "extra check" is omitted when EOS has been received. In this case it is
 * enough when data[0] contains a valid LOAS header.
 *
 * This function may set the #needed_data to indicate that a possible frame
 * candidate has been found, but more data (#needed_data bytes) is needed to
 * be absolutely sure. When this situation occurs, FALSE will be returned.
 *
 * When a valid frame is detected, this function will use
 * gst_base_parse_set_min_frame_size() function from #GstBaseParse class
 * to set the needed bytes for next frame.This way next data chunk is already
 * of correct size.
 *
 * LOAS can have three different formats, if I read the spec correctly. Only
 * one of them is supported here, as the two samples I have use this one.
 *
 * Returns: TRUE if the given data contains a valid LOAS header.
 */
static gboolean
gst_aac_parse_check_loas_frame (GstAacParse * aacparse,
    const guint8 * data, const guint avail, gboolean drain,
    guint * framesize, guint * needed_data)
{
  *needed_data = 0;

  /* 3 byte header */
  if (G_UNLIKELY (avail < 3)) {
    *needed_data = 3;
    return FALSE;
  }

  if ((data[0] == 0x56) && ((data[1] & 0xe0) == 0xe0)) {
    *framesize = gst_aac_parse_loas_get_frame_len (data);
    GST_DEBUG_OBJECT (aacparse, "Found %u byte LOAS frame", *framesize);

    /* In EOS mode this is enough. No need to examine the data further.
       We also relax the check when we have sync, on the assumption that
       if we're not looking at random data, we have a much higher chance
       to get the correct sync, and this avoids losing two frames when
       a single bit corruption happens. */
    if (drain || !GST_BASE_PARSE_LOST_SYNC (aacparse)) {
      return TRUE;
    }

    if (*framesize + LOAS_MAX_SIZE > avail) {
      /* We have found a possible frame header candidate, but can't be
         sure since we don't have enough data to check the next frame */
      GST_DEBUG ("NEED MORE DATA: we need %d, available %d",
          *framesize + LOAS_MAX_SIZE, avail);
      *needed_data = *framesize + LOAS_MAX_SIZE;
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
          *framesize + LOAS_MAX_SIZE);
      return FALSE;
    }

    if ((data[*framesize] == 0x56) && ((data[*framesize + 1] & 0xe0) == 0xe0)) {
      guint nextlen = gst_aac_parse_loas_get_frame_len (data + (*framesize));

      GST_LOG ("LOAS frame found, len: %d bytes", *framesize);
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
          nextlen + LOAS_MAX_SIZE);
      return TRUE;
    }
  }
  return FALSE;
}

/* caller ensure sufficient data */
static inline void
gst_aac_parse_parse_adts_header (GstAacParse * aacparse, const guint8 * data,
    gint * rate, gint * channels, gint * object, gint * version)
{

  if (rate) {
    gint sr_idx = (data[2] & 0x3c) >> 2;

    *rate = gst_codec_utils_aac_get_sample_rate_from_index (sr_idx);
  }
  if (channels)
    *channels = ((data[2] & 0x01) << 2) | ((data[3] & 0xc0) >> 6);

  if (version)
    *version = (data[1] & 0x08) ? 2 : 4;
  if (object)
    *object = ((data[2] & 0xc0) >> 6) + 1;
}

/**
 * gst_aac_parse_detect_stream:
 * @aacparse: #GstAacParse.
 * @data: A block of data that needs to be examined for stream characteristics.
 * @avail: Size of the given datablock.
 * @framesize: If valid stream was found, this will be set to tell the
 *             first frame size in bytes.
 * @skipsize: If valid stream was found, this will be set to tell the first
 *            audio frame position within the given data.
 *
 * Examines the given piece of data and try to detect the format of it. It
 * checks for "ADIF" header (in the beginning of the clip) and ADTS frame
 * header. If the stream is detected, TRUE will be returned and #framesize
 * is set to indicate the found frame size. Additionally, #skipsize might
 * be set to indicate the number of bytes that need to be skipped, a.k.a. the
 * position of the frame inside given data chunk.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_aac_parse_detect_stream (GstAacParse * aacparse,
    const guint8 * data, const guint avail, gboolean drain,
    guint * framesize, gint * skipsize)
{
  gboolean found = FALSE;
  guint need_data_adts = 0, need_data_loas;
  guint i = 0;

  GST_DEBUG_OBJECT (aacparse, "Parsing header data");

  /* FIXME: No need to check for ADIF if we are not in the beginning of the
     stream */

  /* Can we even parse the header? */
  if (avail < MAX (ADTS_MAX_SIZE, LOAS_MAX_SIZE)) {
    GST_DEBUG_OBJECT (aacparse, "Not enough data to check");
    return FALSE;
  }

  for (i = 0; i < avail - 4; i++) {
    if (((data[i] == 0xff) && ((data[i + 1] & 0xf6) == 0xf0)) ||
        ((data[0] == 0x56) && ((data[1] & 0xe0) == 0xe0)) ||
        strncmp ((char *) data + i, "ADIF", 4) == 0) {
      GST_DEBUG_OBJECT (aacparse, "Found signature at offset %u", i);
      found = TRUE;

      if (i) {
        /* Trick: tell the parent class that we didn't find the frame yet,
           but make it skip 'i' amount of bytes. Next time we arrive
           here we have full frame in the beginning of the data. */
        *skipsize = i;
        return FALSE;
      }
      break;
    }
  }
  if (!found) {
    if (i)
      *skipsize = i;
    return FALSE;
  }

  if (gst_aac_parse_check_adts_frame (aacparse, data, avail, drain,
          framesize, &need_data_adts)) {
    gint rate, channels;

    GST_INFO ("ADTS ID: %d, framesize: %d", (data[1] & 0x08) >> 3, *framesize);

    gst_aac_parse_parse_adts_header (aacparse, data, &rate, &channels,
        &aacparse->object_type, &aacparse->mpegversion);

    if (!channels || !framesize) {
      GST_DEBUG_OBJECT (aacparse, "impossible ADTS configuration");
      return FALSE;
    }

    aacparse->header_type = DSPAAC_HEADER_ADTS;
    gst_base_parse_set_frame_rate (GST_BASE_PARSE (aacparse), rate,
        aacparse->frame_samples, 2, 2);

    GST_DEBUG ("ADTS: samplerate %d, channels %d, objtype %d, version %d",
        rate, channels, aacparse->object_type, aacparse->mpegversion);

    gst_base_parse_set_syncable (GST_BASE_PARSE (aacparse), TRUE);

    return TRUE;
  }

  if (gst_aac_parse_check_loas_frame (aacparse, data, avail, drain,
          framesize, &need_data_loas)) {
    gint rate, channels;

    GST_INFO ("LOAS, framesize: %d", *framesize);

    aacparse->header_type = DSPAAC_HEADER_LOAS;

    if (!gst_aac_parse_read_loas_config (aacparse, data, avail, &rate,
            &channels, &aacparse->mpegversion)) {
      GST_WARNING_OBJECT (aacparse, "Error reading LOAS config");
      return FALSE;
    }

    if (rate && channels) {
      gst_base_parse_set_frame_rate (GST_BASE_PARSE (aacparse), rate,
          aacparse->frame_samples, 2, 2);

      GST_DEBUG ("LOAS: samplerate %d, channels %d, objtype %d, version %d",
          rate, channels, aacparse->object_type, aacparse->mpegversion);
      aacparse->sample_rate = rate;
      aacparse->channels = channels;
    }

    gst_base_parse_set_syncable (GST_BASE_PARSE (aacparse), TRUE);

    return TRUE;
  }

  if (need_data_adts || need_data_loas) {
    /* This tells the parent class not to skip any data */
    *skipsize = 0;
    return FALSE;
  }

  if (avail < ADIF_MAX_SIZE)
    return FALSE;

  if (memcmp (data + i, "ADIF", 4) == 0) {
    const guint8 *adif;
    int skip_size = 0;
    int bitstream_type;
    int sr_idx;
    GstCaps *sinkcaps;

    aacparse->header_type = DSPAAC_HEADER_ADIF;
    aacparse->mpegversion = 4;

    /* Skip the "ADIF" bytes */
    adif = data + i + 4;

    /* copyright string */
    if (adif[0] & 0x80)
      skip_size += 9;           /* skip 9 bytes */

    bitstream_type = adif[0 + skip_size] & 0x10;
    aacparse->bitrate =
        ((unsigned int) (adif[0 + skip_size] & 0x0f) << 19) |
        ((unsigned int) adif[1 + skip_size] << 11) |
        ((unsigned int) adif[2 + skip_size] << 3) |
        ((unsigned int) adif[3 + skip_size] & 0xe0);

    /* CBR */
    if (bitstream_type == 0) {
#if 0
      /* Buffer fullness parsing. Currently not needed... */
      guint num_elems = 0;
      guint fullness = 0;

      num_elems = (adif[3 + skip_size] & 0x1e);
      GST_INFO ("ADIF num_config_elems: %d", num_elems);

      fullness = ((unsigned int) (adif[3 + skip_size] & 0x01) << 19) |
          ((unsigned int) adif[4 + skip_size] << 11) |
          ((unsigned int) adif[5 + skip_size] << 3) |
          ((unsigned int) (adif[6 + skip_size] & 0xe0) >> 5);

      GST_INFO ("ADIF buffer fullness: %d", fullness);
#endif
      aacparse->object_type = ((adif[6 + skip_size] & 0x01) << 1) |
          ((adif[7 + skip_size] & 0x80) >> 7);
      sr_idx = (adif[7 + skip_size] & 0x78) >> 3;
    }
    /* VBR */
    else {
      aacparse->object_type = (adif[4 + skip_size] & 0x18) >> 3;
      sr_idx = ((adif[4 + skip_size] & 0x07) << 1) |
          ((adif[5 + skip_size] & 0x80) >> 7);
    }

    /* FIXME: This gives totally wrong results. Duration calculation cannot
       be based on this */
    aacparse->sample_rate =
        gst_codec_utils_aac_get_sample_rate_from_index (sr_idx);

    /* baseparse is not given any fps,
     * so it will give up on timestamps, seeking, etc */

    /* FIXME: Can we assume this? */
    aacparse->channels = 2;

    GST_INFO ("ADIF: br=%d, samplerate=%d, objtype=%d",
        aacparse->bitrate, aacparse->sample_rate, aacparse->object_type);

    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse), 512);

    /* arrange for metadata and get out of the way */
    sinkcaps = gst_pad_get_current_caps (GST_BASE_PARSE_SINK_PAD (aacparse));
    gst_aac_parse_set_src_caps (aacparse, sinkcaps);
    if (sinkcaps)
      gst_caps_unref (sinkcaps);

    /* not syncable, not easily seekable (unless we push data from start */
    gst_base_parse_set_syncable (GST_BASE_PARSE_CAST (aacparse), FALSE);
    gst_base_parse_set_passthrough (GST_BASE_PARSE_CAST (aacparse), TRUE);
    gst_base_parse_set_average_bitrate (GST_BASE_PARSE_CAST (aacparse), aacparse->bitrate);

    *framesize = avail;
    return TRUE;
  }

  /* This should never happen */
  return FALSE;
}

/**
 * gst_aac_parse_get_audio_profile_object_type
 * @aacparse: #GstAacParse.
 *
 * Gets the MPEG-2 profile or the MPEG-4 object type value corresponding to the
 * mpegversion and profile of @aacparse's src pad caps, according to the
 * values defined by table 1.A.11 in ISO/IEC 14496-3.
 *
 * Returns: the profile or object type value corresponding to @aacparse's src
 * pad caps, if such a value exists; otherwise G_MAXUINT8.
 */
static guint8
gst_aac_parse_get_audio_profile_object_type (GstAacParse * aacparse)
{
  GstCaps *srccaps;
  GstStructure *srcstruct;
  const gchar *profile;
  guint8 ret;

  srccaps = gst_pad_get_current_caps (GST_BASE_PARSE_SRC_PAD (aacparse));
  srcstruct = gst_caps_get_structure (srccaps, 0);
  profile = gst_structure_get_string (srcstruct, "profile");
  if (G_UNLIKELY (profile == NULL)) {
    gst_caps_unref (srccaps);
    return G_MAXUINT8;
  }

  if (g_strcmp0 (profile, "main") == 0) {
    ret = (guint8) 0U;
  } else if (g_strcmp0 (profile, "lc") == 0) {
    ret = (guint8) 1U;
  } else if (g_strcmp0 (profile, "ssr") == 0) {
    ret = (guint8) 2U;
  } else if (g_strcmp0 (profile, "ltp") == 0) {
    if (G_LIKELY (aacparse->mpegversion == 4))
      ret = (guint8) 3U;
    else
      ret = G_MAXUINT8;         /* LTP Object Type allowed only for MPEG-4 */
  } else {
    ret = G_MAXUINT8;
  }

  gst_caps_unref (srccaps);
  return ret;
}

/**
 * gst_aac_parse_get_audio_channel_configuration
 * @num_channels: number of audio channels.
 *
 * Gets the Channel Configuration value, as defined by table 1.19 in ISO/IEC
 * 14496-3, for a given number of audio channels.
 *
 * Returns: the Channel Configuration value corresponding to @num_channels, if
 * such a value exists; otherwise G_MAXUINT8.
 */
static guint8
gst_aac_parse_get_audio_channel_configuration (gint num_channels)
{
  if (num_channels >= 1 && num_channels <= 6)   /* Mono up to & including 5.1 */
    return (guint8) num_channels;
  else if (num_channels == 8)   /* 7.1 */
    return (guint8) 7U;
  else
    return G_MAXUINT8;
}

/**
 * gst_aac_parse_get_audio_sampling_frequency_index:
 * @sample_rate: audio sampling rate.
 *
 * Gets the Sampling Frequency Index value, as defined by table 1.18 in ISO/IEC
 * 14496-3, for a given sampling rate.
 *
 * Returns: the Sampling Frequency Index value corresponding to @sample_rate,
 * if such a value exists; otherwise G_MAXUINT8.
 */
static guint8
gst_aac_parse_get_audio_sampling_frequency_index (gint sample_rate)
{
  switch (sample_rate) {
    case 96000:
      return 0x0U;
    case 88200:
      return 0x1U;
    case 64000:
      return 0x2U;
    case 48000:
      return 0x3U;
    case 44100:
      return 0x4U;
    case 32000:
      return 0x5U;
    case 24000:
      return 0x6U;
    case 22050:
      return 0x7U;
    case 16000:
      return 0x8U;
    case 12000:
      return 0x9U;
    case 11025:
      return 0xAU;
    case 8000:
      return 0xBU;
    case 7350:
      return 0xCU;
    default:
      return G_MAXUINT8;
  }
}

/**
 * gst_aac_parse_prepend_adts_headers:
 * @aacparse: #GstAacParse.
 * @frame: raw AAC frame to which ADTS headers shall be prepended.
 *
 * Prepends ADTS headers to a raw AAC audio frame.
 *
 * Returns: TRUE if ADTS headers were successfully prepended; FALSE otherwise.
 */
static gboolean
gst_aac_parse_prepend_adts_headers (GstAacParse * aacparse,
    GstBaseParseFrame * frame)
{
  GstMemory *mem;
  guint8 *adts_headers;
  gsize buf_size;
  gsize frame_size;
  guint8 id, profile, channel_configuration, sampling_frequency_index;

  id = (aacparse->mpegversion == 4) ? 0x0U : 0x1U;
  profile = gst_aac_parse_get_audio_profile_object_type (aacparse);
  if (profile == G_MAXUINT8) {
    GST_ERROR_OBJECT (aacparse, "Unsupported audio profile or object type");
    return FALSE;
  }
  channel_configuration =
      gst_aac_parse_get_audio_channel_configuration (aacparse->channels);
  if (channel_configuration == G_MAXUINT8) {
    GST_ERROR_OBJECT (aacparse, "Unsupported number of channels");
    return FALSE;
  }
  sampling_frequency_index =
      gst_aac_parse_get_audio_sampling_frequency_index (aacparse->sample_rate);
  if (sampling_frequency_index == G_MAXUINT8) {
    GST_ERROR_OBJECT (aacparse, "Unsupported sampling frequency");
    return FALSE;
  }

  frame->out_buffer = gst_buffer_copy (frame->buffer);
  buf_size = gst_buffer_get_size (frame->out_buffer);
  frame_size = buf_size + ADTS_HEADERS_LENGTH;

  if (G_UNLIKELY (frame_size >= 0x4000)) {
    GST_ERROR_OBJECT (aacparse, "Frame size is too big for ADTS");
    return FALSE;
  }

  adts_headers = (guint8 *) g_malloc0 (ADTS_HEADERS_LENGTH);

  /* Note: no error correction bits are added to the resulting ADTS frames */
  adts_headers[0] = 0xFFU;
  adts_headers[1] = 0xF0U | (id << 3) | 0x1U;
  adts_headers[2] = (profile << 6) | (sampling_frequency_index << 2) | 0x2U |
      (channel_configuration & 0x4U);
  adts_headers[3] = ((channel_configuration & 0x3U) << 6) | 0x30U |
      (guint8) (frame_size >> 11);
  adts_headers[4] = (guint8) ((frame_size >> 3) & 0x00FF);
  adts_headers[5] = (guint8) (((frame_size & 0x0007) << 5) + 0x1FU);
  adts_headers[6] = 0xFCU;

  mem = gst_memory_new_wrapped (0, adts_headers, ADTS_HEADERS_LENGTH, 0,
      ADTS_HEADERS_LENGTH, adts_headers, g_free);
  gst_buffer_prepend_memory (frame->out_buffer, mem);

  return TRUE;
}

/**
 * gst_aac_parse_check_valid_frame:
 * @parse: #GstBaseParse.
 * @frame: #GstBaseParseFrame.
 * @skipsize: How much data parent class should skip in order to find the
 *            frame header.
 *
 * Implementation of "handle_frame" vmethod in #GstBaseParse class.
 *
 * Also determines frame overhead.
 * ADTS streams have a 7 byte header in each frame. MP4 and ADIF streams don't have
 * a per-frame header. LOAS has 3 bytes.
 *
 * We're making a couple of simplifying assumptions:
 *
 * 1. We count Program Configuration Elements rather than searching for them
 *    in the streams to discount them - the overhead is negligible.
 *
 * 2. We ignore CRC. This has a worst-case impact of (num_raw_blocks + 1)*16
 *    bits, which should still not be significant enough to warrant the
 *    additional parsing through the headers
 *
 * Returns: a #GstFlowReturn.
 */
static GstFlowReturn
gst_aac_parse_handle_frame (GstBaseParse * parse,
    GstBaseParseFrame * frame, gint * skipsize)
{
  GstMapInfo map;
  GstAacParse *aacparse;
  gboolean ret = FALSE;
  gboolean lost_sync;
  GstBuffer *buffer;
  guint framesize;
  gint rate, channels;

  aacparse = GST_AAC_PARSE (parse);
  buffer = frame->buffer;

  gst_buffer_map (buffer, &map, GST_MAP_READ);

  *skipsize = -1;
  lost_sync = GST_BASE_PARSE_LOST_SYNC (parse);

  if (aacparse->header_type == DSPAAC_HEADER_ADIF ||
      aacparse->header_type == DSPAAC_HEADER_NONE) {
    /* There is nothing to parse */
    framesize = map.size;
    ret = TRUE;

  } else if (aacparse->header_type == DSPAAC_HEADER_NOT_PARSED || lost_sync) {

    ret = gst_aac_parse_detect_stream (aacparse, map.data, map.size,
        GST_BASE_PARSE_DRAINING (parse), &framesize, skipsize);

  } else if (aacparse->header_type == DSPAAC_HEADER_ADTS) {
    guint needed_data = 1024;

    ret = gst_aac_parse_check_adts_frame (aacparse, map.data, map.size,
        GST_BASE_PARSE_DRAINING (parse), &framesize, &needed_data);

    if (!ret && needed_data) {
      GST_DEBUG ("buffer didn't contain valid frame");
      *skipsize = 0;
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
          needed_data);
    }

  } else if (aacparse->header_type == DSPAAC_HEADER_LOAS) {
    guint needed_data = 1024;

    ret = gst_aac_parse_check_loas_frame (aacparse, map.data,
        map.size, GST_BASE_PARSE_DRAINING (parse), &framesize, &needed_data);

    if (!ret && needed_data) {
      GST_DEBUG ("buffer didn't contain valid frame");
      *skipsize = 0;
      gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
          needed_data);
    }

  } else {
    GST_DEBUG ("buffer didn't contain valid frame");
    gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse),
        ADTS_MAX_SIZE);
  }

  if (G_UNLIKELY (!ret))
    goto exit;

  if (aacparse->header_type == DSPAAC_HEADER_ADTS) {
    /* see above */
    frame->overhead = 7;

    gst_aac_parse_parse_adts_header (aacparse, map.data,
        &rate, &channels, NULL, NULL);

    GST_LOG_OBJECT (aacparse, "rate: %d, chans: %d", rate, channels);

    if (G_UNLIKELY (rate != aacparse->sample_rate
            || channels != aacparse->channels)) {
      aacparse->sample_rate = rate;
      aacparse->channels = channels;

#ifdef GST_EXT_AACPARSER_MODIFICATION
    if (!gst_aac_parse_set_src_caps (aacparse,
            gst_pad_get_current_caps (GST_BASE_PARSE_SINK_PAD (aacparse)))) {
      /* If linking fails, we need to return appropriate error */
      ret = GST_FLOW_NOT_LINKED;
    }
#else
      if (!gst_aac_parse_set_src_caps (aacparse, NULL)) {
        /* If linking fails, we need to return appropriate error */
        ret = GST_FLOW_NOT_LINKED;
      }
#endif
      gst_base_parse_set_frame_rate (GST_BASE_PARSE (aacparse),
          aacparse->sample_rate, aacparse->frame_samples, 2, 2);
    }

#ifdef GST_EXT_AACPARSER_MODIFICATION /* to get more correct duration */
    if (aacparse->first_frame == TRUE) {
      gboolean ret = FALSE;
      gint64 upstream_total_bytes = 0;
      GstFormat fmt = GST_FORMAT_BYTES;

      /* obtain real upstream total bytes */
      if (!gst_pad_peer_query_duration (parse->sinkpad, fmt, &upstream_total_bytes))
        upstream_total_bytes = 0;

      GST_INFO_OBJECT (aacparse, "gst_pad_query_peer_duration -upstream_total_bytes (%"G_GUINT64_FORMAT")", upstream_total_bytes);
      aacparse->file_size = upstream_total_bytes;
      aacparse->first_frame = FALSE;

      ret = gst_aac_parse_estimate_duration(parse);
      if (!ret) {
        GST_WARNING_OBJECT (aacparse, "can not estimate total duration");
        ret = GST_FLOW_NOT_SUPPORTED;
      }
    }
#endif
  } else if (aacparse->header_type == DSPAAC_HEADER_LOAS) {
    gboolean setcaps = FALSE;

    /* see above */
    frame->overhead = 3;

    if (!gst_aac_parse_read_loas_config (aacparse, map.data, map.size, &rate,
            &channels, NULL)) {
      GST_WARNING_OBJECT (aacparse, "Error reading LOAS config");
    } else if (G_UNLIKELY (rate != aacparse->sample_rate
            || channels != aacparse->channels)) {
      aacparse->sample_rate = rate;
      aacparse->channels = channels;
      setcaps = TRUE;
      GST_INFO_OBJECT (aacparse, "New LOAS config: %d Hz, %d channels", rate,
          channels);
    }

    /* We want to set caps both at start, and when rate/channels change.
       Since only some LOAS frames have that info, we may receive frames
       before knowing about rate/channels. */
    if (setcaps
        || !gst_pad_has_current_caps (GST_BASE_PARSE_SRC_PAD (aacparse))) {
      if (!gst_aac_parse_set_src_caps (aacparse, NULL)) {
        /* If linking fails, we need to return appropriate error */
        ret = GST_FLOW_NOT_LINKED;
      }

      gst_base_parse_set_frame_rate (GST_BASE_PARSE (aacparse),
          aacparse->sample_rate, aacparse->frame_samples, 2, 2);
    }
  }
#ifdef GST_EXT_AACPARSER_MODIFICATION /* to get more correct duration */
  else if(aacparse->header_type == DSPAAC_HEADER_ADIF)
  {
      int file_size = 0;
      float estimated_duration = 0;
      gint64 total_file_size;

      gst_base_parse_get_upstream_size(parse, &total_file_size);

      estimated_duration = ((total_file_size * 8) / (float)(aacparse->bitrate * 1000))* GST_SECOND;

      gst_base_parse_set_duration (parse, GST_FORMAT_TIME, estimated_duration * 1000, 0);
  }
#endif

  if (aacparse->header_type == DSPAAC_HEADER_NONE
      && aacparse->output_header_type == DSPAAC_HEADER_ADTS) {
    if (!gst_aac_parse_prepend_adts_headers (aacparse, frame)) {
      GST_ERROR_OBJECT (aacparse, "Failed to prepend ADTS headers to frame");
      ret = GST_FLOW_ERROR;
    }
  }

exit:
  gst_buffer_unmap (buffer, &map);

  if (ret) {
    /* found, skip if needed */
    if (*skipsize > 0)
      return GST_FLOW_OK;
    *skipsize = 0;
  } else {
    if (*skipsize < 0)
      *skipsize = 1;
  }

  if (ret && framesize <= map.size) {
    return gst_base_parse_finish_frame (parse, frame, framesize);
  }

  return GST_FLOW_OK;
}

static GstFlowReturn
gst_aac_parse_pre_push_frame (GstBaseParse * parse, GstBaseParseFrame * frame)
{
  GstAacParse *aacparse = GST_AAC_PARSE (parse);

  if (!aacparse->sent_codec_tag) {
    GstTagList *taglist;
    GstCaps *caps;

    taglist = gst_tag_list_new_empty ();

    /* codec tag */
    caps = gst_pad_get_current_caps (GST_BASE_PARSE_SRC_PAD (parse));
    gst_pb_utils_add_codec_description_to_tag_list (taglist,
        GST_TAG_AUDIO_CODEC, caps);
    gst_caps_unref (caps);

    gst_pad_push_event (GST_BASE_PARSE_SRC_PAD (aacparse),
        gst_event_new_tag (taglist));

    /* also signals the end of first-frame processing */
    aacparse->sent_codec_tag = TRUE;
  }

  /* As a special case, we can remove the ADTS framing and output raw AAC. */
  if (aacparse->header_type == DSPAAC_HEADER_ADTS
      && aacparse->output_header_type == DSPAAC_HEADER_NONE) {
    guint header_size;
    GstMapInfo map;
    gst_buffer_map (frame->buffer, &map, GST_MAP_READ);
    header_size = (map.data[1] & 1) ? 7 : 9;    /* optional CRC */
    gst_buffer_unmap (frame->buffer, &map);
    gst_buffer_resize (frame->buffer, header_size,
        gst_buffer_get_size (frame->buffer) - header_size);
  }

  return GST_FLOW_OK;
}


/**
 * gst_aac_parse_start:
 * @parse: #GstBaseParse.
 *
 * Implementation of "start" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE if startup succeeded.
 */
static gboolean
gst_aac_parse_start (GstBaseParse * parse)
{
  GstAacParse *aacparse;

  aacparse = GST_AAC_PARSE (parse);
  GST_DEBUG ("start");
  aacparse->frame_samples = 1024;
  gst_base_parse_set_min_frame_size (GST_BASE_PARSE (aacparse), ADTS_MAX_SIZE);
  aacparse->sent_codec_tag = FALSE;
  return TRUE;
}


/**
 * gst_aac_parse_stop:
 * @parse: #GstBaseParse.
 *
 * Implementation of "stop" vmethod in #GstBaseParse class.
 *
 * Returns: TRUE is stopping succeeded.
 */
static gboolean
gst_aac_parse_stop (GstBaseParse * parse)
{
  GST_DEBUG ("stop");
  return TRUE;
}

static void
remove_fields (GstCaps * caps)
{
  guint i, n;

  n = gst_caps_get_size (caps);
  for (i = 0; i < n; i++) {
    GstStructure *s = gst_caps_get_structure (caps, i);

    gst_structure_remove_field (s, "framed");
  }
}

static void
add_conversion_fields (GstCaps * caps)
{
  guint i, n;

  n = gst_caps_get_size (caps);
  for (i = 0; i < n; i++) {
    GstStructure *s = gst_caps_get_structure (caps, i);

    if (gst_structure_has_field (s, "stream-format")) {
      const GValue *v = gst_structure_get_value (s, "stream-format");

      if (G_VALUE_HOLDS_STRING (v)) {
        const gchar *str = g_value_get_string (v);

        if (strcmp (str, "adts") == 0 || strcmp (str, "raw") == 0) {
          GValue va = G_VALUE_INIT;
          GValue vs = G_VALUE_INIT;

          g_value_init (&va, GST_TYPE_LIST);
          g_value_init (&vs, G_TYPE_STRING);
          g_value_set_string (&vs, "adts");
          gst_value_list_append_value (&va, &vs);
          g_value_set_string (&vs, "raw");
          gst_value_list_append_value (&va, &vs);
          gst_structure_set_value (s, "stream-format", &va);
          g_value_unset (&va);
          g_value_unset (&vs);
        }
      } else if (GST_VALUE_HOLDS_LIST (v)) {
        gboolean contains_raw = FALSE;
        gboolean contains_adts = FALSE;
        guint m = gst_value_list_get_size (v), j;

        for (j = 0; j < m; j++) {
          const GValue *ve = gst_value_list_get_value (v, j);
          const gchar *str;

          if (G_VALUE_HOLDS_STRING (ve) && (str = g_value_get_string (ve))) {
            if (strcmp (str, "adts") == 0)
              contains_adts = TRUE;
            else if (strcmp (str, "raw") == 0)
              contains_raw = TRUE;
          }
        }

        if (contains_adts || contains_raw) {
          GValue va = G_VALUE_INIT;
          GValue vs = G_VALUE_INIT;

          g_value_init (&va, GST_TYPE_LIST);
          g_value_init (&vs, G_TYPE_STRING);
          g_value_copy (v, &va);

          if (!contains_raw) {
            g_value_set_string (&vs, "raw");
            gst_value_list_append_value (&va, &vs);
          }
          if (!contains_adts) {
            g_value_set_string (&vs, "adts");
            gst_value_list_append_value (&va, &vs);
          }

          gst_structure_set_value (s, "stream-format", &va);

          g_value_unset (&vs);
          g_value_unset (&va);
        }
      }
    }
  }
}

static GstCaps *
gst_aac_parse_sink_getcaps (GstBaseParse * parse, GstCaps * filter)
{
  GstCaps *peercaps, *templ;
  GstCaps *res;

  templ = gst_pad_get_pad_template_caps (GST_BASE_PARSE_SINK_PAD (parse));

  if (filter) {
    GstCaps *fcopy = gst_caps_copy (filter);
    /* Remove the fields we convert */
    remove_fields (fcopy);
    add_conversion_fields (fcopy);
    peercaps = gst_pad_peer_query_caps (GST_BASE_PARSE_SRC_PAD (parse), fcopy);
    gst_caps_unref (fcopy);
  } else
    peercaps = gst_pad_peer_query_caps (GST_BASE_PARSE_SRC_PAD (parse), NULL);

  if (peercaps) {
    peercaps = gst_caps_make_writable (peercaps);
    /* Remove the fields we convert */
    remove_fields (peercaps);
    add_conversion_fields (peercaps);

    res = gst_caps_intersect_full (peercaps, templ, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (peercaps);
    gst_caps_unref (templ);
  } else {
    res = templ;
  }

  if (filter) {
    GstCaps *intersection;

    intersection =
        gst_caps_intersect_full (filter, res, GST_CAPS_INTERSECT_FIRST);
    gst_caps_unref (res);
    res = intersection;
  }

  return res;
}

#ifdef GST_EXT_AACPARSER_MODIFICATION /* to get more correct duration */
/**
 * get_aac_parse_get_adts_framelength:
 * @data: #GstBufferData.
 * @offset: #GstBufferData offset
 *
 * Implementation to get adts framelength by using first some frame.
 *
 * Returns: frame size
 */
int get_aac_parse_get_adts_frame_length (const unsigned char* data, gint64 offset)
{
  const gint adts_header_length_no_crc = 7;
  const gint adts_header_length_with_crc = 9;
  gint frame_size = 0;
  gint protection_absent;
  gint head_size;

  /* check of syncword */
  if ((data[offset+0] != 0xff) || ((data[offset+1] & 0xf6) != 0xf0)) {
    GST_ERROR("check sync word is fail\n");
    return  -1;
  }

  /* check of protection absent */
  protection_absent = (data[offset+1] & 0x01);

  /*check of frame length */
  frame_size = (data[offset+3] & 0x3) << 11 | data[offset+4] << 3 | data[offset+5] >> 5;

  /* check of header size */
  /* protectionAbsent is 0 if there is CRC */
  head_size = protection_absent ? adts_header_length_no_crc : adts_header_length_with_crc;
  if (head_size > frame_size) {
    GST_ERROR("return frame length as 0 (frameSize %u < headSize %u)", frame_size, head_size);
    return 0;
  }

  return frame_size;
}

/**
 * gst_aac_parse_estimate_duration:
 * @parse: #GstBaseParse.
 *
 * Implementation to get estimated total duration by using first some frame.
 *
 * Returns: TRUE if we can get estimated total duraion
 */
static gboolean
gst_aac_parse_estimate_duration (GstBaseParse * parse)
{
  GstFlowReturn res = GST_FLOW_OK;
  gint64 pull_size = 0, file_size = 0, offset = 0, num_frames=0, duration=0;
  guint profile = 0, sample_rate_index = 0, sample_rate = 0, channel = 0;
  guint frame_size = 0, frame_duration_us = 0, estimated_bitrate = 0;
  guint  lost_sync_count=0;
  GstClockTime estimated_duration = GST_CLOCK_TIME_NONE;
  GstBuffer *buffer = NULL;
  guint8 *buf = NULL;
  gint i = 0;
  GstPadMode pad_mode = GST_PAD_MODE_NONE;
  GstAacParse *aacparse;
  gint64 buffer_size = 0;

  aacparse = GST_AAC_PARSE (parse);
  GST_LOG_OBJECT (aacparse, "gst_aac_parse_estimate_duration enter");

#ifdef GST_EXT_BASEPARSER_MODIFICATION /* check baseparse define these fuction */
  gst_base_parse_get_pad_mode(parse, &pad_mode);
  if (pad_mode != GST_PAD_MODE_PULL) {
    GST_INFO_OBJECT (aacparse, "aac parser is not pull mode. can not estimate duration");
    return FALSE;
  }

  gst_base_parse_get_upstream_size (parse, &file_size);
#else
  GST_WARNING_OBJECT (aacparse, "baseparser does not define get private param functions");
  return FALSE;
#endif

  if (file_size < ADIF_MAX_SIZE) {
    GST_ERROR_OBJECT (aacparse, "file size is too short");
    return FALSE;
  }

  pull_size = MIN(file_size, AAC_MAX_ESTIMATE_DURATION_BUF);

  res = gst_pad_pull_range (parse->sinkpad, 0, pull_size, &buffer);
  if (res != GST_FLOW_OK) {
    GST_ERROR_OBJECT (aacparse, "gst_pad_pull_range failed!");
    return FALSE;
  }

  GstMapInfo map;
  gst_buffer_map(buffer, &map, GST_MAP_READ);
  buf = map.data;
  buffer_size = map.size;
  if(buffer_size != pull_size)
  {
    GST_ERROR_OBJECT(aacparse, "We got different buffer_size(%d) with pull_size(%d).", buffer_size, pull_size);
  }
  /*MODIFICATION : add defence codes for real buffer_size is different with pull_size*/
  //for (i = 0; i < pull_size; i ++) {
  for (i = 0; i < buffer_size; i ++) {
    if ((buf[i] == 0xff) && ((buf[i+1] & 0xf6) == 0xf0)) { /* aac sync word */
      profile = (buf[i+2] >> 6) & 0x3;
      sample_rate_index = (buf[i+2] >> 2) & 0xf;
      sample_rate = gst_aac_parse_get_sample_rate_from_index(sample_rate_index);
      if (sample_rate == 0) {
          gst_buffer_unmap(buffer, &map);
          GST_WARNING_OBJECT (aacparse, "Invalid sample rate index (0)");
          return FALSE;
      }
      channel = (buf[i+2] & 0x1) << 2 | (buf[i+3] >> 6);

      GST_INFO_OBJECT (aacparse, "found sync. aac sample_rate=%d, channel=%d", sample_rate, channel);

      /* count number of frames */
      /*MODIFICATION : add defence codes for real buffer_size is different with pull_size*/
      //while (offset < pull_size) {
      while (offset < buffer_size) {
        frame_size = get_aac_parse_get_adts_frame_length(buf, i + offset);
        if (frame_size  == 0) {
          GST_ERROR_OBJECT (aacparse, "framesize error at offset %"G_GINT64_FORMAT, offset);
          break;
        } else if (frame_size == -1) {
          offset++;
          lost_sync_count++;    //  lost sync count limmitation 2K Bytes
          if (lost_sync_count > (1024*2))
          {
            gst_buffer_unmap(buffer, &map);
            return FALSE;
          }
        } else {
          offset += frame_size;
          num_frames++;
          lost_sync_count=0;
        }
      } /* while */

      /* if we can got full file, we can calculate the accurate duration */
      /*MODIFICATION : add defence codes for real buffer_size is different with pull_size*/
      //if (pull_size == file_size) {
      if (buffer_size == file_size) {
        gfloat duration_for_one_frame = 0;
        GstClockTime calculated_duration = GST_CLOCK_TIME_NONE;

        GST_INFO_OBJECT (aacparse, "we got total file (%d bytes). do not estimate but make Accurate total duration.", pull_size);

        duration_for_one_frame = (gfloat)AAC_SAMPLE_PER_FRAME / (gfloat)sample_rate;
        calculated_duration = num_frames * duration_for_one_frame * 1000 * 1000 * 1000;

        GST_INFO_OBJECT (aacparse, "duration_for_one_frame %f ms", duration_for_one_frame);
        GST_INFO_OBJECT (aacparse, "calculated duration = %"GST_TIME_FORMAT, GST_TIME_ARGS(calculated_duration));
        gst_base_parse_set_duration (parse, GST_FORMAT_TIME, calculated_duration, 0); /* 0 means disable estimate */

      } else {
        GST_INFO_OBJECT (aacparse, "we got %d bytes in total file (%"G_GINT64_FORMAT
            "). can not make accurate duration but Estimate.", pull_size, file_size);
        frame_duration_us = (1024 * 1000000ll + (sample_rate - 1)) / sample_rate;
        duration = num_frames * frame_duration_us;

        estimated_bitrate = (gint)((gfloat)(offset * 8) / (gfloat)(duration / 1000));
        estimated_duration = (GstClockTime)((file_size * 8) / (estimated_bitrate * 1000)) * GST_SECOND;

        GST_INFO_OBJECT (aacparse, "number of frame = %"G_GINT64_FORMAT, num_frames);
        GST_INFO_OBJECT (aacparse, "duration = %"G_GINT64_FORMAT, duration / 1000000);
        GST_INFO_OBJECT (aacparse, "byte = %"G_GINT64_FORMAT, offset);
        GST_INFO_OBJECT (aacparse, "estimated bitrate = %d bps", estimated_bitrate);
        GST_INFO_OBJECT (aacparse, "estimated duration = %"GST_TIME_FORMAT, GST_TIME_ARGS(estimated_duration));

        gst_base_parse_set_average_bitrate (parse, estimated_bitrate * 1000);
        /* set update_interval as duration(sec)/2 */
        gst_base_parse_set_duration (parse, GST_FORMAT_TIME, estimated_duration, (gint)(duration/2));
      }

      break;
    }
  }

  gst_buffer_unmap(buffer, &map);
  gst_buffer_unref (buffer);
  return TRUE;
}
#endif

#ifdef GST_EXT_AACPARSER_MODIFICATION
/* perform seek in push based mode:
   find BYTE position to move to based on time and delegate to upstream
*/
static gboolean
gst_aac_audio_parse_do_push_seek (GstBaseParse * parse, GstPad * pad, GstEvent * event)
{
  GstAacParse *aacparse;
  aacparse = GST_AAC_PARSE(parse);

  gdouble rate;
  GstFormat format;
  GstSeekFlags flags;
  GstSeekType cur_type, stop_type;
  gint64 cur, stop;
  gboolean res;
  gint64 byte_cur;
  gint64 esimate_byte;
  gint32 frame_dur;

  GST_INFO_OBJECT (parse, "doing aac push-based seek");

  gst_event_parse_seek (event, &rate, &format, &flags, &cur_type, &cur, &stop_type, &stop);

  /* FIXME, always play to the end */
  stop = -1;

  /* only forward streaming and seeking is possible */
  if (rate <= 0)
    goto unsupported_seek;

  if ( cur == 0 ) {
    /* handle rewind only */
    cur_type = GST_SEEK_TYPE_SET;
    byte_cur = 0;
    stop_type = GST_SEEK_TYPE_NONE;
    stop = -1;
    flags |= GST_SEEK_FLAG_FLUSH;
  } else {
    /* handle normal seek */
    cur_type = GST_SEEK_TYPE_SET;
    stop_type = GST_SEEK_TYPE_NONE;
    stop = -1;
    flags |= GST_SEEK_FLAG_FLUSH;

    esimate_byte = (cur / (1000 * 1000)) * aacparse->frame_byte;
    if (aacparse->sample_rate> 0)
      frame_dur = (aacparse->spf * 1000) / aacparse->sample_rate;
    else
      goto unsupported_seek;
    if (frame_dur > 0)
      byte_cur =  esimate_byte / (frame_dur);
    else
      goto unsupported_seek;

    GST_INFO_OBJECT(parse, "frame_byte(%d) spf(%d)  rate (%d) ", aacparse->frame_byte, aacparse->spf, aacparse->sample_rate);
    GST_INFO_OBJECT(parse, "seek cur (%"G_GINT64_FORMAT") = (%"GST_TIME_FORMAT") ", cur, GST_TIME_ARGS (cur));
    GST_INFO_OBJECT(parse, "esimate_byte(%"G_GINT64_FORMAT")  esimate_byte (%d)", esimate_byte, frame_dur );
  }

  if ( (byte_cur == -1) || (byte_cur > aacparse->file_size))
  {
    GST_INFO_OBJECT(parse, "[WEB-ERROR] seek cur (%"G_GINT64_FORMAT") > file_size (%"G_GINT64_FORMAT") ", cur, aacparse->file_size );
    goto abort_seek;
  }

  GST_INFO_OBJECT (parse, "Pushing BYTE seek rate %g, "  "start %" G_GINT64_FORMAT ", stop %" G_GINT64_FORMAT, rate, byte_cur,  stop);

  if (!(flags & GST_SEEK_FLAG_KEY_UNIT)) {
    GST_INFO_OBJECT (parse, "Requested seek time: %" GST_TIME_FORMAT ", calculated seek offset: %" G_GUINT64_FORMAT, GST_TIME_ARGS (cur), byte_cur);
  }

  /* BYTE seek event */
  event = gst_event_new_seek (rate, GST_FORMAT_BYTES, flags, cur_type, byte_cur, stop_type, stop);
  res = gst_pad_push_event (parse->sinkpad, event);

  return res;

  /* ERRORS */

abort_seek:
  {
    GST_DEBUG_OBJECT (parse, "could not determine byte position to seek to, " "seek aborted.");
    return FALSE;
  }

unsupported_seek:
  {
    GST_DEBUG_OBJECT (parse, "unsupported seek, seek aborted.");
    return FALSE;
  }
}


static guint
gst_aac_parse_adts_get_fast_frame_len (const guint8 * data)
{
  int length;
  if ((data[0] == 0xff) && ((data[1] & 0xf6) == 0xf0)) {
    length = ((data[3] & 0x03) << 11) | (data[4] << 3) | ((data[5] & 0xe0) >> 5);
  } else {
    length = 0;
  }
  return length;
}

// evil
static gboolean
gst_aac_parse_src_eventfunc(GstBaseParse * parse, GstEvent * event)
{
  gboolean  handled = FALSE;
  GstAacParse *aacparse;
  aacparse = GST_AAC_PARSE(parse);

  GST_DEBUG("Entering gst_aac_parse_src_eventfunc header type = %d", aacparse->header_type);
  if(aacparse->header_type == DSPAAC_HEADER_ADIF)
    return gst_aac_parse_adif_src_eventfunc(parse, event);
  else if(aacparse->header_type == DSPAAC_HEADER_ADTS)
    return gst_aac_parse_adts_src_eventfunc (parse, event);
  else
    goto aac_seek_null_exit;
aac_seek_null_exit:

  /* call baseparse src_event function to handle event */
  handled = GST_BASE_PARSE_CLASS (parent_class)->src_event (parse, event);
  return handled;
}

/**
 * gst_aac_parse_adts_src_eventfunc:
 * @parse: #GstBaseParse. #event
 *
 * before baseparse handles seek event, make full amr index table.
 *
 * Returns: TRUE on success.
 */
static gboolean
gst_aac_parse_adts_src_eventfunc (GstBaseParse * parse, GstEvent * event)
{
  gboolean handled = FALSE;
  GstAacParse *aacparse;
  aacparse = GST_AAC_PARSE (parse);

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
      {
        GstFlowReturn res = GST_FLOW_OK;
        gint64 base_offset = 0, sync_offset = 0, cur = 0;
        gint32 frame_count = 1;    /* do not add first frame because it is already in index table */
        gint64 second_count = 0;   /* initial 1 second */
        gint64 total_file_size = 0, start_offset = 0;
        GstClockTime current_ts = GST_CLOCK_TIME_NONE;
        GstPadMode pad_mode = GST_PAD_MODE_NONE;
       gboolean large_file_flag = FALSE;

#ifdef GST_EXT_BASEPARSER_MODIFICATION /* check baseparse define these fuction */
        gst_base_parse_get_pad_mode(parse, &pad_mode);
        if (pad_mode != GST_PAD_MODE_PULL && aacparse->file_size) {
          gboolean ret = FALSE;
          GST_INFO_OBJECT (aacparse, "aac parser is PUSH MODE.");
          GstPad* srcpad = parse->srcpad;
          /* check NULL */
          ret = gst_aac_audio_parse_do_push_seek(parse, srcpad, event);
          gst_object_unref(srcpad);
          return ret;
        } else if (!aacparse->file_size) {
          GST_INFO_OBJECT (aacparse, "Seek will be handled by demuxer");
          break;
        }
        gst_base_parse_get_upstream_size(parse, &total_file_size);
        gst_base_parse_get_index_last_offset(parse, &start_offset);
        gst_base_parse_get_index_last_ts(parse, &current_ts);
#else
        GST_ERROR("baseparser does not define get private param functions. can not make index table here.");
        break;
#endif

        if (total_file_size > AAC_LARGE_FILE_SIZE ) {
           large_file_flag = TRUE;
          gst_base_parse_set_seek_mode(parse, 0);
          GST_INFO_OBJECT (aacparse, "larger than big size (2MB).");
          goto aac_seek_null_exit;
        }

        GST_DEBUG("gst_aac_parse_adts_src_eventfunc GST_EVENT_SEEK enter");

        if (total_file_size == 0 || start_offset >= total_file_size) {
          GST_ERROR("last index offset %d is larger than file size %d", start_offset, total_file_size);
          break;
        }

        gst_event_parse_seek (event, NULL, NULL, NULL, NULL, &cur, NULL, NULL);
        if (cur <= current_ts) {
          GST_INFO("seek to %"GST_TIME_FORMAT" within index table %"GST_TIME_FORMAT". do not make index table",
              GST_TIME_ARGS(cur), GST_TIME_ARGS(current_ts));
          break;
        } else {
          GST_INFO("seek to %"GST_TIME_FORMAT" without index table %"GST_TIME_FORMAT". make index table",
              GST_TIME_ARGS(cur), GST_TIME_ARGS(current_ts));
        }

        GST_INFO("make AAC(ADTS) Index Table. file_size  = %"G_GINT64_FORMAT" last idx offset=%"G_GINT64_FORMAT
            ", last idx ts=%"GST_TIME_FORMAT, total_file_size, start_offset, GST_TIME_ARGS(current_ts));

        base_offset = start_offset; /* set base by start offset */
        second_count = current_ts + GST_SECOND; /* 1sec */

        /************************************/
        /* STEP 0: Setting parse information */
        /************************************/
          aacparse->spf = aacparse->frame_samples;
          aacparse->frame_duration = (aacparse->spf * 1000 * 100) / aacparse->sample_rate;  /* duration per frame (msec) */
          aacparse->frame_per_sec = (aacparse->sample_rate) / aacparse->spf;          /* frames per second (ea) */

        /************************************/
        /* STEP 1: MAX_PULL_RANGE_BUF cycle */
        /************************************/
        while (total_file_size - base_offset >= AAC_MAX_PULL_RANGE_BUF) {
          gint64 offset = 0;
          GstBuffer *buffer = NULL;
          guint8 *buf = NULL;

         GST_INFO("gst_pad_pull_range %d bytes (from %"G_GINT64_FORMAT") use max size", AAC_MAX_PULL_RANGE_BUF, base_offset);
          res = gst_pad_pull_range (parse->sinkpad, base_offset, base_offset + AAC_MAX_PULL_RANGE_BUF, &buffer);
          if (res != GST_FLOW_OK) {
            GST_ERROR ("gst_pad_pull_range failed!");
            break;
          }

          GstMapInfo map;
          gst_buffer_map(buffer, &map, GST_MAP_READ);
          buf = map.data;
          if (buf == NULL) {
            gst_buffer_unmap(buffer, &map);
            GST_WARNING("buffer is NULL in make aac seek table's STEP1");
            gst_buffer_unref (buffer);
            goto aac_seek_null_exit;
          }

          while (offset <= AAC_MAX_PULL_RANGE_BUF) {
            gint frame_size = 0;
            guint32 header;

            /* make sure the values in the frame header look sane */
            frame_size = gst_aac_parse_adts_get_fast_frame_len (buf);

            if ((frame_size > 0) && (frame_size < (AAC_MAX_PULL_RANGE_BUF - offset))) {
              if (current_ts > second_count)  { /* 1 sec == xx frames. we make idx per sec */
                gst_base_parse_add_index_entry (parse, base_offset +offset, current_ts, TRUE, TRUE); /* force */
                GST_DEBUG("Adding  index ts=%"GST_TIME_FORMAT" offset %"G_GINT64_FORMAT,
                    GST_TIME_ARGS(current_ts), base_offset + offset);
                second_count += GST_SECOND;    /* 1sec */
              }

              current_ts += (aacparse->frame_duration * GST_MSECOND) / 100; /* each frame is (frame_duration) ms */
              offset += frame_size;
              buf += frame_size;
              frame_count++;
            } else if (frame_size >= (AAC_MAX_PULL_RANGE_BUF - offset)) {
              GST_DEBUG("we need refill buffer");
              break;
            } else {
              GST_WARNING("we lost sync");
              buf++;
              offset++;
            }
          } /* while */

          base_offset = base_offset + offset;

          gst_buffer_unmap(buffer, &map);
          gst_buffer_unref (buffer);
        } /* end MAX buffer cycle */

        /*******************************/
        /* STEP 2: Remain Buffer cycle */
        /*******************************/
        if (total_file_size - base_offset > 0) {
          gint64 offset = 0;
          GstBuffer *buffer = NULL;
          guint8 *buf = NULL;

          GST_INFO("gst_pad_pull_range %"G_GINT64_FORMAT" bytes (from %"G_GINT64_FORMAT") use remain_buf size",
              total_file_size - base_offset, base_offset);
          res = gst_pad_pull_range (parse->sinkpad, base_offset, total_file_size, &buffer);
          if (res != GST_FLOW_OK) {
            GST_ERROR ("gst_pad_pull_range failed!");
            break;
          }

          GstMapInfo map;
          gst_buffer_map(buffer, &map, GST_MAP_READ);
          buf = map.data;
          if (buf == NULL) {
            gst_buffer_unmap(buffer, &map);
            GST_WARNING("buffer is NULL in make aac seek table's STEP2");
            gst_buffer_unref (buffer);
            goto aac_seek_null_exit;
          }

          while (base_offset + offset < total_file_size) {
            gint frame_size = 0;
            guint32 header;

            /* make sure the values in the frame header look sane */
            frame_size = gst_aac_parse_adts_get_fast_frame_len (buf);

            if ((frame_size > 0) && (frame_size <= (total_file_size - (base_offset + offset)))) {
              if (current_ts > second_count) {  /* 1 sec == xx frames. we make idx per sec */
                gst_base_parse_add_index_entry (parse, base_offset +offset, current_ts, TRUE, TRUE); /* force */
                GST_DEBUG("Adding  index ts=%"GST_TIME_FORMAT" offset %"G_GINT64_FORMAT,
                    GST_TIME_ARGS(current_ts), base_offset + offset);
                second_count += GST_SECOND;    /* 1sec */
              }

              current_ts += (aacparse->frame_duration * GST_MSECOND) / 100; /* each frame is (frame_duration) ms */
              offset += frame_size;
              buf += frame_size;
              frame_count++;
            } else if (frame_size == 0) {
              GST_DEBUG("Frame size is 0 so, Decoding end..");
              break;
            } else {
              GST_WARNING("we lost sync");
              buf++;
              offset++;
            }
          } /* while */

          gst_buffer_unmap(buffer, &map);
          gst_buffer_unref (buffer);
        } /* end remain_buf buffer cycle */

        GST_DEBUG("gst_aac_parse_adts_src_eventfunc GST_EVENT_SEEK leave");
      }
     break;

    default:
      break;
  }

aac_seek_null_exit:

  /* call baseparse src_event function to handle event */
  handled = GST_BASE_PARSE_CLASS (parent_class)->src_event (parse, event);

  return handled;
}

// evil (ADIF SEEK)
// Enter this function if the detected format is adif.
static gboolean
gst_aac_parse_adif_src_eventfunc (GstBaseParse * parse, GstEvent * event)
{
  // Case 1: Constant bit rate => Find the byte boundary using bit rate. Add these values as indices using gst_base_parse_add_index_entry.
  // Case 2: Variable bit rate => Need to parse the whole file here itself to find frame boundaries.
  //gboolean handled = FALSE;
  gint64 cur = 0;
  int offset = 0;
  gboolean handled = FALSE;
  GstBuffer* buffer = NULL;
  const guint8 * data;

  GstAacParse *aacparse;
  aacparse = GST_AAC_PARSE(parse);

  GST_DEBUG("gst_aac_parse_adif_src_eventfunc enter");

  aacparse->frame_duration = (aacparse->frame_samples* 1000 * 100) / aacparse->sample_rate;  /* duration per frame (msec) */

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_SEEK:
    {
      int base_offset = aacparse->read_bytes;
      int framesize = 0, frame_duration_us = 0, duration = 0, estimated_bitrate, estimated_duration;
      GstFlowReturn res = GST_FLOW_OK;
      static int framecount = 0;
      GstClockTime current_ts = GST_CLOCK_TIME_NONE;

      GST_LOG_OBJECT(aacparse, "ADIF: Let us Handle seek event");

      // Parse the event and get the timestamp where to seek.
      //gst_event_parse_seek (event, NULL, NULL, NULL, NULL, &cur, NULL, NULL);

      res = gst_pad_pull_range (parse->sinkpad, base_offset, aacparse->file_size, &buffer);

      if (res != GST_FLOW_OK) {
        GST_ERROR ("gst_pad_pull_range failed!");
        break;
      }

      GstMapInfo map;
      gst_buffer_map(buffer, &map, GST_MAP_READ);
      data = map.data;
      if (data == NULL) {
        gst_buffer_unmap(buffer, &map);
        GST_WARNING("buffer is NULL in make aac seek table's STEP2");
        gst_buffer_unref (buffer);
        goto aac_seek_null_exit;
      }
      offset = base_offset;
      while(offset <= aacparse->file_size) {
        gst_base_parse_add_index_entry (parse, offset, current_ts, TRUE, TRUE); /* force */
        framesize = ((int)data[2] << 8) + data[3] + 4;
        offset += framesize;
        data += framesize;
        framecount++;
        current_ts += (aacparse->frame_duration * GST_MSECOND) / 100; /* each frame is (frame_duration) ms */
      }
      gst_base_parse_set_frame_rate (GST_BASE_PARSE (aacparse), aacparse->sample_rate, 1024, 2, 2);
      gst_buffer_unmap(buffer, &map);
      gst_buffer_unref (buffer);
      break;
    }
    default:
      break;
  }
aac_seek_null_exit:

  /* call baseparse src_event function to handle event */
  handled = GST_BASE_PARSE_CLASS (parent_class)->src_event (parse, event);

  return handled;
}

#endif //end of #ifdef GST_EXT_AACPARSER_MODIFICATION
