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

#include "config.h"

#if USE(UI_SIDE_COMPOSITING)
#include "LayerTreeCoordinatorProxy.h"

#include "LayerTreeCoordinatorMessages.h"
#include "UpdateInfo.h"
#include "WebCoreArgumentCoders.h"
#include "WebLayerTreeInfo.h"
#include "WebLayerTreeRenderer.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "WebLayerTreeRendererTizen.h"
#endif

namespace WebKit {

using namespace WebCore;

LayerTreeCoordinatorProxy::LayerTreeCoordinatorProxy(DrawingAreaProxy* drawingAreaProxy)
    : m_drawingAreaProxy(drawingAreaProxy)
#if !ENABLE(TIZEN_WEBKIT2_TILED_AC)
    , m_renderer(adoptRef(new WebLayerTreeRenderer(this)))
#else
    , m_renderer(adoptRef(new WebLayerTreeRenderer(this, drawingAreaProxy)))
    , m_updateViewportAnimator(0)
    , m_updateViewportWithAnimatorEnabled(false)
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_isLowScaleLayerEnabled(false)
#endif
#endif
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    m_renderer->setActive(true);
#endif
}

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
LayerTreeCoordinatorProxy::LayerTreeCoordinatorProxy(DrawingAreaProxy* drawingAreaProxy, bool isGLMode)
    : m_drawingAreaProxy(drawingAreaProxy)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    , m_updateViewportAnimator(0)
    , m_updateViewportWithAnimatorEnabled(false)
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_isLowScaleLayerEnabled(false)
#endif
#endif
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    WebLayerTreeRenderer* renderer = isGLMode ? new WebLayerTreeRendererTizen(this, drawingAreaProxy) : new WebLayerTreeRenderer(this, drawingAreaProxy, isGLMode);
    m_renderer = adoptRef(renderer);
#else
    m_renderer = adoptRef(new WebLayerTreeRenderer(this, drawingAreaProxy, isGLMode));
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    m_renderer->setActive(true);
}
#endif // ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)

LayerTreeCoordinatorProxy::~LayerTreeCoordinatorProxy()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (m_updateViewportAnimator) {
        ecore_animator_del(m_updateViewportAnimator);
        m_updateViewportAnimator = 0;
    }
#endif
    m_renderer->detach();
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
Eina_Bool LayerTreeCoordinatorProxy::updateViewport(void* data)
{
    LayerTreeCoordinatorProxy* layerTreeCoordinator = static_cast<LayerTreeCoordinatorProxy*>(data);

    layerTreeCoordinator->m_drawingAreaProxy->updateViewport();
    layerTreeCoordinator->m_updateViewportAnimator = 0;

    return ECORE_CALLBACK_CANCEL;
}
#endif


void LayerTreeCoordinatorProxy::updateViewport()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (m_updateViewportWithAnimatorEnabled) {
        if (!m_updateViewportAnimator)
            m_updateViewportAnimator = ecore_animator_add(updateViewport, this);
    } else
        m_drawingAreaProxy->updateViewport();
#else
    m_drawingAreaProxy->updateViewport();
#endif
}

void LayerTreeCoordinatorProxy::dispatchUpdate(const Function<void()>& function)
{
    m_renderer->appendUpdate(function);
}

void LayerTreeCoordinatorProxy::createTileForLayer(int layerID, int tileID, const IntRect& targetRect, const WebKit::SurfaceUpdateInfo& updateInfo)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    dispatchUpdate(bind(&WebLayerTreeRenderer::createTile, m_renderer.get(), layerID, tileID, updateInfo.scaleFactor, updateInfo.isLowScaleTile));
#else
    dispatchUpdate(bind(&WebLayerTreeRenderer::createTile, m_renderer.get(), layerID, tileID, updateInfo.scaleFactor));
#endif
    updateTileForLayer(layerID, tileID, targetRect, updateInfo);
}

void LayerTreeCoordinatorProxy::updateTileForLayer(int layerID, int tileID, const IntRect& targetRect, const WebKit::SurfaceUpdateInfo& updateInfo)
{
    RefPtr<ShareableSurface> surface;
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    if (!m_renderer->isUsingPlatformSurface()) {
#endif
#if USE(GRAPHICS_SURFACE)
    int token = updateInfo.surfaceHandle.graphicsSurfaceToken();
    if (token) {
        HashMap<uint32_t, RefPtr<ShareableSurface> >::iterator it = m_surfaces.find(token);
        if (it == m_surfaces.end()) {
            surface = ShareableSurface::create(updateInfo.surfaceHandle);
            m_surfaces.add(token, surface);
        } else
            surface = it->second;
    } else
        surface = ShareableSurface::create(updateInfo.surfaceHandle);
#else
    surface = ShareableSurface::create(updateInfo.surfaceHandle);
#endif
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    }
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    dispatchUpdate(bind(&WebLayerTreeRenderer::updateTileWithUpdateInfo, m_renderer.get(), layerID, tileID, WebLayerTreeRenderer::TileUpdate(updateInfo.updateRect, targetRect, surface, updateInfo.surfaceOffset, updateInfo.platformSurfaceID, updateInfo.platformSurfaceSize, updateInfo.partialUpdate, updateInfo.isLowScaleTile)));
#else
    dispatchUpdate(bind(&WebLayerTreeRenderer::updateTileWithUpdateInfo, m_renderer.get(), layerID, tileID, WebLayerTreeRenderer::TileUpdate(updateInfo.updateRect, targetRect, surface, updateInfo.surfaceOffset)));
#endif
#else
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    dispatchUpdate(bind(&WebLayerTreeRenderer::updatePlatformSurfaceTile, m_renderer.get(), layerID, tileID, updateInfo.updateRect, targetRect, updateInfo.platformSurfaceID, updateInfo.platformSurfaceSize, updateInfo.partialUpdate));
#else
    dispatchUpdate(bind(&WebLayerTreeRenderer::updateTile, m_renderer.get(), layerID, tileID, WebLayerTreeRenderer::TileUpdate(updateInfo.updateRect, targetRect, surface, updateInfo.surfaceOffset)));
#endif
#endif
}

void LayerTreeCoordinatorProxy::removeTileForLayer(int layerID, int tileID)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::removeTile, m_renderer.get(), layerID, tileID));
}

void LayerTreeCoordinatorProxy::deleteCompositingLayer(WebLayerID id)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::deleteLayer, m_renderer.get(), id));
#if !ENABLE(TIZEN_WEBKIT2_TILED_AC)
    // For efl port, we should not call updateViewport here immediately.
    // This may trigger rendering with broken LayerTreeHost.
    // And actually, we don't need to call this here, because it will be called by didRenderFrame for each update of LayerTreeHost.
    updateViewport();
#endif
}

void LayerTreeCoordinatorProxy::setRootCompositingLayer(WebLayerID id)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::setRootLayerID, m_renderer.get(), id));
    updateViewport();
}

void LayerTreeCoordinatorProxy::setCompositingLayerState(WebLayerID id, const WebLayerInfo& info)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::setLayerState, m_renderer.get(), id, info));
}

void LayerTreeCoordinatorProxy::setCompositingLayerChildren(WebLayerID id, const Vector<WebLayerID>& children)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::setLayerChildren, m_renderer.get(), id, children));
}

#if ENABLE(CSS_FILTERS)
void LayerTreeCoordinatorProxy::setCompositingLayerFilters(WebLayerID id, const FilterOperations& filters)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::setLayerFilters, m_renderer.get(), id, filters));
}
#endif

void LayerTreeCoordinatorProxy::didRenderFrame(const IntPoint& scrollPosition)
{
#if PLATFORM(TIZEN)
    m_drawingAreaProxy->page()->didRenderFrame();
#endif
    dispatchUpdate(bind(&WebLayerTreeRenderer::flushLayerChanges, m_renderer.get(), scrollPosition));
#if PLATFORM(TIZEN)
    m_renderer->syncRemoteContent();
#endif
    updateViewport();
}

void LayerTreeCoordinatorProxy::createDirectlyCompositedImage(int64_t key, const WebKit::ShareableBitmap::Handle& handle)
{
    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::create(handle);
    dispatchUpdate(bind(&WebLayerTreeRenderer::createImage, m_renderer.get(), key, bitmap));
}

void LayerTreeCoordinatorProxy::destroyDirectlyCompositedImage(int64_t key)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::destroyImage, m_renderer.get(), key));
}

void LayerTreeCoordinatorProxy::setContentsSize(const FloatSize& contentsSize)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::setContentsSize, m_renderer.get(), contentsSize));
}

void LayerTreeCoordinatorProxy::setLayerAnimations(WebLayerID id, const GraphicsLayerAnimations& animations)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::setLayerAnimations, m_renderer.get(), id, animations));
}

void LayerTreeCoordinatorProxy::setAnimationsLocked(bool locked)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::setAnimationsLocked, m_renderer.get(), locked));
}

void LayerTreeCoordinatorProxy::setVisibleContentsRect(const IntRect& rect, float scale, const FloatPoint& trajectoryVector, const WebCore::FloatPoint& accurateVisibleContentsPosition)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    m_renderer->setVisibleContentsRect(rect, scale, accurateVisibleContentsPosition);
#else
    dispatchUpdate(bind(&WebLayerTreeRenderer::setVisibleContentsRect, m_renderer.get(), rect, scale, accurateVisibleContentsPosition));
#endif
    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::SetVisibleContentsRect(rect, scale, trajectoryVector), m_drawingAreaProxy->page()->pageID());
}

void LayerTreeCoordinatorProxy::renderNextFrame()
{
    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::RenderNextFrame(), m_drawingAreaProxy->page()->pageID());
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
void LayerTreeCoordinatorProxy::syncCanvas(uint32_t id, const IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer, int flags)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::syncCanvas, m_renderer.get(), id, canvasSize, graphicsSurfaceToken, frontBuffer, flags));
}
#else
void LayerTreeCoordinatorProxy::syncCanvas(uint32_t id, const IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer)
{
    dispatchUpdate(bind(&WebLayerTreeRenderer::syncCanvas, m_renderer.get(), id, canvasSize, graphicsSurfaceToken, frontBuffer));
}
#endif

void LayerTreeCoordinatorProxy::purgeBackingStores()
{
    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::PurgeBackingStores(), m_drawingAreaProxy->page()->pageID());
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
void LayerTreeCoordinatorProxy::clearBackingStores()
{
    m_renderer->syncRemoteContent();
    m_renderer->clearBackingStores();
    purgeBackingStores();
}
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
bool LayerTreeCoordinatorProxy::hasOverflowLayer() const
{
    return m_renderer->scrollingContentsLayerCounts() ? true : false;
}
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
bool LayerTreeCoordinatorProxy::setOffsetForFocusedScrollingContentsLayer(const WebCore::FloatPoint& offset)
{
    return m_renderer && m_renderer->setOffset(m_renderer->focusedLayerID(), offset);
}

void LayerTreeCoordinatorProxy::setVisibleContentsRectForScrollingContentsLayers(const WebCore::IntRect& visibleRect)
{
    if (m_renderer)
        m_renderer->setVisibleContentsRectForScrollingContentsLayers(visibleRect);
}
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
void LayerTreeCoordinatorProxy::initializeAcceleratedCompositingMode(bool isGLMode)
{
    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::SetAccelerationMode(isGLMode), m_drawingAreaProxy->page()->pageID());
}
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
void LayerTreeCoordinatorProxy::setLowScaleNonCompositedLayerID(WebLayerID lowScalelayerID)
{
    if (!m_renderer)
        return;
    m_renderer->setLowScaleNonCompositedLayerID(lowScalelayerID);
}
void LayerTreeCoordinatorProxy::setNonCompositedLayerID(WebLayerID nonCompositedLayerID)
{
    if (!m_renderer)
        return;
    m_renderer->setNonCompositedLayerID(nonCompositedLayerID);
}

void LayerTreeCoordinatorProxy::setCoverWithTilesForLowScaleLayerOnly(bool lowScaleLayerOnly)
{
    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::SetCoverWithTilesForLowScaleLayerOnly(lowScaleLayerOnly),
                                                m_drawingAreaProxy->page()->pageID());
}
#endif

void LayerTreeCoordinatorProxy::setBackgroundColor(const Color& color)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (m_renderer->backgroundColor() == color)
        return;

    m_renderer->setBackgroundColor(color);
    updateViewport();
#else
    dispatchUpdate(bind(&WebLayerTreeRenderer::setBackgroundColor, m_renderer.get(), color));
#endif
}

}
#endif // USE(UI_SIDE_COMPOSITING)
