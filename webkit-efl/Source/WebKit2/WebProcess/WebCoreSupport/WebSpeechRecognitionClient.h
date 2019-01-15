/*
 *  Copyright (C) 2012 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef WebSpeechRecognitionClient_h
#define WebSpeechRecognitionClient_h

#if ENABLE(SCRIPTED_SPEECH)

#include <WebCore/SpeechRecognitionClient.h>
#include <wtf/OwnPtr.h>

namespace WebCore {
class SpeechRecognitionProviderTizen;
}

namespace WebKit {

class WebPage;

class WebSpeechRecognitionClient : public WebCore::SpeechRecognitionClient {
public:
    WebSpeechRecognitionClient(WebPage* page)
        : m_page(page)
    {
    }

    virtual ~WebSpeechRecognitionClient() {}

private:
    virtual void start(WebCore::SpeechRecognition*, const WebCore::SpeechGrammarList*, const String&, bool continuous, bool interimResults, unsigned long maxAlternatives) OVERRIDE;
    virtual void stop(WebCore::SpeechRecognition*) OVERRIDE;
    virtual void abort(WebCore::SpeechRecognition*) OVERRIDE;

    WebPage* m_page;
    OwnPtr<WebCore::SpeechRecognitionProviderTizen> m_provider;
};

}

#endif // ENABLE(SCRIPTED_SPEECH)

#endif //WebSpeechRecognitionClient_h

