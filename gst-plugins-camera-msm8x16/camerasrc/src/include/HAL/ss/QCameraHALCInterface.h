/*
**
** Copyright (c) 2012-2013, The Linux Foundation. All rights reserved.
** Not a Contribution. Apache license notifications and license are
** retained for attribution purposes only.
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#ifndef __QCAMERAHAL_CINTERFACE_H__
#define __QCAMERAHAL_CINTERFACE_H__

#include <stdint.h>
#include "gstqcamerasrc_defs.h"

struct QCameraHALCInterface;
typedef struct QCameraHALCInterface QCameraHALCInterface;

#ifdef __cplusplus
QCAMERAHAL_CPPINTF_DECL_START
#endif

int
get_number_of_cameras(void);
int
get_camera_info(int camera_id, void **info);
int
camera_device_open(int camera_id, void **hw_device_data,void**cam_dev_handle,
                                                void* destroy_cb,void* user_data);
int
camera_device_close(void *info);
int
set_preview_window(void* cam_dev,void* window);
void
enable_msg_type(void* cam_dev, int msg_type);
void
disable_msg_type(void* cam_dev, int msg_type);
int
msg_type_enabled(void* cam_dev, int msg_type);
int
start_camera_preview(void* cam_dev,void* preview_cb,void* user_data);
void
stop_camera_preview(void* cam_dev);
int
preview_enabled(void* cam_dev);
int
store_meta_data_in_buffers(void* cam_dev, int enable);
int
start_recording(void* cam_dev,void* record_cb,void* user_data);
void
stop_recording(void* cam_dev);
int
recording_enabled(void* cam_dev);
void
release_recording_frame(void* cam_dev, const void *opaque);
void
release_preview_frame(void* cam_dev, const void *opaque);
int
auto_focus(void* cam_dev);
int
cancel_auto_focus(void* cam_dev);
int
take_picture(void* cam_dev);

int
take_picture_internal(void* cam_dev);


int
cancel_picture(void* cam_dev);
int
set_parameters(void* cam_dev, const char *parms);
char*
get_parameters(void* cam_dev);
void
put_parameters(void* cam_dev, char * parms);
int
send_command(void* cam_dev,int cmd,int arg1,int arg2);
void
release(void* cam_dev);
int
dump(void* cam_dev,int fd);
void
set_CallBacks(void* cam_dev,void* notify_cb,void* data_cb,
                                void* data_cb_timestamp,void* get_memory,void *user_data);
int
set_camera_parameter(void* cam_dev,int64_t control,int value_1,int value_2,char * string_1);
int
get_camera_parameter(void* cam_dev,int64_t control,int* value_1,int* value_2,char * string_1);
int
set_batch_camera_parameter(void* cam_dev,int64_t control,qcamerasrc_batch_ctrl_t* control_data);

#ifdef __cplusplus
QCAMERAHAL_CPPINTF_DECL_END
#endif

#endif//__QCAMERAHAL_CINTERFACE_H__