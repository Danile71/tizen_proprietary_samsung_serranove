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

#ifndef RecordingSurfaceSetCairo_h
#define RecordingSurfaceSetCairo_h

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)

#include <WebCore/Region.h>
#include <cairo.h>

namespace WebCore {
class Region;
class IntRect;
class IntSize;
}

namespace WebKit {

class RecordingSurfaceSet {

public:
    RecordingSurfaceSet();
    ~RecordingSurfaceSet();
    void add(const WebCore::IntRect&, const float scale);
    bool draw(cairo_t* cairoContext, const WebCore::IntRect&);
    void clear();
    void setRecordingSurface(size_t, cairo_surface_t* recordingSurfaceCairo, cairo_t* recordingContextCairo, WebCore::IntSize tileSize);
    const WebCore::IntRect& bounds(size_t i) const { return m_recordingSurfaces[i].m_area; }
    bool upToDate(size_t i) const { return m_recordingSurfaces[i].m_recordingSurface != 0; }
    size_t size() const { return m_recordingSurfaces.size(); }

private:
    struct RecordingSurface {
        WebCore::IntRect m_area;
        WebCore::IntSize m_tileSize;
        cairo_surface_t* m_recordingSurface;
        cairo_t* m_recordingContext;
        float m_scale;
        bool m_base : 8;
    };

    float m_baseArea;
    float m_additionalArea;
    void add(const RecordingSurface* temp);
    WTF::Vector<RecordingSurface> m_recordingSurfaces;
};

} // namespace WebKit
#endif
#endif
