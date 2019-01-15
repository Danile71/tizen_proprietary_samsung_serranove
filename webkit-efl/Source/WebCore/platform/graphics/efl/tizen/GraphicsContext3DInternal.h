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

#ifndef GraphicsContext3DInternal_h
#define GraphicsContext3DInternal_h

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)

#if ENABLE(TIZEN_WEBGL_ANGLE_INTEGRATION)
#include "ANGLEWebKitBridge.h"
#endif
#include "GraphicsContext3D.h"
#if ENABLE(TIZEN_WEBKIT2)
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#if USE(TIZEN_TEXTURE_MAPPER)
#include <texmap/TextureMapper.h>
#endif

#define GL_CMD(x) x

#else
#include <Evas_GL.h>

#if USE(TIZEN_TEXTURE_MAPPER)
#include "TextureMapperPlatformLayer.h"
#endif

#define GL_CMD(x) m_api->x

#endif

namespace WebCore {

class Extensions3DTizen;

class GraphicsContext3DInternal
{
public:
    static PassOwnPtr<GraphicsContext3DInternal> create(GraphicsContext3D::Attributes, HostWindow*, bool);
    virtual ~GraphicsContext3DInternal();

    PlatformGraphicsContext3D platformGraphicsContext3D() const { return m_context; }
    virtual Platform3DObject platformTexture() const { return 0; }
#if USE(ACCELERATED_COMPOSITING)
    virtual PlatformLayer* platformLayer() const { return 0; }
#endif

#if ENABLE(TIZEN_WEBGL_ANIMATION_SNAPSHOT_FIX)
    virtual void paintRenderingResultsToCanvas(ImageBuffer*, int, int, bool isForcePaint = false) { }
#else
    virtual void paintRenderingResultsToCanvas(ImageBuffer*, int, int) { }
#endif

    virtual PassRefPtr<ImageData> paintRenderingResultsToImageData(int width, int height); 
    virtual bool paintsIntoCanvasBuffer() const { return false; }

    virtual void prepareTexture(int width, int height) {}

    virtual void reshape(int width, int height) {}

    virtual void markContextChanged() {}
    virtual void markLayerComposited() {}
    virtual bool layerComposited() const { return false; }

    virtual bool makeContextCurrent();

    virtual void activeTexture(GC3Denum texture);
    virtual void attachShader(Platform3DObject program, Platform3DObject shader);
    virtual void bindAttribLocation(Platform3DObject, GC3Duint index, const String& name);
    virtual void bindBuffer(GC3Denum target, Platform3DObject);
    virtual void bindFramebuffer(GC3Denum target, Platform3DObject);
    virtual void bindRenderbuffer(GC3Denum target, Platform3DObject);
    virtual void bindTexture(GC3Denum target, Platform3DObject);
    virtual void blendColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha);
    virtual void blendEquation(GC3Denum mode);
    virtual void blendEquationSeparate(GC3Denum modeRGB, GC3Denum modeAlpha);
    virtual void blendFunc(GC3Denum sfactor, GC3Denum dfactor);
    virtual void blendFuncSeparate(GC3Denum srcRGB, GC3Denum dstRGB, GC3Denum srcAlpha, GC3Denum dstAlpha);

    virtual void bufferData(GC3Denum target, GC3Dsizeiptr, const void* data, GC3Denum usage);
    virtual void bufferSubData(GC3Denum target, GC3Dintptr offset, GC3Dsizeiptr, const void* data);

    virtual GC3Denum checkFramebufferStatus(GC3Denum target);
    virtual void clear(GC3Dbitfield mask);
    virtual void clearColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha);
    virtual void clearDepth(GC3Dclampf depth);
    virtual void clearStencil(GC3Dint s);
    virtual void colorMask(GC3Dboolean red, GC3Dboolean green, GC3Dboolean blue, GC3Dboolean alpha);
    virtual void compileShader(Platform3DObject);

    virtual void copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border);
    virtual void copyTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);
    virtual void cullFace(GC3Denum mode);
    virtual void depthFunc(GC3Denum func);
    virtual void depthMask(GC3Dboolean flag);
    virtual void depthRange(GC3Dclampf zNear, GC3Dclampf zFar);
    virtual void detachShader(Platform3DObject, Platform3DObject);
    virtual void disable(GC3Denum cap);
    virtual void disableVertexAttribArray(GC3Duint index);
    virtual void drawArrays(GC3Denum mode, GC3Dint first, GC3Dsizei count);
    virtual void drawElements(GC3Denum mode, GC3Dsizei count, GC3Denum type, GC3Dintptr offset);

    virtual void enable(GC3Denum cap);
    virtual void enableVertexAttribArray(GC3Duint index);
    virtual void finish();
    virtual void flush();
    virtual void framebufferRenderbuffer(GC3Denum target, GC3Denum attachment, GC3Denum renderbuffertarget, Platform3DObject);
    virtual void framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, Platform3DObject, GC3Dint level);
    virtual void frontFace(GC3Denum mode);
    virtual void generateMipmap(GC3Denum target);

    virtual bool getActiveAttrib(Platform3DObject program, GC3Duint index, ActiveInfo&);
    virtual bool getActiveUniform(Platform3DObject program, GC3Duint index, ActiveInfo&);
    virtual void getAttachedShaders(Platform3DObject program, GC3Dsizei maxCount, GC3Dsizei* count, Platform3DObject* shaders);
    virtual GC3Dint getAttribLocation(Platform3DObject, const String& name);
    virtual void getBooleanv(GC3Denum pname, GC3Dboolean* value);
    virtual void getBufferParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value);
    virtual GraphicsContext3D::Attributes getContextAttributes();
    virtual GC3Denum getError();
    virtual void getFloatv(GC3Denum pname, GC3Dfloat* value);
    virtual void getFramebufferAttachmentParameteriv(GC3Denum target, GC3Denum attachment, GC3Denum pname, GC3Dint* value);
    virtual void getIntegerv(GC3Denum pname, GC3Dint* value);
    virtual void getProgramiv(Platform3DObject program, GC3Denum pname, GC3Dint* value);
    virtual String getProgramInfoLog(Platform3DObject);
    virtual void getRenderbufferParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value);
    virtual void getShaderiv(Platform3DObject, GC3Denum pname, GC3Dint* value);
    virtual String getShaderInfoLog(Platform3DObject);
    virtual void getShaderPrecisionFormat(GC3Denum shaderType, GC3Denum precisionType, GC3Dint* range, GC3Dint* precision);
    virtual String getShaderSource(Platform3DObject);
    virtual String getString(GC3Denum name);
    virtual void getTexParameterfv(GC3Denum target, GC3Denum pname, GC3Dfloat* value);
    virtual void getTexParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value);
    virtual void getUniformfv(Platform3DObject program, GC3Dint location, GC3Dfloat* value);
    virtual void getUniformiv(Platform3DObject program, GC3Dint location, GC3Dint* value);
    virtual GC3Dint getUniformLocation(Platform3DObject, const String& name);
    virtual void getVertexAttribfv(GC3Duint index, GC3Denum pname, GC3Dfloat* value);
    virtual void getVertexAttribiv(GC3Duint index, GC3Denum pname, GC3Dint* value);
    virtual GC3Dsizeiptr getVertexAttribOffset(GC3Duint index, GC3Denum pname);

    virtual void hint(GC3Denum target, GC3Denum mode);
    virtual GC3Dboolean isBuffer(Platform3DObject);
    virtual GC3Dboolean isEnabled(GC3Denum cap);
    virtual GC3Dboolean isFramebuffer(Platform3DObject);
    virtual GC3Dboolean isProgram(Platform3DObject);
    virtual GC3Dboolean isRenderbuffer(Platform3DObject);
    virtual GC3Dboolean isShader(Platform3DObject);
    virtual GC3Dboolean isTexture(Platform3DObject);
    virtual void lineWidth(GC3Dfloat);
    virtual void linkProgram(Platform3DObject);
    virtual void pixelStorei(GC3Denum pname, GC3Dint param);
    virtual void polygonOffset(GC3Dfloat factor, GC3Dfloat units);

    virtual void readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data);

    virtual void releaseShaderCompiler();

    virtual void renderbufferStorage(GC3Denum target, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height);
    virtual void sampleCoverage(GC3Dclampf value, GC3Dboolean invert);
    virtual void scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);
    virtual void shaderSource(Platform3DObject, const String& string);
    virtual void stencilFunc(GC3Denum func, GC3Dint ref, GC3Duint mask);
    virtual void stencilFuncSeparate(GC3Denum face, GC3Denum func, GC3Dint ref, GC3Duint mask);
    virtual void stencilMask(GC3Duint mask);
    virtual void stencilMaskSeparate(GC3Denum face, GC3Duint mask);
    virtual void stencilOp(GC3Denum fail, GC3Denum zfail, GC3Denum zpass);
    virtual void stencilOpSeparate(GC3Denum face, GC3Denum fail, GC3Denum zfail, GC3Denum zpass);

    virtual bool texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels);
    virtual void texParameterf(GC3Denum target, GC3Denum pname, GC3Dfloat param);
    virtual void texParameteri(GC3Denum target, GC3Denum pname, GC3Dint param);
    virtual void texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, const void* pixels);

    virtual void uniform1f(GC3Dint location, GC3Dfloat x);
    virtual void uniform1fv(GC3Dint location, GC3Dsizei, GC3Dfloat* v);
    virtual void uniform1i(GC3Dint location, GC3Dint x);
    virtual void uniform1iv(GC3Dint location, GC3Dsizei, GC3Dint* v);
    virtual void uniform2f(GC3Dint location, GC3Dfloat x, float y);
    virtual void uniform2fv(GC3Dint location, GC3Dsizei, GC3Dfloat* v);
    virtual void uniform2i(GC3Dint location, GC3Dint x, GC3Dint y);
    virtual void uniform2iv(GC3Dint location, GC3Dsizei, GC3Dint* v);
    virtual void uniform3f(GC3Dint location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z);
    virtual void uniform3fv(GC3Dint location, GC3Dsizei, GC3Dfloat* v);
    virtual void uniform3i(GC3Dint location, GC3Dint x, GC3Dint y, GC3Dint z);
    virtual void uniform3iv(GC3Dint location, GC3Dsizei, GC3Dint* v);
    virtual void uniform4f(GC3Dint location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w);
    virtual void uniform4fv(GC3Dint location, GC3Dsizei, GC3Dfloat* v);
    virtual void uniform4i(GC3Dint location, GC3Dint x, GC3Dint y, GC3Dint z, GC3Dint w);
    virtual void uniform4iv(GC3Dint location, GC3Dsizei, GC3Dint* v);
    virtual void uniformMatrix2fv(GC3Dint location, GC3Dsizei, GC3Dboolean transpose, GC3Dfloat* value);
    virtual void uniformMatrix3fv(GC3Dint location, GC3Dsizei, GC3Dboolean transpose, GC3Dfloat* value);
    virtual void uniformMatrix4fv(GC3Dint location, GC3Dsizei, GC3Dboolean transpose, GC3Dfloat* value);

    virtual void useProgram(Platform3DObject);
    virtual void validateProgram(Platform3DObject);

    virtual void vertexAttrib1f(GC3Duint index, GC3Dfloat x);
    virtual void vertexAttrib1fv(GC3Duint index, GC3Dfloat* values);
    virtual void vertexAttrib2f(GC3Duint index, GC3Dfloat x, GC3Dfloat y);
    virtual void vertexAttrib2fv(GC3Duint index, GC3Dfloat* values);
    virtual void vertexAttrib3f(GC3Duint index, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z);
    virtual void vertexAttrib3fv(GC3Duint index, GC3Dfloat* values);
    virtual void vertexAttrib4f(GC3Duint index, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w);
    virtual void vertexAttrib4fv(GC3Duint index, GC3Dfloat* values);
    virtual void vertexAttribPointer(GC3Duint index, GC3Dint size, GC3Denum type, GC3Dboolean normalized, GC3Dsizei stride, GC3Dintptr offset);

    virtual void viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height);

    virtual Platform3DObject createBuffer();
    virtual Platform3DObject createFramebuffer();
    virtual Platform3DObject createProgram();
    virtual Platform3DObject createRenderbuffer();
    virtual Platform3DObject createShader(GC3Denum);
    virtual Platform3DObject createTexture();

    virtual void deleteBuffer(Platform3DObject);
    virtual void deleteFramebuffer(Platform3DObject);
    virtual void deleteProgram(Platform3DObject);
    virtual void deleteRenderbuffer(Platform3DObject);
    virtual void deleteShader(Platform3DObject);
    virtual void deleteTexture(Platform3DObject);

    virtual void synthesizeGLError(GC3Denum error);

    virtual Extensions3D* getExtensions();

    virtual void validateAttributes();

    // Read rendering results into a pixel array with the same format as the
    // backbuffer.
    virtual void readRenderingResults(unsigned char* pixels, int pixelsSize, int width, int height) { }

    virtual bool isGLES2Compliant() const { return true; }

protected:
    GraphicsContext3DInternal(GraphicsContext3D::Attributes attrs, GraphicsContext3D::RenderStyle);

    virtual bool initialize(HostWindow* hostWindow);
    virtual bool createContext(Evas_Object* view) { return false; }
    virtual bool createSurface(Evas_Object* view) { return false; }

    GraphicsContext3D::Attributes m_attributes;

    OwnPtr<Extensions3DTizen> m_extensions;

    ListHashSet<GC3Denum> m_syntheticErrors;

    GraphicsContext3D::RenderStyle m_renderStyle;

#if ENABLE(TIZEN_WEBGL_ANGLE_INTEGRATION)
    friend class Extensions3DTizen;
    ANGLEWebKitBridge m_compiler;
#endif

#if !ENABLE(TIZEN_WEBKIT2)
    Evas_GL* m_evasGL;
    Evas_GL_Context* m_context;
    Evas_GL_Surface* m_surface;
    Evas_GL_API* m_api;
#if ENABLE(TIZEN_EVAS_GL_DIRECT_RENDERING)
    Evas_GL_Config* m_config;
#endif
#else
    int m_width, m_height;

    EGLDisplay m_display;
    EGLContext m_context;
    EGLSurface m_surface;
#endif
};

} // namespace WebCore

#endif

#endif // GraphicsContext3DInternal_h
