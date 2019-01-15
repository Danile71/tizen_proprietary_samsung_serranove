/*
 * gstqcamerasrc_defs.h
 *
 * Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd.
 *
 * Contact: Abhijit Dey <abhijit.dey@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef __GSTQCAMERASRC_DEFS_H__
#define __GSTQCAMERASRC_DEFS_H__

#ifndef YUV422_SIZE
#define YUV422_SIZE(width,height) ( ((width)*(height)) << 1 )
#endif

#ifndef YUV420_SIZE
#define YUV420_SIZE(width,height) ( ((width)*(height)*3) >> 1 )
#endif

#if !defined (PAGE_SHIFT)
#define PAGE_SHIFT sysconf(_SC_PAGESIZE)
#endif
#if !defined (PAGE_SIZE)
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#endif
#if !defined (PAGE_MASK)
#define PAGE_MASK (~(PAGE_SIZE-1))
#endif
#if !defined (PAGE_ALIGN)
#define PAGE_ALIGN(addr)    (((addr)+PAGE_SIZE-1)&PAGE_MASK)
#endif

#define ALIGN_SIZE_I420                 (1024<<2)
#define ALIGN_SIZE_NV12                 (1024<<6)
#define CAMERASRC_ALIGN(addr,size)      (((addr)+((size)-1))&(~((size)-1)))

#define ALIGN_SIZE_2K                       2048
#define ALIGN_SIZE_4K                       4096
#define ALIGN_SIZE_8K                       8192

#if !defined (CLEAR)
#define CLEAR(x)            memset (&(x), 0, sizeof (x))
#endif

#ifndef LOG_DEBUG
#define _ENABLE_CAMERASRC_DEBUG                 0
#else
#define _ENABLE_CAMERASRC_DEBUG                 0
#endif

#define QCAMERASRC_ERRMSG_MAX_LEN               128
#define QCAMERASRC_DEV_FD_INIT                   -1
#define _DEFAULT_WIDTH                          640
#define _DEFAULT_HEIGHT                         480
#define _DEFAULT_VIDEO_WIDTH                    640
#define _DEFAULT_VIDEO_HEIGHT                   480
#define _DEFAULT_HFR_MODE                       0
#define _DEFAULT_FPS                            15
#define _DEFAULT_HIGH_SPEED_FPS                 0
#define _DEFAULT_FPS_AUTO                       FALSE
#define _DEFAULT_PIX_FORMAT                     CAMERASRC_PIX_UYVY
#define _DEFAULT_FOURCC                         GST_MAKE_FOURCC ('J', 'P', 'E', 'G')
#define _DEFAULT_COLORSPACE                     CAMERASRC_COL_RAW
#define _DEFAULT_CAMERA_ID                      CAMERASRC_DEV_ID_PRIMARY
#define _DEFAULT_NUM_LIVE_BUFFER                0
#define _DEFAULT_BUFFER_COUNT                   0
#define _DEFAULT_BUFFER_CIRULATION_COUNT        0
#define _DEFAULT_BUFFER_RUNNING                 FALSE
#define _DEFAULT_DEQUE_WAITINGTIME              200     /* msec */
#define _DEFAULT_CAP_QUALITY                    GST_CAMERASRC_QUALITY_HIGH
#define _DEFAULT_CAP_JPG_QUALITY                95
#define _DEFAULT_CAP_WIDTH                      2048
#define _DEFAULT_CAP_HEIGHT                     1232
#define _DEFAULT_CAP_COUNT                      1
#define _DEFAULT_CAP_INTERVAL                   1
#define _DEFAULT_CAP_PROVIDE_EXIF               TRUE
#define _DEFAULT_SHOT_MODE              0
#define _DEFAULT_ENABLE_ZSL_MODE                FALSE
#define _DEFAULT_DO_FACE_DETECT                 FALSE
#define _DEFAULT_DO_AF                          FALSE
#define _DEFAULT_SIGNAL_AF                      FALSE
#define _DEFAULT_SIGNAL_STILL_CAPTURE           FALSE
#define _DEFAULT_PREVIEW_WIDTH                  _DEFAULT_WIDTH
#define _DEFAULT_PREVIEW_HEIGHT                 _DEFAULT_HEIGHT
#define _DEFAULT_KEEPING_BUFFER                 0
#define _DEFAULT_NUM_ALLOC_BUF                  6
#define _DEFAULT_FRAME_RATE                     30
#define _DEFAULT_CAP_HDR                        CAMERASRC_HDR_CAPTURE_OFF
#define _DEFAULT_ENABLE_VDIS_MODE               FALSE
#define _DEFAULT_ENABLE_HYBRID_MODE             FALSE
#define _DEFAULT_LOW_LIGHT_DETECTION            LOW_LIGHT_DETECTION_OFF
#define _DEFAULT_LOW_LIGHT_MODE                 LOW_LIGHT_MODE_OFF
#define _DEFAULT_DUAL_CAMERA_MODE               FALSE
#define _DEFAULT_MEDIAPACKET_RECORD_CB_MODE     FALSE
#define _DEFAULT_ENABLE_CAMERA_CLOCK_LOCK       FALSE
#define _DEFAULT_VT_MODE                        VT_MODE_OFF

#define _LLM_MIN_FPS                            8    /* Request from system team */
#define _MIN_FPS_FRONT                          8    /* Request from system team */
#define _MIN_FPS_REAR                           15    /* Request from system team */
#define _MAX_FPS                                30    /* Max for auto fps */
#define _MAX_NUM_ALLOC_BUF                      100
#define _MAX_TRIAL_WAIT_FRAME                   10
#define _PAD_ALLOC_RETRY_PERIOD                 25
#define _MAX_PAD_ALLOC_RETRY_COUNT              300
#define _CONTINUOUS_SHOT_MARGIN                 50

#define _FD_DEFAULT     (-1)
#define _FD_MIN         (-1)
#define _FD_MAX         (1<<15) /* 2^15 == 32768 */

#define _THUMBNAIL_WIDTH    320
#define _THUMBNAIL_HEIGHT   240

#define MM_CAMERA_CH_PREVIEW_MASK    (0x01 << MM_CAMERA_CH_PREVIEW)
#define MM_CAMERA_CH_VIDEO_MASK      (0x01 << MM_CAMERA_CH_VIDEO)
#define MM_CAMERA_CH_SNAPSHOT_MASK   (0x01 << MM_CAMERA_CH_SNAPSHOT)

#define MCI_FN_ENABLE
//#define MCI_FN_ENABLE_PREVIEW
//#define MCI_FN_ENABLE_SNAPSHOT
//#define MCI_FN_ENABLE_VIDEO

#define PREVIEW_BUF_NUMBER                        4
#define VIDEO_BUF_NUMBER                            4

//Assuming preview to be faster than 3.3 FPS
#define PREVIEW_FRAME_MAX_WAIT_TIME     300000
#define VT_PREVIEW_FRAME_MAX_WAIT_TIME     1000000

#define PREVIEW_FRAME_MEM_ALIGN             1
#define ENABLE_TIMING_DATA                           1

#define QCAMERAHAL_CPPINTF_DECL_START extern "C" \
{

#define QCAMERAHAL_CPPINTF_DECL_END }

#define QCAMERAHAL_CPPINTF_DEF_START    QCAMERAHAL_CPPINTF_DECL_START
#define QCAMERAHAL_CPPINTF_DEF_END      QCAMERAHAL_CPPINTF_DECL_END

typedef enum {
	QCAMERASRC_STREAM_TYPE_DEFAULT = 0,       /* default stream type */
	QCAMERASRC_STREAM_TYPE_PREVIEW,       /* preview */
	QCAMERASRC_STREAM_TYPE_POSTVIEW,      /* postview */
	QCAMERASRC_STREAM_TYPE_SNAPSHOT,      /* snapshot */
	QCAMERASRC_STREAM_TYPE_VIDEO,         /* video */
	QCAMERASRC_STREAM_TYPE_RAW,           /* raw dump from camif */
	QCAMERASRC_STREAM_TYPE_METADATA,      /* meta data */
	QCAMERASRC_STREAM_TYPE_OFFLINE_PROC,  /* offline process */
	QCAMERASRC_STREAM_TYPE_MAX,
} qcamerasrc_stream_type_t;

typedef enum {
    QCAMERASRC_COLOR_CTRL_BRIGHTNESS = 0,          /**< Brightness control entry*/
    QCAMERASRC_COLOR_CTRL_CONTRAST,                /**< Contrast control entry*/
    QCAMERASRC_COLOR_CTRL_WHITE_BALANCE,           /**< White balance control entry*/
    QCAMERASRC_COLOR_CTRL_COLOR_TONE,              /**< Color tone control entry*/
    QCAMERASRC_COLOR_CTRL_SATURATION,              /**< Saturation value control */
    QCAMERASRC_COLOR_CTRL_SHARPNESS,               /**< Sharpness value control */
    QCAMERASRC_COLOR_CTRL_NUM,                     /**< Number of Controls*/
}qcamerasrc_color_ctrl_t;

#define QCAMERASRC_PARAM_CONTROL_START  (int64_t)(1)
#define QCAMERASRC_PARAM_CONTROL_PREVIEW_SIZE QCAMERASRC_PARAM_CONTROL_START<<1
#define QCAMERASRC_PARAM_CONTROL_PREVIEW_FORMAT QCAMERASRC_PARAM_CONTROL_START<<2
#define QCAMERASRC_PARAM_CONTROL_VIDEO_SIZE QCAMERASRC_PARAM_CONTROL_START<<3
#define QCAMERASRC_PARAM_CONTROL_VIDEO_FORMAT QCAMERASRC_PARAM_CONTROL_START<<4
#define QCAMERASRC_PARAM_CONTROL_RECORDING_HINT QCAMERASRC_PARAM_CONTROL_START<<5
#define QCAMERASRC_PARAM_CONTROL_SNAPSHOT_SIZE QCAMERASRC_PARAM_CONTROL_START<<6
#define QCAMERASRC_PARAM_CONTROL_SNAPSHOT_FORMAT QCAMERASRC_PARAM_CONTROL_START<<7
#define QCAMERASRC_PARAM_CONTROL_FPS_MODE QCAMERASRC_PARAM_CONTROL_START<<8
#define QCAMERASRC_PARAM_CONTROL_FPS QCAMERASRC_PARAM_CONTROL_START<<9
#define QCAMERASRC_PARAM_CONTROL_BURST_CONFIG QCAMERASRC_PARAM_CONTROL_START<<10
#define QCAMERASRC_PARAM_CONTROL_FACE_DETECT QCAMERASRC_PARAM_CONTROL_START<<11
#define QCAMERASRC_PARAM_CONTROL_FLASH_MODE QCAMERASRC_PARAM_CONTROL_START<<12
#define QCAMERASRC_PARAM_CONTROL_BRIGHTNESS QCAMERASRC_PARAM_CONTROL_START<<13
#define QCAMERASRC_PARAM_CONTROL_CONTRAST QCAMERASRC_PARAM_CONTROL_START<<14
#define QCAMERASRC_PARAM_CONTROL_COLOR_TONE QCAMERASRC_PARAM_CONTROL_START<<15
#define QCAMERASRC_PARAM_CONTROL_PROGRAM_MODE QCAMERASRC_PARAM_CONTROL_START<<16
#define QCAMERASRC_PARAM_CONTROL_PARTCOLOR_SRC QCAMERASRC_PARAM_CONTROL_START<<17
#define QCAMERASRC_PARAM_CONTROL_PARTCOLOR_DST QCAMERASRC_PARAM_CONTROL_START<<18
#define QCAMERASRC_PARAM_CONTROL_PARTCOLOR_MODE QCAMERASRC_PARAM_CONTROL_START<<19
#define QCAMERASRC_PARAM_CONTROL_WIDE_DYNAMIC_RANGE QCAMERASRC_PARAM_CONTROL_START<<20
#define QCAMERASRC_PARAM_CONTROL_SATURATION QCAMERASRC_PARAM_CONTROL_START<<21
#define QCAMERASRC_PARAM_CONTROL_SHARPNESS QCAMERASRC_PARAM_CONTROL_START<<22
#define QCAMERASRC_PARAM_CONTROL_PHOTOMETRY QCAMERASRC_PARAM_CONTROL_START<<23
#define QCAMERASRC_PARAM_CONTROL_METERING QCAMERASRC_PARAM_CONTROL_START<<24
#define QCAMERASRC_PARAM_CONTROL_WB QCAMERASRC_PARAM_CONTROL_START<<25
#define QCAMERASRC_PARAM_CONTROL_ISO QCAMERASRC_PARAM_CONTROL_START<<26
#define QCAMERASRC_PARAM_CONTROL_ANTI_SHAKE QCAMERASRC_PARAM_CONTROL_START<<27
#define QCAMERASRC_PARAM_CONTROL_LOW_LIGHT_DETECTION QCAMERASRC_PARAM_CONTROL_START<<28
#define QCAMERASRC_PARAM_CONTROL_BURST_SHOT QCAMERASRC_PARAM_CONTROL_START<<29
#define QCAMERASRC_PARAM_CONTROL_ZOOM QCAMERASRC_PARAM_CONTROL_START<<30
#define QCAMERASRC_PARAM_CONTROL_AF QCAMERASRC_PARAM_CONTROL_START<<31
#define QCAMERASRC_PARAM_CONTROL_AF_AREAS QCAMERASRC_PARAM_CONTROL_START<<32
#define QCAMERASRC_PARAM_CONTROL_SCENE_MODE QCAMERASRC_PARAM_CONTROL_START<<33
#define QCAMERASRC_PARAM_CONTROL_HDR QCAMERASRC_PARAM_CONTROL_START<<34
#define QCAMERASRC_PARAM_CONTROL_PREVIEW_FLIP QCAMERASRC_PARAM_CONTROL_START<<35
#define QCAMERASRC_PARAM_CONTROL_VIDEO_FLIP QCAMERASRC_PARAM_CONTROL_START<<36
#define QCAMERASRC_PARAM_CONTROL_SNAPSHOT_FLIP QCAMERASRC_PARAM_CONTROL_START<<37
#define QCAMERASRC_PARAM_CONTROL_SHOOTING_MODE QCAMERASRC_PARAM_CONTROL_START<<38
#define QCAMERASRC_PARAM_CONTROL_DUAL_CAMERA_MODE QCAMERASRC_PARAM_CONTROL_START<<39
#define QCAMERASRC_PARAM_CONTROL_VDIS QCAMERASRC_PARAM_CONTROL_START<<40
#define QCAMERASRC_PARAM_CONTROL_JPEG_QUALITY QCAMERASRC_PARAM_CONTROL_START<<41
#define QCAMERASRC_PARAM_CONTROL_DUAL_RECORDING_HINT QCAMERASRC_PARAM_CONTROL_START<<42
#define QCAMERASRC_PARAM_CONTROL_AE_LOCK QCAMERASRC_PARAM_CONTROL_START<<43
#define QCAMERASRC_PARAM_CONTROL_AWB_LOCK QCAMERASRC_PARAM_CONTROL_START<<44
#define QCAMERASRC_PARAM_CONTROL_VT_MODE QCAMERASRC_PARAM_CONTROL_START<<45
#define QCAMERASRC_PARAM_CONTROL_END QCAMERASRC_PARAM_CONTROL_START<<46

typedef struct {
    int preview_width;
    int preview_height;
    int preview_format;
    int video_width;
    int video_height;
    int video_format;
    int recording_hint;
    int hfr_value;
    int snapshot_width;
    int snapshot_height;
    int snapshot_format;
    int fps_mode;
    int fps_min;
    int fps_max;
    int burst_number;
    int burst_interval;
    int face_detect;
    int flash_mode;
    int brightness_value;
    int contrast_value;
    int color_tone_value;
    int program_mode;
    int partcolor_src;
    int partcolor_dest;
    int partcolor_mode;
    int wdr_value;
    int saturation_value;
    int sharpness_value;
    int photometry_value;
    int metering_value;
    int wb_value;
    int iso_value;
    int antishake_value;
    int lld_value;
    int burst_shot_config;
    int zoom_value;
    int af_config;
    char * af_area;
    int scene_mode;
    int hdr_ae_bracket;
    int preview_flip;
    int video_flip;
    int snapshot_flip;
    int shoting_mode;
    int dual_cam_mode;
    int vdis_mode;
    int jpeg_quality;
    int dual_recording_hint;
    int ae_lock;
    int awb_lock;
    int vt_mode;
} qcamerasrc_batch_ctrl_t;

#if ENABLE_TIMING_DATA
#define TIMER_START(arg...) \
if(arg){ \
    g_timer_reset(arg); \
}else{ \
    arg=g_timer_new(); \
}
#define TIMER_MSG(fmt,arg...) \
if(arg){ \
    fprintf(stderr,"\n\x1b[44m\x1b[37m QCameraSrc:TIMER [%s:%05d]  " fmt ,__func__, __LINE__,g_timer_elapsed(arg,NULL) ); \
    fprintf(stderr,"\x1b[0m\n"); \
}
#define TIMER_STOP(arg...) \
if(arg){ \
    g_timer_stop(arg); \
    g_timer_destroy(arg); \
    arg = NULL; \
}
#else
#define TIMER_START(arg...)
#define TIMER_MSG(fmt,arg...)
#define TIMER_STOP(arg...)
#endif

#define ENABLE_ZERO_COPY                       1

#define ION_DEVICE                          "/dev/ion"

#define IMAGE_1MP_WIDTH      1280
#define IMAGE_1MP_HEIGHT      960
#define IMAGE_2MP_WIDTH      1600
#define IMAGE_2MP_HEIGHT     1200
#define IMAGE_3MP_WIDTH      2048
#define IMAGE_3MP_HEIGHT     1536
#define IMAGE_5MP_WIDTH      2592
#define IMAGE_5MP_HEIGHT     1944
#define IMAGE_5MP_BALTIC_WIDTH      2608
#define IMAGE_5MP_BALTIC_HEIGHT     1960

#define FRAME_VGA_WIDTH                             640
#define FRAME_VGA_HEIGHT                            480
#define FRAME_WVGA_WIDTH                        800
#define FRAME_WVGA_HEIGHT                       480


#define MAX_NUM_CTRL_LIST_INFO  64
#define MAX_NUM_CTRL_MENU       64
#define MAX_SZ_CTRL_NAME_STRING 32
#define MAX_SZ_DEV_NAME_STRING  32

//HDR
#define HDR_BRACKET_FRAME_NUM                   3
#define SCREEN_NAIL_BUFFER_HANDLE           -32

#endif /*__GSTQCAMERASRC_DEFS_H__*/
