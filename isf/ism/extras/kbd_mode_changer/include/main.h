/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Wonkeun Oh <wonkeun.oh@samsung.com>, Jihoon Kim <jihoon48.kim@samsung.com>
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

#ifndef __MAIN_H__
#define __MAIN_H__

#include <Elementary.h>
#include <app.h>
#include <dlog.h>

#if !defined(PACKAGE)
#  define PACKAGE "isf-kbd-mode-changer"
#endif

#if !defined(PKGNAME)
#  define PKGNAME "org.tizen.isf-kbd-mode-changer"
#endif

#ifdef LOG_TAG
# undef LOG_TAG
#endif
#define LOG_TAG "ISF_KBD_MODE_CHANGER"

#endif /* __MAIN_H__ */
