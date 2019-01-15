/*
 * texture
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jian He <rocket.he@samsung.com>
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

#ifndef __TEXTURE_H_
#define __TEXTURE_H_

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <X11/Xlib.h>
#include <TizenGLManager.h>
#include <SecImagingNativeFilterDefs.h>
#include <tbm_surface.h>
#include <tbm_surface_internal.h>
#include <X11/Xmd.h>
#include <dri2/dri2.h>
#include <mm_util_jpeg.h>
#include "pip-define.h"
#include <tbm_bufmgr.h>
#include <exynos_drm.h>
#include <sys/ioctl.h>
#include <mm_types.h>

#define SURFACE_COUNT    6
#define MAX_INPUT_BUFFER 30
#define PIP_WAIT_TIMEOUT            1000000 /* usec */
#define BUFFER_WAIT_TIMEOUT         500000  /* usec */

enum BUFFER_TYPE {
	BUF_INVALID = -1,
	BUF_0 = 0,
	BUF_1,
	BUFFER_MAX
};

enum HANDLE_BUFFER_TYPE {
	HB_NOT_INIT = 0,
	HB_WAIT_HANDLE,
	HB_HANDLING
};

enum THREAD_TYPE {
	TH_HANDLE_BUF = 0,
	TH_HANDLE_OUT_BUF,
	TH_CAPTURE,
	THREAD_MAX
};

enum MUTEX_TYPE {
	MU_INPUT_QUEUE = 0,
	MU_OUTPUT_QUEUE,
	MU_CAPTURE_QUEUE,
	MU_PRO_CHANGE,
	MU_PIXMAP,
	MUTEX_MAX
};


// Dual Camera GPU Effects (SISO pip style) styles are named by GD_XX
typedef enum {
	FILTER_VALUE_GD_NORMAL_PIP = 0,
	FILTER_VALUE_GD_CUBISM,
	FILTER_VALUE_GD_POSTCARD,
	FILTER_VALUE_GD_SIGNATURE,
	FILTER_VALUE_GD_OVAL_BLUR,
	FILTER_VALUE_GD_SHINY,
	FILTER_VALUE_GD_SHAPE_HEART,
	FILTER_VALUE_GD_EXPOSER_OVERLAY,
	FILTER_VALUE_GD_SPLIT_VIEW,
	FILTER_VALUE_GD_POLAROID,
	FILTER_VALUE_GD_PIP_FISHEYE,
	FILTER_VALUE_GD_FRAME_OVERLAY,
	FILTER_VALUE_GD_TRACKING,
    FILTER_VALUE_GD_MAX
} FilterType;

typedef enum {
	ORIENTATION_LEFT = 0,
	ORIENTATION_DOWN,
	ORIENTATION_RIGHT,
	ORIENTATION_UP,
	ORIENTATION_MAX
} OrientationType;

typedef enum {
	IWD_FRONT = 0,
	IWD_REAR,
	IWD_MAX
} InsetWindowDeviceType;

typedef enum {
	REAR_CAMERA = 0, /*single camera - rear*/
	FRONT_CAMERA, /*single camera - front*/
	DUAL_CAMERA, /*dual camera*/
	CAMERA_MODE_MAX
} CameraMode;

typedef enum {
	CAPTURE_DATA_JPEG = 0,  /* JPEG data */
	CAPTURE_DATA_RAW,       /* RAW data */
} CaptureDataType;

//Single Camera GPU Effects (SISO filter) filters are named by GS_XX
typedef enum {
	FILTER_VALUE_GS_NO_EFFECT = 0,
	FILTER_VALUE_GS_SEPIA,
	FILTER_VALUE_GS_BW,
	FILTER_VALUE_GS_NEGATIVE,
	FILTER_VALUE_GS_OLDPHOTO,
	FILTER_VALUE_GS_SUNSHINE,
	FILTER_VALUE_GS_VINTAGE,
	FILTER_VALUE_GS_RETRO,
	FILTER_VALUE_GS_FADEDCOLOR,
	FILTER_VALUE_GS_NOSTALGIA,
	FILTER_VALUE_GS_COMIC,
	FILTER_VALUE_GS_PASTEL_SKETCH,
	FILTER_VALUE_GS_GOTHIC_NOIR,
	FILTER_VALUE_GS_IMPRESSIONIST,
	FILTER_VALUE_GS_SANDSTONE,
	FILTER_VALUE_GS_RAINBOW,
	FILTER_VALUE_GS_INSTAGRAM_NASHVILLE,
	FILTER_VALUE_GS_FISHEYE,
	FILTER_VALUE_GS_FOR_REAL,
	FILTER_VALUE_GS_STUCCHEVOLE,
	FILTER_VALUE_GS_NOIR_NOTE,
	FILTER_VALUE_GS_VINCENT,
	FILTER_VALUE_GS_VIGNETTE,
	FILTER_VALUE_GS_DOWNLOAD_EFFECT,
	FILTER_VALUE_GS_MAX
} SingleCameraEffect;

enum PROPERTY_CHANGE_TYPE {
	PROC_IW = 0, //inset window was changed
	PROC_IWPC, //inset window preview crop was changed
	PROC_STYLE, //style was changed
	PROC_ORIEN, //orientation was changed
	PROC_IWD, //inset window device was changed
	PROC_CM, //camera mode was changed
	PROC_EFFECT, //single camera effect was changed
	PROC_DOWNLOAD_EFFECT, //single camera effect was changed
	PROC_HIW, //Hide inset window was changed
	PROC_FADING,
	PROC_CAPTURE_SIZE, //capture size was changed
	PROC_ROI_INFO,
	PROC_MAX
};

typedef struct _OutputData {
	tbm_surface_h surface;
	int buf_share_method;
	gint pixmap_id;
	GstClockTime timestamp;
	GstClockTime duration;
} OutputData;

typedef struct _CaptureJpegData {
	/* JPEG data */
	void *data;
	/* JPEG size */
	int size;
	/*type */
	CaptureDataType type;
	/* Image Width */
	int width;
	/* Image height */
	int height;
	/* Image format */
	int fourcc;
} CaptureJpegData;

typedef struct _CaptureRawData {
	/* RAW data */
	void *data;
	/* size */
	int size;
} CaptureRawData;

typedef struct _PipGlueFilterArea PipGlueFilterArea;

typedef struct {
	int xAxis;
	int yAxis;
} PipWinsetArea;

struct _PipGlueFilterArea {	// in landscape mode
	PipWinsetArea startxy;
	int width;
	int height;
	PipWinsetArea roi_left_top;
	PipWinsetArea roi_right_top;
	PipWinsetArea roi_left_bottom;
	PipWinsetArea roi_right_bottom;
	gboolean stop_render;
};


typedef struct _CaptureData {
	/* data */
	void *data;
	/* size */
	int size;
	/*type */
	CaptureDataType type;
} CaptureData;

typedef struct _RectPosition {
	int x;
	int y;
	int w;
	int h;
} RectPosition;

typedef struct _InputBuffer {
	int dma_buf_fd;
	int gem_handle;
	tbm_bo bo;
	tbm_surface_h surface;
} InputBuffer;

typedef struct _AppData {
	int fd;
	tbm_bufmgr bufmgr;

	/*GL Manager*/
	Display *display;
	Window window;
	/*GLM instance for preview*/
	tbm_surface_h surface_buffer[SURFACE_COUNT];
	gboolean surface_using[SURFACE_COUNT];
	gboolean has_setted_ibp;/*has setted input buffer property*/
	gboolean has_created_glm;
	gboolean ready_for_glm;
	TizenGLManager_t *manager;
	/*GLM instance for capture*/
	Display *capture_display;
	gboolean has_created_capture_glm;
	Pixmap capture_pixmap;
	tbm_surface_h capture_surface;
	tbm_surface_h surface[2];
	tbm_bo input_bo[2];
	TizenGLManager_t *capture_manager;
	gint capture_w;
	gint capture_h;

	GMutex *thread_mutex[MUTEX_MAX];
	GCond *thread_cond[MUTEX_MAX];
	GThread *threads[THREAD_MAX];
	GQueue  *input[BUFFER_MAX];
	GQueue *capture_buffer_list[BUFFER_MAX];            /**< queue for buffer to capture */
	GQueue  *output;

	gboolean exit_loop[THREAD_MAX];

	/*property*/
	gint inset_window_x;
	gint inset_window_y;
	gint inset_window_w;
	gint inset_window_h;
	gint inset_window_preview_crop_x;
	gint inset_window_preview_crop_y;
	gint inset_window_preview_crop_w;
	gint inset_window_preview_crop_h;
	gint style_type;       /*enum type*/
	gint orientation;        /*enum type*/
	gint inset_window_device;        /*enum type*/
	gint camera_mode;		/*enum type*/
	gint single_camera_effect;	/*enum type*/
	gchar *single_camera_download_effect;	/*string type*/
	gboolean signal_still_capture;          /**< enable still capture signal */
	gboolean hide_inset_window;             /**< hide inset window */
	gboolean enable_fading;                 /**< enable fading when starts */
	gboolean property_change[PROC_MAX];
	gint capture_width;
	gint capture_height;
	gboolean ready_to_stop;                 /**< ready to stop */
	gboolean buffer_flush_done;             /**< buffer flush done */
	gboolean property_updated;

	/*input buffer property*/
	gint width[2];
	gint height[2];
	gint rate;
	gint scale;
	guint32 fourcc;

	/*buffer share method*/
	int buf_share_method;

	/*input buffer*/
	InputBuffer in_buf[MAX_INPUT_BUFFER];

	PipGlueFilterArea roi_info;

	gint input_cnt;

} AppData;

/*! @struct camerasrc_buffer_t
 *  @brief data buffer
 *  Image data buffer
 */
typedef struct _pip_buffer_t {
	unsigned int size;    /**< Size of stored data */
	unsigned char *start;   /**< Start address of data */
	int width;                                      /**< width of image */
	int height;                                     /**< height of image */
} pip_buffer_t;

typedef struct _capture_buffer_info_t {
	void *data;                                     /**< data pointer of image */
	int size;                                       /**< data size of image */
	int fourcc;                                     /**< format of image */
	int width;                                      /**< width of image */
	int height;                                     /**< height of image */
} capture_buffer_info_t;

gint get_available_pixmap_id(AppData *app_data);
gboolean init_gl_manager(AppData *app_data);
void create_gl_manager(AppData *app_data);
void create_window(AppData *app_data);
void destroy_window(AppData *app_data);
gboolean destroy_gl_manager(AppData *app_data);
void destroy_OutputData(gpointer data);
gboolean create_capture_glm(AppData *app_data, gint capture_w, gint capture_h);
gboolean destroy_capture_glm(AppData *app_data);
void destroy_CaptureJpegData(gpointer data);
void destroy_CaptureRawData(gpointer data);
gboolean destroy_app_data(AppData *app_data);
AppData *init_app_data();
gboolean init_working_threads(AppData *app_data);
gboolean destroy_working_threads(AppData *app_data);
gboolean init_input_buf_property(AppData *app_data, gint width_0, gint height_0, gint width_1, gint height_1, gint rate, gint scale, guint32 fourcc);
gboolean handle_camerasrc_buffer(AppData *app_data, GstBuffer *buf[BUFFER_MAX]);
int tbm_export_buffer(AppData *app_data, int id, int dmabuf_fd, tbm_surface_info_s surface_info, unsigned int *gem_handle, tbm_surface_h *buffer);
int tbm_export_buffer2(AppData *app_data, int id, tbm_bo bo, tbm_surface_info_s surface_info, tbm_surface_h *buffer);
void gem_handle_close(int drm_fd, unsigned int gem_handle);
gboolean is_dual_camera_mode(AppData *app_data);

#endif
