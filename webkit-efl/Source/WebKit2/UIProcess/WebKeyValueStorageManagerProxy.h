/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WebKeyValueStorageManagerProxy_h
#define WebKeyValueStorageManagerProxy_h

#include "APIObject.h"
#include "GenericCallback.h"
#include "ImmutableArray.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace CoreIPC {
    class ArgumentDecoder;
    class Connection;
    class MessageID;
}

namespace WebKit {

struct SecurityOriginData;
class WebContext;
class WebProcessProxy;
class WebSecurityOrigin;

typedef GenericCallback<WKArrayRef> ArrayCallback;
#if ENABLE(TIZEN_WEB_STORAGE)
typedef GenericCallback<WKStringRef, StringImpl*> WebStorageStringCallback;
#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
typedef GenericCallback<WKInt64Ref> WebStorageInt64Callback;
#endif
#endif

class WebKeyValueStorageManagerProxy : public APIObject {
public:
    static const Type APIType = TypeKeyValueStorageManager;

    static PassRefPtr<WebKeyValueStorageManagerProxy> create(WebContext*);
    virtual ~WebKeyValueStorageManagerProxy();

    void invalidate();
    void clearContext() { m_webContext = 0; }
    
    void getKeyValueStorageOrigins(PassRefPtr<ArrayCallback>);
    void deleteEntriesForOrigin(WebSecurityOrigin*);
    void deleteAllEntries();

#if ENABLE(TIZEN_WEB_STORAGE)
    void getKeyValueStoragePath(PassRefPtr<WebStorageStringCallback>);
    void syncLocalStorage();
#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
    void getKeyValueStorageUsageForOrigin(PassRefPtr<WebStorageInt64Callback>, WebSecurityOrigin*);
#endif
#endif

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    bool shouldTerminate(WebProcessProxy*) const;

private:
    WebKeyValueStorageManagerProxy(WebContext*);

    virtual Type type() const { return APIType; }

    void didGetKeyValueStorageOrigins(const Vector<SecurityOriginData>&, uint64_t callbackID);
#if ENABLE(TIZEN_WEB_STORAGE)
    void didGetKeyValueStoragePath(const String& path, uint64_t callbackID);
#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
    void didGetKeyValueStorageUsageForOrigin(int64_t usage, uint64_t callbackID);
#endif
#endif
    
    void didReceiveWebKeyValueStorageManagerProxyMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    WebContext* m_webContext;
    HashMap<uint64_t, RefPtr<ArrayCallback> > m_arrayCallbacks;
#if ENABLE(TIZEN_WEB_STORAGE)
    HashMap<uint64_t, RefPtr<WebStorageStringCallback> > m_webStorageStringCallbacks;
#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
    HashMap<uint64_t, RefPtr<WebStorageInt64Callback> > m_webStorageInt64Callbacks;
#endif
#endif
};

} // namespace WebKit

#endif // WebKeyValueStorageManagerProxy_h
