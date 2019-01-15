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

#ifndef Extensions3DEfl_h
#define Extensions3DEfl_h

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)

#include "Extensions3D.h"
#include "GraphicsContext3D.h"

namespace WebCore {

class Extensions3DEfl : public Extensions3D {
public:
    Extensions3DEfl() {}
    virtual ~Extensions3DEfl() {}

    virtual bool supports(const String& name) { return false; }

    virtual void ensureEnabled(const String& name) {}

    virtual bool isEnabled(const String& name) { return false; }

    virtual int getGraphicsResetStatusARB() { return GraphicsContext3D::NO_ERROR; }

    virtual void blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter) {}

    virtual void renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height) {}

    virtual Platform3DObject createVertexArrayOES() { return 0; }
    virtual void deleteVertexArrayOES(Platform3DObject) {}
    virtual GC3Dboolean isVertexArrayOES(Platform3DObject) { return false; }
    virtual void bindVertexArrayOES(Platform3DObject) {}

    // GL_ANGLE_translated_shader_source
    virtual String getTranslatedShaderSourceANGLE(Platform3DObject) { return String(); }
};

} // namespace WebCore

#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING)
#endif // Extensions3D_h
