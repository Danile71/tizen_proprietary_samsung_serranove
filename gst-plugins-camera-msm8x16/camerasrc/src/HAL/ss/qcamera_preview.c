/*
 * qcamera_preview.c
 *
 * Copyright (c) 2012 - 2013 Samsung Electronics Co., Ltd.
 *
 * Contact: Aditi Narula<aditi.n@samsung.com>
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
#include "qcamera_preview.h"
#include "gstqcamerasrc.h"
#include "QCameraHALCInterface.h"
#if _ENABLE_CAMERASRC_DEBUG
#include <stdio.h>
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
#endif
#ifndef GST_CAT_DEFAULT
GST_DEBUG_CATEGORY_EXTERN(gst_qcamera_src_debug);
#define GST_CAT_DEFAULT gst_qcamera_src_debug
#endif /* GST_CAT_DEFAULT */

#define PANORAMA_SHOT_CAMERA_WIDTH      1920
#define PANORAMA_SHOT_CAMERA_HEIGHT     1080
#define DRAMA_SHOT_CAMERA_WIDTH         1280
#define DRAMA_SHOT_CAMERA_HEIGHT        720
#define PANORAMA_DRAMA_SHOT_FPS         30

void
preview_data_cb(const camera_memory_t *mem,void *user_data){
#if _ENABLE_CAMERASRC_DEBUG
    data_dump ("/usr/etc/preview.yuv",mem->data,mem->size);
#endif
    return;
}

void
screen_nail_data_cb(camera_memory_t *mem,void *user_data){
    if(mem && mem->handle){
        if(*((int*)mem->handle)!=SCREEN_NAIL_BUFFER_HANDLE)
            return;
        qcamera_preview_hw_intf *preview_stream=(qcamera_preview_hw_intf*)
                                        ((camerasrc_handle_t*)user_data)->preview_stream;
        cache_screen_nail_frame(preview_stream,mem);
    }
    return;
}

qcamera_preview_hw_intf*
initialize_preview(void* camerasrc,preview_cb cb_fn){
    qcamera_preview_hw_intf* preview_stream=NULL;
    preview_stream=malloc(sizeof(qcamera_preview_hw_intf));

    preview_stream->preview_width = FRAME_VGA_WIDTH;
    preview_stream->preview_height = FRAME_VGA_HEIGHT;
    preview_stream->preview_format = CAM_FORMAT_YUV_420_NV12;

    preview_stream->fps = _DEFAULT_FPS;
    preview_stream->fps_auto = _DEFAULT_FPS_AUTO;

    g_mutex_init (&preview_stream->preview_mutex);
    g_cond_init (&preview_stream->preview_cond);
    preview_stream->preview_frame_queue = g_queue_new ();

    g_mutex_init (&preview_stream->screen_nail_mutex);
    preview_stream->screen_nail_queue = g_queue_new ();

    preview_stream->preview_frame_cb=cb_fn;
    preview_stream->camerasrc=camerasrc;
    preview_stream->qcam_handle=0;
    preview_stream->ion_handle=0;
#ifdef CONFIG_MACH_Z3LTE
    preview_stream->low_light_mode=FALSE;
#endif
    return preview_stream;
}

gboolean
start_preview(qcamera_preview_hw_intf* preview_stream){
    //TODO Add API to pass the preview frame params to HAL.hardcoded now
    int ret=CAMERASRC_SUCCESS;
    int shot_mode = SHOOTINGMODE_AUTO;
    GST_WARNING("%s Enter",__FUNCTION__);
    camerasrc_handle_t *phandle=(camerasrc_handle_t *)preview_stream->qcam_handle;
    qcamerasrc_batch_ctrl_t* batch_control=(qcamerasrc_batch_ctrl_t*)malloc(sizeof(qcamerasrc_batch_ctrl_t));
    int64_t control_value=QCAMERASRC_PARAM_CONTROL_START;
    GstCameraSrc *camerasrc=(GstCameraSrc*)preview_stream->camerasrc;

    if(!batch_control){
        GST_ERROR("%s failed to allocate batch_control str",__FUNCTION__);
        return FALSE;
    }
    batch_control->preview_width=preview_stream->preview_width;
    batch_control->preview_height=preview_stream->preview_height;
    control_value=control_value|QCAMERASRC_PARAM_CONTROL_PREVIEW_SIZE;

    batch_control->preview_format=preview_stream->preview_format;
    control_value=control_value|QCAMERASRC_PARAM_CONTROL_PREVIEW_FORMAT;

    if (camerasrc->camera_id == CAMERASRC_DEV_ID_SECONDARY) {
        batch_control->fps_min=preview_stream->fps_auto ? _MIN_FPS_FRONT: preview_stream->fps;
    } else {
        batch_control->fps_min=preview_stream->fps_auto ? _MIN_FPS_REAR: preview_stream->fps;
    }

    batch_control->fps_max=preview_stream->fps_auto ? _MAX_FPS : preview_stream->fps;
    control_value=control_value|QCAMERASRC_PARAM_CONTROL_FPS;

    /* check shot mode */

    if (preview_stream->recording_hint) {
          shot_mode = VIDEO_RECORDING_MODE;
    } else {
#ifdef CONFIG_MACH_Z3LTE
        if(camerasrc->shot_mode == 1) {
            shot_mode = SHOOTINGMODE_NIGHT;
        } else if(camerasrc->shot_mode == 2) {
            shot_mode = SHOOTINGMODE_PANORAMA;
        }
#endif
    }

    GST_WARNING("Shot mode : %d", shot_mode);

    batch_control->shoting_mode=shot_mode;
    control_value=control_value|QCAMERASRC_PARAM_CONTROL_SHOOTING_MODE;


    set_batch_camera_parameter(phandle->cam_device_handle,
            control_value,
            batch_control);


    free(batch_control);

    ret = start_camera_preview(phandle->cam_device_handle,
                                        preview_stream->preview_frame_cb,
                                        preview_stream->camerasrc);

    if(CAMERASRC_SUCCESS==ret) {
        GST_WARNING("%s Exit",__FUNCTION__);
        return TRUE;
    }
    GST_ERROR("%s is failed ret(%d)",__FUNCTION__, ret);
    return FALSE;
}

void
set_preview_flip(qcamera_preview_hw_intf* preview_stream,int flip_mode)
{
    GstCameraSrc *camerasrc=(GstCameraSrc*)preview_stream->camerasrc;
    gst_qcamerasrc_set_camera_control(camerasrc,
                QCAMERASRC_PARAM_CONTROL_PREVIEW_FLIP,
                flip_mode,
                0,
                NULL);
    return;
}

gboolean
stop_preview(qcamera_preview_hw_intf* preview_stream){

    camerasrc_handle_t *phandle=(camerasrc_handle_t *)preview_stream->qcam_handle;
    stop_camera_preview(phandle->cam_device_handle);
    return TRUE;
}

void
done_preview_frame(qcamera_preview_hw_intf* preview_stream,void* opaque){
    camerasrc_handle_t *phandle=(camerasrc_handle_t *)preview_stream->qcam_handle;
    release_preview_frame(phandle->cam_device_handle,opaque);
}

void
free_preview(qcamera_preview_hw_intf* preview_stream){
    g_mutex_clear (&preview_stream->preview_mutex);
    g_cond_clear (&preview_stream->preview_cond);
    g_queue_free (preview_stream->preview_frame_queue);
    g_mutex_clear (&preview_stream->screen_nail_mutex);
    g_queue_free (preview_stream->screen_nail_queue);
    preview_stream->preview_frame_queue = NULL;
    free(preview_stream);
    return;
}

gboolean
frame_available(qcamera_preview_hw_intf* preview_stream){
    gboolean ret=TRUE;
    if(g_queue_is_empty (preview_stream->preview_frame_queue))
        ret = FALSE;
    return ret;
}

void
wait_for_frame(qcamera_preview_hw_intf* preview_stream){
    GTimeVal abstimeout;
    GstCameraSrc *camerasrc=(GstCameraSrc*)preview_stream->camerasrc;

    if(g_queue_is_empty (preview_stream->preview_frame_queue)){
        g_get_current_time(&abstimeout);
        if(camerasrc->vt_mode && camerasrc->firsttime) {
                g_time_val_add(&abstimeout, VT_PREVIEW_FRAME_MAX_WAIT_TIME);
        } else {
                g_time_val_add(&abstimeout, PREVIEW_FRAME_MAX_WAIT_TIME);
        }
        g_cond_timed_wait(&preview_stream->preview_cond, &preview_stream->preview_mutex,&abstimeout);
    }
    return;
}

GstBuffer *
get_preview_frame(qcamera_preview_hw_intf* preview_stream){
    return g_queue_pop_head (preview_stream->preview_frame_queue);
}

void
cache_preview_frame(qcamera_preview_hw_intf* preview_stream,GstBuffer * buffer){
    if(buffer && preview_stream->preview_frame_queue)
        g_queue_push_tail (preview_stream->preview_frame_queue, buffer);
    g_cond_broadcast (&preview_stream->preview_cond);
}

void
signal_flush_frame(qcamera_preview_hw_intf* preview_stream){
    g_cond_broadcast (&preview_stream->preview_cond);
}

void
get_preview_stream_mutex(qcamera_preview_hw_intf* preview_stream){
    g_mutex_lock (&preview_stream->preview_mutex);
}

void
release_preview_stream_mutex(qcamera_preview_hw_intf* preview_stream){
    g_mutex_unlock (&preview_stream->preview_mutex);
}

camera_memory_t *
get_screen_nail_frame(qcamera_preview_hw_intf* preview_stream){
    camera_memory_t* ret=NULL;
    if(preview_stream->screen_nail_queue){
        g_mutex_lock (&preview_stream->screen_nail_mutex);
        if(!g_queue_is_empty (preview_stream->screen_nail_queue)){
            ret = g_queue_pop_head (preview_stream->screen_nail_queue);
        }else{
            ret = NULL;
        }
        g_mutex_unlock (&preview_stream->screen_nail_mutex);
    }
    return ret;
}

void
cache_screen_nail_frame(qcamera_preview_hw_intf* preview_stream,camera_memory_t * buffer){
    if(preview_stream->screen_nail_queue){
        g_mutex_lock (&preview_stream->screen_nail_mutex);
        g_queue_push_tail (preview_stream->screen_nail_queue, buffer);
        g_mutex_unlock (&preview_stream->screen_nail_mutex);
    }
    return;
}

void
free_cached_screen_nail_frame(camera_memory_t* frame){
    if(frame){
        if(frame->data)
            free(frame->data);
        if(frame->handle)
            free(frame->handle);
        free(frame);
    }
    return;
}
