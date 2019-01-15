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

#ifndef Extensions3DTizen_h
#define Extensions3DTizen_h

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)

#include "Extensions3D.h"
#include "GraphicsContext3DInternal.h"

namespace WebCore {

class Extensions3DTizen : public Extensions3D {
public:
    Extensions3DTizen(GraphicsContext3DInternal*);
    virtual ~Extensions3DTizen();
    void initialize();
    virtual bool supports(const String& name) ;

    virtual void ensureEnabled(const String& name);

    virtual bool isEnabled(const String& name);

    virtual int getGraphicsResetStatusARB() { return GraphicsContext3D::NO_ERROR; }

    virtual void blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter) {}

    virtual void renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height) {}

    virtual Platform3DObject createVertexArrayOES() { return 0; }
    virtual void deleteVertexArrayOES(Platform3DObject) {}
    virtual GC3Dboolean isVertexArrayOES(Platform3DObject) { return false; }
    virtual void bindVertexArrayOES(Platform3DObject) {}

    // GL_ANGLE_translated_shader_source
    virtual String getTranslatedShaderSourceANGLE(Platform3DObject) { return String(); }

private:
    HashSet<String> m_availableExtensions;
    GraphicsContext3DInternal* m_context;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING)
#endif // Extensions3D_h

