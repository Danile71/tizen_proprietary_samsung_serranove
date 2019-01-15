/*
*   Copyright (C) 2011 Samsung Electronics
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

#include "config.h"
#include "TiledBackingStoreRemoteTileTizen.h"

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "WebProcess.h"
#include <WebCore/CairoUtilities.h>
#include <WebCore/PlatformContextCairo.h>
#include <wtf/RefPtr.h>
#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
#include <stdio.h>
#endif

using namespace WebCore;

namespace WebKit {

TiledBackingStoreRemoteTileBackendTizen::TiledBackingStoreRemoteTileBackendTizen(TiledBackingStoreRemoteTileClient* client)
    : m_client(client)
{
}

PassRefPtr<WebCore::Tile> TiledBackingStoreRemoteTileBackendTizen::createTile(WebCore::TiledBackingStore* tiledBackingStore, const WebCore::Tile::Coordinate& tileCoordinate)
{
    return TiledBackingStoreRemoteTileTizen::create(m_client, tiledBackingStore, tileCoordinate);
}

TiledBackingStoreRemoteTileTizen::TiledBackingStoreRemoteTileTizen(TiledBackingStoreRemoteTileClient* client, TiledBackingStore* tiledBackingStore, const Coordinate& tileCoordinate)
    : TiledBackingStoreRemoteTile(client, tiledBackingStore, tileCoordinate)
    , m_tiledBackingStoreRemoteTileBufferTizen(adoptPtr(new TiledBackingStoreRemoteTileBufferTizen(client, tiledBackingStore, m_rect)))
    , m_isLowScaleTile(false)
{
}

static inline bool needUpdateBackBufferPartially(const IntRect& entireRect, const IntRect& dirtyRect)
{
    // FIXME: apply partial update only for tiles whoes size is larger than 384*384.
    // because otherwise, i.e. for small tiles, partial update takes even longer time than repainting whole tile.
    // we need to tune the threshold tile size through more testing.
    if (entireRect.width() * entireRect.height() < 147456)
        return false;

    if (entireRect.size() != dirtyRect.size())
        return true;

    return false;
}

Vector<IntRect> TiledBackingStoreRemoteTileTizen::updateBackBuffer()
{
    if (!isDirty())
        return Vector<IntRect>();

    SurfaceUpdateInfo updateInfo;

    static int id = 0;
    bool needToCreateTile = false;
    if (!m_ID) {
        m_ID = ++id;
        needToCreateTile = true;
    }

    if (!m_isLowScaleTile) {
        if (!m_tiledBackingStoreRemoteTileBufferTizen->drawPlatformSurface(m_tiledBackingStore->tileSize(), m_dirtyRect, m_ID)) {
            if(needToCreateTile) {
                m_ID = 0;
                id--;
            }
            return Vector<IntRect>();
        }
    } else {
        if (!m_tiledBackingStoreRemoteTileBufferTizen->drawPlatformLowScaleSurface(m_tiledBackingStore->tileSize(), m_dirtyRect, m_ID)) {
            if(needToCreateTile) {
                m_ID = 0;
                id--;
            }
            return Vector<IntRect>();
        }
    }

    updateInfo.updateRect = m_dirtyRect;
    updateInfo.updateRect.move(-m_rect.x(), -m_rect.y());

    updateInfo.scaleFactor = m_tiledBackingStore->contentsScale();
    updateInfo.platformSurfaceID = m_tiledBackingStoreRemoteTileBufferTizen->platformSurfaceID();
    updateInfo.platformSurfaceSize = m_tiledBackingStoreRemoteTileBufferTizen->platformSurfaceSize();
    updateInfo.partialUpdate = m_tiledBackingStoreRemoteTileBufferTizen->isPartialUpdate();
    updateInfo.isLowScaleTile = m_isLowScaleTile;

    if (needToCreateTile)
        m_client->createTile(m_ID, updateInfo, m_rect);
    else
        m_client->updateTile(m_ID, updateInfo, m_rect);

    m_dirtyRect = IntRect();

    Vector<IntRect> paintedRects;
    paintedRects.append(m_rect);
    return paintedRects;
}

void TiledBackingStoreRemoteTileTizen::resize(const IntSize& newSize)
{
    m_rect = IntRect(m_rect.location(), newSize);
    m_dirtyRect = m_rect;

    m_tiledBackingStoreRemoteTileBufferTizen->resize(m_rect.size());
}

bool TiledBackingStoreRemoteTileBufferTizen::drawPlatformSurface(const IntSize& size, const IntRect& dirtyRect, int tileID)
{
    PlatformSurfacePoolTizen* platformSurfacePool = WebProcess::shared().platformSurfacePool();
    if(!platformSurfacePool)
        return false;

#if ENABLE(TIZEN_RECORDING_SURFACE_PAINT_THREAD)
    platformSurfacePool->m_recordingSurfaceSetIsReplaying = m_tiledBackingStore->client()->recordingSurfaceSetIsReplayingGet();
#endif

    SharedPlatformSurfaceTizen* sharedPlatformSurface = platformSurfacePool->acquirePlatformSurface(size, tileID);
    if (!sharedPlatformSurface)
        return false;

    unsigned char* dstBuffer;
    RefPtr<cairo_surface_t> dstSurface;
    RefPtr<cairo_t> dstBitmapContext;
    OwnPtr<WebCore::GraphicsContext> graphicsContext;
    IntRect paintRect = m_rect;
    m_partialUpdate = false;

    dstBuffer = (unsigned char*)sharedPlatformSurface->lockTbmBuffer();
    if (!dstBuffer) {
        platformSurfacePool->freePlatformSurface(sharedPlatformSurface->id());
        return false;
    }

    dstSurface = adoptRef(cairo_image_surface_create_for_data(dstBuffer, CAIRO_FORMAT_ARGB32,
        sharedPlatformSurface->size().width(), sharedPlatformSurface->size().height(), 4 * sharedPlatformSurface->size().width()));

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    if (m_basePlatformSurfaceID > 0 && needUpdateBackBufferPartially(m_rect, dirtyRect) && !m_tiledBackingStore->client()->recordingSurfaceSetEnableGet()) {
        paintRect = dirtyRect;
        m_partialUpdate = true;
    }
#else
    if (m_basePlatformSurfaceID > 0 && needUpdateBackBufferPartially(m_rect, dirtyRect)) {
        paintRect = dirtyRect;
        m_partialUpdate = true;
    }
#endif

    dstBitmapContext = adoptRef(cairo_create(dstSurface.get()));
    graphicsContext = adoptPtr(new GraphicsContext(dstBitmapContext.get()));
    graphicsContext->clearRect(WebCore::FloatRect(WebCore::FloatPoint(0, 0), sharedPlatformSurface->size()));

#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
    double startTime = 0;
    double endTime = 0;
    if (m_tiledBackingStore->client()->drawTileInfo()) {
        graphicsContext->save();
        startTime = currentTime();
    }
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    if (m_tiledBackingStore->client()->recordingSurfaceSetEnableGet()) {
        m_tiledBackingStore->client()->tiledBackingStorePaint(graphicsContext.get(), m_rect);
    } else
#endif
    {
        graphicsContext->translate(-paintRect.x(), -paintRect.y());
        graphicsContext->scale(FloatSize(m_tiledBackingStore->contentsScale(), m_tiledBackingStore->contentsScale()));
        m_tiledBackingStore->client()->tiledBackingStorePaint(graphicsContext.get(), m_tiledBackingStore->mapToContents(paintRect));
    }

    m_platformSurfaceID = sharedPlatformSurface->id();
    m_platformSurfaceSize = sharedPlatformSurface->size();

    if (!m_partialUpdate)
        m_basePlatformSurfaceID = sharedPlatformSurface->id();

#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
    if (m_tiledBackingStore->client()->drawTileInfo()) {
        endTime = currentTime();
        graphicsContext->restore();

        char tileInfo[256];
        int fontSize = 15;
        snprintf(tileInfo, 256, "[%d] %.1fms (%d,%d / %dX%d)", m_platformSurfaceID, (endTime - startTime) * 1000, m_rect.x(), m_rect.y(), m_platformSurfaceSize.width(), m_platformSurfaceSize.height());
        cairo_set_source_rgb(dstBitmapContext.get(), 0.0, 0.0, 1.0);
        cairo_select_font_face(dstBitmapContext.get(), "Samsung Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
        cairo_set_font_size(dstBitmapContext.get(), fontSize);
        cairo_move_to(dstBitmapContext.get(), 0, fontSize);
        cairo_show_text(dstBitmapContext.get(), tileInfo);
    }
#endif
    dstBitmapContext.release();
    graphicsContext.release();

    if (!sharedPlatformSurface->unlockTbmBuffer())
        return false;

    return true;
}


bool TiledBackingStoreRemoteTileBufferTizen::drawPlatformLowScaleSurface(const IntSize& size, const IntRect& dirtyRect, int tileID)
{
    PlatformSurfacePoolTizen* platformSurfacePool = WebProcess::shared().platformSurfacePool();
    if(!platformSurfacePool)
        return false;

    const float newLowScale = 2.0;
    IntSize newSize = size;
    newSize.setWidth(size.width() / newLowScale);
    newSize.setHeight(size.height() / newLowScale);

    SharedPlatformSurfaceTizen* sharedPlatformSurface = platformSurfacePool->acquirePlatformSurface(newSize, tileID);
    if (!sharedPlatformSurface)
        return false;

    unsigned char* dstBuffer;
    RefPtr<cairo_surface_t> dstSurface;
    RefPtr<cairo_t> dstBitmapContext;
    OwnPtr<WebCore::GraphicsContext> graphicsContext;
    IntRect paintRect = m_rect;

    dstBuffer = (unsigned char*)sharedPlatformSurface->lockTbmBuffer();
    if (!dstBuffer) {
        platformSurfacePool->freePlatformSurface(sharedPlatformSurface->id());
        return false;
    }

    dstSurface = adoptRef(cairo_image_surface_create_for_data(dstBuffer, CAIRO_FORMAT_ARGB32, sharedPlatformSurface->size().width(), sharedPlatformSurface->size().height(), 4 * sharedPlatformSurface->size().width()));

    dstBitmapContext = adoptRef(cairo_create(dstSurface.get()));
    graphicsContext = adoptPtr(new GraphicsContext(dstBitmapContext.get()));
    graphicsContext->clearRect(WebCore::FloatRect(WebCore::FloatPoint(0, 0), sharedPlatformSurface->size()));

    graphicsContext->translate(-paintRect.x() / newLowScale, -paintRect.y() / newLowScale);
    graphicsContext->scale(FloatSize(m_tiledBackingStore->contentsScale() / newLowScale, m_tiledBackingStore->contentsScale() / newLowScale));
    m_tiledBackingStore->client()->tiledBackingStorePaint(graphicsContext.get(), m_tiledBackingStore->mapToContents(paintRect));

    m_basePlatformSurfaceID = 0;
    m_platformSurfaceID = sharedPlatformSurface->id();
    m_platformSurfaceSize = sharedPlatformSurface->size();

    dstBitmapContext.release();
    graphicsContext.release();

    if (!sharedPlatformSurface->unlockTbmBuffer())
        return false;

    return true;
}


} // namespace WebKit

#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
