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

#include "config.h"
#include "GestureRecognizer.h"

#include <WebCore/IntSize.h>
#include <WebCore/IntPoint.h>

#define FINGER_SIZE 80

using namespace WebCore;

namespace WebKit {

static int getPointDistanceSquare(const IntPoint& point1, const IntPoint& point2)
{
    int xDistance = point1.x() - point2.x();
    int yDistance = point1.y() - point2.y();
    return xDistance * xDistance + yDistance * yDistance;
}

GestureRecognizer::GestureRecognizer(Evas_Object* ewkView)
    : m_viewWidget(ewkView)
    , m_pressedPoint()
    , m_currentPoint()
    , m_isTapScheduled(false)
    , m_isLongTapProcessed(false)
    , m_longTapCount(0)
    , m_doubleTapCount(0)
{
    ASSERT(m_viewWidget);

    m_gestureObject = elm_gesture_layer_add(m_viewWidget);
    elm_gesture_layer_hold_events_set(m_gestureObject, false);
    elm_gesture_layer_attach(m_gestureObject, m_viewWidget);
    elm_gesture_layer_zoom_distance_tolerance_set(m_gestureObject, 0);
    elm_gesture_layer_double_tap_timeout_set(m_gestureObject, 0.4);
    ecore_animator_frametime_set(1.0 / s_defaultFramerate); // it should be done in the application??
    initializeCallbacks();
}

GestureRecognizer::~GestureRecognizer()
{
    // Delete gesture callbacks.
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_START, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_ABORT, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_END, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_START, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_MOVE, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_END, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_ABORT, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_DOUBLE_TAPS, ELM_GESTURE_STATE_START, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_DOUBLE_TAPS, ELM_GESTURE_STATE_MOVE, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_START, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_MOVE, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_END, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_ABORT, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_START, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_END, 0, 0);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_ABORT, 0, 0);

    if (m_gestureObject)
        evas_object_del(m_gestureObject);

    evas_object_event_callback_del(m_viewWidget, EVAS_CALLBACK_MOUSE_UP, onMouseUp);
}

void GestureRecognizer::setDoubleTapEnabled(bool enabled)
{
    if (enabled) {
        elm_gesture_layer_tap_finger_size_set(m_gestureObject, FINGER_SIZE);
        elm_gesture_layer_double_tap_timeout_set(m_gestureObject, 0.4);
    // FIXME: If double tap timeout is zero, the gesture_layer does not make tap and makes double tap without any delay.
    // So, we have to set double tap timeout as small value to make tap and disable double tap.
    // The value will be changed to zero when elm_gesture_layer works correctly.
    } else {
        elm_gesture_layer_tap_finger_size_set(m_gestureObject, 0);
        elm_gesture_layer_double_tap_timeout_set(m_gestureObject, 0.00001);
    }
}

void GestureRecognizer::initializeCallbacks()
{
    // Add gesture callbacks.
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_START, onTapStart, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_ABORT, onTapAbort, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_TAPS, ELM_GESTURE_STATE_END, onTapEnd, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_START, onLongTapStart, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_MOVE, onLongTapMove, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_END, onLongTapEnd, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_LONG_TAPS, ELM_GESTURE_STATE_ABORT, onLongTapEnd, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_DOUBLE_TAPS, ELM_GESTURE_STATE_START, onDoubleTapStart, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_N_DOUBLE_TAPS, ELM_GESTURE_STATE_MOVE, onDoubleTapMove, this);

    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_START, onMomentumStart, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_MOVE, onMomentumMove, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_END, onMomentumEnd, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_MOMENTUM, ELM_GESTURE_STATE_ABORT, onMomentumAbort, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_START, onZoomStart, this);

    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_MOVE, onZoomMove, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_END, onZoomEnd, this);
    elm_gesture_layer_cb_set(m_gestureObject, ELM_GESTURE_ZOOM, ELM_GESTURE_STATE_ABORT, onZoomEnd, this);

    // add evas callbacks.
    evas_object_event_callback_add(m_viewWidget, EVAS_CALLBACK_MOUSE_UP, onMouseUp, this);
}

Evas_Event_Flags GestureRecognizer::onTapStart(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onTapStart is called");
#endif
    static_cast<GestureRecognizer*>(data)->startTap(static_cast<Elm_Gesture_Taps_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onTapEnd(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onTapEnd is called");
#endif
    static_cast<GestureRecognizer*>(data)->endTap(static_cast<Elm_Gesture_Taps_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onTapAbort(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onTapAbort is called event->n[%d]", static_cast<Elm_Gesture_Taps_Info*>(eventInfo)->n);
#endif
    static_cast<GestureRecognizer*>(data)->abortTap(static_cast<Elm_Gesture_Taps_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onLongTapStart(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onLongTapStart is called");
#endif
    static_cast<GestureRecognizer*>(data)->startLongTap(static_cast<Elm_Gesture_Taps_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onLongTapMove(void* data, void* eventInfo)
{
    static_cast<GestureRecognizer*>(data)->moveLongTap(static_cast<Elm_Gesture_Taps_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onLongTapEnd(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onLongTapEnd is called");
#endif
    static_cast<GestureRecognizer*>(data)->endLongTap(static_cast<Elm_Gesture_Taps_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onDoubleTapStart(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onDoubleTapStart is called");
#endif
    static_cast<GestureRecognizer*>(data)->m_doubleTapCount = 0;

    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onDoubleTapMove(void* data, void* eventInfo)
{
    // Process double tap in the timing of "mouse down -> up -> down" instead of "down -> up -> down -> up".
    GestureRecognizer* gestureRecognizer = static_cast<GestureRecognizer*>(data);
    gestureRecognizer->m_doubleTapCount++;
    if (gestureRecognizer->m_doubleTapCount == 2)
        gestureRecognizer->endDoubleTap(static_cast<Elm_Gesture_Taps_Info*>(eventInfo));

    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onMomentumStart(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onMomentumStart is called");
#endif
    static_cast<GestureRecognizer*>(data)->startMomentum(static_cast<Elm_Gesture_Momentum_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onMomentumMove(void* data, void* eventInfo)
{
    static_cast<GestureRecognizer*>(data)->moveMomentum(static_cast<Elm_Gesture_Momentum_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onMomentumEnd(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onMomentumEnd is called");
#endif
    static_cast<GestureRecognizer*>(data)->endMomentum(static_cast<Elm_Gesture_Momentum_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onMomentumAbort(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onMomentumAbort is called");
#endif
    Elm_Gesture_Momentum_Info* event = static_cast<Elm_Gesture_Momentum_Info*>(eventInfo);
    static_cast<GestureRecognizer*>(data)->endMomentum(event);
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onZoomStart(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onZoomStart is called");
#endif
    static_cast<GestureRecognizer*>(data)->startZoom(static_cast<Elm_Gesture_Zoom_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

void GestureRecognizer::onMouseUp(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Up* event = static_cast<Evas_Event_Mouse_Up*>(eventInfo);
    GestureRecognizer* gestureRecognizer = static_cast<GestureRecognizer*>(data);

#if ENABLE(TIZEN_WEBKIT2_TOUCH_CANCEL)
    if (event->event_flags & EVAS_EVENT_FLAG_ON_HOLD) {
        gestureRecognizer->setTapScheduled(false);
        gestureRecognizer->m_isLongTapProcessed = false;
        return;
    }
#endif

    gestureRecognizer->processScheduledTap(event->canvas.x, event->canvas.y, event->timestamp);
}

void GestureRecognizer::startTap(Elm_Gesture_Taps_Info* event)
{
    m_currentPoint = m_pressedPoint = IntPoint(event->x, event->y);

    setTapScheduled(false);
    m_isLongTapProcessed = false;
    startGesture(EWK_GESTURE_TAP, IntPoint(event->x, event->y), IntPoint(), 1, 1);
}

void GestureRecognizer::endTap(Elm_Gesture_Taps_Info* event)
{
    if (event->n != 1)
        return;

    m_currentPoint = IntPoint(event->x, event->y);
    processTap(event->x, event->y, event->timestamp);
}

void GestureRecognizer::abortTap(Elm_Gesture_Taps_Info* event)
{
    // The elm_gesture_layer aborts tap when 0.4 second is passed, but we want to process tap after 0.4 second.
    // So, I set isTapScheduled to true to process tap when mouse up.
    if (event->n == 1) // Tap is aborted after 0.4 second.
        setTapScheduled(true);
    else // Tap is aborted by moving before 0.4 second.
        setTapScheduled(false);
}

void GestureRecognizer::processTap(int x, int y, unsigned int timestamp)
{
    endGesture(EWK_GESTURE_TAP, IntPoint(x, y), IntPoint(), 1, 1);
    setTapScheduled(false);
}

void GestureRecognizer::processScheduledTap(int x, int y, unsigned int timestamp)
{
    if (m_isTapScheduled)
        processTap(x, y, timestamp);
}

void GestureRecognizer::startLongTap(Elm_Gesture_Taps_Info* event)
{
    m_longTapCount = 0;
}

void GestureRecognizer::moveLongTap(Elm_Gesture_Taps_Info* event)
{
    m_longTapCount++;

    // We have to process only first movement as a long tap.
    if (m_longTapCount != 1 || event->n != 1)
        return;

    setTapScheduled(false);

    startGesture(EWK_GESTURE_LONG_PRESS, IntPoint(event->x, event->y), IntPoint(), 1, 1);
    m_isLongTapProcessed = true;
}

void GestureRecognizer::endLongTap(Elm_Gesture_Taps_Info* event)
{
    if (event->n != 1 || !m_isLongTapProcessed)
        return;

    endGesture(EWK_GESTURE_LONG_PRESS, IntPoint(event->x, event->y), IntPoint(), 1, 1);
}

void GestureRecognizer::endDoubleTap(Elm_Gesture_Taps_Info* event)
{
    if (event->n != 1)
        return;

    // We do not process tap after double tap occurs.
    setTapScheduled(false);

    endGesture(EWK_GESTURE_TAP, IntPoint(event->x, event->y), IntPoint(), 1, 2);
}

// The isTapScheduled will be set to true when tap is aborted,
// and set to false when zoom, pan, double tap and long tap is occured.
// Additionally, focus will be set when focus timer is expired,
// and will be clear when tap is processed or we don't want to process scheduled tap.
void GestureRecognizer::setTapScheduled(bool scheduled)
{
    m_isTapScheduled = scheduled;
}

void GestureRecognizer::startMomentum(Elm_Gesture_Momentum_Info* event)
{
    if (event->n != 1)
        return;

    startGesture(EWK_GESTURE_PAN, IntPoint(event->x2, event->y2), IntPoint(), 1, 1);
}

void GestureRecognizer::moveMomentum(Elm_Gesture_Momentum_Info* event)
{
    if (event->n != 1)
        return;

    m_currentPoint = IntPoint(event->x2, event->y2);

    if (m_isTapScheduled && (getPointDistanceSquare(m_pressedPoint, m_currentPoint) > s_tapThreshold))
        setTapScheduled(false);

    moveGesture(EWK_GESTURE_PAN, m_currentPoint, IntPoint(event->mx, event->my), 1, 1);
}

void GestureRecognizer::endMomentum(Elm_Gesture_Momentum_Info* event)
{
    endGesture(EWK_GESTURE_PAN, IntPoint(event->x2, event->y2), IntPoint(event->mx, event->my), 1, 1);
}

void GestureRecognizer::startZoom(Elm_Gesture_Zoom_Info* event)
{
    setTapScheduled(false);
    startGesture(EWK_GESTURE_PINCH, IntPoint(event->x, event->y), IntPoint(), event->zoom, 1);
}

Evas_Event_Flags GestureRecognizer::onZoomMove(void* data, void* eventInfo)
{
    static_cast<GestureRecognizer*>(data)->moveZoom(static_cast<Elm_Gesture_Zoom_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

Evas_Event_Flags GestureRecognizer::onZoomEnd(void* data, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] onZoomEnd is called");
#endif
    static_cast<GestureRecognizer*>(data)->endZoom(static_cast<Elm_Gesture_Zoom_Info*>(eventInfo));
    return EVAS_EVENT_FLAG_NONE;
}

void GestureRecognizer::moveZoom(Elm_Gesture_Zoom_Info* event)
{
    moveGesture(EWK_GESTURE_PINCH, IntPoint(event->x, event->y), IntPoint(), event->zoom, 1);
}

void GestureRecognizer::endZoom(Elm_Gesture_Zoom_Info* event)
{
    endGesture(EWK_GESTURE_PINCH, IntPoint(event->x, event->y), IntPoint(), event->zoom, 1);
}

void GestureRecognizer::startGesture(Ewk_Gesture_Type type, const IntPoint& position, const IntPoint& velocity, double scale, int count)
{
    const Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_viewWidget));
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->gesture_start);

    Ewk_Event_Gesture gestureEvent;
    memset(&gestureEvent, 0, sizeof(Ewk_Event_Gesture));
    gestureEvent.type = type;
    gestureEvent.position.x = position.x();
    gestureEvent.position.y = position.y();
    gestureEvent.velocity.x = velocity.x();
    gestureEvent.velocity.y = velocity.y();
    gestureEvent.scale = scale;
    gestureEvent.count = count;
    gestureEvent.timestamp = ecore_time_get() * 1000;

    smartData->api->gesture_start(const_cast<Ewk_View_Smart_Data*>(smartData), &gestureEvent);
}

void GestureRecognizer::endGesture(Ewk_Gesture_Type type, const IntPoint& position, const IntPoint& velocity, double scale, int count)
{
    const Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_viewWidget));
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->gesture_end);

    Ewk_Event_Gesture gestureEvent;
    memset(&gestureEvent, 0, sizeof(Ewk_Event_Gesture));
    gestureEvent.type = type;
    gestureEvent.position.x = position.x();
    gestureEvent.position.y = position.y();
    gestureEvent.velocity.x = velocity.x();
    gestureEvent.velocity.y = velocity.y();
    gestureEvent.scale = scale;
    gestureEvent.count = count;
    gestureEvent.timestamp = ecore_time_get() * 1000;

    smartData->api->gesture_end(const_cast<Ewk_View_Smart_Data*>(smartData), &gestureEvent);
}

void GestureRecognizer::moveGesture(Ewk_Gesture_Type type, const IntPoint& position, const IntPoint& velocity, double scale, int count)
{
    const Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_viewWidget));
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->gesture_move);

    Ewk_Event_Gesture gestureEvent;
    memset(&gestureEvent, 0, sizeof(Ewk_Event_Gesture));
    gestureEvent.type = type;
    gestureEvent.position.x = position.x();
    gestureEvent.position.y = position.y();
    gestureEvent.velocity.x = velocity.x();
    gestureEvent.velocity.y = velocity.y();
    gestureEvent.scale = scale;
    gestureEvent.count = count;
    gestureEvent.timestamp = ecore_time_get() * 1000;

    smartData->api->gesture_move(const_cast<Ewk_View_Smart_Data*>(smartData), &gestureEvent);
}

} // namespace WebKit
