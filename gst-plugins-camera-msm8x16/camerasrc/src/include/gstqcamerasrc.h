/*
 * gstqcamerasrc.h
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

#ifndef __GST_QCAMERASRC_H__
#define __GST_QCAMERASRC_H__

#include <gst/gst.h>
#include <gst/base/gstpushsrc.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gst/video/colorbalance.h>
#include <gst/video/cameracontrol.h>
#include <gst/video/video-format.h>
#include <mm_types.h>
#include "qcamera_snapshot.h"
#include "qcamera_preview.h"
#include "qcamera_record.h"
#include "qcamera_main.h"

#ifdef CONFIG_MACH_Z3LTE
//HDR
#include "HDR/camerahdr.h"           /* HDR supershot */
#endif

G_BEGIN_DECLS

#define GST_TYPE_CAMERASRC (gst_qcamerasrc_get_type())
#define GST_CAMERASRC(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_CAMERASRC,GstCameraSrc))
#define GST_CAMERASRC_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_CAMERASRC,GstCameraSrcClass))
#define GST_IS_CAMERASRC(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_CAMERASRC))
#define GST_IS_CAMERASRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_CAMERASRC))

#define ASPECT_RATIO_4_3 1.5  //4//4:3
#define ASPECT_RATIO_16_9 1.7  //16:9
#define ASPECT_RATIO_1_1 1  //1:1

enum {
    GST_QCAMERASRC_FOCUS_MODE_AUTO,
    GST_QCAMERASRC_FOCUS_MODE_MANUAL,
    GST_QCAMERASRC_FOCUS_MODE_PAN,
    GST_QCAMERASRC_FOCUS_MODE_TOUCH_AUTO,
    GST_QCAMERASRC_FOCUS_MODE_CONTINUOUS,
    GST_QCAMERASRC_FOCUS_MODE_MAX
};

enum {
    GST_QCAMERASRC_FOCUS_RANGE_NORMAL,
    GST_QCAMERASRC_FOCUS_RANGE_MACRO,
    GST_QCAMERASRC_FOCUS_RANGE_FULL,
    GST_QCAMERASRC_FOCUS_RANGE_MAX
};


typedef struct _qcamerasrc_record_handle_t qcamerasrc_record_handle_t;
typedef struct _GstCameraSrc      GstCameraSrc;
typedef struct _GstCameraSrcClass GstCameraSrcClass;
typedef struct _GstQCameraBuffer GstQCameraBuffer;
typedef struct _qcamerasrc_capture_done_handle_t qcamerasrc_capture_done_handle_t;

struct _qcamerasrc_record_handle_t {
    GThread *thread;            /**< thread for record */
    GMutex thread_lock;         /**< mutex for record thread */
    GCond thread_cond;          /**< cond for record thread */
    gboolean thread_exit;       /**< thread exit flag */
};

struct _qcamerasrc_capture_done_handle_t {
	GThread *capture_done_thread;            /**< thread for record */
	GMutex capture_done_thread_lock;         /**< mutex for record thread */
	GCond capture_done_thread_cond;          /**< cond for record thread */
	gboolean capture_done_thread_exit;       /**< thread exit flag */
};

struct _GstQCameraBuffer {
    GstBuffer *buffer;
    GstCameraSrc *camerasrc;
    void* buffer_metadata;
    int buffer_type;
};

struct _GstCameraSrc
{
    GstPushSrc  element;

    //camera property
    gboolean ready_state;
    int     width;
    int     height;
    int     fps;
    const gchar *format_str;
    int     pix_format;
    int     colorspace;
    int     rotate;
    gboolean    use_rotate_caps;

    //Element properties
    gboolean    fps_auto;
    gint    camera_id;
    guint32     cap_fourcc;
    gint    prev_cap_width;
    gint    cap_width;
    gint    cap_height;
    gint    cap_jpg_quality;
    gboolean    cap_provide_exif;
    gint    video_width;
    gint    video_height;
    gboolean    enable_hybrid_mode;
    gboolean    local_recording;
    int     sensor_orientation;
    gint    face_detect;
    gboolean    face_detected;
    gboolean    vflip;
    gboolean    hflip;
#ifdef CONFIG_MACH_Z3LTE
    gint    high_speed_fps;
    camerasrc_capture_quality     cap_quality;
    gint    cap_interval;
    gint    cap_count;
    camerasrc_hdr_mode      cap_hdr;
    gboolean    enable_zsl_mode;
    gboolean    signal_still_capture;
    gint    low_light_detection;
    gint    low_light_condition;
#else
    gboolean    dual_camera_mode;
    gboolean enable_vdis_mode;
#endif
    gboolean    mediapacket_record_mode;
    int     vt_mode;
    int 	shot_mode;
    gint    stop_camera;

    //Internal variable
    void*   qcam_handle;
    gint    mode;
    gint    ion_fd;
    gboolean firsttime;
    gboolean firsttime_record;
    GQueue* command_list;
    GMutex commandq_mutex;
    GCond commandq_cond;
    GQueue* cmd_list;
    GMutex* cmd_lock;
    GCond* cmd_cond;
    gint num_live_buffers;
    gint num_video_buffers;
    gint qbuf_without_display;
    GCond *preview_buffer_cond;                   /**< condition for buffer control */
    GMutex *preview_buffer_lock;                  /**< lock for buffer control */
    GCond *video_buffer_cond;                     /**< condition for buffer control */
    GMutex *video_buffer_lock;                    /**< lock for buffer control */
    GCond *stop_camera_cond;                      /**< condition for stop camera */
    GMutex *stop_camera_lock;                     /**< lock for stop camera */

    int focus_mode;                               /**< focus mode */
    int focus_range;                              /**< focus range */
    int is_focusing;                              /**< focus status */
    qcamera_preview_hw_intf* preview_stream;
    qcamera_snapshot_hw_intf* snapshot_stream;
    qcamera_record_hw_intf* record_stream;
    gboolean is_start_failed;

    gboolean is_capturing;
    int face_cb_flag;
    // Colorbalance , CameraControl interface
    GList*  colors;
    GList*  camera_controls;

    unsigned char *snapshot_buffer;		/**< dualcamera, snapshot buffer*/
    int snapshot_buffer_size;			/**< dualcamera, snapshot buffersize*/

    qcamerasrc_batch_ctrl_t* batch_control_value_cache;
    int64_t batch_control_id_cache;
    gboolean setting_value_cached;
    gboolean camera_cpu_clock_lock;

    GCond *restart_camera_cond;                      /**< condition for restart camera */
    GMutex *restart_camera_lock;                     /**< lock for restart camera */

    qcamerasrc_record_handle_t *rec_handle;      /**< record handle */
    qcamerasrc_capture_done_handle_t *capture_done_handle;
    gboolean received_capture_command;
    MMVideoBuffer* img_buffer_cache;
};

struct _GstCameraSrcClass
{
    GstPushSrcClass parent_class;

    void    (*still_capture)    (GstElement *element, GstBuffer *main, GstBuffer *sub);
    void    (*video_stream_cb)    (GstElement *element, GstBuffer *main);
    void    (*mediapacket_stream_cb)    (GstElement *element, GstBuffer *main);
    void    (*nego_complete)    (GstElement *element);
    void    (*register_trouble)    (GstElement *element);
};

int gst_qcamerasrc_set_camera_control(GstCameraSrc* camerasrc,int64_t control_type,int value_1,int value_2,char* string_1);
void gst_qcamerasrc_set_capture_command( GstCameraSrc* camerasrc, GstCameraControlCaptureCommand cmd );
void gst_qcamerasrc_set_record_command( GstCameraSrc* camerasrc, GstCameraControlRecordCommand cmd );
void gst_qcamerasrc_post_message_int(GstCameraSrc *camerasrc, const char *msg_name, const char *field_name, int value);
unsigned long gst_get_current_time(void);
GType gst_qcamerasrc_get_type (void);

G_END_DECLS

#endif /* __GST_QCAMERASRC_H__ */
