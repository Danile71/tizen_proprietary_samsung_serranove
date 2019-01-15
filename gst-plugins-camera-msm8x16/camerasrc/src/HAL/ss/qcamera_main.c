/*
 * qcamera_main.c
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

#include "qcamera_main.h"
#include "qcamera_snapshot.h"
#include "qcamera_preview.h"
#include "qcamera_record.h"
#include "gstqcamerasrc.h"
#include "QCameraHALCInterface.h"
#include <hardware/camera.h>
#ifndef GST_CAT_DEFAULT
GST_DEBUG_CATEGORY_EXTERN(gst_qcamera_src_debug);
#define GST_CAT_DEFAULT gst_qcamera_src_debug
#endif /* GST_CAT_DEFAULT */

static void
release_memory(struct camera_memory *mem){
    if(!mem->handle){
        free(mem->data);
        mem->data = NULL;
        free(mem);
        mem = NULL;
    }else{
        munmap(mem->data, mem->size);
        free(mem);
        mem = NULL;
    }
    return;
}

static camera_memory_t*
cam_request_memory(int fd, size_t buf_size, unsigned int num_bufs,void *user_data){
    camera_memory_t* cam_memory=0;
    cam_memory = (camera_memory_t*)malloc(sizeof(camera_memory_t));
    memset(cam_memory,0,sizeof(camera_memory_t));
    void* memory_data=0;
    if(fd<0){
        memory_data = (void*)malloc(buf_size*num_bufs);
        memset(memory_data,0,buf_size*num_bufs);
        cam_memory->handle=0;
    }else{
        memory_data = mmap(NULL,buf_size*num_bufs,
                    PROT_READ | PROT_WRITE,MAP_SHARED,fd, 0);
        cam_memory->handle=user_data;
    }
    cam_memory->data=memory_data;
    cam_memory->size=buf_size*num_bufs;
    cam_memory->release=release_memory;
    return cam_memory;
}

static void
cam_notify_callback(int32_t msg_type,int32_t ext1,int32_t ext2, void *user_data){
    if(!user_data) {
        GST_ERROR("user data received is NULL");
        return;
    }
    switch (msg_type){
        case CAMERA_MSG_ERROR:
        break;
        case CAMERA_MSG_SHUTTER:
        break;
        case CAMERA_MSG_FOCUS:
        {
            qcamera_preview_hw_intf *preview = (qcamera_preview_hw_intf*)
                                                    ((camerasrc_handle_t*)user_data)->preview_stream;
            if(ext1 == AUTO_FOCUS_FAIL)
                preview->af_cb(preview->camerasrc, CAMERASRC_AUTO_FOCUS_RESULT_FAILED);
            else if(ext1 == AUTO_FOCUS_SUCCESS)
                preview->af_cb(preview->camerasrc,  CAMERASRC_AUTO_FOCUS_RESULT_FOCUSED);
            else if(ext1 == AUTO_FOCUS_FOCUSING)
                preview->af_cb(preview->camerasrc,  CAMERASRC_AUTO_FOCUS_RESULT_FUCUSING);
            else if(ext1 == AUTO_FOCUS_RESTART)
                preview->af_cb(preview->camerasrc,CAMERASRC_AUTO_FOCUS_RESULT_RESTART);
        break;
        }
        case CAMERA_MSG_ZOOM:
        break;
        case CAMERA_MSG_FOCUS_MOVE:
        break;
        case CAMERA_MSG_RAW_IMAGE_NOTIFY:
        {
            qcamera_snapshot_hw_intf *snapshot = (qcamera_snapshot_hw_intf*)
                                                    ((camerasrc_handle_t*)user_data)->snapshot_stream;
            snapshot->raw_capture_cb(snapshot->camerasrc);
        }
        break;
        case CAMERA_MSG_SHOT_END:
            takepicture_end_cb(user_data);
        break;
        default:
        break;
    }
    return;
}

static void
cam_data_callback(int32_t msg_type,camera_memory_t *data, unsigned int index,
        camera_frame_metadata_t *metadata, void *user_data){
    if(!user_data) {
        GST_ERROR("user data received is NULL");
        return;
    }
    switch (msg_type){
        case CAMERA_MSG_PREVIEW_FRAME:
            preview_data_cb(data,user_data);
        break;
        case CAMERA_MSG_VIDEO_FRAME:
            //record_data_cb(data,user_data);
        break;
        case CAMERA_MSG_POSTVIEW_FRAME:
            screen_nail_data_cb(data,user_data);
        break;
        case CAMERA_MSG_RAW_IMAGE:
        break;
        case CAMERA_MSG_COMPRESSED_IMAGE:
            snapshot_data_cb(data,user_data);
        break;
        case CAMERA_MSG_RAW_IMAGE_NOTIFY:
        break;
        case CAMERA_MSG_PREVIEW_METADATA:
        {
            qcamera_preview_hw_intf *preview = (qcamera_preview_hw_intf*)
                                ((camerasrc_handle_t*)user_data)->preview_stream;
            preview->preview_metadata_cb(preview->camerasrc,metadata);
        }
        break;
        case CAMERA_MSG_STATS_DATA:
        break;
        default:
        break;
    }
    return;
}

static void
cam_data_timestamp_callback(int64_t timestamp,int32_t msg_type,
        const camera_memory_t *data, unsigned int index,void *user_data){
    if(!user_data) {
        GST_ERROR("user data received is NULL");
        return;
    }
    switch (msg_type){
        case CAMERA_MSG_VIDEO_FRAME:
            record_timestamp_cb(timestamp,data,index,user_data);
        break;
        default:
        break;
    }
    return;
}

static void
qcamera_hal_destroy_callback(void *user_data){
    camerasrc_handle_t* phandle = (camerasrc_handle_t*)user_data;
    if(!phandle)
        return;
    if(phandle->cam_info_data){
        free(phandle->cam_info_data);
        phandle->cam_info_data = 0;
    }
    phandle->hw_device_data = 0;//Memory is already freed inside QCAM-HAL
    phandle->cam_device_handle = 0;//Memory is already freed inside QCAM-HAL
    return;
}

int
create_camera_handle(camerasrc_handle_t **phandle){
    camerasrc_handle_t* p = 0;

    if (phandle == NULL) {
        return CAMERASRC_ERR_NULL_POINTER;
    }
    p = (camerasrc_handle_t *)malloc(sizeof(camerasrc_handle_t));
    if(p == NULL) {
        return CAMERASRC_ERR_ALLOCATION;
    }
    memset(p, '0', sizeof(camerasrc_handle_t));
    *phandle = p;

    return CAMERASRC_SUCCESS;
}

void
free_camera_handle(camerasrc_handle_t **phandle){
    if (*phandle) {
        free(*phandle);
        *phandle = NULL;
    }
}

int
open_camera_device(camerasrc_handle_t *phandle, int camera_id, int *sensor_orientation){
    int ret=CAMERASRC_SUCCESS;

    ret = camera_device_open(camera_id,
                                            &(phandle->hw_device_data),
                                            &(phandle->cam_device_handle),
                                            qcamera_hal_destroy_callback,
                                            phandle);

    if(CAMERASRC_SUCCESS!=ret) {
        switch (-ret) {
        case EPERM:
        //fall through
        case EACCES:
            GST_ERROR( "Permission error was happened");
            ret = CAMERASRC_ERR_PRIVILEGE;
            break;
        case EBUSY:
            GST_ERROR( "Device busy error was happened");
            ret = CAMERASRC_ERR_BUSY;
            break;
        case ENXIO:  //device not found
            GST_ERROR( "device was not found");
            ret = CAMERASRC_ERR_UNAVAILABLE_DEVICE;
            break;
        default:
            ret = CAMERASRC_ERR_DEVICE_OPEN;
            break;
       }
       GST_ERROR( "[%s] is failed(%d)",__func__, ret);
       return ret;
    }

    ret = get_camera_info(camera_id,(void**)(&(phandle->cam_info_data)));
    if(CAMERASRC_SUCCESS!=ret)
        return ret;

    set_CallBacks(phandle->cam_device_handle,
                        cam_notify_callback,
                        cam_data_callback,
                        cam_data_timestamp_callback,
                        cam_request_memory,
                        phandle);

    enable_msg_type(phandle->cam_device_handle,CAMERA_MSG_COMPRESSED_IMAGE);
    enable_msg_type(phandle->cam_device_handle,CAMERA_MSG_POSTVIEW_FRAME);
    /*[Abhijit]We don't really need this message since timestamp will be set for all buffers in the record_frame_cb function*/
    //enable_msg_type(phandle->cam_device_handle,CAMERA_MSG_VIDEO_FRAME);
    enable_msg_type(phandle->cam_device_handle,CAMERA_MSG_FOCUS);
    //enable_msg_type(phandle->cam_device_handle,CAMERA_MSG_SHUTTER);
    enable_msg_type(phandle->cam_device_handle,CAMERA_MSG_PREVIEW_METADATA);
#ifndef DEFAULT_ZSL_MODE_ON
    enable_msg_type(phandle->cam_device_handle,CAMERA_MSG_RAW_IMAGE_NOTIFY);
#endif
    /* get sensor orientation */
    if (sensor_orientation) {
        *sensor_orientation = phandle->cam_info_data->orientation;
    }

    return ret;
}

int
close_camera_device(camerasrc_handle_t *phandle){
    int ret=CAMERASRC_SUCCESS;
    ret = camera_device_close(phandle->hw_device_data);
    if(phandle->cam_info_data){
        free(phandle->cam_info_data);
        phandle->cam_info_data = 0;
    }
    phandle->hw_device_data = 0;//Memory is already freed inside QCAM-HAL
    phandle->cam_device_handle = 0;//Memory is already freed inside QCAM-HAL
    return ret;
}

int
set_camera_control(camerasrc_handle_t *phandle,int64_t control_type,int value_1,int value_2,char* string_1){
    if(!phandle){
        GST_ERROR( "%s Null handle",__FUNCTION__);
        return CAMERASRC_ERR_NULL_POINTER;
    }
    if(!phandle->cam_device_handle ) {
        GST_ERROR("phandle->cam_device_handle is null");
        return CAMERASRC_ERR_NULL_POINTER;
    }
    return set_camera_parameter(phandle->cam_device_handle,control_type,value_1,value_2,string_1);
}

int
get_camera_control(camerasrc_handle_t *phandle,int64_t control_type,int* value_1,int* value_2,char* string_1){
    if(!phandle){
        GST_ERROR( "%s Null handle",__FUNCTION__);
        return CAMERASRC_ERR_NULL_POINTER;
    }
    if(!phandle->cam_device_handle ) {
        GST_ERROR("phandle->cam_device_handle is null");
        return CAMERASRC_ERR_NULL_POINTER;
    }
    return get_camera_parameter(phandle->cam_device_handle,control_type,value_1,value_2,string_1);
}

int
start_camera_auto_focus(camerasrc_handle_t *phandle){
    if(!phandle ) {
        GST_ERROR( "%s Null handle",__FUNCTION__);
        return CAMERASRC_ERR_NULL_POINTER;
    }
    if(!phandle->cam_device_handle ) {
        GST_ERROR("phandle->cam_device_handle is null");
        return CAMERASRC_ERR_NULL_POINTER;
    }
    return auto_focus(phandle->cam_device_handle);
}

int
cancel_camera_auto_focus(camerasrc_handle_t *phandle){
    if(!phandle ) {
        GST_ERROR( "%s Null handle",__FUNCTION__);
        return CAMERASRC_ERR_NULL_POINTER;
    }
    if(!phandle->cam_device_handle ) {
        GST_ERROR("phandle->cam_device_handle is null");
        return CAMERASRC_ERR_NULL_POINTER;
    }
    return cancel_auto_focus(phandle->cam_device_handle);
}

int
set_low_light_auto_detection(camerasrc_handle_t *phandle,int lld_mode){
    int ret=CAMERASRC_SUCCESS;
    if(!phandle ) {
        GST_ERROR( "%s Null handle",__FUNCTION__);
        return CAMERASRC_ERR_NULL_POINTER;
    }
    if(!phandle->cam_device_handle ) {
        GST_ERROR("phandle->cam_device_handle is null");
        return CAMERASRC_ERR_NULL_POINTER;
    }
    ret = send_command(phandle->cam_device_handle,CAMERA_CMD_SET_AUTO_LOW_LIGHT, lld_mode, 0);
    if(CAMERASRC_SUCCESS!=ret){
        GST_ERROR( "%s CAMERA_CMD_SET_AUTO_LOW_LIGHT setting failed ",__func__);
    }
    ret = send_command(phandle->cam_device_handle,CAMERA_CMD_ENABLE_FLASH_AUTO_CALLBACK, lld_mode, 0);
    if(CAMERASRC_SUCCESS!=ret){
        GST_ERROR( "%s CAMERA_CMD_ENABLE_FLASH_AUTO_CALLBACK setting failed ",__func__);
    }
    return ret;
}

int
set_low_light_mode(camerasrc_handle_t *phandle,int llm_mode){
    int ret=CAMERASRC_SUCCESS;
    if(!phandle ) {
        GST_ERROR( "%s Null handle",__FUNCTION__);
        return CAMERASRC_ERR_NULL_POINTER;
    }
    if(!phandle->cam_device_handle ) {
        GST_ERROR("phandle->cam_device_handle is null");
        return CAMERASRC_ERR_NULL_POINTER;
    }
    ret = send_command(phandle->cam_device_handle,CAMERA_CMD_LOW_LIGHT_MODE, llm_mode, 0);
    if(CAMERASRC_SUCCESS!=ret){
        GST_ERROR( "%s Auto Low Light Detection setting failed  for Snapshot",__FUNCTION__);
    }
    return ret;
}

int
set_face_detection(camerasrc_handle_t *phandle,int fd_mode){
    int ret=CAMERASRC_SUCCESS;
    if(!phandle ) {
        GST_ERROR( "%s Null handle",__FUNCTION__);
        return CAMERASRC_ERR_NULL_POINTER;
    }
    if(!phandle->cam_device_handle ) {
        GST_ERROR("phandle->cam_device_handle is null");
        return CAMERASRC_ERR_NULL_POINTER;
        }
    if(fd_mode){
        ret = send_command(phandle->cam_device_handle,CAMERA_CMD_START_FACE_DETECTION, fd_mode, 0);
        if(CAMERASRC_SUCCESS!=ret){
            GST_ERROR( "%s Start face detect fail",__FUNCTION__);
        }
    } else {
        ret = send_command(phandle->cam_device_handle,CAMERA_CMD_STOP_FACE_DETECTION, fd_mode, 0);
        if(CAMERASRC_SUCCESS!=ret){
            GST_ERROR( "%s Stop face detect fail",__FUNCTION__);
        }
    }
    return ret;
}

int
set_dual_camera_mode(GstCameraSrc *camerasrc, int dc_mode){
    int ret=CAMERASRC_SUCCESS;
    ret=gst_qcamerasrc_set_camera_control(camerasrc,
                QCAMERASRC_PARAM_CONTROL_DUAL_CAMERA_MODE,
                dc_mode,
                0,
                NULL);

    if(CAMERASRC_SUCCESS!=ret){
        GST_ERROR( "%s dual camera mode setting failed ",__FUNCTION__);
    }
    GST_ERROR( "%s dual camera mode(%d) set ",__FUNCTION__, dc_mode);
    return ret;
}

int
set_camera_launch_setting(camerasrc_handle_t *phandle, int64_t control_id,qcamerasrc_batch_ctrl_t* control_value){
    GST_INFO("set_batch_camera_parameter:preview start");
        set_batch_camera_parameter(phandle->cam_device_handle,
            control_id,
            control_value);

    return CAMERASRC_SUCCESS;
}

void
get_pixel_format_type(int fourcc, int *pix_format, int *colorspace){
    switch (fourcc) {
        case GST_MAKE_FOURCC ('I', '4', '2', '0'):  //V4L2_PIX_FMT_YUV420
            *pix_format = CAM_FORMAT_YUV_420_YV12;//5TODO:Should be CAM_FORMAT_YUV_420_YU12 waiting QC patch
            *colorspace = CAMERASRC_COL_RAW;
        break;
        case GST_MAKE_FOURCC ('N', 'V', '1', '2'):  //V4L2_PIX_FMT_NV12
        case GST_MAKE_FOURCC ('S', 'N', '1', '2'):  //V4L2_PIX_FMT_NV12
            *pix_format = CAM_FORMAT_YUV_420_NV12;
            *colorspace = CAMERASRC_COL_RAW;
        break;
        case GST_MAKE_FOURCC ('N', 'V', '2', '1'):  //V4L2_PIX_FMT_NV21
            *pix_format = CAM_FORMAT_YUV_420_NV21;
            *colorspace = CAMERASRC_COL_RAW;
        break;
        default:
            *pix_format = CAMERASRC_PIX_NONE;
            *colorspace = CAMERASRC_COL_NONE;
        break;
    }
    return;
}
