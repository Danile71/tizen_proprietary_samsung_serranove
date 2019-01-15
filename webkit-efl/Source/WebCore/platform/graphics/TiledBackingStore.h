/*
 Copyright (C) 2010-2012 Nokia Corporation and/or its subsidiary(-ies)
 
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

#ifndef TiledBackingStore_h
#define TiledBackingStore_h

#if USE(TILED_BACKING_STORE)

#include "FloatPoint.h"
#include "IntPoint.h"
#include "IntRect.h"
#include "Tile.h"
#include "TiledBackingStoreBackend.h"
#include "Timer.h"
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>

const float DEFAULT_COVER_AREA_MULTIPLIER = 2.3f;
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
const float DEFAULT_LOWSCALE_COVER_AREA_MULTIPLIER = 8.0f;
const float SMALL_LOWSCALE_COVER_AREA_MULTIPLIER = 2.0f;
#endif

namespace WebCore {

class GraphicsContext;
class TiledBackingStore;
class TiledBackingStoreClient;

class TiledBackingStore {
    WTF_MAKE_NONCOPYABLE(TiledBackingStore); WTF_MAKE_FAST_ALLOCATED;
public:
    TiledBackingStore(TiledBackingStoreClient*, PassOwnPtr<TiledBackingStoreBackend> = TiledBackingStoreBackend::create());
    ~TiledBackingStore();

    TiledBackingStoreClient* client() { return m_client; }

    void coverWithTilesIfNeeded(const FloatPoint& panningTrajectoryVector = FloatPoint());

    float contentsScale() { return m_contentsScale; }
    void setContentsScale(float);

    bool contentsFrozen() const { return m_contentsFrozen; }
    void setContentsFrozen(bool);

    void updateTileBuffers();
#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    void startTileBufferUpdateTimer();
    void updateTileBuffersBegin();
    void setContentsInvalid(bool invalid) { m_contentsInvalid = invalid; }
    bool dirtyTileExist() { return !m_dirtyTiles.isEmpty(); }
#endif

    void invalidate(const IntRect& dirtyRect);
    void paint(GraphicsContext*, const IntRect&);

    IntSize tileSize() { return m_tileSize; }
    void setTileSize(const IntSize&);

    double tileCreationDelay() const { return m_tileCreationDelay; }
    void setTileCreationDelay(double delay);

    IntRect mapToContents(const IntRect&) const;
    IntRect mapFromContents(const IntRect&) const;

    IntRect tileRectForCoordinate(const Tile::Coordinate&) const;
    Tile::Coordinate tileCoordinateForPoint(const IntPoint&) const;
    double tileDistance(const IntRect& viewport, const Tile::Coordinate&) const;

    bool visibleAreaIsCovered() const;
    void removeAllNonVisibleTiles();

    void setSupportsAlpha(bool);
    bool supportsAlpha() const { return m_supportsAlpha; }

#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
    void setUpdateOnlyVisibleAreaState(bool state);
#endif
#if ENABLE(TIZEN_COVERAREA_BY_MEMORY_USAGE)
    void setCoverAreaMultiplier(double coverAreaMultiplier);
    double backingStoreMemoryEstimate();
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setLowScaleNonCompositedBackingStore(bool);
    void setCanInvalidateLowScaleBackingStore(bool canInvalidate) { m_canInvalidateLowScaleBackingStore = canInvalidate; }
#endif

private:
#if !ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    void startTileBufferUpdateTimer();
#endif
    void startBackingStoreUpdateTimer();

    void tileBufferUpdateTimerFired(Timer<TiledBackingStore>*);
    void backingStoreUpdateTimerFired(Timer<TiledBackingStore>*);

    void createTiles();
    void computeCoverAndKeepRect(const IntRect& visibleRect, IntRect& coverRect, IntRect& keepRect) const;

    bool isBackingStoreUpdatesSuspended() const;
    bool isTileBufferUpdatesSuspended() const;

    void commitScaleChange();

    bool resizeEdgeTiles();
    void setCoverRect(const IntRect& rect) { m_coverRect = rect; }
    void setKeepRect(const IntRect&);

    PassRefPtr<Tile> tileAt(const Tile::Coordinate&) const;
    void setTile(const Tile::Coordinate& coordinate, PassRefPtr<Tile> tile);
    void removeTile(const Tile::Coordinate& coordinate);

    IntRect visibleRect() const;

    float coverageRatio(const IntRect&) const;
    void adjustForContentsRect(IntRect&) const;

    void paintCheckerPattern(GraphicsContext*, const IntRect&, const Tile::Coordinate&);
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    IntSize computeAppropriateTileSize(const IntRect&, const IntRect&);
#endif

private:
    TiledBackingStoreClient* m_client;
    OwnPtr<TiledBackingStoreBackend> m_backend;

    typedef HashMap<Tile::Coordinate, RefPtr<Tile> > TileMap;
    TileMap m_tiles;

    Timer<TiledBackingStore> m_tileBufferUpdateTimer;
    Timer<TiledBackingStore> m_backingStoreUpdateTimer;

    IntSize m_tileSize;
    double m_tileCreationDelay;
    float m_coverAreaMultiplier;

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    Vector<RefPtr<Tile> > m_dirtyTiles;
    bool m_contentsInvalid;
#endif

#if ENABLE(TIZEN_WEBKIT2_PRE_RENDERING_WITH_DIRECTIVITY)
    float m_trajectoryVectorScalar;
#endif
    FloatPoint m_trajectoryVector;
    IntRect m_visibleRect;

    IntRect m_coverRect;
    IntRect m_keepRect;
    IntRect m_rect;

    float m_contentsScale;
    float m_pendingScale;

    bool m_contentsFrozen;
    bool m_supportsAlpha;

#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
    bool m_updateOnlyVisibleArea;
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    bool m_isLowScaleNonCompositedBackingStore;
    bool m_needUpdateDelay;
    bool m_canInvalidateLowScaleBackingStore;
#endif

    friend class Tile;
};

}

#endif
#endif
