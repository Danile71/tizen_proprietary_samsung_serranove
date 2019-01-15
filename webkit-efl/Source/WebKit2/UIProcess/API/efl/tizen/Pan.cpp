/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Pan.h"

#include "EwkViewImpl.h"
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
#include "LayerTreeCoordinatorProxy.h"
#endif
#include "PageClientImpl.h"
#include "WKAPICast.h"
#include "WKPageTizen.h"
#include "ewk_view_private.h"

#if ENABLE(TIZEN_PROD_CHANNEL_SCROLL)
#include <wkext.h>
#endif

#if ENABLE(TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN) || ENABLE(TIZEN_BROWSER_DASH_MODE)
#include <TizenSystemUtilities.h>
#endif
using namespace WebCore;
using namespace WebKit;

Pan::Pan(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
    , m_panAnimator(0)
    , m_lastPoint()
    , m_currentPoint()
    , m_isScrollPositionChanged(false)
    , m_lastHistoryIndex(-1)
    , m_historyCount(0)
#if ENABLE(TIZEN_GESTURE_FEATURE)
    , m_smoothAlgorithm(SmoothAlgorithm::create())
#endif
    , m_isTheFirstPanAnimatorProcessed(false)
    , m_dataStoreTrigger(false)
#if ENABLE(TIZEN_ENABLE_MINIMUM_2_CPU_ON_MOUSE_DOWN)
    , m_enable2CpuJob(0)
#endif
#if ENABLE(TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN)
    , m_gpuWakeUpJob(0)
#endif
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    , m_urlBarStatus(0)
    , m_isUrlBarMoving(0)
    , m_urlBarOffeset(0)
    , m_urlBarTriggerPoint(0)
#endif
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    , m_isDashMode(0)
    , m_enableCpuDashMode(0)
    , m_panDashModeTimer(this, &Pan::panDashModeFired)
#endif
{
    evas_object_event_callback_add(m_viewImpl->view(), EVAS_CALLBACK_MOUSE_DOWN, onMouseDown, this);
    evas_object_event_callback_add(m_viewImpl->view(), EVAS_CALLBACK_MOUSE_MOVE, onMouseMove, this);
    evas_object_event_callback_add(m_viewImpl->view(), EVAS_CALLBACK_MULTI_DOWN, onMultiDown, this);
    evas_event_callback_add(evas_object_evas_get(m_viewImpl->view()), EVAS_CALLBACK_RENDER_PRE, viewRenderPreCallback, this);
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    evas_object_smart_callback_add(m_viewImpl->view(), "browser,urlbar,show", urlBarShowCallback, this);
    evas_object_smart_callback_add(m_viewImpl->view(), "browser,urlbar,hide", urlBarHideCallback, this);
    evas_object_smart_callback_add(m_viewImpl->view(), "urlbar,scroll,remain", urlBarScrollOffsetCallback, this);
#endif
}

Pan::~Pan()
{
    if (m_panAnimator)
        ecore_animator_del(m_panAnimator);
    evas_object_event_callback_del(m_viewImpl->view(), EVAS_CALLBACK_MOUSE_DOWN, onMouseDown);
    evas_object_event_callback_del(m_viewImpl->view(), EVAS_CALLBACK_MOUSE_MOVE, onMouseMove);
    evas_object_event_callback_del(m_viewImpl->view(), EVAS_CALLBACK_MULTI_DOWN, onMultiDown);
    evas_event_callback_del_full(evas_object_evas_get(m_viewImpl->view()), EVAS_CALLBACK_RENDER_PRE, viewRenderPreCallback, this);
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    evas_object_smart_callback_del(m_viewImpl->view(), "browser,urlbar,show", urlBarShowCallback);
    evas_object_smart_callback_del(m_viewImpl->view(), "browser,urlbar,hide", urlBarHideCallback);
    evas_object_smart_callback_del(m_viewImpl->view(), "urlbar,scroll,remain", urlBarScrollOffsetCallback);
#endif
#if ENABLE(TIZEN_ENABLE_MINIMUM_2_CPU_ON_MOUSE_DOWN)
    if (m_enable2CpuJob)
        ecore_job_del(m_enable2CpuJob);
#endif
#if ENABLE(TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN)
    if (m_gpuWakeUpJob)
        ecore_job_del(m_gpuWakeUpJob);
#endif
}

void Pan::viewRenderPreCallback(void* data, Evas*, void*)
{
    Pan* pan = (static_cast<Pan*>(data));
    if (pan->isWorking()) {
        if (pan->m_isScrollPositionChanged)
            pan->m_isScrollPositionChanged = false;
    }
}

void Pan::start(const IntPoint& point)
{
    if (m_panAnimator)
        return;

    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    pageClientImpl->findScrollableNode(point);
#endif
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    m_urlBarOffeset = 0;
    m_urlBarTriggerPoint = 0;
#endif

    ewkViewClearEdges(m_viewImpl->view());
    // Below statement means the update() was not called before start().
    if (m_lastPoint == IntPoint::zero())
        m_lastPoint = point;
    m_currentPoint = point;

    if (!m_dataStoreTrigger) {
        m_dataStoreTrigger = true;
        initializeHistory();
    }

    // Process pan directly when Pan is started for situation that Pan is delayed by TouchEvent.
    process();
    m_panAnimator = ecore_animator_add(panAnimatorCallback, this);
    m_isTheFirstPanAnimatorProcessed = false;
    pageClientImpl->setUpdateViewportWithAnimatorEnabled(true);

#if ENABLE(TIZEN_PROD_CHANNEL_SCROLL)
    Wkext_Channel_Event channelEvent;
    channelEvent.viewWidget = m_viewImpl->view();
    channelEvent.width = m_viewImpl->page()->contentsSize().width();
    channelEvent.height = m_viewImpl->page()->contentsSize().height();
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    channelEvent.width *= pageClientImpl->scaleFactor();
    channelEvent.height *= pageClientImpl->scaleFactor();
#endif

    Evas_Coord_Point currentPoint;
    currentPoint.x = point.x();
    currentPoint.y = point.y();

    wkext_channel_scroll_start_pan(&channelEvent, &currentPoint, m_viewImpl->page()->deviceScaleFactor());
#endif
}

void Pan::update(const IntPoint& point)
{
    if (!m_panAnimator)
        m_lastPoint = m_currentPoint;
    m_currentPoint = point;
}

void Pan::stop(bool doFlick)
{
    bool flickTrigger = false;
    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);

    if (m_panAnimator) {
        ecore_animator_del(m_panAnimator);
        m_panAnimator = 0;
        pageClientImpl->setUpdateViewportWithAnimatorEnabled(false);

        if (m_dataStoreTrigger && doFlick)
            flickTrigger = true;
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (!flickTrigger) {
            if (m_viewImpl->textSelection()->isTextSelectionMode())
                m_viewImpl->textSelection()->updateVisible(true);
        }
#endif
    }

    if (flickTrigger) {
        // Increase the velocity with 30%.
        startFlick(1.3);
        m_dataStoreTrigger = false;
    }

    m_lastPoint = m_currentPoint = IntPoint::zero();
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    stopPanDashMode();
#endif
}

void Pan::getVelocity(int* xVelocity, int* yVelocity)
{
    if (m_historyCount <= 1) {
        *xVelocity = 0;
        *yVelocity = 0;
        return;
    }

    // Get recent 5 history's velocity.
    int index1 = m_lastHistoryIndex;
    int index2 = m_historyCount >= 5 ? (m_lastHistoryIndex - 4) : (m_lastHistoryIndex - m_historyCount + 1);
    index2 = index2 < 0 ? index2 + s_maxSizeOfHistory : index2;
    double timeDifference = ecore_time_get() - m_history[index2].time - s_propagationTime;
    if (timeDifference < 0)
        timeDifference += s_propagationTime;
    *xVelocity = (m_history[index1].x - m_history[index2].x) / timeDifference;
    *yVelocity = (m_history[index1].y - m_history[index2].y) / timeDifference;
}

void Pan::startFlick(float multiplier)
{
    int xVelocity;
    int yVelocity;
    getVelocity(&xVelocity, &yVelocity);

    xVelocity *= multiplier;
    yVelocity *= multiplier;
    if (!xVelocity && !yVelocity)
        return;

    // Do not start flick if velocity is smaller than s_flickThreshold (500 pixel/second)
    xVelocity = abs(xVelocity) < s_flickThreshold ? 0 : xVelocity;
    yVelocity = abs(yVelocity) < s_flickThreshold ? 0 : yVelocity;

    // xVelocity and yVelocity should not over flickMaxVelocity (9000 pixel/second)
    if (abs(xVelocity) > s_flickMaxVelocity)
        xVelocity = xVelocity > 0 ? s_flickMaxVelocity : -1 * s_flickMaxVelocity;
    if (abs(yVelocity) > s_flickMaxVelocity)
        yVelocity = yVelocity > 0 ? s_flickMaxVelocity : -1 * s_flickMaxVelocity;

    const Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_viewImpl->view()));
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->gesture_start);

    Ewk_Event_Gesture gestureEvent;
    memset(&gestureEvent, 0, sizeof(Ewk_Event_Gesture));
    gestureEvent.type = EWK_GESTURE_FLICK;
    gestureEvent.position.x = m_currentPoint.x();
    gestureEvent.position.y = m_currentPoint.y();
    gestureEvent.velocity.x = xVelocity;
    gestureEvent.velocity.y = yVelocity;
    gestureEvent.scale = 1;
    gestureEvent.count = 1;
    gestureEvent.timestamp = ecore_time_get() * 1000;

    smartData->api->gesture_start(const_cast<Ewk_View_Smart_Data*>(smartData), &gestureEvent);
}

Eina_Bool Pan::panAnimatorCallback(void* data)
{
    Pan* pan = static_cast<Pan*>(data);

    // We have to skip first vsync animator in order to prevent
    // to draw two times within 16ms during GPU clock is low.
    if (!pan->m_isTheFirstPanAnimatorProcessed) {
        pan->m_isTheFirstPanAnimatorProcessed = true;
        return ECORE_CALLBACK_RENEW;
    }
    pan->process();

    return ECORE_CALLBACK_RENEW;
}

void Pan::process()
{
#if ENABLE(TIZEN_GESTURE_FEATURE)
    if (m_historyCount > 1) {
        int index1 = m_lastHistoryIndex;
        int index2 = m_lastHistoryIndex == 0 ? (s_maxSizeOfHistory - 1) : (m_lastHistoryIndex - 1);
        m_smoothAlgorithm->correctPoint(m_currentPoint,
                                        m_history[index1].x, m_history[index1].y, m_history[index1].time,
                                        m_history[index2].x, m_history[index2].y, m_history[index2].time);
    }
#endif
    int deltaX = m_lastPoint.x() - m_currentPoint.x();
    int deltaY = m_lastPoint.y() - m_currentPoint.y();

    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);

    if (!deltaX && !deltaY)
        return;

#if ENABLE(TIZEN_PROD_CHANNEL_SCROLL)
    wkext_channel_scroll_adjust_pan(deltaX, deltaY);
#endif

    int xVelocity;
    int yVelocity;
    getVelocity(&xVelocity, &yVelocity);
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    pageClientImpl->setUrlBarScrollTrigger(false);
    if (ewk_settings_extra_feature_get(m_viewImpl->settings(), "urlbar,hide")) {
        if (m_urlBarStatus && deltaY > 0) {
            m_isUrlBarMoving = true;
            pageClientImpl->setUrlBarScrollTrigger(true);
        } else if (m_urlBarStatus && deltaY < 0) {
            if (m_isUrlBarMoving)
                pageClientImpl->setUrlBarScrollTrigger(true);
            else if (m_urlBarOffeset != 0) {
                deltaY = m_urlBarOffeset;
                m_urlBarOffeset = 0;
            }
        } else if (!m_urlBarStatus && deltaY > 0) {
            if (m_isUrlBarMoving)
                pageClientImpl->setUrlBarScrollTrigger(true);
            else {
                if (m_urlBarOffeset != 0) {
                    deltaY = m_urlBarOffeset;
                    m_urlBarOffeset = 0;
                }
                if (!m_urlBarTriggerPoint)
                    m_urlBarTriggerPoint = m_currentPoint.y();
            }
        } else if (!m_urlBarStatus && deltaY < 0 && (m_currentPoint.y() >= m_urlBarTriggerPoint || m_urlBarTriggerPoint == 0)) {
            m_isUrlBarMoving = true;
            pageClientImpl->setUrlBarScrollTrigger(true);
        }

        if (m_urlBarOffeset != 0) {
            pageClientImpl->setUrlBarScrollTrigger(false);
            m_urlBarOffeset = 0;
        }
    }
#endif
    ewkViewSendScrollEvent(m_viewImpl->view(), deltaX, deltaY, xVelocity, yVelocity);
    if (ewk_view_horizontal_panning_hold_get(m_viewImpl->view()))
        deltaX = 0;
    if (ewk_view_vertical_panning_hold_get(m_viewImpl->view()))
        deltaY = 0;

    // Get scrollPosition before scrolling.
    IntPoint scrollPosition = pageClientImpl->scrollPosition();
    if (WKPageScrollBy(toAPI(m_viewImpl->page()), toAPI(IntSize(deltaX, deltaY))))
        ewkViewSendEdgeEvent(m_viewImpl->view(), scrollPosition, deltaX, deltaY);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (m_viewImpl->textSelection()->isTextSelectionMode())
        m_viewImpl->textSelection()->updateVisible(false);
#endif
    m_lastPoint = m_currentPoint;
    m_isScrollPositionChanged = true;
}

void Pan::initializeHistory()
{
    m_lastHistoryIndex = -1;
    m_historyCount = 0;
#if ENABLE(TIZEN_GESTURE_FEATURE)
    m_smoothAlgorithm->initialize();
#endif
}

void Pan::onMouseDown(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Pan* pan = static_cast<Pan*>(data);
    pan->m_dataStoreTrigger = true;
    pan->initializeHistory();

    Evas_Event_Mouse_Down* event = static_cast<Evas_Event_Mouse_Down*>(eventInfo);
    pan->storeDataToHistory(event->canvas.x, event->canvas.y, event->timestamp / 1000.0);
    pan->m_lastPoint = IntPoint(event->canvas.x, event->canvas.y);

#if ENABLE(TIZEN_ENABLE_MINIMUM_2_CPU_ON_MOUSE_DOWN)
    // Enable minimum 2 CPU here.
    // If we do not enable 2 CPU, other process can interrupt during panning,
    // and it will reduce the touch responsiveness.
    // We have to enable 2 CPU before waking up GPU in order to reduce consuming time for enabling GPU.
    pan->m_enable2CpuJob = ecore_job_add(enable2CpuJobCallback, data);
#endif
#if ENABLE(TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN)
    // Wakeup GPU here.
    // If we do not wake up GPU here, the GPU will wake up in the first rendering,
    // so first rendering will take much time and touch responsiveness will be bad.
    // usual GPU wake up time: 9 ms
    pan->m_gpuWakeUpJob = ecore_job_add(gpuWakeUpJobCallback, data);
#endif
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    pan->startPanDashMode();
#endif
}

void Pan::onMouseMove(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Move* event = static_cast<Evas_Event_Mouse_Move*>(eventInfo);
    Pan* pan = static_cast<Pan*>(data);
    if (pan->m_dataStoreTrigger)
        pan->storeDataToHistory(event->cur.canvas.x, event->cur.canvas.y, event->timestamp / 1000.0);
}

void Pan::onMultiDown(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    static_cast<Pan*>(data)->m_dataStoreTrigger = false;
}

void Pan::storeDataToHistory(int x, int y, double timestamp)
{
    int index = (m_lastHistoryIndex + 1) % s_maxSizeOfHistory;
    m_lastHistoryIndex = index;
    m_history[index].x = x;
    m_history[index].y = y;
    m_history[index].time = timestamp;
    if (m_historyCount < s_maxSizeOfHistory)
        m_historyCount++;
}

bool Pan::isWorking()
{
    return m_panAnimator ? true : false;
}

void Pan::invokeFlick()
{
    if (m_dataStoreTrigger) {
        // compensate velocity lose due to delay of receiving response.
        startFlick(1.8);
        m_dataStoreTrigger = false;
    }
}

#if ENABLE(TIZEN_ENABLE_MINIMUM_2_CPU_ON_MOUSE_DOWN)
void Pan::enable2CpuJobCallback(void* data)
{
    const char* parameters[1];
    char time[4] = "100"; // ms.
    parameters[0] = time;
    int result = invokeDbusMethodSync("org.tizen.system.deviced", "/Org/Tizen/System/DeviceD/PmQos",
                                         "org.tizen.system.deviced.PmQos", "BrowserScroll", "i", parameters);
    static_cast<Pan*>(data)->m_enable2CpuJob = 0;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] Enable minimum 2 CPU[%d]", result);
#endif
}
#endif

#if ENABLE(TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN)
void Pan::gpuWakeUpJobCallback(void* data)
{
    const char* parameters[1];
    char time[2] = "1"; // Use Default time (3000ms).
    parameters[0] = time;
    int result = invokeDbusMethodSync("org.tizen.system.deviced", "/Org/Tizen/System/DeviceD/PmQos",
                                      "org.tizen.system.deviced.PmQos", "GpuWakeup", "i", parameters);
    static_cast<Pan*>(data)->m_gpuWakeUpJob = 0;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Touch] Wake up GPU[%d]", result);
#endif
}
#endif

#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
void Pan::urlBarShowCallback(void *data, Evas_Object *, void *)
{
    Pan* pan = (static_cast<Pan*>(data));
    pan->m_urlBarStatus = true;
}

void Pan::urlBarHideCallback(void *data, Evas_Object *, void *)
{
    Pan* pan = (static_cast<Pan*>(data));
    pan->m_urlBarStatus = false;
    pan->m_isUrlBarMoving = false;
}

void Pan::urlBarScrollOffsetCallback(void *data, Evas_Object *, void *eventInfo)
{
    Pan* pan = (static_cast<Pan*>(data));
    pan->m_urlBarOffeset = *((int *)eventInfo);
    pan->m_isUrlBarMoving = false;
    if (pan->isWorking())
        pan->process();
}

void Pan::resetUrlBarTriggerPoint()
{
    m_urlBarTriggerPoint = 0;
}
#endif
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
void Pan::startPanDashMode()
{
    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    float scaleFactor = pageClientImpl->scaleFactor();

    if (pageClientImpl->userScalable() && scaleFactor > pageClientImpl->viewportConstraints().minimumScale)
        m_enableCpuDashMode = true;

    if (!m_panDashModeTimer.isActive()) {
        if (m_enableCpuDashMode)
            WTF::setWebkitDashMode(WTF::DashModeBrowserJavaScript, 3000);
        m_panDashModeTimer.startOneShot(2.9);
        m_isDashMode = true;
    }
}

void Pan::stopPanDashMode()
{
    m_isDashMode = false;
    m_enableCpuDashMode = false;
}

void Pan::panDashModeFired(WebCore::Timer<Pan>*)
{
    if (m_panAnimator && m_isDashMode) {
        if (m_enableCpuDashMode)
            WTF::setWebkitDashMode(WTF::DashModeBrowserJavaScript, 3000);
        m_panDashModeTimer.startOneShot(2.9);
    } else {
        m_isDashMode = false;
        m_enableCpuDashMode = false;
    }
}
#endif
