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

#ifndef WebLayerTreeRenderer_h
#define WebLayerTreeRenderer_h

#if USE(UI_SIDE_COMPOSITING)
#include "BackingStore.h"
#include "GraphicsSurface.h"
#include "ShareableSurface.h"
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
#include "SurfaceUpdateInfo.h"
#endif
#include "TextureMapper.h"
#include "TextureMapperBackingStore.h"
#include "TextureMapperFPSCounter.h"
#include "WebLayerTreeInfo.h"
#include "WebPageGroup.h"
#include "WebPageProxy.h"
#include "WebPreferences.h"
#include <WebCore/GraphicsContext.h>
#include <WebCore/GraphicsLayer.h>
#include <WebCore/GraphicsLayerAnimation.h>
#include <WebCore/IntRect.h>
#include <WebCore/IntSize.h>
#include <WebCore/RunLoop.h>
#include <WebCore/Timer.h>
#include <wtf/Functional.h>
#include <wtf/HashSet.h>
#include <wtf/ThreadingPrimitives.h>

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
#include "DrawingAreaProxy.h"
#endif

namespace WebKit {

class LayerBackingStore;
class LayerTreeCoordinatorProxy;
class WebLayerInfo;
class WebLayerUpdateInfo;

class WebLayerTreeRenderer : public ThreadSafeRefCounted<WebLayerTreeRenderer>, public WebCore::GraphicsLayerClient {
public:
    struct TileUpdate {
        WebCore::IntRect sourceRect;
        WebCore::IntRect targetRect;
        RefPtr<ShareableSurface> surface;
        WebCore::IntPoint offset;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
        int platformSurfaceID;
        WebCore::IntSize platformSurfaceSize;
        bool partialUpdate;
        bool isLowScaleTile;
        TileUpdate(const WebCore::IntRect& source, const WebCore::IntRect& target, PassRefPtr<ShareableSurface> newSurface, const WebCore::IntPoint& newOffset, int pmID, WebCore::IntSize pmSize, bool partialUpdate, bool isLowScaleTile)
#else
        TileUpdate(const WebCore::IntRect& source, const WebCore::IntRect& target, PassRefPtr<ShareableSurface> newSurface, const WebCore::IntPoint& newOffset)
#endif
            : sourceRect(source)
            , targetRect(target)
            , surface(newSurface)
            , offset(newOffset)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
            , platformSurfaceID(pmID)
            , platformSurfaceSize(pmSize)
            , partialUpdate(partialUpdate)
            , isLowScaleTile(isLowScaleTile)
#endif
        {
        }
    };
    WebLayerTreeRenderer(LayerTreeCoordinatorProxy*);
    virtual ~WebLayerTreeRenderer();

    TIZEN_VIRTUAL void purgeGLResources();
    void paintToCurrentGLContext(const WebCore::TransformationMatrix&, float, const WebCore::FloatRect&, WebCore::TextureMapper::PaintFlags = 0);
    void paintToGraphicsContext(BackingStore::PlatformGraphicsContext);
    void syncRemoteContent();
    void setContentsSize(const WebCore::FloatSize&);
    void setVisibleContentsRect(const WebCore::IntRect&, float scale, const WebCore::FloatPoint& accurateVisibleContentsPosition);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    TIZEN_VIRTUAL void syncCanvas(uint32_t id, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer, int flags);
#else
    void syncCanvas(uint32_t id, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer);
#endif

    TIZEN_VIRTUAL void detach();
    TIZEN_VIRTUAL void appendUpdate(const Function<void()>&);
    void updateViewport();
    TIZEN_VIRTUAL void setActive(bool);
    TIZEN_VIRTUAL void deleteLayer(WebLayerID);
    void setRootLayerID(WebLayerID);
    void setLayerChildren(WebLayerID, const Vector<WebLayerID>&);
    TIZEN_VIRTUAL void setLayerState(WebLayerID, const WebLayerInfo&);

#if ENABLE(CSS_FILTERS)
    void setLayerFilters(WebLayerID, const WebCore::FilterOperations&);
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    void createTile(WebLayerID, int, float scale, bool isLowScaleTile = false);
#else
    void createTile(WebLayerID, int, float scale);
#endif
    TIZEN_VIRTUAL void removeTile(WebLayerID, int);

    void updateTile(WebLayerID, int, const TileUpdate&);
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    virtual void updateTileWithUpdateInfo(WebLayerID, int, const TileUpdate&);
    virtual bool isUsingPlatformSurface() { return false; }
#endif
    TIZEN_VIRTUAL void flushLayerChanges(const WebCore::IntPoint& scrollPosition);

    void createImage(int64_t, PassRefPtr<ShareableBitmap>);
    void destroyImage(int64_t);
    void setLayerAnimations(WebLayerID, const WebCore::GraphicsLayerAnimations&);
    void setAnimationsLocked(bool);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    WebLayerTreeRenderer(LayerTreeCoordinatorProxy*, DrawingAreaProxy*, bool isGLMode = true);
    TIZEN_VIRTUAL void clearBackingStores();

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    virtual void freePlatformSurface() { }
    virtual bool hasPlatformSurfaceToFree() { return false; }

    virtual void updatePlatformSurfaceTile(WebLayerID, int tileID, const WebCore::IntRect& sourceRect, const WebCore::IntRect& targetRect, int platformSurfaceID, const WebCore::IntSize& pixplatformSurfaceSizemapSize, bool partialUpdate) { }
#endif
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    int scrollingContentsLayerCounts() const { return m_scrollingContentsLayers.size(); }
    virtual void platformLayerChanged(WebCore::GraphicsLayer*, WebCore::PlatformLayer* oldPlatformLayer, WebCore::PlatformLayer* newPlatformLayer);
#endif
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    void setAngle(int angle) { m_angle = angle; }
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    void paintToBackupTexture(const WebCore::TransformationMatrix&, float, const WebCore::IntRect&, const WebCore::IntSize&);
    void showBackupTexture(const WebCore::TransformationMatrix&, float, const WebCore::FloatRect&);
#endif
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    WebLayerID focusedLayerID() { return m_focusedLayerID; }
    void setFocusedLayerID(const WebLayerID);
    bool setOffset(const WebLayerID, const WebCore::FloatPoint&);
    void setVisibleContentsRectForScrollingContentsLayers(const WebCore::IntRect&);
#endif
#if PLATFORM(EFL)
    void updateViewportFired();
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setLowScaleNonCompositedLayerID(WebLayerID);
    void setNonCompositedLayerID(WebLayerID layerID);
    void setPaintOnlyLowScaleLayer(bool b);
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    WebCore::Color backgroundColor() { return m_backgroundColor; }
#endif
    void setBackgroundColor(const WebCore::Color&);
    void setDrawsBackground(bool enable) { m_setDrawsBackground = enable; }

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    void setIsProviderAC(bool b) { m_isProviderAC = b; }
#endif

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    void setDirectRendering(bool b);
#endif

private:
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
protected:
    virtual void createTextureMapper();
#endif
    PassOwnPtr<WebCore::GraphicsLayer> createLayer(WebLayerID);

    WebCore::GraphicsLayer* layerByID(WebLayerID id) { return (id == InvalidWebLayerID) ? 0 : m_layers.get(id); }
    WebCore::GraphicsLayer* rootLayer() { return m_rootLayer.get(); }

    // Reimplementations from WebCore::GraphicsLayerClient.
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double) { }
    virtual void notifySyncRequired(const WebCore::GraphicsLayer*) { }
#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    virtual void forceRepaint() {}
#endif
    virtual bool showDebugBorders(const WebCore::GraphicsLayer*) const { return m_drawingAreaProxy->page()->pageGroup()->preferences()->compositingBordersVisible(); }
    virtual bool showRepaintCounter(const WebCore::GraphicsLayer*) const { return false; }
    void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect&) { }
    void callOnMainThread(const Function<void()>&);
    void adjustPositionForFixedLayers();
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    void adjustPositionForOverflowLayers();
#endif

    typedef HashMap<WebLayerID, WebCore::GraphicsLayer*> LayerMap;
    WebCore::FloatSize m_contentsSize;
    WebCore::IntRect m_visibleContentsRect;
    WebCore::FloatPoint m_accurateVisibleContentsPosition;
    float m_contentsScale;

    // Render queue can be accessed ony from main thread or updatePaintNode call stack!
    Vector<Function<void()> > m_renderQueue;

#if USE(TEXTURE_MAPPER)
    OwnPtr<WebCore::TextureMapper> m_textureMapper;
    TIZEN_VIRTUAL PassRefPtr<LayerBackingStore> getBackingStore(WebLayerID);
    HashMap<int64_t, RefPtr<WebCore::TextureMapperBackingStore> > m_directlyCompositedImages;
    HashSet<RefPtr<LayerBackingStore> > m_backingStoresWithPendingBuffers;
#endif
#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    typedef HashMap<WebLayerID, RefPtr<WebCore::TextureMapperSurfaceBackingStore> > SurfaceBackingStoreMap;
    SurfaceBackingStoreMap m_surfaceBackingStores;
#endif

    void removeBackingStoreIfNeeded(WebLayerID);
    void scheduleWebViewUpdate();
    void synchronizeViewport();
    void assignImageToLayer(WebCore::GraphicsLayer*, int64_t imageID);
    void ensureRootLayer();
    void ensureLayer(WebLayerID);
    void commitTileOperations();
    void syncAnimations();
    TIZEN_VIRTUAL void renderNextFrame();
    TIZEN_VIRTUAL void purgeBackingStores();
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    void clearPlatformLayerPlatformSurfaces();
#endif

    LayerTreeCoordinatorProxy* m_layerTreeCoordinatorProxy;
    OwnPtr<WebCore::GraphicsLayer> m_rootLayer;
    Vector<WebLayerID> m_layersToDelete;

    LayerMap m_layers;
    LayerMap m_fixedLayers;
    WebLayerID m_rootLayerID;

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    WebLayerID m_lowScaleCompositedLayerID;
    WebLayerID m_nonCompositedLayerID;
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    LayerMap m_scrollingContentsLayers;
#endif
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    WebLayerID m_focusedLayerID;
#endif

    WebCore::IntPoint m_renderedContentsScrollPosition;
    bool m_isActive;
    bool m_animationsLocked;

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    int m_angle;
    bool m_isDirectRendering;
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    DrawingAreaProxy* m_drawingAreaProxy;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    RefPtr<BitmapTexture> m_backupTexture;
#endif

    bool m_isGLMode;
#endif
#if PLATFORM(EFL)
    WebCore::RunLoop::Timer<WebLayerTreeRenderer> m_updateViewportTimer;
#endif
    WebCore::TextureMapperFPSCounter m_fpsCounter;

    WebCore::Color m_backgroundColor;
    bool m_setDrawsBackground;

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    bool m_isProviderAC;
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    bool m_backgroundColorHasAlpha;
#endif
};

};

#endif // USE(UI_SIDE_COMPOSITING)

#endif // WebLayerTreeRenderer_h
