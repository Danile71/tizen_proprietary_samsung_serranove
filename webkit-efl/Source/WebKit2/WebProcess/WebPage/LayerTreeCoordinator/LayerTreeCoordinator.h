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

#ifndef LayerTreeCoordinator_h
#define LayerTreeCoordinator_h

#include "LayerTreeContext.h"
#include "LayerTreeHost.h"
#include "Timer.h"
#include "UpdateAtlas.h"
#include "WebGraphicsLayer.h"
#include <WebCore/GraphicsLayerClient.h>
#include <wtf/OwnPtr.h>
#if ENABLE(CSS_SHADERS)
#include "WebCustomFilterProgramProxy.h"
#endif

#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
#include <WebCore/ScrollingCoordinatorCoordinatedGraphics.h>
#endif

namespace WebKit {

class UpdateInfo;
class WebPage;

class LayerTreeCoordinator : public LayerTreeHost, WebCore::GraphicsLayerClient
                           , public WebGraphicsLayerClient
#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
    , public WebCore::ScrollingCoordinatorCoordinatedGraphicsClient
#endif
#if ENABLE(CSS_SHADERS)
    , WebCustomFilterProgramProxyClient
#endif
{
public:
    static PassRefPtr<LayerTreeCoordinator> create(WebPage*);
    virtual ~LayerTreeCoordinator();

    static bool supportsAcceleratedCompositing();

    virtual const LayerTreeContext& layerTreeContext() { return m_layerTreeContext; }
    virtual void setLayerFlushSchedulingEnabled(bool);
    virtual void scheduleLayerFlush();
    virtual void setShouldNotifyAfterNextScheduledLayerFlush(bool);
    virtual void setRootCompositingLayer(WebCore::GraphicsLayer*);
    virtual void invalidate();

    virtual void setNonCompositedContentsNeedDisplay(const WebCore::IntRect&);
    virtual void scrollNonCompositedContents(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset);
    virtual void forceRepaint();
    virtual void sizeDidChange(const WebCore::IntSize& newSize);

    virtual void didInstallPageOverlay();
    virtual void didUninstallPageOverlay();
    virtual void setPageOverlayNeedsDisplay(const WebCore::IntRect&);
    virtual void setPageOverlayOpacity(float);
    virtual bool pageOverlayShouldApplyFadeWhenPainting() const { return false; }

#if ENABLE(TIZEN_WEBKIT2_TILED_AC) && !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    virtual void pauseRendering();
#else
    virtual void pauseRendering() { m_isSuspended = true; }
#endif
    virtual void resumeRendering() { m_isSuspended = false; scheduleLayerFlush(); }
    virtual void deviceScaleFactorDidChange() { }
    virtual int64_t adoptImageBackingStore(WebCore::Image*);
    virtual void releaseImageBackingStore(int64_t);

    virtual void createTile(WebLayerID, int tileID, const SurfaceUpdateInfo&, const WebCore::IntRect&);
    virtual void updateTile(WebLayerID, int tileID, const SurfaceUpdateInfo&, const WebCore::IntRect&);
    virtual void removeTile(WebLayerID, int tileID);
    virtual WebCore::IntRect visibleContentsRect() const;
    virtual void renderNextFrame();
    virtual void purgeBackingStores();
    virtual bool layerTreeTileUpdatesAllowed() const;
    virtual void setVisibleContentsRect(const WebCore::IntRect&, float scale, const WebCore::FloatPoint&);
#if PLATFORM(TIZEN)
    virtual void setVisibleContentsRectAndScaleForBackingStore(const WebCore::IntRect& rect,const float newScale);
#endif
    virtual void didReceiveLayerTreeCoordinatorMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    virtual void syncLayerState(WebLayerID, const WebLayerInfo&);
    virtual void syncLayerChildren(WebLayerID, const Vector<WebLayerID>&);
    virtual void setLayerAnimations(WebLayerID, const WebCore::GraphicsLayerAnimations&);

#if ENABLE(CSS_FILTERS)
    virtual void syncLayerFilters(WebLayerID, const WebCore::FilterOperations&);
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    virtual void syncCanvas(WebLayerID, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer, int flags) OVERRIDE;
#else
    virtual void syncCanvas(WebLayerID, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer) OVERRIDE;
#endif
    virtual void attachLayer(WebCore::WebGraphicsLayer*);
    virtual void detachLayer(WebCore::WebGraphicsLayer*);
    virtual void syncFixedLayers();

    virtual PassOwnPtr<WebCore::GraphicsContext> beginContentUpdate(const WebCore::IntSize&, ShareableBitmap::Flags, ShareableSurface::Handle&, WebCore::IntPoint&);
#if USE(UI_SIDE_COMPOSITING)
    virtual void scheduleAnimation() OVERRIDE;
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    virtual bool recordingSurfaceSetEnableGet();
    virtual bool recordingSurfaceSetLoadStartGet();
    virtual bool recordingSurfaceSetLoadFinishedGet();
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    virtual void freePlatformSurface(int layerID, int platformSurfaceId);
    virtual void freePlatformSurfaceByTileID(int tileID);
    virtual void removePlatformSurface(int layerID, int platformSurfaceId);
#endif

#if ENABLE(TIZEN_ONESHOT_DRAWING_SYNCHRONIZATION)
    virtual void setNeedsOneShotDrawingSynchronization();
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    virtual void platformLayerChanged(WebCore::GraphicsLayer*, WebCore::PlatformLayer* oldPlatformLayer, WebCore::PlatformLayer* newPlatformLayer) { ASSERT_NOT_REACHED(); }
    virtual void addOrUpdateScrollingLayer(WebCore::GraphicsLayer* scrollingLayer, WebCore::GraphicsLayer* contentsLayer, const WebCore::IntSize& scrollSize); 
    virtual void removeScrollingLayer(WebCore::GraphicsLayer* scrollingLayer, WebCore::GraphicsLayer* contentsLayer);
#endif // ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    virtual void setVisibleContentsLocationAndTrajectoryVectorForLayer(int layerID, const WebCore::IntPoint&, const WebCore::FloatPoint&);
#endif

#if ENABLE(TIZEN_WEBKIT2)
    virtual float contentsScaleFactor() const OVERRIDE { return m_contentsScale; };
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    virtual void setAccelerationMode(bool mode) { m_accelerationMode = (mode ? OpenGLMode : SoftwareMode); }
    virtual AccelerationMode accelerationMode() const { return m_accelerationMode; }
#endif
#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
    void updateOnlyVisibleAreaState(bool state);
#endif

#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
    // ScrollingCoordinatorCoordinatedGraphicsClient
    virtual void setTouchEventTargetRects(const Vector<WebCore::IntRect>& absoluteHitTestRects);
#endif

    virtual void setBackgroundColor(const WebCore::Color&);

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setCoverWithTilesForLowScaleLayerOnly(bool);
#endif

protected:
    explicit LayerTreeCoordinator(WebPage*);

private:
    // GraphicsLayerClient
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double time);
    virtual void notifySyncRequired(const WebCore::GraphicsLayer*);
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& clipRect);
    virtual bool showDebugBorders(const WebCore::GraphicsLayer*) const;
    virtual bool showRepaintCounter(const WebCore::GraphicsLayer*) const;

    // LayerTreeCoordinator
    void createPageOverlayLayer();
    void destroyPageOverlayLayer();
    bool flushPendingLayerChanges();
    void cancelPendingLayerFlush();
    void performScheduledLayerFlush();
    void didPerformScheduledLayerFlush();
    void lockAnimations();
    void unlockAnimations();

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void createLowScaleNonCompositedLayer();
    void destroyLowScaleNonCompositedLayer();
#endif
#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    bool recordingSurfaceAllowed();
    int compositedContentLayersCount(GraphicsLayer*);
#endif
#if ENABLE(TIZEN_LAYER_FLUSH_THROTTLING)
    void setDeferLayerFlush(bool deferLayerFlushEnabled);
    float deferredLayerFlushDelay();
#endif

    void scheduleReleaseInactiveAtlases();

    void releaseInactiveAtlasesTimerFired(WebCore::Timer<LayerTreeCoordinator>*);
#if ENABLE(CSS_SHADERS)
    // WebCustomFilterProgramProxyClient
    void removeCustomFilterProgramProxy(WebCustomFilterProgramProxy*);

    void checkCustomFilterProgramProxies(const WebCore::FilterOperations&);
    void disconnectCustomFilterPrograms();
#endif

    OwnPtr<WebCore::GraphicsLayer> m_rootLayer;

    // The layer which contains all non-composited content.
    OwnPtr<WebCore::GraphicsLayer> m_nonCompositedContentLayer;

    // The page overlay layer. Will be null if there's no page overlay.
    OwnPtr<WebCore::GraphicsLayer> m_pageOverlayLayer;

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    // The low scale non composited layer.
    OwnPtr<WebCore::GraphicsLayer> m_lowScaleNonCompositedLayer;
#endif

    HashSet<WebCore::WebGraphicsLayer*> m_registeredLayers;
    HashMap<int64_t, int> m_directlyCompositedImageRefCounts;
    Vector<OwnPtr<UpdateAtlas> > m_updateAtlases;
#if ENABLE(CSS_SHADERS)
    HashSet<WebCustomFilterProgramProxy*> m_customFilterPrograms;
#endif

    bool m_notifyAfterScheduledLayerFlush;
    bool m_isValid;

    bool m_waitingForUIProcess;
    bool m_isSuspended;
    WebCore::IntRect m_visibleContentsRect;
    float m_contentsScale;

    LayerTreeContext m_layerTreeContext;
    bool m_shouldSyncFrame;
    bool m_shouldSyncRootLayer;
    void layerFlushTimerFired(WebCore::Timer<LayerTreeCoordinator>*);
    WebCore::Timer<LayerTreeCoordinator> m_layerFlushTimer;
    WebCore::Timer<LayerTreeCoordinator> m_releaseInactiveAtlasesTimer;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    void performScheduledLayerFlushForcely();
    RefPtr<CoreIPC::Connection::WorkItem> m_layerFlushWorkItemToDispatchWhileWaitingForSyncReply;
#endif
    bool m_layerFlushSchedulingEnabled;
#if ENABLE(TIZEN_LAYER_FLUSH_THROTTLING)
    float m_layerFlushTimerDelay;
    bool m_deferLayerFlushEnabled;
    int m_layerFlushTimerDelayLevel;
    static const int s_layerFlushTimerDelayMaxLevel = 5;
    static const float s_layerFlushTimerDelayTable[s_layerFlushTimerDelayMaxLevel];
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    HashSet<uint32_t> m_compositedContentLayers;
    bool m_suspendedJavaScript;
#endif
#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
    HashSet<uint32_t> m_compositedContentCanvas2DLayers;
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    int m_compositedContentLayersCount;
#endif

#if ENABLE(TIZEN_ONESHOT_DRAWING_SYNCHRONIZATION)
    bool m_needsOneShotDrawingSynchronization;
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    AccelerationMode m_accelerationMode;
#endif
    bool m_animationsLocked;
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    WebCore::IntRect m_fixedVisibleContentRect;
#endif
#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
    bool m_touchEventTargetRectsChanged;
    Vector<WebCore::IntRect> m_touchEventTargetRects;
#endif
#if ENABLE(REQUEST_ANIMATION_FRAME)
    double m_lastAnimationServiceTime;
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    bool m_updateLowScaleLayerOnly;
    bool m_refreshAllLayer;
#endif

#if ENABLE(TIZEN_DEFER_FIXED_LAYER_UPDATE)
    bool m_needUpdateFixedLayerPosition;
#endif
};

}

#endif // LayerTreeCoordinator_h
