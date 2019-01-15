/*
 * Copyright (C) 2011 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebProcess.h"

#include "WebProcessCreationParameters.h"
#include <WebCore/NotImplemented.h>

#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
#include <WebCore/MemoryCache.h>
#include <WebCore/PageCache.h>
#include <WebCore/ResourceHandle.h>
#if USE(SOUP)
#include <WebCore/CookieJarSoup.h>
#include <WebCore/FileSystem.h>
#include <glib-object.h>
#include <libsoup/soup.h>
#if ENABLE(TIZEN_SOUP_CACHE)
#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup-cache.h>
#if ENABLE (TIZEN_SOUP_CACHE_DIRECTORY_PATH_SET)
#include "app_common.h"
#endif
#endif
#if ENABLE(TIZEN_PERSISTENT_COOKIES_SQL)
#include <libsoup/soup-cookie-jar-sqlite.h>
#endif
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "WebPage/efl/tizen/PlatformSurfacePoolTizen.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
#include <WebCore/EflScreenUtilities.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR)
#include "WebInspectorServerEfl.h"
#endif

#if ENABLE(TIZEN_EXTENSIBLE_API)
#include <WebCore/TizenExtensibleAPI.h>
#endif

#if ENABLE(TIZEN_RESET_PATH)
#include "WebDatabaseManager.h"
#include "WebKeyValueStorageManager.h"
#include "WebPage.h"
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/DatabaseTracker.h>
#include <WebCore/LocalFileSystem.h>
#include <WebCore/StorageTracker.h>
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
#include <WebCore/BufferDatabaseTizen.h>
#endif
#endif

namespace WebKit {
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
class PlatformSurfacePoolTizen;
#endif

#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
#if USE(SOUP)
static uint64_t getCacheDiskFreeSize(SoupCache* cache)
{
    if (!cache)
        return 0;

    GOwnPtr<char> cacheDir;
    g_object_get(G_OBJECT(cache), "cache-dir", &cacheDir.outPtr(), NULL);
    if (!cacheDir)
        return 0;

    return WebCore::getVolumeFreeSizeForPath(cacheDir.get());
}
#endif

static uint64_t getMemorySize()
{
    static uint64_t kDefaultMemorySize = 512;
#if !OS(WINDOWS)
    long pageSize = sysconf(_SC_PAGESIZE);
    if (pageSize == -1)
        return kDefaultMemorySize;

    long physPages = sysconf(_SC_PHYS_PAGES);
    if (physPages == -1)
        return kDefaultMemorySize;

    return ((pageSize / 1024) * physPages) / 1024;
#else
    // Fallback to default for other platforms.
    return kDefaultMemorySize;
#endif
}

void WebProcess::platformSetCacheModel(CacheModel cacheModel)
{
    unsigned cacheTotalCapacity = 0;
    unsigned cacheMinDeadCapacity = 0;
    unsigned cacheMaxDeadCapacity = 0;
    double deadDecodedDataDeletionInterval = 0;
    unsigned pageCacheCapacity = 0;
    uint64_t diskFreeSize = 0;

    unsigned long urlCacheMemoryCapacity = 0;
    unsigned long urlCacheDiskCapacity = 0;

#if USE(SOUP)
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    if (cache)
        diskFreeSize = getCacheDiskFreeSize(cache);
#endif

    uint64_t memSize = getMemorySize();
    calculateCacheSizes(cacheModel, memSize, diskFreeSize,
                        cacheTotalCapacity, cacheMinDeadCapacity, cacheMaxDeadCapacity, deadDecodedDataDeletionInterval,
                        pageCacheCapacity, urlCacheMemoryCapacity, urlCacheDiskCapacity);

#if ENABLE(TIZEN_JPEGIMAGE_DECODING_THREAD)
    cacheTotalCapacity = WebCore::memoryCache()->getMaxAsyncDecodeTotalBytes();
#endif

    WebCore::memoryCache()->setCapacities(cacheMinDeadCapacity, cacheMaxDeadCapacity, cacheTotalCapacity);
    WebCore::memoryCache()->setDeadDecodedDataDeletionInterval(deadDecodedDataDeletionInterval);
#if ENABLE(TIZEN_LITE_PAGECACHE_OPTIMIZATION)
    WebCore::pageCache()->setCapacity(0);
#else
    WebCore::pageCache()->setCapacity(pageCacheCapacity);
#endif

#if USE(SOUP)
    if (cache) {
        if (urlCacheDiskCapacity > soup_cache_get_max_size(cache))
            soup_cache_set_max_size(cache, urlCacheDiskCapacity);
    }
#endif
}
#else
void WebProcess::platformSetCacheModel(CacheModel)
{
    notImplemented();
}
#endif

#if ENABLE(TIZEN_CACHE_CONTROL)
void WebProcess::platformSetCacheDisabled(bool cacheDisabled)
{
#if USE(SOUP)
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    static String cacheDirPath;

    TIZEN_LOGI("cacheDisabled = %d", cacheDisabled);

    if (cacheDisabled) {
        SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));

        GOwnPtr<char> cacheDir;
        g_object_get(G_OBJECT(cache), "cache-dir", &cacheDir.outPtr(), NULL);
        if (!cacheDir)
            return;
        cacheDirPath = String::fromUTF8(cacheDir.get());
        soup_cache_dump(cache);
        soup_session_remove_feature(session, SOUP_SESSION_FEATURE(cache));
    } else {
        SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
        if (cache)
            return;

        if (!cacheDirPath.isEmpty()) {
            cache = soup_cache_new(cacheDirPath.utf8().data(), SOUP_CACHE_SINGLE_USER);
            if (!cache)
                return;
            soup_session_add_feature(session, SOUP_SESSION_FEATURE(cache));
            soup_cache_load(cache);
            g_object_unref(cache);
        }
    }
#endif
}
#endif

#if ENABLE(TIZEN_COMPRESSION_PROXY)
void WebProcess::platformSetCompressionProxyEnabled(bool enabled)
{
    WebCore::ResourceHandle::setCompressionProxyEnabled(enabled);
}

void WebProcess::platformCompressionProxyDataSizeGet(unsigned int& original_size, unsigned int& compressed_size)
{
    WebCore::ResourceHandle::compressionProxyDataSizeGet(original_size, compressed_size);
}

void WebProcess::platformCompressionProxyDataSizeSet(unsigned int original_size, unsigned int compressed_size)
{
    WebCore::ResourceHandle::compressionProxyDataSizeSet(original_size, compressed_size);
}

void WebProcess::platformCompressionProxyImageQualitySet(int quality)
{
    WebCore::ResourceHandle::compressionProxyImageQualitySet(quality);
}
#endif

void WebProcess::platformClearResourceCaches(ResourceCachesToClear cachesToClear)
{
#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
#if ENABLE(TIZEN_SOUP_CACHE)
    if (cachesToClear == InMemoryResourceCachesOnly)
        return;

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    if (!cache)
        return;

    soup_cache_clear(cache);
#endif
#endif
    notImplemented();
}

#if ENABLE(TIZEN_CACHE_DUMP_SYNC)
void WebProcess::platformDumpResourceCaches()
{
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    if (!cache)
        return;
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    ProxyRequestContext* context = WebCore::ResourceHandle::getProxyRequestContext();
    if (context) {
        WebCore::ResourceHandle::compressionProxyDataSizeDump();
    }
#endif
    soup_cache_dump(cache);
}
#endif

void WebProcess::platformInitializeWebProcess(const WebProcessCreationParameters& parameters, CoreIPC::ArgumentDecoder*)
{
#if ENABLE(TIZEN_WEBKIT2_PROXY)
    setProxy(parameters.proxyAddress);
#endif

#if ENABLE(TIZEN_SOUP_CACHE_DIRECTORY_SET)
    setSoupDataDirectory(parameters.soupDataDirectory);
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    setCertificateFile(parameters.certificateFile);
#endif
    notImplemented();
}

#if ENABLE(TIZEN_SOUP_CACHE_DIRECTORY_SET)

#if ENABLE (TIZEN_SOUP_CACHE_DIRECTORY_PATH_SET)
String getSoupCacheDirectory (const String& soupDataDirectory)
{
    String soupCacheDirectoryPath;
    char *cachePath = app_get_cache_path();

    if (cachePath && fileExists(cachePath))
        soupCacheDirectoryPath = String (cachePath);

    return soupCacheDirectoryPath;
}
#endif

void WebProcess::setSoupDataDirectory(const String& soupDataDirectory)
{
    if (soupDataDirectory.isEmpty())
    {
        TIZEN_LOGI("[Loader] WebProcess::setSoupDataDirectory is empty!!");
        SoupSession* session = WebCore::ResourceHandle::defaultSession();
        SoupSessionFeature* oldjar = soup_session_get_feature(session, SOUP_TYPE_COOKIE_JAR);
        if (oldjar)
            soup_session_remove_feature(session, oldjar);

        SoupCache* oldcache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
        if (oldcache) {
            soup_cache_dump(oldcache);
            soup_session_remove_feature(session, SOUP_SESSION_FEATURE(oldcache));
        }
        return;
    }
    TIZEN_SECURE_LOGI("[Loader] WebProcess::setSoupDataDirectory soupDataDirectory [%s]", soupDataDirectory.utf8().data());

    #if ENABLE(TIZEN_COMPRESSION_PROXY)
        WebCore::ResourceHandle::compressionProxyDataSizeFilePathSet (soupDataDirectory);
        WebCore::ResourceHandle::compressionProxyDataSizeLoad();
    #endif

#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
#if USE(SOUP)
    SoupCookieJar* cookieJar = 0;
    String cookieDirectoryPath = WebCore::pathByAppendingComponent(soupDataDirectory, "cookie");

    if (WebCore::makeAllDirectories(cookieDirectoryPath)) {
        String cookieFilePath = WebCore::pathByAppendingComponent(cookieDirectoryPath, ".cookie.db");
#if ENABLE(TIZEN_PERSISTENT_COOKIES_SQL)
        cookieJar = soup_cookie_jar_sqlite_new(cookieFilePath.utf8().data(), FALSE);
#else
        cookieJar = soup_cookie_jar_text_new(cookieFilePath.utf8().data(), FALSE);
#endif
    } else
        cookieJar = soup_cookie_jar_new();

    soup_cookie_jar_set_accept_policy(cookieJar, SOUP_COOKIE_JAR_ACCEPT_NO_THIRD_PARTY);
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    SoupSessionFeature* oldjar = soup_session_get_feature(session, SOUP_TYPE_COOKIE_JAR);
    if (oldjar)
        soup_session_remove_feature(session, oldjar);

    WebCore::setSoupCookieJar(cookieJar);
    soup_session_add_feature(session, SOUP_SESSION_FEATURE(cookieJar));

#if ENABLE(TIZEN_SOUP_CACHE)
    SoupCache* oldcache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    if (oldcache) {
        soup_cache_dump(oldcache);
        soup_session_remove_feature(session, SOUP_SESSION_FEATURE(oldcache));
    }

    SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    if (cache)
        return;

#if ENABLE (TIZEN_SOUP_CACHE_DIRECTORY_PATH_SET)
    String cacheDirectory = getSoupCacheDirectory(soupDataDirectory);
    if (!cacheDirectory.isEmpty())
        cache = soup_cache_new(cacheDirectory.utf8().data(), SOUP_CACHE_SINGLE_USER);

    if (!cache) {
        cacheDirectory = WebCore::pathByAppendingComponent(soupDataDirectory,"cache");
        if (!cacheDirectory.length())
            return;

        cache = soup_cache_new(cacheDirectory.utf8().data(), SOUP_CACHE_SINGLE_USER);
        if (!cache)
            return;
    }
    TIZEN_SECURE_LOGI ("SoupCache directory path [%s]", cacheDirectory.utf8().data());
#else
    String cacheDirectory = WebCore::pathByAppendingComponent(soupDataDirectory,"cache");
    if (!cacheDirectory.length())
        return;

    cache = soup_cache_new(cacheDirectory.utf8().data(), SOUP_CACHE_SINGLE_USER);
    if (!cache)
        return;
#endif

    SoupSession* soupSession = WebCore::ResourceHandle::defaultSession();
    if (!soupSession)
        return;

    soup_session_add_feature(soupSession, SOUP_SESSION_FEATURE(cache));
    soup_cache_load(cache);
    g_object_unref(cache);
#endif
#endif
#endif
}
#endif

void WebProcess::platformTerminate()
{
#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
#if ENABLE(TIZEN_SOUP_CACHE)
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    if (!session)
        return;

    SoupCache* cache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
    if (!cache)
        return;

    soup_cache_dump(cache);

    soup_session_remove_feature(session, SOUP_SESSION_FEATURE(cache));
#endif
#endif
}

#if ENABLE(TIZEN_WEBKIT2_PROXY)
void WebProcess::setProxy(const WTF::String& proxyAddress)
{
    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    if (!proxyAddress) {
        TIZEN_LOGI("!proxyAddress soup_session_remove_feature_by_type SOUP_TYPE_PROXY_RESOLVER");
        g_object_set(session, SOUP_SESSION_PROXY_URI, 0, NULL);
        soup_session_remove_feature_by_type(session, SOUP_TYPE_PROXY_RESOLVER);
        return;
    }

    WTF::String proxyValue = proxyAddress;
    if (!proxyAddress.startsWith("http://", false))
        proxyValue.insert("http://", 0);

    SoupURI* uri = soup_uri_new(proxyValue.utf8().data());
    if (uri) {
        TIZEN_SECURE_LOGI("proxyAddress [%s]", proxyAddress.utf8().data());
        g_object_set(session, SOUP_SESSION_PROXY_URI, uri, NULL);
        soup_uri_free(uri);
    }
}
#endif

#if ENABLE(TIZEN_SESSION_REQUEST_CANCEL)
void WebProcess::abortSession()
{
    TIZEN_LOGI("abortSession() is called. call soup_session_abort()");
    soup_session_abort(WebCore::ResourceHandle::defaultSession());
}
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
void WebProcess::setCertificateFile(const WTF::String& certificateFile)
{
#if USE(SOUP)
    SoupSession* session = WebCore::ResourceHandle::defaultSession();

    TIZEN_LOGI("setCertificateFile() is called. call g_object_set().");
#ifdef SOUP_SESSION_CERTIFICATE_PATH
    g_object_set(session, SOUP_SESSION_SSL_STRICT, FALSE,
                SOUP_SESSION_CERTIFICATE_PATH, certificateFile.utf8().data(), NULL);
#else
    TIZEN_LOGI("SOUP_SESSION_CERTIFICATE_PATH is not defined");
#endif
#endif
}
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void WebProcess::removePlatformSurfacesFromPool(const Vector<int>& platformSurfacesToRemove)
{
    if (!m_platformSurfacePool)
        return;

    size_t size = platformSurfacesToRemove.size();
    for (size_t index = 0; index < size; index++)
        m_platformSurfacePool.get()->removePlatformSurface(platformSurfacesToRemove[index]);
}

void WebProcess::adjustPlatformSurfacePoolSize(const WebCore::IntSize& viewportSize)
{
    m_platformSurfacePool.get()->adjustMaxSizeInCount(viewportSize);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_MEMORY_SAVING_MODE)
void WebProcess::setMemorySavingMode(bool mode)
{
    m_memorySavingModeEnabled = mode;
    if (!mode)
        return;

    // FIXME: appropriate settings will be added.

}
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
void WebProcess::setXWindow(unsigned xWindow)
{
    WebCore::setXWindow(xWindow);
}
#endif

#if ENABLE(TIZEN_EXTENSIBLE_API)
void WebProcess::setTizenExtensibleAPI(int extensibleAPI, bool enable)
{
    WebCore::TizenExtensibleAPI::extensibleAPI().setTizenExtensibleAPI(static_cast<WebCore::ExtensibleAPI>(extensibleAPI), enable);
}
#endif

#if ENABLE(TIZEN_RESET_PATH)
void WebProcess::resetStoragePath()
{
    WebCore::cacheStorage().setCacheDirectory(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/appcache"));
    if (!WebCore::LocalFileSystem::initializeLocalFileSystem(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/localFileSystem")))
        WebCore::LocalFileSystem::localFileSystem().changeFileSystemBasePath(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/localFileSystem"));
    if (!WebCore::StorageTracker::initializeTracker(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/localStorage"),  &WebKeyValueStorageManager::shared()))
        WebCore::StorageTracker::tracker().setDatabaseDirectoryPath(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/localStorage"));
    if (!WebDatabaseManager::initialize(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/databases")))
        WebCore::DatabaseTracker::tracker().setDatabaseDirectoryPath(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/databases"));

    HashMap<uint64_t, RefPtr<WebPage> >::iterator end = m_pageMap.end();
    for (HashMap<uint64_t, RefPtr<WebPage> >::iterator it = m_pageMap.begin(); it != end; ++it) {
        WebPage* page = (*it).second.get();
#if ENABLE(TIZEN_INDEXED_DATABASE)
        page->setIndexedDatabaseDirectory(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/indexedDatabases"));
#endif
        page->setLocalStorageDirectory(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/localStorage"));
    }
#if ENABLE(TIZEN_INDEXED_DATABASE)
    m_indexedDatabaseDirectory = WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/indexedDatabases");
#endif
    m_localStorageDirectory = WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/localStorage");
#if ENABLE(TIZEN_SOUP_CACHE_DIRECTORY_SET)
    setSoupDataDirectory(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/soupData"));
#endif
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    WebCore::bufferDatabase().setResourceCacheDirectory(WebCore::pathByAppendingComponent(WebCore::homeDirectoryPath(), ".webkit/resourceCache/"));
#endif
}
#endif

#if ENABLE(TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR)
void WebProcess::inspectorServerStart(uint32_t port,  uint32_t& assignedPort)
{
    bool ret = WebInspectorServerEfl::server()->startServer(port);
    if (ret)
        assignedPort = WebInspectorServerEfl::server()->getServerPort();
    else
        assignedPort = 0;
}

void WebProcess::inspectorServerStop(bool& result)
{
    result = WebInspectorServerEfl::server()->stopServer();
}
#endif
} // namespace WebKit
