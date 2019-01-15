/*
    Copyright (C) 2011 Samsung Electronics

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
#include "GraphicsContext3DOnscreen.h"

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)
#if USE(ACCELERATED_COMPOSITING)
#include "CanvasRenderingContext.h"
#include "HostWindow.h"
#include "Logging.h"
#include "NotImplemented.h"
#include "ewk_view_private.h"

#include <wtf/Assertions.h>

namespace WebCore {

PassOwnPtr<GraphicsContext3DOnscreen> GraphicsContext3DOnscreen::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow)
{
    OwnPtr<GraphicsContext3DOnscreen> internal = adoptPtr(new GraphicsContext3DOnscreen(attrs));
    if (!internal->initialize(hostWindow))
        return nullptr;

    return internal.release();
}

GraphicsContext3DOnscreen::GraphicsContext3DOnscreen(GraphicsContext3D::Attributes attrs)
    : GraphicsContext3DInternal(attrs, GraphicsContext3D::RenderDirectlyToHostWindow)
    , m_view(0)
{
    LOG(AcceleratedCompositing, "[%p] GraphicsContext3DOnscreen::%s() - CONSTRUCTOR\n", this, __func__);
}

GraphicsContext3DOnscreen::~GraphicsContext3DOnscreen()
{
    LOG(AcceleratedCompositing, "[%p] GraphicsContext3DOnscreen::%s() - DESTRUCTOR\n", this, __func__);
}

bool GraphicsContext3DOnscreen::initialize(HostWindow* hostWindow)
{
    return GraphicsContext3DInternal::initialize(hostWindow);
}

bool GraphicsContext3DOnscreen::createContext(Evas_Object* view)
{
    m_context = evas_gl_context_create(m_evasGL, 0);
    if (!m_context)
        return false;
    return true;
}

bool GraphicsContext3DOnscreen::createSurface(Evas_Object* view)
{
    m_view = view;
    return setSurface();
}

void GraphicsContext3DOnscreen::markContextChanged()
{
    ASSERT_NOT_REACHED();
}

void GraphicsContext3DOnscreen::markLayerComposited()
{
    ASSERT_NOT_REACHED();
}

bool GraphicsContext3DOnscreen::layerComposited() const
{
    ASSERT_NOT_REACHED();
    return false;
}

platformSurface3DObject GraphicsContext3DOnscreen::platformSurfaceTexture() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

platformSurfaceLayer* GraphicsContext3DOnscreen::platformSurfaceLayer() const
{
    ASSERT_NOT_REACHED();
    return 0;
}

void GraphicsContext3DOnscreen::paintRenderingResultsToCanvas(CanvasRenderingContext*)
{
    ASSERT_NOT_REACHED();
}

bool GraphicsContext3DOnscreen::paintsIntoCanvasBuffer() const
{
    ASSERT_NOT_REACHED();
    return false;
}

void GraphicsContext3DOnscreen::prepareTexture(int width, int height)
{
    ASSERT_NOT_REACHED();
}

void GraphicsContext3DOnscreen::reshape(int width, int height)
{
    LOG(AcceleratedCompositing, "[%p] GraphicsContext3DOnscreen::%s() - w: %d h: %d\n", this, __func__, width, height);
    if (width < 1 || height < 1) {
        LOG(AcceleratedCompositing, "---> the surface size is too small!\n");
        return;
    }

    evas_gl_surface_destroy(m_evasGL, m_surface);

    setSurface();
}

#if ENABLE(TIZEN_EVAS_GL_DIRECT_RENDERING)
void GraphicsContext3DOnscreen::clear(GC3Dbitfield mask)
{
    makeContextCurrent();
    GL_CMD(glClear(GraphicsContext3D::DEPTH_BUFFER_BIT));
}
#endif

bool GraphicsContext3DOnscreen::setSurface()
{
    LOG(AcceleratedCompositing, "[%p] GraphicsContext3DOnscreen::%s()\n", this, __func__);

    int x = 0, y = 0, w = 0, h = 0;
    evas_object_geometry_get(m_view, &x, &y, &w, &h);
    LOG(AcceleratedCompositing, "---> ewk_view's geometry - x: %d y: %d w: %d h: %d\n", x, y, w, h);

#if ENABLE(TIZEN_EVAS_GL_DIRECT_RENDERING)
    m_config = evas_gl_config_new();
    m_config->options_bits = EVAS_GL_OPTIONS_DIRECT;
    m_config->color_format = EVAS_GL_RGBA_8888;
    m_config->depth_bits = EVAS_GL_DEPTH_BIT_8;
    #if USE(TIZEN_TEXTURE_MAPPER)
    m_config->stencil_bits = EVAS_GL_STENCIL_BIT_8;
    #else
    m_config->stencil_bits = EVAS_GL_STENCIL_NONE;
    #endif

    m_surface = evas_gl_surface_create(m_evasGL, m_config, w, h);
#else
    Evas_GL_Config config = {
        EVAS_GL_RGBA_8,
        EVAS_GL_DEPTH_BIT_8,
    #if USE(TIZEN_TEXTURE_MAPPER)
        EVAS_GL_STENCIL_BIT_8
    #else
        EVAS_GL_STENCIL_NONE
    #endif
    };

    m_surface = evas_gl_surface_create(m_evasGL, &config, w, h);
#endif

    if (!m_surface) {
        LOG(AcceleratedCompositing, "---> failed to create Evas_GL_Surface\n");
        return false;
    }

    Evas_Native_Surface nativeSurface;
    evas_gl_native_surface_get(m_evasGL, m_surface, &nativeSurface);

    ewk_view_evas_gl_object_create(m_view, &nativeSurface, x, y, w, h);
    return true;
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
#endif
