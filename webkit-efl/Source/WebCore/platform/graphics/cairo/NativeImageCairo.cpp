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

#include "config.h"
#include "NativeImageCairo.h"

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
#include "ImageDecoder.h"
#endif

namespace WebCore {

NativeImageCairo::NativeImageCairo()
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    : m_surface(0)
    , m_glsurface(0)
    , m_imageframe(0)
    , m_surfaceType(NATIVE_NONE_SURFACE)
#endif
{
}

NativeImageCairo::NativeImageCairo(cairo_surface_t* surface)
    : m_surface(adoptRef(surface))
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
    , m_glsurface(0)
    , m_imageframe(0)
    , m_surfaceType(NATIVE_IMAGE_SURFACE)
#endif
{
}

NativeImageCairo::~NativeImageCairo()
{
}

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
void NativeImageCairo::setGLSurface(cairo_surface_t* surface)
{
    m_glsurface = adoptRef(surface);
    m_surfaceType = NATIVE_GL_SURFACE;
}

void NativeImageCairo::setImageSurface(cairo_surface_t* surface)
{
    m_surface = adoptRef(surface);
    m_surfaceType = NATIVE_IMAGE_SURFACE;
}

void NativeImageCairo::setImageFrame(void* imageframe)
{
    m_imageframe = imageframe;
}

void NativeImageCairo::addImageSurfaceFromData()
{
    ImageFrame* imageframe = static_cast<ImageFrame*>(m_imageframe);
    imageframe->addImageSurfaceFromData(this);
}

void NativeImageCairo::addGLSurfaceFromData()
{
    ImageFrame* imageframe = static_cast<ImageFrame*>(m_imageframe);
    imageframe->addGLSurfaceFromData(this);
}
#endif

} // namespace WebCore
