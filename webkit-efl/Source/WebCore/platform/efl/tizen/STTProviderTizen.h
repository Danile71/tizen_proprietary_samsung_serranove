#ifndef STTProviderTizen_h
#define STTProviderTizen_h

#if ENABLE(SCRIPTED_SPEECH)

#include <stt.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class SpeechRecognition;
class SpeechRecognitionError;

class STTProviderTizen {
    WTF_MAKE_NONCOPYABLE(STTProviderTizen);
public:
    static PassOwnPtr<STTProviderTizen> create();
    static PassRefPtr<SpeechRecognitionError> toCoreError(stt_error_e);

    ~STTProviderTizen();

    int sttCreate();
    int sttPrepare();
    int sttUnprepare();
    int sttStart(const char* lang, const char* type);
    int sttStop();
    int sttCancel();
    int sttDestroy();

    int sttResultCallbackSet(stt_recognition_result_cb, void*);
    int sttResultCallbackUnset();

    int sttStateChangedCallbackSet(stt_state_changed_cb, void*);
    int sttStateChangedCallbackUnset();

    int sttErrorCallbackSet(stt_error_cb, void*);
    int sttErrorCallbackUnset();

    int sttResultTypeSupported(const char* type, bool* partialResult);

    int sttDefaultLanguage(char** language);

    int sttSilenceDetectionSet(stt_option_silence_detection_e);
    int sttState(stt_state_e*);

    bool sttReady() const { return m_sttReady; }

private:
    STTProviderTizen();

    stt_h m_stt;

    bool m_sttReady;
};

}

#endif // ENABLE(SCRIPTED_SPEECH)

#endif // STTProviderTizen_h
