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
 /*
 * Copyright (C) 2006 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef Flick_h
#define Flick_h

#include <Ecore.h>
#include <Eina.h>
#include <Evas.h>
#include <WebCore/FloatPoint.h>
#include <WebCore/IntPoint.h>
#include <wtf/PassOwnPtr.h>

class EwkViewImpl;

class Flick {
public:
    static PassOwnPtr<Flick> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new Flick(viewImpl));
    }
    ~Flick();

    void start(WebCore::IntPoint velocity);
    void stop();
    void scrollTo(const WebCore::IntPoint& destination);

    bool isWorking();

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void checkCoverWithTilesForLowScaleLayerOnly(WebCore::IntPoint velocity);
    void checkContinuingScrollTimerFired();
#endif

private:
    static Eina_Bool flickAnimatorCallback(void*);
    static Eina_Bool scrollToAnimatorCallback(void* data);

    // Below 3 functions are referenced from Android framework to get the filck duraion
    // File name : android/frameworks/base/core/java/android/widget/Scroller.java
    // Oringinal function names in Android are added by comment
    double getSplineDeceleration(float velocity); // getSplineDeceleration()
    float computeDeceleration(float friction); // computeDeceleration()
    double getFlickDuration(WebCore::IntPoint velocity); // getSplineFlingDuration()

private:
    explicit Flick(EwkViewImpl*);
    bool process();
    Eina_Bool scrollToAnimator();

private:
    EwkViewImpl* m_viewImpl;
    WebCore::IntPoint m_velocity;
    int m_flickIndex;
    int m_flickDuration;
    Ecore_Animator* m_flickAnimator;
    Ecore_Animator* m_scrollAnimator;
    WebCore::IntPoint m_scrollToPosition;
    static const double s_scrollSpeedFactor; // Decide a scroll speed in scrollToAnimator()

    // Below variables are used for the referenced functions from Android framework
    // Original variable names in Android are added by comment
    static const float s_decelerationRate; // DECELERATION_RATE
    static const float s_inflexion; // INFLEXION
    static const float s_flickFriction; // mFlingFriction
    static const float s_gravityEarth; // GRAVITY_EARTH

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    static const int s_lowScaleOnlyStrightPixel;
    static const int s_lowScaleOnlyDiagonalPixel;
    static const int s_lowScaleOnlyWidthLimit;
    static const int s_lowScaleOnlyHeightLimit;
    static const int s_lowScaleOnlyCancelInterval;

    WebCore::RunLoop::Timer<Flick> m_checkContinuingScrollTimer;
#endif
};

#endif // Flick_h
