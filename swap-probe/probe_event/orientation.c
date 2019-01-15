/*
 *  DA probe
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact:
 *
 * Jaewon Lim <jaewon81.lim@samsung.com>
 * Woojin Jung <woojin2.jung@samsung.com>
 * Juyoung Kim <j0.kim@samsung.com>
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

#include <Ecore.h>
#include <Ecore_X.h>

#include "daprobe.h"
#include "dahelper.h"
#include "common_probe_init.h"


Ecore_Event_Handler *register_orientation_event_listener();
void unregister_orientation_event_listener(Ecore_Event_Handler *handler);

Ecore_Event_Handler *handler = NULL;


Eina_Bool _da_onclientmessagereceived(void __unused *pData, int __unused type,
				      void *pEvent)
{
	static Ecore_X_Atom *__ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_PREPARE_P = NULL;
	Ecore_X_Event_Client_Message *pClientEvent;

	rtld_default_set_once(__ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_PREPARE_P, "ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_PREPARE");

	pClientEvent = (Ecore_X_Event_Client_Message*)pEvent;

	if (pClientEvent != NULL) {

		//This code from ecore_x
		//So I don't know what 32 does mean
		if (pClientEvent->format != 32)
			return ECORE_CALLBACK_PASS_ON;

		if (pClientEvent->message_type ==
		    *__ECORE_X_ATOM_E_WINDOW_ROTATION_CHANGE_PREPARE_P) {
			int orientation = (int)pClientEvent->data.l[1];
			on_orientation_changed(orientation, false);
		}
	}

	return ECORE_CALLBACK_RENEW;
}

Ecore_Event_Handler *register_orientation_event_listener()
{
	static Ecore_Event_Handler *(*__ecore_event_handler_add_p)(int type, Ecore_Event_Handler_Cb func, const void *data) = NULL;
	static int *__ECORE_X_EVENT_CLIENT_MESSAGE_P = NULL;
	Ecore_Event_Handler *handler;

	rtld_default_set_once(__ecore_event_handler_add_p, "ecore_event_handler_add");
	rtld_default_set_once(__ECORE_X_EVENT_CLIENT_MESSAGE_P, "ECORE_X_EVENT_CLIENT_MESSAGE");

	handler = __ecore_event_handler_add_p(*__ECORE_X_EVENT_CLIENT_MESSAGE_P,
					  _da_onclientmessagereceived, NULL);

	return handler;
}

void unregister_orientation_event_listener(Ecore_Event_Handler *handler)
{
	static void *(*__ecore_event_handler_del_p)(Ecore_Event_Handler *event_handler) = NULL;

	rtld_default_set_once(__ecore_event_handler_del_p, "ecore_event_handler_del");

	if (handler)
		__ecore_event_handler_del_p(handler);
}

EAPI int PROBE_NAME(ecore_x_init)(const char *name)
{
	static Ecore_Event_Handler *event_handler = NULL;

	int res = 0;
	static int (*ecore_x_initp)(const char *name);
	PRINTMSG("(%s)", name);

	rtld_default_set_once(ecore_x_initp, "ecore_x_init");
	res = ecore_x_initp(name);

	if (event_handler == NULL) {
		event_handler = register_orientation_event_listener();
		if (event_handler == NULL)
			PRINTERR("Fail to init event listener");
	}
	return res;
}
