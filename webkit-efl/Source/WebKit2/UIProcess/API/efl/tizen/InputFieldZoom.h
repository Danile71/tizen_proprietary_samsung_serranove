/*
 * Copyright (C) 2013 Samsung Electronics All rights reserved.
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

#ifndef InputFieldZoom_h
#define InputFieldZoom_h

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
#include <Ecore.h>
#include <Evas.h>
#include <WebCore/IntRect.h>
#include <WebCore/IntPoint.h>

class EwkViewImpl;

namespace WebKit {

class InputFieldZoom {
public:
    static PassOwnPtr<InputFieldZoom> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new InputFieldZoom(viewImpl));
    }
    ~InputFieldZoom();

    bool scheduleInputFieldZoom();
    void stop();
    bool isWorking();
    bool process();
    bool isInputFieldZoomScheduled() { return (bool)m_timer; }

    bool m_isScaleFactorChanged;

private:
    explicit InputFieldZoom(EwkViewImpl*);
    static Eina_Bool startInputFieldZoom(void *data);
    bool calculateAnimationFactors();

    EwkViewImpl* m_viewImpl;
    Ecore_Animator* m_scaleAnimator;
    WebCore::FloatRect m_baseRect;
    WebCore::FloatRect m_targetRect;
    int m_scaleIndex;
    float m_targetScale;
    bool m_isWorking;
    Ecore_Timer* m_timer;
    bool m_needToStop;
};

} // namespace WebKit
#endif // TIZEN_WEBKIT2_INPUT_FIELD_ZOOM
#endif // InputFieldZoom_h
