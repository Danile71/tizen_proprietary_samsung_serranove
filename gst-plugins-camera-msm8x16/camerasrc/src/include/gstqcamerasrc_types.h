/*
 * gstqcamerasrc_types.h
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

#ifndef __GSTQCAMERASRC_TYPES_H__
#define __GSTQCAMERASRC_TYPES_H__

#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <math.h>
#include <gst/gst.h>
#include <asm/types.h>
#include <linux/videodev2.h>

#include "gstqcamerasrc_errors.h"
#include "gstqcamerasrc_defs.h"
#include "mm_camera_interface.h"

typedef enum {
    CAMERASRC_AUTO_FOCUS_RESULT_INVALID = -1,   /**< AF Invalid */
    CAMERASRC_AUTO_FOCUS_RESULT_RESTART = 0,    /**< AF Restart */
    CAMERASRC_AUTO_FOCUS_RESULT_FUCUSING,       /**< AF Focusing */
    CAMERASRC_AUTO_FOCUS_RESULT_FOCUSED,        /**< AF Focused */
    CAMERASRC_AUTO_FOCUS_RESULT_FAILED,         /**< AF failed */
    CAMERASRC_AUTO_FOCUS_RESULT_NUM,            /**< Number of AF result */
}camerasrc_auto_focus_result_t;

typedef enum {
    CAMERASRC_AUTO_FOCUS_STATUS_RELEASED,
    CAMERASRC_AUTO_FOCUS_STATUS_ONGOING,
    CAMERASRC_AUTO_FOCUS_STATUS_NUM,
}camerasrc_auto_focus_status_t;

typedef enum {
    CAMERASRC_DEV_ID_PRIMARY,
    CAMERASRC_DEV_ID_SECONDARY,
    CAMERASRC_DEV_ID_EXTENSION,
    CAMERASRC_DEV_ID_UNKNOWN,
    CAMERASRC_DEV_ID_NUM,
}camerasrc_dev_id_t;

typedef enum {
    CAMERASRC_PIX_NONE = -1,      /**< Default value or Not supported */
    CAMERASRC_PIX_YUV422P = 0,    /**< Pixel format like YYYYYYYYUUUUVVVV*/
    CAMERASRC_PIX_YUV420P,        /**< Pixel format like YYYYYYYYUUVV*/
    CAMERASRC_PIX_YUV420,         /**< Pixel format like YYYYYYYYUVUV*/
    CAMERASRC_PIX_SN12,           /**< YUV420 (interleaved, non-linear) */
    CAMERASRC_PIX_ST12,           /**< YUV420 (interleaved, tiled, non-linear) */
    CAMERASRC_PIX_YUY2,           /**< YUV 4:2:2 as for UYVY but with different component ordering within the u_int32 macropixel */
    CAMERASRC_PIX_RGGB8,          /**< Raw RGB Pixel format like CCD order, a pixel consists of 8 bits, Actually means JPEG + JPEG image output */
    CAMERASRC_PIX_RGGB10,         /**< Raw RGB Pixel format like CCD order, a pixel consists of 10 bits, Actually means JPEG + YUV image output */
    CAMERASRC_PIX_RGB565,         /**< Raw RGB Pixel format like CCD order, a pixel consists of 10 bits, Actually means JPEG + YUV image output */
    CAMERASRC_PIX_UYVY,           /**< YUV 4:2:2 */
    CAMERASRC_PIX_NV12,           /**< YUV 4:2:0, 8-bit Y plane followed by an interleaved U/V plane with 2x2 subsampling */
    CAMERASRC_PIX_NV21,           /**< YUV 4:2:0, 8-bit Y plane followed by an interleaved V/U plane with 2x2 subsampling */
    CAMERASRC_PIX_NUM,            /**< Number of pixel formats*/
}camerasrc_pix_format_t;

typedef enum {
    CAMERASRC_COL_NONE = -1,    /**< Default value or Not supported */
    CAMERASRC_COL_RAW,   /**< Non-compressed RGB/YUV pixel data*/
    CAMERASRC_COL_JPEG, /**< Compressed jpg data*/
    CAMERASRC_COL_NUM,  /**< Number of colorspace data*/
}camerasrc_colorspace_t;

typedef enum {
    VIDEO_IN_MODE_UNKNOWN,
    VIDEO_IN_MODE_PREVIEW,
    VIDEO_IN_MODE_VIDEO,
    VIDEO_IN_MODE_CAPTURE,
    VIDEO_IN_MODE_STOP,
}camerasrc_video_mode_t;

typedef enum {
    GST_CAMERASRC_QUALITY_LOW,
    GST_CAMERASRC_QUALITY_HIGH,
}camerasrc_capture_quality;

typedef enum {
    CAMERASRC_SENSOR_MODE_CAMERA = 0,
    CAMERASRC_SENSOR_MODE_MOVIE,
}camerasrc_sensor_mode_t;

typedef enum {
    PREVIEW_BUFFER = 0,
    RECORD_BUFFER,
}camerasrc_buffer_type_t;

typedef enum {
    CAMERASRC_HDR_CAPTURE_OFF,
    CAMERASRC_HDR_CAPTURE_ON,
    CAMERASRC_HDR_CAPTURE_ON_AND_ORIGINAL,
} camerasrc_hdr_mode;

typedef enum {
    CAMERASRC_FLIP_NONE,
    CAMERASRC_FLIP_HORIZONTAL,
    CAMERASRC_FLIP_VERTICAL,
    CAMERASRC_FLIP_BOTH,
} camerasrc_flip_mode;

typedef enum {
    LOW_LIGHT_DETECTION_OFF,
    LOW_LIGHT_DETECTION_ON,
} camerasrc_low_light_detection_mode;

typedef enum {
	CAMERASRC_FACE_DETECT_OFF,
	CAMERASRC_FACE_DETECT_ON,
} camerasrc_face_detect_mode;

typedef enum {
    LOW_LIGHT_INIT = -1,
    LOW_LIGHT_NOT_DETECTED,
    LOW_LIGHT_IS_DETECTED,
} camerasrc_low_light_condition;

typedef enum {
    LOW_LIGHT_MODE_OFF,
    LOW_LIGHT_MODE_ON,
} camerasrc_low_light_mode;

typedef enum {
    VT_MODE_OFF = 0,
    VT_MODE_ON,
    VT_MODE_HD,
    VT_MODE_INTELLIGENT,
} camerasrc_vt_mode;

typedef struct _camerasrc_handle_t {
#ifdef CAMERA_INTF
    void* preview_stream;
    void* snapshot_stream;
    void* record_stream;
    void* cam_device_handle;
    void* hw_device_data;
    struct camera_info *cam_info_data;
#endif
    int     msm_cam_id;
} camerasrc_handle_t;


typedef struct _camerasrc_rect_t {
    int x;
    int y;
    int width;
    int height;
}camerasrc_rect_t;

typedef enum {
    BUF_SHARE_METHOD_PADDR = 0,
    BUF_SHARE_METHOD_FD,
    BUF_SHARE_METHOD_TIZEN_BUFFER,
    BUF_SHARE_METHOD_FLUSH_BUFFER
} buf_share_method_t;

typedef struct _capture_buffer_info_t {
	void *data;                                     /**< data pointer of image */
	int size;                                       /**< data size of image */
	int fourcc;                                     /**< format of image */
	int width;                                      /**< width of image */
	int height;                                     /**< height of image */
} capture_buffer_info_t;

/*! @struct camerasrc_buffer_t
 *  @brief data buffer
 *  Image data buffer
 */
typedef struct _qcamerasrc_buffer_t {
	/* Supports for Planes & DMA-buf */
	struct {
		unsigned int length;    /**< Size of stored data */
		unsigned char *start;   /**< Start address of data */
		int fd;                 /* dmabuf-fd */
	} planes[4];                    /* planes: SCMN_IMGB_MAX_PLANE */
	int num_planes;
	//camerasrc_buffer_queued_status queued_status;   /**< Queued or Dequeued status */
	//unsigned char *p_embedded_data;                 /**< Pointer of embedded data(available if ITLV format) */
	int width;                                      /**< width of image */
	int height;                                     /**< height of image */
} qcamerasrc_buffer_t;

/*! @struct camerasrc_ctrl_menu_t
 *  @brief For querying menu of specified controls
 */
typedef struct {
    int menu_index;                                             /**< What number is used for accessing this menu */
    char menu_name[MAX_SZ_CTRL_NAME_STRING];                    /**< name of each menu */
}qcamerasrc_ctrl_menu_t;

/*! @struct camerasrc_ctrl_info_t
 *  @brief For querying controls detail
 */
typedef struct {
    int64_t camsrc_ctrl_id;                             /**< camsrc camera control ID for controlling this */
    int v4l2_ctrl_id;                                           /**< v4l2 ctrl id, user not need to use this. see @struct camerasrc_ctrl_t */
    int ctrl_type;                                              /**< Type of this control */
    char ctrl_name[MAX_SZ_CTRL_NAME_STRING];                    /**< Name of this control */
    int min;                                                    /**< minimum value */
    int max;                                                    /**< maximum value */
    int step;                                                   /**< unit of the values */
    int default_val;                                            /**< Default value of the array or range */
    int num_ctrl_menu;                                          /**< In the case of array type control, number of supported menu information */
    qcamerasrc_ctrl_menu_t ctrl_menu[MAX_NUM_CTRL_MENU];         /**< @struct camerasrc_ctrl_menu_t for detailed each menu information*/
} qcamerasrc_ctrl_info_t;

/*! @struct camerasrc_ctrl_list_info_t
 *  @brief For querying controls
 */
typedef struct {
    int num_ctrl_list_info;                                     /**< Number of supported controls */
    qcamerasrc_ctrl_info_t ctrl_info[MAX_NUM_CTRL_LIST_INFO];    /**< @struct camerasrc_ctrl_info_t for each control information */
} qcamerasrc_ctrl_list_info_t;

typedef enum {
	INTERFACE_NONE,
	INTERFACE_COLOR_BALANCE,
	INTERFACE_CAMERA_CONTROL,
} GstInterfaceType;

static char *qcamerasrc_ctrl_label[QCAMERASRC_COLOR_CTRL_NUM] =
{
    "brightness",               /**< label for CAMERASRC_CTRL_BRIGHTNESS */
    "contrast",                 /**< label for CAMERASRC_CTRL_CONTRAST */
    "white balance",            /**< label for CAMERASRC_CTRL_WHITE_BALANCE */
    "color tone",               /**< label for CAMERASRC_CTRL_COLOR_TONE */
    "saturation",               /**< label for CAMERASRC_CTRL_SATURATION */
    "sharpness",                /**< label for CAMERASRC_CTRL_SHARPNESS */
};

#endif /*__GSTQCAMERASRC_TYPES_H__*/
