/*
 *  Copyright (C) 2013 Samsung Electronics.  All rights reserved.
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

#ifndef SpeechRecognitionProviderTizen_h
#define SpeechRecognitionProviderTizen_h

#if ENABLE(SCRIPTED_SPEECH)

#include <stt.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class STTProviderTizen;
class SpeechGrammarList;
class SpeechRecognition;

class SpeechRecognitionProviderTizen {
public:
    static PassOwnPtr<SpeechRecognitionProviderTizen> create();
    ~SpeechRecognitionProviderTizen();

    void start(SpeechRecognition*, const SpeechGrammarList*, const String&, bool continuous, bool interimResults, unsigned long maxAlternatives);
    void stop(SpeechRecognition*);
    void abort(SpeechRecognition*);

    // Callbacks
    static void sttResultCallback(stt_h, stt_result_event_e, const char** data, int dataCount, const char* msg, void* userData);
    static void sttErrorCallback(stt_h, stt_error_e, void* useData);
    static void sttStateChangedCallback(stt_h, stt_state_e, stt_state_e, void* userData);

private:
    SpeechRecognitionProviderTizen();

    void startRecognition();

    bool initProvider();
    bool startProvider();
    bool isError(int err);
    bool noMatchResults(const String&);

    SpeechRecognition* m_speechRecognition;
    const SpeechGrammarList* m_grammarList;
    String m_lang;

    bool m_continuous;
    bool m_interimResults;
    bool m_maxAlternatives;
    bool m_providerInitialized;

    OwnPtr<STTProviderTizen> m_sttProvider;
};

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)

#endif // SpeechRecognitionProviderTizen_h
