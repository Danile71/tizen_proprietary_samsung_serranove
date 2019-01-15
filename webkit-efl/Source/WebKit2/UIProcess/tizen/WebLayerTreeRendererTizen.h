/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#ifndef WebLayerTreeRendererTizen_h
#define WebLayerTreeRendererTizen_h

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include <wtf/Functional.h>

#include "PlatformSurfaceTexturePoolEfl.h"
#include "WebLayerTreeRenderer.h"

namespace WebKit {

class WebLayerTreeRendererTizen : public WebLayerTreeRenderer {
public:

    WebLayerTreeRendererTizen(LayerTreeCoordinatorProxy*, DrawingAreaProxy*);
    virtual ~WebLayerTreeRendererTizen() { }

    virtual void purgeGLResources();

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    TIZEN_VIRTUAL void syncCanvas(uint32_t id, const WebCore::IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer, int flags);
#endif

    virtual void detach();
    virtual void appendUpdate(const Function<void()>&);
    virtual void setActive(bool);
    virtual void deleteLayer(WebLayerID);
    virtual void setLayerState(WebLayerID, const WebLayerInfo&);
    virtual void removeTile(WebLayerID, int);
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    virtual void updateTileWithUpdateInfo(WebLayerID, int, const TileUpdate&);
    virtual bool isUsingPlatformSurface() { return true; }
#endif
    virtual void flushLayerChanges(const IntPoint& scrollPosition);
    virtual void clearBackingStores();
    virtual PassRefPtr<LayerBackingStore> getBackingStore(WebLayerID);

    virtual void freePlatformSurface();
    virtual bool hasPlatformSurfaceToFree();

    virtual void updatePlatformSurfaceTile(WebLayerID, int tileID, const WebCore::IntRect& sourceRect, const WebCore::IntRect& targetRect, int platformSurfaceID, const WebCore::IntSize& platformSurfaceSize, bool partialUpdate, bool lowScaleTile);

private:
    void createTextureMapper();

    void clearPlatformLayerPlatformSurfaces();
    void freePlatformSurfacesAfterClearingBackingStores();

    virtual void renderNextFrame();
    virtual void purgeBackingStores();

    struct FreePlatformSurfaceData {
        unsigned int platformSurfaceId;
        WebLayerID layerID;
    };
    Vector<FreePlatformSurfaceData> m_freePlatformSurfaces;

    typedef HashMap<unsigned int, WebLayerID > PlatformLayerPlatformSurfaceMap;
    PlatformLayerPlatformSurfaceMap m_platformLayerPlatformSurfaces;
};

}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

#endif // WebLayerTreeRendererTizen_h
