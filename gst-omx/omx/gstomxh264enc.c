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

#ifdef CODEC_OUTPUT_DUMP
#include <stdio.h>
#endif

#include "gstomxh264enc.h"

GST_DEBUG_CATEGORY_STATIC (gst_omx_h264_enc_debug_category);
#define GST_CAT_DEFAULT gst_omx_h264_enc_debug_category

/* prototypes */
static gboolean gst_omx_h264_enc_set_format (GstOMXVideoEnc * enc, GstOMXPort * port, GstVideoCodecState * state);
static GstCaps *gst_omx_h264_enc_get_caps (GstOMXVideoEnc * enc, GstOMXPort * port, GstVideoCodecState * state);
static GstFlowReturn gst_omx_h264_enc_handle_output_frame (GstOMXVideoEnc * self, GstOMXPort * port, GstOMXBuffer * buf, GstVideoCodecFrame * frame);

enum
{
  PROP_0
};

/* class initialization */

#define DEBUG_INIT \
  GST_DEBUG_CATEGORY_INIT (gst_omx_h264_enc_debug_category, "omxh264enc", 0, \
      "debug category for gst-omx video encoder base class");

G_DEFINE_TYPE_WITH_CODE (GstOMXH264Enc, gst_omx_h264_enc, GST_TYPE_OMX_VIDEO_ENC, DEBUG_INIT);

static void
gst_omx_h264_enc_class_init (GstOMXH264EncClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  GstOMXVideoEncClass *videoenc_class = GST_OMX_VIDEO_ENC_CLASS (klass);

  videoenc_class->set_format = GST_DEBUG_FUNCPTR (gst_omx_h264_enc_set_format);
  videoenc_class->get_caps = GST_DEBUG_FUNCPTR (gst_omx_h264_enc_get_caps);

  videoenc_class->cdata.default_src_template_caps = "video/x-h264, " "width=(int) [ 16, 4096 ], " "height=(int) [ 16, 4096 ]";
  videoenc_class->handle_output_frame = GST_DEBUG_FUNCPTR (gst_omx_h264_enc_handle_output_frame);

  gst_element_class_set_static_metadata (element_class,
      "OpenMAX H.264 Video Encoder",
      "Codec/Encoder/Video",
      "Encode H.264 video streams",
      "Sebastian Dröge <sebastian.droege@collabora.co.uk>");

  gst_omx_set_default_role (&videoenc_class->cdata, "video_encoder.avc");
}

static void
gst_omx_h264_enc_init (GstOMXH264Enc * self)
{
}

#ifdef CODEC_OUTPUT_DUMP
static inline void
encoder_packet_dump (void *inbuf, gint size)
{
  char filename[100]={0};
  FILE *fp = NULL;

  sprintf(filename, "/tmp/h264_enc_packet_dump.yuv");
  fp = fopen(filename, "ab");

  if(!fp) {
    GST_ERROR("%s open failed", filename);
    return;
  }

  fwrite(inbuf, size, 1, fp);

  GST_WARNING("codec encoder packet dumped. Size[%d]!!", size);
  fclose(fp);
}
#endif

/*
 *  description : convert byte-stream format to packetized frame
 *  params      : @self : GstOMXH264Enc, @buf: byte-stream buf, @sync: notify this buf is sync frame
 *  return      : none
 *  comments    :
 */
static void
gst_omx_h264_enc_convert_to_packetized_frame (GstOMXH264Enc *self, GstOMXBuffer *buf)
{
  unsigned char *data = buf->omx_buf->pBuffer;
  unsigned int size = buf->omx_buf->nFilledLen;
  int idx = 0;
  gint start_idx = -1;
  unsigned char *nalu_start = NULL;
  unsigned char *buf_pos, *end_pos;

  GST_LOG_OBJECT (self, "Convert to packetized format. size = %d", buf->omx_buf->nFilledLen);

  if (size == 0)
    return;

  /* 1 slice per frame */
  GST_LOG_OBJECT (self, "Handle single NALU per buffer");

  buf_pos = data;
  end_pos = buf_pos + size;

  /* Reading 8byte per loop (for efficiency) and determines whether NAL starting point is contained or not */
  for (end_pos -= 7; buf_pos < end_pos; buf_pos += 4) {
    int idx_flag = 0;
    unsigned int value0 = *((unsigned int*)buf_pos);
    unsigned int value1 = *((unsigned int*)buf_pos + 1);

    if (!(value0 & 0xFF000000)) {    /* |XXX0| */
      if ( !(value0 & 0xFFFFFF00) && ((value1 & 0x000000FF) == 0x00000001)) {
        /* |X000|1 */
        /* found */
        idx = buf_pos+1 - data;
        idx_flag = 1;
        goto out;

      } else if ( !(value0 & 0xFFFF0000) && ((value1 & 0x0000FFFF) == 0x00000100)) {
        /* XX00|01*/
        /* found */
        idx = buf_pos+2 - data;
        idx_flag = 1;
        goto out;

      } else if ((value1 & 0x00FFFFFF) == 0x00010000) {
        /* XXX0|001 */
        /* found */
        idx = buf_pos+3 - data;
        idx_flag = 1;
        goto out;
      } else if (!(value0&0xFFFF0000)&&((value1&0x000000FF)==0x00000001)) {
        /* XX00|1*/
        goto err;
      } else if ((value1&0x0000FFFF)==0x00000100) {
        /* XXX0|01 */
        goto err;
      }

    } else if (value0 == 0x01000000) {
      /* found */
      idx = buf_pos - data;
      idx_flag = 1;
      goto out;
    } else if ((value0 & 0x00FFFFFF)==0x00010000) {
      /* |001X| */
      goto err;
    } else if ((value0 & 0xFFFFFF00)==0x01000000) {
      /* |X001| */
err:
      GST_ERROR_OBJECT (self, "ERROR : NALU header is 3-bytes, yet to support !!");
      continue;
    }

out:
    if (idx_flag) {
      if (start_idx >= 0) {
        if (idx <= start_idx) {
          GST_ERROR_OBJECT (self, "ERROR : idx <= start_idx !!");
          return;
        }
        GST_LOG_OBJECT (self, "size of current nal unit = %d", idx-start_idx);
        GST_OMX_H264_WB32 (nalu_start, idx - start_idx - GST_OMX_H264_NAL_START_LEN);
      }
      start_idx = idx;
      nalu_start = data + idx;
    }
  }
  idx = size;
  /* converting last nal unit */
  if (start_idx >= 0) {
    GST_LOG_OBJECT (self, "size of current nal unit = %d", idx-start_idx);
    GST_OMX_H264_WB32 (nalu_start, idx - start_idx - GST_OMX_H264_NAL_START_LEN);
  }
}

/*
 *  description : convert byte-stream codec data to packtized codec_data
 *  params      : @self : GstOMXH264Enc, @inbuf: byte-stream codec data (omx buf), @outbuf: packetized codec_data
 *  return      : true on successes / false on failure
 *  comments    :
 */
static gboolean
gst_omx_h264_enc_convert_to_packetized_dci (GstOMXH264Enc *self, unsigned char *nalu_dci, unsigned nalu_dci_len,
    GstBuffer **packetized_dci, gint *out_sps_cnt, gint *out_pps_cnt)
{
  gint idx = 0;
  gint sps_cnt = 0;
  gint pps_cnt = 0;
  GQueue *sps_queue = 0;
  GQueue *pps_queue = 0;
  unsigned char *packet_dci = NULL;
  gint prev_nalu_start = 0;
  gint prev_nalu_type = GST_OMX_H264_ENC_NUT_UNKNOWN;
  gint sps_size = 0;
  gint pps_size = 0;
  GstBuffer *sps_data = NULL;
  GstBuffer *pps_data = NULL;
  GstBuffer *queue_data = NULL;
  gint nal_type = GST_OMX_H264_ENC_NUT_UNKNOWN;
  unsigned char profile = 0;
  unsigned char level = 0;
  unsigned char profile_comp = 0;
  gboolean bret = TRUE;
  gboolean single_sps_pps = FALSE; /* if there is only 1 sps, pps set, */
  GstMapInfo pk_map,map;

  sps_queue = g_queue_new ();
  pps_queue = g_queue_new ();

  /* find no.of SPS & PPS units */
  while (idx < nalu_dci_len) {
    if ((nalu_dci[idx] == 0x00) && (nalu_dci[idx + 1] == 0x00) && (nalu_dci[idx + 2] == 0x01)) {
      /* copy previous nal unit */
      if (prev_nalu_start && prev_nalu_type == GST_OMX_H264_ENC_NUT_SPS) {
        if (nalu_dci[idx -1] == 0x00) {
          sps_size = idx -1 - prev_nalu_start;
        } else {
          sps_size = idx - prev_nalu_start;
        }
        sps_data = gst_buffer_new_allocate (NULL, sps_size + GST_OMX_H264_C_DCI_LEN, NULL);
        if (!sps_data) {
          GST_ERROR_OBJECT (self, "failed to allocate memory..");
          bret = FALSE;
          goto exit;
        }

        gst_buffer_map (sps_data, &map, GST_MAP_WRITE);
        GST_OMX_H264_WB16 (map.data, sps_size);
        memcpy (map.data + GST_OMX_H264_C_DCI_LEN, nalu_dci + prev_nalu_start, sps_size);
        gst_buffer_unmap (sps_data, &map);
        g_queue_push_tail (sps_queue, sps_data);
      } else if (prev_nalu_start && prev_nalu_type == GST_OMX_H264_ENC_NUT_PPS) {
        if (nalu_dci[idx -1] == 0x00) {
          pps_size = idx -1 - prev_nalu_start;
        } else {
          pps_size = idx - prev_nalu_start;
        }

        pps_data = gst_buffer_new_allocate (NULL, pps_size + GST_OMX_H264_C_DCI_LEN, NULL);
        if (!pps_data) {
          GST_ERROR_OBJECT (self, "failed to allocate memory..");
          bret = FALSE;
          goto exit;
        }
        gst_buffer_map (pps_data, &map, GST_MAP_WRITE);
        GST_OMX_H264_WB16 (map.data, pps_size);
        memcpy (map.data + GST_OMX_H264_C_DCI_LEN, nalu_dci + prev_nalu_start, pps_size);
        gst_buffer_unmap (pps_data, &map);
        g_queue_push_tail (pps_queue, pps_data);
      }
      /* present nalu type */
      nal_type = nalu_dci[idx+3] & 0x1f;

      if (nal_type == GST_OMX_H264_ENC_NUT_SPS) {
        sps_cnt++;
        prev_nalu_start = idx + 3;
        prev_nalu_type = GST_OMX_H264_ENC_NUT_SPS;
        profile = nalu_dci[idx + 4];
        level = nalu_dci[idx + 6];
        GST_INFO_OBJECT (self, "Profile Number = %d and Level = %d...", nalu_dci[idx + 4], level);
        GST_INFO_OBJECT (self, "Profile is %s", (profile == 66) ? "BaseLine Profile": (profile == 77)? "Main Profile": profile==88 ?
            "Extended Profile": profile==100 ? "High Profile": "Not Supported codec");
      } else if ((nalu_dci[idx + 3] & 0x1f) == GST_OMX_H264_ENC_NUT_PPS) {
        pps_cnt++;
        prev_nalu_start = idx + 3;
        prev_nalu_type = GST_OMX_H264_ENC_NUT_PPS;
      }
    }
    idx++;
  }

  /* copy previous nal unit */
  if (prev_nalu_start && prev_nalu_type == GST_OMX_H264_ENC_NUT_SPS) {
    sps_size = idx - prev_nalu_start;

    sps_data = gst_buffer_new_allocate (NULL, sps_size + GST_OMX_H264_C_DCI_LEN, NULL);
    if (!sps_data) {
      GST_ERROR_OBJECT (self, "failed to allocate memory..");
      bret = FALSE;
      goto exit;
    }
    gst_buffer_map (sps_data, &map, GST_MAP_WRITE);
    GST_OMX_H264_WB16 (map.data, sps_size);
    memcpy (map.data + GST_OMX_H264_C_DCI_LEN, nalu_dci + prev_nalu_start, sps_size);
    gst_buffer_unmap (sps_data, &map);
    g_queue_push_tail (sps_queue, sps_data);
  } else if (prev_nalu_start && prev_nalu_type == GST_OMX_H264_ENC_NUT_PPS) {
    pps_size = idx - prev_nalu_start;

    pps_data = gst_buffer_new_allocate (NULL, pps_size + GST_OMX_H264_C_DCI_LEN, NULL);
    if (!pps_data) {
      GST_ERROR_OBJECT (self, "failed to allocate memory..");
      bret = FALSE;
      goto exit;
    }
    gst_buffer_map (pps_data, &map, GST_MAP_WRITE);
    GST_OMX_H264_WB16 (map.data, pps_size);
    memcpy (map.data + GST_OMX_H264_C_DCI_LEN, nalu_dci + prev_nalu_start, pps_size);
    gst_buffer_unmap (pps_data, &map);
    g_queue_push_tail (pps_queue, pps_data);
  }

  /* make packetized codec data */
  if (sps_cnt == 1 && pps_cnt == 1) {
    single_sps_pps = TRUE;
    *packetized_dci = gst_buffer_new_allocate (NULL, GST_OMX_H264_MDATA + GST_OMX_H264_C_DCI_LEN + sps_size + GST_OMX_H264_CNT_LEN + GST_OMX_H264_C_DCI_LEN + pps_size, NULL);
    GST_LOG_OBJECT (self, "allocate packetized_dci in case of single sps, pps. (size=%d)", gst_buffer_get_size (*packetized_dci));
  } else {
    *packetized_dci = gst_buffer_new_allocate (NULL, GST_OMX_H264_MDATA, NULL);
    GST_LOG_OBJECT (self, "allocate packetized_dci in case of multi sps, pps");
  }

  if (*packetized_dci == NULL) {
    GST_ERROR_OBJECT (self, "Failed to allocate memory..");
    bret = FALSE;
    goto exit;
  }
  gst_buffer_map (*packetized_dci, &pk_map, GST_MAP_WRITE);
  packet_dci = pk_map.data;
  packet_dci[0] = 0x01; /* configurationVersion */
  packet_dci[1] = profile; /* AVCProfileIndication */
  packet_dci[2] = profile_comp; /* profile_compatibility*/
  packet_dci[3] = level;  /* AVCLevelIndication */
  packet_dci[4] = 0xff; /* Reserver (6bits.111111) + LengthSizeMinusOne (2bits). lengthoflength = 4 for present */
  packet_dci[5] = 0xe0 | sps_cnt; /* Reserver (3bits. 111) + numOfSequenceParameterSets (5bits) */

  /* copy SPS sets */
  while (!g_queue_is_empty (sps_queue)) {
    sps_data = g_queue_pop_head (sps_queue);

    if (TRUE == single_sps_pps) {
      gst_buffer_map (sps_data, &map, GST_MAP_READ);
      memcpy (packet_dci + GST_OMX_H264_MDATA, map.data, map.size);
      gst_buffer_unmap (sps_data, &map);
      gst_buffer_unref (sps_data);
      sps_data = NULL;
    } else {
      *packetized_dci = gst_buffer_append (*packetized_dci, sps_data);
    }
  }

  /* add number of PPS sets (1byte)*/
  if (TRUE == single_sps_pps) {
    packet_dci[GST_OMX_H264_MDATA + GST_OMX_H264_C_DCI_LEN + sps_size] = pps_cnt;
  } else {
    GstBuffer *next_data = gst_buffer_new_allocate (NULL, GST_OMX_H264_CNT_LEN, NULL);
    if (!next_data) {
      GST_ERROR_OBJECT (self, "Failed to allocate memory..");
      bret = FALSE;
      gst_buffer_unmap (*packetized_dci, &pk_map);
      goto exit;
    }

    GstMapInfo next_data_map;
    gst_buffer_map (next_data, &next_data_map, GST_MAP_WRITE);

    next_data_map.data[0] = pps_cnt;
    gst_buffer_unmap (next_data, &next_data_map);
    *packetized_dci = gst_buffer_append (*packetized_dci, next_data);
  }

  /* copy PPS sets */
  while (!g_queue_is_empty (pps_queue)) {
    pps_data = g_queue_pop_head (pps_queue);

    if (TRUE == single_sps_pps) {
      gst_buffer_map (pps_data, &map, GST_MAP_READ);
      memcpy (packet_dci + GST_OMX_H264_MDATA + GST_OMX_H264_C_DCI_LEN + sps_size + GST_OMX_H264_CNT_LEN,
          map.data, map.size);
      gst_buffer_unmap (pps_data, &map);
      gst_buffer_unref (pps_data);
      pps_data = NULL;
    } else {
      *packetized_dci = gst_buffer_append (*packetized_dci, pps_data);
    }
  }

  gst_buffer_unmap (*packetized_dci, &pk_map);

exit:
  while (!g_queue_is_empty (sps_queue)) {
    queue_data = g_queue_pop_head (sps_queue);
    gst_buffer_unref (queue_data);
    queue_data = NULL;
  }
  g_queue_free (sps_queue);

  while (!g_queue_is_empty (pps_queue)) {
    queue_data = g_queue_pop_head (pps_queue);
    gst_buffer_unref (queue_data);
    queue_data = NULL;
  }
  g_queue_free (pps_queue);

  *out_sps_cnt = sps_cnt;
  *out_pps_cnt = sps_cnt;

  return bret;
}

/*
 *  description : resotre packetized dci (codec_data)
 *  params      : @self : GstOmxH264Enc, @inbuf: codec data, @outbuf: h264enc->dci
 *  return      : none
 *  comments    :
 *    from original packetized codec_data: METADATA(6) + SPS_CNT(0) + {SPS_SIZE(2)+SPS_DATA(sps_size)}*n +
                                                             PPS_CNT(1) + {PPS_SIZE(2)+PPS_DATA(pps_size)}*n
 *    to restore packetized dci: {SPS_SIZE(4)+SPS_DATA(sps_size)}*n + {PPS_SIZE(4)+PPS_DATA(pps_size)}*n
 */
static gboolean
gst_omx_h264_enc_restore_packetized_dci (GstOMXH264Enc *self, GstBuffer *inbuf, GstBuffer **outbuf, gint sps_cnt, gint pps_cnt)
{
  GstMapInfo in_map,out_map,next_map;
  unsigned char *indata = NULL;
  guint sps_size = 0;
  guint pps_size = 0;
  gint i = 0;
  GstBuffer *next_data = NULL;

  GST_INFO_OBJECT (self, "restore_packetized_dci. sps_cnt = %d, pps_cnt = %d", sps_cnt, pps_cnt);
  gst_buffer_map (inbuf, &in_map, GST_MAP_READ);
  indata = in_map.data;

  if (sps_cnt == 1 && pps_cnt == 1) {
    sps_size = GST_OMX_H264_RB16 (indata + GST_OMX_H264_MDATA);
    pps_size = GST_OMX_H264_RB16 (indata + GST_OMX_H264_MDATA + GST_OMX_H264_C_DCI_LEN + sps_size + GST_OMX_H264_CNT_LEN);

    *outbuf = gst_buffer_new_allocate (NULL, GST_OMX_H264_A_DCI_LEN + sps_size + GST_OMX_H264_A_DCI_LEN + pps_size, NULL);

    GST_INFO_OBJECT (self, "Codec data size = %d", GST_OMX_H264_A_DCI_LEN + sps_size + GST_OMX_H264_A_DCI_LEN + pps_size);

    if (!*outbuf) {
      GST_ERROR_OBJECT (self, "Failed to allocate memory in gst_buffer_new_and_alloc.");
      if(inbuf)
      {
        gst_buffer_unmap (inbuf, &in_map);
      }
      goto error_exit;
    }
    gst_buffer_map (*outbuf, &out_map, GST_MAP_WRITE);
    GST_OMX_H264_WB32 (out_map.data, sps_size);
    indata += GST_OMX_H264_MDATA + GST_OMX_H264_C_DCI_LEN;
    memcpy (out_map.data + GST_OMX_H264_A_DCI_LEN, indata, sps_size);
    indata += sps_size;

    GST_OMX_H264_WB32 (out_map.data + GST_OMX_H264_A_DCI_LEN + sps_size, pps_size);
    indata += GST_OMX_H264_CNT_LEN + GST_OMX_H264_C_DCI_LEN;
    memcpy (out_map.data + GST_OMX_H264_A_DCI_LEN + sps_size + GST_OMX_H264_A_DCI_LEN, indata, pps_size);
    indata += pps_size;
    gst_buffer_unmap (*outbuf, &out_map);
  } else {
    /* in this case, dci has multi nalu. ex) sps + sps + sps + pps + pps */
    indata += GST_OMX_H264_MDATA;
    *outbuf = gst_buffer_new ();

    for (i = 0; i < sps_cnt; i++) {
      sps_size = GST_OMX_H264_RB16 (indata); /* metadata(6) */

      next_data = gst_buffer_new_allocate(NULL, GST_OMX_H264_A_DCI_LEN + sps_size, NULL);
      if (!next_data) {
        GST_ERROR_OBJECT (self, "Failed to allocate memory in gst_buffer_new_and_alloc.");
        if(inbuf)
        {
          gst_buffer_unmap (inbuf, &in_map);
        }
        goto error_exit;
      }
      gst_buffer_map (next_data, &next_map, GST_MAP_WRITE);
      GST_OMX_H264_WB32(next_map.data, sps_size);
      indata += GST_OMX_H264_C_DCI_LEN; /* sps size field (2 byte) */
      memcpy (next_map.data + GST_OMX_H264_A_DCI_LEN, indata, sps_size);
      gst_buffer_unmap (next_data, &next_map);
      *outbuf = gst_buffer_append (*outbuf, next_data);
      indata += sps_size;
    }
    indata += GST_OMX_H264_CNT_LEN; /* pps cnt field (1 byte) */

    for (i = 0; i < pps_cnt; i++) {
      pps_size = GST_OMX_H264_RB16 (indata);

      next_data = gst_buffer_new_allocate (NULL, GST_OMX_H264_A_DCI_LEN + pps_size, NULL);
      if (!next_data) {
        GST_ERROR_OBJECT (self, "Failed to allocate memory in gst_buffer_new_and_alloc.");
        if(inbuf)
        {
          gst_buffer_unmap(inbuf, &in_map);
        }
        goto error_exit;
      }
      gst_buffer_map (next_data, &next_map, GST_MAP_WRITE);
      GST_OMX_H264_WB32 (next_map.data, pps_size);
      indata += GST_OMX_H264_C_DCI_LEN;
      memcpy (next_map.data + GST_OMX_H264_A_DCI_LEN, indata, pps_size);
      gst_buffer_unmap (next_data, &next_map);
      *outbuf = gst_buffer_append (*outbuf, next_data);
      indata += pps_size;
    }
  }

  if(inbuf)
  {
    gst_buffer_unmap (inbuf, &in_map);
  }

  return TRUE;

  error_exit:

  if (*outbuf) {
    gst_buffer_unref (*outbuf);
    *outbuf = NULL;
  }
  return FALSE;
}

static gboolean
gst_omx_h264_enc_set_format (GstOMXVideoEnc * enc, GstOMXPort * port, GstVideoCodecState * state)
{
  GstOMXH264Enc *self = GST_OMX_H264_ENC (enc);
  GstCaps *peercaps;
  OMX_PARAM_PORTDEFINITIONTYPE port_def;
  OMX_VIDEO_PARAM_PROFILELEVELTYPE param;
  OMX_ERRORTYPE err;
  const gchar *profile_string, *level_string;

  gst_omx_port_get_port_definition (GST_OMX_VIDEO_ENC (self)->enc_out_port, &port_def);
  port_def.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;

  err = gst_omx_port_update_port_definition (GST_OMX_VIDEO_ENC(self)->enc_out_port, &port_def);
  if (err != OMX_ErrorNone)
    return FALSE;

  GST_OMX_INIT_STRUCT (&param);
  param.nPortIndex = GST_OMX_VIDEO_ENC (self)->enc_out_port->index;

  err = gst_omx_component_get_parameter (GST_OMX_VIDEO_ENC (self)->enc, OMX_IndexParamVideoProfileLevelCurrent, &param);
  if (err != OMX_ErrorNone) {
    GST_WARNING_OBJECT (self, "Setting profile/level not supported by component");
    return TRUE;
  }

  peercaps = gst_pad_peer_query_caps (GST_VIDEO_ENCODER_SRC_PAD (enc), gst_pad_get_pad_template_caps (GST_VIDEO_ENCODER_SRC_PAD (enc)));
  if (peercaps) {
    GstStructure *s;

    if (gst_caps_is_empty (peercaps)) {
      gst_caps_unref (peercaps);
      GST_ERROR_OBJECT (self, "Empty caps");
      return FALSE;
    }

    s = gst_caps_get_structure (peercaps, 0);
    profile_string = gst_structure_get_string (s, "profile");
    if (profile_string) {
      if (g_str_equal (profile_string, "baseline")) {
        param.eProfile = OMX_VIDEO_AVCProfileBaseline;
      } else if (g_str_equal (profile_string, "main")) {
        param.eProfile = OMX_VIDEO_AVCProfileMain;
      } else if (g_str_equal (profile_string, "extended")) {
        param.eProfile = OMX_VIDEO_AVCProfileExtended;
      } else if (g_str_equal (profile_string, "high")) {
        param.eProfile = OMX_VIDEO_AVCProfileHigh;
      } else if (g_str_equal (profile_string, "high-10")) {
        param.eProfile = OMX_VIDEO_AVCProfileHigh10;
      } else if (g_str_equal (profile_string, "high-4:2:2")) {
        param.eProfile = OMX_VIDEO_AVCProfileHigh422;
      } else if (g_str_equal (profile_string, "high-4:4:4")) {
        param.eProfile = OMX_VIDEO_AVCProfileHigh444;
      } else {
        goto unsupported_profile;
      }
    }
    level_string = gst_structure_get_string (s, "level");
    if (level_string) {
      if (g_str_equal (level_string, "1")) {
        param.eLevel = OMX_VIDEO_AVCLevel1;
      } else if (g_str_equal (level_string, "1b")) {
        param.eLevel = OMX_VIDEO_AVCLevel1b;
      } else if (g_str_equal (level_string, "1.1")) {
        param.eLevel = OMX_VIDEO_AVCLevel11;
      } else if (g_str_equal (level_string, "1.2")) {
        param.eLevel = OMX_VIDEO_AVCLevel12;
      } else if (g_str_equal (level_string, "1.3")) {
        param.eLevel = OMX_VIDEO_AVCLevel13;
      } else if (g_str_equal (level_string, "2")) {
        param.eLevel = OMX_VIDEO_AVCLevel2;
      } else if (g_str_equal (level_string, "2.1")) {
        param.eLevel = OMX_VIDEO_AVCLevel21;
      } else if (g_str_equal (level_string, "2.2")) {
        param.eLevel = OMX_VIDEO_AVCLevel22;
      } else if (g_str_equal (level_string, "3")) {
        param.eLevel = OMX_VIDEO_AVCLevel3;
      } else if (g_str_equal (level_string, "3.1")) {
        param.eLevel = OMX_VIDEO_AVCLevel31;
      } else if (g_str_equal (level_string, "3.2")) {
        param.eLevel = OMX_VIDEO_AVCLevel32;
      } else if (g_str_equal (level_string, "4")) {
        param.eLevel = OMX_VIDEO_AVCLevel4;
      } else if (g_str_equal (level_string, "4.1")) {
        param.eLevel = OMX_VIDEO_AVCLevel41;
      } else if (g_str_equal (level_string, "4.2")) {
        param.eLevel = OMX_VIDEO_AVCLevel42;
      } else if (g_str_equal (level_string, "5")) {
        param.eLevel = OMX_VIDEO_AVCLevel5;
      } else if (g_str_equal (level_string, "5.1")) {
        param.eLevel = OMX_VIDEO_AVCLevel51;
      } else {
        goto unsupported_level;
      }
    }
    gst_caps_unref (peercaps);
  }

  err =
      gst_omx_component_set_parameter (GST_OMX_VIDEO_ENC (self)->enc,
      OMX_IndexParamVideoProfileLevelCurrent, &param);
  if (err == OMX_ErrorUnsupportedIndex) {
    GST_WARNING_OBJECT (self, "Setting profile/level not supported by component");
  } else if (err != OMX_ErrorNone) {
    GST_ERROR_OBJECT (self, "Error setting profile %lu and level %lu: %s (0x%08x)", param.eProfile, param.eLevel, gst_omx_error_to_string (err), err);
    return FALSE;
  }

  return TRUE;

unsupported_profile:
  GST_ERROR_OBJECT (self, "Unsupported profile %s", profile_string);
  gst_caps_unref (peercaps);
  return FALSE;

unsupported_level:
  GST_ERROR_OBJECT (self, "Unsupported level %s", level_string);
  gst_caps_unref (peercaps);
  return FALSE;
}

static GstCaps *
gst_omx_h264_enc_get_caps (GstOMXVideoEnc * enc, GstOMXPort * port, GstVideoCodecState * state)
{
  GstOMXH264Enc *self = GST_OMX_H264_ENC (enc);
  GstCaps *caps;
  OMX_ERRORTYPE err;
  OMX_VIDEO_PARAM_PROFILELEVELTYPE param;
  const gchar *profile, *level;

  if (enc->byte_stream) {
    caps = gst_caps_new_simple ("video/x-h264",
        "stream-format", G_TYPE_STRING, "byte-stream",
        "alignment", G_TYPE_STRING, "au", NULL);
  } else {
    caps = gst_caps_new_simple ("video/x-h264",
        "stream-format", G_TYPE_STRING, "avc",
        "alignment", G_TYPE_STRING, "au", NULL);
  }

  GST_OMX_INIT_STRUCT (&param);
  param.nPortIndex = GST_OMX_VIDEO_ENC (self)->enc_out_port->index;

  err = gst_omx_component_get_parameter (GST_OMX_VIDEO_ENC (self)->enc, OMX_IndexParamVideoProfileLevelCurrent, &param);
  if (err != OMX_ErrorNone && err != OMX_ErrorUnsupportedIndex)
    return NULL;

  if (err == OMX_ErrorNone) {
    switch (param.eProfile) {
      case OMX_VIDEO_AVCProfileBaseline:
        profile = "baseline";
        break;
      case OMX_VIDEO_AVCProfileMain:
        profile = "main";
        break;
      case OMX_VIDEO_AVCProfileExtended:
        profile = "extended";
        break;
      case OMX_VIDEO_AVCProfileHigh:
        profile = "high";
        break;
      case OMX_VIDEO_AVCProfileHigh10:
        profile = "high-10";
        break;
      case OMX_VIDEO_AVCProfileHigh422:
        profile = "high-4:2:2";
        break;
      case OMX_VIDEO_AVCProfileHigh444:
        profile = "high-4:4:4";
        break;
      default:
        g_assert_not_reached ();
        return NULL;
    }

    switch (param.eLevel) {
      case OMX_VIDEO_AVCLevel1:
        level = "1";
        break;
      case OMX_VIDEO_AVCLevel1b:
        level = "1b";
        break;
      case OMX_VIDEO_AVCLevel11:
        level = "1.1";
        break;
      case OMX_VIDEO_AVCLevel12:
        level = "1.2";
        break;
      case OMX_VIDEO_AVCLevel13:
        level = "1.3";
        break;
      case OMX_VIDEO_AVCLevel2:
        level = "2";
        break;
      case OMX_VIDEO_AVCLevel21:
        level = "2.1";
        break;
      case OMX_VIDEO_AVCLevel22:
        level = "2.2";
        break;
      case OMX_VIDEO_AVCLevel3:
        level = "3";
        break;
      case OMX_VIDEO_AVCLevel31:
        level = "3.1";
        break;
      case OMX_VIDEO_AVCLevel32:
        level = "3.2";
        break;
      case OMX_VIDEO_AVCLevel4:
        level = "4";
        break;
      case OMX_VIDEO_AVCLevel41:
        level = "4.1";
        break;
      case OMX_VIDEO_AVCLevel42:
        level = "4.2";
        break;
      case OMX_VIDEO_AVCLevel5:
        level = "5";
        break;
      case OMX_VIDEO_AVCLevel51:
        level = "5.1";
        break;
      default:
        g_assert_not_reached ();
        return NULL;
    }
    gst_caps_set_simple (caps, "profile", G_TYPE_STRING, profile, "level", G_TYPE_STRING, level, NULL);
  }

  return caps;
}

static GstFlowReturn
gst_omx_h264_enc_handle_output_frame (GstOMXVideoEnc * self, GstOMXPort * port,
    GstOMXBuffer * buf, GstVideoCodecFrame * frame)
{
  GstOMXH264Enc *enc = GST_OMX_H264_ENC (self);

  if (buf->omx_buf->nFlags & OMX_BUFFERFLAG_CODECCONFIG) {
    /* The codec data is SPS/PPS with a startcode => bytestream stream format
     * For bytestream stream format the SPS/PPS is only in-stream and not
     * in the caps!
     */
    if (buf->omx_buf->nFilledLen >= 4 &&
        GST_READ_UINT32_BE (buf->omx_buf->pBuffer + buf->omx_buf->nOffset) == 0x00000001) {
      GList *l = NULL;
      GstBuffer *hdrs;
      GstMapInfo map = GST_MAP_INFO_INIT;
      GstBuffer *codec_data = NULL;
      gint sps_cnt = 0;
      gint pps_cnt = 0;
      gboolean ret = FALSE;

      GST_DEBUG_OBJECT (self, "got codecconfig in byte-stream format");

      if (!(self->byte_stream)) {
        ret = gst_omx_h264_enc_convert_to_packetized_dci (enc, buf->omx_buf->pBuffer + buf->omx_buf->nOffset,
          buf->omx_buf->nFilledLen, &codec_data, &sps_cnt, &pps_cnt);

        if (FALSE == ret) {
          GST_ERROR_OBJECT (self, "Convert to packetized dci failed.");
          if (codec_data) {
            gst_buffer_unref (codec_data);
            codec_data = NULL;
          }
        }

        GST_DEBUG_OBJECT (self, "codec_data length [%d]", gst_buffer_get_size (codec_data));

        ret = gst_omx_h264_enc_restore_packetized_dci (enc, codec_data, &hdrs, sps_cnt, pps_cnt);
        if (ret == FALSE) {
          GST_ERROR_OBJECT (self, "ERROR: restore packetized dci");
        }

        GST_DEBUG_OBJECT (self, "hdrs length [%d]", gst_buffer_get_size (hdrs));

        gst_buffer_map (codec_data, &map, GST_MAP_READ);
        memcpy (buf->omx_buf->pBuffer + buf->omx_buf->nOffset, map.data, map.size);
        buf->omx_buf->nFilledLen = map.size;
        gst_buffer_unmap (codec_data, &map);

        l = g_list_append (l, hdrs);
        gst_video_encoder_set_headers (GST_VIDEO_ENCODER (self), l);
      } else {
        buf->omx_buf->nFlags &= ~OMX_BUFFERFLAG_CODECCONFIG;

        hdrs = gst_buffer_new_and_alloc (buf->omx_buf->nFilledLen);

        gst_buffer_map (hdrs, &map, GST_MAP_WRITE);
        memcpy (map.data, buf->omx_buf->pBuffer + buf->omx_buf->nOffset, buf->omx_buf->nFilledLen);
        gst_buffer_unmap (hdrs, &map);
        l = g_list_append (l, hdrs);
        gst_video_encoder_set_headers (GST_VIDEO_ENCODER (self), l);
      }
    }
  } else {
    if (!(self->byte_stream))
      gst_omx_h264_enc_convert_to_packetized_frame (enc, buf);
  }

#ifdef CODEC_OUTPUT_DUMP
  encoder_packet_dump (buf->omx_buf->pBuffer + buf->omx_buf->nOffset, buf->omx_buf->nFilledLen);
#endif

  return GST_OMX_VIDEO_ENC_CLASS (gst_omx_h264_enc_parent_class)->handle_output_frame (self, port, buf, frame);
}
