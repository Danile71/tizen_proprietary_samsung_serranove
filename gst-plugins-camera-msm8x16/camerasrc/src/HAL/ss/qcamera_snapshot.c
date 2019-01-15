/*
 * qcamera_snapshot.c
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

#include "qcamera_snapshot.h"
#include "gstqcamerasrc.h"
#include "QCameraHALCInterface.h"
#define MAX_BURST_FRAME 20
#if _ENABLE_CAMERASRC_DEBUG
#include <stdio.h>
uint8_t* frame_buf[MAX_BURST_FRAME];
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

#ifdef USE_NEXT_CAPTURE_TIME
static void *take_picture_thread(void *data)
{
    qcamera_snapshot_hw_intf *snapshot_stream = (qcamera_snapshot_hw_intf *)data;
    camerasrc_handle_t *phandle = NULL;
    unsigned long current_time = 0;

    if (snapshot_stream == NULL) {
        GST_ERROR("handle is NULL");
        return NULL;
    }

    GST_DEBUG("start take_picture_thread");

    g_mutex_lock(&snapshot_stream->prev_restart_aft_snapshot_mutex);

    while (snapshot_stream->take_picture_thread_run) {
        GST_DEBUG("wait signal for take picture");
        g_cond_wait(&snapshot_stream->prev_restart_aft_snapshot_cond, &snapshot_stream->prev_restart_aft_snapshot_mutex);
        GST_DEBUG("received signal for take picture - thread run %d", snapshot_stream->take_picture_thread_run);
        if (snapshot_stream->take_picture_thread_run == FALSE) {
            break;
        }

        phandle = (camerasrc_handle_t *)snapshot_stream->qcam_handle;
        current_time = gst_get_current_time();

        /* sleep if next capture time is not reached */
        if (snapshot_stream->next_capture_time > current_time) {
            GST_DEBUG("sleep time %u ms", snapshot_stream->next_capture_time - current_time);
            usleep((snapshot_stream->next_capture_time - current_time)*1000);
        }

        GST_DEBUG("take_picture_internal");

        /* capture */
        take_picture_internal(phandle->cam_device_handle);
        snapshot_stream->next_capture_time += snapshot_stream->capture_interval;
    }

    g_mutex_unlock(&snapshot_stream->prev_restart_aft_snapshot_mutex);

    GST_DEBUG("exit take_picture_thread");

    return NULL;
}
#endif

void takepicture_end_cb(void *user_data)
{
    qcamera_snapshot_hw_intf *snapshot_stream = (qcamera_snapshot_hw_intf*)((camerasrc_handle_t*)user_data)->snapshot_stream;
    camerasrc_handle_t *phandle = (camerasrc_handle_t *)snapshot_stream->qcam_handle;

    snapshot_stream->next_capture_time = gst_get_current_time() + snapshot_stream->capture_interval;
    disable_msg_type(phandle->cam_device_handle,CAMERA_MSG_SHOT_END);
    GST_WARNING("Disable CAMERA_MSG_SHOT_END message ");
}

void snapshot_data_cb(const camera_memory_t *mem,void *user_data)
{
    qcamera_snapshot_hw_intf *snapshot_stream = (qcamera_snapshot_hw_intf*)((camerasrc_handle_t*)user_data)->snapshot_stream;

    if (mem == NULL) {
        snapshot_stream->no_of_callbacks++;
        GST_DEBUG("snapshot_data_cb - mBurstCount:%d longshot %d burst %d",
              snapshot_stream->no_of_callbacks, snapshot_stream->longshot_mode, snapshot_stream->burst_mode);

        g_mutex_lock(&snapshot_stream->prev_restart_aft_snapshot_mutex);
        if (snapshot_stream->longshot_mode &&
            snapshot_stream->no_of_callbacks < snapshot_stream->capture_num &&
            snapshot_stream->burst_mode) {
            GST_DEBUG("take picture signal send");
            g_cond_signal(&snapshot_stream->prev_restart_aft_snapshot_cond);
            g_mutex_unlock(&snapshot_stream->prev_restart_aft_snapshot_mutex);
            return;
        } else {
            GST_DEBUG("Extra Call back after BustMode false");
            g_mutex_unlock(&snapshot_stream->prev_restart_aft_snapshot_mutex);
            return;
        }
    }
#if _ENABLE_CAMERASRC_DEBUG
    static int img_count=0;
    char filePath[70];
    int frame_number=0;
    if(snapshot_stream->capture_fourcc==GST_MAKE_FOURCC ('J', 'P', 'E', 'G')){
        sprintf(filePath, "/opt/usr/media/snapshotburst%02d.jpeg",img_count);
        data_dump (filePath,mem->data,mem->size);
        img_count++;
    }else if((snapshot_stream->capture_fourcc==GST_MAKE_FOURCC ('N', 'V', '1', '2'))||
                (snapshot_stream->capture_fourcc==GST_MAKE_FOURCC ('N', 'V', '2', '1'))||
                (snapshot_stream->capture_fourcc==GST_MAKE_FOURCC ('S', 'N', '1', '2'))){

        frame_buf[img_count]=(uint8_t*)malloc((snapshot_stream->snapshot_width*snapshot_stream->snapshot_height*3)/2);
        memcpy(frame_buf[img_count],mem->data,(snapshot_stream->snapshot_width*snapshot_stream->snapshot_height*3)/2);
        img_count++;

        if(img_count==snapshot_stream->capture_num){
            sprintf(filePath, "/opt/usr/media/snapshotburst%02d.yuv",snapshot_stream->capture_num);
            for(frame_number=0;frame_number<snapshot_stream->capture_num;frame_number++){
                data_dump (filePath,frame_buf[frame_number],
                                    (snapshot_stream->snapshot_width*snapshot_stream->snapshot_height*3)/2);
            }
        }
    }
#endif
    GST_DEBUG("snapshot_data_cb ++");
    if (snapshot_stream->m_hdr_mode) {
        // alloc m_bracket_buffer sequentially.
        snapshot_stream->m_bracket_buffer[snapshot_stream->m_bracket_buffer_count].planes[0].start = (unsigned char *)malloc(mem->size);
        memcpy(snapshot_stream->m_bracket_buffer[snapshot_stream->m_bracket_buffer_count].planes[0].start,mem->data,mem->size);
        // set bracket data when each callback comes
        snapshot_stream->m_bracket_buffer[snapshot_stream->m_bracket_buffer_count].planes[0].length= (int) mem->size;
        snapshot_stream->m_bracket_buffer_count += 1;
        if (snapshot_stream->m_bracket_buffer_count != HDR_BRACKET_FRAME_NUM) {
            // wait [HDR_BRACKET_FRAME_NUM] bracket frames
            return;
        }
    }

    if (!snapshot_stream->m_hdr_mode) {   // not HDR mode
        snapshot_stream->capture_data_buffer = mem->data;
        if (snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('J', 'P', 'E', 'G')) {
            snapshot_stream->capture_data_size = mem->size;
        } else if (snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('N', 'V', '1', '2') ||
                    snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('N', 'V', '2', '1') ||
                   snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('S', 'N', '1', '2')) {
            snapshot_stream->capture_data_size = (snapshot_stream->snapshot_width*snapshot_stream->snapshot_height * 3) / 2;
        }
    } else {
        snapshot_stream->capture_data_buffer = NULL;
        snapshot_stream->capture_data_size = 0;
    }
    if (snapshot_stream->capture_num > 1) {
        if (snapshot_stream->longshot_mode == TRUE) {
            if(snapshot_stream->burst_mode == FALSE)
            {
               GST_DEBUG("Getting BurstMode FALSE %d",snapshot_stream->burst_mode);
               return;
            }
        } else  if (!((snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('N', 'V', '1', '2') ||
            snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('S', 'N', '1', '2')) && snapshot_stream->capture_num==4 && snapshot_stream->capture_interval ==1)) {//Check LLS mode or not.

            snapshot_stream->no_of_callbacks++;
            g_mutex_lock(&snapshot_stream->prev_restart_aft_snapshot_mutex);
            if (snapshot_stream->no_of_callbacks < snapshot_stream->capture_num &&
                    snapshot_stream->burst_mode) {
                GST_DEBUG("take picture signal send on no longshot");
                g_cond_signal(&snapshot_stream->prev_restart_aft_snapshot_cond);
            }
            g_mutex_unlock(&snapshot_stream->prev_restart_aft_snapshot_mutex);
        }
    }

    snapshot_stream->cb_signal((void*)snapshot_stream);

    return;
}

qcamera_snapshot_hw_intf*
initialize_snapshot(void* camerasrc,snapshot_done_cb cb_signal)
{
    qcamera_snapshot_hw_intf* snapshot_stream=NULL;
    snapshot_stream=malloc(sizeof(qcamera_snapshot_hw_intf));

    snapshot_stream->snapshot_width = IMAGE_1MP_WIDTH;
    snapshot_stream->snapshot_height = IMAGE_1MP_HEIGHT;
    snapshot_stream->snapshot_format = CAM_FORMAT_YUV_420_NV12;
    snapshot_stream->thumbnail_width = _THUMBNAIL_WIDTH;
    snapshot_stream->thumbnail_height = _THUMBNAIL_HEIGHT;
    snapshot_stream->capture_interval=_DEFAULT_CAP_INTERVAL;
    snapshot_stream->capture_num=_DEFAULT_CAP_COUNT;
    g_mutex_init (&snapshot_stream->prev_restart_aft_snapshot_mutex);
    g_cond_init (&snapshot_stream->prev_restart_aft_snapshot_cond);
    snapshot_stream->cb_signal = cb_signal;
    snapshot_stream->camerasrc = camerasrc;
    snapshot_stream->qcam_handle = 0;
    snapshot_stream->ion_handle = 0;
    snapshot_stream->m_bracket_buffer_count =0;
    snapshot_stream->no_of_callbacks = 0;
    snapshot_stream->burst_mode = FALSE;
    snapshot_stream->longshot_mode = FALSE;
    snapshot_stream->m_hdr_mode = 0;
    snapshot_stream->m_pre_hdr_mode = 0;
    snapshot_stream->flash_mode = 0;
    snapshot_stream->restart_flag = FALSE;
    g_mutex_init (&snapshot_stream->cmd_mutex);
    g_cond_init(&snapshot_stream->cmd_cond);
#ifdef USE_NEXT_CAPTURE_TIME
    snapshot_stream->take_picture_thread_run = TRUE;
    pthread_create(&snapshot_stream->take_picture_thread_id, NULL, take_picture_thread, (void *)snapshot_stream);
#endif
    return snapshot_stream;
}


void set_snapshot_dimension(qcamera_snapshot_hw_intf* snapshot_stream, int width, int height)
{
    if (!snapshot_stream || width < 1 || height < 1) {
        GST_ERROR("NULL handle[%p] or invalid size[%dx%d]", snapshot_stream, width, height);
        return;
    }

    gst_qcamerasrc_set_camera_control((GstCameraSrc *)snapshot_stream->camerasrc,
        QCAMERASRC_PARAM_CONTROL_SNAPSHOT_SIZE,
        width,
        height,
        NULL);

    get_camera_control((camerasrc_handle_t *)snapshot_stream->qcam_handle,
                                    QCAMERASRC_PARAM_CONTROL_SNAPSHOT_SIZE,
                                    &snapshot_stream->snapshot_width,
                                    &snapshot_stream->snapshot_height,
                                    0);

    if ((snapshot_stream->snapshot_width!=width)||(snapshot_stream->snapshot_height!=height)) {
        GST_WARNING("Snapshot dimension[%dX%d] just is requested. This is not yet set. Current Snapshot dimension value is [%dx%d]", width, height,
            snapshot_stream->snapshot_width,
            snapshot_stream->snapshot_height);
    }

    return;
}


gboolean start_snapshot(qcamera_snapshot_hw_intf* snapshot_stream)
{
    //TODO Add API to pass the snapshot frame params to HAL.hardcoded now
    int ret=CAMERASRC_SUCCESS;

    GST_DEBUG("start_snapshot ++ [%d %d]",
          snapshot_stream->snapshot_width,
          snapshot_stream->snapshot_height);

    camerasrc_handle_t *phandle = (camerasrc_handle_t *)snapshot_stream->qcam_handle;
    GstCameraSrc *camerasrc=(GstCameraSrc*)snapshot_stream->camerasrc;

    if (snapshot_stream->capture_num > 1) { //Burst Shot
        if (snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('J', 'P', 'E', 'G')) {
            snapshot_stream->longshot_mode = TRUE;
        } else {
            snapshot_stream->longshot_mode = FALSE;
        }
        if(camerasrc->shot_mode == 0) {
            GST_WARNING("Enable CAMERA_MSG_SHOT_END message ");
            enable_msg_type(phandle->cam_device_handle,CAMERA_MSG_SHOT_END);
        }
        snapshot_stream->burst_mode = TRUE;
        snapshot_stream->no_of_callbacks = 0;
        GST_DEBUG("start_snapshot snapshot_stream->capture_num > 1, %d ",snapshot_stream->longshot_mode);

    }

    if (snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('J', 'P', 'E', 'G')) {
        send_command(phandle->cam_device_handle, CAMERA_CMD_SET_RAW_CAPTURE_MODE, 0, 0);
        if (snapshot_stream->longshot_mode && snapshot_stream->capture_num == 30) {
            send_command(phandle->cam_device_handle, CAMERA_CMD_ENABLE_SCREENNAIL, 0, 0);
        } else {
            send_command(phandle->cam_device_handle, CAMERA_CMD_ENABLE_SCREENNAIL, 1, 0);
        }
    } else if (snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('N', 'V', '1', '2') ||
                snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('N', 'V', '2', '1') ||
               snapshot_stream->capture_fourcc == GST_MAKE_FOURCC ('S', 'N', '1', '2')) {

        if(snapshot_stream->capture_num==4) {
            GST_WARNING("This is for LLS shot");
            send_command(phandle->cam_device_handle, CAMERA_CMD_SET_RAW_CAPTURE_MODE, 0, 0);
            send_command(phandle->cam_device_handle, CAMERA_CMD_ENABLE_SCREENNAIL, 1, 0);
            send_command(phandle->cam_device_handle, CAMERA_CMD_LOW_LIGHT_MODE, 1, 0);
        } else {
        GST_WARNING("This is not for LLS shot");
            send_command(phandle->cam_device_handle, CAMERA_CMD_SET_RAW_CAPTURE_MODE, 1, 0);
            send_command(phandle->cam_device_handle, CAMERA_CMD_ENABLE_SCREENNAIL, 0, 0);
        }

    }

    if (snapshot_stream->capture_num > 1) {//Burst Shot
        send_command(phandle->cam_device_handle,CAMERA_CMD_RUN_BURST_TAKE, 0, 0);
        if(snapshot_stream->longshot_mode == FALSE)
        {
               GST_DEBUG("start_snapshot start_preview done ");
        } else {
            /* Now send longshot command. Note: The order is important
                         Sending 'run_burst_take' cmd above ensures any existing
                         timer shot state is cleaned up cleanly and HAL is in preview mode.
                         longshot_on' command send after this, prepares the hal to take burst
                         in long shot mode
                      */
            send_command(phandle->cam_device_handle,CAMERA_CMD_LONGSHOT_ON, 0, 0);
        }
    }

    set_camera_parameter(phandle->cam_device_handle,
                         QCAMERASRC_PARAM_CONTROL_SNAPSHOT_FORMAT,
                         snapshot_stream->snapshot_format,
                         0,
                         NULL);
#ifdef USE_NEXT_CAPTURE_TIME
    snapshot_stream->next_capture_time = gst_get_current_time() + snapshot_stream->capture_interval;
#endif
    ret = take_picture(phandle->cam_device_handle);
    if (CAMERASRC_SUCCESS == ret) {
        return TRUE;
    }
    if ((snapshot_stream->capture_num > 1) && (camerasrc->shot_mode == 0)) {
        disable_msg_type(phandle->cam_device_handle,CAMERA_MSG_SHOT_END);
        GST_ERROR("Disable CAMERA_MSG_SHOT_END message ");
    }
    return FALSE;
}



gboolean stop_snapshot(qcamera_snapshot_hw_intf* snapshot_stream)
{
    int ret=CAMERASRC_SUCCESS;

    GST_DEBUG("stop_snapshot ++");

    camerasrc_handle_t *phandle = (camerasrc_handle_t *)snapshot_stream->qcam_handle;
    g_mutex_lock (&snapshot_stream->prev_restart_aft_snapshot_mutex);
    snapshot_stream->burst_mode = FALSE;
    snapshot_stream->longshot_mode = FALSE;
    GST_DEBUG("Setting BurstMode FALSE %d",snapshot_stream->burst_mode);
    if (snapshot_stream->capture_num > 1) {//Burst Shot
        snapshot_stream->no_of_callbacks = 0;
        GST_DEBUG("start_snapshot snapshot_stream->capture_num > 1  and num=%d",snapshot_stream->capture_num);
        ret = send_command(phandle->cam_device_handle,CAMERA_CMD_STOP_BURST_TAKE, 0, 0);
        set_camera_parameter(phandle->cam_device_handle,QCAMERASRC_PARAM_CONTROL_FLASH_MODE,snapshot_stream->flash_mode,0,NULL);
    }
    snapshot_stream->capture_num = 1;
    gst_qcamerasrc_set_camera_control((GstCameraSrc *)snapshot_stream->camerasrc,
                                      QCAMERASRC_PARAM_CONTROL_BURST_CONFIG,
                                      1, 1, NULL);

    g_mutex_unlock (&snapshot_stream->prev_restart_aft_snapshot_mutex);
    return ret;
}


void set_snapshot_flip(qcamera_snapshot_hw_intf* snapshot_stream,int flip_mode)
{
    camerasrc_handle_t *phandle = (camerasrc_handle_t *)snapshot_stream->qcam_handle;
    send_command(phandle->cam_device_handle,CAMERA_CMD_SET_SNAPSHOT_FLIP,flip_mode, 0);
    /*Following alternative way can also be expored by setting the snapshot stream property.Not working now.
    set_camera_parameter(phandle->cam_device_handle,
                                     QCAMERASRC_PARAM_CONTROL_SNAPSHOT_FLIP,
                                     flip_mode, 0,NULL);*/
}

gboolean
cancel_snapshot(qcamera_snapshot_hw_intf* snapshot_stream){
    int ret=CAMERASRC_SUCCESS;
    camerasrc_handle_t *phandle=(camerasrc_handle_t *)snapshot_stream->qcam_handle;
    ret = cancel_picture(phandle->cam_device_handle);
    return ret;
}

void
free_snapshot(qcamera_snapshot_hw_intf* snapshot_stream){

#ifdef USE_NEXT_CAPTURE_TIME
    g_mutex_lock(&snapshot_stream->prev_restart_aft_snapshot_mutex);
    snapshot_stream->take_picture_thread_run = FALSE;
    g_cond_signal(&snapshot_stream->prev_restart_aft_snapshot_cond);
    g_mutex_unlock(&snapshot_stream->prev_restart_aft_snapshot_mutex);
    GST_DEBUG("join take picture thread");
    pthread_join(snapshot_stream->take_picture_thread_id, NULL);
#endif
    g_mutex_clear (&snapshot_stream->prev_restart_aft_snapshot_mutex);
    g_cond_clear (&snapshot_stream->prev_restart_aft_snapshot_cond);
    g_mutex_clear (&snapshot_stream->cmd_mutex);
    g_cond_clear (&snapshot_stream->cmd_cond);
    free(snapshot_stream);
    return;
}
