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

#ifndef FocusRing_h
#define FocusRing_h

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)

#include <Ecore.h>
#include <Evas.h>
#include <WebCore/IntPoint.h>
#include <WebCore/IntRect.h>
#include <wtf/PassOwnPtr.h>

class EwkViewImpl;

class FocusRing {
public:
    static PassOwnPtr<FocusRing> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new FocusRing(viewImpl));
    }
    ~FocusRing();

    void setImage(const String&, int, int);
    void setColor(const WebCore::Color color) { m_focusRingColor = color; }

    void requestToShow(const WebCore::IntPoint&, bool = false);
    void requestToHide(bool immediately = false);

    void show(const Vector<WebCore::IntRect>&);
    void showWithContextMenu(const WebCore::IntPoint&, const WebCore::Color&, const Vector<WebCore::IntRect>&);
    void didChangeVisibleContentRect(const WebCore::IntPoint&, float);
    void moveBy(const WebCore::IntSize&);

    void hide(bool = true);

    bool canUpdate() { return (m_state < FocusRingHideState); }
    bool rectsChanged(const Vector<WebCore::IntRect>&);
    WebCore::IntPoint centerPointInScreen();

    bool isShowTimerWorking() { return m_showTimer ? true : false; }

private:
    static const int s_showTimerTime = 100;
    static Eina_Bool showTimerCallback(void*);

    static const int s_hideTimerTime = 200;
    static Eina_Bool hideTimerCallback(void*);

private:
    FocusRing(EwkViewImpl*);

    void nodeRectAtPosition(Vector<WebCore::IntRect>&);
    bool ensureFocusRingObject();
    void adjustFocusRingObject(const WebCore::IntRect&, const WebCore::IntPoint&, const WebCore::IntRect&);

    EwkViewImpl* m_viewImpl;

    Evas_Object* m_focusRingObject;

    String m_imagePath;
    int m_imageOuterWidth;
    int m_imageInnerWidth;

    Ecore_Timer* m_showTimer;
    Ecore_Timer* m_hideTimer;
    WebCore::IntPoint m_position;

    Vector<WebCore::IntRect> m_rects;
    WebCore::Color m_focusRingColor;

    enum FocusRingState {
        FocusRingShowState,
        FocusRingWaitRectState,
        FocusRingWaitRectWithContextMenuState,
        FocusRingHideState,
        FocusRingWaitHideKeypadState
    };
    FocusRingState m_state;

    // Incase node is only image and link/anchor is not associated with it
    bool m_isImageWithNoLink;
};

#endif // #if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)

#endif // FocusRing_h
