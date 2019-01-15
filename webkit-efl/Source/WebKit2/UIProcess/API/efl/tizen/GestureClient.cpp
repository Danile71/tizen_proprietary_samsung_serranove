/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "GestureClient.h"

#include "EwkViewImpl.h"
#include "NativeWebMouseEvent.h"
#include "PageClientImpl.h"
#include "WebEvent.h"
#include "ewk_view_private.h"
#include <Evas.h>
#include <WebCore/IntPoint.h>

#if ENABLE(TOUCH_ADJUSTMENT)
#include <Elementary.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_ZOOM)
#include "WebPageGroup.h"
#include "WebPreferences.h"
#endif

#if ENABLE(TIZEN_PROD_CHANNEL_SCROLL)
#include <wkext.h>
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
#include "LinkMagnifierProxy.h"
#endif

using namespace WebCore;
using std::numeric_limits;

namespace WebKit {

GestureClient::GestureClient(EwkViewImpl* viewImpl)
    : m_isGestureEnabled(false)
    , m_isTapEnabled(false)
    , m_isMovingEnabled(false)
    , m_isTapScheduled(false)
    , m_isDoubleTapScheduled(false)
    , m_isPanScheduled(false)
    , m_isFlickScheduled(false)
    , m_isPinchScheduled(false)
    , m_canScheduleFlick(false)
    , m_viewImpl(viewImpl)
    , m_pan(Pan::create(viewImpl))
    , m_flick(Flick::create(viewImpl))
    , m_smartZoom(SmartZoom::create(viewImpl))
    , m_zoom(Zoom::create(viewImpl))
{
}

GestureClient::~GestureClient()
{
}

void GestureClient::reset()
{
    // reset all variables.
    m_isGestureEnabled = false;
    m_isTapEnabled = false;
    m_isMovingEnabled = false;
    m_isPinchScheduled = false;

    m_canScheduleFlick = false;
    stopGestures();
}

void GestureClient::setGestureEnabled(bool enabled)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] enabled[%d]", enabled);
#endif
    m_isGestureEnabled = enabled;

    if (m_isGestureEnabled && m_isDoubleTapScheduled)
        endDoubleTap(m_currentPosition);

    setDoubleTapScheduled(false, m_currentPosition);
}

void GestureClient::setTapEnabled(bool enabled)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] enabled[%d]", enabled);
#endif
    m_isTapEnabled = enabled;

    if (m_isGestureEnabled && m_isTapEnabled && m_isTapScheduled)
        endTap(m_currentPosition);

    // We have to reset TapScheduled here whether isTapEnabled is true or not.
    setTapScheduled(false, m_currentPosition);
}

void GestureClient::setMovingEnabled(bool enabled)
{
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (enabled) {
        PageClientImpl* pageClientImpl = ewkViewGetPageClient(m_viewImpl->view());
        pageClientImpl->setLowScaleLayerFlag(true);
    }
#endif

#if ENABLE(TIZEN_DLOG_SUPPORT)
    if (m_isMovingEnabled != enabled)
        TIZEN_LOGI("[Touch] enabled[%d]", enabled);
#endif
    m_isMovingEnabled = enabled;

    if (m_isGestureEnabled && m_isMovingEnabled) {
        if (m_isPanScheduled)
            startPan(m_currentPosition);
        else if (m_isFlickScheduled)
            startFlick(m_currentPosition, m_currentMomentum);
        else if (m_isPinchScheduled) {
            movePinch(m_currentPinchPosition, m_currentPinchScale);

            Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_viewImpl->view()));
            if (evas_touch_point_list_count(smartData->base.evas) < 2)
                endPinch(m_currentPinchPosition, m_currentPinchScale);
        }
        else if (m_canScheduleFlick)
            m_pan->invokeFlick();
    } else
        stopGestures();

    setFlickScheduled(false, m_currentPosition, IntPoint::zero());
    setPanScheduled(false, m_currentPosition);
    setPinchScheduled(false, m_currentPinchPosition, 0);
    m_canScheduleFlick = false;
}

void GestureClient::stopGestures()
{
    m_pan->stop(false);
    m_flick->stop();
    m_zoom->stop();
}

void GestureClient::setTapScheduled(bool scheduled, IntPoint position)
{
    if (m_isTapScheduled == scheduled)
        return;

    m_isTapScheduled = scheduled;
    m_currentPosition = position;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] scheduled[%d]", scheduled);
#endif
}

void GestureClient::setDoubleTapScheduled(bool scheduled, IntPoint position)
{
    if (m_isDoubleTapScheduled == scheduled)
        return;

    m_isDoubleTapScheduled = scheduled;
    m_currentPosition = position;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] scheduled[%d]", scheduled);
#endif
}

void GestureClient::setPanScheduled(bool scheduled, const IntPoint& position)
{
    if (m_isPanScheduled == scheduled)
        return;

    m_isPanScheduled = scheduled;
    m_currentPosition = position;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] scheduled[%d]", scheduled);
#endif
}

void GestureClient::setPinchScheduled(bool scheduled, const IntPoint& position, double scale)
{
    m_isPinchScheduled = scheduled;
    m_currentPinchPosition = position;
    m_currentPinchScale = scale;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] scheduled[%d]", scheduled);
#endif
}

void GestureClient::setFlickScheduled(bool scheduled, IntPoint position, IntPoint momentum)
{
    if (m_isFlickScheduled == scheduled)
        return;

    m_isFlickScheduled = scheduled;
    m_currentPosition = position;
    m_currentMomentum = momentum;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] scheduled[%d]", scheduled);
#endif
}

void GestureClient::startTap(const IntPoint& position)
{
#if ENABLE(TIZEN_PROD_CHANNEL_SCROLL)
    Evas_Coord_Point point;
    point.x = position.x();
    point.y = position.y();

    wkext_channel_scroll_initialize(&point);
#endif
    m_currentPosition = position;
    setTapScheduled(false, position);
    setDoubleTapScheduled(false, position);
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
}

void GestureClient::endTap(const IntPoint& position)
{
    if (!m_isTapEnabled) {
        setTapScheduled(true, position);
        return;
    }

    if (!m_isGestureEnabled)
        return;

    PageClientImpl* pageClientImpl = ewkViewGetPageClient(m_viewImpl->view());
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);

    Evas_Point viewPosition = {0, 0};
    evas_object_geometry_get(m_viewImpl->view(), &viewPosition.x, &viewPosition.y, 0, 0);
    IntPoint tapPosition = m_viewImpl->transformFromScene().mapPoint(position);
    unsigned int timestamp = ecore_time_get() * 1000;
    IntSize fingerSize;

#if ENABLE(TOUCH_ADJUSTMENT)
    Evas_Coord size = elm_config_finger_size_get();
    fingerSize = IntSize(size, size);
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    double scaleFactor = pageClientImpl->scaleFactor();
    if (scaleFactor - 1 > numeric_limits<float>::epsilon())
        fingerSize.scale(1 / scaleFactor, 1 / scaleFactor);
#endif
#endif

#if ENABLE(GESTURE_EVENTS)
#if ENABLE(TIZEN_LINK_MAGNIFIER)
    const float linkMagnifierBaseScaleFactor = (float)getMobileDPI() / 160;
    float currentScaleFactor = pageClientImpl->scaleFactor();
    if (ewk_settings_extra_feature_get(ewk_view_settings_get(m_viewImpl->view()), "link,magnifier")
        && currentScaleFactor < linkMagnifierBaseScaleFactor
        && currentScaleFactor < pageClientImpl->viewportConstraints().maximumScale
        &&!LinkMagnifierProxy::linkMagnifier().isClickedExactly()) {
        m_viewImpl->page()->getLinkMagnifierRect(tapPosition);
        return;
    }
#endif
    WebGestureEvent gesture(WebEvent::GestureSingleTap, tapPosition, IntPoint(tapPosition.x() + viewPosition.x, tapPosition.y() + viewPosition.y), WebEvent::Modifiers(0), timestamp, fingerSize, FloatPoint(0, 0));
    m_viewImpl->page()->handleGestureEvent(gesture);
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
#endif
}

void GestureClient::showContextMenu(const IntPoint& position)
{
    if (!m_isGestureEnabled)
        return;

    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);

    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_viewImpl->view()));
    if (!smartData || !smartData->api || !smartData->api->mouse_down)
        return;

    Evas* evas = evas_object_evas_get(m_viewImpl->view());

    // Send mouse down event to show context menu.
    // We do not need to send a corresponding mouse release because in case of
    // right-click, the context menu takes capture and consumes all events.
    Evas_Event_Mouse_Down mouseDown;
    mouseDown.button = 3;
    mouseDown.output.x = mouseDown.canvas.x = position.x();
    mouseDown.output.y = mouseDown.canvas.y = position.y();
    mouseDown.data = 0;
    mouseDown.modifiers = const_cast<Evas_Modifier*>(evas_key_modifier_get(evas));
    mouseDown.locks = const_cast<Evas_Lock*>(evas_key_lock_get(evas));
    mouseDown.flags = EVAS_BUTTON_NONE;
    mouseDown.timestamp = ecore_time_get() * 1000;
    mouseDown.event_flags = EVAS_EVENT_FLAG_NONE;
    mouseDown.dev = 0;
    smartData->api->mouse_down(smartData, &mouseDown);
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
}

void GestureClient::startPan(const IntPoint& position)
{
    if (!m_isMovingEnabled) {
        setPanScheduled(true, position);
        return;
    }

    m_pan->start(position);
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
}

void GestureClient::endPan(const IntPoint& position)
{
    if (!m_isMovingEnabled && m_isPanScheduled && !m_pan->isWorking()) {
        m_canScheduleFlick = true;
    }

    setPanScheduled(false, position);
    m_pan->stop();
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
}

void GestureClient::movePan(const IntPoint& position)
{
    if (!m_isMovingEnabled && m_isPanScheduled)
        m_currentPosition = position;

    m_pan->update(position);
}

void GestureClient::startFlick(const IntPoint& position, const IntPoint& velocity)
{
    if (!m_isMovingEnabled) {
        setFlickScheduled(true, position, velocity);
        return;
    }

    m_flick->start(velocity);
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
}

void GestureClient::endFlick(const IntPoint& position, const IntPoint& velocity)
{
    m_flick->stop();
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
}

void GestureClient::startPinch(const IntPoint& position, double scale)
{
    if (!m_isMovingEnabled)
        return;

    if (m_flick->isWorking())
        m_flick->stop();

    int viewX, viewY;
    evas_object_geometry_get(m_viewImpl->view(), &viewX, &viewY, 0, 0);
    m_zoom->start(position, IntPoint(viewX, viewY), scale);
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
}

void GestureClient::endPinch(const IntPoint& position, double scale)
{
    m_zoom->stop();
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
}

void GestureClient::movePinch(const IntPoint& position, double scale)
{
    if (!m_isGestureEnabled || !m_isMovingEnabled) {
        setPinchScheduled(true, position, scale);
        return;
    }

    m_zoom->update(scale, position);
}

void GestureClient::endDoubleTap(const IntPoint& position)
{
    if (!m_isGestureEnabled || !m_isTapEnabled) {
        setDoubleTapScheduled(true, position);
        return;
    }

    PageClientImpl* pageClientImpl = ewkViewGetPageClient(m_viewImpl->view());

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (ewk_settings_select_word_automatically_get(ewk_view_settings_get(m_viewImpl->view()))) {
        m_viewImpl->textSelection()->selectWordAutomatically(position);
        return;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_ZOOM)
    if (!pageClientImpl || m_viewImpl->page()->pageGroup()->preferences()->textZoomEnabled())
        return;
#endif

    if (pageClientImpl->userScalable()) {
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        pageClientImpl->setLowScaleLayerFlag(true);
#endif
        m_smartZoom->start(position.x(), position.y());
    }
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] position[%d, %d]", position.x(), position.y());
#endif
}

void GestureClient::setZoomableArea(const IntPoint& target, const IntRect& area)
{
    m_smartZoom->setZoomableArea(target, area);
}

bool GestureClient::scrollToWithAnimation(const int x, const int y)
{
    int scrollWidth, scrollHeight;
    ewk_view_scroll_size_get(m_viewImpl->view(), &scrollWidth, &scrollHeight);

    if (x < 0 || x > scrollWidth || y < 0 || y > scrollHeight)
        return false;

    m_flick->scrollTo(IntPoint(x, y));
    return true;
}

bool GestureClient::isGestureWorking()
{
    return m_pan->isWorking() || m_flick->isWorking() || m_smartZoom->isWorking() || m_zoom->isWorking();
}

#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
void GestureClient::resetUrlBarTriggerPoint()
{
    if (m_pan->isWorking())
        m_pan->resetUrlBarTriggerPoint();
}
#endif
} // namespace WebKit
