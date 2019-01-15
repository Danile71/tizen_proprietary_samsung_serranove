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

#ifndef __GST_OMX_H264_ENC_H__
#define __GST_OMX_H264_ENC_H__

#include <gst/gst.h>
#include "gstomxvideoenc.h"

G_BEGIN_DECLS

#define GST_TYPE_OMX_H264_ENC \
  (gst_omx_h264_enc_get_type())
#define GST_OMX_H264_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_OMX_H264_ENC,GstOMXH264Enc))
#define GST_OMX_H264_ENC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_OMX_H264_ENC,GstOMXH264EncClass))
#define GST_OMX_H264_ENC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_OMX_H264_ENC,GstOMXH264EncClass))
#define GST_IS_OMX_H264_ENC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_OMX_H264_ENC))
#define GST_IS_OMX_H264_ENC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_OMX_H264_ENC))

#define GST_OMX_H264_MDATA 6 /* packtized meta data size. (ver, profile...) */
#define GST_OMX_H264_A_DCI_LEN 4 /* 4-byte sps, pps size field in append dci*/
#define GST_OMX_H264_C_DCI_LEN 2 /* 2-byte sps, pps size field in codec data*/
#define GST_OMX_H264_CNT_LEN 1 /* 1-byte sps, pps count field */
#define GST_OMX_H264_NAL_START_LEN 4

#define GST_OMX_H264_RB16(x) ((((const unsigned char*)(x))[0] << 8) | ((const unsigned char*)(x))[1])
#define GST_OMX_H264_WB32(p, d) \
    do { \
      ((unsigned char*)(p))[3] = (d); \
      ((unsigned char*)(p))[2] = (d)>>8; \
      ((unsigned char*)(p))[1] = (d)>>16; \
      ((unsigned char*)(p))[0] = (d)>>24; \
    } while(0);

#define GST_OMX_H264_WB16(p, d) \
    do { \
      ((unsigned char*)(p))[1] = (d); \
      ((unsigned char*)(p))[0] = (d)>>8; \
    } while(0);

typedef struct _GstOMXH264Enc GstOMXH264Enc;
typedef struct _GstOMXH264EncClass GstOMXH264EncClass;

typedef enum
{
  GST_OMX_H264_ENC_NUT_UNKNOWN = 0,
  GST_OMX_H264_ENC_NUT_SLICE = 1,
  GST_OMX_H264_ENC_NUT_DPA = 2,
  GST_OMX_H264_ENC_NUT_DPB = 3,
  GST_OMX_H264_ENC_NUT_DPC = 4,
  GST_OMX_H264_ENC_NUT_IDR = 5,
  GST_OMX_H264_ENC_NUT_SEI = 6,
  GST_OMX_H264_ENC_NUT_SPS = 7,
  GST_OMX_H264_ENC_NUT_PPS = 8,
  GST_OMX_H264_ENC_NUT_AUD = 9,
  GST_OMX_H264_ENC_NUT_EOSEQ = 10,
  GST_OMX_H264_ENC_NUT_EOSTREAM = 11,
  GST_OMX_H264_ENC_NUT_FILL = 12,
  GST_OMX_H264_ENC_NUT_MIXED = 24,
} GstOmxH264NalUnitType;

struct _GstOMXH264Enc
{
  GstOMXVideoEnc parent;
};

struct _GstOMXH264EncClass
{
  GstOMXVideoEncClass parent_class;
};

GType gst_omx_h264_enc_get_type (void);

G_END_DECLS

#endif /* __GST_OMX_H264_ENC_H__ */

