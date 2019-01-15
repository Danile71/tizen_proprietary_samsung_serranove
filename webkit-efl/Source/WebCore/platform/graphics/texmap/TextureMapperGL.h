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

#ifndef TextureMapperGL_h
#define TextureMapperGL_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatQuad.h"
#include "IntSize.h"
#include "OpenGLShims.h"
#include "TextureMapper.h"
#include "TransformationMatrix.h"

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "PlatformSurfaceTextureGL.h"
#endif

#if USE(TIZEN_TEXTURE_MAPPER)
// FIXME
#define GL_TEXTURE_2D 0x0DE1
typedef unsigned int GLuint;
#endif

namespace WebCore {

class TextureMapperGLData;
class GraphicsContext;
class TextureMapperShaderProgram;

// An OpenGL-ES2 implementation of TextureMapper.
class TextureMapperGL : public TextureMapper {
public:
    static PassOwnPtr<TextureMapperGL> create() { return adoptPtr(new TextureMapperGL); }
    TextureMapperGL();
    virtual ~TextureMapperGL();

    enum Flag {
        SupportsBlending = 0x01,
        ShouldFlipTexture = 0x02
    };

    typedef int Flags;

    // TextureMapper implementation
    virtual void drawBorder(const Color&, float borderWidth, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix = TransformationMatrix()) OVERRIDE;
    virtual void drawNumber(int number, int pointSize, const FloatPoint&, const TransformationMatrix& modelViewMatrix = TransformationMatrix()) OVERRIDE;
    virtual void drawTexture(const BitmapTexture&, const FloatRect&, const TransformationMatrix&, float opacity, unsigned exposedEdges) OVERRIDE;
    virtual void drawSolidColor(const FloatRect&, const TransformationMatrix&, const Color&) OVERRIDE;
    // reimps from TextureMapper
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    virtual void drawTexture(uint32_t texture, Flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const IntSize& platformSurfaceSize, bool premultipliedAlpha, unsigned exposedEdges = AllEdges);
#else
    virtual void drawTexture(uint32_t texture, Flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, unsigned exposedEdges = AllEdges);
#endif

#if defined(GL_ARB_texture_rectangle)
    virtual void drawTextureRectangleARB(uint32_t texture, Flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture);
#endif

    virtual void bindSurface(BitmapTexture* surface) OVERRIDE;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    virtual void unbindSurface() OVERRIDE;
#endif
    virtual void beginClip(const TransformationMatrix&, const FloatRect&) OVERRIDE;
    virtual void beginPainting(PaintFlags = 0) OVERRIDE;
    virtual void endPainting() OVERRIDE;
    virtual void endClip() OVERRIDE;
#if ENABLE(TIZEN_OVERLAP_REGION_PAINT_OPTIMIZATION)
    virtual IntRect clipBounds() OVERRIDE;
#endif
    virtual IntSize maxTextureSize() const OVERRIDE { return IntSize(2000, 2000); }
    virtual PassRefPtr<BitmapTexture> createTexture() OVERRIDE;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    virtual PassRefPtr<PlatformSurfaceTexture> createPlatformSurfaceTexture(int, IntSize, bool);
#endif
    virtual GraphicsContext* graphicsContext() OVERRIDE { return m_context; }
    virtual void setGraphicsContext(GraphicsContext* context) OVERRIDE { m_context = context; }

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    void setAngle(int angle);
    virtual void setBindSurface(bool);
#endif

#if ENABLE(CSS_FILTERS)
    void drawFiltered(const BitmapTexture& sourceTexture, const BitmapTexture& contentTexture, const FilterOperation&, int pass);
#endif

    void setEnableEdgeDistanceAntialiasing(bool enabled) { m_enableEdgeDistanceAntialiasing = enabled; }

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    virtual void setDirectRendering(bool);
#endif

private:

    struct ClipState {
        IntRect scissorBox;
        int stencilIndex;
        ClipState(const IntRect& scissors = IntRect(), int stencil = 1)
            : scissorBox(scissors)
            , stencilIndex(stencil)
        { }
    };

    class ClipStack {
    public:
        void push();
        void pop();
        void apply();
        inline ClipState& current() { return clipState; }
        void init(const IntRect&);

    private:
        ClipState clipState;
        Vector<ClipState> clipStack;
    };

    struct DrawQuad {
        DrawQuad(const FloatRect& originalTargetRect, const FloatQuad& targetRectMappedToUnitSquare = FloatRect(FloatPoint(), FloatSize(1, 1)))
            : originalTargetRect(originalTargetRect)
            , targetRectMappedToUnitSquare(targetRectMappedToUnitSquare)
        {
        }

        FloatRect originalTargetRect;
        FloatQuad targetRectMappedToUnitSquare;
    };

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    bool drawTextureWithAntialiasing(uint32_t texture, Flags, const FloatRect& originalTargetRect, const TransformationMatrix& modelViewMatrix, float opacity, const IntSize& textureSize, const IntSize& platformSurfaceSize, bool premultipliedAlpha, unsigned exposedEdges);
    void drawTexturedQuadWithProgram(TextureMapperShaderProgram*, uint32_t texture, Flags, const DrawQuad&, const TransformationMatrix& modelViewMatrix, float opacity, const IntSize& textureSize, const IntSize& platformSurfaceSize, bool premultipliedAlpha);
    void drawQuad(const DrawQuad&, const TransformationMatrix& modelViewMatrix, TextureMapperShaderProgram*, GLenum drawingMode, bool needsBlending, bool premultipliedAlpha);
    PassRefPtr<TextureMapperShaderProgram> getShaderProgram();
#else
    bool drawTextureWithAntialiasing(uint32_t texture, Flags, const FloatRect& originalTargetRect, const TransformationMatrix& modelViewMatrix, float opacity, unsigned exposedEdges);
    void drawTexturedQuadWithProgram(TextureMapperShaderProgram*, uint32_t texture, Flags, const DrawQuad&, const TransformationMatrix& modelViewMatrix, float opacity);
    void drawQuad(const DrawQuad&, const TransformationMatrix& modelViewMatrix, TextureMapperShaderProgram*, GLenum drawingMode, bool needsBlending);
#endif

    bool beginScissorClip(const TransformationMatrix&, const FloatRect&);
    void bindDefaultSurface();
    ClipStack& clipStack();
    inline TextureMapperGLData& data() { return *m_data; }
    TextureMapperGLData* m_data;
    GraphicsContext* m_context;
    ClipStack m_clipStack;
    bool m_enableEdgeDistanceAntialiasing;

    friend class BitmapTextureGL;
};

class BitmapTextureGL : public BitmapTexture {
public:
    virtual IntSize size() const;
    virtual bool isValid() const;
    virtual bool canReuseWith(const IntSize& contentsSize, Flags = 0);
    virtual void didReset();
    void bind();
    void initializeStencil();
    ~BitmapTextureGL();
    virtual uint32_t id() const { return m_id; }
    uint32_t textureTarget() const { return GL_TEXTURE_2D; }
    IntSize textureSize() const { return m_textureSize; }
    void setTextureMapper(TextureMapperGL* texmap) { m_textureMapper = texmap; }
    void updateContents(Image*, const IntRect&, const IntPoint&);
    virtual void updateContents(const void*, const IntRect& target, const IntPoint& sourceOffset, int bytesPerLine);
    virtual bool isBackedByOpenGL() const { return true; }

#if ENABLE(CSS_FILTERS)
    virtual PassRefPtr<BitmapTexture> applyFilters(const BitmapTexture& contentTexture, const FilterOperations&);
#endif

private:
    GLuint m_id;
    IntSize m_textureSize;
    IntRect m_dirtyRect;
    GLuint m_fbo;
    GLuint m_rbo;
    bool m_shouldClear;
    TextureMapperGL* m_textureMapper;
    TextureMapperGL::ClipStack m_clipStack;
    BitmapTextureGL()
        : m_id(0)
        , m_fbo(0)
        , m_rbo(0)
        , m_shouldClear(true)
        , m_textureMapper(0)
    {
    }

    void clearIfNeeded();
    void createFboIfNeeded();

    friend class TextureMapperGL;
};

typedef uint64_t ImageUID;
ImageUID uidForImage(Image*);
BitmapTextureGL* toBitmapTextureGL(BitmapTexture*);

}
#endif

#endif
