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

#include "config.h"
#include "Flick.h"

#include "EasingUtilities.h"
#include "PageClientImpl.h"
#include "WKAPICast.h"
#include "WKPageTizen.h"
#include "ewk_view_private.h"

#if ENABLE(TIZEN_PROD_CHANNEL_SCROLL)
#include <wkext.h>
#endif

using namespace WebCore;
using namespace WebKit;

const double Flick::s_scrollSpeedFactor = 10;
const float Flick::s_decelerationRate = log(0.78) / log(0.9);
const float Flick::s_inflexion = 0.35; // Tension lines cross at (INFLEXION, 1)
const float Flick::s_flickFriction = 0.015; // The coefficient of friction applied to flick/scrolls
const float Flick::s_gravityEarth = 9.80665; //  The acceleration of gravity

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
const int Flick::s_lowScaleOnlyStrightPixel = 8500;
const int Flick::s_lowScaleOnlyDiagonalPixel = 3000;
const int Flick::s_lowScaleOnlyWidthLimit = 800;
const int Flick::s_lowScaleOnlyHeightLimit = 9000;
const int Flick::s_lowScaleOnlyCancelInterval = 20;
#endif

Flick::Flick(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
    , m_velocity()
    , m_flickIndex(0)
    , m_flickAnimator(0)
    , m_scrollAnimator(0)
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_checkContinuingScrollTimer(RunLoop::current(), this, &Flick::checkContinuingScrollTimerFired)
#endif
{
}

Flick::~Flick()
{
    if (m_flickAnimator) {
        ecore_animator_del(m_flickAnimator);
        m_flickAnimator = 0;
    }

    if (m_scrollAnimator) {
        ecore_animator_del(m_scrollAnimator);
        m_scrollAnimator = 0;
    }
}

void Flick::start(IntPoint velocity)
{
    stop();

    if (!velocity.x() && !velocity.y()) {
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
        evas_object_smart_callback_call(m_viewImpl->view(), "flick,canceled", 0);
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
        EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);
        pageClientImpl->setUpdateLowScaleLayerOnly(false);
#endif
        return;
    }

    m_velocity = velocity;
#if ENABLE(TIZEN_PROD_CHANNEL_SCROLL)
    Evas_Coord_Point point;
    point.x = m_velocity.x();
    point.y = m_velocity.y();
    wkext_channel_scroll_adjust_flick(&point);
    m_velocity.setX(point.x);
    m_velocity.setY(point.y);
#endif

    m_flickIndex = 0;
    // Set flick duration to process flick during getFlickDuration(sec).
    m_flickDuration = 1 / ecore_animator_frametime_get() * getFlickDuration(m_velocity);
    m_flickAnimator = ecore_animator_add(flickAnimatorCallback, this);
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    // Enable Dash Mode when m_flickDuration is over 40frame (2/3sec)
    if (m_flickDuration > 40) {
        int dashModeTime = 1000;
        // If flick time is over 1sec, set the dash mode time to 2sec
        if (m_flickDuration > 60)
            dashModeTime = 2000;

        WTF::setWebkitDashMode(WTF::DashModeBrowserJavaScript, dashModeTime);
    }
#endif

    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);
    pageClientImpl->setUpdateViewportWithAnimatorEnabled(true);

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    checkCoverWithTilesForLowScaleLayerOnly(m_velocity);
#endif
}

void Flick::stop()
{
    if (m_flickAnimator) {
        ecore_animator_del(m_flickAnimator);
        m_flickAnimator = 0;

#if ENABLE(TIZEN_EDGE_SUPPORT)
        m_viewImpl->edgeEffect()->hide();
#endif

        PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
        EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);
        pageClientImpl->setUpdateViewportWithAnimatorEnabled(false);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (m_viewImpl->textSelection()->isTextSelectionMode())
            m_viewImpl->textSelection()->updateVisible(true);
#endif
    }

    if (m_scrollAnimator) {
        ecore_animator_del(m_scrollAnimator);
        m_scrollAnimator = 0;
    }
}

Eina_Bool Flick::flickAnimatorCallback(void* data)
{
    return static_cast<Flick*>(data)->process();
}

bool Flick::process()
{
    FloatPoint delta = FloatPoint(m_velocity);
    // Multiply frametime to the velocity because velocity is moved pixels during 1 second
    // and we want to get pixels to move during frametime.
    float multiplier = -1 * ecore_animator_frametime_get() * easeInSine(m_flickDuration, m_flickIndex);
    delta.scale(multiplier, multiplier);
    int deltaX = (delta.x() > 0) ? ceilf(delta.x()) : floorf(delta.x());
    int deltaY = (delta.y() > 0) ? ceilf(delta.y()) : floorf(delta.y());

    m_flickIndex++;

    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    if (pageClientImpl)
        pageClientImpl->setUrlBarScrollTrigger(false);
#endif

    if (m_flickIndex > m_flickDuration || !(deltaX || deltaY)) {
        m_flickAnimator = 0;
        EINA_SAFETY_ON_NULL_RETURN_VAL(pageClientImpl, false);
        pageClientImpl->setUpdateViewportWithAnimatorEnabled(false);

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        pageClientImpl->setUpdateLowScaleLayerOnly(false);
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (m_viewImpl->textSelection()->isTextSelectionMode())
            m_viewImpl->textSelection()->updateVisible(true);
#endif
#if ENABLE(TIZEN_EDGE_SUPPORT)
        m_viewImpl->edgeEffect()->hide();
#endif
        return false;

    } else {
        EINA_SAFETY_ON_NULL_RETURN_VAL(pageClientImpl, true);
        ewkViewSendScrollEvent(m_viewImpl->view(), deltaX, deltaY, 0, 0);
        if (ewk_view_horizontal_panning_hold_get(m_viewImpl->view()))
            deltaX = 0;
        if (ewk_view_vertical_panning_hold_get(m_viewImpl->view()))
            deltaY = 0;

        IntPoint scrollPosition = pageClientImpl->scrollPosition();
        if (WKPageScrollBy(toAPI(m_viewImpl->page()), toAPI(IntSize(deltaX, deltaY))))
            ewkViewSendEdgeEvent(m_viewImpl->view(), scrollPosition, deltaX, deltaY);
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (m_viewImpl->textSelection()->isTextSelectionMode())
            m_viewImpl->textSelection()->updateVisible(false);
#endif

        // Stop flick if we reach the end of contents.
        int scrollX;
        int scrollY;
        int scrollWidth;
        int scrollHeight;
        ewk_view_scroll_pos_get(m_viewImpl->view(), &scrollX, &scrollY);
        ewk_view_scroll_size_get(m_viewImpl->view(), &scrollWidth, &scrollHeight);
        if ((!deltaX || (deltaX < 0 && scrollX == 0) || (deltaX > 0 && scrollX >= scrollWidth))
            && (!deltaY || (deltaY < 0 && scrollY == 0) || (deltaY > 0 && scrollY >= scrollHeight))) {
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
            // Enable flick in the overflow scroll area even if we reach the end of contents
            if (pageClientImpl->isScrollableLayerFocused() || pageClientImpl->isScrollableNodeFocused())
                return true;
#endif
            m_flickAnimator = 0;
            pageClientImpl->setUpdateViewportWithAnimatorEnabled(false);

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
            pageClientImpl->setUpdateLowScaleLayerOnly(false);
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
            if (m_viewImpl->textSelection()->isTextSelectionMode())
                m_viewImpl->textSelection()->updateVisible(true);
#endif
#if ENABLE(TIZEN_EDGE_SUPPORT)
            m_viewImpl->edgeEffect()->hide();
#endif
            return false;
        }

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        if (pageClientImpl->updateLowScaleLayerOnly()) {
            if ((m_flickDuration - m_flickIndex) < s_lowScaleOnlyCancelInterval) {
                pageClientImpl->setUpdateLowScaleLayerOnly(false);
            } else {
                m_checkContinuingScrollTimer.startOneShot(0.1);
            }
        }
#endif

        return true;
    }
}

void Flick::scrollTo(const IntPoint& destination)
{
    m_scrollToPosition = destination;
    m_scrollAnimator = ecore_animator_add(scrollToAnimatorCallback, this);
}

Eina_Bool Flick::scrollToAnimatorCallback(void* data)
{
    return static_cast<Flick*>(data)->scrollToAnimator();
}

Eina_Bool Flick::scrollToAnimator()
{
    int scrollX, scrollY;
    ewk_view_scroll_pos_get(m_viewImpl->view(), &scrollX, &scrollY);
    int remainX = m_scrollToPosition.x() - scrollX;
    int remainY = m_scrollToPosition.y() - scrollY;

    if (!remainX && !remainY) {
        if (m_scrollAnimator) {
            ecore_animator_del(m_scrollAnimator);
            m_scrollAnimator = 0;
        }
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (m_viewImpl->textSelection()->isTextSelectionMode())
            m_viewImpl->textSelection()->updateVisible(true);
#endif
        return false;
    }

    // Scroll speed becomes fast as scrollRatio set to large value.
    // Scroll speed is adaptive to the frame rate of animator, so the total time duration
    // to scroll to the destination position is almost same even for different devices.
    double scrollRatio = s_scrollSpeedFactor * ecore_animator_frametime_get();
    int deltaX = (remainX > 0) ? ceilf(remainX * scrollRatio) : floorf(remainX * scrollRatio);
    int deltaY = (remainY > 0) ? ceilf(remainY * scrollRatio) : floorf(remainY * scrollRatio);

    ewkViewSendScrollEvent(m_viewImpl->view(), deltaX, deltaY, 0, 0);
    if (WKPageScrollBy(toAPI(m_viewImpl->page()), toAPI(IntSize(deltaX, deltaY))))
        ewkViewSendEdgeEvent(m_viewImpl->view(), IntPoint(scrollX, scrollY), deltaX, deltaY);
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (m_viewImpl->textSelection()->isTextSelectionMode())
        m_viewImpl->textSelection()->updateVisible(false);
#endif
    return true;
}

double Flick::getSplineDeceleration(float velocity)
{
    // 0.84 means the value of look and feel tuning
    float physicalCoeff = computeDeceleration(0.84);

    return log(s_inflexion * abs(velocity) / (s_flickFriction * physicalCoeff));
}

float Flick::computeDeceleration(float friction)
{
    // 39.37 means inch/meter
    return s_gravityEarth * 39.37 * getDPI() * friction;
}

double Flick::getFlickDuration(IntPoint velocity)
{
    float finalVelocity = sqrt(velocity.x() * velocity.x() + velocity.y() * velocity.y());
    double l = getSplineDeceleration(finalVelocity);
    double decelMinusOne = s_decelerationRate - 1.0;

    return exp(l / decelMinusOne);
}

bool Flick::isWorking()
{
    return m_flickAnimator ? true : false;
}

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
void Flick::checkCoverWithTilesForLowScaleLayerOnly(IntPoint velocity)
{
    if (m_viewImpl->scaleFactor() <= 3)
        return;

    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);

    IntSize scaledContentsSize = m_viewImpl->page()->contentsSize();
    scaledContentsSize.scale(m_viewImpl->scaleFactor());

    if (pageClientImpl->updateLowScaleLayerOnly() ||
        (scaledContentsSize.width() < s_lowScaleOnlyWidthLimit || scaledContentsSize.height() < s_lowScaleOnlyHeightLimit))
        return;

    int velocityX = abs(velocity.x());
    int velocityY = abs(velocity.y());
    if ((velocityX > s_lowScaleOnlyDiagonalPixel && velocityY > s_lowScaleOnlyDiagonalPixel) ||
        (velocityX > s_lowScaleOnlyStrightPixel || velocityY > s_lowScaleOnlyStrightPixel)) {
        pageClientImpl->setUpdateLowScaleLayerOnly(true);
    }
}

void Flick::checkContinuingScrollTimerFired()
{
    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN(pageClientImpl);

    pageClientImpl->setUpdateLowScaleLayerOnly(false);
}
#endif
