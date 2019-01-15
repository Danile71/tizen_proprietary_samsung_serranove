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

#ifndef GestureClient_h
#define GestureClient_h

#include "Flick.h"
#include "Pan.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include "Flick.h"
#include "SmartZoom.h"
#include "Zoom.h"

namespace WebCore {
class IntPoint;
}

class EwkViewImpl;

namespace WebKit {

class GestureClient {
public:
    static PassOwnPtr<GestureClient> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new GestureClient(viewImpl));
    }
    ~GestureClient();

    void reset();
    void setGestureEnabled(bool);
    void setTapEnabled(bool);
    void setMovingEnabled(bool);

    void stopGestures();
    void setTapScheduled(bool, WebCore::IntPoint);
    void setDoubleTapScheduled(bool, WebCore::IntPoint);
    void setPanScheduled(bool, const WebCore::IntPoint&);
    void setPinchScheduled(bool, const WebCore::IntPoint&, double);
    void setFlickScheduled(bool, WebCore::IntPoint, WebCore::IntPoint);

    void startTap(const WebCore::IntPoint&);
    void endTap(const WebCore::IntPoint&);
    void showContextMenu(const WebCore::IntPoint&);

    void startPan(const WebCore::IntPoint&);
    void endPan(const WebCore::IntPoint&);
    void movePan(const WebCore::IntPoint&);
    void startFlick(const WebCore::IntPoint&, const WebCore::IntPoint&);
    void endFlick(const WebCore::IntPoint&, const WebCore::IntPoint&);
    void startPinch(const WebCore::IntPoint&, double);
    void endPinch(const WebCore::IntPoint&, double);
    void movePinch(const WebCore::IntPoint&, double);
    void endDoubleTap(const WebCore::IntPoint&);
    void setZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&);
    bool scrollToWithAnimation(const int x, const int y);

    bool isGestureWorking();
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    void resetUrlBarTriggerPoint();
#endif

private:
    explicit GestureClient(EwkViewImpl*);

    bool m_isGestureEnabled;
    bool m_isTapEnabled;
    bool m_isMovingEnabled;

    bool m_isTapScheduled;
    bool m_isDoubleTapScheduled;
    bool m_isPanScheduled;
    bool m_isFlickScheduled;
    bool m_isPinchScheduled;

    bool m_canScheduleFlick;

    WebCore::IntPoint m_currentPosition;
    WebCore::IntPoint m_currentMomentum;
    WebCore::IntPoint m_currentPinchPosition;
    double m_currentPinchScale;

    EwkViewImpl* m_viewImpl;
    OwnPtr<Pan> m_pan;
    OwnPtr<Flick> m_flick;
    OwnPtr<SmartZoom> m_smartZoom;
    OwnPtr<Zoom> m_zoom;
};

} // namespace WebKit

#endif // GestureClient_h
