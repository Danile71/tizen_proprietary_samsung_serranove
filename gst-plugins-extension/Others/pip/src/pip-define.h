/*
 * Pip
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

#ifndef __PIP_DEFINE_H
#define __PIP_DEFINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <gst/gst.h>
#include <pthread.h>
#include <unistd.h>

#ifndef GST_CAT_DEFAULT
GST_DEBUG_CATEGORY_EXTERN(gst_pip_debug);
#define GST_CAT_DEFAULT gst_pip_debug
#endif /* GST_CAT_DEFAULT */

#define startfunc   		GST_DEBUG("+-  START -------------------------");
#define endfunc     		GST_DEBUG("+-  END  --------------------------");

#define IF_FREE(object) do { \
	if(object) { \
		free(object); \
		object = NULL; \
	} \
} while (0)

#define IF_GST_BUFFER_UNREF(buffer) do { \
        if(buffer) { \
                gst_buffer_unref(buffer); \
        } \
} while (0)

#define IF_G_MUTEX_LOCK(mutex) do { \
        if(mutex) { \
                g_mutex_lock(mutex); \
        } \
} while (0)

#define IF_G_COND_WAIT(cond, mutex) do { \
        if(cond && mutex) { \
                g_cond_wait(cond, mutex); \
        } \
} while (0)

#define IF_G_COND_SIGNAL(cond) do { \
        if(cond) { \
                g_cond_signal(cond); \
        } \
} while (0)

#define IF_G_MUTEX_UNLOCK(mutex) do { \
        if(mutex) { \
                g_mutex_unlock(mutex); \
        } \
} while (0)

#define IF_G_MUTEXT_FREE(mutex) do { \
	if(mutex) { \
		g_mutex_free(mutex); \
		mutex = NULL; \
	} \
} while (0)

#define IF_G_COND_FREE(cond) do { \
	if(cond) { \
		g_cond_free(cond); \
		cond = NULL; \
	} \
} while (0)


#define IF_G_THREAD_JOIN(thread) do { \
	if(thread) { \
		g_thread_join(thread); \
		thread = NULL; \
	} \
} while (0)

#define _PIP_MIME_TYPE_IMAGE_JPEG "image/jpeg"
#define _PIP_MIME_TYPE_VIDEO_RGB888 "video/x-raw-rgb"

#endif

