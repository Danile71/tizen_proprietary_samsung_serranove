 /*
 * gstqcamerasrc.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gst/gst.h>
#include <gst/gstutils.h>
#include <glib-object.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sched.h> // sched_yield()

#include <camsrcjpegenc.h>
#include "gstqcamerasrc.h"
#include "gstcamerasrccontrol.h"
#include "gstcamerasrccolorbalance.h"

#include <mm_util_imgp.h>
#include <mm_util_jpeg.h>

//#define DEVELOPER_TEST
#ifdef DEVELOPER_TEST
static pthread_t mRecordThread;
#endif
GST_DEBUG_CATEGORY (gst_qcamera_src_debug);
#define GST_CAT_DEFAULT gst_qcamera_src_debug
#if ENABLE_TIMING_DATA
static GTimer *capture_timer = NULL;
#endif

#define _PREVIEW_BUFFER_WAIT_TIMEOUT            1000000 /* usec */
#define _VIDEO_BUFFER_WAIT_TIMEOUT              100000  /* usec */
#define DEFAULT_JPEG_ENCODE_QUALITY             85 /* from QCameraParameters */
#define ENV_NAME_USE_HW_CODEC           "IMAGE_UTIL_USE_HW_CODEC"
#define MAX_FACE_NUM 10

#define SAFE_FREE_GMUTEX(gmutex) \
	if (gmutex) { \
		g_mutex_free(gmutex); \
		gmutex = NULL; \
	}

#define SAFE_FREE_GCOND(gcond) \
	if (gcond) { \
		g_cond_free(gcond); \
		gcond = NULL; \
	}

#define SAFE_FREE_GQUEUE(gqueue) \
	if (gqueue) { \
		g_queue_free(gqueue); \
		gqueue = NULL; \
	}
//#ifdef SAMSUNG_HDR_SISO
#define _DEFAULT_NUM_OF_CPU_CORES               4
#define _DEFAULT_HDR_METHOD                     SS_MODE_HDRTMO
#define _DEFAULT_HDR_FORMAT                     SS_FORMAT_NV12
//#endif

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

#define MSM_ALIGN(w,h) ALIGN(ALIGN(w, 128) * ALIGN(h, 32) + ALIGN(w, 128) * ALIGN((h+1)/2, 16) + (4*1024)+(8*1024), (4*1024)) // 4K buffer alignment for QC component

enum {
    SIGNAL_STILL_CAPTURE,
    SIGNAL_VIDEO_STREAM_CB,
    SIGNAL_MEDIAPACKET_RECORD_CB,
    SIGNAL_NEGO_COMPLETE,
    LAST_SIGNAL
};

enum {
    GST_QCAMERASRC_CMD_NONE,
    GST_QCAMERASRC_CMD_PREVIEW_RESTART,
    GST_QCAMERASRC_CMD_RECORD_START,
    GST_QCAMERASRC_CMD_RECORD_STOP,
    GST_QCAMERASRC_CMD_CAPTURE_START,
    GST_QCAMERASRC_CMD_CAPTURE_STOP
};

enum {
    PROP_0,
    PROP_CAMERA_AUTO_FPS,
    PROP_CAMERA_ID,
    PROP_CAMERA_CAPTURE_FOURCC,
    PROP_CAMERA_CAPTURE_WIDTH,
    PROP_CAMERA_CAPTURE_HEIGHT,
    PROP_CAMERA_CAPTURE_JPG_QUALITY,
    PROP_CAMERA_CAPTURE_PROVIDE_EXIF,
    PROP_CAMERA_VIDEO_WIDTH,
    PROP_CAMERA_VIDEO_HEIGHT,
    PROP_CAMERA_ENABLE_HYBRID_MODE,
    PROP_VFLIP,
    PROP_HFLIP,
    PROP_SENSOR_ORIENTATION,
    PROP_CAMERA_CPU_LOCK_MODE,
    PROP_VT_MODE,
    PROP_STOP_CAMERA,
    PROP_OPEN_CAMERA,
#ifdef CONFIG_MACH_Z3LTE
    PROP_CAMERA_HIGH_SPEED_FPS,
    PROP_CAMERA_CAPTURE_QUALITY,
    PROP_CAMERA_CAPTURE_INTERVAL,
    PROP_CAMERA_CAPTURE_COUNT,
    PROP_CAMERA_CAPTURE_HDR,
    PROP_CAMERA_ENABLE_ZSL_MODE,
#else
    PROP_CAMERA_ENABLE_VDIS,
#endif
    PROP_CAMERA_SHOT_MODE,
#ifdef CONFIG_MACH_Z3LTE
    PROP_CAMERA_LOW_LIGHT_AUTO_DETECTION,
    PROP_SIGNAL_STILLCAPTURE,
 #else
    PROP_DUAL_CAMERA_MODE,
#endif
    PROP_CAMERA_ENABLE_MEDIAPACKET_RECORD_CB,
    PROP_NUM,
};
static void gst_camerasrc_uri_handler_init (gpointer g_iface, gpointer iface_data);

static guint gst_qcamerasrc_signals[LAST_SIGNAL] = { 0 };

//Element template variables
static GstStaticPadTemplate src_factory =
    GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw,"
        "format = (string) { I420 }, "
        "width = (int) [ 1, 4096 ], "
        "height = (int) [ 1, 4096 ]; "
        "video/x-raw,"
        "format = (string) { NV12 }, "
        "width = (int) [ 1, 4096 ], "
        "height = (int) [ 1, 4096 ]; "
        "video/x-raw,"
        "format = (string) { SN12 }, "
        "width = (int) [ 1, 4096 ], "
        "height = (int) [ 1, 4096 ]; "
        "video/x-raw,"
        "format = (string) { NV21 }, "
        "width = (int) [ 1, 4096 ], "
        "height = (int) [ 1, 4096 ]; ")
    );
#define GST_TYPE_QCAMERASRC_QUALITY (gst_qcamerasrc_quality_get_type ())
#define GST_IS_QCAMERASRC_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_QCAMERASRC_BUFFER))
#define GST_QCAMERASRC_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_QCAMERASRC_BUFFER, GstQCameraBuffer))

/******************************************************************************
 * Function Declarations
 *******************************************************************************/

/*********************************Util functions***********************************/
static void GenerateYUV420BlackFrame(unsigned char *buf, int buf_size, int width, int height);
#if _ENABLE_CAMERASRC_DEBUG
static int __util_write_file(char *filename, void *data, int size);
static int data_dump(char *filename, void *data, int size);
#endif
static gboolean gst_qcamerasrc_get_timeinfo(GstCameraSrc * camerasrc, GstBuffer* buffer);
static gboolean gst_qcamerasrc_get_realtimeinfo(GstCameraSrc * camerasrc , GstClockTime *time, GstClockTime *dur);
static gboolean gst_qcamerasrc_is_zero_copy_format(const gchar *format_str);
static gboolean gst_qcamerasrc_process_cmd(GstCameraSrc *camerasrc, int cmd);
static int cache_settings_for_batch_update(GstCameraSrc* camerasrc,int64_t control_type,int value_1,int value_2,char* string_1);
static int settings_batch_update_from_cache(GstCameraSrc* camerasrc);
/*********************************QCAM functions***********************************/
static gboolean
capture_snapshot(GstCameraSrc *camerasrc);
#ifdef CAMERA_INTF
static void
preview_frame_cb(mm_camera_super_buf_t *bufs,void *user_data);
static void
record_frame_cb(mm_camera_super_buf_t *bufs,void *user_data);
#endif
/**********************Snapshot Stream***********************/
static void
snapshot_done_signal(void* user_data);

/******************************Suppliment functions*********************************/
static gboolean
gst_qcamerasrc_get_caps_info (GstCameraSrc * camerasrc, GstCaps * caps);
static void
gst_qcamerasrc_error_handler(GstCameraSrc *camerasrc, int ret);
static gboolean
gst_qcamerasrc_create(GstCameraSrc *camerasrc);
static gboolean
gst_qcamerasrc_destroy(GstCameraSrc *camerasrc);
static gboolean
gst_qcamerasrc_fill_ctrl_list(GstCameraSrc *camerasrc);
static gboolean
gst_camerasrc_empty_ctrl_list(GstCameraSrc *camerasrc);
static gboolean
gst_qcamerasrc_start(GstCameraSrc *camerasrc);
static gboolean
gst_qcamerasrc_stop(GstCameraSrc *camerasrc);
static GstFlowReturn
gst_qcamerasrc_read_frame(GstCameraSrc *camerasrc,GstBuffer **buffer);

/*********************************GST functions***********************************/

static void
gst_qcamerasrc_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec);
static void
gst_qcamerasrc_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec);
static void
gst_qcamerasrc_finalize (GObject * object);
static GstStateChangeReturn
gst_qcamerasrc_change_state (GstElement *element, GstStateChange transition);
static gboolean
gst_qcamerasrc_src_start (GstBaseSrc * src);
static gboolean
gst_qcamerasrc_src_stop (GstBaseSrc * src);
static GstCaps *
gst_qcamerasrc_get_caps (GstBaseSrc * src, GstCaps * caps);
static gboolean
gst_qcamerasrc_set_caps (GstBaseSrc * src, GstCaps * caps);
static GstFlowReturn
gst_qcamerasrc_src_create (GstPushSrc * src, GstBuffer ** buffer);

/*********************************BUFFER functions***********************************/
static void gst_qcamerasrc_buffer_class_init(gpointer g_class, gpointer class_data);
static GstQCameraBuffer* gst_qcamerasrc_buffer_new(GstCameraSrc *camerasrc);
static void gst_qcamerasrc_buffer_finalize(GstQCameraBuffer *buffer);
static void gst_qcamerasrc_wait_all_preview_buffer_finalized(GstCameraSrc *camerasrc);
static void gst_qcamerasrc_wait_all_video_buffer_finalized(GstCameraSrc *camerasrc);
static void gst_qcamerasrc_release_all_queued_preview_buffer(GstCameraSrc *camerasrc);


/******************************************************************************
 * Function Implementations
 *******************************************************************************/

/*********************************HDR functions**************************************/
static gboolean gst_camerasrc_run_hdr_processing(GstCameraSrc *camerasrc, qcamerasrc_buffer_t *input_buffer, HDRInputPictureFormat format, unsigned char **result_data);
static long _gst_camerasrc_hdr_progressing_cb(long progress, long status, void *user_data);

GST_IMPLEMENT_CAMERASRC_COLOR_BALANCE_METHODS( GstCameraSrc, gst_qcamera_src );
GST_IMPLEMENT_CAMERASRC_CONTROL_METHODS( GstCameraSrc, gst_qcamera_src );

G_DEFINE_TYPE_WITH_CODE(GstCameraSrc, gst_qcamerasrc, GST_TYPE_PUSH_SRC,
                        G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, gst_camerasrc_uri_handler_init)
                        G_IMPLEMENT_INTERFACE(GST_TYPE_CAMERA_CONTROL, gst_qcamera_src_control_interface_init)
                        G_IMPLEMENT_INTERFACE(GST_TYPE_COLOR_BALANCE, gst_qcamera_src_color_balance_interface_init));

/*********************************Util functions***********************************/
static  void
GenerateYUV420BlackFrame (unsigned char *buf, int buf_size, int width, int height)
{
    int i;
    int y_len = 0;
    int yuv_len = 0;

    y_len = width * height;
    yuv_len = (width * height * 3) >> 1;

    if (buf_size < yuv_len) {
        return;
    }

    if (width%4){
        for (i = 0; i < y_len; i++) {
            buf[i] = 0x10;
        }

        for (; i < yuv_len ; i++) {
            buf[i] = 0x80;
        }
    }else{
        //faster way
        int *ibuf = NULL;
        short *sbuf = NULL;
        ibuf = (int*)buf;

        for (i = 0; i < y_len/4; i++){
            ibuf[i] = 0x10101010;       //set YYYY
        }
        sbuf = (short*)(&buf[y_len]);

        for (i = 0 ; i < (yuv_len - y_len) / 2 ; i++){
            sbuf[i] = 0x8080;       //set UV
        }
    }

    return;
}

unsigned long gst_get_current_time(void)
{
    struct timeval lc_time;
    gettimeofday (&lc_time, NULL);
    return ((unsigned long)(lc_time.tv_sec * 1000L) + (unsigned long)(lc_time.tv_usec / 1000L));
}

static gboolean
gst_qcamerasrc_get_timeinfo(GstCameraSrc * camerasrc, GstBuffer* buffer)
{
    GstClock *clock = NULL;
    GstClockTime timestamp = GST_CLOCK_TIME_NONE;
    GstClockTime duration = GST_CLOCK_TIME_NONE;
    int fps_nu = 0, fps_de = 0;

    if (!camerasrc || !buffer){
        GST_WARNING_OBJECT (camerasrc, "[hadle:%p, buffer:%p]", camerasrc, buffer);
        return FALSE;
    }

    if ((clock = GST_ELEMENT_CLOCK (camerasrc))){
        timestamp = GST_ELEMENT (camerasrc)->base_time;
        gst_object_ref (clock);

        timestamp = gst_clock_get_time (clock) - timestamp;
        gst_object_unref (clock);

        if (camerasrc->fps_auto){
            duration = GST_CLOCK_TIME_NONE;
        }else{
#ifdef CONFIG_MACH_Z3LTE
            if (camerasrc->high_speed_fps == 0){
                if (camerasrc->fps == 0) {
                    fps_nu   = 0;
                    fps_de = 0;
                }else {
                    fps_nu   = 1;
                    fps_de = camerasrc->fps;
                }
            }else
#endif
            {
                fps_nu = 1;
                fps_de = camerasrc->fps;
            }

            if (fps_nu > 0 && fps_de > 0) {
                GstClockTime latency;

                latency = gst_util_uint64_scale_int (GST_SECOND, fps_nu, fps_de);
                duration = latency;
            }
        }
    }else{
        timestamp = GST_CLOCK_TIME_NONE;
    }

    GST_BUFFER_TIMESTAMP(buffer) = timestamp;
    GST_BUFFER_DURATION(buffer) = duration;

    /*
    GST_LOG_OBJECT(camerasrc, "[%"GST_TIME_FORMAT " dur %" GST_TIME_FORMAT "]",
                              GST_TIME_ARGS(GST_BUFFER_TIMESTAMP (buffer)),
                              GST_TIME_ARGS(GST_BUFFER_DURATION (buffer)));
    */

    return TRUE;
}

static gboolean
gst_qcamerasrc_get_realtimeinfo(GstCameraSrc * camerasrc , GstClockTime *time, GstClockTime *dur)
{
    GstClock *clock = NULL;
    GstClockTime timestamp = GST_CLOCK_TIME_NONE;
    GstClockTime duration = GST_CLOCK_TIME_NONE;
    int fps_nu = 0, fps_de = 0;

    if (!camerasrc){
        GST_WARNING_OBJECT (camerasrc, "[hadle:%p]", camerasrc);
        return FALSE;
    }

    if ((clock = GST_ELEMENT_CLOCK (camerasrc))){
        timestamp = GST_ELEMENT (camerasrc)->base_time;
        gst_object_ref (clock);

        timestamp = gst_clock_get_time (clock) - timestamp;
        gst_object_unref (clock);

        if (camerasrc->fps_auto){
            duration = GST_CLOCK_TIME_NONE;
        }else{
#ifdef CONFIG_MACH_Z3LTE
            if (camerasrc->high_speed_fps == 0){
                if (camerasrc->fps == 0) {
                    fps_nu   = 0;
                    fps_de = 0;
                }else {
                    fps_nu   = 1;
                    fps_de = camerasrc->fps;
                }
            }else
#endif
            {
                fps_nu = 1;
                fps_de = camerasrc->fps;
            }

            if (fps_nu > 0 && fps_de > 0) {
                GstClockTime latency;

                latency = gst_util_uint64_scale_int (GST_SECOND, fps_nu, fps_de);
                duration = latency;
            }
        }
    }else{
        timestamp = GST_CLOCK_TIME_NONE;
    }

	*time = timestamp;
	*dur = duration;

//    GST_BUFFER_TIMESTAMP(buffer) = timestamp;
//    GST_BUFFER_DURATION(buffer) = duration;

    /*
    GST_LOG_OBJECT(camerasrc, "[%"GST_TIME_FORMAT " dur %" GST_TIME_FORMAT "]",
                              GST_TIME_ARGS(GST_BUFFER_TIMESTAMP (buffer)),
                              GST_TIME_ARGS(GST_BUFFER_DURATION (buffer)));
    */

    return TRUE;
}


static gboolean
gst_qcamerasrc_is_zero_copy_format(const gchar *format_str){
    gboolean result=FALSE;
    if(strcmp(format_str, "SN12") == 0){
        result=TRUE;
    }
    else{
        result=FALSE;
    }
    return result;
}


/*********************************QCAM functions***********************************/
static gboolean
capture_snapshot(GstCameraSrc *camerasrc)
{
    GST_WARNING_OBJECT(camerasrc, "ENTERED");
    camerasrc->snapshot_stream->capture_num = camerasrc->cap_count;
    camerasrc->snapshot_stream->capture_interval = camerasrc->cap_interval;
    camerasrc->snapshot_stream->capture_fourcc = camerasrc->cap_fourcc;
#ifdef USE_NEXT_CAPTURE_TIME
    camerasrc->snapshot_stream->next_captured_time = 0;
#endif /* USE_NEXT_CAPTURE_TIME */
    camerasrc->is_focusing = TRUE;
#ifdef DEFAULT_ZSL_MODE_ON
    int cap_width= 0;
    int cap_height= 0;

    if ((camerasrc->snapshot_stream->snapshot_width != camerasrc->cap_width)||(camerasrc->snapshot_stream->snapshot_height != camerasrc->cap_height)) {
        GST_WARNING("camerasrc's cap_width(%d) cap_height(%d)",camerasrc->cap_width, camerasrc->cap_height);
        get_camera_control((camerasrc_handle_t *)camerasrc->snapshot_stream->qcam_handle,
                                    QCAMERASRC_PARAM_CONTROL_SNAPSHOT_SIZE,
                                    &cap_width,
                                    &cap_height,
                                    0);

        GST_WARNING("snapshot_stream w/h value is updated[%dX%d] to [%dx%d].",
            camerasrc->snapshot_stream->snapshot_width,
            camerasrc->snapshot_stream->snapshot_height, cap_width, cap_height);

        camerasrc->snapshot_stream->snapshot_width = cap_width;
        camerasrc->snapshot_stream->snapshot_height = cap_height;
    }

    if(!start_snapshot(camerasrc->snapshot_stream)) {
        GST_ERROR_OBJECT( camerasrc, "start_snapshot_stream fails");
        gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_INTERNAL);
        return FALSE;
    }
    camerasrc->is_capturing = TRUE;
#else
    if (VIDEO_IN_MODE_UNKNOWN != camerasrc->mode) {
        camerasrc->mode = VIDEO_IN_MODE_CAPTURE;

        /* release remained buffer in queue */
        gst_qcamerasrc_release_all_queued_preview_buffer(camerasrc);

        /* wait all preview buffer finalized for previiew restart */
        gst_qcamerasrc_wait_all_preview_buffer_finalized(camerasrc);
#ifdef CONFIG_MACH_Z3LTE
        GST_WARNING_OBJECT(camerasrc, "snapshot resolution [%dx%d], hdr mode %d, signal_still_capture %d",
                                            camerasrc->snapshot_stream->snapshot_width,
                                            camerasrc->snapshot_stream->snapshot_height,
                                            camerasrc->snapshot_stream->m_hdr_mode,
                                            camerasrc->signal_still_capture);

        GST_WARNING_OBJECT(camerasrc, "fourcc : %c%c%c%c, count %d, interval %d",
                                            camerasrc->cap_fourcc, camerasrc->cap_fourcc>>8,
                                            camerasrc->cap_fourcc>>16, camerasrc->cap_fourcc>>24,
                                            camerasrc->cap_count, camerasrc->cap_interval);
#else
        GST_WARNING_OBJECT(camerasrc, "snapshot resolution [%dx%d], hdr mode %d",
                                            camerasrc->snapshot_stream->snapshot_width,
                                            camerasrc->snapshot_stream->snapshot_height,
                                            camerasrc->snapshot_stream->m_hdr_mode);

        GST_WARNING_OBJECT(camerasrc, "fourcc : %c%c%c%c",
                                            camerasrc->cap_fourcc, camerasrc->cap_fourcc>>8,
                                            camerasrc->cap_fourcc>>16, camerasrc->cap_fourcc>>24);
#endif

        /* set snapshot resolution */
        if(camerasrc->cap_width != camerasrc->snapshot_stream->snapshot_width ||  camerasrc->cap_height != camerasrc->snapshot_stream->snapshot_height){
            set_snapshot_dimension(camerasrc->snapshot_stream, camerasrc->cap_width, camerasrc->cap_height);
        }

        if(!start_snapshot(camerasrc->snapshot_stream)) {
            GST_ERROR_OBJECT( camerasrc, "start_snapshot_stream fails");
            return FALSE;
        }
    }
#endif
    GST_WARNING_OBJECT (camerasrc, "LEAVED");

    return TRUE;
}

static void snapshot_done_signal(void* user_data)
{
    qcamera_snapshot_hw_intf *snapshot_stream = (qcamera_snapshot_hw_intf*)user_data;
    GstCameraSrc *camerasrc = (GstCameraSrc*)snapshot_stream->camerasrc;
    int i = 0;
    gboolean ret = CAMERASRC_SUCCESS;
    unsigned char *hdr_out_data = NULL;
    int hdr_jpeg_buffer_size, hdr_ref_img_jpeg_buffer_size;
    unsigned char *hdr_jpeg_buffer = NULL;
    unsigned char *hdr_ref_img_jpeg_buffer = NULL;
    char *priv_env_value = NULL;
    GstCaps *hdr_jpeg_buffer_caps = NULL;
    unsigned char *hdr_raw_buffer = NULL;
    GstSample *hdr_jpeg_buffer_sample = NULL;
    camera_memory_t *screennail_data = NULL;
    int screen_nail_width=0;
    int screen_nail_height=0;
#ifdef USE_NEXT_CAPTURE_TIME
    unsigned long current_time = 0;
#endif /* USE_NEXT_CAPTURE_TIME */

    GST_WARNING_OBJECT (camerasrc, "ENTERED");
#ifndef DEFAULT_ZSL_MODE_ON
    g_mutex_lock(camerasrc->restart_camera_lock);
    GST_WARNING_OBJECT(camerasrc, "restart camera signal");
    g_cond_signal(camerasrc->restart_camera_cond);
    g_mutex_unlock(camerasrc->restart_camera_lock);
#endif
    screennail_data=get_screen_nail_frame(camerasrc->preview_stream);
    GstCaps *buf_caps_main = NULL;
    GstSample *buf_sample_main = NULL;
    GstBuffer *main_buf_cap_signal = NULL;
    GstBuffer *ssnail_buf_cap_signal = NULL;
    GstCaps *ssnail_buf_caps = NULL;
    GstSample *ssnail_buf_sample = NULL;
    GstBuffer *hdr_ref_img_buf_cap_signal = NULL;

    if(camerasrc->signal_still_capture) {
        unsigned char *temp_buffer = NULL;
        GST_DEBUG_OBJECT( camerasrc, "Do not emit the signal, copy capture data for signal_still_capture");
        if (camerasrc->snapshot_buffer) {
            GST_WARNING_OBJECT(camerasrc, "snapshot_buffer is not NULL (%p)", camerasrc->snapshot_buffer);
            free(camerasrc->snapshot_buffer);
            camerasrc->snapshot_buffer = NULL;
        }
        // malloc snapshot buffer for dualcamera
        temp_buffer = (unsigned char*)malloc(snapshot_stream->capture_data_size);
        if (temp_buffer == NULL) {
            GST_ERROR_OBJECT(camerasrc,"snapshot_buffer alloc failed");
            camerasrc->signal_still_capture = FALSE;
            gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_ALLOCATION);
            return;
        }
        // copy the buffer
        memcpy (temp_buffer, snapshot_stream->capture_data_buffer, snapshot_stream->capture_data_size);

        camerasrc->snapshot_buffer_size = snapshot_stream->capture_data_size;
        camerasrc->snapshot_buffer = temp_buffer;
        temp_buffer = NULL;

        /* send message for noti to complete to get frames */
        gst_qcamerasrc_post_message_int(camerasrc, "camerasrc-Capture", "capture-done", TRUE);
    } else{
        if(snapshot_stream->m_hdr_mode){
            gboolean ret = CAMERASRC_SUCCESS;
            unsigned int thumb_width = 0;
            unsigned int thumb_height = 0;
            unsigned int thumb_length = 0;
            float ratio =0;
            /* send message for noti to complete to get frames */
        //    gst_qcamerasrc_post_message_int(camerasrc, "camerasrc-Capture", "capture-done", TRUE);

            ret = gst_camerasrc_run_hdr_processing(camerasrc, snapshot_stream->m_bracket_buffer, HDR_FORMAT_NV12, &hdr_out_data);
            if (ret == FALSE) {
                gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_INTERNAL);
                return;
            } else { // success
                GST_WARNING_OBJECT( camerasrc, "HDR process success");
                #if _ENABLE_CAMERASRC_DEBUG
                data_dump("/opt/usr/media/snapshot_hdr_result.raw",hdr_out_data,YUV422_SIZE(snapshot_stream->snapshot_width,snapshot_stream->snapshot_height));
                #endif
            }

            hdr_jpeg_buffer_size = (snapshot_stream->snapshot_width*snapshot_stream->snapshot_height*3)/2;
            ret = mm_util_jpeg_encode_to_memory_hw(
                              &hdr_jpeg_buffer,
                              &hdr_jpeg_buffer_size,
                              hdr_out_data,
                              snapshot_stream->snapshot_width,
                              snapshot_stream->snapshot_height,
                              MM_UTIL_JPEG_FMT_NV12,
                              DEFAULT_JPEG_ENCODE_QUALITY);


            if (NULL != ret)
                GST_WARNING_OBJECT( camerasrc, "HDR JPEG encode failed");

            if (camerasrc->cap_hdr == CAMERASRC_HDR_CAPTURE_ON_AND_ORIGINAL) {
                ret = mm_util_jpeg_encode_to_memory_hw(
                            &hdr_ref_img_jpeg_buffer,
                            &hdr_ref_img_jpeg_buffer_size,
                            snapshot_stream->m_bracket_buffer[1].planes[0].start,
                            camerasrc->cap_width,
                            camerasrc->cap_height,
                            MM_UTIL_JPEG_FMT_NV12,
                            DEFAULT_JPEG_ENCODE_QUALITY);

                if (NULL != ret)
                    GST_WARNING_OBJECT( camerasrc, "ORIGINAL JPEG for HDR encode failed");
            }
            ratio=(float)snapshot_stream->snapshot_width/snapshot_stream->snapshot_height;
            GST_WARNING(" ratio = %f",ratio);

            if(ratio == ASPECT_RATIO_1_1) {
                thumb_width = 240;
                thumb_height = 240;
            } else if(ratio > ASPECT_RATIO_1_1 && ratio < ASPECT_RATIO_4_3) {
                thumb_width = 512;
                thumb_height = 384;
            } else if(ratio >= ASPECT_RATIO_4_3 && ratio < ASPECT_RATIO_16_9) {
                thumb_width = 320;
                thumb_height = 192;
            } else if(ratio >= ASPECT_RATIO_16_9) {
                thumb_width = 512;
                thumb_height = 288;
            }
            GST_WARNING(" screen nail size (%d x %d)",thumb_width,thumb_height);

            thumb_length = thumb_width * thumb_height*3/2;
            hdr_raw_buffer = (unsigned char *)malloc(thumb_length);

            ret = mm_util_resize_image(hdr_out_data, snapshot_stream->snapshot_width, snapshot_stream->snapshot_height, 3,
                              hdr_raw_buffer, &thumb_width, &thumb_height);
            if (NULL != ret) {
                GST_WARNING_OBJECT( camerasrc, "HDR raw resize failed");
            } else {
                GST_WARNING_OBJECT( camerasrc, "HDR raw resize success");
            }
            ssnail_buf_cap_signal = gst_buffer_new_wrapped_full(0, hdr_raw_buffer, thumb_length, 0, thumb_length, NULL, NULL);
            if (ssnail_buf_cap_signal != NULL) {
                ssnail_buf_caps = gst_caps_new_simple("video/x-raw",
                                                                     "format", G_TYPE_STRING, "NV12",
                                                                     "width", G_TYPE_INT, thumb_width,
                                                                     "height", G_TYPE_INT, thumb_height,
                                                                     NULL);
                ssnail_buf_sample = gst_sample_new(ssnail_buf_cap_signal, ssnail_buf_caps, NULL, NULL);
            }
            camerasrc->cap_provide_exif = FALSE;
        }

        if(snapshot_stream->m_hdr_mode){
                main_buf_cap_signal = gst_buffer_new_wrapped_full(0,
                                                    hdr_jpeg_buffer,
                                                    hdr_jpeg_buffer_size,
                                                    0,
                                                    hdr_jpeg_buffer_size,
                                                    NULL,
                                                    NULL);

                if (camerasrc->cap_hdr == CAMERASRC_HDR_CAPTURE_ON_AND_ORIGINAL) {
                    hdr_ref_img_buf_cap_signal = gst_buffer_new_wrapped_full(0,
                                                    hdr_ref_img_jpeg_buffer,
                                                    hdr_ref_img_jpeg_buffer_size,
                                                    0,
                                                    hdr_ref_img_jpeg_buffer_size,
                                                    NULL,
                                                    NULL);

                    if (NULL == hdr_ref_img_buf_cap_signal) {
                        GST_ERROR_OBJECT(camerasrc, "gst buffer alloc error : hdr_ref_img_buf_cap_signal");
                    }

            }
        }else {
            main_buf_cap_signal = gst_buffer_new_wrapped_full(0,
                                        snapshot_stream->capture_data_buffer,
                                        snapshot_stream->capture_data_size,
                                        0,
                                        snapshot_stream->capture_data_size,
                                        NULL,
                                        NULL);
            if(screennail_data){
                if(screennail_data->data){
                    ssnail_buf_cap_signal =  gst_buffer_new_wrapped_full(0,
                        screennail_data->data,
                        screennail_data->size,
                        0,
                        screennail_data->size,
                        NULL,
                        NULL);
                    if(screennail_data->size==((snapshot_stream->snapshot_width*snapshot_stream->snapshot_height*3)/2)){
                        screen_nail_width=snapshot_stream->snapshot_width;
                        screen_nail_height=snapshot_stream->snapshot_height;
                    }else{
                        screen_nail_width=camerasrc->preview_stream->preview_width;
                        screen_nail_height=camerasrc->preview_stream->preview_height;
                    }
                }
            }else{
                GST_ERROR_OBJECT(camerasrc,"No Screen Nail data");
            }
        }


        if (snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('J', 'P', 'E', 'G')) {
            if (NULL != main_buf_cap_signal) {
                 buf_caps_main = gst_caps_new_simple("image/jpeg",
                                     "width", G_TYPE_INT, snapshot_stream->snapshot_width,
                                     "height", G_TYPE_INT, snapshot_stream->snapshot_height,NULL);
                 buf_sample_main = gst_sample_new(main_buf_cap_signal, buf_caps_main, NULL, NULL);
            }
            else {
                GST_ERROR_OBJECT(camerasrc, "main_buf_cap_signal is NULL");
            }

            if (camerasrc->cap_hdr == CAMERASRC_HDR_CAPTURE_ON_AND_ORIGINAL) {
                  if (NULL != hdr_ref_img_buf_cap_signal) {
                  hdr_jpeg_buffer_caps = gst_caps_new_simple("image/jpeg",
                                            "width", G_TYPE_INT, snapshot_stream->snapshot_width,
                                            "height", G_TYPE_INT,snapshot_stream->snapshot_height,
                                            NULL);
                  hdr_jpeg_buffer_sample = gst_sample_new(hdr_ref_img_buf_cap_signal, hdr_jpeg_buffer_caps, NULL, NULL);
            }
            else {
                  GST_ERROR_OBJECT(camerasrc, "gst buffer alloc error : hdr_ref_img_buf_cap_signal so not setting caps");
            }
            }
        } else if (snapshot_stream->capture_fourcc==GST_MAKE_FOURCC ('N', 'V', '1', '2') ||
                    snapshot_stream->capture_fourcc==GST_MAKE_FOURCC ('N', 'V', '2', '1') ||
                   snapshot_stream->capture_fourcc==GST_MAKE_FOURCC ('S', 'N', '1', '2')) {
            if (NULL != main_buf_cap_signal) {
                buf_caps_main = gst_caps_new_simple("video/x-raw",
                                                    "format", G_TYPE_STRING, "NV12",
                                                    "width", G_TYPE_INT, snapshot_stream->snapshot_width,
                                                    "height", G_TYPE_INT, snapshot_stream->snapshot_height,NULL);
                buf_sample_main = gst_sample_new(main_buf_cap_signal, buf_caps_main, NULL, NULL);
            }
            else {
                GST_ERROR_OBJECT(camerasrc, "main_buf_cap_signal is NULL in raw callback case");
            }
        }

        if (!snapshot_stream->m_hdr_mode && (ssnail_buf_cap_signal != NULL)) {
            ssnail_buf_caps = gst_caps_new_simple("video/x-raw",
                                                   "format", G_TYPE_STRING, "NV12",
                                                   "width", G_TYPE_INT, screen_nail_width,
                                                   "height", G_TYPE_INT, screen_nail_height,
                                                   NULL);
            ssnail_buf_sample = gst_sample_new(ssnail_buf_cap_signal, ssnail_buf_caps, NULL, NULL);
        }

        if (camerasrc->cap_hdr == CAMERASRC_HDR_CAPTURE_ON_AND_ORIGINAL) {
            g_signal_emit( G_OBJECT (camerasrc),
                            gst_qcamerasrc_signals[SIGNAL_STILL_CAPTURE],
                            0,
                            hdr_jpeg_buffer_sample,
                            NULL,//TODO:Is there a way to get thumbnail buffer separately with mm_camera_interface2
                            (snapshot_stream->m_hdr_mode? NULL: ssnail_buf_sample));//TODO:Save last preview before capture and add it here??
        }

#ifdef USE_NEXT_CAPTURE_TIME
        /* check capture interval */
        if (snapshot_stream->capture_num > 1) {
            current_time = gst_get_current_time();
            if (snapshot_stream->next_captured_time != 0) {
                if (snapshot_stream->next_captured_time > current_time) {
                    GST_WARNING_OBJECT(camerasrc, "sleep time %u ms", snapshot_stream->next_captured_time - current_time);
                    usleep((snapshot_stream->next_captured_time - current_time)*1000);
                } else {
                    GST_WARNING_OBJECT(camerasrc, "no need to sleep");
                }
                snapshot_stream->next_captured_time += snapshot_stream->capture_interval;
            } else {
                snapshot_stream->next_captured_time = current_time + snapshot_stream->capture_interval;
            }
        }
#endif /* USE_NEXT_CAPTURE_TIME */

        g_signal_emit( G_OBJECT (camerasrc),
                        gst_qcamerasrc_signals[SIGNAL_STILL_CAPTURE],
                        0,
                        buf_sample_main,
                        NULL,//TODO:Is there a way to get thumbnail buffer separately with mm_camera_interface2
                       ssnail_buf_sample);//TODO:Save last preview before capture and add it here??

        camerasrc->cap_provide_exif = TRUE;

        if(snapshot_stream->m_hdr_mode
            && snapshot_stream->m_bracket_buffer_count == HDR_BRACKET_FRAME_NUM){
            for(i=0;i<snapshot_stream->m_bracket_buffer_count;i++){
                GST_WARNING_OBJECT(camerasrc, "HDR: freeing bracket buffer");
                free(snapshot_stream->m_bracket_buffer[i].planes[0].start);
            }

            if (NULL != hdr_jpeg_buffer) {
                GST_WARNING_OBJECT(camerasrc, "HDR: freeing jpeg  bufferr");
                free(hdr_jpeg_buffer);
                hdr_jpeg_buffer = NULL;
            }

            if (NULL != hdr_raw_buffer) {
                GST_WARNING_OBJECT(camerasrc, "HDR: freeing raw bufferr");
                free(hdr_raw_buffer);
                hdr_raw_buffer = NULL;
            }

            if ((camerasrc->cap_hdr == CAMERASRC_HDR_CAPTURE_ON_AND_ORIGINAL) &&
                (NULL != hdr_ref_img_jpeg_buffer)) {
                GST_WARNING_OBJECT(camerasrc, "HDR ORIGINAL: freeing jpeg buffer");
                free(hdr_ref_img_jpeg_buffer);
                hdr_ref_img_jpeg_buffer = NULL;
            }

            snapshot_stream->m_bracket_buffer_count = 0;
        }

        if (hdr_out_data != NULL) {
            GST_WARNING_OBJECT(camerasrc, "HDR: freeing hdr out buffer");
            free(hdr_out_data);
            hdr_out_data = NULL;
        }
    }

    if(main_buf_cap_signal != NULL) {
                GST_WARNING_OBJECT(camerasrc,"free main_buf_cap_signal");
                gst_buffer_unref(main_buf_cap_signal);
    }

    if(ssnail_buf_cap_signal != NULL) {
                GST_WARNING_OBJECT(camerasrc,"free ssnail_buf_cap_signal");
                gst_buffer_unref(ssnail_buf_cap_signal);
    }

    if(hdr_ref_img_buf_cap_signal != NULL){
                GST_WARNING_OBJECT(camerasrc,"free hdr_ref_img_buf_cap_signal");
                gst_buffer_unref(hdr_ref_img_buf_cap_signal);
    }

    if(buf_caps_main != NULL) {
                GST_WARNING_OBJECT(camerasrc,"free buf_caps_main");
                gst_caps_unref(buf_caps_main);
    }

    if(ssnail_buf_caps != NULL) {
                GST_WARNING_OBJECT(camerasrc,"free ssnail_buf_caps");
                gst_caps_unref(ssnail_buf_caps);
    }

    if(hdr_jpeg_buffer_caps != NULL) {
                GST_WARNING_OBJECT(camerasrc,"free hdr_jpeg_buffer_caps");
                gst_caps_unref(hdr_jpeg_buffer_caps);
    }

    free_cached_screen_nail_frame(screennail_data);
#ifdef DEFAULT_ZSL_MODE_ON
    TIMER_MSG("Total Capture time  : %12.6lf s", capture_timer)
    TIMER_STOP(capture_timer)
#endif
    GST_WARNING_OBJECT (camerasrc, "LEAVED");
    return;
}
#if _ENABLE_CAMERASRC_DEBUG
static int
data_dump (char *filename, void *data, int size){
    FILE *fp = NULL;
    fp = fopen (filename, "a+");
    if (!fp)
        return FALSE;
    fwrite (data, 1, size, fp);
    fclose (fp);
    return TRUE;
}

static int
data_dump_record(char *filename,MMVideoBuffer *inbuf){
	FILE *fp = NULL;
	char *temp = (char *)inbuf->a[0];
	int i = 0;
	fp = fopen (filename, "a+");
	if (!fp)
		return FALSE;

	for (i = 0; i < inbuf->h[0]; i++) {
		fwrite(temp, inbuf->w[0], 1, fp);
		temp += inbuf->s[0];
	}
	inbuf->h[1] = inbuf->h[0]/2;
	inbuf->w[1] = inbuf->w[0];
	inbuf->s[1] = inbuf->s[0];
	temp = (char*)inbuf->a[0] + inbuf->s[0] * inbuf->e[0];

	for(i = 0; i < inbuf->h[1] ; i++) {
		fwrite(temp, inbuf->w[1], 1, fp);
		temp += inbuf->s[1];
	}
	fclose (fp);
	return TRUE;
}
#endif
#ifdef CAMERA_INTF
static void
record_frame_cb(mm_camera_super_buf_t *bufs,void *user_data){
    GstCameraSrc *camerasrc=(GstCameraSrc*)user_data;
    mm_camera_buf_def_t *frame = bufs->bufs[0];
#if ENABLE_ZERO_COPY
    MMVideoBuffer* img_buffer =NULL;
#endif
    GstQCameraBuffer *buffer = NULL;
    GstCaps *buffer_caps = NULL;
    GstSample *buffer_sample = NULL;
    GstClockTime timestamp = GST_CLOCK_TIME_NONE;
    GstClockTime duration = GST_CLOCK_TIME_NONE;

    GST_DEBUG_OBJECT(camerasrc, "Entered record_frame_cb");

    int size = 0;
    int video_width = 0;
    int video_height = 0;
    gst_qcamerasrc_get_realtimeinfo(camerasrc, &timestamp, &duration);

    GST_DEBUG_OBJECT(camerasrc, "timestamp = [%"GST_TIME_FORMAT"] , duration = [%"GST_TIME_FORMAT"]" , GST_TIME_ARGS(timestamp) , GST_TIME_ARGS(duration));

    g_mutex_lock(camerasrc->video_buffer_lock);

    if (camerasrc->mode == VIDEO_IN_MODE_UNKNOWN) {
        GST_WARNING_OBJECT(camerasrc, "UNKNOWN video mode - release buffer");
        done_record_frame(camerasrc->record_stream, frame);
        g_mutex_unlock(camerasrc->video_buffer_lock);
        return;
    } else if (camerasrc->mode != VIDEO_IN_MODE_VIDEO) {
        GST_WARNING_OBJECT(camerasrc, "STOP video mode - just return");
        g_mutex_unlock(camerasrc->video_buffer_lock);
        return;
    }

    buffer = gst_qcamerasrc_buffer_new(camerasrc);
    video_width = camerasrc->video_width;
    video_height = camerasrc->video_height;
    size = frame->frame_len;
    GST_DEBUG_OBJECT(camerasrc, "Recording cb frame (%dx%d) size(%d)", video_width, video_height, size);

#if ENABLE_ZERO_COPY
    if (gst_qcamerasrc_is_zero_copy_format(camerasrc->format_str)) {
        img_buffer = (MMVideoBuffer*) g_malloc(sizeof(MMVideoBuffer));
        if (img_buffer) {
            memset(img_buffer, 0x0, sizeof(MMVideoBuffer));

            img_buffer->type = MM_VIDEO_BUFFER_TYPE_TBM_BO;
            img_buffer->handle_num = 1;
            img_buffer->handle.bo[0] = frame->bo;
            img_buffer->stride_height[0] = ALIGN(video_height,32);
            img_buffer->stride_width[0] = ALIGN(video_width,128);
            img_buffer->stride_height[1] = ALIGN((video_height>>1),16);
            img_buffer->stride_width[1] = ALIGN(video_width,128);
            img_buffer->height[0] = video_height;
            img_buffer->width[0] = video_width;
            img_buffer->height[1] = video_height >> 1;
            img_buffer->width[1] = video_width >> 1;
            img_buffer->data[0] = frame->buffer;
			img_buffer->data[1] = frame->buffer + img_buffer->stride_height[0]*img_buffer->stride_width[0];
            buffer->buffer_metadata = (void *)frame;
            buffer->buffer_type = RECORD_BUFFER;
            }
	} else
#endif
    {
        GST_DEBUG_OBJECT(camerasrc, "Using camera buffer Non Zero-Copy format");
        GstMapInfo map;
        gst_buffer_map(buffer->buffer,&map,GST_MAP_WRITE);
        map.data = frame->buffer;
        map.size = size;
        gst_buffer_unmap(buffer->buffer,&map);
        buffer->buffer_type = RECORD_BUFFER;
    }

    /* increase video buffer count */
    GST_DEBUG_OBJECT(camerasrc,"get video buffer - live %d -> %d",
                               camerasrc->num_video_buffers, camerasrc->num_video_buffers + 1);
   if( camerasrc->num_video_buffers > 10) {
        GST_WARNING_OBJECT(camerasrc, "already %d buffer was sent to encoder",camerasrc->num_video_buffers);
   }
    camerasrc->num_video_buffers++;

    g_mutex_unlock(camerasrc->video_buffer_lock);

    /* get time info */
    //gst_qcamerasrc_get_timeinfo(camerasrc, (GstBuffer*)buffer);
    GST_BUFFER_TIMESTAMP(buffer->buffer) = timestamp;
    GST_BUFFER_DURATION(buffer->buffer) = duration;


    gst_buffer_append_memory(buffer->buffer,
                    gst_memory_new_wrapped(0,
                                            frame->buffer,
                                            size,
                                            0,
                                            size,
                                            buffer,
                                            (GDestroyNotify)gst_qcamerasrc_buffer_finalize));

    if (img_buffer) {
        GST_DEBUG_OBJECT(camerasrc, "appending:Using camera buffer Zero-Copy format");
        gst_buffer_append_memory(buffer->buffer,
                    gst_memory_new_wrapped(0,
                                            img_buffer,
                                            sizeof(*img_buffer),
                                            0,
                                            sizeof(*img_buffer),
                                            img_buffer,
                                            free)
                                            );
    }

    if (buffer->buffer) {
        int fps_de = 0;
        int fps_nu = 0;

        if (camerasrc->fps <= 0) {
            fps_nu = 0;
            fps_de = 1;
        } else {
            fps_nu = camerasrc->fps;
            fps_de = 1;
        }

        GST_INFO_OBJECT(camerasrc, "media record cb frame : format %s, size %dx%d, framerate %d/%d",
                                      camerasrc->format_str,
                                      video_width, video_height, fps_nu, fps_de);

        buffer_caps = gst_caps_new_simple("video/x-raw",
                                                      "format", G_TYPE_STRING, camerasrc->format_str,
                                                      "width", G_TYPE_INT, video_width,
                                                      "height", G_TYPE_INT, video_height,
                                                      "framerate", GST_TYPE_FRACTION, fps_nu, fps_de,
                                                      NULL);
       buffer_sample = gst_sample_new(buffer->buffer, buffer_caps, NULL, NULL);
    }
#if ENABLE_ZERO_COPY
    if(gst_qcamerasrc_is_zero_copy_format(camerasrc->format_str))
#endif
    {
        GST_DEBUG_OBJECT(camerasrc, "record buffer %p, img_buffer %p, fd %d",
                                buffer, img_buffer, img_buffer->handle.dmabuf_fd[0]);
    }

#if _ENABLE_CAMERASRC_DEBUG
    data_dump_record("/opt/usr/media/record_plugin.yuv",img_buffer);
#endif
    if(camerasrc->mediapacket_record_mode){
        g_signal_emit(G_OBJECT (camerasrc),
                  gst_qcamerasrc_signals[SIGNAL_MEDIAPACKET_RECORD_CB],
                  0,
                  buffer_sample);
        GST_DEBUG_OBJECT(camerasrc, "return mediapacket-record-cb");
    }

    if(camerasrc->local_recording && buffer){
        gboolean ret_sig = FALSE;

        g_signal_emit(G_OBJECT (camerasrc),
                      gst_qcamerasrc_signals[SIGNAL_VIDEO_STREAM_CB],
                      0,
                      buffer_sample,
                      &ret_sig);
        if (ret_sig == FALSE) {
            GST_WARNING_OBJECT(camerasrc, "need to unref buffer");
            gst_buffer_unref(buffer->buffer);
        }

        GST_DEBUG_OBJECT(camerasrc, "return video-stream-cb");
    }

    gst_caps_unref(buffer_caps);
    gst_sample_unref(buffer_sample);
    GST_DEBUG_OBJECT (camerasrc, "LEAVED");

    return;
}
#endif

#ifdef CAMERA_INTF
static void
preview_frame_cb(mm_camera_super_buf_t *bufs,void *user_data)
{
    GstCameraSrc *camerasrc=(GstCameraSrc*)user_data;
    mm_camera_buf_def_t *frame = bufs->bufs[0];
#if ENABLE_ZERO_COPY
    MMVideoBuffer* img_buffer =NULL;
#endif
    GstQCameraBuffer *buffer = NULL;
    GstClockTime timestamp = GST_CLOCK_TIME_NONE;
    GstClockTime duration = GST_CLOCK_TIME_NONE;

    GST_DEBUG_OBJECT(camerasrc, "Entered preview_frame_cb");

    gst_qcamerasrc_get_realtimeinfo(camerasrc, &timestamp, &duration);

    GST_DEBUG_OBJECT(camerasrc, "timestamp = [%"GST_TIME_FORMAT"] , duration = [%"GST_TIME_FORMAT"]" , GST_TIME_ARGS(timestamp) , GST_TIME_ARGS(duration));

    get_preview_stream_mutex (camerasrc->preview_stream);

    if (camerasrc->mode == VIDEO_IN_MODE_UNKNOWN || camerasrc->mode == VIDEO_IN_MODE_CAPTURE) {
        GST_WARNING_OBJECT(camerasrc, "UNKNOWN/CAPTURE mode - release buffer");

        release_preview_stream_mutex (camerasrc->preview_stream);
        done_preview_frame(camerasrc->preview_stream, frame);
        return;
    } else if (camerasrc->mode == VIDEO_IN_MODE_STOP) {
        GST_WARNING_OBJECT(camerasrc, "STOP preview mode - just return");
        release_preview_stream_mutex (camerasrc->preview_stream);
        return;
    }

    g_mutex_lock(camerasrc->preview_buffer_lock);
    if(camerasrc->num_live_buffers > 5) {
        GST_WARNING_OBJECT(camerasrc, "Number of live buffers more than 6 qbuf without display %d",camerasrc->num_live_buffers);
        camerasrc->qbuf_without_display++;
        if(camerasrc->qbuf_without_display > 50){
            GST_ERROR_OBJECT(camerasrc, "50 times continuous qbuf without display return error %d",camerasrc->qbuf_without_display);
            release_preview_stream_mutex(camerasrc->preview_stream);
            done_preview_frame(camerasrc->preview_stream, frame);
            g_mutex_unlock(camerasrc->preview_buffer_lock);
            return;
        }
        release_preview_stream_mutex(camerasrc->preview_stream);
        done_preview_frame(camerasrc->preview_stream, frame);
        g_mutex_unlock(camerasrc->preview_buffer_lock);
        return;
    }
    camerasrc->qbuf_without_display = 0;
    g_mutex_unlock(camerasrc->preview_buffer_lock);

    buffer = gst_qcamerasrc_buffer_new(camerasrc);

    int size = ((camerasrc->preview_stream->preview_width * camerasrc->preview_stream->preview_height * 3) >> 1);
#if ENABLE_ZERO_COPY
    if (gst_qcamerasrc_is_zero_copy_format(camerasrc->format_str)) {
        GST_DEBUG_OBJECT(camerasrc, "Using camera buffer Zero-Copy format");
        img_buffer = (MMVideoBuffer*) g_malloc(sizeof(MMVideoBuffer));
        if (img_buffer) {
            memset(img_buffer, 0x0, sizeof(MMVideoBuffer));
            img_buffer->type = MM_VIDEO_BUFFER_TYPE_TBM_BO;
            img_buffer->handle_num = 1;
            img_buffer->handle.bo[0] = frame->bo;
            img_buffer->stride_height[0] = camerasrc->preview_stream->preview_height;
            img_buffer->stride_width[0] = camerasrc->preview_stream->preview_width;
            img_buffer->stride_height[1] = (camerasrc->preview_stream->preview_height) >> 1;
            img_buffer->stride_width[1] = camerasrc->preview_stream->preview_width;
            img_buffer->plane_num = 2;
            img_buffer->height[0] = camerasrc->preview_stream->preview_height;
            img_buffer->width[0] = camerasrc->preview_stream->preview_width;
            img_buffer->height[1] = camerasrc->preview_stream->preview_height >> 1;
            img_buffer->width[1] = camerasrc->preview_stream->preview_width;
            img_buffer->data[0] = frame->buffer;
            img_buffer->data[1] = img_buffer->data[0] + (camerasrc->preview_stream->preview_height * camerasrc->preview_stream->preview_width);
            img_buffer->size[0] = img_buffer->height[0] * img_buffer->width[0];
            img_buffer->size[1] = img_buffer->height[1] * img_buffer->width[1];
            buffer->buffer_metadata = (void*)frame;
            buffer->buffer_type = PREVIEW_BUFFER;
        }else {
            GST_ERROR_OBJECT(camerasrc, "MMVideobuffer structure allocate fail");
        }
    } else
#endif
    {
        GST_DEBUG_OBJECT(camerasrc, "Using camera buffer Non Zero-Copy format");
        buffer->buffer_metadata = (void*)frame;
        buffer->buffer_type = PREVIEW_BUFFER;
    }

    GST_BUFFER_TIMESTAMP(buffer->buffer) = timestamp;
    GST_BUFFER_DURATION(buffer->buffer) = duration;

    /* increase preview buffer count */
    g_mutex_lock(camerasrc->preview_buffer_lock);
    GST_DEBUG_OBJECT(camerasrc,"get preview buffer - num_live_buffers %d -> %d",
                               camerasrc->num_live_buffers, camerasrc->num_live_buffers + 1);
    camerasrc->num_live_buffers++;
    g_mutex_unlock(camerasrc->preview_buffer_lock);

    if (camerasrc->firsttime) {
        camerasrc->firsttime = FALSE;
        GST_WARNING_OBJECT(camerasrc, "first preview frame is comming");
    }

    gst_buffer_append_memory(buffer->buffer,
                    gst_memory_new_wrapped(0,
                                            frame->buffer,
                                            size,
                                            0,
                                            size,
                                            buffer,
                                            (GDestroyNotify)gst_qcamerasrc_buffer_finalize));

    if (img_buffer) {
        GST_DEBUG_OBJECT(camerasrc, "appending:Using camera buffer Zero-Copy format");
        gst_buffer_append_memory(buffer->buffer,
                    gst_memory_new_wrapped(0,
                                            img_buffer,
                                            sizeof(*img_buffer),
                                            0,
                                            sizeof(*img_buffer),
                                            img_buffer,
                                            free)
                                            );
    }

    buffer = (GstQCameraBuffer*)buffer->buffer;
    GST_DEBUG_OBJECT(camerasrc, "gst_buffer_append_memory done %p",buffer);

    cache_preview_frame(camerasrc->preview_stream,(GstBuffer*)buffer);

    release_preview_stream_mutex(camerasrc->preview_stream);

    GST_DEBUG_OBJECT(camerasrc,"leave");

    return;
}
#endif
/******************************Suppliment functions*********************************/

/* VOID:OBJECT,OBJECT (generated by 'glib-genmarshal') */
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
void
gst_qcamerasrc_VOID__OBJECT_OBJECT (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_OBJECT) (gpointer     data1,
                                                    gpointer     arg_1,
                                                    gpointer     arg_2,
                                                    gpointer     arg_3,
                                                    gpointer     data2);
  register GMarshalFunc_VOID__OBJECT_OBJECT callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 4);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_OBJECT) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_object (param_values + 1),
            g_marshal_value_peek_object (param_values + 2),
            g_marshal_value_peek_object (param_values + 3),
            data2);
}

void
gst_qcamerasrc_VOID__OBJECT_MEDIAPACKET_STREAM (GClosure     *closure,
                                         GValue       *return_value,
                                         guint         n_param_values,
                                         const GValue *param_values,
                                         gpointer      invocation_hint,
                                         gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__OBJECT_MEDIAPACKET_STREAM) (gpointer     data1,
                                                    gpointer     arg,
                                                    gpointer     data2);
  register GMarshalFunc_VOID__OBJECT_MEDIAPACKET_STREAM callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 2);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__OBJECT_MEDIAPACKET_STREAM) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_object (param_values + 1),
            data2);
}

static GType
gst_qcamerasrc_quality_get_type (void)
{
    static GType camerasrc_quality_type = 0;
    static const GEnumValue quality_types[] = {
        {GST_CAMERASRC_QUALITY_LOW, "Low quality", "low"},
        {GST_CAMERASRC_QUALITY_HIGH, "High quality", "high"},
        {0, NULL, NULL}
    };

    if (!camerasrc_quality_type) {
        camerasrc_quality_type = g_enum_register_static ("GstCameraSrcQuality", quality_types);
    }
    return camerasrc_quality_type;
}

static gboolean
gst_qcamerasrc_get_caps_info (GstCameraSrc * camerasrc, GstCaps * caps)
{
    GstStructure *structure;
    const GValue *framerate;
    const gchar *format_gst = NULL;
    gint fps_n = 0, fps_d = 0;
    gint w = 0, h = 0, rot = 0;
    const gchar *mimetype;

    GST_WARNING_OBJECT(camerasrc, "ENTERED");

    GST_WARNING_OBJECT(camerasrc, "Collect data for given caps.(caps:%x)", (unsigned int)caps);

    structure = gst_caps_get_structure (caps, 0);

    if (!gst_structure_get_int (structure, "width", &w)){
        GST_ERROR_OBJECT(camerasrc, "Failed to get width info in caps");
        goto _caps_info_failed;
    }else{
        GST_WARNING_OBJECT(camerasrc, "width caps proved [%d]",w);
    }

    if (!gst_structure_get_int (structure, "height", &h)){
        GST_ERROR_OBJECT(camerasrc, "Failed to get height info in caps");
        goto _caps_info_failed;
    }else{
        GST_WARNING_OBJECT(camerasrc, "width caps proved [%d]",h);
    }

    if (!gst_structure_get_int (structure, "rotate", &rot)){
        GST_WARNING_OBJECT(camerasrc, "Failed to get rotate info in caps. set default 0.");
        camerasrc->use_rotate_caps = FALSE;
    } else {
        GST_WARNING_OBJECT(camerasrc, "Succeed to get rotate[%d] info in caps", rot);
        camerasrc->use_rotate_caps = TRUE;
    }

    camerasrc->rotate = rot;

    camerasrc->preview_stream->preview_width = camerasrc->width = w;
    camerasrc->preview_stream->preview_height = camerasrc->height = h;

    framerate = gst_structure_get_value (structure, "framerate");
    if (!framerate){
        GST_ERROR_OBJECT(camerasrc, "Failed to get framerate info in caps");
        goto _caps_info_failed;
    }

    fps_n = gst_value_get_fraction_numerator (framerate);
    fps_d = gst_value_get_fraction_denominator (framerate);
    camerasrc->fps = (int)((float)fps_n / (float)fps_d);
    camerasrc->preview_stream->fps = camerasrc->fps;
    camerasrc->preview_stream->fps_auto = camerasrc->fps_auto;
    GST_INFO_OBJECT(camerasrc, "auto (%d), fps (%d) = fps_n (%d) / fps_d (%d)",
        camerasrc->fps_auto, camerasrc->fps, fps_n, fps_d);

    mimetype = gst_structure_get_name (structure);
    if (!mimetype){
        GST_ERROR_OBJECT(camerasrc, "Failed to get mimetype info in caps");
        goto _caps_info_failed;
    }
    if (!(format_gst = gst_structure_get_string (structure, "format"))) {
        GST_DEBUG_OBJECT (camerasrc, "Couldnt get format");
    }
   camerasrc->format_str = format_gst;
   if ( strcmp(format_gst, "SN12") == 0 &&
          strcmp(format_gst, "NV12") == 0) {
        camerasrc->preview_stream->preview_format = CAM_FORMAT_YUV_420_NV12;
        camerasrc->colorspace = CAMERASRC_COL_RAW;
        GST_INFO_OBJECT (camerasrc, "received standard source format...");
    }

    camerasrc->snapshot_stream->snapshot_format= CAM_FORMAT_JPEG;//camerasrc->preview_stream->preview_format;

    camerasrc->preview_stream->format_str=camerasrc->format_str;

    if (camerasrc->use_rotate_caps) {
        GST_WARNING_OBJECT(camerasrc, "Caps : resolution (%dx%d) rotate (%d) at %d/%d fps, "
        "pix_format %d, colorspace %d", w, h, rot, fps_n, fps_d, camerasrc->preview_stream->preview_format, camerasrc->colorspace);
    }else{
        GST_WARNING_OBJECT(camerasrc, "Caps : resolution (%dx%d) at %d/%d fps, "
        "pix_format %d, colorspace %d", w, h, fps_n, fps_d, camerasrc->preview_stream->preview_format, camerasrc->colorspace);
    }

    GST_WARNING_OBJECT(camerasrc, "LEAVED");

    return TRUE;

_caps_info_failed:
    GST_ERROR_OBJECT (camerasrc, "Failed to get caps info.");

    return FALSE;
}

static void
gst_qcamerasrc_error_handler(GstCameraSrc *camerasrc, int ret)
{
    switch (ret) {
    case CAMERASRC_SUCCESS:
    break;
    case CAMERASRC_ERR_IO_CONTROL:
        GST_ELEMENT_ERROR (camerasrc, RESOURCE, FAILED, ("IO control error"), GST_ERROR_SYSTEM);
    break;
    case CAMERASRC_ERR_UNAVAILABLE_DEVICE:
        GST_ELEMENT_ERROR (camerasrc, RESOURCE, NOT_FOUND, ("Device was not found"), GST_ERROR_SYSTEM);
    break;
    case CAMERASRC_ERR_DEVICE_WAIT_TIMEOUT:
        GST_ELEMENT_ERROR (camerasrc, RESOURCE, TOO_LAZY, ("Timeout"), GST_ERROR_SYSTEM);
    break;
    case CAMERASRC_ERR_DEVICE_NOT_SUPPORT:
        GST_ELEMENT_ERROR (camerasrc, RESOURCE, SETTINGS, ("Not supported"), GST_ERROR_SYSTEM);
    break;
    case CAMERASRC_ERR_ALLOCATION:
        GST_ELEMENT_ERROR (camerasrc, RESOURCE, NO_SPACE_LEFT, ("memory allocation failed"), GST_ERROR_SYSTEM);
    break;
    case CAMERASRC_ERR_BUSY:
        GST_ELEMENT_ERROR (camerasrc, RESOURCE, BUSY, ("Device busy"), GST_ERROR_SYSTEM);
    break;
    case CAMERASRC_ERR_PRIVILEGE:
        GST_ELEMENT_ERROR (camerasrc, RESOURCE, OPEN_WRITE, ("Permission denied"), GST_ERROR_SYSTEM);
    break;
    case CAMERASRC_ERR_DEVICE_OPEN:
        GST_ELEMENT_ERROR (camerasrc, RESOURCE, OPEN_READ_WRITE, ("Open failed"), GST_ERROR_SYSTEM);
    break;

    default:
        GST_ELEMENT_ERROR (camerasrc, RESOURCE, SEEK, (("General video device error[ret=%x]"), ret), GST_ERROR_SYSTEM);
    break;
    }

    return;
}

static gpointer _gst_qcamerasrc_record_thread_func(gpointer data)
{
    GstCameraSrc *camerasrc = (GstCameraSrc *)data;
    qcamerasrc_record_handle_t *rec_handle = NULL;

    if (camerasrc == NULL || camerasrc->rec_handle == NULL) {
        GST_ERROR("handle is NULL");
        return NULL;
    }

    GST_WARNING_OBJECT(camerasrc, "start record thread");

    rec_handle = camerasrc->rec_handle;

    g_mutex_lock(&rec_handle->thread_lock);
    while (!rec_handle->thread_exit) {
        GST_WARNING_OBJECT(camerasrc, "wait for record signal");
        g_cond_wait(&rec_handle->thread_cond, &rec_handle->thread_lock);
        GST_WARNING_OBJECT(camerasrc, "record signal received");

        if (rec_handle->thread_exit) {
            break;
        }

        gst_qcamerasrc_process_cmd(camerasrc, GST_QCAMERASRC_CMD_RECORD_START);
    }
    g_mutex_unlock(&rec_handle->thread_lock);

    GST_WARNING_OBJECT(camerasrc, "record thread exit");

    return NULL;
}

static gpointer _gst_qcamerasrc_capture_done_thread_func(gpointer data)
{
    GstCameraSrc *camerasrc = (GstCameraSrc *)data;
    qcamerasrc_capture_done_handle_t *capture_done_handle = NULL;

    if (camerasrc == NULL || camerasrc->capture_done_handle == NULL) {
        GST_ERROR("handle is NULL");
        return NULL;
    }

    GST_WARNING_OBJECT(camerasrc, "start capture_done_ thread");

    capture_done_handle = camerasrc->capture_done_handle;

    g_mutex_lock(&capture_done_handle->capture_done_thread_lock);
    while (!capture_done_handle->capture_done_thread_exit) {
        GST_WARNING_OBJECT(camerasrc, "wait for capture_done_ signal");
        g_cond_wait(&capture_done_handle->capture_done_thread_cond, &capture_done_handle->capture_done_thread_lock);
        GST_WARNING_OBJECT(camerasrc, "capture_done_ signal received");

        if (capture_done_handle->capture_done_thread_exit) {
            break;
        }

        /* send message for noti to complete to get frames */
        gst_qcamerasrc_post_message_int(camerasrc, "camerasrc-Capture", "capture-done", TRUE);

    }
    g_mutex_unlock(&capture_done_handle->capture_done_thread_lock);

    GST_WARNING_OBJECT(camerasrc, "capture_done thread exit");

    return NULL;
}

static gboolean
gst_qcamerasrc_create(GstCameraSrc *camerasrc)
{
    int ret;
    qcamerasrc_record_handle_t *rec_handle = NULL;
    qcamerasrc_capture_done_handle_t *capture_done_handle = NULL;


    GST_WARNING_OBJECT(camerasrc, "start");

    if (camerasrc->qcam_handle == NULL) {
      GST_INFO("create_camera_handle");
      ret = create_camera_handle((camerasrc_handle_t**)&camerasrc->qcam_handle);

      if (ret != CAMERASRC_SUCCESS) {
          goto _ERROR;
      }
    }

    GST_WARNING_OBJECT(camerasrc, "create record thread");

    /* init record thread */
    if (camerasrc->rec_handle != NULL) {
        free(camerasrc->rec_handle);
        camerasrc->rec_handle = NULL;
    }

    rec_handle = (qcamerasrc_record_handle_t *)malloc(sizeof(qcamerasrc_record_handle_t));
    if (rec_handle == NULL) {
        GST_ERROR_OBJECT(camerasrc, "failed to alloc record handle");
        goto _ERROR;
    }

    /* init capture_done preview thread */
    if (camerasrc->capture_done_handle != NULL) {
        free(camerasrc->capture_done_handle);
        camerasrc->capture_done_handle = NULL;
    }

    capture_done_handle = (qcamerasrc_capture_done_handle_t *)malloc(sizeof(qcamerasrc_capture_done_handle_t));
    if (capture_done_handle == NULL) {
        GST_ERROR_OBJECT(camerasrc, "failed to alloc capture_done_handle");
        free(rec_handle);
        rec_handle = NULL;
        goto _ERROR;
    }

    camerasrc->rec_handle = rec_handle;
    g_mutex_init(&rec_handle->thread_lock);
    g_cond_init(&rec_handle->thread_cond);
    rec_handle->thread_exit = FALSE;
    rec_handle->thread = g_thread_try_new("qcamerasrc_record",
                                          (GThreadFunc)_gst_qcamerasrc_record_thread_func,
                                          (gpointer)camerasrc,
                                          NULL);
    if (rec_handle->thread == NULL) {
        GST_ERROR_OBJECT(camerasrc, "g_thread_try_new failed");
        free(capture_done_handle);
        capture_done_handle = NULL;
        goto _ERROR;
    }

    camerasrc->capture_done_handle = capture_done_handle;
    g_mutex_init(&capture_done_handle->capture_done_thread_lock);
    g_cond_init(&capture_done_handle->capture_done_thread_cond);
    capture_done_handle->capture_done_thread_exit = FALSE;
    capture_done_handle->capture_done_thread = g_thread_try_new("qcamerasrc_capture_done",
                                          (GThreadFunc)_gst_qcamerasrc_capture_done_thread_func,
                                          (gpointer)camerasrc,
                                          NULL);
    if (capture_done_handle->capture_done_thread == NULL) {
        GST_ERROR_OBJECT(camerasrc, "g_thread_try_new failed");
        goto _ERROR;
    }

    camerasrc->preview_stream->qcam_handle = camerasrc->qcam_handle;
    camerasrc->snapshot_stream->qcam_handle = camerasrc->qcam_handle;
    camerasrc->record_stream->qcam_handle = camerasrc->qcam_handle;

    /* set focus mode PAN if front camera */
    if (camerasrc->camera_id == CAMERASRC_DEV_ID_SECONDARY) {
        camerasrc->focus_mode = GST_QCAMERASRC_FOCUS_MODE_PAN;
    }

    GST_INFO("        open_camera_device");
    ret = open_camera_device((camerasrc_handle_t *)camerasrc->qcam_handle, camerasrc->camera_id, &(camerasrc->sensor_orientation));
    if (ret != CAMERASRC_SUCCESS) {
        GST_ERROR_OBJECT(camerasrc, "open_camera_device() failed. errcode = %d", ret);
        goto _ERROR;
    }

    GST_WARNING_OBJECT (camerasrc, "open_camera_device success - sensor orientation %d", camerasrc->sensor_orientation);

#ifdef CAMERA_INTF
    ((camerasrc_handle_t*)(camerasrc->qcam_handle))->preview_stream = (void*)camerasrc->preview_stream;
    ((camerasrc_handle_t*)(camerasrc->qcam_handle))->snapshot_stream = (void*)camerasrc->snapshot_stream;
    ((camerasrc_handle_t*)(camerasrc->qcam_handle))->record_stream = (void*)camerasrc->record_stream;
#endif

    GST_INFO("gst_qcamerasrc_fill_ctrl_list");
    if (!gst_qcamerasrc_fill_ctrl_list(camerasrc)) {
        GST_WARNING_OBJECT(camerasrc,"Can't fill v4l2 control list.");
    }

    GST_WARNING_OBJECT(camerasrc, "done");

    return TRUE;

_ERROR:
    if (camerasrc->rec_handle) {
        if (camerasrc->rec_handle->thread) {
            /* send exit signal to rec thread */
            g_mutex_lock(&camerasrc->rec_handle->thread_lock);
            GST_WARNING_OBJECT(camerasrc, "send signal to exit record thread");
            camerasrc->rec_handle->thread_exit = TRUE;
            g_cond_signal(&camerasrc->rec_handle->thread_cond);
            g_mutex_unlock(&camerasrc->rec_handle->thread_lock);

            /* wait for exit */
            g_thread_join(camerasrc->rec_handle->thread);
            camerasrc->rec_handle->thread = NULL;
        }

        /* release mutex and cond */
        g_mutex_clear(&camerasrc->rec_handle->thread_lock);
        g_cond_clear(&camerasrc->rec_handle->thread_cond);

        free(camerasrc->rec_handle);
        camerasrc->rec_handle = NULL;
    }

    if (camerasrc->capture_done_handle) {
        if (camerasrc->capture_done_handle->capture_done_thread) {
            /* send exit signal to rec capture_done_thread */
            g_mutex_lock(&camerasrc->capture_done_handle->capture_done_thread_lock);
            GST_WARNING_OBJECT(camerasrc, "send signal to exit record capture_done_thread");
            camerasrc->capture_done_handle->capture_done_thread_exit = TRUE;
            g_cond_signal(&camerasrc->capture_done_handle->capture_done_thread_cond);
            g_mutex_unlock(&camerasrc->capture_done_handle->capture_done_thread_lock);

            /* wait for exit */
            g_thread_join(camerasrc->capture_done_handle->capture_done_thread);
            camerasrc->capture_done_handle->capture_done_thread = NULL;
        }

        /* release mutex and cond */
        g_mutex_clear(&camerasrc->capture_done_handle->capture_done_thread_lock);
        g_cond_clear(&camerasrc->capture_done_handle->capture_done_thread_cond);

        free(camerasrc->capture_done_handle);
        camerasrc->capture_done_handle = NULL;
    }

    free_camera_handle((camerasrc_handle_t**)&camerasrc->qcam_handle);

    gst_qcamerasrc_error_handler(camerasrc, ret);

    return FALSE;
}


static gboolean
gst_qcamerasrc_destroy(GstCameraSrc *camerasrc)
{
    GST_WARNING_OBJECT(camerasrc, "ENTERED - mode %d", camerasrc->mode);

    /*Empty control list */
    gst_camerasrc_empty_ctrl_list(camerasrc);

    /* remove record thread */
    g_mutex_lock(&camerasrc->rec_handle->thread_lock);
    GST_WARNING_OBJECT(camerasrc, "send signal to exit record thread");
    camerasrc->rec_handle->thread_exit = TRUE;
    g_cond_signal(&camerasrc->rec_handle->thread_cond);
    g_mutex_unlock(&camerasrc->rec_handle->thread_lock);

    /* remove capture_done thread */
    g_mutex_lock(&camerasrc->capture_done_handle->capture_done_thread_lock);
    GST_WARNING_OBJECT(camerasrc, "send signal to exit capture done thread");
    camerasrc->capture_done_handle->capture_done_thread_exit = TRUE;
    g_cond_signal(&camerasrc->capture_done_handle->capture_done_thread_cond);
    g_mutex_unlock(&camerasrc->capture_done_handle->capture_done_thread_lock);

    if (VIDEO_IN_MODE_PREVIEW == camerasrc->mode) {
        stop_preview(camerasrc->preview_stream);
        camerasrc->mode = VIDEO_IN_MODE_UNKNOWN;

        GST_WARNING_OBJECT(camerasrc, "done stop_preview");
    }

    close_camera_device(camerasrc->qcam_handle);
    GST_WARNING_OBJECT(camerasrc, "close_camera_device done");
    camerasrc->ready_state = FALSE;

    /* join record thread */
    g_thread_join(camerasrc->rec_handle->thread);
    camerasrc->rec_handle->thread = NULL;
    GST_WARNING_OBJECT(camerasrc, "record thread joined");
    g_mutex_clear(&camerasrc->rec_handle->thread_lock);
    g_cond_clear(&camerasrc->rec_handle->thread_cond);
    free(camerasrc->rec_handle);
    camerasrc->rec_handle = NULL;

    g_thread_join(camerasrc->capture_done_handle->capture_done_thread);
    camerasrc->capture_done_handle->capture_done_thread = NULL;
    GST_WARNING_OBJECT(camerasrc, "capture done thread joined");
    g_mutex_clear(&camerasrc->capture_done_handle->capture_done_thread_lock);
    g_cond_clear(&camerasrc->capture_done_handle->capture_done_thread_cond);
    free(camerasrc->capture_done_handle);
    camerasrc->capture_done_handle = NULL;

    GST_WARNING_OBJECT(camerasrc, "LEAVED");

    return TRUE;
}

static gboolean gst_qcamerasrc_fill_ctrl_list(GstCameraSrc *camerasrc)
{
	int n = 0;
	qcamerasrc_ctrl_info_t ctrl_info;
	g_return_val_if_fail(camerasrc, FALSE);

	GST_DEBUG_OBJECT(camerasrc, "ENTERED");
	for (n = QCAMERASRC_COLOR_CTRL_BRIGHTNESS; n < QCAMERASRC_COLOR_CTRL_NUM; n++) {
		GstCameraSrcColorBalanceChannel *camerasrc_color_channel = NULL;
		GstColorBalanceChannel *color_channel = NULL;
		gint channel_type;

		memset(&ctrl_info, 0x0, sizeof(qcamerasrc_ctrl_info_t));

		switch (n) {
			case QCAMERASRC_COLOR_CTRL_BRIGHTNESS:
			case QCAMERASRC_COLOR_CTRL_CONTRAST:
			case QCAMERASRC_COLOR_CTRL_WHITE_BALANCE:
			case QCAMERASRC_COLOR_CTRL_COLOR_TONE:
			case QCAMERASRC_COLOR_CTRL_SATURATION:
			case QCAMERASRC_COLOR_CTRL_SHARPNESS:
				channel_type = INTERFACE_COLOR_BALANCE;
				break;
			default:
				channel_type = INTERFACE_NONE;
				continue;
		}

		if (channel_type == INTERFACE_COLOR_BALANCE) {

			camerasrc_color_channel = g_object_new(GST_TYPE_CAMERASRC_COLOR_BALANCE_CHANNEL, NULL);
			color_channel = GST_COLOR_BALANCE_CHANNEL(camerasrc_color_channel);

			color_channel->label = g_strdup((const gchar *)qcamerasrc_ctrl_label[n]);
			if(color_channel->label  == NULL)
				continue;
			camerasrc_color_channel->id = n;
			color_channel->min_value = ctrl_info.min;
			color_channel->max_value = ctrl_info.max;

			camerasrc->colors = g_list_append(camerasrc->colors, (gpointer)color_channel);
			GST_INFO_OBJECT(camerasrc, "Adding Color Balance Channel %s (%x)",
			                           color_channel->label, camerasrc_color_channel->id);
		}
	}

	GST_DEBUG_OBJECT(camerasrc, "LEAVED");
	return TRUE;
}
static gboolean gst_camerasrc_empty_ctrl_list(GstCameraSrc *camerasrc)
{
	g_return_val_if_fail(camerasrc, FALSE);

	GST_DEBUG_OBJECT (camerasrc, "ENTERED");

	g_list_foreach(camerasrc->colors, (GFunc)g_object_unref, NULL);
	g_list_free(camerasrc->colors);
	camerasrc->colors = NULL;

	g_list_foreach(camerasrc->camera_controls, (GFunc)g_object_unref, NULL);
	g_list_free(camerasrc->camera_controls);
	camerasrc->camera_controls = NULL;

	GST_DEBUG_OBJECT(camerasrc, "LEAVED");

	return TRUE;
}
void gst_qcamerasrc_post_message_int(GstCameraSrc *camerasrc, const char *msg_name, const char *field_name, int value)
{
	GstMessage *m = NULL;
	GstStructure *s = NULL;

	if (!camerasrc || !msg_name || !field_name) {
		GST_ERROR("pointer is NULL %p, %p, %p", camerasrc, msg_name, field_name);
		return;
	}

	GST_INFO("post message [%s] %s %d", msg_name, field_name, value);

	s = gst_structure_new(msg_name, field_name, G_TYPE_INT, value, NULL);
	if (s == NULL) {
		GST_ERROR("gst_structure_new failed");
		//gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_ALLOCATION);
		return;
	}

	m = gst_message_new_element(GST_OBJECT(camerasrc), s);
	if (m == NULL) {
		GST_ERROR("gst_message_new_element failed");
		//gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_ALLOCATION);
		return;
	}

	gst_element_post_message(GST_ELEMENT(camerasrc), m);

	return;
}


static long _gst_camerasrc_hdr_progressing_cb(long progress, long status, void *user_data)
{
	GstCameraSrc *camerasrc = GST_CAMERASRC(user_data);

	if (camerasrc == NULL) {
		GST_WARNING("handle is NULL");
		return HDR_PROCESS_NO_ERROR;
	}

	GST_INFO("HDR progressing %d percent completed(state:%d)...", progress, status);

	gst_qcamerasrc_post_message_int(camerasrc, "camerasrc-HDR", "progress", progress);

	return HDR_PROCESS_NO_ERROR;
}


static gboolean gst_camerasrc_run_hdr_processing(GstCameraSrc *camerasrc, qcamerasrc_buffer_t *input_buffer, HDRInputPictureFormat format, unsigned char **result_data)
{
	int ret = HDR_PROCESS_NO_ERROR;
	if (camerasrc == NULL || result_data == NULL) {
		GST_ERROR(camerasrc,"handle[%p] or result_data[%p] is NULL",
		camerasrc, result_data);
		return FALSE;
	}

	GST_INFO(camerasrc,"cap_height = %d,cap_width =%d format=%d",camerasrc->cap_height,camerasrc->cap_width,format);
	ret= run_hdr_processing(input_buffer,
							camerasrc->cap_width,
							camerasrc->cap_height,
							format,
							result_data,
							(void*)_gst_camerasrc_hdr_progressing_cb,
							(void*)camerasrc);
	if(HDR_PROCESS_NO_ERROR!=ret){
		GST_ERROR(camerasrc,"HDR process failed with error %d", ret);
		return FALSE;
	}
	GST_INFO(camerasrc,"HDR process success");
	return TRUE;
}

static int gst_camerasrc_raw_capture_done_cb(void *usr_param,int state)
{
	GstCameraSrc *camerasrc = (GstCameraSrc *)usr_param;
#ifndef DEFAULT_ZSL_MODE_ON
	if (camerasrc->mode == VIDEO_IN_MODE_CAPTURE)
#endif
	{
	    g_mutex_lock(&camerasrc->capture_done_handle->capture_done_thread_lock);
	    GST_WARNING_OBJECT(camerasrc, "send capture_done  signal");
	    g_cond_signal(&camerasrc->capture_done_handle->capture_done_thread_cond);
	    g_mutex_unlock(&camerasrc->capture_done_handle->capture_done_thread_lock);
	}

	return CAMERASRC_SUCCESS;
}

static int gst_camerasrc_af_cb(void *usr_param,int state)
{

	GstCameraSrc *camerasrc = (GstCameraSrc *)usr_param;

	if (camerasrc->is_focusing) {
		gst_qcamerasrc_post_message_int(camerasrc, "camerasrc-AF", "focus-state", state);
	} else {
		GST_WARNING("not focusing status. skip focus message %d", state);
	}

	return CAMERASRC_SUCCESS;
}

static int qcamerasrc_get_face_detect_info(GstCameraSrc *camerasrc, camera_frame_metadata_t *metadata, GstCameraControlFaceDetectInfo **fd_info)
{
    int num_of_faces = 0;
    int i = 0;
    int rect_x = 0;
    int rect_y = 0;
    GstCameraControlFaceDetectInfo *fd_info_tmp = NULL;

    if(camerasrc == NULL || !fd_info){
        GST_ERROR("pointer[%p] or fd_info[%p] is NULL",
                    camerasrc, fd_info);
        return CAMERASRC_ERR_INVALID_PARAMETER;
    }

    if(metadata){
        num_of_faces = metadata->number_of_faces;
    }

    if(num_of_faces < 1 && !camerasrc->face_detected){
        *fd_info = NULL;
        return CAMERASRC_SUCCESS;
    }

    if(num_of_faces < 1){
        camerasrc->face_detected = FALSE;
    } else {
        camerasrc->face_detected = TRUE;
    }

    if(num_of_faces > MAX_FACE_NUM){
       GST_WARNING("too many faces are detected %d, set max %d", num_of_faces, MAX_FACE_NUM);
        num_of_faces = MAX_FACE_NUM;
    }

    fd_info_tmp = (GstCameraControlFaceDetectInfo *)malloc(sizeof(GstCameraControlFaceDetectInfo));
    if(!fd_info_tmp){
        GST_ERROR("fd_info alloc failed");
        *fd_info = NULL;
        return CAMERASRC_ERR_ALLOCATION;
    }

    memset(fd_info_tmp, 0x0, sizeof(GstCameraControlFaceDetectInfo));
    fd_info_tmp->num_of_faces = num_of_faces;

    if(metadata){
        for(i =0 ; i < num_of_faces ; i++) {
            fd_info_tmp->face_info[i].id = i;
            fd_info_tmp->face_info[i].score = metadata->faces[i].score;
            rect_x = metadata->faces[i].rect[0];
            rect_y = metadata->faces[i].rect[1];
            fd_info_tmp->face_info[i].rect.x = rect_x;
            fd_info_tmp->face_info[i].rect.y = rect_y;
            fd_info_tmp->face_info[i].rect.width = metadata->faces[i].rect[2];
            fd_info_tmp->face_info[i].rect.height = metadata->faces[i].rect[3];
        }
    }
    *fd_info = fd_info_tmp;
    return CAMERASRC_SUCCESS;
}

static void
gst_camerasrc_post_message_pointer(GstCameraSrc *camerasrc,
        const char *msg_name, const char *field_name, gpointer value)
{
        GstMessage *message = NULL;
        GstStructure *structure = NULL;
        structure = gst_structure_new(msg_name, field_name, G_TYPE_POINTER, value, NULL);
        message = gst_message_new_element(GST_OBJECT(camerasrc), structure);

        if(!gst_element_post_message(GST_ELEMENT(camerasrc), message)) {
            GST_ERROR("gst_element_post_message failed.");
            return;
        }
        GST_INFO("post face detect info successfully");
		return;
}

static void gst_qcamerasrc_post_face_detection_info (GstCameraSrc *camerasrc, camera_frame_metadata_t *metadata)
{
    GstCameraControlFaceDetectInfo *fd_info = NULL;
    if(camerasrc == NULL || metadata == NULL){
        GST_WARNING("pointer[%p] or metadata[%p] is NULL",
                    camerasrc, metadata);
        return ;
    }

    qcamerasrc_get_face_detect_info(camerasrc, metadata, &fd_info);

    /* post message if face is detected */
    if (fd_info) {
        gst_camerasrc_post_message_pointer(camerasrc, "camerasrc-FD", "face-info", (gpointer)fd_info);
    }
    return;
}

static int
gst_camerasrc_preview_metadata_cb(void *usr_param,camera_frame_metadata_t *metadata)
{
    GstCameraSrc *camerasrc = (GstCameraSrc *)usr_param;
    //GST_WARNING_OBJECT(camerasrc, "metadata->number_of_faces = %d",metadata->number_of_faces);
    if(CAMERASRC_FACE_DETECT_ON==camerasrc->face_detect)
    {
        if((camerasrc->is_capturing==FALSE) && ((camerasrc->face_cb_flag)%2 == 0 || metadata->number_of_faces == 0)) {
            gst_qcamerasrc_post_face_detection_info (camerasrc, metadata);
            camerasrc->face_cb_flag = 0;
        }
        camerasrc->face_cb_flag = (camerasrc->face_cb_flag+1)%2;
    }
    /*Handle face detect result here as well*/

#ifdef CONFIG_MACH_Z3LTE
    if(LOW_LIGHT_DETECTION_ON==camerasrc->low_light_detection){
        if(camerasrc->low_light_condition!=metadata->needLLS){
            GST_WARNING_OBJECT(camerasrc,"Low light condition changed to [%d]",metadata->needLLS);
            gst_qcamerasrc_post_message_int(camerasrc,
                                                                "camerasrc-Light",
                                                                "low-light-state",
                                                                metadata->needLLS);
            camerasrc->low_light_condition = metadata->needLLS;
        }
    }
#endif

    return CAMERASRC_SUCCESS;
}

static gboolean
gst_qcamerasrc_start(GstCameraSrc *camerasrc)
{
    int set_flip = CAMERASRC_FLIP_NONE;
    gboolean result = FALSE;

    GST_WARNING_OBJECT(camerasrc, "ENTERED");

    if (camerasrc->is_start_failed) {
        GST_ERROR_OBJECT(camerasrc, "already error is occurred");
        return FALSE;
    }

    /*set focus callback function callback*/
    camerasrc->preview_stream->af_cb = gst_camerasrc_af_cb;

    /*set rawcapture callback function callback*/
    camerasrc->snapshot_stream->raw_capture_cb = gst_camerasrc_raw_capture_done_cb;

    /*set preview metadata(lowlight/face detect) callback function callback*/
    camerasrc->preview_stream->preview_metadata_cb = gst_camerasrc_preview_metadata_cb;

    /* set Focus mode */
    gst_camerasrc_control_set_focus(camerasrc, camerasrc->focus_mode, camerasrc->focus_range);

    /* set snapshot resolution */
    set_snapshot_dimension(camerasrc->snapshot_stream, camerasrc->cap_width, camerasrc->cap_height);

    /* init number of live buffers */
    camerasrc->num_live_buffers = 0;

    camerasrc->qbuf_without_display = 0;

    /* init restart flag */
    camerasrc->snapshot_stream->restart_flag = FALSE;

    /* is_capturing flag */
    camerasrc->is_capturing = FALSE;

    /* face_cb_flag for dropping cb */
    camerasrc->face_cb_flag = 0;

    /* auto lls detect */
    GST_WARNING_OBJECT(camerasrc, "low_light_detection (%d)", camerasrc->low_light_detection);
    set_low_light_auto_detection((camerasrc_handle_t *)camerasrc->qcam_handle, camerasrc->low_light_detection);

    /* night shot mode */
    GST_WARNING("camerasrc->shot_mode =%d",camerasrc->shot_mode);
    if(camerasrc->shot_mode == 1) {
       camerasrc->preview_stream->low_light_mode = 1;
    }	else {
       camerasrc->preview_stream->low_light_mode = 0;
    }
    set_low_light_mode((camerasrc_handle_t *)camerasrc->qcam_handle, camerasrc->preview_stream->low_light_mode);

#ifndef CONFIG_MACH_Z3LTE
    }
    /*set dual camera mode*/
    GST_INFO("                set_dual_camera_mode");
    set_dual_camera_mode(camerasrc, camerasrc->dual_camera_mode);
#endif
    /*set vt mode*/
    gst_qcamerasrc_set_camera_control(camerasrc,
                    QCAMERASRC_PARAM_CONTROL_VT_MODE,
                    camerasrc->vt_mode,0,NULL);

    /* set flip */
    if (camerasrc->vflip && camerasrc->hflip) {
        set_flip = CAMERASRC_FLIP_BOTH;
    } else if (camerasrc->hflip) {
        set_flip = CAMERASRC_FLIP_HORIZONTAL;
    } else if (camerasrc->vflip) {
        set_flip = CAMERASRC_FLIP_VERTICAL;
    }

    GST_WARNING_OBJECT(camerasrc, "hflip %d, vflip %d, set_flip %d",
                                  camerasrc->hflip, camerasrc->vflip, set_flip);

    set_preview_flip(camerasrc->preview_stream, set_flip);
    set_snapshot_flip(camerasrc->snapshot_stream, set_flip);
    set_record_flip(camerasrc->record_stream, set_flip);

    if (camerasrc->enable_hybrid_mode) {
        GST_WARNING_OBJECT(camerasrc, "start RECORDING STREAM");
        GST_INFO("                prepare_record");
        prepare_record(camerasrc->record_stream);
        camerasrc->preview_stream->recording_hint = TRUE;
    } else {
        GST_WARNING_OBJECT(camerasrc, "start PREVIEW STREAM");
        GST_INFO("unprepare_record");
        unprepare_record(camerasrc->record_stream);
        camerasrc->preview_stream->recording_hint = FALSE;
    }

    // HDR SET if preset mode and postset mode is different
    GST_DEBUG_OBJECT(camerasrc,"gst_qcamerasrc_start pre hdr mode = %d , hdr mode = %d" ,camerasrc->snapshot_stream->m_pre_hdr_mode,camerasrc->snapshot_stream->m_hdr_mode );
    if (camerasrc->snapshot_stream->m_hdr_mode != camerasrc->snapshot_stream->m_pre_hdr_mode){
        gst_qcamerasrc_set_camera_control(camerasrc,
            QCAMERASRC_PARAM_CONTROL_HDR,
            camerasrc->snapshot_stream->m_hdr_mode,0,NULL);
        send_command(camerasrc->preview_stream->qcam_handle->cam_device_handle,CAMERA_CMD_HDR_MODE, camerasrc->snapshot_stream->m_hdr_mode, 0);
        camerasrc->snapshot_stream->m_pre_hdr_mode = camerasrc->snapshot_stream->m_hdr_mode;
    }

    send_command(camerasrc->preview_stream->qcam_handle->cam_device_handle,CAMERA_CMD_SET_CAMERA_ACCESS_MODE, camerasrc->camera_cpu_clock_lock, 0);

    // Set Jpeg Quality
    gst_qcamerasrc_set_camera_control(camerasrc,
                    QCAMERASRC_PARAM_CONTROL_JPEG_QUALITY,
                    camerasrc->cap_jpg_quality,0,NULL);

    send_command(camerasrc->preview_stream->qcam_handle->cam_device_handle,CAMERA_CMD_SET_SAMSUNG_CAMERA, 0, 0);

    settings_batch_update_from_cache(camerasrc);

    GST_INFO("start_preview");
    result = start_preview(camerasrc->preview_stream);

    if (!result) {
        GST_WARNING_OBJECT( camerasrc, "start_preview fails");
        camerasrc->is_start_failed = TRUE;
        gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_INTERNAL);
    } else {
        camerasrc->mode = VIDEO_IN_MODE_PREVIEW;
        camerasrc->firsttime = TRUE;
        camerasrc->stop_camera = FALSE;
    }

    GST_WARNING_OBJECT(camerasrc, "LEAVED - result %d", result);

    return result;
}


static gboolean
gst_qcamerasrc_stop(GstCameraSrc *camerasrc)
{
    GST_WARNING_OBJECT (camerasrc, "ENTERED - mode %d", camerasrc->mode);

    if (VIDEO_IN_MODE_UNKNOWN != camerasrc->mode) {
        camerasrc->mode = VIDEO_IN_MODE_STOP;

        /* release remained buffer in queue */
        GST_INFO("STOP_PREVIEW:gst_qcamerasrc_release_all_queued_preview_buffer");
        gst_qcamerasrc_release_all_queued_preview_buffer(camerasrc);

        /* wait all preview buffer finalized for previiew restart */
        GST_INFO("        STOP_PREVIEW:gst_qcamerasrc_wait_all_preview_buffer_finalized");
        gst_qcamerasrc_wait_all_preview_buffer_finalized(camerasrc);

        /* stop preview and release buffer */
        GST_INFO("        STOP_PREVIEW:stop_preview");
        stop_preview(camerasrc->preview_stream);

        camerasrc->mode = VIDEO_IN_MODE_UNKNOWN;
    }

    GST_WARNING_OBJECT (camerasrc, "LEAVED");

    return TRUE;
}

#ifdef DEVELOPER_TEST
static void *record_thread(void *data){
    GstCameraSrc *camerasrc=(GstCameraSrc *)data;
    camerasrc->record_stream->record_width = camerasrc->width;
    camerasrc->record_stream->record_height = camerasrc->height;

    GST_DEBUG_OBJECT(camerasrc,"start video stream - size [%dx%d]",
        camerasrc->video_width, camerasrc->video_height);

    start_record(camerasrc->record_stream, TRUE);
}
#endif


static gboolean gst_qcamerasrc_process_cmd(GstCameraSrc *camerasrc, int cmd)
{
    if (!camerasrc){
        GST_ERROR_OBJECT(camerasrc, "NULL handle");
        return FALSE;
    }

    switch (cmd) {
    case GST_QCAMERASRC_CMD_PREVIEW_RESTART:
        GST_WARNING_OBJECT(camerasrc, "GST_QCAMERASRC_CMD_PREVIEW_RESTART was called");
        /* stop preview */
        gst_qcamerasrc_stop(camerasrc);

        if((camerasrc->snapshot_stream->longshot_mode == FALSE)
            &&(camerasrc->snapshot_stream->capture_num > 1)
             &&(camerasrc->cap_hdr == CAMERASRC_HDR_CAPTURE_OFF)){
            /*WARNING:Cannot set interval after ZSL stream has started.Will add it in later.*/
            int skip_frame = camerasrc->snapshot_stream->capture_interval/50;
            if (skip_frame < 1) {
                 skip_frame = 1;
            }
            GST_WARNING_OBJECT(camerasrc, "burst capture num=%d and skip_frame=%d",
                                                camerasrc->snapshot_stream->capture_num,skip_frame);
            gst_qcamerasrc_set_camera_control(camerasrc,
                                    QCAMERASRC_PARAM_CONTROL_BURST_CONFIG,
                                    camerasrc->snapshot_stream->capture_num,
                                    skip_frame,
                                    NULL);
        }

        /* start preview */
        gst_qcamerasrc_start(camerasrc);

        /* completed signal */
        g_cond_signal(camerasrc->cmd_cond);
        g_cond_signal(&camerasrc->snapshot_stream->cmd_cond);
        GST_WARNING_OBJECT(camerasrc, "camerasrc->snapshot_stream->cmd_cond was signaled");
        break;
    case GST_QCAMERASRC_CMD_RECORD_START:
        if (!camerasrc->enable_hybrid_mode) {
            /* set mode as UNKNOWN to stop preview */
            camerasrc->mode = VIDEO_IN_MODE_STOP;

            /* release buffer in queue */
            GST_INFO("		RECORDING_START:gst_qcamerasrc_release_all_queued_preview_buffer");
            gst_qcamerasrc_release_all_queued_preview_buffer(camerasrc);

            /* wait all preview buffer finalized for previiew restart */
            GST_INFO("		RECORDING_START:gst_qcamerasrc_wait_all_preview_buffer_finalized");
            gst_qcamerasrc_wait_all_preview_buffer_finalized(camerasrc);

        }

        GST_WARNING_OBJECT(camerasrc, "start video stream - resolution [%dx%d]",
                                      camerasrc->record_stream->record_width,
                                      camerasrc->record_stream->record_height);

        camerasrc->firsttime_record = TRUE;
        camerasrc->num_video_buffers = 0;


        /* set mode as VIDEO */
        camerasrc->mode = VIDEO_IN_MODE_VIDEO;

        GST_INFO("		qcamerasrc:start_record");
        if(!start_record(camerasrc->record_stream, !camerasrc->enable_hybrid_mode))
        {
                GST_ERROR_OBJECT(camerasrc, "start_record is failed.");
                gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_INTERNAL);
        }
        /* completed signal */
        g_cond_signal(camerasrc->cmd_cond);
        break;
    case GST_QCAMERASRC_CMD_RECORD_STOP:
        GST_WARNING_OBJECT(camerasrc, "stop record in read_frame");
        if (!camerasrc->enable_hybrid_mode) {
            /* set mode as UNKNOWN to stop preview */
            camerasrc->mode = VIDEO_IN_MODE_STOP;

            /* release buffer in queue */
            GST_INFO("		RECORDING_STOP:gst_qcamerasrc_release_all_queued_preview_buffer");
            gst_qcamerasrc_release_all_queued_preview_buffer(camerasrc);

            /* wait all preview buffer finalized for previiew restart */
            GST_INFO("		RECORDING_STOP:gst_qcamerasrc_wait_all_preview_buffer_finalized");
            gst_qcamerasrc_wait_all_preview_buffer_finalized(camerasrc);
        } else {
            /* set mode as PREVIEW */
            camerasrc->mode = VIDEO_IN_MODE_PREVIEW;
        }

        /* wait for video buffer finalized */
        GST_INFO("		RECORDING_STOP:gst_qcamerasrc_wait_all_video_buffer_finalized");
        gst_qcamerasrc_wait_all_video_buffer_finalized(camerasrc);

        GST_INFO("		RECORDING_STOP:stop_record");
        if(!stop_record(camerasrc->record_stream, !camerasrc->enable_hybrid_mode))
        {
                GST_ERROR_OBJECT(camerasrc, "stop_record is failed.");
                gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_INTERNAL);
        }
        GST_WARNING_OBJECT(camerasrc, "stop record done in read_frame");

        /* completed signal */
        g_cond_signal(camerasrc->cmd_cond);

        /* set mode as PREVIEW */
        camerasrc->mode = VIDEO_IN_MODE_PREVIEW;
        break;
    case GST_QCAMERASRC_CMD_CAPTURE_START:
        GST_INFO("    capture_snapshot start");
            capture_snapshot (camerasrc);
        break;
    case GST_QCAMERASRC_CMD_CAPTURE_STOP:
        /*Not handled now*/
        break;
    default:
        break;
    }

    return TRUE;
}

static int
cache_settings_for_batch_update(GstCameraSrc* camerasrc,int64_t control_type,int value_1,int value_2,char* string_1){
    if(!camerasrc->batch_control_value_cache){
        camerasrc->batch_control_value_cache=(qcamerasrc_batch_ctrl_t*)malloc(sizeof(qcamerasrc_batch_ctrl_t));
        if(!camerasrc->batch_control_value_cache){
            GST_INFO_OBJECT(camerasrc, "alloc failed for batch_control_value_cache");
            return CAMERASRC_ERROR;
        }
        memset(camerasrc->batch_control_value_cache, 0, sizeof (qcamerasrc_batch_ctrl_t));
        camerasrc->batch_control_id_cache=QCAMERASRC_PARAM_CONTROL_START;
    }
    camerasrc->batch_control_id_cache=camerasrc->batch_control_id_cache|control_type;
    camerasrc->setting_value_cached=TRUE;

    switch(control_type)
    {
        case QCAMERASRC_PARAM_CONTROL_PREVIEW_SIZE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_PREVIEW_SIZE %dx%d", value_1, value_2);
            camerasrc->batch_control_value_cache->preview_width=value_1;
            camerasrc->batch_control_value_cache->preview_height=value_2;
            break;
        case QCAMERASRC_PARAM_CONTROL_PREVIEW_FORMAT:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_PREVIEW_FORMAT %d", value_1);
            camerasrc->batch_control_value_cache->preview_format=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_VIDEO_SIZE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_VIDEO_SIZE %dx%d", value_1, value_2);
            camerasrc->batch_control_value_cache->video_width=value_1;
            camerasrc->batch_control_value_cache->video_height=value_2;
            break;
        case QCAMERASRC_PARAM_CONTROL_VIDEO_FORMAT:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_VIDEO_FORMAT %d", value_1);
            camerasrc->batch_control_value_cache->video_format=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_RECORDING_HINT:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_RECORDING_HINT %d %d", value_1, value_2);
            camerasrc->batch_control_value_cache->recording_hint=value_1;
            camerasrc->batch_control_value_cache->hfr_value=value_2;
            break;
        case QCAMERASRC_PARAM_CONTROL_SNAPSHOT_SIZE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_SNAPSHOT_SIZE %dx%d", value_1, value_2);
            camerasrc->batch_control_value_cache->snapshot_width=value_1;
            camerasrc->batch_control_value_cache->snapshot_height=value_2;
            break;
        case QCAMERASRC_PARAM_CONTROL_SNAPSHOT_FORMAT:
            camerasrc->batch_control_value_cache->snapshot_format=value_1;
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_SNAPSHOT_FORMAT %d", value_1);
            break;
        case QCAMERASRC_PARAM_CONTROL_FPS_MODE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_FPS_MODE %d", value_1);
            camerasrc->batch_control_value_cache->fps_mode=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_FPS:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_FPS %d %d", value_1, value_2);
            camerasrc->batch_control_value_cache->fps_min=value_1;
            camerasrc->batch_control_value_cache->fps_max=value_2;
            break;
        case QCAMERASRC_PARAM_CONTROL_BURST_CONFIG:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_BURST_CONFIG %d %d", value_1, value_2);
            camerasrc->batch_control_value_cache->burst_number=value_1;
            camerasrc->batch_control_value_cache->burst_interval=value_2;
            break;
        case QCAMERASRC_PARAM_CONTROL_FACE_DETECT:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_FACE_DETECT %d", value_1);
            camerasrc->batch_control_value_cache->face_detect=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_FLASH_MODE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_FLASH_MODE %d", value_1);
            camerasrc->batch_control_value_cache->flash_mode=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_BRIGHTNESS:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_BRIGHTNESS %d", value_1);
            camerasrc->batch_control_value_cache->brightness_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_CONTRAST:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_CONTRAST %d", value_1);
            camerasrc->batch_control_value_cache->contrast_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_COLOR_TONE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_COLOR_TONE %d", value_1);
            camerasrc->batch_control_value_cache->color_tone_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_PROGRAM_MODE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_PROGRAM_MODE %d", value_1);
            camerasrc->batch_control_value_cache->program_mode=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_PARTCOLOR_SRC:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_PARTCOLOR_SRC %d", value_1);
            camerasrc->batch_control_value_cache->partcolor_src=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_PARTCOLOR_DST:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_PARTCOLOR_DST %d", value_1);
            camerasrc->batch_control_value_cache->partcolor_dest=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_PARTCOLOR_MODE:
            camerasrc->batch_control_value_cache->partcolor_mode=value_1;
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_PARTCOLOR_MODE %d", value_1);
            break;
        case QCAMERASRC_PARAM_CONTROL_WIDE_DYNAMIC_RANGE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_WIDE_DYNAMIC_RANGE %d", value_1);
            camerasrc->batch_control_value_cache->wdr_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_SATURATION:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_SATURATION %d", value_1);
            camerasrc->batch_control_value_cache->saturation_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_SHARPNESS:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_SHARPNESS %d", value_1);
            camerasrc->batch_control_value_cache->sharpness_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_PHOTOMETRY:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_PHOTOMETRY %d", value_1);
            camerasrc->batch_control_value_cache->photometry_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_METERING:
            camerasrc->batch_control_value_cache->metering_value=value_1;
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_METERING %d", value_1);
            break;
        case QCAMERASRC_PARAM_CONTROL_WB:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_WB %d", value_1);
            camerasrc->batch_control_value_cache->wb_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_ISO:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_ISO %d", value_1);
            camerasrc->batch_control_value_cache->iso_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_ANTI_SHAKE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_ANTI_SHAKE %d", value_1);
            camerasrc->batch_control_value_cache->antishake_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_LOW_LIGHT_DETECTION:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_LOW_LIGHT_DETECTION %d", value_1);
            camerasrc->batch_control_value_cache->lld_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_BURST_SHOT:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_BURST_SHOT %d", value_1);
            camerasrc->batch_control_value_cache->burst_shot_config=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_ZOOM:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_ZOOM %d", value_1);
            camerasrc->batch_control_value_cache->zoom_value=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_AF:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_AF %d", value_1);
            camerasrc->batch_control_value_cache->af_config=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_AF_AREAS:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_AF_AREAS %s", string_1);
            if(camerasrc->batch_control_value_cache->af_area){
                free(camerasrc->batch_control_value_cache->af_area);
            }
            camerasrc->batch_control_value_cache->af_area=(char*)malloc(strlen(string_1));
            memcpy(camerasrc->batch_control_value_cache->af_area,string_1,strlen(string_1));
            break;
        case QCAMERASRC_PARAM_CONTROL_SCENE_MODE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_SCENE_MODE %d", value_1);
            camerasrc->batch_control_value_cache->scene_mode=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_HDR:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_HDR %d", value_1);
            camerasrc->batch_control_value_cache->hdr_ae_bracket=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_PREVIEW_FLIP:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_PREVIEW_FLIP %d", value_1);
            camerasrc->batch_control_value_cache->preview_flip=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_VIDEO_FLIP:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_VIDEO_FLIP %d", value_1);
            camerasrc->batch_control_value_cache->video_flip=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_SNAPSHOT_FLIP:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_SNAPSHOT_FLIP %d", value_1);
            camerasrc->batch_control_value_cache->snapshot_flip=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_SHOOTING_MODE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_SHOOTING_MODE %d", value_1);
            camerasrc->batch_control_value_cache->shoting_mode=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_DUAL_CAMERA_MODE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_DUAL_CAMERA_MODE %d", value_1);
            camerasrc->batch_control_value_cache->dual_cam_mode=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_VDIS:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_VDIS %d", value_1);
            camerasrc->batch_control_value_cache->vdis_mode=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_JPEG_QUALITY:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_JPEG_QUALITY %d", value_1);
            camerasrc->batch_control_value_cache->jpeg_quality=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_DUAL_RECORDING_HINT:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_DUAL_RECORDING_HINT %d", value_1);
            camerasrc->batch_control_value_cache->dual_recording_hint=value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_AE_LOCK:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_AE_LOCK %d", value_1);
            camerasrc->batch_control_value_cache->ae_lock = value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_AWB_LOCK:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_AWB_LOCK %d", value_1);
            camerasrc->batch_control_value_cache->awb_lock = value_1;
            break;
        case QCAMERASRC_PARAM_CONTROL_VT_MODE:
            GST_INFO_OBJECT(camerasrc, "QCAMERASRC_PARAM_CONTROL_VT_MODE %d", value_1);
            camerasrc->batch_control_value_cache->vt_mode = value_1;
            break;
        default:
            GST_ERROR_OBJECT(camerasrc, "Invalid setting : 0x%x", control_type);
            break;
    }
    return CAMERASRC_SUCCESS;
}

static int
settings_batch_update_from_cache(GstCameraSrc* camerasrc){
    GST_WARNING_OBJECT(camerasrc, "ENTER - value cached %d", camerasrc->setting_value_cached);
    if(camerasrc->setting_value_cached && camerasrc->batch_control_value_cache){
        set_camera_launch_setting((camerasrc_handle_t *)camerasrc->qcam_handle,
                                                        camerasrc->batch_control_id_cache,
                                                        camerasrc->batch_control_value_cache);
        camerasrc->setting_value_cached=FALSE;
        camerasrc->batch_control_id_cache=QCAMERASRC_PARAM_CONTROL_START;
    }
    GST_WARNING_OBJECT(camerasrc, "LEAVE");
    return CAMERASRC_SUCCESS;
}

static GstFlowReturn
gst_qcamerasrc_read_frame(GstCameraSrc *camerasrc,GstBuffer **buffer)
{
    GstFlowReturn ret=GST_FLOW_OK;
    GstBuffer *black_frame_buffer = NULL;
    int black_buf_len = 0;
    int cmd = GST_QCAMERASRC_CMD_NONE;
    static int frame_count = 0;
    GstMemory *img_buf_memory = NULL;
    GstMapInfo mapInfo;
#ifdef DEVELOPER_TEST
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    static int counter=0;
    counter++;
    if(counter==50){
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&mRecordThread, &attr, record_thread, (void *) camerasrc);
    }
#endif

    g_mutex_lock(camerasrc->stop_camera_lock);
    if (camerasrc->stop_camera) {
        GST_WARNING_OBJECT(camerasrc, "stop camera thread");
        g_cond_signal(camerasrc->stop_camera_cond);
        g_mutex_unlock(camerasrc->stop_camera_lock);
        gst_qcamerasrc_stop(camerasrc);
        return GST_FLOW_EOS;
    }
    g_mutex_unlock(camerasrc->stop_camera_lock);

    /* check command */
    g_mutex_lock(camerasrc->cmd_lock);
    if (!g_queue_is_empty(camerasrc->cmd_list)) {
        /* process cmd */
        if(!camerasrc->enable_hybrid_mode  && (int)g_queue_peek_head(camerasrc->cmd_list)==GST_QCAMERASRC_CMD_RECORD_START
                && camerasrc->snapshot_stream->restart_flag) {
                GST_WARNING_OBJECT(camerasrc, "we will run the record start command in next read frame");
        } else {
                cmd = (int)g_queue_pop_head(camerasrc->cmd_list);
                gst_qcamerasrc_process_cmd(camerasrc, cmd);
        }
    }
    g_mutex_unlock(camerasrc->cmd_lock);

    switch (camerasrc->mode) {
        case VIDEO_IN_MODE_PREVIEW:
        case VIDEO_IN_MODE_VIDEO:
            GST_DEBUG_OBJECT(camerasrc, "Getting mutex in thread %p",g_thread_self ());
            get_preview_stream_mutex (camerasrc->preview_stream);
            GST_DEBUG_OBJECT(camerasrc, "Got mutex in thread %p",g_thread_self ());

            wait_for_frame (camerasrc->preview_stream);
            if (frame_available(camerasrc->preview_stream)){
                *buffer = get_preview_frame (camerasrc->preview_stream);
                frame_count = 0;
#if ENABLE_ZERO_COPY
                if(gst_qcamerasrc_is_zero_copy_format(camerasrc->format_str)){
                    MMVideoBuffer* img_buffer =  NULL;
                    if (gst_buffer_n_memory(*buffer) > 1){
                        img_buf_memory = gst_buffer_peek_memory(*buffer,1);
                        gst_memory_map(img_buf_memory,&mapInfo,GST_MAP_WRITE);
                        img_buffer = (MMVideoBuffer *)mapInfo.data;
                    }

                    if (img_buffer) {
                        GST_DEBUG_OBJECT(camerasrc, "Passing Frame[%d] size [%dx%d]",
                                                    img_buffer->handle.dmabuf_fd[0], img_buffer->stride_width[0], img_buffer->stride_height[0]);
#ifdef CONFIG_MACH_Z3LTE
                        if (camerasrc->signal_still_capture && camerasrc->snapshot_buffer != NULL) {
                            camerasrc->signal_still_capture = FALSE;
                            capture_buffer_info_t *img_info = malloc(sizeof(capture_buffer_info_t));
                            if (img_info) {
                                img_info->data = camerasrc->snapshot_buffer;
                                img_info->size = camerasrc->snapshot_buffer_size;
                                camerasrc->snapshot_buffer = NULL;
                                camerasrc->snapshot_buffer_size = 0;
                                img_info->width = camerasrc->cap_width;
                                img_info->height = camerasrc->cap_height;
                                img_info->fourcc = camerasrc->cap_fourcc;
                                img_buffer->jpeg_data = (void *)img_info;
                                img_buffer->jpeg_size= sizeof(capture_buffer_info_t);
                            } else {
                                GST_WARNING_OBJECT(camerasrc, "malloc failed on jpeg_buffer_info_t");
                                gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_ALLOCATION);
                            }
                            GST_DEBUG_OBJECT(camerasrc, "signal_still_capture Snapshot Frame[%p] size [%d]",
                                            img_buffer->jpeg_data, img_buffer->jpeg_size);
                        }
#endif
                        if (camerasrc->snapshot_stream->restart_flag) {
                            GST_WARNING_OBJECT(camerasrc, "make FLUSH_BUFFER");
                            img_buffer->flush_request = TRUE;
                            if(camerasrc->snapshot_stream->restart_flag ==GST_QCAMERASRC_CMD_RECORD_START) {
                                camerasrc->snapshot_stream->restart_flag = FALSE;
                            } else {
                                camerasrc->snapshot_stream->restart_flag = FALSE;
                                g_queue_push_tail(camerasrc->cmd_list, (gpointer)GST_QCAMERASRC_CMD_PREVIEW_RESTART);
                            }
                        }else if(camerasrc->received_capture_command){
                            GST_WARNING_OBJECT(camerasrc, "make FLUSH_BUFFER");
                            img_buffer->flush_request = TRUE;
                            camerasrc->received_capture_command = FALSE;
                            g_mutex_lock(camerasrc->cmd_lock);
                            g_queue_push_tail(camerasrc->cmd_list, (gpointer)GST_QCAMERASRC_CMD_CAPTURE_START);
                            g_mutex_unlock(camerasrc->cmd_lock);
                        }
                        gst_memory_unmap(img_buf_memory,&mapInfo);
                    }
                }
#endif
            }else if (camerasrc->received_capture_command) {
                if(camerasrc->img_buffer_cache){

                    GST_WARNING_OBJECT(camerasrc, "make dummy FLUSH_BUFFER");
                    camerasrc->img_buffer_cache->flush_request = TRUE;
                    *buffer = gst_buffer_new();
                    GstMapInfo map;
                    gst_buffer_map(*buffer,&map,GST_MAP_WRITE);
                    map.data = camerasrc->img_buffer_cache->data[0];
                    map.size = ((camerasrc->width * camerasrc->height * 3) >> 1);
                    gst_buffer_unmap(*buffer,&map);

                    gst_buffer_append_memory(*buffer, gst_memory_new_wrapped(0,
                                                camerasrc->img_buffer_cache->data[0],
                                                (camerasrc->width * camerasrc->height * 3) >> 1,
                                                0,
                                                (camerasrc->width * camerasrc->height * 3) >> 1,NULL,NULL));

                    gst_buffer_append_memory(*buffer, gst_memory_new_wrapped(0,
                                                camerasrc->img_buffer_cache, sizeof(*camerasrc->img_buffer_cache), 0,
                                                sizeof(*camerasrc->img_buffer_cache), NULL,NULL));


                    camerasrc->received_capture_command = FALSE;
                    /*gst_buffer_unref will free GST_BUFFER_MALLOCDATA so allocate new*/
                    camerasrc->img_buffer_cache = (MMVideoBuffer*) g_malloc(sizeof(MMVideoBuffer));
                    g_mutex_lock(camerasrc->cmd_lock);
                    g_queue_push_tail(camerasrc->cmd_list, (gpointer)GST_QCAMERASRC_CMD_CAPTURE_START);
                    g_mutex_unlock(camerasrc->cmd_lock);
                }
            }else{
                GST_WARNING_OBJECT(camerasrc, "Failed to retrive preview frame. frame_count(%d), num live buffers (%d)",
                                              frame_count, camerasrc->num_live_buffers);
                frame_count++;
            }

            release_preview_stream_mutex(camerasrc->preview_stream);
            GST_DEBUG_OBJECT(camerasrc, "mutex released in thread %p",g_thread_self ());
            if (_MAX_TRIAL_WAIT_FRAME == frame_count) {
                GST_ERROR_OBJECT(camerasrc, "Could not get preview frame more than 3 sec. - num live buffers %d",
                                            camerasrc->num_live_buffers);
                if (camerasrc->firsttime) {
                    gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_IO_CONTROL);
                } else {
                    gst_qcamerasrc_error_handler(camerasrc, CAMERASRC_ERR_UNAVAILABLE_DEVICE);
                }
                ret = GST_FLOW_ERROR;
                frame_count = 0;
            }
        break;
        case VIDEO_IN_MODE_CAPTURE:
        {
            g_mutex_lock(camerasrc->restart_camera_lock);
            GST_WARNING_OBJECT(camerasrc, "wait for signal");
            g_cond_wait(camerasrc->restart_camera_cond, camerasrc->restart_camera_lock);
            GST_WARNING_OBJECT(camerasrc, "signal received");
            g_mutex_unlock(camerasrc->restart_camera_lock);

            if(!start_preview(camerasrc->preview_stream)){
                GST_WARNING_OBJECT( camerasrc, "start_preview failed");
                ret = GST_FLOW_ERROR;
            }
            camerasrc->firsttime = TRUE;
            camerasrc->mode = VIDEO_IN_MODE_PREVIEW;
        }
        break;
        case VIDEO_IN_MODE_UNKNOWN:
            if(!gst_qcamerasrc_is_zero_copy_format(camerasrc->format_str)){
                black_buf_len = camerasrc->preview_stream->preview_width * camerasrc->preview_stream->preview_height * 3 /2;
                black_frame_buffer = gst_buffer_new_and_alloc(black_buf_len);
                GstMapInfo map;
                gst_buffer_map(black_frame_buffer,&map,GST_MAP_WRITE);
                GenerateYUV420BlackFrame(map.data,
                                        black_buf_len,
                                        camerasrc->preview_stream->preview_width ,
                                        camerasrc->preview_stream->preview_height);
                gst_buffer_unmap(black_frame_buffer,&map);

                *buffer = black_frame_buffer;
            }
        break;
        default:
            ret = GST_FLOW_ERROR;
            GST_ERROR_OBJECT (camerasrc, "can't reach statement.[camerasrc->mode=%d]", camerasrc->mode);
        break;
    }

    if (!buffer || !(*buffer) || !GST_IS_BUFFER(*buffer)) {
        /* To avoid seg fault, make dummy buffer. */
        GST_WARNING_OBJECT(camerasrc, "Make a dummy buffer");
        if(gst_qcamerasrc_is_zero_copy_format(camerasrc->format_str)){
            *buffer = gst_buffer_new();
            GstMapInfo map;
            gst_buffer_map(*buffer,&map,GST_MAP_WRITE);
            map.data = NULL;
            map.size = 0;
            gst_buffer_unmap(*buffer,&map);
        }else{
            /*VT call uses non-zero copy format(NV21) data from camera.
            Not all gst-plugins used in VT pipeline handle NULL GST buffer well*/
            black_buf_len = camerasrc->preview_stream->preview_width * camerasrc->preview_stream->preview_height * 3 /2;
            *buffer = gst_buffer_new_and_alloc(black_buf_len);
            GstMapInfo map;
            gst_buffer_map(*buffer,&map,GST_MAP_WRITE);
            GenerateYUV420BlackFrame(map.data,
                                                black_buf_len,
                                                camerasrc->preview_stream->preview_width ,
                                                camerasrc->preview_stream->preview_height);

            map.size = black_buf_len;
            gst_buffer_unmap(*buffer,&map);
        }
    }

    gst_qcamerasrc_get_timeinfo(camerasrc, (GstBuffer*)*buffer);

    return ret;
}
/*********************************GST functions***********************************/

static void
gst_qcamerasrc_set_property (GObject * object, guint prop_id, const GValue * value, GParamSpec * pspec)
{
    GstCameraSrc *camerasrc;
    GstState current_state = GST_STATE_NULL;

    g_return_if_fail (GST_IS_CAMERASRC (object));
    camerasrc = GST_CAMERASRC (object);

    switch (prop_id) {
        case PROP_CAMERA_AUTO_FPS:
            camerasrc->fps_auto = g_value_get_boolean (value);
        break;
        case PROP_CAMERA_ID:
            camerasrc->camera_id  = g_value_get_int(value);
#ifndef CONFIG_MACH_Z3LTE //This is wearable feature because wearble device use secondary.
            camerasrc->camera_id = camerasrc->camera_id ? CAMERASRC_DEV_ID_PRIMARY:CAMERASRC_DEV_ID_SECONDARY;
#endif
        break;
        case PROP_CAMERA_CAPTURE_FOURCC:
            camerasrc->cap_fourcc = g_value_get_uint (value);
        break;
        case PROP_CAMERA_CAPTURE_WIDTH:
            camerasrc->prev_cap_width = camerasrc->cap_width;
            camerasrc->cap_width = g_value_get_int (value);
        break;
        case PROP_CAMERA_CAPTURE_HEIGHT:
             if(camerasrc->cap_height == g_value_get_int(value) && camerasrc->cap_width == camerasrc->prev_cap_width)
             {
                 GST_WARNING_OBJECT(camerasrc, "height&width same as previous , need not restart preview,");
                 break;
             }
            camerasrc->cap_height = g_value_get_int (value);
#ifdef DEFAULT_ZSL_MODE_ON
            gst_element_get_state(GST_ELEMENT(camerasrc), &current_state, NULL, 0);
            GST_INFO("current_state %d", current_state);
            if (current_state == GST_STATE_PLAYING) {
                g_mutex_lock(camerasrc->cmd_lock);
                GST_WARNING_OBJECT(camerasrc, "push PREVIEW_RESTART cmd by Capture Height");
                camerasrc->snapshot_stream->restart_flag = TRUE;
                g_cond_wait(camerasrc->cmd_cond, camerasrc->cmd_lock);
                GST_WARNING_OBJECT(camerasrc, "cmd_cond was recieved and restart complete from Capture Height");
                g_mutex_unlock(camerasrc->cmd_lock);
            }
#endif
        break;
        case PROP_CAMERA_CAPTURE_JPG_QUALITY:
            camerasrc->cap_jpg_quality = g_value_get_int (value);
            gst_element_get_state(GST_ELEMENT(camerasrc), &current_state, NULL, 0);
            GST_INFO("current_state %d", current_state);
            if (current_state == GST_STATE_PLAYING && camerasrc->snapshot_stream
                    && camerasrc->snapshot_stream->qcam_handle) {
                gst_qcamerasrc_set_camera_control(camerasrc,
                        QCAMERASRC_PARAM_CONTROL_JPEG_QUALITY,
                        camerasrc->cap_jpg_quality,0,NULL);
            }
        break;
        case PROP_CAMERA_CAPTURE_PROVIDE_EXIF:
            GST_WARNING_OBJECT(camerasrc, "can not set provide-exif");
        break;
        case PROP_CAMERA_VIDEO_WIDTH:
            camerasrc->video_width = g_value_get_int (value);
            GST_WARNING_OBJECT(camerasrc, "video width %d", camerasrc->video_width);
        break;
        case PROP_CAMERA_VIDEO_HEIGHT:
            camerasrc->video_height = g_value_get_int (value);
            GST_WARNING_OBJECT(camerasrc, "video height %d", camerasrc->video_height);
            if (camerasrc->record_stream) {
                camerasrc->record_stream->record_width = camerasrc->video_width;
                camerasrc->record_stream->record_height = camerasrc->video_height;
            }
        break;
        case PROP_CAMERA_ENABLE_HYBRID_MODE:
            camerasrc->enable_hybrid_mode = g_value_get_boolean(value);
        break;
        case PROP_VFLIP:
            camerasrc->vflip = g_value_get_boolean ( value );
        break;
        case PROP_HFLIP:
            camerasrc->hflip = g_value_get_boolean ( value );
        break;
        case PROP_SENSOR_ORIENTATION:
            GST_WARNING_OBJECT(camerasrc, "can not set sensor orientation");
        break;
        case PROP_CAMERA_CPU_LOCK_MODE:
            camerasrc->camera_cpu_clock_lock = g_value_get_boolean ( value );
            GST_INFO_OBJECT(camerasrc, "clock lock (%d)", camerasrc->camera_cpu_clock_lock);
        break;
        case PROP_VT_MODE:
            if(camerasrc->vt_mode != g_value_get_int (value))
            {
                camerasrc->vt_mode = g_value_get_int (value);
                GST_INFO_OBJECT(camerasrc, "vt_mode (%d)", camerasrc->vt_mode);

                //need to restrat preview for applying vt mode
                gst_element_get_state(GST_ELEMENT(camerasrc), &current_state, NULL, 0);
                GST_INFO("current_state %d", current_state);
                if (current_state == GST_STATE_PLAYING) {
                    g_mutex_lock(camerasrc->cmd_lock);
                    GST_WARNING_OBJECT(camerasrc, "PREVIEW_RESTART with restart_flag by vt_mode");
                    camerasrc->snapshot_stream->restart_flag = TRUE;
                    g_cond_wait(camerasrc->cmd_cond, camerasrc->cmd_lock);
                    GST_WARNING_OBJECT(camerasrc, "cmd_cond was recieved and restart complete from vt_mode");
                    g_mutex_unlock(camerasrc->cmd_lock);
                }
            }
        break;
        case PROP_STOP_CAMERA:
            g_mutex_lock(camerasrc->stop_camera_lock);
            camerasrc->stop_camera = g_value_get_int (value);
            if (camerasrc->stop_camera) {
                GST_WARNING_OBJECT(camerasrc, "wait for signal");
                g_cond_wait(camerasrc->stop_camera_cond, camerasrc->stop_camera_lock);
                GST_WARNING_OBJECT(camerasrc, "signal received");
            }
            g_mutex_unlock(camerasrc->stop_camera_lock);
        break;
        case PROP_OPEN_CAMERA:
            GST_WARNING_OBJECT(camerasrc, "open camera");
            g_mutex_lock(camerasrc->cmd_lock);
            if (camerasrc->ready_state == FALSE) {
                camerasrc->ready_state = TRUE;
                GST_INFO("    gst_qcamerasrc_create");
                if (!gst_qcamerasrc_create(camerasrc)){
                    camerasrc->ready_state = FALSE;
                    GST_ERROR_OBJECT(camerasrc, "gst_qcamerasrc_create failed");
                }
            } else {
                GST_WARNING_OBJECT(camerasrc, "ready_state is true, Already gst_qcamerasrc_create is done");
            }
            g_mutex_unlock(camerasrc->cmd_lock);
        break;
#ifdef CONFIG_MACH_Z3LTE
        case PROP_CAMERA_HIGH_SPEED_FPS:
            camerasrc->high_speed_fps = g_value_get_int (value);
            if (camerasrc->record_stream) {
                camerasrc->record_stream->hfr_mode = camerasrc->high_speed_fps;
            }
        break;
        case PROP_CAMERA_CAPTURE_QUALITY:
            camerasrc->cap_quality = g_value_get_enum (value);
        break;
        case PROP_CAMERA_CAPTURE_INTERVAL:
            camerasrc->cap_interval = g_value_get_int (value);
        break;
        case PROP_CAMERA_CAPTURE_COUNT:
            camerasrc->cap_count = g_value_get_int (value);
        break;
        case PROP_CAMERA_CAPTURE_HDR:
            camerasrc->cap_hdr = g_value_get_int(value);
            if (camerasrc->cap_hdr == CAMERASRC_HDR_CAPTURE_OFF) {
                camerasrc->snapshot_stream->m_hdr_mode = CAM_EXP_BRACKETING_OFF;
                camerasrc->snapshot_stream->m_bracket_buffer_count = 0;
            } else {
                camerasrc->snapshot_stream->m_hdr_mode = CAM_EXP_BRACKETING_ON;
            }
            // different preset with postset : set restart preview
            if (camerasrc->snapshot_stream->m_pre_hdr_mode != camerasrc->snapshot_stream->m_hdr_mode){
                gst_element_get_state(GST_ELEMENT(camerasrc), &current_state, NULL, 0);
                GST_INFO("current_state %d", current_state);
                if (current_state == GST_STATE_PLAYING) {
                   g_mutex_lock(camerasrc->cmd_lock);
                   GST_WARNING_OBJECT(camerasrc, "push GST_QCAMERASRC_CMD_PROCESS_HDR cmd");
                   camerasrc->snapshot_stream->restart_flag = TRUE;
                   g_cond_wait(camerasrc->cmd_cond, camerasrc->cmd_lock);
                   GST_WARNING_OBJECT(camerasrc, "cmd_cond was recieved and restart complete from HDR");
                   g_mutex_unlock(camerasrc->cmd_lock);
                }
            }
        break;
        case PROP_CAMERA_ENABLE_ZSL_MODE:
            camerasrc->enable_zsl_mode = g_value_get_boolean (value);
        break;
#else
        case PROP_CAMERA_ENABLE_VDIS:
        {
            gboolean current_vdis_mode = FALSE;
            current_vdis_mode = camerasrc->enable_vdis_mode;

            /* set new one */
            camerasrc->enable_vdis_mode = g_value_get_boolean(value);
            if(camerasrc->record_stream)
                camerasrc->record_stream->enable_vdis_mode = camerasrc->enable_vdis_mode;
            GST_INFO("Set VDIS mode current %d, new %d",
                    current_vdis_mode, camerasrc->enable_vdis_mode);
        }
        break;
#endif
#ifdef CONFIG_MACH_Z3LTE
        case PROP_CAMERA_LOW_LIGHT_AUTO_DETECTION:
            camerasrc->low_light_detection = g_value_get_int (value);
            GST_WARNING_OBJECT(camerasrc, "low_light_detection (%d)", camerasrc->low_light_detection);
            set_low_light_auto_detection((camerasrc_handle_t *)camerasrc->qcam_handle, camerasrc->low_light_detection);
        break;
        case PROP_SIGNAL_STILLCAPTURE:
            camerasrc->signal_still_capture = g_value_get_boolean (value);
            GST_DEBUG("signal_still_capture %d", camerasrc->signal_still_capture);
            if (camerasrc->signal_still_capture) {
                capture_snapshot (camerasrc);
            }
        break;
#else
        case PROP_DUAL_CAMERA_MODE:
            camerasrc->dual_camera_mode = g_value_get_boolean ( value );
            GST_INFO_OBJECT(camerasrc, "dual camera mode (%d)", camerasrc->dual_camera_mode);
        break;
#endif
        case PROP_CAMERA_ENABLE_MEDIAPACKET_RECORD_CB:
            camerasrc->mediapacket_record_mode = g_value_get_boolean ( value );
            GST_INFO_OBJECT(camerasrc, "Media Packet Record CB (%d)", camerasrc->mediapacket_record_mode);
            if(camerasrc->mediapacket_record_mode) {
                /* set video stream size */
                camerasrc->record_stream->record_width = camerasrc->video_width = camerasrc->width;
                camerasrc->record_stream->record_height = camerasrc->video_height = camerasrc->height;
#ifdef CONFIG_MACH_Z3LTE
                camerasrc->record_stream->hfr_mode = camerasrc->high_speed_fps;
#endif
                /* if preview restart is needed,
                   this should be called in read_frame function */
                g_mutex_lock(camerasrc->cmd_lock);
                GST_WARNING_OBJECT(camerasrc, "send record start signal GST_QCAMERASRC_CMD_RECORD_START");
                g_queue_push_tail(camerasrc->cmd_list, (gpointer)GST_QCAMERASRC_CMD_RECORD_START);
                g_mutex_unlock(camerasrc->cmd_lock);
            }
            else {
                /* if preview restart is needed,
                   this should be called in read_frame function */
                g_mutex_lock(camerasrc->cmd_lock);
                GST_WARNING_OBJECT(camerasrc, "send record stop process command GST_QCAMERASRC_CMD_RECORD_STOP");
                g_queue_push_tail(camerasrc->cmd_list, (gpointer)GST_QCAMERASRC_CMD_RECORD_STOP);
                g_cond_wait(camerasrc->cmd_cond, camerasrc->cmd_lock);
                g_mutex_unlock(camerasrc->cmd_lock);
             }
        break;
        case PROP_CAMERA_SHOT_MODE:
            camerasrc->shot_mode = g_value_get_int (value);
            GST_WARNING_OBJECT(camerasrc, "shot_mode (%d)", camerasrc->shot_mode);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gst_qcamerasrc_get_property (GObject * object, guint prop_id, GValue * value, GParamSpec * pspec)
{
    GstCameraSrc *camerasrc;

    g_return_if_fail (GST_IS_CAMERASRC (object));
    camerasrc = GST_CAMERASRC (object);

    switch (prop_id) {
        case PROP_CAMERA_AUTO_FPS:
            g_value_set_boolean (value, camerasrc->fps_auto);
        break;
        case PROP_CAMERA_ID:
#ifndef CONFIG_MACH_Z3LTE //This is wearable feature because wearble device use secondary.
            g_value_set_int(value, camerasrc->camera_id ? CAMERASRC_DEV_ID_PRIMARY:CAMERASRC_DEV_ID_SECONDARY);
#else
            g_value_set_int(value, camerasrc->camera_id);
#endif
        break;
        case PROP_CAMERA_CAPTURE_FOURCC:
            g_value_set_uint (value, camerasrc->cap_fourcc);
        break;
        case PROP_CAMERA_CAPTURE_WIDTH:
            g_value_set_int (value, camerasrc->cap_width);
        break;
        case PROP_CAMERA_CAPTURE_HEIGHT:
            g_value_set_int (value, camerasrc->cap_height);
        break;
        case PROP_CAMERA_CAPTURE_JPG_QUALITY:
            g_value_set_int (value, camerasrc->cap_jpg_quality);
        break;
        case PROP_CAMERA_CAPTURE_PROVIDE_EXIF:
            g_value_set_boolean (value, camerasrc->cap_provide_exif);
        break;
        case PROP_CAMERA_VIDEO_WIDTH:
            g_value_set_int (value, camerasrc->video_width);
        break;
        case PROP_CAMERA_VIDEO_HEIGHT:
            g_value_set_int (value, camerasrc->video_height);
        break;
        case PROP_CAMERA_ENABLE_HYBRID_MODE:
            g_value_set_boolean(value, camerasrc->enable_hybrid_mode);
        break;
        case PROP_VFLIP:
            g_value_set_boolean(value, camerasrc->vflip);
        break;
        case PROP_HFLIP:
            g_value_set_boolean(value, camerasrc->hflip);
        break;
        case PROP_SENSOR_ORIENTATION:
            g_value_set_int(value, camerasrc->sensor_orientation);
        break;
        case PROP_CAMERA_CPU_LOCK_MODE:
            g_value_set_boolean(value, camerasrc->camera_cpu_clock_lock);
        break;
        case PROP_VT_MODE:
            g_value_set_int(value, camerasrc->vt_mode);
        break;
        case PROP_STOP_CAMERA:
            g_value_set_int(value, camerasrc->stop_camera);
        break;
        case PROP_OPEN_CAMERA:
            /* do nothing */
        break;
#ifdef CONFIG_MACH_Z3LTE
        case PROP_CAMERA_HIGH_SPEED_FPS:
            g_value_set_int (value, camerasrc->high_speed_fps);
        break;
        case PROP_CAMERA_CAPTURE_QUALITY:
            g_value_set_enum (value, camerasrc->cap_quality);
        break;
        case PROP_CAMERA_CAPTURE_INTERVAL:
            g_value_set_int (value, camerasrc->cap_interval);
        break;
        case PROP_CAMERA_CAPTURE_COUNT:
            g_value_set_int (value, camerasrc->cap_count);
        break;
        case PROP_CAMERA_CAPTURE_HDR:
            g_value_set_int(value, camerasrc->cap_hdr);
        break;
        case PROP_CAMERA_ENABLE_ZSL_MODE:
            g_value_set_boolean (value, camerasrc->enable_zsl_mode);
        break;
#else
        case PROP_CAMERA_ENABLE_VDIS:
            g_value_set_boolean(value, camerasrc->enable_vdis_mode);
        break;
#endif
#ifdef CONFIG_MACH_Z3LTE
        case PROP_CAMERA_LOW_LIGHT_AUTO_DETECTION:
            g_value_set_int(value, camerasrc->low_light_detection);
        break;
#else
        case PROP_DUAL_CAMERA_MODE:
            g_value_set_boolean(value, camerasrc->dual_camera_mode);
        break;
#endif
#ifdef CONFIG_MACH_Z3LTE
        case PROP_SIGNAL_STILLCAPTURE:
            g_value_set_boolean (value, camerasrc->signal_still_capture);
        break;
        case PROP_CAMERA_ENABLE_MEDIAPACKET_RECORD_CB:
            g_value_set_boolean(value, camerasrc->mediapacket_record_mode);
        break;
#endif
	case PROP_CAMERA_SHOT_MODE:
            g_value_set_int(value, camerasrc->shot_mode);
        break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
gst_qcamerasrc_finalize (GObject * object)
{
    GstCameraSrc *camerasrc = GST_CAMERASRC (object);
    GST_INFO_OBJECT(camerasrc, "ENTERED");
    free_preview(camerasrc->preview_stream);
    free_snapshot(camerasrc->snapshot_stream);
    free_record(camerasrc->record_stream);

    if( camerasrc->command_list != NULL ){
        g_queue_free( camerasrc->command_list );
        camerasrc->command_list = NULL;
    }
    g_mutex_clear (&camerasrc->commandq_mutex);
    g_cond_clear (&camerasrc->commandq_cond);

    SAFE_FREE_GCOND(camerasrc->preview_buffer_cond);
    SAFE_FREE_GMUTEX(camerasrc->preview_buffer_lock);
    SAFE_FREE_GCOND(camerasrc->video_buffer_cond);
    SAFE_FREE_GMUTEX(camerasrc->video_buffer_lock);
    SAFE_FREE_GCOND(camerasrc->stop_camera_cond);
    SAFE_FREE_GMUTEX(camerasrc->stop_camera_lock);
    SAFE_FREE_GQUEUE(camerasrc->cmd_list);
    SAFE_FREE_GCOND(camerasrc->cmd_cond);
    SAFE_FREE_GMUTEX(camerasrc->cmd_lock);
    SAFE_FREE_GCOND(camerasrc->restart_camera_cond);
    SAFE_FREE_GMUTEX(camerasrc->restart_camera_lock);

    if(camerasrc->batch_control_value_cache){
        if(camerasrc->batch_control_value_cache->af_area){
            free(camerasrc->batch_control_value_cache->af_area);
            camerasrc->batch_control_value_cache->af_area=NULL;
        }
        free(camerasrc->batch_control_value_cache);
        camerasrc->batch_control_value_cache=NULL;
    }

    if(camerasrc->img_buffer_cache){
        free(camerasrc->img_buffer_cache);
        camerasrc->img_buffer_cache = NULL;
    }

    free_camera_handle((camerasrc_handle_t**)&camerasrc->qcam_handle);
    GST_INFO_OBJECT(camerasrc, "LEAVED");

    if (G_OBJECT_CLASS (gst_qcamerasrc_parent_class)->finalize)
        G_OBJECT_CLASS(gst_qcamerasrc_parent_class)->finalize(object);
}

static GstStateChangeReturn
gst_qcamerasrc_change_state (GstElement *element, GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    GstCameraSrc *camerasrc;
    camerasrc = GST_CAMERASRC (element);

    switch (transition) {
        case GST_STATE_CHANGE_NULL_TO_READY:
            GST_WARNING_OBJECT(camerasrc, "GST CAMERA SRC: NULL -> READY");
            g_mutex_lock(camerasrc->cmd_lock);
            if (camerasrc->ready_state == FALSE) {
                GST_INFO("    gst_qcamerasrc_create");
                if (!gst_qcamerasrc_create(camerasrc)){
                    camerasrc->ready_state = FALSE;
                    GST_ERROR_OBJECT(camerasrc, "gst_qcamerasrc_create failed");
                    g_mutex_unlock(camerasrc->cmd_lock);
                    goto statechange_failed;
                }
                camerasrc->ready_state = TRUE;

            } else {
                GST_WARNING_OBJECT(camerasrc, "ready_state is true, Already gst_qcamerasrc_create is done");
            }
            g_mutex_unlock(camerasrc->cmd_lock);
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            GST_WARNING_OBJECT(camerasrc, "GST CAMERA SRC: READY -> PAUSED");
            ret = GST_STATE_CHANGE_NO_PREROLL;
        break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            GST_WARNING_OBJECT(camerasrc, "GST CAMERA SRC: PAUSED -> PLAYING");
            break;
        default:
            break;
    }

    ret = GST_ELEMENT_CLASS(gst_qcamerasrc_parent_class)->change_state (element, transition);
    if (ret == GST_STATE_CHANGE_FAILURE){
        return ret;
    }

    switch (transition){
        case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            GST_WARNING_OBJECT(camerasrc, "GST CAMERA SRC: PLAYING -> PAUSED");
            ret = GST_STATE_CHANGE_NO_PREROLL;
            break;
        case GST_STATE_CHANGE_PAUSED_TO_READY:
            GST_WARNING_OBJECT(camerasrc, "GST CAMERA SRC: PAUSED -> READY");
            break;
        case GST_STATE_CHANGE_READY_TO_NULL:
            GST_WARNING_OBJECT(camerasrc, "GST CAMERA SRC: READY -> NULL");
            GST_INFO("    gst_qcamerasrc_destroy");
            if (!gst_qcamerasrc_destroy(camerasrc)){
                goto statechange_failed;
            }
            break;
        default:
            break;
    }

    return ret;

statechange_failed:
    /* subclass must post a meaningfull error message */
    GST_ERROR_OBJECT (camerasrc, "state change failed");
    return GST_STATE_CHANGE_FAILURE;
}

int
gst_qcamerasrc_set_camera_control(GstCameraSrc* camerasrc,int64_t control_type,int value_1,int value_2,char* string_1){
    GstState current_state = GST_STATE_NULL;
    int ret=CAMERASRC_SUCCESS;
    if(!camerasrc){
        GST_ERROR ("null handle encountered...");
        return CAMERASRC_ERROR;
    }
    gst_element_get_state(GST_ELEMENT(camerasrc), &current_state, NULL, 0);
    if (current_state == GST_STATE_PLAYING) {
        GST_INFO_OBJECT(camerasrc, "direct setting");
        ret = set_camera_control(camerasrc->qcam_handle,control_type,value_1,value_2,string_1);
    }else{
        GST_INFO_OBJECT(camerasrc, "cache setting");
        ret = cache_settings_for_batch_update(camerasrc,control_type,value_1,value_2,string_1);
    }
    return ret;
}

void
gst_qcamerasrc_set_capture_command(GstCameraSrc *camerasrc, GstCameraControlCaptureCommand cmd){

    GST_INFO_OBJECT(camerasrc, "ENTERED");
    g_mutex_lock(&camerasrc->commandq_mutex);

    switch (cmd){
        case GST_CAMERA_CONTROL_CAPTURE_COMMAND_START:
#ifdef DEFAULT_ZSL_MODE_ON
            TIMER_START(capture_timer);
            capture_snapshot (camerasrc);
#else
            TIMER_START(capture_timer)
            camerasrc->received_capture_command = TRUE;
            signal_flush_frame(camerasrc->preview_stream);
#endif
            break;
        case GST_CAMERA_CONTROL_CAPTURE_COMMAND_STOP:
            stop_snapshot(camerasrc->snapshot_stream);
            camerasrc->is_capturing = FALSE;
            break;
        default:
            break;
    }

    g_mutex_unlock(&camerasrc->commandq_mutex);

    GST_LOG_OBJECT (camerasrc, "LEAVED");

    return;
}

void
gst_qcamerasrc_set_record_command(GstCameraSrc *camerasrc, GstCameraControlRecordCommand cmd)
{
    GST_WARNING_OBJECT(camerasrc, "ENTERED - %d", cmd);

    if(camerasrc->mediapacket_record_mode){
        GST_WARNING_OBJECT(camerasrc, "Already recording is running");
        if(cmd == GST_CAMERA_CONTROL_RECORD_COMMAND_START)
            camerasrc->local_recording = TRUE;
        else if(cmd == GST_CAMERA_CONTROL_RECORD_COMMAND_STOP)
            camerasrc->local_recording = FALSE;
        return;
    }
    if (cmd == GST_CAMERA_CONTROL_RECORD_COMMAND_START) {
        /* set video stream size */
        camerasrc->record_stream->record_width = camerasrc->video_width;
        camerasrc->record_stream->record_height = camerasrc->video_height;
#ifdef CONFIG_MACH_Z3LTE
        camerasrc->record_stream->hfr_mode = camerasrc->high_speed_fps;
#endif
        camerasrc->local_recording = TRUE;
        if (camerasrc->enable_hybrid_mode) {
            /* record start will be done in seperated thread */
            g_mutex_lock(&camerasrc->rec_handle->thread_lock);
            GST_WARNING_OBJECT(camerasrc, "send record start signal");
            g_cond_signal(&camerasrc->rec_handle->thread_cond);
            g_mutex_unlock(&camerasrc->rec_handle->thread_lock);
        } else {
            /* if preview restart is needed,
               this should be called in read_frame function */
            g_mutex_lock(camerasrc->cmd_lock);
            GST_WARNING_OBJECT(camerasrc, "PREVIEW_RESTART with restart_flag by record start");
            camerasrc->snapshot_stream->restart_flag = GST_QCAMERASRC_CMD_RECORD_START;
            g_queue_push_tail(camerasrc->cmd_list, (gpointer)GST_QCAMERASRC_CMD_RECORD_START);
            g_mutex_unlock(camerasrc->cmd_lock);
        }
    } else if (cmd == GST_CAMERA_CONTROL_RECORD_COMMAND_STOP) {
        camerasrc->local_recording = FALSE;
        if (camerasrc->enable_hybrid_mode) {
            /* call record command here,
               because preview delay could be observed
               if this is called in read_frame function */
            gst_qcamerasrc_process_cmd(camerasrc, GST_QCAMERASRC_CMD_RECORD_STOP);
        } else {
            /* if preview restart is needed,
               this should be called in read_frame function */
            g_mutex_lock(camerasrc->cmd_lock);
            g_queue_push_tail(camerasrc->cmd_list, (gpointer)GST_QCAMERASRC_CMD_RECORD_STOP);
            g_cond_wait(camerasrc->cmd_cond, camerasrc->cmd_lock);
            g_mutex_unlock(camerasrc->cmd_lock);
        }
    } else {
        GST_ERROR_OBJECT(camerasrc, "unknown record cmd - %d", cmd);
    }

    GST_WARNING_OBJECT(camerasrc, "LEAVED");

    return;
}

static gboolean
gst_qcamerasrc_src_start (GstBaseSrc * src)
{
    GstCameraSrc *camerasrc = GST_CAMERASRC (src);
    int ret = TRUE;

    GST_DEBUG_OBJECT (camerasrc, "ENTERED");

    //TODo:gst_camerasrc_set_caps' will call gst_camerasrc_start(). So skip to call it.
    //ret = gst_camerasrc_start (camerasrc);

    GST_DEBUG_OBJECT (camerasrc, "LEAVED");

    return ret;
}


static gboolean
gst_qcamerasrc_src_stop (GstBaseSrc * src)
{
    int ret = 0;
    GstCameraSrc *camerasrc = GST_CAMERASRC (src);

    GST_WARNING_OBJECT (camerasrc, "ENTERED");

    ret = gst_qcamerasrc_stop(camerasrc);


    GST_WARNING_OBJECT (camerasrc, "LEAVED - ret %d", ret);

    return TRUE;
}

static GstCaps *
gst_qcamerasrc_get_caps (GstBaseSrc * src, GstCaps * filter)
{
    GstCameraSrc *camerasrc = GST_CAMERASRC (src);

    GstCaps *ret=NULL;

    GST_DEBUG_OBJECT (camerasrc, "ENTERED");
    if (camerasrc->mode == VIDEO_IN_MODE_UNKNOWN) {
        GST_INFO_OBJECT (camerasrc, "Unknown mode. Just return template caps.");
        ret= gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD(camerasrc)));
        return filter ? gst_caps_intersect(ret, filter) : gst_caps_copy(ret);
    }else{
        /*FIXME: Using "VIDIOC_ENUM_FMT".*/
        ret = gst_caps_copy (gst_pad_get_pad_template_caps (GST_BASE_SRC_PAD(camerasrc)));
    }
    GST_DEBUG ("Probed caps: %" GST_PTR_FORMAT, ret);
    GST_DEBUG_OBJECT (camerasrc, "LEAVED");
    return ret;
}

static gboolean
gst_qcamerasrc_set_caps (GstBaseSrc * src, GstCaps * caps)
{
    gboolean ret = TRUE;
    GstCameraSrc *camerasrc;
    camerasrc = GST_CAMERASRC (src);

    GST_DEBUG_OBJECT (camerasrc, "ENTERED");

    /*if ((camerasrc->mode == VIDEO_IN_MODE_PREVIEW)||
    (camerasrc->mode == VIDEO_IN_MODE_VIDEO)){
        GST_INFO_OBJECT (camerasrc, "Proceed set_caps.");
        __ta__("            gst_qcamerasrc_stop",
            if (!gst_qcamerasrc_stop(camerasrc))
            {
            GST_INFO_OBJECT (camerasrc, "Cam sensor stop failed.");
            }
        )
    }else if (camerasrc->mode == VIDEO_IN_MODE_CAPTURE){
        GST_ERROR_OBJECT (camerasrc, "A mode of avsystem camera is capture. Not to proceed set_caps.");
        GST_DEBUG_OBJECT (camerasrc, "LEAVED");
        return FALSE;
    }else{
        //VIDEO_IN_MODE_UNKNOWN
        GST_INFO_OBJECT (camerasrc, "A mode of avsystem camera is unknown. Proceed set_caps.");
    }*/

    if (!gst_qcamerasrc_get_caps_info (camerasrc, caps)) {
        GST_INFO_OBJECT (camerasrc, "can't get capture information from caps %x", caps);
        GST_DEBUG_OBJECT (camerasrc, "LEAVED");
        return FALSE;
    }

    g_mutex_lock(camerasrc->cmd_lock);
    if (camerasrc->ready_state == FALSE) {
        GST_INFO("    gst_qcamerasrc_create");
        if (!gst_qcamerasrc_create(camerasrc)){
            camerasrc->ready_state = FALSE;
            GST_ERROR_OBJECT(camerasrc, "gst_qcamerasrc_create failed");
            g_mutex_unlock(camerasrc->cmd_lock);
            return FALSE;
        }
        camerasrc->ready_state = TRUE;
    } else {
        GST_WARNING_OBJECT(camerasrc, "ready_state is true, Already gst_qcamerasrc_create is done");
    }
    g_mutex_unlock(camerasrc->cmd_lock);

    if (camerasrc->mode != VIDEO_IN_MODE_PREVIEW) {
        GST_INFO("            gst_qcamerasrc_start");
        ret = gst_qcamerasrc_start(camerasrc);
        if (!ret) {
            GST_ERROR_OBJECT (camerasrc,  "Cam sensor start failed.");
        }
    }
    ret = gst_pad_push_event (GST_BASE_SRC_PAD (src), gst_event_new_caps (caps));

    GST_WARNING_OBJECT(camerasrc, "LEAVED - ret %d", ret);

    return ret;
}

static GstFlowReturn
gst_qcamerasrc_src_create (GstPushSrc * src, GstBuffer ** buffer) {
    GstCameraSrc *camerasrc = GST_CAMERASRC (src);
    GstFlowReturn ret=GST_FLOW_OK;
    //GST_DEBUG_OBJECT (camerasrc, "ENTERED");

    ret=gst_qcamerasrc_read_frame(camerasrc,buffer);

    //GST_DEBUG_OBJECT (camerasrc, "LEAVED");
    return ret;
}

/*********************************BUFFER functions***********************************/

G_DEFINE_BOXED_TYPE(GstQCameraBuffer, gst_qcamerasrc_buffer, NULL, gst_qcamerasrc_buffer_finalize);

static void
gst_qcamerasrc_buffer_class_init(gpointer g_class, gpointer class_data)
{
}

static GstQCameraBuffer*
gst_qcamerasrc_buffer_new(GstCameraSrc *camerasrc)
{
    GstQCameraBuffer *ret = NULL;
    ret = (GstQCameraBuffer *)malloc(sizeof(*ret));
    ret->buffer = gst_buffer_new();
    ret->camerasrc = gst_object_ref(GST_OBJECT(camerasrc));
    GST_LOG_OBJECT(camerasrc, "creating buffer : %p", ret);
    return ret;
}

static void
gst_qcamerasrc_buffer_finalize(GstQCameraBuffer *buffer)
{
    GstCameraSrc *camerasrc = buffer->camerasrc;
    MMVideoBuffer *imgb = NULL;
    GST_DEBUG_OBJECT(camerasrc,"ENTER - buffer type %d", buffer->buffer_type);

    /*Free memory*/
    if (gst_qcamerasrc_is_zero_copy_format(camerasrc->format_str)) {
        if (gst_buffer_n_memory(buffer->buffer) > 1){
            GstMemory *img_buf_memory = NULL;
            GstMapInfo mapInfo;
            img_buf_memory = gst_buffer_peek_memory(buffer->buffer,1);
            gst_memory_map(img_buf_memory,&mapInfo,GST_MAP_READ);
            imgb = (MMVideoBuffer *)mapInfo.data;
            if (imgb->jpeg_data) {
                GST_DEBUG_OBJECT(camerasrc, "release JPEG data %p", imgb->jpeg_data);
                if (((capture_buffer_info_t *)imgb->jpeg_data)->data) {
                    free(((capture_buffer_info_t *)imgb->jpeg_data)->data);
                    ((capture_buffer_info_t *)imgb->jpeg_data)->data = NULL;
                }
                free(imgb->jpeg_data);
                imgb->jpeg_data = NULL;
            }
            if (imgb->flush_request== TRUE) {
                GST_WARNING_OBJECT(camerasrc, "FLUSH_BUFFER finalize");
            }
            gst_memory_unmap(img_buf_memory,&mapInfo);
        }
    }

    /* Buffer Q again */
    if (PREVIEW_BUFFER == buffer->buffer_type) {
        /* release preview buffer */
        done_preview_frame(camerasrc->preview_stream, buffer->buffer_metadata);

        /* decrease live buffer count and send signal */
        g_mutex_lock(camerasrc->preview_buffer_lock);
        GST_DEBUG_OBJECT(camerasrc, "release preview buffer - num_live_buffers %d -> %d",
                                    camerasrc->num_live_buffers, camerasrc->num_live_buffers - 1);
        camerasrc->num_live_buffers--;

        if (camerasrc->num_live_buffers < 0)
            GST_WARNING("Preview frame buffers count is negative. (Live=%d)", camerasrc->num_live_buffers);

        g_cond_signal(camerasrc->preview_buffer_cond);
        g_mutex_unlock(camerasrc->preview_buffer_lock);
    } else if (RECORD_BUFFER == buffer->buffer_type) {
        /* release video buffer */
        done_record_frame(camerasrc->record_stream, buffer->buffer_metadata);

        /* decrease live buffer count and send signal */
        g_mutex_lock(camerasrc->video_buffer_lock);
        GST_DEBUG_OBJECT(camerasrc, "release video buffer - live %d -> %d",
                                    camerasrc->num_video_buffers, camerasrc->num_video_buffers - 1);
        camerasrc->num_video_buffers--;

        if (camerasrc->num_video_buffers < 0)
            GST_WARNING("Recorder frame buffers count is negative. (Live=%d)", camerasrc->num_video_buffers);

        g_cond_signal(camerasrc->video_buffer_cond);
        g_mutex_unlock(camerasrc->video_buffer_lock);
    }

    GST_DEBUG_OBJECT(camerasrc,"done buffer[type %d] release", buffer->buffer_type);

	gst_object_unref(camerasrc);
	free(buffer);

    GST_DEBUG_OBJECT(camerasrc, "LEAVE");
    return;
}


static void gst_qcamerasrc_wait_all_preview_buffer_finalized(GstCameraSrc *camerasrc)
{
    if (!camerasrc ||
        !camerasrc->preview_buffer_lock ||
        !camerasrc->preview_buffer_cond) {
        GST_ERROR("some handle is NULL");
        return;
    }

    /* check preview buffer count */
    g_mutex_lock(camerasrc->preview_buffer_lock);

    GST_WARNING_OBJECT(camerasrc, "wait preview buffer - live %d",
                                  camerasrc->num_live_buffers);

    while (camerasrc->num_live_buffers > 0) {
        GTimeVal abstimeout;
        g_get_current_time(&abstimeout);
        g_time_val_add(&abstimeout, _PREVIEW_BUFFER_WAIT_TIMEOUT);
        if (!g_cond_timed_wait(camerasrc->preview_buffer_cond, camerasrc->preview_buffer_lock, &abstimeout)) {
            GST_ERROR_OBJECT(camerasrc, "Preview Buffer wait timeout[%d usec].(Live=%d) Skip waiting...",
                                        _PREVIEW_BUFFER_WAIT_TIMEOUT, camerasrc->num_live_buffers);
            break;
        } else {
            GST_WARNING_OBJECT(camerasrc, "Signal received.");
        }
    }

    GST_WARNING("Waiting free Preview buffer finished. (Live=%d)",
                camerasrc->num_live_buffers);

    g_mutex_unlock(camerasrc->preview_buffer_lock);

    return;
}


static void gst_qcamerasrc_wait_all_video_buffer_finalized(GstCameraSrc *camerasrc)
{
    if (!camerasrc ||
        !camerasrc->video_buffer_lock ||
        !camerasrc->video_buffer_cond) {
        GST_ERROR("some handle is NULL");
        return;
    }

    GST_WARNING_OBJECT(camerasrc, "wait video buffer - live %d",
                                  camerasrc->num_video_buffers);

    /* check video buffer count */
    g_mutex_lock(camerasrc->video_buffer_lock);

    while (camerasrc->num_video_buffers > 0) {
        GTimeVal abstimeout;
        g_get_current_time(&abstimeout);
        g_time_val_add(&abstimeout, _VIDEO_BUFFER_WAIT_TIMEOUT);
        if (!g_cond_timed_wait(camerasrc->video_buffer_cond, camerasrc->video_buffer_lock, &abstimeout)) {
            GST_ERROR_OBJECT(camerasrc, "Video Buffer wait timeout[%d usec].(Live=%d) Skip waiting...",
                                        _VIDEO_BUFFER_WAIT_TIMEOUT, camerasrc->num_video_buffers);
            break;
        } else {
            GST_WARNING_OBJECT(camerasrc, "Signal received.");
        }
    }

    GST_WARNING_OBJECT(camerasrc, "Waiting free Video buffer finished. (Live=%d)",
                                  camerasrc->num_video_buffers);

    g_mutex_unlock(camerasrc->video_buffer_lock);

    return;
}


static void gst_qcamerasrc_release_all_queued_preview_buffer(GstCameraSrc *camerasrc)
{
    GstBuffer *buffer = NULL;

    if (!camerasrc ||
        !camerasrc->preview_stream ||
        !camerasrc->video_buffer_cond) {
        GST_ERROR("some handle is NULL");
        return;
    }

    get_preview_stream_mutex(camerasrc->preview_stream);

    GST_WARNING_OBJECT(camerasrc, "release remained buffer in queue");

    /* release remained buffer in queue */
    while (TRUE) {
        if (frame_available(camerasrc->preview_stream)) {
            buffer = get_preview_frame (camerasrc->preview_stream);
            GST_WARNING_OBJECT(camerasrc, "release buffer %p", buffer);
            gst_buffer_unref(buffer);
            buffer = NULL;
        } else {
            GST_WARNING_OBJECT(camerasrc, "no available buffer in queue");
            break;
        }
    }

    GST_WARNING_OBJECT(camerasrc, "release remained buffer done");

    release_preview_stream_mutex(camerasrc->preview_stream);

    return;
}


static void
gst_qcamerasrc_base_init (gpointer klass)
{

    GST_DEBUG("ENTERED");

    GST_DEBUG("LEAVED");
}


static void
gst_qcamerasrc_class_init (GstCameraSrcClass * klass)
{
    GST_DEBUG_CATEGORY_INIT (gst_qcamera_src_debug, "camerasrc", 0, "mm_camera_interface camerasrc");

    GObjectClass *gobject_class;
    GstElementClass *element_class;
    GstBaseSrcClass *basesrc_class;
    GstPushSrcClass *pushsrc_class;

    GST_DEBUG("ENTERED");

    gobject_class = G_OBJECT_CLASS (klass);
    element_class = GST_ELEMENT_CLASS (klass);
    basesrc_class = GST_BASE_SRC_CLASS (klass);
    pushsrc_class = GST_PUSH_SRC_CLASS (klass);

    gobject_class->set_property = gst_qcamerasrc_set_property;
    gobject_class->get_property = gst_qcamerasrc_get_property;

    gobject_class->finalize = gst_qcamerasrc_finalize;

    element_class->change_state = gst_qcamerasrc_change_state;

    basesrc_class->start = gst_qcamerasrc_src_start;
    basesrc_class->stop = gst_qcamerasrc_src_stop;

    basesrc_class->get_caps = gst_qcamerasrc_get_caps;
    basesrc_class->set_caps = gst_qcamerasrc_set_caps;

    pushsrc_class->create = gst_qcamerasrc_src_create;

    g_object_class_install_property (gobject_class, PROP_CAMERA_AUTO_FPS,
                                                g_param_spec_boolean ("fps-auto", "FPS Auto",
                                                "Field for auto fps setting",
                                                _DEFAULT_FPS_AUTO,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_ID,
                                                g_param_spec_int ("camera-id", "index number of camera to activate",
                                                "index number of camera to activate",
                                                _FD_MIN, _FD_MAX, 0,
                                                G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_CAMERA_CAPTURE_FOURCC,
                                                g_param_spec_uint ("capture-fourcc", "Capture format",
                                                "Fourcc value for capture format",
                                                0, G_MAXUINT, 0,
                                                G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_CAMERA_CAPTURE_WIDTH,
                                                g_param_spec_int ("capture-width", "Capture width",
                                                "Width for camera size to capture",
                                                0, G_MAXINT, _DEFAULT_CAP_WIDTH,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_CAPTURE_HEIGHT,
                                                g_param_spec_int ("capture-height", "Capture height",
                                                "Height for camera size to capture",
                                                0, G_MAXINT, _DEFAULT_CAP_HEIGHT,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_CAPTURE_JPG_QUALITY,
                                                g_param_spec_int ("capture-jpg-quality", "JPEG Capture compress ratio",
                                                "Quality of capture image compress ratio",
                                                1, 100, _DEFAULT_CAP_JPG_QUALITY,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_CAPTURE_PROVIDE_EXIF,
                                                g_param_spec_boolean ("provide-exif", "Whether EXIF is provided",
                                                "Does capture provide EXIF?",
                                                _DEFAULT_CAP_PROVIDE_EXIF,
                                                G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_VIDEO_WIDTH,
                                                g_param_spec_int ("video-width", "Video width",
                                                "Width for video size to send via video-stream-cb",
                                                0, G_MAXINT, _DEFAULT_VIDEO_WIDTH,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_VIDEO_HEIGHT,
                                                g_param_spec_int ("video-height", "Video height",
                                                "Height for video size to send via video-stream-cb",
                                                0, G_MAXINT, _DEFAULT_VIDEO_HEIGHT,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_CAMERA_ENABLE_HYBRID_MODE,
                                                g_param_spec_boolean("enable-hybrid-mode", "Enable Video Recording Mode",
                                                "Enable Video Recording mode",
                                                _DEFAULT_ENABLE_HYBRID_MODE,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_VFLIP,
                                                g_param_spec_boolean ("vflip", "Flip vertically",
                                                "Flip camera input vertically",
                                                FALSE,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_HFLIP,
                                                g_param_spec_boolean ("hflip", "Flip horizontally",
                                                "Flip camera input horizontally",
                                                FALSE,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_SENSOR_ORIENTATION,
                                                g_param_spec_int("sensor-orientation", "Sensor orientation",
                                                "Sensor mounted orientation, read-only",
                                                0, 360, 0,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_CAMERA_CPU_LOCK_MODE,
                                                g_param_spec_boolean ("camera-cpu-lock", "Camera CPU Lock",
                                                "Camera CPU Lock enable/disable",
                                                _DEFAULT_ENABLE_CAMERA_CLOCK_LOCK,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_VT_MODE,
                                                g_param_spec_int("vt-mode", "Camera mode for VT call",
                                                "VT mode for fps-setting",
                                                VT_MODE_OFF, VT_MODE_ON, _DEFAULT_VT_MODE,
                                                G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_STOP_CAMERA,
                                    g_param_spec_int("stop-camera", "Stop Camera preview",
                                    "Stop Camera preview asynchronously",
                                    FALSE, TRUE, FALSE,
                                    G_PARAM_READWRITE));

    g_object_class_install_property(gobject_class, PROP_OPEN_CAMERA,
                                    g_param_spec_int("open-camera", "Open Camera device",
                                    "Open camera device",
                                    FALSE, TRUE, FALSE,
                                    G_PARAM_READWRITE));

#ifdef CONFIG_MACH_Z3LTE
    g_object_class_install_property (gobject_class, PROP_CAMERA_HIGH_SPEED_FPS,
                                                g_param_spec_int ("high-speed-fps", "Fps for high speed recording",
                                                "If this value is 0, the element doesn't activate high speed recording.",
                                                0, G_MAXINT, _DEFAULT_HIGH_SPEED_FPS,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_CAPTURE_QUALITY,
                                                g_param_spec_enum ("capture-quality", "Capture quality",
                                                "Quality of capture image (JPEG: 'high', RAW: 'high' or 'low')",
                                                GST_TYPE_QCAMERASRC_QUALITY,
                                                _DEFAULT_CAP_QUALITY,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_CAPTURE_INTERVAL,
                                                g_param_spec_int ("capture-interval", "Capture interval",
                                                "Interval time to capture (millisecond)",
                                                0, G_MAXINT, _DEFAULT_CAP_INTERVAL,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_CAPTURE_COUNT,
                                                g_param_spec_int ("capture-count", "Capture count",
                                                "Capture conut for multishot",
                                                1, G_MAXINT, _DEFAULT_CAP_COUNT,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_CAMERA_CAPTURE_HDR,
                                                g_param_spec_int("hdr-capture", "HDR capture mode",
                                                "HDR(High Dynamic Range) Capture makes the captured image with proper exposure",
                                                CAMERASRC_HDR_CAPTURE_OFF, CAMERASRC_HDR_CAPTURE_ON_AND_ORIGINAL, _DEFAULT_CAP_HDR,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property (gobject_class, PROP_CAMERA_ENABLE_ZSL_MODE,
                                                g_param_spec_boolean ("enable-zsl-mode", "Enable zero shutter lag capture mode",
                                                "Zero shutter lag is mode to capture still image without shutter lag",
                                                _DEFAULT_ENABLE_ZSL_MODE,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#else
    g_object_class_install_property(gobject_class, PROP_CAMERA_ENABLE_VDIS,
                                                g_param_spec_boolean("enable-vdis-mode", "Enable Video Digital Image Stabilization mode",
                                                "Anti-handshake on preview image, not captured image",
                                                _DEFAULT_ENABLE_VDIS_MODE,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif
#ifdef CONFIG_MACH_Z3LTE
    g_object_class_install_property (gobject_class, PROP_CAMERA_LOW_LIGHT_AUTO_DETECTION,
                                                g_param_spec_int ("low-light-detection", "Auto Low light detection",
                                                "Auto Low light detection enable/disable",
                                                LOW_LIGHT_DETECTION_OFF,LOW_LIGHT_DETECTION_ON,_DEFAULT_LOW_LIGHT_DETECTION,
                                                G_PARAM_READWRITE));

    g_object_class_install_property (gobject_class, PROP_SIGNAL_STILLCAPTURE,
                                                g_param_spec_boolean ("signal-still-capture", "Signal still capture",
                                                "Send a signal before pushing the buffer",
                                                _DEFAULT_SIGNAL_STILL_CAPTURE,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#else
    g_object_class_install_property(gobject_class, PROP_DUAL_CAMERA_MODE,
                                                g_param_spec_boolean ("dual-camera-mode", "Dual Camera Mode",
                                                "Dual Camera mode enable/disable",
                                                _DEFAULT_DUAL_CAMERA_MODE,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
#endif
    g_object_class_install_property (gobject_class, PROP_CAMERA_SHOT_MODE,
                                                g_param_spec_int ("shotmode", "shot mode",
                                                "shot mode to be going to",
                                                0, 2 ,_DEFAULT_SHOT_MODE,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

    g_object_class_install_property(gobject_class, PROP_CAMERA_ENABLE_MEDIAPACKET_RECORD_CB,
                                                g_param_spec_boolean ("enable-mediapacket-recordcb", "MediaPacket-Record-Cb Mode",
                                                "Media Packet Record Cb enable/disable",
                                                _DEFAULT_MEDIAPACKET_RECORD_CB_MODE,
                                                G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
    /**
    * GstCameraSrc::still-capture:
    * @camerasrc: the camerasrc instance
    * @buffer: the buffer that will be pushed - Main
    * @buffer: the buffer that will be pushed - Thumbnail
    * @buffer: the buffer that will be pushed - Screennail
    *
    * This signal gets emitted before sending the buffer.
    */
    gst_qcamerasrc_signals[SIGNAL_STILL_CAPTURE] =
                                        g_signal_new( "still-capture",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET(GstCameraSrcClass, still_capture),
                                        NULL,
                                        NULL,
                                        gst_qcamerasrc_VOID__OBJECT_OBJECT,
                                        G_TYPE_NONE,
                                        3, /* Number of parameter */
                                        GST_TYPE_SAMPLE,  /* Main image sample */
                                        GST_TYPE_SAMPLE,  /* Thumbnail image sample */
                                        GST_TYPE_SAMPLE); /* Screennail image sample */
    /**
    * GstCameraSrc::start-record:
    * @camerasrc: the camerasrc instance
    * @buffer: the buffer that will be pushed
    *
    * This signal gets emitted before sending the buffer.
    */
    gst_qcamerasrc_signals[SIGNAL_VIDEO_STREAM_CB] =
                                        g_signal_new( "video-stream-cb",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET(GstCameraSrcClass, video_stream_cb),
                                        NULL,
                                        NULL,
                                        g_cclosure_marshal_generic,
                                        G_TYPE_BOOLEAN,
                                        1, /* Number of parameter */
                                        GST_TYPE_SAMPLE); /* record sample */
    /**
    * GstCameraSrc::start-record:
    * @camerasrc: the camerasrc instance
    * @buffer: the buffer that will be pushed
    *
    * This signal gets emitted before sending the buffer.
    */
    gst_qcamerasrc_signals[SIGNAL_MEDIAPACKET_RECORD_CB] =
                                        g_signal_new( "mediapacket-record-cb",
                                        G_TYPE_FROM_CLASS(klass),
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET(GstCameraSrcClass, mediapacket_stream_cb),
                                        NULL,
                                        NULL,
                                        gst_qcamerasrc_VOID__OBJECT_MEDIAPACKET_STREAM,
                                        G_TYPE_NONE,
                                        1, /* Number of parameter */
                                        GST_TYPE_SAMPLE); /* media stream sample */

    //wh01.cho:req-negotiation:+:To notify user of camerasrc, after changing resolution.
    /**
    * GstCameraSrc::nego-complete:
    * @camerasrc: the camerasrc instance
    * @start: when re-negotiation is finished.
    *
    */
    gst_qcamerasrc_signals[SIGNAL_NEGO_COMPLETE] =
                                        g_signal_new ("nego-complete",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_LAST,
                                        G_STRUCT_OFFSET (GstCameraSrcClass, nego_complete),
                                        NULL,
                                        NULL,
                                        g_cclosure_marshal_generic,
                                        G_TYPE_NONE, 0);

    gst_element_class_add_pad_template (element_class, gst_static_pad_template_get (&src_factory));
    gst_element_class_set_metadata (element_class,
    "Camera Source GStreamer Plug-in",
    "Src/Video",
    "QCOM mm_camera_interface based GStreamer Plug-in",
    "Ravi Patil<ravi.spatil@samsung.com>");

    GST_DEBUG("LEAVED");
}


static void
gst_qcamerasrc_init (GstCameraSrc *camerasrc)
{
    GST_DEBUG_OBJECT (camerasrc, "ENTERED");
    camerasrc->ready_state = FALSE;
    camerasrc->width=_DEFAULT_WIDTH;
    camerasrc->height=_DEFAULT_HEIGHT;
    camerasrc->fps=_DEFAULT_FPS;
    camerasrc->format_str = "NV12";
    camerasrc->pix_format=_DEFAULT_PIX_FORMAT;
    camerasrc->colorspace=_DEFAULT_COLORSPACE;
    camerasrc->rotate=0;
    camerasrc->use_rotate_caps=FALSE;
    camerasrc->num_live_buffers = 0;
    camerasrc->num_video_buffers = 0;
    camerasrc->qbuf_without_display = 0;

    //Element properties
    camerasrc->fps_auto=FALSE;
    camerasrc->camera_id=_DEFAULT_CAMERA_ID;
    camerasrc->cap_fourcc=_DEFAULT_FOURCC;
    camerasrc->prev_cap_width=_DEFAULT_CAP_WIDTH;
    camerasrc->cap_width=_DEFAULT_CAP_WIDTH;
    camerasrc->cap_height=_DEFAULT_CAP_HEIGHT;
    camerasrc->cap_jpg_quality=_DEFAULT_CAP_JPG_QUALITY;
    camerasrc->cap_provide_exif=_DEFAULT_CAP_PROVIDE_EXIF;
    camerasrc->video_width = _DEFAULT_VIDEO_WIDTH;
    camerasrc->video_height = _DEFAULT_VIDEO_HEIGHT;
    camerasrc->sensor_orientation = 0;
    camerasrc->vflip=FALSE;
    camerasrc->hflip=FALSE;
    camerasrc->camera_cpu_clock_lock=_DEFAULT_ENABLE_CAMERA_CLOCK_LOCK;
    camerasrc->is_start_failed = FALSE;
    camerasrc->vt_mode = _DEFAULT_VT_MODE;
    camerasrc->stop_camera = FALSE;
    camerasrc->shot_mode = _DEFAULT_SHOT_MODE;
#ifdef CONFIG_MACH_Z3LTE
    camerasrc->high_speed_fps=_DEFAULT_HIGH_SPEED_FPS;
    camerasrc->cap_quality=_DEFAULT_CAP_QUALITY;
    camerasrc->cap_interval=_DEFAULT_CAP_INTERVAL;
    camerasrc->cap_count=_DEFAULT_CAP_COUNT;
    camerasrc->cap_hdr=_DEFAULT_CAP_HDR;
    camerasrc->enable_zsl_mode=_DEFAULT_ENABLE_ZSL_MODE;
    camerasrc->signal_still_capture=_DEFAULT_SIGNAL_STILL_CAPTURE;
    camerasrc->low_light_detection=_DEFAULT_LOW_LIGHT_DETECTION;
    camerasrc->low_light_condition = LOW_LIGHT_INIT;
#else
    camerasrc->dual_camera_mode=_DEFAULT_DUAL_CAMERA_MODE;
    camerasrc->enable_vdis_mode=_DEFAULT_ENABLE_VDIS_MODE;
#endif
    camerasrc->mediapacket_record_mode =_DEFAULT_MEDIAPACKET_RECORD_CB_MODE;
    camerasrc->local_recording = FALSE;

    //Internal variable
    camerasrc->is_capturing=FALSE;
    camerasrc->face_cb_flag = 0;
    camerasrc->face_detect=0;
    camerasrc->face_detected=FALSE;
    camerasrc->qcam_handle=NULL;
    camerasrc->mode = VIDEO_IN_MODE_UNKNOWN;
    camerasrc->ion_fd = -1;
    camerasrc->firsttime = FALSE;
    camerasrc->firsttime_record = FALSE;
    camerasrc->command_list = g_queue_new ();
    g_mutex_init (&camerasrc->commandq_mutex);
    g_cond_init (&camerasrc->commandq_cond);
#ifdef CAMERA_INTF
    camerasrc->preview_stream=initialize_preview((void*)camerasrc,preview_frame_cb);
    camerasrc->record_stream=initialize_record((void*)camerasrc,record_frame_cb);
#endif
    camerasrc->snapshot_stream=initialize_snapshot((void*)camerasrc,snapshot_done_signal);

    /* init mutex and cond */
    camerasrc->preview_buffer_lock = g_mutex_new ();
    camerasrc->preview_buffer_cond = g_cond_new ();
    camerasrc->video_buffer_lock = g_mutex_new ();
    camerasrc->video_buffer_cond = g_cond_new ();
    camerasrc->stop_camera_lock = g_mutex_new ();
    camerasrc->stop_camera_cond = g_cond_new ();
    camerasrc->restart_camera_lock = g_mutex_new ();
    camerasrc->restart_camera_cond = g_cond_new ();

    /* record command */
    camerasrc->cmd_list = g_queue_new ();
    camerasrc->cmd_lock = g_mutex_new ();
    camerasrc->cmd_cond = g_cond_new ();

    /* focus */
    camerasrc->focus_mode = GST_QCAMERASRC_FOCUS_MODE_PAN; //KiranLTE does not support AF Mode.
    camerasrc->focus_range = GST_QCAMERASRC_FOCUS_RANGE_NORMAL;
    camerasrc->is_focusing = FALSE;

    camerasrc->snapshot_buffer = NULL;
    camerasrc->snapshot_buffer_size = 0;

    camerasrc->batch_control_value_cache=0;
    camerasrc->batch_control_id_cache=QCAMERASRC_PARAM_CONTROL_START;
    camerasrc->setting_value_cached=FALSE;

    /* record handle */
    camerasrc->rec_handle = NULL;

    camerasrc->capture_done_handle = NULL;
    camerasrc->received_capture_command = FALSE;
    camerasrc->img_buffer_cache = (MMVideoBuffer *)g_malloc(sizeof(MMVideoBuffer));

    /* we operate in time */
    gst_base_src_set_format (GST_BASE_SRC (camerasrc), GST_FORMAT_TIME);
    gst_base_src_set_live (GST_BASE_SRC (camerasrc), TRUE);
    gst_base_src_set_do_timestamp (GST_BASE_SRC (camerasrc), TRUE);

    GST_DEBUG("LEAVED");
    return;
}


static gboolean
qcamerasrc_init (GstPlugin * qcamerasrc)
{
    return gst_element_register (qcamerasrc, "camerasrc", GST_RANK_NONE,
    GST_TYPE_CAMERASRC);
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    camerasrc,
    "Camera source plug-in",
    qcamerasrc_init,
    PACKAGE_VERSION,
    "Proprietary",
    "Samsung Electronics Co",
    "http://www.samsung.com")

    /* GstURIHandler interface */
static GstURIType
gst_camerasrc_uri_get_type (GType type)
{
    return GST_URI_SRC;
}

static const gchar * const*
gst_camerasrc_uri_get_protocols (GType type)
{
    static const gchar *protocols[] = { "camera", NULL };
    return protocols;
}

static gchar *
gst_camerasrc_uri_get_uri (GstURIHandler * handler)
{
    return strdup("camera://0");
}

static gboolean
gst_camerasrc_uri_set_uri (GstURIHandler * handler, const gchar * uri, GError **error)
{
    GstCameraSrc *camerasrc = GST_CAMERASRC (handler);
    const gchar *device = "0";
    if (strcmp (uri, "camera://") != 0) {
        device = uri + 9;
    }
    g_object_set (camerasrc, "camera-id", atoi(device), NULL);

    return TRUE;
}


static void
gst_camerasrc_uri_handler_init (gpointer g_iface, gpointer iface_data)
{
    GstURIHandlerInterface *iface = (GstURIHandlerInterface *) g_iface;

    iface->get_type = gst_camerasrc_uri_get_type;
    iface->get_protocols = gst_camerasrc_uri_get_protocols;
    iface->get_uri = gst_camerasrc_uri_get_uri;
    iface->set_uri = gst_camerasrc_uri_set_uri;
}

