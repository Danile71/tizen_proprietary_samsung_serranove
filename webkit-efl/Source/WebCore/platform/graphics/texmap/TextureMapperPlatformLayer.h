/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef TextureMapperPlatformLayer_h
#define TextureMapperPlatformLayer_h

#include "TransformationMatrix.h"

namespace WebCore {

class TextureMapper;
class BitmapTexture;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
class Image;
class CanvasRenderingContext2D;
#endif

#if ENABLE(TIZEN_INHERIT_REF_COUNT_TO_TEXTURE_MAPPER_LAYER)
class TextureMapperPlatformLayer : public ThreadSafeRefCounted<TextureMapperPlatformLayer> {
#else
class TextureMapperPlatformLayer {
#endif
public:
    virtual ~TextureMapperPlatformLayer() { }
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect&, const TransformationMatrix& modelViewMatrix = TransformationMatrix(), float opacity = 1.0) = 0;
    virtual void swapBuffers() { }
#if USE(GRAPHICS_SURFACE) || ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    virtual uint32_t copyToGraphicsSurface() { return 0; }
    virtual uint64_t graphicsSurfaceToken() const { return 0; }
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    virtual int graphicsSurfaceFlags() const { return 0; }
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    virtual bool swapPlatformSurfaces() { return false; }
    virtual void freePlatformSurface(int id) { }
    virtual void removePlatformSurface(int id) { }
#endif
#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
    virtual void flushPlatformSurfaces() { }
    virtual void flushSurface() { }
    virtual void resetSurface() { }
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    virtual void increaseDrawCount() { }
#endif

#endif
    virtual void setContext(CanvasRenderingContext2D* context) { }
    virtual int contentType() { return -1; }
    virtual void setContentsOpaque(const bool) { }
#endif
};

};

#endif // TextureMapperPlatformLayer_h
