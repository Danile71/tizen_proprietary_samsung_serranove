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

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)
#if USE(ACCELERATED_COMPOSITING) && ENABLE(WEBGL)

#include "CanvasRenderingContext.h" // Moved here due to build break in WebKit2

#include "GraphicsContext3DOffscreen.h"

#include "BitmapImage.h"
#include "HostWindow.h"
#include "HTMLCanvasElement.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "NotImplemented.h"
#if !ENABLE(TIZEN_WEBKIT2)
#include "PageClientEfl.h"
#endif
#include <wtf/text/CString.h>

#if USE(TIZEN_TEXTURE_MAPPER) && !ENABLE(TIZEN_WEBKIT2)
#include "TextureMapperEvasGL.h"
#endif

#if !USE(TIZEN_TEXTURE_MAPPER)
#include "EflLayer.h"
#endif

#if !ENABLE(TIZEN_WEBKIT2)
#include "ewk_view_private.h"
#else // TIZEN_WEBKIT2
#if !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#else
#include "Extensions3D.h" // Temporary include for DDK issue workaround.
#endif
#endif

#if ENABLE(TIZEN_BROWSER_WEBGL_DASH_MODE)
#include <TizenSystemUtilities.h>
#endif

#ifndef GLchar
#define GLchar char
#endif

#if ENABLE(TIZEN_LITE_TEXTURE_RESIZE_OPTIMIZATION) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#endif

#if !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
static Display* g_nativeDisplay = 0;
static int g_nativeWindow = 0;
#endif
namespace WebCore {

#if ENABLE(TIZEN_LITE_TEXTURE_RESIZE_OPTIMIZATION) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
static const float s_resourceAdjustedRatio = 0.96;
const int s_maxScaleAttempts = 3;
extern Display* g_nativeDisplay;
#endif

PassOwnPtr<GraphicsContext3DOffscreen> GraphicsContext3DOffscreen::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("");
#endif

    OwnPtr<GraphicsContext3DOffscreen> internal = adoptPtr(new GraphicsContext3DOffscreen(attrs));
    if (!internal->initialize(hostWindow))
        return nullptr;

    return internal.release();
}

GraphicsContext3DOffscreen::GraphicsContext3DOffscreen(GraphicsContext3D::Attributes attrs)
    : GraphicsContext3DInternal(attrs, GraphicsContext3D::RenderOffscreen)
    , m_acceleratedCompositingEnabled(0)
    , m_layerComposited(0)
    , m_boundTexture0(0)
    , m_boundFBO(0)
    , m_activeTexture(GraphicsContext3D::TEXTURE0)
    , m_compositorTexture(0)
    , m_depthBuffer(0)
    , m_stencilBuffer(0)
    , m_fbo(0)
    , m_internalColorFormat(0)
    , m_texture(0)
#if ENABLE(TIZEN_WEBKIT2)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    , m_currentPlatformSurface(0)
    , m_previousPlatformSurface(0)
    , m_maxPlatformSurface(3)
#else
    , m_display(EGL_NO_DISPLAY)
    , m_context(EGL_NO_CONTEXT)
    , m_surface(EGL_NO_SURFACE)
    , m_platformSurfaceID(0)
#endif
#endif
#if ENABLE(TIZEN_BROWSER_WEBGL_DASH_MODE)
    , m_webglDashModeTimer(this, &GraphicsContext3DOffscreen::webglDashModeFired)
#endif
{
}

GraphicsContext3DOffscreen::~GraphicsContext3DOffscreen()
{
    makeContextCurrent();

    if (m_texture)
        GL_CMD(glDeleteTextures(1, &m_texture));
    if (m_compositorTexture)
        GL_CMD(glDeleteTextures(1, &m_compositorTexture));
    if (m_stencilBuffer)
        GL_CMD(glDeleteRenderbuffers(1, &m_stencilBuffer));
    if (m_depthBuffer)
        GL_CMD(glDeleteRenderbuffers(1, &m_depthBuffer));
    if (m_fbo)
        GL_CMD(glDeleteFramebuffers(1, &m_fbo));
#if ENABLE(TIZEN_WEBKIT2)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    PlatformSurfaceMap::iterator end = m_platformSurfaces.end();
    for (PlatformSurfaceMap::iterator iter = m_platformSurfaces.begin(); iter != end; ++iter) {
        RefPtr<PlatformSurfaceInfo> surfaceInfo = iter->second;
        surfaceInfo.release();
    }
    m_platformSurfaces.clear();
#else
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_surface != EGL_NO_SURFACE) {
        eglDestroySurface(m_display, m_surface);
        m_surface = EGL_NO_SURFACE;
    }
    if (m_context != EGL_NO_CONTEXT) {
        eglDestroyContext(m_display, m_context);
        m_context = EGL_NO_CONTEXT;
    }
    if (m_platformSurfaceID) {
        XFreePixmap(g_nativeDisplay, m_platformSurfaceID);
        m_platformSurfaceID = 0;
    }
    m_display = 0;
#endif
#endif
#if USE(ACCELERATED_COMPOSITING) && !USE(TIZEN_TEXTURE_MAPPER)
    m_compositingLayer.clear();
#endif
#if ENABLE(TIZEN_BROWSER_WEBGL_DASH_MODE)
    if (m_webglDashModeTimer.isActive())
        m_webglDashModeTimer.stop();
#endif
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* GraphicsContext3DOffscreen::platformLayer() const
{
#if USE(TIZEN_TEXTURE_MAPPER)
    return const_cast<TextureMapperPlatformLayer*>(static_cast<const TextureMapperPlatformLayer*>(this));
#else
    return m_compositingLayer.get();
#endif
}
#endif

bool GraphicsContext3DOffscreen::initialize(HostWindow* hostWindow)
{
    if (!GraphicsContext3DInternal::initialize(hostWindow))
        return false;
#if ENABLE(TIZEN_WEBKIT2)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    m_platformContext = adoptRef(new PlatformContextInfo(m_attributes.alpha, m_attributes.depth, m_attributes.stencil));
    RefPtr<PlatformSurfaceInfo> newSurface = adoptRef(new PlatformSurfaceInfo(IntSize(1, 1), m_attributes.alpha, m_attributes.depth, m_attributes.stencil, m_platformContext->m_pixmapContext.get()));
    if (!newSurface->m_platformSurface)
        return false;

    m_platformSurfaces.add(newSurface->m_platformSurface->id(), newSurface);
    m_currentPlatformSurface = newSurface->m_platformSurface.get();
#else
    if (!g_nativeDisplay)
        g_nativeDisplay = XOpenDisplay(0);

    if (!g_nativeDisplay)
        return false;

    g_nativeWindow = XCreateSimpleWindow(g_nativeDisplay, XDefaultRootWindow(g_nativeDisplay),
                            0, 0, 1, 1, 0,
                            BlackPixel(g_nativeDisplay, 0), WhitePixel(g_nativeDisplay, 0));
    XFlush(g_nativeDisplay);

    EGLint major, minor;
    EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

    m_display = eglGetDisplay(g_nativeDisplay);
    if (m_display == EGL_NO_DISPLAY)
        return false;

    if (eglInitialize(m_display, &major, &minor) != EGL_TRUE)
        return false;

    EGLint configCount = 0;
    int i = 0;
    EGLint configAttribs[32];
    configAttribs[i++] = EGL_LEVEL;
    configAttribs[i++] = 0;
    configAttribs[i++] = EGL_RED_SIZE;
    configAttribs[i++] = 8;
    configAttribs[i++] = EGL_GREEN_SIZE;
    configAttribs[i++] = 8;
    configAttribs[i++] = EGL_BLUE_SIZE;
    configAttribs[i++] = 8;
    configAttribs[i++] = EGL_ALPHA_SIZE;
    configAttribs[i++] = 8;
    configAttribs[i++] = EGL_RENDERABLE_TYPE;
    configAttribs[i++] = EGL_OPENGL_ES2_BIT;
    configAttribs[i++] = EGL_NONE;

    if (eglChooseConfig(m_display, configAttribs, &m_surfaceConfig, 1, &configCount) != EGL_TRUE)
        return false;

    m_context = eglCreateContext(m_display, m_surfaceConfig, EGL_NO_CONTEXT, contextAttribs);
    if (m_context == EGL_NO_CONTEXT)
        return false;
    m_platformSurfaceID = XCreatePixmap(g_nativeDisplay, g_nativeWindow, 1, 1, DefaultDepth(g_nativeDisplay, DefaultScreen(g_nativeDisplay)));
    XFlush(g_nativeDisplay);
    m_surface = eglCreatePixmapSurface(m_display, m_surfaceConfig, m_platformSurfaceID, 0);
    if (m_surface == EGL_NO_SURFACE)
        return false;
#endif
    m_acceleratedCompositingEnabled = true;
#else
    PageClientEfl* pageClient = static_cast<PageClientEfl*>(hostWindow->platformPageClient());
    m_acceleratedCompositingEnabled = ewk_view_setting_accelerated_compositing_enable_get(pageClient->view());
#endif

    makeContextCurrent();
    validateAttributes();
#if !ENABLE(TIZEN_WEBKIT2)
    GL_CMD(glGenTextures(1, &m_texture));
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_texture));
    GL_CMD(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CMD(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    GL_CMD(glGenTextures(1, &m_compositorTexture));
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_compositorTexture));
    GL_CMD(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CMD(glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));

    GL_CMD(glBindTexture(GL_TEXTURE_2D, 0));

    // Create buffers for the canvas FBO.
    GL_CMD(glGenFramebuffers(1, &m_fbo));

    GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    m_boundFBO = m_fbo;

    if (!m_attributes.antialias) {
        if (m_attributes.stencil)
            GL_CMD(glGenRenderbuffers(1, &m_stencilBuffer));
        if (m_attributes.depth)
            GL_CMD(glGenRenderbuffers(1, &m_depthBuffer));
    }
#endif

// ANGLE integration starts
#if ENABLE(TIZEN_WEBGL_ANGLE_INTEGRATION)
    ShBuiltInResources ANGLEResources;
    ShInitBuiltInResources(&ANGLEResources);

    GL_CMD(getIntegerv(GraphicsContext3D::MAX_VERTEX_ATTRIBS, &ANGLEResources.MaxVertexAttribs));
    GL_CMD(getIntegerv(GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS, &ANGLEResources.MaxVertexUniformVectors));
    GL_CMD(getIntegerv(GraphicsContext3D::MAX_VARYING_VECTORS, &ANGLEResources.MaxVaryingVectors));
    GL_CMD(getIntegerv(GraphicsContext3D::MAX_VERTEX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxVertexTextureImageUnits));
    GL_CMD(getIntegerv(GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxCombinedTextureImageUnits));
    GL_CMD(getIntegerv(GraphicsContext3D::MAX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxTextureImageUnits));
    GL_CMD(getIntegerv(GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS, &ANGLEResources.MaxFragmentUniformVectors));

    // Always set to 1 for OpenGL ES.
    ANGLEResources.MaxDrawBuffers = 1;
    m_compiler.setResources(ANGLEResources);
#endif
// ANGLE Integration block ENDs


    GL_CMD(glClearColor(0.0, 0.0, 0.0, 0.0));

#if USE(ACCELERATED_COMPOSITING)
#if !USE(TIZEN_TEXTURE_MAPPER)
    m_compositingLayer = EflLayer::create(0);
#endif
#endif

    return true;
}

#if !ENABLE(TIZEN_WEBKIT2)
bool GraphicsContext3DOffscreen::createContext(Evas_Object* view)
{
    Evas_GL_Context* sharedContext = 0;
#if USE(ACCELERATED_COMPOSITING)
    // to share resources with accelerated compositing context
    GraphicsContext3D* compositingContext = ewk_view_accelerated_compositing_context_get(view);

    if (compositingContext)
        sharedContext = static_cast<Evas_GL_Context*>(compositingContext->platformGraphicsContext3D());
#endif
    m_context = evas_gl_context_create(m_evasGL, sharedContext);
    if (!m_context)
        return false;
    return true;
}

bool GraphicsContext3DOffscreen::createSurface(Evas_Object* view)
{
#if ENABLE(TIZEN_EVAS_GL_DIRECT_RENDERING)
    m_config = evas_gl_config_new();
    m_config->options_bits = EVAS_GL_OPTIONS_NONE;
    m_config->color_format = EVAS_GL_RGBA_8888;
    m_config->depth_bits = EVAS_GL_DEPTH_BIT_8;
    m_config->stencil_bits = EVAS_GL_STENCIL_NONE;

    m_surface = evas_gl_surface_create(m_evasGL, m_config, 1, 1);
#else
    Evas_GL_Config config = {
        EVAS_GL_RGBA_8,
        EVAS_GL_DEPTH_BIT_8,
        EVAS_GL_STENCIL_NONE
    };

    m_surface = evas_gl_surface_create(m_evasGL, &config, 1, 1);
#endif
    if (!m_surface)
        return false;
    return true;
}
#endif

#if ENABLE(TIZEN_WEBKIT2)
bool GraphicsContext3DOffscreen::makeContextCurrent()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (!m_currentPlatformSurface)
        m_currentPlatformSurface = getFreePlatformSurface();

    return m_currentPlatformSurface ? m_currentPlatformSurface->makeContextCurrent() : false;
#else
    if (eglGetCurrentSurface(EGL_READ) == m_surface && eglGetCurrentContext() == m_context)
        return true;

    return eglMakeCurrent(m_display, m_surface, m_surface, m_context);
#endif
}
#endif

bool GraphicsContext3DOffscreen::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight, int canvasWidth, int canvasHeight, PlatformContextCairo* context)
{
    if (!imagePixels || imageWidth <= 0 || imageHeight <= 0 || canvasWidth <= 0 || canvasHeight <= 0 || !context)
        return false;

    cairo_t* cr = context->cr();
    context->save();

    cairo_rectangle(cr, 0, 0, canvasWidth, canvasHeight);
    cairo_set_operator(cr, CAIRO_OPERATOR_CLEAR);
    cairo_paint(cr);

    RefPtr<cairo_surface_t> imageSurface = adoptRef(cairo_image_surface_create_for_data(
        const_cast<unsigned char*>(imagePixels), CAIRO_FORMAT_ARGB32, imageWidth, imageHeight, imageWidth * 4));

    // OpenGL keeps the pixels stored bottom up, so we need to flip the image here.
    cairo_translate(cr, 0, imageHeight);
    cairo_scale(cr, 1, -1);

    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_surface(cr, imageSurface.get(), 0, 0);
    cairo_rectangle(cr, 0, 0, canvasWidth, -canvasHeight);

    cairo_fill(cr);
    context->restore();

    return true;
}

#if ENABLE(TIZEN_WEBKIT2)
PlatformGraphicsContext3D GraphicsContext3DOffscreen::platformGraphicsContext3D() const
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    return m_currentPlatformSurface ? m_currentPlatformSurface->context() : 0;
#else
    return m_context;
#endif
}
#endif

#if ENABLE(TIZEN_WEBGL_ANIMATION_SNAPSHOT_FIX)
void GraphicsContext3DOffscreen::paintRenderingResultsToCanvas(ImageBuffer* imageBuffer, int width, int height, bool isForcePaint)
#else
void GraphicsContext3DOffscreen::paintRenderingResultsToCanvas(ImageBuffer* imageBuffer, int width, int height)
#endif
{
#if ENABLE(TIZEN_IMAGEBUFFER_FAILURE_CHECK)
    if (!imageBuffer)
        return;
#endif
    cairo_surface_t* surface = imageBuffer->getSurface();
    int totalBytes = 4 * width * height;

#if ENABLE(TIZEN_WEBKIT2)
    OwnArrayPtr<unsigned char> pixels = adoptArrayPtr(new unsigned char[totalBytes]);
    if (!pixels)
        return;

#if ENABLE(TIZEN_WEBGL_ANIMATION_SNAPSHOT_FIX)
    if (isForcePaint && layerComposited() && m_previousPlatformSurface) {
        SharedPlatformSurfaceTizen* toRestorePlatformSurface = m_currentPlatformSurface;
        m_currentPlatformSurface = m_previousPlatformSurface;
        readRenderingResults(pixels.get(), totalBytes , width , height);
        m_currentPlatformSurface = toRestorePlatformSurface;
      }
    else
        readRenderingResults(pixels.get(), totalBytes , width , height);
#else
    readRenderingResults(pixels.get(), totalBytes , width , height);
#endif

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_GL) {
        // TODO: premultipliedAlpha currently not getting applied. Need to apply the value properly.
        if (!m_attributes.premultipliedAlpha) {
            for (int i = 0; i < totalBytes; i += 4) {
                // Premultiply alpha.
                pixels[i + 0] = std::min(255, pixels[i + 0] * pixels[i + 3] / 255);
                pixels[i + 1] = std::min(255, pixels[i + 1] * pixels[i + 3] / 255);
                pixels[i + 2] = std::min(255, pixels[i + 2] * pixels[i + 3] / 255);
            }
        }
    }
#endif
    paintToCanvas(pixels.get(), width, height,
        imageBuffer->internalSize().width(), imageBuffer->internalSize().height(), imageBuffer->context()->platformContext());
#else
    unsigned char* pixels = cairo_image_surface_get_data(surface);
    readRenderingResults(pixels, totalBytes, width, height);
    cairo_surface_mark_dirty_rectangle(surface, 0, 0, width, height);
#endif
}

PassRefPtr<ImageData> GraphicsContext3DOffscreen::paintRenderingResultsToImageData(int width, int height)
{
    // Reading premultiplied alpha would involve unpremultiplying, which is
    // lossy
    if (m_attributes.premultipliedAlpha)
        return 0;

    RefPtr<ImageData> imageData = ImageData::create(IntSize(width, height));
    unsigned char* pixels = imageData->data()->data();
    int totalBytes = 4 * width * height;

    readRenderingResults(pixels, totalBytes, width, height);

    // Convert to RGBA
    for (int i = 0; i < totalBytes; i += 4)
        std::swap(pixels[i], pixels[i + 2]);

    return imageData.release();
}

#if USE(TIZEN_TEXTURE_MAPPER)
#if ENABLE(TIZEN_WEBKIT2)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
SharedPlatformSurfaceTizen* GraphicsContext3DOffscreen::getFreePlatformSurface()
{
    SharedPlatformSurfaceTizen* newPlatformSurface = 0;
    PlatformSurfaceMap::iterator end = m_platformSurfaces.end();
    for (PlatformSurfaceMap::iterator iter = m_platformSurfaces.begin(); iter != end; ++iter) {
        RefPtr<PlatformSurfaceInfo> surfaceInfo = iter->second;
        if (!surfaceInfo->m_used) {
            newPlatformSurface = surfaceInfo->m_platformSurface.get();
            break;
        }
    }
    if (!newPlatformSurface && m_platformContext) {
        if (m_platformSurfaces.size() < m_maxPlatformSurface) {
            RefPtr<PlatformSurfaceInfo> newSurface = adoptRef(new PlatformSurfaceInfo(IntSize(m_width, m_height), m_attributes.alpha, m_attributes.depth, m_attributes.stencil, m_platformContext->m_pixmapContext.get()));
            if (!newSurface->m_platformSurface)
                return 0;

            m_platformSurfaces.add(newSurface->m_platformSurface->id(), newSurface);
            newPlatformSurface = newSurface->m_platformSurface.get();
        }
    }

#if ENABLE(TIZEN_DLOG_SUPPORT)
    if (!newPlatformSurface)
        TIZEN_LOGI("no free surfaces");
#endif
    if (!newPlatformSurface) {
        // refresh buffer state
        RefPtr<PlatformSurfaceInfo> surfaceInfo;
        for (PlatformSurfaceMap::iterator iter = m_platformSurfaces.begin(); iter != m_platformSurfaces.end(); ++iter) {
            surfaceInfo = iter->second;
            surfaceInfo->m_used = false;
        }
        newPlatformSurface = surfaceInfo->m_platformSurface.get();
    }

    return newPlatformSurface;
}

bool GraphicsContext3DOffscreen::swapPlatformSurfaces()
{
    if (!m_currentPlatformSurface) {
        m_currentPlatformSurface = getFreePlatformSurface();
        return m_currentPlatformSurface ? true : false;
    }
    if (m_layerComposited)
        return true;

    GL_CMD(glFlush());
#if ENABLE(TIZEN_BROWSER_WEBGL_DASH_MODE)
    if (!m_webglDashModeTimer.isActive()) {
        WTF::setWebkitDashMode(WTF::DashModeBrowserJavaScript, 3000);
        m_webglDashModeTimer.startOneShot(2.9);
    }
#endif

    m_layerComposited = true;

    if (m_currentPlatformSurface) {
        RefPtr<PlatformSurfaceInfo> surfaceInfo = m_platformSurfaces.get(m_currentPlatformSurface->id());
        ASSERT(surfaceInfo);
        surfaceInfo->m_used = true;
    }

    m_previousPlatformSurface = m_currentPlatformSurface;
    m_currentPlatformSurface = getFreePlatformSurface();
    return m_currentPlatformSurface ? true : false;
}

void GraphicsContext3DOffscreen::freePlatformSurface(int id)
{
    RefPtr<PlatformSurfaceInfo> surfaceInfo = m_platformSurfaces.get(id);
    if (surfaceInfo) {
        surfaceInfo->m_used = false;
        if (surfaceInfo->m_size != IntSize(m_width, m_height)) {
            surfaceInfo.release();
            m_platformSurfaces.remove(id);
        }
    }
}

void GraphicsContext3DOffscreen::removePlatformSurface(int id)
{
    RefPtr<PlatformSurfaceInfo> surfaceInfo = m_platformSurfaces.get(id);
    if (surfaceInfo) {
        surfaceInfo->m_used = false;
        if (layerComposited() && m_previousPlatformSurface) {
            if (m_previousPlatformSurface == surfaceInfo->m_platformSurface) {
                m_currentPlatformSurface = 0;
                return;
            }
        } else if (m_currentPlatformSurface == surfaceInfo->m_platformSurface) {
            m_previousPlatformSurface = 0;
            return;
        }
        surfaceInfo.release();
    }
    m_platformSurfaces.remove(id);
}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
uint32_t GraphicsContext3DOffscreen::copyToGraphicsSurface()
{
#if !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (!m_graphicsSurface)
        return 0;

    eglCopyBuffers(m_display, m_surface, m_platformSurfaceID);
    return 1;
#else
    // If current buffer is not drawn, use previous buffer.
    if (layerComposited() && m_previousPlatformSurface)
        return m_previousPlatformSurface->id();

    return m_currentPlatformSurface ? m_currentPlatformSurface->id() : 0;
#endif
}

uint64_t GraphicsContext3DOffscreen::graphicsSurfaceToken() const
{
    return m_graphicsSurface ? m_graphicsSurface->exportToken() : 0;
}

int GraphicsContext3DOffscreen::graphicsSurfaceFlags() const
{
    int graphicsSurfaceFlags = 0;
    if (m_attributes.alpha)
        graphicsSurfaceFlags |= GraphicsSurface::Alpha;
    if (m_attributes.premultipliedAlpha)
        graphicsSurfaceFlags |= GraphicsSurface::PremultipliedAlpha;
    return graphicsSurfaceFlags;
}


void GraphicsContext3DOffscreen::createGraphicsSurfaces(const IntSize& size)
{
    if (size.isEmpty())
        m_graphicsSurface.clear();
    else
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    {
        if (!m_currentPlatformSurface)
            return;
        m_graphicsSurface = GraphicsSurface::create(m_currentPlatformSurface->id());
    }
#else
        m_graphicsSurface = GraphicsSurface::create(m_platformSurfaceID);
#endif
}
#endif // ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
#endif // ENABLE(TIZEN_WEBKIT2)

void GraphicsContext3DOffscreen::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity)
{
#if ENABLE(TIZEN_WEBKIT2)
    notImplemented();
#else
    if (m_attributes.preserveDrawingBuffer) {
        notImplemented();
        return;
    }

    TextureMapperEvasGL* texmapEvasGL = static_cast<TextureMapperEvasGL*>(textureMapper);
    texmapEvasGL->drawTexture(m_compositorTexture, !m_attributes.alpha, FloatSize(1, 1), targetRect, matrix, opacity, true);
#endif
}
#endif // USE(TIZEN_TEXTURE_MAPPER)

void GraphicsContext3DOffscreen::prepareTexture(int width, int height)
{
    if (m_layerComposited)
        return;

    // FIXME: See the below comment about preserveDrawingBuffer in GC3DOffscreen::reshape().
    bool needToRestoreFBO = false;
    makeContextCurrent();
    if (m_boundFBO != m_fbo) {
        needToRestoreFBO = true;
        GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    }
    GL_CMD(glActiveTexture(GL_TEXTURE0));
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_compositorTexture));
    GL_CMD(glCopyTexImage2D(GL_TEXTURE_2D, 0, m_internalColorFormat, 0, 0, width, height, 0));
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_boundTexture0));
    GL_CMD(glActiveTexture(m_activeTexture));
    if (needToRestoreFBO)
        GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_boundFBO));
    GL_CMD(glFinish());

    m_layerComposited = true;
}

void GraphicsContext3DOffscreen::readRenderingResults(unsigned char *pixels, int pixelsSize, int width, int height)
{
    int totalBytes = width * height * 4;
    if (pixelsSize < totalBytes)
        return;

    makeContextCurrent();

    bool mustRestoreFBO = false;
    if (m_attributes.antialias)
        notImplemented();
    else {
        if (m_boundFBO != m_fbo) {
            mustRestoreFBO = true;
            GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
        }
    }

    GLint packAlignment = 4;
    bool mustRestorePackAlignment = false;
    GL_CMD(glGetIntegerv(GL_PACK_ALIGNMENT, &packAlignment));
    if (packAlignment > 4) {
        GL_CMD(glPixelStorei(GL_PACK_ALIGNMENT, 4));
        mustRestorePackAlignment = true;
    }

    GL_CMD(glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels));
    // Convert to BGRA
    for (int i = 0; i < totalBytes; i += 4)
        std::swap(pixels[i], pixels[i + 2]);

    if (mustRestorePackAlignment)
        GL_CMD(glPixelStorei(GL_PACK_ALIGNMENT, packAlignment));

    if (mustRestoreFBO)
        GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_boundFBO));
}

void GraphicsContext3DOffscreen::reshape(int width, int height)
{
    if (!platformGraphicsContext3D())
        return;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("width %d height %d", width, height);
#endif

#if ENABLE(TIZEN_WEBKIT2)
#if ENABLE(TIZEN_LITE_TEXTURE_RESIZE_OPTIMIZATION) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    IntSize adjustedSize(width,height);
    int screenWidth  = DefaultScreenOfDisplay(g_nativeDisplay)->width;
    int screenHeight = DefaultScreenOfDisplay(g_nativeDisplay)->height;
    int scaleAttempts = 0;
    while ( adjustedSize.width() > screenWidth && adjustedSize.height() > screenHeight ) {
        if(adjustedSize.width()!=adjustedSize.height() || scaleAttempts >= s_maxScaleAttempts)
            break;
        scaleAttempts++;
        adjustedSize.scale(s_resourceAdjustedRatio);
    }
    m_width = adjustedSize.width();
    m_height = adjustedSize.height();
#else
    m_width = width;
    m_height = height;
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    PlatformSurfaceMap::iterator end = m_platformSurfaces.end();
    for (PlatformSurfaceMap::iterator iter = m_platformSurfaces.begin(); iter != end; ++iter) {
        RefPtr<PlatformSurfaceInfo> surfaceInfo = iter->second;
        if (!surfaceInfo->m_used) {
            surfaceInfo.release();
            m_platformSurfaces.remove(iter);
        }
    }

    m_previousPlatformSurface = 0;
    m_currentPlatformSurface = getFreePlatformSurface();
#else
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_surface != EGL_NO_SURFACE) {
        eglDestroySurface(m_display, m_surface);
        m_surface = EGL_NO_SURFACE;
    }
    if (m_platformSurfaceID) {
        XFreePixmap(g_nativeDisplay, m_platformSurfaceID);
        m_platformSurfaceID = 0;
    }
    // FIXME: Set platformSurface depth according to alpha flag.
    m_platformSurfaceID = XCreatePixmap(g_nativeDisplay, g_nativeWindow, m_width, m_height, DefaultDepth(g_nativeDisplay, DefaultScreen(g_nativeDisplay)));
    XFlush(g_nativeDisplay);
    m_surface = eglCreatePixmapSurface(m_display, m_surfaceConfig, m_platformSurfaceID, 0);
    if (m_surface == EGL_NO_SURFACE) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("failed to create PixmapSurface");
#endif
        return;
    }
#endif

#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    GL_CMD(glFlush()); // Make sure all GL calls have been committed before resizing.
    createGraphicsSurfaces(IntSize(width, height));
#endif
#endif // ENABLE(TIZEN_WEBKIT2)

    makeContextCurrent();

#if !ENABLE(TIZEN_WEBKIT2)
    GLuint colorFormat = GL_RGBA;
    GLuint type = GL_UNSIGNED_BYTE;
#endif // !ENABLE(TIZEN_WEBKIT2)

    if (m_attributes.alpha) {
        m_internalColorFormat = GL_RGBA;
    } else {
        m_internalColorFormat = GL_RGB;
#if !ENABLE(TIZEN_WEBKIT2)
        colorFormat = GL_RGB;
        type = GL_UNSIGNED_SHORT_5_6_5;
#endif // !ENABLE(TIZEN_WEBKIT2)
    }

    if (m_attributes.antialias)
        notImplemented();

#if !ENABLE(TIZEN_WEBKIT2)
    // resize regular FBO
    bool mustRestoreFBO = false;
    if (m_boundFBO != m_fbo) {
        mustRestoreFBO = true;
        GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    }

    // FIXME: Do we need to care for preserveDrawingBuffer here? I haven't found the case.
    // It seems we don't need to copy from m_texture to m_compositorTexture. It might be enough
    // that using m_compositorTexture only, then we might be able to improve a bit performance
    // since no texture copy is required.
    // But I don't have any confidence & evidence it'll make no problem, so let's keep
    // the legacy codes. If redering directly to compositorTexture causes an any issue,
    // then recover the case seperating preserveDrawingBuffer.
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_texture));
    GL_CMD(glTexImage2D(GL_TEXTURE_2D, 0, m_internalColorFormat, width, height, 0, colorFormat, type, 0));
    GL_CMD(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0));
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_compositorTexture));
    GL_CMD(glTexImage2D(GL_TEXTURE_2D, 0, m_internalColorFormat, width, height, 0, colorFormat, type, 0));

    GL_CMD(glBindTexture(GL_TEXTURE_2D, 0));

    if (!m_attributes.antialias) {
        if (m_attributes.stencil) {
            GL_CMD(glBindRenderbuffer(GL_RENDERBUFFER, m_stencilBuffer));
            GL_CMD(glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, width, height));
            GL_CMD(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_stencilBuffer));
        }

        if (m_attributes.depth) {
            GL_CMD(glBindRenderbuffer(GL_RENDERBUFFER, m_depthBuffer));
            GL_CMD(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height));
            GL_CMD(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depthBuffer));
        }

        GL_CMD(glBindRenderbuffer(GL_RENDERBUFFER, 0));
    }
#endif // !ENABLE(TIZEN_WEBKIT2)

    // Initialize renderbuffers to 0.
    GLfloat clearColorValue[] = {0, 0, 0, 0}, clearDepthValue = 0;
    GLint clearStencilValue = 0;
    GLboolean colorMaskValue[] = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE}, depthMaskValue = GL_TRUE;
    GLuint stencilMask = 0xffffffff;
    GLboolean isScissorEnabled = GL_FALSE;
    GLboolean isDitherEnabled = GL_FALSE;

    GLbitfield clearMask = GL_COLOR_BUFFER_BIT;

    GL_CMD(glGetFloatv(GL_COLOR_CLEAR_VALUE, clearColorValue));
    GL_CMD(glClearColor(0, 0, 0, 0));
    GL_CMD(glGetBooleanv(GL_COLOR_WRITEMASK, colorMaskValue));
    GL_CMD(glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE));

    if (m_attributes.depth) {
        GL_CMD(glGetFloatv(GL_DEPTH_CLEAR_VALUE, &clearDepthValue));
        GL_CMD(glClearDepthf(1.0));
        GL_CMD(glGetBooleanv(GL_DEPTH_WRITEMASK, &depthMaskValue));
        GL_CMD(glDepthMask(GL_TRUE));
        clearMask |= GL_DEPTH_BUFFER_BIT;
    }
    if (m_attributes.stencil) {
        GL_CMD(glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &clearStencilValue));
        GL_CMD(glClearStencil(0));
        GL_CMD(glGetIntegerv(GL_STENCIL_WRITEMASK, reinterpret_cast<GLint*>(&stencilMask)));
        GL_CMD(glStencilMaskSeparate(GL_FRONT, 0xffffffff));
        clearMask |= GL_STENCIL_BUFFER_BIT;
    }

    isScissorEnabled = GL_CMD(glIsEnabled(GL_SCISSOR_TEST));
    GL_CMD(glDisable(GL_SCISSOR_TEST));
    isDitherEnabled = GL_CMD(glIsEnabled(GL_DITHER));
    GL_CMD(glDisable(GL_DITHER));

    GL_CMD(glClear(clearMask));

    GL_CMD(glClearColor(clearColorValue[0], clearColorValue[1], clearColorValue[2], clearColorValue[3]));
    GL_CMD(glColorMask(colorMaskValue[0], colorMaskValue[1], colorMaskValue[2], colorMaskValue[3]));

    if (m_attributes.depth) {
        GL_CMD(glClearDepthf(clearDepthValue));
        GL_CMD(glDepthMask(depthMaskValue));
    }
    if (m_attributes.stencil) {
        GL_CMD(glClearStencil(clearStencilValue));
        GL_CMD(glStencilMaskSeparate(GL_FRONT, stencilMask));
    }

    if (isScissorEnabled)
        GL_CMD(glEnable(GL_SCISSOR_TEST));
    else
        GL_CMD(glDisable(GL_SCISSOR_TEST));

    if (isDitherEnabled)
        GL_CMD(glEnable(GL_DITHER));
    else
        GL_CMD(glDisable(GL_DITHER));

#if !ENABLE(TIZEN_WEBKIT2)
    if (mustRestoreFBO)
        GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_boundFBO));
#endif

    GL_CMD(glFlush());
}

void GraphicsContext3DOffscreen::activeTexture(GC3Denum texture)
{
    m_activeTexture = texture;
    GraphicsContext3DInternal::activeTexture(texture);
}

void GraphicsContext3DOffscreen::bindFramebuffer(GC3Denum target, Platform3DObject framebuffer)
{
    GLuint fbo = 0;
    if (framebuffer)
        fbo = framebuffer;
    else
        fbo = m_fbo;

    if (fbo != m_boundFBO) {
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
        // FIXME : Below lines are workaround codes.(driver issue)
        // PlatformSurface surface is not updated properly after binding FBO.
        // It seems previous frame is remained.
        if (framebuffer && !m_layerComposited)
            GL_CMD(glFlush());
#endif
        m_boundFBO = fbo;
        GraphicsContext3DInternal::bindFramebuffer(target, fbo);
    }
}

void GraphicsContext3DOffscreen::bindTexture(GC3Denum target, Platform3DObject texture)
{
    if (m_activeTexture == GL_TEXTURE0 && target == GL_TEXTURE_2D)
        m_boundTexture0 = texture;
    GraphicsContext3DInternal::bindTexture(target, texture);
}

void GraphicsContext3DOffscreen::bufferData(GC3Denum target, GC3Dsizeiptr size, const void* data, GC3Denum usage)
{
    GraphicsContext3DInternal::bufferData(target, size, data, usage);

    // FIXME : Driver issue!
    // Following lines are temporary to avoid webgl conformance test failures.
    // See draw-arrays-out-of-bounds.html.
    // If a parameter size inputted to glBufferData is zero,
    // GL_OUT_OF_MEMORY error is occurred while running in Mali driver.
    if (!size) {
        GC3Denum error = getError();
        if (error == GL_OUT_OF_MEMORY)
            return; // No error.
        synthesizeGLError(error); // Push the error back again.
    }
}
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void GraphicsContext3DOffscreen::framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, Platform3DObject texture, GC3Dint level)
{
    // We should probably remove the tid that was set to framebuffer from m_renderTargetTextures
    // if texture is no longer rendered to, but since this is temporary code that will be removed
    // when the DDK is fixed, and that scenario is quite unlikely to happen, I'll just leave it as is.
    if (target != Extensions3D::READ_FRAMEBUFFER && texture)
        m_renderTargetTextures.add(texture);

    GraphicsContext3DInternal::framebufferTexture2D(target, attachment, textarget, texture, level);
}

void GraphicsContext3DOffscreen::generateMipmap(GC3Denum target)
{
    // FIXME: Render to texture related driver issue.
    // Remove GC3DOffscreen::generateMipmap and related code when driver issue is fixed.
    // The rendered texture contents are not properly flushed before generating mipmaps.
    int boundTexture;
    GL_CMD(glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTexture));
    if (m_renderTargetTextures.contains(boundTexture))
        GL_CMD(glFlush());

    GraphicsContext3DInternal::generateMipmap(target);
}
#endif
bool GraphicsContext3DOffscreen::getActiveAttrib(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return false;
    }

    makeContextCurrent();
    GLint maxAttributeSize = 0;
    GL_CMD(glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxAttributeSize));

    GLchar name[maxAttributeSize]; // GL_ACTIVE_ATTRIBUTE_MAX_LENGTH includes null termination
    GLsizei nameLength = 0;
    GLint size = 0;
    GLenum type = 0;
    GL_CMD(glGetActiveAttrib(program, index, maxAttributeSize, &nameLength, &size, &type, name));
    if (!nameLength)
        return false;

    info.name = String(name, nameLength);
    info.type = type;
    info.size = size;
    return true;
}


bool GraphicsContext3DOffscreen::getActiveUniform(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(GL_INVALID_VALUE);
        return false;
    }

    makeContextCurrent();
    GC3Dint maxUniformSize = 0;
    GL_CMD(glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformSize));

    GLchar name[maxUniformSize]; // GL_ACTIVE_UNIFORM_MAX_LENGTH includes null termination
    GC3Dsizei nameLength = 0;
    GC3Dint size = 0;
    GC3Denum type = 0;
    GL_CMD(glGetActiveUniform(program, index, maxUniformSize, &nameLength, &size, &type, name));
    if (!nameLength)
        return false;

    info.name = String(name, nameLength);
    info.type = type;
    info.size = size;

    // FIXME : "[0]" is added at end of name in WebGLRenderingContext::getActiveUniform()
    // when isGLES2Compliant is only false. I can't understand the reason.
    // I add "[0]" here to avoid faling webgl conformance test(get-active-test.html), 
    // but if it makes something wrong later, remove following lines
    if (isGLES2Compliant() && info.size > 1 && !info.name.endsWith("[0]"))
        info.name.append("[0]");

    return true;
}

void GraphicsContext3DOffscreen::getFloatv(GC3Denum pname, GC3Dfloat* value)
{
    GraphicsContext3DInternal::getFloatv(pname, value);

    // FIXME : Fllowing lines are for fixing SGX & Mali driver issue.
    switch (pname) {
    case GL_ALIASED_POINT_SIZE_RANGE:
    case GL_ALIASED_LINE_WIDTH_RANGE:
        if (value[0] == value[1]) {
            if (!value[0])
                value[1] = 1.0;
            else {
                if (value[0] < 1.0)
                    value[1] = 1.0;
                else
                    value[0] = 1.0;
            }
        } else {
            if (value[0] < value[1]) {
                if (value[1] < 1.0) {
                    value[0] = value[1];
                    value[1] = 1.0;
                } else
                    value[0] = 1.0;
            } else {
                if (value[0] < 1.0)
                    value[1] = 1.0;
                else {
                    value[1] = value[0];
                    value[0] = 1.0;
                }
            }
        }
        break;
    }
}

void GraphicsContext3DOffscreen::getIntegerv(GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    GL_CMD(glGetIntegerv(pname, value));
}

bool GraphicsContext3DOffscreen::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels)
{
    GraphicsContext3DInternal::texImage2D(target, level, internalformat, width, height, border, format, type, pixels);
    return true;
}

// below functions are for ANGLE integration
#if ENABLE(TIZEN_WEBGL_ANGLE_INTEGRATION)
void GraphicsContext3DOffscreen::compileShader(Platform3DObject shader)
{
    ASSERT(shader);
    makeContextCurrent();

    int GLshaderType;
    ANGLEShaderType shaderType;

    GL_CMD(glGetShaderiv(shader, GraphicsContext3D::SHADER_TYPE, &GLshaderType));

    if (GLshaderType == GraphicsContext3D::VERTEX_SHADER)
        shaderType = SHADER_TYPE_VERTEX;
    else if (GLshaderType == GraphicsContext3D::FRAGMENT_SHADER)
        shaderType = SHADER_TYPE_FRAGMENT;
    else
        return; // Invalid shader type.

    HashMap<Platform3DObject, ShaderSourceEntry>::iterator result = m_shaderSourceMap.find(shader);

    if (result == m_shaderSourceMap.end())
        return;

    ShaderSourceEntry& entry = result->second;

    String translatedShaderSource;
    String shaderInfoLog;

    Vector<ANGLEShaderSymbol> symbols;
   bool isValid = m_compiler.compileShaderSource(entry.source.utf8().data(), shaderType, translatedShaderSource, shaderInfoLog,symbols);

    entry.log = shaderInfoLog;
    entry.isValid = isValid;

    if (!isValid)
        return; // Shader didn't validate, don't move forward with compiling translated source.

    int len = entry.source.length();
    CString cstr = entry.source.utf8();
    const char* s = cstr.data();

    GL_CMD(glShaderSource(shader, 1, &s, &len));
    GL_CMD(glCompileShader(shader));

}

void GraphicsContext3DOffscreen::getShaderiv(Platform3DObject shader, GC3Denum pname, GC3Dint* value)
{
    ASSERT(shader);

    makeContextCurrent();

    HashMap<Platform3DObject, ShaderSourceEntry>::iterator result = m_shaderSourceMap.find(shader);

    switch (pname) {
    case GraphicsContext3D::DELETE_STATUS:
    case GraphicsContext3D::SHADER_TYPE:
        GL_CMD(glGetShaderiv(shader, pname, value));
        break;
    case GraphicsContext3D::COMPILE_STATUS:
        if (result == m_shaderSourceMap.end()) {
            *value = static_cast<int>(false);
            return;
        }
        *value = static_cast<int>(result->second.isValid);
        break;
    case GraphicsContext3D::INFO_LOG_LENGTH:
        if (result == m_shaderSourceMap.end()) {
            *value = 0;
            return;
        }
        *value = getShaderInfoLog(shader).length();
        break;
    case GraphicsContext3D::SHADER_SOURCE_LENGTH:
        *value = getShaderSource(shader).length();
        break;
    default:
        synthesizeGLError(GraphicsContext3D::INVALID_ENUM);
    }
}

String GraphicsContext3DOffscreen::getShaderInfoLog(Platform3DObject shader)
{
    ASSERT(shader);

    makeContextCurrent();

    HashMap<Platform3DObject, ShaderSourceEntry>::iterator result = m_shaderSourceMap.find(shader);
    if (result == m_shaderSourceMap.end())
        return String();

    ShaderSourceEntry entry = result->second;
    if (!entry.isValid)
        return entry.log;

    GLint length = 0;
    GL_CMD(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length));
    if (!length)
        return String();

    GLsizei size = 0;
    OwnArrayPtr<GLchar> info = adoptArrayPtr(new GLchar[length]);
    GL_CMD(glGetShaderInfoLog(shader, length, &size, info.get()));

    return String(info.get());
}

String GraphicsContext3DOffscreen::getShaderSource(Platform3DObject shader)
{
    ASSERT(shader);

    makeContextCurrent();

    HashMap<Platform3DObject, ShaderSourceEntry>::iterator result = m_shaderSourceMap.find(shader);
    if (result == m_shaderSourceMap.end())
        return String();

    return result->second.source;
}

void GraphicsContext3DOffscreen::shaderSource(Platform3DObject shader, const String& string)
{
    ASSERT(shader);

    makeContextCurrent();

    ShaderSourceEntry entry;

    entry.source = string;

    m_shaderSourceMap.set(shader, entry);
}
#endif
// ANGLE Integration block ENDs

} // namespace WebCore

#endif
#endif
