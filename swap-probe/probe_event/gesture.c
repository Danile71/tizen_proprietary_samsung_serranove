/*
 *  DA probe
 *
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
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
 * - Samsung RnD Institute Russia
 *
 */

#include "daprobe.h"
#include "dahelper.h"
#include "probeinfo.h"
#include "tizen_probe.h"
#include "dacollection.h"
#include "da_event.h"
#include "binproto.h"

#include "common_probe_init.h"

#include "gesture.h"
#include "da_event.h"

#define LOGW PRINTWRN

#define MAX_INFO1_LEN 64

/* START - pack for gestures */

/*
   ELM_GESTURE_N_TAPS,
   ELM_GESTURE_N_LONG_TAPS,
   ELM_GESTURE_N_DOUBLE_TAPS,
   ELM_GESTURE_N_TRIPLE_TAPS,
*/
static inline char *__n_finger_tap_pack(char *to, void *event_info)
{
	Elm_Gesture_Taps_Info *ev = (Elm_Gesture_Taps_Info *) event_info;
	char *p = to;

	PACK_UIEVENT_XY(ev->x, ev->y);
	PACK_UIEVENT_INFO1(NULL);
	PACK_UIEVENT_INFO2(ev->n);

	return p;
}

/*
   ELM_GESTURE_MOMENTUM,
*/
static inline char *__momentum_pack(char *to, void *event_info)
{
	Elm_Gesture_Momentum_Info *ev = (Elm_Gesture_Momentum_Info *) event_info;
	char *p = to;

	PACK_UIEVENT_XY(ev->x1, ev->y1);
	PACK_UIEVENT_INFO1(NULL);
	PACK_UIEVENT_INFO2(ev->n);

	return p;
}

/*
   ELM_GESTURE_N_LINES,
   ELM_GESTURE_N_FLICKS,
*/
static inline char *__line_flick_pack(char *to, void *event_info)
{
	Elm_Gesture_Line_Info *ev = (Elm_Gesture_Line_Info *) event_info;
	char *p = to;

	PACK_UIEVENT_XY(ev->momentum.x1, ev->momentum.y1);
	p += snprintf(p, MAX_INFO1_LEN, "%d, %d, %d, %d",
		      ev->momentum.x2, ev->momentum.y2,
		      ev->momentum.mx, ev->momentum.my) + 1;
	PACK_UIEVENT_INFO2((int)ev->angle);

	return p;
}

/*
    ELM_GESTURE_ZOOM
*/
static inline char *__zoom_pack(char *to, void *event_info)
{
	Elm_Gesture_Zoom_Info *ev = (Elm_Gesture_Zoom_Info *) event_info;
	char *p = to;

	PACK_UIEVENT_XY(ev->x, ev->y);
	p += snprintf(p, MAX_INFO1_LEN, "%f, %f", ev->zoom, ev->momentum) + 1;
	PACK_UIEVENT_INFO2(ev->radius);

	return p;
}

/*
    ELM_GESTURE_ROTATE
*/
static inline char *__rotate_pack(char *to, void *event_info)
{
	Elm_Gesture_Rotate_Info *ev = (Elm_Gesture_Rotate_Info *) event_info;
	char *p = to;

	PACK_UIEVENT_XY(ev->x, ev->y);
	p += snprintf(p, MAX_INFO1_LEN, "%f, %f, %f", ev->base_angle, ev->angle,
		      ev->momentum) + 1;
	PACK_UIEVENT_INFO2(ev->radius);

	return p;
}
/* END   - pack for gestures */

char *__pack_callback_data(char *to,
			   struct __elm_gesture_layer_cb_set_data *data,
			   void *event_info)
{
	char *res = NULL;

	if (to == NULL) {
		PRINTERR("destination pointer is NULL");
		return NULL;
	}

	switch (data->idx) {
	case ELM_GESTURE_N_TAPS: /**< N fingers single taps */
	case ELM_GESTURE_N_LONG_TAPS: /**< N fingers single long-taps */
	case ELM_GESTURE_N_DOUBLE_TAPS: /**< N fingers double-single taps */
	case ELM_GESTURE_N_TRIPLE_TAPS: /**< N fingers triple-single taps */
		res = __n_finger_tap_pack(to, event_info);
		break;

	case ELM_GESTURE_MOMENTUM: /**< Reports momentum in the direction of movement */
		res = __momentum_pack(to, event_info);
		break;

	case ELM_GESTURE_N_LINES: /**< N fingers line gesture */
	case ELM_GESTURE_N_FLICKS: /**< N fingers flick gesture */
		res = __line_flick_pack(to, event_info);
		break;

	case ELM_GESTURE_ZOOM: /**< Zoom */
		res = __zoom_pack(to, event_info);
		break;

	case ELM_GESTURE_ROTATE: /**< Rotate */
		res = __rotate_pack(to, event_info);
		break;
	default:
		PRINTERR("wrong event: obj=%p idx=%d cb_type=%d cb=%p data=%p",
			 data->obj, data->idx, data->cb_type, data->cb, data->data);
	}

	return res;
}

static inline enum __event_type __get_event_type_code(Elm_Gesture_Type idx)
{
	/* I am so sorry */
	switch (idx) {
	case ELM_GESTURE_N_TAPS: return _EVENT_GESTURE_N_TAPS;
	case ELM_GESTURE_N_LONG_TAPS: return _EVENT_GESTURE_N_LONG_TAPS;
	case ELM_GESTURE_N_DOUBLE_TAPS: return _EVENT_GESTURE_N_DOUBLE_TAPS;
	case ELM_GESTURE_N_TRIPLE_TAPS: return _EVENT_GESTURE_N_TRIPLE_TAPS;
	case ELM_GESTURE_MOMENTUM: return _EVENT_GESTURE_MOMENTUM;
	case ELM_GESTURE_N_LINES: return _EVENT_GESTURE_N_LINES;
	case ELM_GESTURE_N_FLICKS: return _EVENT_GESTURE_N_FLICKS;
	case ELM_GESTURE_ZOOM: return _EVENT_GESTURE_ZOOM;
	case ELM_GESTURE_ROTATE: return _EVENT_GESTURE_ROTATE;
	default:
		PRINTERR("wrong event type");
	}

	return _EVENT_UNKNOWN;
}

/* Callbacks */
Evas_Event_Flags __common_elm_gesture_layer_cb(void *data , void *event_info)
{
	probeInfo_t	probeInfo;
	struct __elm_gesture_layer_cb_set_data *d = data;
	Evas_Event_Flags res;

	res = d->cb(d->data, event_info);

	setProbePoint(&probeInfo);
	PREPARE_LOCAL_BUF();
	PACK_COMMON_BEGIN(MSG_PROBE_UIEVENT, API_ID___common_elm_gesture_layer_cb, "p", CALLER_ADDRESS);
	PACK_COMMON_END('d', res, 0, 0);
	PACK_UIEVENT_HEAD(__get_event_type_code(d->idx), d->cb_type);

	BUF_PTR = __pack_callback_data(BUF_PTR, data, event_info);

	FLUSH_LOCAL_BUF();

	return res;
}

void PROBE_NAME(elm_gesture_layer_cb_set)(Evas_Object *obj, Elm_Gesture_Type idx,
			      Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb,
			      void *data)
{
	struct __elm_gesture_layer_cb_set_data d, *elm;
	static void (*elm_gesture_layer_cb_setp)(Evas_Object *obj, Elm_Gesture_Type idx,
		      Elm_Gesture_State cb_type, Elm_Gesture_Event_Cb cb,
		      void *data);

	d.obj = obj;
	d.idx = idx;
	d.cb_type = cb_type;
	d.cb = cb;
	d.data = data;

	elm = add_gesture_hash(&d);

	GET_REAL_FUNCP_RTLD_DEFAULT(elm_gesture_layer_cb_set, elm_gesture_layer_cb_setp);

	elm_gesture_layer_cb_setp(obj, idx, cb_type, __common_elm_gesture_layer_cb, elm);
	return;
}

