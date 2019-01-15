/*
 * Copyright (C) 2014 Samsung Electronics. All rights reserved.
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
#include "WebIndexedDatabaseManager.h"

#if ENABLE(TIZEN_INDEXED_DATABASE)
#include "MessageID.h"
#include "SecurityOriginData.h"
#include "WebIndexedDatabaseManagerProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityOriginHash.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

WebIndexedDatabaseManager& WebIndexedDatabaseManager::shared()
{
    static WebIndexedDatabaseManager& shared = *new WebIndexedDatabaseManager;
    return shared;
}

WebIndexedDatabaseManager::WebIndexedDatabaseManager()
{
}

void WebIndexedDatabaseManager::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceiveWebIndexedDatabaseManagerMessage(connection, messageID, arguments);
}

void WebIndexedDatabaseManager::deleteAllIndexedDatabase() const
{
    Vector<String> paths = listDirectory(WebProcess::shared().indexedDatabaseDirectory(), ("*"));
    size_t pathCount = paths.size();
    for (size_t i = 0; i < pathCount; ++i)
        removeDirectory(paths[i]);
}

void WebIndexedDatabaseManager::deleteIndexedDatabaseForOrigin(const String& originIdentifier) const
{
    DEFINE_STATIC_LOCAL(const String, originMatchPattern, (("*/") + originIdentifier + ".indexeddb.leveldb"));
    String basePath = WebProcess::shared().indexedDatabaseDirectory();
    Vector<String> paths = listDirectory(basePath, originMatchPattern);
    size_t pathCount = paths.size();
    for (size_t i = 0; i < pathCount; ++i)
        removeDirectory(paths[i]);
}

void WebIndexedDatabaseManager::getIndexedDatabaseOrigins(uint64_t callbackID) const
{
    DEFINE_STATIC_LOCAL(const String, fileMatchPattern, ("*"));
    String basePath = WebProcess::shared().indexedDatabaseDirectory();
    Vector<String> originIdentifiers;
    Vector<String> originPaths = listDirectory(basePath, fileMatchPattern);
    size_t pathCount = originPaths.size();
    for (size_t i= 0; i < pathCount; ++i) {
        String originIdentifier = pathGetFileName(originPaths[i]);
        if (originIdentifiers.find(originIdentifier))
            originIdentifiers.append(originIdentifier);
    }

    WebProcess::shared().connection()->send(Messages::WebIndexedDatabaseManagerProxy::DidGetIndexedDatabaseOrigins(originIdentifiers, callbackID), 0);
}

} // namespace WebKit
#endif
