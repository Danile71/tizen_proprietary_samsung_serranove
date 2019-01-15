/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef APIObject_h
#define APIObject_h

#include <wtf/RefCounted.h>

namespace WebKit {

class APIObject : public ThreadSafeRefCounted<APIObject> {
public:
    enum Type {
        // Base types
        TypeNull = 0,
        TypeArray,
        TypeAuthenticationChallenge,
        TypeAuthenticationDecisionListener,
        TypeCertificateInfo,
        TypeConnection,
        TypeContextMenuItem,
        TypeCredential,
        TypeData,
        TypeDictionary,
        TypeError,
        TypeGraphicsContext,
        TypeImage,
        TypeProtectionSpace,
        TypeRenderLayer,
        TypeRenderObject,
        TypeSecurityOrigin,
        TypeSerializedScriptValue,
        TypeString,
        TypeURL,
        TypeURLRequest,
        TypeURLResponse,
        TypeUserContentURLPattern,

        // Base numeric types
        TypeBoolean,
        TypeDouble,
        TypeUInt64,
#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
        TypeUInt32,
        TypeInt64,
#endif

        // Geometry types
        TypePoint,
        TypeSize,
        TypeRect,
        
        // UIProcess types
        TypeApplicationCacheManager,
        TypeBackForwardList,
        TypeBackForwardListItem,
        TypeBatteryManager,
        TypeBatteryStatus,
        TypeCacheManager,
#if ENABLE(TIZEN_INPUT_COLOR_PICKER) // wait for upstream
        TypeColorPickerResultListener,
#endif
        TypeContext,
        TypeCookieManager,
        TypeDatabaseManager,
        TypeDownload,
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
        TypeFormData,
#endif
        TypeFormSubmissionListener,
        TypeFrame,
        TypeFramePolicyListener,
        TypeFullScreenManager,
        TypeGeolocationManager,
        TypeGeolocationPermissionRequest,
        TypeHitTestResult,
        TypeGeolocationPosition,
        TypeGrammarDetail,
        TypeIconDatabase,
#if ENABLE(TIZEN_INDEXED_DATABASE)
        TypeIndexedDatabaseManager,
#endif
        TypeInspector,
        TypeIntentData,
        TypeIntentServiceInfo,
        TypeKeyValueStorageManager,
#if ENABLE(TIZEN_FILE_SYSTEM)
        TypeLocalFileSystemManager,
#endif
        TypeMediaCacheManager,
        TypeNavigationData,
        TypeNetworkInfo,
        TypeNetworkInfoManager,
        TypeNotification,
        TypeNotificationManager,
        TypeNotificationPermissionRequest,
        TypeOpenPanelParameters,
        TypeOpenPanelResultListener,
        TypePage,
        TypePageGroup,
        TypePluginSiteDataManager,
        TypePreferences,
        TypeTextChecker,
#if ENABLE(TIZEN_MEDIA_STREAM)
        TypeUserMediaPermissionRequest,
#endif
        TypeVibration,

        // Bundle types
        TypeBundle,
        TypeBundleBackForwardList,
        TypeBundleBackForwardListItem,
        TypeBundleDOMWindowExtension,
        TypeBundleFrame,
        TypeBundleHitTestResult,
        TypeBundleInspector,
        TypeBundleIntentRequest,
        TypeBundleNavigationAction,
        TypeBundleNodeHandle,
        TypeBundlePage,
        TypeBundlePageGroup,
        TypeBundlePageOverlay,
        TypeBundleRangeHandle,
        TypeBundleScriptWorld,

        // Platform specific
        TypeEditCommandProxy,
        TypeView,
#if USE(SOUP)
        TypeSoupRequestManager,
#endif
    };

    virtual ~APIObject()
    {
    }

    virtual Type type() const = 0;

protected:
    APIObject();
};

} // namespace WebKit

#endif // APIObject_h
