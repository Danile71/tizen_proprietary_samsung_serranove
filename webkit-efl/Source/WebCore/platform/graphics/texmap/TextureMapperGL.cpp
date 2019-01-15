/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2012 Igalia S.L.

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
#include "TextureMapperGL.h"

#include "GraphicsContext.h"
#include "Image.h"
#include "NotImplemented.h"
#include "TextureMapperShaderManager.h"
#include "Timer.h"
#include <wtf/HashMap.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#if USE(GRAPHICS_SURFACE)
#include "GraphicsSurface.h"
#endif

#if PLATFORM(QT)
#include "NativeImageQt.h"
#if HAVE(QT5)
#include <QOpenGLContext>
#else
#include <QGLContext>
#endif // QT_VERSION
#elif OS(WINDOWS)
#include <windows.h>
#elif OS(MAC_OS_X)
#include <AGL/agl.h>
#elif defined(XP_UNIX) && !USE(TIZEN_TEXTURE_MAPPER)
#include <GL/glx.h>
#endif

#if USE(CAIRO)
#include "CairoUtilities.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <wtf/text/CString.h>
#endif

#define GL_CMD(...) do { __VA_ARGS__; ASSERT_ARG(__VA_ARGS__, !glGetError()); } while (0)

#if USE(TIZEN_TEXTURE_MAPPER)
#define GL_BGRA 0x80E1
#endif

namespace WebCore {
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
static int gAngle = 0;
static bool gDirectRendering = false;
#if ENABLE(TIZEN_CORRECT_CLIPPING_WITH_INTERMEDIATE_SURFACE)
static bool gBindSurface = false;
#endif
#endif

struct TextureMapperGLData {
    struct SharedGLData : public RefCounted<SharedGLData> {
#if PLATFORM(QT) && QT_VERSION >= 0x050000
        typedef QOpenGLContext* GLContext;
        static GLContext getCurrentGLContext()
        {
            return QOpenGLContext::currentContext();
        }
#elif OS(WINDOWS)
        typedef HGLRC GLContext;
        static GLContext getCurrentGLContext()
        {
            return wglGetCurrentContext();
        }
#elif OS(MAC_OS_X)
        typedef AGLContext GLContext;
        static GLContext getCurrentGLContext()
        {
            return aglGetCurrentContext();
        }
#elif defined(XP_UNIX) && !USE(TIZEN_TEXTURE_MAPPER)
        typedef GLXContext GLContext;
        static GLContext getCurrentGLContext()
        {
            return glXGetCurrentContext();
        }
#else
        // Default implementation for unknown opengl.
        // Returns always increasing number and disables GL context data sharing.
        typedef unsigned int GLContext;
        static GLContext getCurrentGLContext()
        {
            static GLContext dummyContextCounter = 0;
            return ++dummyContextCounter;
        }

#endif

        typedef HashMap<GLContext, SharedGLData*> GLContextDataMap;
        static GLContextDataMap& glContextDataMap()
        {
            static GLContextDataMap map;
            return map;
        }

        static PassRefPtr<SharedGLData> currentSharedGLData()
        {
            GLContext currentGLConext = getCurrentGLContext();
            GLContextDataMap::iterator it = glContextDataMap().find(currentGLConext);
            if (it != glContextDataMap().end())
                return it->second;

            return adoptRef(new SharedGLData(getCurrentGLContext()));
        }

        TextureMapperShaderManager textureMapperShaderManager;

        SharedGLData(GLContext glContext)
        {
            glContextDataMap().add(glContext, this);
        }

        ~SharedGLData()
        {
            GLContextDataMap::const_iterator end = glContextDataMap().end();
            GLContextDataMap::iterator it;
            for (it = glContextDataMap().begin(); it != end; ++it) {
                if (it->second == this)
                    break;
            }

            ASSERT(it != end);
            glContextDataMap().remove(it);
        }
    };

    SharedGLData& sharedGLData() const
    {
        return *(m_sharedGLData.get());
    }

    void initializeStencil();

    TextureMapperGLData()
        : PaintFlags(0)
        , previousProgram(0)
        , targetFrameBuffer(0)
        , didModifyStencil(false)
        , previousScissorState(0)
        , previousDepthState(0)
        , m_sharedGLData(TextureMapperGLData::SharedGLData::currentSharedGLData())
    { }

    TransformationMatrix projectionMatrix;
    TextureMapper::PaintFlags PaintFlags;
    GLint previousProgram;
    GLint targetFrameBuffer;
    bool didModifyStencil;
    GLint previousScissorState;
    GLint previousDepthState;
    GLint viewport[4];
    GLint previousScissor[4];
    RefPtr<SharedGLData> m_sharedGLData;
    RefPtr<BitmapTexture> currentSurface;
};

void TextureMapperGL::ClipStack::init(const IntRect& rect)
{
    clipStack.clear();
    clipState = TextureMapperGL::ClipState(rect);
}

void TextureMapperGL::ClipStack::push()
{
    clipStack.append(clipState);
}

void TextureMapperGL::ClipStack::pop()
{
    if (clipStack.isEmpty())
        return;
    clipState = clipStack.last();
    clipStack.removeLast();
}

static void scissorClip(const IntRect& rect)
{
    if (rect.isEmpty())
        return;

    GLint viewport[4];
    GL_CMD(glGetIntegerv(GL_VIEWPORT, viewport));
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (gDirectRendering) {
        if (gAngle == 90 || gAngle == 270) {
#if ENABLE(TIZEN_CORRECT_CLIPPING_WITH_INTERMEDIATE_SURFACE)
            if (gBindSurface)
                GL_CMD(glScissor(rect.x(), rect.y(), rect.height(), rect.width()));
            else
#endif
                GL_CMD(glScissor(rect.x(), rect.y(), rect.width(), rect.height()));
        }
        else if (gAngle == 180)
            GL_CMD(glScissor(viewport[2] - rect.maxX(), rect.y(), rect.width(), rect.height()));
        else {
#if ENABLE(TIZEN_CORRECT_CLIPPING_WITH_INTERMEDIATE_SURFACE)
            if (gBindSurface)
                GL_CMD(glScissor(rect.x(), rect.y(), rect.width(), rect.height()));
            else
#endif
                GL_CMD(glScissor(rect.x(), viewport[3] - rect.maxY(), rect.width(), rect.height()));
        }
    } else
#endif
    GL_CMD(glScissor(rect.x(), viewport[3] - rect.maxY(), rect.width(), rect.height()));
}

void TextureMapperGL::ClipStack::apply()
{
    scissorClip(clipState.scissorBox);
    GL_CMD(glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP));
    GL_CMD(glStencilFunc(GL_EQUAL, clipState.stencilIndex - 1, clipState.stencilIndex - 1));
    if (clipState.stencilIndex == 1)
        GL_CMD(glDisable(GL_STENCIL_TEST));
    else
        GL_CMD(glEnable(GL_STENCIL_TEST));
}


void TextureMapperGLData::initializeStencil()
{
    if (currentSurface) {
        static_cast<BitmapTextureGL*>(currentSurface.get())->initializeStencil();
        return;
    }

    if (didModifyStencil)
        return;

    GL_CMD(glClearStencil(0));
    GL_CMD(glClear(GL_STENCIL_BUFFER_BIT));
    didModifyStencil = true;
}

BitmapTextureGL* toBitmapTextureGL(BitmapTexture* texture)
{
    if (!texture || !texture->isBackedByOpenGL())
        return 0;

    return static_cast<BitmapTextureGL*>(texture);
}

TextureMapperGL::TextureMapperGL()
    : TextureMapper(OpenGLMode)
    , m_data(new TextureMapperGLData)
    , m_context(0)
    , m_enableEdgeDistanceAntialiasing(false)
{
}

TextureMapperGL::ClipStack& TextureMapperGL::clipStack()
{
    return data().currentSurface ? toBitmapTextureGL(data().currentSurface.get())->m_clipStack : m_clipStack;
}

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
void TextureMapperGL::setAngle(int angle)
{
    gAngle = angle;
}

void TextureMapperGL::setBindSurface(bool bBind)
{
#if ENABLE(TIZEN_CORRECT_CLIPPING_WITH_INTERMEDIATE_SURFACE)
    gBindSurface = bBind;
#endif
}
#endif

void TextureMapperGL::beginPainting(PaintFlags flags)
{
    // Make sure that no GL error code stays from previous operations.
    glGetError();

    if (!initializeOpenGLShims())
        return;

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (gDirectRendering) {
        data().didModifyStencil = false;
        GL_CMD(glGetIntegerv(GL_VIEWPORT, data().viewport));
        m_clipStack.init(IntRect(0, 0, data().viewport[2], data().viewport[3]));
        GL_CMD(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &data().targetFrameBuffer));
    } else {
#endif
    GL_CMD(glGetIntegerv(GL_CURRENT_PROGRAM, &data().previousProgram));
    data().previousScissorState = glIsEnabled(GL_SCISSOR_TEST);
    data().previousDepthState = glIsEnabled(GL_DEPTH_TEST);
#if PLATFORM(QT)
    if (m_context) {
        QPainter* painter = m_context->platformContext();
        painter->save();
        painter->beginNativePainting();
    }
#endif
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    data().didModifyStencil = false;
    GL_CMD(glDepthMask(0));
    GL_CMD(glGetIntegerv(GL_VIEWPORT, data().viewport));
    GL_CMD(glGetIntegerv(GL_SCISSOR_BOX, data().previousScissor));
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &data().targetFrameBuffer);
    m_clipStack.init(IntRect(0, 0, data().viewport[2], data().viewport[3]));
    GL_CMD(glGetIntegerv(GL_FRAMEBUFFER_BINDING, &data().targetFrameBuffer));
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    }
#endif

    data().PaintFlags = flags;
    bindSurface(0);
}

void TextureMapperGL::endPainting()
{
    if (data().didModifyStencil) {
        glClearStencil(1);
        glClear(GL_STENCIL_BUFFER_BIT);
    }

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (gDirectRendering)
        return;
#endif

    glUseProgram(data().previousProgram);

    glScissor(data().previousScissor[0], data().previousScissor[1], data().previousScissor[2], data().previousScissor[3]);
    if (data().previousScissorState)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);

    if (data().previousDepthState)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);

#if PLATFORM(QT)
    if (!m_context)
        return;
    QPainter* painter = m_context->platformContext();
    painter->endNativePainting();
    painter->restore();
#endif
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void TextureMapperGL::drawQuad(const DrawQuad& quadToDraw, const TransformationMatrix& modelViewMatrix, TextureMapperShaderProgram* shaderProgram, GLenum drawingMode, bool needsBlending, bool premultipliedAlpha)
#else
void TextureMapperGL::drawQuad(const DrawQuad& quadToDraw, const TransformationMatrix& modelViewMatrix, TextureMapperShaderProgram* shaderProgram, GLenum drawingMode, bool needsBlending)
#endif
{
    GL_CMD(glEnableVertexAttribArray(shaderProgram->vertexAttrib()));
    GL_CMD(glBindBuffer(GL_ARRAY_BUFFER, 0));

    const GLfloat quad[] = {
        quadToDraw.targetRectMappedToUnitSquare.p1().x(), quadToDraw.targetRectMappedToUnitSquare.p1().y(),
        quadToDraw.targetRectMappedToUnitSquare.p2().x(), quadToDraw.targetRectMappedToUnitSquare.p2().y(),
        quadToDraw.targetRectMappedToUnitSquare.p3().x(), quadToDraw.targetRectMappedToUnitSquare.p3().y(),
        quadToDraw.targetRectMappedToUnitSquare.p4().x(), quadToDraw.targetRectMappedToUnitSquare.p4().y()
    };
    GL_CMD(glVertexAttribPointer(shaderProgram->vertexAttrib(), 2, GL_FLOAT, GL_FALSE, 0, quad));

    TransformationMatrix matrix = TransformationMatrix(data().projectionMatrix).multiply(modelViewMatrix).multiply(TransformationMatrix(
            quadToDraw.originalTargetRect.width(), 0, 0, 0,
            0, quadToDraw.originalTargetRect.height(), 0, 0,
            0, 0, 1, 0,
            quadToDraw.originalTargetRect.x(), quadToDraw.originalTargetRect.y(), 0, 1));
    const GLfloat m4[] = {
        static_cast<const GLfloat>(matrix.m11()), static_cast<const GLfloat>(matrix.m12()), static_cast<const GLfloat>(matrix.m13()), static_cast<const GLfloat>(matrix.m14()),
        static_cast<const GLfloat>(matrix.m21()), static_cast<const GLfloat>(matrix.m22()), static_cast<const GLfloat>(matrix.m23()), static_cast<const GLfloat>(matrix.m24()),
        static_cast<const GLfloat>(matrix.m31()), static_cast<const GLfloat>(matrix.m32()), static_cast<const GLfloat>(matrix.m33()), static_cast<const GLfloat>(matrix.m34()),
        static_cast<const GLfloat>(matrix.m41()), static_cast<const GLfloat>(matrix.m42()), static_cast<const GLfloat>(matrix.m43()), static_cast<const GLfloat>(matrix.m44())
    };
    GL_CMD(glUniformMatrix4fv(shaderProgram->matrixLocation(), 1, GL_FALSE, m4));

    if (isInMaskMode()) {
        GL_CMD(glBlendFunc(GL_ZERO, GL_SRC_ALPHA));
        GL_CMD(glEnable(GL_BLEND));
    } else {
        if (needsBlending) {
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
            if (premultipliedAlpha)
                GL_CMD(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
            else
                GL_CMD(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
#else
            GL_CMD(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
#endif
            GL_CMD(glEnable(GL_BLEND));
        } else
            GL_CMD(glDisable(GL_BLEND));
    }

    GL_CMD(glDrawArrays(drawingMode, 0, 4));
    GL_CMD(glDisableVertexAttribArray(shaderProgram->vertexAttrib()));

    GL_CMD(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
    GL_CMD(glEnable(GL_BLEND));
}

void TextureMapperGL::drawBorder(const Color& color, float width, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix)
{
    if (clipStack().current().scissorBox.isEmpty())
        return;

    RefPtr<TextureMapperShaderProgramSolidColor> program = data().sharedGLData().textureMapperShaderManager.solidColorProgram();
    GL_CMD(glUseProgram(program->id()));

    float alpha = color.alpha() / 255.0;
    GL_CMD(glUniform4f(program->colorLocation(),
                       (color.red() / 255.0) * alpha,
                       (color.green() / 255.0) * alpha,
                       (color.blue() / 255.0) * alpha,
                       alpha));
    GL_CMD(glLineWidth(width));

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
	drawQuad(targetRect, modelViewMatrix, program.get(), GL_LINE_LOOP, color.hasAlpha(), true);
#else
    drawQuad(targetRect, modelViewMatrix, program.get(), GL_LINE_LOOP, color.hasAlpha());
#endif
}

// FIXME: drawNumber() should save a number texture-atlas and re-use whenever possible.
void TextureMapperGL::drawNumber(int number,int pointSize, const FloatPoint& targetPoint, const TransformationMatrix& modelViewMatrix)
{
#if PLATFORM(QT)
    QString counterString = QString::number(number);

    QFont font(QString::fromLatin1("Monospace"), pointSize, QFont::Bold);
    font.setStyleHint(QFont::TypeWriter);

    QFontMetrics fontMetrics(font);
    int width = fontMetrics.width(counterString) + 4;
    int height = fontMetrics.height();

    IntSize size(width, height);
    IntRect sourceRect(IntPoint::zero(), size);
    IntRect targetRect(roundedIntPoint(targetPoint), size);

    QImage image(size, NativeImageQt::defaultFormatForAlphaEnabledImages());
    QPainter painter(&image);
    painter.fillRect(sourceRect, Qt::blue); // Since we won't swap R+B for speed, this will paint red.
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(2, height * 0.85, counterString);

    RefPtr<BitmapTexture> texture = acquireTextureFromPool(size);
    const uchar* bits = image.bits();
    texture->updateContents(bits, sourceRect, IntPoint::zero(), image.bytesPerLine());
    drawTexture(*texture, targetRect, modelViewMatrix, 1.0f, AllEdges);
#elif USE(CAIRO)
    CString counterString = String::number(number).ascii();
    // cairo_text_extents() requires a cairo_t, so dimensions need to be guesstimated.
    int width = counterString.length() * pointSize * 1.2;
    int height = pointSize * 1.5;

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
    cairo_t* cr = cairo_create(surface);
    cairo_surface_destroy(surface);

    cairo_set_source_rgb(cr, 1, 0, 0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    cairo_select_font_face(cr, "Monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, pointSize);
    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_move_to(cr, 2, pointSize);
    cairo_show_text(cr, counterString.data());

    IntSize size(width, height);
    IntRect sourceRect(IntPoint::zero(), size);
    IntRect targetRect(roundedIntPoint(targetPoint), size);

    RefPtr<BitmapTexture> texture = acquireTextureFromPool(size);
    const unsigned char* bits = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    texture->updateContents(bits, sourceRect, IntPoint::zero(), stride);
    drawTexture(*texture, targetRect, modelViewMatrix, 1.0f, 0);

    cairo_destroy(cr);

#else
    notImplemented();
#endif
}

void TextureMapperGL::drawTexture(const BitmapTexture& texture, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity, unsigned exposedEdges)
{
    if (!texture.isValid())
        return;

    if (clipStack().current().scissorBox.isEmpty())
        return;

    const BitmapTextureGL& textureGL = static_cast<const BitmapTextureGL&>(texture);
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
	drawTexture(textureGL.id(), textureGL.isOpaque() ? 0 : SupportsBlending, textureGL.size(), targetRect, matrix, opacity, textureGL.size(), true, exposedEdges);
#else
    drawTexture(textureGL.id(), textureGL.isOpaque() ? 0 : SupportsBlending, textureGL.size(), targetRect, matrix, opacity, exposedEdges);
#endif
}

void TextureMapperGL::drawSolidColor(const FloatRect& rect, const TransformationMatrix& matrix, const Color& color)
{
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    if (clipStack().current().scissorBox.isEmpty())
        return;
#endif
    RefPtr<TextureMapperShaderProgramSolidColor> program = data().sharedGLData().textureMapperShaderManager.solidColorProgram();
    GL_CMD(glUseProgram(program->id()));

    float r, g, b, a;
    color.getRGBA(r, g, b, a);
    GL_CMD(glUniform4f(program->colorLocation(), r, g, b, a));

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    drawQuad(rect, matrix, program.get(), GL_TRIANGLE_FAN, a < 1, false);
#else
    drawQuad(rect, matrix, program.get(), GL_TRIANGLE_FAN, a < 1);
#endif
}

#if defined(GL_ARB_texture_rectangle)
void TextureMapperGL::drawTextureRectangleARB(uint32_t texture, Flags flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const BitmapTexture* maskTexture)
{
    RefPtr<TextureMapperShaderProgram> program;
    if (maskTexture)
        program = data().sharedGLData().textureMapperShaderManager.getShaderProgram(TextureMapperShaderManager::RectOpacityAndMask);
    else
        program = data().sharedGLData().textureMapperShaderManager.getShaderProgram(TextureMapperShaderManager::RectSimple);
    GL_CMD(glUseProgram(program->id()));

    GL_CMD(glEnableVertexAttribArray(program->vertexAttrib()));
    GL_CMD(glActiveTexture(GL_TEXTURE0));
    GL_CMD(glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture));
    GL_CMD(glUniform1i(program->sourceTextureLocation(), 0));

    GL_CMD(glUniform1f(program->flipLocation(), !!(flags & ShouldFlipTexture)));
    GL_CMD(glUniform2f(program->textureSizeLocation(), textureSize.width(), textureSize.height()));

    if (TextureMapperShaderProgram::isValidUniformLocation(program->opacityLocation()))
        GL_CMD(glUniform1f(program->opacityLocation(), opacity));

    if (maskTexture && maskTexture->isValid() && TextureMapperShaderProgram::isValidUniformLocation(program->maskTextureLocation())) {
        const BitmapTextureGL* maskTextureGL = static_cast<const BitmapTextureGL*>(maskTexture);
        GL_CMD(glActiveTexture(GL_TEXTURE1));
        GL_CMD(glBindTexture(GL_TEXTURE_2D, maskTextureGL->id()));
        GL_CMD(glUniform1i(program->maskTextureLocation(), 1));
        GL_CMD(glActiveTexture(GL_TEXTURE0));
    }

    bool needsBlending = (flags & SupportsBlending) || opacity < 0.99 || maskTexture;
    drawQuad(targetRect, modelViewMatrix, program.get(), GL_TRIANGLE_FAN, needsBlending);
}
#endif // defined(GL_ARB_texture_rectangle) 

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void TextureMapperGL::drawTexture(uint32_t texture, Flags flags, const IntSize& textureSize, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, const IntSize& platformSurfaceSize, bool premultipliedAlpha, unsigned exposedEdges)
#else
void TextureMapperGL::drawTexture(uint32_t texture, Flags flags, const IntSize& /* textureSize */, const FloatRect& targetRect, const TransformationMatrix& modelViewMatrix, float opacity, unsigned exposedEdges)
#endif
{
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    if (clipStack().current().scissorBox.isEmpty())
        return;
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    TransformationMatrix matrix = TransformationMatrix(data().projectionMatrix).multiply(modelViewMatrix).multiply(TransformationMatrix(
            targetRect.width(), 0, 0, 0,
            0, targetRect.height(), 0, 0,
            0, 0, 1, 0,
            targetRect.x(), targetRect.y(), 0, 1));

    FloatRect transformedTarget = matrix.mapRect(FloatRect(0, 0, 1.0, 1.0));
    if(!transformedTarget.intersects(FloatRect(-1.0, -1.0, 2.0, 2.0)))
        return;
#endif

    bool needsAntiliaing = m_enableEdgeDistanceAntialiasing && !modelViewMatrix.isIntegerTranslation();
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (needsAntiliaing && drawTextureWithAntialiasing(texture, flags, targetRect, modelViewMatrix, opacity, textureSize, platformSurfaceSize, premultipliedAlpha, exposedEdges))
#else
    if (needsAntiliaing && drawTextureWithAntialiasing(texture, flags, targetRect, modelViewMatrix, opacity, exposedEdges))
#endif
       return;

    RefPtr<TextureMapperShaderProgram> program;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    program = getShaderProgram();
#else
    program = data().sharedGLData().textureMapperShaderManager.getShaderProgram(TextureMapperShaderManager::Simple);
#endif

    GL_CMD(glUseProgram(program->id()));

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    drawTexturedQuadWithProgram(program.get(), texture, flags, targetRect, modelViewMatrix, opacity, textureSize, platformSurfaceSize, premultipliedAlpha);
#else
    drawTexturedQuadWithProgram(program.get(), texture, flags, targetRect, modelViewMatrix, opacity);
#endif
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
PassRefPtr<TextureMapperShaderProgram> TextureMapperGL::getShaderProgram()
{
#if ENABLE(TIZEN_UV_MAPPING)
    const bool useUVMapping = true;
#else
    const bool useUVMapping = false;
#endif
    TextureMapperShaderManager::ShaderType shaderType = useUVMapping ? TextureMapperShaderManager::SimpleUV : TextureMapperShaderManager::Simple;

    return data().sharedGLData().textureMapperShaderManager.getShaderProgram(shaderType);
}
#endif

static TransformationMatrix viewportMatrix()
{
    GLint viewport[4];
    GL_CMD(glGetIntegerv(GL_VIEWPORT, viewport));

    TransformationMatrix matrix;
    matrix.translate3d(viewport[0], viewport[1], 0);
    matrix.scale3d(viewport[2], viewport[3], 0);

    // Map x, y and z to unit square from OpenGL normalized device
    // coordinates which are -1 to 1 on every axis.
    matrix.translate3d(0.5, 0.5, 0.5);
    matrix.scale3d(0.5, 0.5, 0.5);

    return matrix;
}

static void scaleLineEquationCoeffecientsToOptimizeDistanceCalculation(float* coeffecients)
{
    // In the fragment shader we want to calculate the distance from this
    // line to a point (p), which is given by the formula:
    // (A*p.x + B*p.y + C) / sqrt (a^2 + b^2)
    // We can do a small amount of precalculation here to reduce the
    // amount of math in the shader by scaling the coeffecients now.
    float scale = 1.0 / FloatPoint(coeffecients[0], coeffecients[1]).length();
    coeffecients[0] = coeffecients[0] * scale;
    coeffecients[1] = coeffecients[1] * scale;
    coeffecients[2] = coeffecients[2] * scale;
}

static void getStandardEquationCoeffecientsForLine(const FloatPoint& p1, const FloatPoint& p2, float* coeffecients)
{
    // Given two points, the standard equation of a line (Ax + By + C = 0)
    // can be calculated via the formula:
    // (p1.y – p2.y)x + (p1.x – p2.x)y + ((p1.x*p2.y) – (p2.x*p1.y)) = 0
    coeffecients[0] = p1.y() - p2.y();
    coeffecients[1] = p2.x() - p1.x();
    coeffecients[2] = p1.x() * p2.y() - p2.x() * p1.y();
    scaleLineEquationCoeffecientsToOptimizeDistanceCalculation(coeffecients);
}

static void quadToEdgeArray(const FloatQuad& quad, float* edgeArray)
{
    if (quad.isCounterclockwise()) {
        getStandardEquationCoeffecientsForLine(quad.p4(), quad.p3(), edgeArray);
        getStandardEquationCoeffecientsForLine(quad.p3(), quad.p2(), edgeArray + 3);
        getStandardEquationCoeffecientsForLine(quad.p2(), quad.p1(), edgeArray + 6);
        getStandardEquationCoeffecientsForLine(quad.p1(), quad.p4(), edgeArray + 9);
        return;
    }
    getStandardEquationCoeffecientsForLine(quad.p4(), quad.p1(), edgeArray);
    getStandardEquationCoeffecientsForLine(quad.p1(), quad.p2(), edgeArray + 3);
    getStandardEquationCoeffecientsForLine(quad.p2(), quad.p3(), edgeArray + 6);
    getStandardEquationCoeffecientsForLine(quad.p3(), quad.p4(), edgeArray + 9);
}

static FloatSize scaledVectorDifference(const FloatPoint& point1, const FloatPoint& point2, float scale)
{
    FloatSize vector = point1 - point2;
    if (vector.diagonalLengthSquared())
        vector.scale(1.0 / vector.diagonalLength());

    vector.scale(scale);
    return vector;
}

static FloatQuad inflateQuad(const FloatQuad& quad, float distance)
{
    FloatQuad expandedQuad = quad;
    expandedQuad.setP1(expandedQuad.p1() + scaledVectorDifference(quad.p1(), quad.p2(), distance));
    expandedQuad.setP4(expandedQuad.p4() + scaledVectorDifference(quad.p4(), quad.p3(), distance));

    expandedQuad.setP1(expandedQuad.p1() + scaledVectorDifference(quad.p1(), quad.p4(), distance));
    expandedQuad.setP2(expandedQuad.p2() + scaledVectorDifference(quad.p2(), quad.p3(), distance));

    expandedQuad.setP2(expandedQuad.p2() + scaledVectorDifference(quad.p2(), quad.p1(), distance));
    expandedQuad.setP3(expandedQuad.p3() + scaledVectorDifference(quad.p3(), quad.p4(), distance));

    expandedQuad.setP3(expandedQuad.p3() + scaledVectorDifference(quad.p3(), quad.p2(), distance));
    expandedQuad.setP4(expandedQuad.p4() + scaledVectorDifference(quad.p4(), quad.p1(), distance));

    return expandedQuad;
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
bool TextureMapperGL::drawTextureWithAntialiasing(uint32_t texture, Flags flags, const FloatRect& originalTargetRect, const TransformationMatrix& modelViewMatrix, float opacity, const IntSize& textureSize, const IntSize& platformSurfaceSize, bool premultipliedAlpha, unsigned exposedEdges)
#else
bool TextureMapperGL::drawTextureWithAntialiasing(uint32_t texture, Flags flags, const FloatRect& originalTargetRect, const TransformationMatrix& modelViewMatrix, float opacity, unsigned exposedEdges)
#endif
{
    // For now we punt on rendering tiled layers with antialiasing. It's quite hard
    // to render them without seams.
    if (exposedEdges != AllEdges)
        return false;

    // The goal here is render a slightly larger (0.75 pixels in screen space) quad and to
    // gradually taper off the alpha values to do a simple version of edge distance
    // antialiasing. Note here that we are also including the viewport matrix (which
    // translates from normalized device coordinates to screen coordinates), because these
    // values are consumed in the fragment shader, which works in screen coordinates.
    TransformationMatrix screenSpaceTransform = viewportMatrix().multiply(TransformationMatrix(data().projectionMatrix)).multiply(modelViewMatrix).to2dTransform();
    if (!screenSpaceTransform.isInvertible())
        return false;
    FloatQuad quadInScreenSpace = screenSpaceTransform.mapQuad(originalTargetRect);

    const float inflationDistance = 0.75;
    FloatQuad expandedQuadInScreenSpace = inflateQuad(quadInScreenSpace, inflationDistance);

    // In the non-antialiased case the vertices passed are the unit rectangle and double
    // as the texture coordinates (0,0 1,0, 1,1 and 0,1). Here we map the expanded quad
    // coordinates in screen space back to the original rect's texture coordinates.
    // This has the effect of slightly increasing the size of the original quad's geometry
    // in the vertex shader.
    FloatQuad expandedQuadInTextureCoordinates = screenSpaceTransform.inverse().mapQuad(expandedQuadInScreenSpace);
    expandedQuadInTextureCoordinates.move(-originalTargetRect.x(), -originalTargetRect.y());
    expandedQuadInTextureCoordinates.scale(1 / originalTargetRect.width(), 1 / originalTargetRect.height());

    // We prepare both the expanded quad for the fragment shader as well as the rectangular bounding
    // box of that quad, as that seems necessary to properly antialias backfacing quads.
    float targetQuadEdges[24];
    quadToEdgeArray(expandedQuadInScreenSpace, targetQuadEdges);
    quadToEdgeArray(inflateQuad(quadInScreenSpace.boundingBox(),  inflationDistance), targetQuadEdges + 12);

    RefPtr<TextureMapperShaderProgramAntialiasingNoMask> program = data().sharedGLData().textureMapperShaderManager.antialiasingNoMaskProgram();
    GL_CMD(glUseProgram(program->id()));
    GL_CMD(glUniform3fv(program->expandedQuadEdgesInScreenSpaceLocation(), 8, targetQuadEdges));

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    drawTexturedQuadWithProgram(program.get(), texture, flags, DrawQuad(originalTargetRect, expandedQuadInTextureCoordinates), modelViewMatrix, opacity, textureSize, platformSurfaceSize, premultipliedAlpha);
#else
    drawTexturedQuadWithProgram(program.get(), texture, flags, DrawQuad(originalTargetRect, expandedQuadInTextureCoordinates), modelViewMatrix, opacity);
#endif
    return true;
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void TextureMapperGL::drawTexturedQuadWithProgram(TextureMapperShaderProgram* program, uint32_t texture, Flags flags, const DrawQuad& quadToDraw, const TransformationMatrix& modelViewMatrix, float opacity, const IntSize& textureSize, const IntSize& platformSurfaceSize, bool premultipliedAlpha)
#else
void TextureMapperGL::drawTexturedQuadWithProgram(TextureMapperShaderProgram* program, uint32_t texture, Flags flags, const DrawQuad& quadToDraw, const TransformationMatrix& modelViewMatrix, float opacity)
#endif
{
    GL_CMD(glEnableVertexAttribArray(program->vertexAttrib()));
    GL_CMD(glActiveTexture(GL_TEXTURE0));
    GL_CMD(glBindTexture(GL_TEXTURE_2D, texture));
    GL_CMD(glUniform1i(program->sourceTextureLocation(), 0));

    GL_CMD(glUniform1f(program->flipLocation(), !!(flags & ShouldFlipTexture)));
    if (TextureMapperShaderProgram::isValidUniformLocation(program->opacityLocation()))
        GL_CMD(glUniform1f(program->opacityLocation(), opacity));

#if ENABLE(TIZEN_UV_MAPPING)
    if (TextureMapperShaderProgram::isValidUniformLocation(program->textureSizeLocation())) {
        GL_CMD(glUniform2f(program->textureSizeLocation(),
            static_cast<float>(textureSize.width()) / static_cast<float>(platformSurfaceSize.width()),
            static_cast<float>(textureSize.height()) / static_cast<float>(platformSurfaceSize.height())));
    }
#endif

    bool needsBlending = (flags & SupportsBlending) || opacity < 0.99;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
	drawQuad(quadToDraw, modelViewMatrix, program, GL_TRIANGLE_FAN, needsBlending, premultipliedAlpha);
#else
    drawQuad(quadToDraw, modelViewMatrix, program, GL_TRIANGLE_FAN, needsBlending);
#endif
}

bool BitmapTextureGL::canReuseWith(const IntSize& contentsSize, Flags)
{
    return contentsSize == m_textureSize;
}

#if OS(DARWIN)
#define DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#else
#define DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE GL_UNSIGNED_BYTE
#endif

static void swizzleBGRAToRGBA(uint32_t* data, const IntRect& rect, int stride = 0)
{
    stride = stride ? stride : rect.width();
    for (int y = rect.y(); y < rect.maxY(); ++y) {
        uint32_t* p = data + y * stride;
        for (int x = rect.x(); x < rect.maxX(); ++x)
            p[x] = ((p[x] << 16) & 0xff0000) | ((p[x] >> 16) & 0xff) | (p[x] & 0xff00ff00);
    }
}

static bool driverSupportsBGRASwizzling()
{
#if defined(TEXMAP_OPENGL_ES_2)
    // FIXME: Implement reliable detection. See also https://bugs.webkit.org/show_bug.cgi?id=81103.
    return false;
#else
    return true;
#endif
}

static bool driverSupportsSubImage()
{
#if defined(TEXMAP_OPENGL_ES_2)
    // FIXME: Implement reliable detection.
    return false;
#else
    return true;
#endif
}

void BitmapTextureGL::didReset()
{
    if (!m_id)
        GL_CMD(glGenTextures(1, &m_id));

    m_shouldClear = true;
    if (m_textureSize == contentSize())
        return;

    GLuint format = driverSupportsBGRASwizzling() ? GL_BGRA : GL_RGBA;

    m_textureSize = contentSize();
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_id));
    GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CMD(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CMD(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_textureSize.width(), m_textureSize.height(), 0, format, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, 0));
#if !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    // some graphics driver did not support odd texture size.
    if (m_textureSize.width() % 2)
        m_textureSize.setWidth(m_textureSize.width() - 1);
    if (m_textureSize.height() % 2)
        m_textureSize.setHeight(m_textureSize.height() - 1);
#endif
}


void BitmapTextureGL::updateContents(const void* data, const IntRect& targetRect, const IntPoint& sourceOffset, int bytesPerLine)
{
    GLuint glFormat = GL_RGBA;
    GL_CMD(glBindTexture(GL_TEXTURE_2D, m_id));

    const unsigned bytesPerPixel = 4;
    if (driverSupportsBGRASwizzling())
        glFormat = GL_BGRA;
    else
        swizzleBGRAToRGBA(reinterpret_cast<uint32_t*>(const_cast<void*>(data)), IntRect(sourceOffset, targetRect.size()), bytesPerLine / bytesPerPixel);

    if (bytesPerLine == targetRect.width() / 4 && sourceOffset == IntPoint::zero()) {
        GL_CMD(glTexSubImage2D(GL_TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, (const char*)data));
        return;
    }

    // For ES drivers that don't support sub-images.
    if (!driverSupportsSubImage()) {
        const char* bits = static_cast<const char*>(data);
#if PLATFORM(TIZEN)
        for (int y = 0; y < targetRect.height() ; ++y) {
            const char *row = bits + (((sourceOffset.y() + y) * bytesPerLine) + (sourceOffset.x() * 4));
            GL_CMD(glTexSubImage2D(GL_TEXTURE_2D, 0, targetRect.x(), targetRect.y() + y, targetRect.width() , 1, glFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, row));
        }
#else
        const char* src = bits + sourceOffset.y() * bytesPerLine + sourceOffset.x() * bytesPerPixel;
        Vector<char> temporaryData(targetRect.width() * targetRect.height() * bytesPerPixel);
        char* dst = temporaryData.data();

        const int targetBytesPerLine = targetRect.width() * bytesPerPixel;
        for (int y = 0; y < targetRect.height(); ++y) {
            memcpy(dst, src, targetBytesPerLine);
            src += bytesPerLine;
            dst += targetBytesPerLine;
        }

        GL_CMD(glTexSubImage2D(GL_TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, temporaryData.data()));
#endif
        return;
    }

#if !defined(TEXMAP_OPENGL_ES_2)
    // Use the OpenGL sub-image extension, now that we know it's available.
    GL_CMD(glPixelStorei(GL_UNPACK_ROW_LENGTH, bytesPerLine / bytesPerPixel));
    GL_CMD(glPixelStorei(GL_UNPACK_SKIP_ROWS, sourceOffset.y()));
    GL_CMD(glPixelStorei(GL_UNPACK_SKIP_PIXELS, sourceOffset.x()));
    GL_CMD(glTexSubImage2D(GL_TEXTURE_2D, 0, targetRect.x(), targetRect.y(), targetRect.width(), targetRect.height(), glFormat, DEFAULT_TEXTURE_PIXEL_TRANSFER_TYPE, (const char*)data));
    GL_CMD(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
    GL_CMD(glPixelStorei(GL_UNPACK_SKIP_ROWS, 0));
    GL_CMD(glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0));
#endif
}

void BitmapTextureGL::updateContents(Image* image, const IntRect& targetRect, const IntPoint& offset)
{
    if (!image)
        return;
    NativeImagePtr frameImage = image->nativeImageForCurrentFrame();
    if (!frameImage)
        return;

    int bytesPerLine;
    const char* imageData;

#if PLATFORM(QT)
    imageData = reinterpret_cast<const char*>(frameImage->constBits());
    bytesPerLine = frameImage->bytesPerLine();
#elif USE(CAIRO)
    cairo_surface_t* surface = frameImage->surface();
    imageData = reinterpret_cast<const char*>(cairo_image_surface_get_data(surface));
    bytesPerLine = cairo_image_surface_get_stride(surface);
#endif

    updateContents(imageData, targetRect, offset, bytesPerLine);
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
PassRefPtr<PlatformSurfaceTexture> TextureMapperGL::createPlatformSurfaceTexture(int platformSurfaceID, IntSize size, bool useLinearFilter)
{
    PlatformSurfaceTextureGL* texture = new PlatformSurfaceTextureGL(platformSurfaceID, size);
    texture->initialize(useLinearFilter);
    return adoptRef(texture);
}
#endif

#if ENABLE(CSS_FILTERS)
void TextureMapperGL::drawFiltered(const BitmapTexture& sourceTexture, const BitmapTexture& contentTexture, const FilterOperation& filter, int pass)
{
    // For standard filters, we always draw the whole texture without transformations.
    RefPtr<StandardFilterProgram> program = data().sharedGLData().textureMapperShaderManager.getShaderForFilter(filter, pass);
    ASSERT(program);

    program->prepare(filter, pass, sourceTexture.contentSize(), static_cast<const BitmapTextureGL&>(contentTexture).id());

    GL_CMD(glEnableVertexAttribArray(program->vertexAttrib()));
    GL_CMD(glEnableVertexAttribArray(program->texCoordAttrib()));
    GL_CMD(glActiveTexture(GL_TEXTURE0));
    GL_CMD(glBindTexture(GL_TEXTURE_2D, static_cast<const BitmapTextureGL&>(sourceTexture).id()));
    glUniform1i(program->textureUniform(), 0);
    const GLfloat targetVertices[] = {-1, -1, 1, -1, 1, 1, -1, 1};
    const GLfloat sourceVertices[] = {0, 0, 1, 0, 1, 1, 0, 1};
    GL_CMD(glVertexAttribPointer(program->vertexAttrib(), 2, GL_FLOAT, GL_FALSE, 0, targetVertices));
    GL_CMD(glVertexAttribPointer(program->texCoordAttrib(), 2, GL_FLOAT, GL_FALSE, 0, sourceVertices));
    GL_CMD(glDisable(GL_BLEND));
    GL_CMD(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));
    GL_CMD(glDisableVertexAttribArray(program->vertexAttrib()));
    GL_CMD(glDisableVertexAttribArray(program->texCoordAttrib()));
}

PassRefPtr<BitmapTexture> BitmapTextureGL::applyFilters(const BitmapTexture& contentTexture, const FilterOperations& filters)
{
    RefPtr<BitmapTexture> previousSurface = m_textureMapper->data().currentSurface;

    RefPtr<BitmapTexture> source = this;
    RefPtr<BitmapTexture> target = m_textureMapper->acquireTextureFromPool(m_textureSize);
    for (unsigned i = 0; i < filters.size(); ++i) {
        const FilterOperation* filter = filters.at(i);
        ASSERT(filter);

        int numPasses = m_textureMapper->data().sharedGLData().textureMapperShaderManager.getPassesRequiredForFilter(*filter);
        for (int j = 0; j < numPasses; ++j) {
            m_textureMapper->bindSurface(target.get());
            m_textureMapper->drawFiltered((i || j) ? *source : contentTexture, contentTexture, *filter, j);
            std::swap(source, target);
        }
    }

    m_textureMapper->bindSurface(previousSurface.get());
    return source;
}
#endif

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
static inline TransformationMatrix createProjectionMatrix(const IntSize& size, bool mirrored, int angle = 0)
#else
static inline TransformationMatrix createProjectionMatrix(const IntSize& size, bool mirrored)
#endif
{
    const float near = 9999999;
    const float far = -99999;

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (gDirectRendering)
        if (angle == 90 || angle == 270)
            return TransformationMatrix(2.0 / float(size.height()), 0, 0, 0,
                    0, (mirrored ? 2.0 : -2.0) / float(size.width()), 0, 0,
                    0, 0, -2.f / (far - near), 0,
                    -1, mirrored ? -1 : 1, -(far + near) / (far - near), 1);
#endif

    return TransformationMatrix(2.0 / float(size.width()), 0, 0, 0,
                                0, (mirrored ? 2.0 : -2.0) / float(size.height()), 0, 0,
                                0, 0, -2.f / (far - near), 0,
                                -1, mirrored ? -1 : 1, -(far + near) / (far - near), 1);
}

#if PLATFORM(TIZEN)
static inline bool withinEpsilon(float a, float b)
{
    const float epsilon = 0.009f;
    return fabs(a - b) < epsilon;
}

static inline bool isRectilinear(const FloatQuad& rect)
{
    return (withinEpsilon(rect.p1().x(), rect.p2().x()) && withinEpsilon(rect.p2().y(), rect.p3().y()) && withinEpsilon(rect.p3().x(), rect.p4().x()) && withinEpsilon(rect.p4().y(), rect.p1().y()))
        || (withinEpsilon(rect.p1().y(), rect.p2().y()) && withinEpsilon(rect.p2().x(), rect.p3().x()) && withinEpsilon(rect.p3().y(), rect.p4().y()) && withinEpsilon(rect.p4().x(), rect.p1().x()));
}
#endif

void BitmapTextureGL::initializeStencil()
{
    if (m_rbo)
        return;
    GL_CMD(glGenRenderbuffers(1, &m_rbo));
    GL_CMD(glBindRenderbuffer(GL_RENDERBUFFER, m_rbo));
#ifdef TEXMAP_OPENGL_ES_2
    GL_CMD(glRenderbufferStorage(GL_RENDERBUFFER, GL_STENCIL_INDEX8, m_textureSize.width(), m_textureSize.height()));
#else
    GL_CMD(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, m_textureSize.width(), m_textureSize.height()));
#endif
    GL_CMD(glBindRenderbuffer(GL_RENDERBUFFER, 0));
    GL_CMD(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_rbo));
    GL_CMD(glClearStencil(0));
    GL_CMD(glClear(GL_STENCIL_BUFFER_BIT));
}

void BitmapTextureGL::clearIfNeeded()
{
    if (!m_shouldClear)
        return;

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (gDirectRendering && (gAngle == 90 || gAngle == 270))
        m_clipStack.init(IntRect(IntPoint::zero(), m_textureSize.transposedSize()));
    else
#endif
    m_clipStack.init(IntRect(IntPoint::zero(), m_textureSize));
    m_clipStack.apply();
    GL_CMD(glClearColor(0, 0, 0, 0));
    GL_CMD(glClear(GL_COLOR_BUFFER_BIT));
    m_shouldClear = false;
}

void BitmapTextureGL::createFboIfNeeded()
{
    if (m_fbo)
        return;

    GL_CMD(glGenFramebuffers(1, &m_fbo));
    GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    GL_CMD(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id(), 0));
    m_shouldClear = true;
}

void BitmapTextureGL::bind()
{
    GL_CMD(glBindTexture(GL_TEXTURE_2D, 0));
    createFboIfNeeded();
    GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, m_fbo));
    GL_CMD(glViewport(0, 0, m_textureSize.width(), m_textureSize.height()));
    clearIfNeeded();
    m_textureMapper->data().projectionMatrix = createProjectionMatrix(m_textureSize, true /* mirrored */);
    m_clipStack.apply();
}

BitmapTextureGL::~BitmapTextureGL()
{
    if (m_id)
        GL_CMD(glDeleteTextures(1, &m_id));

    if (m_fbo)
        GL_CMD(glDeleteFramebuffers(1, &m_fbo));

    if (m_rbo)
        GL_CMD(glDeleteRenderbuffers(1, &m_rbo));
}

bool BitmapTextureGL::isValid() const
{
    return m_id;
}

IntSize BitmapTextureGL::size() const
{
    return m_textureSize;
}

TextureMapperGL::~TextureMapperGL()
{
    delete m_data;
}

void TextureMapperGL::bindDefaultSurface()
{
    GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, data().targetFrameBuffer));
    IntSize viewportSize(data().viewport[2], data().viewport[3]);

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (gDirectRendering)
        data().projectionMatrix = createProjectionMatrix(viewportSize, data().PaintFlags & PaintingMirrored, gAngle);
    else
#endif
    data().projectionMatrix = createProjectionMatrix(viewportSize, data().PaintFlags & PaintingMirrored);

#if !USE(TIZEN_TEXTURE_MAPPER)
    GL_CMD(glViewport(data().viewport[0], data().viewport[1], viewportSize.width(), viewportSize.height()));
#else
    GL_CMD(glViewport(0, 0, viewportSize.width(), viewportSize.height()));
#endif

    m_clipStack.apply();
    data().currentSurface.clear();
}

void TextureMapperGL::bindSurface(BitmapTexture *surface)
{
    if (!surface) {
        bindDefaultSurface();
        return;
    }

    static_cast<BitmapTextureGL*>(surface)->bind();
    data().currentSurface = surface;
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
void TextureMapperGL::unbindSurface()
{
    glGetError();

    data().targetFrameBuffer = 0;
    GL_CMD(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    GLuint errorCode = glGetError();
    if (errorCode != GL_NO_ERROR)
        TIZEN_LOGE("'glBindFramebuffer(GL_FRAMEBUFFER, 0)' returned gl error (0x%x)", errorCode);
}
#endif

bool TextureMapperGL::beginScissorClip(const TransformationMatrix& modelViewMatrix, const FloatRect& targetRect)
{
    glEnable(GL_SCISSOR_TEST);

    // 3D transforms are currently not supported in scissor clipping
    // resulting in cropped surfaces when z>0.
    if (!modelViewMatrix.isAffine())
        return false;

    FloatQuad quad = modelViewMatrix.projectQuad(targetRect);
    IntRect rect = quad.enclosingBoundingBox();

    // Only use scissors on rectilinear clips.
#if PLATFORM(TIZEN)
    if (!isRectilinear(quad) || rect.isEmpty())
#else
    if (!quad.isRectilinear() || rect.isEmpty())
#endif
        return false;

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (gDirectRendering) {
        if (gAngle == 270)
            rect = IntRect(rect.y(), rect.x(), rect.height(), rect.width());
        else if (gAngle == 90) {
            GLint viewport[4];
            GL_CMD(glGetIntegerv(GL_VIEWPORT, viewport));
            rect = IntRect(viewport[3] - rect.maxY(), viewport[2] - rect.maxX(), rect.height(), rect.width());
        }
    }
#endif

    clipStack().current().scissorBox.intersect(rect);
    clipStack().apply();
    return true;
}

void TextureMapperGL::beginClip(const TransformationMatrix& modelViewMatrix, const FloatRect& targetRect)
{
    clipStack().push();
    if (beginScissorClip(modelViewMatrix, targetRect))
        return;

    data().initializeStencil();

    RefPtr<TextureMapperShaderProgram> program = data().sharedGLData().textureMapperShaderManager.getShaderProgram(TextureMapperShaderManager::Simple);

    GL_CMD(glUseProgram(program->id()));
    GL_CMD(glEnableVertexAttribArray(program->vertexAttrib()));
    const GLfloat unitRect[] = {0, 0, 1, 0, 1, 1, 0, 1};
    GL_CMD(glVertexAttribPointer(program->vertexAttrib(), 2, GL_FLOAT, GL_FALSE, 0, unitRect));

    TransformationMatrix matrix = TransformationMatrix(data().projectionMatrix)
            .multiply(modelViewMatrix)
            .multiply(TransformationMatrix(targetRect.width(), 0, 0, 0,
                0, targetRect.height(), 0, 0,
                0, 0, 1, 0,
                targetRect.x(), targetRect.y(), 0, 1));

    const GLfloat m4[] = {
        static_cast<const GLfloat>(matrix.m11()), static_cast<const GLfloat>(matrix.m12()), static_cast<const GLfloat>(matrix.m13()), static_cast<const GLfloat>(matrix.m14()),
        static_cast<const GLfloat>(matrix.m21()), static_cast<const GLfloat>(matrix.m22()), static_cast<const GLfloat>(matrix.m23()), static_cast<const GLfloat>(matrix.m24()),
        static_cast<const GLfloat>(matrix.m31()), static_cast<const GLfloat>(matrix.m32()), static_cast<const GLfloat>(matrix.m33()), static_cast<const GLfloat>(matrix.m34()),
        static_cast<const GLfloat>(matrix.m41()), static_cast<const GLfloat>(matrix.m42()), static_cast<const GLfloat>(matrix.m43()), static_cast<const GLfloat>(matrix.m44())
    };

    const GLfloat m4all[] = {
        2, 0, 0, 0,
        0, 2, 0, 0,
        0, 0, 1, 0,
        -1, -1, 0, 1
    };

    int& stencilIndex = clipStack().current().stencilIndex;

    GL_CMD(glEnable(GL_STENCIL_TEST));

    // Make sure we don't do any actual drawing.
    GL_CMD(glStencilFunc(GL_NEVER, stencilIndex, stencilIndex));

    // Operate only on the stencilIndex and above.
    GL_CMD(glStencilMask(0xff & ~(stencilIndex - 1)));

    // First clear the entire buffer at the current index.
    GL_CMD(glUniformMatrix4fv(program->matrixLocation(), 1, GL_FALSE, m4all));
    GL_CMD(glStencilOp(GL_ZERO, GL_ZERO, GL_ZERO));
    GL_CMD(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

    // Now apply the current index to the new quad.
    GL_CMD(glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE));
    GL_CMD(glUniformMatrix4fv(program->matrixLocation(), 1, GL_FALSE, m4));
    GL_CMD(glDrawArrays(GL_TRIANGLE_FAN, 0, 4));

    // Clear the state.
    GL_CMD(glDisableVertexAttribArray(program->vertexAttrib()));
    GL_CMD(glStencilMask(0));

    // Increase stencilIndex and apply stencil testing.
    stencilIndex *= 2;
    clipStack().apply();
}

void TextureMapperGL::endClip()
{
    clipStack().pop();
    clipStack().apply();
}

#if ENABLE(TIZEN_OVERLAP_REGION_PAINT_OPTIMIZATION)
IntRect TextureMapperGL::clipBounds()
{
    IntRect rect = clipStack().current().scissorBox;

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (gDirectRendering) {
        if (gAngle == 270)
            rect = IntRect(rect.y(), rect.x(), rect.height(), rect.width());
        else if (gAngle == 90) {
            GLint viewport[4];
            GL_CMD(glGetIntegerv(GL_VIEWPORT, viewport));
            rect = IntRect(viewport[3] - rect.maxY(), viewport[2] - rect.maxX(), rect.height(), rect.width());
        }
    }
#endif

    return rect;
}
#endif

PassRefPtr<BitmapTexture> TextureMapperGL::createTexture()
{
    BitmapTextureGL* texture = new BitmapTextureGL();
    texture->setTextureMapper(this);
    return adoptRef(texture);
}

PassOwnPtr<TextureMapper> TextureMapper::platformCreateAccelerated()
{
    return TextureMapperGL::create();
}

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
void TextureMapperGL::setDirectRendering(bool b)
{
    gDirectRendering = b;
}
#endif

}
