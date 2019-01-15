/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

// FIXME: Use TIZEN_WEBKIT2_TILED_AC because we don't have COORDINATED_GRAPHICS yet.
#if ENABLE(TIZEN_WEBKIT2_TILED_AC) // #if USE(COORDINATED_GRAPHICS)

#include "ScrollingCoordinatorCoordinatedGraphics.h"

#include "Page.h"

namespace WebCore {

ScrollingCoordinatorCoordinatedGraphics::ScrollingCoordinatorCoordinatedGraphics(Page* page)
    : ScrollingCoordinator(page)
{
}

#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
void ScrollingCoordinatorCoordinatedGraphics::frameViewLayoutUpdated(FrameView*)
{
    ASSERT(m_page);

    Vector<IntRect> touchEventTargetRects;
    computeAbsoluteTouchEventTargetRects(m_page->mainFrame()->document(), touchEventTargetRects);
    setTouchEventTargetRects(touchEventTargetRects);
}

void ScrollingCoordinatorCoordinatedGraphics::touchEventTargetRectsDidChange(const Document*)
{
    // The rects are always evaluated and used in the main frame coordinates.
    FrameView* frameView = m_page->mainFrame()->view();
    Document* document = m_page->mainFrame()->document();

    // Wait until after layout to update.
    if (frameView->needsLayout() || !document)
        return;

    Vector<IntRect> touchEventTargetRects;
    computeAbsoluteTouchEventTargetRects(document, touchEventTargetRects);
    setTouchEventTargetRects(touchEventTargetRects);
}

void ScrollingCoordinatorCoordinatedGraphics::setTouchEventTargetRects(const Vector<IntRect>& absoluteHitTestRects)
{
    ASSERT(m_page);
    if (!m_client)
        return;
    m_client->setTouchEventTargetRects(absoluteHitTestRects);
}
#endif

} // namespace WebCore

#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC) // #endif // USE(COORDINATED_GRAPHICS)
