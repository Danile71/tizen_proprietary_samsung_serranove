
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

#include "config.h"

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)

#include "AcceleratedCompositingInspector.h"

#include <stdio.h>

namespace WebCore {

PassOwnPtr<AcceleratedCompositingInspector> AcceleratedCompositingInspector::create()
{
    return adoptPtr(new AcceleratedCompositingInspector());
}

AcceleratedCompositingInspector::AcceleratedCompositingInspector()
     : m_totalDrawContentsTime(0)
     , m_totalUpdateTime(0)
     , m_totalRenderTime(0)
     , m_dumpLayerTreeInterval(1)
     , m_dumpFullLayersEnabled(false)
     , m_enabled(false)
{
    reset();
    initializeDumpLayerTreeTime();
}

AcceleratedCompositingInspector::~AcceleratedCompositingInspector()
{
}

void AcceleratedCompositingInspector::reset()
{
    m_layerCount = 0;
    m_visibleLayerCount = 0;
    m_totalTextureSize = 0;
    m_renderTime = 0;
    m_updateTime = 0;
    m_drawContentsTime = 0;
    m_frameCount = 0;
    m_renderStart = 0;
    m_inspectorStart = 0;
    m_drawContentsStart = 0;
}

void AcceleratedCompositingInspector::increaseFrameCount()
{
    if (!m_frameCount)
        m_inspectorStart = currentTime();

    m_frameCount++;
}

void AcceleratedCompositingInspector::printRenderingInformation()
{
    double drawContentsTimePerFrame = m_drawContentsTime / m_frameCount * 1000;
    double updateTimePerFrame = m_updateTime / m_frameCount * 1000;
    double renderTimePerFrame = m_renderTime / m_frameCount * 1000;
    double totalTextureSizePerFrame = m_totalTextureSize / m_frameCount / 1024 / 256.0; // 1024/4 = 256
    int layerCountPerFrame = m_layerCount / m_frameCount;
    int visibleLayerCountPerFrame = m_visibleLayerCount / m_frameCount;
    printf("TotalDrawContentsTime:%.2fs, TotalUpdateTime:%.2fs, TotalGPURenderTime:%.2fs\n", m_totalDrawContentsTime, m_totalUpdateTime, m_totalRenderTime);
    printf("DrawContentsTimePerFrame:%.1fms, UpdateTimePerFrame:%.1fms, GPURenderTimePerFrame:%.1fms\n", drawContentsTimePerFrame, updateTimePerFrame, renderTimePerFrame);
    printf("Layer count(total):%d, Layer count(visible):%d, TotalTextureSizePerFrame:%.2fM\n", layerCountPerFrame, visibleLayerCountPerFrame, totalTextureSizePerFrame);
}

void AcceleratedCompositingInspector::startDrawContents()
{
    m_drawContentsStart = currentTime();
}

void AcceleratedCompositingInspector::endDrawContents()
{
    m_drawContentsTime += currentTime() - m_drawContentsStart;
    m_totalDrawContentsTime += currentTime() - m_drawContentsStart;
}

void AcceleratedCompositingInspector::startUpdate()
{
    m_updateStart = currentTime();
}

void AcceleratedCompositingInspector::endUpdate()
{
    m_updateTime += currentTime() - m_updateStart;
    m_totalUpdateTime += currentTime() - m_updateStart;
}

void AcceleratedCompositingInspector::startRender()
{
    m_renderStart = currentTime();
}

void AcceleratedCompositingInspector::endRender()
{
    m_renderTime += currentTime() - m_renderStart;
    m_totalRenderTime += currentTime() - m_renderStart;
}

} // namespace WebCore

#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING)
