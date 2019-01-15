/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebLocalFileSystemManagerProxy_h
#define WebLocalFileSystemManagerProxy_h

#if ENABLE(TIZEN_FILE_SYSTEM)
#include "APIObject.h"
#include "GenericCallback.h"
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
class MessageID;
class WebSecurityOrigin;
}

namespace WebKit {

class WebContext;
class WebProcessProxy;
class WebSecurityOrigin;

typedef GenericCallback<WKArrayRef> ArrayCallback;

class WebLocalFileSystemManagerProxy : public APIObject {
public:
    static const Type APIType = TypeLocalFileSystemManager;

    static PassRefPtr<WebLocalFileSystemManagerProxy> create(WebContext*);
    virtual ~WebLocalFileSystemManagerProxy();

    void invalidate();
    void clearContext() { m_webContext = 0; }

    void deleteAllLocalFileSystem();
    void deleteLocalFileSystemForOrigin(WebSecurityOrigin*);
    void getLocalFileSystemOrigins(PassRefPtr<ArrayCallback>);

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    bool shouldTerminate(WebProcessProxy*) const;

private:
    explicit WebLocalFileSystemManagerProxy(WebContext*);

    virtual Type type() const { return APIType; }

    void didGetLocalFileSystemOrigins(const Vector<String>& originIdentifiers, uint64_t callbackID);

    void didReceiveWebLocalFileSystemManagerProxyMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    WebContext* m_webContext;
    HashMap<uint64_t, RefPtr<ArrayCallback> > m_arrayCallbacks;
};

} // namespace WebKit
#endif
#endif // WebLocalFileSystemManagerProxy_h
