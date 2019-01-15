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

#ifndef LayerBackingStoreTizen_h
#define LayerBackingStoreTizen_h

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "DrawingAreaProxy.h"
#include "LayerBackingStore.h"
#include "PlatformSurfaceTexturePoolEfl.h"

namespace WebKit {

class LayerBackingStoreTileTizen : public LayerBackingStoreTile {
public:
    LayerBackingStoreTileTizen(float scale = 1, bool isLowScaleTile = false)
        : LayerBackingStoreTile(scale)
        , m_platformSurfaceId(0)
        , m_textureId(0)
        , m_platformSurfaceTexture(0)
        , m_isLowScaleTile(isLowScaleTile)
    {
    }

    void paintPlatformSurfaceTile(WebCore::TextureMapper*, const WebCore::TransformationMatrix&, float);
    void setPlatformSurfaceTexture(const WebCore::IntRect& sourceRect, const WebCore::IntRect& targetRect, PassRefPtr<WebCore::PlatformSurfaceTexture>);
    int platformSurfaceId() { return m_platformSurfaceId; }
    PassRefPtr<WebCore::PlatformSurfaceTexture> platformSurfaceTexture() { return m_platformSurfaceTexture; }

private:
    int m_platformSurfaceId;
    uint32_t m_textureId;
    WebCore::IntSize m_platformSurfaceSize;
    RefPtr<WebCore::PlatformSurfaceTexture> m_platformSurfaceTexture;
    bool m_isLowScaleTile;
};

class LayerBackingStoreTizen : public LayerBackingStore {
public:
    void createTile(int, float);
    void createTile(int, float, bool);
    void removeTile(int);
    static PassRefPtr<LayerBackingStoreTizen> create() { return adoptRef(new LayerBackingStoreTizen); }
    void commitTileOperations(WebCore::TextureMapper*);
    PassRefPtr<WebCore::BitmapTexture> texture() const;
    void paintToTextureMapper(WebCore::TextureMapper*, const WebCore::FloatRect&, const WebCore::TransformationMatrix&, float);

    ~LayerBackingStoreTizen();
    void clearFreePlatformSurfaceTiles() { m_freePlatformSurfaces.clear(); }
    const Vector<int>* freePlatformSurfaceTiles() { return &m_freePlatformSurfaces; }
    int layerId() const { return m_layerId; }
    void updatePlatformSurfaceTile(int id, const WebCore::IntRect& sourceRect, const WebCore::IntRect& target, int layerId, int platformSurfaceId, const WebCore::IntSize& platformSurfaceSize, bool partialUpdate, WebCore::TextureMapper* textureMapper);
    int tilePlatformSurfaceId(int tileId);
    void setPlatformSurfaceTexturePool(DrawingAreaProxy* drawingAreaProxy);
    void freeAllPlatformSurfacesAsFree();
#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)
    virtual bool isHavingTiles() { return m_platformSurfaceTiles.size() > 0 ? true : false;}
#endif

#if ENABLE(TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM)
    void changeScaleForLowScaleLayer();
#endif

private:
    LayerBackingStoreTizen()
        : LayerBackingStore()
        , m_layerId(0)
#if ENABLE(TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM)
        , m_nextScale(0)
#endif
        , m_drawingAreaProxy(0)
    { }

    HashMap<int, LayerBackingStoreTileTizen> m_platformSurfaceTiles;
    Vector<int> m_freePlatformSurfaces;
    uint32_t m_layerId;
    RefPtr<PlatformSurfaceTexturePool> m_platformSurfaceTexturePool;
    DrawingAreaProxy* m_drawingAreaProxy;

#if ENABLE(TIZEN_DEFER_CLEARING_LOW_SCALE_LAYER_AT_ZOOM)
    bool m_nextScale;
#endif
};

} // namespace WebKit

#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#endif
