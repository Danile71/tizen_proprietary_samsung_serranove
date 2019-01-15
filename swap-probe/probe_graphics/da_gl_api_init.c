/*
 *  DA probe
 *
 * Copyright (c) 2000 - 2014 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Vitaliy Cherepanov <v.cherepanov@samsung.com>
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
 *
 */

#include <unistd.h>
#include "da_gles20.h"
#include "binproto.h"
#include "common_probe_init.h"
#include "real_functions.h"

/* GL __local_* functions prototypes */
#define X(func) extern void __local_##func(void);
#include "da_gl_api_func_list.h"
	GL_ALL_FUNCTIONS;
#undef X
/* --------------------------------- */

Evas_GL_API *__gl_api = NULL;
static Evas_GL_API __gl_api_fake;

void __init_gl_api__(void)
{
	memset(&__gl_api_fake, 0, sizeof(__gl_api_fake));
}

/* IMPORTANT this code must be right before change_gl_api_list function!
 *
 * next define is GL api replacing by probe functions
 *
 */
#define X(func) \
	do {									\
		__gl_api_fake.func = (typeof(api->func)) __local_##func;	\
		if (__gl_api_fake.func == NULL)					\
			PRINTWRN("api->%s not setted", #func);			\
	} while(0);
#include "da_gl_api_func_list.h"
/* --------------------------------------------------------------------- */

Evas_GL_API *get_gl_api_fake_list(Evas_GL_API *api)
{
	/* change links */

	if (api == NULL)
		return NULL;

	__gl_api = api;

	/* clone api to fake */
	memcpy(&__gl_api_fake, api, sizeof(*api));

	/* replace by our functions */
	GL_ALL_FUNCTIONS;

	return &__gl_api_fake;
}

#undef X
