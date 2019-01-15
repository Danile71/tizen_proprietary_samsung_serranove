/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "LayerTreeCoordinator.h"

#include "CoordinatedGraphicsArgumentCoders.h"
#include "DrawingAreaImpl.h"
#include "GraphicsContext.h"
#include "LayerTreeCoordinatorProxyMessages.h"
#include "MessageID.h"
#include "SurfaceUpdateInfo.h"
#include "WebCoreArgumentCoders.h"
#include "WebGraphicsLayer.h"
#include "WebPage.h"
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/Page.h>
#include <WebCore/RenderLayer.h>
#include <WebCore/RenderLayerBacking.h>
#include <WebCore/RenderLayerCompositor.h>
#include <WebCore/RenderView.h>
#include <WebCore/Settings.h>

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
#include <WebCore/Chrome.h>
#include <WebCore/ChromeClient.h>
#endif

#if ENABLE(CSS_SHADERS)
#include "CustomFilterValidatedProgram.h"
#include "ValidatedCustomFilterOperation.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#include "WebProcess.h"
#endif

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_LAYER_FLUSH_THROTTLING)
const float LayerTreeCoordinator::s_layerFlushTimerDelayTable[s_layerFlushTimerDelayMaxLevel] = {0, 0.1, 0.2, 0.6, 1.3};
#endif

PassRefPtr<LayerTreeCoordinator> LayerTreeCoordinator::create(WebPage* webPage)
{
    return adoptRef(new LayerTreeCoordinator(webPage));
}

LayerTreeCoordinator::~LayerTreeCoordinator()
{
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (m_lowScaleNonCompositedLayer)
        destroyLowScaleNonCompositedLayer();
#endif

#if ENABLE(CSS_SHADERS)
    disconnectCustomFilterPrograms();
#endif
    // Prevent setWebGraphicsLayerClient(0) -> detachLayer() from modifying the set while we iterate it.
    HashSet<WebCore::WebGraphicsLayer*> registeredLayers;
    registeredLayers.swap(m_registeredLayers);

    HashSet<WebCore::WebGraphicsLayer*>::iterator end = registeredLayers.end();
    for (HashSet<WebCore::WebGraphicsLayer*>::iterator it = registeredLayers.begin(); it != end; ++it)
        (*it)->setWebGraphicsLayerClient(0);
}

LayerTreeCoordinator::LayerTreeCoordinator(WebPage* webPage)
    : LayerTreeHost(webPage)
    , m_notifyAfterScheduledLayerFlush(false)
    , m_isValid(true)
#if PLATFORM(TIZEN)
    , m_waitingForUIProcess(false)
#else
    , m_waitingForUIProcess(true)
#endif
    , m_isSuspended(false)
    , m_contentsScale(1)
    , m_shouldSyncFrame(false)
    , m_shouldSyncRootLayer(true)
    , m_layerFlushTimer(this, &LayerTreeCoordinator::layerFlushTimerFired)
    , m_releaseInactiveAtlasesTimer(this, &LayerTreeCoordinator::releaseInactiveAtlasesTimerFired)
    , m_layerFlushSchedulingEnabled(true)
#if ENABLE(TIZEN_LAYER_FLUSH_THROTTLING)
    , m_layerFlushTimerDelay(0.0)
    , m_deferLayerFlushEnabled(false)
    , m_layerFlushTimerDelayLevel(0)
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    , m_suspendedJavaScript(false)
#endif
#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    , m_compositedContentLayersCount(0)
#endif
#if ENABLE(TIZEN_ONESHOT_DRAWING_SYNCHRONIZATION)
    , m_needsOneShotDrawingSynchronization(false)
#endif
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    , m_accelerationMode(NotReady)
#endif
    , m_animationsLocked(false)
#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
    , m_touchEventTargetRectsChanged(false)
#endif
#if ENABLE(REQUEST_ANIMATION_FRAME)
    , m_lastAnimationServiceTime(0)
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_updateLowScaleLayerOnly(false)
    , m_refreshAllLayer(false)
#endif
#if ENABLE(TIZEN_DEFER_FIXED_LAYER_UPDATE)
    , m_needUpdateFixedLayerPosition(false)
#endif
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    m_webPage->corePage()->settings()->setScrollingCoordinatorEnabled(true);
#endif

#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
    static_cast<ScrollingCoordinatorCoordinatedGraphics*>(m_webPage->corePage()->scrollingCoordinator())->setClient(this);
#endif

    // Create a root layer.
    m_rootLayer = GraphicsLayer::create(this);
    WebGraphicsLayer* webRootLayer = toWebGraphicsLayer(m_rootLayer.get());
    webRootLayer->setRootLayer(true);
#ifndef NDEBUG
    m_rootLayer->setName("LayerTreeCoordinator root layer");
#endif
    m_rootLayer->setDrawsContent(false);
    m_rootLayer->setSize(m_webPage->size());
    m_layerTreeContext.webLayerID = toWebGraphicsLayer(webRootLayer)->id();

    m_nonCompositedContentLayer = GraphicsLayer::create(this);
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    WebGraphicsLayer* webNonCompositedLayer = toWebGraphicsLayer(m_nonCompositedContentLayer.get());
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetNonCompositedLayerID(webNonCompositedLayer->id()));
#endif
#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    webNonCompositedLayer->setNonCompositedLayer(true);
#endif

    toWebGraphicsLayer(m_rootLayer.get())->setWebGraphicsLayerClient(this);
#ifndef NDEBUG
    m_nonCompositedContentLayer->setName("LayerTreeCoordinator non-composited content");
#endif
    m_nonCompositedContentLayer->setDrawsContent(true);
    m_nonCompositedContentLayer->setSize(m_webPage->size());

    m_rootLayer->addChild(m_nonCompositedContentLayer.get());

    if (m_webPage->hasPageOverlay())
        createPageOverlayLayer();

    scheduleLayerFlush();
}

void LayerTreeCoordinator::setLayerFlushSchedulingEnabled(bool layerFlushingEnabled)
{
    if (m_layerFlushSchedulingEnabled == layerFlushingEnabled)
        return;

    m_layerFlushSchedulingEnabled = layerFlushingEnabled;

    if (m_layerFlushSchedulingEnabled) {
        scheduleLayerFlush();
        return;
    }

    cancelPendingLayerFlush();
}

void LayerTreeCoordinator::scheduleLayerFlush()
{
    if (!m_layerFlushSchedulingEnabled)
        return;

    if (!m_layerFlushTimer.isActive()) {
#if ENABLE(TIZEN_LAYER_FLUSH_THROTTLING)
        m_layerFlushTimerDelay = deferredLayerFlushDelay();
        m_layerFlushTimer.startOneShot(m_layerFlushTimerDelay);
    }
#if ENABLE(TIZEN_SYNC_REQUEST_ANIMATION_FRAME)
    if (m_layerFlushTimerDelay > 0.0)
        m_webPage->suspendAnimationController();
#endif
#else
        m_layerFlushTimer.startOneShot(0.0);
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    if (!m_layerFlushWorkItemToDispatchWhileWaitingForSyncReply)
        m_layerFlushWorkItemToDispatchWhileWaitingForSyncReply = m_webPage->connection()->dispatchWorkItemWhileWaitingForSyncReply(WTF::bind(&LayerTreeCoordinator::performScheduledLayerFlushForcely, this));
#endif
}

void LayerTreeCoordinator::cancelPendingLayerFlush()
{
    m_layerFlushTimer.stop();
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    m_webPage->connection()->clearWorkItemWhileWaitingForSyncReply(m_layerFlushWorkItemToDispatchWhileWaitingForSyncReply);
    m_layerFlushWorkItemToDispatchWhileWaitingForSyncReply.clear();
#endif
}

void LayerTreeCoordinator::setShouldNotifyAfterNextScheduledLayerFlush(bool notifyAfterScheduledLayerFlush)
{
    m_notifyAfterScheduledLayerFlush = notifyAfterScheduledLayerFlush;
}

void LayerTreeCoordinator::setRootCompositingLayer(WebCore::GraphicsLayer* graphicsLayer)
{
    m_nonCompositedContentLayer->removeAllChildren();
    m_nonCompositedContentLayer->setContentsOpaque(m_webPage->drawsBackground() && !m_webPage->drawsTransparentBackground());

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    // Purge backing store before the browser moves to the previos page or new link.
    if (m_lowScaleNonCompositedLayer)
        static_cast<WebCore::WebGraphicsLayer*>(m_lowScaleNonCompositedLayer.get())->purgeBackingStores();
#endif

    // Add the accelerated layer tree hierarchy.
    if (graphicsLayer)
        m_nonCompositedContentLayer->addChild(graphicsLayer);

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    m_compositedContentLayersCount = compositedContentLayersCount(m_nonCompositedContentLayer.get());
#endif
}

void LayerTreeCoordinator::invalidate()
{
    cancelPendingLayerFlush();

    ASSERT(m_isValid);
    m_rootLayer = nullptr;
    m_isValid = false;
}

void LayerTreeCoordinator::setNonCompositedContentsNeedDisplay(const WebCore::IntRect& rect)
{
    m_nonCompositedContentLayer->setNeedsDisplayInRect(rect);
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (m_lowScaleNonCompositedLayer)
        m_lowScaleNonCompositedLayer->setNeedsDisplayInRect(rect);
#endif
    if (m_pageOverlayLayer)
        m_pageOverlayLayer->setNeedsDisplayInRect(rect);

    scheduleLayerFlush();
}

void LayerTreeCoordinator::scrollNonCompositedContents(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset)
{
    setNonCompositedContentsNeedDisplay(scrollRect);
}

void LayerTreeCoordinator::forceRepaint()
{
#if PLATFORM(TIZEN)
    // Make sure that core page is onScreen. If it is offscreen, there won't be any RootGraphicsLayer attached.
    // And so there won't be any accelerated layer tree hierarchy to paint which will make flickering.
    if (!m_webPage->corePage()->isOnscreen())
        return;
#endif

    // We need to schedule another flush, otherwise the forced paint might cancel a later expected flush.
    // This is aligned with LayerTreeHostCA.
    scheduleLayerFlush();
    flushPendingLayerChanges();
}

void LayerTreeCoordinator::sizeDidChange(const WebCore::IntSize& newSize)
{
    if (m_rootLayer->size() == newSize)
        return;

    m_rootLayer->setSize(newSize);

    // If the newSize exposes new areas of the non-composited content a setNeedsDisplay is needed
    // for those newly exposed areas.
    FloatSize oldSize = m_nonCompositedContentLayer->size();
    m_nonCompositedContentLayer->setSize(newSize);
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (m_lowScaleNonCompositedLayer)
        m_lowScaleNonCompositedLayer->setSize(newSize);
#endif
    if (newSize.width() > oldSize.width()) {
        float height = std::min(static_cast<float>(newSize.height()), oldSize.height());
        m_nonCompositedContentLayer->setNeedsDisplayInRect(FloatRect(oldSize.width(), 0, newSize.width() - oldSize.width(), height));
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        if (m_lowScaleNonCompositedLayer)
            m_lowScaleNonCompositedLayer->setNeedsDisplayInRect(FloatRect(oldSize.width(), 0, newSize.width() - oldSize.width(), height));
#endif
    }

    if (newSize.height() > oldSize.height()) {
        m_nonCompositedContentLayer->setNeedsDisplayInRect(FloatRect(0, oldSize.height(), newSize.width(), newSize.height() - oldSize.height()));
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        if (m_lowScaleNonCompositedLayer)
            m_lowScaleNonCompositedLayer->setNeedsDisplayInRect(FloatRect(0, oldSize.height(), newSize.width(), newSize.height() - oldSize.height()));
#endif
    }

    if (m_pageOverlayLayer)
        m_pageOverlayLayer->setSize(newSize);

    scheduleLayerFlush();
}

void LayerTreeCoordinator::didInstallPageOverlay()
{
    createPageOverlayLayer();
    scheduleLayerFlush();
}

void LayerTreeCoordinator::didUninstallPageOverlay()
{
    destroyPageOverlayLayer();
    scheduleLayerFlush();
}

void LayerTreeCoordinator::setPageOverlayNeedsDisplay(const WebCore::IntRect& rect)
{
    ASSERT(m_pageOverlayLayer);
    m_pageOverlayLayer->setNeedsDisplayInRect(rect);
    scheduleLayerFlush();
}

void LayerTreeCoordinator::setPageOverlayOpacity(float value)
{
    ASSERT(m_pageOverlayLayer);
    m_pageOverlayLayer->setOpacity(value);
    scheduleLayerFlush();
}

bool LayerTreeCoordinator::flushPendingLayerChanges()
{
    if (m_waitingForUIProcess)
        return false;

    bool didSync = m_webPage->corePage()->mainFrame()->view()->syncCompositingStateIncludingSubframes();
    m_nonCompositedContentLayer->syncCompositingStateForThisLayerOnly();
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (m_lowScaleNonCompositedLayer)
        m_lowScaleNonCompositedLayer->syncCompositingStateForThisLayerOnly();
#endif

    if (m_pageOverlayLayer)
        m_pageOverlayLayer->syncCompositingStateForThisLayerOnly();

    m_rootLayer->syncCompositingStateForThisLayerOnly();

    if (m_shouldSyncRootLayer) {
        m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetRootCompositingLayer(toWebGraphicsLayer(m_rootLayer.get())->id()));
        m_shouldSyncRootLayer = false;
    }

    if (!m_shouldSyncFrame)
        return didSync;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (!m_compositedContentLayers.isEmpty()) {
        Vector<uint32_t> weblayerIDsCopy;
        copyToVector(m_compositedContentLayers, weblayerIDsCopy);
        m_compositedContentLayers.clear();
        bool pauseJavaScript = false;
        for (size_t i = 0; i < weblayerIDsCopy.size(); ++i) {
            WebGraphicsLayer* layer = WebGraphicsLayer::layerByID(weblayerIDsCopy[i]);
            if (layer && !layer->swapPlatformSurfaces())
                pauseJavaScript = true;
        }
        if (pauseJavaScript) {
            m_webPage->suspendJavaScriptAndResources();
            m_suspendedJavaScript = true;
        }
    }
#endif

#if ENABLE(TIZEN_SYNC_REQUEST_ANIMATION_FRAME)
    m_webPage->suspendAnimationController();
#endif
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::DidRenderFrame(m_visibleContentsRect.location()));
    m_waitingForUIProcess = true;
    m_shouldSyncFrame = false;
#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
    if (m_touchEventTargetRectsChanged) {
        m_webPage->setTouchEventTargetRects(m_touchEventTargetRects);
        m_touchEventTargetRects.clear();
        m_touchEventTargetRectsChanged = false;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    WebProcess::shared().platformSurfacePool()->shrinkIfNeeded();
    SharedPlatformSurfaceManagement::getInstance().allowRemove();
#endif

    return true;
}

void LayerTreeCoordinator::syncLayerState(WebLayerID id, const WebLayerInfo& info)
{
    m_shouldSyncFrame = true;
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetCompositingLayerState(id, info));
}

void LayerTreeCoordinator::syncLayerChildren(WebLayerID id, const Vector<WebLayerID>& children)
{
    m_shouldSyncFrame = true;
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetCompositingLayerChildren(id, children));
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
void LayerTreeCoordinator::syncCanvas(WebLayerID id, const IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer, int flags)
#else
void LayerTreeCoordinator::syncCanvas(WebLayerID id, const IntSize& canvasSize, uint64_t graphicsSurfaceToken, uint32_t frontBuffer)
#endif
{
    m_shouldSyncFrame = true;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    m_compositedContentLayers.add(id);
#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
    m_compositedContentCanvas2DLayers.add(id);
#endif
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SyncCanvas(id, canvasSize, graphicsSurfaceToken, frontBuffer, flags));
#else
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SyncCanvas(id, canvasSize, graphicsSurfaceToken, frontBuffer));
#endif
}

#if ENABLE(CSS_FILTERS)
void LayerTreeCoordinator::syncLayerFilters(WebLayerID id, const FilterOperations& filters)
{
    m_shouldSyncFrame = true;
#if ENABLE(CSS_SHADERS)
    checkCustomFilterProgramProxies(filters);
#endif
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetCompositingLayerFilters(id, filters));
}
#endif

#if ENABLE(CSS_SHADERS)
void LayerTreeCoordinator::checkCustomFilterProgramProxies(const FilterOperations& filters)
{
    // We need to create the WebCustomFilterProgramProxy objects before we get to serialize the
    // custom filters to the other process. That's because WebCustomFilterProgramProxy needs
    // to link back to the coordinator, so that we can send a message to the UI process when 
    // the program is not needed anymore.
    // Note that the serialization will only happen at a later time in ArgumentCoder<WebCore::FilterOperations>::encode.
    // At that point the program will only be serialized once. All the other times it will only use the ID of the program.
    for (size_t i = 0; i < filters.size(); ++i) {
        const FilterOperation* operation = filters.at(i);
        if (operation->getOperationType() != FilterOperation::VALIDATED_CUSTOM)
            continue;
        const ValidatedCustomFilterOperation* customOperation = static_cast<const ValidatedCustomFilterOperation*>(operation);
        ASSERT(customOperation->validatedProgram()->isInitialized());
        TextureMapperPlatformCompiledProgram* program = customOperation->validatedProgram()->platformCompiledProgram();
        if (!program->client())
            program->setClient(WebCustomFilterProgramProxy::create(this));
        else {
            WebCustomFilterProgramProxy* customFilterProgramProxy = static_cast<WebCustomFilterProgramProxy*>(program->client());
            if (!customFilterProgramProxy->client()) {
                // Just in case the LayerTreeCoordinator was destroyed and recreated.
                customFilterProgramProxy->setClient(this);
            } else {
                // If the client was not disconnected then this coordinator must be the client for it.
                ASSERT(customFilterProgramProxy->client() == this);
            }
        }
    }
}

void LayerTreeCoordinator::removeCustomFilterProgramProxy(WebCustomFilterProgramProxy* customFilterProgramProxy)
{
    // At this time the shader is not needed anymore, so we remove it from our set and 
    // send a message to the other process to delete it.
    m_customFilterPrograms.remove(customFilterProgramProxy);
    // FIXME: Send a message to delete the object on the UI process.
    // https://bugs.webkit.org/show_bug.cgi?id=101801
}

void LayerTreeCoordinator::disconnectCustomFilterPrograms()
{
    // Make sure that WebCore will not call into this coordinator anymore.
    HashSet<WebCustomFilterProgramProxy*>::iterator iter = m_customFilterPrograms.begin();
    for (; iter != m_customFilterPrograms.end(); ++iter)
        (*iter)->setClient(0);
}
#endif

void LayerTreeCoordinator::attachLayer(WebGraphicsLayer* layer)
{
    ASSERT(!m_registeredLayers.contains(layer));
    m_registeredLayers.add(layer);

    layer->setContentsScale(m_contentsScale);
    layer->adjustVisibleRect();

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    m_compositedContentLayersCount = compositedContentLayersCount(m_nonCompositedContentLayer.get());
#endif
}

void LayerTreeCoordinator::detachLayer(WebGraphicsLayer* layer)
{
    m_registeredLayers.remove(layer);
    m_shouldSyncFrame = true;
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::DeleteCompositingLayer(layer->id()));

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    m_compositedContentLayersCount = compositedContentLayersCount(m_nonCompositedContentLayer.get());
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    m_compositedContentLayers.remove(layer->id());
#endif
#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
    m_compositedContentCanvas2DLayers.remove(layer->id());
#endif
}

static void updateOffsetFromViewportForSelf(RenderLayer* renderLayer)
{
    // These conditions must match the conditions in RenderLayerCompositor::requiresCompositingForPosition.
    RenderLayerBacking* backing = renderLayer->backing();
    if (!backing)
        return;

#if ENABLE(TIZEN_CSS_FIXED_ACCELERATION)
    WebGraphicsLayer* graphicsLayer = toWebGraphicsLayer(backing->graphicsLayer());
    graphicsLayer->setFixedToViewport(false);
#endif

    RenderStyle* style = renderLayer->renderer()->style();
    if (!style)
        return;

    if (!renderLayer->renderer()->isOutOfFlowPositioned() || renderLayer->renderer()->style()->position() != FixedPosition)
        return;

    if (!renderLayer->renderer()->container()->isRenderView())
        return;

    if (!renderLayer->isStackingContainer())
        return;

#if !ENABLE(TIZEN_CSS_FIXED_ACCELERATION)
    WebGraphicsLayer* graphicsLayer = toWebGraphicsLayer(backing->graphicsLayer());
#endif
    graphicsLayer->setFixedToViewport(true);
}

static void updateOffsetFromViewportForLayer(RenderLayer* renderLayer)
{
    updateOffsetFromViewportForSelf(renderLayer);

    if (renderLayer->firstChild())
        updateOffsetFromViewportForLayer(renderLayer->firstChild());
    if (renderLayer->nextSibling())
        updateOffsetFromViewportForLayer(renderLayer->nextSibling());
}

void LayerTreeCoordinator::syncFixedLayers()
{
    if (!m_webPage->corePage()->settings() || !m_webPage->corePage()->settings()->acceleratedCompositingForFixedPositionEnabled())
        return;

#if !ENABLE(TIZEN_CSS_FIXED_ACCELERATION)
    if (!m_webPage->mainFrame()->view()->hasFixedObjects())
        return;
#endif

    RenderLayer* rootRenderLayer = m_webPage->mainFrame()->contentRenderer()->compositor()->rootRenderLayer();
    ASSERT(rootRenderLayer);
    if (rootRenderLayer->firstChild())
        updateOffsetFromViewportForLayer(rootRenderLayer->firstChild());
}

void LayerTreeCoordinator::lockAnimations()
{
    m_animationsLocked = true;
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetAnimationsLocked(true));
}

void LayerTreeCoordinator::unlockAnimations()
{
    if (!m_animationsLocked)
        return;

    m_animationsLocked = false;
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetAnimationsLocked(false));
}

void LayerTreeCoordinator::performScheduledLayerFlush()
{
#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
    if ((m_isSuspended || m_waitingForUIProcess) && !m_compositedContentCanvas2DLayers.isEmpty()) {
        Vector<uint32_t> weblayerIDsCopy;
        copyToVector(m_compositedContentCanvas2DLayers, weblayerIDsCopy);
        if (!m_isSuspended)
            m_compositedContentCanvas2DLayers.clear();
        for (size_t i = 0; i < weblayerIDsCopy.size(); ++i) {
            WebGraphicsLayer* layer = WebGraphicsLayer::layerByID(weblayerIDsCopy[i]);
            if (layer)
                layer->flushPlatformSurfaces();
        }
        return;
    }
#endif

    if (m_isSuspended || m_waitingForUIProcess)
        return;

#if PLATFORM(TIZEN)
    // Make sure that core page is onScreen. If it is offscreen, there won't be any RootGraphicsLayer attached.
    // And so there won't be any accelerated layer tree hierarchy to paint which will make flickering.
    if (!m_webPage->corePage()->isOnscreen())
        return;
#endif

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER) && !USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    // Make sure that any previously registered animation callbacks are being executed before we flush the layers.
    m_lastAnimationServiceTime = WTF::monotonicallyIncreasingTime();
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    if (!WebProcess::shared().isWaitingForJavaScriptPopupFinished())
        m_webPage->corePage()->mainFrame()->view()->serviceScriptedAnimations(convertSecondsToDOMTimeStamp(currentTime()));
#else
    m_webPage->corePage()->mainFrame()->view()->serviceScriptedAnimations(convertSecondsToDOMTimeStamp(currentTime()));
#endif
#endif

    // We lock the animations while performing layout, to avoid flickers caused by animations continuing in the UI process while
    // the web process layout wants to cancel them.
    lockAnimations();

    m_webPage->layoutIfNeeded();

#if ENABLE(TIZEN_DEFER_FIXED_LAYER_UPDATE)
    if (m_needUpdateFixedLayerPosition && m_webPage->mainFrameView()) {
        if (m_webPage->corePage()->settings()->acceleratedCompositingForFixedPositionEnabled())
            m_webPage->mainFrameView()->updateFixedElementsAfterScrolling();
        m_needUpdateFixedLayerPosition = false;
    }
#endif

#if PLATFORM(TIZEN)
    m_webPage->didChangeContents();
#endif

    // We can unlock the animations before flushing if there are no visible changes, for example if there are content updates
    // in a layer with opacity 0.
    bool canUnlockBeforeFlush = !m_isValid || !toWebGraphicsLayer(m_rootLayer.get())->hasPendingVisibleChanges();
    if (canUnlockBeforeFlush)
        unlockAnimations();

    if (!m_isValid)
        return;

#if ENABLE(TIZEN_ONESHOT_DRAWING_SYNCHRONIZATION)
    if (m_needsOneShotDrawingSynchronization) {
        scheduleLayerFlush();
        toWebGraphicsLayer(m_nonCompositedContentLayer.get())->adjustVisibleRect();
        m_needsOneShotDrawingSynchronization = false;
        return;
    }
#endif

    if (flushPendingLayerChanges())
        didPerformScheduledLayerFlush();
}

void LayerTreeCoordinator::didPerformScheduledLayerFlush()
{
    if (m_notifyAfterScheduledLayerFlush) {
        static_cast<DrawingAreaImpl*>(m_webPage->drawingArea())->layerHostDidFlushLayers();
        m_notifyAfterScheduledLayerFlush = false;
    }
}

void LayerTreeCoordinator::layerFlushTimerFired(Timer<LayerTreeCoordinator>*)
{
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    m_webPage->connection()->clearWorkItemWhileWaitingForSyncReply(m_layerFlushWorkItemToDispatchWhileWaitingForSyncReply);
    m_layerFlushWorkItemToDispatchWhileWaitingForSyncReply.clear();
#endif
    performScheduledLayerFlush();
}

void LayerTreeCoordinator::createPageOverlayLayer()
{
    ASSERT(!m_pageOverlayLayer);

    m_pageOverlayLayer = GraphicsLayer::create(this);
#ifndef NDEBUG
    m_pageOverlayLayer->setName("LayerTreeCoordinator page overlay content");
#endif

    m_pageOverlayLayer->setDrawsContent(true);
    m_pageOverlayLayer->setSize(m_webPage->size());

    m_rootLayer->addChild(m_pageOverlayLayer.get());
}

void LayerTreeCoordinator::destroyPageOverlayLayer()
{
    ASSERT(m_pageOverlayLayer);
    m_pageOverlayLayer->removeFromParent();
    m_pageOverlayLayer = nullptr;
}

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
void LayerTreeCoordinator::createLowScaleNonCompositedLayer()
{
    if (m_lowScaleNonCompositedLayer)
        return;

    m_lowScaleNonCompositedLayer = GraphicsLayer::create(this);
    WebGraphicsLayer* webLowScaleNonCompositedLayer = toWebGraphicsLayer(m_lowScaleNonCompositedLayer.get());
    webLowScaleNonCompositedLayer->setLowScaleNonCompositedLayer(true);
#ifndef NDEBUG
    m_lowScaleNonCompositedLayer->setName("LayerTreeCoordinator low scale non-composited content");
#endif
    m_lowScaleNonCompositedLayer->setDrawsContent(true);
    m_lowScaleNonCompositedLayer->setSize(m_webPage->size());

    // add low scale non composited layer to below the non composited layer
    // so that low scale non-composited layer shows below non-composited layer.
    m_rootLayer->addChildBelow(m_lowScaleNonCompositedLayer.get() , m_nonCompositedContentLayer.get());

    // send low scale layer ID to UIProcess for excluding from TextureMapper Cull.
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetLowScaleNonCompositedLayerID(webLowScaleNonCompositedLayer->id()));
}

void LayerTreeCoordinator::destroyLowScaleNonCompositedLayer()
{
    ASSERT(m_lowScaleNonCompositedLayer);
    m_lowScaleNonCompositedLayer->removeFromParent();
    m_lowScaleNonCompositedLayer = nullptr;
}
#endif

int64_t LayerTreeCoordinator::adoptImageBackingStore(Image* image)
{
    if (!image)
        return InvalidWebLayerID;

    int64_t key = 0;

#if PLATFORM(QT)
    QImage* nativeImage = image->nativeImageForCurrentFrame();

    if (!nativeImage)
        return InvalidWebLayerID;

    key = nativeImage->cacheKey();
#elif ENABLE(TIZEN_WEBKIT2_TILED_AC)
    key = (int64_t)(image);
#endif

    HashMap<int64_t, int>::iterator it = m_directlyCompositedImageRefCounts.find(key);

    if (it != m_directlyCompositedImageRefCounts.end()) {
        ++(it->second);
        return key;
    }

    RefPtr<ShareableBitmap> bitmap = ShareableBitmap::createShareable(IntSize(image->size()), (!image->currentFrameKnownToBeOpaque() ? ShareableBitmap::SupportsAlpha : 0));
    {
        OwnPtr<WebCore::GraphicsContext> graphicsContext = bitmap->createGraphicsContext();
        graphicsContext->drawImage(image, ColorSpaceDeviceRGB, IntPoint::zero());
    }

    ShareableBitmap::Handle handle;
    bitmap->createHandle(handle);
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::CreateDirectlyCompositedImage(key, handle));
    m_directlyCompositedImageRefCounts.add(key, 1);
    return key;
}

void LayerTreeCoordinator::releaseImageBackingStore(int64_t key)
{
    if (!key)
        return;
    HashMap<int64_t, int>::iterator it = m_directlyCompositedImageRefCounts.find(key);
    if (it == m_directlyCompositedImageRefCounts.end())
        return;

    it->second--;

    if (it->second)
        return;

    m_directlyCompositedImageRefCounts.remove(it);
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::DestroyDirectlyCompositedImage(key));
}


void LayerTreeCoordinator::notifyAnimationStarted(const WebCore::GraphicsLayer*, double time)
{
}

void LayerTreeCoordinator::notifySyncRequired(const WebCore::GraphicsLayer*)
{
}

void LayerTreeCoordinator::paintContents(const WebCore::GraphicsLayer* graphicsLayer, WebCore::GraphicsContext& graphicsContext, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& clipRect)
{
    if (graphicsLayer == m_nonCompositedContentLayer) {
        m_webPage->drawRect(graphicsContext, clipRect);
        return;
    }

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (graphicsLayer == m_lowScaleNonCompositedLayer) {
        m_webPage->drawRect(graphicsContext, clipRect);
        return;
    }
#endif

    if (graphicsLayer == m_pageOverlayLayer) {
        // Overlays contain transparent contents and won't clear the context as part of their rendering, so we do it here.
        graphicsContext.clearRect(clipRect);
        m_webPage->drawPageOverlay(graphicsContext, clipRect);
        return;
    }
}

bool LayerTreeCoordinator::showDebugBorders(const WebCore::GraphicsLayer*) const
{
    return m_webPage->corePage()->settings()->showDebugBorders();
}

bool LayerTreeCoordinator::showRepaintCounter(const WebCore::GraphicsLayer*) const
{
    return m_webPage->corePage()->settings()->showRepaintCounter();
}

bool LayerTreeHost::supportsAcceleratedCompositing()
{
    return true;
}

void LayerTreeCoordinator::createTile(WebLayerID layerID, int tileID, const SurfaceUpdateInfo& updateInfo, const WebCore::IntRect& targetRect)
{
    m_shouldSyncFrame = true;
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::CreateTileForLayer(layerID, tileID, targetRect, updateInfo));
}

void LayerTreeCoordinator::updateTile(WebLayerID layerID, int tileID, const SurfaceUpdateInfo& updateInfo, const WebCore::IntRect& targetRect)
{
    m_shouldSyncFrame = true;
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::UpdateTileForLayer(layerID, tileID, targetRect, updateInfo));
}

void LayerTreeCoordinator::removeTile(WebLayerID layerID, int tileID)
{
    m_shouldSyncFrame = true;
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::RemoveTileForLayer(layerID, tileID));
}

WebCore::IntRect LayerTreeCoordinator::visibleContentsRect() const
{
    return m_visibleContentsRect;
}

void LayerTreeCoordinator::setLayerAnimations(WebLayerID layerID, const GraphicsLayerAnimations& animations)
{
    m_shouldSyncFrame = true;
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetLayerAnimations(layerID, animations.getActiveAnimations()));
}

#if PLATFORM(TIZEN)
void LayerTreeCoordinator::setVisibleContentsRectAndScaleForBackingStore(const WebCore::IntRect& rect,const float scale)
{
    bool contentsRectDidChange = rect != m_visibleContentsRect;
    bool contentsScaleDidChange = scale != m_contentsScale;

    if (contentsRectDidChange || contentsScaleDidChange) {
        m_visibleContentsRect = rect;
        m_contentsScale = scale;
        HashSet<WebCore::WebGraphicsLayer*>::iterator end = m_registeredLayers.end();
        for (HashSet<WebCore::WebGraphicsLayer*>::iterator it = m_registeredLayers.begin(); it != end; ++it) {
            if (contentsScaleDidChange)
                (*it)->setContentsScale(scale);
            if (contentsRectDidChange)
                (*it)->adjustVisibleRect();
        }
    }
}
#endif

void LayerTreeCoordinator::setVisibleContentsRect(const IntRect& rect, float scale, const FloatPoint& trajectoryVector)
{
    bool contentsRectDidChange = rect != m_visibleContentsRect;
    bool contentsScaleDidChange = scale != m_contentsScale;

    if (trajectoryVector != FloatPoint::zero()) {
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        if (!m_updateLowScaleLayerOnly)
#endif
        toWebGraphicsLayer(m_nonCompositedContentLayer.get())->setVisibleContentRectTrajectoryVector(trajectoryVector);

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        if (m_lowScaleNonCompositedLayer)
            toWebGraphicsLayer(m_lowScaleNonCompositedLayer.get())->setVisibleContentRectTrajectoryVector(trajectoryVector);
#endif
    }

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if ((contentsRectDidChange || contentsScaleDidChange) || m_refreshAllLayer) {
#else
    if (contentsRectDidChange || contentsScaleDidChange) {
#endif
        m_visibleContentsRect = rect;
        m_contentsScale = scale;

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
        if (m_updateLowScaleLayerOnly && !contentsScaleDidChange) {
            if (m_lowScaleNonCompositedLayer)
                toWebGraphicsLayer(m_lowScaleNonCompositedLayer.get())->setVisibleContentRectTrajectoryVector(trajectoryVector);
        } else
#endif
        {
            HashSet<WebCore::WebGraphicsLayer*>::iterator end = m_registeredLayers.end();
            for (HashSet<WebCore::WebGraphicsLayer*>::iterator it = m_registeredLayers.begin(); it != end; ++it) {
                if (contentsScaleDidChange)
                    (*it)->setContentsScale(scale);

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
                if (contentsRectDidChange || m_refreshAllLayer)
#else
                if (contentsRectDidChange)
#endif
#if ENABLE(TIZEN_WEBKIT2_PRE_RENDERING_WITH_DIRECTIVITY)
                (*it)->setVisibleContentRectTrajectoryVector(trajectoryVector);
#else
                (*it)->adjustVisibleRect();
#endif
            }
        }
    }

    scheduleLayerFlush();
    if (m_webPage->useFixedLayout()) {
#if ENABLE(TIZEN_DEFER_FIXED_LAYER_UPDATE)
        m_webPage->setFixedVisibleContentRectWithoutFixedElementUpdate(rect);
        m_needUpdateFixedLayerPosition = true;
#else
        m_webPage->setFixedVisibleContentRect(rect);
#endif
    }

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    m_refreshAllLayer = false;
#endif
}

#if USE(UI_SIDE_COMPOSITING)
void LayerTreeCoordinator::scheduleAnimation()
{
    if (m_waitingForUIProcess)
        return;

    if (m_layerFlushTimer.isActive())
        return;
#if ENABLE(REQUEST_ANIMATION_FRAME)
     // According to the requestAnimationFrame spec, rAF callbacks should not be faster than 60FPS.
     static const double MinimalTimeoutForAnimations = 1. / 60.;
     m_layerFlushTimer.startOneShot(std::max<double>(0., MinimalTimeoutForAnimations - WTF::monotonicallyIncreasingTime() + m_lastAnimationServiceTime));
#else
     static const double cAnimationTimerDelay = 0.015;
     m_layerFlushTimer.startOneShot(cAnimationTimerDelay);
#endif
     scheduleLayerFlush();
}
#endif

void LayerTreeCoordinator::renderNextFrame()
{
    m_waitingForUIProcess = false;
#if ENABLE(TIZEN_SYNC_REQUEST_ANIMATION_FRAME)
#if ENABLE(TIZEN_LAYER_FLUSH_THROTTLING)
    if (m_layerFlushTimerDelay <= 0.0)
#endif
        m_webPage->resumeAnimationController();
#endif
    scheduleLayerFlush();
    for (unsigned i = 0; i < m_updateAtlases.size(); ++i)
        m_updateAtlases[i]->didSwapBuffers();

#if ENABLE(TIZEN_SYNC_REQUEST_ANIMATION_FRAME)
#if ENABLE(TIZEN_LAYER_FLUSH_THROTTLING)
    if (m_layerFlushTimerDelay > 0.0)
        m_webPage->resumeAnimationController();
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    SharedPlatformSurfaceManagement::getInstance().doDeferredRemove();
#endif
}

#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
void LayerTreeCoordinator::performScheduledLayerFlushForcely()
{
    if (!WebProcess::shared().isWaitingForJavaScriptPopupFinished())
        return;

    m_layerFlushTimer.stop();
    m_webPage->connection()->clearWorkItemWhileWaitingForSyncReply(m_layerFlushWorkItemToDispatchWhileWaitingForSyncReply);
    m_layerFlushWorkItemToDispatchWhileWaitingForSyncReply.clear();

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    m_webPage->corePage()->chrome()->client()->resizeEventDone();
#endif

    m_waitingForUIProcess = false;

    performScheduledLayerFlush();
}
#endif

bool LayerTreeCoordinator::layerTreeTileUpdatesAllowed() const
{
    return !m_isSuspended && !m_waitingForUIProcess;
}

void LayerTreeCoordinator::purgeBackingStores()
{
    HashSet<WebCore::WebGraphicsLayer*>::iterator end = m_registeredLayers.end();
    for (HashSet<WebCore::WebGraphicsLayer*>::iterator it = m_registeredLayers.begin(); it != end; ++it)
        (*it)->purgeBackingStores();

    ASSERT(!m_directlyCompositedImageRefCounts.size());
    m_updateAtlases.clear();
}

PassOwnPtr<WebCore::GraphicsContext> LayerTreeCoordinator::beginContentUpdate(const WebCore::IntSize& size, ShareableBitmap::Flags flags, ShareableSurface::Handle& handle, WebCore::IntPoint& offset)
{
    OwnPtr<WebCore::GraphicsContext> graphicsContext;
    for (unsigned i = 0; i < m_updateAtlases.size(); ++i) {
        UpdateAtlas* atlas = m_updateAtlases[i].get();
        if (atlas->flags() == flags) {
            // This will return null if there is no available buffer space.
            graphicsContext = atlas->beginPaintingOnAvailableBuffer(handle, size, offset);
            if (graphicsContext)
                return graphicsContext.release();
        }
    }

    static const int ScratchBufferDimension = 1024;
    m_updateAtlases.append(adoptPtr(new UpdateAtlas(ScratchBufferDimension, flags)));
    scheduleReleaseInactiveAtlases();
    return m_updateAtlases.last()->beginPaintingOnAvailableBuffer(handle, size, offset);
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC) && !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void LayerTreeCoordinator::pauseRendering()
{
    m_isSuspended = true;
#if ENABLE(WEBGL)
    // Make sure compositing webgl layer after returning from home screen.
    HashSet<WebCore::WebGraphicsLayer*>::iterator end = m_registeredLayers.end();
    for (HashSet<WebCore::WebGraphicsLayer*>::iterator it = m_registeredLayers.begin(); it != end; ++it) {
        if ((*it)->contentType() == WebLayerInfo::Canvas3DContentType)
            (*it)->markCanvasPlatformLayerNeedsUpdate();
    }
#endif
}
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void LayerTreeCoordinator::freePlatformSurface(int layerID, int platformSurfaceId)
{
    WebGraphicsLayer* layer = WebGraphicsLayer::layerByID(layerID);
    if (layer && (layer->contentType() == WebLayerInfo::Canvas3DContentType || layer->contentType() == WebLayerInfo::MediaContentType || layer->contentType() == WebLayerInfo::Canvas2DContentType)) {
        layer->freePlatformSurface(platformSurfaceId);

        if (m_suspendedJavaScript && layer->swapPlatformSurfaces()) {
            m_webPage->resumeJavaScriptAndResources();
            m_suspendedJavaScript = false;
        }
    }

    // Even after a graphics layer just destroyed in WebProcess side,
    // freePlatformSurface messages that destroyed layer can surely come.
    // Therefore, below code should be run always before layer validity check.
    WebProcess::shared().platformSurfacePool()->freePlatformSurface(platformSurfaceId);
}

void LayerTreeCoordinator::freePlatformSurfaceByTileID(int tileID)
{
    WebProcess::shared().platformSurfacePool()->freePlatformSurfaceByTileID(tileID);
}

void LayerTreeCoordinator::removePlatformSurface(int layerID, int platformSurfaceId)
{
    WebGraphicsLayer* layer = WebGraphicsLayer::layerByID(layerID);
    if (layer && (layer->contentType() == WebLayerInfo::Canvas3DContentType || layer->contentType() == WebLayerInfo::MediaContentType || layer->contentType() == WebLayerInfo::Canvas2DContentType))
        layer->removePlatformSurface(platformSurfaceId);
}
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
bool LayerTreeCoordinator::recordingSurfaceSetEnableGet()
{
    return recordingSurfaceAllowed() && m_webPage->recordingSurfaceEnabled();
}

bool LayerTreeCoordinator::recordingSurfaceSetLoadStartGet()
{
    if (m_webPage->recordingSurfaceLoadStart()) {
        m_webPage->setRecordingSurfaceLoadStart(false);
        return true;
    }
    return false;
}

bool LayerTreeCoordinator::recordingSurfaceSetLoadFinishedGet()
{
    if (m_webPage->recordingSurfaceLoadFinish()) {
        m_webPage->setRecordingSurfaceLoadFinish(false);
        return true;
    }
    return false;
}

bool LayerTreeCoordinator::recordingSurfaceAllowed()
{
    if (m_compositedContentLayersCount > 0)
        return false;

    return true;
}

int LayerTreeCoordinator::compositedContentLayersCount(GraphicsLayer* layer)
{
    int count = 0;
    int contentType = toWebGraphicsLayer(layer)->contentType();

    if (contentType == WebLayerInfo::Canvas3DContentType || contentType == WebLayerInfo::MediaContentType || contentType == WebLayerInfo::Canvas2DContentType)
        count = 1;

    const Vector<GraphicsLayer*>& childLayers = layer->children();
    for (int i = 0; i < childLayers.size(); ++i)
        count += compositedContentLayersCount(childLayers[i]);

    return count;
}
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
void LayerTreeCoordinator::setVisibleContentsLocationAndTrajectoryVectorForLayer(int layerID, const WebCore::IntPoint& visibleContentsLocation, const WebCore::FloatPoint& trajectoryVector)
{
    GraphicsLayer* contentsLayer = WebGraphicsLayer::layerByID(layerID);
    if (!contentsLayer)
        return;

    GraphicsLayer* scrollingLayer = contentsLayer->parent();
    if (!scrollingLayer)
        return;

    if (trajectoryVector != FloatPoint::zero()) {
        m_webPage->scrollOverflowWithTrajectoryVector(trajectoryVector);
        toWebGraphicsLayer(contentsLayer)->setVisibleContentRectTrajectoryVector(trajectoryVector);

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SCROLLBAR)
        const Vector<GraphicsLayer*>& childLayers = contentsLayer->children();
        for (unsigned i = 0; i < childLayers.size(); ++i)
            if (childLayers[i]->isScrollbar())
               toWebGraphicsLayer(childLayers[i])->startAnimation();
#endif
    }
    else
        toWebGraphicsLayer(contentsLayer)->adjustVisibleRect();

    scheduleLayerFlush();
}
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
void LayerTreeCoordinator::addOrUpdateScrollingLayer(WebCore::GraphicsLayer* scrollingLayer, WebCore::GraphicsLayer* contentsLayer, const WebCore::IntSize& scrollSize)
{
    if (!scrollingLayer || !contentsLayer)
        return;

#if !ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    toWebGraphicsLayer(contentsLayer)->setIsOverflow(true);
    // FIXME: We might need to consider paddingBox margin for overflow scroll layer
    // FloatPoint offset(-scrollingLayer->offsetFromRenderer().width() - scrollingLayer->boundsOrigin().x()
    //        , -scrollingLayer->offsetFromRenderer().height() - scrollingLayer->boundsOrigin().y());
    FloatPoint offset(scrollingLayer->boundsOrigin());
    contentsLayer->setPosition(-offset);

    IntRect visibleRect(FloatRect(offset, scrollingLayer->size()));

    // FIXME: Need to set trajectoryVector?
    // FloatPoint trajectoryVector(scrollingLayer->trajectoryVector());
    // if (trajectoryVector != FloatPoint::zero())
        //toWebGraphicsLayer(contentsLayer)->setVisibleContentRectTrajectoryVector(-trajectoryVector);

    // FIXME: Scale factor might be changed.
    // toWebGraphicsLayer(contentsLayer)->setContentsScale(m_contentsScale);
#else
    if (!toWebGraphicsLayer(contentsLayer)->isOverflow()) {
        setNeedsOneShotDrawingSynchronization();
        toWebGraphicsLayer(contentsLayer)->setIsOverflow(true);
    }
#endif
    toWebGraphicsLayer(contentsLayer)->adjustVisibleRect();
}

void LayerTreeCoordinator::removeScrollingLayer(WebCore::GraphicsLayer* scrollingLayer, WebCore::GraphicsLayer* contentsLayer)
{
    if (!scrollingLayer || !contentsLayer)
        return;

    toWebGraphicsLayer(contentsLayer)->setIsOverflow(false);
    toWebGraphicsLayer(contentsLayer)->syncLayerState();
}
#endif // ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)

#if ENABLE(TIZEN_ONESHOT_DRAWING_SYNCHRONIZATION)
void LayerTreeCoordinator::setNeedsOneShotDrawingSynchronization()
{
    m_needsOneShotDrawingSynchronization = true;
}
#endif

#if ENABLE(TIZEN_LAYER_FLUSH_THROTTLING)
void LayerTreeCoordinator::setDeferLayerFlush(bool deferLayerFlushEnabled)
{
    m_deferLayerFlushEnabled = deferLayerFlushEnabled;
    m_layerFlushTimerDelayLevel = 0;
    double newDelay = deferredLayerFlushDelay();

    // If the m_layerFlushTimer is already activated, we stop the old timer and then begin the timer with a new delay
    if (m_layerFlushTimerDelay != newDelay && m_layerFlushTimer.isActive()) {
        m_layerFlushTimer.stop();
        m_layerFlushTimer.startOneShot(newDelay);
    }
    m_layerFlushTimerDelay = newDelay;

#if ENABLE(TIZEN_SYNC_REQUEST_ANIMATION_FRAME)
    if (m_layerFlushTimerDelay == 0.0)
        m_webPage->resumeAnimationController();
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    // create low scale non composited layer after finishing the page load if GLAcceleratedMode is on.
    if (!deferLayerFlushEnabled && m_accelerationMode == OpenGLMode)
        createLowScaleNonCompositedLayer();
#endif
}

float LayerTreeCoordinator::deferredLayerFlushDelay()
{
    if (!m_deferLayerFlushEnabled)
         return 0;

    if (m_layerFlushTimerDelayLevel >= s_layerFlushTimerDelayMaxLevel)
        m_layerFlushTimerDelayLevel = s_layerFlushTimerDelayMaxLevel - 1;

    return s_layerFlushTimerDelayTable[m_layerFlushTimerDelayLevel++];
}
#endif

const double ReleaseInactiveAtlasesTimerInterval = 0.5;

void LayerTreeCoordinator::scheduleReleaseInactiveAtlases()
{
    if (!m_releaseInactiveAtlasesTimer.isActive())
        m_releaseInactiveAtlasesTimer.startRepeating(ReleaseInactiveAtlasesTimerInterval);
}

void LayerTreeCoordinator::releaseInactiveAtlasesTimerFired(Timer<LayerTreeCoordinator>*)
{
    // We always want to keep one atlas for non-composited content.
    OwnPtr<UpdateAtlas> atlasToKeepAnyway;
    bool foundActiveAtlasForNonCompositedContent = false;
    for (int i = m_updateAtlases.size() - 1;  i >= 0; --i) {
        UpdateAtlas* atlas = m_updateAtlases[i].get();
        if (!atlas->isInUse())
            atlas->addTimeInactive(ReleaseInactiveAtlasesTimerInterval);
        bool usableForNonCompositedContent = !atlas->supportsAlpha();
        if (atlas->isInactive()) {
            if (!foundActiveAtlasForNonCompositedContent && !atlasToKeepAnyway && usableForNonCompositedContent)
                atlasToKeepAnyway = m_updateAtlases[i].release();
            m_updateAtlases.remove(i);
        } else if (usableForNonCompositedContent)
            foundActiveAtlasForNonCompositedContent = true;
    }

    if (!foundActiveAtlasForNonCompositedContent && atlasToKeepAnyway)
        m_updateAtlases.append(atlasToKeepAnyway.release());

    if (m_updateAtlases.size() <= 1)
        m_releaseInactiveAtlasesTimer.stop();
}

#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
void LayerTreeCoordinator::setTouchEventTargetRects(const Vector<IntRect>& touchEventTargetRects)
{
    m_shouldSyncFrame = true;
    m_touchEventTargetRects = touchEventTargetRects;
    m_touchEventTargetRectsChanged = true;
}
#endif

#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
void LayerTreeCoordinator::updateOnlyVisibleAreaState(bool state)
{
    toWebGraphicsLayer(m_nonCompositedContentLayer.get())->updateOnlyVisibleAreaState(state);
}
#endif

void LayerTreeCoordinator::setBackgroundColor(const WebCore::Color& color)
{
    m_webPage->send(Messages::LayerTreeCoordinatorProxy::SetBackgroundColor(color));
}

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
void LayerTreeCoordinator::setCoverWithTilesForLowScaleLayerOnly(bool updateLowScaleLayerOnly)
{
    if (!m_lowScaleNonCompositedLayer)
        return;

    if (m_updateLowScaleLayerOnly && !updateLowScaleLayerOnly)
        m_refreshAllLayer = true;

    m_updateLowScaleLayerOnly = updateLowScaleLayerOnly;
    HashSet<WebCore::WebGraphicsLayer*>::iterator end = m_registeredLayers.end();
    for (HashSet<WebCore::WebGraphicsLayer*>::iterator it = m_registeredLayers.begin(); it != end; ++it)
        (*it)->setCanInvalidateLowScaleBackingStore(!updateLowScaleLayerOnly);
}
#endif

} // namespace WebKit
