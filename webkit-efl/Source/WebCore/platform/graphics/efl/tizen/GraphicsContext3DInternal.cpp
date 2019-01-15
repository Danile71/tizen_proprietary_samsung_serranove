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
#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsContext3DInternal.h"

#include "Extensions3DTizen.h"
#if ENABLE(WEBGL)
#include "GraphicsContext3DOffscreen.h"
#endif

#if ENABLE(TIZEN_WEBKIT2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include "GraphicsContext3DOnscreen.h"
#endif

#include "HostWindow.h"
#include "ImageData.h"
#include "NotImplemented.h"
#if !ENABLE(TIZEN_WEBKIT2)
#include "PageClientEfl.h"
#endif
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

namespace WebCore {

PassOwnPtr<GraphicsContext3DInternal> GraphicsContext3DInternal::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, bool renderDirectlyToHostWindow)
{
    if (renderDirectlyToHostWindow)
#if !ENABLE(TIZEN_WEBKIT2)
        return GraphicsContext3DOnscreen::create(attrs, hostWindow);
#else
        return nullptr;
#endif

#if ENABLE(WEBGL)
    return GraphicsContext3DOffscreen::create(attrs, hostWindow);
#else
    return nullptr;
#endif
}

GraphicsContext3DInternal::GraphicsContext3DInternal(GraphicsContext3D::Attributes attrs, GraphicsContext3D::RenderStyle renderStyle)
    : m_attributes(attrs)
    , m_renderStyle(renderStyle)
#if ENABLE(TIZEN_WEBKIT2)
    , m_width(0)
    , m_height(0)
    , m_display(EGL_NO_DISPLAY)
    , m_context(EGL_NO_CONTEXT)
    , m_surface(EGL_NO_SURFACE)
#else
    , m_evasGL(0)
    , m_context(0)
    , m_surface(0)
    , m_api(0)
#if ENABLE(TIZEN_EVAS_GL_DIRECT_RENDERING)
    , m_config(0)
#endif
#endif
{
    LOG(AcceleratedCompositing, "[%p] GraphicsContext3DInternal::%s() - CONSTRUCTOR\n", this, __func__);
    LOG(AcceleratedCompositing, "---> Evas_GL\n");
}

GraphicsContext3DInternal::~GraphicsContext3DInternal()
{
#if !ENABLE(TIZEN_WEBKIT2)
    evas_gl_surface_destroy(m_evasGL, m_surface);
    evas_gl_context_destroy(m_evasGL, m_context);
    evas_gl_free(m_evasGL);
#if ENABLE(TIZEN_EVAS_GL_DIRECT_RENDERING)
    evas_gl_config_free(m_config);
#endif
#endif
    m_syntheticErrors.clear();
    LOG(AcceleratedCompositing, "[%p] GraphicsContext3DInternal::%s() - DESTRUCTOR\n", this, __func__);
}

bool GraphicsContext3DInternal::initialize(HostWindow* hostWindow)
{
#if !ENABLE(TIZEN_WEBKIT2)
    LOG(AcceleratedCompositing, "[%p] GraphicsContext3DInternal::%s()\n", this, __func__);

    PageClientEfl* pageClient = static_cast<PageClientEfl*>(hostWindow->platformPageClient());
    Evas_Object* view = pageClient->view();

    Evas* evas = evas_object_evas_get(view);
    if (!evas)
        return false;

    m_evasGL = evas_gl_new(evas);
    if (!m_evasGL) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("failed to evas_gl_new()");
#endif
        return false;
    }

    m_api = evas_gl_api_get(m_evasGL);
    if (!m_api) {
        evas_gl_free(m_evasGL);
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("failed to evas_gl_api_get()");
#endif
        return false;
    }
    if (!createContext(view)) {
        evas_gl_free(m_evasGL);
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("failed to create Evas_GL_Context");
#endif
        return false;
    }
    if (!createSurface(view)) {
        evas_gl_context_destroy(m_evasGL, m_context);
        evas_gl_free(m_evasGL);
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGE("failed to create Evas_GL_Surface");
#endif
        return false;
    }
    if (!makeContextCurrent()) {
        evas_gl_surface_destroy(m_evasGL, m_surface);
        evas_gl_context_destroy(m_evasGL, m_context);
        evas_gl_free(m_evasGL);
        return false;
    }
#endif
    return true;
}

bool GraphicsContext3DInternal::makeContextCurrent()
{
#if ENABLE(TIZEN_WEBKIT2)
    return false;
#else
    return (evas_gl_make_current(m_evasGL, m_surface, m_context) == EINA_TRUE);
#endif
}

PassRefPtr<ImageData> GraphicsContext3DInternal::paintRenderingResultsToImageData(int width, int height)
{
    return 0;
}

void GraphicsContext3DInternal::synthesizeGLError(GC3Denum error)
{
    m_syntheticErrors.add(error);
}

Extensions3D* GraphicsContext3DInternal::getExtensions()
{
    if (!m_extensions)
        m_extensions = adoptPtr(new Extensions3DTizen(this));
    return m_extensions.get();
}

void GraphicsContext3DInternal::validateAttributes()
{
    Extensions3D* extensions = getExtensions();
    if (m_attributes.stencil) {
        if (extensions->supports("GL_EXT_packed_depth_stencil")) {
            extensions->ensureEnabled("GL_EXT_packed_depth_stencil");
            // Force depth if stencil is true.
            m_attributes.depth = true;
        }
    }
    if (m_attributes.antialias) {
        bool isValidVendor = true;
        // Currently in Mac we only turn on antialias if vendor is NVIDIA.
        String vendor(reinterpret_cast<const char*>(GL_CMD(glGetString(GraphicsContext3D::VENDOR))));

        if (vendor.isEmpty() || !vendor.contains("NVIDIA"))
            isValidVendor = false;
        if (!isValidVendor || !extensions->supports("GL_ANGLE_framebuffer_multisample"))
            m_attributes.antialias = false;
        else
            extensions->ensureEnabled("GL_ANGLE_framebuffer_multisample");
    }
}

void GraphicsContext3DInternal::activeTexture(GC3Denum texture)
{
    makeContextCurrent();
    GL_CMD(glActiveTexture(texture));
}

void GraphicsContext3DInternal::attachShader(Platform3DObject program, Platform3DObject shader)
{
    makeContextCurrent();
    GL_CMD(glAttachShader(program, shader));
}

void GraphicsContext3DInternal::bindAttribLocation(Platform3DObject program, GC3Duint index, const String& name)
{
    makeContextCurrent();
    GL_CMD(glBindAttribLocation(program, index, name.utf8().data()));
}

void GraphicsContext3DInternal::bindBuffer(GC3Denum target, Platform3DObject buffer)
{
    makeContextCurrent();
    GL_CMD(glBindBuffer(target, buffer));
}

void GraphicsContext3DInternal::bindFramebuffer(GC3Denum target, Platform3DObject framebuffer)
{
    makeContextCurrent();
    GL_CMD(glBindFramebuffer(target, framebuffer));
}

void GraphicsContext3DInternal::bindRenderbuffer(GC3Denum target, Platform3DObject buffer)
{
    makeContextCurrent();
    GL_CMD(glBindRenderbuffer(target, buffer));
}

void GraphicsContext3DInternal::bindTexture(GC3Denum target, Platform3DObject texture)
{
    makeContextCurrent();
    GL_CMD(glBindTexture(target, texture));
}

void GraphicsContext3DInternal::blendColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha)
{
    makeContextCurrent();
    GL_CMD(glBlendColor(red, green, blue, alpha));
}

void GraphicsContext3DInternal::blendEquation(GC3Denum mode)
{
    makeContextCurrent();
    GL_CMD(glBlendEquation(mode));
}

void GraphicsContext3DInternal::blendEquationSeparate(GC3Denum modeRGB, GC3Denum modeAlpha)
{
    makeContextCurrent();
    GL_CMD(glBlendEquationSeparate(modeRGB, modeAlpha));
}

void GraphicsContext3DInternal::blendFunc(GC3Denum sfactor, GC3Denum dfactor)
{
    makeContextCurrent();
    GL_CMD(glBlendFunc(sfactor, dfactor));
}

void GraphicsContext3DInternal::blendFuncSeparate(GC3Denum srcRGB, GC3Denum dstRGB, GC3Denum srcAlpha, GC3Denum dstAlpha)
{
    makeContextCurrent();
    GL_CMD(glBlendFuncSeparate(srcRGB, dstRGB, srcAlpha, dstAlpha));
}

void GraphicsContext3DInternal::bufferData(GC3Denum target, GC3Dsizeiptr size, const void* data, GC3Denum usage)
{
    makeContextCurrent();
    GL_CMD(glBufferData(target, size, data, usage));
}

void GraphicsContext3DInternal::bufferSubData(GC3Denum target, GC3Dintptr offset, GC3Dsizeiptr size, const void* data)
{
    makeContextCurrent();
    GL_CMD(glBufferSubData(target, offset, size, data));
}

GC3Denum GraphicsContext3DInternal::checkFramebufferStatus(GC3Denum target)
{
    makeContextCurrent();
    return GL_CMD(glCheckFramebufferStatus(target));
}

void GraphicsContext3DInternal::clear(GC3Dbitfield mask)
{
    makeContextCurrent();
    GL_CMD(glClear(mask));
}

void GraphicsContext3DInternal::clearColor(GC3Dclampf red, GC3Dclampf green, GC3Dclampf blue, GC3Dclampf alpha)
{
    makeContextCurrent();
    GL_CMD(glClearColor(red, green, blue, alpha));
}

void GraphicsContext3DInternal::clearDepth(GC3Dclampf depth)
{
    makeContextCurrent();
    GL_CMD(glClearDepthf(depth));
}

void GraphicsContext3DInternal::clearStencil(GC3Dint s)
{
    makeContextCurrent();
    GL_CMD(glClearStencil(s));
}

void GraphicsContext3DInternal::colorMask(GC3Dboolean red, GC3Dboolean green, GC3Dboolean blue, GC3Dboolean alpha)
{
    makeContextCurrent();
    GL_CMD(glColorMask(red, green, blue, alpha));
}

void GraphicsContext3DInternal::compileShader(Platform3DObject shader)
{
    makeContextCurrent();
    GL_CMD(glCompileShader(shader));
}

void GraphicsContext3DInternal::copyTexImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Dint border)
{
    makeContextCurrent();
    GL_CMD(glCopyTexImage2D(target, level, internalformat, x, y, width, height, border));
}

void GraphicsContext3DInternal::copyTexSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    GL_CMD(glCopyTexSubImage2D(target, level, xoffset, yoffset, x, y, width, height));
}

void GraphicsContext3DInternal::cullFace(GC3Denum mode)
{
    makeContextCurrent();
    GL_CMD(glCullFace(mode));
}

void GraphicsContext3DInternal::depthFunc(GC3Denum func)
{
    if (!m_attributes.depth)
        return;

    makeContextCurrent();
    GL_CMD(glDepthFunc(func));
}

void GraphicsContext3DInternal::depthMask(GC3Dboolean flag)
{
    makeContextCurrent();
    GL_CMD(glDepthMask(flag));
}

void GraphicsContext3DInternal::depthRange(GC3Dclampf zNear, GC3Dclampf zFar)
{
    makeContextCurrent();
    GL_CMD(glDepthRangef(zNear, zFar));
}

void GraphicsContext3DInternal::detachShader(Platform3DObject program, Platform3DObject shader)
{
    makeContextCurrent();
    GL_CMD(glDetachShader(program, shader));
}

void GraphicsContext3DInternal::disable(GC3Denum cap)
{
    makeContextCurrent();
    GL_CMD(glDisable(cap));
}

void GraphicsContext3DInternal::disableVertexAttribArray(GC3Duint index)
{
    makeContextCurrent();
    GL_CMD(glDisableVertexAttribArray(index));
}

void GraphicsContext3DInternal::drawArrays(GC3Denum mode, GC3Dint first, GC3Dsizei count)
{
    makeContextCurrent();
    GL_CMD(glDrawArrays(mode, first, count));
}

void GraphicsContext3DInternal::drawElements(GC3Denum mode, GC3Dsizei count, GC3Denum type, GC3Dintptr offset)
{
    makeContextCurrent();
    GL_CMD(glDrawElements(mode, count, type, reinterpret_cast<GLvoid*>(static_cast<intptr_t>(offset))));
}

void GraphicsContext3DInternal::enable(GC3Denum cap)
{
    makeContextCurrent();
    GL_CMD(glEnable(cap));
}

void GraphicsContext3DInternal::enableVertexAttribArray(GC3Duint index)
{
    makeContextCurrent();
    GL_CMD(glEnableVertexAttribArray(index));
}

void GraphicsContext3DInternal::finish()
{
    makeContextCurrent();
    GL_CMD(glFinish());
}

void GraphicsContext3DInternal::flush()
{
    makeContextCurrent();
    GL_CMD(glFlush());
}

void GraphicsContext3DInternal::framebufferRenderbuffer(GC3Denum target, GC3Denum attachment, GC3Denum renderbuffertarget, Platform3DObject renderbuffer)
{
    makeContextCurrent();
    GL_CMD(glFramebufferRenderbuffer(target, attachment, renderbuffertarget, renderbuffer));
}

void GraphicsContext3DInternal::framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, Platform3DObject texture, GC3Dint level)
{
    makeContextCurrent();
    GL_CMD(glFramebufferTexture2D(target, attachment, textarget, texture, level));
}

void GraphicsContext3DInternal::frontFace(GC3Denum mode)
{
    makeContextCurrent();
    GL_CMD(glFrontFace(mode));
}

void GraphicsContext3DInternal::generateMipmap(GC3Denum target)
{
    makeContextCurrent();
    GL_CMD(glGenerateMipmap(target));
}

bool GraphicsContext3DInternal::getActiveAttrib(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(GL_INVALID_VALUE);
        return false;
    }

    makeContextCurrent();

    int maxNameLength = -1;
    GL_CMD(glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &maxNameLength));
    if (maxNameLength < 0)
        return false;

    OwnArrayPtr<char> name = adoptArrayPtr(new char[maxNameLength]);
    if (!name) {
        synthesizeGLError(GL_OUT_OF_MEMORY);
        return false;
    }

    int length = 0;
    int size = -1;
    GLenum type = 0;

    GL_CMD(glGetActiveAttrib(program, index, maxNameLength, &length, &size, &type, name.get()));
    if (size < 0)
        return false;

    info.name = String::fromUTF8(name.get(), length);
    info.type = type;
    info.size = size;
    return true;
}

bool GraphicsContext3DInternal::getActiveUniform(Platform3DObject program, GC3Duint index, ActiveInfo& info)
{
    if (!program) {
        synthesizeGLError(GL_INVALID_VALUE);
        return false;
    }

    makeContextCurrent();

    int maxNameLength = -1;
    GL_CMD(glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxNameLength));
    if (maxNameLength < 0)
        return false;

    OwnArrayPtr<char> name = adoptArrayPtr(new char[maxNameLength]);
    if (!name) {
        synthesizeGLError(GL_OUT_OF_MEMORY);
        return false;
    }

    int length = 0;
    int size = -1;
    GLenum type = 0;

    GL_CMD(glGetActiveUniform(program, index, maxNameLength, &length, &size, &type, name.get()));
    if (size < 0)
        return false;

    info.name = String::fromUTF8(name.get(), length);
    info.type = type;
    info.size = size;
    return true;
}

void GraphicsContext3DInternal::getAttachedShaders(Platform3DObject program, GC3Dsizei maxCount, GC3Dsizei* count, Platform3DObject* shaders)
{
    if (!program) {
        synthesizeGLError(GraphicsContext3D::INVALID_VALUE);
        return;
    }
    makeContextCurrent();
    GL_CMD(glGetAttachedShaders(program, maxCount, count, shaders));
}

int GraphicsContext3DInternal::getAttribLocation(Platform3DObject program, const String& name)
{
    if (!program)
        return -1;

    makeContextCurrent();
    return GL_CMD(glGetAttribLocation(program, name.utf8().data()));
}

void GraphicsContext3DInternal::getBooleanv(GC3Denum pname, GC3Dboolean* value)
{
    makeContextCurrent();
    if (pname == GraphicsContext3D::DITHER)
        *value = GL_CMD(glIsEnabled(pname));
    else
        GL_CMD(glGetBooleanv(pname, value));
}

void GraphicsContext3DInternal::getBufferParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    GL_CMD(glGetBufferParameteriv(target, pname, value));
}

GraphicsContext3D::Attributes GraphicsContext3DInternal::getContextAttributes()
{
    return m_attributes;
}

GC3Denum GraphicsContext3DInternal::getError()
{
    if (m_syntheticErrors.size() > 0) {
        ListHashSet<GC3Denum>::iterator iter = m_syntheticErrors.begin();
        GC3Denum err = *iter;
        m_syntheticErrors.remove(iter);
        return err;
    }

    makeContextCurrent();
    return GL_CMD(glGetError());
}

void GraphicsContext3DInternal::getFloatv(GC3Denum pname, GC3Dfloat* value)
{
    makeContextCurrent();
    GL_CMD(glGetFloatv(pname, value));
}

void GraphicsContext3DInternal::getFramebufferAttachmentParameteriv(GC3Denum target, GC3Denum attachment, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    if (attachment == GraphicsContext3D::DEPTH_STENCIL_ATTACHMENT)
        attachment = GraphicsContext3D::DEPTH_ATTACHMENT; // Or STENCIL_ATTACHMENT, either works.
    GL_CMD(glGetFramebufferAttachmentParameteriv(target, attachment, pname, value));
}

void GraphicsContext3DInternal::getIntegerv(GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();

    switch (pname) {
    case GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS:
        GL_CMD(glGetIntegerv(GL_MAX_FRAGMENT_UNIFORM_VECTORS, value));
        *value /= 4;
        break;
    case GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS:
        GL_CMD(glGetIntegerv(GL_MAX_VERTEX_UNIFORM_VECTORS, value));
        *value /= 4;
        break;
    case GraphicsContext3D::MAX_VARYING_VECTORS:
        GL_CMD(glGetIntegerv(GL_MAX_VARYING_VECTORS, value));
        *value /= 4;
        break;
    default:
        GL_CMD(glGetIntegerv(pname, value));
    }
}

void GraphicsContext3DInternal::getProgramiv(Platform3DObject program, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    GL_CMD(glGetProgramiv(program, pname, value));
}

String GraphicsContext3DInternal::getProgramInfoLog(Platform3DObject program)
{
    makeContextCurrent();

    int logLength = 0;
    GL_CMD(glGetProgramiv(program, GraphicsContext3D::INFO_LOG_LENGTH, &logLength));
    if (!logLength)
        return String();

    OwnArrayPtr<char> log = adoptArrayPtr(new char[logLength]);
    if (!log)
        return String();

    int returnedLogLength = 0;
    GL_CMD(glGetProgramInfoLog(program, logLength, &returnedLogLength, log.get()));
    ASSERT(logLength == returnedLogLength + 1);

    String res = String::fromUTF8(log.get(), returnedLogLength);
    return res;
}

void GraphicsContext3DInternal::getRenderbufferParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    GL_CMD(glGetRenderbufferParameteriv(target, pname, value));
}

void GraphicsContext3DInternal::getShaderiv(Platform3DObject shader, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    GL_CMD(glGetShaderiv(shader, pname, value));
}

String GraphicsContext3DInternal::getShaderInfoLog(Platform3DObject shader)
{
    makeContextCurrent();

    int logLength = 0;
    GL_CMD(glGetShaderiv(shader, GraphicsContext3D::INFO_LOG_LENGTH, &logLength));
    if (logLength <= 1)
        return String();

    OwnArrayPtr<char> log = adoptArrayPtr(new char[logLength]);
    if (!log)
        return String();

    int returnedLogLength = 0;
    GL_CMD(glGetShaderInfoLog(shader, logLength, &returnedLogLength, log.get()));
    ASSERT(logLength == returnedLogLength + 1);

    String res = String::fromUTF8(log.get(), returnedLogLength);
    return res;
}

void GraphicsContext3DInternal::getShaderPrecisionFormat(GC3Denum shaderType, GC3Denum precisionType, GC3Dint* range, GC3Dint* precision)
{
    ASSERT(range);
    ASSERT(precision);

    makeContextCurrent();
    GL_CMD(glGetShaderPrecisionFormat(shaderType, precisionType, range, precision));
}

String GraphicsContext3DInternal::getShaderSource(Platform3DObject shader)
{
    makeContextCurrent();

    int logLength = 0;
    GL_CMD(glGetShaderiv(shader, GL_SHADER_SOURCE_LENGTH, &logLength));
    if (logLength <= 1)
        return String();

    OwnArrayPtr<char> log = adoptArrayPtr(new char[logLength]);
    if (!log)
        return String();

    int returnedLogLength = 0;
    GL_CMD(glGetShaderSource(shader, logLength, &returnedLogLength, log.get()));
    ASSERT(logLength == returnedLogLength + 1);

    String res = String::fromUTF8(log.get(), returnedLogLength);
    return res;
}

String GraphicsContext3DInternal::getString(GC3Denum name)
{
    makeContextCurrent();
    return String(reinterpret_cast<const char*>(GL_CMD(glGetString(name))));
}

void GraphicsContext3DInternal::getTexParameterfv(GC3Denum target, GC3Denum pname, GC3Dfloat* value)
{
    makeContextCurrent();
    GL_CMD(glGetTexParameterfv(target, pname, value));
}

void GraphicsContext3DInternal::getTexParameteriv(GC3Denum target, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    GL_CMD(glGetTexParameteriv(target, pname, value));
}

void GraphicsContext3DInternal::getUniformfv(Platform3DObject program, GC3Dint location, GC3Dfloat* value)
{
    makeContextCurrent();
    GL_CMD(glGetUniformfv(program, location, value));
}

void GraphicsContext3DInternal::getUniformiv(Platform3DObject program, GC3Dint location, GC3Dint* value)
{
    makeContextCurrent();
    GL_CMD(glGetUniformiv(program, location, value));
}

GC3Dint GraphicsContext3DInternal::getUniformLocation(Platform3DObject program, const String& name)
{
    makeContextCurrent();
    return GL_CMD(glGetUniformLocation(program, name.utf8().data()));
}

void GraphicsContext3DInternal::getVertexAttribfv(GC3Duint index, GC3Denum pname, GC3Dfloat* value)
{
    makeContextCurrent();
    GL_CMD(glGetVertexAttribfv(index, pname, value));
}

void GraphicsContext3DInternal::getVertexAttribiv(GC3Duint index, GC3Denum pname, GC3Dint* value)
{
    makeContextCurrent();
    GL_CMD(glGetVertexAttribiv(index, pname, value));
}

GC3Dsizeiptr GraphicsContext3DInternal::getVertexAttribOffset(GC3Duint index, GC3Denum pname)
{
    makeContextCurrent();
    void* pointer;
    GL_CMD(glGetVertexAttribPointerv(index, pname, &pointer));
    return reinterpret_cast<GC3Dsizeiptr>(pointer);
}

void GraphicsContext3DInternal::hint(GC3Denum target, GC3Denum mode)
{
    makeContextCurrent();
    GL_CMD(glHint(target, mode));
}

GC3Dboolean GraphicsContext3DInternal::isBuffer(Platform3DObject buffer)
{
    if (!buffer)
        return GL_FALSE;

    makeContextCurrent();
    return GL_CMD(glIsBuffer(buffer));
}

GC3Dboolean GraphicsContext3DInternal::isEnabled(GC3Denum cap)
{
    makeContextCurrent();
    return GL_CMD(glIsEnabled(cap));
}

GC3Dboolean GraphicsContext3DInternal::isFramebuffer(Platform3DObject framebuffer)
{
    if (!framebuffer)
        return GL_FALSE;

    makeContextCurrent();
    return GL_CMD(glIsFramebuffer(framebuffer));
}

GC3Dboolean GraphicsContext3DInternal::isProgram(Platform3DObject program)
{
    if (!program)
        return GL_FALSE;

    makeContextCurrent();
    return GL_CMD(glIsProgram(program));
}

GC3Dboolean GraphicsContext3DInternal::isRenderbuffer(Platform3DObject renderbuffer)
{
    if (!renderbuffer)
        return GL_FALSE;

    makeContextCurrent();
    return GL_CMD(glIsRenderbuffer(renderbuffer));
}

GC3Dboolean GraphicsContext3DInternal::isShader(Platform3DObject shader)
{
    if (!shader)
        return GL_FALSE;

    makeContextCurrent();
    return GL_CMD(glIsShader(shader));
}

GC3Dboolean GraphicsContext3DInternal::isTexture(Platform3DObject texture)
{
    if (!texture)
        return GL_FALSE;

    makeContextCurrent();
    return GL_CMD(glIsTexture(texture));
}

void GraphicsContext3DInternal::lineWidth(GC3Dfloat width)
{
    makeContextCurrent();
    GL_CMD(glLineWidth(width));
}

void GraphicsContext3DInternal::linkProgram(Platform3DObject program)
{
    makeContextCurrent();
    GL_CMD(glLinkProgram(program));
}

void GraphicsContext3DInternal::pixelStorei(GC3Denum pname, GC3Dint param)
{
    makeContextCurrent();
    GL_CMD(glPixelStorei(pname, param));
}

void GraphicsContext3DInternal::polygonOffset(GC3Dfloat factor, GC3Dfloat units)
{
    makeContextCurrent();
    GL_CMD(glPolygonOffset(factor, units));
}

void GraphicsContext3DInternal::readPixels(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data)
{
    makeContextCurrent();

    GL_CMD(glFlush());
    GL_CMD(glReadPixels(x, y, width, height, format, type, data));
}

void GraphicsContext3DInternal::releaseShaderCompiler()
{
    makeContextCurrent();
}

void GraphicsContext3DInternal::renderbufferStorage(GC3Denum target, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    GL_CMD(glRenderbufferStorage(target, internalformat, width, height));
}

void GraphicsContext3DInternal::sampleCoverage(GC3Dclampf value, GC3Dboolean invert)
{
    makeContextCurrent();
    GL_CMD(glSampleCoverage(value, invert));
}

void GraphicsContext3DInternal::scissor(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    GL_CMD(glScissor(x, y, width, height));
}

void GraphicsContext3DInternal::shaderSource(Platform3DObject shader, const String& string)
{
    makeContextCurrent();
    CString sourceCS = string.utf8();
    const char* str = sourceCS.data();
    int length = string.length();
    GL_CMD(glShaderSource(shader, 1, &str, &length));
}

void GraphicsContext3DInternal::stencilFunc(GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    makeContextCurrent();
    GL_CMD(glStencilFunc(func, ref, mask));
}

void GraphicsContext3DInternal::stencilFuncSeparate(GC3Denum face, GC3Denum func, GC3Dint ref, GC3Duint mask)
{
    makeContextCurrent();
    GL_CMD(glStencilFuncSeparate(face, func, ref, mask));
}

void GraphicsContext3DInternal::stencilMask(GC3Duint mask)
{
    makeContextCurrent();
    GL_CMD(glStencilMask(mask));
}

void GraphicsContext3DInternal::stencilMaskSeparate(GC3Denum face, GC3Duint mask)
{
    makeContextCurrent();
    GL_CMD(glStencilMaskSeparate(face, mask));
}

void GraphicsContext3DInternal::stencilOp(GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    makeContextCurrent();
    GL_CMD(glStencilOp(fail, zfail, zpass));
}

void GraphicsContext3DInternal::stencilOpSeparate(GC3Denum face, GC3Denum fail, GC3Denum zfail, GC3Denum zpass)
{
    makeContextCurrent();
    GL_CMD(glStencilOpSeparate(face, fail, zfail, zpass));
}

bool GraphicsContext3DInternal::texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels)
{
    makeContextCurrent();
    GL_CMD(glTexImage2D(target, level, internalformat, width, height, border, format, type, pixels));
    return true;
}

void GraphicsContext3DInternal::texParameterf(GC3Denum target, GC3Denum pname, GC3Dfloat param)
{
    makeContextCurrent();
    GL_CMD(glTexParameterf(target, pname, param));
}

void GraphicsContext3DInternal::texParameteri(GC3Denum target, GC3Denum pname, GC3Dint param)
{
    makeContextCurrent();
    GL_CMD(glTexParameteri(target, pname, param));
}

void GraphicsContext3DInternal::texSubImage2D(GC3Denum target, GC3Dint level, GC3Dint xoffset, GC3Dint yoffset, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, const void* pixels)
{
    makeContextCurrent();
    GL_CMD(glTexSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels));
}

void GraphicsContext3DInternal::uniform1f(GC3Dint location, GC3Dfloat x)
{
    makeContextCurrent();
    GL_CMD(glUniform1f(location, x));
}

void GraphicsContext3DInternal::uniform1fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* v)
{
    makeContextCurrent();
    GL_CMD(glUniform1fv(location, size, v));
}

void GraphicsContext3DInternal::uniform1i(GC3Dint location, GC3Dint x)
{
    makeContextCurrent();
    GL_CMD(glUniform1i(location, x));
}

void GraphicsContext3DInternal::uniform1iv(GC3Dint location, GC3Dsizei size, GC3Dint* v)
{
    makeContextCurrent();
    GL_CMD(glUniform1iv(location, size, v));
}

void GraphicsContext3DInternal::uniform2f(GC3Dint location, GC3Dfloat x, GC3Dfloat y)
{
    makeContextCurrent();
    GL_CMD(glUniform2f(location, x, y));
}

void GraphicsContext3DInternal::uniform2fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* v)
{
    makeContextCurrent();
    GL_CMD(glUniform2fv(location, size, v));
}

void GraphicsContext3DInternal::uniform2i(GC3Dint location, GC3Dint x, GC3Dint y)
{
    makeContextCurrent();
    GL_CMD(glUniform2i(location, x, y));
}

void GraphicsContext3DInternal::uniform2iv(GC3Dint location, GC3Dsizei size, GC3Dint* v)
{
    makeContextCurrent();
    GL_CMD(glUniform2iv(location, size, v));
}

void GraphicsContext3DInternal::uniform3f(GC3Dint location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z)
{
    makeContextCurrent();
    GL_CMD(glUniform3f(location, x, y, z));
}

void GraphicsContext3DInternal::uniform3fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* v)
{
    makeContextCurrent();
    GL_CMD(glUniform3fv(location, size, v));
}

void GraphicsContext3DInternal::uniform3i(GC3Dint location, GC3Dint x, GC3Dint y, GC3Dint z)
{
    makeContextCurrent();
    GL_CMD(glUniform3i(location, x, y, z));
}

void GraphicsContext3DInternal::uniform3iv(GC3Dint location, GC3Dsizei size, GC3Dint* v)
{
    makeContextCurrent();
    GL_CMD(glUniform3iv(location, size, v));
}

void GraphicsContext3DInternal::uniform4f(GC3Dint location, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w)
{
    makeContextCurrent();
    GL_CMD(glUniform4f(location, x, y, z, w));
}

void GraphicsContext3DInternal::uniform4fv(GC3Dint location, GC3Dsizei size, GC3Dfloat* v)
{
    makeContextCurrent();
    GL_CMD(glUniform4fv(location, size, v));
}

void GraphicsContext3DInternal::uniform4i(GC3Dint location, GC3Dint x, GC3Dint y, GC3Dint z, GC3Dint w)
{
    makeContextCurrent();
    GL_CMD(glUniform4i(location, x, y, z, w));
}

void GraphicsContext3DInternal::uniform4iv(GC3Dint location, GC3Dsizei size, GC3Dint* v)
{
    makeContextCurrent();
    GL_CMD(glUniform4iv(location, size, v));
}

void GraphicsContext3DInternal::uniformMatrix2fv(GC3Dint location, GC3Dsizei size, GC3Dboolean transpose, GC3Dfloat* value)
{
    makeContextCurrent();
    GL_CMD(glUniformMatrix2fv(location, size, transpose, value));
}

void GraphicsContext3DInternal::uniformMatrix3fv(GC3Dint location, GC3Dsizei size, GC3Dboolean transpose, GC3Dfloat* value)
{
    makeContextCurrent();
    GL_CMD(glUniformMatrix3fv(location, size, transpose, value));
}

void GraphicsContext3DInternal::uniformMatrix4fv(GC3Dint location, GC3Dsizei size, GC3Dboolean transpose, GC3Dfloat* value)
{
    makeContextCurrent();
    GL_CMD(glUniformMatrix4fv(location, size, transpose, value));
}

void GraphicsContext3DInternal::useProgram(Platform3DObject program)
{
    makeContextCurrent();
    GL_CMD(glUseProgram(program));
}

void GraphicsContext3DInternal::validateProgram(Platform3DObject program)
{
    makeContextCurrent();
    GL_CMD(glValidateProgram(program));
}

void GraphicsContext3DInternal::vertexAttrib1f(GC3Duint index, GC3Dfloat x)
{
    makeContextCurrent();
    GL_CMD(glVertexAttrib1f(index, x));
}

void GraphicsContext3DInternal::vertexAttrib1fv(GC3Duint index, GC3Dfloat* values)
{
    makeContextCurrent();
    GL_CMD(glVertexAttrib1fv(index, values));
}

void GraphicsContext3DInternal::vertexAttrib2f(GC3Duint index, GC3Dfloat x, GC3Dfloat y)
{
    makeContextCurrent();
    GL_CMD(glVertexAttrib2f(index, x, y));
}

void GraphicsContext3DInternal::vertexAttrib2fv(GC3Duint index, GC3Dfloat* values)
{
    makeContextCurrent();
    GL_CMD(glVertexAttrib2fv(index, values));
}

void GraphicsContext3DInternal::vertexAttrib3f(GC3Duint index, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z)
{
    makeContextCurrent();
    GL_CMD(glVertexAttrib3f(index, x, y, z));
}

void GraphicsContext3DInternal::vertexAttrib3fv(GC3Duint index, GC3Dfloat* values)
{
    makeContextCurrent();
    GL_CMD(glVertexAttrib3fv(index, values));
}

void GraphicsContext3DInternal::vertexAttrib4f(GC3Duint index, GC3Dfloat x, GC3Dfloat y, GC3Dfloat z, GC3Dfloat w)
{
    makeContextCurrent();
    GL_CMD(glVertexAttrib4f(index, x, y, z, w));
}

void GraphicsContext3DInternal::vertexAttrib4fv(GC3Duint index, GC3Dfloat* values)
{
    makeContextCurrent();
    GL_CMD(glVertexAttrib4fv(index, values));
}

void GraphicsContext3DInternal::vertexAttribPointer(GC3Duint index, GC3Dint size, GC3Denum type, GC3Dboolean normalized, GC3Dsizei stride, GC3Dintptr offset)
{
    makeContextCurrent();
    GL_CMD(glVertexAttribPointer(index, size, type, normalized, stride, reinterpret_cast<GLvoid*>(static_cast<intptr_t>(offset))));
}

void GraphicsContext3DInternal::viewport(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height)
{
    makeContextCurrent();
    GL_CMD(glViewport(x, y, width, height));
}

Platform3DObject GraphicsContext3DInternal::createBuffer()
{
    makeContextCurrent();
    Platform3DObject o = 0;
    GL_CMD(glGenBuffers(1, &o));
    return o;
}

Platform3DObject GraphicsContext3DInternal::createFramebuffer()
{
    makeContextCurrent();
    Platform3DObject o = 0;
    GL_CMD(glGenFramebuffers(1, &o));
    return o;
}

Platform3DObject GraphicsContext3DInternal::createProgram()
{
    makeContextCurrent();
    return GL_CMD(glCreateProgram());
}

Platform3DObject GraphicsContext3DInternal::createRenderbuffer()
{
    makeContextCurrent();
    Platform3DObject o;
    GL_CMD(glGenRenderbuffers(1, &o));
    return o;
}

Platform3DObject GraphicsContext3DInternal::createShader(GC3Denum shaderType)
{
    makeContextCurrent();
    return GL_CMD(glCreateShader(shaderType));
}

Platform3DObject GraphicsContext3DInternal::createTexture()
{
    makeContextCurrent();
    Platform3DObject o;
    GL_CMD(glGenTextures(1, &o));
    return o;
}

void GraphicsContext3DInternal::deleteBuffer(Platform3DObject buffer)
{
    makeContextCurrent();
    GL_CMD(glDeleteBuffers(1, &buffer));
}

void GraphicsContext3DInternal::deleteFramebuffer(Platform3DObject framebuffer)
{
    makeContextCurrent();
    GL_CMD(glDeleteFramebuffers(1, &framebuffer));
}

void GraphicsContext3DInternal::deleteProgram(Platform3DObject program)
{
    makeContextCurrent();
    GL_CMD(glDeleteProgram(program));
}

void GraphicsContext3DInternal::deleteRenderbuffer(Platform3DObject renderbuffer)
{
    makeContextCurrent();
    GL_CMD(glDeleteRenderbuffers(1, &renderbuffer));
}

void GraphicsContext3DInternal::deleteShader(Platform3DObject shader)
{
    makeContextCurrent();
    GL_CMD(glDeleteShader(shader));
}

void GraphicsContext3DInternal::deleteTexture(Platform3DObject texture)
{
    makeContextCurrent();
    GL_CMD(glDeleteTextures(1, &texture));
}

} // namespace WebCore


#endif
#endif
