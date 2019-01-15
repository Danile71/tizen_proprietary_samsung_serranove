/*
    Copyright (C) 2011 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef WebKitDOM_EventTarget_h
#define WebKitDOM_EventTarget_h

#include "WebKitDOM_Object.h"
#include "WebKitDOM_Defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEBKITDOM_NODELIST_INIT(objPtr) \
    (((WebKitDOM_Object*)(objPtr))->p_coreObject) = 0; \
    (((WebKitDOM_Object*)(objPtr))->deinit) = ewk_webkitdom_nodelist_deinit

struct _WebKitDOM_EventTarget {
    WebKitDOM_Object parent_object;

    void          (* dispatch_event)(WebKitDOM_EventTarget* target, WebKitDOM_Event* event, int** error);
};


EAPI void ewk_webkitdom_eventtarget_dispatch_event(WebKitDOM_EventTarget* target, WebKitDOM_Event* event, int** error);

EAPI void ewk_webkitdom_eventtarget_deinit(const WebKitDOM_Object*);

#ifdef __cplusplus
}
#endif

#endif
