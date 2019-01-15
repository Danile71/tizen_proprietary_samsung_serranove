/*
   Copyright (C) 2013 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ewk_intercept_request.h"

#if PLATFORM(TIZEN)
#include "HTTPParsers.h"
#include "WKAPICast.h"
#include "WKFrame.h"
#include "WKFramePolicyListener.h"
#include "WKRetainPtr.h"
#include "WKURL.h"
#include "WKURLRequest.h"
#include "WKURLResponseTizen.h"
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>

using namespace WebKit;

#if ENABLE(TIZEN_INTERCEPT_REQUEST)

struct _Ewk_Intercept_Request {
    const char* url;
    const char* method;
    const char* headers;
    const char* requestBody;
    Eina_Hash* requestHeaders;
    Eina_Bool interceptResponse;
    uint64_t listenerID;
    WKFrameRef frame;
    WKPageRef page;
    bool isDecided;
};

static void freeRequestHeaders(void* data)
{
    EINA_SAFETY_ON_NULL_RETURN(data);
    eina_stringshare_del(static_cast<char*>(data));
}

Ewk_Intercept_Request* ewkInterceptDecisionCreate(WKURLRequestRef request, WKPageRef page, WKFrameRef frame, WKStringRef body, uint64_t listenerID)
{
    Ewk_Intercept_Request* interceptRequest = new Ewk_Intercept_Request;
    interceptRequest->url = eina_stringshare_add(toImpl(request)->resourceRequest().url().string().utf8().data());
    interceptRequest->method = eina_stringshare_add(toImpl(request)->resourceRequest().httpMethod().utf8().data());
    interceptRequest->requestBody = eina_stringshare_add(toImpl(body)->string().utf8().data());
    interceptRequest->page = page;
    interceptRequest->frame = frame;
    interceptRequest->listenerID = listenerID;
    interceptRequest->isDecided = false;
    interceptRequest->requestHeaders = 0;

    if (request) {
        interceptRequest->requestHeaders = eina_hash_string_small_new(freeRequestHeaders);
        WebCore::HTTPHeaderMap map = toImpl(request)->resourceRequest().httpHeaderFields();
        WebCore::HTTPHeaderMap::const_iterator end = map.end();
        for (WebCore::HTTPHeaderMap::const_iterator it = map.begin(); it != end; ++it) {
            String key = it->first;
            String value = it->second;
            eina_hash_add(interceptRequest->requestHeaders, key.utf8().data(), eina_stringshare_add(value.utf8().data()));
        }
    }

    return interceptRequest;
}

void ewkInterceptRequestDelete(Ewk_Intercept_Request* interceptRequest)
{
    EINA_SAFETY_ON_NULL_RETURN(interceptRequest);

    if (interceptRequest->url)
        eina_stringshare_del(interceptRequest->url);
    if (interceptRequest->method)
        eina_stringshare_del(interceptRequest->method);
    if (interceptRequest->requestBody)
        eina_stringshare_del(interceptRequest->requestBody);
    if (interceptRequest->requestHeaders)
        eina_hash_free(interceptRequest->requestHeaders);

    delete interceptRequest;
}

bool ewkInterceptRequestDecided(Ewk_Intercept_Request* interceptRequest)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(interceptRequest, false);
    return interceptRequest->isDecided;
}

Eina_Bool ewk_intercept_request_ignore(Ewk_Intercept_Request* interceptRequest)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(interceptRequest, false);
    interceptRequest->isDecided = true;
    toImpl(interceptRequest->frame)->receivedInterceptResponseIgnore(interceptRequest->listenerID);
    return true;
}

const char* ewk_intercept_request_url_get (Ewk_Intercept_Request* interceptRequest)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(interceptRequest, 0);

    return interceptRequest->url;
}

const char* ewk_intercept_request_http_method_get (Ewk_Intercept_Request* interceptRequest)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(interceptRequest, 0);

    return interceptRequest->method;
}

const char* ewk_intercept_request_body_get (Ewk_Intercept_Request* interceptRequest)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(interceptRequest, 0);

    return interceptRequest->requestBody;
}

const Eina_Hash* ewk_intercept_request_headers_get (Ewk_Intercept_Request* interceptRequest)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(interceptRequest, 0);

    return interceptRequest->requestHeaders;
}

Eina_Bool ewk_intercept_request_response_set (Ewk_Intercept_Request* interceptRequest, const char* headers, const char* body, int length)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(interceptRequest, false);
    WKRetainPtr<WKStringRef> headersRef(AdoptWK, WKStringCreateWithUTF8CString(headers));
    WKRetainPtr<WKDataRef> bodyRef(AdoptWK, WKDataCreate(reinterpret_cast<const unsigned char*>(body), length));
    toImpl(interceptRequest->frame)->receivedInterceptResponseSet(interceptRequest->listenerID, toImpl(headersRef.get()), toImpl(bodyRef.get()));
    ewkInterceptRequestDelete(interceptRequest);

    return true;
}

#endif
#endif // #if PLATFORM(TIZEN)
