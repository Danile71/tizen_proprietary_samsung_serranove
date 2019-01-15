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

#ifndef SmartZoom_h
#define SmartZoom_h

#include <Ecore.h>
#include <Evas.h>
#include <WebCore/FloatRect.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {
class IntPoint;
class IntRect;
}

class EwkViewImpl;

class SmartZoom {
public:
    static PassOwnPtr<SmartZoom> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new SmartZoom(viewImpl));
    }
    ~SmartZoom();

    void start(int x, int y);
    void setZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&);
    void stop();

    bool isWorking();

private:
    static const int s_numberOfScaleRatio = 17;
    static const float s_scaleRatio[s_numberOfScaleRatio];
    static const int s_widthMargin = 4;

    static Eina_Bool scaleAnimatorCallback(void*);
    static void viewRenderPreCallback(void*, Evas*, void*);

private:
    explicit SmartZoom(EwkViewImpl*);
    bool process();

    EwkViewImpl* m_viewImpl;
    Ecore_Animator* m_scaleAnimator;
    WebCore::FloatRect m_baseRect;
    WebCore::FloatRect m_targetRect;
    int m_scaleIndex;
    bool m_isWorking;
    bool m_isScaleFactorChanged;
};

#endif // SmartZoom_h
