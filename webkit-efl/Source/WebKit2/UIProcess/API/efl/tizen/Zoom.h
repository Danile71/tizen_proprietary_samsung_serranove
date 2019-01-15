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

#ifndef Zoom_h
#define Zoom_h

#include <Ecore.h>
#include <Evas.h>
#include <WebCore/IntPoint.h>
#include <wtf/PassOwnPtr.h>

class EwkViewImpl;

class Zoom {
public:
    static PassOwnPtr<Zoom> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new Zoom(viewImpl));
    }
    ~Zoom();

    void start(const WebCore::IntPoint& centerPoint, const WebCore::IntPoint& viewLocation, const double zoom);
    void update(const double zoom, const WebCore::IntPoint& centerPoint);
    void stop();

    bool isWorking();

private:
    static Eina_Bool zoomAnimatorCallback(void*);
    static void viewRenderPreCallback(void*, Evas*, void*);

private:
    explicit Zoom(EwkViewImpl* viewImpl);
    void realStop();
    void process();

    EwkViewImpl* m_viewImpl;
    bool m_isStopped;
    bool m_isZooming;
    bool m_isZoomInformationUpdated;
    bool m_isScaleFactorChanged;
    double m_zoomRate;
    double m_zoomBeginningRate;
    double m_zoomBaseRate;
    double m_baseScaleFactor;
    double m_newScaleFactor;
    double m_currentScaleFactor;
    WebCore::IntPoint m_baseScrollPosition;
    WebCore::IntPoint m_newScrollPosition;
    WebCore::IntPoint m_basisPoint;
    WebCore::IntPoint m_centerPoint;
    WebCore::IntPoint m_viewLocation;
    Ecore_Animator* m_zoomAnimator;
#if !ENABLE(TIZEN_SUSPEND_PAINTING_DURING_ZOOMING) && ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
    bool m_needChangeToMediumScaleImage;
#endif
};

#endif // Zoom_h
