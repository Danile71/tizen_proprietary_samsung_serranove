/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Holger Hans Peter Freyther <zecke@selfish.org>
 * Copyright (C) 2008, 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ImageBuffer.h"

#include "BitmapImage.h"
#include "CairoUtilities.h"
#include "Color.h"
#include "GraphicsContext.h"
#include "ImageData.h"
#include "MIMETypeRegistry.h"
#include "NotImplemented.h"
#include "Pattern.h"
#include "PlatformContextCairo.h"
#include "PlatformString.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <wtf/Vector.h>
#include <wtf/text/Base64.h>

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#include <cairo-gl.h>
#include <stdio.h>
#include <Ecore_X.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING) || ENABLE(TIZEN_CANVAS_SURFACE_LOCKING)
#include "PixmapContextTizen.h"
#include "SharedPlatformSurfaceTizen.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

using namespace std;

namespace WebCore {

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING) || ENABLE(TIZEN_CANVAS_SURFACE_LOCKING)
extern void* g_nativeDisplay;
extern int g_nativeWindow;
void* g_egl_display = 0;
#endif

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
cairo_device_t* g_sharedCairoDevice = 0;
static PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHR = 0;
static PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOES = 0;
static PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHR = 0;
#endif

ImageBufferData::ImageBufferData(const IntSize& size)
    : m_surface(0)
    , m_platformContext(0)
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
    , m_frontSurface(0)
    , m_isOffscreenEnabled(false)
#endif
{
}

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING) || ENABLE(TIZEN_CANVAS_SURFACE_LOCKING)
ImageBuffer::ImageBuffer(const FloatSize& size, float /* resolutionScale */, ColorSpace, RenderingMode renderingMode, DeferralMode, bool& success)
    : m_data(IntSize(size))
    , m_size(size)
    , m_logicalSize(size)
    , m_platformSurfaceID(0)
{
    success = false;  // Make early return mean error.
#if ENABLE(TIZEN_CANVAS_SURFACE_LOCKING)
    m_isLockable = true;
#else
    m_isLockable = (renderingMode == AcceleratedMemorySaving) ? true : false;
#endif

    bool hasDepth = true;
    bool hasStencil = true;
    bool hasSamples = true;

    if (renderingMode == Accelerated || renderingMode == AcceleratedMemorySaving) {
        m_data.m_platformSurface = SharedPlatformSurfaceTizen::create(IntSize(size), false, true, hasDepth, hasStencil, hasSamples);
        m_platformSurfaceID = m_data.m_platformSurface->id();

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
        if (!g_sharedCairoDevice) {
            g_egl_display = m_data.m_platformSurface->display();
            g_sharedCairoDevice = cairo_egl_device_create(g_egl_display, m_data.m_platformSurface->context());
            if (cairo_device_status(g_sharedCairoDevice) != CAIRO_STATUS_SUCCESS) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
                TIZEN_LOGE("cairo_egl_device_create failed!\n");
#endif
                return;
            }
            cairo_gl_device_set_thread_aware(g_sharedCairoDevice, 0);
        }
#endif
    }

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    if (m_isLockable || renderingMode == Unaccelerated)
        m_data.m_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.width(), size.height());
    else {
#if !ENABLE(TIZEN_CANVAS_CAIRO_GLES_SURFACE_CREATION_FIX)
        m_data.m_surface = cairo_gl_surface_create_for_egl(static_cast<cairo_device_t*>(g_sharedCairoDevice), m_data.m_platformSurface->eglSurface(), size.width(), size.height());
#endif
        if (!setBindingTexture(m_data.m_surface, m_data.m_glData, m_platformSurfaceID)) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGE("setBindingTexture() failed!\n");
#endif
            return;
        }

        if (m_data.m_platformSurface->lockTbmBuffer())
            m_data.m_platformSurface->unlockTbmBuffer();
        else {
            m_data.m_platformSurface->unlockTbmBuffer();
            return;
        }
    }
#else
    m_data.m_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, size.width(), size.height());
#endif

    if (cairo_surface_status(m_data.m_surface) != CAIRO_STATUS_SUCCESS) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("cairo_gl_surface_create_for_egl() failed!\n");
#endif
        return;  // create will notice we didn't set m_initialized and fail.
    }

    RefPtr<cairo_t> cr = adoptRef(cairo_create(m_data.m_surface));
    m_data.m_platformContext.setCr(cr.get());
    m_context = adoptPtr(new GraphicsContext(&m_data.m_platformContext));
    if (renderingMode == Accelerated || renderingMode == AcceleratedMemorySaving) {
        cairo_set_tolerance(cr.get(), 0.5);
        context()->clearRect(FloatRect(0, 0, size.width(), size.height()));
    }
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
    setOffscreen(true);
#endif
    success = true;
}
#else
ImageBuffer::ImageBuffer(const FloatSize& size, float /* resolutionScale */, ColorSpace, RenderingMode, DeferralMode, bool& success)
    : m_data(IntSize(size))
    , m_size(size)
    , m_logicalSize(size)
{
    success = false;  // Make early return mean error.
    m_data.m_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                                                  size.width(),
                                                  size.height());
    if (cairo_surface_status(m_data.m_surface) != CAIRO_STATUS_SUCCESS)
        return;  // create will notice we didn't set m_initialized and fail.

    RefPtr<cairo_t> cr = adoptRef(cairo_create(m_data.m_surface));
    m_data.m_platformContext.setCr(cr.get());
    m_context = adoptPtr(new GraphicsContext(&m_data.m_platformContext));
    success = true;
}
#endif // ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING) || ENABLE(TIZEN_CANVAS_SURFACE_LOCKING)

ImageBuffer::~ImageBuffer()
{
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING) || ENABLE(TIZEN_CANVAS_SURFACE_LOCKING)
    if (m_platformSurfaceID > 0) {
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
        setOffscreen(false);
#endif
        if (!m_isLockable) {
            if (m_data.m_surface)
                cairo_surface_flush(m_data.m_surface);
            glFlush();
        }

        destroyTexture(m_data.m_glData);
#endif
    }
#endif

    if (m_data.m_surface)
        cairo_surface_destroy(m_data.m_surface);
}

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
bool ImageBuffer::setBindingTexture(cairo_surface_t* surface, ImageBufferGLData& glData, int platformSurfaceID)
{
#if !ENABLE(TIZEN_CANVAS_CAIRO_GLES_SURFACE_CREATION_FIX)
    // FIXME: workaround codes to support canvas-to-canvas copy by using cairo_gl_surface_set_binding_texture API
    // FIXME: this codes have to be removed soon!
    RefPtr<cairo_t> cr = adoptRef(cairo_create(surface));
    cairo_set_source_rgba(cr.get(), 1.0, 1.0, 1.0, 1.0);
    cairo_paint(cr.get());

    glGenTextures(1, &glData.texture);
#endif

    m_data.m_platformSurface->makeContextCurrent();

    if (!eglCreateImageKHR) {
        eglCreateImageKHR = reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        if (!eglCreateImageKHR) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGE("cannot get eglCreateImageKHR\n");
#endif
            return false;
        }
    }
    if (!glEGLImageTargetTexture2DOES) {
        glEGLImageTargetTexture2DOES = reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(eglGetProcAddress("glEGLImageTargetTexture2DOES"));
        if (!glEGLImageTargetTexture2DOES) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
            TIZEN_LOGE("cannot get glEGLImageTargetTexture2DOES\n");
#endif
            return false;
        }
    }

    EGLint attr_pixmap[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    glData.eglImage = eglCreateImageKHR(g_egl_display, EGL_NO_CONTEXT, EGL_NATIVE_PIXMAP_KHR, reinterpret_cast<EGLClientBuffer>(platformSurfaceID), attr_pixmap);
    if (!glData.eglImage) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("cannot create egl image\n");
#endif
        return false;
    }

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_SURFACE_CREATION_FIX)
    glGenTextures(1, &glData.texture);
#endif

    GLint boundTex;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);

    glBindTexture(GL_TEXTURE_2D, glData.texture);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, static_cast<GLeglImageOES>(glData.eglImage));
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_SURFACE_CREATION_FIX)
    surface = cairo_gl_surface_create_for_texture(g_sharedCairoDevice, CAIRO_CONTENT_COLOR_ALPHA, glData.texture, m_size.width(), m_size.height());
    if (!surface) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("cairo_gl_surface_create_for_texture() failed!\n");
#endif
        return false;
    }
#else
    cairo_status_t status = cairo_gl_surface_set_binding_texture(surface, glData.texture);
    if (status != CAIRO_STATUS_SUCCESS) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("cairo_gl_surface_set_binding_texture() failed!\n");
#endif
        return false;
    }
#endif

    glBindTexture(GL_TEXTURE_2D, boundTex);
    return true;
}

void ImageBuffer::destroyTexture(ImageBufferGLData& glData)
{
    if (glData.texture) {
        glDeleteTextures(1, &glData.texture);
        glData.texture = 0;
    }

    if (glData.eglImage) {
        if (!eglDestroyImageKHR)
            eglDestroyImageKHR = reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
        if (eglDestroyImageKHR(g_egl_display, glData.eglImage) != EGL_TRUE) {
            TIZEN_LOGE("eglDestroyImageKHR failed!\n");
        }
        glData.eglImage = 0;
    }
}
#endif

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING) || ENABLE(TIZEN_CANVAS_SURFACE_LOCKING)
void ImageBuffer::swapPlatformSurfaces()
{
    RefPtr<cairo_surface_t> destinationSurface = querySurface();
    if (!destinationSurface)
        return;

    // XXX: This is likely inefficient and should be improved.
    // Optimally, we would write directly in GPU memory and
    // swap buffers.
    RefPtr<cairo_t> cr = adoptRef(cairo_create(destinationSurface.get()));
    cairo_set_source_surface(cr.get(), m_data.m_surface, 0, 0);
    cairo_set_operator(cr.get(), CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr.get());

    m_data.m_platformSurface->unlockTbmBuffer();
}

PassRefPtr<cairo_surface_t> ImageBuffer::querySurface() const
{
    unsigned char* bitmapPtr = static_cast<unsigned char*>(m_data.m_platformSurface->lockTbmBuffer());
    int pitch = m_data.m_platformSurface->getStride();

    return adoptRef(cairo_image_surface_create_for_data(bitmapPtr, CAIRO_FORMAT_ARGB32, m_size.width(), m_size.height(), pitch));
}
#endif

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
bool ImageBuffer::makeEGLContextCurrent()
{
    if (cairo_surface_get_type(m_data.m_surface) == CAIRO_SURFACE_TYPE_GL)
        return m_data.m_platformSurface->makeContextCurrent();

    return false;
}
#endif

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER)
void ImageBuffer::setOffscreen(bool enabled)
{
    bool success = true;
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER_GL_SURFACE)
    if (enabled && m_platformSurfaceID > 0 && !isOffscreenEnabled()) {
        bool hasDepth = true;
        bool hasStencil = true;
        bool hasSamples = true;
        m_data.m_frontPlatformSurface = SharedPlatformSurfaceTizen::create(m_size, false, true, hasDepth, hasStencil, hasSamples);
#if !ENABLE(TIZEN_CANVAS_CAIRO_GLES_SURFACE_CREATION_FIX)
        m_data.m_frontSurface = cairo_gl_surface_create_for_egl(static_cast<cairo_device_t*>(g_sharedCairoDevice), m_data.m_frontPlatformSurface->eglSurface(), m_size.width(), m_size.height());
#endif
        if (!setBindingTexture(m_data.m_frontSurface, m_data.m_frontGlData, m_data.m_frontPlatformSurface->id())) {
            TIZEN_LOGE("setBindingTexture() failed!\n");
            success = false;
        }

        if (success)
            success = (cairo_surface_status(m_data.m_frontSurface) == CAIRO_STATUS_SUCCESS);

        if (success) {
            if (m_data.m_frontPlatformSurface->lockTbmBuffer())
                m_data.m_frontPlatformSurface->unlockTbmBuffer();
            else {
                m_data.m_frontPlatformSurface->unlockTbmBuffer();
                success = false;
            }
        }

        if (success) {
            m_data.m_platformContext.setPixmapCr(cairo_create(m_data.m_frontSurface));
            m_platformSurfaceID = m_data.m_frontPlatformSurface->id();
            m_data.m_isOffscreenEnabled = true;
            TIZEN_LOGI("Canvas Offscreen Enabled(gl_surface)");
        } else {
            TIZEN_LOGE("Enable Canvas Offscreen Failed!\n");
            destroyTexture(m_data.m_frontGlData);
            cairo_surface_destroy(m_data.m_frontSurface);
            m_data.m_frontPlatformSurface.clear();
        }
    } else if (!enabled) {
        if (isOffscreenEnabled()) {
            m_data.m_isOffscreenEnabled = false;
            m_platformSurfaceID = m_data.m_platformSurface->id();
            m_data.m_platformContext.setPixmapCr(nullptr);
            destroyTexture(m_data.m_frontGlData);
            cairo_surface_destroy(m_data.m_frontSurface);
            m_data.m_frontPlatformSurface.clear();
            TIZEN_LOGI("Canvas Offscreen Removed");
        }
    }
#else
    if (enabled && m_platformSurfaceID > 0 && !isOffscreenEnabled()) {
        cairo_surface_t* similarSurface = cairo_surface_create_similar(m_data.m_surface, CAIRO_CONTENT_COLOR_ALPHA, m_size.width(), m_size.height());

        if (success) {
            success = (cairo_surface_status(similarSurface) == CAIRO_STATUS_SUCCESS);
            TIZEN_LOGI("[SOONI]surface type - back:%d, front:%d", cairo_surface_get_type(similarSurface), cairo_surface_get_type(m_data.m_surface));
        }

        if (success) {
            m_data.m_frontSurface = m_data.m_surface;
            m_data.m_surface = similarSurface;
            m_data.m_platformContext.setPixmapCr(m_data.m_platformContext.cr());
            m_data.m_platformContext.setCr(cairo_create(m_data.m_surface));
            m_data.m_isOffscreenEnabled = true;
            TIZEN_LOGI("Canvas Offscreen Enabled(similar_surface)");
        } else {
            TIZEN_LOGE("Enable Canvas Offscreen Failed!\n");
            cairo_surface_destroy(similarSurface);
        }
    } else if (!enabled) {
        if (isOffscreenEnabled()) {
            m_data.m_platformContext.drawCanvasOffScreenBuffer(this);
            cairo_surface_t* similarSurface = m_data.m_surface;
            m_data.m_surface = m_data.m_frontSurface;
            m_data.m_platformContext.setCr(m_data.m_platformContext.pixmapCr());
            m_data.m_isOffscreenEnabled = false;
            m_data.m_platformContext.setPixmapCr(nullptr);
            cairo_surface_destroy(similarSurface);
            TIZEN_LOGI("Canvas Offscreen Removed");
        }
    }
#endif
}
#endif

GraphicsContext* ImageBuffer::context() const
{
    return m_context.get();
}

PassRefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior) const
{
    if (copyBehavior == CopyBackingStore)
        return BitmapImage::create(copyCairoImageSurface(m_data.m_surface).leakRef());

    // BitmapImage will release the passed in surface on destruction
    return BitmapImage::create(cairo_surface_reference(m_data.m_surface));
}

void ImageBuffer::clip(GraphicsContext* context, const FloatRect& maskRect) const
{
    context->platformContext()->pushImageMask(m_data.m_surface, maskRect);
}

void ImageBuffer::draw(GraphicsContext* destinationContext, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect, 
    CompositeOperator op, BlendMode blendMode, bool useLowQualityScale)
{
    BackingStoreCopy copyMode = destinationContext == context() ? CopyBackingStore : DontCopyBackingStore;
    RefPtr<Image> image = copyImage(copyMode);
    destinationContext->drawImage(image.get(), styleColorSpace, destRect, srcRect, op, blendMode, DoNotRespectImageOrientation, useLowQualityScale);
}

void ImageBuffer::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform,
                              const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    RefPtr<Image> image = copyImage(DontCopyBackingStore);
    image->drawPattern(context, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    cairo_surface_t* src = 0;
    if (cairo_surface_get_type(m_data.m_surface) == CAIRO_SURFACE_TYPE_GL)
        src = cairo_surface_map_to_image(m_data.m_surface, 0);
    else {
        ASSERT(cairo_surface_get_type(m_data.m_surface) == CAIRO_SURFACE_TYPE_IMAGE);
        src = m_data.m_surface;
    }

    unsigned char* dataSrc = cairo_image_surface_get_data(src);
    int stride = cairo_image_surface_get_stride(src);
#else
    ASSERT(cairo_surface_get_type(m_data.m_surface) == CAIRO_SURFACE_TYPE_IMAGE);

    unsigned char* dataSrc = cairo_image_surface_get_data(m_data.m_surface);
    int stride = cairo_image_surface_get_stride(m_data.m_surface);
#endif
    for (int y = 0; y < m_size.height(); ++y) {
        unsigned* row = reinterpret_cast<unsigned*>(dataSrc + stride * y);
        for (int x = 0; x < m_size.width(); x++) {
            unsigned* pixel = row + x;
            Color pixelColor = colorFromPremultipliedARGB(*pixel);
            pixelColor = Color(lookUpTable[pixelColor.red()],
                               lookUpTable[pixelColor.green()],
                               lookUpTable[pixelColor.blue()],
                               pixelColor.alpha());
            *pixel = premultipliedARGBFromColor(pixelColor);
        }
    }
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    cairo_surface_mark_dirty_rectangle(src, 0, 0, m_size.width(), m_size.height());
    if (cairo_surface_get_type (m_data.m_surface) == CAIRO_SURFACE_TYPE_GL)
        cairo_surface_unmap_image(m_data.m_surface, src);
#else
    cairo_surface_mark_dirty_rectangle(m_data.m_surface, 0, 0, m_size.width(), m_size.height());
#endif
}

template <Multiply multiplied>
PassRefPtr<Uint8ClampedArray> getImageData(const IntRect& rect, const ImageBufferData& data, const IntSize& size)
{
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    unsigned char* dataSrc;
    bool isGlSurface = cairo_surface_get_type(data.m_surface) == CAIRO_SURFACE_TYPE_GL;
    cairo_surface_t* surface = data.m_surface;

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER) && !ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER_GL_SURFACE)
    if(isGlSurface && data.m_isOffscreenEnabled) {
        surface = cairo_surface_map_to_image(data.m_surface, 0);
        if (!surface || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
            return 0;
        isGlSurface = false;
    }
#endif

    if (isGlSurface) {
        cairo_surface_flush(data.m_surface);
        glFinish();
        dataSrc = static_cast<unsigned char*>(data.m_platformSurface->lockTbmBuffer());
    } else {
        ASSERT(cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_IMAGE);
        dataSrc = cairo_image_surface_get_data(surface);
    }
#else
    ASSERT(cairo_surface_get_type(data.m_surface) == CAIRO_SURFACE_TYPE_IMAGE);

    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);
    unsigned char* dataSrc = cairo_image_surface_get_data(data.m_surface);
#endif
    unsigned char* dataDst = result->data();

    if (rect.x() < 0 || rect.y() < 0 || (rect.x() + rect.width()) > size.width() || (rect.y() + rect.height()) > size.height())
        result->zeroFill();

    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }
    int endx = rect.maxX();
    if (endx > size.width())
        endx = size.width();
    int numColumns = endx - originx;

    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }
    int endy = rect.maxY();
    if (endy > size.height())
        endy = size.height();
    int numRows = endy - originy;

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    int stride;
    if (isGlSurface)
        stride = data.m_platformSurface->getStride();
    else
        stride = cairo_image_surface_get_stride(surface);
#else
    int stride = cairo_image_surface_get_stride(data.m_surface);
#endif
    unsigned destBytesPerRow = 4 * rect.width();

    unsigned char* destRows = dataDst + desty * destBytesPerRow + destx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast<unsigned*>(dataSrc + stride * (y + originy));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + originx;

            // Avoid calling Color::colorFromPremultipliedARGB() because one
            // function call per pixel is too expensive.
            unsigned alpha = (*pixel & 0xFF000000) >> 24;
            unsigned red = (*pixel & 0x00FF0000) >> 16;
            unsigned green = (*pixel & 0x0000FF00) >> 8;
            unsigned blue = (*pixel & 0x000000FF);

            if (multiplied == Unmultiplied) {
                if (alpha && alpha != 255) {
                    red = red * 255 / alpha;
                    green = green * 255 / alpha;
                    blue = blue * 255 / alpha;
                }
            }

            destRows[basex]     = red;
            destRows[basex + 1] = green;
            destRows[basex + 2] = blue;
            destRows[basex + 3] = alpha;
        }
        destRows += destBytesPerRow;
    }

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    if (isGlSurface)
        data.m_platformSurface->unlockTbmBuffer();
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER) && !ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER_GL_SURFACE)
    else if(data.m_isOffscreenEnabled)
        cairo_surface_unmap_image(data.m_surface, surface);
#endif
#endif

    return result.release();
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    return getImageData<Unmultiplied>(rect, m_data, m_size);
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    return getImageData<Premultiplied>(rect, m_data, m_size);
}

void ImageBuffer::putByteArray(Multiply multiplied, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem)
{
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    bool isGlSurface = cairo_surface_get_type(m_data.m_surface) == CAIRO_SURFACE_TYPE_GL;
    cairo_surface_t* surface = m_data.m_surface;

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER) && !ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER_GL_SURFACE)
    if(isGlSurface && m_data.m_isOffscreenEnabled) {
        surface = cairo_surface_map_to_image(m_data.m_surface, 0);
        if (!surface || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
            return;
        isGlSurface = false;
    }
#endif

    unsigned char* dataDst;
    if (isGlSurface) {
        cairo_surface_flush(m_data.m_surface);
        glFinish();
        dataDst = static_cast<unsigned char*>(m_data.m_platformSurface->lockTbmBuffer());
    } else {
        ASSERT(cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_IMAGE);
        dataDst = cairo_image_surface_get_data(surface);
    }
#else
    ASSERT(cairo_surface_get_type(m_data.m_surface) == CAIRO_SURFACE_TYPE_IMAGE);

    unsigned char* dataDst = cairo_image_surface_get_data(m_data.m_surface);
#endif

    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);

    int originx = sourceRect.x();
    int destx = destPoint.x() + sourceRect.x();
    ASSERT(destx >= 0);
    ASSERT(destx < m_size.width());
    ASSERT(originx >= 0);
    ASSERT(originx <= sourceRect.maxX());

    int endx = destPoint.x() + sourceRect.maxX();
    ASSERT(endx <= m_size.width());

    int numColumns = endx - destx;

    int originy = sourceRect.y();
    int desty = destPoint.y() + sourceRect.y();
    ASSERT(desty >= 0);
    ASSERT(desty < m_size.height());
    ASSERT(originy >= 0);
    ASSERT(originy <= sourceRect.maxY());

    int endy = destPoint.y() + sourceRect.maxY();
    ASSERT(endy <= m_size.height());
    int numRows = endy - desty;

    unsigned srcBytesPerRow = 4 * sourceSize.width();
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    int stride;
    if (isGlSurface)
        stride = m_data.m_platformSurface->getStride();
    else
        stride = cairo_image_surface_get_stride(surface);
#else
    int stride = cairo_image_surface_get_stride(m_data.m_surface);
#endif

    unsigned char* srcRows = source->data() + originy * srcBytesPerRow + originx * 4;
    for (int y = 0; y < numRows; ++y) {
        unsigned* row = reinterpret_cast<unsigned*>(dataDst + stride * (y + desty));
        for (int x = 0; x < numColumns; x++) {
            int basex = x * 4;
            unsigned* pixel = row + x + destx;

            // Avoid calling Color::premultipliedARGBFromColor() because one
            // function call per pixel is too expensive.
            unsigned red = srcRows[basex];
            unsigned green = srcRows[basex + 1];
            unsigned blue = srcRows[basex + 2];
            unsigned alpha = srcRows[basex + 3];

            if (multiplied == Unmultiplied) {
                if (alpha != 255) {
                    red = (red * alpha + 254) / 255;
                    green = (green * alpha + 254) / 255;
                    blue = (blue * alpha + 254) / 255;
                }
            }

            *pixel = (alpha << 24) | red  << 16 | green  << 8 | blue;
        }
        srcRows += srcBytesPerRow;
    }
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    cairo_surface_mark_dirty_rectangle(surface, destx, desty, numColumns, numRows);
    if (isGlSurface)
        m_data.m_platformSurface->unlockTbmBuffer();
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER) && !ENABLE(TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER_GL_SURFACE)
    else if(m_data.m_isOffscreenEnabled)
        cairo_surface_unmap_image(m_data.m_surface, surface);
#endif
#else
    cairo_surface_mark_dirty_rectangle(m_data.m_surface,
                                        destx, desty,
                                        numColumns, numRows);
#endif
}

#if !PLATFORM(GTK) && !ENABLE(TIZEN_CANVAS_TODATAURL_USING_IMAGE_ENCODER)
static cairo_status_t writeFunction(void* output, const unsigned char* data, unsigned int length)
{
    if (!reinterpret_cast<Vector<unsigned char>*>(output)->tryAppend(data, length))
        return CAIRO_STATUS_WRITE_ERROR;
    return CAIRO_STATUS_SUCCESS;
}

static bool encodeImage(cairo_surface_t* image, const String& mimeType, Vector<char>* output)
{
    ASSERT(mimeType == "image/png"); // Only PNG output is supported for now.

    return cairo_surface_write_to_png_stream(image, writeFunction, output) == CAIRO_STATUS_SUCCESS;
}

String ImageBuffer::toDataURL(const String& mimeType, const double*, CoordinateSystem) const
{
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType));

    cairo_surface_t* image = cairo_get_target(context()->platformContext()->cr());

    Vector<char> encodedImage;
    if (!image || !encodeImage(image, mimeType, &encodedImage))
        return "data:,";

    Vector<char> base64Data;
    base64Encode(encodedImage, base64Data);

    return "data:" + mimeType + ";base64," + base64Data;
}
#endif

#if ENABLE(TIZEN_CANVAS_TODATAURL_USING_IMAGE_ENCODER)
static bool encodeImage(const ImageData& image, const String& mimeType, const double* quality, Vector<char>* output)
{
    if (mimeType == "image/jpeg") {
        Vector<unsigned char>* encodedImage = reinterpret_cast<Vector<unsigned char>*>(output);
        int compressionQuality = 65;
        if (quality && *quality >= 0.0 && *quality <= 1.0)
            compressionQuality = static_cast<int>(*quality * 100);
        return encodeImageDataToJPEG(image, compressionQuality, encodedImage);
    }

    return encodeImageDataToPNG(image, output);
}

String ImageBuffer::toDataURL(const String& mimeType, const double* quality, CoordinateSystem) const
{
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType));

    IntRect rect(0, 0, m_size.width(), m_size.height());
    RefPtr<ImageData> imageData = ImageData::create(rect.size(), getPremultipliedImageData(rect));

    Vector<char> encodedImage;
    if (!imageData || !encodeImage(*imageData, mimeType, quality, &encodedImage))
        return "data:,";

    Vector<char> base64Data;
    base64Encode(encodedImage, base64Data);

    return "data:" + mimeType + ";base64," + base64Data;
}
#endif

} // namespace WebCore
