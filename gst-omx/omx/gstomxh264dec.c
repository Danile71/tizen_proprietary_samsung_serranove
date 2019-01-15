/*
 * Copyright (C) 2011, Hewlett-Packard Development Company, L.P.
 *   Author: Sebastian Dröge <sebastian.droege@collabora.co.uk>, Collabora Ltd.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation
 * version 2.1 of the License.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>

#include "gstomxh264dec.h"

GST_DEBUG_CATEGORY_STATIC (gst_omx_h264_dec_debug_category);
#define GST_CAT_DEFAULT gst_omx_h264_dec_debug_category

/* prototypes */
static gboolean gst_omx_h264_dec_is_format_change (GstOMXVideoDec * dec, GstOMXPort * port, GstVideoCodecState * state);
static gboolean gst_omx_h264_dec_set_format (GstOMXVideoDec * dec, GstOMXPort * port, GstVideoCodecState * state);
static GstFlowReturn gst_omx_h264_dec_prepare_frame (GstOMXVideoDec * self, GstVideoCodecFrame *frame);

enum
{
  PROP_0
};

/* class initialization */

#define DEBUG_INIT \
  GST_DEBUG_CATEGORY_INIT (gst_omx_h264_dec_debug_category, "omxh264dec", 0, \
      "debug category for gst-omx video decoder base class");

G_DEFINE_TYPE_WITH_CODE (GstOMXH264Dec, gst_omx_h264_dec, GST_TYPE_OMX_VIDEO_DEC, DEBUG_INIT);



/*
 **
 **  Description : Function used to handle packetized h264 header
 *#  params      : @dec : GstOMXH264Dec, @buf: codec_data in sink setcaps
 **  return      : return TRUE on sucess and FALSE on failure
 **  Comments    : 1. get codec_data from setcaps from sinkpad
 **                2. converting sps, pps from packetized format to byte-stream format
 **
 */
static gboolean
gst_omx_convert_dci (GstOMXH264Dec *dec, GstBuffer *buf, GstBuffer **dci_nalu)
{
  gboolean ret = FALSE;
  unsigned char *out = NULL;
  unsigned int unitSize = 0;
  unsigned int totalSize = 0;
  unsigned char unitNb = 0;
  unsigned char spsDone = 0;
  GstMapInfo map;

  gst_buffer_map(buf, &map, GST_MAP_READ);
  unsigned char *pInputStream = map.data;
  unsigned int pBuffSize = map.size;

  const unsigned char *extraData = (unsigned char *)pInputStream + 4;
  static const unsigned char naluHeader[GST_OMX_H264_NAL_START_LEN] = {0, 0, 0, 1};

  if (pInputStream != NULL) {
      /* retrieve Length of Length*/
      dec->h264_nal_length_size = (*extraData++ & 0x03) + 1;
       GST_INFO("Length Of Length is %d", dec->h264_nal_length_size);
      if (dec->h264_nal_length_size == 3)
      {
          GST_INFO("LengthOfLength is WRONG...");
          goto EXIT;
      }
     /* retrieve sps and pps unit(s) */
      unitNb = *extraData++ & 0x1f;
      GST_INFO("No. of SPS units = %u", unitNb);
      if (!unitNb) {
          GST_INFO("SPS is not present...");
          goto EXIT;
      }

      while (unitNb--) {
          /* get SPS/PPS data Length*/
          unitSize = GST_OMX_H264_RB16(extraData);

          /* Extra 4 bytes for adding delimiter */
          totalSize += unitSize + GST_OMX_H264_NAL_START_LEN;

          /* Check if SPS/PPS Data Length crossed buffer Length */
          if ((extraData + 2 + unitSize) > (pInputStream + pBuffSize)) {
              GST_INFO("SPS length is wrong in DCI...");
              goto EXIT;
          }

          if (out)
            out = g_realloc(out, totalSize);
          else
            out = g_malloc(totalSize);

          if (!out) {
              GST_INFO("realloc failed...");
              goto EXIT;
          }

          /* Copy NALU header */
          memcpy(out + totalSize - unitSize - GST_OMX_H264_NAL_START_LEN, naluHeader, GST_OMX_H264_NAL_START_LEN);

          /* Copy SPS/PPS Length and data */
          memcpy(out + totalSize - unitSize, extraData + GST_OMX_H264_SPSPPS_LEN, unitSize);

          extraData += (GST_OMX_H264_SPSPPS_LEN + unitSize);

          if (!unitNb && !spsDone++)
          {
              /* Completed reading SPS data, now read PPS data */
              unitNb = *extraData++; /* number of pps unit(s) */
              GST_INFO( "No. of PPS units = %d", unitNb);
          }
      }

      *dci_nalu = gst_buffer_new ();
      if (*dci_nalu == NULL) {
          GST_ERROR_OBJECT(dec, " gst_buffer_new_and_alloc failed...\n");
          goto EXIT;
      }
      gst_buffer_append_memory (*dci_nalu, gst_memory_new_wrapped (0, out, totalSize, 0, totalSize, out, g_free));

      GST_INFO( "Total SPS+PPS size = %d", totalSize);
  }

  ret = TRUE;

EXIT:
  gst_buffer_unmap(buf, &map);
  return ret;
}



/*
 *  description : find stream format(3gpp or nalu)
 *  params      : @dec : GstOMXH264Dec, @buf: input gstbuffer in pad_chain
 *  return      : none
 *  comments    : finding whether the stream format of input buf is 3GPP or Elementary Stream(nalu)
 */
static void
check_frame (GstOMXH264Dec *dec , GstBuffer * buf)
{
  guint buf_size = 0;
  guint8 *buf_data = NULL;
  guint nal_type = 0;
  guint forbidden_zero_bit = 0;
  guint check_byte= 0;
  GstMapInfo map;

  gst_buffer_map(buf, &map, GST_MAP_READ);
  buf_data = map.data;
  buf_size = map.size;

  if (buf_data == NULL || buf_size < GST_OMX_H264_NAL_START_LEN + 1) {
    gst_buffer_unmap(buf, &map);
    dec->h264_format = GST_OMX_H264_FORMAT_UNKNOWN;
    GST_WARNING_OBJECT(dec, "H264 format is unknown");
    return;
  }

  dec->h264_format = GST_OMX_H264_FORMAT_PACKETIZED;

  if ((buf_data[0] == 0x00)&&(buf_data[1] == 0x00)&&
    ((buf_data[2] == 0x01)||((buf_data[2] == 0x00)&&(buf_data[3] == 0x01)))) {
    check_byte = (buf_data[2] == 0x01) ? 3 : 4;

    nal_type = (guint)(buf_data[check_byte] & 0x1f);
    forbidden_zero_bit = (guint)(buf_data[check_byte] & 0x80);
    GST_LOG_OBJECT(dec, "check first frame: nal_type=%d, forbidden_zero_bit=%d", nal_type, forbidden_zero_bit);

    if (forbidden_zero_bit == 0) {
      /* check nal_unit_type is invaild value: ex) slice, DPA,DPB... */
      if ((0 < nal_type && nal_type <= 15) || nal_type == 19 || nal_type == 20) {
        GST_INFO_OBJECT(dec, "H264 format is Byte-stream format");
        dec->h264_format = GST_OMX_H264_FORMAT_BYTE_STREAM;
      }
    }
  }

  gst_buffer_unmap(buf, &map);

  if (dec->h264_format == GST_OMX_H264_FORMAT_PACKETIZED)
    GST_INFO_OBJECT(dec, "H264 format is Packetized format");
}


/*
 *  description : convert input 3gpp buffer to nalu based buffer
 *  params      : @dec : GstOMXH264Dec, @buf: buffer to be converted
 *  return      : none
 *  comments    : none
 */
static int
convert_frame (GstOMXH264Dec *dec, GstBuffer **buf)
{
  unsigned char frameType = 0;
  unsigned int nalSize = 0;
  unsigned int cumulSize = 0;
  unsigned int offset = 0;
  unsigned char nalHeaderSize = 0;
  unsigned int outSize = 0;
  unsigned char *frame_3gpp = NULL;
  unsigned int frame_3gpp_size = 0;
  GstBuffer *nalu_next_buf = NULL;
  GstBuffer *nalu_buf = NULL;
  GstMapInfo map;
  int ret = 0;


  gst_buffer_map(*buf, &map, GST_MAP_WRITE);
  frame_3gpp = map.data;
  frame_3gpp_size = map.size;
  do {
      /* get NAL Length based on length of length*/
      if (dec->h264_nal_length_size == 1) {
          nalSize = frame_3gpp[0];
      } else if (dec->h264_nal_length_size == 2) {
          nalSize = GST_OMX_H264_RB16 (frame_3gpp);
      } else {
          nalSize = GST_OMX_H264_RB32 (frame_3gpp);
      }

      GST_LOG_OBJECT(dec, "packetized frame size = %d", nalSize);

      frame_3gpp += dec->h264_nal_length_size;

      /* Checking frame type */
      frameType = *frame_3gpp & 0x1f;

      switch (frameType)
      {
          case GST_OMX_H264_NUT_SLICE:
             GST_LOG_OBJECT(dec, "Frame is non-IDR frame...");
              break;
          case GST_OMX_H264_NUT_IDR:
             GST_LOG_OBJECT(dec, "Frame is an IDR frame...");
              break;
          case GST_OMX_H264_NUT_SEI:
             GST_LOG_OBJECT(dec, "Found SEI Data...");
              break;
          case GST_OMX_H264_NUT_SPS:
             GST_LOG_OBJECT(dec, "Found SPS data...");
              break;
          case GST_OMX_H264_NUT_PPS:
             GST_LOG_OBJECT(dec, "Found PPS data...");
              break;
          case GST_OMX_H264_NUT_EOSEQ:
             GST_LOG_OBJECT(dec, "End of sequence...");
              break;
          case GST_OMX_H264_NUT_EOSTREAM:
             GST_LOG_OBJECT(dec, "End of stream...");
              break;
          case GST_OMX_H264_NUT_DPA:
          case GST_OMX_H264_NUT_DPB:
          case GST_OMX_H264_NUT_DPC:
          case GST_OMX_H264_NUT_AUD:
          case GST_OMX_H264_NUT_FILL:
          case GST_OMX_H264_NUT_PREFIX:
          case GST_OMX_H264_NUT_MIXED:
              break;
          default:
             GST_INFO_OBJECT(dec, "Unknown Frame type: %d. do check_frame one more time with next frame.", frameType);
             dec->h264_format = GST_OMX_H264_FORMAT_UNKNOWN;
             goto EXIT;
      }

      /* if nal size is same, we can change only start code */
      if((nalSize + GST_OMX_H264_NAL_START_LEN) == frame_3gpp_size) {
          GST_LOG_OBJECT(dec, "only change start code");
          //*buf = gst_buffer_make_writable(*buf); /* make writable to support memsrc */
          GST_OMX_H264_WB32(map.data, 1);
          gst_buffer_unmap(*buf, &map);

          return 0;
      }

      /* Convert 3GPP Frame to NALU Frame */
      offset = outSize;
      nalHeaderSize = offset ? 3 : 4;

      outSize += nalSize + nalHeaderSize;

      if (nalSize > frame_3gpp_size) {
          GST_ERROR_OBJECT(dec, "out of bounds Error. frame_nalu_size=%d", outSize);
          goto EXIT;
      }

      if (nalu_buf) {
          nalu_next_buf = gst_buffer_new_allocate(NULL, nalSize + nalHeaderSize, NULL);
          if (nalu_next_buf == NULL) {
              GST_ERROR_OBJECT(dec, "gst_buffer_new_and_alloc failed.(nalu_next_buf)");
              goto EXIT;
          }
      } else {
          nalu_buf = gst_buffer_new_allocate(NULL, outSize, NULL);
      }

      if (nalu_buf == NULL) {
          GST_ERROR_OBJECT(dec, "gst_buffer_new_and_alloc failed.(nalu_buf)");
          goto EXIT;
      }

      GstMapInfo nalu_map;
      if (!offset) {
          gst_buffer_map(nalu_buf, &nalu_map, GST_MAP_WRITE);
          memcpy((guint8*)(nalu_map.data)+nalHeaderSize, frame_3gpp, nalSize);
          GST_OMX_H264_WB32(nalu_map.data, 1);
          gst_buffer_unmap(nalu_buf, &nalu_map);
      } else {
          if (nalu_next_buf) {
              gst_buffer_copy_into(nalu_buf, nalu_next_buf, GST_BUFFER_COPY_ALL, 0, -1);
              gst_buffer_unref(nalu_next_buf);
              nalu_next_buf = NULL;
          }

          gst_buffer_map(nalu_buf, &nalu_map, GST_MAP_WRITE);
          guint8* nalu_buf_data = nalu_map.data;
          memcpy(nalu_buf_data + nalHeaderSize + offset, frame_3gpp, nalSize);
          (nalu_buf_data + offset)[0] = (nalu_buf_data + offset)[1] = 0;
          (nalu_buf_data + offset)[2] = 1;
          gst_buffer_unmap(nalu_buf, &nalu_map);
      }

      frame_3gpp += nalSize;
      cumulSize += nalSize + dec->h264_nal_length_size;
      GST_LOG_OBJECT(dec, "frame_3gpp_size = %d => frame_nalu_size=%d", frame_3gpp_size, outSize);
  } while (cumulSize < frame_3gpp_size);

  gst_buffer_unmap(*buf, &map);
  gst_buffer_copy_into(nalu_buf, *buf, GST_BUFFER_COPY_METADATA, 0, -1);
  gst_buffer_unref (*buf);
  *buf = nalu_buf;

  return 0;

EXIT:
  gst_buffer_unmap(*buf, &map);
  if (nalu_buf) {
    gst_buffer_copy_into(nalu_buf, *buf, GST_BUFFER_COPY_METADATA, 0, -1);
    gst_buffer_unref (*buf);
    *buf = nalu_buf;
  }
  GST_ERROR_OBJECT(dec, "converting frame error.");

  return -1;
}


static void
gst_omx_h264_dec_class_init (GstOMXH264DecClass * klass)
{
  GstOMXVideoDecClass *videodec_class = GST_OMX_VIDEO_DEC_CLASS (klass);
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  videodec_class->is_format_change = GST_DEBUG_FUNCPTR (gst_omx_h264_dec_is_format_change);
  videodec_class->set_format = GST_DEBUG_FUNCPTR (gst_omx_h264_dec_set_format);
  videodec_class->prepare_frame = GST_DEBUG_FUNCPTR (gst_omx_h264_dec_prepare_frame);

  videodec_class->cdata.default_sink_template_caps = "video/x-h264, "
      "alignment=(string) { au }, "
      "stream-format=(string){ avc, byte-stream },"
      "width=(int) [1,MAX], " "height=(int) [1,MAX],"
      "framerate = (fraction) [0/1, MAX]";

  gst_element_class_set_static_metadata (element_class,
      "OpenMAX H.264 Video Decoder",
      "Codec/Decoder/Video",
      "Decode H.264 video streams",
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  gst_omx_set_default_role (&videodec_class->cdata, "video_decoder.avc");
}

static void
gst_omx_h264_dec_init (GstOMXH264Dec * self)
{
}

static gboolean
gst_omx_h264_dec_is_format_change (GstOMXVideoDec * parent, GstOMXPort * port, GstVideoCodecState * state)
{
  return FALSE;
}

static gboolean
gst_omx_h264_dec_set_format (GstOMXVideoDec * parent, GstOMXPort * port, GstVideoCodecState * state)
{
  gboolean ret;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  GstOMXH264Dec *dec = GST_OMX_H264_DEC (parent);

  gst_omx_port_get_port_definition (port, &port_def);
  port_def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
  ret = gst_omx_port_update_port_definition (port, &port_def) == OMX_ErrorNone;

  if (state->codec_data)
  {
    guint8 *buf_data = NULL;
    GstMapInfo map;
    gst_buffer_map(state->codec_data, &map, GST_MAP_READ);
    buf_data = map.data;

    if (gst_buffer_get_size(state->codec_data) <= GST_OMX_H264_NAL_START_LEN) {
        gst_buffer_unmap(state->codec_data, &map);
        GST_ERROR_OBJECT (dec, "codec data size (%d) is less than start code length.", gst_buffer_get_size(state->codec_data));
        dec->h264_format = GST_OMX_H264_FORMAT_UNKNOWN;
    } else {
      if ((buf_data[0] == 0x00)&&(buf_data[1] == 0x00)&&
         ((buf_data[2] == 0x01)||((buf_data[2] == 0x00)&&(buf_data[3] == 0x01)))) {
           dec->h264_format = GST_OMX_H264_FORMAT_BYTE_STREAM;
           GST_WARNING_OBJECT(dec, "H264 codec_data format is Byte-stream.");
      } else {
         dec->h264_format = GST_OMX_H264_FORMAT_PACKETIZED;
         GST_WARNING_OBJECT(dec, "H264 codec_data format is Packetized.");
      }
      gst_buffer_unmap(state->codec_data, &map);


      /* if codec data is 3gpp format, convert nalu format */
      if(dec->h264_format == GST_OMX_H264_FORMAT_PACKETIZED) {
        GstBuffer *nalu_dci = NULL;

        ret = gst_omx_convert_dci(dec, state->codec_data, &nalu_dci);
        if (ret) {
          state->codec_data = NULL;
          state->codec_data= nalu_dci;
        } else {
          GST_ERROR_OBJECT(dec, "converting dci error.");
          if (nalu_dci) {
            gst_buffer_unref (nalu_dci);
            nalu_dci = NULL;
          }
        }
      }
    }
  }

  return ret;
}

static GstFlowReturn
gst_omx_h264_dec_prepare_frame (GstOMXVideoDec * parent, GstVideoCodecFrame *frame)
{
  GstFlowReturn ret = GST_FLOW_OK;
  int return_value = 0;
  GstOMXH264Dec *dec = GST_OMX_H264_DEC (parent);
  GstVideoDecoder *decoder = GST_VIDEO_DECODER(parent);

  if (dec->h264_format == GST_OMX_H264_FORMAT_UNKNOWN) {
    check_frame(dec, frame->input_buffer);
  }

  if (dec->h264_format == GST_OMX_H264_FORMAT_PACKETIZED) {
    GST_LOG_OBJECT(dec, "H264 format is Packetized format. convert to Byte-stream format");
    return_value = convert_frame(dec, &frame->input_buffer);
    if (return_value < 0) {
      GST_VIDEO_DECODER_ERROR (decoder, 2, STREAM, DECODE, (NULL), ("convert frame is failed"), ret);
    }
  }

  return ret;
}
