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

#ifndef WebTizenClient_h
#define WebTizenClient_h

#include "APIClient.h"
#include "WKPageTizen.h"
#include <wtf/Forward.h>

namespace WebCore {
class ResourceError;
}

namespace WebKit {

class WebPageProxy;
class WebFrameProxy;
#if ENABLE(TIZEN_FILE_SYSTEM)
class WebSecurityOrigin;
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
class UserMediaPermissionRequest;
#endif

class WebTizenClient : public APIClient<WKPageTizenClient, kWKPageTizenClientCurrentVersion> {
public:
#if ENABLE(TIZEN_MEDIA_STREAM)
    bool decidePolicyForUserMediaPermissionRequest(WebPageProxy*, WebSecurityOrigin*, UserMediaPermissionRequest*);
#endif

#if ENABLE(TIZEN_JSBRIDGE_PLUGIN)
    void processJSBridgePlugin(WebPageProxy*, const String& request, const String& message);
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    bool decidePolicyForCertificateError(WebPageProxy*, const String& url, const String& certificate, int error);
    void setCertificatePemFile(const String& certificate);
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    bool blockingToLoadForMalwareScan(WebPageProxy*, const WebCore::ResourceError& error);
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
    bool allowFileSystem(WebPageProxy*, WebFrameProxy*, WebSecurityOrigin*);
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
    bool decidePolicyForApplicationCachePermissionRequest(WebPageProxy*, WebSecurityOrigin*, WebFrameProxy*);
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
    bool exceededIndexedDatabaseQuota(WebPageProxy*, WebSecurityOrigin*, long long, WebFrameProxy*);
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
    bool exceededLocalFileSystemQuota(WebPageProxy*, WebSecurityOrigin*, long long, WebFrameProxy*);
#endif
};

} // namespace WebKit

#endif // WebTizenClient_h
