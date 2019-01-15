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

#ifndef __UCProxy__HTTPCacheQuerierImpl__H__
#define __UCProxy__HTTPCacheQuerierImpl__H__

#include <UCProxySDK/HTTPCacheQuerier.h>
#include <UCProxySDK/HTTPCacheQuerierClient.h>
#include <UCProxySDK/HTTPURLRequest.h>
#include <UCProxySDK/HTTPURLResponse.h>
#include <string>

#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup.h>
#include <libsoup/soup-cache.h>

namespace proxy {

enum DiskCacheState
{
    EDiskCacheStateFresh,
    EDiskCacheStateNeedValidation,
    EDiskCacheStateStale,
    EDiskCacheStateNone,
    EDiskCacheStateInvalid
};

class HTTPCacheQuerierImpl : public HTTPCacheQuerier
{
public:
    /*************************************************************************************************
    * @Destructor for LocalCacheDelegate.
    * 
    * Detailed description: 
        Pure abstract class should have a virtual destructor.
    *************************************************************************************************/
    HTTPCacheQuerierImpl();
    ~HTTPCacheQuerierImpl();

//////////////////////// inherited from HTTPCacheQuerier START

    /*************************************************************************************************
    * @Function name:  setClient 
    * 
    * Detailed description: set local cache proxy client.
    * @param[in] client : client of local cache proxy
    *************************************************************************************************/
    void setClient(HTTPCacheQuerierClient* client);
    
    /*************************************************************************************************
    * @Function name:  getResourceCacheLocation 
    * 
    * Detailed description: get location of resource cache
    * @param[in] url : url of resource cache
    * @param[in] isMainResource Defaults to false.: if the resource is main resource
    * @return ResourceLocation  
    *************************************************************************************************/
    ResourceLocation getResourceCacheLocation(const std::string& url);
    
    /*************************************************************************************************
    * @Function name:  canonicalRequestURL 
    * 
    * Detailed description: get canonical request url
    * @param[in] urlString : url input
    * @param[in] outURLString : url output
    * @return const std::string&  : 
    *************************************************************************************************/
    const std::string& canonicalRequestURL(const  std::string& urlString,  std::string& outURLString);

//////////////////////// inherited from HTTPCacheQuerier END

    DiskCacheState getDiskCacheState(SoupMessage* msg);

    /*************************************************************************************************
     * @Function name:  onCacheResourceRemove
     *
     * Detailed description: called on cache resource is removed.
     * @param[in] url : url of cache resource
     *************************************************************************************************/
    void onCacheResourceRemove(const std::string& url);
    
    /*************************************************************************************************
     * @Function name:  onCacheResourceClear
     *
     * Detailed description: called on cache resource is cleared.
     *************************************************************************************************/
     void onCacheResourceClear();
     /*************************************************************************************************
      * @Function name:  attachSoupCache
      *
      * Detailed description: keep soupCache in HTTPCacheQuerier and attach libsoup callbacks
      *************************************************************************************************/
     void attachSoupCache();
     /*************************************************************************************************
      * @Function name:  detachSoupCache
      *
      * Detailed description: detach libsoup callbacks from soupCache and remove from HTTPCacheQuerier
      *************************************************************************************************/
     void detachSoupCache();

private:
    HTTPCacheQuerierClient* m_client;
    SoupCache* m_soupCache;
    gulong handler_id_removedCacheResource;
    gulong handler_id_cacheCleared;
};

class HTTPCachePushEntry
{
public:
    HTTPCachePushEntry();
    ~HTTPCachePushEntry();

    bool cachePushStart(const HTTPURLRequest& request, const HTTPURLResponse& response);
    void cachePushSendData(const char* data, int length);
    void cachePushFinished();
    void cachePushCancel();

private:
    SoupCache* m_soupCache;
    SoupMessage* m_soupMessage;
    gpointer m_handle;
};

}// namespace Proxy

#endif //__UCProxy__HTTPCacheQuerierImpl__H__
