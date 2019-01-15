/*
   Copyright (C) 2012 Samsung Electronics

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
#include "ewk_geolocation.h"

#if ENABLE(TIZEN_GEOLOCATION)
#include "EwkViewImpl.h"
#include "WKAPICast.h"
#include "WKGeolocationManager.h"
#include "WKGeolocationPermissionRequest.h"
#include "WKGeolocationPosition.h"
#include "WKRetainPtr.h"
#include "ewk_context_private.h"
#include "ewk_geolocation_private.h"
#include "ewk_view_private.h"

#include <Evas.h>
#include <location/locations.h>
#include <wtf/text/CString.h>

using namespace WebKit;

struct _Ewk_Geolocation_Permission_Request {
    WKRetainPtr<WKGeolocationPermissionRequestRef> requestRef;
    WKPageRef page;
    Ewk_Security_Origin* origin;
    bool isDecided;
    bool isSuspended;

    _Ewk_Geolocation_Permission_Request(WKPageRef pageRef, WKRetainPtr<WKGeolocationPermissionRequestRef> geolocationPermissionRequestRef, WKSecurityOriginRef originRef)
        : requestRef(geolocationPermissionRequestRef)
        , page(pageRef)
        , isDecided(false)
        , isSuspended(false)
    {
        origin = createSecurityOrigin(originRef);
    }

    ~_Ewk_Geolocation_Permission_Request()
    {
        deleteSecurityOrigin(origin);
    }
};

Ewk_Geolocation_Permission_Request* ewkGeolocationCreatePermissionRequest(WKPageRef pageRef, WKGeolocationPermissionRequestRef geolocationPermissionRequestRef, WKSecurityOriginRef originRef)
{
    return new Ewk_Geolocation_Permission_Request(pageRef, geolocationPermissionRequestRef, originRef);
}
#endif

const Ewk_Security_Origin* ewk_geolocation_permission_request_origin_get(const Ewk_Geolocation_Permission_Request* permissionRequest)
{
#if ENABLE(TIZEN_GEOLOCATION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(permissionRequest, 0);

    return permissionRequest->origin;
#else
    UNUSED_PARAM(permissionRequest);
    return 0;
#endif
}

void ewk_geolocation_permission_reply(Ewk_Geolocation_Permission_Request* permissionRequest, Eina_Bool allow)
{
#if ENABLE(TIZEN_GEOLOCATION)
    TIZEN_LOGI("geolocation allow %d", allow);
    EINA_SAFETY_ON_NULL_RETURN(permissionRequest);

    Evas_Object* ewkView = const_cast<Evas_Object*>(EwkViewImpl::viewFromPageViewMap(permissionRequest->page));
    if (ewkView)
        allow ? WKGeolocationPermissionRequestAllow(permissionRequest->requestRef.get()) : WKGeolocationPermissionRequestDeny(permissionRequest->requestRef.get());

    delete permissionRequest;
#else
    UNUSED_PARAM(permissionRequest);
    UNUSED_PARAM(allow);
#endif
}

Eina_Bool ewk_geolocation_permission_request_set(Ewk_Geolocation_Permission_Request* permissionRequest, Eina_Bool allow)
{
#if ENABLE(TIZEN_GEOLOCATION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(permissionRequest, false);
    TIZEN_LOGI("allow(%d)", allow);

    permissionRequest->isDecided = true;

    Evas_Object* ewkView = const_cast<Evas_Object*>(EwkViewImpl::viewFromPageViewMap(permissionRequest->page));
    if (ewkView) {
        allow ? WKGeolocationPermissionRequestAllow(permissionRequest->requestRef.get()) : WKGeolocationPermissionRequestDeny(permissionRequest->requestRef.get());
        delete permissionRequest;
    }

    return true;
#else
    UNUSED_PARAM(permissionRequest);
    UNUSED_PARAM(allow);
    return false;
#endif
}

void ewk_geolocation_permission_request_suspend(Ewk_Geolocation_Permission_Request* permissionRequest)
{
#if ENABLE(TIZEN_GEOLOCATION)
    EINA_SAFETY_ON_NULL_RETURN(permissionRequest);
    TIZEN_LOGI("request(%d) is suspended", permissionRequest);

    permissionRequest->isSuspended = true;
#else
    UNUSED_PARAM(permissionRequest);
#endif
}
