/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2008 David Smith <catfish.man@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef ElementRareData_h
#define ElementRareData_h

#include "ClassList.h"
#include "DatasetDOMStringMap.h"
#include "Element.h"
#include "ElementShadow.h"
#include "HTMLCollection.h"
#include "NamedNodeMap.h"
#include "NodeRareData.h"
#include "UndoManager.h"
#include "PseudoElement.h"
#include <wtf/OwnPtr.h>

namespace WebCore {

class HTMLCollection;

class ElementRareData : public NodeRareData {
public:
    ElementRareData();
    virtual ~ElementRareData();

    void setPseudoElement(PseudoId, PassRefPtr<PseudoElement>);
    PseudoElement* pseudoElement(PseudoId) const;

    void resetComputedStyle();

    using NodeRareData::needsFocusAppearanceUpdateSoonAfterAttach;
    using NodeRareData::setNeedsFocusAppearanceUpdateSoonAfterAttach;

    bool hasCachedHTMLCollections() const
    {
        return m_cachedCollections;
    }

    PassRefPtr<HTMLCollection> ensureCachedHTMLCollection(Element*, CollectionType);
    HTMLCollection* cachedHTMLCollection(CollectionType type)
    {
        if (!m_cachedCollections)
            return 0;

        return (*m_cachedCollections)[type - FirstNodeCollectionType];
    }

    void removeCachedHTMLCollection(HTMLCollection* collection, CollectionType type)
    {
        ASSERT(m_cachedCollections);
        ASSERT_UNUSED(collection, (*m_cachedCollections)[type - FirstNodeCollectionType] == collection);
        (*m_cachedCollections)[type - FirstNodeCollectionType] = 0;
    }

    void clearHTMLCollectionCaches(const QualifiedName* attrName)
    {
        if (!m_cachedCollections)
            return;

        bool shouldIgnoreType = !attrName || *attrName == HTMLNames::idAttr || *attrName == HTMLNames::nameAttr;

        for (unsigned i = 0; i < (*m_cachedCollections).size(); i++) {
            if (HTMLCollection* collection = (*m_cachedCollections)[i]) {
                if (shouldIgnoreType || DynamicNodeListCacheBase::shouldInvalidateTypeOnAttributeChange(collection->invalidationType(), *attrName))
                    collection->invalidateCache();
            }
        }
    }

    void adoptTreeScope(Document* oldDocument, Document* newDocument)
    {
        if (!m_cachedCollections)
            return;

        for (unsigned i = 0; i < (*m_cachedCollections).size(); i++) {
            HTMLCollection* collection = (*m_cachedCollections)[i];
            if (!collection)
                continue;
            collection->invalidateCache();
            if (oldDocument != newDocument) {
                oldDocument->unregisterNodeListCache(collection);
                newDocument->registerNodeListCache(collection);
            }
        }
    }

    typedef FixedArray<HTMLCollection*, NumNodeCollectionTypes> CachedHTMLCollectionArray;
    OwnPtr<CachedHTMLCollectionArray> m_cachedCollections;

    LayoutSize m_minimumSizeForResizing;
    RefPtr<RenderStyle> m_computedStyle;
    AtomicString m_shadowPseudoId;

    OwnPtr<DatasetDOMStringMap> m_datasetDOMStringMap;
    OwnPtr<ClassList> m_classList;
    OwnPtr<ElementShadow> m_shadow;
    OwnPtr<NamedNodeMap> m_attributeMap;

    bool m_styleAffectedByEmpty : 1;
    bool m_isInCanvasSubtree : 1;

    RefPtr<PseudoElement> m_generatedBefore;
    RefPtr<PseudoElement> m_generatedAfter;

    IntSize m_savedLayerScrollOffset;

#if ENABLE(FULLSCREEN_API)
    bool m_containsFullScreenElement;
#endif

#if ENABLE(UNDO_MANAGER)
    RefPtr<UndoManager> m_undoManager;
    bool m_undoScope;
#endif

private:
    void releasePseudoElement(PseudoElement*);
};

inline IntSize defaultMinimumSizeForResizing()
{
    return IntSize(MAX_LAYOUT_UNIT, MAX_LAYOUT_UNIT);
}

inline ElementRareData::ElementRareData()
    : NodeRareData()
    , m_minimumSizeForResizing(defaultMinimumSizeForResizing())
    , m_styleAffectedByEmpty(false)
    , m_isInCanvasSubtree(false)
    , m_generatedBefore(0)
    , m_generatedAfter(0)
#if ENABLE(FULLSCREEN_API)
    , m_containsFullScreenElement(false)
#endif
{
}

inline ElementRareData::~ElementRareData()
{
    ASSERT(!m_shadow);
    ASSERT(!m_generatedBefore);
    ASSERT(!m_generatedAfter);
}

inline void ElementRareData::setPseudoElement(PseudoId pseudoId, PassRefPtr<PseudoElement> element)
{
    switch (pseudoId) {
    case BEFORE:
        releasePseudoElement(m_generatedBefore.get());
        m_generatedBefore = element;
        break;
    case AFTER:
        releasePseudoElement(m_generatedAfter.get());
        m_generatedAfter = element;
        break;
    default:
        ASSERT_NOT_REACHED();
    }
}

inline PseudoElement* ElementRareData::pseudoElement(PseudoId pseudoId) const
{
    switch (pseudoId) {
    case BEFORE:
        return m_generatedBefore.get();
    case AFTER:
        return m_generatedAfter.get();
    default:
        ASSERT_NOT_REACHED();
        return 0;
    }
}

inline void ElementRareData::releasePseudoElement(PseudoElement* element)
{
    if (!element)
        return;

    if (element->attached())
        element->detach();

    ASSERT(!element->nextSibling());
    ASSERT(!element->previousSibling());

    element->setParentOrHostNode(0);
}

inline void ElementRareData::resetComputedStyle()
{
    m_computedStyle.clear();
}

}
#endif // ElementRareData_h
