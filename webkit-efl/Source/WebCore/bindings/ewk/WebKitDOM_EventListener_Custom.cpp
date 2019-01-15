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
#include "WebKitDOM_EventListener.h"
#include "WebKitDOM_EventListener_Private.h"

#include "WebKitDOM_Event.h"
#include "WebKitDOM_Event_Private.h"

#include "Event.h"
#include "EventListener.h"
#include "WebKitDOM_String.h"
#include "WebKitDOM_String_Private.h"
#include <wtf/text/CString.h>
#include <wtf/GetPtr.h>
#include <wtf/RefPtr.h>

void ewk_webkitdom_eventlistener_handle_event(WebKitDOM_EventListener* self, const WebKitDOM_Event* evt)
{
    WebCore::EventListener* coreObj = _to_webcore_eventlistener(self);

    coreObj->handleEvent(0, _to_webcore_event(evt) );
}

