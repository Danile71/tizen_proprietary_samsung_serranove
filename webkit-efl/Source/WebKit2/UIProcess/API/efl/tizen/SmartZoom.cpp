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
#include "SmartZoom.h"

#include "EwkViewImpl.h"
#include "PageClientImpl.h"

#if ENABLE(TOUCH_ADJUSTMENT)
#include <Elementary.h>
#endif

using namespace WebCore;
using namespace WebKit;
using namespace std;

// Scale ratios are to present a natural behavior of smart zoom
// The delta value between scale ratios is increase almost lineary until the median value of the scale ratio number
// and decrease again almost lineray until the final value of the scale ratio number
const float SmartZoom::s_scaleRatio[s_numberOfScaleRatio] =
{ 1.0f, 0.97f, 0.93f, 0.89f, 0.84f, 0.78f, 0.71f, 0.63f, 0.54f, 0.45f, 0.37f, 0.29f, 0.22f, 0.16f, 0.11f, 0.07f, 0.03f };

SmartZoom::SmartZoom(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
    , m_scaleAnimator(0)
    , m_scaleIndex(0)
    , m_isWorking(false)
    , m_isScaleFactorChanged(false)
{
    evas_event_callback_add(evas_object_evas_get(m_viewImpl->view()), EVAS_CALLBACK_RENDER_PRE, viewRenderPreCallback, this);
}

SmartZoom::~SmartZoom()
{
    if (m_scaleAnimator) {
        ecore_animator_del(m_scaleAnimator);
        m_scaleAnimator = 0;
    }
    evas_event_callback_del_full(evas_object_evas_get(m_viewImpl->view()), EVAS_CALLBACK_RENDER_PRE, viewRenderPreCallback, this);
}

void SmartZoom::viewRenderPreCallback(void* data, Evas*, void*)
{
    SmartZoom* smartZoom = (static_cast<SmartZoom*>(data));
    if (smartZoom->isWorking()) {
        if (smartZoom->m_isScaleFactorChanged)
            smartZoom->m_isScaleFactorChanged = false;
        else
            smartZoom->process();
    }
}

void SmartZoom::start(int x, int y)
{
    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);
    EINA_SAFETY_ON_NULL_RETURN(m_viewImpl->page());

    IntPoint position = m_viewImpl->transformFromScene().mapPoint(IntPoint(x, y));
    IntSize fingerSize;
#if ENABLE(TOUCH_ADJUSTMENT)
    Evas_Coord size = elm_config_finger_size_get();
    fingerSize = IntSize(size, size);
#endif

    if (fabs(pageClientImpl->scaleFactor() - pageClientImpl->viewportConstraints().minimumScale) < numeric_limits<float>::epsilon())
        m_viewImpl->page()->findZoomableAreaForPoint(position, fingerSize);
    else
        setZoomableArea(position, IntRect(IntPoint(), m_viewImpl->page()->contentsSize()));

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    pageClientImpl->setUpdateLowScaleLayerOnly(false);
#endif
}

void SmartZoom::setZoomableArea(const IntPoint& target, const IntRect& area)
{
    static float s_maxScaleFactor = 0.0;
    static float s_scaleThreshold = 0.0;

    if (area.isEmpty())
        return;

    m_isWorking = true;

    // Notify that zoom is started.
    evas_object_smart_callback_call(m_viewImpl->view(), "zoom,started", 0);

    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);
    EINA_SAFETY_ON_NULL_RETURN(m_viewImpl->page());

    FloatRect targetRect = area;
    targetRect.inflateX(s_widthMargin);
    FloatRect viewportRect(FloatPoint(), m_viewImpl->page()->viewSize());
    float targetScale = pageClientImpl->adjustScaleWithViewport(viewportRect.width() / targetRect.width());
    FloatRect newContentsRect;

    if (m_viewImpl->page()->deviceScaleFactor() == 2.0) {
        s_maxScaleFactor = 4;
        s_scaleThreshold = 0.2;
    } else {
        s_maxScaleFactor = 3.5;
        s_scaleThreshold = 0.1;
    }

    if (targetScale > s_maxScaleFactor)
        targetScale = s_maxScaleFactor;
   // Scale to proper scale factor if target scale factor is same as current scale factor.
    // If width of area is same as width of contents and contents is fitted to the viewport now,
    // the current scale factor and target scale factor can be same.
    if (fabs(pageClientImpl->scaleFactor() - targetScale) < s_scaleThreshold) {
        targetScale = m_viewImpl->page()->deviceScaleFactor() * 1.33;
        newContentsRect = FloatRect(target, FloatSize(viewportRect.width() / targetScale, viewportRect.height() / targetScale));
    } else {
        newContentsRect = FloatRect(targetRect.center(), FloatSize(viewportRect.width() / targetScale, viewportRect.height() / targetScale));
        if (targetRect.height() > newContentsRect.height())
            newContentsRect.setY(target.y());
    }
    newContentsRect.move(-targetRect.width() / 2, -newContentsRect.height() / 2);
    newContentsRect.setLocation(pageClientImpl->boundContentsPositionAtScale(newContentsRect.location(), targetScale));

    FloatRect currentContentsRect = m_viewImpl->transformFromView().mapRect(viewportRect);
    m_baseRect = currentContentsRect;
    m_targetRect = newContentsRect;
    m_scaleIndex = s_numberOfScaleRatio - 1;

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)

#if ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
#if ENABLE(TIZEN_FULLSCREEN_API)
    if (!m_viewImpl->page()->fullScreenManager()->isFullScreen()) {
#endif
    IntPoint scrollPosition(m_targetRect.x() * targetScale, m_targetRect.y() * targetScale);
    m_viewImpl->page()->changeToMediumScaleImage(targetScale, scrollPosition);
#if ENABLE(TIZEN_FULLSCREEN_API)
    }
#endif
#endif

    if (m_scaleAnimator)
        ecore_animator_del(m_scaleAnimator);
    m_scaleAnimator = ecore_animator_add(scaleAnimatorCallback, this);
    process();
#else
    IntPoint scrollPosition(m_targetRect.x() * targetScale, m_targetRect.y() * targetScale);
    m_viewImpl->page()->scale(targetScale, scrollPosition);
#endif
}

void SmartZoom::stop()
{
    if (m_isWorking) {
        if (m_scaleAnimator) {
            ecore_animator_del(m_scaleAnimator);
            m_scaleAnimator = 0;
        }
        PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
        EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);
        EINA_SAFETY_ON_NULL_RETURN(m_viewImpl->page());

        float scaleFactor = m_viewImpl->page()->viewSize().width() / m_targetRect.width();
        IntPoint scrollPosition(m_targetRect.x() * scaleFactor, m_targetRect.y() * scaleFactor);
        m_viewImpl->page()->scale(scaleFactor, scrollPosition);

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        m_viewImpl->page()->setPaintOnlyLowScaleLayer(false);
#endif

        m_isWorking = false;

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (m_viewImpl->textSelection()->isTextSelectionMode())
            m_viewImpl->textSelection()->updateVisible(true);
#endif

        // Notify that zoom is finished.
        evas_object_smart_callback_call(m_viewImpl->view(), "zoom,finished", 0);
    }

}

Eina_Bool SmartZoom::scaleAnimatorCallback(void* data)
{
    static_cast<SmartZoom*>(data)->m_isScaleFactorChanged = true;

    return static_cast<SmartZoom*>(data)->process();
}

bool SmartZoom::process()
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(m_viewImpl->page(), false);

    FloatRect rect;
    float multiplier = s_scaleRatio[m_scaleIndex--];
    rect.setX(m_baseRect.x() + (m_targetRect.x() - m_baseRect.x()) * multiplier);
    rect.setY(m_baseRect.y() + (m_targetRect.y() - m_baseRect.y()) * multiplier);
    rect.setWidth(m_baseRect.width() + (m_targetRect.width() - m_baseRect.width()) * multiplier);
    rect.setHeight(m_baseRect.height() + (m_targetRect.height() - m_baseRect.height()) * multiplier);

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (m_targetRect.width() > m_baseRect.width() && m_scaleIndex < 12)
        m_viewImpl->page()->setPaintOnlyLowScaleLayer(true);
    else
        m_viewImpl->page()->setPaintOnlyLowScaleLayer(false);
#endif

    float scaleFactor = m_viewImpl->page()->viewSize().width() / rect.width();
    IntPoint scrollPosition(rect.x() * scaleFactor, rect.y() * scaleFactor);
    m_viewImpl->page()->scaleImage(scaleFactor, scrollPosition);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (m_viewImpl->textSelection()->isTextSelectionMode())
        m_viewImpl->textSelection()->updateVisible(false);
#endif

    if (m_scaleIndex < 0)
        stop();

    return true;
}

bool SmartZoom::isWorking()
{
    return m_isWorking;
}
