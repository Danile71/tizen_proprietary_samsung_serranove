/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
#include "WebRegisterContentHandlerClient.h"

#if ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)

#include "Connection.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"

using namespace WebCore;

namespace WebKit {

void WebRegisterContentHandlerClient::registerContentHandler(const String& mimeType, const String& baseURL, const String& url, const String& title)
{
    WebProcess::shared().connection()->send(Messages::WebPageProxy::RegisterContentHandler(mimeType, baseURL, url, title), m_page->pageID());
}

RegisterContentHandlerClient::CustomHandlersState WebRegisterContentHandlerClient::isContentHandlerRegistered(const String& mimeType, const String& baseURL, const String& url)
{
    unsigned int result;
    WebProcess::shared().connection()->sendSync(Messages::WebPageProxy::IsContentHandlerRegistered(mimeType, baseURL, url), Messages::WebPageProxy::IsContentHandlerRegistered::Reply(result), m_page->pageID());
    return static_cast<RegisterContentHandlerClient::CustomHandlersState>(result);
}

void WebRegisterContentHandlerClient::unregisterContentHandler(const String& mimeType, const String& baseURL, const String& url)
{
    WebProcess::shared().connection()->send(Messages::WebPageProxy::UnregisterContentHandler(mimeType, baseURL, url), m_page->pageID());
}

} // namespace WebKit

#endif // ENABLE(TIZEN_REGISTER_PROTOCOL_HANDLER) || ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)
