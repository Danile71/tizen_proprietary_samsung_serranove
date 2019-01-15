#include "config.h"
#include "DOMWindowSpeech.h"

#if ENABLE(SCRIPTED_SPEECH)

#include "DOMWindow.h"
#include "ScriptExecutionContext.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

DOMWindowSpeech::DOMWindowSpeech(DOMWindow* window)
    : DOMWindowProperty(window->frame())
{
}

DOMWindowSpeech::~DOMWindowSpeech()
{
}

DOMWindowSpeech* DOMWindowSpeech::from(DOMWindow* window)
{
    DEFINE_STATIC_LOCAL(AtomicString, name, ("DOMWindowSpeech", AtomicString::ConstructFromLiteral));
    DOMWindowSpeech* supplement = static_cast<DOMWindowSpeech*>(Supplement<DOMWindow>::from(window, name));
    if (!supplement) {
        supplement = new DOMWindowSpeech(window);
        provideTo(window, name, adoptPtr(supplement));
    }
    return supplement;
}

SpeechRecognition* DOMWindowSpeech::speechRecognition(ScriptExecutionContext* context, DOMWindow* window)
{
    return DOMWindowSpeech::from(window)->speechRecognition(context);
}

SpeechRecognition* DOMWindowSpeech::speechRecognition(ScriptExecutionContext* context)
{
    if (!m_speechRecognition && frame())
        m_speechRecognition = SpeechRecognition::create(context);
    return m_speechRecognition.get();
}

} // namespace WebCore

#endif
