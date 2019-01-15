/*
 * qcamera_preview.h
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
#ifndef __QCAMERA_PREVIEW_H__
#define __QCAMERA_PREVIEW_H__

#include <gst/gst.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hardware/camera.h"
#include "gstqcamerasrc_types.h"

typedef void (*preview_cb)(mm_camera_super_buf_t *bufs,void *user_data);
typedef int (*autofocus_callback_t) (void* usr_data, int state);
typedef int (*preview_metadata_callback_t) (void* usr_data, camera_frame_metadata_t *metadata);

typedef struct {
    cam_format_t    preview_format;
    int     preview_width;
    int     preview_height;
//    int     fourcc_format;
    const gchar *format_str;
    int     fps;
    gboolean fps_auto;
    gboolean recording_hint;

    GCond   preview_cond;
    GMutex  preview_mutex;
    GQueue*     preview_frame_queue;
    GMutex  screen_nail_mutex;
    GQueue*     screen_nail_queue;
    preview_cb preview_frame_cb;
    autofocus_callback_t af_cb;
    preview_metadata_callback_t preview_metadata_cb;
    void*   camerasrc;
    camerasrc_handle_t*   qcam_handle;
    int     ion_handle;
#ifdef CONFIG_MACH_Z3LTE
    gboolean low_light_mode;
#endif
}qcamera_preview_hw_intf;

qcamera_preview_hw_intf*
initialize_preview(void* camerasrc,preview_cb cb_fn);
gboolean
start_preview(qcamera_preview_hw_intf* preview_stream);
void
set_preview_flip(qcamera_preview_hw_intf* preview_stream,int flip_mode);
gboolean
stop_preview(qcamera_preview_hw_intf* preview_stream);
void
done_preview_frame(qcamera_preview_hw_intf* preview_stream,void* opaque);
void
preview_data_cb(const camera_memory_t *mem,void *user_data);
void
screen_nail_data_cb(camera_memory_t *mem,void *user_data);
void
free_preview(qcamera_preview_hw_intf* preview_stream);
gboolean
frame_available(qcamera_preview_hw_intf* preview_stream);
void
wait_for_frame(qcamera_preview_hw_intf* preview_stream);
GstBuffer *
get_preview_frame(qcamera_preview_hw_intf* preview_stream);
void
cache_preview_frame(qcamera_preview_hw_intf* preview_stream,GstBuffer * buffer);
void
signal_flush_frame(qcamera_preview_hw_intf* preview_stream);
void
get_preview_stream_mutex(qcamera_preview_hw_intf* preview_stream);
void
release_preview_stream_mutex(qcamera_preview_hw_intf* preview_stream);
camera_memory_t *
get_screen_nail_frame(qcamera_preview_hw_intf* preview_stream);
void
cache_screen_nail_frame(qcamera_preview_hw_intf* preview_stream,camera_memory_t * buffer);
void
free_cached_screen_nail_frame(camera_memory_t* frame);
#endif /* __QCAMERA_PREVIEW_H__ */
