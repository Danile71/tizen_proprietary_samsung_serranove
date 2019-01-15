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

#ifndef PageClientImpl_h
#define PageClientImpl_h

#include "PageClient.h"
#include <Evas.h>

#if PLATFORM(TIZEN)
#include "WebEditCommandProxy.h"
#include <WebCore/ViewportArguments.h>

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
#include "DrawingAreaProxyImpl.h"
#include <WebCore/Color.h>
#endif
#endif
#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include "OfflinePageSave.h"
#endif
#if ENABLE(TIZEN_DRAG_SUPPORT)
#include "Drag.h"
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#include "AutoFillPopup.h"
#include "PasswordSaveConfirmPopup.h"
#endif
using std::numeric_limits;
#endif // #if PLATFORM(TIZEN)

class EwkViewImpl;

namespace WebKit {

#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
class MainFrameScrollbarTizen;
#endif

#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
class ClipboardHelper;
#endif

class PageClientImpl : public PageClient {
public:
    static PassOwnPtr<PageClientImpl> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new PageClientImpl(viewImpl));
    }
    virtual ~PageClientImpl();

    EwkViewImpl* viewImpl() const;

#if PLATFORM(TIZEN)
    struct ViewportConstraints {
        ViewportConstraints()
            : initialScale(1.0)
            , minimumScale(1.0)
            , maximumScale(1.0)
            , userScalable(true)
            , fixedInitialScale(false)
            , layoutSize(WebCore::IntSize())
        {
        }
        double initialScale;
        double minimumScale;
        double maximumScale;
        bool userScalable;
        // fixedInitialScale is enabled when below conditions are satisfied
        // 1. There's content defined "initial-scale" value on viewport meta tag
        // 2. initialScale is calculated by disabled autofitting setting
        bool fixedInitialScale;
        WebCore::IntSize layoutSize;
    };

    bool userScalable() { return fabs(viewportConstraints().minimumScale - viewportConstraints().maximumScale) > numeric_limits<float>::epsilon(); }
    ViewportConstraints viewportConstraints();
    ViewportConstraints computeViewportConstraints(const WebCore::ViewportAttributes&);
    double adjustScaleWithViewport(double);
#if USE(TILED_BACKING_STORE) && ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    TIZEN_VIRTUAL void updateViewportSize(const WebCore::IntSize&);
    TIZEN_VIRTUAL void updateVisibleContentRectSize(const WebCore::IntSize&);
#endif
#if ENABLE(TIZEN_WEBKIT2_HISTORICAL_RESTORE_VISIBLE_CONTENT_RECT)
    void prepareRestoredVisibleContectRect();
#if ENABLE(TIZEN_WEBKIT2_RESTORE_SCROLLPOINT_ON_FRAME_LOAD_FINISH)
    // Return true if it's restored by history control
    bool restoreVisibleContentRect();
#endif
#endif
    void initializeVisibleContentRect();
    double availableMinimumScale();
    bool isContentSizeSameAsViewport();
    void fitViewportToContent(bool = true);
    bool wasViewportFitsToContent() { return m_viewportFitsToContent; }

    bool pageDidRendered() { return m_pageDidRendered; }

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    void textChangeInTextField(const String& name, const String& value, bool isInputInForm, bool isForcePopupShow, bool isHandleEvent);
#else
    void textChangeInTextField(const String& name, const String& value);
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif

    void setViewFocused(bool focused) { m_viewFocused = focused; }
    void setViewWindowActive(bool active) { m_viewWindowActive = active; }

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    void updateFormNavigation(int length, int offset, bool prevState, bool nextState);
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    DrawingAreaProxy* drawingArea() const;
#endif
    float scaleFactor();
    const WebCore::IntPoint scrollPosition();
    WebCore::IntRect adjustVisibleContentRect(WebCore::IntRect, float);
    void setVisibleContentRect(const WebCore::IntRect&, float newScale, const WebCore::FloatPoint& trajectory = WebCore::FloatPoint());
    void scheduleDisplayViewport(float);
    void displayViewportTimerFired(WebCore::Timer<PageClientImpl>*);
    TIZEN_VIRTUAL void displayViewport();
    TIZEN_VIRTUAL void drawContents();
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    virtual void drawContents(BackingStore::PlatformGraphicsContext);
#endif
    void scaleImage(double, WebCore::IntPoint);
    void scaleContents(double, const WebCore::IntPoint&);
    WebCore::IntRect visibleContentRect() { return m_visibleContentRect; }

#if ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
    void changeToMediumScaleImage(double, const WebCore::IntPoint&);
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
    void updateScrollbar();
    void frameRectChanged();
    void updateVisibility();
#endif

    // FIXME: The concept of suspending content comes from webkit opensource's PageViewportController,
    // so below code should be replaced when PageViewportController codes are merged.
    // Please do not use below codes. They are only for scaling contents.
    void suspendContent();
    void resumeContent();
    WebCore::FloatPoint boundContentsPositionAtScale(const WebCore::FloatPoint&, float scale);
#else
    float scaleFactor() { return page()->pageScaleFactor(); }
    WebCore::IntPoint scrollPosition() { return page()->scrollPosition(); }
#endif // #if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    bool textSelectionDown(const WebCore::IntPoint& point);
    void textSelectionMove(const WebCore::IntPoint& point);
    void textSelectionUp(const WebCore::IntPoint& point, bool isStartedTextSelectionFromOutside = false);
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    void saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName);
    void saveSubresourcesData(Vector<WebSubresourceTizen>& subresourceData);
    void startOfflinePageSave(String& path, String& url, String& title);
#endif

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
    void setClipboardData(const String& data, const String& type);
    void clearClipboardData();
#endif

#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
    void pasteContextMenuSelected();
#endif


#if ENABLE(TIZEN_WEBKIT2_VIEW_VISIBILITY)
    TIZEN_VIRTUAL void setIsVisible(bool isVisible);
    TIZEN_VIRTUAL bool isVisible() { return m_isVisible; }
#endif

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    void setIsForeground(bool isForeground);
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    bool scrollBy(WebCore::IntSize);
    void scrollTo(WebCore::IntPoint);
#endif

    void setIsScrollableLayerFocused(const bool b) { m_isScrollableLayerFocused = b; }
    bool isScrollableLayerFocused() const { return m_isScrollableLayerFocused; }

    void setIsScrollableNodeFocused(const bool b) { m_isScrollableNodeFocused = b; }
    bool isScrollableNodeFocused() const { return m_isScrollableNodeFocused; }

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    void setOverflowResult(bool pressed, WebLayerID);
    void findScrollableNode(const WebCore::IntPoint&);
#endif

#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    void setUrlBarScrollTrigger (const bool b) { m_urlBarScrollTirgger = b; }
#endif

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    bool isContextMenuVisible() { return m_isContextMenuVisible; }
    void setIsContextMenuVisible(bool);
#endif
    TIZEN_VIRTUAL void didRenderFrame();
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    TIZEN_VIRTUAL bool makeContextCurrent() { return true; }
    TIZEN_VIRTUAL void setUpdateViewportWithAnimatorEnabled(bool) {}
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    TIZEN_VIRTUAL void setLowScaleLayerFlag(bool) {}

    TIZEN_VIRTUAL bool updateLowScaleLayerOnly(void) { return false; }
    TIZEN_VIRTUAL void setUpdateLowScaleLayerOnly(bool) {}
#endif //TIZEN_LOW_SCALE_LAYER

#endif

    virtual void didFindZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&);

#if ENABLE(TIZEN_ICON_DATABASE)
    virtual void didReceiveIcon();
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    void clipboardContextMenuSelected();
    bool isClipboardWindowOpened();
    void closeClipboardWindow();
    bool isPastedItemOnlyImage();
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
    void updateDragPosition();
    void setDragPoint(const WebCore::IntPoint& point);
    bool isDragMode();
    void setDragMode(bool isDragMode);
#endif

#if ENABLE(TIZEN_POPUP_BLOCKED_NOTIFICATION)
    virtual void popupBlocked(const String& urlString);
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    bool isShowingAutoFillPopup();
    void updateAutoFillPopup(const Vector<AutoFillPopupItem>& data);
    void hideAutoFillPopup();
    void showAutoFillPopup(const WebCore::IntRect& rect);
#endif

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    void setPreparingRotation(bool preparingRotation) { m_preparingRotation = preparingRotation; }
#endif
    // Added to check whether visible content rect can be updated or not
    // Returns true when first frame is rendered and resizing view is done
    bool canUpdateVisibleContentRect();

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    void mediaControlsRequestRotate(const String&);
#endif

    bool notifiedNonemptyLayout() { return m_nonemptyLayoutRendered; }

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_WEB_SEARCH)
    void searchWebContextMenuSelected(String& linkURL);
#endif

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
    void addMessageToConsole(unsigned level, const String& message, unsigned lineNumber, const String& source);
#endif

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    void resizeEventDone();
    void resetPreparingRotationStatus();
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setPaintOnlyLowScaleLayer(bool b) { m_shouldPaintOnlyLowScaleLayer = b; }
#endif
#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
    void didCreatePagesToPDF();
#endif

#if ENABLE(TIZEN_BACKGROUND_COLOR_API)
    virtual WebCore::Color backgroundColor() { return WebCore::Color::white; }
    virtual void setBackgroundColor(const WebCore::Color&) {}
#endif

#endif // #if PLATFORM(TIZEN)

private:
    // PageClient
    virtual PassOwnPtr<DrawingAreaProxy> createDrawingAreaProxy();
    virtual void setViewNeedsDisplay(const WebCore::IntRect&);
    virtual void displayView();
    virtual void scrollView(const WebCore::IntRect&, const WebCore::IntSize&);
    virtual bool isViewWindowActive() { return m_viewWindowActive; }
    virtual bool isViewFocused() { return m_viewFocused; }
    virtual bool isViewVisible();
    virtual bool isViewInWindow();

    virtual void processDidCrash();
    virtual void didRelaunchProcess();
    virtual void pageClosed();

    virtual void toolTipChanged(const String&, const String&);

    virtual void setCursor(const WebCore::Cursor&);
    virtual void setCursorHiddenUntilMouseMoves(bool);
    virtual void didChangeViewportProperties(const WebCore::ViewportAttributes&);

    virtual void registerEditCommand(PassRefPtr<WebEditCommandProxy>, WebPageProxy::UndoOrRedo);
    virtual void clearAllEditCommands();
    virtual bool canUndoRedo(WebPageProxy::UndoOrRedo);
    virtual void executeUndoRedo(WebPageProxy::UndoOrRedo);
    virtual WebCore::FloatRect convertToDeviceSpace(const WebCore::FloatRect&);
    virtual WebCore::FloatRect convertToUserSpace(const WebCore::FloatRect&);
    virtual WebCore::IntPoint screenToWindow(const WebCore::IntPoint&);
    virtual WebCore::IntRect windowToScreen(const WebCore::IntRect&);

    void updateTextInputState();
    virtual void handleDownloadRequest(DownloadProxy*);

    virtual void doneWithKeyEvent(const NativeWebKeyboardEvent&, bool);
#if ENABLE(GESTURE_EVENTS)
    virtual void doneWithGestureEvent(const WebGestureEvent&, bool wasEventHandled);
#endif
#if ENABLE(TOUCH_EVENTS)
    virtual void doneWithTouchEvent(const NativeWebTouchEvent&, bool wasEventHandled);
#endif

    virtual PassRefPtr<WebPopupMenuProxy> createPopupMenuProxy(WebPageProxy*);
    virtual PassRefPtr<WebContextMenuProxy> createContextMenuProxy(WebPageProxy*);

#if ENABLE(INPUT_TYPE_COLOR)
    virtual PassRefPtr<WebColorChooserProxy> createColorChooserProxy(WebPageProxy*, const WebCore::Color& initialColor);
#endif

    virtual void setFindIndicator(PassRefPtr<FindIndicator>, bool, bool);
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    TIZEN_VIRTUAL void initializeAcceleratedCompositingMode();
    TIZEN_VIRTUAL void finalizeAcceleratedCompositingMode() {}
    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&) {}
    virtual void exitAcceleratedCompositingMode() {}

    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&);
#endif

    virtual void didChangeScrollbarsForMainFrame() const;

#if PLATFORM(TIZEN)
    virtual void didFirstVisuallyNonEmptyLayoutForMainFrame();
    virtual void didChangeContentsSize(const WebCore::IntSize);
    virtual void pageScaleFactorDidChange();
#endif

    virtual void didCommitLoadForMainFrame(bool);
    virtual void didFinishLoadingDataForCustomRepresentation(const String&, const CoreIPC::DataReference&);
    virtual double customRepresentationZoomFactor();
    virtual void setCustomRepresentationZoomFactor(double);

    virtual void flashBackingStoreUpdates(const Vector<WebCore::IntRect>&);
    virtual void findStringInCustomRepresentation(const String&, FindOptions, unsigned);
    virtual void countStringMatchesInCustomRepresentation(const String&, FindOptions, unsigned);

#if USE(TILED_BACKING_STORE)
    virtual void pageDidRequestScroll(const WebCore::IntPoint&);
#endif

#if ENABLE(TIZEN_WEBKIT2_HISTORICAL_RESTORE_VISIBLE_CONTENT_RECT)
    virtual void pageDidRequestRestoreVisibleContentRect(const WebCore::IntPoint&, float);
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
    virtual void startDrag(const WebCore::DragData&, PassRefPtr<ShareableBitmap> dragImage);
#endif

#if ENABLE(TIZEN_REGISTER_PROTOCOL_HANDLER)
    virtual void registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title);
#endif
#if ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)
    virtual unsigned int isProtocolHandlerRegistered(const String& scheme, const String& baseURL, const String& url);
    virtual void unregisterProtocolHandler(const String& scheme, const String& baseURL, const String& url);
#endif
#if ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)
    virtual void registerContentHandler(const String& mimeType, const String& baseURL, const String& url, const String& title);
    virtual unsigned int isContentHandlerRegistered(const String& mimeType, const String& baseURL, const String& url);
    virtual void unregisterContentHandler(const String& mimeType, const String& baseURL, const String& url);
#endif
#if ENABLE(TIZEN_SEARCH_PROVIDER)
    virtual void addSearchProvider(const String& baseURL, const String& engineURL);
    virtual unsigned long isSearchProviderInstalled(const String& baseURL, const String& engineURL);
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    virtual bool getStandaloneStatus();
#endif

#if ENABLE(SCREEN_ORIENTATION_SUPPORT) && ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
    virtual bool lockOrientation(int willLockOrientation);
    virtual void unlockOrientation();
#endif

protected:
    explicit PageClientImpl(EwkViewImpl*);

    virtual WebCore::IntSize viewSize();

    EwkViewImpl* m_viewImpl;

#if PLATFORM(TIZEN)
    ViewportConstraints m_viewportConstraints;

    bool m_viewFocused;
    bool m_viewWindowActive;

    bool m_pageDidRendered;

    // m_viewResizeCount is added to sync updateViewportSize() and didChangeViewportProperties()
    int m_viewResizeCount;
    // m_viewportFitsToContent is added to handle scale factor after load finish
    bool m_viewportFitsToContent;

    WebCore::IntRect m_focusedNodeRect;

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    WebCore::IntRect m_visibleContentRect;
    float m_scaleFactor;

    // FIXME: The concept of suspending content comes from webkit opensource's PageViewportController,
    // so below code should be replaced when PageViewportController codes are merged.
    // Please do not use below codes. They are only for scaling contents.
    bool m_hasSuspendedContent;
#endif // ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)

#if ENABLE(TIZEN_WEBKIT2_HISTORICAL_RESTORE_VISIBLE_CONTENT_RECT)
    WebCore::IntPoint m_restoredScrollPosition;
    float m_restoredScaleFactor;
#endif
#if ENABLE(TIZEN_WEBKIT2_BEFORE_PAGE_RENDERED_SCROLL_POSITION)
    WebCore::IntPoint m_scrollPositionBeforePageRendered;
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
    RefPtr<MainFrameScrollbarTizen> m_horizontalScrollbar;
    RefPtr<MainFrameScrollbarTizen> m_verticalScrollbar;
#endif
#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    OwnPtr<OfflinePageSave> m_offlinePageSave;
#endif
#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
    OwnPtr<ClipboardHelper> m_clipboardHelper;
#endif
#if ENABLE(TIZEN_DRAG_SUPPORT)
    OwnPtr<Drag> m_drag;
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    OwnPtr<AutoFillPopup> m_autoFillPopup;
#endif
    bool m_suspendPainting;
    bool m_suspendResource;
    bool m_suspendRequested;
    bool m_isVisible;

    bool m_isScrollableLayerFocused;
    bool m_isScrollableNodeFocused;

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    bool m_isOverflowMovingEnabled;
#endif

#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    bool m_urlBarScrollTirgger;
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    bool m_shouldMakeBackupTexture;
    bool m_shouldShowBackupTexture;
    WebCore::IntRect m_initialViewRect;
#endif

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    bool m_isContextMenuVisible;
#endif

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    bool m_isForeground;
#endif

    WebCore::Color m_bgColor;

    Vector<RefPtr<WebKit::WebEditCommandProxy> > m_undoCommands;
    Vector<RefPtr<WebKit::WebEditCommandProxy> > m_redoCommands;

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    bool m_waitFrameOfNewViewortSize;
    bool m_preparingRotation;
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    bool m_shouldPaintOnlyLowScaleLayer;
    bool m_updateLowScaleLayerOnly;
#endif

    bool m_nonemptyLayoutRendered;

    WebCore::IntSize m_viewportSize;

    WebCore::Timer<PageClientImpl> m_displayViewportTimer;
#endif // #if PLATFORM(TIZEN)
};


#if ENABLE(TIZEN_WEBKIT2_TILED_AC) && ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
class PageClientEvasGL : public PageClientImpl {
public:
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    static PassOwnPtr<PageClientEvasGL> create(EwkViewImpl* viewImpl, int pixmap)
    {
        return adoptPtr(new PageClientEvasGL(viewImpl, pixmap));
    }
#else
    static PassOwnPtr<PageClientEvasGL> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new PageClientEvasGL(viewImpl));
    }
#endif
    ~PageClientEvasGL();

    virtual void updateViewportSize(const WebCore::IntSize&);
    virtual void updateVisibleContentRectSize(const WebCore::IntSize&);
    virtual void setViewNeedsDisplay(const WebCore::IntRect&);
    virtual void displayViewport();
    virtual void drawContents();
    virtual void didRenderFrame();
    virtual bool makeContextCurrent();
    void setUpdateViewportWithAnimatorEnabled(bool);

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setLowScaleLayerFlag(bool);

    bool updateLowScaleLayerOnly(void) { return m_updateLowScaleLayerOnly; }
    void setUpdateLowScaleLayerOnly(bool);
#endif //TIZEN_LOW_SCALE_LAYER

#if ENABLE(TIZEN_WEBKIT2_VIEW_VISIBILITY)
    virtual void setIsVisible(bool isVisible);
#endif

#if ENABLE(TIZEN_BACKGROUND_COLOR_API)
   virtual WebCore::Color backgroundColor();
   virtual void setBackgroundColor(const WebCore::Color& color);
#endif

private:
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    explicit PageClientEvasGL(EwkViewImpl* viewImpl, int pixmap);
#else
    explicit PageClientEvasGL(EwkViewImpl* viewImpl);
#endif

    virtual void initializeAcceleratedCompositingMode();
    virtual void finalizeAcceleratedCompositingMode();
    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode();

    void setTargetSurface();
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    void setProviderSurface();
#endif

#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    bool isDirectRendering() { return (m_config->options_bits & EVAS_GL_OPTIONS_DIRECT); }
#endif

    Evas_GL* m_evasGL;
    Evas_GL_API* m_evasGlApi;
    Evas_GL_Context* m_context;
    Evas_GL_Surface* m_surface;
    Evas_GL_Config* m_config;
#if ENABLE(TIZEN_WEBKIT2_DIRECT_RENDERING)
    int m_angle;
#endif
#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
    int m_backupAngle;
#endif
    bool m_isAcceleratedCompositingModeInitialized;
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    bool m_isProviderAC;
    int m_pixmap;
#endif
};
#endif

} // namespace WebKit

#endif // PageClientImpl_h
