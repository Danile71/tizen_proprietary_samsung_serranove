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

#include "config.h"
#include "WebKitDOM_EventTarget.h"
#include "WebKitDOM_EventTarget_Private.h"

#include "EventTarget.h"

#include "WebKitDOM_String.h"
#include "WebKitDOM_String_Private.h"
#include "wtf/text/AtomicString.h"
#include <wtf/GetPtr.h>
#include <wtf/RefPtr.h>


void ewk_webkitdom_eventtarget_dispatch_event(WebKitDOM_EventTarget* target, WebKitDOM_Event* event, int** error)
{
    if (target->dispatch_event)
        target->dispatch_event(target, event, error);
}

WebCore::EventTarget* _to_webcore_eventtarget(WebKitDOM_EventTarget* kitObj)
{
    return (WebCore::EventTarget*)WEBKITDOM_COREOBJ(kitObj);
}

WebKitDOM_EventTarget* _to_webkit_eventtarget(WebCore::EventTarget* coreObj, WebKitDOM_EventTarget* ret)
{
    WebCore::EventTarget* tmpPtr = _to_webcore_eventtarget(ret);
    if (tmpPtr && tmpPtr != coreObj)
        tmpPtr->deref();
    if (coreObj)
        coreObj->ref();
    ((WebKitDOM_Object*)ret)->p_coreObject = coreObj;
    return ret;
}

void ewk_webkitdom_eventtarget_deinit(const WebKitDOM_Object* objPtr)
{
    if( WEBKITDOM_ISNULL(objPtr) )
       return;
    ((WebCore::EventTarget*)objPtr)->deref();
    (((WebKitDOM_Object*)(objPtr))->p_coreObject) = 0;
}

