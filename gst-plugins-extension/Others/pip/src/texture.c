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

#include "texture.h"

static int _drm_init (AppData *app_data)
{
	startfunc

	if (!app_data || !app_data->display) {
		GST_ERROR("app_data is null");
        return -1;
	}

	int dri2_ev_base = 0;
	int dri2_err_base = 0;
	int dri2_major = 0;
	int dri2_minor = 0;
	char *drv_name = NULL;
	char *dev_name = NULL;
	drm_magic_t magic = 0;
	int fd = -1;

	if(!DRI2QueryExtension(app_data->display, &dri2_ev_base, &dri2_err_base)) {
		GST_ERROR("Failed to get dri2 extension");
		return -1;
	}

	if(!DRI2QueryVersion(app_data->display, &dri2_major, &dri2_minor )) {
		GST_ERROR("Failed to get dri2 version");
		return -1;
	}

	if(!DRI2Connect(app_data->display, app_data->window, &drv_name, &dev_name )) {
		GST_ERROR("Failed to get dri2 version");
		return -1;
	}

	GST_WARNING("open [%s]", dev_name);

	fd = open(dev_name, O_RDWR);

	IF_FREE(drv_name);
	IF_FREE(dev_name);

	if (fd < 0) {
		GST_ERROR("[DRM] couldn't open");
		return -1;
	}

	GST_WARNING("opened fd [%d]", fd);

	if(drmGetMagic(fd, &magic)) {
		GST_ERROR("[DRM] can't get magic");
		close(fd);
		return -1;
	}

	if (!DRI2Authenticate(app_data->display, app_data->window, (unsigned int)magic)) {
		GST_ERROR("authentication failed");
		close(fd);
		return -1;
	}

	endfunc

	return fd;
}

void create_window(AppData *app_data)
{
	startfunc

	if (!app_data) {
		GST_WARNING("app_data is null");
		return;
	}

	/*create Window*/
	app_data->display = XOpenDisplay ( 0 );
	app_data->window = XDefaultRootWindow(app_data->display);

	GST_WARNING("XDisplay: %d, XWindow: %d", (int)app_data->display, (int)app_data->window);

	endfunc

	return;
}

void destroy_window(AppData *app_data)
{
	startfunc

	if (!app_data) {
		GST_ERROR("app_data is null");
		return;
	}

	GST_WARNING("XDestroyWindow display %d, window %d",
	            (int)app_data->display, (int)app_data->window);

	XDestroyWindow(app_data->display, app_data->window);
	XCloseDisplay(app_data->display);

	endfunc

	return;
}

static void reset_flag(AppData *app_data)
{
	startfunc

	if (!app_data || !app_data->manager) {
		GST_ERROR("AppData is null");
		return;
	}

	app_data->has_created_glm = FALSE;
	app_data->ready_for_glm = FALSE;
	app_data->has_setted_ibp = FALSE;

	endfunc

	return;
}

static gint get_max_buf_id(AppData *app_data)
{
	if (!app_data) {
		GST_ERROR("AppData is null");
		return BUF_0;
	}
	gint max = BUF_1;
	if (is_dual_camera_mode(app_data)) {
		max = BUFFER_MAX;
	}
	return max;
}

static gboolean input_queue_is_empty(AppData *app_data)
{
	if (!app_data) {
		return TRUE;
	}

	if (is_dual_camera_mode(app_data)) {
		return (g_queue_is_empty(app_data->input[BUF_0]) &&
			g_queue_is_empty(app_data->input[BUF_1]));
	} else {
		return (g_queue_is_empty(app_data->input[BUF_0]));
	}
}

static gint get_capture_buf_id(GstBuffer *buf[BUFFER_MAX])
{
	gint i = 0;
	MMVideoBuffer*imgb = NULL;
	for (i = BUF_0; i < BUFFER_MAX; i++) {
		if (!buf[i]) {
			GST_LOG("buf : %d is null", i);
			return -1;
		}
		GstMapInfo mapInfo;
		GstMemory *img_buf_memory = NULL;
		if (buf[i] && gst_buffer_n_memory(buf[i]) > 1){
			img_buf_memory = gst_buffer_peek_memory(buf[i],1);
			gst_memory_map(img_buf_memory,&mapInfo,GST_MAP_READ);
			imgb = (MMVideoBuffer *)mapInfo.data;
			gst_memory_unmap(img_buf_memory,&mapInfo);
		}
		if (imgb != NULL && imgb->jpeg_data != NULL) {
			GST_LOG("buf : %d comes from capture JPEG", i);
			return i;
		}
	}

	return -1;
}


static void gl_manager_set_style(TizenGLManager_t *manager, AppData *app_data)
{
	if (!app_data || !manager) {
		GST_ERROR("AppData or TizenGLManager is null");
		return;
	}

	GST_LOG("style_type is : %d", app_data->style_type);

	TizenGLManager_setFilter(manager, app_data->style_type+SI_KEY_FILTER_VALUE_GD_NORMAL_PIP);
	/* enable fading */
	TizenGLManager_setOptionI(manager, "fading", TRUE);

	return;
}


static void gl_manager_set_option_coordinates(TizenGLManager_t *manager, RectPosition *inset_rect, RectPosition *surface_rect)
{
	if (!inset_rect || !surface_rect || !manager) {
		GST_ERROR("TizenGLManager or rect is null");
        return;
	}

	gchar *value = g_strdup_printf("x1=%d;y1=%d;x2=%d;y2=%d;surfaceWidth=%d;surfaceHeight=%d",
	                               inset_rect->x, inset_rect->y,
	                               inset_rect->x + inset_rect->w,
	                               inset_rect->y + inset_rect->h,
	                               surface_rect->w, surface_rect->h);

	if (value) {
		GST_WARNING("coordinates value is : %s", value);
		TizenGLManager_setOption (manager, "coordinates", value);
		g_free(value);
		value = NULL;
	} else {
		GST_ERROR("g_strdup_printf failed");
	}

	return;
}

#ifdef USE_CROP_OPTION
static void gl_manager_set_option_crop(TizenGLManager_t *manager, AppData *app_data)
{
	if (!app_data || !manager) {
		GST_ERROR("AppData or TizenGLManager is null");
		return;
	}

	gchar *value = g_strdup_printf("x1=%d;y1=%d;x2=%d;y2=%d;surfaceWidth=%d;surfaceHeight=%d",
	                                app_data->inset_window_preview_crop_x, app_data->inset_window_preview_crop_y,
	                                app_data->inset_window_preview_crop_x + app_data->inset_window_preview_crop_w,
	                                app_data->inset_window_preview_crop_y + app_data->inset_window_preview_crop_h,
	                                app_data->width[app_data->inset_window_device],
	                                app_data->height[app_data->inset_window_device]);

	if (value) {
		GST_WARNING("crop value is : %s", value);
		TizenGLManager_setOption (manager, "crop", value);
		g_free(value);
		value = NULL;
	} else {
		GST_ERROR("g_strdup_printf failed");
	}

	return;
}
#endif /* USE_CROP_OPTION */


static void gl_manager_set_option_orientation(TizenGLManager_t *manager, AppData *app_data)
{
	if (!app_data || !manager) {
		GST_ERROR("AppData or TizenGLManager is null");
		return;
	}

	GST_WARNING("orientation is : %d", app_data->orientation);

	TizenGLManager_setOptionI (manager, "orientation", app_data->orientation);

	return;
}


static void gl_manager_set_option_inset_window_device(TizenGLManager_t *manager, AppData *app_data)
{
	if (!app_data || !manager) {
                GST_ERROR("AppData or TizenGLManager is null");
		return;
	}

	/* inset window device - 0: Rear camera, 1: Front camera
	 * layerorder - 0: Rear camera is BACK, 1: Front camera is BACK
	 */

	GST_WARNING("inset_window_device is : %d", app_data->inset_window_device);

	TizenGLManager_setOptionI(manager, "layerorder", !(app_data->inset_window_device));

	return;
}

static void gl_manager_set_camera_mode(TizenGLManager_t *manager, AppData *app_data)
{
	if (!app_data || !manager) {
                GST_ERROR("AppData or TizenGLManager is null");
		return;
	}

	GST_WARNING("camera_mode is : %d", app_data->camera_mode);

	if (app_data->camera_mode == DUAL_CAMERA) {
		TizenGLManager_setCameraMode (manager, SI_VAL_DUAL_CAMERA);
	} else {
		GST_WARNING("set camera_mode REAR mode although FRONT");
		TizenGLManager_setCameraMode (manager, SI_VAL_REAR_CAMERA);
	}

	return;
}

static void gl_manager_set_inset_window_hide(TizenGLManager_t *manager, AppData *app_data)
{
	if (!app_data || !manager) {
				GST_ERROR("AppData or TizenGLManager is null");
		return;
	}

	GST_WARNING("set inset window hide : %d", app_data->hide_inset_window);

	TizenGLManager_setOptionI(manager, "fronthide", app_data->hide_inset_window);

	return;
}

static void gl_manager_set_effect(TizenGLManager_t *manager, AppData *app_data)
{
	if (!app_data || !manager) {
		GST_ERROR("AppData or TizenGLManager is null");
		return;
	}

	GST_WARNING("single_camera_effect is : %d", app_data->single_camera_effect);

	if(FILTER_VALUE_GS_DOWNLOAD_EFFECT != app_data->single_camera_effect) {
		TizenGLManager_setFilter (manager, app_data->single_camera_effect+SI_KEY_FILTER_VALUE_GS_NO_EFFECT);
	} else if (FILTER_VALUE_GS_DOWNLOAD_EFFECT == app_data->single_camera_effect) {
		TizenGLManager_setFilterExternal (manager, app_data->single_camera_download_effect);
	}

	/* enable fading */
	TizenGLManager_setOptionI(manager, "fading", TRUE);

	return;
}

static void gl_manager_set_fading(TizenGLManager_t *manager, AppData *app_data)
{
	if (!app_data || !manager) {
		GST_ERROR("AppData or TizenGLManager is null");
		return;
	}

	GST_WARNING("gl_manager_set_fading is : %d", app_data->enable_fading);

	/* enable fading */
	TizenGLManager_setOptionI(manager, "fading", app_data->enable_fading);

	return;
}

static void gl_manager_set_option_glue_roi(TizenGLManager_t *manager, AppData *app_data)
{
	RectPosition inset_rect = {0, 0, 0, 0};
	RectPosition surface_rect = {0, 0, 0, 0};
	int roi_x_1 = 0;
	int roi_y_1 = 0;
	int roi_x_2 = 0;
	int roi_y_2 = 0;
	int roi_x_3 = 0;
	int roi_y_3 = 0;
	int roi_x_4 = 0;
	int roi_y_4 = 0;

	if (!manager || !app_data) {
		GST_ERROR("TizenGLManager or rect is null");
        return;
	}

	if (manager == app_data->capture_manager) {
		/* for capture GL manager */
		float multiple_w = (float)app_data->capture_w/(float)app_data->width[0];
		float multiple_h = (float)app_data->capture_h/(float)app_data->height[0];

		inset_rect.x = (float)app_data->roi_info.startxy.xAxis * multiple_w;
		inset_rect.y = (float)app_data->roi_info.startxy.yAxis * multiple_h;
		inset_rect.w = (float)app_data->roi_info.width * multiple_w;
		inset_rect.h = (float)app_data->roi_info.height * multiple_h;
		surface_rect.w = (float)app_data->capture_w;
		surface_rect.h = (float)app_data->capture_h;
		roi_x_1 = (float)app_data->roi_info.roi_left_top.xAxis * multiple_w;
		roi_y_1 = (float)app_data->roi_info.roi_left_top.yAxis * multiple_h;
		roi_x_2 = (float)app_data->roi_info.roi_right_top.xAxis * multiple_w;
		roi_y_2 = (float)app_data->roi_info.roi_right_top.yAxis * multiple_h;

		roi_x_3 = (float)app_data->roi_info.roi_right_bottom.xAxis * multiple_w;
		roi_y_3 = (float)app_data->roi_info.roi_right_bottom.yAxis * multiple_h;
		roi_x_4 = (float)app_data->roi_info.roi_left_bottom.xAxis * multiple_w;
		roi_y_4 = (float)app_data->roi_info.roi_left_bottom.yAxis * multiple_h;

		GST_WARNING("capture_inset %d %d %dx%d, surface %dx%d, multiple w[%f],h[%f]",
			            inset_rect.x, inset_rect.y, inset_rect.w, inset_rect.h,
			            surface_rect.w, surface_rect.h, multiple_w, multiple_h);

		GST_WARNING("capture_inset roi_point1(%d, %d), roi_point2(%d, %d), roi_point3 (%d, %d), roi_point4 (%d, %d)",
			            roi_x_1, roi_y_1, roi_x_2, roi_y_2,
			            roi_x_3, roi_y_3, roi_x_4, roi_y_4);
	} else {
		/* for preview GL manager */
		inset_rect.x = app_data->roi_info.startxy.xAxis;
		inset_rect.y = app_data->roi_info.startxy.yAxis;
		inset_rect.w = app_data->roi_info.width;
		inset_rect.h = app_data->roi_info.height;
		surface_rect.w = app_data->width[0];
		surface_rect.h = app_data->height[0];
		roi_x_1 = app_data->roi_info.roi_left_top.xAxis;
		roi_y_1 = app_data->roi_info.roi_left_top.yAxis;
		roi_x_2 = app_data->roi_info.roi_right_top.xAxis;
		roi_y_2 = app_data->roi_info.roi_right_top.yAxis;

		roi_x_3 = app_data->roi_info.roi_right_bottom.xAxis;
		roi_y_3 = app_data->roi_info.roi_right_bottom.yAxis;
		roi_x_4 = app_data->roi_info.roi_left_bottom.xAxis;
		roi_y_4 = app_data->roi_info.roi_left_bottom.yAxis;

		GST_WARNING("normal_inset %d %d %dx%d, surface %dx%d",
			            inset_rect.x, inset_rect.y, inset_rect.w, inset_rect.h,
			            surface_rect.w, surface_rect.h);

		GST_WARNING("normal_inset roi_point1(%d, %d), roi_point2(%d, %d), roi_point3 (%d, %d), roi_point4 (%d, %d)",
			            roi_x_1, roi_y_1, roi_x_2, roi_y_2,
			            roi_x_3, roi_y_3, roi_x_4, roi_y_4);
	}

	gchar *value = g_strdup_printf("x1=%d;y1=%d;x2=%d;y2=%d;surfaceWidth=%d;surfaceHeight=%d;rx1=%d;ry1=%d;rx2=%d;ry2=%d;rx3=%d;ry3=%d;rx4=%d;ry4=%d;stopRender=%d",
	                               inset_rect.x, inset_rect.y,
	                               inset_rect.x + inset_rect.w,
	                               inset_rect.y + inset_rect.h,
	                               surface_rect.w, surface_rect.h, roi_x_1, roi_y_1, roi_x_2,roi_y_2,roi_x_3,roi_y_3,roi_x_4,roi_y_4, app_data->roi_info.stop_render);

	if (value) {
		TizenGLManager_setOption (manager, "roi_coordinates", value);
		g_free(value);
		value = NULL;
	} else {
		GST_ERROR("g_strdup_printf failed");
	}

	return;
}



static void gl_manager_set_options(TizenGLManager_t *manager, AppData *app_data)
{
	RectPosition inset_rect = {0, 0, 0, 0};
	RectPosition surface_rect = {0, 0, 0, 0};

	if (!app_data || !manager) {
		GST_ERROR("AppData or TizenGLManager is null");
		return;
	}

	if (manager == app_data->capture_manager) {
		/* for capture GL manager */
		float multiple_w = (float)app_data->capture_w/(float)app_data->width[0];
		float multiple_h = (float)app_data->capture_h/(float)app_data->height[0];

		inset_rect.x = app_data->inset_window_x * multiple_w;
		inset_rect.y = app_data->inset_window_y * multiple_h;
		inset_rect.w = app_data->inset_window_w * multiple_w;
		inset_rect.h = app_data->inset_window_h * multiple_h;
		surface_rect.w = app_data->capture_w;
		surface_rect.h = app_data->capture_h;

		GST_WARNING("inset %d %d %dx%d, surface %dx%d, multiple w[%f],h[%f]",
		            inset_rect.x, inset_rect.y, inset_rect.w, inset_rect.h,
		            surface_rect.w, surface_rect.h, multiple_w, multiple_h);
	} else {
		/* for preview GL manager */
		inset_rect.x = app_data->inset_window_x;
		inset_rect.y = app_data->inset_window_y;
		inset_rect.w = app_data->inset_window_w;
		inset_rect.h = app_data->inset_window_h;
		surface_rect.w = app_data->width[0];
		surface_rect.h = app_data->height[0];
	}

	gl_manager_set_camera_mode(manager, app_data);

	if(is_dual_camera_mode(app_data)) {
		gl_manager_set_style (manager, app_data);
		GST_WARNING("pip style is %d",app_data->style_type);
		if ( app_data->style_type == FILTER_VALUE_GD_TRACKING ) {
			gl_manager_set_option_glue_roi(manager, app_data);
		} else {
			gl_manager_set_option_coordinates (manager, &inset_rect, &surface_rect);
		}
#ifdef USE_CROP_OPTION
		gl_manager_set_option_crop (manager, app_data);
#endif /* USE_CROP_OPTION */
		gl_manager_set_option_orientation (manager, app_data);
		gl_manager_set_option_inset_window_device (manager, app_data);
		gl_manager_set_inset_window_hide(manager, app_data);
	} else {
		gl_manager_set_effect(manager, app_data);
	}

	if (manager == app_data->capture_manager) {
		GST_WARNING("Enable Fading capture_manager: %d", app_data->enable_fading);
		/* disable fading */
		TizenGLManager_setOptionI(manager, "fading", FALSE);
	} else {
		GST_WARNING("Enable Fading : %d", app_data->enable_fading);
		/* enable fading */
		TizenGLManager_setOptionI(manager, "fading", app_data->enable_fading);
	}

	return;
}


static gboolean gl_manager_update_options(AppData *app_data)
{
	if (!app_data || !app_data->manager) {
		GST_ERROR("gl_manager_update_options: AppData is null");
		return FALSE;
	}

	int index = 0;
	gboolean flag = FALSE;

	for (index = PROC_IW; index < PROC_MAX; index++) {
		if (app_data->property_change[index]) {
			IF_G_MUTEX_LOCK (app_data->thread_mutex[MU_PRO_CHANGE]);

			GST_LOG("gl_manager_update_options: Update options: %d", index);

			if(PROC_IW == index){
				RectPosition inset_rect = {0, 0, 0, 0};
				RectPosition surface_rect = {0, 0, 0, 0};

				inset_rect.x = app_data->inset_window_x;
				inset_rect.y = app_data->inset_window_y;
				inset_rect.w = app_data->inset_window_w;
				inset_rect.h = app_data->inset_window_h;
				surface_rect.w = app_data->width[0];
				surface_rect.h = app_data->height[0];

				gl_manager_set_option_coordinates(app_data->manager, &inset_rect, &surface_rect);
			} else if(PROC_IWPC == index){
#ifdef USE_CROP_OPTION
				gl_manager_set_option_crop(app_data->manager, app_data);
#endif /* USE_CROP_OPTION */
			} else if(PROC_STYLE == index){
				gl_manager_set_options (app_data->manager, app_data);
			} else if(PROC_ORIEN == index){
				gl_manager_set_option_orientation(app_data->manager, app_data);
			} else if(PROC_IWD == index){
				gl_manager_set_option_inset_window_device(app_data->manager, app_data);
			} else if(PROC_CM == index){
				gl_manager_set_camera_mode (app_data->manager, app_data);
			} else if(PROC_EFFECT == index){
				gl_manager_set_effect (app_data->manager, app_data);
			} else if(PROC_DOWNLOAD_EFFECT == index){
				gl_manager_set_effect (app_data->manager, app_data);
			} else if(PROC_HIW == index){
				gl_manager_set_inset_window_hide (app_data->manager, app_data);
			} else if(PROC_FADING == index){
				gl_manager_set_fading (app_data->manager, app_data);
			} else if(PROC_ROI_INFO == index) {
				gl_manager_set_option_glue_roi(app_data->manager, app_data);
			}
			app_data->property_change[index] = FALSE;
			app_data->property_updated = TRUE;
			flag = TRUE;

			IF_G_MUTEX_UNLOCK (app_data->thread_mutex[MU_PRO_CHANGE]);
		}
	}

	return flag;
}

void create_gl_manager(AppData *app_data)
{
	startfunc

	int i = 0;

	/*create GLManager*/
	app_data->manager = TizenGLManager_create(app_data->width[0], app_data->height[0], app_data->display, app_data->window);
	if(!app_data->manager) {
		GST_ERROR("TizenGLManager_create failed");
		return;
	}

	/* set size */
	if (is_dual_camera_mode(app_data)) {
		TizenGLManager_setPreviewSize(app_data->manager, app_data->width[0], app_data->height[0], app_data->width[1], app_data->height[1]);
	}

	gl_manager_set_options(app_data->manager, app_data);

	for (i = 0; i < SURFACE_COUNT; i++) {
		GST_WARNING("TizenGLManager_setNativeBufferSurface buffer 0x%x", app_data->surface_buffer[i]);
		TizenGLManager_setNativeBufferSurface(app_data->manager, app_data->surface_buffer[i]);
	}

	app_data->has_created_glm = TRUE;

	endfunc

	return;
}


static CaptureData *alloc_and_copy_CaptureData(gint size, void *data, int type)
{
	CaptureData *capture_data = NULL;

	if (!data || size <= 0) {
		GST_ERROR("invalid input parameter");
		return NULL;
	}

	capture_data = (CaptureData*)malloc(sizeof(CaptureData));
	if (!capture_data) {
		GST_ERROR("Allocate CaptureData failed");
		return NULL;
	}

	memset(capture_data, 0x0, sizeof(CaptureData));
	capture_data->size = size;
	capture_data->data = malloc(size);
	if (!capture_data->data) {
		GST_ERROR("Allocate data[size %d] failed", size);
		free(capture_data);
		return NULL;
	}

	GST_WARNING("start memcpy src %p dst %p size %d, type %d",
	            data, capture_data->data, size, type);

	memcpy(capture_data->data, data, size);
	capture_data->type = type;

	GST_INFO("memcpy done");

	return capture_data;
}


static CaptureJpegData *alloc_and_copy_CaptureJpegData(gint size, void *data)
{
	CaptureJpegData *jpegData = NULL;

	if (!data || size <= 0) {
		GST_ERROR("invalid input parameter");
		return NULL;
	}

	jpegData = (CaptureJpegData*)malloc(sizeof(CaptureJpegData));
	if (!jpegData) {
		GST_ERROR("Allocate CaptureJpegData failed");
		return NULL;
	}

	memset(jpegData, 0x0, sizeof(CaptureJpegData));
	jpegData->size = size;
	jpegData->data = malloc(size);
	if (!jpegData->data) {
		GST_ERROR("Allocate data[size %d] failed", size);
		free(jpegData);
		return NULL;
	}

	GST_WARNING("start memcpy src %p dst %p size %d", data, jpegData->data, size);

	memcpy(jpegData->data, data, size);

	GST_INFO("memcpy done");

	return jpegData;
}


static CaptureRawData *alloc_and_copy_CaptureRawData(GstBuffer *buffer)
{
	if (!buffer || !gst_buffer_n_memory(buffer)) {
		GST_ERROR("invalid buffer %p", buffer);
		return NULL;
	}

	CaptureRawData *rawData = (CaptureRawData*)malloc(sizeof(CaptureRawData));
	if (!rawData) {
		GST_ERROR("Allocate CaptureRawData failed");
		return NULL;
	}

	memset(rawData, 0x0, sizeof(CaptureRawData));
	GstMapInfo map;
	gst_buffer_map(buffer,&map,GST_MAP_READ);
	rawData->size = map.size;

	if (rawData->size <= 0) {
		IF_FREE(rawData);
		GST_ERROR("invalid size %d", rawData->size);
		gst_buffer_unmap(buffer,&map);
		return NULL;
	}

	rawData->data = malloc(rawData->size);
	if (!rawData->data) {
		GST_ERROR("Allocate data[size %d] failed", rawData->size);
		free(rawData);
		gst_buffer_unmap(buffer,&map);
		return NULL;
	}
	memcpy(rawData->data, map.data, map.size);
	gst_buffer_unmap(buffer,&map);

	return rawData;
}


void destroy_CaptureJpegData(gpointer data)
{
	CaptureJpegData *jpegData = (CaptureJpegData*)data;

	if (!jpegData) {
		GST_WARNING("jpegData is already NULL");
		return;
	}

	IF_FREE(jpegData->data);
	IF_FREE(jpegData);

	return;
}


void destroy_CaptureRawData(gpointer data)
{
	CaptureRawData *rawData = (CaptureRawData*)data;

	if (!rawData) {
		GST_WARNING("rawData is already NULL");
		return;
	}

	IF_FREE(rawData->data);
    IF_FREE(rawData);

	return;
}


void destroy_OutputData(gpointer data)
{
	/*unref input gstbuffer*/
	OutputData *output = (OutputData *)data;

        if (!output) {
		GST_WARNING("output is already NULL");
		return;
	}

    IF_FREE(output);

	return;
}

gint get_available_pixmap_id(AppData *app_data)
{
        gint i = 0;
	gint id = -1;

	if (!app_data) {
		GST_ERROR("AppData is null");
		return id;
	}

	for (i = 0 ; i < SURFACE_COUNT ; i++) {
		if (app_data->surface_using[i] == FALSE) {
			app_data->surface_using[i] = TRUE;
			id = i;
			break;
		}
	}

	return id;
}


static gpointer handle_buffer_thread_cb (gpointer data)
{
	startfunc

        gint i = 0;
	gint max_id = 0;
	gint j = 0;
	AppData *app_data = (AppData *)data;
	if (!app_data) {
		GST_ERROR("AppData is null");
		return NULL;
	}

	IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_INPUT_QUEUE]);

	if (!app_data->has_created_glm &&
	    app_data->width[0] > 0 && app_data->height[0] > 0) {
		GST_WARNING("init_gl_manager call");
		init_gl_manager(app_data);
		GST_WARNING("create_gl_manager call");
		create_gl_manager(app_data);
	}

	while (!app_data->exit_loop[TH_HANDLE_BUF]) {
		int available_pixmap_id = -1;
		tbm_surface_h surface_output = NULL;
		MMVideoBuffer *imgb[2] = {NULL, NULL};
		tbm_bo input_bo[2] = {NULL, NULL};
		tbm_surface_h input_buffer[2] = {NULL,NULL};
		unsigned int gem_handle[2] = {0, 0};
		int ret = 0;
		GstBuffer *buf[BUFFER_MAX];

		if (input_queue_is_empty(app_data)) {
			GTimeVal abstimeout;
			g_get_current_time(&abstimeout);
			g_time_val_add(&abstimeout, PIP_WAIT_TIMEOUT);
			GST_LOG("mu input list is empty. wait capture signal...");
			if(!g_cond_timed_wait(app_data->thread_cond[MU_INPUT_QUEUE],
						app_data->thread_mutex[MU_INPUT_QUEUE],&abstimeout)) {
				GST_LOG("mu input queue signal time out , 1 sec");
			} else {
				GST_LOG("mu input signal received, check again...");
			}
			continue;
		}

		max_id = get_max_buf_id(app_data);
		for (i = 0; i < max_id; i++) {
			buf[i] = g_queue_pop_head(app_data->input[i]);
		}

		IF_G_MUTEX_UNLOCK (app_data->thread_mutex[MU_INPUT_QUEUE]);

		if (!buf[0] && !buf[1]) {
			GST_WARNING("buf is null ...");
			IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_INPUT_QUEUE]);
			continue;
		}

		if (!app_data->has_created_glm) {
			GST_WARNING("init_gl_manager call");
			init_gl_manager(app_data);
			GST_WARNING("create_gl_manager call");
			create_gl_manager(app_data);
		}

		/*update options for setting property*/
		gl_manager_update_options(app_data);

		/*push buffer to capture queue*/
		if (is_dual_camera_mode(app_data)) {
			gint capture_buf_id = get_capture_buf_id(buf);
			if (capture_buf_id != -1) {
				MMVideoBuffer *scmn_imgb = NULL;
				CaptureJpegData *jpegData = NULL;
				CaptureRawData *rawData = NULL;

				GST_WARNING("DUAL CAMERA: This input buf is capture buf");

				IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_CAPTURE_QUEUE]);

				if (app_data->inset_window_device == FRONT_CAMERA) {
					/*Copy jpeg data*/
					GstMapInfo mapInfo;
					GstMemory *img_buf_memory = NULL;
					if (buf[capture_buf_id] && gst_buffer_n_memory(buf[capture_buf_id]) > 1){
						img_buf_memory = gst_buffer_peek_memory(buf[capture_buf_id],1);
						gst_memory_map(img_buf_memory,&mapInfo,GST_MAP_READ);
						scmn_imgb = (MMVideoBuffer *)mapInfo.data;
						gst_memory_unmap(img_buf_memory,&mapInfo);
					}
					/*
					jpegData = alloc_and_copy_CaptureJpegData(scmn_imgb->jpeg_size,
					                                          scmn_imgb->jpeg_data);
					*/
					jpegData = (CaptureJpegData*)malloc(sizeof(CaptureJpegData));
					if (!jpegData) {
						GST_ERROR("Allocate jpegData failed");
						jpegData = NULL;
					} else {
						memset(jpegData, 0x0, sizeof(CaptureJpegData));
						jpegData->data = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->data;
						jpegData->size = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->size;
						jpegData->width = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->width;
						jpegData->height = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->height;
						jpegData->fourcc = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->fourcc;
						free(scmn_imgb->jpeg_data);
						scmn_imgb->jpeg_data = NULL;
						scmn_imgb->jpeg_size = 0;
						jpegData->type = CAPTURE_DATA_JPEG;
					}
					g_queue_push_tail(app_data->capture_buffer_list[BUF_0], jpegData);
				} else {
					/* copy raw data */
					rawData = alloc_and_copy_CaptureRawData(buf[capture_buf_id]);
					g_queue_push_tail(app_data->capture_buffer_list[BUF_0], rawData);
				}

				for (i = BUF_0 ; i < max_id ; i++) {
					if (capture_buf_id != i){
						rawData = alloc_and_copy_CaptureRawData(buf[i]);
						g_queue_push_tail(app_data->capture_buffer_list[BUF_1], rawData);
						break;
					}
				}

				GST_WARNING("push capture buffer queue done");

				IF_G_COND_SIGNAL(app_data->thread_cond[MU_CAPTURE_QUEUE]);
				IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_CAPTURE_QUEUE]);
			}
		} else {
			MMVideoBuffer *scmn_imgb = NULL;
			GstMapInfo mapInfo;
			GstMemory *img_buf_memory = NULL;
			if (buf[0] && gst_buffer_n_memory(buf[0]) > 1){
				img_buf_memory = gst_buffer_peek_memory(buf[0],1);
				gst_memory_map(img_buf_memory,&mapInfo,GST_MAP_READ);
				scmn_imgb = (MMVideoBuffer *)mapInfo.data;
				gst_memory_unmap(img_buf_memory,&mapInfo);
			}
			if (scmn_imgb->jpeg_data) {
				CaptureJpegData *jpegData = NULL;
				GST_WARNING("jpeg_data is come");

				IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_CAPTURE_QUEUE]);

				/*Copy JPEG data*/
				/*
				capture_data = alloc_and_copy_CaptureData(scmn_imgb->jpeg_size,
				                                          scmn_imgb->jpeg_data,
				                                          CAPTURE_DATA_JPEG);
				*/
				jpegData = (CaptureJpegData*)malloc(sizeof(CaptureJpegData));
				if (!jpegData) {
					GST_ERROR("Allocate jpegData failed");
					jpegData = NULL;
				} else {
					memset(jpegData, 0x0, sizeof(CaptureData));
					jpegData->data = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->data;
					jpegData->size = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->size;
					jpegData->width = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->width;
					jpegData->height = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->height;
					jpegData->fourcc = ((capture_buffer_info_t *)scmn_imgb->jpeg_data)->fourcc;
					free(scmn_imgb->jpeg_data);
					scmn_imgb->jpeg_data = NULL;
					scmn_imgb->jpeg_size = 0;
					jpegData->type = CAPTURE_DATA_JPEG;
				}
				g_queue_push_tail(app_data->capture_buffer_list[BUF_0], jpegData);

				GST_WARNING("push capture buffer[JPEG data] queue done");

				IF_G_COND_SIGNAL(app_data->thread_cond[MU_CAPTURE_QUEUE]);
				IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_CAPTURE_QUEUE]);
			} else if (app_data->signal_still_capture) {
				CaptureData *capture_data = NULL;
				GST_WARNING("signal_still_capture is set");

				IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_CAPTURE_QUEUE]);

				GstMapInfo mapInfo;
				gst_memory_map(buf[0],&mapInfo,GST_MAP_READ);
				capture_data = alloc_and_copy_CaptureData(mapInfo.size,
				                                          mapInfo.data,
				                                          CAPTURE_DATA_RAW);
				gst_memory_unmap(buf[0],&mapInfo);
				g_queue_push_tail(app_data->capture_buffer_list[BUF_0], capture_data);
				GST_WARNING("push capture buffer[RAW data] queue done");

				IF_G_COND_SIGNAL(app_data->thread_cond[MU_CAPTURE_QUEUE]);
				IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_CAPTURE_QUEUE]);
				app_data->signal_still_capture = FALSE;
			}
		}

		GST_DEBUG("handling buffer thread run");
		if(buf[0] == NULL){
			GST_WARNING("buf[0] is null");
			goto RELEASE_RESOURCE;
		}

		MMVideoBuffer *scmn_imgb = NULL;
		GstMapInfo mapInfo;
		GstMemory *img_buf_memory = NULL;
		if (buf[0] && gst_buffer_n_memory(buf[0]) > 1){
			img_buf_memory = gst_buffer_peek_memory(buf[0],1);
			gst_memory_map(img_buf_memory,&mapInfo,GST_MAP_READ);
			imgb[0] = (MMVideoBuffer *)mapInfo.data;
			gst_memory_unmap(img_buf_memory,&mapInfo);
		}
		if(imgb[0] == NULL){
			GST_WARNING("imgb[0] is null");
			goto RELEASE_RESOURCE;
		}

		if(is_dual_camera_mode(app_data)) {
			if(buf[1] == NULL){
				GST_WARNING("buf[1] is null");
				goto RELEASE_RESOURCE;
			}

			MMVideoBuffer *scmn_imgb = NULL;
			GstMapInfo mapInfo;
			GstMemory *img_buf_memory = NULL;
			if (buf[1] && gst_buffer_n_memory(buf[1]) > 1){
				img_buf_memory = gst_buffer_peek_memory(buf[1],1);
				gst_memory_map(img_buf_memory,&mapInfo,GST_MAP_READ);
				imgb[1] = (MMVideoBuffer *)mapInfo.data;
				gst_memory_unmap(img_buf_memory,&mapInfo);
			}
			if(imgb[1] == NULL){
				GST_WARNING("imgb[1] is null");
				goto RELEASE_RESOURCE;
			}
		}

		tbm_surface_info_s tsurf_info[2];

		/* get tbm surface */
		for (i = BUF_0 ; i < max_id ; i++) {
			switch (app_data->fourcc) {
				case GST_MAKE_FOURCC('S', 'N', '1', '2') :
					tsurf_info[i].format = TBM_FORMAT_NV12;
					break;
				case GST_MAKE_FOURCC('S', '4', '2', '0') :
					tsurf_info[i].format = TBM_FORMAT_YUV420;
					break;
				default :
					GST_WARNING("Invalid format");
					break;
		}
			tsurf_info[i].width = imgb[i]->width[0];
			tsurf_info[i].height = imgb[i]->height[0];
			tsurf_info[i].bpp = tbm_surface_internal_get_bpp(tsurf_info[i].format);
			tsurf_info[i].num_planes = tbm_surface_internal_get_num_planes(tsurf_info[i].format);

			tsurf_info[i].planes[0].offset = 0;
			tsurf_info[i].planes[0].size = imgb[i]->stride_width[0] * imgb[i]->stride_height[0];
			tsurf_info[i].planes[0].stride = imgb[i]->stride_width[0];
			tsurf_info[i].size = tsurf_info[i].planes[0].size;

			/* get size, stride and offset */
			for (j = 1; j < tsurf_info[i].num_planes; j++)
			{
			tsurf_info[i].planes[j].size = imgb[i]->stride_width[j] * imgb[i]->stride_height[j];
			tsurf_info[i].planes[j].offset = tsurf_info[i].planes[j-1].size;
			tsurf_info[i].planes[j].stride = imgb[i]->stride_width[j];
			tsurf_info[i].size = tsurf_info[i].size +  tsurf_info[i].planes[j].size;
			}

			if (app_data->buf_share_method == MM_VIDEO_BUFFER_TYPE_DMABUF_FD) {
				/*need to make bo using fd itself*/
				if (!tbm_export_buffer(app_data, i, imgb[i]->handle.dmabuf_fd[0], tsurf_info[i], &gem_handle[i], &input_buffer[i])) {
					GST_WARNING("tbm_export_buffer[index:%d] failed", i);
					break;
				}
			} else if (app_data->buf_share_method == MM_VIDEO_BUFFER_TYPE_TBM_BO) {
				if (!tbm_export_buffer2(app_data, i, imgb[i]->handle.bo[0], tsurf_info[i], &input_buffer[i])) {
					GST_WARNING("tbm_export_buffer2[index:%d] failed", i);
					break;
				}
			} else {
				GST_ERROR("should not be reached here");
			}
		}

		if (input_buffer[0] == NULL || (is_dual_camera_mode(app_data) &&
		    input_buffer[1] == NULL)) {
			GST_ERROR("Input Buffer is NULL");
			goto RELEASE_RESOURCE;
		}

		tbm_bo_map(imgb[0]->handle.bo[0], TBM_DEVICE_CPU, TBM_OPTION_READ|TBM_OPTION_WRITE);
		tbm_bo_unmap(imgb[0]->handle.bo[0]);
		ret = TizenGLManager_setNativeBufferTexture (app_data->manager, input_buffer[0], 0);
		GST_LOG("TizenGLManager_setNativeBufferTexture[0] ret=%d", ret);
		if (ret != 0) {
			GST_ERROR("TizenGLManager_setNativeBufferTexture id:0 failed");
			goto RELEASE_RESOURCE;
		}

		if (is_dual_camera_mode(app_data)) {
			tbm_bo_map(imgb[1]->handle.bo[0], TBM_DEVICE_CPU, TBM_OPTION_READ|TBM_OPTION_WRITE);
			tbm_bo_unmap(imgb[1]->handle.bo[0]);
			ret = TizenGLManager_setNativeBufferTexture (app_data->manager, input_buffer[1], 1);
			GST_LOG("TizenGLManager_setNativeBufferTexture[1] ret=%d", ret);
			if (ret != 0) {
				GST_ERROR("TizenGLManager_setNativeBufferTexture id:1 failed");
				goto RELEASE_RESOURCE;
			}
		}

		IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_PIXMAP]);
		while (available_pixmap_id == -1) {
			available_pixmap_id = get_available_pixmap_id(app_data);
			if (available_pixmap_id == -1) {
				GTimeVal abstimeout;
				g_get_current_time(&abstimeout);
				g_time_val_add(&abstimeout, PIP_WAIT_TIMEOUT);
				GST_LOG("all pixmap are using now. wait...");
				if(!g_cond_timed_wait(app_data->thread_cond[MU_PIXMAP],
					app_data->thread_mutex[MU_PIXMAP],&abstimeout)) {
					GST_LOG("pixmap signal time out , 1 sec");
				} else {
					GST_LOG("pixmap signal received, check again");
				}
			}
		}
		IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_PIXMAP]);

		GST_LOG("available_pixmap_id %d", available_pixmap_id);

		/*add pixmap to pool and get handled native buffer*/
		GST_LOG("TizenGLManager_render START ======");

		int manager_status = TizenGLManager_render(app_data->manager);

		GST_LOG("TizenGLManager_render END status: %d ======", manager_status);

		surface_output = TizenGLManager_getPixmapBufferSurface(app_data->manager);
		TizenGLManager_getNativeBufferTexture(app_data->manager, 0);
		if (is_dual_camera_mode(app_data)) {
			TizenGLManager_getNativeBufferTexture(app_data->manager, 1);
		}

		GST_DEBUG("surface_output %p", surface_output);

		if (manager_status == 1) {
			/*get output data*/
			OutputData *outputData = (OutputData *)malloc (sizeof (OutputData));
			if (!outputData) {
				GST_ERROR("Allocate OutputData failed");
				goto RELEASE_RESOURCE;
			}
			memset(outputData, 0x0, sizeof(OutputData));
			outputData->surface = surface_output;
			outputData->pixmap_id = available_pixmap_id;
			outputData->buf_share_method = app_data->buf_share_method;
			if (buf[0]) {
				outputData->timestamp = GST_BUFFER_TIMESTAMP(buf[0]);
				outputData->duration = GST_BUFFER_DURATION(buf[0]);
			}

			GST_LOG("handling buffer END ==================================");

			IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_OUTPUT_QUEUE]);
			g_queue_push_tail(app_data->output, outputData);
			IF_G_COND_SIGNAL(app_data->thread_cond[MU_OUTPUT_QUEUE]);
			IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_OUTPUT_QUEUE]);

			GST_LOG("Send output buffer signal");
		} else {
			GST_ERROR("failed to render.. pixmap id restore");
			IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_PIXMAP]);
			app_data->surface_using[available_pixmap_id] = FALSE;
			IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_PIXMAP]);
		}

RELEASE_RESOURCE:
		/*release resources*/
		for (i = BUF_0 ; i < max_id ; i++) {
			if (buf[i]) {
				GST_LOG("unref gst buffer[%d] %p, input_cnt %d", i, buf[i], app_data->input_cnt);
				gst_buffer_unref(buf[i]);
				IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_INPUT_QUEUE]);
				app_data->input_cnt--;
				IF_G_MUTEX_UNLOCK (app_data->thread_mutex[MU_INPUT_QUEUE]);
				buf[i] = NULL;
			}
		}

		IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_INPUT_QUEUE]);
	}

	/* release remained buffer */
	GST_INFO("release remained buffer");
	for (i = BUF_0; i < max_id; i++) {
		g_queue_free_full (app_data->input[i], (GDestroyNotify)gst_buffer_unref);
		app_data->input[i] = g_queue_new();
	}

	/*destroy preview glm*/
	destroy_gl_manager(app_data);

	IF_G_MUTEX_UNLOCK (app_data->thread_mutex[MU_INPUT_QUEUE]);

	GST_WARNING("handle_buffer_thread_cb end");

	return NULL;
}

static void init_mutex(AppData *app_data)
{
	startfunc

    gint i = 0;

	for (i = 0; i < MUTEX_MAX; i++) {
		app_data->thread_mutex[i] = g_mutex_new();
		app_data->thread_cond[i] = g_cond_new();
	}

	endfunc

	return;
}

static void mutex_fini(AppData *app_data)
{
	startfunc

    gint i = 0;

	for (i = 0; i < MUTEX_MAX; i++) {
		IF_G_MUTEXT_FREE(app_data->thread_mutex[i]);
		IF_G_COND_FREE(app_data->thread_cond[i]);
	}

	endfunc

	return;
}

static void init_thread (AppData *app_data)
{
	startfunc

	if (!g_thread_supported ())
		g_thread_init (NULL);

	app_data->threads[TH_HANDLE_BUF] = g_thread_create ( (GThreadFunc) handle_buffer_thread_cb, app_data, TRUE, NULL);

	endfunc

	return;
}

static void thread_fini (AppData *app_data)
{
	startfunc

	/* release capture thread */
	if (app_data->threads[TH_CAPTURE]) {
		GST_WARNING("lock thread_mutex[MU_CAPTURE_QUEUE]");
		IF_G_MUTEX_LOCK (app_data->thread_mutex[MU_CAPTURE_QUEUE]);
		app_data->exit_loop[TH_CAPTURE] = TRUE;
		GST_WARNING("set exit_loop TH_CAPTURE TRUE");
		IF_G_COND_SIGNAL (app_data->thread_cond[MU_CAPTURE_QUEUE]);
		GST_WARNING("signal MU_CAPTURE_QUEUE");
		IF_G_MUTEX_UNLOCK (app_data->thread_mutex[MU_CAPTURE_QUEUE]);
		GST_WARNING("join TH_CAPTURE");
		IF_G_THREAD_JOIN(app_data->threads[TH_CAPTURE]);
	}

	/* release handle buffer  thread */
	if (app_data->threads[TH_HANDLE_BUF]) {
		GST_WARNING("lock thread_mutex[MU_INPUT_QUEUE]");
		IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_INPUT_QUEUE]);
		app_data->exit_loop[TH_HANDLE_BUF] = TRUE;
		GST_WARNING("set exit_loop TH_HANDLE_BUF TRUE");
		IF_G_COND_SIGNAL (app_data->thread_cond[MU_INPUT_QUEUE]);
		GST_WARNING("signal MU_INPUT_QUEUE");
		IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_INPUT_QUEUE]);
		GST_WARNING("join TH_HANDLE_BUF");
		IF_G_THREAD_JOIN(app_data->threads[TH_HANDLE_BUF]);
	}

	/* release handle output buffer thread */
	if (app_data->threads[TH_HANDLE_OUT_BUF]) {
		GST_WARNING("lock thread_mutex[MU_OUTPUT_QUEUE]");
		IF_G_MUTEX_LOCK (app_data->thread_mutex[MU_OUTPUT_QUEUE]);
		app_data->exit_loop[TH_HANDLE_OUT_BUF] = TRUE;
		GST_WARNING("set exit_loop TH_HANDLE_OUT_BUF TRUE");
		IF_G_COND_SIGNAL (app_data->thread_cond[MU_OUTPUT_QUEUE]);
		GST_WARNING("signal MU_OUTPUT_QUEUE");
		IF_G_MUTEX_UNLOCK (app_data->thread_mutex[MU_OUTPUT_QUEUE]);
		GST_WARNING("join TH_HANDLE_OUT_BUF");
		IF_G_THREAD_JOIN(app_data->threads[TH_HANDLE_OUT_BUF]);
	}

	endfunc

	return;
}

gboolean destroy_gl_manager(AppData *app_data)
{
	startfunc

	gint i = 0;

	if (!app_data) {
		GST_ERROR("app_data is null");
		return FALSE;
	}

	GST_WARNING("check not returend output buffer");

	/* wait for output buffer */
	IF_G_MUTEX_LOCK(app_data->thread_mutex[MU_PIXMAP]);
	for (i = 0 ; i < SURFACE_COUNT ; i++) {
		if (app_data->surface_using[i]) {
			GTimeVal abstimeout;
			g_get_current_time(&abstimeout);
			g_time_val_add(&abstimeout, BUFFER_WAIT_TIMEOUT);

			GST_WARNING("wait for output buffer [pixmap id %d]", i);

			if (!g_cond_timed_wait(app_data->thread_cond[MU_PIXMAP], app_data->thread_mutex[MU_PIXMAP], &abstimeout)) {
				GST_ERROR("buffer wait timeout, keep going...");
				break;
			} else {
				GST_WARNING("signal received, check again...");
				i = -1;
			}
		}
	}
	IF_G_MUTEX_UNLOCK(app_data->thread_mutex[MU_PIXMAP]);

	reset_flag(app_data);

	/*destroy GLM*/
	GST_WARNING("TizenGLManager_destroy %p", app_data->manager);
	if (app_data->manager) {
		TizenGLManager_destroy (app_data->manager);
		app_data->manager = NULL;
		for (i=0; i<SURFACE_COUNT; i++) {
			tbm_surface_destroy(app_data->surface_buffer[i]);
			app_data->surface_buffer[i] = 0;
		}
	}

	/*release input buffer*/
	for (i = 0 ; i < MAX_INPUT_BUFFER ; i++) {
		if (app_data->in_buf[i].surface) {
			GST_INFO(" tbm_surface_destroy %p", app_data->in_buf[i].surface);
			tbm_surface_destroy(app_data->in_buf[i].surface);
			app_data->in_buf[i].surface= NULL;
		}
		if (app_data->in_buf[i].gem_handle > 0) {
			GST_WARNING("close input buffer[%d] gem_handle[%d]",
			            i, app_data->in_buf[i].gem_handle);
			gem_handle_close(app_data->fd, app_data->in_buf[i].gem_handle);
			app_data->in_buf[i].gem_handle = 0;

			/* release tbm bo imported by pip plugin */
			if (app_data->in_buf[i].bo) {
				GST_WARNING("release input buffer[%d] bo[%p]",
				            i, app_data->in_buf[i].bo);
				tbm_bo_unref(app_data->in_buf[i].bo);
			}
		}
		app_data->in_buf[i].bo = NULL;
		app_data->in_buf[i].dma_buf_fd = 0;
	}

	/* reset width and height */
	app_data->width[0] = 0;
	app_data->height[0] = 0;

	/*destroy TBM buffer manager*/
	GST_WARNING("tbm_bufmgr_deinit %p", app_data->bufmgr);
	if (app_data->bufmgr) {
		tbm_bufmgr_deinit (app_data->bufmgr);
		app_data->bufmgr = NULL;
	}

	close (app_data->fd);

	endfunc;

	return TRUE;
}

gboolean destroy_app_data(AppData *app_data)
{
	startfunc

	int i = 0;

	if (!app_data) {
		GST_ERROR("app_data is null");
		return FALSE;
	}

	/*destroy window*/
	destroy_window(app_data);

	for (i = BUF_0; i < BUFFER_MAX; i++) {
		g_queue_free(app_data->input[i]);
		g_queue_free(app_data->capture_buffer_list[i]);
	}

	g_queue_free(app_data->output);

    IF_FREE(app_data);

	endfunc

	return TRUE;
}

AppData *init_app_data()
{
	startfunc

	/*init*/
	AppData *app_data =NULL;
	int i = 0;
	app_data = (AppData *)calloc (1, sizeof (AppData));
	if (!app_data) {
		GST_ERROR("Alloc AppData failed");
		return NULL;
	}

	app_data->has_created_glm = FALSE;
	app_data->ready_for_glm = FALSE;
	app_data->has_setted_ibp = FALSE;

	/*initialize preview(input ,output) and capture queue*/
	for (i = BUF_0; i < BUFFER_MAX; i++) {
		app_data->input[i] = g_queue_new();
		app_data->capture_buffer_list[i] = g_queue_new();
		app_data->surface[i] = NULL;
		app_data->input_bo[i] = NULL;
	}
	app_data->output = g_queue_new();

	/* init surface_using */
	for (i = 0 ; i < SURFACE_COUNT ; i++) {
		app_data->surface_using[i] = FALSE;
	}

	GST_WARNING("app_data %p", app_data);


	/*create window*/
	create_window(app_data);

	endfunc

	return app_data;
}


gboolean init_working_threads(AppData *app_data)
{
	int i = 0;
	if (!app_data) {
		GST_ERROR("AppData is null");
		return FALSE;
	}

	/*init exit_loop*/
	for(i = BUF_0; i < THREAD_MAX; i++) {
		app_data->exit_loop[i] = FALSE;
	}

	/*initialize mutex*/
	init_mutex(app_data);

	/*initialize thread*/
	init_thread(app_data);

	return TRUE;
}


gboolean destroy_working_threads(AppData *app_data)
{
	if (!app_data) {
		GST_ERROR("AppData is null");
		return FALSE;
	}

	/*destroy thread*/
	thread_fini(app_data);

	/*destroy mutex*/
	mutex_fini(app_data);

	return TRUE;
}


gboolean init_input_buf_property(AppData *app_data, gint width_0, gint height_0, gint width_1, gint height_1, gint rate, gint scale, guint32 fourcc)
{
	if (!app_data) {
		GST_ERROR("AppData is null");
		return FALSE;
	}

	if (app_data->has_setted_ibp) {
		GST_ERROR("Width and Height has been setted");
		return TRUE;
	}

	GST_WARNING("[0] %dx%d, [1] %dx%d, rate %d, scale %d",
	            width_0, height_0, width_1, height_1, rate, scale);

	app_data->width[0] = width_0;
	app_data->height[0] = height_0;
	app_data->width[1] = width_1;
	app_data->height[1] = height_1;
	app_data->rate = rate;
	app_data->scale = scale;
	app_data->has_setted_ibp = TRUE;
	app_data->fourcc = fourcc;

	return TRUE;
}

gboolean init_gl_manager(AppData *app_data)
{
	startfunc
	int i;
	if (!app_data) {
		GST_ERROR("AppData is null");
		return FALSE;
	}

	if (app_data->ready_for_glm) {
		GST_WARNING("GLM has been initialized");
		return FALSE;
	}

	/*create fd */
	app_data->fd = _drm_init (app_data);
	if (app_data->fd < 0) {
		GST_LOG("create fd failed, app_data->fd: %d", app_data->fd);
		return FALSE;
	}

	app_data->bufmgr = tbm_bufmgr_init (app_data->fd);

	/*create output pixmap*/
	for (i = 0 ; i < SURFACE_COUNT ; i++) {
		app_data->surface_buffer[i] = tbm_surface_create (app_data->width[0], app_data->height[0],TBM_FORMAT_BGRA8888);
		GST_WARNING("pixmap[%d] %p", i, app_data->surface_buffer[i]);
	}

	/*init input buffer*/
	for (i = 0 ; i < MAX_INPUT_BUFFER ; i++) {
		app_data->in_buf[i].dma_buf_fd = 0;
		app_data->in_buf[i].gem_handle = 0;
		app_data->in_buf[i].bo = NULL;
		app_data->in_buf[i].surface = NULL;
	}

	app_data->ready_for_glm = TRUE;

	endfunc

	return TRUE;
}

gboolean create_capture_glm(AppData *app_data, gint capture_w, gint capture_h)
{
	startfunc

	if (!app_data) {
		GST_ERROR("AppData is null");
		return FALSE;
	}

	if (app_data->has_created_capture_glm) {
		GST_WARNING("Capture GLM has been created");
		return FALSE;
	}

	GST_WARNING("start - size %dx%d", capture_w, capture_h);

	app_data->capture_display = XOpenDisplay ( 0 );

	if (!app_data->capture_display ) {
		GST_WARNING("capture_display XOpenDisplay failed");
		return FALSE;
	}

	app_data->capture_w = capture_w;
	app_data->capture_h = capture_h;

	app_data->capture_surface = tbm_surface_create (app_data->capture_w, app_data->capture_h, TBM_FORMAT_BGRA8888);

	/*create GLManager*/
	app_data->capture_manager = TizenGLManager_create(app_data->capture_w, app_data->capture_h,
	                                                  app_data->capture_display, app_data->window);
	if(!app_data->capture_manager) {
		GST_WARNING("TizenGLManager_create capture failed");
		return FALSE;
	}

	/*set options*/
	gl_manager_set_options(app_data->capture_manager, app_data);

	TizenGLManager_setNativeBufferSurface (app_data->capture_manager, app_data->capture_surface);
	app_data->has_created_capture_glm = TRUE;

	GST_WARNING("done");

	endfunc

	return TRUE;
}

gboolean destroy_capture_glm(AppData *app_data)
{
	int i = 0;

	startfunc

	if (!app_data) {
		GST_ERROR("app_data is null");
		return FALSE;
	}

	/*destroy GLM*/
	if(app_data->has_created_capture_glm) {
		app_data->has_created_capture_glm = FALSE;
		GST_WARNING("TizenGLManager_destroy");
		TizenGLManager_destroy (app_data->capture_manager);
		app_data->capture_manager = NULL;
		GST_WARNING("capture output buffer destroy");
		tbm_surface_destroy (app_data->capture_surface);
		app_data->capture_surface = NULL;
		GST_WARNING("capture input surface destroy");
		for (i = 0 ; i < BUFFER_MAX ; i++) {
			if (app_data->input_bo[i]) {
				tbm_bo_unref(app_data->input_bo[i]);
				app_data->input_bo[i] = NULL;
			}
			if (app_data->surface[i]) {
				tbm_surface_destroy(app_data->surface[i]);
				app_data->surface[i] = NULL;
			}
		}
	}

	GST_WARNING("XCloseDisplay");
	XCloseDisplay (app_data->capture_display);

	endfunc;

	return TRUE;
}


gboolean handle_camerasrc_buffer(AppData *app_data, GstBuffer *buf[BUFFER_MAX])
{
	startfunc

	gint i = 0;
	gint max_cnt = 4;

	if (!app_data || !buf || !app_data->input[BUF_0] ||
		(is_dual_camera_mode(app_data) && !app_data->input[BUF_1])) {
		GST_ERROR("handle_camerasrc_buffer FAILED");
		return FALSE;
	}

	if (is_dual_camera_mode(app_data))
		max_cnt = 8;

	while(app_data->input_cnt > max_cnt && i < 300) {
		usleep(10000);
		i++;
		if (i == 300)
			GST_WARNING("Wait for freeing input buffer (%d*10 ms)", i);
	}


	/*push buffer to queue*/
	IF_G_MUTEX_LOCK (app_data->thread_mutex[MU_INPUT_QUEUE]);
	gint max = get_max_buf_id(app_data);
	for (i = BUF_0; i < max; i++) {
		g_queue_push_tail(app_data->input[i], buf[i]);
		app_data->input_cnt++;
		GST_LOG("gst buffer[%d] : %p, input_cnt %d", i, buf[i], app_data->input_cnt);
	}
	IF_G_COND_SIGNAL (app_data->thread_cond[MU_INPUT_QUEUE]);
	IF_G_MUTEX_UNLOCK (app_data->thread_mutex[MU_INPUT_QUEUE]);

	endfunc

	return TRUE;
}


void gem_handle_close(int drm_fd, unsigned int gem_handle)
{
	struct drm_gem_close arg;

	if (drm_fd < 0) {
		GST_ERROR("drm fd is invalid %d", drm_fd);
		return;
	}

	if (gem_handle == 0) {
		GST_ERROR("invalid gem handle %u", gem_handle);
		return;
	}

	/* close gem handle */
	arg.handle = gem_handle;
	if (drmIoctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &arg) != 0) {
		GST_ERROR("GEM free failed [drm_fd:%d, handle %u]", drm_fd, gem_handle);
	}

	return;
}


int tbm_export_buffer(AppData *app_data, int id, int dmabuf_fd, tbm_surface_info_s surface_info, unsigned int *gem_handle, tbm_surface_h *psurface )
{
	int i = 0;
	int ret = FALSE;
	struct drm_prime_handle prime_arg = {0,};
	struct drm_gem_flink flink_arg = {0,};
	tbm_format buf_format ;

	if (!app_data) {
		GST_ERROR("app_data is NULL");
		return FALSE;
	}

	if (app_data->fd <= 0 || dmabuf_fd < 0 || !gem_handle || !psurface) {
		GST_ERROR("invalid drm fd[%d] dmabuf fd[%d] gem_handle[%p] buffer[%p]",
		          app_data->fd, dmabuf_fd, gem_handle, psurface);
		return FALSE;
	}
	/* check converted fd */
	for (i = 0 ; i < MAX_INPUT_BUFFER ; i++) {
		if (dmabuf_fd == app_data->in_buf[i].dma_buf_fd) {
			GST_DEBUG("already converted fd[%d], return index[%d] bo[%p] gem_handle[%d]",
			          dmabuf_fd, i, app_data->in_buf[i].bo, app_data->in_buf[i].gem_handle);
			*gem_handle = app_data->in_buf[i].gem_handle;
			*psurface = app_data->in_buf[i].surface;
			return TRUE;
		}
	}

	/* get gem handle */
	prime_arg.fd = dmabuf_fd;
	ret = ioctl(app_data->fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime_arg);
	if (ret) {
		GST_ERROR("DRM_IOCTL_PRIME_FD_TO_HANDLE failed. ret %d, dmabuf fd : %u",
		          ret, dmabuf_fd);
		return FALSE;
	}

	/* get gem name */
	flink_arg.handle = prime_arg.handle;
	ret = ioctl(app_data->fd, DRM_IOCTL_GEM_FLINK, &flink_arg);
	if (ret) {
		GST_ERROR("DRM_IOCTL_GEM_FLINK failed. ret %d, gem_handle %u, gem_name %u",
		          ret, prime_arg.handle, flink_arg.name);
		close(prime_arg.handle);
		return FALSE;
	}

	/* set gem handle */
	*gem_handle = prime_arg.handle;

	/* export bo with gem name */
	tbm_bo bo = tbm_bo_import(app_data->bufmgr, flink_arg.name);

	*psurface = tbm_surface_internal_create_with_bos(&surface_info,&bo,1);

	/* keep bo and gem handle */
	for (i = 0 ; i < MAX_INPUT_BUFFER ; i++) {
		if (app_data->in_buf[i].dma_buf_fd == 0) {
			app_data->in_buf[i].dma_buf_fd = dmabuf_fd;
			app_data->in_buf[i].gem_handle = *gem_handle;
			app_data->in_buf[i].bo = bo;
			app_data->in_buf[i].surface = *psurface;
			break;
		}
	}

	GST_DEBUG("done - bo %p, gem handle %u", bo, *gem_handle);

	return TRUE;
}


int tbm_export_buffer2(AppData *app_data, int id, tbm_bo bo, tbm_surface_info_s surface_info, tbm_surface_h *psurface)
{
	int i = 0;
	int ret = FALSE;

	if (!app_data) {
		GST_ERROR("app_data is NULL");
		return FALSE;
	}

	if (app_data->fd <= 0 || bo == NULL || !psurface) {
		GST_ERROR("invalid drm fd[%d] bo[%p] buffer[%p]", app_data->fd, bo, psurface);
		return FALSE;
	}


	/* check converted bo */
	for (i = 0 ; i < MAX_INPUT_BUFFER ; i++) {
		if (bo == app_data->in_buf[i].bo) {
			GST_DEBUG("already converted bo[%p], return index[%d] surface",
			          bo, i, app_data->in_buf[i].surface);
			*psurface = app_data->in_buf[i].surface;
			return TRUE;
		}
	}

	*psurface = tbm_surface_internal_create_with_bos (&surface_info,&bo,1);
	if (*psurface == NULL) {
		GST_ERROR("failed to create native buffer. bo[%p]", bo);
		return FALSE;
	}

	/* keep bo */
	for (i = 0 ; i < MAX_INPUT_BUFFER ; i++) {
		if (app_data->in_buf[i].bo == NULL) {
			app_data->in_buf[i].bo = bo;
			app_data->in_buf[i].surface = *psurface;
			GST_INFO("keep[index:%d] bo[%p] surface [%p]", i, bo, *psurface);
			break;
		}
	}

	GST_DEBUG("done - bo %p, surface %p", bo, *psurface);

	return TRUE;
}


gboolean is_dual_camera_mode(AppData *app_data)
{
	if (!app_data) {
		GST_ERROR("app_data is NULL");
		return FALSE;
	}
	return (app_data->camera_mode == DUAL_CAMERA);
}

