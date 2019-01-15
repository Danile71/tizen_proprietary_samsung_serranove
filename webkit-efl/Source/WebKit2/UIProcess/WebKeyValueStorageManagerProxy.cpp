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

#include "config.h"
#include "WebKeyValueStorageManagerProxy.h"

#include "SecurityOriginData.h"
#include "WebKeyValueStorageManagerMessages.h"
#include "WebContext.h"
#include "WebSecurityOrigin.h"

namespace WebKit {

PassRefPtr<WebKeyValueStorageManagerProxy> WebKeyValueStorageManagerProxy::create(WebContext* context)
{
    return adoptRef(new WebKeyValueStorageManagerProxy(context));
}

WebKeyValueStorageManagerProxy::WebKeyValueStorageManagerProxy(WebContext* context)
    : m_webContext(context)
{
}

WebKeyValueStorageManagerProxy::~WebKeyValueStorageManagerProxy()
{
}

void WebKeyValueStorageManagerProxy::invalidate()
{
    invalidateCallbackMap(m_arrayCallbacks);
}

bool WebKeyValueStorageManagerProxy::shouldTerminate(WebProcessProxy*) const
{
    return m_arrayCallbacks.isEmpty();
}

void WebKeyValueStorageManagerProxy::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceiveWebKeyValueStorageManagerProxyMessage(connection, messageID, arguments);
}

void WebKeyValueStorageManagerProxy::getKeyValueStorageOrigins(PassRefPtr<ArrayCallback> prpCallback)
{
    RefPtr<ArrayCallback> callback = prpCallback;
    uint64_t callbackID = callback->callbackID();
    m_arrayCallbacks.set(callbackID, callback.release());

    // FIXME (Multi-WebProcess): Should key-value storage be handled in the web process?
    m_webContext->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebKeyValueStorageManager::GetKeyValueStorageOrigins(callbackID));
}
    
void WebKeyValueStorageManagerProxy::didGetKeyValueStorageOrigins(const Vector<SecurityOriginData>& originDatas, uint64_t callbackID)
{
    RefPtr<ArrayCallback> callback = m_arrayCallbacks.take(callbackID);
    performAPICallbackWithSecurityOriginDataVector(originDatas, callback.get());
}

void WebKeyValueStorageManagerProxy::deleteEntriesForOrigin(WebSecurityOrigin* origin)
{
    SecurityOriginData securityOriginData;
    securityOriginData.protocol = origin->protocol();
    securityOriginData.host = origin->host();
    securityOriginData.port = origin->port();

    // FIXME (Multi-WebProcess): Should key-value storage be handled in the web process?
    m_webContext->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebKeyValueStorageManager::DeleteEntriesForOrigin(securityOriginData));
}

void WebKeyValueStorageManagerProxy::deleteAllEntries()
{
    // FIXME (Multi-WebProcess): Should key-value storage be handled in the web process?
    m_webContext->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebKeyValueStorageManager::DeleteAllEntries());
}

#if ENABLE(TIZEN_WEB_STORAGE)
void WebKeyValueStorageManagerProxy::getKeyValueStoragePath(PassRefPtr<WebStorageStringCallback> prpCallback)
{
    RefPtr<WebStorageStringCallback> callback = prpCallback;
    uint64_t callbackID = callback->callbackID();
    m_webStorageStringCallbacks.set(callbackID, callback.release());

    // FIXME (Multi-WebProcess): Should key-value storage be handled in the web process?
    m_webContext->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebKeyValueStorageManager::GetKeyValueStoragePath(callbackID));
}

void WebKeyValueStorageManagerProxy::didGetKeyValueStoragePath(const String& path, uint64_t callbackID)
{
    RefPtr<WebStorageStringCallback> callback = m_webStorageStringCallbacks.take(callbackID);
    if (!callback)
        return;

    m_webStorageStringCallbacks.remove(callbackID);
    callback->performCallbackWithReturnValue(path.impl());
}

#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
void WebKeyValueStorageManagerProxy::getKeyValueStorageUsageForOrigin(PassRefPtr<WebStorageInt64Callback> prpCallback, WebSecurityOrigin* origin)
{
    RefPtr<WebStorageInt64Callback> callback = prpCallback;
    uint64_t callbackID = callback->callbackID();
    m_webStorageInt64Callbacks.set(callbackID, callback.release());

    SecurityOriginData securityOriginData;
    securityOriginData.protocol = origin->protocol();
    securityOriginData.host = origin->host();
    securityOriginData.port = origin->port();

    // FIXME (Multi-WebProcess): Should key-value storage be handled in the web process?
    m_webContext->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebKeyValueStorageManager::GetKeyValueStorageUsageForOrigin(callbackID, securityOriginData));
}

void WebKeyValueStorageManagerProxy::didGetKeyValueStorageUsageForOrigin(int64_t usage, uint64_t callbackID)
{
    RefPtr<WebStorageInt64Callback> callback = m_webStorageInt64Callbacks.take(callbackID);
    if (!callback)
        return;

    RefPtr<WebInt64> int64Object = WebInt64::create(usage);
    callback->performCallbackWithReturnValue(int64Object.release().leakRef());
}
#endif

void WebKeyValueStorageManagerProxy::syncLocalStorage()
{
    if (m_webContext && m_webContext->process())
        m_webContext->process()->sendSync(Messages::WebKeyValueStorageManager::SyncLocalStorage(), Messages::WebKeyValueStorageManager::SyncLocalStorage::Reply(), 0);
}
#endif
} // namespace WebKit
