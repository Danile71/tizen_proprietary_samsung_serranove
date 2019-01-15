/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "WebLayerTreeRendererTizen.h"

using namespace WebCore;

namespace WebKit {

WebLayerTreeRendererTizen::WebLayerTreeRendererTizen(LayerTreeCoordinatorProxy* layerTreeCoordinatorProxy, DrawingAreaProxy* drawingAreaProxy)
    : WebLayerTreeRenderer(layerTreeCoordinatorProxy, drawingAreaProxy)
{
}

void WebLayerTreeRendererTizen::purgeGLResources()
{
    TextureMapperLayer* layer = toTextureMapperLayer(rootLayer());

    if (layer)
        layer->clearBackingStoresRecursive();

    clearPlatformLayerPlatformSurfaces();

    m_directlyCompositedImages.clear();
#if USE(GRAPHICS_SURFACE)
    m_surfaceBackingStores.clear();
#endif

    m_rootLayer->removeAllChildren();
    m_rootLayer.clear();
    m_rootLayerID = InvalidWebLayerID;
    m_layers.clear();
    m_fixedLayers.clear();
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    m_scrollingContentsLayers.clear();
#endif
    m_textureMapper.clear();
    m_backingStoresWithPendingBuffers.clear();

    setActive(false);

    callOnMainThread(WTF::bind(&WebLayerTreeRendererTizen::purgeBackingStores, this));
}

void WebLayerTreeRendererTizen::syncCanvas(WebLayerID id, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer, int flags)
{
    if (canvasSize.isEmpty() || !m_textureMapper)
        return;

#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    ensureLayer(id);
    GraphicsLayer* layer = layerByID(id);

    RefPtr<TextureMapperSurfaceBackingStore> canvasBackingStore;
    SurfaceBackingStoreMap::iterator it = m_surfaceBackingStores.find(id);
    if (it == m_surfaceBackingStores.end()) {
        canvasBackingStore = TextureMapperSurfaceBackingStore::create();
        m_surfaceBackingStores.set(id, canvasBackingStore);
    } else {
        canvasBackingStore = it->second;

        if ((canvasBackingStore->graphicsSurfaceFrontBuffer() > 0 && canvasBackingStore->graphicsSurfaceFrontBuffer() != frontBuffer) || flags & GraphicsSurface::Is2D) {
            FreePlatformSurfaceData data;
            data.platformSurfaceId = canvasBackingStore->graphicsSurfaceFrontBuffer();
            data.layerID = id;
            m_freePlatformSurfaces.append(data);
        }
    }
    m_platformLayerPlatformSurfaces.set(frontBuffer, id);
    canvasBackingStore->setGraphicsSurface(graphicsSurfaceToken, canvasSize, frontBuffer, flags);
    layer->setContentsOpaque(!(flags & GraphicsSurface::Alpha));
    layer->setContentsToMedia(canvasBackingStore.get());
#endif
}

void WebLayerTreeRendererTizen::detach()
{
    clearBackingStores();
    WebLayerTreeRenderer::detach();
}

void WebLayerTreeRendererTizen::appendUpdate(const WTF::Function<void()>& function)
{
    m_renderQueue.append(function);
}

void WebLayerTreeRendererTizen::setActive(bool active)
{
    if (m_isActive == active)
        return;

    // Have to clear render queue in both cases.
    // If there are some updates in queue during activation then those updates are from previous instance of paint node
    // and cannot be applied to the newly created instance.
    if(!active)
        m_renderQueue.clear();
    m_isActive = active;
}

void WebLayerTreeRendererTizen::deleteLayer(WebLayerID layerID)
{
    GraphicsLayer* layer = layerByID(layerID);
    if (!layer)
        return;

    LOG(AcceleratedCompositing, "[UI ] delete layer %u @WebLayerTreeRenderer::deleteLayer \n", layerID);
    layer->removeFromParent();
    m_layers.remove(layerID);
    m_fixedLayers.remove(layerID);
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    m_scrollingContentsLayers.remove(layerID);
#endif

#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    SurfaceBackingStoreMap::iterator it = m_surfaceBackingStores.find(layerID);
    if (it != m_surfaceBackingStores.end())
        m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::FreePlatformSurface(layerID, it->second->graphicsSurfaceFrontBuffer()), m_drawingAreaProxy->page()->pageID());

    m_surfaceBackingStores.remove(layerID);
#endif
    delete layer;
}

void WebLayerTreeRendererTizen::setLayerState(WebLayerID id, const WebLayerInfo& layerInfo)
{
    ensureLayer(id);
    LayerMap::iterator it = m_layers.find(id);
    ASSERT(it != m_layers.end());

    GraphicsLayer* layer = it->second;
    if (!layer)
        return;

    layer->setReplicatedByLayer(layerByID(layerInfo.replica));
    layer->setMaskLayer(layerByID(layerInfo.mask));

    layer->setPosition(layerInfo.pos);
    layer->setSize(layerInfo.size);
    layer->setTransform(layerInfo.transform);
    layer->setAnchorPoint(layerInfo.anchorPoint);
    layer->setChildrenTransform(layerInfo.childrenTransform);
    layer->setBackfaceVisibility(layerInfo.backfaceVisible);

    if (!(layerInfo.contentType == WebLayerInfo::Canvas3DContentType || layerInfo.contentType == WebLayerInfo::MediaContentType || layerInfo.contentType == WebLayerInfo::Canvas2DContentType))
        layer->setContentsOpaque(layerInfo.contentsOpaque);

    layer->setContentsRect(layerInfo.contentsRect);
    layer->setContentsToBackgroundColor(layerInfo.backgroundColor);
    layer->setDrawsContent(layerInfo.drawsContent);
    layer->setContentsVisible(layerInfo.contentsVisible);
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    layer->setFullScreenLayerForVideo(layerInfo.fullScreenLayerForVideo);
#endif
    toGraphicsLayerTextureMapper(layer)->setFixedToViewport(layerInfo.fixedToViewport);

    if (layerInfo.fixedToViewport)
        m_fixedLayers.add(id, layer);
    else {
#if ENABLE(TIZEN_RESET_FIXED_LAYER_POSITION_ON_REMOVE)
        LayerMap::iterator it = m_fixedLayers.find(id);
        if (it != m_fixedLayers.end())
             toTextureMapperLayer(it->second)->setScrollPositionDeltaIfNeeded(FloatSize());
#endif
        m_fixedLayers.remove(id);
    }

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    if (layerInfo.needsSyncOverflowScrolling)
        layer->setNeedsSyncOverflowScrolling(true);

    if (layerInfo.isScrollingContentsLayer) {
        LayerMap::iterator it = m_scrollingContentsLayers.find(id);
        if (it == m_scrollingContentsLayers.end() || (layer->needsSyncOverflowScrolling() && layer->boundsOrigin() != layerInfo.boundsOrigin)) {
            GraphicsLayer* scrollingLayer = layerByID(layerInfo.parent);
            if (scrollingLayer) {
                layer->setBoundsOrigin(layerInfo.boundsOrigin);
                const IntRect newVisibleRect(FloatRect(layer->boundsOrigin(), scrollingLayer->size()));
                m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::SetVisibleContentsLocationAndTrajectoryVectorForLayer(id, newVisibleRect.location(), WebCore::FloatPoint()), m_drawingAreaProxy->page()->pageID());
            }
        }
        m_scrollingContentsLayers.add(id, layer);
    }
    else
        m_scrollingContentsLayers.remove(id);
#endif
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR)
    layer->setIsScrollbar(layerInfo.isScrollbar);
#endif

    if (!(layerInfo.contentType == WebLayerInfo::Canvas3DContentType || layerInfo.contentType == WebLayerInfo::MediaContentType || layerInfo.contentType == WebLayerInfo::Canvas2DContentType))
        assignImageToLayer(layer, layerInfo.imageBackingStoreID);

    // Never make the root layer clip.
    layer->setMasksToBounds(layerInfo.isRootLayer ? false : layerInfo.masksToBounds);
    layer->setOpacity(layerInfo.opacity);
    layer->setPreserves3D(layerInfo.preserves3D);
    if (layerInfo.isRootLayer && m_rootLayerID != id)
        setRootLayerID(id);
}

void WebLayerTreeRendererTizen::removeTile(WebLayerID layerID, int tileID)
{
#if PLATFORM(TIZEN)
    // Check whether composited graphics layer already been detached
    GraphicsLayer* pGraphicsLayer = layerByID(layerID);
    if (!pGraphicsLayer) {
        m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::FreePlatformSurfaceByTileID(tileID), m_drawingAreaProxy->page()->pageID());
        LOG(TiledAC, "layerID %d already has been deleted.", layerID);
        return;
    }
#endif

    m_drawingAreaProxy->page()->makeContextCurrent();

    LayerBackingStoreTizen* backingStore = static_cast<LayerBackingStoreTizen*>(getBackingStore(layerID).get());
    if (!backingStore)
        return;

    if (backingStore->tilePlatformSurfaceId(tileID) > 0)
        m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::FreePlatformSurface(layerID, backingStore->tilePlatformSurfaceId(tileID)), m_drawingAreaProxy->page()->pageID());

    backingStore->removeTile(tileID);
}

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
void WebLayerTreeRendererTizen::createTextureMapper()
{
    if (!m_textureMapper)
        m_textureMapper = TextureMapper::create(TextureMapper::OpenGLMode);
    ASSERT(m_textureMapper->accelerationMode() == TextureMapper::OpenGLMode);
}

void WebLayerTreeRendererTizen::updateTileWithUpdateInfo(WebLayerID layerID, int tileID, const TileUpdate& updateInfo)
{
    updatePlatformSurfaceTile(layerID, tileID, updateInfo.sourceRect, updateInfo.targetRect, updateInfo.platformSurfaceID, updateInfo.platformSurfaceSize, updateInfo.partialUpdate, updateInfo.isLowScaleTile);
}
#endif

void WebLayerTreeRendererTizen::flushLayerChanges(const IntPoint& scrollPosition)
{
    m_renderedContentsScrollPosition = scrollPosition;

    // Since the frame has now been rendered, we can safely unlock the animations until the next layout.
    setAnimationsLocked(false);

    m_rootLayer->syncCompositingState(FloatRect());

    freePlatformSurface();
    commitTileOperations();

#if ENABLE(TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM)
    if (m_lowScaleCompositedLayerID != InvalidWebLayerID) {
        LayerBackingStoreTizen* backingStore = static_cast<LayerBackingStoreTizen*>(getBackingStore(m_lowScaleCompositedLayerID).get());
        if (backingStore)
            backingStore->changeScaleForLowScaleLayer();
    }
#endif

    // The pending tiles state is on its way for the screen, tell the web process to render the next one.
    callOnMainThread(WTF::bind(&WebLayerTreeRendererTizen::renderNextFrame, this));
}

void WebLayerTreeRendererTizen::clearBackingStores()
{
    LayerMap::iterator end = m_layers.end();
    for(LayerMap::iterator it = m_layers.begin(); it != end; ++it)
        toTextureMapperLayer(it->second)->clearBackingStore();

    clearPlatformLayerPlatformSurfaces();

    m_directlyCompositedImages.clear();
    m_backingStoresWithPendingBuffers.clear();

    freePlatformSurfacesAfterClearingBackingStores();
}

PassRefPtr<LayerBackingStore> WebLayerTreeRendererTizen::getBackingStore(WebLayerID id)
{
    TextureMapperLayer* layer = toTextureMapperLayer(layerByID(id));
    ASSERT(layer);
    RefPtr<LayerBackingStoreTizen> backingStore = static_cast<LayerBackingStoreTizen*>(layer->backingStore().get());
    if (!backingStore) {
        backingStore = LayerBackingStoreTizen::create();
        layer->setBackingStore(backingStore.get());
        backingStore->setPlatformSurfaceTexturePool(m_drawingAreaProxy);
    }
    ASSERT(backingStore);
    return backingStore;
}

void WebLayerTreeRendererTizen::freePlatformSurface()
{
    if (!hasPlatformSurfaceToFree())
        return;

    HashSet<RefPtr<LayerBackingStore> >::iterator end = m_backingStoresWithPendingBuffers.end();
    for (HashSet<RefPtr<LayerBackingStore> >::iterator it = m_backingStoresWithPendingBuffers.begin(); it != end; ++it) {
        LayerBackingStoreTizen* backingStore = static_cast<LayerBackingStoreTizen*>((*it).get());
        int layerId = backingStore->layerId();
        Vector<int> freePlatformSurface = *(backingStore->freePlatformSurfaceTiles());

        for (unsigned n = 0; n < backingStore->freePlatformSurfaceTiles()->size(); ++n)
            m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::FreePlatformSurface(layerId, freePlatformSurface[n]), m_drawingAreaProxy->page()->pageID());
        backingStore->clearFreePlatformSurfaceTiles();
    }
    m_backingStoresWithPendingBuffers.clear();

    // m_freePlatformSurfaces is for platformSurfaces which are not corresponding to platformSurface tiles, like TextureMapperPlatformLayer's.
    for (unsigned n = 0; n < m_freePlatformSurfaces.size(); ++n)
        m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::FreePlatformSurface(m_freePlatformSurfaces[n].layerID, m_freePlatformSurfaces[n].platformSurfaceId), m_drawingAreaProxy->page()->pageID());

    m_freePlatformSurfaces.clear();
}

bool WebLayerTreeRendererTizen::hasPlatformSurfaceToFree()
{
    if (m_backingStoresWithPendingBuffers.size() > 0)
        return true;

    if (m_freePlatformSurfaces.isEmpty())
        return false;

    return true;
}

void WebLayerTreeRendererTizen::updatePlatformSurfaceTile(WebLayerID layerID, int tileID, const IntRect& sourceRect, const IntRect& targetRect, int platformSurfaceId, const IntSize& platformSurfaceSize, bool partialUpdate, bool lowScaleTile)
{
    if (!platformSurfaceId)
        return;

    m_drawingAreaProxy->page()->makeContextCurrent();

    RefPtr<LayerBackingStoreTizen> backingStore = static_cast<LayerBackingStoreTizen*>(getBackingStore(layerID).get());
    backingStore->updatePlatformSurfaceTile(tileID, sourceRect, targetRect, layerID, platformSurfaceId, platformSurfaceSize, partialUpdate, m_textureMapper.get());
    if (!m_backingStoresWithPendingBuffers.contains(backingStore))
        m_backingStoresWithPendingBuffers.add(backingStore);
}

void WebLayerTreeRendererTizen::freePlatformSurfacesAfterClearingBackingStores()
{
    // Before calling this function, destroying BackingStores should be done,
    // because platformSurfacesToFree is filled in ~LayerBackingStore().

    RefPtr<PlatformSurfaceTexturePool> platformSurfaceTexturePool = m_drawingAreaProxy->page()->process()->platformSurfaceTexturePool();
    Vector<int> platformSurfacesToFree = platformSurfaceTexturePool->platformSurfacesToFree();
    m_drawingAreaProxy->page()->makeContextCurrent();
    for (unsigned n = 0; n < platformSurfacesToFree.size(); ++n)
        // Actually, m_rootLayerID is not correct for all platformSurfaces.
        // But LayerTreeCoordinator::freePlatformSurface() doesn't care layerID for HTML contents layers
        // so any dummy layerID is okay for now.
        m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::FreePlatformSurface(m_rootLayerID, platformSurfacesToFree[n]), m_drawingAreaProxy->page()->pageID());

    platformSurfaceTexturePool->clearPlatformSurfacesToFree();
}

void WebLayerTreeRendererTizen::clearPlatformLayerPlatformSurfaces()
{
    PlatformLayerPlatformSurfaceMap::iterator end = m_platformLayerPlatformSurfaces.end();
    for (PlatformLayerPlatformSurfaceMap::iterator it = m_platformLayerPlatformSurfaces.begin(); it != end; ++it)
        m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::RemovePlatformSurface(it->second, it->first), m_drawingAreaProxy->page()->pageID());

    m_platformLayerPlatformSurfaces.clear();
}

void WebLayerTreeRendererTizen::renderNextFrame()
{
    WebLayerTreeRenderer::renderNextFrame();
}

void WebLayerTreeRendererTizen::purgeBackingStores()
{
    WebLayerTreeRenderer::purgeBackingStores();
}

}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

