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
 * - Samsung RnD Institute Russia
 *
 */

//disable tizen redefines
#define _GL2_MACRO_H_
#define _GL_MACRO_H_

#include "common_probe_init.h"
#ifdef __cplusplus
extern "C" {
#endif

#define REAL_NAME(func) PROBE_NAME(func)
#define BEFORE BEFORE_GL_ORIG
#define CALL_ORIG(func, ...) func##p(__VA_ARGS__)
#define TYPEDEF(type) typedef type

#include "da_gles20.inc"

#ifdef __cplusplus
} /* extern C */
#endif

#undef _GL2_MACRO_H_
#undef _GL_MACRO_H_

