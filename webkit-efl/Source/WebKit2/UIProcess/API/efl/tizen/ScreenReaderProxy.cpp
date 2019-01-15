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

#include "config.h"
#include "ScreenReaderProxy.h"

#if ENABLE(TIZEN_SCREEN_READER)

#include "EwkViewImpl.h"
#include "PageClientImpl.h"

#include <Ecore_X.h>

using namespace WebCore;

namespace WebKit {

ScreenReaderProxy& ScreenReaderProxy::screenReader()
{
    DEFINE_STATIC_LOCAL(ScreenReaderProxy, screenReader, ());
    return screenReader;
}

ScreenReaderProxy::ScreenReaderProxy()
    : m_activeViewImpl(0)
    , m_ttsHandle(0)
    , m_gestureStarted(false)
    , m_screenReaderPoint(0, 0)
    , m_isActivate(false)
    , m_isAbleExtend(false)
{
}

ScreenReaderProxy::~ScreenReaderProxy()
{
    if (m_ttsHandle)
        tts_destroy(m_ttsHandle);
}

void ScreenReaderProxy::setActiveViewImpl(EwkViewImpl* viewImpl)
{
    if (viewImpl == m_activeViewImpl)
        return;

    if (m_activeViewImpl)
        clearActiveViewImpl(m_activeViewImpl);

    m_activeViewImpl = viewImpl;

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    if (!m_focusRing) {
        m_focusRing = FocusRing::create(m_activeViewImpl);
        m_focusRing->setImage(SCREEN_READER_FOCUS_RING_IMAGE_PATH, 2, 4);
    }
#endif

    initializeTTS(false);
}

void ScreenReaderProxy::clearActiveViewImpl(EwkViewImpl* viewImpl)
{
    if (viewImpl != m_activeViewImpl)
        return;

    m_activeViewImpl = 0;
    viewImpl->page()->clearScreenReader();

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    m_focusRing.clear();
#endif

    tts_stop(m_ttsHandle);
}

void ScreenReaderProxy::initialize(EwkViewImpl* viewImpl)
{
    Evas_Object* accessObject = elm_access_object_get(viewImpl->view());
    if (!accessObject)
        return;

    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_HIGHLIGHT, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_UNHIGHLIGHT, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_HIGHLIGHT_PREV, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_ACTIVATE, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_SCROLL, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_MOUSE, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_UP, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_DOWN, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_BACK, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_OVER, executeAction, viewImpl);
    elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_READ, executeAction, viewImpl);
}

void ScreenReaderProxy::finalize(EwkViewImpl* viewImpl)
{
    clearActiveViewImpl(viewImpl);

    Evas_Object* accessObject = elm_access_object_get(viewImpl->view());
    if (accessObject) {
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_HIGHLIGHT, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_UNHIGHLIGHT, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_HIGHLIGHT_NEXT, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_HIGHLIGHT_PREV, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_ACTIVATE, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_SCROLL, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_MOUSE, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_UP, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_DOWN, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_BACK, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_OVER, 0, 0);
        elm_access_action_cb_set(accessObject, ELM_ACCESS_ACTION_READ, 0, 0);
    }
}

bool ScreenReaderProxy::initializeTTS(bool isForced)
{
    if (m_ttsHandle) {
        if (!isForced)
            return true;

        tts_destroy(m_ttsHandle);
        m_ttsHandle = 0;
    }

    if (tts_create(&m_ttsHandle)
        || tts_set_state_changed_cb(m_ttsHandle, didTTSStateChanged, 0)
        || tts_set_mode(m_ttsHandle, TTS_MODE_SCREEN_READER)
        || tts_prepare(m_ttsHandle)) {
        tts_destroy(m_ttsHandle);
        m_ttsHandle = 0;
        return false;
    }

    return true;
}

void ScreenReaderProxy::setText(const String& text)
{
    m_ttsText = text;

    if (m_ttsText.isEmpty()) {
        tts_stop(m_ttsHandle);
        return;
    }

    if (!initializeTTS(false))
        return;

    tts_state_e state;
    if (!tts_get_state(m_ttsHandle, &state) && state == TTS_STATE_READY) {
        didTTSStateChanged(0, TTS_STATE_READY, TTS_STATE_READY, 0);
        return;
    }

    if (tts_stop(m_ttsHandle))
        initializeTTS(true);
}

Eina_Bool ScreenReaderProxy::executeAction(void* data, Evas_Object*, Elm_Access_Action_Info* actionInfo)
{
    if (!actionInfo)
        return false;

    EwkViewImpl* viewImpl = static_cast<EwkViewImpl*>(data);

    switch (actionInfo->action_type) {
    case ELM_ACCESS_ACTION_HIGHLIGHT:
        screenReader().m_screenReaderPoint = IntPoint(-1, -1);
        if (screenReader().m_isActivate)
            screenReader().m_isAbleExtend = true;
        screenReader().setActiveViewImpl(viewImpl);

        if (actionInfo->action_by == ELM_ACCESS_ACTION_HIGHLIGHT_NEXT)
            return viewImpl->page()->moveScreenReaderFocus(true);
        if (actionInfo->action_by == ELM_ACCESS_ACTION_HIGHLIGHT_PREV)
            return viewImpl->page()->moveScreenReaderFocus(false);
        break;

    case ELM_ACCESS_ACTION_UNHIGHLIGHT:
        screenReader().m_isActivate = false;
        screenReader().m_isAbleExtend = false;
        screenReader().clearActiveViewImpl(viewImpl);
        break;

    case ELM_ACCESS_ACTION_HIGHLIGHT_NEXT:
        screenReader().m_screenReaderPoint = IntPoint(-1, -1);

        if (screenReader().m_isAbleExtend && viewImpl->page()->extendParagraphOnScreenReader(WebCore::DirectionForward))
            return true;
        else {
            screenReader().m_isActivate = false;
            screenReader().m_isAbleExtend = false;
            return viewImpl->page()->moveScreenReaderFocus(true);
        }

    case ELM_ACCESS_ACTION_HIGHLIGHT_PREV:
        screenReader().m_screenReaderPoint = IntPoint(-1, -1);

        if (screenReader().m_isAbleExtend && viewImpl->page()->extendParagraphOnScreenReader(WebCore::DirectionBackward))
            return true;
        else {
            screenReader().m_isActivate = false;
            screenReader().m_isAbleExtend = false;
            return viewImpl->page()->moveScreenReaderFocus(false);
        }

    case ELM_ACCESS_ACTION_ACTIVATE: {
        screenReader().m_isActivate = true;
        IntPoint point(-1, -1);
        IntPoint focusedPoint(-1,-1);
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        if (screenReader().m_focusRing) {
            if (screenReader().m_screenReaderPoint.x() < 0 || screenReader().m_screenReaderPoint.y() < 0)
                point = screenReader().m_focusRing->centerPointInScreen();
            else
                point = screenReader().m_screenReaderPoint;
        }
#endif
        if (point.x() < 0 || point.y() < 0)
            return false;

#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
        viewImpl->setFocusManually();
#endif
        focusedPoint = viewImpl->transformFromScene().mapPoint(point);
        WebHitTestResult::Data hitTestResultData = viewImpl->page()->hitTestResultAtPoint(focusedPoint, WebHitTestResult::HitTestModeDefault | WebHitTestResult::HitTestModeSetFocus);
        viewImpl->page()->raiseTapEvent(focusedPoint);
        if (viewImpl->page()->isViewVisible() && !(hitTestResultData.focusedRects.isEmpty())) {
            // on selecting the item on webpage need to clear the screen reader rect, because new rect should initialize after this action.
            // ex: clicking on link,iframe etc
            viewImpl->page()->clearScreenReaderFocus();
            screenReader().m_focusRing->hide(false);
        }
        break;
    }

    case ELM_ACCESS_ACTION_SCROLL:
        if (ewk_view_vertical_panning_hold_get(viewImpl->view()))
            return false;

    case ELM_ACCESS_ACTION_MOUSE: {
        Ewk_Event_Gesture gestureEvent;
        memset(&gestureEvent, 0, sizeof(Ewk_Event_Gesture));
        gestureEvent.type = EWK_GESTURE_PAN;
        gestureEvent.position.x = actionInfo->x;
        gestureEvent.position.y = actionInfo->y;
        gestureEvent.scale = 1;
        gestureEvent.count = 1;
        gestureEvent.timestamp = ecore_time_get() * 1000;

        Eina_List* touchPointList = 0;
        Ewk_Touch_Point* touchPoint = new Ewk_Touch_Point;
        touchPoint->id = 0;
        touchPoint->x = actionInfo->x;
        touchPoint->y = actionInfo->y;

        const Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(viewImpl->view()));

        if (actionInfo->mouse_type == 0) {
            touchPoint->state = EVAS_TOUCH_POINT_DOWN;
            touchPointList = eina_list_append(touchPointList, touchPoint);
            ewk_view_feed_touch_event(viewImpl->view(), EWK_TOUCH_START, touchPointList, 0);

            if (smartData && smartData->api && smartData->api->gesture_start)
                smartData->api->gesture_start(const_cast<Ewk_View_Smart_Data*>(smartData), &gestureEvent);

            screenReader().m_gestureStarted = true;
        } else if (actionInfo->mouse_type == 1) {
            touchPointList = eina_list_append(touchPointList, touchPoint);

            if (!screenReader().m_gestureStarted) {
                touchPoint->state = EVAS_TOUCH_POINT_DOWN;
                ewk_view_feed_touch_event(viewImpl->view(), EWK_TOUCH_START, touchPointList, 0);

                if (smartData && smartData->api && smartData->api->gesture_start)
                    smartData->api->gesture_start(const_cast<Ewk_View_Smart_Data*>(smartData), &gestureEvent);

                screenReader().m_gestureStarted = true;
            }

            touchPoint->state = EVAS_TOUCH_POINT_MOVE;
            ewk_view_feed_touch_event(viewImpl->view(), EWK_TOUCH_MOVE, touchPointList, 0);

            if (smartData && smartData->api && smartData->api->gesture_move)
                smartData->api->gesture_move(const_cast<Ewk_View_Smart_Data*>(smartData), &gestureEvent);
        } else if (actionInfo->mouse_type == 2) {
            touchPoint->state = EVAS_TOUCH_POINT_UP;
            touchPointList = eina_list_append(touchPointList, touchPoint);
            ewk_view_feed_touch_event(viewImpl->view(), EWK_TOUCH_END, touchPointList, 0);

            if (smartData && smartData->api && smartData->api->gesture_end)
                smartData->api->gesture_end(const_cast<Ewk_View_Smart_Data*>(smartData), &gestureEvent);

            screenReader().m_gestureStarted = false;
        }

        if (touchPointList) {
            void* item;
            EINA_LIST_FREE(touchPointList, item)
                delete static_cast<Ewk_Touch_Point*>(item);
        } else
            delete static_cast<Ewk_Touch_Point*>(touchPoint);

        break;
    }

    case ELM_ACCESS_ACTION_UP:
        viewImpl->page()->adjustScreenReaderFocusedObjectValue(true);
        break;

    case ELM_ACCESS_ACTION_DOWN:
        viewImpl->page()->adjustScreenReaderFocusedObjectValue(false);
        break;

    case ELM_ACCESS_ACTION_BACK:
        if (!viewImpl->pageProxy->canGoBack())
            return false;

        viewImpl->pageProxy->goBack();
        break;

    case ELM_ACCESS_ACTION_OVER:
    case ELM_ACCESS_ACTION_READ: {
        screenReader().m_screenReaderPoint = IntPoint(actionInfo->x, actionInfo->y);
        IntPoint localPoint = viewImpl->transformFromScene().mapPoint(IntPoint(actionInfo->x, actionInfo->y));
        viewImpl->page()->moveScreenReaderFocusByPoint(localPoint, (actionInfo->action_type == ELM_ACCESS_ACTION_READ));
        break;
    }

    default:
        break;
    }

    return true;
}

void ScreenReaderProxy::didTTSStateChanged(tts_h, tts_state_e, tts_state_e currentState, void*)
{
    if (currentState != TTS_STATE_READY || screenReader().m_ttsText.isEmpty())
        return;

    int uttID;
    if (tts_add_text(screenReader().m_ttsHandle, screenReader().m_ttsText.utf8().data(), 0, TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &uttID)
        || tts_play(screenReader().m_ttsHandle)) {
        screenReader().initializeTTS(true);
        return;
    }

    screenReader().m_ttsText = emptyString();
}

} // namespace WebKit

#endif // ENABLE(TIZEN_SCREEN_READER)
