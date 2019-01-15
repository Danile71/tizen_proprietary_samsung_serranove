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

#ifndef __GESTURE_H__
#define __GESTURE_H__

#include <Evas.h>
#include <elm_gesture_layer.h>

struct __elm_gesture_layer_cb_set_data{
	Evas_Object *obj;
	Elm_Gesture_Type idx;
	Elm_Gesture_State cb_type;
	Elm_Gesture_Event_Cb cb;
	void *data;
};

//int kh_gesture_cmp(struct __elm_gesture_layer_cb_set_data *elm1,
//		   struct __elm_gesture_layer_cb_set_data *elm2);
//uint32_t kh_gesture_calc_hash(struct __elm_gesture_layer_cb_set_data *elm);

int kh_gesture_cmp(void *elm1, void *elm2);
uint32_t kh_gesture_calc_hash(void *elm);


#endif
