/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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
#include "WebGraphicsLayer.h"

#include "BackingStore.h"
#include "FloatQuad.h"
#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "LayerTreeCoordinatorProxyMessages.h"
#include "Page.h"
#include "TextureMapperPlatformLayer.h"
#include "TiledBackingStoreRemoteTile.h"
#include "WebPage.h"
#include <wtf/CurrentTime.h>
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>

using namespace WebKit;

namespace WebCore {

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
static const double cSemaphoreWaitTime = std::numeric_limits<double>::max(); // infinite time value for BinarySemaphore
#endif

static HashMap<WebLayerID, WebGraphicsLayer*>& layerByIDMap()
{
    static HashMap<WebLayerID, WebGraphicsLayer*> globalMap;
    return globalMap;
}

WebGraphicsLayer* WebGraphicsLayer::layerByID(WebKit::WebLayerID id)
{
    HashMap<WebLayerID, WebGraphicsLayer*>& table = layerByIDMap();
    HashMap<WebLayerID, WebGraphicsLayer*>::iterator it = table.find(id);
    if (it == table.end())
        return 0;
    return it->second;
}

static WebLayerID toWebLayerID(GraphicsLayer* layer)
{
    return layer ? toWebGraphicsLayer(layer)->id() : 0;
}

void WebGraphicsLayer::didChangeLayerState()
{
    m_shouldSyncLayerState = true;
    if (client())
        client()->notifySyncRequired(this);
}

void WebGraphicsLayer::didChangeAnimations()
{
    m_shouldSyncAnimations = true;
    if (client())
        client()->notifySyncRequired(this);
}

void WebGraphicsLayer::didChangeChildren()
{
    m_shouldSyncChildren = true;
    if (client())
        client()->notifySyncRequired(this);
}

#if ENABLE(CSS_FILTERS)
void WebGraphicsLayer::didChangeFilters()
{
    m_shouldSyncFilters = true;
    if (client())
        client()->notifySyncRequired(this);
}
#endif

void WebGraphicsLayer::setShouldUpdateVisibleRect()
{
    m_shouldUpdateVisibleRect = true;
    for (size_t i = 0; i < children().size(); ++i)
        toWebGraphicsLayer(children()[i])->setShouldUpdateVisibleRect();
    if (replicaLayer())
        toWebGraphicsLayer(replicaLayer())->setShouldUpdateVisibleRect();
}

void WebGraphicsLayer::didChangeGeometry()
{
    didChangeLayerState();
    setShouldUpdateVisibleRect();
}

WebGraphicsLayer::WebGraphicsLayer(GraphicsLayerClient* client)
    : GraphicsLayer(client)
    , m_maskTarget(0)
    , m_inUpdateMode(false)
    , m_shouldUpdateVisibleRect(true)
    , m_shouldSyncLayerState(true)
    , m_shouldSyncChildren(true)
    , m_shouldSyncAnimations(true)
    , m_fixedToViewport(false)
    , m_movingVisibleRect(false)
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    , m_isOverflow(false)
#endif
    , m_canvasNeedsDisplay(false)
    , m_webGraphicsLayerClient(0)
    , m_contentsScale(1)
#if ENABLE(TIZEN_USE_FIXED_SCALE_ANIMATION)
    , m_fixedAnimationScale(1)
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_isLowScaleNonCompositedLayer(false)
    , m_nonCompositedLayer(false)
#endif
    , m_canvasPlatformLayer(0)
    , m_animationStartedTimer(this, &WebGraphicsLayer::animationStartedTimerFired)
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR)
    , m_animationTimer(this, &WebGraphicsLayer::animationTimerFired)
#endif
{
    static WebLayerID nextLayerID = 1;
    m_id = nextLayerID++;
    layerByIDMap().add(id(), this);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    m_layerInfo.isRootLayer = false;
    m_layerInfo.contentType = WebKit::WebLayerInfo::HTMLContentType;
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    m_recordingSurfaceSet = new RecordingSurfaceSet();
    m_recordingSurfaceSetStatus = RecordingSurfaceSetInit;
    m_recordingSurfaceSetIsReplaying = false;
    m_changedZoomSet = false;
    m_dirtyRects.clear();
#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    m_waitForSyncSemaphore.signal();
#endif
#endif
}

WebGraphicsLayer::~WebGraphicsLayer()
{
    layerByIDMap().remove(id());

    if (m_webGraphicsLayerClient) {
        purgeBackingStores();
        m_webGraphicsLayerClient->detachLayer(this);
    }
    willBeDestroyed();
}

void WebGraphicsLayer::willBeDestroyed()
{
    GraphicsLayer::willBeDestroyed();
}

bool WebGraphicsLayer::setChildren(const Vector<GraphicsLayer*>& children)
{
    bool ok = GraphicsLayer::setChildren(children);
    if (!ok)
        return false;
    for (size_t i = 0; i < children.size(); ++i) {
        WebGraphicsLayer* child = toWebGraphicsLayer(children[i]);
        child->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
        child->didChangeLayerState();
    }
    didChangeChildren();
    return true;
}

void WebGraphicsLayer::addChild(GraphicsLayer* layer)
{
    GraphicsLayer::addChild(layer);
    toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(layer)->didChangeLayerState();
    didChangeChildren();
}

void WebGraphicsLayer::addChildAtIndex(GraphicsLayer* layer, int index)
{
    GraphicsLayer::addChildAtIndex(layer, index);
    toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(layer)->didChangeLayerState();
    didChangeChildren();
}

void WebGraphicsLayer::addChildAbove(GraphicsLayer* layer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildAbove(layer, sibling);
    toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(layer)->didChangeLayerState();
    didChangeChildren();
}

void WebGraphicsLayer::addChildBelow(GraphicsLayer* layer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(layer, sibling);
    toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(layer)->didChangeLayerState();
    didChangeChildren();
}

bool WebGraphicsLayer::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    bool ok = GraphicsLayer::replaceChild(oldChild, newChild);
    if (!ok)
        return false;
    didChangeChildren();
    toWebGraphicsLayer(oldChild)->didChangeLayerState();
    toWebGraphicsLayer(newChild)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    toWebGraphicsLayer(newChild)->didChangeLayerState();
    return true;
}

void WebGraphicsLayer::removeFromParent()
{
    if (WebGraphicsLayer* parentLayer = toWebGraphicsLayer(parent()))
        parentLayer->didChangeChildren();
    GraphicsLayer::removeFromParent();

    didChangeLayerState();
}

void WebGraphicsLayer::setPosition(const FloatPoint& p)
{
    if (position() == p)
        return;

    GraphicsLayer::setPosition(p);
    didChangeGeometry();
}

void WebGraphicsLayer::setAnchorPoint(const FloatPoint3D& p)
{
    if (anchorPoint() == p)
        return;

    GraphicsLayer::setAnchorPoint(p);
    didChangeGeometry();
}

void WebGraphicsLayer::setSize(const FloatSize& size)
{
    if (this->size() == size)
        return;

    GraphicsLayer::setSize(size);
    setNeedsDisplay();
    if (maskLayer())
        maskLayer()->setSize(size);
    didChangeGeometry();
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    adjustVisibleRect();
#endif
}

void WebGraphicsLayer::setTransform(const TransformationMatrix& t)
{
    if (transform() == t)
        return;

    GraphicsLayer::setTransform(t);
    didChangeGeometry();
}

void WebGraphicsLayer::setChildrenTransform(const TransformationMatrix& t)
{
    if (childrenTransform() == t)
        return;

    GraphicsLayer::setChildrenTransform(t);
    didChangeGeometry();
}

void WebGraphicsLayer::setPreserves3D(bool b)
{
    if (preserves3D() == b)
        return;

    GraphicsLayer::setPreserves3D(b);
    didChangeGeometry();
}

void WebGraphicsLayer::setMasksToBounds(bool b)
{
    if (masksToBounds() == b)
        return;
    GraphicsLayer::setMasksToBounds(b);
    didChangeGeometry();
}

void WebGraphicsLayer::setDrawsContent(bool b)
{
    if (drawsContent() == b)
        return;
    GraphicsLayer::setDrawsContent(b);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (b)
        setNeedsDisplay();
#endif
    didChangeLayerState();
}

void WebGraphicsLayer::setContentsVisible(bool b)
{
    if (contentsAreVisible() == b)
        return;
    GraphicsLayer::setContentsVisible(b);

    didChangeLayerState();
}

void WebGraphicsLayer::setContentsOpaque(bool b)
{
    if (contentsOpaque() == b)
        return;
    if (m_mainBackingStore)
        m_mainBackingStore->setSupportsAlpha(!b);
    GraphicsLayer::setContentsOpaque(b);
    didChangeLayerState();
}

void WebGraphicsLayer::setBackfaceVisibility(bool b)
{
    if (backfaceVisibility() == b)
        return;

    GraphicsLayer::setBackfaceVisibility(b);
    didChangeLayerState();
}

void WebGraphicsLayer::setOpacity(float opacity)
{
    if (this->opacity() == opacity)
        return;

    GraphicsLayer::setOpacity(opacity);
    didChangeLayerState();
}

void WebGraphicsLayer::setContentsRect(const IntRect& r)
{
    if (contentsRect() == r)
        return;

    GraphicsLayer::setContentsRect(r);
    didChangeLayerState();
}

void WebGraphicsLayer::setContentsNeedsDisplay()
{
    RefPtr<Image> image = m_image;
    setContentsToImage(0);
    setContentsToImage(image.get());
    m_canvasNeedsDisplay = true;
    if (client())
        client()->notifySyncRequired(this);
}


#if ENABLE(CSS_FILTERS)
bool WebGraphicsLayer::setFilters(const FilterOperations& newFilters)
{
#if ENABLE(TIZEN_USE_SW_PATH_FOR_BLUR_FILTER)
    static int threshold = 360 * 360;
    if (newFilters.hasBlurFilter() && (threshold < size().width() * size().height())) {
        if (filters().size()) {
            clearFilters();
            didChangeFilters();
        }
        return false;
    }
#endif

    if (filters() == newFilters)
        return true;
    didChangeFilters();
    return GraphicsLayer::setFilters(newFilters);
}
#endif

void WebGraphicsLayer::setContentsToBackgroundColor(const Color& color)
{
    if (m_layerInfo.backgroundColor == color)
        return;
    m_layerInfo.backgroundColor = color;

    // This is in line with what CA does.
    setBackgroundColor(color);
    didChangeLayerState();
}

void WebGraphicsLayer::setContentsToImage(Image* image)
{
    if (image == m_image)
        return;
    int64_t newID = 0;
    if (m_webGraphicsLayerClient) {
        // We adopt first, in case this is the same frame - that way we avoid destroying and recreating the image.
        newID = m_webGraphicsLayerClient->adoptImageBackingStore(image);
        m_webGraphicsLayerClient->releaseImageBackingStore(m_layerInfo.imageBackingStoreID);
        didChangeLayerState();
        if (m_layerInfo.imageBackingStoreID && newID == m_layerInfo.imageBackingStoreID)
            return;
    } else {
        // If m_webGraphicsLayerClient is not set yet there should be no backing store ID.
        ASSERT(!m_layerInfo.imageBackingStoreID);
        didChangeLayerState();
    }

    m_layerInfo.imageBackingStoreID = newID;
    m_image = image;
    GraphicsLayer::setContentsToImage(image);
}

void WebGraphicsLayer::setContentsToCanvas(PlatformLayer* platformLayer)
{
    m_canvasPlatformLayer = platformLayer;
    m_canvasNeedsDisplay = true;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (!m_canvasPlatformLayer)
        return;

    if (static_cast<TextureMapperPlatformLayer*>(m_canvasPlatformLayer)->contentType() == WebKit::WebLayerInfo::Canvas2DContentType)
        m_layerInfo.contentType = WebKit::WebLayerInfo::Canvas2DContentType;
    else
        m_layerInfo.contentType = WebKit::WebLayerInfo::Canvas3DContentType;
#endif

    if (client())
       client()->notifySyncRequired(this);
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
void WebGraphicsLayer::setContentsToMedia(PlatformLayer* platformLayer)
{
    m_canvasPlatformLayer = platformLayer;
    m_canvasNeedsDisplay = true;
    m_layerInfo.contentType = WebKit::WebLayerInfo::MediaContentType;

    if (client())
        client()->notifySyncRequired(this);
}
#endif

void WebGraphicsLayer::setMaskLayer(GraphicsLayer* layer)
{
    if (layer == maskLayer())
        return;

    GraphicsLayer::setMaskLayer(layer);

    if (!layer)
        return;

    layer->setSize(size());
    WebGraphicsLayer* webGraphicsLayer = toWebGraphicsLayer(layer);
    webGraphicsLayer->setWebGraphicsLayerClient(m_webGraphicsLayerClient);
    webGraphicsLayer->setMaskTarget(this);
    webGraphicsLayer->didChangeLayerState();
    didChangeLayerState();

}

void WebGraphicsLayer::setReplicatedByLayer(GraphicsLayer* layer)
{
    if (layer == replicaLayer())
        return;

    if (layer)
        toWebGraphicsLayer(layer)->setWebGraphicsLayerClient(m_webGraphicsLayerClient);

    GraphicsLayer::setReplicatedByLayer(layer);
    didChangeLayerState();
}

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
void WebGraphicsLayer::setLowScaleNonCompositedLayer(bool isLowScaleNonCompositedLayer)
{
    m_isLowScaleNonCompositedLayer = isLowScaleNonCompositedLayer;
}

void WebGraphicsLayer::setNonCompositedLayer(bool isNonCompositedLayer)
{
    m_nonCompositedLayer = isNonCompositedLayer;
}

void WebGraphicsLayer::setCanInvalidateLowScaleBackingStore(bool canInvalidate)
{
    if (m_mainBackingStore)
        m_mainBackingStore->setCanInvalidateLowScaleBackingStore(canInvalidate);
}
#endif


#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
IntRect WebGraphicsLayer::dirtyUnionRect()
{
    return m_dirtyUnionRect;
}

void WebGraphicsLayer::setDirtyUnionRect(const IntRect& rect)
{
    m_dirtyUnionRect = rect;
}

void WebGraphicsLayer::uniteDirtyUnionRect(const IntRect& rect)
{
    m_dirtyUnionRect.unite(rect);
}

void WebGraphicsLayer::setDirtyRect(const IntRect& rect)
{
    IntRect contentRect(0, 0, size().width(), size().height());

    if (rect.isEmpty() || contentRect.isEmpty())
        return;

    IntRect dirtyRect(intersection(rect, contentRect));
    unsigned size = m_dirtyRects.size();
    if (size) {
        for (int index = size - 1; index > 0; index--) {
            if (m_dirtyRects[index].contains(dirtyRect))
                return;
        }
    }
    m_dirtyRects.append(dirtyRect);
}

void WebGraphicsLayer::dirtyRectInvalidate()
{
    unsigned size = m_dirtyRects.size();

    for (size_t index = 0; index < size; index++)
        m_mainBackingStore->invalidate(m_dirtyRects[index]);
}

bool WebGraphicsLayer::recordingSurfaceSetEnableGet()
{
    if (m_webGraphicsLayerClient)
        if (m_webGraphicsLayerClient->recordingSurfaceSetEnableGet() && m_nonCompositedLayer)
            return true;

    return false;
}

bool WebGraphicsLayer::recordingSurfaceSetLoadStartGet()
{
    if (m_webGraphicsLayerClient)
        return m_webGraphicsLayerClient->recordingSurfaceSetLoadStartGet();
    return false;
}

bool WebGraphicsLayer::recordingSurfaceSetLoadFinishedGet()
{
    if (m_webGraphicsLayerClient)
        return m_webGraphicsLayerClient->recordingSurfaceSetLoadFinishedGet();
    return false;
}

void WebGraphicsLayer::recordingSurfaceSetRebuild(RecordingSurfaceSet* recordingSurfaceSet, float scale)
{
    size_t size = recordingSurfaceSet->size();

    for (size_t index = 0; index < size; index++) {
        if (recordingSurfaceSet->upToDate(index))
            continue;

        const WebCore::IntRect& rect = recordingSurfaceSet->bounds(index);
        cairo_rectangle_t extents = {0, 0, static_cast<float>(rect.width() * scale), static_cast<float>(rect.height() * scale)};
        RefPtr<cairo_surface_t> recordingSurfaceCairo = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, &extents);
        RefPtr<cairo_t> recordingContextCairo = cairo_create(recordingSurfaceCairo.get());

        OwnPtr<WebCore::GraphicsContext> graphicsContext = adoptPtr(new GraphicsContext(recordingContextCairo.get()));
        cairo_save(recordingContextCairo.get());
        graphicsContext->save();
        cairo_translate(recordingContextCairo.get(), -static_cast<int>(rect.x() * scale), -static_cast<int>(rect.y() * scale));
        cairo_scale(recordingContextCairo.get(), scale, scale);
        graphicsContext->clip(rect);
        paintGraphicsLayerContents(*graphicsContext.get(), rect);
        graphicsContext->restore();
        cairo_restore(recordingContextCairo.get());

        recordingSurfaceSet->setRecordingSurface(index, recordingSurfaceCairo.get(), recordingContextCairo.get(), m_mainBackingStore->tileSize());
    }
}

void WebGraphicsLayer::recordingSurfaceSetRecord()
{
    IntRect contentRect(0, 0, size().width(), size().height());
    m_recordingSurfaceSetStatus = RecordingSurfaceSetInit;

    if (recordingSurfaceSetLoadFinishedGet() || recordingSurfaceSetLoadStartGet() || m_changedZoomSet) {
        m_recordingSurfaceSet->clear();
        m_recordingSurfaceSet->add(contentRect, m_contentsScale);
    } else {
        unsigned size = m_dirtyRects.size();
        for (unsigned n = 0; n < size; n++)
            m_recordingSurfaceSet->add(m_dirtyRects[n], m_contentsScale);
    }

    recordingSurfaceSetRebuild(m_recordingSurfaceSet, m_contentsScale);
    m_recordingSurfaceSetStatus = RecordingSurfaceSetComplete;
    m_changedZoomSet = false;
    m_dirtyRects.clear();
}

void WebGraphicsLayer::recordingSurfaceSetReplay(WebCore::GraphicsContext& graphicsContext, const WebCore::IntRect& rect)
{
    if (m_recordingSurfaceSetStatus == RecordingSurfaceSetComplete) {
        RefPtr<cairo_t> cairoContext = ((WebCore::PlatformContextCairo *)graphicsContext.platformContext())->cr();
        m_recordingSurfaceSet->draw(cairoContext.get(), rect);
    }
}

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
bool WebGraphicsLayer::recordingSurfaceSetIsReplayingGet()
{
     return m_recordingSurfaceSetIsReplaying;
}

void WebGraphicsLayer::updateTileBuffers()
{
    m_mainBackingStore->updateTileBuffers();
    m_recordingSurfaceSetIsReplaying = false;
    m_waitForSyncSemaphore.signal();
}

void WebGraphicsLayer::scheduleUpdateTileBuffersAsync()
{
    if (m_mainBackingStore->contentsFrozen() || !tiledBackingStoreUpdatesAllowed())
        return;

    m_waitForSyncSemaphore.wait(cSemaphoreWaitTime);
    m_mainBackingStore->setContentsInvalid(false);
    m_mainBackingStore->updateTileBuffersBegin();
    m_recordingSurfaceSetIsReplaying = true;
    WebProcess::shared().paintThreadWorkQueue()->dispatch(WTF::bind(&WebGraphicsLayer::updateTileBuffers, this));
}

void WebGraphicsLayer::requestSyncForPaintThread()
{
    m_shouldSyncLayerState = true;
    if (client())
        client()->forceRepaint();
}
#endif
#endif

void WebGraphicsLayer::setNeedsDisplay()
{
    setNeedsDisplayInRect(IntRect(IntPoint::zero(), IntSize(size().width(), size().height())));
}

void WebGraphicsLayer::setNeedsDisplayInRect(const FloatRect& rect)
{
#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    if (recordingSurfaceSetEnableGet()) {
        setDirtyRect(IntRect(rect));

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
        if (m_mainBackingStore)
            m_mainBackingStore->startTileBufferUpdateTimer();
#else
        if (m_mainBackingStore)
            m_mainBackingStore->invalidate(IntRect(rect));
        didChangeLayerState();
#endif
        return;
    }
#endif

    if (m_mainBackingStore)
        m_mainBackingStore->invalidate(IntRect(rect));
    didChangeLayerState();
}

WebLayerID WebGraphicsLayer::id() const
{
    return m_id;
}

void WebGraphicsLayer::syncCompositingState(const FloatRect& rect)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (!m_webGraphicsLayerClient)
        return;
#endif

    if (WebGraphicsLayer* mask = toWebGraphicsLayer(maskLayer()))
        mask->syncCompositingStateForThisLayerOnly();

    if (WebGraphicsLayer* replica = toWebGraphicsLayer(replicaLayer()))
        replica->syncCompositingStateForThisLayerOnly();

#if !ENABLE(TIZEN_CSS_FIXED_ACCELERATION)
    m_webGraphicsLayerClient->syncFixedLayers();
#endif

    syncCompositingStateForThisLayerOnly();

    for (size_t i = 0; i < children().size(); ++i)
        children()[i]->syncCompositingState(rect);
}

WebGraphicsLayer* toWebGraphicsLayer(GraphicsLayer* layer)
{
    return static_cast<WebGraphicsLayer*>(layer);
}

void WebGraphicsLayer::syncChildren()
{
    if (!m_shouldSyncChildren)
        return;
    m_shouldSyncChildren = false;
    Vector<WebLayerID> childIDs;
    for (size_t i = 0; i < children().size(); ++i)
        childIDs.append(toWebLayerID(children()[i]));

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (m_webGraphicsLayerClient)
#endif
    m_webGraphicsLayerClient->syncLayerChildren(m_id, childIDs);
}

#if ENABLE(CSS_FILTERS)
void WebGraphicsLayer::syncFilters()
{
    if (!m_shouldSyncFilters)
        return;
    m_shouldSyncFilters = false;
    m_webGraphicsLayerClient->syncLayerFilters(m_id, filters());
}
#endif

void WebGraphicsLayer::syncLayerState()
{
    if (!m_shouldSyncLayerState)
        return;

    m_shouldSyncLayerState = false;
    m_layerInfo.fixedToViewport = fixedToViewport();
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR)
    m_layerInfo.isScrollbar = isScrollbar();
#endif
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    m_layerInfo.isScrollingContentsLayer = isOverflow();
    if (m_layerInfo.isScrollingContentsLayer) {
        m_layerInfo.boundsOrigin = parent() ? parent()->boundsOrigin() : boundsOrigin();
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
        m_layerInfo.needsSyncOverflowScrolling = needsSyncOverflowScrolling();
        setNeedsSyncOverflowScrolling(false);
#endif
    }
#endif

    m_layerInfo.anchorPoint = anchorPoint();
    m_layerInfo.backfaceVisible = backfaceVisibility();
    m_layerInfo.childrenTransform = childrenTransform();
    m_layerInfo.contentsOpaque = contentsOpaque();
    m_layerInfo.contentsRect = contentsRect();
    m_layerInfo.drawsContent = drawsContent();
    m_layerInfo.contentsVisible = contentsAreVisible();
    m_layerInfo.mask = toWebLayerID(maskLayer());
    m_layerInfo.masksToBounds = masksToBounds();
    m_layerInfo.opacity = opacity();
    m_layerInfo.parent = toWebLayerID(parent());
    m_layerInfo.pos = position();
    m_layerInfo.preserves3D = preserves3D();
    m_layerInfo.replica = toWebLayerID(replicaLayer());
    m_layerInfo.size = size();
    m_layerInfo.transform = transform();
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    m_layerInfo.fullScreenLayerForVideo = fullScreenLayerForVideo();
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (m_webGraphicsLayerClient)
#endif
    m_webGraphicsLayerClient->syncLayerState(m_id, m_layerInfo);
}

void WebGraphicsLayer::syncAnimations()
{
    if (!m_shouldSyncAnimations)
        return;

    m_shouldSyncAnimations = false;

    m_webGraphicsLayerClient->setLayerAnimations(m_id, m_animations);
}

void WebGraphicsLayer::syncCanvas()
{
    if (!m_canvasNeedsDisplay)
        return;

    if (!m_canvasPlatformLayer) {
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE) && ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
        if (m_webGraphicsLayerClient) {
            m_webGraphicsLayerClient->syncCanvas(m_id, IntSize(contentsRect().width(), contentsRect().height()), 0, 0, 0);
            m_canvasNeedsDisplay = false;
        }
#endif
        return;
    }

#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    uint64_t token = m_canvasPlatformLayer->graphicsSurfaceToken();
    if (!token) {
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE) && ENABLE(TIZEN_GSTREAMER_VIDEO)
        if (m_webGraphicsLayerClient) {
            m_webGraphicsLayerClient->syncCanvas(m_id, IntSize(contentsRect().width(), contentsRect().height()), 0, 0, 0);
            m_canvasNeedsDisplay = false;
        }
#endif
        return;
    }
    uint32_t frontBuffer = m_canvasPlatformLayer->copyToGraphicsSurface();
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    int flags = m_canvasPlatformLayer->graphicsSurfaceFlags();
    if (m_webGraphicsLayerClient)
        m_webGraphicsLayerClient->syncCanvas(m_id, IntSize(contentsRect().width(), contentsRect().height()), token, frontBuffer, flags);
#else
    m_webGraphicsLayerClient->syncCanvas(m_id, IntSize(size().width(), size().height()), token, frontBuffer);
#endif
#endif
    m_canvasNeedsDisplay = false;
}

void WebGraphicsLayer::ensureImageBackingStore()
{
    if (!m_image)
        return;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (!m_webGraphicsLayerClient)
        return;
#endif

    if (!m_layerInfo.imageBackingStoreID)
        m_layerInfo.imageBackingStoreID = m_webGraphicsLayerClient->adoptImageBackingStore(m_image.get());
}

void WebGraphicsLayer::syncCompositingStateForThisLayerOnly()
{
    // When we have a transform animation, we need to update visible rect every frame to adjust the visible rect of a backing store.
    bool hasActiveTransformAnimation = selfOrAncestorHasActiveTransformAnimation();
    if (hasActiveTransformAnimation)
        m_movingVisibleRect = true;

    // The remote image might have been released by purgeBackingStores.
    ensureImageBackingStore();
    syncLayerState();
    syncAnimations();

    computeTransformedVisibleRect();
    syncChildren();
#if ENABLE(CSS_FILTERS)
    syncFilters();
#endif
    updateContentBuffers();
    syncCanvas();
    // Only unset m_movingVisibleRect after we have updated the visible rect after the animation stopped.
    if (!hasActiveTransformAnimation)
        m_movingVisibleRect = false;
}

void WebGraphicsLayer::tiledBackingStorePaintBegin()
{
#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    if (recordingSurfaceSetEnableGet() && m_mainBackingStore) {
        if (m_dirtyRects.isEmpty())
            return;

        recordingSurfaceSetRecord();
    }
#endif
}

void WebGraphicsLayer::setRootLayer(bool isRoot)
{
    m_layerInfo.isRootLayer = isRoot;
    didChangeLayerState();
}

void WebGraphicsLayer::setVisibleContentRectTrajectoryVector(const FloatPoint& trajectoryVector)
{
#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    if (recordingSurfaceSetEnableGet())
        m_waitForSyncSemaphore.wait(cSemaphoreWaitTime);
#endif

    if (m_mainBackingStore)
        m_mainBackingStore->coverWithTilesIfNeeded(trajectoryVector);

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    if (recordingSurfaceSetEnableGet())
        m_waitForSyncSemaphore.signal();
#endif
}

void WebGraphicsLayer::setContentsScale(float scale)
{
#if ENABLE(TIZEN_USE_FIXED_SCALE_ANIMATION)
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform))
        m_fixedAnimationScale = scale;
#endif

    m_contentsScale = scale;
    adjustContentsScale();
}

float WebGraphicsLayer::effectiveContentsScale()
{
#if ENABLE(TIZEN_USE_FIXED_SCALE_ANIMATION)
    if (m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform))
        return m_fixedAnimationScale;
#endif

    return selfOrAncestorHaveNonAffineTransforms() ? 1 : m_contentsScale;
}

void WebGraphicsLayer::adjustContentsScale()
{
    if (!drawsContent())
        return;

    if (!m_mainBackingStore || m_mainBackingStore->contentsScale() == effectiveContentsScale())
        return;

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    if (recordingSurfaceSetEnableGet()) {
#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
        m_waitForSyncSemaphore.wait(cSemaphoreWaitTime);
#endif
        m_changedZoomSet = true;
        recordingSurfaceSetRecord();
    }
#endif

    // Between creating the new backing store and painting the content,
    // we do not want to drop the previous one as that might result in
    // briefly seeing flickering as the old tiles may be dropped before
    // something replaces them.
#if ENABLE(TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM)
    if (!m_isLowScaleNonCompositedLayer || !m_previousBackingStore)
#endif
    m_previousBackingStore = m_mainBackingStore.release();

    // No reason to save the previous backing store for non-visible areas.
#if ENABLE(TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM)
    if (!m_isLowScaleNonCompositedLayer)
#endif
    m_previousBackingStore->removeAllNonVisibleTiles();

    createBackingStore();

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    if (recordingSurfaceSetEnableGet())
        m_waitForSyncSemaphore.signal();
#endif
}

void WebGraphicsLayer::createBackingStore()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    switch (m_webGraphicsLayerClient->accelerationMode()) {
    case WebGraphicsLayerClient::NotReady:
        TIZEN_LOGI("setAccelerationMode() IPC did not arrived. Defer the creation of BackingStore.");
        return;
    case WebGraphicsLayerClient::SoftwareMode:
        m_mainBackingStore = adoptPtr(new TiledBackingStore(this, TiledBackingStoreRemoteTileBackend::create(this)));
        break;
    case WebGraphicsLayerClient::OpenGLMode:
        m_mainBackingStore = adoptPtr(new TiledBackingStore(this, TiledBackingStoreRemoteTileBackendTizen::create(this)));
        break;
    }
#else
    m_mainBackingStore = adoptPtr(new TiledBackingStore(this, TiledBackingStoreRemoteTileBackendTizen::create(this)));
#endif
#else
    m_mainBackingStore = adoptPtr(new TiledBackingStore(this, TiledBackingStoreRemoteTileBackend::create(this)));
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (m_isLowScaleNonCompositedLayer && m_webGraphicsLayerClient->accelerationMode() == WebGraphicsLayerClient::OpenGLMode)
        m_mainBackingStore->setLowScaleNonCompositedBackingStore(true);
#endif
    m_mainBackingStore->setSupportsAlpha(!contentsOpaque());
    m_mainBackingStore->setContentsScale(effectiveContentsScale());
#if ENABLE(TIZEN_COVERAREA_BY_MEMORY_USAGE)
    m_mainBackingStore->setCoverAreaMultiplier(m_coverAreaMultiplier);
#endif
}

void WebGraphicsLayer::tiledBackingStorePaint(GraphicsContext* context, const IntRect& rect)
{
    if (rect.isEmpty())
        return;

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    if (recordingSurfaceSetEnableGet()) {
        recordingSurfaceSetReplay(*context, rect);
        return;
    }
#endif

    paintGraphicsLayerContents(*context, rect);
}

void WebGraphicsLayer::tiledBackingStorePaintEnd(const Vector<IntRect>& updatedRects)
{
#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    if (recordingSurfaceSetEnableGet() && m_mainBackingStore->dirtyTileExist())
        RunLoop::main()->dispatch(bind(&WebGraphicsLayer::requestSyncForPaintThread, this));
#endif
}

bool WebGraphicsLayer::tiledBackingStoreUpdatesAllowed() const
{
    if (!m_inUpdateMode)
        return false;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (!m_webGraphicsLayerClient)
        return false;
#endif

    return m_webGraphicsLayerClient->layerTreeTileUpdatesAllowed();
}

IntRect WebGraphicsLayer::tiledBackingStoreContentsRect()
{
    return IntRect(0, 0, size().width(), size().height());
}

IntRect WebGraphicsLayer::tiledBackingStoreVisibleRect()
{
    // Non-invertible layers are not visible.
    if (!m_layerTransform.combined().isInvertible())
        return IntRect();

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (!m_webGraphicsLayerClient)
        return IntRect();
#endif

    FloatRect perspectiveRect = m_webGraphicsLayerClient->visibleContentsRect();

    if (m_movingVisibleRect) {
        IntRect visibleRect = enclosingIntRect(m_layerTransform.combined().inverse().clampedBoundsOfProjectedQuad(FloatQuad(perspectiveRect)));
        IntRect endAnimationRect = enclosingIntRect(m_layerSettledTransform.combined().inverse().clampedBoundsOfProjectedQuad(FloatQuad(perspectiveRect)));
        endAnimationRect.unite(visibleRect);

        // Prevent that create too large backingstore.
        static const float visibleRectMultiplierForAnimations = 3;
        visibleRect.inflateX(visibleRect.width() * (visibleRectMultiplierForAnimations - 1) / 2);
        visibleRect.inflateY(visibleRect.height() * (visibleRectMultiplierForAnimations - 1) / 2);

        endAnimationRect.intersect(visibleRect);

        return endAnimationRect;
    }

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR)
    if (isScrollbar()) {
        WebGraphicsLayer* parentLayer = toWebGraphicsLayer(parent());
        if (parentLayer && parentLayer->isOverflow()) {
            GraphicsLayerTransform adjustedTransform = m_layerTransform;
            adjustedTransform.setPosition(FloatPoint(position().x() - parentLayer->position().x(),
                    position().y() - parentLayer->position().y()));
            adjustedTransform.combineTransforms(parent() ? toWebGraphicsLayer(parent())->m_layerTransform.combinedForChildren() : TransformationMatrix());
            return enclosingIntRect(adjustedTransform.combined().inverse().clampedBoundsOfProjectedQuad(FloatQuad(perspectiveRect)));
        }
    }
#endif

    // Return a projection of the visible rect (surface coordinates) onto the layer's plane (layer coordinates).
    // The resulting quad might be squewed and the visible rect is the bounding box of this quad,
    // so it might spread further than the real visible area (and then even more amplified by the cover rect multiplier).
    return enclosingIntRect(m_layerTransform.combined().inverse().clampedBoundsOfProjectedQuad(FloatQuad(perspectiveRect)));
}

Color WebGraphicsLayer::tiledBackingStoreBackgroundColor() const
{
    return contentsOpaque() ? Color::white : Color::transparent;
}

PassOwnPtr<WebCore::GraphicsContext> WebGraphicsLayer::beginContentUpdate(const WebCore::IntSize& size, ShareableSurface::Handle& handle, WebCore::IntPoint& offset)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (!m_webGraphicsLayerClient)
        return PassOwnPtr<WebCore::GraphicsContext>();
#endif

    return m_webGraphicsLayerClient->beginContentUpdate(size, contentsOpaque() ? 0 : ShareableBitmap::SupportsAlpha, handle, offset);
}

void WebGraphicsLayer::createTile(int tileID, const SurfaceUpdateInfo& updateInfo, const IntRect& targetRect)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (m_webGraphicsLayerClient)
#endif
    m_webGraphicsLayerClient->createTile(id(), tileID, updateInfo, targetRect);
}

void WebGraphicsLayer::updateTile(int tileID, const SurfaceUpdateInfo& updateInfo, const IntRect& targetRect)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (m_webGraphicsLayerClient)
#endif
    m_webGraphicsLayerClient->updateTile(id(), tileID, updateInfo, targetRect);
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void WebGraphicsLayer::freePlatformSurface(int platformSurfaceID)
{
    if (m_canvasPlatformLayer)
        m_canvasPlatformLayer->freePlatformSurface(platformSurfaceID);
}

void WebGraphicsLayer::removePlatformSurface(int platformSurfaceID)
{
    if (m_canvasPlatformLayer)
        m_canvasPlatformLayer->removePlatformSurface(platformSurfaceID);
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING) || ENABLE(TIZEN_CANVAS_SURFACE_LOCKING)
    if (m_canvasPlatformLayer && m_layerInfo.contentType == WebKit::WebLayerInfo::Canvas2DContentType)
        m_canvasNeedsDisplay = true;
#endif
#if ENABLE(WEBGL)
    if (m_canvasPlatformLayer && m_layerInfo.contentType == WebKit::WebLayerInfo::Canvas3DContentType)
        m_canvasNeedsDisplay = true;
#endif
}

bool WebGraphicsLayer::swapPlatformSurfaces()
{
    if (m_canvasPlatformLayer)
        return m_canvasPlatformLayer->swapPlatformSurfaces();
    return true;
}
#endif

#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
void WebGraphicsLayer::flushPlatformSurfaces()
{
    if (m_canvasPlatformLayer)
        return m_canvasPlatformLayer->flushPlatformSurfaces();
}
#endif

void WebGraphicsLayer::removeTile(int tileID)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (m_webGraphicsLayerClient)
#endif
    m_webGraphicsLayerClient->removeTile(id(), tileID);
}

void WebGraphicsLayer::updateContentBuffers()
{
    if (!drawsContent()) {
        m_mainBackingStore.clear();
        m_previousBackingStore.clear();
        return;
    }

    m_inUpdateMode = true;
    // This is the only place we (re)create the main tiled backing store, once we
    // have a remote client and we are ready to send our data to the UI process.
    if (!m_mainBackingStore)
        createBackingStore();

#if ENABLE(TIZEN_COVERAREA_BY_MEMORY_USAGE)
    if (m_mainBackingStore)
        m_mainBackingStore->setCoverAreaMultiplier(m_coverAreaMultiplier);
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    if (recordingSurfaceSetEnableGet()) {
        m_waitForSyncSemaphore.wait(cSemaphoreWaitTime);
        m_waitForSyncSemaphore.signal();

        scheduleUpdateTileBuffersAsync();
    } else
        m_mainBackingStore->updateTileBuffers();
#else
    if (m_mainBackingStore)
        m_mainBackingStore->updateTileBuffers();
#endif
    m_inUpdateMode = false;

    // The previous backing store is kept around to avoid flickering between
    // removing the existing tiles and painting the new ones. The first time
    // the visibleRect is full painted we remove the previous backing store.
#if ENABLE(TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM)
    if (m_previousBackingStore)
#endif
    if (m_mainBackingStore && m_mainBackingStore->visibleAreaIsCovered())
        m_previousBackingStore.clear();
}

void WebGraphicsLayer::purgeBackingStores()
{
#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    if (recordingSurfaceSetEnableGet()) {
        m_waitForSyncSemaphore.wait(cSemaphoreWaitTime);
        m_waitForSyncSemaphore.signal();
    }
#endif
    m_mainBackingStore.clear();
    m_previousBackingStore.clear();

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (m_webGraphicsLayerClient)
#endif
    if (m_layerInfo.imageBackingStoreID) {
        m_webGraphicsLayerClient->releaseImageBackingStore(m_layerInfo.imageBackingStoreID);
        m_layerInfo.imageBackingStoreID = 0;
    }

    didChangeLayerState();
    didChangeChildren();
}

void WebGraphicsLayer::setWebGraphicsLayerClient(WebKit::WebGraphicsLayerClient* client)
{
    if (m_webGraphicsLayerClient == client)
        return;

    if (WebGraphicsLayer* replica = toWebGraphicsLayer(replicaLayer()))
        replica->setWebGraphicsLayerClient(client);
    if (WebGraphicsLayer* mask = toWebGraphicsLayer(maskLayer()))
        mask->setWebGraphicsLayerClient(client);
    for (size_t i = 0; i < children().size(); ++i) {
        WebGraphicsLayer* layer = toWebGraphicsLayer(this->children()[i]);
        layer->setWebGraphicsLayerClient(client);
    }

    // We have to release resources on the UI process here if the remote client has changed or is removed.
    if (m_webGraphicsLayerClient) {
        purgeBackingStores();
        m_webGraphicsLayerClient->detachLayer(this);
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
        didChangeAnimations();
#endif
    }
    m_webGraphicsLayerClient = client;
    if (client)
        client->attachLayer(this);
}

void WebGraphicsLayer::adjustVisibleRect()
{
    if (m_mainBackingStore)
        m_mainBackingStore->coverWithTilesIfNeeded();
}

bool WebGraphicsLayer::hasPendingVisibleChanges()
{
    if (opacity() < 0.01 && !m_animations.hasActiveAnimationsOfType(AnimatedPropertyOpacity))
        return false;

    for (size_t i = 0; i < children().size(); ++i) {
        if (toWebGraphicsLayer(children()[i])->hasPendingVisibleChanges())
            return true;
    }

    if (!m_shouldSyncLayerState && !m_shouldSyncChildren && !m_shouldSyncFilters && !m_shouldSyncAnimations && !m_canvasNeedsDisplay)
        return false;

    return tiledBackingStoreVisibleRect().intersects(tiledBackingStoreContentsRect());
}

void WebGraphicsLayer::computeTransformedVisibleRect()
{
    // When we have a transform animation, we need to update visible rect every frame to adjust the visible rect of a backing store.
    if (!m_shouldUpdateVisibleRect && !m_movingVisibleRect)
        return;

    m_shouldUpdateVisibleRect = false;
    TransformationMatrix currentTransform = transform();
    if (m_movingVisibleRect) {
        m_layerSettledTransform.setLocalTransform(currentTransform);
        m_layerSettledTransform.setPosition(position());
        m_layerSettledTransform.setAnchorPoint(anchorPoint());
        m_layerSettledTransform.setSize(size());
        m_layerSettledTransform.setFlattening(!preserves3D());
        m_layerSettledTransform.setChildrenTransform(childrenTransform());
        m_layerSettledTransform.combineTransforms(parent() ? toWebGraphicsLayer(parent())->m_layerTransform.combinedForChildren() : TransformationMatrix());

        client()->getCurrentTransform(this, currentTransform);
    }

    m_layerTransform.setLocalTransform(currentTransform);
    m_layerTransform.setPosition(position());
    m_layerTransform.setAnchorPoint(anchorPoint());
    m_layerTransform.setSize(size());
    m_layerTransform.setFlattening(!preserves3D());
    m_layerTransform.setChildrenTransform(childrenTransform());
    m_layerTransform.combineTransforms(parent() ? toWebGraphicsLayer(parent())->m_layerTransform.combinedForChildren() : TransformationMatrix());

    // The combined transform will be used in tiledBackingStoreVisibleRect.
    adjustVisibleRect();
    adjustContentsScale();
}

static PassOwnPtr<GraphicsLayer> createWebGraphicsLayer(GraphicsLayerClient* client)
{
    return adoptPtr(new WebGraphicsLayer(client));
}

void WebGraphicsLayer::initFactory()
{
    GraphicsLayer::setGraphicsLayerFactory(createWebGraphicsLayer);
}

bool WebGraphicsLayer::selfOrAncestorHasActiveTransformAnimation()
{
    if (m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform))
        return true;

    if (!parent())
        return false;

    return toWebGraphicsLayer(parent())->selfOrAncestorHasActiveTransformAnimation();
}

bool WebGraphicsLayer::selfOrAncestorHaveNonAffineTransforms()
{
    if (m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform))
        return true;

#if !ENABLE(TIZEN_USE_CONTENTS_SCALE)
    if (!m_layerTransform.combined().isAffine())
        return true;
#endif

    return false;
}

#if ENABLE(TIZEN_ANIMATION_TIME_FROM_START)
bool WebGraphicsLayer::addAnimation(const KeyframeValueList& valueList, const IntSize& boxSize, const Animation* anim, const String& keyframesName, double delayAsNegativeTimeOffset)
#else
bool WebGraphicsLayer::addAnimation(const KeyframeValueList& valueList, const IntSize& boxSize, const Animation* anim, const String& keyframesName, double timeOffset)
#endif
{
    ASSERT(!keyframesName.isEmpty());

    if (!anim || anim->isEmptyOrZeroDuration() || valueList.size() < 2 || (valueList.property() != AnimatedPropertyWebkitTransform && valueList.property() != AnimatedPropertyOpacity))
        return false;

    bool listsMatch = false;
    bool ignoredHasBigRotation;

    if (valueList.property() == AnimatedPropertyWebkitTransform)
        listsMatch = validateTransformOperations(valueList, ignoredHasBigRotation) >= 0;

#if ENABLE(TIZEN_ANIMATION_TIME_FROM_START)
    m_lastAnimationStartTime = WTF::currentTime() - delayAsNegativeTimeOffset;
#if ENABLE(TIZEN_ANIMATION_DELAY_START)
    m_animations.add(GraphicsLayerAnimation(keyframesName, valueList, boxSize, anim, -delayAsNegativeTimeOffset, listsMatch));
#else
    m_animations.add(GraphicsLayerAnimation(keyframesName, valueList, boxSize, anim, m_lastAnimationStartTime, listsMatch));
#endif
#else
#if ENABLE(TIZEN_ANIMATION_DELAY_START)
    m_animations.add(GraphicsLayerAnimation(keyframesName, valueList, boxSize, anim, -timeOffset, listsMatch));
#else
    m_animations.add(GraphicsLayerAnimation(keyframesName, valueList, boxSize, anim, WTF::currentTime() - timeOffset, listsMatch));
#endif
#endif

    m_animationStartedTimer.startOneShot(0);
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    didChangeAnimations();
#endif
    didChangeLayerState();
    return true;
}

#if ENABLE(TIZEN_ANIMATION_TIME_FROM_START)
void WebGraphicsLayer::pauseAnimation(const String& animationName, double time)
{
    m_animations.pause(animationName, time);
    didChangeAnimations();
}
#else
void WebGraphicsLayer::pauseAnimation(const String& animationName, double timeOffset)
{
    m_animations.pause(animationName, timeOffset);
    didChangeAnimations();
}
#endif

void WebGraphicsLayer::removeAnimation(const String& animationName)
{
    m_animations.remove(animationName);
    didChangeAnimations();
}

void WebGraphicsLayer::animationStartedTimerFired(Timer<WebGraphicsLayer>*)
{
#if ENABLE(TIZEN_ANIMATION_TIME_FROM_START)
    client()->notifyAnimationStarted(this, m_lastAnimationStartTime);
#else
    client()->notifyAnimationStarted(this, WTF::currentTime());
#endif
}

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
void WebGraphicsLayer::setIsOverflow(const bool b)
{
    if (m_isOverflow == b)
        return;

    m_isOverflow = b;
    didChangeLayerState();
}
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR)
void WebGraphicsLayer::startAnimation()
{
    setOpacity(1.0);
    if (m_animationTimer.isActive())
        m_animationTimer.stop();
    m_animationTimer.startOneShot(1.0);
}

void WebGraphicsLayer::animationTimerFired(Timer<WebGraphicsLayer>*)
{
    setOpacity(0.0);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_MEMORY_SAVING_MODE)
bool WebGraphicsLayer::memorySavingModeEnabled() const
{
    return WebProcess::shared().memorySavingModeEnabled();
}
#endif

#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
bool WebGraphicsLayer::drawTileInfo() const
{
    static bool drawTileInfo = String(getenv("TIZEN_WEBKIT_SHOW_COMPOSITING_DEBUG_VISUALS")) == "1";
    return drawTileInfo;
}
#endif

#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
void WebGraphicsLayer::updateOnlyVisibleAreaState(bool state)
{
    if (!m_mainBackingStore)
        return;

    m_mainBackingStore->setUpdateOnlyVisibleAreaState(state);
}
#endif

#if ENABLE(TIZEN_CSS_FIXED_ACCELERATION)
void WebGraphicsLayer::syncFixedLayers()
{
    if (m_webGraphicsLayerClient)
        m_webGraphicsLayerClient->syncFixedLayers();
}
#endif

#if ENABLE(TIZEN_COVERAREA_BY_MEMORY_USAGE)
double WebGraphicsLayer::backingStoreMemoryEstimate() const
{
    if (!m_mainBackingStore)
        return 0;

    return m_mainBackingStore->backingStoreMemoryEstimate();
}
#endif

}
#endif
