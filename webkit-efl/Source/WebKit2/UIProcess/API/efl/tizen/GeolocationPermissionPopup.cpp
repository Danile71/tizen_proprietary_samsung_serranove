/*
   Copyright (C) 2014 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "GeolocationPermissionPopup.h"

#if ENABLE(TIZEN_GEOLOCATION)

#include "ewk_view.h"
#include "ewk_geolocation.h"

namespace WebKit {

GeolocationPermissionPopup::GeolocationPermissionPopup(Ewk_Geolocation_Permission_Request* request, const Ewk_Security_Origin* origin, const String& message)
    : PermissionPopup(origin, message)
    , m_request(request)
{
}

void GeolocationPermissionPopup::sendDecidedPermission(Evas_Object* ewkView, bool isDecided)
{
    ewk_geolocation_permission_reply(m_request, isDecided);
}

} // namespace WebKit

#endif // ENABLE(TIZEN_GEOLOCATION)
