/*
   Copyright (C) 2012 Samsung Electronics

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
#include "ewk_policy_decision.h"

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
#include <libsoup/soup.h>

using namespace WebKit;

struct _Ewk_Policy_Decision {
    WKFramePolicyListenerRef listener;
    WKRetainPtr<WKURLRef> requestUrl;
    Evas_Object* ewkView;

    const char* cookie;
    const char* url;
    const char* host;
    const char* scheme;
    const char* responseMime;
    Eina_Hash* responseHeaders;
    Ewk_Policy_Decision_Type decisionType;
    Ewk_Policy_Navigation_Type navigationType;
    Ewk_Policy_Callback_Type callbackType;
    WKFrameRef frame;

    bool isDecided;
    bool isSuspended;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    bool isAttachment;
#endif
#if ENABLE(TIZEN_ON_REDIRECTION_REQUESTED)
    int responseStatusCode;
#endif
#if ENABLE(TIZEN_POLICY_DECISION_HTTP_METHOD_GET)
    const char* httpMethod;
#endif
#if ENABLE(TIZEN_POLICY_DECISION_HTTP_BODY_GET)
    const char* httpBody;
#endif
    const char* AuthUser;
    const char* AuthPassword;
};

static void freeResponseHeaders(void* data)
{
    EINA_SAFETY_ON_NULL_RETURN(data);
    eina_stringshare_del(static_cast<char*>(data));
}

void setAuthorizationIfNecessary(Ewk_Policy_Decision* policyDecision, WKURLRequestRef request)
{
    String auth = toImpl(request)->resourceRequest().httpHeaderField("Authorization");
    if (auth.isEmpty()) {
        policyDecision->AuthUser = 0;
        policyDecision->AuthPassword = 0;
        return;
    }

    Vector<String> split;
    auth.split(' ', split);

    if (!equalIgnoringCase(split[0], "Basic")) {
        policyDecision->AuthUser = 0;
        policyDecision->AuthPassword = 0;
        return;
    }

    Vector<char> decoded;
    base64Decode(split[1], decoded);

    String decodedString(decoded.begin(), decoded.size());

    Vector<String> decodedSplit;
    decodedString.split(':', decodedSplit);

    if (decodedSplit.size() < 2 || decodedSplit[0].isEmpty() || decodedSplit[1].isEmpty()) {
        policyDecision->AuthUser = 0;
        policyDecision->AuthPassword = 0;
    } else {
        policyDecision->AuthUser = eina_stringshare_add(decodedSplit[0].utf8().data());
        policyDecision->AuthPassword = eina_stringshare_add(decodedSplit[1].utf8().data());
    }
}

Ewk_Policy_Decision* ewkPolicyDecisionCreate(Evas_Object* ewkView, Ewk_Policy_Callback_Type callbackType, WKFramePolicyListenerRef listener, WKURLRequestRef request, WKFrameRef frame, WKURLResponseRef response, WKFrameNavigationType navigationType)
{
    Ewk_Policy_Decision* policyDecision = new Ewk_Policy_Decision;
    policyDecision->ewkView = ewkView;
    policyDecision->listener = listener;
    policyDecision->requestUrl = adoptWK(WKURLRequestCopyURL(request));
    policyDecision->cookie = eina_stringshare_add(toImpl(request)->resourceRequest().httpHeaderField("Cookie").utf8().data());

    policyDecision->url = 0;
    policyDecision->host = 0;
    policyDecision->scheme = 0;
    policyDecision->responseMime = 0;
    policyDecision->responseHeaders = 0;

    policyDecision->decisionType = EWK_POLICY_DECISION_USE;
    policyDecision->navigationType = static_cast<Ewk_Policy_Navigation_Type>(navigationType);
    policyDecision->callbackType = callbackType;

    policyDecision->frame = frame;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    policyDecision->isAttachment = false;
#endif

#if ENABLE(TIZEN_POLICY_DECISION_HTTP_METHOD_GET)
    policyDecision->httpMethod = eina_stringshare_add(toImpl(request)->resourceRequest().httpMethod().utf8().data());
#endif

#if ENABLE(TIZEN_POLICY_DECISION_HTTP_BODY_GET)
    policyDecision->httpBody = eina_stringshare_add(toImpl(request)->resourceRequest().httpBody()
        ? toImpl(request)->resourceRequest().httpBody()->flattenToString().utf8().data() : 0);
#endif

    if (response) {
        policyDecision->responseMime = eina_stringshare_add(toImpl(response)->resourceResponse().mimeType().utf8().data());

        policyDecision->responseHeaders = eina_hash_string_small_new(freeResponseHeaders);
        WebCore::HTTPHeaderMap map = toImpl(response)->resourceResponse().httpHeaderFields();
        WebCore::HTTPHeaderMap::const_iterator end = map.end();
        for (WebCore::HTTPHeaderMap::const_iterator it = map.begin(); it != end; ++it) {
            String key = it->first;
            String value = it->second;
            eina_hash_add(policyDecision->responseHeaders, key.utf8().data(), eina_stringshare_add(value.utf8().data()));
        }

        WKRetainPtr<WKStringRef> contentType(AdoptWK, WKURLResponseEflCopyContentType(response));

        if (!contentType)
            policyDecision->decisionType = EWK_POLICY_DECISION_DOWNLOAD;
        else {
            if (WebCore::contentDispositionType(toImpl(response)->resourceResponse().httpHeaderField("Content-Disposition")) == WebCore::ContentDispositionAttachment)
                policyDecision->decisionType = EWK_POLICY_DECISION_DOWNLOAD;
            else if (frame && WKFrameCanShowMIMEType(frame, contentType.get()))
                policyDecision->decisionType = EWK_POLICY_DECISION_USE;
            else
                policyDecision->decisionType = EWK_POLICY_DECISION_DOWNLOAD;
        }

#if ENABLE(TIZEN_ON_REDIRECTION_REQUESTED)
        int httpStatusCode = toImpl(response)->resourceResponse().httpStatusCode();
        policyDecision->responseStatusCode = httpStatusCode;

        if (SOUP_STATUS_IS_REDIRECTION(httpStatusCode) && (httpStatusCode != SOUP_STATUS_NOT_MODIFIED))
            policyDecision->decisionType = EWK_POLICY_DECISION_USE;
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        String mimeType = toImpl(response)->resourceResponse().mimeType().lower();
        policyDecision->isAttachment = toImpl(response)->resourceResponse().isAttachment();

        if (frame && toImpl(frame)->isMediaMIMEType(mimeType) && !policyDecision->isAttachment)
            policyDecision->decisionType = EWK_POLICY_DECISION_USE;
#endif
    }

    policyDecision->isDecided = false;
    policyDecision->isSuspended = false;

    setAuthorizationIfNecessary(policyDecision, request);

    return policyDecision;
}

void ewkPolicyDecisionDelete(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN(policyDecision);

    if (policyDecision->cookie)
        eina_stringshare_del(policyDecision->cookie);
    if (policyDecision->url)
        eina_stringshare_del(policyDecision->url);
    if (policyDecision->host)
        eina_stringshare_del(policyDecision->host);
    if (policyDecision->scheme)
        eina_stringshare_del(policyDecision->scheme);
    if (policyDecision->responseMime)
        eina_stringshare_del(policyDecision->responseMime);
    if (policyDecision->responseHeaders)
        eina_hash_free(policyDecision->responseHeaders);
#if ENABLE(TIZEN_POLICY_DECISION_HTTP_METHOD_GET)
    if (policyDecision->httpMethod)
        eina_stringshare_del(policyDecision->httpMethod);
#endif
#if ENABLE(TIZEN_POLICY_DECISION_HTTP_BODY_GET)
    if (policyDecision->httpBody)
        eina_stringshare_del(policyDecision->httpBody);
#endif

    delete policyDecision;
}

bool ewkPolicyDecisionDecided(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, false);
    return policyDecision->isDecided;
}

bool ewkPolicyDecisionSuspended(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, false);
    return policyDecision->isSuspended;
}

const char* ewk_policy_decision_cookie_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);

    return policyDecision->cookie;
}

const char* ewk_policy_decision_userid_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);

    return policyDecision->AuthUser;
}

const char* ewk_policy_decision_password_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);

    return policyDecision->AuthPassword;
}

const char* ewk_policy_decision_url_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision->requestUrl, 0);

    if(!policyDecision->url)
        policyDecision->url = eina_stringshare_add(toImpl(policyDecision->requestUrl.get())->string().utf8().data());

    return policyDecision->url;
}

const char* ewk_policy_decision_scheme_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision->requestUrl, 0);

    if(!policyDecision->scheme)
        policyDecision->scheme = eina_stringshare_add(toImpl(policyDecision->requestUrl.get())->protocol().utf8().data());
    return policyDecision->scheme;
}

const char* ewk_policy_decision_host_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision->requestUrl, 0);

    if(!policyDecision->host)
        policyDecision->host = eina_stringshare_add(toImpl(policyDecision->requestUrl.get())->host().utf8().data());
    return policyDecision->host;
}

#if ENABLE(TIZEN_POLICY_DECISION_HTTP_METHOD_GET)
const char* ewk_policy_decision_http_method_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);

    return policyDecision->httpMethod;
}
#endif

Ewk_Policy_Decision_Type ewk_policy_decision_type_get(const Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, EWK_POLICY_DECISION_USE);

    return policyDecision->decisionType;
}

const char* ewk_policy_decision_response_mime_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);

    return policyDecision->responseMime;
}

const Eina_Hash* ewk_policy_decision_response_headers_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);

    return policyDecision->responseHeaders;
}

int ewk_policy_decision_response_status_code_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);

#if ENABLE(TIZEN_ON_REDIRECTION_REQUESTED)
    return policyDecision->responseStatusCode;
#else
    return 0;
#endif
}

Eina_Bool ewk_policy_decision_suspend(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision->requestUrl, false);

    policyDecision->isSuspended = true;
    return true;
}

Eina_Bool ewk_policy_decision_use(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, false);

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    const String mimeType(policyDecision->responseMime);
    if (policyDecision->callbackType == EWK_POLICY_CALLBACK_TYPE_RESPONSE && toImpl(policyDecision->frame)->isMediaMIMEType(mimeType.lower())) {
        const String url = toImpl(policyDecision->requestUrl.get())->string();
        const String cookie(policyDecision->cookie);
        ewkViewOpenMediaPlayer(policyDecision->ewkView, url, cookie, mimeType.lower());
        ewk_policy_decision_ignore(policyDecision);
        return true;
    }
#endif

    policyDecision->isDecided = true;
    WKFramePolicyListenerUse(policyDecision->listener);
    return true;
}

Eina_Bool ewk_policy_decision_ignore(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, false);
    policyDecision->isDecided = true;
    WKFramePolicyListenerIgnore(policyDecision->listener);
    return true;
}

Eina_Bool ewk_policy_decision_download(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, false);
    policyDecision->isDecided = true;
    WKFramePolicyListenerDownload(policyDecision->listener);
    return true;
}

Ewk_Policy_Navigation_Type ewk_policy_decision_navigation_type_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, EWK_POLICY_NAVIGATION_TYPE_OTHER);

    return policyDecision->navigationType;
}

Ewk_Frame_Ref ewk_policy_decision_frame_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);

    return static_cast<Ewk_Frame_Ref>(policyDecision->frame);
}

const char* ewk_policy_decision_http_body_get(Ewk_Policy_Decision* policyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(policyDecision, 0);

#if ENABLE(TIZEN_POLICY_DECISION_HTTP_BODY_GET)
    return policyDecision->httpBody;
#else
    return 0;
#endif
}

#endif // #if PLATFORM(TIZEN)
