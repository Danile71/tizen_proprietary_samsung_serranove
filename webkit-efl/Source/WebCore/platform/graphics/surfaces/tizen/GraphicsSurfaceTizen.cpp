/*
    Copyright (C) 2012 Samsung Electronics

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

#include "config.h"
#include "GraphicsSurface.h"

#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "TextureMapperGL.h"
#include "PlatformSurfaceTextureGL.h"

#include "NotImplemented.h"
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

namespace WebCore {

struct GraphicsSurfacePrivate {
    GraphicsSurfacePrivate()
        : m_textureIsYInverted(false)
        , m_hasAlpha(false)
        , m_frontBuffer(0)
    {
    }

    ~GraphicsSurfacePrivate()
    {
    }

    void createImage(const IntSize& size, uint32_t platformSurfaceID)
    {
        m_size = size;
    }

    int getPlatformSurfaceTextureID(bool useLinearFilter) {
        if (!m_frontBuffer)
            return 0;

        PlatformSurfaceTextureMap::iterator it = m_platformSurfaceTextures.find(m_frontBuffer);
        RefPtr<PlatformSurfaceTexture> platformSurfaceTexture;
        if (it == m_platformSurfaceTextures.end()) {
            platformSurfaceTexture = m_textureMapper->createPlatformSurfaceTexture(m_frontBuffer, expandedIntSize(m_size), useLinearFilter);
            m_platformSurfaceTextures.set(m_frontBuffer, platformSurfaceTexture);
        } else
            platformSurfaceTexture = it->second;
        return platformSurfaceTexture->id();
    }

    void setTextureMapper(TextureMapper* textureMapper) { m_textureMapper = textureMapper; }

    void swapBuffers(uint32_t frontBuffer) { m_frontBuffer = frontBuffer; }
    void swapBuffers() { }
    uint32_t frontBuffer() { return m_frontBuffer; }

    IntSize size() const { return m_size; }
    bool hasAlpha() const { return m_hasAlpha; }
private:
    IntSize m_size;
    bool m_textureIsYInverted;
    bool m_hasAlpha;
    uint32_t m_frontBuffer;
    TextureMapper* m_textureMapper;
    typedef HashMap<uint32_t, RefPtr<PlatformSurfaceTexture> > PlatformSurfaceTextureMap;
    PlatformSurfaceTextureMap m_platformSurfaceTextures;
};

uint64_t GraphicsSurface::platformExport()
{
    return m_platformSurface;
}

uint32_t GraphicsSurface::platformGetTextureID()
{
    return m_private->getPlatformSurfaceTextureID(m_canvasFlags & GraphicsSurface::UseLinearFilter);
}

void GraphicsSurface::platformPaintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO) && ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (textureMapper->fullScreenForVideoMode() && (m_canvasFlags & GraphicsSurface::IsVideo))
        return;
#endif
    bool alpha = m_canvasFlags & GraphicsSurface::Alpha;
    bool premultipliedAlpha = m_canvasFlags & GraphicsSurface::PremultipliedAlpha;
    m_private->setTextureMapper(textureMapper);

    if (!platformGetTextureID())
        return;

    // FIXME: Do we really need both SupportsBlending and contentsOpaque??
    if (m_canvasFlags & GraphicsSurface::Is2D)
        static_cast<TextureMapperGL*>(textureMapper)->drawTexture(platformGetTextureID(), alpha ? TextureMapperGL::SupportsBlending : 0, m_size, targetRect, transform, opacity,expandedIntSize(targetRect.size()), true);
    else
        static_cast<TextureMapperGL*>(textureMapper)->drawTexture(platformGetTextureID(), alpha ? TextureMapperGL::SupportsBlending : 0, m_size, targetRect, transform, opacity,expandedIntSize(targetRect.size()), premultipliedAlpha);
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformCreate(const IntSize& size, Flags flags)
{
    return 0;
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformCreate(uint64_t platformSurfaceID)
{
    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(IntSize(), 0));
    surface->m_private = new GraphicsSurfacePrivate();
    surface->m_platformSurface = platformSurfaceID;
    return surface;
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformImport(const IntSize& size, Flags flags, uint64_t token, Flags canvasFlags)
{
    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate();
    surface->m_platformSurface = token;
    surface->m_canvasFlags = canvasFlags;
    surface->m_private->createImage(size, token);
    surface->m_size = surface->m_private->size();
    return surface;
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformImport(const IntSize& size, Flags flags, uint64_t token)
{
    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate();
    surface->m_platformSurface = token;
    surface->m_private->createImage(size, token);
    surface->m_size = surface->m_private->size();

    return surface;
}

void GraphicsSurface::platformDestroy()
{
    delete m_private;
    m_private = 0;
}

void GraphicsSurface::platformCopyToGLTexture(uint32_t target, uint32_t id, const IntRect& targetRect, const IntPoint& offset)
{
}

void GraphicsSurface::platformCopyFromFramebuffer(uint32_t originFbo, const IntRect& sourceRect)
{
}

uint32_t GraphicsSurface::platformFrontBuffer() const
{
    return m_private ? m_private->frontBuffer() : 0;
}

uint32_t GraphicsSurface::platformSwapBuffers()
{
    return 0;
}

#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
uint32_t GraphicsSurface::platformSwapBuffers(uint32_t frontBuffer)
{
    m_private->swapBuffers(frontBuffer);
    return m_private->frontBuffer();
}
#endif

char* GraphicsSurface::platformLock(const IntRect& rect, int* outputStride, LockOptions lockOptions)
{
    // GraphicsSurface is currently only being used for WebGL, which does not require this locking mechanism.
    return 0;
}

void GraphicsSurface::platformUnlock()
{
    // GraphicsSurface is currently only being used for WebGL, which does not require this locking mechanism.
}

PassOwnPtr<GraphicsContext> GraphicsSurface::platformBeginPaint(const IntSize& size, char* bits, int stride)
{
    notImplemented();
    return nullptr;
}

PassRefPtr<Image> GraphicsSurface::createReadOnlyImage(const IntRect& rect)
{
    notImplemented();
    return 0;
}
}
#endif
#endif
