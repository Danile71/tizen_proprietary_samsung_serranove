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
#include "WebKitDOM_String.h"
#include "WebKitDOM_String_Private.h"
#include <string.h>

#include "KURL.h"
#include "WTFString.h"
#include "PlatformString.h"
#include <wtf/text/CString.h>

char* ewk_webkitdom_string_get_cstring(WebKitDOM_String* self)
{
    return strdup( _to_webcore_string(self)->utf8().data() );
}

void ewk_webkitdom_string_set_data(WebKitDOM_String* self, char* data)
{
    WTF::String* tmpPtr = _to_webcore_string(self);
    if (tmpPtr)
        delete tmpPtr;

    ((WebKitDOM_Object*)self)->p_coreObject = new WTF::String( data );
}

void ewk_webkitdom_string_get_substring(WebKitDOM_String* self, unsigned pos, unsigned len, WebKitDOM_String* ret)
{
    if( WEBKITDOM_EQUAL(self, ret ) )
        return;
    WEBKITDOM_DEINIT( ret );

    ((WebKitDOM_Object*)ret)->p_coreObject = new WTF::String( _to_webcore_string(self)->substring( pos, len) );
}

unsigned ewk_webkitdom_string_get_length(WebKitDOM_String* self)
{
    return _to_webcore_string(self)->length();
}

EAPI void ewk_webkitdom_string_deinit(const WebKitDOM_Object* objPtr)
{
    if( WEBKITDOM_ISNULL(objPtr) )
       return;
    delete ((WTF::String*)(objPtr->p_coreObject));
    (((WebKitDOM_Object*)(objPtr))->p_coreObject) = 0;
}

void _copy_webkit_string(WebKitDOM_String* kitObjSrc, WebKitDOM_String* ret)
{
    if (WEBKITDOM_COREOBJ(kitObjSrc) == WEBKITDOM_COREOBJ(ret))
        return;
    ((WebKitDOM_Object*)ret)->p_coreObject = WEBKITDOM_COREOBJ(kitObjSrc);
}

WTF::String* _to_webcore_string(const WebKitDOM_String* kitObj)
{
    return (WTF::String*)WEBKITDOM_COREOBJ(kitObj);
}

WTF::String _to_webcore_string_value(const WebKitDOM_String* kitObj)
{
    return *((WTF::String*)WEBKITDOM_COREOBJ(kitObj));
}

WebKitDOM_String* _to_webkit_string(WTF::String* coreObj, WebKitDOM_String* ret)
{
    WTF::String* tmpPtr = _to_webcore_string(ret);
    if (tmpPtr && tmpPtr != coreObj)
        delete tmpPtr;
    ((WebKitDOM_Object*)ret)->p_coreObject = new WTF::String( *coreObj );
    return ret;
}

