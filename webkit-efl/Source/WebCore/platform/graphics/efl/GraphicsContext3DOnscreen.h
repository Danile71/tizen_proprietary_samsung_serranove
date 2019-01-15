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

#ifndef GraphicsContext3DOnscreen_h
#define GraphicsContext3DOnscreen_h

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)

#include "GraphicsContext3DInternal.h"

namespace WebCore {

class CanvasRenderingContext;

class GraphicsContext3DOnscreen : public GraphicsContext3DInternal {
public:
    static PassOwnPtr<GraphicsContext3DOnscreen> create(GraphicsContext3D::Attributes, HostWindow*);
    virtual ~GraphicsContext3DOnscreen();

    virtual Platform3DObject platformTexture() const;
#if USE(ACCELERATED_COMPOSITING)
    virtual PlatformLayer* platformLayer() const;
#endif
    virtual void markContextChanged();
    virtual void markLayerComposited();
    virtual bool layerComposited() const;

    virtual void paintRenderingResultsToCanvas(CanvasRenderingContext*);
    virtual bool paintsIntoCanvasBuffer() const;

    virtual void prepareTexture(int width, int height);

    virtual void reshape(int width, int height);

#if ENABLE(TIZEN_EVAS_GL_DIRECT_RENDERING)
    virtual void clear(GC3Dbitfield mask);
#endif

private:
    GraphicsContext3DOnscreen(GraphicsContext3D::Attributes attrs);

    virtual bool initialize(HostWindow* hostWindow);
    virtual bool createContext(Evas_Object* view);
    virtual bool createSurface(Evas_Object* view);

    bool setSurface();

    Evas_Object* m_view;
};

} // namespace WebCore

#endif

#endif // GraphicsContext3DOnscreen_h
