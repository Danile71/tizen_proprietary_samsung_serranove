/*
*   Copyright (C) 2012 Samsung Electronics
*
*   This library is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Library General Public
*   License as published by the Free Software Foundation; either
*   version 2 of the License, or (at your option) any later version.
*
*   This library is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Library General Public License for more details.
*
*   You should have received a copy of the GNU Library General Public License
*   along with this library; see the file COPYING.LIB.  If not, write to
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
*   Boston, MA 02110-1301, USA.
*/

#ifndef TiledBackingStoreRemoteTileTizen_h
#define TiledBackingStoreRemoteTileTizen_h

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "TiledBackingStoreRemoteTile.h"
#include "SharedPlatformSurfaceTizen.h"
#include "efl/tizen/PlatformSurfacePoolTizen.h"

namespace WebCore {
class ImageBuffer;
class TiledBackingStore;
}

namespace WebKit {

class TiledBackingStoreRemoteTileClient;
class SurfaceUpdateInfo;

class PlatformSurfacePoolTizen;
class TiledBackingStoreRemoteTileBufferTizen {
public:
    TiledBackingStoreRemoteTileBufferTizen(TiledBackingStoreRemoteTileClient* client, WebCore::TiledBackingStore* tiledBackingStore, IntRect rect)
        : m_platformSurfaceID(0)
        , m_basePlatformSurfaceID(0)
        , m_client(client)
        , m_tiledBackingStore(tiledBackingStore)
        , m_rect(rect)
        , m_partialUpdate(false) { };
    bool drawPlatformSurface(const IntSize& size, const IntRect& dirty, int tileID);
    bool drawPlatformLowScaleSurface(const IntSize& size, const IntRect& dirty, int tileID);

    int platformSurfaceID() { return m_platformSurfaceID;}
    int basePlatformSurfaceID() { return m_basePlatformSurfaceID;}
    void resize(const WebCore::IntSize& size) { m_rect.setSize(size); }
    IntSize platformSurfaceSize() { return m_platformSurfaceSize; }
    bool isPartialUpdate() { return m_partialUpdate; }
private:
    int m_platformSurfaceID;
    int m_basePlatformSurfaceID;
    IntSize m_platformSurfaceSize;
    TiledBackingStoreRemoteTileClient* m_client;
    WebCore::TiledBackingStore* m_tiledBackingStore;
    IntRect m_rect;
    bool m_partialUpdate;
};

class TiledBackingStoreRemoteTileTizen : public TiledBackingStoreRemoteTile {
public:
    static PassRefPtr<Tile> create(TiledBackingStoreRemoteTileClient* client, WebCore::TiledBackingStore* tiledBackingStore, const Coordinate& tileCoordinate) { return adoptRef(new TiledBackingStoreRemoteTileTizen(client, tiledBackingStore, tileCoordinate)); }
    ~TiledBackingStoreRemoteTileTizen() {}

    Vector<WebCore::IntRect> updateBackBuffer();

    void resize(const WebCore::IntSize&);

    void setLowScaleTile(bool isLowScaleTile) { m_isLowScaleTile = isLowScaleTile;}
    bool isLowScaleTile() { return m_isLowScaleTile; }
private:
    TiledBackingStoreRemoteTileTizen(TiledBackingStoreRemoteTileClient*, WebCore::TiledBackingStore*, const Coordinate&);
    OwnPtr<TiledBackingStoreRemoteTileBufferTizen> m_tiledBackingStoreRemoteTileBufferTizen;
    bool m_isLowScaleTile;
};


class TiledBackingStoreRemoteTileBackendTizen : public WebCore::TiledBackingStoreBackend {
public:
    static PassOwnPtr<WebCore::TiledBackingStoreBackend> create(TiledBackingStoreRemoteTileClient* client) { return adoptPtr(new TiledBackingStoreRemoteTileBackendTizen(client)); }
    PassRefPtr<WebCore::Tile> createTile(WebCore::TiledBackingStore*, const WebCore::Tile::Coordinate&);
    void paintCheckerPattern(WebCore::GraphicsContext*, const WebCore::FloatRect&) {}
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    virtual bool isGLAccelerationMode() const { return true; }
#endif
private:
    TiledBackingStoreRemoteTileBackendTizen(TiledBackingStoreRemoteTileClient*);
    TiledBackingStoreRemoteTileClient* m_client;
};

}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

#endif

