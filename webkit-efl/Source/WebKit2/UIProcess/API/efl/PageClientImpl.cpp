/*
 * Copyright (C) 2011 Samsung Electronics
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
#include "PageClientImpl.h"

#include "EwkViewImpl.h"
#include "InputMethodContextEfl.h"
#include "NativeWebKeyboardEvent.h"
#include "NotImplemented.h"
#include "TransformationMatrix.h"
#include "WebContext.h"
#include "WebContextMenuProxy.h"
#include "WebPageProxy.h"
#include "WebPopupMenuProxyEfl.h"
#include "ewk_context.h"
#include "ewk_context_private.h"
#include "ewk_download_job.h"
#include "ewk_download_job_private.h"
#include "ewk_view.h"

#if PLATFORM(TIZEN)
#include "DrawingAreaProxyImpl.h"
#include "Editor.h"
#include "EflScreenUtilities.h"
#include "LayerTreeCoordinatorProxy.h"
#include "OpenGLShims.h"
#include "WebContextMenuProxyTizen.h"
#include "WebLayerTreeRenderer.h"
#include "WebPageGroup.h"
#include "WebPageMessages.h"
#include "WebPopupMenuProxyEfl.h"
#include "WebPreferences.h"
#include "ewk_view.h"
#include "ewk_view_utilx.h"
#include <Ecore_Evas.h>
#include <Ecore_X.h>

#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
#include "MainFrameScrollbarTizen.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
#include "ClipboardHelper.h"
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
#include "DragData.h"
#endif
#endif

#if ENABLE(TIZEN_CACHE_MEMORY_OPTIMIZATION)
#include "ewk_context_private.h"
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
#include "LinkMagnifierProxy.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_WEB_SEARCH)
#include <app_control_internal.h>
#endif

#if ENABLE(TIZEN_SCREEN_READER)
#include "ScreenReaderProxy.h"
#endif

#define EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE (1 << 12)
#define EVAS_GL_OPTIONS_DIRECT_OVERRIDE (1 << 13)

using namespace WebCore;
using namespace std;

namespace WebKit {

PageClientImpl::PageClientImpl(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
#if PLATFORM(TIZEN)
    , m_viewportConstraints()
    , m_viewFocused(false)
    , m_viewWindowActive(true)
    , m_pageDidRendered(true)
    , m_viewResizeCount(0)
    , m_viewportFitsToContent(false)
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    , m_visibleContentRect(IntRect())
    , m_scaleFactor(0)
    , m_hasSuspendedContent(false)
#endif
#if ENABLE(TIZEN_WEBKIT2_HISTORICAL_RESTORE_VISIBLE_CONTENT_RECT)
    , m_restoredScrollPosition(IntPoint())
    , m_restoredScaleFactor(0)
#endif
#if ENABLE(TIZEN_WEBKIT2_BEFORE_PAGE_RENDERED_SCROLL_POSITION)
    , m_scrollPositionBeforePageRendered(IntPoint())
#endif
    , m_isVisible(true)
    , m_isScrollableLayerFocused(false)
    , m_isScrollableNodeFocused(false)
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    , m_isOverflowMovingEnabled(false)
#endif
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    , m_urlBarScrollTirgger(false)
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    , m_shouldMakeBackupTexture(false)
    , m_shouldShowBackupTexture(false)
#endif
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    , m_isContextMenuVisible(false)
#endif
#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    , m_waitFrameOfNewViewortSize(false)
    , m_preparingRotation(false)
#endif
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    , m_isForeground(true)
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    , m_shouldPaintOnlyLowScaleLayer(false)
    , m_updateLowScaleLayerOnly(false)
#endif
    , m_nonemptyLayoutRendered(false)
    , m_displayViewportTimer(this, &PageClientImpl::displayViewportTimerFired)
#endif // #if PLATFORM(TIZEN)
{
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
    setenv("CAIRO_GL_COMPOSITOR", "msaa", 1);
    setenv("CAIRO_GL_LAZY_FLUSHING", "yes", 1);
#endif

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    m_offlinePageSave = OfflinePageSave::create(m_viewImpl);
#endif
#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
    m_clipboardHelper = ClipboardHelper::create(m_viewImpl);
#endif
#if ENABLE(TIZEN_DRAG_SUPPORT)
    m_drag = Drag::create(m_viewImpl);
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    m_autoFillPopup = AutoFillPopup::create(m_viewImpl);
#endif
#endif
}

PageClientImpl::~PageClientImpl()
{
    TIZEN_LOGI("PageClientImpl [%p]", this);

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    ewkViewScreenShotForRotationDelete(m_viewImpl->view());
#endif

#if !PLATFORM(TIZEN)
    // FIXME: According to the open source, ~WebPageProxy() must move here.
    if (m_viewImpl && m_viewImpl->page())
        m_viewImpl->page()->close();
#endif
}

#if PLATFORM(TIZEN)
PageClientImpl::ViewportConstraints PageClientImpl::viewportConstraints()
{
    PageClientImpl::ViewportConstraints constraints = m_viewportConstraints;
    if (m_viewImpl->page()->pageGroup()->preferences()->forceZoomEnabled()) {
        constraints.userScalable = true;
        constraints.maximumScale = 4 * m_viewImpl->page()->deviceScaleFactor();
    }
    return constraints;
}

PageClientImpl::ViewportConstraints PageClientImpl::computeViewportConstraints(const WebCore::ViewportAttributes& attributes)
{
    PageClientImpl::ViewportConstraints constraints;
    constraints.minimumScale = attributes.minimumScale * attributes.devicePixelRatio;
    constraints.maximumScale = attributes.maximumScale * attributes.devicePixelRatio;
    constraints.userScalable = attributes.userScalable;
    constraints.layoutSize = attributes.layoutSize;
    constraints.fixedInitialScale = (ViewportArguments::ValueAuto != attributes.initialScale);

    bool forceZoomEnabled = m_viewImpl->page()->pageGroup()->preferences()->forceZoomEnabled();
    if (forceZoomEnabled && (fabs(constraints.minimumScale - constraints.maximumScale) < numeric_limits<float>::epsilon())) {
        constraints.maximumScale = 5.0;
        constraints.userScalable = true;
    }

    bool autoFittingEnabled = m_viewImpl->page()->pageGroup()->preferences()->autoFittingEnabled();
    if (autoFittingEnabled)
        constraints.initialScale = attributes.minimumScale * attributes.devicePixelRatio;
    else {
        // if content doesn't set initial scale value, set readable scale factor
        // if not, set initial scale factor of viewport attribute
        if (attributes.initialScale == ViewportArguments::ValueAuto) {
            constraints.initialScale = m_viewImpl->page()->deviceScaleFactor();
            // Enable fixedInitialScale like content is defining initial scale factor.
            // Because it should be fixed by disabled auto fit setting
            constraints.fixedInitialScale = true;
        } else
            constraints.initialScale = attributes.initialScale * attributes.devicePixelRatio;
    }

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("initial: [%f -> %f], min/max: [%f, %f]",
        constraints.initialScale, adjustScaleWithViewport(constraints.initialScale),
        m_viewportConstraints.minimumScale, m_viewportConstraints.maximumScale);
#endif

    m_viewportConstraints = constraints;

    // adjust scale with both minimum and maximum scale factor
    constraints.initialScale = adjustScaleWithViewport(constraints.initialScale);

    return constraints;
}

double PageClientImpl::adjustScaleWithViewport(double scale)
{
    double minimumScale = min(viewportConstraints().minimumScale, viewportConstraints().maximumScale);
    double maximumScale = max(viewportConstraints().minimumScale, viewportConstraints().maximumScale);
    return clampTo(scale, minimumScale, maximumScale);
}

#if USE(TILED_BACKING_STORE) && ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
void PageClientImpl::updateViewportSize(const IntSize& viewportSize)
{
    if (m_viewportSize == viewportSize)
        return;

    // save current visible content rect ratio per contents' size
    if (drawingArea() && !drawingArea()->isWaitingForDidUpdateBackingStoreState())
        m_viewResizeCount++;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI(" view size: [%d, %d], scale: [%.2f, %.2f, %.2f], resizeCount: [%d]", viewportSize.width(), viewportSize.height(),
        m_scaleFactor, viewportConstraints().minimumScale, viewportConstraints().maximumScale, m_viewResizeCount);
#endif

    // update viewport size of webkit
    m_visibleContentRect = adjustVisibleContentRect(m_visibleContentRect, m_scaleFactor);
    m_viewImpl->setScrollPosition(m_visibleContentRect.location());

    m_viewportSize = viewportSize;
}

void PageClientImpl::updateVisibleContentRectSize(const IntSize& size)
{
    // update visible content rect's size and scroll position
    m_visibleContentRect.setSize(size);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("size: [%d, %d], visibleContentRect: [%d, %d, %d, %d]", size.width(), size.height(),
        m_visibleContentRect.x(), m_visibleContentRect.y(), m_visibleContentRect.width(), m_visibleContentRect.height());
#endif
#if ENABLE(TIZEN_DRAG_SUPPORT)
    if (isDragMode())
        updateDragPosition();
#endif
}
#endif

void PageClientImpl::initializeVisibleContentRect()
{
    ewk_view_resume_painting(m_viewImpl->view());

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    IntPoint initialScrollPosition;
    float initialScaleFactor = m_viewportConstraints.initialScale;
#if ENABLE(TIZEN_WEBKIT2_BEFORE_PAGE_RENDERED_SCROLL_POSITION)
    initialScrollPosition = m_scrollPositionBeforePageRendered;
    m_scrollPositionBeforePageRendered = IntPoint();
#endif

#if ENABLE(TIZEN_WEBKIT2_RESTORE_SCROLLPOINT_ON_FRAME_LOAD_FINISH)
    // If it's restored by restoreVisibleContentRect, return
    if (restoreVisibleContentRect()) {
        m_restoredScaleFactor = 0;
        m_restoredScrollPosition = IntPoint();
        return;
    }
#endif

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("visibleContentRect: [%d, %d, %d, %d], scale factor: [%.2f]",
        initialScrollPosition.x(), initialScrollPosition.y(),
        m_visibleContentRect.width(), m_visibleContentRect.height(), initialScaleFactor);
#endif
    setVisibleContentRect(IntRect(initialScrollPosition, m_visibleContentRect.size()), initialScaleFactor);
    displayViewport();
#else
    // Set initial scale.
    m_viewImpl->page()->scalePage(m_viewportConstraints.initialScale, IntPoint(0, 0));
#endif
}

double PageClientImpl::availableMinimumScale()
{
    // recalculate minimum scale factor if contents' width exceeds viewport layout width and userScalable is true.
    // minimum scale factor shouldn't be smaller than 0.25(minimum zoom level)
    IntSize contentsSize = m_viewImpl->page()->contentsSize();
    // At times contentsSize.width might come as 0 or not set properly. In that case horizontalMinScale
    // becomes infinity. So to avoid that need to set minimum zoom level as 0.25
    if (!contentsSize.width())
        return 0.25;
    double horizontalMinScale = max(((double)viewSize().width() / contentsSize.width()), 0.25);
    return horizontalMinScale;
}

bool PageClientImpl::isContentSizeSameAsViewport()
{
    IntSize contentsSize = m_viewImpl->page()->contentsSize();
    return contentsSize == m_viewportSize;
}

void PageClientImpl::fitViewportToContent(bool immediately)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("scroll position: [%d, %d], scale factor: [%.2f]", m_visibleContentRect.x(), m_visibleContentRect.y(), m_viewportConstraints.minimumScale);
#endif

    // if current scale factor is already minimized, do not update visible content rect
    if (fabs(scaleFactor() - m_viewportConstraints.minimumScale) < numeric_limits<float>::epsilon())
        return;

    setVisibleContentRect(m_visibleContentRect, m_viewportConstraints.minimumScale);
    const float displayDelay = 0.05;
    immediately ? displayViewport() : scheduleDisplayViewport(displayDelay);
}

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
bool PageClientImpl::scrollBy(IntSize scrollOffset)
{
#if ENABLE(TIZEN_FULLSCREEN_API)
    // We don't want to process scrolling in the FullScreen mode.
    if (m_viewImpl->page()->fullScreenManager()->isFullScreen())
        return false;
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    // scrollOffset means device screen coordiate, not an actual offset of contents.
    // Therefore, scrollOffset should be nomalized in order to make a tiled backing store
    // in the actual scale.
    IntSize scaledScrollOffset = m_viewImpl->transformFromScene().mapSize(scrollOffset);
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SPLIT)
    if ((m_isScrollableLayerFocused || m_isScrollableNodeFocused)) {
        if (m_viewImpl->page()->splitScrollOverflowEnabled()) {
            FloatPoint remainingOffset = m_viewImpl->page()->splitScrollOverflow(FloatPoint(scaledScrollOffset.width(), scaledScrollOffset.height()));
            if (!remainingOffset.x() && !remainingOffset.y()) {
                displayViewport();
                return false;
            }
            scrollOffset = m_viewImpl->transformToScene().mapSize(IntSize(remainingOffset.x(), remainingOffset.y()));
        } else {
            if (m_viewImpl->page()->scrollOverflow(FloatPoint(scaledScrollOffset.width(), scaledScrollOffset.height()))) {
                if (!m_isOverflowMovingEnabled) {
                    m_isOverflowMovingEnabled = true;
                    evas_object_smart_callback_call(m_viewImpl->view(), "overflow,scroll,on", 0);
                }
                displayViewport();
                return false;
            }
        }
    }
#else
    if ((m_isScrollableLayerFocused || m_isScrollableNodeFocused)
        && m_viewImpl->page()->scrollOverflow(FloatPoint(scaledScrollOffset.width(), scaledScrollOffset.height()))) {
        if (!m_isOverflowMovingEnabled) {
            m_isOverflowMovingEnabled = true;
            evas_object_smart_callback_call(m_viewImpl->view(), "overflow,scroll,on", 0);
        }
        displayViewport();
        return false;
    }
#endif
    if (m_isOverflowMovingEnabled) {
        m_isOverflowMovingEnabled = false;
        evas_object_smart_callback_call(m_viewImpl->view(), "overflow,scroll,off", 0);
    }
#endif
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    if (m_urlBarScrollTirgger) {
        int urlBarDelta = scrollOffset.height();
        evas_object_smart_callback_call(m_viewImpl->view(), "urlbar,scroll", &urlBarDelta);
        m_urlBarScrollTirgger = false;
        scrollOffset.setHeight(0);
        if ((int)(m_viewImpl->page()->viewSize().width()) == (int)(m_viewImpl->pageProxy->contentsSize().width() * scaleFactor()))
            scrollOffset.setWidth(0);

        if (!scrollOffset.width())
            return false;
    }
#endif

    IntPoint oldScrollPosition = scrollPosition();
    setVisibleContentRect(IntRect(oldScrollPosition + scrollOffset, m_visibleContentRect.size()), scaleFactor(), FloatPoint(scrollOffset.width(), scrollOffset.height()));
    displayViewport();

    return true;
}

void PageClientImpl::scrollTo(IntPoint requestedScrollPosition)
{
#if ENABLE(TIZEN_FULLSCREEN_API)
    // We don't want to process scrolling in the FullScreen mode.
    if (m_viewImpl->page()->fullScreenManager()->isFullScreen())
        return;
#endif
    setVisibleContentRect(IntRect(requestedScrollPosition, m_visibleContentRect.size()), scaleFactor());
    displayViewport();
}
#endif // #if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)

#endif // #if PLATFORM(TIZEN)

EwkViewImpl* PageClientImpl::viewImpl() const
{
    return m_viewImpl;
}

// PageClient
PassOwnPtr<DrawingAreaProxy> PageClientImpl::createDrawingAreaProxy()
{
    return DrawingAreaProxyImpl::create(m_viewImpl->page());
}

void PageClientImpl::setViewNeedsDisplay(const WebCore::IntRect& rect)
{
#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    if (m_waitFrameOfNewViewortSize) {
        ewk_view_mark_for_sync(m_viewImpl->view());
        m_preparingRotation = false;
        m_waitFrameOfNewViewortSize = false;
#if ENABLE(TIZEN_FULLSCREEN_API)
        if (m_viewImpl->page()->fullScreenManager()->isFullScreen())
            m_viewImpl->page()->fullScreenManager()->didRotateScreen();
#endif
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
        if (!m_viewImpl->page()->fullScreenManager()->isUsingHwVideoOverlay())
#endif
        ewkViewScreenShotForRotationDelete(m_viewImpl->view());
        evas_object_smart_callback_call(m_viewImpl->view(), "rotate,prepared", 0);
    } else
#endif
#if PLATFORM(TIZEN)
    ewk_view_mark_for_sync(m_viewImpl->view());
#else
    m_viewImpl->redrawRegion(rect);
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
    LinkMagnifierProxy::linkMagnifier().requestUpdate(rect);
#endif
}

void PageClientImpl::displayView()
{
    notImplemented();
}

void PageClientImpl::scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize&)
{
    setViewNeedsDisplay(scrollRect);
}

WebCore::IntSize PageClientImpl::viewSize()
{
    return m_viewImpl->size();
}

bool PageClientImpl::isViewVisible()
{
#if PLATFORM(TIZEN)
    return m_isVisible;
#else
    return m_viewImpl->isVisible();
#endif
}

bool PageClientImpl::isViewInWindow()
{
    notImplemented();
    return true;
}

void PageClientImpl::processDidCrash()
{
    // Check if loading was ongoing, when web process crashed.
    double loadProgress = ewk_view_load_progress_get(m_viewImpl->view());
    if (loadProgress >= 0 && loadProgress < 1)
        m_viewImpl->informLoadProgress(1);
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("Notify ewkviewimpl of crash: %x", m_viewImpl);
#endif
    m_viewImpl->informWebProcessCrashed();
}

void PageClientImpl::didRelaunchProcess()
{
    const char* themePath = m_viewImpl->themePath();
    if (themePath)
        m_viewImpl->page()->setThemePath(themePath);
}

void PageClientImpl::pageClosed()
{
    notImplemented();
}

void PageClientImpl::toolTipChanged(const String&, const String& newToolTip)
{
    m_viewImpl->informTooltipTextChange(newToolTip);
}

void PageClientImpl::setCursor(const Cursor& cursor)
{
    m_viewImpl->setCursor(cursor);
}

void PageClientImpl::setCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves)
{
    notImplemented();
}

void PageClientImpl::didChangeViewportProperties(const WebCore::ViewportAttributes& attributes)
{
    m_viewResizeCount = max(m_viewResizeCount - 1, 0);
    if (m_viewResizeCount)
        return;

    float scaleRatioBeforeResize = m_scaleFactor / m_viewportConstraints.minimumScale;

    m_viewportConstraints = computeViewportConstraints(attributes);

    // Initially, m_scaleFactor is not decided yet.
    // So, we should update visible content rect at here.
    if (!m_scaleFactor) {
        m_scaleFactor = m_viewportConstraints.initialScale;
        return;
    }

    // if content is reloaded, contents size will not be changed
    // so, we need to calculate minimum scale here.
    // if content size is changed later, minimum scale will be re-calculated on didChangeContentsSize()
    if (m_viewportConstraints.userScalable) {
        if (!m_viewportConstraints.fixedInitialScale
            && fabs(m_viewportConstraints.initialScale - m_viewportConstraints.minimumScale) < numeric_limits<float>::epsilon())
            m_viewportConstraints.initialScale = availableMinimumScale();
        m_viewportConstraints.minimumScale = availableMinimumScale();
    }

#if ENABLE(TIZEN_GESTURE)
    bool doubleTapEnabled = userScalable();
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    doubleTapEnabled |= ewk_settings_select_word_automatically_get(ewk_view_settings_get(m_viewImpl->view()));
#endif
    m_viewImpl->setDoubleTapEnabled(doubleTapEnabled);
#endif

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("layout size: [%d, %d], scale: [%.2f, %.2f, %.2f], user scalable: [%s]",
        m_viewportConstraints.layoutSize.width(), m_viewportConstraints.layoutSize.height(),
        m_viewportConstraints.initialScale, m_viewportConstraints.minimumScale, m_viewportConstraints.maximumScale,
        m_viewportConstraints.userScalable ? "TRUE" : "FALSE");
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    if (canUpdateVisibleContentRect())
        setLowScaleLayerFlag(true);
#endif

    // setVisibleContentRect() should be called to adjust visible content rect only when view is resized
    if (!canUpdateVisibleContentRect())
        return;

    float newScale = scaleFactor();
    IntPoint newScrollPosition = m_visibleContentRect.location();

    // we need to keep visible content rect after resizing view
    newScale = m_viewportConstraints.minimumScale * scaleRatioBeforeResize;
    if (m_viewportFitsToContent)
        newScale = m_viewportConstraints.minimumScale;
    newScrollPosition.scale(newScale / m_scaleFactor, newScale / m_scaleFactor);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("scroll position: [%d, %d], new scale: [%.2f], ratio: [%.2f], fitted: [%s]", m_visibleContentRect.x(), m_visibleContentRect.y(), newScale, scaleRatioBeforeResize, m_viewportFitsToContent ? "Fitted" : "Not fitted");
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    // updateVisibleContentRectSize() should be called here
    // because didChangeViewportProperties() can be called before ewk_view_smart_calculate.
    // In this case, visibleContentRect's size is not updated properly yet.
    // So, force updating visibleContentRect by viewSize
    PageClientImpl::updateVisibleContentRectSize(viewSize());
    setVisibleContentRect(IntRect(newScrollPosition, m_visibleContentRect.size()), newScale);
#else
    m_viewImpl->page()->scalePage(newScale, newScrollPosition);
#endif

// Input field zoom will be triggered by ewk_view_focused_node_adjust() if it's necessary
if (ewk_view_focused_node_adjust(m_viewImpl->view(), EINA_TRUE, EINA_TRUE))
        return;

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_viewImpl->view()));
    InputMethodContextEfl* inputMethodContext = m_viewImpl->inputMethodContext();
    if (smartData->api->formdata_candidate_is_showing(smartData) && !inputMethodContext->isShow())
        smartData->api->formdata_candidate_hide(smartData);
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN) && ENABLE(TIZEN_EXTENSIBLE_API)
    if (m_preparingRotation && m_viewImpl->page()->fullScreenManager()->isUsingHwVideoOverlay()
        && ewk_context_tizen_extensible_api_string_get(m_viewImpl->ewkContext(), "rotation,lock")) {
        m_waitFrameOfNewViewortSize = true;
        ewk_view_resume_painting(m_viewImpl->view());
    }
#endif
#if ENABLE(TIZEN_FULLSCREEN_API)
    if (m_viewImpl->page()->fullScreenManager()->isFullScreen())
        m_viewImpl->page()->fullScreenManager()->updateMediaControlsStyle();
#endif
}

#if PLATFORM(TIZEN)
void PageClientImpl::registerEditCommand(PassRefPtr<WebEditCommandProxy> command, WebPageProxy::UndoOrRedo undoOrRedo)
{
    if (undoOrRedo == WebPageProxy::Undo) {
        m_undoCommands.append(command);

        int undoSize = m_undoCommands.size();
        evas_object_smart_callback_call(m_viewImpl->view(), "undo,size", &undoSize);
    }
    else {
        m_redoCommands.append(command);

        int redoSize = m_redoCommands.size();
        evas_object_smart_callback_call(m_viewImpl->view(), "redo,size", &redoSize);
    }
}

void PageClientImpl::clearAllEditCommands()
{
    m_undoCommands.clear();
    m_redoCommands.clear();

    int undoSize = m_undoCommands.size();
    evas_object_smart_callback_call(m_viewImpl->view(), "undo,size", &undoSize);

    int redoSize = m_redoCommands.size();
    evas_object_smart_callback_call(m_viewImpl->view(), "redo,size", &redoSize);
}

bool PageClientImpl::canUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    if (undoOrRedo == WebPageProxy::Undo)
        return !m_undoCommands.isEmpty();
    else
        return !m_redoCommands.isEmpty();
}

void PageClientImpl::executeUndoRedo(WebPageProxy::UndoOrRedo undoOrRedo)
{
    if (undoOrRedo == WebPageProxy::Undo) {
        m_undoCommands.last()->unapply();
        m_undoCommands.removeLast();

        int undoSize = m_undoCommands.size();
        evas_object_smart_callback_call(m_viewImpl->view(), "undo,size", &undoSize);
    } else {
        m_redoCommands.last()->reapply();
        m_redoCommands.removeLast();

        int redoSize = m_redoCommands.size();
        evas_object_smart_callback_call(m_viewImpl->view(), "redo,size", &redoSize);
    }
}
#else
void PageClientImpl::registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo)
{
    notImplemented();
}

void PageClientImpl::clearAllEditCommands()
{
    notImplemented();
}

bool PageClientImpl::canUndoRedo(WebPageProxy::UndoOrRedo)
{
    notImplemented();
    return false;
}

void PageClientImpl::executeUndoRedo(WebPageProxy::UndoOrRedo)
{
    notImplemented();
}
#endif

FloatRect PageClientImpl::convertToDeviceSpace(const FloatRect& viewRect)
{
    notImplemented();
    return viewRect;
}

FloatRect PageClientImpl::convertToUserSpace(const FloatRect& viewRect)
{
    notImplemented();
    return viewRect;
}

IntPoint PageClientImpl::screenToWindow(const IntPoint& point)
{
    notImplemented();
    return point;
}

IntRect PageClientImpl::windowToScreen(const IntRect& rect)
{
    return m_viewImpl->transformToScene().mapRect(rect);
}

void PageClientImpl::doneWithKeyEvent(const NativeWebKeyboardEvent&, bool)
{
    notImplemented();
}

#if ENABLE(GESTURE_EVENTS)
void PageClientImpl::doneWithGestureEvent(const WebGestureEvent& event, bool wasEventHandled)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (event.type() == WebEvent::GestureSingleTap && m_viewImpl->textSelection()->isTextSelectionMode()) {
        m_viewImpl->textSelection()->setIsTextSelectionMode(TextSelection::ModeNone);
        return;
    }
#endif

    notImplemented();
}
#endif

#if ENABLE(TOUCH_EVENTS)
void PageClientImpl::doneWithTouchEvent(const NativeWebTouchEvent& event, bool wasEventHandled)
{
#if PLATFORM(TIZEN)
    ewk_view_touch_event_handler_result_set(m_viewImpl->view(), event.type(), wasEventHandled);
#else
    notImplemented();
#endif // #if PLATFORM(TIZEN)
}
#endif

PassRefPtr<WebPopupMenuProxy> PageClientImpl::createPopupMenuProxy(WebPageProxy* page)
{
    return WebPopupMenuProxyEfl::create(m_viewImpl, page);
}

PassRefPtr<WebContextMenuProxy> PageClientImpl::createContextMenuProxy(WebPageProxy* page)
{
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    return WebContextMenuProxyTizen::create(m_viewImpl->view(), page, this);
#else
    notImplemented();
    return 0;
#endif
}

#if ENABLE(INPUT_TYPE_COLOR)
PassRefPtr<WebColorChooserProxy> PageClientImpl::createColorChooserProxy(WebPageProxy*, const WebCore::Color&)
{
    notImplemented();
    return 0;
}
#endif

void PageClientImpl::setFindIndicator(PassRefPtr<FindIndicator>, bool, bool)
{
    notImplemented();
}

#if !PLATFORM(TIZEN)
#if USE(ACCELERATED_COMPOSITING)
void PageClientImpl::enterAcceleratedCompositingMode(const LayerTreeContext&)
{
    m_viewImpl->enterAcceleratedCompositingMode();
}

void PageClientImpl::exitAcceleratedCompositingMode()
{
    m_viewImpl->exitAcceleratedCompositingMode();
}

void PageClientImpl::updateAcceleratedCompositingMode(const LayerTreeContext&)
{
    notImplemented();
}
#endif // USE(ACCELERATED_COMPOSITING)
#endif

void PageClientImpl::initializeAcceleratedCompositingMode()
{
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
void PageClientImpl::updateAcceleratedCompositingMode(const LayerTreeContext&)
{
    notImplemented();
}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC)

void PageClientImpl::didChangeScrollbarsForMainFrame() const
{
#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
    const_cast<PageClientImpl*>(this)->updateScrollbar();
#endif
}

#if PLATFORM(TIZEN)
void PageClientImpl::didFirstVisuallyNonEmptyLayoutForMainFrame()
{
    m_nonemptyLayoutRendered = true;
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    m_initialViewRect.setSize(viewSize());
#endif
}

void PageClientImpl::didChangeContentsSize(const WebCore::IntSize size)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI(" [%d, %d]", size.width(), size.height());
#endif
    m_viewImpl->informContentsSizeChange(size);

#if USE(TILED_BACKING_STORE)
    if (size.isEmpty())
        return;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    if (drawingArea()->layerTreeCoordinatorProxy())
        drawingArea()->layerTreeCoordinatorProxy()->setContentsSize(WebCore::FloatSize(size.width(), size.height()));
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    setLowScaleLayerFlag(false);
#endif

    // if minimum scale factor is changed, update minimumScale.
    if (m_viewportConstraints.userScalable && !m_viewResizeCount) {
        double minimumScale = availableMinimumScale();

        // Sometimes initializeVisibleContentRect can be called after content size change.
        // So, if initialScale is not set explicitly in content's meta viewport tag and is same to minimumScale, update initialScale too.
        if (!m_viewportConstraints.fixedInitialScale
            && fabs(m_viewportConstraints.initialScale - m_viewportConstraints.minimumScale) < numeric_limits<float>::epsilon())
            m_viewportConstraints.initialScale = minimumScale;
        m_viewportConstraints.minimumScale = minimumScale;
#if ENABLE(TIZEN_GESTURE)
        m_viewImpl->setDoubleTapEnabled(userScalable());
#endif
    }
#endif

    if (!canUpdateVisibleContentRect())
        return;

    // FIXME: Do we really need to adjust visible content rect at here?
    // if contents size is changed smaller and visible content rect is outside of content area,
    // adjust visible content rect
    bool needScrollAdjustment = (adjustVisibleContentRect(m_visibleContentRect, scaleFactor()) != m_visibleContentRect);
    bool needScaleAdjustment = (fabs(adjustScaleWithViewport(scaleFactor()) - scaleFactor()) > numeric_limits<float>::epsilon());
    if (needScrollAdjustment || needScaleAdjustment) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("visible content rect adjusted : [%d, %d], [%.2f]", m_visibleContentRect.x(), m_visibleContentRect.y(), scaleFactor());
#endif
        setVisibleContentRect(m_visibleContentRect, scaleFactor());
        displayViewport();
    }

    if (wasViewportFitsToContent() && !viewportConstraints().fixedInitialScale && (m_viewImpl->page()->estimatedProgress() < 1.0))
        fitViewportToContent(false);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    // TextSelection should be updated after the content size get changed and LeftRect and rightRect also changes to update handle and contextmenu
    if (m_viewImpl->textSelection()->isTextSelectionMode() && m_viewImpl->textSelection()->isUpdateRequired())
        updateTextInputState();
#endif
}

void PageClientImpl::pageScaleFactorDidChange()
{
    ewk_view_focused_node_adjust(m_viewImpl->view());
}
#endif // #if PLATFORM(TIZEN)

void PageClientImpl::didCommitLoadForMainFrame(bool)
{
#if PLATFORM(TIZEN)
    m_pageDidRendered = false;
    m_viewportFitsToContent = false;
    m_nonemptyLayoutRendered = false;
    return;
#endif
    notImplemented();
}

void PageClientImpl::didFinishLoadingDataForCustomRepresentation(const String&, const CoreIPC::DataReference&)
{
    notImplemented();
}

double PageClientImpl::customRepresentationZoomFactor()
{
    notImplemented();
    return 0;
}

void PageClientImpl::setCustomRepresentationZoomFactor(double)
{
    notImplemented();
}

void PageClientImpl::flashBackingStoreUpdates(const Vector<IntRect>&)
{
    notImplemented();
}

void PageClientImpl::findStringInCustomRepresentation(const String&, FindOptions, unsigned)
{
    notImplemented();
}

void PageClientImpl::countStringMatchesInCustomRepresentation(const String&, FindOptions, unsigned)
{
    notImplemented();
}

void PageClientImpl::updateTextInputState()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    bool didRequestUpdatingEditorState = m_viewImpl->textSelection()->didRequestUpdatingEditorState();
    m_viewImpl->textSelection()->updateTextInputState();
    if (m_viewImpl->textSelection()->isTextSelectionMode() && m_viewImpl->textSelection()->isTextSelectionHandleDowned())
        return;
    if (didRequestUpdatingEditorState)
        return;
#endif

    InputMethodContextEfl* inputMethodContext = m_viewImpl->inputMethodContext();
    if (inputMethodContext) {
#if ENABLE(TIZEN_SCREEN_READER)
        if (inputMethodContext->isShow() && ScreenReaderProxy::screenReader().focusRing()) {
            m_viewImpl->page()->clearScreenReaderFocus();
            ScreenReaderProxy::screenReader().focusRing()->hide(false);
        }
#endif
        inputMethodContext->updateTextInputState();
    }

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    if (m_viewImpl->page()->editorState().updateEditorRectOnly) {
        Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_viewImpl->view()));
        InputMethodContextEfl* inputMethodContext = m_viewImpl->inputMethodContext();
        if (smartData->api->formdata_candidate_is_showing(smartData) && inputMethodContext->isShow()) {
            IntRect inputFieldRect = m_viewImpl->focusedNodeRect(true);
            smartData->api->formdata_candidate_show(smartData, inputFieldRect.x(), inputFieldRect.y(), inputFieldRect.width(), inputFieldRect.height());
        }
    }
#endif
}

void PageClientImpl::handleDownloadRequest(DownloadProxy* download)
{
    Ewk_Context* context = m_viewImpl->ewkContext();
    context->downloadManager()->registerDownload(download, m_viewImpl);
}

#if USE(TILED_BACKING_STORE)
void PageClientImpl::pageDidRequestScroll(const IntPoint& point)
{
#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    if (m_viewImpl->inputFieldZoom->isInputFieldZoomScheduled() || m_viewImpl->inputFieldZoom->isWorking())
        return;
#endif

    // If requested point is same to current point, ignore this.
    IntPoint currentPoint(m_visibleContentRect.location().x() / scaleFactor(), m_visibleContentRect.location().y() / scaleFactor());
    if (point == currentPoint)
        return;

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    IntPoint newPoint = point;
    newPoint.scale(scaleFactor(), scaleFactor());

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("scroll position: [%d, %d]", newPoint.x(), newPoint.y());
#endif

#if ENABLE(TIZEN_WEBKIT2_BEFORE_PAGE_RENDERED_SCROLL_POSITION)
    if (!m_pageDidRendered)
        m_scrollPositionBeforePageRendered = newPoint;
    else
        setVisibleContentRect(IntRect(newPoint, m_visibleContentRect.size()), scaleFactor());
#else
    setVisibleContentRect(IntRect(newPoint, m_visibleContentRect.size()), scaleFactor());
#endif
#endif
    displayViewport();
}
#endif

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_HISTORICAL_RESTORE_VISIBLE_CONTENT_RECT)
void PageClientImpl::pageDidRequestRestoreVisibleContentRect(const IntPoint& point, float scale)
{
    if (!scale)
        return;

    m_restoredScrollPosition = point;
    m_restoredScrollPosition.scale(scale, scale);
    m_restoredScaleFactor = scale;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("scroll position: [%d, %d], scale factor: [%.2f]", m_restoredScrollPosition.x(), m_restoredScrollPosition.y(), m_restoredScaleFactor);
#endif

#if ENABLE(TIZEN_WEBKIT2_RESTORE_SCROLLPOINT_ON_FRAME_LOAD_FINISH)
    // If page is already rendered, restore
    if (canUpdateVisibleContentRect()) {
        restoreVisibleContentRect();
        return;
    }
#endif

    // Before contents size is fixed, just update visible content rect's position
    m_visibleContentRect.setLocation(m_restoredScrollPosition);
}

void PageClientImpl::prepareRestoredVisibleContectRect()
{
    m_restoredScaleFactor = scaleFactor();
    m_restoredScrollPosition = scrollPosition();
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("scroll position: [%d, %d], scale factor: [%.2f]", m_restoredScrollPosition.x(), m_restoredScrollPosition.y(), m_restoredScaleFactor);
#endif
}

#if ENABLE(TIZEN_WEBKIT2_RESTORE_SCROLLPOINT_ON_FRAME_LOAD_FINISH)
bool PageClientImpl::restoreVisibleContentRect()
{
    // Try to restore the scroll position, as soon as we have enough content size to restore the scrollposition.
    //Some times, even after we have full content is ready, estimatedProgress is 0.1.
    if (!m_restoredScaleFactor)
        return false;

    IntRect adjustedVisibleContentRect = adjustVisibleContentRect(IntRect(m_restoredScrollPosition, m_visibleContentRect.size()), m_restoredScaleFactor);

    // Restore only if it's possible
    if (adjustedVisibleContentRect.location() != m_restoredScrollPosition)
        return false;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("visibleContentRect: [%d, %d, %d, %d], scale factor: [%.2f]",
        m_restoredScrollPosition.x(), m_restoredScrollPosition.y(),
        m_visibleContentRect.width(), m_visibleContentRect.height(), m_restoredScaleFactor);
#endif
    //Many times notifiedNonemptyLayout is not triggered during navigating cached pages.
    //If we are able to restore the visibleRect, then we can set m_nonemptyLayoutRendered as true;
    if (canUpdateVisibleContentRect())
       m_nonemptyLayoutRendered = true;

    setVisibleContentRect(IntRect(m_restoredScrollPosition, m_visibleContentRect.size()), m_restoredScaleFactor);

    displayViewport();
    return true;
}
#endif

#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
void PageClientImpl::textChangeInTextField(const String& name, const String& value, bool isInputInForm, bool isForcePopupShow, bool isHandleEvent)
#else
void PageClientImpl::textChangeInTextField(const String& name, const String& value)
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
{
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    if (!isForcePopupShow && value == m_autoFillPopup->getCandidateValue())
#else
    if (value == m_autoFillPopup->getCandidateValue())
#endif
    {
        m_autoFillPopup->updateCandidateValue(emptyString());
        if(isShowingAutoFillPopup() && value.isEmpty())
            hideAutoFillPopup();
        return;
    }

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    ewk_view_text_change_in_textfield(m_viewImpl->view(), name, value, isInputInForm, isForcePopupShow, isHandleEvent);
#else
    m_autoFillPopup->updateCandidateValue(value);
    ewk_view_text_change_in_textfield(m_viewImpl->view(), name, value);
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
void PageClientImpl::updateFormNavigation(int length, int offset, bool prevState, bool nextState)
{
    m_viewImpl->formNavigation.count = length;
    m_viewImpl->formNavigation.position = offset;
    m_viewImpl->formNavigation.prevState = prevState;
    m_viewImpl->formNavigation.nextState = nextState;
}
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
DrawingAreaProxy* PageClientImpl::drawingArea() const
{
    return m_viewImpl->page()->drawingArea();
}
#endif

// Before rendering, scale factor and scroll position is different from m_viewImpl.
float PageClientImpl::scaleFactor()
{
    return m_pageDidRendered ? m_viewImpl->scaleFactor() : (m_restoredScaleFactor ? m_restoredScaleFactor : m_viewportConstraints.initialScale);
}

const IntPoint PageClientImpl::scrollPosition()
{
    return m_pageDidRendered ? m_viewImpl->scrollPosition() : (m_restoredScaleFactor ? m_restoredScrollPosition : IntPoint());
}

IntRect PageClientImpl::adjustVisibleContentRect(IntRect visibleContentRect, float targetScale)
{
    IntSize contentsSize = m_viewImpl->page()->contentsSize();
    contentsSize.scale(targetScale);
    if (contentsSize.width() < visibleContentRect.width())
        visibleContentRect.setX(0);
    else
        visibleContentRect.setX(clampTo(visibleContentRect.x(), 0, contentsSize.width() - visibleContentRect.width()));

    if (contentsSize.height() < visibleContentRect.height())
        visibleContentRect.setY(0);
    else
        visibleContentRect.setY(clampTo(visibleContentRect.y(), 0, contentsSize.height() - visibleContentRect.height()));
    return visibleContentRect;
}

void PageClientImpl::setVisibleContentRect(const IntRect& newRect, float newScale, const FloatPoint& trajectory)
{
#if PLATFORM(TIZEN)
    IntPoint previousScrollPosition(m_viewImpl->scrollPosition());
    float previousScale = m_viewImpl->scaleFactor();
#endif

    m_scaleFactor = adjustScaleWithViewport(newScale);
    m_viewportFitsToContent = fabs(m_scaleFactor - m_viewportConstraints.minimumScale) < numeric_limits<float>::epsilon();
    m_visibleContentRect.setLocation(newRect.location());
    m_visibleContentRect = adjustVisibleContentRect(m_visibleContentRect, m_scaleFactor);

    // update both drawing scale factor and scroll position after page is rendered
    if (m_pageDidRendered) {
        if (!m_hasSuspendedContent) {
            // FIXME: We have to update EwkViewImpl's scale and position here because we use them to draw contents.
            // PageViewport's values are updated when resuming content in the webkit opensource,
            // but we have to update viewImpl's values here to sync with PageClient's values.
            // However, We should not update them when hasSuspendedContent is true in order to maintain last values.
            // The values will be updated when resuming content.
            // Below codes should be refactored when PageViewportController codes are merged into Tizen.
            m_viewImpl->setScaleFactor(m_scaleFactor);
            m_viewImpl->setScrollPosition(m_visibleContentRect.location());
        }
    }

    // enclosingIntRect produces inconsistent width and height when scale factor is not 1.
    // So we removed enclosingIntRect and replaced with floorf and ceilf.
    IntRect mapToContentsVisibleContentRect = IntRect(floorf(m_visibleContentRect.x() / m_scaleFactor),
                                                      floorf(m_visibleContentRect.y() / m_scaleFactor),
#if PLATFORM(TIZEN)
                                                      roundf(m_visibleContentRect.width() / m_scaleFactor),
                                                      roundf(m_visibleContentRect.height() / m_scaleFactor));
#else
                                                      ceilf(m_visibleContentRect.width() / m_scaleFactor),
                                                      ceilf(m_visibleContentRect.height() / m_scaleFactor));
#endif
    if (!drawingArea())
        return;
    drawingArea()->setVisibleContentsRect(mapToContentsVisibleContentRect, m_scaleFactor, trajectory, FloatPoint(m_viewImpl->scrollPosition()));
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    // FIXME: We need to calculate exact visibleRect size here instead of mapToContentsVisibleContentRect.
    drawingArea()->setVisibleContentsRectForScrollingContentsLayers(mapToContentsVisibleContentRect);
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    if (m_viewImpl->focusRing())
        m_viewImpl->focusRing()->didChangeVisibleContentRect(previousScrollPosition, previousScale);

#if ENABLE(TIZEN_SCREEN_READER)
    if (ScreenReaderProxy::screenReader().focusRing())
        ScreenReaderProxy::screenReader().focusRing()->didChangeVisibleContentRect(previousScrollPosition, previousScale);
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (!isClipboardWindowOpened() && m_viewImpl->textSelection()->isTextSelectionMode()) {
        if (m_viewImpl->textSelection()->relayoutRequired()) {
            m_viewImpl->textSelection()->setRelayoutRequired(false);
            m_viewImpl->textSelection()->requestUpdatingEditorState();
        } else if (!m_viewImpl->textSelection()->didRequestUpdatingEditorState()) {
            bool isChangedVisibleContentRect = false;
            //This will update the context menu and handles to proper position.
            if (m_viewImpl->scrollPosition() != previousScrollPosition
                || m_viewImpl->scaleFactor() != previousScale)
                isChangedVisibleContentRect = true;

            if (!isChangedVisibleContentRect && m_viewImpl->textSelection()->isUpdateRequired())
                isChangedVisibleContentRect = true;

            if (isChangedVisibleContentRect) {
                m_viewImpl->textSelection()->updateVisible(false);
                m_viewImpl->textSelection()->requestToShow();
            }
        }
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_viewImpl->view()));
    InputMethodContextEfl* inputMethodContext = m_viewImpl->inputMethodContext();
    if (smartData->api->formdata_candidate_is_showing(smartData) && inputMethodContext->isShow()) {
        IntRect inputFieldRect = m_viewImpl->focusedNodeRect(true);
        smartData->api->formdata_candidate_show(smartData, inputFieldRect.x(), inputFieldRect.y(), inputFieldRect.width(), inputFieldRect.height());
    }
#endif
}

void PageClientImpl::scheduleDisplayViewport(float delay)
{
    if (!m_displayViewportTimer.isActive())
        m_displayViewportTimer.startOneShot(delay);
}

void PageClientImpl::displayViewportTimerFired(WebCore::Timer<PageClientImpl>*)
{
    displayViewport();
}

void PageClientImpl::displayViewport()
{
    if (m_displayViewportTimer.isActive())
        m_displayViewportTimer.stop();

    setViewNeedsDisplay(IntRect(IntPoint(), viewSize()));

#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
    updateScrollbar();
#endif
}

void PageClientImpl::drawContents()
{
}

void PageClientImpl::drawContents(BackingStore::PlatformGraphicsContext context)
{
    cairo_save(context);
    const cairo_matrix_t matrix = cairo_matrix_t(m_viewImpl->transformToView());
    cairo_transform(context, &matrix);

    if (drawingArea()) {
        if (drawingArea()->layerTreeCoordinatorProxy()) {
            WebLayerTreeRenderer* renderer = drawingArea()->layerTreeCoordinatorProxy()->layerTreeRenderer();
            renderer->setDrawsBackground(m_viewImpl->page()->drawsBackground());
            renderer->paintToGraphicsContext(context);
        }
    }

    cairo_restore(context);
}

void PageClientImpl::scaleImage(double scaleFactor, IntPoint scrollPosition)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    double intialScaleFactor = scaleFactor;
#endif

#if PLATFORM(TIZEN) && ENABLE(FULLSCREEN_API)
    // We don't want to process scaling in the FullScreen mode.
    if (m_viewImpl->page()->fullScreenManager()->isFullScreen())
        return;
#endif

    // Adjust scaleFactor.
#if ENABLE(TIZEN_WEBKIT2_TEXT_ZOOM)
    if (!m_viewImpl->page()->pageGroup()->preferences()->textZoomEnabled())
        scaleFactor = adjustScaleWithViewport(scaleFactor);
#else
    scaleFactor = adjustScaleWithViewport(scaleFactor);
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (!m_viewImpl->textSelection()->isTextSelectionMode() || intialScaleFactor != scaleFactor)
        m_viewImpl->textSelection()->hideHandlers();
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    m_viewImpl->setScaleFactor(scaleFactor);
    IntRect rectForDrawContents(scrollPosition, viewSize());
    rectForDrawContents = adjustVisibleContentRect(rectForDrawContents, scaleFactor);
    m_viewImpl->setScrollPosition(rectForDrawContents.location());
    // Update only WebLayerTreeRenderer's visibleContentsRect to adjust position of fixed layers.
    // Do not update LayerTreeCoordinatorProxy's visibleContentsRect here because it causes re-layout.
    if (drawingArea()) {
        if (drawingArea()->layerTreeCoordinatorProxy()) {
            IntRect mapToContentsVisibleContentRect = IntRect(floorf(rectForDrawContents.x() / scaleFactor),
                                                              floorf(rectForDrawContents.y() / scaleFactor),
                                                              ceilf(rectForDrawContents.width() / scaleFactor),
                                                              ceilf(rectForDrawContents.height() / scaleFactor));
            drawingArea()->layerTreeCoordinatorProxy()->layerTreeRenderer()->setVisibleContentsRect(mapToContentsVisibleContentRect, scaleFactor, FloatPoint(m_viewImpl->scrollPosition()));
        }
    }
    displayViewport();
#else
    scaleContents(scaleFactor, scrollPosition);
#endif
}

void PageClientImpl::scaleContents(double scaleFactor, const IntPoint& origin)
{
#if PLATFORM(TIZEN) && ENABLE(FULLSCREEN_API)
    // We don't want to process scaling in the FullScreen mode.
    if (m_viewImpl->page()->fullScreenManager()->isFullScreen())
        return;
#endif
    scaleFactor = adjustScaleWithViewport(scaleFactor);

    setVisibleContentRect(IntRect(origin, m_visibleContentRect.size()), scaleFactor);
    displayViewport();
}

#if ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
void PageClientImpl::changeToMediumScaleImage(double scaleFactor, const IntPoint& origin)
{
    IntRect rectForDrawContents(origin, viewSize());
    rectForDrawContents = adjustVisibleContentRect(rectForDrawContents, scaleFactor);
    IntRect mapToContentsVisibleContentRect = IntRect(floorf(rectForDrawContents.x() / scaleFactor),
                                                      floorf(rectForDrawContents.y() / scaleFactor),
                                                      ceilf(rectForDrawContents.width() / scaleFactor),
                                                      ceilf(rectForDrawContents.height() / scaleFactor));
    if (!drawingArea())
        return;
    drawingArea()->layerTreeCoordinatorProxy()->setVisibleContentsRect(mapToContentsVisibleContentRect, scaleFactor, FloatPoint(), FloatPoint(origin));
}
#endif

// FIXME: The concept of suspending content comes from webkit opensource's PageViewportController,
// so below code should be replaced when PageViewportController codes are merged.
// Please do not use below codes. They are only for scaling contents.
void PageClientImpl::suspendContent()
{
    if (m_hasSuspendedContent)
        return;

    m_hasSuspendedContent = true;
    ewk_view_suspend_painting(m_viewImpl->view());
}

void PageClientImpl::resumeContent()
{
    if (!m_hasSuspendedContent)
        return;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("scroll position: [%d, %d], scale factor: [%.2f]", m_viewImpl->scrollPosition().x(), m_viewImpl->scrollPosition().y(), m_viewImpl->scaleFactor());
#endif

    // FIXME: Update visibleContentRect with m_viewImpl after resuming content.
    // The concept is that the values of EwkViewImpl and PageClient can be different
    // during suspending content and they become same when content is resumed.
    // Below codes should be refactored when PageViewportController codes are merged into Tizen.
    setVisibleContentRect(IntRect(m_viewImpl->scrollPosition(), m_visibleContentRect.size()), m_viewImpl->scaleFactor());
    displayViewport();
    m_hasSuspendedContent = false;
    ewk_view_resume_painting(m_viewImpl->view());
}

FloatPoint PageClientImpl::boundContentsPositionAtScale(const FloatPoint& framePosition, float scale)
{
    // We need to floor the viewport here as to allow aligning the content in device units. If not,
    // it might not be possible to scroll the last pixel and that affects fixed position elements.
    FloatRect bounds;
    const IntSize& contentsSize = m_viewImpl->page()->contentsSize();
    bounds.setWidth(std::max(0.f, contentsSize.width() - floorf(viewSize().width() / scale)));
    bounds.setHeight(std::max(0.f, contentsSize.height() - floorf(viewSize().height() / scale)));

    FloatPoint position;
    // Unfortunately it doesn't seem to be enough, so just always allow one pixel more.
    position.setX(clampTo(framePosition.x(), bounds.x(), bounds.width() + 1));
    position.setY(clampTo(framePosition.y(), bounds.y(), bounds.height() + 1));

    return position;
}

#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
void PageClientImpl::updateScrollbar()
{
    IntSize scaledContentsSize = m_viewImpl->page()->contentsSize();
    scaledContentsSize.scale(scaleFactor());

    if (viewSize().width() < scaledContentsSize.width()) {
        if (!m_horizontalScrollbar)
            m_horizontalScrollbar = MainFrameScrollbarTizen::createNativeScrollbar(m_viewImpl->view(), HorizontalScrollbar);
        m_horizontalScrollbar->setEnabled(m_viewImpl->page()->hasHorizontalScrollbar());
        m_horizontalScrollbar->setFrameRect(IntRect(0, viewSize().height(), viewSize().width(), 0));
        m_horizontalScrollbar->setProportion(viewSize().width(), scaledContentsSize.width());
        m_horizontalScrollbar->setPosition(m_viewImpl->scrollPosition().x());
    } else if (m_horizontalScrollbar)
        m_horizontalScrollbar->setEnabled(false);

    if (viewSize().height() < scaledContentsSize.height()) {
        if (!m_verticalScrollbar)
            m_verticalScrollbar = MainFrameScrollbarTizen::createNativeScrollbar(m_viewImpl->view(), VerticalScrollbar);
        m_verticalScrollbar->setEnabled(m_viewImpl->page()->hasVerticalScrollbar());
        m_verticalScrollbar->setFrameRect(IntRect(viewSize().width(), 0, 0, viewSize().height()));
        m_verticalScrollbar->setProportion(viewSize().height(), scaledContentsSize.height());
        m_verticalScrollbar->setPosition(m_viewImpl->scrollPosition().y());
    } else if (m_verticalScrollbar)
        m_verticalScrollbar->setEnabled(false);
}

void PageClientImpl::frameRectChanged()
{
    if (m_horizontalScrollbar)
        m_horizontalScrollbar->setFrameRect(IntRect(0, viewSize().height(), viewSize().width(), 0));
    if (m_verticalScrollbar)
        m_verticalScrollbar->setFrameRect(IntRect(viewSize().width(), 0, 0, viewSize().height()));
}

void PageClientImpl::updateVisibility()
{
    if (m_horizontalScrollbar)
        m_horizontalScrollbar->updateVisibility();
    if (m_verticalScrollbar)
        m_verticalScrollbar->updateVisibility();
}
#endif
#endif // ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
bool PageClientImpl::textSelectionDown(const WebCore::IntPoint& point)
{
    if (!evas_object_focus_get(m_viewImpl->view())) {
        InputMethodContextEfl* inputMethodContext = m_viewImpl->inputMethodContext();
        if (inputMethodContext)
            inputMethodContext->hideIMFContext();
    }

    return m_viewImpl->textSelection()->textSelectionDown(point);
}

void PageClientImpl::textSelectionMove(const WebCore::IntPoint& point)
{
    m_viewImpl->textSelection()->textSelectionMove(point);
}

void PageClientImpl::textSelectionUp(const WebCore::IntPoint& point, bool isStartedTextSelectionFromOutside)
{
    m_viewImpl->textSelection()->textSelectionUp(point, isStartedTextSelectionFromOutside);
}
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
void PageClientImpl::saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName)
{
    m_offlinePageSave->saveSerializedHTMLDataForMainPage(serializedData, fileName);
}

void PageClientImpl::saveSubresourcesData(Vector<WebSubresourceTizen>& subresourceData)
{
    m_offlinePageSave->saveSubresourcesData(subresourceData);
}

void PageClientImpl::startOfflinePageSave(String& path, String& url, String& title)
{
    m_offlinePageSave->startOfflinePageSave(path, url, title);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
void PageClientImpl::pasteContextMenuSelected()
{
    m_clipboardHelper->pasteClipboardLastItem(m_viewImpl->page()->editorState().isContentRichlyEditable);
}
#endif

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
void PageClientImpl::setClipboardData(const String& data, const String& type)
{
#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
    m_clipboardHelper->setData(data, type);
#endif
}

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
void PageClientImpl::clipboardContextMenuSelected()
{
    m_clipboardHelper->openClipboardWindow(m_viewImpl->page()->editorState().isContentRichlyEditable);
}

bool PageClientImpl::isClipboardWindowOpened()
{
    return m_clipboardHelper->isClipboardWindowOpened();
}

void PageClientImpl::closeClipboardWindow()
{
    m_clipboardHelper->closeClipboardWindow();
}

bool PageClientImpl::isPastedItemOnlyImage()
{
    return m_clipboardHelper->isPastedItemOnlyImage(m_viewImpl->page()->editorState().isContentRichlyEditable);
}
#endif

void PageClientImpl::clearClipboardData()
{
#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
    m_clipboardHelper->clear();
#endif
}
#endif

#if ENABLE(TIZEN_WEBKIT2_VIEW_VISIBILITY)
void PageClientImpl::setIsVisible(bool isVisible)
{
    m_isVisible = isVisible;

    WebPageProxy* pageProxy = m_viewImpl->page();
    if (pageProxy)
        pageProxy->viewStateDidChange(WebPageProxy::ViewIsVisible);

#if ENABLE(TIZEN_CACHE_MEMORY_OPTIMIZATION)
    if (!m_isVisible) {
        ewk_view_context_get(m_viewImpl->view())->clearAllDeadResources();
        ewk_view_context_get(m_viewImpl->view())->clearAllLiveDecodedResources();
    }
#endif
}
#endif

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
void PageClientImpl::setIsForeground(bool isForeground)
{
    m_isForeground = isForeground;
    ewk_view_context_get(m_viewImpl->view())->setIsForeground(isForeground);

    if (m_isForeground) {
        ewk_view_context_get(m_viewImpl->view())->restoreAllEncodedResources();
        return;
    }

    if (!m_isForeground) {
        ewk_view_context_get(m_viewImpl->view())->markVisibleContents(m_visibleContentRect);
        return;
    }
}
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
void PageClientImpl::updateDragPosition()
{
    m_drag->updateDragPosition();
}
void PageClientImpl::setDragPoint(const WebCore::IntPoint& point)
{
    m_drag->setDragPoint(point);
}
bool PageClientImpl::isDragMode()
{
    return m_drag->isDragMode();
}
void PageClientImpl::setDragMode(bool isDragMode)
{
    m_drag->setDragMode(isDragMode);
}
void PageClientImpl::startDrag(const DragData& dragData, PassRefPtr<ShareableBitmap> dragImage)
{
    DragData* dragInfo = new DragData(dragData.platformData(), m_drag->getDragPoint(),
        m_drag->getDragPoint(), dragData.draggingSourceOperationMask(), dragData.flags());

    String dragStorageName("Drag");
    m_viewImpl->page()->dragEntered(dragInfo, dragStorageName);
    setDragMode(true);
    m_drag->setDragData(dragInfo, dragImage);
    m_drag->Show();
}
#endif

#if ENABLE(TIZEN_POPUP_BLOCKED_NOTIFICATION)
void PageClientImpl::popupBlocked(const String& urlString)
{
    if (urlString.isEmpty()) {
        TIZEN_LOGI("popup,blocked urlString is empty");
        evas_object_smart_callback_call(m_viewImpl->view(), "popup,blocked", 0);
    }
    else {
        TIZEN_SECURE_LOGI("popup,blocked urlString : %s", urlString.utf8().data());
        evas_object_smart_callback_call(m_viewImpl->view(), "popup,blocked", const_cast<char *>(urlString.utf8().data()));
    }
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
bool PageClientImpl::isShowingAutoFillPopup()
{
    return m_autoFillPopup->isShowing();
}

void PageClientImpl::updateAutoFillPopup(const Vector<AutoFillPopupItem>& data)
{
    m_autoFillPopup->updateFormData(data);
}

void PageClientImpl::hideAutoFillPopup()
{
    m_autoFillPopup->hide();
}

void PageClientImpl::showAutoFillPopup(const WebCore::IntRect& rect)
{
    m_autoFillPopup->show(rect);
}
#endif

#if ENABLE(TIZEN_REGISTER_PROTOCOL_HANDLER)
void PageClientImpl::registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title)
{
    ewkViewRegisterProtocolHandlers(m_viewImpl->view(), scheme.utf8().data(), baseURL.utf8().data(), url.utf8().data(), title.utf8().data());
}
#endif

#if ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)
unsigned int PageClientImpl::isProtocolHandlerRegistered(const String& scheme, const String& baseURL, const String& url)
{
    return ewkViewIsProtocolHandlerRegistered(m_viewImpl->view(), scheme.utf8().data(), baseURL.utf8().data(), url.utf8().data());
}

void PageClientImpl::unregisterProtocolHandler(const String& scheme, const String& baseURL, const String& url)
{
    ewkViewUnregisterProtocolHandlers(m_viewImpl->view(), scheme.utf8().data(), baseURL.utf8().data(), url.utf8().data());
}
#endif

#if ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)
void PageClientImpl::registerContentHandler(const String& mimeType, const String& baseURL, const String& url, const String& title)
{
    ewkViewRegisterContentHandlers(m_viewImpl->view(), mimeType.utf8().data(), baseURL.utf8().data(), url.utf8().data(), title.utf8().data());
}

unsigned int PageClientImpl::isContentHandlerRegistered(const String& mimeType, const String& baseURL, const String& url)
{
    return ewkViewIsContentHandlerRegistered(m_viewImpl->view(), mimeType.utf8().data(), baseURL.utf8().data(), url.utf8().data());
}

void PageClientImpl::unregisterContentHandler(const String& mimeType, const String& baseURL, const String& url)
{
    ewkViewUnregisterContentHandlers(m_viewImpl->view(), mimeType.utf8().data(), baseURL.utf8().data(), url.utf8().data());
}
#endif

#if ENABLE(TIZEN_SEARCH_PROVIDER)
void PageClientImpl::addSearchProvider(const String& baseURL, const String& engineURL)
{
    notImplemented();
}

unsigned long PageClientImpl::isSearchProviderInstalled(const String& baseURL, const String& engineURL)
{
    notImplemented();
    return 0;
}
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
bool PageClientImpl::getStandaloneStatus()
{
    return ewkViewGetStandaloneStatus(m_viewImpl->view());
}
#endif

#if ENABLE(SCREEN_ORIENTATION_SUPPORT) && ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
bool PageClientImpl::lockOrientation(int willLockOrientation)
{
    return ewk_view_orientation_lock(m_viewImpl->view(), willLockOrientation);
}

void PageClientImpl::unlockOrientation()
{
    ewk_view_orientation_unlock(m_viewImpl->view());
}
#endif

#if PLATFORM(TIZEN)
bool PageClientImpl::canUpdateVisibleContentRect()
{
    // Visible content rect can be updated when below conditions are satisfied
    // 1. page render is done(m_pageDidRendered && (m_viewImpl->page()->estimatedProgress())
    // 2. function call count of both updateViewportSize() and didChangeViewportProperties() should be matched
    return m_pageDidRendered && (m_viewImpl->page()->estimatedProgress() > 0.1) && !m_viewResizeCount;
}
#endif

void PageClientImpl::didRenderFrame()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    if (m_shouldShowBackupTexture && m_isVisible) {
        m_shouldMakeBackupTexture = false;
        m_shouldShowBackupTexture = false;
    }
#endif
    if (!m_pageDidRendered) {
        m_pageDidRendered = true;
        initializeVisibleContentRect();
    }
}

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
void PageClientImpl::setOverflowResult(bool pressed, WebLayerID webLayerID)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Overflow] pressed[%d] webLayerID[%d]", pressed, webLayerID);
#endif
    setIsScrollableLayerFocused(false);
    setIsScrollableNodeFocused(false);

    if (pressed) {
        m_isOverflowMovingEnabled = true;
        evas_object_smart_callback_call(m_viewImpl->view(), "overflow,scroll,on", 0);
        if (webLayerID) {
            setIsScrollableLayerFocused(true);
            m_viewImpl->page()->drawingArea()->layerTreeCoordinatorProxy()->layerTreeRenderer()->setFocusedLayerID(webLayerID);
        } else {
            setIsScrollableNodeFocused(true);
        }
    }
}

void PageClientImpl::findScrollableNode(const IntPoint& point)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    double startTime = ecore_time_get();
#endif
    WebPageProxy* pageProxy = m_viewImpl->page();
    IntPoint pointForPress(m_viewImpl->transformFromScene().mapPoint(point));
    if (pageProxy && pageProxy->isLoadingFinished() && pageProxy->askOverflow(pointForPress)) {
        WebLayerID webLayerID = 0;
        bool checkOverflowLayer = false;
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
        DrawingAreaProxy* drawingArea = pageProxy->drawingArea();
        checkOverflowLayer = drawingArea && drawingArea->layerTreeCoordinatorProxy() && drawingArea->layerTreeCoordinatorProxy()->hasOverflowLayer();
#endif
        setOverflowResult(pageProxy->setPressedNodeAtPoint(pointForPress, checkOverflowLayer, webLayerID), webLayerID);
    }
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Overflow] elapse[%lf sec]", ecore_time_get() - startTime);
#endif
}
#endif

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
void PageClientImpl::setIsContextMenuVisible(bool isVisible)
{
    if (m_isContextMenuVisible == isVisible)
        return;

    m_isContextMenuVisible = isVisible;

#if ENABLE(TIZEN_FOCUS_UI)
    if (!isVisible)
        m_viewImpl->clearKeyDownEventDeferred();
#endif
}
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void PageClientImpl::mediaControlsRequestRotate(const String& status)
{
    ewkViewMediaControlsRequestRotate(m_viewImpl->view(), status);
}
#endif

void PageClientImpl::didFindZoomableArea(const IntPoint& target, const IntRect& area)
{
    ewk_view_zoomable_area_set(m_viewImpl->view(), target, area);
}

#if ENABLE(TIZEN_ICON_DATABASE)
void PageClientImpl::didReceiveIcon()
{
    ewkViewIconReceived(m_viewImpl->view());
}
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_WEB_SEARCH)
void PageClientImpl::searchWebContextMenuSelected(String& linkURL)
{
    const char* selected_text = ewk_view_text_selection_text_get(m_viewImpl->view());
    char* keyword = 0;

    if (strlen(selected_text) == 0)
        keyword = g_strdup(linkURL.utf8().data());
    else
        keyword = g_strdup(selected_text);

    app_control_h appControl = 0;
    app_control_create(&appControl);
    app_control_set_window(appControl, elm_win_xwindow_get(elm_object_parent_widget_get(m_viewImpl->view())));
    app_control_set_operation(appControl, APP_CONTROL_OPERATION_SEARCH);
    app_control_add_extra_data(appControl, "http://tizen.org/appcontrol/data/keyword", keyword);
    app_control_send_launch_request(appControl, NULL, NULL);
    app_control_destroy(appControl);

    free(keyword);
}
#endif

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
void PageClientImpl::addMessageToConsole(unsigned level, const String& message, unsigned lineNumber, const String& source)
{
    ewkViewAddMessageToConsole(m_viewImpl->view(), level, message, lineNumber, source);
}
#endif

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
void PageClientImpl::resizeEventDone()
{
    if (m_preparingRotation) {
        m_preparingRotation = false;
        m_waitFrameOfNewViewortSize = true;
        ewk_view_resume_painting(m_viewImpl->view());
    }
}

void PageClientImpl::resetPreparingRotationStatus()
{
    if (m_waitFrameOfNewViewortSize) {
        m_waitFrameOfNewViewortSize = false;
        ewkViewScreenShotForRotationDelete(m_viewImpl->view());
    }
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (m_viewImpl->page()->fullScreenManager()->isUsingHwVideoOverlay()) {
        // The backing has stale snapshot. Clear it before suspending paint.
        m_viewImpl->page()->drawingArea()->layerTreeCoordinatorProxy()->clearBackingStores();
    }
#endif
}
#endif

#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
void PageClientImpl::didCreatePagesToPDF()
{
    evas_object_smart_callback_call(m_viewImpl->view(), "pdf,created", 0);
}
#endif

#endif // #if PLATFORM(TIZEN)


#if ENABLE(TIZEN_WEBKIT2_TILED_AC) && ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
PageClientEvasGL::PageClientEvasGL(EwkViewImpl* viewImpl, int pixmap)
#else
PageClientEvasGL::PageClientEvasGL(EwkViewImpl* viewImpl)
#endif
    : PageClientImpl(viewImpl)
    , m_evasGL(0)
    , m_evasGlApi(0)
    , m_context(0)
    , m_surface(0)
    , m_config(0)
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    , m_angle(0)
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    , m_backupAngle(0)
#endif
    , m_isAcceleratedCompositingModeInitialized(false)
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    , m_isProviderAC(false)
    , m_pixmap(0)
#endif
{
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    if (pixmap){
        m_pixmap = pixmap;
        m_isProviderAC = true;
    }
#endif
    initializeAcceleratedCompositingMode();
}

PageClientEvasGL::~PageClientEvasGL()
{
#if !PLATFORM(TIZEN)
    // FIXME: It should be compared with open source.
    if (m_viewImpl && m_viewImpl->page())
        m_viewImpl->page()->close();
#endif
}

void PageClientEvasGL::updateViewportSize(const WebCore::IntSize& viewportSize)
{
    PageClientImpl::updateViewportSize(viewportSize);
}

void PageClientEvasGL::updateVisibleContentRectSize(const WebCore::IntSize& size)
{
    if (m_surface) {
        evas_gl_surface_destroy(m_evasGL, m_surface);
        m_surface = 0;
    }

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    if (m_isProviderAC)
        setProviderSurface();
    else
#endif
    setTargetSurface();

    PageClientImpl::updateVisibleContentRectSize(size);
}

void PageClientEvasGL::setViewNeedsDisplay(const WebCore::IntRect& rect)
{
#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    if (m_waitFrameOfNewViewortSize) {
        drawContents();
        ewk_view_mark_for_sync(m_viewImpl->view());
        m_preparingRotation = false;
        m_waitFrameOfNewViewortSize = false;

#if ENABLE(TIZEN_FULLSCREEN_API)
        if (m_viewImpl->page()->fullScreenManager()->isFullScreen())
            m_viewImpl->page()->fullScreenManager()->didRotateScreen();
#endif
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
        if (!m_viewImpl->page()->fullScreenManager()->isUsingHwVideoOverlay())
#endif
        ewkViewScreenShotForRotationDelete(m_viewImpl->view());
        evas_object_smart_callback_call(m_viewImpl->view(), "rotate,prepared", 0);
    } else
#endif
    {
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
        if (m_isProviderAC) {
            drawContents();
            ewk_view_data_update_add(m_viewImpl->view(), rect.x(), rect.y(), rect.width(), rect.height());
        }
        else
#endif
            ewk_view_mark_for_sync(m_viewImpl->view());
    }

#if ENABLE(TIZEN_LINK_MAGNIFIER)
    LinkMagnifierProxy::linkMagnifier().requestUpdate(rect);
#endif
}

void PageClientEvasGL::displayViewport()
{
    if (m_displayViewportTimer.isActive())
        m_displayViewportTimer.stop();

    setViewNeedsDisplay(IntRect(IntPoint(), viewSize()));

#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
    updateScrollbar();
#endif
}

void PageClientEvasGL::drawContents()
{
    if (!drawingArea() || !(drawingArea()->layerTreeCoordinatorProxy()))
        return;
    WebLayerTreeRenderer* renderer = drawingArea()->layerTreeCoordinatorProxy()->layerTreeRenderer();
    if (!renderer)
        return;
    if (!makeContextCurrent())
        return;

    WebCore::TransformationMatrix matrix;
    IntRect clipRect;
    IntSize ewkViewSize = viewSize();

    bool needToRotate = false;

    // FIXME : evas_gl_proc_address_get() didn't work. So I annotated the code until fix the evas_gl_proc_address_get().
    // typedef int (*evas_gl_ext_surface_is_texture_func) (Evas_GL *evas_gl, Evas_GL_Surface *surf);
    // evas_gl_ext_surface_is_texture_func evas_gl_ext_surface_is_texture = NULL;
    // evas_gl_ext_surface_is_texture = reinterpret_cast<evas_gl_ext_surface_is_texture_func>(evas_gl_proc_address_get(m_evasGL, "evas_gl_ext_surface_is_texture"));
    // if (!evas_gl_ext_surface_is_texture) {
    //     TIZEN_LOGE("failed to get evas_gl_ext_surface_is_texture pointer");
    //     return;
    // }

    int is_texture = 0; //evas_gl_ext_surface_is_texture(m_evasGL, m_surface);
    needToRotate |= is_texture;

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    needToRotate |= m_isProviderAC;
    renderer->setIsProviderAC(m_isProviderAC);
#endif

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    renderer->setDirectRendering(isDirectRendering() && !is_texture);

    if (isDirectRendering() && !is_texture) {
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
        if (m_shouldMakeBackupTexture) {
            m_backupAngle = m_angle; // angle for our backup texture.
            m_angle = 0; // calculate a matrix with 0 angle during FBO(backup texture) rendering.
        } else {
            m_angle = evas_gl_rotation_get(m_evasGL);
            if (m_shouldShowBackupTexture)
                m_shouldShowBackupTexture = (m_backupAngle == m_angle); // if m_angle is changed, give up ShowBackupTexture.
        }
#endif
        renderer->setAngle(m_angle);
        if (!is_texture) {
            if (!needToRotate)
                matrix.rotate3d(0.0, 0.0, 1.0, 360 - m_angle);

            if (m_angle == 90 || m_angle == 270) {
                glViewport(0, 0, ewkViewSize.height(), ewkViewSize.width());
                if (m_angle == 90)
                    matrix.translate(-ewkViewSize.width(), 0);
                else if (m_angle == 270)
                    matrix.translate(0, -ewkViewSize.height());
                clipRect = IntRect(IntPoint(), ewkViewSize.transposedSize());
            } else {
                glViewport(0, 0, ewkViewSize.width(), ewkViewSize.height());
                if (m_angle == 180)
                    matrix.translate(-ewkViewSize.width(), -ewkViewSize.height());
                clipRect = IntRect(IntPoint(), ewkViewSize);
            }
        } else {
                glViewport(0, 0, ewkViewSize.width(), ewkViewSize.height());
                clipRect = IntRect(IntPoint(), ewkViewSize);
        }
    } else
#endif
    {
        glViewport(0, 0, ewkViewSize.width(), ewkViewSize.height());
        clipRect = IntRect(IntPoint(), ewkViewSize);
    }

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (needToRotate) {
         matrix.translate(0, ewkViewSize.height());
         matrix.flipY();
    }
#endif

    matrix *= m_viewImpl->transformToView().toTransformationMatrix();
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    renderer->setPaintOnlyLowScaleLayer(m_shouldPaintOnlyLowScaleLayer);
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    if (m_shouldMakeBackupTexture) {
        matrix.makeIdentity();
        matrix.setMatrix(scaleFactor(), 0, 0, scaleFactor(), -m_visibleContentRect.x(), -m_visibleContentRect.y());
        renderer->paintToBackupTexture(matrix, 1.0f, clipRect, m_visibleContentRect.size());
        m_shouldMakeBackupTexture = false;
    } else if (m_shouldShowBackupTexture) {
        matrix.makeIdentity();
        matrix.rotate3d(0.0, 0.0, 1.0, 360 - m_angle);
        renderer->showBackupTexture(matrix, 1.0f, clipRect);
    } else
#endif
    renderer->paintToCurrentGLContext(matrix, 1.0f, clipRect);

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    if (m_isProviderAC)
        glFinish();
#endif
}

void PageClientEvasGL::didRenderFrame()
{
    if (notifiedNonemptyLayout() && canUpdateVisibleContentRect())
        ewkViewFrameRendered(m_viewImpl->view());

    PageClientImpl::didRenderFrame();
}

bool PageClientEvasGL::makeContextCurrent()
{
    if (m_surface && m_context)
        return evas_gl_make_current(m_evasGL, m_surface, m_context);

    return false;
}

void PageClientEvasGL::setUpdateViewportWithAnimatorEnabled(bool enabled)
{
    if (drawingArea()->layerTreeCoordinatorProxy())
        drawingArea()->layerTreeCoordinatorProxy()->setUpdateViewportWithAnimatorEnabled(enabled);
}

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
void PageClientEvasGL::setLowScaleLayerFlag(bool enabled)
{
    if (drawingArea()->layerTreeCoordinatorProxy())
        drawingArea()->layerTreeCoordinatorProxy()->setLowScaleLayerFlag(enabled);
}

void PageClientEvasGL::setUpdateLowScaleLayerOnly(bool enabled)
{
    if (m_updateLowScaleLayerOnly == enabled)
        return;

    m_updateLowScaleLayerOnly = enabled;

    if (drawingArea()->layerTreeCoordinatorProxy())
        drawingArea()->layerTreeCoordinatorProxy()->setCoverWithTilesForLowScaleLayerOnly(m_updateLowScaleLayerOnly);

    if (!enabled)
        setVisibleContentRect(IntRect(m_viewImpl->scrollPosition(), m_visibleContentRect.size()), m_viewImpl->scaleFactor());
}
#endif

#if ENABLE(TIZEN_WEBKIT2_VIEW_VISIBILITY)
void PageClientEvasGL::setIsVisible(bool isVisible)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (isDirectRendering())
#endif
    if (m_isVisible != isVisible && m_viewImpl->view() && m_pageDidRendered) {
        if (!isVisible && (drawingArea() && drawingArea()->layerTreeCoordinatorProxy()) && !m_shouldShowBackupTexture) {
            m_shouldMakeBackupTexture = true;
            m_shouldShowBackupTexture = true;
            drawContents();
        }
    }
#endif

    PageClientImpl::setIsVisible(isVisible);
}
#endif

void PageClientEvasGL::initializeAcceleratedCompositingMode()
{
    Evas* evas = evas_object_evas_get(m_viewImpl->view());

    m_config = evas_gl_config_new();
    m_config->color_format = EVAS_GL_RGBA_8888;
    m_config->depth_bits = EVAS_GL_DEPTH_BIT_24;
    m_config->stencil_bits = EVAS_GL_STENCIL_BIT_8;

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    char* directRenderingEnv = getenv("TIZEN_WEBKIT_DIRECT_RENDERING");
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    if (!m_isProviderAC)
#endif
    if (!directRenderingEnv || atoi(directRenderingEnv)) {
        m_config->options_bits = (Evas_GL_Options_Bits)(EVAS_GL_OPTIONS_DIRECT | EVAS_GL_OPTIONS_CLIENT_SIDE_ROTATION | EVAS_GL_OPTIONS_DIRECT_MEMORY_OPTIMIZE | EVAS_GL_OPTIONS_DIRECT_OVERRIDE);
        m_config->color_format = EVAS_GL_RGB_888;
    }
#endif

    m_evasGL = evas_gl_new(evas);
    if (!m_evasGL) {
        evas_gl_config_free(m_config);
        m_config = 0;
        TIZEN_LOGE("failed to create evas_gl");
        return;
    }

    Evas_GL_API* evasGlApi = evas_gl_api_get(m_evasGL);
    if (!evasGlApi) {
        evas_gl_free(m_evasGL);
        m_evasGL = 0;
        evas_gl_config_free(m_config);
        m_config = 0;
        TIZEN_LOGE("failed to get evas_gl_api");
        return;
    }
    WebCore::EvasGlApiInterface::shared().initialize(evasGlApi);

    m_context = evas_gl_context_create(m_evasGL, 0);
    if (!m_context) {
        evas_gl_free(m_evasGL);
        m_evasGL = 0;
        evas_gl_config_free(m_config);
        m_config = 0;
        TIZEN_LOGE("failed to create evas_gl_context");
        return;
    }

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    if (m_isProviderAC)
        setProviderSurface();
    else
#endif
    setTargetSurface();

    m_isAcceleratedCompositingModeInitialized =  true;
}

void PageClientEvasGL::finalizeAcceleratedCompositingMode()
{
    if (m_evasGL) {
        if (m_surface) {
            evas_gl_surface_destroy(m_evasGL, m_surface);
            m_surface = 0;
        }
        if (m_context) {
            evas_gl_context_destroy(m_evasGL, m_context);
            m_context = 0;
        }
        if (m_config) {
            evas_gl_config_free(m_config);
            m_config = 0;
        }
        evas_gl_free(m_evasGL);
        m_evasGL = 0;
    }

    m_isAcceleratedCompositingModeInitialized = false;

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    if (!m_isProviderAC)
#endif
    ewk_view_image_native_surface_set(m_viewImpl->view(), 0);
}

void PageClientEvasGL::enterAcceleratedCompositingMode(const LayerTreeContext&)
{
    if (!m_isAcceleratedCompositingModeInitialized)
        initializeAcceleratedCompositingMode();
}

void PageClientEvasGL::exitAcceleratedCompositingMode()
{
    if (m_isAcceleratedCompositingModeInitialized)
        finalizeAcceleratedCompositingMode();
}

void PageClientEvasGL::setTargetSurface()
{
    if (!m_evasGL)
        return;

    int width, height;
    evas_object_geometry_get(m_viewImpl->view(), 0, 0, &width, &height);
    if (width == 0 || height == 0)
        return;

    m_surface = evas_gl_surface_create(m_evasGL, m_config, width, height);
    if (!m_surface) {
        TIZEN_LOGE("failed to create Evas_GL_Surface");
        return;
    }

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    if (isDirectRendering())
        makeContextCurrent();
    else
#endif
    {
        if (makeContextCurrent()) {
            glViewport(0, 0, width, height);
            glClearColor(1.0, 1.0, 1.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);
            glFinish();
        }
    }

    Evas_Native_Surface nativeSurface;
    if (evas_gl_native_surface_get(m_evasGL, m_surface, &nativeSurface))
        ewk_view_image_native_surface_set(m_viewImpl->view(), &nativeSurface);

}

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
void PageClientEvasGL::setProviderSurface()
{
    if (!m_evasGL)
        return;

    int width, height;
    evas_object_geometry_get(m_viewImpl->view(), 0, 0, &width, &height);
    if (width == 0 || height == 0)
        return;

    typedef Evas_GL_Surface *(*evas_gl_ext_surface_from_native_create_func) (Evas_GL *evas_gl, Evas_GL_Config *config, int target, void *native);
    evas_gl_ext_surface_from_native_create_func evas_gl_ext_surface_from_native_create = NULL;
    evas_gl_ext_surface_from_native_create = reinterpret_cast<evas_gl_ext_surface_from_native_create_func>(evas_gl_proc_address_get(m_evasGL, "evas_gl_ext_surface_from_native_create"));
    if (!evas_gl_ext_surface_from_native_create) {
        TIZEN_LOGE("failed to get evas_gl_ext_surface_from_native_create pointer");
        return;
    }

    m_surface = evas_gl_ext_surface_from_native_create(m_evasGL, m_config, 1, (void*)m_pixmap);
    if (!m_surface) {
        TIZEN_LOGE("failed to create Evas_GL_Surface");
        return;
    }

    makeContextCurrent();
}
#endif

#if ENABLE(TIZEN_BACKGROUND_COLOR_API)
WebCore::Color PageClientEvasGL::backgroundColor()
{
    return drawingArea()->layerTreeCoordinatorProxy()->layerTreeRenderer()->backgroundColor();
}

void PageClientEvasGL::setBackgroundColor(const WebCore::Color& color)
{
    drawingArea()->layerTreeCoordinatorProxy()->layerTreeRenderer()->setBackgroundColor(color);
}
#endif

#endif

} // namespace WebKit
