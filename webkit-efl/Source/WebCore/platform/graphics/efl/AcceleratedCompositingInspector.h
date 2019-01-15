/*
    Copyright (C) 2012 Samsung Electronics

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

#ifndef AcceleratedCompositingInspector_h
#define AcceleratedCompositingInspector_h

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)

#include "IntSize.h"
#include <wtf/CurrentTime.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class AcceleratedCompositingInspector {
public:
    static PassOwnPtr<AcceleratedCompositingInspector> create();
    void reset();
    void increaseDepth() { m_depth++; }
    void decreaseDepth() { m_depth--; }
    void initializeDepth() { m_depth = 0; }
    int depth() { return m_depth; }
    void increaseLayerCount() { m_layerCount++; }
    void increaseVisibleLayerCount() { m_visibleLayerCount++; }
    void increaseFrameCount();
    void addTextureSize(const IntSize& size) { m_totalTextureSize += size.width() * size.height(); }
    void printRenderingInformation();
    AcceleratedCompositingInspector();
    ~AcceleratedCompositingInspector();
    void startDrawContents();
    void endDrawContents();
    void startUpdate();
    void endUpdate();
    void startRender();
    void endRender();
    double inspectTime() { return currentTime() - m_inspectorStart; }
    bool needsDumpLayerTree() { return currentTime() - m_lastDumpLayerTreeTime > m_dumpLayerTreeInterval; }
    void initializeDumpLayerTreeTime() { m_lastDumpLayerTreeTime = currentTime(); }
    bool enabled() { return m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }
    bool dumpFullLayersEnabled() { return m_dumpFullLayersEnabled; }
    void setDumpFullLayersEnabled(bool enabled) { m_dumpFullLayersEnabled = enabled; }
private:
    int m_layerCount;
    int m_visibleLayerCount;
    int m_totalTextureSize;
    int m_frameCount;
    double m_drawContentsTime;
    double m_updateTime;
    double m_renderTime;
    double m_totalDrawContentsTime;
    double m_totalUpdateTime;
    double m_totalRenderTime;
    double m_inspectorStart;
    double m_drawContentsStart;
    double m_updateStart;
    double m_renderStart;
    double m_dumpLayerTreeInterval;
    double m_lastDumpLayerTreeTime;
    int m_depth;
    bool m_dumpFullLayersEnabled;
    bool m_enabled;
};

} // namespace WebCore

#endif

#endif // AcceleratedCompositingInspector_h
