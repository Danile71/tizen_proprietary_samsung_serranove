/*
 * Copyright (C) 2014 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "HTTPCacheQuerierImpl.h"

#include "Logging.h"
#include "ResourceHandle.h"
#include <UCProxySDK/HTTPHeaderMap.h>

namespace proxy {

static void removedCacheResourceCallback(SoupCache* soupCache, const char* url, gpointer user_data);
static void cacheClearedCallback(SoupCache* soupCache, gpointer user_data);

HTTPCacheQuerierImpl::HTTPCacheQuerierImpl()
{
    attachSoupCache();
}

HTTPCacheQuerierImpl::~HTTPCacheQuerierImpl()
{
    detachSoupCache();
}

void HTTPCacheQuerierImpl::setClient(HTTPCacheQuerierClient* client)
{
        m_client = client;
}

ResourceLocation HTTPCacheQuerierImpl::getResourceCacheLocation(const std::string& url)
{
        g_return_val_if_fail(SOUP_IS_CACHE(m_soupCache), EResLocNone);

        SoupMessage* msg = soup_message_new ("GET", url.c_str());
        if (!msg)
                return EResLocNone;

        SoupCacheResponseUC response = SOUP_CACHE_GET_CLASS (m_soupCache)->has_response(m_soupCache, msg);
        g_object_unref(msg);

        switch (response) {
        case SOUP_CACHE_RESPONSE_UC_FRESH:
        case SOUP_CACHE_RESPONSE_UC_NEEDS_VALIDATION:
        case SOUP_CACHE_RESPONSE_UC_STALE:
                return EResLocInDiskCache;

        default:
                return EResLocNone;
        }
}

const std::string& HTTPCacheQuerierImpl::canonicalRequestURL(const  std::string& urlString,  std::string& outURLString)
{
        // return the original urlString
        outURLString = urlString;
        return outURLString;
}

DiskCacheState HTTPCacheQuerierImpl::getDiskCacheState(SoupMessage* msg)
{
        if (!msg)
                return EDiskCacheStateInvalid;

        g_return_val_if_fail(SOUP_IS_CACHE(m_soupCache), EDiskCacheStateNone);

        SoupCacheResponseUC response = SOUP_CACHE_GET_CLASS (m_soupCache)->has_response(m_soupCache, msg);

        switch (response) {
        case SOUP_CACHE_RESPONSE_UC_FRESH:
                return EDiskCacheStateFresh;

        case SOUP_CACHE_RESPONSE_UC_NEEDS_VALIDATION:
                return EDiskCacheStateNeedValidation;

        case SOUP_CACHE_RESPONSE_UC_STALE:
                return EDiskCacheStateStale;

        case SOUP_CACHE_RESPONSE_UC_NONE:
        default :
                return EDiskCacheStateNone;
	}
}

void HTTPCacheQuerierImpl::onCacheResourceRemove(const std::string& url)
{
    if(m_client)
       m_client->onCacheResourceRemove(url);
}
        
void HTTPCacheQuerierImpl::onCacheResourceClear()
{
    if(m_client)
       m_client->onCacheResourceClear();
}

void HTTPCacheQuerierImpl::attachSoupCache()
{
    SoupSession* session = WebCore::ResourceHandle::defaultSession();

    m_soupCache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));

    g_object_ref(m_soupCache);
    handler_id_removedCacheResource = g_signal_connect(m_soupCache, "removed_cache_resource", G_CALLBACK(removedCacheResourceCallback), this);
    handler_id_cacheCleared = g_signal_connect(m_soupCache, "cache_cleared", G_CALLBACK(cacheClearedCallback), this);
}

void HTTPCacheQuerierImpl::detachSoupCache()
{
    g_signal_handler_disconnect(m_soupCache, handler_id_removedCacheResource);
    g_signal_handler_disconnect(m_soupCache, handler_id_cacheCleared);

    g_object_unref(m_soupCache);
    m_soupCache = NULL;
}

HTTPCachePushEntry::HTTPCachePushEntry()
: m_soupMessage(NULL)
, m_handle(NULL)
{
	SoupSession* session = WebCore::ResourceHandle::defaultSession();
	m_soupCache = reinterpret_cast<SoupCache*>(soup_session_get_feature(session, SOUP_TYPE_CACHE));
	if (m_soupCache)
		g_object_ref (m_soupCache);
}

HTTPCachePushEntry::~HTTPCachePushEntry()
{
	if(m_soupMessage) {
		g_object_unref(m_soupMessage);
		m_soupMessage = NULL;
	}
	if (m_soupCache)
		g_object_unref (m_soupCache);
	m_soupCache = NULL;
}

bool HTTPCachePushEntry::cachePushStart(const HTTPURLRequest& request, const HTTPURLResponse& response)
{
	if(m_handle) {
		TIZEN_LOGI ("[HTTPWrapper] m_handle exists");
		return false;
	}

	if(m_soupMessage) {
		TIZEN_LOGI ("[HTTPWrapper] m_soupMessage exists");
		return false;
	}

	m_soupMessage = soup_message_new (request.httpMethod().c_str(), request.getUrl().c_str());
	if(!m_soupMessage) {
		TIZEN_SECURE_LOGI ("[HTTPWrapper] fail to create m_soupMessage for url[%s]", request.getUrl().c_str());
		return false;
	}

	soup_message_set_status(m_soupMessage, response.httpStatusCode());

    HTTPHeaderMap headerMap = request.getAllHttpHeaderFields();
    for (HTTPHeaderMap::iterator it=headerMap.begin(); it!=headerMap.end(); ++it) {
        const proxy::UCString name = it->first;
        const proxy::UCString value = it->second;
        soup_message_headers_append(m_soupMessage->request_headers, name.c_str(), value.c_str());
    }

    headerMap = response.getAllHttpHeaderFields();
    for (HTTPHeaderMap::iterator it=headerMap.begin(); it!=headerMap.end(); ++it) {
        const proxy::UCString name = it->first;
        const proxy::UCString value = it->second;
        soup_message_headers_append(m_soupMessage->response_headers, name.c_str(), value.c_str());
    }

    if(!soup_message_headers_get_one (m_soupMessage->response_headers, "Content-Type") && !response.mimeType().empty())
        soup_message_headers_replace (m_soupMessage->response_headers, "Content-Type", response.mimeType().c_str());

    g_return_val_if_fail (m_soupCache, false);
    g_return_val_if_fail (SOUP_IS_CACHE (m_soupCache), false);
	m_handle = SOUP_CACHE_GET_CLASS (m_soupCache)->push_start(m_soupCache, m_soupMessage);
    if (!m_handle) {
        TIZEN_SECURE_LOGI ("[HTTPWrapper] Push failed for the resource [%s]", request.getUrl().c_str());
        return false;
    }

	return true;
}

void HTTPCachePushEntry::cachePushSendData(const char* data, int length)
{
    if(!m_handle || !m_soupMessage || !SOUP_IS_CACHE (m_soupCache)) {
		TIZEN_LOGI("[HTTPWrapper] No m_handle[%p] or m_soupMessage[%p] or SOUP_IS_CACHE (m_soupCache)[%d]", m_handle, m_soupMessage, SOUP_IS_CACHE (m_soupCache));
		return;
	}

	SOUP_CACHE_GET_CLASS (m_soupCache)->push_ongoing(m_handle, data, length);
}

void HTTPCachePushEntry::cachePushFinished()
{
    if(!m_handle || !m_soupMessage || !SOUP_IS_CACHE (m_soupCache)) {
		TIZEN_LOGI("[HTTPWrapper] No m_handle[%p] or m_soupMessage[%p] or SOUP_IS_CACHE (m_soupCache)[%d]", m_handle, m_soupMessage
, SOUP_IS_CACHE (m_soupCache));
		return;
	}

	SOUP_CACHE_GET_CLASS (m_soupCache)->push_ongoing(m_handle, NULL, 0);

	m_handle = NULL;
}

void HTTPCachePushEntry::cachePushCancel()
{
    if(!m_handle || !m_soupMessage || !SOUP_IS_CACHE (m_soupCache)) {
		TIZEN_LOGI("[HTTPWrapper] No m_handle[%p] or m_soupMessage[%p] or SOUP_IS_CACHE (m_soupCache)[%d]", m_handle, m_soupMessage
 , SOUP_IS_CACHE (m_soupCache));
		return;
	}

	SOUP_CACHE_GET_CLASS (m_soupCache)->push_cancel(m_handle);

	m_handle = NULL;
}

/////////////////////////////////////////////////////////////////////////
// callbacks for libsoup
/////////////////////////////////////////////////////////////////////////
void removedCacheResourceCallback(SoupCache* soupCache, const char* url, gpointer user_data)
{
	HTTPCacheQuerierImpl* cacheQuerier = static_cast<HTTPCacheQuerierImpl*>(user_data);

	if(cacheQuerier)
		cacheQuerier->onCacheResourceRemove(url);
}

void cacheClearedCallback(SoupCache* soupCache, gpointer user_data)
{
	HTTPCacheQuerierImpl* cacheQuerier = static_cast<HTTPCacheQuerierImpl*>(user_data);

	if(cacheQuerier)
		cacheQuerier->onCacheResourceClear();
}

} //namespace proxy
