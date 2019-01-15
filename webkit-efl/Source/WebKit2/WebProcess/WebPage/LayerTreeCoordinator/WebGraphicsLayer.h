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


#ifndef WebGraphicsLayer_h
#define WebGraphicsLayer_h

#include "FloatPoint3D.h"
#include "GraphicsLayer.h"
#include "GraphicsLayerAnimation.h"
#include "GraphicsLayerTransform.h"
#include "Image.h"
#include "IntSize.h"
#include "ShareableBitmap.h"
#include "TiledBackingStore.h"
#include "TiledBackingStoreClient.h"
#include "TiledBackingStoreRemoteTile.h"
#include "TransformationMatrix.h"
#include "UpdateInfo.h"
#include "WebLayerTreeInfo.h"
#include "WebProcess.h"
#include <WebCore/RunLoop.h>
#include <wtf/text/StringHash.h>

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
#include "../WebPage/cairo/RecordingSurfaceSetCairo.h"
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
#include <wtf/threads/BinarySemaphore.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "efl/tizen/TiledBackingStoreRemoteTileTizen.h"
#endif

#if USE(UI_SIDE_COMPOSITING)
namespace WebCore {
class WebGraphicsLayer;
class GraphicsLayerAnimations;
}

namespace WebKit {
#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
class RecordingSurfaceSet;
#endif

class WebGraphicsLayerClient {
public:
    // TiledBackingStoreRemoteTileClient
    virtual void createTile(WebLayerID, int tileID, const SurfaceUpdateInfo&, const WebCore::IntRect&) = 0;
    virtual void updateTile(WebLayerID, int tileID, const SurfaceUpdateInfo&, const WebCore::IntRect&) = 0;
    virtual void removeTile(WebLayerID, int tileID) = 0;

    virtual WebCore::IntRect visibleContentsRect() const = 0;
    virtual bool layerTreeTileUpdatesAllowed() const = 0;
    virtual int64_t adoptImageBackingStore(WebCore::Image*) = 0;
    virtual void releaseImageBackingStore(int64_t) = 0;
    virtual void syncLayerState(WebLayerID, const WebLayerInfo&) = 0;
    virtual void syncLayerChildren(WebLayerID, const Vector<WebLayerID>&) = 0;
#if ENABLE(CSS_FILTERS)
    virtual void syncLayerFilters(WebLayerID, const WebCore::FilterOperations&) = 0;
#endif
#if PLATFORM(QT) || PLATFORM(EFL)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    virtual void syncCanvas(WebLayerID, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer, int flags) = 0;
#else
    virtual void syncCanvas(WebLayerID, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer) = 0;
#endif
#endif
    virtual void setLayerAnimations(WebLayerID, const WebCore::GraphicsLayerAnimations&) = 0;

    virtual void attachLayer(WebCore::WebGraphicsLayer*) = 0;
    virtual void detachLayer(WebCore::WebGraphicsLayer*) = 0;
    virtual void syncFixedLayers() = 0;
    virtual PassOwnPtr<WebCore::GraphicsContext> beginContentUpdate(const WebCore::IntSize&, ShareableBitmap::Flags, ShareableSurface::Handle&, WebCore::IntPoint&) = 0;

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    virtual bool recordingSurfaceSetEnableGet() = 0;
    virtual bool recordingSurfaceSetLoadStartGet() = 0;
    virtual bool recordingSurfaceSetLoadFinishedGet() = 0;
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    enum AccelerationMode { NotReady, SoftwareMode, OpenGLMode };
    virtual AccelerationMode accelerationMode() const = 0;
#endif
};
}

namespace WebCore {

class WebGraphicsLayer : public WebCore::GraphicsLayer
                       , public TiledBackingStoreClient
                       , public WebKit::TiledBackingStoreRemoteTileClient {
public:
    WebGraphicsLayer(GraphicsLayerClient*);
    virtual ~WebGraphicsLayer();

    // Reimplementations from GraphicsLayer.h.
    bool setChildren(const Vector<GraphicsLayer*>&);
    void addChild(GraphicsLayer*);
    void addChildAtIndex(GraphicsLayer*, int);
    void addChildAbove(GraphicsLayer*, GraphicsLayer*);
    void addChildBelow(GraphicsLayer*, GraphicsLayer*);
    bool replaceChild(GraphicsLayer*, GraphicsLayer*);
    void removeFromParent();
    void setPosition(const FloatPoint&);
    void setAnchorPoint(const FloatPoint3D&);
    void setSize(const FloatSize&);
    void setTransform(const TransformationMatrix&);
    void setChildrenTransform(const TransformationMatrix&);
    void setPreserves3D(bool);
    void setMasksToBounds(bool);
    void setDrawsContent(bool);
    void setContentsVisible(bool);
    void setContentsOpaque(bool);
    void setBackfaceVisibility(bool);
    void setOpacity(float);
    void setContentsRect(const IntRect&);
    void setContentsToImage(Image*);
    void setContentsToBackgroundColor(const Color&);
    void setContentsToCanvas(PlatformLayer*);
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    void setContentsToMedia(PlatformLayer*);
#endif
    void setMaskLayer(GraphicsLayer*);
    void setReplicatedByLayer(GraphicsLayer*);
    void setNeedsDisplay();
    void setNeedsDisplayInRect(const FloatRect&);
    void setContentsNeedsDisplay();
    void setContentsScale(float);
    void setVisibleContentRectTrajectoryVector(const FloatPoint&);
    virtual void syncCompositingState(const FloatRect&);
    virtual void syncCompositingStateForThisLayerOnly();
#if ENABLE(CSS_FILTERS)
    bool setFilters(const FilterOperations&);
#endif

    void setRootLayer(bool);

    WebKit::WebLayerID id() const;
    static WebGraphicsLayer* layerByID(WebKit::WebLayerID);
    void didSynchronize();
    Image* image() { return m_image.get(); }

    bool fixedToViewport() const { return m_fixedToViewport; }
    void setFixedToViewport(bool isFixed) { m_fixedToViewport = isFixed; }

    GraphicsLayer* maskTarget() const { return m_maskTarget; }
    void setMaskTarget(GraphicsLayer* layer) { m_maskTarget = layer; }

    static void initFactory();

    // TiledBackingStoreClient
    virtual void tiledBackingStorePaintBegin();
    virtual void tiledBackingStorePaint(GraphicsContext*, const IntRect&);
    virtual void tiledBackingStorePaintEnd(const Vector<IntRect>& paintedArea);
    virtual bool tiledBackingStoreUpdatesAllowed() const;
    virtual IntRect tiledBackingStoreContentsRect();
    virtual IntRect tiledBackingStoreVisibleRect();
    virtual Color tiledBackingStoreBackgroundColor() const;

    // TiledBackingStoreRemoteTileClient
    virtual void createTile(int tileID, const WebKit::SurfaceUpdateInfo&, const WebCore::IntRect&);
    virtual void updateTile(int tileID, const WebKit::SurfaceUpdateInfo&, const WebCore::IntRect&);
    virtual void removeTile(int tileID);
    virtual PassOwnPtr<WebCore::GraphicsContext> beginContentUpdate(const WebCore::IntSize&, WebKit::ShareableSurface::Handle&, WebCore::IntPoint&);

    void setWebGraphicsLayerClient(WebKit::WebGraphicsLayerClient*);
    void syncChildren();
    void syncLayerState();
#if ENABLE(CSS_FILTERS)
    void syncFilters();
#endif
    void syncCanvas();
    void ensureImageBackingStore();

    void adjustVisibleRect();
    bool isReadyForTileBufferSwap() const;
    void updateContentBuffers();
    void purgeBackingStores();
    bool hasPendingVisibleChanges();

    virtual bool addAnimation(const KeyframeValueList&, const IntSize&, const Animation*, const String&, double);
    virtual void pauseAnimation(const String&, double);
    virtual void removeAnimation(const String&);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    void freePlatformSurface(int platformSurfaceID);
    void removePlatformSurface(int platformSurfaceID);
    bool swapPlatformSurfaces();
#else
    void markCanvasPlatformLayerNeedsUpdate() { m_canvasNeedsDisplay = true; }
#endif
#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
    void flushPlatformSurfaces();
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    void increaseDrawCount();
#endif

#endif
    int contentType() { return m_layerInfo.contentType; }
#endif

#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
    virtual bool drawTileInfo() const;
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    virtual IntRect dirtyUnionRect();
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setLowScaleNonCompositedLayer(bool);
    void setNonCompositedLayer(bool);
    void setCanInvalidateLowScaleBackingStore(bool);
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    virtual void requestSyncForPaintThread();
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    void setIsOverflow(const bool b);
    bool isOverflow() const { return m_isOverflow; }
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR)
    void startAnimation();
    void animationTimerFired(WebCore::Timer<WebGraphicsLayer>*);
#endif

#if ENABLE(TIZEN_WEBKIT2_MEMORY_SAVING_MODE)
    bool memorySavingModeEnabled() const;
#endif
#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
    void updateOnlyVisibleAreaState(bool state);
#endif
#if ENABLE(TIZEN_CSS_FIXED_ACCELERATION)
    virtual void syncFixedLayers();
#endif
#if ENABLE(TIZEN_COVERAREA_BY_MEMORY_USAGE)
    virtual double backingStoreMemoryEstimate() const;
#endif

private:
    virtual void willBeDestroyed();
    WebKit::WebLayerID m_id;
    WebKit::WebLayerInfo m_layerInfo;
    RefPtr<Image> m_image;
    GraphicsLayer* m_maskTarget;
    FloatRect m_needsDisplayRect;
    GraphicsLayerTransform m_layerTransform;
    GraphicsLayerTransform m_layerSettledTransform;
    bool m_inUpdateMode : 1;
    bool m_shouldUpdateVisibleRect: 1;
    bool m_shouldSyncLayerState: 1;
    bool m_shouldSyncChildren: 1;
    bool m_shouldSyncFilters: 1;
    bool m_shouldSyncAnimations: 1;
    bool m_fixedToViewport : 1;
    bool m_movingVisibleRect : 1;
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    bool m_isOverflow : 1;
#endif
    bool m_canvasNeedsDisplay : 1;

    void notifyChange();
    void didChangeAnimations();
    void didChangeGeometry();
    void didChangeLayerState();
    void didChangeChildren();
#if ENABLE(CSS_FILTERS)
    void didChangeFilters();
#endif

    float m_effectiveOpacity;
    TransformationMatrix m_effectiveTransform;

    void createBackingStore();

    bool selfOrAncestorHasActiveTransformAnimation();
    bool selfOrAncestorHaveNonAffineTransforms();
    void adjustContentsScale();
    void computeTransformedVisibleRect();
    void syncLayerParameters();
    void syncAnimations();
    void setShouldUpdateVisibleRect();
    float effectiveContentsScale();

    void animationStartedTimerFired(WebCore::Timer<WebGraphicsLayer>*);

    WebKit::WebGraphicsLayerClient* m_webGraphicsLayerClient;
    OwnPtr<WebCore::TiledBackingStore> m_mainBackingStore;
    OwnPtr<WebCore::TiledBackingStore> m_previousBackingStore;
    float m_contentsScale;
#if ENABLE(TIZEN_USE_FIXED_SCALE_ANIMATION)
    float m_fixedAnimationScale;
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    bool m_isLowScaleNonCompositedLayer;
    bool m_nonCompositedLayer;
#endif
#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    bool recordingSurfaceSetEnableGet();
    bool recordingSurfaceSetLoadStartGet();
    bool recordingSurfaceSetLoadFinishedGet();
    void recordingSurfaceSetRecord();
    void recordingSurfaceSetReplay(WebCore::GraphicsContext&, const WebCore::IntRect&);
    void recordingSurfaceSetRebuild(WebKit::RecordingSurfaceSet*, float scale);
    void setDirtyUnionRect(const IntRect& rect);
    void uniteDirtyUnionRect(const IntRect& rect);
    void setDirtyRect(const IntRect& rect);
    void dirtyRectInvalidate();

    enum RecordingSurfaceSetStatus {
        RecordingSurfaceSetInit,
        RecordingSurfaceSetComplete
    };

    IntRect m_dirtyUnionRect;
    bool m_changedZoomSet;
    bool m_recordingSurfaceSetIsReplaying;
    RecordingSurfaceSetStatus m_recordingSurfaceSetStatus;
    WebKit::RecordingSurfaceSet* m_recordingSurfaceSet;
    Vector<WebCore::IntRect> m_dirtyRects;

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    void updateTileBuffers();
    void scheduleUpdateTileBuffersAsync();
    bool recordingSurfaceSetIsReplayingGet();
    WTF::BinarySemaphore m_waitForSyncSemaphore;
#endif
#endif
    PlatformLayer* m_canvasPlatformLayer;
    Timer<WebGraphicsLayer> m_animationStartedTimer;
    GraphicsLayerAnimations m_animations;

#if ENABLE(TIZEN_ANIMATION_TIME_FROM_START)
    double m_lastAnimationStartTime;
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR)
    WebCore::Timer<WebGraphicsLayer> m_animationTimer;
#endif
};

WebGraphicsLayer* toWebGraphicsLayer(GraphicsLayer*);

}
#endif

#endif // WebGraphicsLayer_H
