/*
 * Copyright (C) 2013 Samsung Electronic.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LinkMagnifier.h"

#if ENABLE(TIZEN_LINK_MAGNIFIER)
#include "EflScreenUtilities.h"

using namespace WebCore;

namespace WebKit {

class LinkMagnifierCandidate {
public:
    LinkMagnifierCandidate(Node* node, const IntRect& rect, int direction, int distance)
        : m_node(node)
        , m_rect(rect)
        , m_direction(direction)
        , m_distance(distance)
    {}

    LinkMagnifierCandidate()
        : m_node(0)
        , m_direction(0)
        , m_distance(std::numeric_limits<int>::max())
    {}

    Node* node() const { return m_node; }
    const IntRect& rect() const { return m_rect; }
    int direction() const { return m_direction; }
    int distance() const { return m_distance; }

    bool canUnite(const LinkMagnifierCandidate&) const;
    void unite(const LinkMagnifierCandidate&);

private:
    Node* m_node;
    IntRect m_rect;
    int m_direction;
    int m_distance;
};

bool LinkMagnifierCandidate::canUnite(const LinkMagnifierCandidate& candidate) const
{
    return (m_node == candidate.node() && m_direction != candidate.direction());
}

void LinkMagnifierCandidate::unite(const LinkMagnifierCandidate& candidate)
{
    m_rect.unite(candidate.rect());
    if (m_distance > candidate.distance())
        m_distance = candidate.distance();
}

class LinkMagnifierCandidateMap {
public:
    LinkMagnifierCandidateMap(WebPage*, const IntPoint&);

    IntRect calculateRect() const;

private:
    enum {
        CandidateDirectionTop = 1,
        CandidateDirectionBottom,
        CandidateDirectionLeft,
        CandidateDirectionRight,
        CandidateDirectionTopLeft,
        CandidateDirectionTopRight,
        CandidateDirectionBottomLeft,
        CandidateDirectionBottomRight,
        CandidateDirectionNone
    };

    static Node* findCandidateNode(Node*);
    static void calculateNodeRects(Node*, Vector<IntRect>&);
    static int candidateDirection(const IntSize&, const IntPoint&, const IntPoint&);
    static int simplifiedDistance(const IntSize&);

    void findCandidate();
    void addCandidate(Node*);
    int calculateThreshold() const;

    IntPoint m_position;
    Frame* m_frame;
    IntPoint m_contentsPosition;
    int m_calculateMargin;
    HashMap<int, LinkMagnifierCandidate> m_map;
};

LinkMagnifierCandidateMap::LinkMagnifierCandidateMap(WebPage* page, const IntPoint& position)
    : m_position(position)
    , m_frame(page->mainFrame())
{
    if (!m_frame || !m_frame->view())
        return;

    m_contentsPosition = m_frame->view()->windowToContents(position);

    float scaleFactor = page->drawingArea()->layerTreeHost() ? page->drawingArea()->layerTreeHost()->contentsScaleFactor() : 1;
    const float targetInch = 0.1;
    m_calculateMargin = static_cast<int>(static_cast<float>(getDPI()) * targetInch / scaleFactor);

    findCandidate();
}

Node* LinkMagnifierCandidateMap::findCandidateNode(Node* node)
{
    if (node->isMediaControlElement() || node->hasTagName(HTMLNames::iframeTag))
        return 0;

    if (!node->renderer() || (!node->isMouseFocusable() && !node->willRespondToMouseClickEvents() && !node->willRespondToMouseMoveEvents()))
        return 0;

    Element* rootEditableElement = node->rootEditableElement();
    if (rootEditableElement) {
        Node* shadowHost = rootEditableElement->shadowHost();
        if (shadowHost)
            return shadowHost;
        else
            return rootEditableElement;
    }

    return node;
}

void LinkMagnifierCandidateMap::calculateNodeRects(Node* node, Vector<IntRect>& rects)
{
    RenderObject* renderer = node->renderer();
    FrameView* view = node->document()->frame()->view();

    IntPoint absolutePoint;
    absolutePoint = view->convertToContainingWindow(view->convertFromRenderer(renderer, absolutePoint));

    renderer->addFocusRingRects(rects, absolutePoint);
}

int LinkMagnifierCandidateMap::candidateDirection(const IntSize& difference, const IntPoint& base, const IntPoint& candidate)
{
    if (difference.isZero())
        return CandidateDirectionNone;
    else if (!difference.width()) {
        if (base.y() > candidate.y())
            return CandidateDirectionTop;
        else
            return CandidateDirectionBottom;
    } else if (!difference.height()) {
        if (base.x() > candidate.x())
            return CandidateDirectionLeft;
        else
            return CandidateDirectionRight;
    } else if (base.y() > candidate.y()) {
        if (base.x() > candidate.x())
            return CandidateDirectionTopLeft;
        else
            return CandidateDirectionTopRight;
    } else {
        if (base.x() > candidate.x())
            return CandidateDirectionBottomLeft;
        else
            return CandidateDirectionBottomRight;
    }
}

int LinkMagnifierCandidateMap::simplifiedDistance(const IntSize& difference)
{
    return abs(difference.width()) + abs(difference.height());
}

void LinkMagnifierCandidateMap::findCandidate()
{
    HitTestRequest::HitTestRequestType hitType = HitTestRequest::ReadOnly | HitTestRequest::Active;
    HitTestResult result = m_frame->eventHandler()->hitTestResultAtPoint(m_contentsPosition, true, false, DontHitTestScrollbars, hitType, IntSize(m_calculateMargin, m_calculateMargin));
    const HitTestResult::NodeSet& nodeSet(result.rectBasedTestResult());

    if (result.innerNode()->isMediaControlElement() || result.innerNonSharedNode()->isMediaControlElement())
        return;

    Vector<Node*> candidateNodes;
    HashSet<Node*> ancestorNodes;
    HitTestResult::NodeSet::const_iterator nodeIterator = nodeSet.begin();
    HitTestResult::NodeSet::const_iterator nodeEnd = nodeSet.end();

    for (; nodeIterator != nodeEnd; ++nodeIterator) {
        for (Node* node = nodeIterator->get(); node; node = node->parentOrHostNode()) {
            if (candidateNodes.contains(node))
                break;

            Node* candidateNode = findCandidateNode(node);
            if (candidateNode) {
                candidateNodes.append(candidateNode);
                for (candidateNode = candidateNode->parentOrHostNode(); candidateNode; candidateNode = candidateNode->parentOrHostNode()) {
                    HashSet<Node*>::AddResult addResult = ancestorNodes.add(candidateNode);
                    if (!addResult.isNewEntry)
                        break;
                }
                break;
            }
        }
    }

    size_t size = candidateNodes.size();
    for (size_t i = 0; i < size; ++i) {
        if (!ancestorNodes.contains(candidateNodes[i]))
            addCandidate(candidateNodes[i]);
    }
}

void LinkMagnifierCandidateMap::addCandidate(Node* node)
{
    Vector<IntRect> rects;
    calculateNodeRects(node, rects);

    size_t rectSize = rects.size();
    if (!rectSize)
        return;

    for (size_t rectsIter = 0; rectsIter < rectSize; ++rectsIter) {
        IntRect rect = rects[rectsIter];
        if (rect.isEmpty())
            continue;

        IntSize difference = rect.differenceToPoint(m_contentsPosition);
        int direction = candidateDirection(difference, m_contentsPosition, rect.location());
        int distance = simplifiedDistance(difference);

        const LinkMagnifierCandidate& previousCandidate = m_map.get(direction);
        LinkMagnifierCandidate candidate(node, rect, direction, distance);

        if (previousCandidate.canUnite(candidate)) {
            candidate.unite(previousCandidate);
            m_map.set(direction, candidate);
        } else if (previousCandidate.distance() > distance)
            m_map.set(direction, candidate);
    }
}

IntRect LinkMagnifierCandidateMap::calculateRect() const
{
    if (m_map.isEmpty())
        return IntRect(m_position, IntSize());

    int threshold = calculateThreshold();

    HashMap<int, LinkMagnifierCandidate>::const_iterator mapIter = m_map.begin();
    const LinkMagnifierCandidate& firstCandidate = mapIter->second;
    IntRect candidatesRect(firstCandidate.rect());
    bool hasMultipleNode = false;

    for (++mapIter; mapIter != m_map.end(); ++mapIter) {
        const LinkMagnifierCandidate& candidate = mapIter->second;
        if (threshold < candidate.distance())
            continue;

        candidatesRect.unite(candidate.rect());
        if (!hasMultipleNode && candidate.node() != firstCandidate.node())
            hasMultipleNode = true;
    }

    if (!hasMultipleNode) {
        if (candidatesRect.contains(m_position))
            return m_frame->view()->contentsToWindow(IntRect(m_position, IntSize()));
        else
            return m_frame->view()->contentsToWindow(IntRect(firstCandidate.rect().center(), IntSize()));
    }

    return m_frame->view()->contentsToWindow(candidatesRect);
}

int LinkMagnifierCandidateMap::calculateThreshold() const
{
    const LinkMagnifierCandidate& targetCandidate = m_map.get(CandidateDirectionNone);
    if (targetCandidate.rect().isEmpty())
        return std::numeric_limits<int>::max();

    const IntRect& targetRect = targetCandidate.rect();
    IntSize difference = targetRect.center() - m_contentsPosition;
    int targetSize;
    int threshold;

    if (targetRect.width() < targetRect.height() || (targetRect.width() == targetRect.height() && difference.width() < difference.height())) {
        targetSize = targetRect.width();
        threshold = abs(difference.width());
    } else {
        targetSize = targetRect.height();
        threshold = abs(difference.height());
    }

    if (targetSize < m_calculateMargin)
        threshold *= (m_calculateMargin / targetSize);

    return threshold;
}

IntRect LinkMagnifier::rect(WebPage* page, const IntPoint& position)
{
    LinkMagnifierCandidateMap candidateMap(page, position);
    return candidateMap.calculateRect();
}

} // namespace WebKit

#endif
