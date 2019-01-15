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

#ifndef WebKitDOM_String_Private_h
#define WebKitDOM_String_Private_h

#include <wtf/Forward.h>

namespace WebCore {
    class KURL;
}

#ifdef __cplusplus
extern "C" {
#endif

WTF::String* _to_webcore_string(const WebKitDOM_String* kitObj);
WTF::String _to_webcore_string_value(const WebKitDOM_String* kitObj);

WebKitDOM_String* _to_webkit_string(WTF::String* coreObj, WebKitDOM_String* ret);


#ifdef __cplusplus
}
#endif

#endif
