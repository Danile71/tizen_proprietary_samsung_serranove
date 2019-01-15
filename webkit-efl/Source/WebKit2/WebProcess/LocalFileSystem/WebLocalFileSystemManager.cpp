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
#include "WebLocalFileSystemManager.h"

#if ENABLE(TIZEN_FILE_SYSTEM)
#include "MessageID.h"
#include "SecurityOriginData.h"
#include "WebLocalFileSystemManagerProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/FileSystem.h>
#include <WebCore/LocalFileSystem.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityOriginHash.h>
#include <WebCore/StorageTracker.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

WebLocalFileSystemManager& WebLocalFileSystemManager::shared()
{
    static WebLocalFileSystemManager& shared = *new WebLocalFileSystemManager;
    return shared;
}

WebLocalFileSystemManager::WebLocalFileSystemManager()
{
}

void WebLocalFileSystemManager::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceiveWebLocalFileSystemManagerMessage(connection, messageID, arguments);
}

void WebLocalFileSystemManager::deleteAllLocalFileSystem() const
{
    DEFINE_STATIC_LOCAL(const String, fileMatchPattern, ("*"));
    String basePath = WebCore::LocalFileSystem::localFileSystem().fileSystemBasePath();
    Vector<String> paths = listDirectory(basePath, fileMatchPattern);
    Vector<String>::const_iterator end = paths.end();
    for (Vector<String>::const_iterator it = paths.begin(); it != end; ++it)
        removeDirectory(*it);
}

void WebLocalFileSystemManager::deleteLocalFileSystemForOrigin(const String& originIdentifier) const
{
    DEFINE_STATIC_LOCAL(const String, originMatchPattern, (("*/") + originIdentifier));
    String basePath = WebCore::LocalFileSystem::localFileSystem().fileSystemBasePath();
    Vector<String> paths = listDirectory(basePath, originMatchPattern);
    Vector<String>::const_iterator end = paths.end();
    for (Vector<String>::const_iterator it = paths.begin(); it != end; ++it)
        removeDirectory(*it);
}

void WebLocalFileSystemManager::getLocalFileSystemOrigins(uint64_t callbackID) const
{
    DEFINE_STATIC_LOCAL(const String, fileMatchPattern, ("*"));
    String basePath = WebCore::LocalFileSystem::localFileSystem().fileSystemBasePath();
    Vector<String> originIdentifiers;
    Vector<String> originPaths = listDirectory(basePath, fileMatchPattern);
    Vector<String>::const_iterator originEnd = originPaths.end();
    for (Vector<String>::const_iterator originIter = originPaths.begin(); originIter != originEnd; ++originIter) {
        String originIdentifier = pathGetFileName(*originIter);
        if (originIdentifiers.find(originIdentifier))
            originIdentifiers.append(originIdentifier);
    }

    WebProcess::shared().connection()->send(Messages::WebLocalFileSystemManagerProxy::DidGetLocalFileSystemOrigins(originIdentifiers, callbackID), 0);
}

} // namespace WebKit
#endif
