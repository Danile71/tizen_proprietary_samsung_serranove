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

#ifndef __GST_OMX_H264_DEC_H__
#define __GST_OMX_H264_DEC_H__

#include <gst/gst.h>
#include "gstomxvideodec.h"

G_BEGIN_DECLS

#define GST_TYPE_OMX_H264_DEC \
  (gst_omx_h264_dec_get_type())
#define GST_OMX_H264_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_OMX_H264_DEC,GstOMXH264Dec))
#define GST_OMX_H264_DEC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_OMX_H264_DEC,GstOMXH264DecClass))
#define GST_OMX_H264_DEC_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj),GST_TYPE_OMX_H264_DEC,GstOMXH264DecClass))
#define GST_IS_OMX_H264_DEC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_OMX_H264_DEC))
#define GST_IS_OMX_H264_DEC_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_OMX_H264_DEC))

typedef struct _GstOMXH264Dec GstOMXH264Dec;
typedef struct _GstOMXH264DecClass GstOMXH264DecClass;

#define GST_OMX_H264_NAL_START_LEN 4
#define GST_OMX_H264_SPSPPS_LEN 2

#define GST_OMX_H264_RB16(x) ((((const unsigned char*)(x))[0] << 8) | ((const unsigned char*)(x))[1])
#define GST_OMX_H264_RB32(x) ((((const unsigned char*)(x))[0] << 24) | \
                                   (((const unsigned char*)(x))[1] << 16) | \
                                   (((const unsigned char*)(x))[2] <<  8) | \
                                   ((const unsigned char*)(x))[3])
#define GST_OMX_H264_WB32(p, d) \
    do { \
        ((unsigned char*)(p))[3] = (d); \
        ((unsigned char*)(p))[2] = (d)>>8; \
        ((unsigned char*)(p))[1] = (d)>>16; \
        ((unsigned char*)(p))[0] = (d)>>24; \
    } while(0);

typedef enum
{
    GST_OMX_H264_NUT_UNKNOWN = 0,
    GST_OMX_H264_NUT_SLICE = 1,
    GST_OMX_H264_NUT_DPA = 2,
    GST_OMX_H264_NUT_DPB = 3,
    GST_OMX_H264_NUT_DPC = 4,
    GST_OMX_H264_NUT_IDR = 5,
    GST_OMX_H264_NUT_SEI = 6,
    GST_OMX_H264_NUT_SPS = 7,
    GST_OMX_H264_NUT_PPS = 8,
    GST_OMX_H264_NUT_AUD = 9,
    GST_OMX_H264_NUT_EOSEQ = 10,
    GST_OMX_H264_NUT_EOSTREAM = 11,
    GST_OMX_H264_NUT_FILL = 12,
    GST_OMX_H264_NUT_PREFIX = 14,
    GST_OMX_H264_NUT_MIXED = 24,
} GSTOMX_H264_NAL_UNIT_TYPE;

typedef enum
{
    GST_OMX_H264_FORMAT_UNKNOWN,
    GST_OMX_H264_FORMAT_PACKETIZED,
    GST_OMX_H264_FORMAT_BYTE_STREAM,
} GST_OMX_H264_STREAM_FORMAT;

struct _GstOMXH264Dec
{
  GstOMXVideoDec parent;
  GST_OMX_H264_STREAM_FORMAT h264_format;
  unsigned int h264_nal_length_size;
};

struct _GstOMXH264DecClass
{
  GstOMXVideoDecClass parent_class;
};

GType gst_omx_h264_dec_get_type (void);

G_END_DECLS

#endif /* __GST_OMX_H264_DEC_H__ */

