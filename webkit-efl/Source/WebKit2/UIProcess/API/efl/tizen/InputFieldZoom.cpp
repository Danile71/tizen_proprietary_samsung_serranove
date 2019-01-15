/*
 * Copyright (C) 2013 Samsung Electronics All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
#include <WebCore/EflScreenUtilities.h>

#include "InputFieldZoom.h"
#include "EwkViewImpl.h"

using namespace WebCore;

namespace WebKit {

static const int s_numberOfCosineValue = 17;
static const int s_inputFieldWidthMargin = 4;
static const float s_cosine[s_numberOfCosineValue] =
{ 1.0f, 0.99f, 0.96f, 0.93f, 0.88f, 0.82f, 0.75f, 0.67f, 0.59f, 0.5f, 0.41f, 0.33f, 0.25f, 0.18f, 0.12f, 0.07f, 0.01f };


static void viewRenderPreCallback(void* data, Evas*, void*)
{
    InputFieldZoom* inputFieldZoom = static_cast<InputFieldZoom*>(data);
    if (inputFieldZoom->isWorking()) {
        if (inputFieldZoom->m_isScaleFactorChanged)
            inputFieldZoom->m_isScaleFactorChanged = false;
        else
            inputFieldZoom->process();
    }
}

static Eina_Bool scaleAnimatorCallback(void* data)
{
    static_cast<InputFieldZoom*>(data)->m_isScaleFactorChanged = true;

    return static_cast<InputFieldZoom*>(data)->process();
}

static float readableScaleFactor()
{
    // Readable scale factor is dependent on device DPI
    static float readableScaleFactor = (float)getMobileDPI() / 160;
    return readableScaleFactor;
}

static float maximumReadableScaleFactor()
{
    static float maximumReadableScaleFactor = readableScaleFactor() + 0.5;
    return maximumReadableScaleFactor;
}

InputFieldZoom::InputFieldZoom(EwkViewImpl* viewImpl)
    :  m_isScaleFactorChanged(false)
    , m_viewImpl(viewImpl)
    , m_scaleAnimator(0)
    , m_scaleIndex(0)
    , m_targetScale(0)
    , m_isWorking(false)
    , m_timer(0)
    , m_needToStop(false)
{
    evas_event_callback_add(evas_object_evas_get(m_viewImpl->view()), EVAS_CALLBACK_RENDER_PRE, viewRenderPreCallback, this);
}

InputFieldZoom::~InputFieldZoom()
{
    if (m_scaleAnimator) {
        ecore_animator_del(m_scaleAnimator);
        m_scaleAnimator = 0;
    }

    if (m_timer) {
        ecore_timer_del(m_timer);
        m_timer = 0;
    }

    evas_event_callback_del_full(evas_object_evas_get(m_viewImpl->view()), EVAS_CALLBACK_RENDER_PRE, viewRenderPreCallback, this);
}

bool InputFieldZoom::calculateAnimationFactors()
{
    PageClientImpl* pageClientImpl = m_viewImpl->pageClient.get();
    EINA_SAFETY_ON_NULL_RETURN_VAL(pageClientImpl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(m_viewImpl->page(), false);

    const double currentScaleFactor = pageClientImpl->scaleFactor();
    double targetScaleFactor = readableScaleFactor();

    // If current scale factor is readable, keep this
    if (currentScaleFactor >= readableScaleFactor() && currentScaleFactor <= maximumReadableScaleFactor())
        targetScaleFactor = currentScaleFactor;

    // Readable zoom value should be inside of viewport scale range
    targetScaleFactor = pageClientImpl->adjustScaleWithViewport(targetScaleFactor);

    // caret position can be outside of visible rect.
    // we need to consider it.
    const IntRect caretRect = m_viewImpl->caretRect(false);
    const IntRect focusedNodeRect = m_viewImpl->focusedNodeRect(false);

    // Scaled means scaled by UIProcess scale factor.
    // These are comparable with the value of pageClientImpl->visibleContentRect()
    IntRect scaledCaretRect = caretRect;
    IntRect scaledFocusedNodeRect = focusedNodeRect;

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    scaledCaretRect.scale(targetScaleFactor);
    scaledFocusedNodeRect.setX((int)ceil(scaledFocusedNodeRect.x() * targetScaleFactor));
    scaledFocusedNodeRect.setY((int)ceil(scaledFocusedNodeRect.y() * targetScaleFactor));
    scaledFocusedNodeRect.setWidth((int)ceil(scaledFocusedNodeRect.width() * targetScaleFactor));
    scaledFocusedNodeRect.setHeight((int)ceil(scaledFocusedNodeRect.height() * targetScaleFactor));
#else
    scaledCaretRect.scale(targetScaleFactor / currentScaleFactor);
    scaledFocusedNodeRect.scale(targetScaleFactor / currentScaleFactor);
#endif

    const IntRect visibleRect = pageClientImpl->visibleContentRect();

    //In this scenario the caret Rect is outside the Focused Node Rect so overflow scroll is required to correct it.
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    if ((pageClientImpl->isScrollableNodeFocused() || pageClientImpl->isScrollableLayerFocused()) && (!focusedNodeRect.contains(caretRect))) {
        m_viewImpl->page()->scrollOverflow(FloatPoint(0, caretRect.y() - focusedNodeRect.y()));
        return false;
    }
#endif

    // If it's already readable, do not update scale factor
    if (visibleRect.contains(scaledCaretRect)
        && (currentScaleFactor >= readableScaleFactor() && currentScaleFactor <= maximumReadableScaleFactor())) {
        TIZEN_LOGI("Keep visible rect: [%f] [%d, %d, %d, %d]", currentScaleFactor, visibleRect.x(), visibleRect.y(), visibleRect.width(), visibleRect.height());
        return false;
    }

    // Adjust target rect with caret rect and focused node rect
    FloatRect targetRect = caretRect;
    targetRect.setX(focusedNodeRect.x());
    targetRect.setWidth(visibleRect.width() / targetScaleFactor);
    if (!targetRect.contains(caretRect))
        targetRect.setX(caretRect.x() - ((visibleRect.width() / targetScaleFactor) * 4) / 5);

    TIZEN_LOGI("target rect: [%d, %d, %d, %d]", targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height());

    // Calculate target rect for animation
    targetRect.inflateX(s_inputFieldWidthMargin);
    FloatRect viewportRect(FloatPoint(), m_viewImpl->page()->viewSize());
    m_targetScale = targetScaleFactor;
    FloatRect newContentsRect;

    newContentsRect = FloatRect(targetRect.center(), FloatSize(viewportRect.width() / m_targetScale, viewportRect.height() / m_targetScale));
    newContentsRect.move(-newContentsRect.width() / 2, -newContentsRect.height() / 2);
    newContentsRect.setLocation(pageClientImpl->boundContentsPositionAtScale(newContentsRect.location(), m_targetScale));

    FloatRect currentContentsRect = m_viewImpl->transformFromView().mapRect(viewportRect);
    m_baseRect = currentContentsRect;
    m_targetRect = newContentsRect;
    m_scaleIndex = s_numberOfCosineValue - 1;

    return true;
}

Eina_Bool InputFieldZoom::startInputFieldZoom(void *data)
{
    if (!data)
        return ECORE_CALLBACK_CANCEL;

    InputFieldZoom* object = (InputFieldZoom*)data;
    object->m_timer = 0;

    if (!object->calculateAnimationFactors()) {
        PageClientImpl* pageClientImpl = object->m_viewImpl->pageClient.get();
        EINA_SAFETY_ON_NULL_RETURN_VAL(pageClientImpl, ECORE_CALLBACK_CANCEL);

        pageClientImpl->setVisibleContentRect(pageClientImpl->visibleContentRect(), pageClientImpl->scaleFactor());
        pageClientImpl->displayViewport();

        return ECORE_CALLBACK_CANCEL;
    }

    // Enable m_isWorking
    object->m_isWorking = true;
    object->m_needToStop = false;

    if (object->m_scaleAnimator)
        ecore_animator_del(object->m_scaleAnimator);

    object->m_scaleAnimator = ecore_animator_add(scaleAnimatorCallback, object);
    object->process();

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    PageClientImpl* pageClientImpl = object->m_viewImpl->pageClient.get();
    if (pageClientImpl->isShowingAutoFillPopup())
        pageClientImpl->hideAutoFillPopup();
#endif

    return ECORE_CALLBACK_CANCEL;
}


bool InputFieldZoom::scheduleInputFieldZoom()
{
    // Schedule a timer to start input field zoom
    if (m_timer) {
        ecore_timer_del(m_timer);
        m_timer = 0;
    }

    m_timer = ecore_timer_add(0.1, startInputFieldZoom, this);
    return (bool)m_timer;
}

bool InputFieldZoom::process()
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(m_viewImpl->page(), false);

    if (m_needToStop) {
        if (m_scaleAnimator) {
            ecore_animator_del(m_scaleAnimator);
            m_scaleAnimator = 0;
        }
        m_isWorking = false;
        return true;
    }

    FloatRect rect;
    float multiplier = s_cosine[m_scaleIndex--];
    rect.setX(m_baseRect.x() + (m_targetRect.x() - m_baseRect.x()) * multiplier);
    rect.setY(m_baseRect.y() + (m_targetRect.y() - m_baseRect.y()) * multiplier);
    rect.setWidth(m_baseRect.width() + (m_targetRect.width() - m_baseRect.width()) * multiplier);
    rect.setHeight(m_baseRect.height() + (m_targetRect.height() - m_baseRect.height()) * multiplier);

    float scaleFactor = m_viewImpl->page()->viewSize().width() / rect.width();
    IntPoint scrollPosition(ceil(rect.x() * scaleFactor), ceil(rect.y() * scaleFactor));
    m_viewImpl->page()->scaleImage(scaleFactor, scrollPosition);

    if (m_scaleIndex < 0)
        stop();

    return true;
}

void InputFieldZoom::stop()
{
    if (!m_isWorking)
        return;

    EINA_SAFETY_ON_NULL_RETURN(m_viewImpl->page());
    float scaleFactor = m_viewImpl->page()->viewSize().width() / m_targetRect.width();
    IntPoint scrollPosition(ceil(m_targetRect.x() * scaleFactor), ceil(m_targetRect.y() * scaleFactor));
    m_viewImpl->page()->scale(scaleFactor, scrollPosition);
    m_needToStop = true;
}

bool InputFieldZoom::isWorking()
{
    return m_isWorking;
}
} // namespace WebKit
#endif // TIZEN_WEBKIT2_INPUT_FIELD_ZOOM
