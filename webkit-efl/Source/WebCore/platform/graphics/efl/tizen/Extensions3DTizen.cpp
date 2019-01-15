/*
* Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)

#include "Extensions3DTizen.h"
#include "GraphicsContext3D.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#if USE(EGL_IMAGE_EXT)
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#endif

#include <wtf/text/CString.h>
#include "GraphicsContext3DInternal.h"

namespace WebCore {

Extensions3DTizen::Extensions3DTizen(GraphicsContext3DInternal* context)
    : m_context(context)
{
    initialize();
}

Extensions3DTizen::~Extensions3DTizen()
{
    m_availableExtensions.clear();
}

bool Extensions3DTizen::supports(const String& name)
{
    return m_availableExtensions.contains(name);
}

void Extensions3DTizen::ensureEnabled(const String& name)
{
#if ENABLE(TIZEN_WEBGL_ANGLE_INTEGRATION)
    if (name == "GL_OES_standard_derivatives") {
        // Enable support in ANGLE (if not enabled already)
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        ShBuiltInResources ANGLEResources = compiler.getResources();
        if (!ANGLEResources.OES_standard_derivatives) {
            ANGLEResources.OES_standard_derivatives = 1;
            compiler.setResources(ANGLEResources);
        }
    }
#endif
}

bool Extensions3DTizen::isEnabled(const String& name)
{
#if ENABLE(TIZEN_WEBGL_ANGLE_INTEGRATION)
    if (name == "GL_OES_standard_derivatives") {
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        return compiler.getResources().OES_standard_derivatives;
    }
#endif
    return supports(name);
}

void Extensions3DTizen::initialize()
{
    String extensionsString(reinterpret_cast<const char*>(GL_CMD(glGetString(GL_EXTENSIONS))));
    Vector<String> availableExtensions;
    extensionsString.split(" ", availableExtensions);
    for (size_t i = 0; i < availableExtensions.size(); ++i)
        m_availableExtensions.add(availableExtensions[i]);
}

} // namespace WebCore

#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING)

