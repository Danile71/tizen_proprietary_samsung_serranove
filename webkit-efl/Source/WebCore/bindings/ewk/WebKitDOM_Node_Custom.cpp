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
#include "WebKitDOM_Node.h"
#include "WebKitDOM_Node_Private.h"

#include "Node.h"
#include <wtf/text/CString.h>
#include <wtf/GetPtr.h>
#include <wtf/RefPtr.h>


void ewk_webkitdom_node_insert_before(WebKitDOM_Node* self, WebKitDOM_Node* newChild, WebKitDOM_Node* refChild, WebKitDOM_Node* ret)
{
    if (!self)
        return;

    WebCore::Node* coreObj = _to_webcore_node(self);
    WebCore::Node* newChildCore = _to_webcore_node(newChild);
    WebCore::Node* refChildCore = _to_webcore_node(refChild);

    WebCore::ExceptionCode ec = 0;
    if (coreObj->insertBefore( newChildCore, refChildCore, ec))
    {
        _copy_webkit_node(newChild, ret);
        return;
    }

    _to_webkit_node(0, ret);
}

void ewk_webkitdom_node_append_child(WebKitDOM_Node* self, WebKitDOM_Node* newChild, WebKitDOM_Node* ret)
{
    if (!self)
        return;

    WebCore::Node* coreObj = _to_webcore_node(self);
    WebCore::Node* newChildCore = _to_webcore_node(newChild);

    WebCore::ExceptionCode ec = 0;
    if (coreObj->appendChild(newChildCore, ec))
    {
        _copy_webkit_node(newChild, ret);
        return;
    }

    _to_webkit_node(0, ret);
}

