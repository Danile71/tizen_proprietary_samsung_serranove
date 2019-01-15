/*
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION embedded systems
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
#include "CairoUtilities.h"

#include "AffineTransform.h"
#include "Color.h"
#include "FloatPoint.h"
#include "FloatRect.h"
#include "IntRect.h"
#include "OwnPtrCairo.h"
#include "Path.h"
#include "PlatformPathCairo.h"
#include "RefPtrCairo.h"
#include <wtf/Vector.h>

#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#include <cairo-gl.h>
#endif

#if ENABLE(TIZEN_CANVAS_TODATAURL_USING_IMAGE_ENCODER)
#include "ImageData.h"
#include "png.h"
#include "jpeglib.h"
#include <setjmp.h>
#endif

namespace WebCore {

void copyContextProperties(cairo_t* srcCr, cairo_t* dstCr)
{
    cairo_set_antialias(dstCr, cairo_get_antialias(srcCr));

    size_t dashCount = cairo_get_dash_count(srcCr);
    Vector<double> dashes(dashCount);

    double offset;
    cairo_get_dash(srcCr, dashes.data(), &offset);
    cairo_set_dash(dstCr, dashes.data(), dashCount, offset);
    cairo_set_line_cap(dstCr, cairo_get_line_cap(srcCr));
    cairo_set_line_join(dstCr, cairo_get_line_join(srcCr));
    cairo_set_line_width(dstCr, cairo_get_line_width(srcCr));
    cairo_set_miter_limit(dstCr, cairo_get_miter_limit(srcCr));
    cairo_set_fill_rule(dstCr, cairo_get_fill_rule(srcCr));
}

void setSourceRGBAFromColor(cairo_t* context, const Color& color)
{
    float red, green, blue, alpha;
    color.getRGBA(red, green, blue, alpha);
    cairo_set_source_rgba(context, red, green, blue, alpha);
}

void appendPathToCairoContext(cairo_t* to, cairo_t* from)
{
    OwnPtr<cairo_path_t> cairoPath = adoptPtr(cairo_copy_path(from));
    cairo_append_path(to, cairoPath.get());
}

void setPathOnCairoContext(cairo_t* to, cairo_t* from)
{
    cairo_new_path(to);
    appendPathToCairoContext(to, from);
}

void appendWebCorePathToCairoContext(cairo_t* context, const Path& path)
{
    appendPathToCairoContext(context, path.platformPath()->context());
}

void appendRegionToCairoContext(cairo_t* to, const cairo_region_t* region)
{
    if (!region)
        return;

    const int rectCount = cairo_region_num_rectangles(region);
    for (int i = 0; i < rectCount; ++i) {
        cairo_rectangle_int_t rect;
        cairo_region_get_rectangle(region, i, &rect);
        cairo_rectangle(to, rect.x, rect.y, rect.width, rect.height);
    }
}

cairo_operator_t toCairoOperator(CompositeOperator op)
{
    switch (op) {
    case CompositeClear:
        return CAIRO_OPERATOR_CLEAR;
    case CompositeCopy:
        return CAIRO_OPERATOR_SOURCE;
    case CompositeSourceOver:
        return CAIRO_OPERATOR_OVER;
    case CompositeSourceIn:
        return CAIRO_OPERATOR_IN;
    case CompositeSourceOut:
        return CAIRO_OPERATOR_OUT;
    case CompositeSourceAtop:
        return CAIRO_OPERATOR_ATOP;
    case CompositeDestinationOver:
        return CAIRO_OPERATOR_DEST_OVER;
    case CompositeDestinationIn:
        return CAIRO_OPERATOR_DEST_IN;
    case CompositeDestinationOut:
        return CAIRO_OPERATOR_DEST_OUT;
    case CompositeDestinationAtop:
        return CAIRO_OPERATOR_DEST_ATOP;
    case CompositeXOR:
        return CAIRO_OPERATOR_XOR;
    case CompositePlusDarker:
        return CAIRO_OPERATOR_DARKEN;
    case CompositePlusLighter:
        return CAIRO_OPERATOR_ADD;
    case CompositeDifference:
        return CAIRO_OPERATOR_DIFFERENCE;
    default:
        return CAIRO_OPERATOR_SOURCE;
    }
}
cairo_operator_t toCairoOperator(BlendMode blendOp)
{
    switch (blendOp) {
    case BlendModeNormal:
        return CAIRO_OPERATOR_OVER;
    case BlendModeMultiply:
        return CAIRO_OPERATOR_MULTIPLY;
    case BlendModeScreen:
        return CAIRO_OPERATOR_SCREEN;
    case BlendModeOverlay:
        return CAIRO_OPERATOR_OVERLAY;
    case BlendModeDarken:
        return CAIRO_OPERATOR_DARKEN;
    case BlendModeLighten:
        return CAIRO_OPERATOR_LIGHTEN;
    case BlendModeColorDodge:
        return CAIRO_OPERATOR_COLOR_DODGE;
    case BlendModeColorBurn:
        return CAIRO_OPERATOR_COLOR_BURN;
    case BlendModeHardLight:
        return CAIRO_OPERATOR_HARD_LIGHT;
    case BlendModeSoftLight:
        return CAIRO_OPERATOR_SOFT_LIGHT;
    case BlendModeDifference:
        return CAIRO_OPERATOR_DIFFERENCE;
    case BlendModeExclusion:
        return CAIRO_OPERATOR_EXCLUSION;
    case BlendModeHue:
        return CAIRO_OPERATOR_HSL_HUE;
    case BlendModeSaturation:
        return CAIRO_OPERATOR_HSL_SATURATION;
    case BlendModeColor:
        return CAIRO_OPERATOR_HSL_COLOR;
    case BlendModeLuminosity:
        return CAIRO_OPERATOR_HSL_LUMINOSITY;
    default:
        return CAIRO_OPERATOR_OVER;
    }
}

#if ENABLE(TIZEN_ADJUST_PATTERN_MATRIX)
// from cairo's round-down function(_cairo_fixed_round_down)
static inline double roundDown(double val)
{
    union {
        double d;
        int32_t i[2];
    } u;
    int fixed;

    u.d = val + ((1LL << (52 - 8/*from CAIRO_FIXED_FRAC_BITS*/)) * 1.5) /*from CAIRO_MAGIC_NUMBER_FIXED*/;
    fixed = u.i[0];
    fixed += 0x7F;
    fixed = fixed & ~0xFF;

    return (double)fixed / 256.0;
}

// calculate a matrix that maps (x1, y1) -> (nx1, ny1) and (x2, y2) -> (nx2, ny2) and invert it
static inline void calculateAdjustMatrix(cairo_matrix_t *matrix, double x1, double y1, double x2, double y2, double nx1, double ny1, double nx2, double ny2)
{
    matrix->xx = (nx2 - nx1) / (x2 - x1);
    matrix->xy = 0.0;
    matrix->yx = 0.0;
    matrix->yy = (ny2 - ny1) / (y2 - y1);
    matrix->x0 = nx1 - matrix->xx * x1;
    matrix->y0 = ny1 - matrix->yy * y1;
}
void adjustPatternMatrixForPixelAlign(cairo_t* cr, cairo_pattern_t* pattern, cairo_matrix_t* patternMatrix, double x, double y, double w, double h)
{
    double x1, y1, x2, y2; // original rectangle
    double nx1, ny1, nx2, ny2; // rounded or inflated rectangle
    cairo_matrix_t adjustMatrix = {1, 0, 0, 1, 0, 0};
    cairo_matrix_t ctm;

    cairo_antialias_t antialias;
    cairo_extend_t extend;
    cairo_filter_t filter;

    cairo_get_matrix(cr, &ctm);

    // don't adjust when rotate or zoom level 1.0
    if (ctm.xy || ctm.yx || (ctm.xx == 1 && ctm.yy == 1))
        return;

    // Validity check - w,h must bigger than 1
    if (w <= 1 || h <= 1)
        return;

    antialias = cairo_get_antialias(cr);
    extend = cairo_pattern_get_extend(pattern);
    filter = cairo_pattern_get_filter(pattern);

    // original rectangle points
    nx1 = x;
    ny1 = y;
    nx2 = x + w;
    ny2 = y + h;

    // transform to destination coordinate space
    cairo_matrix_transform_point(&ctm, &nx1, &ny1);
    cairo_matrix_transform_point(&ctm, &nx2, &ny2);

    // find destination sampling rectangle
    if (antialias == CAIRO_ANTIALIAS_NONE) {
        nx1 = roundDown(nx1);
        ny1 = roundDown(ny1);
        nx2 = roundDown(nx2);
        ny2 = roundDown(ny2);
    } else {
        nx1 = floor(nx1);
        ny1 = floor(ny1);
        nx2 = ceil(nx2);
        ny2 = ceil(ny2);
    }

    if ((ctm.xx > 1 || ctm.yy > 1) && (filter == CAIRO_FILTER_GOOD || filter == CAIRO_FILTER_BILINEAR) && extend != CAIRO_EXTEND_PAD) {
        /* If we can't assume continuity at the marginal area of source pattern, we may
         * get unexpected colors in that area which cannot be seen with zoom level 1.0.
         * So we have to align start and end sampling points to make it look consistent
         * with zoom level 1.0. Any source pixel which is invisible at zoom level 1.0,
         * also should be invisible at other zoom levels.*/
        nx1 += 0.5;
        ny1 += 0.5;
        nx2 -= 0.5;
        ny2 -= 0.5;

        // find sampling points at zoom level 1.0 (mid-point sampling)
        x1 = x + 0.5;
        y1 = y + 0.5;
        x2 = x + w - 0.5;
        y2 = y + h - 0.5;
    } else {
        // original sampling rectangle
        x1 = x;
        y1 = y;
        x2 = x + w;
        y2 = y + h;
    }

    // Validity check - nx2,ny2 must bigger than nx1,ny1
    if (nx1 >= nx2 || ny1 >= ny2)
        return;

    // find corresponding user-space points of the destination rectangle points
    cairo_matrix_invert(&ctm);
    cairo_matrix_transform_point(&ctm, &nx1, &ny1);
    cairo_matrix_transform_point(&ctm, &nx2, &ny2);

    // Validity check - nx2,ny2 must bigger than nx1,ny1
    if (nx1 >= nx2 || ny1 >= ny2)
        return;

    // now we have to find a matrix which maps original points to the grid-points,
    calculateAdjustMatrix(&adjustMatrix, x1, y1, x2, y2, nx1, ny1, nx2, ny2);

    // pattern matrix is a mapping between user to pattern space, so we have to invert it
    cairo_matrix_invert(&adjustMatrix);

    cairo_matrix_multiply(patternMatrix, &adjustMatrix, patternMatrix);
}
#endif

void drawPatternToCairoContext(cairo_t* cr, cairo_surface_t* image, const IntSize& imageSize, const FloatRect& tileRect,
                               const AffineTransform& patternTransform, const FloatPoint& phase, cairo_operator_t op, const FloatRect& destRect)
{
    // Avoid NaN
    if (!isfinite(phase.x()) || !isfinite(phase.y()))
       return;

    cairo_save(cr);

    RefPtr<cairo_surface_t> clippedImageSurface = 0;
    if (tileRect.size() != imageSize) {
        IntRect imageRect = enclosingIntRect(tileRect);
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
        if (cairo_surface_get_type(image) == CAIRO_SURFACE_TYPE_GL)
            clippedImageSurface = adoptRef(cairo_surface_create_similar(image, CAIRO_CONTENT_COLOR_ALPHA, imageRect.width(), imageRect.height()));
        else
            clippedImageSurface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, imageRect.width(), imageRect.height()));
#else
        clippedImageSurface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, imageRect.width(), imageRect.height()));
#endif
        RefPtr<cairo_t> clippedImageContext = adoptRef(cairo_create(clippedImageSurface.get()));
        cairo_set_source_surface(clippedImageContext.get(), image, -tileRect.x(), -tileRect.y());
        cairo_paint(clippedImageContext.get());
        image = clippedImageSurface.get();
    }

#if ENABLE(TIZEN_DRAW_SCALED_PATTERN)
    // Images bigger than this in all directions will be scaled because width or height could be 0 by the ctm.
    const int kImageSizeThreshold = 1;
    // The value of repetitionX * repetitionY smaller than this is considered inefficient source and will not be replaced by scaled pattern.
    // FIXME: Need to figure out the optimum value.
    const float kRepetitionThreshold = 2;

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    int imageWidth, imageHeight;
    if (cairo_surface_get_type(image) == CAIRO_SURFACE_TYPE_GL) {
        imageWidth = cairo_gl_surface_get_width(image);
        imageHeight = cairo_gl_surface_get_height(image);
    } else {
        imageWidth = cairo_image_surface_get_width(image);
        imageHeight = cairo_image_surface_get_height(image);
    }
#else
    int imageWidth = cairo_image_surface_get_width(image);
    int imageHeight = cairo_image_surface_get_height(image);
#endif
    if (!imageWidth || !imageHeight)
        return;

    cairo_matrix_t ctm;
    cairo_get_matrix(cr, &ctm);
    cairo_matrix_t patternMatrix = cairo_matrix_t(patternTransform);
    cairo_matrix_t totalMatrix;
    cairo_matrix_multiply(&totalMatrix, &ctm, &patternMatrix);

    double scaleX = ctm.xx ? ctm.xx : 1;
    double scaleY = ctm.yy ? ctm.yy : 1;
    int destBitmapWidth = ceil(imageWidth * totalMatrix.xx);
    int destBitmapHeight = ceil(imageHeight * totalMatrix.yy);
    float repetitionX = destRect.width() / destBitmapWidth;
    float repetitionY = destRect.height() / destBitmapHeight;

    bool shouldScalePattern = false;
    if (scaleX != 1 && scaleY != 1 && totalMatrix.xx != 1 && totalMatrix.yy != 1 && destBitmapWidth && destBitmapHeight
        && imageWidth > kImageSizeThreshold && imageHeight > kImageSizeThreshold && repetitionX * repetitionY > kRepetitionThreshold)
        shouldScalePattern = true;

    RefPtr<cairo_surface_t> scaledImageSurface = 0;
    if (shouldScalePattern) {
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
        if (cairo_surface_get_type(image) == CAIRO_SURFACE_TYPE_GL)
            scaledImageSurface = adoptRef(cairo_surface_create_similar(image, CAIRO_CONTENT_COLOR_ALPHA, destBitmapWidth, destBitmapHeight));
        else
            scaledImageSurface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, destBitmapWidth, destBitmapHeight));
#else
        scaledImageSurface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, destBitmapWidth, destBitmapHeight));
#endif
        RefPtr<cairo_pattern_t> scaledPattern = adoptRef(cairo_pattern_create_for_surface(image));
        cairo_matrix_t matrix = { 1 / totalMatrix.xx, 0, 0, 1 / totalMatrix.yy, 0, 0 };
        cairo_pattern_set_matrix(scaledPattern.get(), &matrix);
        cairo_pattern_set_extend(scaledPattern.get(), CAIRO_EXTEND_PAD);

        RefPtr<cairo_t> scaledImageContext = adoptRef(cairo_create(scaledImageSurface.get()));
        cairo_set_operator(scaledImageContext.get(), CAIRO_OPERATOR_SOURCE);
        cairo_set_source(scaledImageContext.get(), scaledPattern.get());
        cairo_paint(scaledImageContext.get());
        image = scaledImageSurface.get();
    }
#endif

    cairo_pattern_t* pattern = cairo_pattern_create_for_surface(image);
    cairo_pattern_set_extend(pattern, CAIRO_EXTEND_REPEAT);

#if ENABLE(TIZEN_DRAW_SCALED_PATTERN)
    cairo_matrix_t combined;
    if (shouldScalePattern)
        cairo_matrix_init(&combined, 1 / scaleX, 0, 0, 1 / scaleY, phase.x() + tileRect.x() * patternTransform.a(), phase.y() + tileRect.y() * patternTransform.d());
    else {
        cairo_matrix_t phaseMatrix = {1, 0, 0, 1, phase.x() + tileRect.x() * patternTransform.a(), phase.y() + tileRect.y() * patternTransform.d()};
        cairo_matrix_multiply(&combined, &patternMatrix, &phaseMatrix);
    }
#else
    cairo_matrix_t patternMatrix = cairo_matrix_t(patternTransform);
    cairo_matrix_t phaseMatrix = {1, 0, 0, 1, phase.x() + tileRect.x() * patternTransform.a(), phase.y() + tileRect.y() * patternTransform.d()};
    cairo_matrix_t combined;
    cairo_matrix_multiply(&combined, &patternMatrix, &phaseMatrix);
#endif
    cairo_matrix_invert(&combined);

#if ENABLE(TIZEN_ADJUST_PATTERN_MATRIX)
#if ENABLE(TIZEN_DRAW_SCALED_PATTERN)
    if (!shouldScalePattern)
#endif
    adjustPatternMatrixForPixelAlign(cr, pattern, &combined, destRect.x(), destRect.y(), destRect.width(), destRect.height());
#endif
    cairo_pattern_set_matrix(pattern, &combined);

    cairo_set_operator(cr, op);
    cairo_set_source(cr, pattern);
#if !ENABLE(TIZEN_CAIRO_PATTERN_DESTROY)
    cairo_pattern_destroy(pattern);
#endif
    cairo_rectangle(cr, destRect.x(), destRect.y(), destRect.width(), destRect.height());
    cairo_fill(cr);

    cairo_restore(cr);
#if ENABLE(TIZEN_CAIRO_PATTERN_DESTROY)
    cairo_pattern_destroy(pattern);
#endif
}

PassRefPtr<cairo_surface_t> copyCairoImageSurface(cairo_surface_t* originalSurface)
{
    // Cairo doesn't provide a way to copy a cairo_surface_t.
    // See http://lists.cairographics.org/archives/cairo/2007-June/010877.html
    // Once cairo provides the way, use the function instead of this.
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    RefPtr<cairo_surface_t> newSurface;
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    if (cairo_surface_get_type(originalSurface) == CAIRO_SURFACE_TYPE_GL) {
        int h = cairo_gl_surface_get_height(originalSurface);
        int w = cairo_gl_surface_get_width(originalSurface);
        newSurface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h));
    }
    else
        newSurface = adoptRef(cairo_image_surface_create(cairo_image_surface_get_format(originalSurface),
                                                            cairo_image_surface_get_width(originalSurface),
                                                            cairo_image_surface_get_height(originalSurface)));
#else
    if (cairo_surface_get_type(originalSurface) == CAIRO_SURFACE_TYPE_GL)
        newSurface = adoptRef(cairo_surface_map_to_image(originalSurface, 0));
    else
        newSurface = adoptRef(cairo_image_surface_create(cairo_image_surface_get_format(originalSurface),
                                                             cairo_image_surface_get_width(originalSurface),
                                                             cairo_image_surface_get_height(originalSurface)));
#endif
    RefPtr<cairo_t> cr = adoptRef(cairo_create(newSurface.get()));
    cairo_set_source_surface(cr.get(), originalSurface, 0, 0);
    cairo_set_operator(cr.get(), CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr.get());
    return newSurface.release();
#else
    RefPtr<cairo_surface_t> newSurface = adoptRef(cairo_image_surface_create(cairo_image_surface_get_format(originalSurface), 
                                                                             cairo_image_surface_get_width(originalSurface),
                                                                             cairo_image_surface_get_height(originalSurface)));

    RefPtr<cairo_t> cr = adoptRef(cairo_create(newSurface.get()));
    cairo_set_source_surface(cr.get(), originalSurface, 0, 0);
    cairo_set_operator(cr.get(), CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr.get());
    return newSurface.release();
#endif
}

void copyRectFromCairoSurfaceToContext(cairo_surface_t* from, cairo_t* to, const IntSize& offset, const IntRect& rect)
{
    cairo_set_source_surface(to, from, offset.width(), offset.height());
    cairo_rectangle(to, rect.x(), rect.y(), rect.width(), rect.height());
    cairo_fill(to);
}

void copyRectFromOneSurfaceToAnother(cairo_surface_t* from, cairo_surface_t* to, const IntSize& offset, const IntRect& rect)
{
    RefPtr<cairo_t> context = adoptRef(cairo_create(to));
    copyRectFromCairoSurfaceToContext(from, context.get(), offset, rect);
}

#if ENABLE(TIZEN_CANVAS_TODATAURL_USING_IMAGE_ENCODER)
/* Unpremultiplies data and converts native endian ARGB => RGBA bytes (from cairo unpremultiply_data) */
static void preMultipliedARGBtoRGBACallback(png_structp png, png_row_infop row_info, png_bytep data)
{
    for (unsigned i = 0; i < row_info->rowbytes; i += 4) {
        uint8_t* b = &data[i];
        uint32_t pixel;

        memcpy(&pixel, b, sizeof(uint32_t));
        uint8_t alpha = (pixel & 0xff000000) >> 24;
        if (!alpha)
            b[0] = b[1] = b[2] = b[3] = 0;
        else {
            b[2] = (((pixel & 0xff0000) >> 16) * 255 + alpha / 2) / alpha;
            b[1] = (((pixel & 0x00ff00) >> 8) * 255 + alpha / 2) / alpha;
            b[0] = (((pixel & 0x0000ff) >> 0) * 255 + alpha / 2) / alpha;
            b[3] = alpha;
        }
    }
}

static void writePNGCallback(png_structp png, png_bytep data, png_size_t size)
{
    static_cast<Vector<unsigned char>*>(png_get_io_ptr(png))->append(data, size);
}

bool encodeImageDataToPNG(const ImageData& image, Vector<char>* output)
{
    IntSize imageSize = image.size();
    unsigned char* inputPixels = image.data()->data();

    png_struct* png = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    png_info* info = png_create_info_struct(png);
    if (!png || !info || setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(png ? &png : 0, info ? &info : 0);
        return false;
    }

    png_set_compression_level(png, 3);
    png_set_filter(png, PNG_FILTER_TYPE_BASE, PNG_FILTER_SUB);

    png_set_write_fn(png, output, writePNGCallback, 0);
    png_set_IHDR(png, info, imageSize.width(), imageSize.height(), 8, PNG_COLOR_TYPE_RGB_ALPHA, 0, 0, 0);
    png_write_info(png, info);
    png_set_write_user_transform_fn(png, preMultipliedARGBtoRGBACallback);

    const size_t pixelRowStride = imageSize.width() * 4;
    for (int y = 0; y < imageSize.height(); ++y) {
        png_write_row(png, inputPixels);
        inputPixels += pixelRowStride;
    }

    png_write_end(png, info);
    png_destroy_write_struct(&png, &info);

    return true;
}

struct JPEGOutputBuffer : public jpeg_destination_mgr {
    Vector<unsigned char>* output;
    Vector<unsigned char> buffer;
};

static void jpegInitCallback(j_compress_ptr cinfo)
{
    JPEGOutputBuffer* out = static_cast<JPEGOutputBuffer*>(cinfo->dest);
    out->buffer.resize(8192);
    out->next_output_byte = out->buffer.data();
    out->free_in_buffer = out->buffer.size();
}

static boolean jpegWriteCallback(j_compress_ptr cinfo)
{
    JPEGOutputBuffer* out = static_cast<JPEGOutputBuffer*>(cinfo->dest);
    out->output->append(out->buffer.data(), out->buffer.size());
    out->next_output_byte = out->buffer.data();
    out->free_in_buffer = out->buffer.size();

    return true;
}

static void jpegTerminateCallback(j_compress_ptr cinfo)
{
    JPEGOutputBuffer* out = static_cast<JPEGOutputBuffer*>(cinfo->dest);
    const size_t size = out->buffer.size() - out->free_in_buffer;
    out->output->append(out->buffer.data(), size);
}

static void jpegErrorCallback(j_common_ptr common)
{
    jmp_buf* jumpBufferPtr = static_cast<jmp_buf*>(common->client_data);
    longjmp(*jumpBufferPtr, -1);
}

bool encodeImageDataToJPEG(const ImageData& image, int quality, Vector<unsigned char>* output)
{
    JPEGOutputBuffer destination;
    destination.output = output;
    Vector<JSAMPLE> row;

    jpeg_compress_struct cinfo;
    jpeg_error_mgr error;
    cinfo.err = jpeg_std_error(&error);
    error.error_exit = jpegErrorCallback;
    jmp_buf jumpBuffer;
    cinfo.client_data = &jumpBuffer;

    if (setjmp(jumpBuffer)) {
        jpeg_destroy_compress(&cinfo);
        return false;
    }

    jpeg_create_compress(&cinfo);
    cinfo.dest = &destination;
    cinfo.dest->init_destination = jpegInitCallback;
    cinfo.dest->empty_output_buffer = jpegWriteCallback;
    cinfo.dest->term_destination = jpegTerminateCallback;

    IntSize imageSize = image.size();
    imageSize.clampNegativeToZero();
    cinfo.image_height = imageSize.height();
    cinfo.image_width = imageSize.width();

    cinfo.in_color_space = JCS_RGB;
    cinfo.input_components = 3;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, true);
    jpeg_start_compress(&cinfo, true);

    unsigned char* pixels = image.data()->data();;
    row.resize(cinfo.image_width * cinfo.input_components);
    const unsigned char* pixelEnd = pixels + cinfo.image_width * cinfo.image_height * 4;
    while (pixels < pixelEnd) {
        JSAMPLE* rowdata = row.data();
        for (const unsigned char* rowEnd = pixels + cinfo.image_width * 4; pixels < rowEnd;) {
            *rowdata++ = static_cast<JSAMPLE>(*pixels++ & 0xFF);
            *rowdata++ = static_cast<JSAMPLE>(*pixels++ & 0xFF);
            *rowdata++ = static_cast<JSAMPLE>(*pixels++ & 0xFF);
            ++pixels;
        }
        rowdata = row.data();
        jpeg_write_scanlines(&cinfo, &rowdata, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    return true;
}
#endif

} // namespace WebCore
