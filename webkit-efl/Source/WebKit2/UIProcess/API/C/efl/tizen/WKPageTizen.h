/*
 * Copyright (C) 2011 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WKPageTizen_h
#define WKPageTizen_h

#include <Eina.h>
#include <WebKit2/WKBase.h>
#include <WebKit2/WKGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif

// html5 external video callback
typedef void (*WKPageHTML5VideoCallback)(WKPageRef page, WKStringRef url, WKStringRef cookie, const void* clientInfo);
// #if ENABLE(TIZEN_APPLICATION_CACHE)
typedef bool (*WKPageDecidePolicyForApplicationCachePermissionRequestCallback)(WKPageRef page, WKSecurityOriginRef origin, WKFrameRef frame, const void *clientInfo);
// #endif
// #if ENABLE(TIZEN_MEDIA_STREAM)
typedef void (*WKPageDecidePolicyForUserMediaPermissionRequestCallback)(WKPageRef page, WKSecurityOriginRef origin, WKUserMediaPermissionRequestRef permissionRequest, const void *clientInfo);
// #endif
// callback for jsbridge.
typedef void (*WKPageJSBridgePluginCallback)(WKPageRef page, WKStringRef request, WKStringRef message, const void* clientInfo);
// #if ENABLE(TIZEN_CERTIFICATE_HANDLING)
typedef bool (*WKPageDecidePolicyForCertificateErrorCallback)(WKPageRef page, WKStringRef url, WKStringRef certificate, int error, const void *clientInfo);
typedef void (*WKPageSetCertificatePemFile)(WKStringRef certificate, const void* clientInfo);
// #endif
//#if ENABLE(TIZEN_INDEXED_DATABASE)
typedef bool (*WKPageExceededIndexedDatabaseQuotaCallback)(WKPageRef page, WKSecurityOriginRef origin, long long currentUsage, WKFrameRef frame, const void* clientInfo);
//#endif
//#if ENABLE(TIZEN_FILE_SYSTEM)
typedef bool (*WKPageExceededLocalFileSystemQuotaCallback)(WKPageRef page, WKSecurityOriginRef origin, long long currentUsage, WKFrameRef frame, const void* clientInfo);
//#endif
// #if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
typedef bool (*WKPageBlockingToLoadForMalwareScanCallback)(WKPageRef page, WKErrorRef error, const void *clientInfo);
// #endif

struct WKPageTizenClient {
    int                                                                 version;
    const void *                                                        clientInfo;

    // Version 0
    // html5 video external player
    WKPageHTML5VideoCallback                                            processHTML5Video;
    // #if ENABLE(TIZEN_APPLICATION_CACHE)
    WKPageDecidePolicyForApplicationCachePermissionRequestCallback      decidePolicyForApplicationCachePermissionRequest;
    // #endif
    // userMedia permission
    // #if ENABLE(TIZEN_MEDIA_STREAM)
    WKPageDecidePolicyForUserMediaPermissionRequestCallback             decidePolicyForUserMediaPermissionRequest;
    // #endif
    // jsbridge support the special call which can be reached from JS to Browser App.
    WKPageJSBridgePluginCallback                                        processJSBridgePlugin;
    // #if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    WKPageDecidePolicyForCertificateErrorCallback               decidePolicyForCertificateError;
    WKPageSetCertificatePemFile               setCertificatePemFile;
    // #endif
    //#if ENABLE(TIZEN_INDEXED_DATABASE)
    WKPageExceededIndexedDatabaseQuotaCallback                                  exceededIndexedDatabaseQuota;
    //#endif
    //#if ENABLE(TIZEN_FILE_SYSTEM)
    WKPageExceededLocalFileSystemQuotaCallback                                  exceededLocalFileSystemQuota;
    //#endif
    // #if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    WKPageBlockingToLoadForMalwareScanCallback               blockingToLoadForMalwareScan;
    // #endif
};
typedef struct WKPageTizenClient WKPageTizenClient;

enum { kWKPageTizenClientCurrentVersion = 0 };

WK_EXPORT void WKPageSetPageTizenClient(WKPageRef page, const WKPageTizenClient* client);

/* Tizen Experimental feature */
WK_EXPORT WKImageRef WKPageCreateSnapshot(WKPageRef page, WKRect viewArea, float scaleFactor);
#if ENABLE(TIZEN_CACHE_IMAGE_GET)
WK_EXPORT WKImageRef WKPageCreateCacheImage(WKPageRef page, WKStringRef imageUrl);
#endif
WK_EXPORT void WKPageGetSnapshotPdfFile(WKPageRef page, WKSize surfaceSize, WKSize contentsSize, WKStringRef fileName);

/* WK2 Recording Surface features */
WK_EXPORT void WKPageRecordingSurfaceSetEnable(WKPageRef page, bool enable);

/* WK2 ContextMenu Additional API */
WK_EXPORT WKStringRef WKPageContextMenuCopyAbsoluteLinkURLString(WKPageRef page);
WK_EXPORT WKStringRef WKPageContextMenuCopyAbsoluteImageURLString(WKPageRef page);

WK_EXPORT bool WKPageScrollBy(WKPageRef page, WKSize scrollOffset);

/* WK2 WebStorage APIs and callback */
// #if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
typedef void (*WKPageGetWebStorageQuotaFunction)(WKUInt32Ref quota, WKErrorRef, void*);
WK_EXPORT void WKPageGetWebStorageQuota(WKPageRef page, void* context, WKPageGetWebStorageQuotaFunction function);
// #endif
WK_EXPORT void WKPageSetWebStorageQuota(WKPageRef page, uint32_t quota);

/* WK2 Meta Tag APIs and callback */
// #if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
typedef void (*WKPageGetWebAppCapableFunction)(WKBooleanRef capable, WKErrorRef, void*);
WK_EXPORT void WKPageGetWebAppCapable(WKPageRef page, void* context, WKPageGetWebAppCapableFunction function);
typedef void (*WKPageGetWebAppIconURLFunction)(WKStringRef iconURL, WKErrorRef, void*);
WK_EXPORT void WKPageGetWebAppIconURL(WKPageRef page, void* context, WKPageGetWebAppIconURLFunction function);
typedef void (*WKPageGetWebAppIconURLsFunction)(WKDictionaryRef iconURLs, WKErrorRef, void*);
WK_EXPORT void WKPageGetWebAppIconURLs(WKPageRef page, void* context, WKPageGetWebAppIconURLsFunction function);
// #endif

WK_EXPORT void WKPageExecuteCommandWithArgument(WKPageRef page, WKStringRef command, WKStringRef argument);

// #if ENABLE(TIZEN_APPLICATION_CACHE)
WK_EXPORT void WKPageReplyApplicationCachePermission(WKPageRef page, bool allow);
// #endif

//#if ENABLE(TIZEN_INDEXED_DATABASE)
WK_EXPORT void WKPageReplyExceededIndexedDatabaseQuota(WKPageRef page, bool allow);
//#endif

// #if ENABLE(TIZEN_SQL_DATABASE)
WK_EXPORT void WKPageReplyExceededDatabaseQuota(WKPageRef page, bool allow);
// #endif

//#if ENABLE(TIZEN_FILE_SYSTEM)
WK_EXPORT void WKPageReplyExceededLocalFileSystemQuota(WKPageRef page, bool allow);
//#endif
#ifdef __cplusplus
}
#endif

#endif /* WKPageTizen_h */
