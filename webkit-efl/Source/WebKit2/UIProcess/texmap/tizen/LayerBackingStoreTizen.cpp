/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
#include "LayerBackingStoreTizen.h"

using namespace WebCore;

namespace WebKit {
void LayerBackingStoreTileTizen::paintPlatformSurfaceTile(WebCore::TextureMapper* textureMapper, const WebCore::TransformationMatrix& transform, float opacity)
{
    FloatRect targetRect(m_targetRect);
    targetRect.scale(1. / m_scale);
    if (targetRect != rect())
        setRect(targetRect);

    IntSize textureSize = m_targetRect.size();
    if (m_isLowScaleTile) {
        textureSize.setWidth(m_targetRect.width() / 2.0);
        textureSize.setHeight(m_targetRect.height() / 2.0);
    }

    textureMapper->drawTexture(m_textureId, 0x01, textureSize, rect(), transform, opacity, m_platformSurfaceSize);

    LOG(AcceleratedCompositing, "[UI ]   coord (%d, %d), size (%d, %d), platformSurface %u @LayerBackingStoreTile::paintPlatformSurfaceTile \n", m_targetRect.x(), m_targetRect.y(), m_platformSurfaceSize.width(), m_platformSurfaceSize.height(), m_platformSurfaceId);
}

void LayerBackingStoreTileTizen::setPlatformSurfaceTexture(const IntRect& sourceRect, const IntRect& targetRect, PassRefPtr<PlatformSurfaceTexture> platformSurfaceTexture)
{
    m_sourceRect = sourceRect;
    m_targetRect = targetRect;
    m_platformSurfaceId = platformSurfaceTexture->platformSurfaceId();
    m_textureId = platformSurfaceTexture->id();
    m_platformSurfaceSize = platformSurfaceTexture->size();
    m_platformSurfaceTexture = platformSurfaceTexture;
}

LayerBackingStoreTizen::~LayerBackingStoreTizen()
{
    freeAllPlatformSurfacesAsFree();
}

void LayerBackingStoreTizen::createTile(int id, float scale)
{
    createTile(id, scale, false);
}

void LayerBackingStoreTizen::createTile(int id, float scale, bool isLowScale)
{
    m_platformSurfaceTiles.add(id, LayerBackingStoreTileTizen(scale, isLowScale));

#if ENABLE(TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM)
    if (isLowScale)
        m_nextScale = scale;
    else
#endif
    m_scale = scale;
}

void LayerBackingStoreTizen::removeTile(int id)
{
    if (!m_platformSurfaceTiles.contains(id))
        return;

    HashMap<int, LayerBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.find(id);
    if (it->second.platformSurfaceId() > 0)
        m_platformSurfaceTexturePool->returnTextureToPool(it->second.platformSurfaceId());
    m_platformSurfaceTiles.remove(id);
}

void LayerBackingStoreTizen::commitTileOperations(TextureMapper* textureMapper)
{
    HashMap<int, LayerBackingStoreTileTizen>::iterator end = m_platformSurfaceTiles.end();
    for (HashMap<int, LayerBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.begin(); it != end; ++it)
        it->second.swapBuffers(textureMapper);
}

PassRefPtr<BitmapTexture> LayerBackingStoreTizen::texture() const
{
    HashMap<int, LayerBackingStoreTileTizen>::const_iterator end = m_platformSurfaceTiles.end();
    for (HashMap<int, LayerBackingStoreTileTizen>::const_iterator it = m_platformSurfaceTiles.begin(); it != end; ++it) {
        RefPtr<BitmapTexture> texture = it->second.texture();
        if (texture)
            return texture;
    }

    return PassRefPtr<BitmapTexture>();
}

static bool shouldShowTileDebugVisualsTizen()
{
#if ENABLE(TIZEN_WEBKIT2_DEBUG_BORDERS)
    return getenv("TIZEN_WEBKIT_SHOW_COMPOSITING_DEBUG_VISUALS") ? (atoi(getenv("TIZEN_WEBKIT_SHOW_COMPOSITING_DEBUG_VISUALS")) == 1) : false;
#endif
    return false;
}

void LayerBackingStoreTizen::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    Vector<LayerBackingStoreTileTizen*> tilesToPaint;

    // We have to do this every time we paint, in case the opacity has changed.
    HashMap<int, LayerBackingStoreTileTizen>::iterator end = m_platformSurfaceTiles.end();

    FloatRect coveredRect;
    for (HashMap<int, LayerBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.begin(); it != end; ++it) {
        if (!it->second.platformSurfaceId())
            continue;

        if (it->second.scale() == m_scale) {
            tilesToPaint.append(&it->second);
            coveredRect.unite(it->second.rect());
            continue;
        }

        // Only show the previous tile if the opacity is high, otherwise effect looks like a bug.
        // We show the previous-scale tile anyway if it doesn't intersect with any current-scale tile.
        if (opacity < 0.95 && coveredRect.intersects(it->second.rect()))
            continue;

        tilesToPaint.prepend(&it->second);
        coveredRect.unite(it->second.rect());
    }

    LOG(AcceleratedCompositing, "[UI ] layer id(%u) paint %d tiles (of %d tiles) @%s\n",
        m_layerId, tilesToPaint.size(), m_tiles.size(), "LayerBackingStoreTizen::paintToTextureMapper");

    static bool shouldDebug = shouldShowTileDebugVisualsTizen();
    for (size_t i = 0; i < tilesToPaint.size(); ++i) {
        tilesToPaint[i]->paintPlatformSurfaceTile(textureMapper, transform, opacity);

        if (!shouldDebug)
            continue;

        textureMapper->drawBorder(Color(0xFF, 0, 0), 2, tilesToPaint[i]->rect(), transform);
        textureMapper->drawNumber(static_cast<LayerBackingStoreTile*>(tilesToPaint[i])->repaintCount(), 8, tilesToPaint[i]->rect().location(), transform);
    }
}

void LayerBackingStoreTizen::updatePlatformSurfaceTile(int id, const IntRect& sourceRect, const WebCore::IntRect& targetRect, int layerId, int platformSurfaceId, const WebCore::IntSize& platformSurfaceSize, bool partialUpdate, TextureMapper* textureMapper)
{
    if (id < 0)
        return;

    if (!m_layerId)
        m_layerId = layerId;

    if (!m_platformSurfaceTiles.contains(id))
        createTile(id, m_scale, false);

    HashMap<int, LayerBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.find(id);
    ASSERT(it != m_platformSurfaceTiles.end());

    if (partialUpdate && it->second.platformSurfaceTexture()) {
        RefPtr<PlatformSurfaceTexture> sourceTexture = m_platformSurfaceTexturePool->acquireTextureFromPool(textureMapper, platformSurfaceId, platformSurfaceSize);
        it->second.platformSurfaceTexture()->copyPlatformSurfaceToTextureGL(sourceRect, sourceTexture->sharedPlatformSurfaceSimple());
        m_freePlatformSurfaces.append(platformSurfaceId);
        m_platformSurfaceTexturePool->returnTextureToPool(platformSurfaceId);
    } else {
        RefPtr<PlatformSurfaceTexture> platformSurfaceTexture = m_platformSurfaceTexturePool->acquireTextureFromPool(textureMapper, platformSurfaceId, platformSurfaceSize);
        if (it->second.platformSurfaceId() > 0) {
            m_freePlatformSurfaces.append(it->second.platformSurfaceId());
            m_platformSurfaceTexturePool->returnTextureToPool(it->second.platformSurfaceId());
        }
        it->second.setPlatformSurfaceTexture(sourceRect, targetRect, platformSurfaceTexture);
        it->second.setTexture(static_cast<BitmapTexture*>(platformSurfaceTexture.get()));
    }
}

int LayerBackingStoreTizen::tilePlatformSurfaceId(int tileId)
{
    if (!m_platformSurfaceTiles.contains(tileId))
        return 0;

    HashMap<int, LayerBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.find(tileId);
    return it->second.platformSurfaceId();
}

void LayerBackingStoreTizen::setPlatformSurfaceTexturePool(DrawingAreaProxy* drawingAreaProxy)
{
    m_drawingAreaProxy = drawingAreaProxy;
    m_platformSurfaceTexturePool = drawingAreaProxy->page()->process()->platformSurfaceTexturePool();
}

void LayerBackingStoreTizen::freeAllPlatformSurfacesAsFree()
{
    if (m_drawingAreaProxy && m_drawingAreaProxy->page())
        m_drawingAreaProxy->page()->makeContextCurrent();

    // When destroying BackingStore, all contained tiles are set to free.
    HashMap<int, LayerBackingStoreTileTizen>::iterator end = m_platformSurfaceTiles.end();
    for (HashMap<int, LayerBackingStoreTileTizen>::iterator it = m_platformSurfaceTiles.begin(); it != end; ++it) {
        if (it->second.platformSurfaceId() > 0) {
            m_platformSurfaceTexturePool->returnTextureToPool(it->second.platformSurfaceId());
            m_platformSurfaceTexturePool->addPlatformSurfacesToFree(it->second.platformSurfaceId());
        }
    }
    for (unsigned i = 0; i < m_freePlatformSurfaces.size(); ++i)
        m_platformSurfaceTexturePool->addPlatformSurfacesToFree(m_freePlatformSurfaces[i]);
    m_platformSurfaceTiles.clear();
}

#if ENABLE(TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM)
void LayerBackingStoreTizen::changeScaleForLowScaleLayer()
{
    if (m_nextScale) {
        m_scale = m_nextScale;
        m_nextScale = 0;
    }
}
#endif

}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

