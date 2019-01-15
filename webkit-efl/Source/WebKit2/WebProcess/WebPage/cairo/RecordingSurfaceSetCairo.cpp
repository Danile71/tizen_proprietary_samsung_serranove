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
#include "RecordingSurfaceSetCairo.h"

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)

using namespace WebCore;

namespace WebKit {

static const float maxAdditionalArea = 0.65; // Limit the additional Recording surfaces area
static const int maxAdditionalRecordingSurfaces = 32; // Limit the maximum number of Recording surfaces

RecordingSurfaceSet::RecordingSurfaceSet()
{
    m_baseArea = m_additionalArea = 0;
}

RecordingSurfaceSet::~RecordingSurfaceSet()
{
    clear();
}

void RecordingSurfaceSet::add(const WebCore::IntRect& rect, const float scale)
{
    bool makeNewBase = false;
    RecordingSurface* first = m_recordingSurfaces.begin();
    RecordingSurface* last = m_recordingSurfaces.end();

    if (!m_recordingSurfaces.size())
        makeNewBase = true;

    WebCore::IntRect area = rect;
    for (RecordingSurface* working = first; working != last; working++) {
        bool remove = false;

        if (!working->m_base && area.intersects(working->m_area)) {
            WebCore::IntRect intersectRect = area;
            intersectRect.intersect(working->m_area);
            if (working->m_area.width() * working->m_area.height() * maxAdditionalArea < intersectRect.width() * intersectRect.height())
                remove = true;
        }

        if (working->m_base) {
            WebCore::IntRect baseArea = working->m_area;
            if (area.contains(baseArea)) {
                remove = true;
                makeNewBase = true;
            }
        }

        if (remove) {
            WebCore::IntRect currentArea = working->m_area;
            if (working->m_base)
                m_baseArea -= currentArea.width() * currentArea.height();
            else
                m_additionalArea -= currentArea.width() * currentArea.height();

            area.unite(currentArea);
            working->m_area = IntRect();

            cairo_surface_destroy(working->m_recordingSurface);
            cairo_destroy(working->m_recordingContext);
            working->m_recordingSurface = 0;
            working->m_recordingContext = 0;
        }
    }

    RecordingSurface recordingSurface = {area, IntSize(0, 0), 0, 0, scale, makeNewBase};
    m_recordingSurfaces.append(recordingSurface);

    if (makeNewBase)
        m_baseArea += area.width() * area.height();
    else
        m_additionalArea += area.width() * area.height();

    last = m_recordingSurfaces.end();
    first = m_recordingSurfaces.begin();

    if ((last - first > maxAdditionalRecordingSurfaces) || (m_baseArea > 0 && (m_baseArea * maxAdditionalArea <= m_additionalArea))) {
        for (RecordingSurface* working = m_recordingSurfaces.begin(); working != m_recordingSurfaces.end(); working++) {
            if (!working->m_base)
                working->m_area = IntRect();

            cairo_surface_destroy(working->m_recordingSurface);
            cairo_destroy(working->m_recordingContext);
            working->m_recordingSurface = 0;
            working->m_recordingContext = 0;
        }
        m_additionalArea = 0;
    }

    RecordingSurface* list = first;
    for (RecordingSurface* working = first; working != last; working++) {
         if (working && working->m_area.isEmpty())
            continue;
        *list++ = *working;
    }
    m_recordingSurfaces.shrink(list - first);
}

void RecordingSurfaceSet::add(const RecordingSurface* temp)
{
    RecordingSurface recordingSurface = *temp;
    m_recordingSurfaces.append(recordingSurface);
}

void RecordingSurfaceSet::clear()
{
    RecordingSurface* last = m_recordingSurfaces.end();
    for (RecordingSurface* working = m_recordingSurfaces.begin(); working != last; working++) {
        if (working->m_recordingSurface) {
            cairo_surface_destroy(working->m_recordingSurface);
            cairo_destroy(working->m_recordingContext);
        }
    }
    m_recordingSurfaces.clear();
    m_baseArea = m_additionalArea = 0;
}

bool RecordingSurfaceSet::draw(cairo_t* cairoContext, const WebCore::IntRect& rect)
{
    RecordingSurface* first = m_recordingSurfaces.begin();
    RecordingSurface* last = m_recordingSurfaces.end();
    RecordingSurface* working;
    float scale;

    for (working = last; working != first; ) {
        --working;
        scale = working->m_scale;

        WebCore::IntRect scaledRect(static_cast<int>(working->m_area.x() * scale), static_cast<int>(working->m_area.y() * scale), static_cast<int>(working->m_area.width() * scale), static_cast<int>(working->m_area.height() * scale));
        if (scaledRect.contains(rect)) {
            first = working;
            break;
        }
    }

    for (working = first; working < last; working++) {
        cairo_save(cairoContext);
        scale = working->m_scale;

        WebCore::IntRect scaledRect(static_cast<int>(working->m_area.x() * scale), static_cast<int>(working->m_area.y() * scale), static_cast<int>(working->m_area.width() * scale), static_cast<int>(working->m_area.height() * scale));
        WebCore::IntRect intersectRect = scaledRect;

        intersectRect.intersect(rect);

        if (intersectRect.isEmpty())
            continue;

        cairo_set_operator(cairoContext, CAIRO_OPERATOR_SOURCE);
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
        cairo_translate(cairoContext, -((intersectRect.x() - scaledRect.x())) + (intersectRect.x() % working->m_tileSize.width()), -((intersectRect.y() - scaledRect.y())) + (intersectRect.y() % working->m_tileSize.height()));
#else
        cairo_translate(cairoContext, -((intersectRect.x() - scaledRect.x())), -((intersectRect.y() - scaledRect.y())));
#endif
        cairo_rectangle(cairoContext, intersectRect.x() - scaledRect.x(), intersectRect.y() - scaledRect.y(), intersectRect.width(), intersectRect.height());
        cairo_clip(cairoContext);
        cairo_set_source_surface(cairoContext, working->m_recordingSurface, 0, 0);
        cairo_paint(cairoContext);
        cairo_restore(cairoContext);
    }
    return true;
}

void RecordingSurfaceSet::setRecordingSurface(size_t index, cairo_surface_t* recordingSurfaceCairo, cairo_t* recordingContextCairo, WebCore::IntSize tileSize)
{
    m_recordingSurfaces[index].m_recordingSurface = recordingSurfaceCairo;
    m_recordingSurfaces[index].m_recordingContext = recordingContextCairo;
    m_recordingSurfaces[index].m_tileSize = tileSize;
}

} // namespace WebKit

#endif
