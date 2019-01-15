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

#include "config.h"
#include "VideoLayerTizen.h"
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)

#include "GraphicsContext.h"
#include "IntRect.h"
#include "RenderLayerBacking.h"

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

#ifdef GST_API_VERSION_1
#include <gst/video/videooverlay.h>
#else
#include <gst/interfaces/xoverlay.h>
#endif

#include <gst/video/gstvideosink.h>
#include <gst/video/video.h>
#else
#include <cairo-xlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

#if !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
static Display* g_nativeDisplay = 0;
static int g_nativeWindow = 0;
#endif

namespace WebCore {

#if USE(ACCELERATED_VIDEO_VAAPI)
    void setVaapiEnv()
    {
        static bool envSet = false;
        if (!envSet) {
            envSet = true;
            setenv("PSB_VIDEO_CTEXTURE", "1", true);
        }
    }
#endif

VideoLayerTizen::VideoLayerTizen(HTMLMediaElement* media)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    : m_videoSink(0)
#else
    : m_platformSurfaceID(0)
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    , m_media(media)
{
#if !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (!g_nativeDisplay)
           g_nativeDisplay = XOpenDisplay(0);

       if (!g_nativeDisplay)
           return;

       g_nativeWindow = XCreateSimpleWindow(g_nativeDisplay, XDefaultRootWindow(g_nativeDisplay),
                               0, 0, 1, 1, 0,
                               BlackPixel(g_nativeDisplay, 0), WhitePixel(g_nativeDisplay, 0));
       XFlush(g_nativeDisplay);
#endif
}

VideoLayerTizen::~VideoLayerTizen()
{
    syncLayer(0);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (m_platformSurface)
        m_platformSurface.clear();

    if (m_platformSurfaceToBeRemoved)
        m_platformSurfaceToBeRemoved.clear();

    m_videoSink = 0;
#else
    if (m_platformSurfaceID) {
        XFreePixmap(g_nativeDisplay, m_platformSurfaceID);
        m_platformSurfaceID = 0;
    }
    if (g_nativeWindow) {
        XDestroyWindow(g_nativeDisplay, g_nativeWindow);
        g_nativeWindow = 0;
    }
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
}

PlatformLayer* VideoLayerTizen::platformLayer() const
{
    return const_cast<TextureMapperPlatformLayer*>(static_cast<const TextureMapperPlatformLayer*>(this));
}

void VideoLayerTizen::syncLayer(VideoLayerTizen* layer)
{
    RenderBox* renderBox = m_media->renderBox();
    if (renderBox && renderBox->hasAcceleratedCompositing()) {
        RenderLayer* renderLayer = renderBox->layer();
        if (renderLayer && renderLayer->isComposited()) {
            GraphicsLayer* graphicsLayer = renderLayer->backing()->graphicsLayer();
            if (graphicsLayer)
                graphicsLayer->setContentsToMedia(layer);
        }
    }
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
bool VideoLayerTizen::swapPlatformSurfaces()
{
    return true;
}

void VideoLayerTizen::freePlatformSurface(int id)
{
    if (m_platformSurfaceToBeRemoved)
        m_platformSurfaceToBeRemoved.clear();
}

void VideoLayerTizen::platformSurfaceUpdated()
{
    notifySyncRequired();
}

#if !USE(ACCELERATED_VIDEO_VAAPI) && GST_API_VERSION_1
static int getPixmapIDCallback(void* userData, guint videoWidth, guint videoHight) {
  VideoLayerTizen* layer = static_cast<VideoLayerTizen*>(userData);
  return layer->graphicsSurfaceToken();
}
#endif

GstElement* VideoLayerTizen::createVideoSink()
{
#if !USE(ACCELERATED_VIDEO_VAAPI)
    GstElement* videoSink = gst_element_factory_make("xvimagesink", "xvimagesink");
    g_object_set(videoSink, "rotate", 0, NULL);
#ifdef GST_API_VERSION_1
    // Set gstvideooverlay window ID property right after creation of sink to
    // avoid default window creation.
    g_object_set(GST_VIDEO_OVERLAY(videoSink), "pixmap-id-callback", &getPixmapIDCallback, NULL);
    g_object_set(GST_VIDEO_OVERLAY(videoSink), "pixmap-id-callback-userdata", this, NULL);
#endif // GST_API_VERSION_1
#else
    setVaapiEnv();
    GstElement* videoSink = gst_element_factory_make("vaapisink", "vaapisink");
    g_object_set(videoSink, "is-pixmap", 1, NULL);
#endif

    m_videoSink = videoSink;

    return videoSink;
}

void VideoLayerTizen::notifySyncRequired()
{
    if (!m_platformSurface)
        return;

    syncLayer(this);
}

void VideoLayerTizen::setOverlay(IntSize size)
{
    if (size.isEmpty())
        return;

    if (!m_videoSink)
        return;

    if (m_platformSurface)
        m_platformSurfaceToBeRemoved = m_platformSurface.release();

    // Align to 16. (If not aligned to 16, pixmap will be created again.)
    int remainder = size.width() % 16;
    if (remainder) {
        size.setHeight(size.height() + (16 - remainder) * (static_cast<float>(size.height()) / size.width()));
        size.setWidth(size.width() + (16 - remainder));
    }

    // only Even is allowed to create pixmap. really need to set here?
    size.setHeight(size.height() - size.height() % 2);
    size.setWidth(size.width() - size.width() % 2);

    m_platformSurface = SharedVideoPlatformSurfaceTizen::create(size);
    if (!m_platformSurface || !m_platformSurface->id()) {
        TIZEN_LOGE("ERROR:: Unable to create pixmap");
        return;
    }
    m_platformSurface->setVideoPlatformSurfaceUpdateListener(this);

    if (m_platformSurfaceToBeRemoved)
        m_platformSurface->copySurface(m_platformSurfaceToBeRemoved.get(), SharedVideoPlatformSurfaceTizen::FitToWidth);

#ifdef GST_API_VERSION_1
    g_object_set(GST_VIDEO_OVERLAY(m_videoSink), "pixmap-id-callback", &getPixmapIDCallback, NULL);
    g_object_set(GST_VIDEO_OVERLAY(m_videoSink), "pixmap-id-callback-userdata", this, NULL);
#else
    gst_x_overlay_set_window_handle(GST_X_OVERLAY(m_videoSink), m_platformSurface->id());
#endif
}

void VideoLayerTizen::setOverlay()
{
    if (!m_videoSink || !m_platformSurface || !m_platformSurface->id())
        return;
#ifdef GST_API_VERSION_1
    g_object_set(GST_VIDEO_OVERLAY(m_videoSink), "pixmap-id-callback", &getPixmapIDCallback, NULL);
    g_object_set(GST_VIDEO_OVERLAY(m_videoSink), "pixmap-id-callback-userdata", this, NULL);
#else
    gst_x_overlay_set_window_handle(GST_X_OVERLAY(m_videoSink), m_platformSurface->id());
#endif
}

void VideoLayerTizen::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    if (m_platformSurface)
        m_platformSurface->paintCurrentFrameInContext(context, rect);
}

void VideoLayerTizen::clearSurface()
{
    if (m_platformSurface)
        m_platformSurface->clearSurface();
}
#else

void VideoLayerTizen::paintVideoLayer(IntSize videoSize)
{
    bool isClearNeeded = false;
    if (videoSize.isEmpty())
        return;

    if (m_videoSize != videoSize) {
        m_videoSize = videoSize;
        isClearNeeded = true;

        if (m_platformSurfaceID) {
            XFreePixmap(g_nativeDisplay, m_platformSurfaceID);
            m_platformSurfaceID = 0;
        }
        m_platformSurfaceID = XCreatePixmap(g_nativeDisplay, g_nativeWindow, m_videoSize.width(), m_videoSize.height(), DefaultDepth(g_nativeDisplay, DefaultScreen(g_nativeDisplay)));
        XFlush(g_nativeDisplay);
    }

    RefPtr<cairo_surface_t> surface = adoptRef(cairo_xlib_surface_create(g_nativeDisplay, m_platformSurfaceID, DefaultVisual(g_nativeDisplay, DefaultScreen(g_nativeDisplay)), m_videoSize.width(), m_videoSize.height()));
    if (cairo_surface_status(surface.get()) != CAIRO_STATUS_SUCCESS)
       return;

    RefPtr<cairo_t> cr = adoptRef(cairo_create(surface.get()));

    if(isClearNeeded) {
        cairo_set_source_rgb(cr.get(), 0.1, 0.1, 0.1);
        cairo_rectangle(cr.get(), 0, 0, m_videoSize.width(), m_videoSize.height());
        cairo_fill(cr.get());
    }

    OwnPtr<WebCore::GraphicsContext> context = adoptPtr(new GraphicsContext(cr.get()));
    m_media->player()->paint(context.get(), IntRect(0, 0, m_videoSize.width(), m_videoSize.height()));
    XFlush(g_nativeDisplay);

    syncLayer(this);
}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
uint64_t VideoLayerTizen::graphicsSurfaceToken() const
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    return (m_platformSurface ? m_platformSurface->id() : 0);
#else
    return m_platformSurfaceID;
#endif
}

uint32_t VideoLayerTizen::copyToGraphicsSurface()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    return (m_platformSurface ? m_platformSurface->id() : 0);
#else
    return m_platformSurfaceID;
#endif
}

int VideoLayerTizen::graphicsSurfaceFlags() const
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    return m_platformSurface ? m_platformSurface->graphicsSurfaceFlags() : GraphicsSurface::Is2D;
#else
    return GraphicsSurface::Is2D;
#endif
}
#endif // USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)

} // WebCore

#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)
