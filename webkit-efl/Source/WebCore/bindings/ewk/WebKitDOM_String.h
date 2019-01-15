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

#ifndef WebKitDOM_String_h
#define WebKitDOM_String_h

#include "WebKitDOM_Object.h"
#include "WebKitDOM_Defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WEBKITDOM_STRING_INIT(objPtr) \
    (((WebKitDOM_Object*)(objPtr))->p_coreObject) = 0; \
    (((WebKitDOM_Object*)(objPtr))->deinit) = ewk_webkitdom_string_deinit

struct _WebKitDOM_String {
    WebKitDOM_Object parent_object;
};

EAPI char* ewk_webkitdom_string_get_cstring(WebKitDOM_String* self);
EAPI void ewk_webkitdom_string_set_data(WebKitDOM_String* self, char* data);

EAPI void ewk_webkitdom_string_get_substring(WebKitDOM_String* self, unsigned pos, unsigned len, WebKitDOM_String* ret);
EAPI unsigned ewk_webkitdom_string_get_length(WebKitDOM_String* self);

EAPI void ewk_webkitdom_string_deinit(const WebKitDOM_Object*);
EAPI void _copy_webkit_string(WebKitDOM_String* kitObjSrc, WebKitDOM_String* ret);

#ifdef __cplusplus
}
#endif

#endif
