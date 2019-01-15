/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 * Copyright (C) 2012, Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"
#include "NavigatorRegisterContentHandler.h"

#if ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)

#include "Document.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "Navigator.h"
#include "Page.h"
#include <wtf/HashSet.h>

namespace WebCore {

static HashSet<String>* ContentBlacklist;

static void initContentHandlerBlacklist()
{
    ContentBlacklist = new HashSet<String>;
    static const char* mimeType[] = {
        "application/x-www-form-urlencoded",
        "application/xhtml+xml",
        "application/xml",
        "image/gif",
        "image/jpeg",
        "image/png",
        "image/svg+xml",
        "multipart/x-mixed-replace",
        "text/cache-manifest",
        "text/css",
        "text/html",
        "text/ping",
        "text/plain",
        "text/xml"
    };

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(mimeType); ++i)
        ContentBlacklist->add(mimeType[i]);
}

static bool verifyCustomHandlerURL(const String& baseURL, const String& url, ExceptionCode& ec)
{
    static const char token[] = "%s";
    int index = url.find(token);
    if (-1 == index) {
        ec = SYNTAX_ERR;
        return false;
    }

    String newURL = url;
    newURL.remove(index, WTF_ARRAY_LENGTH(token) - 1);

    KURL base(ParsedURLString, baseURL);
    KURL kurl(base, newURL);

    if (kurl.isEmpty() || !kurl.isValid()) {
        ec = SYNTAX_ERR;
        return false;
    }
    return true;
}

static bool isContentBlacklisted(const String& mimeType)
{
    if (!ContentBlacklist)
        initContentHandlerBlacklist();
    return ContentBlacklist->contains(mimeType);
}

static bool verifyContentHandlerMimeType(const String& mimeType, ExceptionCode& ec)
{
    if (!isContentBlacklisted(mimeType))
        return true;
    ec = SECURITY_ERR;
    return false;
}

NavigatorRegisterContentHandler* NavigatorRegisterContentHandler::from(Page* page)
{
    return static_cast<NavigatorRegisterContentHandler*>(RefCountedSupplement<Page, NavigatorRegisterContentHandler>::from(page, NavigatorRegisterContentHandler::supplementName()));
}

NavigatorRegisterContentHandler::~NavigatorRegisterContentHandler()
{
}

PassRefPtr<NavigatorRegisterContentHandler> NavigatorRegisterContentHandler::create(RegisterContentHandlerClient* client)
{
    return adoptRef(new NavigatorRegisterContentHandler(client));
}

void NavigatorRegisterContentHandler::registerContentHandler(Navigator* navigator, const String& mimeType, const String& url, const String& title, ExceptionCode& ec)
{
    if (!navigator->frame())
        return;

    Document* document = navigator->frame()->document();
    if (!document)
        return;

    String baseURL = document->baseURL().baseAsString();

    if (!verifyCustomHandlerURL(baseURL, url, ec))
        return;

    if (!verifyContentHandlerMimeType(mimeType, ec))
        return;

    NavigatorRegisterContentHandler::from(navigator->frame()->page())->client()->registerContentHandler(mimeType, baseURL, url, navigator->frame()->displayStringModifiedByEncoding(title));
}

static String customHandlersStateString(const RegisterContentHandlerClient::CustomHandlersState state)
{
    DEFINE_STATIC_LOCAL(const String, newHandler, ("new"));
    DEFINE_STATIC_LOCAL(const String, registeredHandler, ("registered"));
    DEFINE_STATIC_LOCAL(const String, declinedHandler, ("declined"));

    switch (state) {
    case RegisterContentHandlerClient::CustomHandlersNew:
        return newHandler;
    case RegisterContentHandlerClient::CustomHandlersRegistered:
        return registeredHandler;
    case RegisterContentHandlerClient::CustomHandlersDeclined:
        return declinedHandler;
    }

    ASSERT_NOT_REACHED();
    return String();
}

String NavigatorRegisterContentHandler::isContentHandlerRegistered(Navigator* navigator, const String& mimeType, const String& url, ExceptionCode& ec)
{
    DEFINE_STATIC_LOCAL(const String, declined, ("declined"));

    if (!navigator->frame())
        return declined;

    Document* document = navigator->frame()->document();
    String baseURL = document->baseURL().baseAsString();

    if (!verifyCustomHandlerURL(baseURL, url, ec))
        return declined;

    if (!verifyContentHandlerMimeType(mimeType, ec))
        return declined;

    return customHandlersStateString(NavigatorRegisterContentHandler::from(navigator->frame()->page())->client()->isContentHandlerRegistered(mimeType, baseURL, url));
}

void NavigatorRegisterContentHandler::unregisterContentHandler(Navigator* navigator, const String& mimeType, const String& url, ExceptionCode& ec)
{
    if (!navigator->frame())
        return;

    Document* document = navigator->frame()->document();
    String baseURL = document->baseURL().baseAsString();

    if (!verifyCustomHandlerURL(baseURL, url, ec))
        return;

    if (!verifyContentHandlerMimeType(mimeType, ec))
        return;

    NavigatorRegisterContentHandler::from(navigator->frame()->page())->client()->unregisterContentHandler(mimeType, baseURL, url);
}

const AtomicString& NavigatorRegisterContentHandler::supplementName()
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("NavigatorRegisterContentHandler"));
    return name;
}

void provideRegisterContentHandlerTo(Page* page, RegisterContentHandlerClient* client)
{
    RefCountedSupplement<Page, NavigatorRegisterContentHandler>::provideTo(page, NavigatorRegisterContentHandler::supplementName(), NavigatorRegisterContentHandler::create(client));
}

} // namespace WebCore

#endif // ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)

