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

#include "config.h"
#include "WebLocalFileSystemManagerProxy.h"

#if ENABLE(TIZEN_FILE_SYSTEM)
#include "ImmutableArray.h"
#include "WebContext.h"
#include "WebLocalFileSystemManagerMessages.h"
#include "WebSecurityOrigin.h"

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebLocalFileSystemManagerProxy> WebLocalFileSystemManagerProxy::create(WebContext* webContext)
{
    return adoptRef(new WebLocalFileSystemManagerProxy(webContext));
}

WebLocalFileSystemManagerProxy::WebLocalFileSystemManagerProxy(WebContext* webContext)
    : m_webContext(webContext)
{
}

WebLocalFileSystemManagerProxy::~WebLocalFileSystemManagerProxy()
{
}

void WebLocalFileSystemManagerProxy::invalidate()
{
    invalidateCallbackMap(m_arrayCallbacks);
}

bool WebLocalFileSystemManagerProxy::shouldTerminate(WebProcessProxy*) const
{
    return m_arrayCallbacks.isEmpty();
}

void WebLocalFileSystemManagerProxy::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceiveWebLocalFileSystemManagerProxyMessage(connection, messageID, arguments);
}

void WebLocalFileSystemManagerProxy::deleteAllLocalFileSystem()
{
    m_webContext->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebLocalFileSystemManager::DeleteAllLocalFileSystem());
}

void WebLocalFileSystemManagerProxy::deleteLocalFileSystemForOrigin(WebSecurityOrigin* origin)
{
    m_webContext->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebLocalFileSystemManager::DeleteLocalFileSystemForOrigin(origin->databaseIdentifier()));
}

void WebLocalFileSystemManagerProxy::getLocalFileSystemOrigins(PassRefPtr<ArrayCallback> prpCallback)
{
    RefPtr<ArrayCallback> callback = prpCallback;
    uint64_t callbackID = callback->callbackID();
    m_arrayCallbacks.set(callbackID, callback.release());

    m_webContext->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebLocalFileSystemManager::GetLocalFileSystemOrigins(callbackID));
}

void WebLocalFileSystemManagerProxy::didGetLocalFileSystemOrigins(const Vector<String>& originIdentifiers, uint64_t callbackID)
{
    RefPtr<ArrayCallback> callback = m_arrayCallbacks.take(callbackID);
    if (!callback)
        return;

    size_t originIdentifiersCount = originIdentifiers.size();
    Vector<RefPtr<APIObject> > securityOrigins(originIdentifiersCount);

    for (size_t i = 0; i < originIdentifiersCount; ++i)
        securityOrigins[i] = WebSecurityOrigin::createFromDatabaseIdentifier(originIdentifiers[i]);

    callback->performCallbackWithReturnValue(ImmutableArray::adopt(securityOrigins).get());
}

} // namespace WebKit
#endif // ENABLE(TIZEN_FILE_SYSTEM)
