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

#ifndef GraphicsContext3DOffscreen_h
#define GraphicsContext3DOffscreen_h

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)

#include "GraphicsContext3DInternal.h"
#if ENABLE(TIZEN_WEBKIT2)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "SharedPlatformSurfaceTizen.h"
#include "PixmapContextTizen.h"
#else
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif
#endif
#if USE(TIZEN_TEXTURE_MAPPER)
#include "TextureMapperPlatformLayer.h"
#endif
#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
#include "GraphicsSurface.h"
#endif

#if ENABLE(TIZEN_BROWSER_WEBGL_DASH_MODE)
#include "Timer.h"
#endif

namespace WebCore {

class GraphicsContext3DOffscreen : public GraphicsContext3DInternal
#if USE(TIZEN_TEXTURE_MAPPER)
                                 , public TextureMapperPlatformLayer
#endif
{
public:
    static PassOwnPtr<GraphicsContext3DOffscreen> create(GraphicsContext3D::Attributes, HostWindow*);
    virtual ~GraphicsContext3DOffscreen();

    virtual Platform3DObject platformTexture() const { return m_compositorTexture; }
#if USE(ACCELERATED_COMPOSITING)
    virtual PlatformLayer* platformLayer() const;
#endif

#if ENABLE(TIZEN_WEBGL_ANIMATION_SNAPSHOT_FIX)
    virtual void paintRenderingResultsToCanvas(ImageBuffer*, int, int, bool isForcePaint = false);
#else
    virtual void paintRenderingResultsToCanvas(ImageBuffer*, int, int);
#endif

#if ENABLE(TIZEN_WEBKIT2)
    PlatformGraphicsContext3D platformGraphicsContext3D() const;
#endif

    virtual PassRefPtr<ImageData> paintRenderingResultsToImageData(int width, int height);
    virtual bool paintsIntoCanvasBuffer() const { return !m_acceleratedCompositingEnabled; }

#if USE(TIZEN_TEXTURE_MAPPER)
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect& target, const TransformationMatrix&, float opacity);
#if ENABLE(TIZEN_WEBKIT2)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    virtual bool swapPlatformSurfaces();
    virtual void freePlatformSurface(int);
    virtual void removePlatformSurface(int);
#endif
#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    virtual uint32_t copyToGraphicsSurface();
    virtual uint64_t graphicsSurfaceToken() const; // used to be platformSurfaceID()
    virtual int graphicsSurfaceFlags() const;
    void createGraphicsSurfaces(const IntSize&);
#endif
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2)
    virtual bool makeContextCurrent();
#endif
    virtual void prepareTexture(int width, int height);

    virtual void reshape(int width, int height);
    virtual void readRenderingResults(unsigned char* pixels, int pixelsSize, int width, int height);
    virtual void markContextChanged() { m_layerComposited = false; }
    virtual void markLayerComposited() { m_layerComposited = true; }
    virtual bool layerComposited() const { return m_layerComposited; }

    virtual void activeTexture(GC3Denum texture);
    virtual void bindFramebuffer(GC3Denum target, Platform3DObject);
    virtual void bindTexture(GC3Denum target, Platform3DObject texture);
    virtual void bufferData(GC3Denum target, GC3Dsizeiptr, const void* data, GC3Denum usage);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    virtual void framebufferTexture2D(GC3Denum target, GC3Denum attachment, GC3Denum textarget, Platform3DObject texture, GC3Dint level);
    virtual void generateMipmap(GC3Denum target);
#endif
    virtual bool getActiveAttrib(Platform3DObject program, GC3Duint index, ActiveInfo&);
    virtual bool getActiveUniform(Platform3DObject program, GC3Duint index, ActiveInfo&);
    virtual void getFloatv(GC3Denum pname, GC3Dfloat* value);
    virtual void getIntegerv(GC3Denum pname, GC3Dint* value);

    virtual bool texImage2D(GC3Denum target, GC3Dint level, GC3Denum internalformat, GC3Dsizei width, GC3Dsizei height, GC3Dint border, GC3Denum format, GC3Denum type, const void* pixels);

    virtual bool paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight,  int canvasWidth, int canvasHeight, PlatformContextCairo* context);
private:
    GraphicsContext3DOffscreen(GraphicsContext3D::Attributes attrs);

    virtual bool initialize(HostWindow* hostWindow);
#if !ENABLE(TIZEN_WEBKIT2)
    virtual bool createContext(Evas_Object* view);
    virtual bool createSurface(Evas_Object* view);
#endif

#if USE(ACCELERATED_COMPOSITING)
#if !USE(TIZEN_TEXTURE_MAPPER)
    RefPtr<EflLayer> m_compositingLayer;
#endif
#endif
    bool m_acceleratedCompositingEnabled;
    bool m_layerComposited;

    Platform3DObject m_boundTexture0;
    Platform3DObject m_boundFBO;

    GLuint m_activeTexture;
    GLuint m_compositorTexture;
    GLuint m_depthBuffer;
    GLuint m_stencilBuffer;
    GLuint m_fbo;
    GLuint m_internalColorFormat;
    GLuint m_texture;
#if ENABLE(TIZEN_WEBKIT2)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    class PlatformContextInfo : public RefCounted<PlatformContextInfo> {
    public :
        PlatformContextInfo(bool hasAlpha, bool hasDepth, bool hasStencil)
        {
            m_pixmapContext = PixmapContextTizen::create(false, hasAlpha, hasDepth, hasStencil);
        };

        ~PlatformContextInfo()
        {
        };

        RefPtr<PixmapContextTizen> m_pixmapContext;
    };

    class PlatformSurfaceInfo : public RefCounted<PlatformSurfaceInfo> {
    public :
        PlatformSurfaceInfo(const IntSize& size, bool hasAlpha, bool hasDepth, bool hasStencil, PixmapContextTizen* platformContext)
            : m_used(false)
            , m_size(size)
        {
            m_platformSurface = SharedPlatformSurfaceTizen::create(size, false, hasAlpha, hasDepth, hasStencil, false, platformContext);
        };

        ~PlatformSurfaceInfo()
        {
            m_platformSurface.release();
        };

        OwnPtr<SharedPlatformSurfaceTizen> m_platformSurface;
        bool m_used;
        IntSize m_size;
    };

    SharedPlatformSurfaceTizen* getFreePlatformSurface();
    typedef HashMap<int, RefPtr<PlatformSurfaceInfo> > PlatformSurfaceMap;
    PlatformSurfaceMap m_platformSurfaces;
    SharedPlatformSurfaceTizen* m_currentPlatformSurface;
    SharedPlatformSurfaceTizen* m_previousPlatformSurface;
    int m_maxPlatformSurface;

    RefPtr<PlatformContextInfo> m_platformContext;

    HashSet<Platform3DObject> m_renderTargetTextures;
#else
    EGLDisplay m_display;
    EGLContext m_context;
    EGLSurface m_surface;
    EGLConfig m_surfaceConfig;
    int m_platformSurfaceID;
#endif
#if ENABLE(TIZEN_CANVAS_GRAPHICS_SURFACE)
    GraphicsSurface::Flags m_surfaceFlags;
    RefPtr<GraphicsSurface> m_graphicsSurface;
#endif
#endif

//Angle intgration block
#if ENABLE(TIZEN_WEBGL_ANGLE_INTEGRATION)
    virtual void compileShader(Platform3DObject shader);
    virtual void getShaderiv(Platform3DObject shader, GC3Denum pname, GC3Dint* value);
    virtual String getShaderInfoLog(Platform3DObject shader);
    virtual String getShaderSource(Platform3DObject shader);
    virtual void shaderSource(Platform3DObject shader, const String& string);

    typedef struct {
        String source;
        String log;
        bool isValid;
    } ShaderSourceEntry;
    HashMap<Platform3DObject, ShaderSourceEntry> m_shaderSourceMap;
#endif
//Angle intgration block

#if ENABLE(TIZEN_BROWSER_WEBGL_DASH_MODE)
    void webglDashModeFired(Timer<GraphicsContext3DOffscreen>*) { };
    Timer<GraphicsContext3DOffscreen> m_webglDashModeTimer;
#endif

};

} // namespace WebCore

#endif

#endif // GraphicsContext3DOffscreen_h
