/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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
#include "TextureMapperLayer.h"
#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)
#include "TextureMapperCuller.h"
#endif
#include "Region.h"

#include "stdio.h"
#include <wtf/text/CString.h>
#if USE(ACCELERATED_COMPOSITING)
#if USE(TIZEN_TEXTURE_MAPPER)
#include "FloatQuad.h"
#endif
#include "GraphicsLayerTextureMapper.h"
#include "ImageBuffer.h"
#include "NotImplemented.h"
#include <wtf/MathExtras.h>

#if USE(CAIRO)
#include "CairoUtilities.h"
#include <wtf/text/CString.h>
#endif

namespace WebCore {

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
static unsigned s_depthInLayerTree = 0;
#endif

TextureMapperLayer* toTextureMapperLayer(GraphicsLayer* layer)
{
    return layer ? toGraphicsLayerTextureMapper(layer)->layer() : 0;
}

TextureMapperLayer* TextureMapperLayer::rootLayer()
{
    if (m_effectTarget)
        return m_effectTarget->rootLayer();
    if (m_parent)
        return m_parent->rootLayer();
    return this;
}

void TextureMapperLayer::setTransform(const TransformationMatrix& matrix)
{
    m_transform.setLocalTransform(matrix);
}

void TextureMapperLayer::clearBackingStoresRecursive()
{
    m_backingStore.clear();
    m_contentsLayer = 0;
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->clearBackingStoresRecursive();
    if (m_state.maskLayer)
        m_state.maskLayer->clearBackingStoresRecursive();
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
void TextureMapperLayer::clearBackingStore()
{
    m_backingStore.clear();
    m_contentsLayer = 0;
    if (m_state.maskLayer)
        m_state.maskLayer->clearBackingStore();
}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC)

#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)
void TextureMapperLayer::computeOpaqueRectRecursive(TextureMapperCuller* culler)
{
    ASSERT(culler);

    if (m_opacity != 1.0)
        return;

    if (!isVisible())
        return;

    // Compute opaque rect recursively
    TransformationMatrix transform;
    transform.multiply(m_transform.combined());

    if (!transform.isAffine())
        return;

    FloatQuad quad = transform.projectQuad(layerRect());
    IntRect rect = quad.enclosingBoundingBox();

    if (!quad.isRectilinear())
        return;

    bool shouldClip = m_state.masksToBounds && !m_state.preserves3D;

    if (shouldClip)
        culler->enterClip(rect);

    IntRect currentClip = culler->currentClip();
    if (!currentClip.isEmpty())
        rect.intersect(currentClip);

    bool isOpaque = m_state.contentsOpaque && (m_contentsLayer || ( m_backingStore && m_backingStore->isHavingTiles()));
    culler->addTargetRect(this, rect, isOpaque);

    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->computeOpaqueRectRecursive(culler);

    if (shouldClip)
        culler->leaveClip();
}
#endif // #if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)

void TextureMapperLayer::computeTransformsRecursive()
{
    if (m_size.isEmpty() && m_state.masksToBounds)
        return;

    // Compute transforms recursively on the way down to leafs.
    TransformationMatrix parentTransform;
    if (m_parent)
        parentTransform = m_parent->m_transform.combinedForChildren();
    else if (m_effectTarget)
        parentTransform = m_effectTarget->m_transform.combined();
    m_transform.combineTransforms(parentTransform);

    m_state.visible = m_state.backfaceVisibility || !m_transform.combined().isBackFaceVisible();
#if USE(TIZEN_TEXTURE_MAPPER)
    if (m_parent && !m_parent->m_state.visible && !m_parent->m_state.preserves3D)
        m_state.visible = false;
#endif

    if (m_parent && m_parent->m_state.preserves3D)
        m_centerZ = m_transform.combined().mapPoint(FloatPoint3D(m_size.width() / 2, m_size.height() / 2, 0)).z();

    if (m_state.maskLayer)
        m_state.maskLayer->computeTransformsRecursive();
    if (m_state.replicaLayer)
        m_state.replicaLayer->computeTransformsRecursive();
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->computeTransformsRecursive();

    // Reorder children if needed on the way back up.
    if (m_state.preserves3D)
        sortByZOrder(m_children, 0, m_children.size());
}

void TextureMapperLayer::updateBackingStore(TextureMapper* textureMapper, GraphicsLayerTextureMapper* layer)
{
    if (!layer || !textureMapper)
        return;

    if (!m_shouldUpdateBackingStoreFromLayer)
        return;

    if (!m_state.drawsContent || !m_state.contentsVisible || m_size.isEmpty()) {
        m_backingStore.clear();
        return;
    }

    IntRect dirtyRect = enclosingIntRect(layerRect());
    if (!m_state.needsDisplay)
        dirtyRect.intersect(enclosingIntRect(m_state.needsDisplayRect));
    if (dirtyRect.isEmpty())
        return;

    if (!m_backingStore)
        m_backingStore = TextureMapperTiledBackingStore::create();

#if PLATFORM(QT)
    ASSERT(dynamic_cast<TextureMapperTiledBackingStore*>(m_backingStore.get()));
#endif

    // Paint the entire dirty rect into an image buffer. This ensures we only paint once.
    OwnPtr<ImageBuffer> imageBuffer = ImageBuffer::create(dirtyRect.size());
    GraphicsContext* context = imageBuffer->context();
    context->setImageInterpolationQuality(textureMapper->imageInterpolationQuality());
    context->setTextDrawingMode(textureMapper->textDrawingMode());
    context->translate(-dirtyRect.x(), -dirtyRect.y());
    layer->paintGraphicsLayerContents(*context, dirtyRect);

    if (layer->showRepaintCounter()) {
        layer->incrementRepaintCount();
        drawRepaintCounter(context, layer);
    }

    RefPtr<Image> image = imageBuffer->copyImage(DontCopyBackingStore);
    TextureMapperTiledBackingStore* backingStore = static_cast<TextureMapperTiledBackingStore*>(m_backingStore.get());
    backingStore->updateContents(textureMapper, image.get(), m_size, dirtyRect);

    backingStore->setShowDebugBorders(layer->showDebugBorders());
    backingStore->setDebugBorder(m_debugBorderColor, m_debugBorderWidth);

    m_state.needsDisplay = false;
    m_state.needsDisplayRect = IntRect();
}

#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)
void TextureMapperLayer::paintWithCuller(const FloatRect& clipRect)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    s_depthInLayerTree = 0;
    LOG(AcceleratedCompositing, "[UI ] ----------------- @%s\n", __PRETTY_FUNCTION__);
#endif

    computeTransformsRecursive();

    TransformationMatrix transform;
    transform.multiply(m_transform.combined());
    TextureMapperCuller culler(transform.projectQuad(clipRect).enclosingBoundingBox());
    computeOpaqueRectRecursive(&culler);

    TextureMapperPaintOptions options;
    options.culler = &culler;
    options.textureMapper = m_textureMapper;
    options.textureMapper->bindSurface(0);
    paintRecursive(options);
}
#endif

void TextureMapperLayer::paint()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    s_depthInLayerTree = 0;
    LOG(AcceleratedCompositing, "[UI ] ----------------- @%s\n", __PRETTY_FUNCTION__);
#endif

    computeTransformsRecursive();

    TextureMapperPaintOptions options;
    options.textureMapper = m_textureMapper;
    options.textureMapper->bindSurface(0);
    paintRecursive(options);
}

void TextureMapperLayer::paintSelf(const TextureMapperPaintOptions& options)
{
    if (!m_state.visible || !m_state.contentsVisible)
        return;

    // We apply the following transform to compensate for painting into a surface, and then apply the offset so that the painting fits in the target rect.
    TransformationMatrix transform;
    transform.translate(options.offset.width(), options.offset.height());
    transform.multiply(options.transform);
    transform.multiply(m_transform.combined());

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    bool skipCompositing = false;
    if (options.textureMapper->fullScreenForVideoMode()) {
        if (!childOfFullScreenLayerForVideoIncludingSelf())
            skipCompositing = true;
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
        if (!options.textureMapper->fullScreenCompositingEnabled())
            skipCompositing = true;
#endif
    }
#endif

    float opacity = options.opacity;
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    if (m_state.backgroundColor.alpha() && !m_state.contentsRect.isEmpty() && !skipCompositing) {
#else
    if (m_state.backgroundColor.alpha() && !m_state.contentsRect.isEmpty()) {
#endif
        Color color = m_state.backgroundColor;
        float r, g, b, a;
        color.getRGBA(r, g, b, a);
        color = Color(r * opacity, g * opacity, b * opacity, a * opacity);
        options.textureMapper->drawSolidColor(m_state.contentsRect, transform, color);
    }

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        if (m_isLowScaleNonCompositedLayer && !options.textureMapper->lowScaleLayerCompositingEnabled())
            return;
        if (m_isNonCompositedLayer && options.textureMapper->paintOnlyLowScaleLayer())
            return;
#endif

#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)
    TextureMapperCuller* culler = options.culler;
    if (culler) {
        culler->removeOpaqueRect(this);

        // If the layer is occluded by opaque rects, avoid draw texture.
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        if (culler->isOccluded(this) && !m_isLowScaleNonCompositedLayer)
#else
        if (culler->isOccluded(this))
#endif
            return;
    }
#endif

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    if (skipCompositing)
        return;
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    {
        char buffer[256];
        if (s_depthInLayerTree >= sizeof(buffer))
            snprintf(buffer, sizeof(buffer), "(too deep)...");
        else
            for (unsigned i=0; i < s_depthInLayerTree; i++)
                snprintf(buffer + i, sizeof(buffer) - i, " ");

        LOG(AcceleratedCompositing, "[UI ] paint %s[%u]layer(%p) size (%u, %u) - %s %s @TextureMapperLayer::paintSelf",
            buffer, s_depthInLayerTree, this, size().width(), size().height(),
            (m_backingStore)?"backingstore":"", (m_contentsLayer)?"contentslayer":"");
    }
#endif

#if ENABLE(TIZEN_TEXTURE_MAPPER_CULLER)
    bool skipRenderingBackingStore = false;
    if (m_contentsLayer && m_state.contentsOpaque)
        skipRenderingBackingStore = m_state.contentsRect.contains(layerRect());

    // On FullScreen mode, if the video element has css background property, then background color is drawn on top of the fullscreen video. So avoid rendering background color for fullscreen video.
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    if (m_contentsLayer && options.textureMapper->fullScreenForVideoMode())
        skipRenderingBackingStore = true;
#endif

    if (!skipRenderingBackingStore && m_backingStore)
        m_backingStore->paintToTextureMapper(options.textureMapper, layerRect(), transform, opacity);
#else
    if (m_backingStore)
        m_backingStore->paintToTextureMapper(options.textureMapper, layerRect(), transform, opacity);
#endif

    if (m_contentsLayer)
        m_contentsLayer->paintToTextureMapper(options.textureMapper, m_state.contentsRect, transform, opacity);
}

int TextureMapperLayer::compareGraphicsLayersZValue(const void* a, const void* b)
{
    TextureMapperLayer* const* layerA = static_cast<TextureMapperLayer* const*>(a);
    TextureMapperLayer* const* layerB = static_cast<TextureMapperLayer* const*>(b);
    return int(((*layerA)->m_centerZ - (*layerB)->m_centerZ) * 1000);
}

void TextureMapperLayer::sortByZOrder(Vector<TextureMapperLayer* >& array, int first, int last)
{
    qsort(array.data(), array.size(), sizeof(TextureMapperLayer*), compareGraphicsLayersZValue);
}

void TextureMapperLayer::paintSelfAndChildren(const TextureMapperPaintOptions& options)
{
    paintSelf(options);

    if (m_children.isEmpty())
        return;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    s_depthInLayerTree++;
#endif

    bool shouldClip = m_state.masksToBounds && !m_state.preserves3D;
    if (shouldClip)
#if ENABLE(TIZEN_CORRECT_CLIPPING_WITH_INTERMEDIATE_SURFACE)
    {
        TransformationMatrix transform;
        if (options.bBindingIntermediateSurface) {
            transform.translate(options.offset.width(), options.offset.height());
            transform.multiply(options.transform);
        } else
            transform = options.transform;

        options.textureMapper->beginClip(TransformationMatrix(transform).multiply(m_transform.combined()), layerRect());
    }
#else
        options.textureMapper->beginClip(TransformationMatrix(options.transform).multiply(m_transform.combined()), layerRect());
#endif

    for (int i = 0; i < int(m_children.size()); ++i)
        m_children[i]->paintRecursive(options);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    s_depthInLayerTree--;
#endif

    if (shouldClip)
        options.textureMapper->endClip();
}

bool TextureMapperLayer::shouldBlend() const
{
    if (m_state.preserves3D)
        return false;

#if ENABLE(CSS_FILTERS)
    if (m_state.filters.size())
        return true;
#endif

    return m_opacity < 1 || m_state.maskLayer || (m_state.replicaLayer && m_state.replicaLayer->m_state.maskLayer);
}

bool TextureMapperLayer::isVisible() const
{
    if (m_size.isEmpty() && (m_state.masksToBounds || m_state.maskLayer || m_children.isEmpty()))
        return false;
    if (!m_state.visible && m_children.isEmpty())
        return false;
    if (!m_state.contentsVisible && m_children.isEmpty())
        return false;
    if (m_opacity < 0.01)
        return false;
    return true;
}

void TextureMapperLayer::paintSelfAndChildrenWithReplica(const TextureMapperPaintOptions& options)
{
    if (m_state.replicaLayer) {
        TextureMapperPaintOptions replicaOptions(options);
        replicaOptions.transform
                  .multiply(m_state.replicaLayer->m_transform.combined())
                  .multiply(m_transform.combined().inverse());
        paintSelfAndChildren(replicaOptions);
    }

    paintSelfAndChildren(options);
}

#if ENABLE(CSS_FILTERS)
static bool shouldKeepContentTexture(const FilterOperations& filters)
{
    for (int i = 0; i < int(filters.size()); ++i) {
        switch (filters.operations().at(i)->getOperationType()) {
        // The drop-shadow filter requires the content texture, because it needs to composite it
        // on top of the blurred shadow color.
        case FilterOperation::DROP_SHADOW:
            return true;
        default:
            break;
        }
    }

    return false;
}

static PassRefPtr<BitmapTexture> applyFilters(const FilterOperations& filters, TextureMapper* textureMapper, BitmapTexture* source)
{
    if (!filters.size())
        return source;

    RefPtr<BitmapTexture> filterSurface = shouldKeepContentTexture(filters) ? textureMapper->acquireTextureFromPool(source->size()) : source;
    return filterSurface->applyFilters(*source, filters);
}
#endif

static void resolveOverlaps(Region newRegion, Region& overlapRegion, Region& nonOverlapRegion)
{
    Region newOverlapRegion(newRegion);
    newOverlapRegion.intersect(nonOverlapRegion);
    nonOverlapRegion.subtract(newOverlapRegion);
    overlapRegion.unite(newOverlapRegion);
    newRegion.subtract(overlapRegion);
    nonOverlapRegion.unite(newRegion);
}

void TextureMapperLayer::computeOverlapRegions(Region& overlapRegion, Region& nonOverlapRegion, bool alwaysResolveSelfOverlap)
{
    if (!m_state.visible || !m_state.contentsVisible)
    return;

    FloatRect boundingRect;
    if (m_backingStore || m_state.masksToBounds || m_state.maskLayer || hasFilters())
        boundingRect = layerRect();
    else if (m_contentsLayer)
        boundingRect = m_state.contentsRect;

    TransformationMatrix replicaMatrix;
    if (m_state.replicaLayer) {
        replicaMatrix = replicaTransform();
        boundingRect.unite(replicaMatrix.mapRect(boundingRect));
    }

    boundingRect = m_transform.combined().mapRect(boundingRect);

    // Count all masks and filters as overlap layers.
    if (hasFilters() || m_state.maskLayer || (m_state.replicaLayer && m_state.replicaLayer->m_state.maskLayer)) {
        Region newOverlapRegion(enclosingIntRect(boundingRect));
        nonOverlapRegion.subtract(newOverlapRegion);
        overlapRegion.unite(newOverlapRegion);
        return;
    }

    Region newOverlapRegion;
    Region newNonOverlapRegion(enclosingIntRect(boundingRect));

    if (!m_state.masksToBounds) {
        for (size_t i = 0; i < m_children.size(); ++i) {
            TextureMapperLayer* child = m_children[i];
            bool alwaysResolveSelfOverlapForChildren = false;
            child->computeOverlapRegions(newOverlapRegion, newNonOverlapRegion, alwaysResolveSelfOverlapForChildren);
        }
    }

     if (m_state.replicaLayer) {
         newOverlapRegion.unite(replicaMatrix.mapRect(newOverlapRegion.bounds()));
         Region replicaRegion(replicaMatrix.mapRect(newNonOverlapRegion.bounds()));
         resolveOverlaps(replicaRegion, newOverlapRegion, newNonOverlapRegion);
     }

     if (!alwaysResolveSelfOverlap && shouldBlend()) {
         newNonOverlapRegion.unite(newOverlapRegion);
         newOverlapRegion = Region();
     }

     overlapRegion.unite(newOverlapRegion);
     resolveOverlaps(newNonOverlapRegion, overlapRegion, nonOverlapRegion);
}

void TextureMapperLayer::paintUsingOverlapRegions(const TextureMapperPaintOptions& options)
{
    Region overlapRegion;
    Region nonOverlapRegion;
    computeOverlapRegions(overlapRegion, nonOverlapRegion);
    if (overlapRegion.isEmpty()) {
        paintSelfAndChildrenWithReplica(options);
        return;
    }

#if ENABLE(TIZEN_OVERLAP_REGION_PAINT_OPTIMIZATION)
    // Having both overlap and non-overlap regions carries some overhead. Avoid it if the overlap area
    // is big anyway.
    if (overlapRegion.bounds().size().area() > nonOverlapRegion.bounds().size().area()) {
        overlapRegion.unite(nonOverlapRegion);
        nonOverlapRegion = Region();
    }

    nonOverlapRegion.translate(options.offset);
#endif
    Vector<IntRect> rects = nonOverlapRegion.rects();

    for (size_t i = 0; i < rects.size(); ++i) {
#if ENABLE(TIZEN_OVERLAP_REGION_PAINT_OPTIMIZATION)
        IntRect rect = rects[i];
        if (!rect.intersects(options.textureMapper->clipBounds()))
            continue;
#endif
        options.textureMapper->beginClip(TransformationMatrix(), rects[i]);
        paintSelfAndChildrenWithReplica(options);
        options.textureMapper->endClip();
    }

    rects = overlapRegion.rects();

#if ENABLE(TIZEN_OVERLAP_REGION_PAINT_OPTIMIZATION)
    static const size_t OverlapRegionConsolidationThreshold = 4;
    if (nonOverlapRegion.isEmpty() && rects.size() > OverlapRegionConsolidationThreshold) {
        rects.clear();
        rects.append(overlapRegion.bounds());
    }
#endif

    IntSize maxTextureSize = options.textureMapper->maxTextureSize();

#if ENABLE(TIZEN_OVERLAP_REGION_PAINT_OPTIMIZATION)
    IntRect adjustedClipBounds(options.textureMapper->clipBounds());
    adjustedClipBounds.move(-options.offset);
#endif

    for (size_t i = 0; i < rects.size(); ++i) {
        IntRect rect = rects[i];
        for (int x = rect.x(); x < rect.maxX(); x += maxTextureSize.width()) {
            for (int y = rect.y(); y < rect.maxY(); y += maxTextureSize.height()) {
                IntRect tileRect(IntPoint(x, y), maxTextureSize);
                tileRect.intersect(rect);

#if ENABLE(TIZEN_OVERLAP_REGION_PAINT_OPTIMIZATION)
                if (!tileRect.intersects(adjustedClipBounds))
                    continue;
#endif

                paintWithIntermediateSurface(options, tileRect);
            }
        }
    }
}

void TextureMapperLayer::applyMask(const TextureMapperPaintOptions& options)
{
    options.textureMapper->setMaskMode(true);
    paintSelf(options);
    options.textureMapper->setMaskMode(false);
}

PassRefPtr<BitmapTexture> TextureMapperLayer::paintIntoSurface(const TextureMapperPaintOptions& options, const IntSize& size)
{
    RefPtr<BitmapTexture> surface = options.textureMapper->acquireTextureFromPool(size);
    options.textureMapper->setBindSurface(true);
    options.textureMapper->bindSurface(surface.get());
    paintSelfAndChildren(options);
    if (m_state.maskLayer)
        m_state.maskLayer->applyMask(options);
#if ENABLE(CSS_FILTERS)
    if (!m_state.filters.isEmpty())
        surface = applyFilters(m_state.filters, options.textureMapper, surface.get());
#endif
    options.textureMapper->bindSurface(surface.get());
    options.textureMapper->setBindSurface(false);
    return surface;
}

static PassRefPtr<BitmapTexture> commitSurface(const TextureMapperPaintOptions& options, PassRefPtr<BitmapTexture> surface, const IntRect& rect, float opacity)
{
    options.textureMapper->bindSurface(options.surface.get());
    TransformationMatrix targetTransform;
    targetTransform.translate(options.offset.width(), options.offset.height());
    targetTransform.multiply(options.transform);
    options.textureMapper->drawTexture(*surface.get(), rect, targetTransform, opacity);
    return 0;
}

void TextureMapperLayer::paintWithIntermediateSurface(const TextureMapperPaintOptions& options, const IntRect& rect)
{
    RefPtr<BitmapTexture> replicaSurface;
    RefPtr<BitmapTexture> mainSurface;
    TextureMapperPaintOptions paintOptions(options);
    paintOptions.offset = -IntSize(rect.x(), rect.y());
    paintOptions.opacity = 1;
    paintOptions.transform = TransformationMatrix();
#if ENABLE(TIZEN_CORRECT_CLIPPING_WITH_INTERMEDIATE_SURFACE)
    paintOptions.bBindingIntermediateSurface = true;
#endif
    if (m_state.replicaLayer) {
        paintOptions.transform = replicaTransform();
        replicaSurface = paintIntoSurface(paintOptions, rect.size());
        paintOptions.transform = TransformationMatrix();
        if (m_state.replicaLayer->m_state.maskLayer)
            m_state.replicaLayer->m_state.maskLayer->applyMask(paintOptions);
    }

    if (replicaSurface && options.opacity == 1)
        replicaSurface = commitSurface(options, replicaSurface, rect, 1);

    mainSurface = paintIntoSurface(paintOptions, rect.size());
    if (replicaSurface) {
        options.textureMapper->bindSurface(replicaSurface.get());
        options.textureMapper->drawTexture(*mainSurface.get(), FloatRect(FloatPoint::zero(), rect.size()));
        mainSurface = replicaSurface;
    }
    commitSurface(options, mainSurface, rect, options.opacity);
}

void TextureMapperLayer::paintRecursive(const TextureMapperPaintOptions& options)
{
    if (!isVisible())
        return;

    if (!shouldBlend()) {
        paintSelfAndChildrenWithReplica(options);
        return;
    }

    TextureMapperPaintOptions paintOptions(options);
    paintOptions.opacity = options.opacity * m_opacity;
    paintUsingOverlapRegions(paintOptions);
}

TextureMapperLayer::~TextureMapperLayer()
{
    for (int i = m_children.size() - 1; i >= 0; --i)
        m_children[i]->m_parent = 0;

    if (m_parent)
        m_parent->m_children.remove(m_parent->m_children.find(this));
}

void TextureMapperLayer::syncCompositingState(GraphicsLayerTextureMapper* graphicsLayer, int options)
{
    syncCompositingState(graphicsLayer, rootLayer()->m_textureMapper, options);
}

void TextureMapperLayer::syncCompositingStateSelf(GraphicsLayerTextureMapper* graphicsLayer, TextureMapper* textureMapper)
{
    int changeMask = graphicsLayer->changeMask();

    if (changeMask == NoChanges && graphicsLayer->m_animations.isEmpty())
        return;

    graphicsLayer->updateDebugIndicators();

    if (changeMask & ParentChange) {
        TextureMapperLayer* newParent = toTextureMapperLayer(graphicsLayer->parent());
        if (newParent != m_parent) {
            // Remove layer from current from child list first.
            if (m_parent) {
                size_t index = m_parent->m_children.find(this);
                m_parent->m_children.remove(index);
                m_parent = 0;
            }
            // Set new layer parent and add layer to the parents child list.
            if (newParent) {
                m_parent = newParent;
                m_parent->m_children.append(this);
            }
        }
    }

    if (changeMask & ChildrenChange) {
        // Clear children parent pointer to avoid unsync and crash on layer delete.
        for (size_t i = 0; i < m_children.size(); i++)
#if ENABLE(TIZEN_TEXTUREMAPPERLAYER_TREE_UPDATE_WHEN_CHILD_CHANGED)
        {
            if (m_children[i]->m_parent == this)
                m_children[i]->m_parent = 0;
        }
#else
            m_children[i]->m_parent = 0;
#endif

        m_children.clear();
        for (size_t i = 0; i < graphicsLayer->children().size(); ++i) {
            TextureMapperLayer* child = toTextureMapperLayer(graphicsLayer->children()[i]);
            if (!child)
                continue;
            m_children.append(child);
            child->m_parent = this;
        }
    }

    m_size = graphicsLayer->size();

    if (changeMask & MaskLayerChange) {
       if (TextureMapperLayer* layer = toTextureMapperLayer(graphicsLayer->maskLayer()))
           layer->m_effectTarget = this;
    }

    if (changeMask & ReplicaLayerChange) {
       if (TextureMapperLayer* layer = toTextureMapperLayer(graphicsLayer->replicaLayer()))
           layer->m_effectTarget = this;
    }

#if ENABLE(TIZEN_ANIMATION_DELAY_START)
    if (changeMask & AnimationChange) {
        m_animations.animations().clear();
        for (unsigned i = 0; i < graphicsLayer->m_animations.size(); ++i) {
            GraphicsLayerAnimation& anims = graphicsLayer->m_animations.animations()[i];
            if (anims.startTime() > 0)
                m_animations.add(anims);
            else
                m_animations.add(GraphicsLayerAnimation(anims.name(), anims.keyframes(), anims.boxSize(), anims.animation().get(), currentTime() + anims.startTime(), anims.listsMatch()));
        }
    }
#else
    if (changeMask & AnimationChange)
        m_animations = graphicsLayer->m_animations;
#endif

    m_state.maskLayer = toTextureMapperLayer(graphicsLayer->maskLayer());
    m_state.replicaLayer = toTextureMapperLayer(graphicsLayer->replicaLayer());
    m_state.pos = graphicsLayer->position();
    m_state.anchorPoint = graphicsLayer->anchorPoint();
    m_state.size = graphicsLayer->size();
    m_state.contentsRect = graphicsLayer->contentsRect();
    m_state.transform = graphicsLayer->transform();
    m_state.preserves3D = graphicsLayer->preserves3D();
    m_state.masksToBounds = graphicsLayer->masksToBounds();
    m_state.drawsContent = graphicsLayer->drawsContent();
    m_state.contentsVisible = graphicsLayer->contentsAreVisible();
    m_state.contentsOpaque = graphicsLayer->contentsOpaque();
    m_state.backfaceVisibility = graphicsLayer->backfaceVisibility();
    m_state.childrenTransform = graphicsLayer->childrenTransform();
    m_state.opacity = graphicsLayer->opacity();
    m_state.backgroundColor = graphicsLayer->backgroundColor();
#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
    m_state.fullScreenLayerForVideo = graphicsLayer->fullScreenLayerForVideo();
#endif

#if ENABLE(CSS_FILTERS)
    if (changeMask & FilterChange)
        m_state.filters = graphicsLayer->filters();
#endif
    m_fixedToViewport = graphicsLayer->fixedToViewport();

    m_state.needsDisplay = m_state.needsDisplay || graphicsLayer->needsDisplay();
    if (!m_state.needsDisplay)
        m_state.needsDisplayRect.unite(graphicsLayer->needsDisplayRect());
    m_contentsLayer = graphicsLayer->contentsLayer();

    m_transform.setPosition(adjustedPosition());
    m_transform.setAnchorPoint(m_state.anchorPoint);
    m_transform.setSize(m_state.size);
    m_transform.setFlattening(!m_state.preserves3D);
    m_transform.setChildrenTransform(m_state.childrenTransform);
}

bool TextureMapperLayer::descendantsOrSelfHaveRunningAnimations() const
{
    if (m_animations.hasRunningAnimations())
        return true;

    for (size_t i = 0; i < m_children.size(); ++i) {
        if (m_children[i]->descendantsOrSelfHaveRunningAnimations())
            return true;
    }

    return false;
}

void TextureMapperLayer::applyAnimationsRecursively()
{
    syncAnimations();
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->applyAnimationsRecursively();
}

void TextureMapperLayer::syncAnimations()
{
#if ENABLE(TIZEN_UI_SIDE_ANIMATION_SYNC)
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform)) {
        if (!hasStoppedAnimation(AnimatedPropertyWebkitTransform))
            setTransform(m_state.transform);
    }
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyOpacity)) {
        if (!hasStoppedAnimation(AnimatedPropertyOpacity))
            setOpacity(m_state.opacity);
    }
    m_animations.apply(this);
#else
    m_animations.apply(this);
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyWebkitTransform))
        setTransform(m_state.transform);
    if (!m_animations.hasActiveAnimationsOfType(AnimatedPropertyOpacity))
        setOpacity(m_state.opacity);
#endif
}

void TextureMapperLayer::syncCompositingState(GraphicsLayerTextureMapper* graphicsLayer, TextureMapper* textureMapper, int options)
{
    if (!textureMapper)
        return;

    if (graphicsLayer && !(options & ComputationsOnly)) {
        syncCompositingStateSelf(graphicsLayer, textureMapper);
        graphicsLayer->didSynchronize();
    }

    if (graphicsLayer && m_state.maskLayer) {
        m_state.maskLayer->syncCompositingState(toGraphicsLayerTextureMapper(graphicsLayer->maskLayer()), textureMapper);

        // A mask layer has its parent's size by default, in case it's not set specifically.
        if (m_state.maskLayer->m_size.isEmpty())
            m_state.maskLayer->m_size = m_size;
    }

#if PLATFORM(TIZEN)
    if (graphicsLayer)
#endif
    if (m_state.replicaLayer)
        m_state.replicaLayer->syncCompositingState(toGraphicsLayerTextureMapper(graphicsLayer->replicaLayer()), textureMapper);

    syncAnimations();
    updateBackingStore(textureMapper, graphicsLayer);

    if (!(options & TraverseDescendants))
        options = ComputationsOnly;

    if (graphicsLayer) {
        Vector<GraphicsLayer*> children = graphicsLayer->children();
        for (int i = children.size() - 1; i >= 0; --i) {
            TextureMapperLayer* layer = toTextureMapperLayer(children[i]);
            if (!layer)
                continue;
            layer->syncCompositingState(toGraphicsLayerTextureMapper(children[i]), textureMapper, options);
        }
    } else {
        for (int i = m_children.size() - 1; i >= 0; --i)
            m_children[i]->syncCompositingState(0, textureMapper, options);
    }
}

bool TextureMapperLayer::isAncestorFixedToViewport() const
{
    for (TextureMapperLayer* parent = m_parent; parent; parent = parent->m_parent) {
        if (parent->m_fixedToViewport)
            return true;
    }

    return false;
}

void TextureMapperLayer::setScrollPositionDeltaIfNeeded(const FloatSize& delta)
{
    // delta is the difference between the scroll offset in the ui process and the scroll offset
    // in the web process. We add this delta to the position of fixed layers, to make
    // sure that they do not move while scrolling. We need to reset this delta to fixed layers
    // that have an ancestor which is also a fixed layer, because the delta will be added to the ancestor.
    if (isAncestorFixedToViewport())
        m_scrollPositionDelta = FloatSize();
    else
        m_scrollPositionDelta = delta;
    m_transform.setPosition(adjustedPosition());
}

void TextureMapperLayer::setDebugBorder(const Color& color, float width)
{
    // The default values for GraphicsLayer debug borders are a little
    // hard to see (some less than one pixel wide), so we double their size here.
    m_debugBorderColor = color;
    m_debugBorderWidth = width * 2;
}

#if USE(CAIRO)
void TextureMapperLayer::drawRepaintCounter(GraphicsContext* context, GraphicsLayer* layer)
{

    cairo_t* cr = context->platformContext()->cr();
    cairo_save(cr);

    CString repaintCount = String::format("%i", layer->repaintCount()).utf8();
    cairo_select_font_face(cr, "sans-serif", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
    cairo_set_font_size(cr, 18);

    cairo_text_extents_t repaintTextExtents;
    cairo_text_extents(cr, repaintCount.data(), &repaintTextExtents);

    static const int repaintCountBorderWidth = 10;
    setSourceRGBAFromColor(cr, layer->showDebugBorders() ? m_debugBorderColor : Color(0, 255, 0, 127));
    cairo_rectangle(cr, 0, 0,
                    repaintTextExtents.width + (repaintCountBorderWidth * 2),
                    repaintTextExtents.height + (repaintCountBorderWidth * 2));
    cairo_fill(cr);

    cairo_set_source_rgb(cr, 1, 1, 1);
    cairo_move_to(cr, repaintCountBorderWidth, repaintTextExtents.height + repaintCountBorderWidth);
    cairo_show_text(cr, repaintCount.data());

    cairo_restore(cr);
}
#else
void TextureMapperLayer::drawRepaintCounter(GraphicsContext* context, GraphicsLayer* layer)
{
    notImplemented();
}

#endif

#if ENABLE(TIZEN_UI_SIDE_ANIMATION_SYNC)
bool TextureMapperLayer::hasStoppedAnimation(AnimatedPropertyID type)
{
    Vector<GraphicsLayerAnimation>& anims = m_animations.animations();
    for (size_t i = 0; anims.size() > i; ++i) {
        if (anims[i].property() == type && anims[i].state() != GraphicsLayerAnimation::PlayingState)
            return true;
    }
    return false;
}
#endif

#if ENABLE(TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO)
bool TextureMapperLayer::childOfFullScreenLayerForVideoIncludingSelf()
{
    if (m_state.fullScreenLayerForVideo)
        return true;

    return m_parent ? m_parent->childOfFullScreenLayerForVideoIncludingSelf() : false;
}

bool TextureMapperLayer::hasFullScreenLayerForVideoIncludingSelf()
{
    if (m_state.fullScreenLayerForVideo)
        return true;

    for (int i = 0; i < int(m_children.size()); ++i) {
        if (m_children[i]->hasFullScreenLayerForVideoIncludingSelf())
            return true;
    }

    return false;
}
#endif

TransformationMatrix TextureMapperLayer::replicaTransform()
{
    return TransformationMatrix(m_state.replicaLayer->m_transform.combined()).multiply(m_transform.combined().inverse());
}

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
void TextureMapperLayer::setLowScaleNonCompositedLayer(bool isLowScaleNonCompositedLayer)
{
    m_isLowScaleNonCompositedLayer = isLowScaleNonCompositedLayer;
}
void TextureMapperLayer::setNonCompositedLayer(bool isNonCompositedLayer)
{
    m_isNonCompositedLayer = isNonCompositedLayer;
}

#endif

}
#endif
