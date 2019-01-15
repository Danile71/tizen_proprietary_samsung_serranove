/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef TextureMapper_h
#define TextureMapper_h

#if PLATFORM(QT)
#include <qglobal.h>
#endif

#if USE(ACCELERATED_COMPOSITING)

#if PLATFORM(QT)
#include <qglobal.h>

#if defined(QT_OPENGL_LIB) || HAVE(QT5)
    #if defined(QT_OPENGL_ES_2) && !defined(TEXMAP_OPENGL_ES_2)
        #define TEXMAP_OPENGL_ES_2
    #endif
#endif
#endif

#if USE(TIZEN_TEXTURE_MAPPER)
    #define TEXMAP_OPENGL_ES_2
#endif

#include "FilterOperations.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "IntSize.h"
#include "TextureMapperPlatformLayer.h"
#include "TransformationMatrix.h"
#include <wtf/UnusedParam.h>
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
#include <wtf/HashMap.h>
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "SharedPlatformSurfaceTizen.h"
#endif
/*
    TextureMapper is a mechanism that enables hardware acceleration of CSS animations (accelerated compositing) without
    a need for a platform specific scene-graph library like CoreAnimations or QGraphicsView.
*/

namespace WebCore {

class TextureMapper;

// A 2D texture that can be the target of software or GL rendering.
class BitmapTexture  : public RefCounted<BitmapTexture> {
public:
    enum Flag {
        SupportsAlpha = 0x01
    };

    typedef unsigned Flags;

    BitmapTexture()
        : m_flags(0)
    {
    }

    virtual ~BitmapTexture() { }
    virtual bool isBackedByOpenGL() const { return false; }

    virtual IntSize size() const = 0;
    virtual void updateContents(Image*, const IntRect&, const IntPoint& offset) = 0;
    virtual void updateContents(const void*, const IntRect& target, const IntPoint& offset, int bytesPerLine) = 0;
    virtual bool isValid() const = 0;
    inline Flags flags() const { return m_flags; }

    virtual int bpp() const { return 32; }
    virtual bool canReuseWith(const IntSize& contentsSize, Flags flags = 0) { return false; }
    void reset(const IntSize& size, Flags flags = 0)
    {
        m_flags = flags;
        m_contentSize = size;
        didReset();
    }
    virtual void didReset() { }

    inline IntSize contentSize() const { return m_contentSize; }
    inline int numberOfBytes() const { return size().width() * size().height() * bpp() >> 3; }
    inline bool isOpaque() const { return !(m_flags & SupportsAlpha); }

#if ENABLE(CSS_FILTERS)
    virtual PassRefPtr<BitmapTexture> applyFilters(const BitmapTexture& contentTexture, const FilterOperations&) { return this; }
#endif

protected:
    IntSize m_contentSize;

private:
    Flags m_flags;
};

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
// A 2D Texture made from sharedPlatformSurface
class PlatformSurfaceTexture : public BitmapTexture {
public:
    enum PixelFormat { BGRAFormat, RGBAFormat, BGRFormat, RGBFormat };
    enum Flag {
        SupportsAlpha = 0x01
    };

    PlatformSurfaceTexture() { }
    virtual ~PlatformSurfaceTexture() { }

    virtual uint32_t id() const = 0;
    virtual int platformSurfaceId() const = 0;
    virtual void setUsed(bool used) = 0;
    virtual bool used() = 0;
    virtual IntSize size() const = 0;
    virtual void updateTexture() = 0;

    virtual void updateContents(Image*, const IntRect&, const IntPoint& offset) { };
    virtual void updateContents(const void*, const IntRect& target, const IntPoint& offset, int bytesPerLine) { };
    virtual bool isValid() const { return true; }
    virtual void copyPlatformSurfaceToTextureGL(const IntRect&, SharedPlatformSurfaceSimpleTizen*) = 0;
    virtual SharedPlatformSurfaceSimpleTizen* sharedPlatformSurfaceSimple() = 0;
};
#endif

// A "context" class used to encapsulate accelerated texture mapping functions: i.e. drawing a texture
// onto the screen or into another texture with a specified transform, opacity and mask.
class TextureMapper {
    friend class BitmapTexture;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    friend class PlatformSurfaceTexture;
#endif

public:
    enum AccelerationMode { SoftwareMode, OpenGLMode };
    enum PaintFlag {
        PaintingMirrored = 1 << 0,
    };
    typedef unsigned PaintFlags;

    static PassOwnPtr<TextureMapper> create(AccelerationMode newMode = SoftwareMode);
    virtual ~TextureMapper() { }

    enum ExposedEdges {
        NoEdges = 0,
        LeftEdge = 1 << 0,
        RightEdge = 1 << 1,
        TopEdge = 1 << 2,
        BottomEdge = 1 << 3,
        AllEdges = LeftEdge | RightEdge | TopEdge | BottomEdge,
    };

    virtual void drawBorder(const Color&, float borderWidth, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix = TransformationMatrix()) = 0;
    virtual void drawNumber(int number,  int pointSize, const FloatPoint&, const TransformationMatrix&) = 0;
    virtual void drawTexture(const BitmapTexture&, const FloatRect& target, const TransformationMatrix& modelViewMatrix = TransformationMatrix(), float opacity = 1.0f, unsigned exposedEdges = AllEdges) = 0;
    virtual void drawSolidColor(const FloatRect&, const TransformationMatrix&, const Color&) = 0;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    virtual void drawTexture(uint32_t texture, int flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const IntSize& platformSurfaceSize, bool premultipliedAlpha = true, unsigned exposedEdges = AllEdges) { }
#endif

    // makes a surface the target for the following drawTexture calls.
    virtual void bindSurface(BitmapTexture* surface) = 0;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    virtual void unbindSurface() {}
#endif
    virtual void setGraphicsContext(GraphicsContext* context) { m_context = context; }
    virtual GraphicsContext* graphicsContext() { return m_context; }
    virtual void beginClip(const TransformationMatrix&, const FloatRect&) = 0;
    virtual void endClip() = 0;
#if ENABLE(TIZEN_OVERLAP_REGION_PAINT_OPTIMIZATION)
    virtual IntRect clipBounds() = 0;
#endif
    virtual PassRefPtr<BitmapTexture> createTexture() = 0;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    virtual PassRefPtr<PlatformSurfaceTexture> createPlatformSurfaceTexture(int, IntSize, bool useLinearFilter = true) { return 0; }
#endif

    void setImageInterpolationQuality(InterpolationQuality quality) { m_interpolationQuality = quality; }
    void setTextDrawingMode(TextDrawingModeFlags mode) { m_textDrawingMode = mode; }

    InterpolationQuality imageInterpolationQuality() const { return m_interpolationQuality; }
    TextDrawingModeFlags textDrawingMode() const { return m_textDrawingMode; }
    AccelerationMode accelerationMode() const { return m_accelerationMode; }

    virtual void beginPainting(PaintFlags flags = 0) { }
    virtual void endPainting() { }

    virtual void setMaskMode(bool m) { m_isMaskMode = m; }

    virtual IntSize maxTextureSize() const { return IntSize(INT_MAX, INT_MAX); }

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    void setFullScreenForVideoMode(bool fullScreenForVideoMode) { m_fullScreenForVideoMode = fullScreenForVideoMode; }
    bool fullScreenForVideoMode() { return m_fullScreenForVideoMode; }
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    void setFullScreenCompositingEnabled(bool compositingEnabled) { m_fullScreenCompositingEnabled = compositingEnabled; }
    bool fullScreenCompositingEnabled() { return m_fullScreenCompositingEnabled; }
#endif
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setLowScaleLayerCompositingEnabled(bool compositingEnabled) { m_lowScaleLayerCompositingEnabled = compositingEnabled; }
    bool lowScaleLayerCompositingEnabled() { return m_lowScaleLayerCompositingEnabled; }
    void setPaintOnlyLowScaleLayer(bool b) { m_shouldPaintOnlyLowScaleLayer= b; }
    bool paintOnlyLowScaleLayer() { return m_shouldPaintOnlyLowScaleLayer; }
#endif
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    virtual void setDirectRendering(bool b) { }
#endif

    // A surface is released implicitly when dereferenced.
    virtual PassRefPtr<BitmapTexture> acquireTextureFromPool(const IntSize&);

    virtual void setBindSurface(bool) {}

protected:
    TextureMapper(AccelerationMode accelerationMode)
        : m_interpolationQuality(InterpolationDefault)
        , m_textDrawingMode(TextModeFill)
        , m_accelerationMode(accelerationMode)
        , m_isMaskMode(false)
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
        , m_fullScreenForVideoMode(false)
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
        , m_fullScreenCompositingEnabled(true)
#endif
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        , m_lowScaleLayerCompositingEnabled(true)
        , m_shouldPaintOnlyLowScaleLayer(false)
#endif
    {}

    GraphicsContext* m_context;

    bool isInMaskMode() const { return m_isMaskMode; }

private:
#if USE(TEXTURE_MAPPER_GL)
    static PassOwnPtr<TextureMapper> platformCreateAccelerated();
#else
    static PassOwnPtr<TextureMapper> platformCreateAccelerated()
    {
        return PassOwnPtr<TextureMapper>();
    }
#endif
    InterpolationQuality m_interpolationQuality;
    TextDrawingModeFlags m_textDrawingMode;
    Vector<RefPtr<BitmapTexture> > m_texturePool;
    AccelerationMode m_accelerationMode;
    bool m_isMaskMode;
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    bool m_fullScreenForVideoMode;
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    bool m_fullScreenCompositingEnabled;
#endif
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    bool m_lowScaleLayerCompositingEnabled;
    bool m_shouldPaintOnlyLowScaleLayer;
#endif
};

}

#endif

#endif
