/*
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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
#include "BitmapImage.h"

#include "ImageObserver.h"
#include "NativeImageCairo.h"
#include "PlatformContextCairo.h"
#include "SharedPlatformSurfaceTizen.h"
#include <cairo.h>
#include <wtf/OwnPtr.h>
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#include <cairo-gl.h>
#endif

namespace WebCore {

PassRefPtr<BitmapImage> BitmapImage::create(cairo_surface_t* surface)
{
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_GL) {
        NativeImageCairo* nativeImage = new NativeImageCairo();
        nativeImage->setGLSurface(surface);
        return BitmapImage::create(nativeImage);
    }
#endif
    return BitmapImage::create(new NativeImageCairo(surface));
}

BitmapImage::BitmapImage(NativeImageCairo* nativeImage, ImageObserver* observer)
    : Image(observer)
    , m_currentFrame(0)
    , m_frames(0)
    , m_frameTimer(0)
    , m_repetitionCount(cAnimationNone)
    , m_repetitionCountStatus(Unknown)
    , m_repetitionsComplete(0)
    , m_decodedSize(0)
    , m_frameCount(1)
    , m_isSolidColor(false)
    , m_checkedForSolidColor(false)
    , m_animationFinished(true)
    , m_allDataReceived(true)
    , m_haveSize(true)
    , m_sizeAvailable(true)
    , m_haveFrameCount(true)
{
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    ASSERT(nativeImage->surfaceType() != NativeImageCairo::NATIVE_NONE_SURFACE);
    cairo_surface_t* surface;
    if (nativeImage->surfaceType() == NativeImageCairo::NATIVE_IMAGE_SURFACE) {
        surface = nativeImage->surface();
        m_isGLTargetSurface = false;
    } else if (nativeImage->surfaceType() == NativeImageCairo::NATIVE_GL_SURFACE) {
        surface = nativeImage->glsurface();
        m_isGLTargetSurface = true;
    }
    m_decodeInCpuMemoryForcely = false;
#else
    cairo_surface_t* surface = nativeImage->surface();
#endif
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    int width = 0, height = 0;
    if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_GL) {
        width = cairo_gl_surface_get_width(surface);
        height = cairo_gl_surface_get_height(surface);
    }
    else {
        width = cairo_image_surface_get_width(surface);
        height = cairo_image_surface_get_height(surface);
    }
#else
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
#endif
    m_decodedSize = width * height * 4;
    m_size = IntSize(width, height);

    m_frames.grow(1);
    m_frames[0].m_frame = nativeImage;
    m_frames[0].m_hasAlpha = cairo_surface_get_content(surface) != CAIRO_CONTENT_COLOR;
    m_frames[0].m_haveMetadata = true;
    checkForSolidColor();
}

#if ENABLE(TIZEN_IMAGE_DECODER_DOWN_SAMPLING) || ENABLE(TIZEN_JPEG_IMAGE_SCALE_DECODING)
static void adjustSourceRectForScaling(FloatRect& srcRect, const FloatSize& origSize, const IntSize& scaledSize)
{
    // We assume down-sampling zoom rates in X direction and in Y direction are same.
    if (origSize.width() == scaledSize.width())
        return;
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    // We assume zoom rates are not same in X,Y direction.
    double rateWidth = static_cast<double>(scaledSize.width()) / origSize.width();
    double rateHeight = static_cast<double>(scaledSize.height()) / origSize.height();
    // Calculate end of srcRect in X direction and apply zoom rate.
    double temp = (srcRect.x() + srcRect.width()) * rateWidth;
    srcRect.setX(srcRect.x() * rateWidth);
    srcRect.setWidth(temp - srcRect.x());
    // Calculate end of srcRect in Y direction and apply zoom rate.
    temp = (srcRect.y() + srcRect.height()) * rateHeight;
    srcRect.setY(srcRect.y() * rateHeight);
    srcRect.setHeight(temp - srcRect.y());
#else
    // Image has been down sampled.
    double rate = static_cast<double>(scaledSize.width()) / origSize.width();
    double temp = (srcRect.x() + srcRect.width()) * rate;
    srcRect.setX(srcRect.x() * rate);
    srcRect.setWidth(temp - srcRect.x());
    temp = (srcRect.y() + srcRect.height()) * rate;
    srcRect.setY(srcRect.y() * rate);
    srcRect.setHeight(temp - srcRect.y());
#endif
}
#endif

void BitmapImage::draw(GraphicsContext* context, const FloatRect& dst, const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op, BlendMode blendMode)
{
    draw(context, dst, src, styleColorSpace, op, blendMode, DoNotRespectImageOrientation);
}

void BitmapImage::draw(GraphicsContext* context, const FloatRect& dst, const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op, BlendMode blendMode, RespectImageOrientationEnum shouldRespectImageOrientation)
{
    FloatRect srcRect(src);
    FloatRect dstRect(dst);

    if (!dst.width() || !dst.height() || !src.width() || !src.height())
        return;

    startAnimation();

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    cairo_surface_type_t surfaceType = cairo_surface_get_type(cairo_get_target(context->platformContext()->cr()));
#endif

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    m_isGLTargetSurface = (surfaceType == CAIRO_SURFACE_TYPE_GL);
    // If the number of frames is not 1 (e.g. animated GIF), then decode it in cpu memory not xpixmap.
    m_decodeInCpuMemoryForcely = (frameCount() != 1);
#endif
    NativeImageCairo* nativeImage = frameAtIndex(m_currentFrame);
    if (!nativeImage) // If it's too early we won't have an image yet.
        return;

    if (mayFillWithSolidColor()) {
        fillWithSolidColor(context, dstRect, solidColor(), styleColorSpace, op);
        return;
    }

    context->save();

    // Set the compositing operation.
    if (op == CompositeSourceOver && blendMode == BlendModeNormal && !frameHasAlphaAtIndex(m_currentFrame))
        context->setCompositeOperation(CompositeCopy);
    else
        context->setCompositeOperation(op, blendMode);

    ImageOrientation orientation = DefaultImageOrientation;
    if (shouldRespectImageOrientation == RespectImageOrientation)
        orientation = frameOrientationAtIndex(m_currentFrame);

    if (orientation != DefaultImageOrientation) {
        // ImageOrientation expects the origin to be at (0, 0).
        context->translate(dstRect.x(), dstRect.y());
        dstRect.setLocation(FloatPoint());
        context->concatCTM(orientation.transformFromDefault(dstRect.size()));
        if (orientation.usesWidthAsHeight()) {
            // The destination rectangle will have it's width and height already reversed for the orientation of
            // the image, as it was needed for page layout, so we need to reverse it back here.
            dstRect = FloatRect(dstRect.x(), dstRect.y(), dstRect.height(), dstRect.width());
        }
    }

#if ENABLE(TIZEN_HIGH_QUALITY_IMAGE_FOR_COMPOSITE_SOURCE_OVER_OPERATION)
    if (op == CompositeSourceOver)
        context->platformContext()->setImageInterpolationQuality(InterpolationMedium);
 #endif

#if ENABLE(TIZEN_IMAGE_DECODER_DOWN_SAMPLING) || ENABLE(TIZEN_JPEG_IMAGE_SCALE_DECODING)
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    IntSize imageSize = IntSize();
    if (nativeImage->surfaceType() == NativeImageCairo::NATIVE_GL_SURFACE) {
        imageSize.setWidth(cairo_gl_surface_get_width(nativeImage->glsurface()));
        imageSize.setHeight(cairo_gl_surface_get_height(nativeImage->glsurface()));
        adjustSourceRectForScaling(srcRect, size(), imageSize);
    } else {
        imageSize.setWidth(cairo_image_surface_get_width(nativeImage->surface()));
        imageSize.setHeight(cairo_image_surface_get_height(nativeImage->surface()));
        adjustSourceRectForScaling(srcRect, size(), imageSize);
    }
#else
    IntSize imageSize(cairo_image_surface_get_width(nativeImage->surface()), cairo_image_surface_get_height(nativeImage->surface()));
    adjustSourceRectForScaling(srcRect, size(), imageSize);
#endif
#endif

#if ENABLE(TIZEN_ADD_AA_CONDITIONS_FOR_NINE_PATCH)
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    if (surfaceType != CAIRO_SURFACE_TYPE_GL) {
        cairo_t* cr = context->platformContext()->cr();
        if ((cairo_get_antialias(cr) != CAIRO_ANTIALIAS_NONE) && (context->getCTM().isIdentityOrTranslationOrFlipped() || context->getCTM().has90MultipleRotation())) {
            cairo_antialias_t savedAntialiasRule = cairo_get_antialias(cr);
            cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
            if (nativeImage->surfaceType() == NativeImageCairo::NATIVE_GL_SURFACE)
                context->platformContext()->drawSurfaceToContext(nativeImage->glsurface(), dstRect, srcRect, context, nativeImage->imageFrame());
            else
                context->platformContext()->drawSurfaceToContext(nativeImage->surface(), dstRect, srcRect, context);
            cairo_set_antialias(cr, savedAntialiasRule);
        } else {
            if (nativeImage->surfaceType() == NativeImageCairo::NATIVE_GL_SURFACE)
                context->platformContext()->drawSurfaceToContext(nativeImage->glsurface(), dstRect, srcRect, context, nativeImage->imageFrame());
            else
                context->platformContext()->drawSurfaceToContext(nativeImage->surface(), dstRect, srcRect, context);
        }
    } else {
        if (nativeImage->surfaceType() == NativeImageCairo::NATIVE_GL_SURFACE)
            context->platformContext()->drawSurfaceToContext(nativeImage->glsurface(), dstRect, srcRect, context, nativeImage->imageFrame());
        else
        context->platformContext()->drawSurfaceToContext(nativeImage->surface(), dstRect, srcRect, context);
    }
#else
    cairo_t* cr = context->platformContext()->cr();
    if ((cairo_get_antialias(cr) != CAIRO_ANTIALIAS_NONE) && (context->getCTM().isIdentityOrTranslationOrFlipped() || context->getCTM().has90MultipleRotation())) {
        cairo_antialias_t savedAntialiasRule = cairo_get_antialias(cr);
        cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);
        context->platformContext()->drawSurfaceToContext(nativeImage->surface(), dstRect, srcRect, context);
        cairo_set_antialias(cr, savedAntialiasRule);
    } else
        context->platformContext()->drawSurfaceToContext(nativeImage->surface(), dstRect, srcRect, context);
#endif
#else
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    if (nativeImage->surfaceType() == NativeImageCairo::NATIVE_GL_SURFACE)
        context->platformContext()->drawSurfaceToContext(nativeImage->glsurface(), dstRect, srcRect, context, nativeImage->imageFrame());
    else
#endif
    context->platformContext()->drawSurfaceToContext(nativeImage->surface(), dstRect, srcRect, context);
#endif

    context->restore();

    if (imageObserver())
        imageObserver()->didDraw(this);
}

void BitmapImage::checkForSolidColor()
{
    m_isSolidColor = false;
    m_checkedForSolidColor = true;

    if (frameCount() > 1)
        return;

    NativeImageCairo* nativeImage = frameAtIndex(m_currentFrame);
    if (!nativeImage) // If it's too early we won't have an image yet.
        return;

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    cairo_surface_t* surface;
    if (nativeImage->surfaceType() == NativeImageCairo::NATIVE_IMAGE_SURFACE)
        surface = nativeImage->surface();
    else
        surface = nativeImage->glsurface();
#else
    cairo_surface_t* surface = nativeImage->surface();
    ASSERT(cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_IMAGE);
#endif

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    int width = 0, height = 0;
    if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_GL) {
        width = cairo_gl_surface_get_width(surface);
        height = cairo_gl_surface_get_height(surface);
    }
    else {
        width = cairo_image_surface_get_width(surface);
        height = cairo_image_surface_get_height(surface);
    }
#else
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
#endif

    if (width != 1 || height != 1)
        return;

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    unsigned* pixelColor = 0;
    if (cairo_surface_get_type(surface) == CAIRO_SURFACE_TYPE_GL) {
        cairo_surface_t* imageSurface = cairo_surface_map_to_image(surface, NULL);
        pixelColor = reinterpret_cast<unsigned*>(cairo_image_surface_get_data(imageSurface));
        m_solidColor = colorFromPremultipliedARGB(*pixelColor);
        cairo_surface_unmap_image(surface, imageSurface);
    } else {
        pixelColor = reinterpret_cast<unsigned*>(cairo_image_surface_get_data(surface));
        m_solidColor = colorFromPremultipliedARGB(*pixelColor);
    }
#else
    unsigned* pixelColor = reinterpret_cast<unsigned*>(cairo_image_surface_get_data(surface));
    m_solidColor = colorFromPremultipliedARGB(*pixelColor);
#endif

    m_isSolidColor = true;
}

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    if (m_frame) {
        delete m_frame;
        m_frame = 0;
        return true;
    }
    return false;
}

#if ENABLE(TIZEN_CACHE_IMAGE_GET)
IntSize BitmapImage::getCacheImageSize(NativeImagePtr bitmapPtr)
{
    IntSize cacheImageSize;

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    if (bitmapPtr->surfaceType() == NativeImageCairo::NATIVE_GL_SURFACE) {
        cacheImageSize.setWidth(cairo_gl_surface_get_width(bitmapPtr->glsurface()));
        cacheImageSize.setHeight(cairo_gl_surface_get_height(bitmapPtr->glsurface()));
    }
    else {
        cacheImageSize.setWidth(cairo_image_surface_get_width(bitmapPtr->surface()));
        cacheImageSize.setHeight(cairo_image_surface_get_height(bitmapPtr->surface()));
    }
#else
    cacheImageSize.setWidth(cairo_image_surface_get_width(bitmapPtr->surface()));
    cacheImageSize.setHeight(cairo_image_surface_get_height(bitmapPtr->surface()));
#endif

    return cacheImageSize;
}
#endif
} // namespace WebCore

