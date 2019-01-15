/*
 * Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007, 2008, 2009 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Google, Inc.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#include "config.h"

#if ENABLE(SVG)
#include "RenderSVGRoot.h"

#include "Chrome.h"
#include "ChromeClient.h"
#include "Frame.h"
#include "GraphicsContext.h"
#include "HitTestResult.h"
#include "LayoutRepainter.h"
#include "Page.h"
#include "RenderPart.h"
#include "RenderSVGContainer.h"
#include "RenderSVGResource.h"
#include "RenderSVGResourceContainer.h"
#include "RenderView.h"
#include "SVGLength.h"
#include "SVGRenderingContext.h"
#include "SVGResources.h"
#include "SVGResourcesCache.h"
#include "SVGSVGElement.h"
#include "SVGStyledElement.h"
#include "SVGViewSpec.h"
#include "TransformState.h"

#if ENABLE(FILTERS)
#include "RenderSVGResourceFilter.h"
#endif

using namespace std;

namespace WebCore {

RenderSVGRoot::RenderSVGRoot(SVGStyledElement* node)
    : RenderReplaced(node)
    , m_objectBoundingBoxValid(false)
    , m_isLayoutSizeChanged(false)
    , m_needsBoundariesOrTransformUpdate(true)
{
}

RenderSVGRoot::~RenderSVGRoot()
{
}

SVGSVGElement* RenderSVGRoot::svgSVGElement() const
{
    return toSVGSVGElement(nodeForNonAnonymous());
}

void RenderSVGRoot::computeIntrinsicRatioInformation(FloatSize& intrinsicSize, double& intrinsicRatio) const
{
    // Spec: http://www.w3.org/TR/SVG/coords.html#IntrinsicSizing
    // SVG needs to specify how to calculate some intrinsic sizing properties to enable inclusion within other languages.

    // The intrinsic aspect ratio of the viewport of SVG content is necessary for example, when including SVG from an ‘object’
    // element in HTML styled with CSS. It is possible (indeed, common) for an SVG graphic to have an intrinsic aspect ratio but
    // not to have an intrinsic width or height. The intrinsic aspect ratio must be calculated based upon the following rules:
    // - The aspect ratio is calculated by dividing a width by a height.
    // - If the ‘width’ and ‘height’ of the rootmost ‘svg’ element are both specified with unit identifiers (in, mm, cm, pt, pc,
    //   px, em, ex) or in user units, then the aspect ratio is calculated from the ‘width’ and ‘height’ attributes after
    //   resolving both values to user units.
    intrinsicSize.setWidth(floatValueForLength(svgSVGElement()->intrinsicWidth(), 0));
    intrinsicSize.setHeight(floatValueForLength(svgSVGElement()->intrinsicHeight(), 0));

    if (!intrinsicSize.isEmpty())
        intrinsicRatio = intrinsicSize.width() / static_cast<double>(intrinsicSize.height());
    else {
        // - If either/both of the ‘width’ and ‘height’ of the rootmost ‘svg’ element are in percentage units (or omitted), the
        //   aspect ratio is calculated from the width and height values of the ‘viewBox’ specified for the current SVG document
        //   fragment. If the ‘viewBox’ is not correctly specified, or set to 'none', the intrinsic aspect ratio cannot be
        //   calculated and is considered unspecified.
        FloatSize viewBoxSize = svgSVGElement()->viewBox().size();
        if (!viewBoxSize.isEmpty()) {
            // The viewBox can only yield an intrinsic ratio, not an intrinsic size.
            intrinsicRatio = viewBoxSize.width() / static_cast<double>(viewBoxSize.height());
        }
    }
}

bool RenderSVGRoot::isEmbeddedThroughSVGImage() const
{
    if (!node())
        return false;

    Frame* frame = node()->document()->frame();
    if (!frame)
        return false;

    // Test whether we're embedded through an img.
    if (!frame->page() || !frame->page()->chrome())
        return false;

    ChromeClient* chromeClient = frame->page()->chrome()->client();
    if (!chromeClient || !chromeClient->isSVGImageChromeClient())
        return false;

    return true;
}

bool RenderSVGRoot::isEmbeddedThroughFrameContainingSVGDocument() const
{
    if (!node())
        return false;

    Frame* frame = node()->document()->frame();
    if (!frame)
        return false;

    // If our frame has an owner renderer, we're embedded through eg. object/embed/iframe,
    // but we only negotiate if we're in an SVG document.
    if (!frame->ownerRenderer())
        return false;
    return frame->document()->isSVGDocument();
}

static inline LayoutUnit resolveLengthAttributeForSVG(const Length& length, float scale, float maxSize, RenderView* renderView)
{
    return static_cast<LayoutUnit>(valueForLength(length, maxSize, renderView) * (length.isFixed() ? scale : 1));
}

LayoutUnit RenderSVGRoot::computeReplacedLogicalWidth(ShouldComputePreferred shouldComputePreferred) const
{
    // When we're embedded through SVGImage (border-image/background-image/<html:img>/...) we're forced to resize to a specific size.
    if (!m_containerSize.isEmpty())
        return m_containerSize.width();

    if (isEmbeddedThroughFrameContainingSVGDocument())
        return containingBlock()->availableLogicalWidth();

    if (style()->logicalWidth().isSpecified() || style()->logicalMaxWidth().isSpecified())
        return RenderReplaced::computeReplacedLogicalWidth(shouldComputePreferred);

    if (svgSVGElement()->hasIntrinsicWidth())
        return resolveLengthAttributeForSVG(svgSVGElement()->intrinsicWidth(), style()->effectiveZoom(), containingBlock()->availableLogicalWidth(), view());

    // SVG embedded via SVGImage (background-image/border-image/etc) / Inline SVG.
    return RenderReplaced::computeReplacedLogicalWidth(shouldComputePreferred);
}

LayoutUnit RenderSVGRoot::computeReplacedLogicalHeight() const
{
    // When we're embedded through SVGImage (border-image/background-image/<html:img>/...) we're forced to resize to a specific size.
    if (!m_containerSize.isEmpty())
        return m_containerSize.height();

    if (isEmbeddedThroughFrameContainingSVGDocument())
        return containingBlock()->availableLogicalHeight(IncludeMarginBorderPadding);

    if (style()->logicalHeight().isSpecified() || style()->logicalMaxHeight().isSpecified())
        return RenderReplaced::computeReplacedLogicalHeight();

    if (svgSVGElement()->hasIntrinsicHeight())
        return resolveLengthAttributeForSVG(svgSVGElement()->intrinsicHeight(), style()->effectiveZoom(), containingBlock()->availableLogicalHeight(IncludeMarginBorderPadding).toFloat(), view());

    // SVG embedded via SVGImage (background-image/border-image/etc) / Inline SVG.
    return RenderReplaced::computeReplacedLogicalHeight();
}

void RenderSVGRoot::layout()
{
    ASSERT(needsLayout());

    m_resourcesNeedingToInvalidateClients.clear();

    // Arbitrary affine transforms are incompatible with LayoutState.
    LayoutStateDisabler layoutStateDisabler(view());

    bool needsLayout = selfNeedsLayout();
    LayoutRepainter repainter(*this, checkForRepaintDuringLayout() && needsLayout);

    LayoutSize oldSize = size();
    updateLogicalWidth();
    updateLogicalHeight();
    buildLocalToBorderBoxTransform();

    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());
    m_isLayoutSizeChanged = needsLayout || (svg->hasRelativeLengths() && oldSize != size());
    SVGRenderSupport::layoutChildren(this, needsLayout || SVGRenderSupport::filtersForceContainerLayout(this));

    if (!m_resourcesNeedingToInvalidateClients.isEmpty()) {
        // Invalidate resource clients, which may mark some nodes for layout.
        HashSet<RenderSVGResourceContainer*>::iterator end = m_resourcesNeedingToInvalidateClients.end();
        for (HashSet<RenderSVGResourceContainer*>::iterator it = m_resourcesNeedingToInvalidateClients.begin(); it != end; ++it)
            (*it)->removeAllClientsFromCache();

        m_isLayoutSizeChanged = false;
        SVGRenderSupport::layoutChildren(this, false);
    }

    // At this point LayoutRepainter already grabbed the old bounds,
    // recalculate them now so repaintAfterLayout() uses the new bounds.
    if (m_needsBoundariesOrTransformUpdate) {
        updateCachedBoundaries();
        m_needsBoundariesOrTransformUpdate = false;
    }

    repainter.repaintAfterLayout();

    setNeedsLayout(false);
}

void RenderSVGRoot::paintReplaced(PaintInfo& paintInfo, const LayoutPoint& paintOffset)
{
    // An empty viewport disables rendering.
    if (pixelSnappedBorderBoxRect().isEmpty())
        return;

    // Don't paint, if the context explicitely disabled it.
    if (paintInfo.context->paintingDisabled())
        return;

    Page* page = 0;
    if (Frame* frame = this->frame())
        page = frame->page();

    // Don't paint if we don't have kids, except if we have filters we should paint those.
    if (!firstChild()) {
        SVGResources* resources = SVGResourcesCache::cachedResourcesForRenderObject(this);
        if (!resources || !resources->filter()) {
            if (page && paintInfo.phase == PaintPhaseForeground)
                page->addRelevantUnpaintedObject(this, visualOverflowRect());
            return;
        }
    }

    if (page && paintInfo.phase == PaintPhaseForeground)
        page->addRelevantRepaintedObject(this, visualOverflowRect());

    // Make a copy of the PaintInfo because applyTransform will modify the damage rect.
    PaintInfo childPaintInfo(paintInfo);
    childPaintInfo.context->save();

    // Apply initial viewport clip - not affected by overflow handling
    childPaintInfo.context->clip(pixelSnappedIntRect(overflowClipRect(paintOffset, paintInfo.renderRegion)));

    // Convert from container offsets (html renderers) to a relative transform (svg renderers).
    // Transform from our paint container's coordinate system to our local coords.
    IntPoint adjustedPaintOffset = roundedIntPoint(paintOffset);
    childPaintInfo.applyTransform(AffineTransform::translation(adjustedPaintOffset.x() - x(), adjustedPaintOffset.y() - y()) * localToParentTransform());

    SVGRenderingContext renderingContext;
    bool continueRendering = true;
    if (childPaintInfo.phase == PaintPhaseForeground) {
        renderingContext.prepareToRenderSVGContent(this, childPaintInfo);
        continueRendering = renderingContext.isRenderingPrepared();
    }

    if (continueRendering)
        RenderBox::paint(childPaintInfo, LayoutPoint());

    childPaintInfo.context->restore();
}

void RenderSVGRoot::willBeDestroyed()
{
    RenderBlock::removePercentHeightDescendant(const_cast<RenderSVGRoot*>(this));

    SVGResourcesCache::clientDestroyed(this);
    RenderReplaced::willBeDestroyed();
}

void RenderSVGRoot::styleWillChange(StyleDifference diff, const RenderStyle* newStyle)
{
    if (diff == StyleDifferenceLayout)
        setNeedsBoundariesUpdate();
    RenderReplaced::styleWillChange(diff, newStyle);
}

void RenderSVGRoot::styleDidChange(StyleDifference diff, const RenderStyle* oldStyle)
{
    RenderReplaced::styleDidChange(diff, oldStyle);
    SVGResourcesCache::clientStyleChanged(this, diff, style());
}

void RenderSVGRoot::addChild(RenderObject* child, RenderObject* beforeChild)
{
    RenderReplaced::addChild(child, beforeChild);
    SVGResourcesCache::clientWasAddedToTree(child, child->style());
}

void RenderSVGRoot::removeChild(RenderObject* child)
{
    SVGResourcesCache::clientWillBeRemovedFromTree(child);
    RenderReplaced::removeChild(child);
}

// RenderBox methods will expect coordinates w/o any transforms in coordinates
// relative to our borderBox origin.  This method gives us exactly that.
void RenderSVGRoot::buildLocalToBorderBoxTransform()
{
    SVGSVGElement* svg = static_cast<SVGSVGElement*>(node());
    float scale = style()->effectiveZoom();
    FloatPoint translate = svg->currentTranslate();
    LayoutSize borderAndPadding(borderLeft() + paddingLeft(), borderTop() + paddingTop());
    m_localToBorderBoxTransform = svg->viewBoxToViewTransform(contentWidth() / scale, contentHeight() / scale);
    if (borderAndPadding.isEmpty() && scale == 1 && translate == FloatPoint::zero())
        return;
    m_localToBorderBoxTransform = AffineTransform(scale, 0, 0, scale, borderAndPadding.width() + translate.x(), borderAndPadding.height() + translate.y()) * m_localToBorderBoxTransform;
}

const AffineTransform& RenderSVGRoot::localToParentTransform() const
{
    // Slightly optimized version of m_localToParentTransform = AffineTransform::translation(x(), y()) * m_localToBorderBoxTransform;
    m_localToParentTransform = m_localToBorderBoxTransform;
    if (x())
        m_localToParentTransform.setE(m_localToParentTransform.e() + roundToInt(x()));
    if (y())
        m_localToParentTransform.setF(m_localToParentTransform.f() + roundToInt(y()));
    return m_localToParentTransform;
}

LayoutRect RenderSVGRoot::clippedOverflowRectForRepaint(RenderBoxModelObject* repaintContainer) const
{
    return SVGRenderSupport::clippedOverflowRectForRepaint(this, repaintContainer);
}

void RenderSVGRoot::computeFloatRectForRepaint(RenderBoxModelObject* repaintContainer, FloatRect& repaintRect, bool fixed) const
{
    // Apply our local transforms (except for x/y translation), then our shadow, 
    // and then call RenderBox's method to handle all the normal CSS Box model bits
    repaintRect = m_localToBorderBoxTransform.mapRect(repaintRect);

    // Apply initial viewport clip - not affected by overflow settings    
    repaintRect.intersect(pixelSnappedBorderBoxRect());

    const SVGRenderStyle* svgStyle = style()->svgStyle();
    if (const ShadowData* shadow = svgStyle->shadow())
        shadow->adjustRectForShadow(repaintRect);

    LayoutRect rect = enclosingIntRect(repaintRect);
    RenderReplaced::computeRectForRepaint(repaintContainer, rect, fixed);
    repaintRect = rect;
}

// This method expects local CSS box coordinates.
// Callers with local SVG viewport coordinates should first apply the localToBorderBoxTransform
// to convert from SVG viewport coordinates to local CSS box coordinates.
void RenderSVGRoot::mapLocalToContainer(RenderBoxModelObject* repaintContainer, TransformState& transformState, MapCoordinatesFlags mode, bool* wasFixed) const
{
    ASSERT(mode & ~IsFixed); // We should have no fixed content in the SVG rendering tree.
    ASSERT(mode & UseTransforms); // mapping a point through SVG w/o respecting trasnforms is useless.

    RenderReplaced::mapLocalToContainer(repaintContainer, transformState, mode | ApplyContainerFlip, wasFixed);
}

const RenderObject* RenderSVGRoot::pushMappingToContainer(const RenderBoxModelObject* ancestorToStopAt, RenderGeometryMap& geometryMap) const
{
    return RenderReplaced::pushMappingToContainer(ancestorToStopAt, geometryMap);
}

void RenderSVGRoot::updateCachedBoundaries()
{
    SVGRenderSupport::computeContainerBoundingBoxes(this, m_objectBoundingBox, m_objectBoundingBoxValid, m_strokeBoundingBox, m_repaintBoundingBox);
    SVGRenderSupport::intersectRepaintRectWithResources(this, m_repaintBoundingBox);
    m_repaintBoundingBox.inflate(borderAndPaddingWidth());
}

bool RenderSVGRoot::nodeAtPoint(const HitTestRequest& request, HitTestResult& result, const HitTestPoint& pointInContainer, const LayoutPoint& accumulatedOffset, HitTestAction hitTestAction)
{
    LayoutPoint pointInParent = pointInContainer.point() - toLayoutSize(accumulatedOffset);
    LayoutPoint pointInBorderBox = pointInParent - toLayoutSize(location());

    // Only test SVG content if the point is in our content box.
    // FIXME: This should be an intersection when rect-based hit tests are supported by nodeAtFloatPoint.
    if (contentBoxRect().contains(pointInBorderBox)) {
        FloatPoint localPoint = localToParentTransform().inverse().mapPoint(FloatPoint(pointInParent));

        for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
            // FIXME: nodeAtFloatPoint() doesn't handle rect-based hit tests yet.
            if (child->nodeAtFloatPoint(request, result, localPoint, hitTestAction)) {
                updateHitTestResult(result, pointInBorderBox);
                if (!result.addNodeToRectBasedTestResult(child->node(), pointInContainer))
                    return true;
            }
        }
    }

    // If we didn't early exit above, we've just hit the container <svg> element. Unlike SVG 1.1, 2nd Edition allows container elements to be hit.
    if (hitTestAction == HitTestBlockBackground && visibleToHitTesting()) {
        // Only return true here, if the last hit testing phase 'BlockBackground' is executed. If we'd return true in the 'Foreground' phase,
        // hit testing would stop immediately. For SVG only trees this doesn't matter. Though when we have a <foreignObject> subtree we need
        // to be able to detect hits on the background of a <div> element. If we'd return true here in the 'Foreground' phase, we are not able 
        // to detect these hits anymore.
        LayoutRect boundsRect(accumulatedOffset + location(), size());
        if (pointInContainer.intersects(boundsRect)) {
            updateHitTestResult(result, pointInBorderBox);
            if (!result.addNodeToRectBasedTestResult(node(), pointInContainer, boundsRect))
                return true;
        }
    }

    return false;
}

bool RenderSVGRoot::hasRelativeDimensions() const
{
    return svgSVGElement()->intrinsicHeight().isPercent() || svgSVGElement()->intrinsicWidth().isPercent();
}

void RenderSVGRoot::addResourceForClientInvalidation(RenderSVGResourceContainer* resource)
{
    RenderObject* svgRoot = resource->parent();
    while (svgRoot && !svgRoot->isSVGRoot())
        svgRoot = svgRoot->parent();
    if (!svgRoot)
        return;
    static_cast<RenderSVGRoot*>(svgRoot)->m_resourcesNeedingToInvalidateClients.add(resource);
}

}

#endif // ENABLE(SVG)
