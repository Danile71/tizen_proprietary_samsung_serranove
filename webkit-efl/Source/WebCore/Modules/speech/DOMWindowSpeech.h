#ifndef DOMWindowSpeech_h
#define DOMWindowSpeech_h

#if ENABLE(SCRIPTED_SPEECH)

#include "DOMWindowProperty.h"
#include "SpeechRecognition.h"
#include "Supplementable.h"

namespace WebCore {

class DOMWindow;
class ScriptExecutionContext;

class DOMWindowSpeech : public Supplement<DOMWindow>, public DOMWindowProperty {
public:
    virtual ~DOMWindowSpeech();

    static SpeechRecognition* speechRecognition(ScriptExecutionContext*, DOMWindow*);
    static DOMWindowSpeech* from(DOMWindow*);

private:
    explicit DOMWindowSpeech(DOMWindow*);

    SpeechRecognition* speechRecognition(ScriptExecutionContext*);

    RefPtr<SpeechRecognition> m_speechRecognition;
};

} // namespace WebCore

#endif // ENABLE(SCRIPTED_SPEECH)

#endif // DOMWindowSpeech_h
