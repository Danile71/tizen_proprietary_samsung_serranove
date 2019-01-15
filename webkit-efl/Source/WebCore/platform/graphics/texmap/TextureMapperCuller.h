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

#ifndef TextureMapperCuller_h
#define TextureMapperCuller_h

#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)

#include "FloatRect.h"
#include "IntRect.h"
#include "TransformationMatrix.h"

#include <wtf/HashMap.h>

namespace WebCore {
class TextureMapperLayer;

typedef Vector<std::pair<TextureMapperLayer*, IntRect> > OpaqueRectsVector;
typedef HashMap<TextureMapperLayer*, IntRect> TargetRectsMap;

class TextureMapperCuller {
public:
    TextureMapperCuller(const IntRect&);
    ~TextureMapperCuller();
    void setTransform(const TransformationMatrix& matrix) { m_matrix = matrix; }
    bool isOccluded(TextureMapperLayer*);
    void addTargetRect(TextureMapperLayer*, const IntRect&, bool);
    void removeOpaqueRect(const TextureMapperLayer*);
    TransformationMatrix matrix() const { return m_matrix; }
    void enterClip(const IntRect&);
    void leaveClip();
    IntRect currentClip() const { return m_currentClip; }
private:
    OpaqueRectsVector m_opaqueRectsVector;
    TargetRectsMap m_targetRectsMap;
    TransformationMatrix m_matrix;
    IntRect m_currentClip;
    Vector<IntRect > m_clipStack;
};

} // namespace WebCore

#endif // #if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)

#endif // TextureMapperCullder_h

