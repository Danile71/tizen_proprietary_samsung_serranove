/*
 * Copyright (C) 2004, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ResourceHandle.h"
#include "ResourceHandleInternal.h"

#include "BlobRegistry.h"
#include "Logging.h"
#include "ResourceHandleClient.h"
#include "Timer.h"
#include <algorithm>
#include <wtf/MainThread.h>
#include <wtf/text/CString.h>
#if ENABLE(TIZEN_COMPRESSION_PROXY)
#include "soup/tizen/uc/HTTPCacheQuerierImpl.h"
#include <UCProxySDK/HTTPCacheQuerier.h>
#include <glib.h>
#endif
namespace WebCore {

static bool shouldForceContentSniffing;

#if ENABLE(TIZEN_COMPRESSION_PROXY)
GHashTable* ResourceHandle::m_uc_proxy_failure_info = NULL;
bool ResourceHandle::m_compressionProxyEnabled = false;
static const char *UCproxy_blacklist[] = {
    "youtube.com","amazon.com","daum.net","testmy.net","irctc.com",
    NULL
};
GHashTable* ResourceHandle::get_proxy_failure_table()
{
    if(!m_uc_proxy_failure_info) {
        m_uc_proxy_failure_info = g_hash_table_new (g_direct_hash, g_direct_equal);
    }
    return m_uc_proxy_failure_info;
}
void ResourceHandle::set_proxy_failure_table(GHashTable *table)
{
    m_uc_proxy_failure_info = table;
}
#endif

typedef HashMap<AtomicString, ResourceHandle::BuiltinConstructor> BuiltinResourceHandleConstructorMap;
static BuiltinResourceHandleConstructorMap& builtinResourceHandleConstructorMap()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(BuiltinResourceHandleConstructorMap, map, ());
    return map;
}

#if ENABLE(TIZEN_COMPRESSION_PROXY)
bool ResourceHandle::is_site_local (const char* host)
{
    GInetAddress *inet_addr = g_inet_address_new_from_string(host);
    gboolean is_local = false;
    if(inet_addr)
         is_local = g_inet_address_get_is_site_local(inet_addr);

    return is_local;
}

bool ResourceHandle::is_uri_blacklisted_for_UCproxy (const char *url)
{
    int i = 0;
    bool result = FALSE;

    if (!url)
        return FALSE;

    while (UCproxy_blacklist[i] != NULL) {
        if (g_strrstr(url, UCproxy_blacklist[i])){
            result = TRUE;
            break;
        }
        ++i;
    }

    return result;
}
#endif

void ResourceHandle::registerBuiltinConstructor(const AtomicString& protocol, ResourceHandle::BuiltinConstructor constructor)
{
    builtinResourceHandleConstructorMap().add(protocol, constructor);
}

ResourceHandle::ResourceHandle(const ResourceRequest& request, ResourceHandleClient* client, bool defersLoading, bool shouldContentSniff)
    : d(adoptPtr(new ResourceHandleInternal(this, request, client, defersLoading, shouldContentSniff && shouldContentSniffURL(request.url()))))
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    , m_proxyRequest(0)
    , m_cachePushEntry(0)
#endif
{
    if (!request.url().isValid()) {
        scheduleFailure(InvalidURLFailure);
        return;
    }

    if (!portAllowed(request.url())) {
        scheduleFailure(BlockedFailure);
        return;
    }
}

#if ENABLE(TIZEN_COMPRESSION_PROXY)
static SoupMessage* getSoupMessageForRequest(const ResourceRequest& request)
{
    SoupMessage* msg = soup_message_new ("GET", request.url().string().utf8().data());
    if (!msg)
        return NULL;
    request.updateSoupMessage(msg);
    return msg;
}
#endif

PassRefPtr<ResourceHandle> ResourceHandle::create(NetworkingContext* context, const ResourceRequest& request, ResourceHandleClient* client, bool defersLoading, bool shouldContentSniff)
{
    BuiltinResourceHandleConstructorMap::iterator protocolMapItem = builtinResourceHandleConstructorMap().find(request.url().protocol());

    if (protocolMapItem != builtinResourceHandleConstructorMap().end())
        return protocolMapItem->second(request, client);

    RefPtr<ResourceHandle> newHandle(adoptRef(new ResourceHandle(request, client, defersLoading, shouldContentSniff)));

    if (newHandle->d->m_scheduledFailureType != NoFailure)
        return newHandle.release();
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    TIZEN_SECURE_LOGI("[UCPROXY] [%s]", ResourceHandle::compressionProxyEnabled() ? "TRUE" : "FALSE");
    DiskCacheState cacheStatus = EDiskCacheStateInvalid;
    ProxyRequestContext* proxyContext = NULL;
    bool is_local = is_site_local(request.url().host().utf8().data());

    if (ResourceHandle::compressionProxyEnabled()) {
        std::string url (request.url().string().utf8().data());
        SoupMessage* msg = getSoupMessageForRequest (request);
        proxyContext = newHandle->getProxyRequestContext();
        if (proxyContext) {
            if (((HTTPCacheQuerierImpl *)proxyContext->getHTTPCacheQuerier())) {
                cacheStatus = (DiskCacheState) ((HTTPCacheQuerierImpl *)proxyContext->getHTTPCacheQuerier())->getDiskCacheState(msg);
                TIZEN_SECURE_LOGI("[UCPROXY] cache status of resource [%s], status [%d]", url.c_str(), cacheStatus);
            }
            if (((ResourceRequestBase) request).isMainFrameRequest() && (WebCore::protocolIs(request.url().string(), "https") || is_local || is_uri_blacklisted_for_UCproxy(request.url().host().utf8().data()))) {
               newHandle->client()->ucBypassSet(true); //Resource Handle by pass happens
               TIZEN_SECURE_LOGI("[UCPROXY] BYPASS url [%s], isMainFrameRequest() [%d], isHttps() [%d], is_site_local [%d], is_blacklisted[%d]", url.c_str(), ((ResourceRequestBase) request).isMainFrameRequest(), WebCore::protocolIs(request.url().string(), "https"),
               is_local, is_uri_blacklisted_for_UCproxy(request.url().host().utf8().data()));
            }
        }
        if(msg)
            g_object_unref(msg);
    }

    if (ResourceHandle::compressionProxyEnabled() && proxyContext && WebCore::protocolIs(request.url().string(), "http")
        && (request.httpMethod() == "GET")
        && newHandle->client()->getUCFrameID()
#if ENABLE(TIZEN_PRIVATE_BROWSING)
        && !newHandle->privateBrowsingEnabledGet()
#endif
        && !newHandle->client()->ucBypassGet()
        && !is_local
        && !((EDiskCacheStateFresh == cacheStatus) || (EDiskCacheStateInvalid == cacheStatus))) {
            if (newHandle->startUC(context, newHandle->firstRequest()))
                return newHandle.release();
    } else {
        if (newHandle->start(context))
            return newHandle.release();
    }
#else
    if (newHandle->start(context))
        return newHandle.release();
#endif

    return 0;
}

void ResourceHandle::scheduleFailure(FailureType type)
{
    d->m_scheduledFailureType = type;
    d->m_failureTimer.startOneShot(0);
}

void ResourceHandle::fireFailure(Timer<ResourceHandle>*)
{
    if (!client())
        return;

    switch (d->m_scheduledFailureType) {
        case NoFailure:
            ASSERT_NOT_REACHED();
            return;
        case BlockedFailure:
            d->m_scheduledFailureType = NoFailure;
            client()->wasBlocked(this);
            return;
        case InvalidURLFailure:
            d->m_scheduledFailureType = NoFailure;
            client()->cannotShowURL(this);
            return;
    }

    ASSERT_NOT_REACHED();
}

ResourceHandleClient* ResourceHandle::client() const
{
    return d->m_client;
}

void ResourceHandle::setClient(ResourceHandleClient* client)
{
    d->m_client = client;
}

ResourceRequest& ResourceHandle::firstRequest()
{
    return d->m_firstRequest;
}

const String& ResourceHandle::lastHTTPMethod() const
{
    return d->m_lastHTTPMethod;
}

bool ResourceHandle::hasAuthenticationChallenge() const
{
    return !d->m_currentWebChallenge.isNull();
}

void ResourceHandle::clearAuthentication()
{
#if PLATFORM(MAC)
    d->m_currentMacChallenge = nil;
#endif
    d->m_currentWebChallenge.nullify();
#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    d->m_soupAuth.clear();
#endif
}
  
bool ResourceHandle::shouldContentSniff() const
{
    return d->m_shouldContentSniff;
}

bool ResourceHandle::shouldContentSniffURL(const KURL& url)
{
#if PLATFORM(MAC)
    if (shouldForceContentSniffing)
        return true;
#endif
    // We shouldn't content sniff file URLs as their MIME type should be established via their extension.
    return !url.protocolIs("file");
}

void ResourceHandle::forceContentSniffing()
{
    shouldForceContentSniffing = true;
}

void ResourceHandle::setDefersLoading(bool defers)
{
    LOG(Network, "Handle %p setDefersLoading(%s)", this, defers ? "true" : "false");

    ASSERT(d->m_defersLoading != defers); // Deferring is not counted, so calling setDefersLoading() repeatedly is likely to be in error.
    d->m_defersLoading = defers;

    if (defers) {
        ASSERT(d->m_failureTimer.isActive() == (d->m_scheduledFailureType != NoFailure));
        if (d->m_failureTimer.isActive())
            d->m_failureTimer.stop();
    } else if (d->m_scheduledFailureType != NoFailure) {
        ASSERT(!d->m_failureTimer.isActive());
        d->m_failureTimer.startOneShot(0);
    }

    platformSetDefersLoading(defers);
}

void ResourceHandle::cacheMetadata(const ResourceResponse&, const Vector<char>&)
{
    // Optionally implemented by platform.
}

#if ENABLE(TIZEN_COMPRESSION_PROXY)
void ResourceHandle::cancel()
{
 #if USE(TIZEN_LOG)
    LOG(Network,"[Network] ResourceHandle::cancel()\n");
#endif
    d->m_cancelled = true;
    this->cancelSoup();
    this->cancelUC();
}
#endif

} // namespace WebCore
