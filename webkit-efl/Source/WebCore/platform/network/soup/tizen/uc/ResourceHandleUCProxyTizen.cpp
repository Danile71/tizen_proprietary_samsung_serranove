/*
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Xan Lopez <xan@gnome.org>
 * Copyright (C) 2008, 2010 Collabora Ltd.
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2009, 2010, 2011 Igalia S.L.
 * Copyright (C) 2009 John Kjellberg <john.kjellberg@power.alstom.com>
 * Copyright (C) 2012 Intel Corporation
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
#include "ResourceHandle.h"

#include "CachedResourceLoader.h"
#include "ChromeClient.h"
#include "CookieJarSoup.h"
#include "FileSystem.h"
#include "Frame.h"
#include "GOwnPtrSoup.h"
#include "HTTPParsers.h"
#include "Logging.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "Page.h"
#include "ResourceError.h"
#include "ResourceHandleClient.h"
#include "ResourceLoader.h"
#include "ResourceHandleInternal.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include "TextEncoding.h"
#include <errno.h>
#include <fcntl.h>
#include <gio/gio.h>
#include <glib.h>
#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup-request-http.h>
#include <libsoup/soup-requester.h>
#include <libsoup/soup.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

#if ENABLE(BLOB)
#include "BlobData.h"
#include "BlobRegistryImpl.h"
#include "BlobStorageData.h"
#endif

#if ENABLE(FILE_SYSTEM)
#include "AsyncFileSystemTizen.h"
#endif

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
#include "CredentialStorage.h"
#endif

#if ENABLE(TIZEN_PRIVATE_BROWSING)
#define LIBSOUP_USE_UNSTABLE_REQUEST_API
#include <libsoup/soup-cache.h>
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
#include <sys/time.h>
#endif

#if ENABLE(TIZEN_COMPRESSION_PROXY)
#include "NetTransactionImpl.h"
#include "CookieStorageAccessorImpl.h"
#include "ProxyDataStreamQuerierImpl.h"
#include "HTTPCacheQuerierImpl.h"
#include <UCProxySDK/ProxyError.h>
#include <UCProxySDK/ProxyURLRequest.h>
#include <UCProxySDK/ProxyURLResponse.h>
#include <UCProxySDK/ProxyRequestContext.h>
#include <UCProxySDK/NetTransaction.h>
#include <UCProxySDK/HTTPHeaderMap.h>
#include <UCProxySDK/NetTransactionDelegate.h>

#define DATA_SAVINGS_VALUES_IN_BYTES 1

using namespace std;

namespace WebCore {

//#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
static void applyAuthenticationToRequest(ResourceHandle* handle, ResourceRequest& request, bool redirect);
//#endif

using namespace proxy;

ProxyRequestContext* ResourceHandle::m_proxyRequestContext = NULL;

void ResourceHandle::ensureProxyRequestContext()
{
    if (!m_proxyRequestContext) {
        TIZEN_LOGI ("[UCPROXY] m_proxyRequestContext = NULL, new ProxyRequestContext is about to create");
        m_proxyRequestContext = new ProxyRequestContext();

        m_proxyRequestContext->initialize();
        m_proxyRequestContext->setCookieStorageAccessor(new CookieStorageAccessorImpl());
        m_proxyRequestContext->setHTTPCacheQuerier(new HTTPCacheQuerierImpl());
        m_proxyRequestContext->setNetworkTransactionFactory(new NetTransactionFactoryImpl());
        m_proxyRequestContext->setProxyDataStreamQuerier(new ProxyDataStreamQuerierImpl());
        TIZEN_LOGI ("[UCPROXY] m_proxyRequestContext = [%p]", m_proxyRequestContext);
    }
}

ProxyRequestContext* ResourceHandle::getProxyRequestContext()
{
    return m_proxyRequestContext;
}

bool ResourceHandle::startUC(NetworkingContext *netContext, ResourceRequest& request)
{
    ProxyRequestContext* context = getProxyRequestContext();
    if (!context) {
        TIZEN_LOGI ("[UCPROXY] context = [%p] should not occur!!", context);
        return false;
    }

    std::string url (request.url().string().utf8().data());
    std::string httpMethod (request.httpMethod().utf8().data());
    int frameId;
    FormData* httpBody = request.httpBody();
    StringBuilder builder;
    unsigned long totalBodySize = 0;

    d->m_context = netContext;
    frameId = client()->getUCFrameID();
    TIZEN_SECURE_LOGI ("[UCPROXY] url [%s]", url.c_str());
    TIZEN_LOGI ("[UCPROXY] httpMethod [%s], frameId [%d], isMainFrameRequest [%d]", httpMethod.c_str(), frameId, request.isMainFrameRequest());
    m_proxyRequest = context->createProxyURLRequest(url, (ProxyURLRequestDelegate *)this);
    m_proxyRequest->setHTTPMethod(httpMethod); /// if it is POST, should set HTTP method explict
    m_proxyRequest->setMainRequest(request.isMainFrameRequest()); ///< set true if it is a main request(HTML document request)
    m_proxyRequest->setFrameId(frameId); ///< id respresent a webview instance

    const HTTPHeaderMap& requestHeaderFields = request.httpHeaderFields();
    if (requestHeaderFields.size() > 0) {
        HTTPHeaderMap::const_iterator end = requestHeaderFields.end();
        for (HTTPHeaderMap::const_iterator it = requestHeaderFields.begin(); it != end; ++it) {
            if (it->first != "Cookie")
                m_proxyRequest->setHTTPHeaderField(std::string((it->first).string().utf8().data()),
                    std::string((it->second).utf8().data()));
        }
    }

    if (httpBody && !httpBody->isEmpty()) {
        size_t numElements = httpBody->elements().size();
        for (size_t i = 0; i < numElements; i++) {
            const FormDataElement& element = httpBody->elements()[i];

            if (element.m_type == FormDataElement::data) {
               totalBodySize += element.m_data.size();
               builder.append(element.m_data.data(), element.m_data.size());
               continue;
           }
           if (element.m_type == FormDataElement::encodedFile) {
               stopUC(false);
               if (this->start(d->m_context.get())) {
                   //TO DO -- Do we need to do soemthing here ??
               }
               return true;
           }
        }
        TIZEN_SECURE_LOGI("[UCPROXY] POST builder.toString().utf8().data() [%s]", builder.toString().utf8().data());
        TIZEN_SECURE_LOGI("[UCPROXY] POST std::string(builder.toString().utf8().data()) [%s]", std::string(builder.toString().utf8().data()).c_str());
        m_proxyRequest->setHTTPBody(std::string(builder.toString().utf8().data()));
    }

    if ((request.httpMethod() == "POST" || request.httpMethod() == "PUT")
        && (!request.httpBody() || request.httpBody()->isEmpty())) {
           m_proxyRequest->setHTTPHeaderField(std::string("Content-Length"), std::string("0"));
    }

    Error err = (Error) m_proxyRequest->start();
    if (OK != err) {
    TIZEN_SECURE_LOGI("[UCPROXY] return value of m_proxyRequest->start() [%d], url [%s]", err, url.c_str());
        stopUC(false);
        this->startRedirectRequestFromUC(request);
    }

    return true;
}

//Commented By UC: centralized the cancel and stop UC in a function to make the code more readable and easy to debug.
void ResourceHandle::stopUC(bool cancelNetwork)
{
    if (m_proxyRequest) {
        if (cancelNetwork) {
            m_proxyRequest->cancel();
        }
        else
        {
            m_proxyRequest->close();
        }
        delete m_proxyRequest;
        m_proxyRequest = NULL;
    }
    //Cache push entry transaction has to be terminated
    if (m_cachePushEntry) {
        m_cachePushEntry->cachePushCancel();
        delete m_cachePushEntry;
        m_cachePushEntry = NULL;
    }
}

void ResourceHandle::cancelUC()
{
    std::string url(this->firstRequest().url().string().utf8().data());
    TIZEN_SECURE_LOGI ("[UCPROXY] cancelled url [%s]", url.c_str());
    stopUC(true);
}

/* TO DO -- Need to delete m_proxyRequest in ResourceHandle's destructor, which is now in ResourceHandleSoupTizen.cpp
ResourceHandle::~ResourceHandle()
{

    //Commented By UC: when the ResourceHandle is deleting. m_proxyRequest should be closed or cancel properly.

    stopUC(false);

}
*/

void ResourceHandle::onResponseReceived(const proxy::ProxyURLResponse& proxy_response)
{
    ResourceHandleInternal* d = this->getInternal();
    ResourceHandleClient* client = this->client();

    if (d->m_cancelled || !client) {
        TIZEN_LOGI ("[UCPROXY] d->m_cancelled || !client return!!");
        return;
    }

    //Handling Redirection

    ResourceResponse response;

    KURL url(KURL(), String::fromUTF8(proxy_response.getUrl().c_str()));
    response.setURL(url);
    int status_code = proxy_response.httpStatusCode();
    TIZEN_SECURE_LOGI ("[UCPROXY] proxy response for url [%s]", url.string().utf8().data());
    TIZEN_SECURE_LOGI ("[UCPROXY] proxy_response.httpStatusCode() [%d]", proxy_response.httpStatusCode());

    if(status_code == 401 || status_code == 407 || status_code == 500) {

        //Commented By UC: if don't want any proxy loading, use cancel proxy network.
        stopUC(true);

        if (this->start(d->m_context.get())) {
            //TO DO -- Do we need to do soemthing here ??
        }
        return;
    }
    response.setHTTPStatusCode(status_code);

       // updateFromSoupMessage could be called several times for the same ResourceResponse object,
    // thus, we need to clear old header values and update m_httpHeaderFields from soupMessage headers.
    response.clearHTTPHeaderFields();

    const proxy::HTTPHeaderMap& headers = proxy_response.getAllHttpHeaderFields();
    String existingHeaderValue;
    // if (!headers.empty()) {
        TIZEN_SECURE_LOGI ("[UCPROXY] ****** RESPONSE HEADERS ******* START");
        for (proxy::HTTPHeaderMap::const_iterator it = headers.begin(); it != headers.end(); ++it )
        {
            String headerNameString = String::fromUTF8WithLatin1Fallback((it->first).c_str(), strlen((it->first).c_str()));
            String headerValueString = String::fromUTF8WithLatin1Fallback((it->second).c_str(), strlen((it->second).c_str()));

            existingHeaderValue = response.httpHeaderField(headerNameString);
            if(!existingHeaderValue.isEmpty()) {
                StringBuilder builder;
                        builder.append(existingHeaderValue);
                        builder.append(", ");
                        builder.append(headerValueString);
                response.setHTTPHeaderField(headerNameString, builder.toString());
                TIZEN_SECURE_LOGI ("[UCPROXY] [%s]: [%s]", headerNameString.utf8().data(), builder.toString().utf8().data());
            } else {
                response.setHTTPHeaderField(headerNameString, headerValueString);
                TIZEN_SECURE_LOGI ("[UCPROXY] [%s]: [%s]", headerNameString.utf8().data(), headerValueString.utf8().data());
            }
        }
        TIZEN_SECURE_LOGI ("[UCPROXY] ****** RESPONSE HEADERS ******* END");
    // }
    //TODO -- check if we need to get sniffedContentType here
    response.setMimeType(String::fromUTF8(proxy_response.mimeType().c_str()));
    response.setTextEncodingName(String::fromUTF8(proxy_response.textEncodingName().c_str()));

    response.setExpectedContentLength(proxy_response.expectedContentLength());
    response.setSuggestedFilename(String::fromUTF8(proxy_response.suggestedFilename().c_str()));
    //Cache Push
    if (status_code == 200) {
        String compressed = response.httpHeaderField("Content-Length");
        String original = response.httpHeaderField("x-orisize");
        if (original.isEmpty())
            original = compressed;
        if (!compressed.isEmpty()) {
            unsigned int orig = (unsigned int) atol(original.utf8().data());
            unsigned int comp = (unsigned int) atol(compressed.utf8().data());
            if (orig > 0 && comp > 0)
                compressionProxyLocalDataSizeSet (orig, comp);
        }
        m_cachePushEntry = new HTTPCachePushEntry();
    }
    if (m_cachePushEntry && !m_cachePushEntry->cachePushStart((HTTPURLRequest& )*m_proxyRequest, (HTTPURLResponse& )proxy_response)) {
        delete m_cachePushEntry;
        m_cachePushEntry = NULL;
    }

    d->m_response = response;
    client->didReceiveResponse (this, d->m_response);

    //TODO -- If needed, we should add willsendRequest code here
}

void ResourceHandle::onDataReceived(const char* data, int length)
{
    ResourceHandleInternal* d = this->getInternal();
    ResourceHandleClient* client = this->client();
    ResourceRequest request = this->firstRequest();
    std::string url (request.url().string().utf8().data());

    if (d->m_cancelled || !client) {
        TIZEN_SECURE_LOGI ("[UCPROXY] d->m_cancelled || !client return!! [%s]", url.c_str());
        return;
    }

    if (!length) {
        TIZEN_SECURE_LOGI ("[UCPROXY] NOLENGTH!!! data [%p], length [%d], [%s]", data, length, url.c_str());
        client->didFinishLoading(this, 0);
        this->setClient(0);
        return;
   }

    TIZEN_SECURE_LOGI ("[UCPROXY] data [%p], length [%d], [%s]", data, length, url.c_str());
    //Cache Push
    if (m_cachePushEntry) {
        m_cachePushEntry->cachePushSendData(data, length);
    }

    client->didReceiveData (this, data, length, length);
    if (d->m_cancelled || !client)
        return;
}

void ResourceHandle::onFinishLoading()
{
    ResourceHandleInternal* d = this->getInternal();
    ResourceHandleClient* client = this->client();
    ResourceRequest request = this->firstRequest();
    std::string url (request.url().string().utf8().data());

    if (d->m_cancelled || !client) {
        TIZEN_SECURE_LOGI ("[UCPROXY] d->m_cancelled || !client return!! [%s]", url.c_str());
        return;
    }
    TIZEN_SECURE_LOGI ("[UCPROXY] onFinishLoading [%s]", url.c_str());
    //TO DO -- If needed, error handling code should be added

    //Cache Push
    if (m_cachePushEntry) {
        m_cachePushEntry->cachePushFinished();
        delete m_cachePushEntry;
        m_cachePushEntry = NULL;
    }
    stopUC(false);
    client->didFinishLoading(this, 0);
}

void ResourceHandle::onWillSendRequest(proxy::ProxyURLRequest& request)
{
    TIZEN_SECURE_LOGI ("[UCPROXY] onWillSendRequest url [%s]", request.getUrl().c_str());
}

static bool shouldRedirectAsGET(int status_code,const String& method, KURL& newURL, bool crossOrigin)
{
    if(method == "HEAD")
        return false;

    if (!newURL.protocolIsInHTTPFamily())
        return true;

    switch (status_code) {
    case 303:
        return true;
    case 302:
    case 301:
        if (method == "POST")
            return true;
        break;
    }

    if (crossOrigin && method == "DELETE")
        return true;

    return false;
}

void ResourceHandle::onRedirectReceived (const proxy::ProxyURLResponse& redirectResponse)
{
    ResourceHandleInternal* d = this->getInternal();
    static const int maxRedirects = 20;

    if (d->m_cancelled)
        return;
    int status_code = redirectResponse.httpStatusCode();
    if (status_code == 300 || status_code == 304 || status_code == 305 || status_code == 306)
        return;
    const char* redirect_uri = redirectResponse.httpHeaderField(std::string("location")).c_str();

    TIZEN_SECURE_LOGI ("[UCPROXY] redirected location [%s]", redirect_uri);
    String location = String::fromUTF8(redirect_uri);
#if ENABLE(TIZEN_REDIRECTED_LOCATION_IS_NOT_UTF_8)
    if (location.isEmpty())
        location = String(redirectResponse.httpHeaderField(string("location")).c_str());
#endif
    KURL newURL = KURL(((ResourceLoader*)this->client())->url(), location);

    ResourceRequest request = firstRequest();
    request.setURL(newURL);
    bool crossOrigin = !protocolHostAndPortAreEqual(this->firstRequest().url(), newURL);
    request.setFirstPartyForCookies(newURL);
    if (request.httpMethod() != "GET") {
        // Change newRequest method to GET if change was made during a previous redirection
        // or if current redirection says so
        if (shouldRedirectAsGET(status_code, request.httpMethod(), newURL, crossOrigin)) {
            request.setHTTPMethod("GET");
            request.setHTTPBody(0);
            request.clearHTTPContentType();
        }
    }

    // Should not set Referer after a redirect from a secure resource to non-secure one.
    if (!newURL.protocolIs("https") && protocolIs(request.httpReferrer(), "https"))
        request.clearHTTPReferrer();

    d->m_user = newURL.user();
    d->m_pass = newURL.pass();
    request.removeCredentials();

    if (crossOrigin) {
        // If the network layer carries over authentication headers from the original request
        // in a cross-origin redirect, we want to clear those headers here.
        request.clearHTTPAuthorization();

        // TODO: We are losing any username and password specified in the redirect URL, as this is the
        // same behavior as the CFNet port. We should investigate if this is really what we want.
    } else
        applyAuthenticationToRequest(this, request, true);

//START d->m_response should be updated with redirected response
ResourceResponse response;
// updateFromSoupMessage could be called several times for the same ResourceResponse object,
// thus, we need to clear old header values and update m_httpHeaderFields from soupMessage headers.
response.setHTTPStatusCode(status_code);
response.clearHTTPHeaderFields();

 const proxy::HTTPHeaderMap& headers = redirectResponse.getAllHttpHeaderFields();
 String existingHeaderValue;
 // if (!headers.empty()) {
 for (proxy::HTTPHeaderMap::const_iterator it = headers.begin(); it != headers.end(); ++it )
 {
         String headerNameString = String::fromUTF8WithLatin1Fallback((it->first).c_str(), strlen((it->first).c_str()));
         String headerValueString = String::fromUTF8WithLatin1Fallback((it->second).c_str(), strlen((it->second).c_str()));

     existingHeaderValue = response.httpHeaderField(headerNameString);
     if(!existingHeaderValue.isEmpty()) {
     StringBuilder builder;
             builder.append(existingHeaderValue);
             builder.append(", ");
             builder.append(headerValueString);
     response.setHTTPHeaderField(headerNameString, builder.toString());
     }else{
     response.setHTTPHeaderField(headerNameString, headerValueString);
     }
 }
 // }
 //TODO -- check if we need to get sniffedContentType here
 response.setMimeType(String::fromUTF8(redirectResponse.mimeType().c_str()));
 response.setTextEncodingName(String::fromUTF8(redirectResponse.textEncodingName().c_str()));

 response.setExpectedContentLength(redirectResponse.expectedContentLength());
 response.setSuggestedFilename(String::fromUTF8(redirectResponse.suggestedFilename().c_str()));

 d->m_response = response;
 //END -- d->m_response should be updated with redirected response

    if (d->client())
#if 0 //ENABLE(TIZEN_RESTARTED_CALLBACK_BUG_FIX)
        d->client()->willSendRequest(this.get(), request, d->m_response);
#else
        d->client()->willSendRequest(this, request, d->m_response);
#endif

    if (d->m_cancelled)
        return;

#if ENABLE(WEB_TIMING)
    d->m_response.setResourceLoadTiming(ResourceLoadTiming::create());
    d->m_response.resourceLoadTiming()->requestTime = monotonicallyIncreasingTime();
#endif
#if 0
    // Update the first party in case the base URL changed with the redirect
    String firstPartyString = request.firstPartyForCookies().string();
   /* if (!firstPartyString.isEmpty()) {
        GOwnPtr<SoupURI> firstParty(soup_uri_new(firstPartyString.utf8().data()));
        soup_message_set_first_party(d->m_soupMessage.get(), firstParty.get());
    } TODO -- NO API TO UPDATE FIRST PARTY IN PROXYURLRESPONSE.H */
 #endif

    if (d->m_redirectCount++ > maxRedirects) {
        ResourceError resourceError(String("soup_http_error_quark"), SOUP_STATUS_TOO_MANY_REDIRECTS, firstRequest().url().string(), String("Too many redirects"));
        stopUC(false);
        d->client()->didFail (this, resourceError);
        return;
    }

    if (newURL.protocolIs("http")  && !is_uri_blacklisted_for_UCproxy(newURL.host().utf8().data()) && !is_site_local(newURL.host().utf8().data())) {
        stopUC(false);
        this->startUC(d->m_context.get(), request);
    } else {
        TIZEN_SECURE_LOGI ("[UCPROXY] BYPASS url [%s], isMainRequest() [%d], isHttps() [%d], is_site_local [%d], is_blacklisted [%d]", request.url().string().utf8().data(), m_proxyRequest->isMainRequest(), newURL.protocolIs("https"), is_site_local(newURL.host().utf8().data()), is_uri_blacklisted_for_UCproxy(newURL.host().utf8().data()));
        if (m_proxyRequest->isMainRequest())
            this->client()->ucBypassSet(true);
        stopUC(false);
        this->startRedirectRequestFromUC(request);
   }
}

void ResourceHandle::onRequestRejected(const proxy::ProxyURLRequest& request, proxy::RejectReasonCode reason)
{
    std::string url (request.getUrl().c_str());
    TIZEN_SECURE_LOGI ("[UCPROXY] onRequestRejected url [%s], reason: [%d]", url.c_str(), reason);

    //Commented By UC: handle the reject reasons
     if (reason == proxyServerDeny || reason == resourceNotLoad) {
         TIZEN_SECURE_LOGI ("[UCPROXY] BYPASS url [%s] reason [%d] isMainRequest() [%d]", url.c_str(), reason, request.isMainRequest());
        if (request.isMainRequest())
            this->client()->ucBypassSet(true); //Resource Handle by pass happens
        stopUC(false);
        this->start(d->m_context.get()); //FIX ME -- Do we need to do soemthing with return value ??
    } else if (reason == protocolError) {
         TIZEN_SECURE_LOGI ("[UCPROXY] BYPASS url [%s] reason [%d] isMainRequest() [%d]", url.c_str(), reason, request.isMainRequest());
        if (request.isMainRequest())
            this->client()->ucBypassSet(true); //Resource Handle by pass happens
        stopUC(true);
        if (this->start(d->m_context.get())) {
            //FIX ME -- Do we need to do something with return value here ??
        }
    } else if (reason == resourceInMemCache || reason == resourceInDiskCache) {
         TIZEN_SECURE_LOGI ("[UCPROXY] BYPASS url [%s] reason [%d] isMainRequest() [%d]", url.c_str(), reason, request.isMainRequest());
        if (request.isMainRequest())
            this->client()->ucBypassSet(true); //Resource Handle by pass happens
        stopUC(false);
        //Commented By UC: set the cache reading policy. need to force use the cache.
         //TO DO -- read the cache and response to webkit
        if (reason == resourceInDiskCache) {
            this->firstRequest().setHTTPHeaderField("Cache-Control", "max-stale=86400");
            this->start(d->m_context.get());
        }
    }

}


void ResourceHandle::onDownloadResponseReceived(const proxy::ProxyURLResponse& response)
{
    std::string url (m_proxyRequest->getUrl().c_str());
    TIZEN_SECURE_LOGI ("[UCPROXY] BYPASS url [%s] isMainRequest() [%d]", url.c_str(), m_proxyRequest->isMainRequest());
    if (m_proxyRequest->isMainRequest())
        this->client()->ucBypassSet(true); //Resource Handle by pass happens
    stopUC(false);
    if (this->start(d->m_context.get())) {
        //TO DO -- Do we need to do soemthing here ??
    }
}

void ResourceHandle::onError(const std::string& errorDomain, int errorCode, const std::string& errorDescription, const std::string& errorURL)
{
    ResourceHandleInternal* d = this->getInternal();
    std::string url (m_proxyRequest->getUrl().c_str());
    bool isMainRequest = m_proxyRequest->isMainRequest();

    //Cache Push
    if (m_cachePushEntry) {
        m_cachePushEntry->cachePushCancel();
        delete m_cachePushEntry;
        m_cachePushEntry = NULL;
    }

    TIZEN_SECURE_LOGI ("[UCPROXY] errorDomain [%s], errorCode [%d], errorDesciption [%s], errorURL [%s]", errorDomain.c_str(), errorCode, errorDescription.c_str(), errorURL.c_str());
    if (d->client()) {
        ResourceError resourceError(String(errorDomain.c_str()), errorCode, String(errorURL.c_str()), String(errorDescription.c_str()));
        stopUC(false);
        if ((!strcmp (errorDomain.c_str(), "UCProxy") && (errorCode == -21)) || (!strcmp (errorDomain.c_str(), "soup_http_error_quark") && (errorCode == 4))) {
            //TO DO -- Notify browser app about Proxy Server is down
            //ResourceHandle::setCompressionProxyEnabled(false);
            if(isMainRequest) {
                SoupURI * uri = soup_uri_new (errorURL.c_str());
                GHashTable * proxy_failure = ResourceHandle::get_proxy_failure_table();
                TIZEN_SECURE_LOGI ("[UCPROXY] proxy_failure %p, uri %p", proxy_failure, uri);
                guint32 count = (guint32) g_hash_table_lookup(proxy_failure, GUINT_TO_POINTER(g_str_hash(soup_uri_get_host (uri))) );
                count++;
                TIZEN_SECURE_LOGI ("[UCPROXY] count %d hash size %d", count, g_hash_table_size(proxy_failure));
                if(count >= 3 || g_hash_table_size(proxy_failure) >= 3) {
                    g_hash_table_destroy(proxy_failure);
                    ResourceHandle::set_proxy_failure_table(NULL);
                    ResourceHandle::setCompressionProxyEnabled(false);
                    TIZEN_LOGI ("[UCPROXY] !!!! TURN OFF PROXY !!!!");
                }
                else {
                    g_hash_table_insert (proxy_failure, GUINT_TO_POINTER(g_str_hash(soup_uri_get_host (uri))), (gpointer)count);
                    TIZEN_SECURE_LOGI ("[UCPROXY] inserted count %d for domain %s", count, soup_uri_get_host (uri));
                }
                if(uri)
                    soup_uri_free(uri);
                this->client()->ucBypassSet(true); //Resource Handle by pass happens
             }
             TIZEN_LOGI ("[UCPROXY] BYPASS PROXY!!");
             if (this->start(d->m_context.get())) {
                    //FIX ME -- Do we need to do something with return value here ??
             }
            return;
        }
        if ((!strcmp (errorDomain.c_str(), "UCProxy") && ((errorCode == -19) || (errorCode == -17))) || (!strcmp (errorDomain.c_str(), "soup_http_error_quark") && (errorCode == 7))) {
            TIZEN_SECURE_LOGI ("[UCPROXY] BYPASS url [%s] isMainRequest() [%d]", url.c_str(), isMainRequest);
            if (isMainRequest)
                this->client()->ucBypassSet(true); //Resource Handle by pass happens
            if (this->start(d->m_context.get())) {
                //TO DO -- Do we need to do soemthing here ??
            }
            return;
        }
        d->client()->didFail(this, resourceError);
    }
}

void ResourceHandle::setCompressionProxyEnabled (bool enabled)
{
    if (m_compressionProxyEnabled == enabled)
        return;
    TIZEN_LOGI ("[UCPROXY] proxy enabled [%d]", enabled);
    m_compressionProxyEnabled = enabled;
    if (enabled)
        ensureProxyRequestContext();
    else {
        ProxyRequestContext* context = getProxyRequestContext();
        if (context)
            context->close();
    }
}

unsigned long int ResourceHandle::m_localOriginalSize = 0;
unsigned long int ResourceHandle::m_localCompressedSize = 0;
unsigned long int ResourceHandle::m_originalSize = 0;
unsigned long int ResourceHandle::m_compressedSize = 0;

void ResourceHandle::compressionProxyDataSizeGet (unsigned int& original, unsigned int& compressed)
{
#if DATA_SAVINGS_VALUES_IN_BYTES
    if (m_originalSize && m_compressedSize) {
        original = (unsigned int) (m_originalSize);
        compressed = (unsigned int) (m_compressedSize);
        TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION original: [%u] Bytes, compressed: [%u] Bytes", original, compressed);
    } else {
        original = (unsigned int) (m_localOriginalSize);
        compressed = (unsigned int) (m_localCompressedSize);
        TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION original: [%u] Bytes, compressed: [%u] Bytes", original, compressed);
    }
#else
    if (m_originalSize && m_compressedSize) {
        original = (unsigned int) (m_originalSize / 1024);
        compressed = (unsigned int) (m_compressedSize / 1024);
        TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION original: [%u] KB, compressed: [%u] KB", original, compressed);
    } else {
        original = (unsigned int) (m_localOriginalSize / 1024);
        compressed = (unsigned int) (m_localCompressedSize / 1024);
        TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION original: [%u] KB, compressed: [%u] KB", original, compressed);
    }
#endif
    m_originalSize = 0;
    m_compressedSize = 0;
    m_localOriginalSize = 0;
    m_localCompressedSize = 0;
}

void ResourceHandle::compressionProxyDataSizeSet (unsigned int original, unsigned int compressed)
{
#if DATA_SAVINGS_VALUES_IN_BYTES
    if ((m_originalSize + original) >= UINT_MAX || (!original && !compressed)) {
#else
    if ((m_originalSize + original) >= ULONG_MAX || (!original && !compressed)) {
#endif
        m_originalSize = 0;
        m_compressedSize = 0;
        TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION RESET m_originalSize, m_compressedSize");
        compressionProxyDataSizeDump();
    } else if (original && compressed) {
        m_originalSize = m_originalSize + original;
        m_compressedSize = m_compressedSize + compressed;
        TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION m_originalSize [%lu], m_compressedSize [%lu]", m_originalSize, m_compressedSize);
    }
}

void ResourceHandle::compressionProxyLocalDataSizeSet (unsigned int original, unsigned int compressed)
{
#if DATA_SAVINGS_VALUES_IN_BYTES
    if ((m_localOriginalSize + original) >= UINT_MAX || (!original && !compressed)) {
#else
    if ((m_localOriginalSize + original) >= ULONG_MAX || (!original && !compressed)) {
#endif
        m_localOriginalSize = 0;
        m_localCompressedSize = 0;
        TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION RESET m_localOriginalSize, m_localCompressedSize");
        compressionProxyDataSizeDump();
    } else if (original && compressed) {
        m_localOriginalSize = m_localOriginalSize + original;
        m_localCompressedSize = m_localCompressedSize + compressed;
        TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION m_localOriginalSize [%lu], m_localCompressedSize [%lu]", m_localOriginalSize, m_localCompressedSize);
    }
}


void ResourceHandle::compressionProxyImageQualitySet(int quality)
{
    ProxyRequestContext* context = getProxyRequestContext();
    if (context) {
        TIZEN_LOGI ("[UCPROXY] COMPRESSION ImageQualityLevel [%d]", quality);
        context->setImageQualityLevel((ImageQualityLevel) quality);
    }
}

String ResourceHandle::m_compressionDataSizeFilePath = String();

void ResourceHandle::compressionProxyDataSizeFilePathSet(const String& soupDataDirectory)
{
    if (!soupDataDirectory.isEmpty()) {
        String compressionDataSizeFilePath = WebCore::pathByAppendingComponent(soupDataDirectory, "compressionDataSize.txt");
        TIZEN_SECURE_LOGI ("[UCPROXY] Compression Proxy data size file path set to [%s]", compressionDataSizeFilePath.utf8().data());
        m_compressionDataSizeFilePath = String(compressionDataSizeFilePath.utf8().data());
    }
 }

String ResourceHandle::compressionProxyDataSizeFilePathGet()
{
    return m_compressionDataSizeFilePath;
}


void ResourceHandle::compressionProxyDataSizeLoad()
{
    String filePath = compressionProxyDataSizeFilePathGet();
    if (!filePath.isEmpty()) {
        FILE *fp = fopen (filePath.utf8().data(), "r");
        if (fp) {
            unsigned long int original;
            unsigned long int compressed;
            fscanf(fp, "%lu %lu", &original, &compressed);
            TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION original [%lu], compressed [%lu]", original, compressed);
            m_originalSize = original;
            m_compressedSize = compressed;
            fclose(fp);
            fp = NULL;
        }
    }
}

void ResourceHandle::compressionProxyDataSizeDump()
{
    String filePath = compressionProxyDataSizeFilePathGet();
    if (!filePath.isEmpty()) {
        FILE *fp = fopen (filePath.utf8().data(), "w");
        if (fp) {
            TIZEN_SECURE_LOGI ("[UCPROXY] COMPRESSION m_originalSize [%lu], m_compressedSize [%lu]", m_originalSize, m_compressedSize);
            fprintf(fp, "%lu %lu", m_originalSize, m_compressedSize);
            fclose(fp);
            fp = NULL;
        }
    }
}

void ResourceHandle::attachSoupCacheToHTTPCacheQuerier()
{
    ProxyRequestContext *context = ResourceHandle::getProxyRequestContext();
    if (context)
        ((HTTPCacheQuerierImpl *)context->getHTTPCacheQuerier())->attachSoupCache();
}

void ResourceHandle::detachSoupCacheFromHTTPCacheQuerier()
{
    ProxyRequestContext *context = ResourceHandle::getProxyRequestContext();
    if (context)
        ((HTTPCacheQuerierImpl *)context->getHTTPCacheQuerier())->detachSoupCache();
}

//#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
static void applyAuthenticationToRequest(ResourceHandle* handle, ResourceRequest& request, bool redirect)
{
    // m_user/m_pass are credentials given manually, for instance, by the arguments passed to XMLHttpRequest.open().
    ResourceHandleInternal* d = handle->getInternal();

    if (handle->shouldUseCredentialStorage()) {
        if (d->m_user.isEmpty() && d->m_pass.isEmpty())
            d->m_initialCredential = CredentialStorage::get(request.url());
        else if (!redirect) {
            // If there is already a protection space known for the URL, update stored credentials
            // before sending a request. This makes it possible to implement logout by sending an
            // XMLHttpRequest with known incorrect credentials, and aborting it immediately (so that
            // an authentication dialog doesn't pop up).
            CredentialStorage::set(Credential(d->m_user, d->m_pass, CredentialPersistenceNone), request.url());
        }
    }

    String user = d->m_user;
    String password = d->m_pass;
    if (!d->m_initialCredential.isEmpty()) {
        user = d->m_initialCredential.user();
        password = d->m_initialCredential.password();
    }

    if (user.isEmpty() && password.isEmpty())
        return;

    // We always put the credentials into the URL. In the CFNetwork-port HTTP family credentials are applied in
    // the didReceiveAuthenticationChallenge callback, but libsoup requires us to use this method to override
    // any previously remembered credentials. It has its own per-session credential storage.
    KURL urlWithCredentials(request.url());
    urlWithCredentials.setUser(user);
    urlWithCredentials.setPass(password);
    request.setURL(urlWithCredentials);
}
//#endif
}
#endif
