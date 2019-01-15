/*
 * Copyright (C) 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#if ENABLE(VIDEO)
#include "RenderMedia.h"

#include "HTMLMediaElement.h"
#include "RenderView.h"

#if ENABLE(TIZEN_FULLSCREEN_API)
#include "Chrome.h"
#include "Page.h"
#endif

namespace WebCore {

#if ENABLE(TIZEN_FULLSCREEN_API)
const FloatSize compareResolution(720.0f, 1280.0f);
#endif

RenderMedia::RenderMedia(HTMLMediaElement* video)
    : RenderImage(video)
{
    setImageResource(RenderImageResource::create());
}

RenderMedia::RenderMedia(HTMLMediaElement* video, const IntSize& intrinsicSize)
    : RenderImage(video)
{
    setImageResource(RenderImageResource::create());
    setIntrinsicSize(intrinsicSize);
}

RenderMedia::~RenderMedia()
{
}

HTMLMediaElement* RenderMedia::mediaElement() const
{ 
    return static_cast<HTMLMediaElement*>(node()); 
}

void RenderMedia::layout()
{
    LayoutSize oldSize = contentBoxRect().size();

    RenderImage::layout();

    RenderBox* controlsRenderer = toRenderBox(m_children.firstChild());
    if (!controlsRenderer)
        return;

    LayoutSize newSize = contentBoxRect().size();
    if (newSize == oldSize && !controlsRenderer->needsLayout())
        return;

#if ENABLE(TIZEN_FULLSCREEN_API)
    // We scaled control-root in MediaControlRootElement::updateMediaControlScale().
    // Need to multiply width and height by scale factor.
    if (mediaElement()->isFullscreen() && document()->page()
        && document()->page()->mainFrame() && document()->page()->mainFrame()->view()) {
        FloatRect windowRect = document()->page()->chrome()->windowRect();
        float compareWidth = windowRect.width() > windowRect.height() ? compareResolution.height() : compareResolution.width();
        float scaleFactor = 1 / document()->page()->chrome()->contentsScaleFactor();
        scaleFactor *= (windowRect.width() / compareWidth);

        FloatRect visibleRect = document()->page()->mainFrame()->view()->visibleContentRect();
        newSize = IntSize((int)(visibleRect.width() / scaleFactor), (int)(visibleRect.height() / scaleFactor));
    }
#endif

    // When calling layout() on a child node, a parent must either push a LayoutStateMaintainter, or 
    // instantiate LayoutStateDisabler. Since using a LayoutStateMaintainer is slightly more efficient,
    // and this method will be called many times per second during playback, use a LayoutStateMaintainer:
    LayoutStateMaintainer statePusher(view(), this, locationOffset(), hasTransform() || hasReflection() || style()->isFlippedBlocksWritingMode());

    controlsRenderer->setLocation(LayoutPoint(borderLeft(), borderTop()) + LayoutSize(paddingLeft(), paddingTop()));
    controlsRenderer->style()->setHeight(Length(newSize.height(), Fixed));
    controlsRenderer->style()->setWidth(Length(newSize.width(), Fixed));
    controlsRenderer->setNeedsLayout(true, MarkOnlyThis);
    controlsRenderer->layout();
    setChildNeedsLayout(false);

    statePusher.pop();
}

void RenderMedia::paintReplaced(PaintInfo&, const LayoutPoint&)
{
}

} // namespace WebCore

#endif
