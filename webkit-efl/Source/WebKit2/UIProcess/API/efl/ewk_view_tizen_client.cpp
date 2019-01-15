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
#include "ewk_view_tizen_client.h"

#if PLATFORM(TIZEN)
#include "EwkViewImpl.h"
#include "WKFrame.h"
#include "WKPageTizen.h"
#include "WKString.h"
#include "ewk_user_media_private.h"
#include "ewk_view.h"
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
#include "ewk_certificate.h"
#endif

static bool decidePolicyForApplicationCachePermissionRequest(WKPageRef page, WKSecurityOriginRef origin, WKFrameRef frame, const void *clientInfo)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    return ewkViewRequestApplicationCachePermission(static_cast<Evas_Object*>(const_cast<void*>(clientInfo)), origin);
#else
    return false;
#endif
}

static void decidePolicyForUserMediaPermissionRequest(WKPageRef page, WKSecurityOriginRef origin, WKUserMediaPermissionRequestRef permissionRequest, const void *clientInfo)
{
#if ENABLE(TIZEN_MEDIA_STREAM)
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));
    Ewk_User_Media_Permission_Request* userMediaPermissionRequest = ewkUserMediaPermissionRequestCreate(page, permissionRequest, origin);

    if (!ewkViewRequestUserMediaPermission(ewkView, userMediaPermissionRequest))
        WKUserMediaPermissionRequestDeny(permissionRequest);
#endif
}

static void processJSBridgePlugin(WKPageRef page, WKStringRef request, WKStringRef message, const void* clientInfo)
{
#if ENABLE(TIZEN_JSBRIDGE_PLUGIN)
    ewkViewProcessJSBridgePlugin(static_cast<Evas_Object*>(const_cast<void*>(clientInfo)), request, message);
#endif
}

static bool decidePolicyForCertificateError(WKPageRef page, WKStringRef url, WKStringRef certificate, int error, const void* clientInfo)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));

    Ewk_Certificate_Policy_Decision* certificatePolicyDecision = ewkCertificatePolicyDecisionCreate(page, url, certificate, error);
    ewkViewRequestCertificateConfirm(ewkView, certificatePolicyDecision);

    if (!ewkCertificatePolicyDecisionDecided(certificatePolicyDecision) && !ewkCertificatePolicyDecisionSuspended(certificatePolicyDecision))
        ewk_certificate_policy_decision_allowed_set(certificatePolicyDecision, true);

    return true;
#else
    return false;
#endif
}

static void setCertificatePemFile(WKStringRef certificate, const void* clientInfo)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));
    ewkViewSetCertificatePem(ewkView, eina_stringshare_add(toImpl(certificate)->string().utf8().data()));
#endif
}

static bool exceededIndexedDatabaseQuota(WKPageRef page, WKSecurityOriginRef origin, long long currentUsage, WKFrameRef frame, const void *clientInfo)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    return ewkViewExceededIndexedDatabaseQuota(static_cast<Evas_Object*>(const_cast<void*>(clientInfo)), origin, currentUsage);
#else
    return false;
#endif
}

static bool exceededLocalFileSystemQuota(WKPageRef page, WKSecurityOriginRef origin, long long currentUsage, WKFrameRef frame, const void *clientInfo)
{
#if ENABLE(TIZEN_FILE_SYSTEM)
    return ewkViewExceededLocalFileSystemQuota(static_cast<Evas_Object*>(const_cast<void*>(clientInfo)), origin, currentUsage);
#else
    return false;
#endif
}

static bool blockingToLoadForMalwareScan(WKPageRef page, WKErrorRef error, const void* clientInfo)
{
#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));

    Ewk_Content_Screening_Detection* contentScreeningDetection = ewkContentScreeningDetectionCreate(page, error);
    ewkViewRequestBlockedLoadingConfirm(ewkView, contentScreeningDetection);
    TIZEN_LOGE("bool blockingToLoadForMalwareScan()");

    if (!ewkContentScreeningDetectionDecided(contentScreeningDetection) && !ewkContentScreeningDetectionSuspended(contentScreeningDetection)) {
        TIZEN_LOGE("bool blockingToLoadForMalwareScan() > call ewk_content_screening_detection_confirmed_set()");
        ewk_content_screening_detection_confirmed_set(contentScreeningDetection, true);
    }

    return true;
#else
    return false;
#endif
}

void ewkViewTizenClientAttachClient(Evas_Object* ewkView)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkView);

    WKPageTizenClient tizenClient = {
        kWKPageTizenClientCurrentVersion,
        ewkView, // clientInfo
        0,
        decidePolicyForApplicationCachePermissionRequest,
        decidePolicyForUserMediaPermissionRequest,
        processJSBridgePlugin,
        decidePolicyForCertificateError,
        setCertificatePemFile,
        exceededIndexedDatabaseQuota,
        exceededLocalFileSystemQuota,
        blockingToLoadForMalwareScan
    };

    WKPageSetPageTizenClient(toAPI(EwkViewImpl::fromEvasObject(ewkView)->page()), &tizenClient);
}

#endif // #if PLATFORM(TIZEN)
