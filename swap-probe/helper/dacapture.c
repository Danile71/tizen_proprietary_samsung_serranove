/*
 *  DA probe
 *
 * Copyright (c) 2000 - 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim <jaewon81.lim@samsung.com>
 * Woojin Jung <woojin2.jung@samsung.com>
 * Juyoung Kim <j0.kim@samsung.com>
 * Anastasia Lyupa <a.lyupa@samsung.com>
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
 * Contributors:
 * - S-Core Co., Ltd
 * - Samsung RnD Institute Russia
 *
 */
/*
 * We refer to the Evas example source code with BSD
 * (Evas/src/examples/evas-buffer-simple.c)
 *
 */

#include <stdlib.h>		// for system
#include <sys/types.h>		// for stat, getpid
#include <sys/stat.h>		// fot stat, chmod
#include <unistd.h>		// fot stat, getpid
#include <sys/shm.h>		// for shmget, shmat
#include <pthread.h>		// for mutex

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/XShm.h>

#include <Ecore.h>
#include <Evas.h>
#include <Evas_Engine_Buffer.h>

#include "real_functions.h"
#include "daprobe.h"
#include "dahelper.h"

#include "binproto.h"

#define MAX_HEIGHT	720
#define CAPTURE_TIMEOUT		2.0

typedef struct _screenshot_data
{
	XImage*			ximage;
	Display*		dpy;
	XShmSegmentInfo	x_shm_info;
} screenshot_data;

/*
int convert_image(	void* srcbuf, void* dstbuf,
					pixman_format_code_t src_format,
					pixman_format_code_t dst_format,
					int src_width, int src_height,
					int dst_width, int dst_height,
					int	rotate)
{
	pixman_image_t *   src_img;
	pixman_image_t *   dst_img;
	pixman_transform_t transform;

	int                src_stride, dst_stride;
	int                src_bpp;
	int                dst_bpp;
	pixman_op_t        op;
	int                rotate_step;
	int                ret = False;

	return_val_if_fail (srcbuf != NULL, False);
	return_val_if_fail (dstbuf != NULL, False);
	return_val_if_fail (rotate <= 360 && rotate >= -360, False);

	op = PIXMAN_OP_SRC;

	src_bpp = PIXMAN_FORMAT_BPP (src_format) / 8;
	return_val_if_fail (src_bpp > 0, False);

	dst_bpp = PIXMAN_FORMAT_BPP (dst_format) / 8;
	return_val_if_fail (dst_bpp > 0, False);

	rotate_step = (rotate + 360) / 90 % 4;

	src_stride = src_width * src_bpp;
	dst_stride = dst_width * dst_bpp;

	src_img = pixman_image_create_bits (src_format, src_width, src_height, srcbuf, src_stride);
	dst_img = pixman_image_create_bits (dst_format, dst_width, dst_height, dstbuf, dst_stride);

	goto_if_fail (src_img != NULL, CANT_CONVERT);
	goto_if_fail (dst_img != NULL, CANT_CONVERT);

	pixman_transform_init_identity (&transform);

	if (rotate_step > 0)
	{
		int c, s, tx = 0, ty = 0;
		switch (rotate_step)
		{
		case 1:
			// 270 degrees
			c = 0;
			s = -pixman_fixed_1;
			ty = pixman_int_to_fixed (dst_width);
			break;
		case 2:
			// 180 degrees
			c = -pixman_fixed_1;
			s = 0;
			tx = pixman_int_to_fixed (dst_width);
			ty = pixman_int_to_fixed (dst_height);
			break;
		case 3:
			// 90 degrees
			c = 0;
			s = pixman_fixed_1;
			tx = pixman_int_to_fixed (dst_height);
			break;
		default:
			// 0 degrees
			c = 0;
			s = 0;
			break;
		}
		pixman_transform_rotate (&transform, NULL, c, s);
		pixman_transform_translate (&transform, NULL, tx, ty);
	}

	pixman_image_set_transform (src_img, &transform);

	pixman_image_composite (op, src_img, NULL, dst_img,
	                        0, 0, 0, 0, 0, 0, dst_width, dst_height);

	ret = True;

CANT_CONVERT:
	if (src_img)
		pixman_image_unref (src_img);
	if (dst_img)
		pixman_image_unref (dst_img);

	return ret;
}
*/

static char* captureScreenShotX(int* pwidth, int* pheight, screenshot_data* sdata)
{
	static Display *(*__XOpenDisplay_p)(_Xconst char *);
	static Bool (*__XShmGetImage_p)(Display *, Drawable, XImage *, int, int,
		     unsigned long);
	static XImage *(*__XShmCreateImage_p)(Display *, Visual *, unsigned int,
			int, char *, XShmSegmentInfo *, unsigned int, unsigned int);
	static Bool (*__XShmAttach_p)(Display *, XShmSegmentInfo *);
	static Bool (*__XShmDetach_p)(Display *, XShmSegmentInfo *);
	static int (*__XSync_p)(Display *, Bool);
	Window			root;
//	Atom			atom_rotation;

	rtld_default_set_once(__XOpenDisplay_p, "XOpenDisplay");
	rtld_default_set_once(__XShmGetImage_p, "XShmGetImage");
	rtld_default_set_once(__XShmCreateImage_p, "XShmCreateImage");
	rtld_default_set_once(__XShmAttach_p, "XShmAttach");
	rtld_default_set_once(__XShmDetach_p, "XShmDetach");
	rtld_default_set_once(__XSync_p, "XSync");

	sdata->dpy = __XOpenDisplay_p(NULL);
	if(unlikely(sdata->dpy == NULL))
	{
		// XOpenDisplay failed!
		return NULL;
	}

	*pwidth = DisplayWidth(sdata->dpy, DefaultScreen(sdata->dpy));
	*pheight = DisplayHeight(sdata->dpy, DefaultScreen(sdata->dpy));

	root = RootWindow(sdata->dpy, DefaultScreen(sdata->dpy));

	sdata->ximage = __XShmCreateImage_p(sdata->dpy, DefaultVisualOfScreen (DefaultScreenOfDisplay (sdata->dpy)), 24,
					ZPixmap, NULL, &sdata->x_shm_info, (unsigned int)*pwidth, (unsigned int)*pheight);

	if(sdata->ximage != NULL)
	{
		sdata->x_shm_info.shmid = shmget(IPC_PRIVATE, sdata->ximage->bytes_per_line * sdata->ximage->height, IPC_CREAT | 0777);
		sdata->x_shm_info.shmaddr = sdata->ximage->data = shmat(sdata->x_shm_info.shmid, 0, 0);
		sdata->x_shm_info.readOnly = False;

		if(__XShmAttach_p(sdata->dpy, &sdata->x_shm_info))
		{
			if(__XShmGetImage_p(sdata->dpy, root, sdata->ximage, 0, 0, AllPlanes))
			{
				__XSync_p (sdata->dpy, False);
				return sdata->ximage->data;
			}
			else
			{
				; // XShmGetImage failed !
			}

			__XShmDetach_p(sdata->dpy, &sdata->x_shm_info);
		}
		else
		{
			; // XShmAttach failed !
		}

		shmdt (sdata->x_shm_info.shmaddr);
		shmctl (sdata->x_shm_info.shmid, IPC_RMID, NULL);
		XDestroyImage(sdata->ximage);
		sdata->ximage = NULL;
	}
	else
	{
		; // XShmCreateImage failed!
	}

	return NULL;
}

static void releaseScreenShotX(screenshot_data* sdata)
{
	static int (*__XCloseDisplay_p)(Display *);
	static Bool (*__XShmDetach_p)(Display *, XShmSegmentInfo *);

	rtld_default_set_once(__XCloseDisplay_p, "XCloseDisplay");
	rtld_default_set_once(__XShmDetach_p, "XShmDetach");

	if(sdata->ximage)
	{
		__XShmDetach_p (sdata->dpy, &sdata->x_shm_info);
		shmdt (sdata->x_shm_info.shmaddr);
		shmctl (sdata->x_shm_info.shmid, IPC_RMID, NULL);
		XDestroyImage(sdata->ximage);
	}
	else { }

	if(sdata->dpy)
	{
		__XCloseDisplay_p(sdata->dpy);
	}
}

static Evas* create_canvas(int width, int height)
{

	static void (*__evas_output_method_set_p)(Evas *e, int render_method);
	static void (*__evas_output_size_set_p)(Evas *e, int w, int h);
	static void (*__evas_output_viewport_set_p)(Evas *e, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
	static int (*__evas_render_method_lookup_p)(const char *name);
	static Evas_Engine_Info *(*__evas_engine_info_get_p)(const Evas *e);
	static Eina_Bool (*__evas_engine_info_set_p)(const Evas *e, Evas_Engine_Info *info);
	static Evas *(*__evas_new_p)(void);
	static void (*__evas_free_p)(Evas *e);
	Evas *canvas;
	Evas_Engine_Info_Buffer *einfo;
	int method;
	void *pixels;

	rtld_default_set_once(__evas_output_method_set_p, "evas_output_method_set");
	rtld_default_set_once(__evas_output_size_set_p, "evas_output_size_set");
	rtld_default_set_once(__evas_output_viewport_set_p, "evas_output_viewport_set");
	rtld_default_set_once(__evas_render_method_lookup_p, "evas_render_method_lookup");
	rtld_default_set_once(__evas_engine_info_get_p, "evas_engine_info_get");
	rtld_default_set_once(__evas_engine_info_set_p, "evas_engine_info_set");
	rtld_default_set_once(__evas_new_p, "evas_new");
	rtld_default_set_once(__evas_free_p, "evas_free");

	method = __evas_render_method_lookup_p("buffer");
	if (unlikely(method <= 0))
	{
		//fputs("ERROR: evas was not compiled with 'buffer' engine!\n", stderr);
		return NULL;
	}

	canvas = __evas_new_p();
	if (unlikely(canvas == NULL))
	{
		//fputs("ERROR: could not instantiate new evas canvas.\n", stderr);
		return NULL;
	}

	__evas_output_method_set_p(canvas, method);
	__evas_output_size_set_p(canvas, width, height);
	__evas_output_viewport_set_p(canvas, 0, 0, width, height);

	einfo = (Evas_Engine_Info_Buffer *)__evas_engine_info_get_p(canvas);
	if (unlikely(einfo == NULL))
	{
		//fputs("ERROR: could not get evas engine info!\n", stderr);
		__evas_free_p(canvas);
		return NULL;
	}

	// ARGB32 is sizeof(int), that is 4 bytes, per pixel
	pixels = real_malloc(width * height * sizeof(int));
	if (unlikely(pixels == NULL)) {
		//fputs("ERROR: could not allocate canvas pixels!\n", stderr);
		__evas_free_p(canvas);
		return NULL;
	}

	einfo->info.depth_type = EVAS_ENGINE_BUFFER_DEPTH_ARGB32;
	einfo->info.dest_buffer = pixels;
	einfo->info.dest_buffer_row_bytes = width * sizeof(int);
	einfo->info.use_color_key = 0;
	einfo->info.alpha_threshold = 0;
	einfo->info.func.new_update_region = NULL;
	einfo->info.func.free_update_region = NULL;

	if (unlikely(__evas_engine_info_set_p(canvas,(Evas_Engine_Info*)einfo) == EINA_FALSE)) {
		PRINTMSG("ERROR: could not set evas engine info!\n");
		__evas_free_p(canvas);
		return NULL;
	}

	return canvas;
}

static void destroy_canvas(Evas* canvas)
{
	static Evas_Engine_Info *(*__evas_engine_info_get_p)(const Evas *e);
	static void (*__evas_free_p)(Evas *e);
	Evas_Engine_Info_Buffer *einfo;

	rtld_default_set_once(__evas_engine_info_get_p, "evas_engine_info_get");
	rtld_default_set_once(__evas_free_p, "evas_free");

	einfo = (Evas_Engine_Info_Buffer *)__evas_engine_info_get_p(canvas);
	if (unlikely(einfo == NULL))
	{
		//fputs("ERROR: could not get evas engine info!\n", stderr);
		__evas_free_p(canvas);
		return;
	}

	free(einfo->info.dest_buffer);
	__evas_free_p(canvas);
}

int captureScreen()
{
	static void (*__evas_object_resize_p)(Evas_Object *obj, Evas_Coord w, Evas_Coord h);
	static void (*__evas_object_image_data_set_p)(Evas_Object *obj, void *data);
	static void (*__evas_object_image_size_set_p)(Evas_Object *obj, int w, int h);
	static void (*__evas_object_image_fill_set_p)(Evas_Object *obj, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h);
	static void (*__evas_object_image_data_update_add_p)(Evas_Object *obj, int x, int y, int w, int h);
	static Eina_Bool (*__evas_object_image_save_p)(Evas_Object *obj, const char *file, const char *key, const char *flags);
	static Evas_Object *(*__evas_object_image_add_p)(Evas *e);
	static pthread_mutex_t captureScreenLock = PTHREAD_MUTEX_INITIALIZER;
	char dstpath[MAX_PATH_LENGTH];
	char* scrimage;
	int width, height;
	Evas* ev = NULL;
	Evas_Object* img;
	screenshot_data sdata;
	probeInfo_t	probeInfo;
	int ret = 0;

	rtld_default_set_once(__evas_object_resize_p, "evas_object_resize");
	rtld_default_set_once(__evas_object_image_data_set_p, "evas_object_image_data_set");
	rtld_default_set_once(__evas_object_image_size_set_p, "evas_object_image_size_set");
	rtld_default_set_once(__evas_object_image_fill_set_p, "evas_object_image_fill_set");
	rtld_default_set_once(__evas_object_image_data_update_add_p, "evas_object_image_data_update_add");
	rtld_default_set_once(__evas_object_image_save_p, "evas_object_image_save");
	rtld_default_set_once(__evas_object_image_add_p, "evas_object_image_add");

	pthread_mutex_lock(&captureScreenLock);

	setProbePoint(&probeInfo);
	sdata.ximage = NULL;
	scrimage = captureScreenShotX(&width, &height, &sdata);
	if(scrimage != NULL)
	{
		ev = create_canvas(width, height);
		if(likely(ev != NULL))
		{
			snprintf(dstpath, sizeof(dstpath),
				 SCREENSHOT_DIRECTORY "/%d_%d.png", getpid(),
				 probeInfo.eventIndex);

			// make image buffer
			if((img = __evas_object_image_add_p(ev)) != NULL)
			{
				//image buffer set
				__evas_object_image_data_set_p(img, NULL);
				__evas_object_image_size_set_p(img, width, height);
				__evas_object_image_data_set_p(img, scrimage);

				// resize image
				if(height > MAX_HEIGHT)
				{
					width = width * MAX_HEIGHT / height;
					height = MAX_HEIGHT;
					__evas_object_resize_p(img, width, height);
					__evas_object_image_fill_set_p(img, 0, 0, width, height);
				}
				__evas_object_image_data_update_add_p(img, 0, 0, width, height);

				//save file
				if(__evas_object_image_save_p(img, dstpath, NULL, "compress=5") != 0)
				{
					chmod(dstpath, 0777);

					/* welcome to the hell */
					log_t log;
					PREPARE_LOCAL_BUF_THOUGH((char *)&log);

					/* skip header */
					LOCAL_BUF += offsetof(log_t, data);
					/* pack screenshot name */
					BUF_PTR = pack_string(LOCAL_BUF, dstpath); /* file name */
					LOCAL_BUF = BUF_PTR;

					/* pack probe */
					PACK_COMMON_BEGIN(MSG_PROBE_SCREENSHOT, API_ID_captureScreen, "", 0);
					PACK_COMMON_END_THOUGH('d', 0, 0, 0);
					PACK_SCREENSHOT(dstpath, getOrientation());
					SET_MSG_LEN();
					log.length = GET_MSG_LEN() + MSG_HDR_LEN + strlen(dstpath) + 1;

					/* send all message */
					printLog(&log, APP_MSG_IMAGE);
				}
				else
				{
					// captureScreen : evas_object_image_save failed
					ret = -1;
				}
			}
			else
			{
				// captureScreen : evas_object_image_add failed
				ret = -1;
			}
		}
		else
		{
			// captureScreen : create canvas failed
			ret = -1;
		}
	}
	else
	{
		// captureScreen : captureScreenShotX failed
		ret = -1;
	}

	// release resources
	releaseScreenShotX(&sdata);
	if(ev)
		destroy_canvas(ev);

	pthread_mutex_unlock(&captureScreenLock);
	return ret;
}

int initialize_screencapture()
{
	// remove all previous screenshot in dir
//	remove_indir(SCREENSHOT_DIRECTORY);

	// make screenshot directory
//	mkdir(SCREENSHOT_DIRECTORY, 0777);

	return 0;
}

int finalize_screencapture()
{
	return 0;
}

// =======================================================================
// screen shot manipulation functions
// =======================================================================

static Eina_Bool _captureTimer(void __unused * data)
{
	SCREENSHOT_TIMEOUT();

	return ECORE_CALLBACK_CANCEL;
}

int activateCaptureTimer()
{
	static Ecore_Timer *(*__ecore_timer_add_p)(double in, Ecore_Task_Cb func, const void *data);

	rtld_default_set_once(__ecore_timer_add_p, "ecore_timer_add");

	__ecore_timer_add_p(CAPTURE_TIMEOUT, _captureTimer, NULL);
	return 0;
}

void _cb_render_post(void __unused * data, Evas __unused * e,
		     void __unused * eventinfo)
{
	SCREENSHOT_DONE();
}
