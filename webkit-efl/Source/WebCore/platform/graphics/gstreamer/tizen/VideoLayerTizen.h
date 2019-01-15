/*
    Copyright (C) 2012 Samsung Electronics.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef VideoLayerTizen_h
#define VideoLayerTizen_h

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)

#include "GraphicsLayer.h"
#include "HTMLMediaElement.h"
#include "TextureMapperGL.h"

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "SharedVideoPlatformSurfaceTizen.h"
#include <gst/gst.h>
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
#include "GraphicsSurface.h"
#endif

namespace WebCore {

class GraphicsContext;
class IntRect;

class VideoLayerTizen : public TextureMapperPlatformLayer
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
                      , public VideoPlatformSurfaceUpdateListener
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
{
public:
    static PassOwnPtr<VideoLayerTizen> create(HTMLMediaElement* media)
    {
        return adoptPtr(new VideoLayerTizen(media));
    }
    ~VideoLayerTizen();

    PlatformLayer* platformLayer() const;

    // TextureMapperPlatformLayer interface
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect& targetRect, const TransformationMatrix&, float opacity) { }
    virtual Image* paintToImage() { return 0; }

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    // TextureMapperPlatformLayer interface
    virtual bool swapPlatformSurfaces();
    virtual void freePlatformSurface(int);

    // VideoplatformSurfaceUpdateListener interface
    virtual void platformSurfaceUpdated();

    GstElement* createVideoSink();
    void notifySyncRequired();
    void setOverlay(IntSize);
    void setOverlay();

    void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);
    void clearSurface();
#else
    void paintVideoLayer(IntSize);
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    virtual uint32_t copyToGraphicsSurface();
    virtual uint64_t graphicsSurfaceToken() const;
    virtual int graphicsSurfaceFlags() const;
#endif

private:
    VideoLayerTizen(HTMLMediaElement* media);
    void syncLayer(VideoLayerTizen* layer);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    GstElement* m_videoSink;
    OwnPtr<SharedVideoPlatformSurfaceTizen> m_platformSurface;
    OwnPtr<SharedVideoPlatformSurfaceTizen> m_platformSurfaceToBeRemoved;
#else
    IntSize m_videoSize;
    int m_platformSurfaceID;
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

    HTMLMediaElement* m_media;
};

} // WebCore

#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)

#endif // VideoLayerTizen_h
