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
#include "WebResourceCacheManager.h"

#include "Connection.h"
#include "MessageID.h"
#include "ResourceCachesToClear.h"
#include "SecurityOriginData.h"
#include "WebCoreArgumentCoders.h"
#include "WebResourceCacheManagerProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/MemoryCache.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityOriginHash.h>
#include <wtf/UnusedParam.h>
#include "TizenSystemUtilities.h"

using namespace WebCore;

namespace WebKit {

WebResourceCacheManager& WebResourceCacheManager::shared()
{
    static WebResourceCacheManager& shared = *new WebResourceCacheManager;
    return shared;
}

WebResourceCacheManager::WebResourceCacheManager()
{
}

WebResourceCacheManager::~WebResourceCacheManager()
{
}

void WebResourceCacheManager::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceiveWebResourceCacheManagerMessage(connection, messageID, arguments);
}

#if ENABLE(TIZEN_CACHE_DUMP_SYNC)
void WebResourceCacheManager::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments, OwnPtr<CoreIPC::ArgumentEncoder>& reply)
{
    didReceiveSyncWebResourceCacheManagerMessage(connection, messageID, arguments, reply);
}
#endif

void WebResourceCacheManager::getCacheOrigins(uint64_t callbackID) const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

    MemoryCache::SecurityOriginSet origins;
    memoryCache()->getOriginsWithCache(origins);

#if USE(CFURLCACHE)
    RetainPtr<CFArrayRef> cfURLHosts = cfURLCacheHostNames();
    CFIndex size = cfURLHosts ? CFArrayGetCount(cfURLHosts.get()) : 0;

    String httpString("http");
    for (CFIndex i = 0; i < size; ++i) {
        CFStringRef host = static_cast<CFStringRef>(CFArrayGetValueAtIndex(cfURLHosts.get(), i));
        origins.add(SecurityOrigin::create(httpString, host, 0));
    }
#endif

    // Create a list with the origins in both of the caches.
    Vector<SecurityOriginData> identifiers;
    identifiers.reserveCapacity(origins.size());

    MemoryCache::SecurityOriginSet::iterator end = origins.end();
    for (MemoryCache::SecurityOriginSet::iterator it = origins.begin(); it != end; ++it) {
        RefPtr<SecurityOrigin> origin = *it;
        
        SecurityOriginData originData;
        originData.protocol = origin->protocol();
        originData.host = origin->host();
        originData.port = origin->port();

        identifiers.uncheckedAppend(originData);
    }

    WebProcess::shared().connection()->send(Messages::WebResourceCacheManagerProxy::DidGetCacheOrigins(identifiers, callbackID), 0);
}

void WebResourceCacheManager::clearCacheForOrigin(SecurityOriginData originData, uint32_t cachesToClear) const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

#if USE(CFURLCACHE)
    ResourceCachesToClear resourceCachesToClear = static_cast<ResourceCachesToClear>(cachesToClear);
#else
    UNUSED_PARAM(cachesToClear);
#endif

    RefPtr<SecurityOrigin> origin = SecurityOrigin::create(originData.protocol, originData.host, originData.port);
    if (!origin)
        return;

    memoryCache()->removeResourcesWithOrigin(origin.get());

#if USE(CFURLCACHE)
    if (resourceCachesToClear != InMemoryResourceCachesOnly) { 
        RetainPtr<CFMutableArrayRef> hostArray(AdoptCF, CFArrayCreateMutable(0, 0, &kCFTypeArrayCallBacks));
        RetainPtr<CFStringRef> host(AdoptCF, origin->host().createCFString());
        CFArrayAppendValue(hostArray.get(), host.get());

        clearCFURLCacheForHostNames(hostArray.get());
    }
#endif
}

void WebResourceCacheManager::clearCacheForAllOrigins(uint32_t cachesToClear) const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

    ResourceCachesToClear resourceCachesToClear = static_cast<ResourceCachesToClear>(cachesToClear);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    size_t memoryFree = WTF::getFreeMemory();
    TIZEN_LOGI("LowMemory: Free Memory before clearing caches = %d KB", memoryFree);
#endif

    WebProcess::shared().clearResourceCaches(resourceCachesToClear);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    memoryFree = WTF::getFreeMemory();
    TIZEN_LOGI("LowMemory: Free Memory after clearing caches = %d KB", memoryFree);
#endif
}

#if ENABLE(TIZEN_CACHE_DUMP_SYNC)
void WebResourceCacheManager::dumpCache() const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());

    WebProcess::shared().dumpResourceCaches();
}
#endif

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
void WebResourceCacheManager::restoreCacheForAllOrigins() const
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());
    memoryCache()->restoreResourcesFromDisk();
}

void WebResourceCacheManager::markVisibleContents(const WebCore::IntRect& visibleRect)
{
    WebProcess::LocalTerminationDisabler terminationDisabler(WebProcess::shared());
    memoryCache()->markVisibleContents(visibleRect);
}
#endif

} // namespace WebKit
