/*
    Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef LayerTreeCoordinatorProxy_h
#define LayerTreeCoordinatorProxy_h

#if USE(UI_SIDE_COMPOSITING)

#include "BackingStore.h"
#include "CoordinatedGraphicsArgumentCoders.h"
#include "DrawingAreaProxy.h"
#include "Region.h"
#include "SurfaceUpdateInfo.h"
#include "WebLayerTreeInfo.h"
#include <WebCore/GraphicsContext.h>
#include <WebCore/GraphicsLayer.h>
#include <WebCore/GraphicsLayerAnimation.h>
#include <WebCore/IntRect.h>
#include <WebCore/IntSize.h>
#include <WebCore/RunLoop.h>
#include <WebCore/Timer.h>
#include <wtf/Functional.h>
#include <wtf/HashSet.h>

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
#include <Ecore.h>
#endif

#if PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QSGNode;
QT_END_NAMESPACE
#endif

namespace WebKit {

class WebLayerInfo;
class WebLayerTreeRenderer;
class WebLayerUpdateInfo;

class LayerTreeCoordinatorProxy {
public:
    LayerTreeCoordinatorProxy(DrawingAreaProxy*);
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    LayerTreeCoordinatorProxy(DrawingAreaProxy*, bool isGLMode);
#endif
    virtual ~LayerTreeCoordinatorProxy();
    void setCompositingLayerState(WebLayerID, const WebLayerInfo&);
    void setCompositingLayerChildren(WebLayerID, const Vector<WebLayerID>&);
#if ENABLE(CSS_FILTERS)
    void setCompositingLayerFilters(WebLayerID, const WebCore::FilterOperations&);
#endif
    void deleteCompositingLayer(WebLayerID);
    void setRootCompositingLayer(WebLayerID);
    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    void purgeGLResources();
    void setContentsSize(const WebCore::FloatSize&);
    void setVisibleContentsRect(const WebCore::IntRect&, float scale, const WebCore::FloatPoint& trajectory, const WebCore::FloatPoint& accurateVisibleContentsPosition);
    void didRenderFrame(const WebCore::IntPoint& scrollPosition);
    void createTileForLayer(int layerID, int tileID, const WebCore::IntRect&, const SurfaceUpdateInfo&);
    void updateTileForLayer(int layerID, int tileID, const WebCore::IntRect&, const SurfaceUpdateInfo&);
    void removeTileForLayer(int layerID, int tileID);
    void createDirectlyCompositedImage(int64_t, const WebKit::ShareableBitmap::Handle&);
    void destroyDirectlyCompositedImage(int64_t);
    void didReceiveLayerTreeCoordinatorProxyMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    void updateViewport();
    void renderNextFrame();
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    void syncCanvas(uint32_t id, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer, int flags);
#else
    void syncCanvas(uint32_t id, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer);
#endif
    void purgeBackingStores();
    WebLayerTreeRenderer* layerTreeRenderer() const { return m_renderer.get(); }
    void setLayerAnimations(WebLayerID, const WebCore::GraphicsLayerAnimations&);
    void setAnimationsLocked(bool);
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    void clearBackingStores();
    void setUpdateViewportWithAnimatorEnabled(bool enabled) { m_updateViewportWithAnimatorEnabled = enabled; }
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setLowScaleLayerFlag(bool enabled) { m_isLowScaleLayerEnabled = enabled; }
    bool isLowScaleLayerFlag() { return m_isLowScaleLayerEnabled; }
#endif
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    bool hasOverflowLayer() const;
    virtual void platformLayerChanged(WebCore::GraphicsLayer*, WebCore::PlatformLayer* oldPlatformLayer, WebCore::PlatformLayer* newPlatformLayer) { ASSERT_NOT_REACHED(); }
#endif
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    bool setOffsetForFocusedScrollingContentsLayer(const WebCore::FloatPoint&);
    void setVisibleContentsRectForScrollingContentsLayers(const WebCore::IntRect&);
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    void initializeAcceleratedCompositingMode(bool isGLMode);
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setLowScaleNonCompositedLayerID(WebLayerID lowScalelayerID);
    void setNonCompositedLayerID(uint32_t nonCompositedLayerID);
    void setCoverWithTilesForLowScaleLayerOnly(bool);
#endif

    void setBackgroundColor(const WebCore::Color&);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    static Eina_Bool updateViewport(void*);
#endif

protected:
    void dispatchUpdate(const Function<void()>&);

    DrawingAreaProxy* m_drawingAreaProxy;
    RefPtr<WebLayerTreeRenderer> m_renderer;
#if USE(GRAPHICS_SURFACE)
    HashMap<uint32_t, RefPtr<ShareableSurface> > m_surfaces;
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
private:
    Ecore_Animator* m_updateViewportAnimator;
    bool m_updateViewportWithAnimatorEnabled;
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    bool m_isLowScaleLayerEnabled;
#endif
#endif
};

}

#endif

#endif // LayerTreeCoordinatorProxy_h
