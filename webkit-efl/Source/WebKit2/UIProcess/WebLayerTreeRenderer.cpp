/*
    Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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

#include "WebLayerTreeRenderer.h"

#include "GraphicsLayerTextureMapper.h"
#include "LayerBackingStore.h"
#include "LayerTreeCoordinatorProxy.h"
#include "MessageID.h"
#include "ShareableBitmap.h"
#include "TextureMapper.h"
#include "TextureMapperBackingStore.h"
#include "TextureMapperGL.h"
#include "TextureMapperLayer.h"
#include "UpdateInfo.h"
#if !ENABLE(TIZEN_WEBKIT2_TILED_AC)
#include <OpenGLShims.h>
#endif
#include <WebCore/Logging.h>
#include <wtf/Atomics.h>
#include <wtf/MainThread.h>

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
#include "LayerTreeCoordinatorMessages.h"
#include "TextureMapperGL.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "tizen/LayerBackingStoreTizen.h"
#endif

namespace WebKit {

using namespace WebCore;

template<class T> class MainThreadGuardedInvoker {
public:
    static void call(PassRefPtr<T> objectToGuard, const Function<void()>& function)
    {
        MainThreadGuardedInvoker<T>* invoker = new MainThreadGuardedInvoker<T>(objectToGuard, function);
        callOnMainThread(invoke, invoker);
    }

private:
    MainThreadGuardedInvoker(PassRefPtr<T> object, const Function<void()>& newFunction)
        : objectToGuard(object)
        , function(newFunction)
    {
    }

    RefPtr<T> objectToGuard;
    Function<void()> function;
    static void invoke(void* data)
    {
        MainThreadGuardedInvoker<T>* invoker = static_cast<MainThreadGuardedInvoker<T>*>(data);
        invoker->function();
        delete invoker;
    }
};

void WebLayerTreeRenderer::callOnMainThread(const Function<void()>& function)
{
    if (isMainThread())
        function();
    else
        MainThreadGuardedInvoker<WebLayerTreeRenderer>::call(this, function);
}

static FloatPoint boundedScrollPosition(const FloatPoint& scrollPosition, const FloatRect& visibleContentRect, const FloatSize& contentSize)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    float scrollPositionX = std::max(scrollPosition.x(), 0.0f);
    scrollPositionX = std::min(scrollPositionX, contentSize.width());

    float scrollPositionY = std::max(scrollPosition.y(), 0.0f);
    scrollPositionY = std::min(scrollPositionY, contentSize.height());

    // Prevent negative scroll position.
    scrollPositionX = std::max(scrollPositionX, 0.0f);
    scrollPositionY = std::max(scrollPositionY, 0.0f);
#else
    float scrollPositionX = std::max(scrollPosition.x(), 0.0f);
    scrollPositionX = std::min(scrollPositionX, contentSize.width() - visibleContentRect.width());

    float scrollPositionY = std::max(scrollPosition.y(), 0.0f);
    scrollPositionY = std::min(scrollPositionY, contentSize.height() - visibleContentRect.height());
#endif

    return FloatPoint(scrollPositionX, scrollPositionY);
}

WebLayerTreeRenderer::WebLayerTreeRenderer(LayerTreeCoordinatorProxy* layerTreeCoordinatorProxy)
    : m_contentsScale(1)
    , m_layerTreeCoordinatorProxy(layerTreeCoordinatorProxy)
    , m_rootLayerID(InvalidWebLayerID)
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_lowScaleCompositedLayerID(InvalidWebLayerID)
    , m_nonCompositedLayerID(InvalidWebLayerID)
#endif
    , m_isActive(false)
    , m_animationsLocked(false)
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    , m_angle(0)
#endif
#if PLATFORM(EFL)
    , m_updateViewportTimer(RunLoop::current(), this, &WebLayerTreeRenderer::updateViewportFired)
#endif
    , m_backgroundColor(Color::white)
    , m_setDrawsBackground(false)
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_backgroundColorHasAlpha(false)
#endif
{
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
WebLayerTreeRenderer::WebLayerTreeRenderer(LayerTreeCoordinatorProxy* layerTreeCoordinatorProxy, DrawingAreaProxy* drawingAreaProxy, bool isGLMode)
    : m_contentsScale(1)
    , m_layerTreeCoordinatorProxy(layerTreeCoordinatorProxy)
    , m_rootLayerID(InvalidWebLayerID)
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_lowScaleCompositedLayerID(InvalidWebLayerID)
    , m_nonCompositedLayerID(InvalidWebLayerID)
#endif
    , m_isActive(false)
    , m_animationsLocked(false)
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    , m_angle(0)
#endif
    , m_drawingAreaProxy(drawingAreaProxy)
    , m_isGLMode(isGLMode)
#if PLATFORM(EFL)
    , m_updateViewportTimer(RunLoop::current(), this, &WebLayerTreeRenderer::updateViewportFired)
#endif
    , m_backgroundColor(Color::white)
    , m_setDrawsBackground(false)
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    , m_isProviderAC(false)
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_backgroundColorHasAlpha(false)
#endif
{
}
#endif

WebLayerTreeRenderer::~WebLayerTreeRenderer()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    if (m_backupTexture && m_backupTexture->isValid()) {
        m_backupTexture.release();
        m_backupTexture = 0;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    deleteAllValues(m_layers);
#endif
}

PassOwnPtr<GraphicsLayer> WebLayerTreeRenderer::createLayer(WebLayerID layerID)
{
    GraphicsLayer* newLayer = new GraphicsLayerTextureMapper(this);
    TextureMapperLayer* layer = toTextureMapperLayer(newLayer);
    layer->setShouldUpdateBackingStoreFromLayer(false);
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (layerID == m_lowScaleCompositedLayerID)
        layer->setLowScaleNonCompositedLayer(true);
    if (layerID == m_nonCompositedLayerID)
        layer->setNonCompositedLayer(true);
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    LOG(AcceleratedCompositing, "[UI ] create layer %u @WebLayerTreeRenderer::createLayer \n", layerID);
#endif

    return adoptPtr(newLayer);
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
void WebLayerTreeRenderer::paintToBackupTexture(const TransformationMatrix& matrix, float opacity, const IntRect& clipRect, const IntSize& texSize)
{
    if (!m_textureMapper)
        m_textureMapper = TextureMapper::create(TextureMapper::OpenGLMode);
    ASSERT(m_textureMapper->accelerationMode() == TextureMapper::OpenGLMode);

    adjustPositionForFixedLayers();
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    adjustPositionForOverflowLayers();
#endif

    GraphicsLayer* currentRootLayer = rootLayer();
    if (!currentRootLayer)
        return;

    TextureMapperLayer* layer = toTextureMapperLayer(currentRootLayer);
    if (!layer)
        return;

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    static_cast<TextureMapperGL*>(m_textureMapper.get())->setAngle(m_angle);
#endif

    if (!m_backupTexture || m_backupTexture->size() != texSize)
        m_backupTexture = m_textureMapper->acquireTextureFromPool(texSize);

    layer->setTextureMapper(m_textureMapper.get());
    if (!m_animationsLocked)
        layer->applyAnimationsRecursively();
    m_textureMapper->bindSurface(m_backupTexture.get());
    m_textureMapper->beginPainting(0);
    m_textureMapper->beginClip(TransformationMatrix(), clipRect);

    m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), m_backgroundColor);

    if (currentRootLayer->opacity() != opacity || currentRootLayer->transform() != matrix) {
        currentRootLayer->setOpacity(opacity);
        currentRootLayer->setTransform(matrix);
        currentRootLayer->syncCompositingStateForThisLayerOnly();
    }

#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)
    layer->paintWithCuller(clipRect);
#else
    layer->paint();
#endif

    m_textureMapper->endClip();
    m_textureMapper->endPainting();
    m_textureMapper->unbindSurface();

    if (layer->descendantsOrSelfHaveRunningAnimations()) {
#if PLATFORM(EFL)
        m_updateViewportTimer.startOneShot(0);
#else
        callOnMainThread(WTF::bind(&WebLayerTreeRenderer::updateViewport, this));
#endif
    }
}
#endif

void WebLayerTreeRenderer::paintToCurrentGLContext(const TransformationMatrix& matrix, float opacity, const FloatRect& clipRect, TextureMapper::PaintFlags PaintFlags)
{
    if (!m_textureMapper)
        m_textureMapper = TextureMapper::create(TextureMapper::OpenGLMode);
    ASSERT(m_textureMapper->accelerationMode() == TextureMapper::OpenGLMode);

    adjustPositionForFixedLayers();
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    adjustPositionForOverflowLayers();
#endif
    GraphicsLayer* currentRootLayer = rootLayer();
    if (!currentRootLayer) {
#if ENABLE(TIZEN_SET_INITIAL_COLOR_OF_WEBVIEW_EVAS_IMAGE) && ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
        if (m_angle == 90 || m_angle == 270)
            glViewport(0, 0, clipRect.height(), clipRect.width());
        glClearColor(m_backgroundColor.red() / 255.0f, m_backgroundColor.green() / 255.0f, m_backgroundColor.blue() / 255.0f, m_backgroundColor.alpha() / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT);
#endif
        return;
    }

    TextureMapperLayer* layer = toTextureMapperLayer(currentRootLayer);

    if (!layer) {
#if ENABLE(TIZEN_SET_INITIAL_COLOR_OF_WEBVIEW_EVAS_IMAGE) && ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
        if (m_angle == 90 || m_angle == 270)
            glViewport(0, 0, clipRect.height(), clipRect.width());
        glClearColor(m_backgroundColor.red() / 255.0f, m_backgroundColor.green() / 255.0f, m_backgroundColor.blue() / 255.0f, m_backgroundColor.alpha() / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT);
#endif
        return;
    }

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    static_cast<TextureMapperGL*>(m_textureMapper.get())->setAngle(m_angle);
#endif

    layer->setTextureMapper(m_textureMapper.get());
    if (!m_animationsLocked)
        layer->applyAnimationsRecursively();

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    // FIXME : This is work around patch.
    // After a popup exits, unbind backup texture is sometimes failed (0x505), so unbindSurface is called here.
    if (m_isDirectRendering)
        m_textureMapper->unbindSurface();
#endif

    m_textureMapper->beginPainting(PaintFlags);
    m_textureMapper->beginClip(TransformationMatrix(), clipRect);

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    if (m_drawingAreaProxy->page()->fullScreenManager()) {
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
        if (m_drawingAreaProxy->page()->fullScreenManager()->isUsingHwVideoOverlay()) {
            m_textureMapper->setFullScreenCompositingEnabled(m_drawingAreaProxy->page()->fullScreenManager()->compositingEnabled());
            glClearColor(0.0, 0.0, 0.0, 0.0);
#else
        if (m_drawingAreaProxy->page()->fullScreenManager()->isFullScreen() && layer->hasFullScreenLayerForVideoIncludingSelf()) {
            glClearColor(0.0, 0.0, 0.0, 1.0);
#endif
            glClear(GL_COLOR_BUFFER_BIT);
            m_textureMapper->setFullScreenForVideoMode(true);
        }
    }

    if (!m_textureMapper->fullScreenForVideoMode())
#endif
    if(m_setDrawsBackground)
        m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), m_backgroundColor);
    else {
        glClearColor(m_backgroundColor.red() / 255.0f, m_backgroundColor.green() / 255.0f, m_backgroundColor.blue() / 255.0f, m_backgroundColor.alpha() / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (m_layerTreeCoordinatorProxy->isLowScaleLayerFlag() && !m_backgroundColorHasAlpha)
        m_textureMapper->setLowScaleLayerCompositingEnabled(true);
    else
        m_textureMapper->setLowScaleLayerCompositingEnabled(false);
#endif

    if (currentRootLayer->opacity() != opacity || currentRootLayer->transform() != matrix) {
        currentRootLayer->setOpacity(opacity);
        currentRootLayer->setTransform(matrix);
        currentRootLayer->syncCompositingStateForThisLayerOnly();
    }

#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)
    layer->paintWithCuller(clipRect);
#else
    layer->paint();
#endif
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get(), clipRect.location(), matrix);
    m_textureMapper->endClip();
    m_textureMapper->endPainting();
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    m_textureMapper->setFullScreenForVideoMode(false);
#endif

    if (layer->descendantsOrSelfHaveRunningAnimations()) {
#if PLATFORM(EFL)
        m_updateViewportTimer.startOneShot(0);
#else
        callOnMainThread(WTF::bind(&WebLayerTreeRenderer::updateViewport, this));
#endif
    }
}

#if PLATFORM(QT) || PLATFORM(EFL)
void WebLayerTreeRenderer::paintToGraphicsContext(BackingStore::PlatformGraphicsContext painter)
{
    if (!m_textureMapper)
        m_textureMapper = TextureMapper::create();
    ASSERT(m_textureMapper->accelerationMode() == TextureMapper::SoftwareMode);
    TextureMapperLayer* layer = toTextureMapperLayer(rootLayer());

    if (!layer && !m_setDrawsBackground)
        return;

    GraphicsContext graphicsContext(painter);
    m_textureMapper->setGraphicsContext(&graphicsContext);
    m_textureMapper->beginPainting();

    IntRect clipRect = graphicsContext.clipBounds();
    if (m_setDrawsBackground)
        m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), m_backgroundColor);
    if (layer)
        layer->paint();
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get(), clipRect.location());
    m_textureMapper->endPainting();
    m_textureMapper->setGraphicsContext(0);
}
#endif

void WebLayerTreeRenderer::setContentsSize(const WebCore::FloatSize& contentsSize)
{
    m_contentsSize = contentsSize;
}

void WebLayerTreeRenderer::setVisibleContentsRect(const IntRect& rect, float scale, const WebCore::FloatPoint& accurateVisibleContentsPosition)
{
    m_visibleContentsRect = rect;
    m_contentsScale = scale;
    m_accurateVisibleContentsPosition = accurateVisibleContentsPosition;
}

#if PLATFORM(EFL)
void WebLayerTreeRenderer::updateViewportFired()
{
    callOnMainThread(WTF::bind(&WebLayerTreeRenderer::updateViewport, this));
}
#endif

void WebLayerTreeRenderer::updateViewport()
{
    if (m_layerTreeCoordinatorProxy)
        m_layerTreeCoordinatorProxy->updateViewport();
}

void WebLayerTreeRenderer::adjustPositionForFixedLayers()
{
    if (m_fixedLayers.isEmpty())
        return;

    // Fixed layer positions are updated by the web process when we update the visible contents rect / scroll position.
    // If we want those layers to follow accurately the viewport when we move between the web process updates, we have to offset
    // them by the delta between the current position and the position of the viewport used for the last layout.
    FloatPoint scrollPosition = boundedScrollPosition(FloatPoint(m_accurateVisibleContentsPosition.x() / m_contentsScale, m_accurateVisibleContentsPosition.y() / m_contentsScale), m_visibleContentsRect, m_contentsSize);
    FloatPoint renderedScrollPosition = boundedScrollPosition(m_renderedContentsScrollPosition, m_visibleContentsRect, m_contentsSize);
    FloatSize delta = scrollPosition - renderedScrollPosition;

    LayerMap::iterator end = m_fixedLayers.end();
    for (LayerMap::iterator it = m_fixedLayers.begin(); it != end; ++it)
        toTextureMapperLayer(it->second)->setScrollPositionDeltaIfNeeded(delta);
}

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
void WebLayerTreeRenderer::adjustPositionForOverflowLayers()
{
    LayerMap::iterator end = m_scrollingContentsLayers.end();
    for (LayerMap::iterator it = m_scrollingContentsLayers.begin(); it != end; ++it) {
        GraphicsLayer* contentsLayer = it->second;
        TextureMapperLayer* textureMapperLayer = contentsLayer ? toTextureMapperLayer(contentsLayer) : 0;
        if (!textureMapperLayer)
            continue;

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR)
        const Vector<GraphicsLayer*>& childLayers = contentsLayer->children();
        for (size_t i = 0; i < childLayers.size(); ++i)
            if (childLayers[i]->isScrollbar())
                toTextureMapperLayer(childLayers[i])->setScrollPositionDeltaIfNeeded(FloatSize(-contentsLayer->boundsOrigin().x(), -contentsLayer->boundsOrigin().y()));
#endif
        textureMapperLayer->setScrollPositionDeltaIfNeeded(FloatSize(contentsLayer->boundsOrigin() - contentsLayer->position()));
    }
}

void WebLayerTreeRenderer::setFocusedLayerID(const WebLayerID id)
{
    m_focusedLayerID = id;

    LayerMap::iterator it = m_scrollingContentsLayers.find(id);
    if (it == m_scrollingContentsLayers.end())
        return;

    GraphicsLayer* contentsLayer = it->second;
    if (!contentsLayer)
        return;

    contentsLayer->setNeedsSyncOverflowScrolling(false);
}

bool WebLayerTreeRenderer::setOffset(const WebLayerID id, const FloatPoint& offset)
{
    LayerMap::iterator it = m_scrollingContentsLayers.find(id);
    if (it == m_scrollingContentsLayers.end())
        return false;

    GraphicsLayer* contentsLayer = it->second;
    GraphicsLayer* scrollingLayer = contentsLayer ? contentsLayer->parent() : 0;
    if (!scrollingLayer || contentsLayer->needsSyncOverflowScrolling())
        return false;

    const IntSize contentLayerSize(contentsLayer->size().width(), contentsLayer->size().height());
    const IntRect boundaryRect(FloatRect(FloatPoint(0, 0), scrollingLayer->size()));
    const IntRect visibleRect(FloatRect(-contentsLayer->boundsOrigin().x(),
                                        -contentsLayer->boundsOrigin().y(),
                                        scrollingLayer->size().width(),
                                        scrollingLayer->size().height()));


    IntRect newVisibleRect = visibleRect;
    newVisibleRect.moveBy(flooredIntPoint(offset));

    if (newVisibleRect.x() < boundaryRect.x())
        newVisibleRect.setX(boundaryRect.x());
    if (contentLayerSize.width() - newVisibleRect.x() < boundaryRect.maxX())
        newVisibleRect.setX(contentLayerSize.width() - boundaryRect.width());
    if (newVisibleRect.y() < boundaryRect.y())
        newVisibleRect.setY(boundaryRect.y());
    if (contentLayerSize.height() - newVisibleRect.y() < boundaryRect.maxY())
        newVisibleRect.setY(contentLayerSize.height() - boundaryRect.height());

    if (visibleRect == newVisibleRect)
        return false;

    contentsLayer->setBoundsOrigin(FloatPoint(-newVisibleRect.x(), -newVisibleRect.y()));

    FloatPoint userOffset = FloatPoint(newVisibleRect.x() - visibleRect.x(), newVisibleRect.y() - visibleRect.y());

    m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::SetVisibleContentsLocationAndTrajectoryVectorForLayer(id, visibleRect.location(), userOffset), m_drawingAreaProxy->page()->pageID());

    return true;
}

void WebLayerTreeRenderer::setVisibleContentsRectForScrollingContentsLayers(const WebCore::IntRect& visibleRect)
{
    LayerMap::iterator end = m_scrollingContentsLayers.end();
    for (LayerMap::iterator it = m_scrollingContentsLayers.begin(); it != end; ++it) {
        GraphicsLayer* contentsLayer = it->second;
        GraphicsLayer* scrollingLayer = contentsLayer ? contentsLayer->parent() : 0;
        if (!scrollingLayer)
            continue;

        // FIXME: We might need to check if the each content layer is in viewport.
        // Simply, we should find out a intersected rect between visibleRect and each scrollingLayer here.
        const IntPoint newVisibleLocation = IntPoint(contentsLayer->boundsOrigin().x(), contentsLayer->boundsOrigin().y());

        m_drawingAreaProxy->page()->process()->send(Messages::LayerTreeCoordinator::SetVisibleContentsLocationAndTrajectoryVectorForLayer(it->first, newVisibleLocation, WebCore::FloatPoint()), m_drawingAreaProxy->page()->pageID());
    }
}
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
void WebLayerTreeRenderer::syncCanvas(WebLayerID id, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer, int flags)
#else
void WebLayerTreeRenderer::syncCanvas(WebLayerID id, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer)
#endif
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
    } else
        canvasBackingStore = it->second;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    canvasBackingStore->setGraphicsSurface(graphicsSurfaceToken, canvasSize, frontBuffer, flags);
#else
    canvasBackingStore->setGraphicsSurface(graphicsSurfaceToken, canvasSize, frontBuffer);
#endif
    layer->setContentsToMedia(canvasBackingStore.get());
#endif
}

void WebLayerTreeRenderer::setLayerChildren(WebLayerID id, const Vector<WebLayerID>& childIDs)
{
    ensureLayer(id);
    LayerMap::iterator it = m_layers.find(id);
    GraphicsLayer* layer = it->second;
    Vector<GraphicsLayer*> children;

    for (size_t i = 0; i < childIDs.size(); ++i) {
        WebLayerID childID = childIDs[i];
        GraphicsLayer* child = layerByID(childID);
        if (!child) {
            child = createLayer(childID).leakPtr();
            m_layers.add(childID, child);
        }
        children.append(child);
    }
    layer->setChildren(children);
}

#if ENABLE(CSS_FILTERS)
void WebLayerTreeRenderer::setLayerFilters(WebLayerID id, const FilterOperations& filters)
{
    ensureLayer(id);
    LayerMap::iterator it = m_layers.find(id);
    ASSERT(it != m_layers.end());

    GraphicsLayer* layer = it->second;
    layer->setFilters(filters);
}
#endif

void WebLayerTreeRenderer::setLayerState(WebLayerID id, const WebLayerInfo& layerInfo)
{
    ensureLayer(id);
    LayerMap::iterator it = m_layers.find(id);
    ASSERT(it != m_layers.end());

    GraphicsLayer* layer = it->second;

    layer->setReplicatedByLayer(layerByID(layerInfo.replica));
    layer->setMaskLayer(layerByID(layerInfo.mask));

    layer->setPosition(layerInfo.pos);
    layer->setSize(layerInfo.size);
    layer->setTransform(layerInfo.transform);
    layer->setAnchorPoint(layerInfo.anchorPoint);
    layer->setChildrenTransform(layerInfo.childrenTransform);
    layer->setBackfaceVisibility(layerInfo.backfaceVisible);
    layer->setContentsOpaque(layerInfo.contentsOpaque);
    layer->setContentsRect(layerInfo.contentsRect);
    layer->setContentsToBackgroundColor(layerInfo.backgroundColor);
    layer->setDrawsContent(layerInfo.drawsContent);
    layer->setContentsVisible(layerInfo.contentsVisible);
    toGraphicsLayerTextureMapper(layer)->setFixedToViewport(layerInfo.fixedToViewport);

    if (layerInfo.fixedToViewport)
        m_fixedLayers.add(id, layer);
    else
        m_fixedLayers.remove(id);

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    if (layerInfo.needsSyncOverflowScrolling)
        layer->setNeedsSyncOverflowScrolling(true);

    if (layerInfo.isScrollingContentsLayer) {
        LayerMap::iterator it = m_scrollingContentsLayers.find(id);
        if (it == m_scrollingContentsLayers.end() || (layer->boundsOrigin() != layerInfo.boundsOrigin)) {
            GraphicsLayer* scrollingLayer = layerByID(layerInfo.parent);
            if (layer->needsSyncOverflowScrolling() && scrollingLayer) {
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

    assignImageToLayer(layer, layerInfo.imageBackingStoreID);

    // Never make the root layer clip.
    layer->setMasksToBounds(layerInfo.isRootLayer ? false : layerInfo.masksToBounds);
    layer->setOpacity(layerInfo.opacity);
    layer->setPreserves3D(layerInfo.preserves3D);
    if (layerInfo.isRootLayer && m_rootLayerID != id)
        setRootLayerID(id);
}

void WebLayerTreeRenderer::deleteLayer(WebLayerID layerID)
{
    GraphicsLayer* layer = layerByID(layerID);
    if (!layer)
        return;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    LOG(AcceleratedCompositing, "[UI ] delete layer %u @WebLayerTreeRenderer::deleteLayer \n", layerID);
#endif

    layer->removeFromParent();
    m_layers.remove(layerID);
    m_fixedLayers.remove(layerID);
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    m_scrollingContentsLayers.remove(layerID);
#endif
#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    m_surfaceBackingStores.remove(layerID);
#endif
    delete layer;
}


void WebLayerTreeRenderer::ensureLayer(WebLayerID id)
{
    // We have to leak the new layer's pointer and manage it ourselves,
    // because OwnPtr is not copyable.
    if (m_layers.find(id) == m_layers.end())
        m_layers.add(id, createLayer(id).leakPtr());
}

void WebLayerTreeRenderer::setRootLayerID(WebLayerID layerID)
{
    if (layerID == m_rootLayerID)
        return;

    m_rootLayerID = layerID;

    m_rootLayer->removeAllChildren();

    if (!layerID)
        return;

    GraphicsLayer* layer = layerByID(layerID);
    if (!layer)
        return;

    m_rootLayer->addChild(layer);
}

PassRefPtr<LayerBackingStore> WebLayerTreeRenderer::getBackingStore(WebLayerID id)
{
    TextureMapperLayer* layer = toTextureMapperLayer(layerByID(id));
    ASSERT(layer);
    RefPtr<LayerBackingStore> backingStore = static_cast<LayerBackingStore*>(layer->backingStore().get());
    if (!backingStore) {
        backingStore = LayerBackingStore::create();
        layer->setBackingStore(backingStore.get());
    }
    ASSERT(backingStore);
    return backingStore;
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void WebLayerTreeRenderer::createTile(WebLayerID layerID, int tileID, float scale, bool isLowScaleTile)
#else
void WebLayerTreeRenderer::createTile(WebLayerID layerID, int tileID, float scale)
#endif
{
#if PLATFORM(TIZEN)
    if (!layerByID(layerID))
        return;
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (isLowScaleTile)
        getBackingStore(layerID)->createTile(tileID, scale, isLowScaleTile);
    else
        getBackingStore(layerID)->createTile(tileID, scale);
#else
    getBackingStore(layerID)->createTile(tileID, scale);
#endif
}

void WebLayerTreeRenderer::removeBackingStoreIfNeeded(WebLayerID layerID)
{
    TextureMapperLayer* layer = toTextureMapperLayer(layerByID(layerID));
    ASSERT(layer);
    RefPtr<LayerBackingStore> backingStore = static_cast<LayerBackingStore*>(layer->backingStore().get());
    ASSERT(backingStore);
    if (backingStore->isEmpty())
        layer->setBackingStore(0);
}

void WebLayerTreeRenderer::removeTile(WebLayerID layerID, int tileID)
{
#if PLATFORM(TIZEN)
    // Check whether composited graphics layer already been detached
    GraphicsLayer* pGraphicsLayer = layerByID(layerID);
    if (!pGraphicsLayer)
        return;
#endif
    getBackingStore(layerID)->removeTile(tileID);
    removeBackingStoreIfNeeded(layerID);
}

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
void WebLayerTreeRenderer::updateTileWithUpdateInfo(WebLayerID layerID, int tileID, const TileUpdate& update)
{
    updateTile(layerID, tileID, update);
}
#endif

void WebLayerTreeRenderer::updateTile(WebLayerID layerID, int tileID, const TileUpdate& update)
{
#if PLATFORM(TIZEN)
    if (!layerByID(layerID))
        return;
#endif

    RefPtr<LayerBackingStore> backingStore = getBackingStore(layerID);
    backingStore->updateTile(tileID, update.sourceRect, update.targetRect, update.surface, update.offset);
    m_backingStoresWithPendingBuffers.add(backingStore);
}

void WebLayerTreeRenderer::createImage(int64_t imageID, PassRefPtr<ShareableBitmap> weakBitmap)
{
    RefPtr<ShareableBitmap> bitmap = weakBitmap;
    RefPtr<TextureMapperTiledBackingStore> backingStore = TextureMapperTiledBackingStore::create();
    m_directlyCompositedImages.set(imageID, backingStore);
    backingStore->updateContents(m_textureMapper.get(), bitmap->createImage().get());
}

void WebLayerTreeRenderer::destroyImage(int64_t imageID)
{
    m_directlyCompositedImages.remove(imageID);
}

void WebLayerTreeRenderer::assignImageToLayer(GraphicsLayer* layer, int64_t imageID)
{
    if (!imageID) {
        layer->setContentsToMedia(0);
        return;
    }

    HashMap<int64_t, RefPtr<TextureMapperBackingStore> >::iterator it = m_directlyCompositedImages.find(imageID);
    ASSERT(it != m_directlyCompositedImages.end());
    layer->setContentsToMedia(it->second.get());
}

void WebLayerTreeRenderer::commitTileOperations()
{
    HashSet<RefPtr<LayerBackingStore> >::iterator end = m_backingStoresWithPendingBuffers.end();
    for (HashSet<RefPtr<LayerBackingStore> >::iterator it = m_backingStoresWithPendingBuffers.begin(); it != end; ++it)
        (*it)->commitTileOperations(m_textureMapper.get());

    m_backingStoresWithPendingBuffers.clear();
}

void WebLayerTreeRenderer::flushLayerChanges(const IntPoint& scrollPosition)
{
    m_renderedContentsScrollPosition = scrollPosition;

    // Since the frame has now been rendered, we can safely unlock the animations until the next layout.
    setAnimationsLocked(false);

    m_rootLayer->syncCompositingState(FloatRect());

    commitTileOperations();

    // The pending tiles state is on its way for the screen, tell the web process to render the next one.
    callOnMainThread(WTF::bind(&WebLayerTreeRenderer::renderNextFrame, this));
}

void WebLayerTreeRenderer::renderNextFrame()
{
    if (m_layerTreeCoordinatorProxy)
        m_layerTreeCoordinatorProxy->renderNextFrame();
}

void WebLayerTreeRenderer::ensureRootLayer()
{
    if (m_rootLayer)
        return;

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    createTextureMapper();
#else
    if (!m_textureMapper) {
        m_textureMapper = TextureMapper::create(TextureMapper::OpenGLMode);
        static_cast<TextureMapperGL*>(m_textureMapper.get())->setEnableEdgeDistanceAntialiasing(true);
    }
#endif

    m_rootLayer = createLayer(InvalidWebLayerID);
    m_rootLayer->setMasksToBounds(false);
    m_rootLayer->setDrawsContent(false);
    m_rootLayer->setAnchorPoint(FloatPoint3D(0, 0, 0));

    // The root layer should not have zero size, or it would be optimized out.
    m_rootLayer->setSize(FloatSize(1.0, 1.0));
    toTextureMapperLayer(m_rootLayer.get())->setTextureMapper(m_textureMapper.get());
}

void WebLayerTreeRenderer::syncRemoteContent()
{
    // We enqueue messages and execute them during paint, as they require an active GL context.
    ensureRootLayer();

    for (size_t i = 0; i < m_renderQueue.size(); ++i)
        m_renderQueue[i]();

    m_renderQueue.clear();
}

void WebLayerTreeRenderer::purgeGLResources()
{
    TextureMapperLayer* layer = toTextureMapperLayer(rootLayer());

    if (layer)
        layer->clearBackingStoresRecursive();

    m_directlyCompositedImages.clear();
#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
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

    callOnMainThread(WTF::bind(&WebLayerTreeRenderer::purgeBackingStores, this));
}

void WebLayerTreeRenderer::setLayerAnimations(WebLayerID id, const GraphicsLayerAnimations& animations)
{
    GraphicsLayerTextureMapper* layer = toGraphicsLayerTextureMapper(layerByID(id));
    if (!layer)
        return;
    layer->setAnimations(animations);
}

void WebLayerTreeRenderer::setAnimationsLocked(bool locked)
{
    m_animationsLocked = locked;
}

void WebLayerTreeRenderer::purgeBackingStores()
{
    if (m_layerTreeCoordinatorProxy)
        m_layerTreeCoordinatorProxy->purgeBackingStores();
}

void WebLayerTreeRenderer::detach()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    m_renderQueue.clear();
#endif
    m_layerTreeCoordinatorProxy = 0;
}

void WebLayerTreeRenderer::appendUpdate(const Function<void()>& function)
{
    if (!m_isActive)
        return;

    m_renderQueue.append(function);
}

void WebLayerTreeRenderer::setActive(bool active)
{
    if (m_isActive == active)
        return;

    // Have to clear render queue in both cases.
    // If there are some updates in queue during activation then those updates are from previous instance of paint node
    // and cannot be applied to the newly created instance.
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (!active)
#endif
    m_renderQueue.clear();
    m_isActive = active;
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
void WebLayerTreeRenderer::platformLayerChanged(WebCore::GraphicsLayer*, WebCore::PlatformLayer* oldPlatformLayer, WebCore::PlatformLayer* newPlatformLayer)
{
    if (m_isActive)
        renderNextFrame();
}
#endif

void WebLayerTreeRenderer::clearBackingStores()
{
    LayerMap::iterator end = m_layers.end();
    for(LayerMap::iterator it = m_layers.begin(); it != end; ++it)
        toTextureMapperLayer(it->second)->clearBackingStore();

    m_directlyCompositedImages.clear();
    m_backingStoresWithPendingBuffers.clear();
}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC)

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
void WebLayerTreeRenderer::showBackupTexture(const TransformationMatrix& matrix, float opacity, const FloatRect& clipRect)
{
    if (!m_textureMapper)
        m_textureMapper = TextureMapper::create(TextureMapper::OpenGLMode);
    ASSERT(m_textureMapper->accelerationMode() == TextureMapper::OpenGLMode);

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    if (m_drawingAreaProxy->page()->fullScreenManager()) {
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
        if (m_drawingAreaProxy->page()->fullScreenManager()->isUsingHwVideoOverlay()) {
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
            return;
        }
#endif
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    static_cast<TextureMapperGL*>(m_textureMapper.get())->setAngle(m_angle);
#endif

    const PlatformSurfaceTexture* textureGL = static_cast<const PlatformSurfaceTexture*>(m_backupTexture.get());

    // FIXME : This is work around patch.
    // After a popup exits, unbind backup texture is sometimes failed (0x505), so unbindSurface is called here.
    m_textureMapper->unbindSurface();

    m_textureMapper->beginPainting(0);
    m_textureMapper->beginClip(TransformationMatrix(), clipRect);
    m_textureMapper->drawSolidColor(clipRect, TransformationMatrix(), m_backgroundColor);

    if (textureGL) {
        FloatRect targetRect(FloatPoint(), textureGL->size());

        TransformationMatrix finalMatrix = matrix;
        if (m_angle == 0) {
            finalMatrix.translate(0, textureGL->size().height());
            finalMatrix.flipY();
        } else if (m_angle == 90) {
            glViewport(0, 0, clipRect.height(), clipRect.width());
            finalMatrix.translate(-textureGL->size().width(), textureGL->size().height());
            finalMatrix.flipY();
        } else if (m_angle == 270) {
            glViewport(0, 0, clipRect.height(), clipRect.width());
            finalMatrix.flipY();
        }

        m_textureMapper->drawTexture(textureGL->id(), 0, textureGL->size(), targetRect, finalMatrix, 1.0, textureGL->size(), false);
    }

    m_textureMapper->endClip();
    m_textureMapper->endPainting();
}
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
void WebLayerTreeRenderer::createTextureMapper()
{
    if (!m_textureMapper)
        m_textureMapper = m_isGLMode ? TextureMapper::create(TextureMapper::OpenGLMode) : TextureMapper::create();

    if (m_isGLMode)
        ASSERT(m_textureMapper->accelerationMode() == TextureMapper::OpenGLMode);
    else
        ASSERT(m_textureMapper->accelerationMode() == TextureMapper::SoftwareMode);
}
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
void WebLayerTreeRenderer::setLowScaleNonCompositedLayerID(WebLayerID layerID)
{
    m_lowScaleCompositedLayerID = layerID;
}

void WebLayerTreeRenderer::setNonCompositedLayerID(WebLayerID layerID)
{
    m_nonCompositedLayerID = layerID;
}

void WebLayerTreeRenderer::setPaintOnlyLowScaleLayer(bool b)
{
   if (m_textureMapper)
      m_textureMapper->setPaintOnlyLowScaleLayer(b);
}
#endif

void WebLayerTreeRenderer::setBackgroundColor(const Color& color)
{
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (!color.hasAlpha())
        m_backgroundColorHasAlpha = false;
    else
        m_backgroundColorHasAlpha = true;
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (color != Color::transparent)
#endif
    m_backgroundColor = color;
}

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
void WebLayerTreeRenderer::setDirectRendering(bool b)
{
    if (!m_textureMapper)
        m_textureMapper = m_isGLMode ? TextureMapper::create(TextureMapper::OpenGLMode) : TextureMapper::create();

    m_textureMapper->setDirectRendering(b);
    m_isDirectRendering = b;
}
#endif

} // namespace WebKit

#endif // USE(UI_SIDE_COMPOSITING)
