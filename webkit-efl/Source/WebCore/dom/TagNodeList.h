/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
 */

#ifndef TagNodeList_h
#define TagNodeList_h

#include "DynamicNodeList.h"
#include <wtf/text/AtomicString.h>

#if ENABLE(TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT)
#include "ExceptionCode.h"
#endif // ENABLE(TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT)

namespace WebCore {

    // NodeList that limits to a particular tag.
    class TagNodeList : public DynamicSubtreeNodeList {
    public:
        static PassRefPtr<TagNodeList> create(PassRefPtr<Node> rootNode, const AtomicString& namespaceURI, const AtomicString& localName)
        {
            ASSERT(namespaceURI != starAtom);
            return adoptRef(new TagNodeList(rootNode, namespaceURI, localName));
        }

        static PassRefPtr<TagNodeList> create(PassRefPtr<Node> rootNode, const AtomicString& localName)
        {
            return adoptRef(new TagNodeList(rootNode, starAtom, localName));
        }

        virtual ~TagNodeList();

#if ENABLE(TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT)
        NodeList* snapshot() const { return m_snapshot.get(); }
        bool hasSnapshot() const { return !!m_snapshot; };
        void takeSnapshot()
        {
            if (!rootNode())
                return;

            ExceptionCode ec;
            m_snapshot = rootNode()->querySelectorAll(m_localName, ec);
        }
        void clearSnapshot() const
        {
            m_snapshot.clear();
        }
#endif // ENABLE(TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT)

    protected:
        TagNodeList(PassRefPtr<Node> rootNode, const AtomicString& namespaceURI, const AtomicString& localName);

        virtual bool nodeMatches(Element*) const;
#if ENABLE(TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT)
        virtual bool isTagNodeList() const OVERRIDE { return true; }
#endif // ENABLE(TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT)

        AtomicString m_namespaceURI;
        AtomicString m_localName;

#if ENABLE(TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT)
        mutable RefPtr<NodeList> m_snapshot;
#endif // ENABLE(TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT)
    };

    class HTMLTagNodeList : public TagNodeList {
    public:
        static PassRefPtr<HTMLTagNodeList> create(PassRefPtr<Node> rootNode, const AtomicString& localName)
        {
            return adoptRef(new HTMLTagNodeList(rootNode, localName));
        }

    private:
        HTMLTagNodeList(PassRefPtr<Node> rootNode, const AtomicString& localName);

        virtual bool nodeMatches(Element*) const;

        AtomicString m_loweredLocalName;
    };
} // namespace WebCore

#endif // TagNodeList_h
