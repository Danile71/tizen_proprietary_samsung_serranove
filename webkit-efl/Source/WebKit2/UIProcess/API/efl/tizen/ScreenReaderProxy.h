/*
 * Copyright (C) 2013 Samsung Electronic.
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

#ifndef ScreenReaderProxy_h
#define ScreenReaderProxy_h

#if ENABLE(TIZEN_SCREEN_READER)

#include <Ecore.h>
#include <Evas.h>
#include <tts.h>
#include <wtf/Vector.h>

class EwkViewImpl;

namespace WebKit {

class ScreenReaderProxy {
public:
    static ScreenReaderProxy& screenReader();

    virtual ~ScreenReaderProxy();

    bool isActive(EwkViewImpl* viewImpl) { return viewImpl == m_activeViewImpl; }

    void initialize(EwkViewImpl*);
    void finalize(EwkViewImpl*);

    void setText(const String&);

    FocusRing* focusRing() { return m_focusRing.get(); }

    static Eina_Bool executeAction(void*, Evas_Object*, Elm_Access_Action_Info*);

private:
    ScreenReaderProxy();

    void setActiveViewImpl(EwkViewImpl*);
    void clearActiveViewImpl(EwkViewImpl*);

    bool initializeTTS(bool);

    static void didTTSStateChanged(tts_h, tts_state_e, tts_state_e, void*);

    EwkViewImpl* m_activeViewImpl;

    tts_h m_ttsHandle;
    String m_ttsText;

    bool m_gestureStarted;
    WebCore::IntPoint m_screenReaderPoint;

    bool m_isActivate;
    bool m_isAbleExtend;

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    OwnPtr<FocusRing> m_focusRing;
#endif
};

} // namespace WebKit

#endif // ENABLE(TIZEN_SCREEN_READER)

#endif // ScreenReaderProxy_h
