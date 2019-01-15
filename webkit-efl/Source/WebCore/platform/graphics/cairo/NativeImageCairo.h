/*
 * Copyright (c) 2008, Google Inc. All rights reserved.
 * Copyright (c) 2012, Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#ifndef NativeImageCairo_h
#define NativeImageCairo_h

#include "RefPtrCairo.h"

namespace WebCore {

class NativeImageCairo {
public:

    typedef enum _native_image_type {
        NATIVE_NONE_SURFACE,
        NATIVE_IMAGE_SURFACE,
        NATIVE_GL_SURFACE
    } native_image_type;

    NativeImageCairo();
    ~NativeImageCairo();
    explicit NativeImageCairo(cairo_surface_t*);
    cairo_surface_t* surface () { return m_surface.get(); }
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    cairo_surface_t* glsurface() { return m_glsurface.get(); }
    void setGLSurface(cairo_surface_t* surface);
    void setImageSurface(cairo_surface_t* surface);
    void setImageFrame(void* imageframe);
    void* imageFrame() { return m_imageframe; }
    void addImageSurfaceFromData();
    void addGLSurfaceFromData();
    native_image_type surfaceType() { return m_surfaceType; }
#endif

private:
    RefPtr<cairo_surface_t> m_surface;
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    RefPtr<cairo_surface_t> m_glsurface;
    void* m_imageframe;
    native_image_type m_surfaceType;
#endif
};

}
#endif // NativeImageCairo_h

