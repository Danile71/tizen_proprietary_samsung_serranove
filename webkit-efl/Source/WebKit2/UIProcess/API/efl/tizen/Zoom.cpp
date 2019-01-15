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
#include "Zoom.h"

#include "EwkViewImpl.h"
#include "PageClientImpl.h"

#if ENABLE(TIZEN_WEBKIT2_TEXT_ZOOM)
#include "WebPageGroup.h"
#include "WebPreferences.h"
#endif

using namespace WebCore;
using namespace WebKit;
using namespace std;

static IntPoint getNewScrollPosition(const IntPoint& scrollPosition, const IntPoint& basisPoint, double baseScaleFactor, double newScaleFactor)
{
    if (!baseScaleFactor)
        return IntPoint();

    double scaleDifference = newScaleFactor / baseScaleFactor;
    int newScrollX = (scrollPosition.x() + basisPoint.x()) * scaleDifference - basisPoint.x();
    int newScrollY = (scrollPosition.y() + basisPoint.y()) * scaleDifference - basisPoint.y();

    return IntPoint(newScrollX, newScrollY);
}

Zoom::Zoom(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
    , m_isZooming(false)
    , m_isZoomInformationUpdated(false)
    , m_isScaleFactorChanged(false)
    , m_zoomRate(1.0)
    , m_zoomBeginningRate(1.0)
    , m_zoomBaseRate(0)
    , m_baseScaleFactor(0)
    , m_newScaleFactor(0)
    , m_currentScaleFactor(0)
    , m_baseScrollPosition()
    , m_newScrollPosition()
    , m_basisPoint()
    , m_centerPoint()
    , m_viewLocation()
    , m_zoomAnimator(0)
#if !ENABLE(TIZEN_SUSPEND_PAINTING_DURING_ZOOMING) && ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
    , m_needChangeToMediumScaleImage(false)
#endif
{
    evas_event_callback_add(evas_object_evas_get(m_viewImpl->view()), EVAS_CALLBACK_RENDER_PRE, viewRenderPreCallback, this);
}

Zoom::~Zoom()
{
    if (m_zoomAnimator) {
        ecore_animator_del(m_zoomAnimator);
        m_zoomAnimator = 0;
    }
    evas_event_callback_del_full(evas_object_evas_get(m_viewImpl->view()), EVAS_CALLBACK_RENDER_PRE, viewRenderPreCallback, this);
}

void Zoom::viewRenderPreCallback(void* data, Evas*, void*)
{
    Zoom* zoom = (static_cast<Zoom*>(data));
    if (zoom->isWorking()) {
        if (zoom->m_isScaleFactorChanged)
            zoom->m_isScaleFactorChanged = false;
        else
            zoom->process();
    }
}

void Zoom::start(const IntPoint& centerPoint, const IntPoint& viewLocation, const double zoom)
{
    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);
    m_isZooming = true;
    m_isStopped = false;
    m_isZoomInformationUpdated = false;
    m_zoomRate = zoom;
    m_zoomBeginningRate = zoom;
    m_baseScaleFactor = pageClientImpl->scaleFactor();
    m_newScaleFactor = m_baseScaleFactor;
    m_zoomBaseRate = m_baseScaleFactor;
    m_currentScaleFactor = m_baseScaleFactor;
    m_newScrollPosition = m_baseScrollPosition = pageClientImpl->scrollPosition();
    m_centerPoint = centerPoint;
    int viewX, viewY;
    evas_object_geometry_get(m_viewImpl->view(), &viewX, &viewY, 0, 0);
    m_viewLocation = IntPoint(viewX, viewY);

    m_basisPoint.setX(centerPoint.x() - m_viewLocation.x());
    m_basisPoint.setY(centerPoint.y() - m_viewLocation.y());

    if (m_zoomAnimator)
        ecore_animator_del(m_zoomAnimator);
    m_zoomAnimator = ecore_animator_add(zoomAnimatorCallback, this);

#if ENABLE(TIZEN_SUSPEND_PAINTING_DURING_ZOOMING)
    if (pageClientImpl->userScalable() && !pageClientImpl->isContentSizeSameAsViewport())
        pageClientImpl->suspendContent();
#endif
    // Notify that zoom is started.
    evas_object_smart_callback_call(m_viewImpl->view(), "zoom,started", 0);

#if !ENABLE(TIZEN_SUSPEND_PAINTING_DURING_ZOOMING) && ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
    if (m_baseScaleFactor < 2)
#if ENABLE(TIZEN_FULLSCREEN_API)
        if (!m_viewImpl->page()->fullScreenManager()->isFullScreen())
#endif
        m_needChangeToMediumScaleImage = true;
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    pageClientImpl->setUpdateLowScaleLayerOnly(false);
#endif
}

void Zoom::stop()
{
    if (!m_isStopped) {
        m_isStopped = true;
        process();
    }
}

void Zoom::update(const double zoom, const IntPoint& centerPoint)
{
    if (!m_isZooming)
        start(centerPoint, IntPoint(), zoom);

    double scaleDiff = zoom - m_zoomRate;
    double newScaleFactor = m_newScaleFactor + (m_zoomBaseRate * (scaleDiff / m_zoomBeginningRate));
    m_zoomRate = zoom;

    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);
#if ENABLE(TIZEN_WEBKIT2_TEXT_ZOOM)
    if (m_viewImpl->page()->pageGroup()->preferences()->textZoomEnabled())
        newScaleFactor = m_baseScaleFactor * zoom;
    else {
        double adjustedScaleFactor = pageClientImpl->adjustScaleWithViewport(newScaleFactor);
        if (adjustedScaleFactor != newScaleFactor) {
            m_zoomBeginningRate = zoom;
            newScaleFactor = m_zoomBaseRate = adjustedScaleFactor;
        }
    }
#else
    double adjustedScaleFactor = pageClientImpl->adjustScaleWithViewport(newScaleFactor);
    if (adjustedScaleFactor != newScaleFactor) {
        m_zoomBeginningRate = zoom;
        newScaleFactor = m_zoomBaseRate = adjustedScaleFactor;
    }
#endif

    m_newScaleFactor = newScaleFactor;
    m_centerPoint = centerPoint;
    m_isZoomInformationUpdated = true;
}

void Zoom::realStop()
{
    if (m_isZooming) {
        if (m_zoomAnimator) {
            ecore_animator_del(m_zoomAnimator);
            m_zoomAnimator = 0;
        }
        m_isZooming = false;

        m_zoomBeginningRate = 1.0;
        m_zoomRate = 1.0;

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        m_viewImpl->page()->setPaintOnlyLowScaleLayer(false);
#endif

        PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
        m_viewImpl->page()->scaleImage(m_newScaleFactor, m_newScrollPosition);
        EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);

#if ENABLE(TIZEN_WEBKIT2_TEXT_ZOOM)
        if (m_viewImpl->page()->pageGroup()->preferences()->textZoomEnabled()) {
            double textScaleFactor = pageClientImpl->adjustScaleWithViewport(m_viewImpl->page()->textZoomFactor() * (m_newScaleFactor / m_baseScaleFactor));
            m_viewImpl->page()->scale(m_baseScaleFactor, m_baseScrollPosition);
            m_viewImpl->page()->setTextZoomFactor(textScaleFactor);
        } else
            m_viewImpl->page()->scale(m_newScaleFactor, m_newScrollPosition);
#endif

#if ENABLE(TIZEN_SUSPEND_PAINTING_DURING_ZOOMING)
        pageClientImpl->resumeContent();
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (m_viewImpl->textSelection()->isTextSelectionMode())
            m_viewImpl->textSelection()->updateVisible(true);
#endif
#if !ENABLE(TIZEN_SUSPEND_PAINTING_DURING_ZOOMING) && ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
        m_needChangeToMediumScaleImage = false;
#endif

    }

    // Notify that zoom is finished.
    evas_object_smart_callback_call(m_viewImpl->view(), "zoom,finished", 0);
}

Eina_Bool Zoom::zoomAnimatorCallback(void* data)
{
    static_cast<Zoom*>(data)->process();
    return ECORE_CALLBACK_RENEW;
}

void Zoom::process()
{
    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);

    if (m_isStopped) {
        IntPoint scrollPosition = pageClientImpl->scrollPosition();
        if (scrollPosition != m_newScrollPosition
            || fabs(m_currentScaleFactor - m_newScaleFactor) < numeric_limits<double>::epsilon()) {
            m_newScrollPosition = scrollPosition;
            realStop();
            return;
        }
    }

    if (!m_isZoomInformationUpdated)
        return;

    double scaleStep = m_currentScaleFactor / 6;
    if (fabs(m_currentScaleFactor - m_newScaleFactor) > scaleStep) {
        if (m_newScaleFactor > m_currentScaleFactor)
            m_currentScaleFactor += scaleStep;
        else
            m_currentScaleFactor -= scaleStep;
    } else
        m_currentScaleFactor = m_newScaleFactor;

    m_newScrollPosition = getNewScrollPosition(m_baseScrollPosition, m_basisPoint, m_baseScaleFactor, m_currentScaleFactor);
    m_newScrollPosition.setX(m_newScrollPosition.x() + m_basisPoint.x() - (m_centerPoint.x() - m_viewLocation.x()));
    m_newScrollPosition.setY(m_newScrollPosition.y() + m_basisPoint.y() - (m_centerPoint.y() - m_viewLocation.y()));

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if ( m_currentScaleFactor / m_baseScaleFactor < 0.84)
        m_viewImpl->page()->setPaintOnlyLowScaleLayer(true);
    else
        m_viewImpl->page()->setPaintOnlyLowScaleLayer(false);
#endif

#if !ENABLE(TIZEN_SUSPEND_PAINTING_DURING_ZOOMING) && ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
    if (m_currentScaleFactor > 1.2 && m_needChangeToMediumScaleImage) {
        const double midiumScale = 2;
        IntPoint tmpScrollPosition = getNewScrollPosition(m_baseScrollPosition, m_basisPoint, m_baseScaleFactor, midiumScale);
        m_viewImpl->page()->changeToMediumScaleImage(midiumScale, tmpScrollPosition);
        m_needChangeToMediumScaleImage = false;
    }
#endif

    m_viewImpl->page()->scaleImage(m_currentScaleFactor, m_newScrollPosition);
    m_newScrollPosition = pageClientImpl->scrollPosition();

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (m_viewImpl->textSelection()->isTextSelectionMode())
        m_viewImpl->textSelection()->updateVisible(false);
#endif

    if (fabs(m_currentScaleFactor - m_newScaleFactor) < numeric_limits<double>::epsilon())
        m_isZoomInformationUpdated = false;

    m_isScaleFactorChanged = true;
}

bool Zoom::isWorking()
{
    return m_isZooming;
}
