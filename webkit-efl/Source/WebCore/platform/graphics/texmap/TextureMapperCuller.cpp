/*
*   Copyright (C) 2012 Samsung Electronics
*
*   This library is free software; you can redistribute it and/or
*   modify it under the terms of the GNU Library General Public
*   License as published by the Free Software Foundation; either
*   version 2 of the License, or (at your option) any later version.
*
*   This library is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*   Library General Public License for more details.
*
*   You should have received a copy of the GNU Library General Public License
*   along with this library; see the file COPYING.LIB.  If not, write to
*   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
*   Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "TextureMapperCuller.h"

#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)
#include "Region.h"
#include "TextureMapperGL.h"
#include "TextureMapperLayer.h"

#include <stdio.h>

namespace WebCore {

TextureMapperCuller::TextureMapperCuller(const IntRect& rect)
    : m_currentClip(rect)
{
    m_clipStack.append(m_currentClip);
}

TextureMapperCuller::~TextureMapperCuller()
{
}

bool TextureMapperCuller::isOccluded(TextureMapperLayer* layer)
{
    TargetRectsMap::iterator it = m_targetRectsMap.find(layer);

    if (it == m_targetRectsMap.end())
        return false;

    IntRect rect = it->second;

    if (rect.isEmpty())
        return false;

    Region region(rect);

    for (size_t i = 0; i < m_opaqueRectsVector.size(); i++)
        region.subtract(m_opaqueRectsVector[i].second);

    return region.bounds().isEmpty();
}

void TextureMapperCuller::addTargetRect(TextureMapperLayer* layer, const IntRect& targetRect, bool isOpaque)
{
    if (targetRect.isEmpty())
        return;

    m_targetRectsMap.add(layer, targetRect);

    if (isOpaque)
        m_opaqueRectsVector.append(std::make_pair(layer, targetRect));
}

void TextureMapperCuller::removeOpaqueRect(const TextureMapperLayer* layer)
{
    if (m_opaqueRectsVector.size() && m_opaqueRectsVector[0].first == layer)
        m_opaqueRectsVector.remove(0);
}

void TextureMapperCuller::enterClip(const IntRect& clipRect)
{
    if (clipRect.isEmpty())
        return;

    m_currentClip.intersect(clipRect);
    m_clipStack.append(m_currentClip);
}

void TextureMapperCuller::leaveClip()
{
    if (m_clipStack.isEmpty())
        return;

    m_currentClip = m_clipStack.last();
    m_clipStack.removeLast();
}

} // namespace WebCore

#endif // #if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)

