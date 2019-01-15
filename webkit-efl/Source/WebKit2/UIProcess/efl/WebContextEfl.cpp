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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS AS IS''
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
#include "WebContext.h"

#include "FileSystem.h"
#include "MessageID.h"
#include "WebProcessMessages.h"
#include "WebProcessCreationParameters.h"
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/NotImplemented.h>

namespace WebKit {

String WebContext::applicationCacheDirectory()
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    String applicationCacheDirectory = WebCore::cacheStorage().cacheDirectory();
    if(applicationCacheDirectory.isEmpty()) {
        applicationCacheDirectory = platformDefaultApplicationCacheDirectory();
    }
    return applicationCacheDirectory;
#else
    return WebCore::cacheStorage().cacheDirectory();
#endif
}

#if ENABLE(TIZEN_APPLICATION_CACHE)
void WebContext::setApplicationCacheDirectory(const String& directory)
{
    WebCore::cacheStorage().setCacheDirectory(directory);
}
#endif

void WebContext::platformInitializeWebProcess(WebProcessCreationParameters& parameters)
{
#if ENABLE(TIZEN_WEBKIT2_PROXY)
    parameters.proxyAddress = m_proxyAddress;
#endif

#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
    parameters.soupDataDirectory = soupDataDirectory();
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    parameters.certificateFile = m_certificateFile;
#endif

    notImplemented();
}

void WebContext::platformInvalidateContext()
{
    notImplemented();
}

#if ENABLE(TIZEN_APPLICATION_CACHE)
String WebContext::platformDefaultApplicationCacheDirectory() const
{
    return WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/appcache");
}
#endif

String WebContext::platformDefaultDatabaseDirectory() const
{
    return WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/databases");
}

String WebContext::platformDefaultIconDatabasePath() const
{
#if ENABLE(TIZEN_ICON_DATABASE)
    return WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/iconDatabase/WebpageIcons.db");
#else
    return String::fromUTF8(efreet_data_home_get()) + "/WebKitEfl/IconDatabase";
#endif
}

#if ENABLE(TIZEN_FILE_SYSTEM)
String WebContext::platformDefaultLocalFileSystemDirectory() const
{
    return WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/localFileSystem");
}
#endif

String WebContext::platformDefaultLocalStorageDirectory() const
{
    return WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/localStorage");
}

#if ENABLE(TIZEN_INDEXED_DATABASE)
String WebContext::platformDefaultIndexedDatabaseDirectory() const
{
    return WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/indexedDatabases");
}
#endif

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
String WebContext::platformDefaultResourceCacheDirectory() const
{
    return WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/resourceCache/");
}
#endif

#if ENABLE(TIZEN_WEBKIT2_PROXY)
void WebContext::setProxy(const String& proxyAddress)
{
    m_proxyAddress = proxyAddress;
    sendToAllProcesses(Messages::WebProcess::SetProxy(proxyAddress));
}
#endif

#if ENABLE(TIZEN_SESSION_REQUEST_CANCEL)
void WebContext::abortSession()
{
    process()->send(Messages::WebProcess::AbortSession(), 0);
}
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
void WebContext::setCertificateFile(const String& certificateFile)
{
    m_certificateFile = certificateFile;
    if(process())
        process()->send(Messages::WebProcess::SetCertificateFile(certificateFile), 0);
}
#endif

void WebContext::notifyLowMemory()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (!m_process->platformSurfaceTexturePool())
        return;
    m_process->removeUnusedPlatformSurfaceTexturesFromPool();
#else
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    m_process->releaseBackingStoreMemory();
#endif
#endif
}

#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
String WebContext::soupDataDirectory()
{
    if (!m_soupDataDirectory.isEmpty()) {
        TIZEN_SECURE_LOGI("[Loader] m_soupDataDirectory = %s", m_soupDataDirectory.utf8().data());
        return m_soupDataDirectory;
    }

    return platformDefaultSoupDataDirectory();
}
String WebContext::platformDefaultSoupDataDirectory()
{
    return WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/soupData");
}
#endif

#if ENABLE(TIZEN_SOUP_CACHE_DIRECTORY_SET)
void WebContext::setSoupDataDirectory(const String& path)
{
    TIZEN_SECURE_LOGI("setSoupDataDirectory Path = %s\n", path.utf8().data());
    m_soupDataDirectory = path;
    sendToAllProcesses(Messages::WebProcess::SetSoupDataDirectory(path));
}
#endif

#if ENABLE(TIZEN_WEBKIT2_MEMORY_SAVING_MODE)
void WebContext::setMemorySavingMode(bool memorySavingMode)
{
    //UI Process
    m_process->setMemorySavingMode(memorySavingMode);
    //Web Process
    sendToAllProcesses(Messages::WebProcess::SetMemorySavingMode(memorySavingMode));
}
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
void WebContext::setXWindow(unsigned xWindow)
{
    sendToAllProcesses(Messages::WebProcess::SetXWindow(xWindow));
}
#endif

#if ENABLE(TIZEN_EXTENSIBLE_API)
void WebContext::setTizenExtensibleAPI(WebCore::ExtensibleAPI extensibleAPI, bool enable)
{
    m_extensibleAPIMap.set(static_cast<uint32_t>(extensibleAPI), enable);
    sendToAllProcesses(Messages::WebProcess::SetTizenExtensibleAPI(static_cast<uint32_t>(extensibleAPI), enable));
}

bool WebContext::getTizenExtensibleAPI(WebCore::ExtensibleAPI extensibleAPI)
{
    return m_extensibleAPIMap.get(static_cast<uint32_t>(extensibleAPI));
}
#endif

#if ENABLE(TIZEN_RESET_PATH)
void WebContext::resetStoragePath()
{
    sendToAllProcesses(Messages::WebProcess::ResetStoragePath());
}
#endif

#if ENABLE(TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR)
unsigned int WebContext::inspectorServerStart(uint32_t port)
{
    uint32_t assignedPort = 0;
    process()->sendSync(Messages::WebProcess::InspectorServerStart(port), Messages::WebProcess::InspectorServerStart::Reply(assignedPort), 0);
    return assignedPort;
}

bool WebContext::inspectorServerStop()
{
    bool result = false;
    process()->sendSync(Messages::WebProcess::InspectorServerStop(), Messages::WebProcess::InspectorServerStop::Reply(result), 0);
    return result;
}
#endif
} // namespace WebKit
