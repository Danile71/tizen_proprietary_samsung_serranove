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

#ifndef WebKitDOM_Object_h
#define WebKitDOM_Object_h

#include <Eina.h>
#include <Evas.h>

#define WEBKITDOM_CAST(type,pointer) ((type*)(pointer))
#define WEBKITDOM_COREOBJ(pointer) (((WebKitDOM_Object*)pointer)->p_coreObject)
#define WEBKITDOM_ISNULL(pointer) ((((WebKitDOM_Object*)pointer)->p_coreObject) == 0)
#define WEBKITDOM_EQUAL(pointer1, pointer2) (((WebKitDOM_Object*)(pointer1))->p_coreObject == ((WebKitDOM_Object*)(pointer2))->p_coreObject)

#define WEBKITDOM_DEINIT(objPtr) (((WebKitDOM_Object*)objPtr)->deinit)((const WebKitDOM_Object*)objPtr)

typedef void* WebCoreObjectPointer;

typedef struct _WebKitDOM_Object WebKitDOM_Object;
struct _WebKitDOM_Object {
    WebCoreObjectPointer p_coreObject;

    void (*deinit)(const WebKitDOM_Object*);
};

#endif
