/*
 * Copyright (C) 2011 Samsung Electronics
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

#ifndef GestureRecognizer_h
#define GestureRecognizer_h

#include "PageClientImpl.h"
#include "ewk_view.h"
#include <Ecore.h>
#include <Elementary.h>
#include <Evas.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {
    class IntPoint;
}

namespace WebKit {

class GestureRecognizer {
public:
    static PassOwnPtr<GestureRecognizer> create(Evas_Object* ewkView)
    {
        return adoptPtr(new GestureRecognizer(ewkView));
    }
    ~GestureRecognizer();

    void setDoubleTapEnabled(bool);

private:
    explicit GestureRecognizer(Evas_Object*);
    void initializeCallbacks();

    void setTapScheduled(bool);

    void startTap(Elm_Gesture_Taps_Info*);
    void endTap(Elm_Gesture_Taps_Info*);
    void abortTap(Elm_Gesture_Taps_Info*);
    void processTap(int x, int y, unsigned int timestamp);
    void processScheduledTap(int x, int y, unsigned int timestamp);
    void startLongTap(Elm_Gesture_Taps_Info*);
    void moveLongTap(Elm_Gesture_Taps_Info*);
    void endLongTap(Elm_Gesture_Taps_Info*);
    void endDoubleTap(Elm_Gesture_Taps_Info*);

    void startMomentum(Elm_Gesture_Momentum_Info*);
    void moveMomentum(Elm_Gesture_Momentum_Info*);
    void endMomentum(Elm_Gesture_Momentum_Info*);
    void startZoom(Elm_Gesture_Zoom_Info*);
    void moveZoom(Elm_Gesture_Zoom_Info*);
    void endZoom(Elm_Gesture_Zoom_Info*);

    void startGesture(Ewk_Gesture_Type, const WebCore::IntPoint& position, const WebCore::IntPoint& velocity, double scale, int count);
    void endGesture(Ewk_Gesture_Type, const WebCore::IntPoint& position, const WebCore::IntPoint& velocity, double scale, int count);
    void moveGesture(Ewk_Gesture_Type, const WebCore::IntPoint& position, const WebCore::IntPoint& velocity, double scale, int count);

    static Evas_Event_Flags onTapStart(void* data, void* eventInfo);
    static Evas_Event_Flags onTapAbort(void* data, void* eventInfo);
    static Evas_Event_Flags onTapEnd(void* data, void* eventInfo);
    static Evas_Event_Flags onLongTapStart(void* data, void* eventInfo);
    static Evas_Event_Flags onLongTapMove(void* data, void* eventInfo);
    static Evas_Event_Flags onLongTapEnd(void* data, void* eventInfo);
    static Evas_Event_Flags onDoubleTapStart(void* data, void* eventInfo);
    static Evas_Event_Flags onDoubleTapMove(void* data, void* eventInfo);
    static Evas_Event_Flags onMomentumStart(void* data, void* eventInfo);
    static Evas_Event_Flags onMomentumMove(void* data, void* eventInfo);
    static Evas_Event_Flags onMomentumEnd(void* data, void* eventInfo);
    static Evas_Event_Flags onMomentumAbort(void* data, void* eventInfo);
    static Evas_Event_Flags onZoomStart(void* data, void* eventInfo);
    static Evas_Event_Flags onZoomMove(void* data, void* eventInfo);
    static Evas_Event_Flags onZoomEnd(void* data, void* eventInfo);

    static void onMouseUp(void* data, Evas*, Evas_Object*, void* eventInfo);

private:
    static const int s_defaultFramerate = 60;
    static const int s_tapThreshold = 500;

private:
    Evas_Object* m_viewWidget;
    Evas_Object* m_gestureObject;

    WebCore::IntPoint m_pressedPoint;
    WebCore::IntPoint m_currentPoint;
    bool m_isTapScheduled;
    bool m_isLongTapProcessed;
    unsigned m_longTapCount;
    unsigned m_doubleTapCount;
};

} // namespace WebKit

#endif // GestureRecognizer_h
