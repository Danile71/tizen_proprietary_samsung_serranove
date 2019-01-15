/*
 * Copyright (C) 2012 Samsung Electronics. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef RegisterContentHandlerClient_h
#define RegisterContentHandlerClient_h

#if ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)

#include <wtf/text/WTFString.h>

namespace WebCore {

class Page;

class RegisterContentHandlerClient {
public:
    virtual ~RegisterContentHandlerClient() { }

    enum CustomHandlersState {
        CustomHandlersNew,
        CustomHandlersRegistered,
        CustomHandlersDeclined
    };

    virtual void registerContentHandler(const String& mimeType, const String& baseURL, const String& url, const String& title) = 0;
    virtual CustomHandlersState isContentHandlerRegistered(const String& mimeType, const String& baseURL, const String& url) = 0;
    virtual void unregisterContentHandler(const String& mimeType, const String& baseURL, const String& url) = 0;
};

void provideRegisterContentHandlerTo(Page*, RegisterContentHandlerClient*);

}

#endif
#endif // RegisterContentHandlerClient_h
