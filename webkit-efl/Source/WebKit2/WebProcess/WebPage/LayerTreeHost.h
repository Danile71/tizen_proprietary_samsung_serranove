/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef LayerTreeHost_h
#define LayerTreeHost_h

#include "Color.h"
#include "LayerTreeContext.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
class MessageID;
}

namespace WebCore {
class FloatPoint;
class IntRect;
class IntSize;
class GraphicsLayer;

#if PLATFORM(WIN) && USE(AVFOUNDATION)
struct GraphicsDeviceAdapter;
#endif
}

namespace WebKit {

class UpdateInfo;
class WebPage;

#if PLATFORM(WIN)
struct WindowGeometry;
#endif

class LayerTreeHost : public RefCounted<LayerTreeHost> {
public:
    static PassRefPtr<LayerTreeHost> create(WebPage*);
    virtual ~LayerTreeHost();

    static bool supportsAcceleratedCompositing();

    virtual const LayerTreeContext& layerTreeContext() = 0;
    virtual void scheduleLayerFlush() = 0;
    virtual void setLayerFlushSchedulingEnabled(bool) = 0;
    virtual void setShouldNotifyAfterNextScheduledLayerFlush(bool) = 0;
    virtual void setRootCompositingLayer(WebCore::GraphicsLayer*) = 0;
    virtual void invalidate() = 0;

    virtual void setNonCompositedContentsNeedDisplay(const WebCore::IntRect&) = 0;
    virtual void scrollNonCompositedContents(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset) = 0;
    virtual void forceRepaint() = 0;
    virtual void sizeDidChange(const WebCore::IntSize& newSize) = 0;
    virtual void deviceScaleFactorDidChange() = 0;

    virtual void didInstallPageOverlay() = 0;
    virtual void didUninstallPageOverlay() = 0;
    virtual void setPageOverlayNeedsDisplay(const WebCore::IntRect&) = 0;
    virtual void setPageOverlayOpacity(float) { }
    virtual bool pageOverlayShouldApplyFadeWhenPainting() const { return true; }

    virtual void pauseRendering() { }
    virtual void resumeRendering() { }

#if USE(UI_SIDE_COMPOSITING)
    virtual void setVisibleContentsRect(const WebCore::IntRect&, float scale, const WebCore::FloatPoint&) { }
#if PLATFORM(TIZEN)
    virtual void setVisibleContentsRectAndScaleForBackingStore(const WebCore::IntRect& rect,const float newScale) { }
#endif
    virtual void setVisibleContentsRectForLayer(int layerID, const WebCore::IntRect&) { }
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    virtual void setVisibleContentsRectAndTrajectoryVectorForLayer(int layerID, const WebCore::IntRect&, const WebCore::FloatPoint&) { }
#endif
    virtual void renderNextFrame() { }
    virtual void purgeBackingStores() { }
    virtual void didReceiveLayerTreeCoordinatorMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*) = 0;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    virtual void freePlatformSurface(int layerID, int platformSurfaceId) { }
    virtual void freePlatformSurfaceByTileID(int tileID) { }
    virtual void removePlatformSurface(int layerID, int platformSurfaceId) { }
#endif
#endif

#if ENABLE(TIZEN_ONESHOT_DRAWING_SYNCHRONIZATION)
    virtual void setNeedsOneShotDrawingSynchronization() = 0;
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    virtual void addOrUpdateScrollingLayer(WebCore::GraphicsLayer* scrollingLayer, WebCore::GraphicsLayer* contentsLayer, const WebCore::IntSize& scrollSize) { }
    virtual void removeScrollingLayer(WebCore::GraphicsLayer* scrollingLayer, WebCore::GraphicsLayer* contentsLayer) { }
#endif

#if ENABLE(TIZEN_WEBKIT2)
    virtual float contentsScaleFactor() const = 0;
#endif

#if ENABLE(TIZEN_LAYER_FLUSH_THROTTLING)
    virtual void setDeferLayerFlush(bool) { }
#endif

#if PLATFORM(WIN)
    virtual void scheduleChildWindowGeometryUpdate(const WindowGeometry&) = 0;
#endif

#if PLATFORM(MAC)
    virtual void setLayerHostingMode(LayerHostingMode) { }
#endif

#if PLATFORM(WIN) && USE(AVFOUNDATION)
    virtual WebCore::GraphicsDeviceAdapter* graphicsDeviceAdapter() const { return 0; }
#endif

#if USE(UI_SIDE_COMPOSITING)
    virtual void scheduleAnimation() = 0;
#endif

    virtual void setBackgroundColor(const WebCore::Color&) { }

protected:
    explicit LayerTreeHost(WebPage*);

    WebPage* m_webPage;

#if USE(UI_SIDE_COMPOSITING)
    bool m_waitingForUIProcess;
#endif
};

#if !PLATFORM(WIN) && !PLATFORM(QT) && !ENABLE(TIZEN_WEBKIT2_TILED_AC)
inline bool LayerTreeHost::supportsAcceleratedCompositing()
{
    return true;
}
#endif

} // namespace WebKit

#endif // LayerTreeHost_h
