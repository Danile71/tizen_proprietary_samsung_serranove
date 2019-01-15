/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef WebPageProxy_h
#define WebPageProxy_h

#include "APIObject.h"
#include "Connection.h"
#include "DragControllerAction.h"
#include "DrawingAreaProxy.h"
#include "EditorState.h"
#include "GeolocationPermissionRequestManagerProxy.h"
#if ENABLE(TOUCH_EVENTS)
#include "NativeWebTouchEvent.h"
#endif
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
#include "NativeWebKeyboardEvent.h"
#endif
#if PLATFORM(QT)
#include "QtNetworkRequestData.h"
#endif
#include "LayerTreeContext.h"
#include "NotificationPermissionRequestManagerProxy.h"
#include "PlatformProcessIdentifier.h"
#include "SandboxExtension.h"
#include "ShareableBitmap.h"
#include "WKBase.h"
#include "WKPagePrivate.h"
#include "WebColorChooserProxy.h"
#include "WebContextMenuItemData.h"
#include "WebCoreArgumentCoders.h"
#include "WebFindClient.h"
#include "WebFormClient.h"
#include "WebFrameProxy.h"
#include "WebFullScreenManagerProxy.h"
#include "WebHistoryClient.h"
#include "WebHitTestResult.h"
#include "WebLoaderClient.h"
#include "WebPageContextMenuClient.h"
#include "WebPolicyClient.h"
#include "WebPopupMenuProxy.h"
#include "WebResourceLoadClient.h"
#include "WebUIClient.h"
#include <WebCore/AlternativeTextClient.h>
#include <WebCore/Color.h>
#include <WebCore/DragActions.h>
#include <WebCore/DragSession.h>
#include <WebCore/HitTestResult.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformScreen.h>
#include <WebCore/ScrollTypes.h>
#include <WebCore/TextChecking.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if ENABLE(DRAG_SUPPORT)
#include <WebCore/DragActions.h>
#include <WebCore/DragSession.h>
#endif

#if PLATFORM(EFL)
#include <Evas.h>
#include <Ecore.h>
#include <Ecore_IMF_Evas.h>
#endif

#if PLATFORM(TIZEN)
#include "WebPageProxyMessages.h"
#include "WebTizenClient.h"
#include <WebCore/Color.h>
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "UserMediaPermissionRequestManagerProxy.h"
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include "WebSubresourceTizen.h"
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
#include "WebLayerTreeInfo.h"
#endif

#if ENABLE(TIZEN_CSP)
#include <WebCore/ContentSecurityPolicy.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
#include <WebCore/VisibleSelection.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#include "WebFormData.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_TOUCH_EVENT_TIMER)
#include <WebCore/Timer.h>
#endif

#if HAVE(ACCESSIBILITY)
#include "WKEinaSharedString.h"
#endif

namespace CoreIPC {
    class ArgumentDecoder;
    class Connection;
    class MessageID;
}

namespace WebCore {
    class AuthenticationChallenge;
    class Cursor;
    class DragData;
    class FloatRect;
    class IntSize;
    class ProtectionSpace;
    struct FileChooserSettings;
    struct TextAlternativeWithRange;
    struct TextCheckingResult;
    struct ViewportAttributes;
    struct WindowFeatures;
}

#if PLATFORM(QT)
class QQuickNetworkReply;
#endif

#if USE(APPKIT)
#ifdef __OBJC__
@class WKView;
#else
class WKView;
#endif
#endif

#if ENABLE(WEB_INTENTS)
class WebIntentData;
#endif

namespace WebKit {

#if !ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
class NativeWebKeyboardEvent;
#endif
class NativeWebMouseEvent;
class NativeWebWheelEvent;
class PageClient;
class PlatformCertificateInfo;
class StringPairVector;
class WebBackForwardList;
class WebBackForwardListItem;
#if ENABLE(TIZEN_INPUT_COLOR_PICKER) // wait for upstream
class WebColorPickerResultListenerProxy;
#endif
class WebContextMenuProxy;
class WebData;
class WebEditCommandProxy;
class WebKeyboardEvent;
class WebMouseEvent;
class WebOpenPanelResultListenerProxy;
class WebPageGroup;
class WebProcessProxy;
#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
class WebSubresourceTizen;
#endif
class WebURLRequest;
class WebWheelEvent;
struct AttributedString;
struct ColorSpaceData;
struct DictionaryPopupInfo;
struct EditorState;
struct PlatformPopupMenuData;
struct PrintInfo;
struct WebPageCreationParameters;
struct WebPopupItem;

#if PLATFORM(WIN)
struct WindowGeometry;
#endif

#if ENABLE(GESTURE_EVENTS)
class WebGestureEvent;
#endif

#if ENABLE(WEB_INTENTS)
struct IntentData;
#endif

#if ENABLE(WEB_INTENTS_TAG)
struct IntentServiceInfo;
#endif

typedef GenericCallback<WKStringRef, StringImpl*> StringCallback;
typedef GenericCallback<WKSerializedScriptValueRef, WebSerializedScriptValue*> ScriptValueCallback;
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
typedef GenericCallback<WKBooleanRef> BooleanCallback;
typedef GenericCallback<WKDictionaryRef> DictionaryCallback;
#endif
#if ENABLE(TIZEN_WEB_STORAGE)
#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
typedef GenericCallback<WKUInt32Ref> WebStorageQuotaCallback;
#endif
#endif

#if PLATFORM(GTK)
typedef GenericCallback<WKErrorRef> PrintFinishedCallback;
#endif

#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
template<typename T>
struct QueuedUIEvents {
    QueuedUIEvents(const T& event)
        : forwardedEvent(event)
    {
    }
    T forwardedEvent;
    Vector<T> deferredEvents;
};
#else
#if ENABLE(TOUCH_EVENTS)
struct QueuedTouchEvents {
    QueuedTouchEvents(const NativeWebTouchEvent& event)
        : forwardedEvent(event)
    {
    }
    NativeWebTouchEvent forwardedEvent;
    Vector<NativeWebTouchEvent> deferredTouchEvents;
};
#endif
#endif

// FIXME: Make a version of CallbackBase with three arguments, and define ValidateCommandCallback as a specialization.
class ValidateCommandCallback : public CallbackBase {
public:
    typedef void (*CallbackFunction)(WKStringRef, bool, int32_t, WKErrorRef, void*);

    static PassRefPtr<ValidateCommandCallback> create(void* context, CallbackFunction callback)
    {
        return adoptRef(new ValidateCommandCallback(context, callback));
    }

    virtual ~ValidateCommandCallback()
    {
        ASSERT(!m_callback);
    }

    void performCallbackWithReturnValue(StringImpl* returnValue1, bool returnValue2, int returnValue3)
    {
        ASSERT(m_callback);

        m_callback(toAPI(returnValue1), returnValue2, returnValue3, 0, context());

        m_callback = 0;
    }
    
    void invalidate()
    {
        ASSERT(m_callback);

        RefPtr<WebError> error = WebError::create();
        m_callback(0, 0, 0, toAPI(error.get()), context());
        
        m_callback = 0;
    }

private:

    ValidateCommandCallback(void* context, CallbackFunction callback)
        : CallbackBase(context)
        , m_callback(callback)
    {
    }

    CallbackFunction m_callback;
};

class WebPageProxy
    : public APIObject
#if ENABLE(INPUT_TYPE_COLOR)
    , public WebColorChooserProxy::Client
#endif
    , public WebPopupMenuProxy::Client {
public:
    static const Type APIType = TypePage;

    static PassRefPtr<WebPageProxy> create(PageClient*, PassRefPtr<WebProcessProxy>, WebPageGroup*, uint64_t pageID);
    virtual ~WebPageProxy();

    uint64_t pageID() const { return m_pageID; }

    WebFrameProxy* mainFrame() const { return m_mainFrame.get(); }
    WebFrameProxy* focusedFrame() const { return m_focusedFrame.get(); }
    WebFrameProxy* frameSetLargestFrame() const { return m_frameSetLargestFrame.get(); }

    DrawingAreaProxy* drawingArea() const { return m_drawingArea.get(); }

    WebBackForwardList* backForwardList() const { return m_backForwardList.get(); }

#if ENABLE(TIZEN_WEBKIT2_NOTIFY_SUSPEND_BY_REMOTE_WEB_INSPECTOR)
    bool isContentSuspendedByInspector() { return m_contentSuspendedByInspector; }
#endif

#if ENABLE(INSPECTOR)
    WebInspectorProxy* inspector();
#endif

#if ENABLE(FULLSCREEN_API)
    WebFullScreenManagerProxy* fullScreenManager();
#endif

#if ENABLE(CONTEXT_MENUS)
    void initializeContextMenuClient(const WKPageContextMenuClient*);
#endif
    void initializeFindClient(const WKPageFindClient*);
    void initializeFindMatchesClient(const WKPageFindMatchesClient*);
    void initializeFormClient(const WKPageFormClient*);
    void initializeLoaderClient(const WKPageLoaderClient*);
    void initializePolicyClient(const WKPagePolicyClient*);
    void initializeResourceLoadClient(const WKPageResourceLoadClient*);
    void initializeUIClient(const WKPageUIClient*);
#if PLATFORM(TIZEN)
    void initializeTizenClient(const WKPageTizenClient*);
#endif

    void initializeWebPage();

    void close();
    bool tryClose();
    bool isClosed() const { return m_isClosed; }

    void loadURL(const String&);
    void loadURLRequest(WebURLRequest*);
    void loadHTMLString(const String& htmlString, const String& baseURL);
    void loadAlternateHTMLString(const String& htmlString, const String& baseURL, const String& unreachableURL);
    void loadPlainTextString(const String& string);
    void loadWebArchiveData(const WebData*);
#if PLATFORM(TIZEN)
    void loadContentsbyMimeType(const WebData*, const String& mimeType, const String& encoding, const String& baseURL);
#endif

    void stopLoading();
    void reload(bool reloadFromOrigin);

    void goForward();
    bool canGoForward() const;
    void goBack();
    bool canGoBack() const;

    void goToBackForwardItem(WebBackForwardListItem*);
    void tryRestoreScrollPosition();
    void didChangeBackForwardList(WebBackForwardListItem* addedItem, Vector<RefPtr<APIObject> >* removedItems);
    void shouldGoToBackForwardListItem(uint64_t itemID, bool& shouldGoToBackForwardListItem);
    void willGoToBackForwardListItem(uint64_t itemID, CoreIPC::ArgumentDecoder* arguments);

    String activeURL() const;
    String provisionalURL() const;
    String committedURL() const;

    bool willHandleHorizontalScrollEvents() const;

    bool canShowMIMEType(const String& mimeType) const;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    bool isMediaMIMEType(const String& mimeType) const;
    bool isAudioMIMEType(const String& mimeType) const;
#endif

    bool drawsBackground() const { return m_drawsBackground; }
    void setDrawsBackground(bool);

    bool drawsTransparentBackground() const { return m_drawsTransparentBackground; }
    void setDrawsTransparentBackground(bool);

    void viewWillStartLiveResize();
    void viewWillEndLiveResize();

    void setInitialFocus(bool forward, bool isKeyboardEventValid, const WebKeyboardEvent&);
    void setWindowResizerSize(const WebCore::IntSize&);
    
    void clearSelection();

    void setViewNeedsDisplay(const WebCore::IntRect&);
    void displayView();
    void scrollView(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollOffset);

    enum {
        ViewWindowIsActive = 1 << 0,
        ViewIsFocused = 1 << 1,
        ViewIsVisible = 1 << 2,
        ViewIsInWindow = 1 << 3,
    };
    typedef unsigned ViewStateFlags;
    void viewStateDidChange(ViewStateFlags flags);

    WebCore::IntSize viewSize() const;
    bool isViewVisible() const { return m_isVisible; }
    bool isViewWindowActive() const;

    void executeEditCommand(const String& commandName);
#if PLATFORM(TIZEN)
    void executeEditCommandWithArgument(const String& commandName, const String& argument);
#endif
    void validateCommand(const String& commandName, PassRefPtr<ValidateCommandCallback>);

    const EditorState& editorState() const { return m_editorState; }
    bool canDelete() const { return hasSelectedRange() && isContentEditable(); }
    bool hasSelectedRange() const { return m_editorState.selectionIsRange; }
    bool isContentEditable() const { return m_editorState.isContentEditable; }
    
    bool maintainsInactiveSelection() const { return m_maintainsInactiveSelection; }
    void setMaintainsInactiveSelection(bool);
#if PLATFORM(QT)
    void registerApplicationScheme(const String& scheme);
    void resolveApplicationSchemeRequest(QtNetworkRequestData);
    void sendApplicationSchemeReply(const QQuickNetworkReply*);
    void authenticationRequiredRequest(const String& hostname, const String& realm, const String& prefilledUsername, String& username, String& password);
    void certificateVerificationRequest(const String& hostname, bool& ignoreErrors);
    void proxyAuthenticationRequiredRequest(const String& hostname, uint16_t port, const String& prefilledUsername, String& username, String& password);
    void setUserScripts(const Vector<String>&);
#endif // PLATFORM(QT).
#if PLATFORM(EFL)
    void setThemePath(const String&);
#endif

#if PLATFORM(QT)
    void setComposition(const String& text, Vector<WebCore::CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd);
    void confirmComposition(const String& compositionString, int64_t selectionStart, int64_t selectionLength);
    void cancelComposition();
#endif
#if PLATFORM(MAC)
    void updateWindowIsVisible(bool windowIsVisible);
    void windowAndViewFramesChanged(const WebCore::IntRect& windowFrameInScreenCoordinates, const WebCore::IntRect& viewFrameInWindowCoordinates, const WebCore::IntPoint& accessibilityViewCoordinates);

    void setComposition(const String& text, Vector<WebCore::CompositionUnderline> underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd);
    void confirmComposition();
    void cancelComposition();
    bool insertText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd);
    bool insertDictatedText(const String& text, uint64_t replacementRangeStart, uint64_t replacementRangeEnd, const Vector<WebCore::TextAlternativeWithRange>& dictationAlternatives);
    void getMarkedRange(uint64_t& location, uint64_t& length);
    void getSelectedRange(uint64_t& location, uint64_t& length);
    void getAttributedSubstringFromRange(uint64_t location, uint64_t length, AttributedString&);
    uint64_t characterIndexForPoint(const WebCore::IntPoint);
    WebCore::IntRect firstRectForCharacterRange(uint64_t, uint64_t);
    bool executeKeypressCommands(const Vector<WebCore::KeypressCommand>&);

    void sendComplexTextInputToPlugin(uint64_t pluginComplexTextInputIdentifier, const String& textInput);
    CGContextRef containingWindowGraphicsContext();
    bool shouldDelayWindowOrderingForEvent(const WebMouseEvent&);
    bool acceptsFirstMouse(int eventNumber, const WebMouseEvent&);
    
#if USE(APPKIT)
    WKView* wkView() const;
#endif
#endif
#if PLATFORM(WIN)
    void didChangeCompositionSelection(bool);
    void confirmComposition(const String&);
    void setComposition(const String&, Vector<WebCore::CompositionUnderline>&, int);
    WebCore::IntRect firstRectForCharacterInSelectedRange(int);
    String getSelectedText();

    bool gestureWillBegin(const WebCore::IntPoint&);
    void gestureDidScroll(const WebCore::IntSize&);
    void gestureDidEnd();

    void setGestureReachedScrollingLimit(bool);

    HWND nativeWindow() const;
#endif
#if PLATFORM(EFL)
    void handleInputMethodKeydown(bool& handled);
    void confirmComposition(const String&);
    void setComposition(const String&, Vector<WebCore::CompositionUnderline>&, int);
    void cancelComposition();
#endif
#if PLATFORM(GTK)
    GtkWidget* viewWidget();
#endif
#if PLATFORM(EFL)
    Evas_Object* viewWidget();
#endif

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
    static Eina_Bool updateOnlyVisibleAreaStateTimerFired(void* data);
#endif
#if ENABLE(TIZEN_TEXT_CARET_HANDLING_WK2)
    void setCaretPosition(const WebCore::IntPoint&);
#endif
    bool scrollMainFrameBy(const WebCore::IntSize&);
    void scrollMainFrameTo(const WebCore::IntPoint&);
    WebCore::IntPoint& scrollPosition() { return m_scrollPosition; }
    WebCore::IntSize& contentsSize() { return m_contentsSize; }
    PassRefPtr<WebImage> createSnapshot(const WebCore::IntRect&, float);
#if ENABLE(TIZEN_CACHE_IMAGE_GET)
    PassRefPtr<WebImage> cacheImageGet(const String& imageUrl);
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    void requestUpdateFormNavigation();
    void moveFocus(int);
    void updateFormNavigation(int length, int offset, bool prevState, bool nextState);
    bool formIsNavigating() const { return m_formIsNavigating; }
    void setFormIsNavigating(bool formIsNavigating);
    void updateTextInputStateByUserAction(bool focus);
#endif

#if ENABLE(TIZEN_WEB_STORAGE)
#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
    void getWebStorageQuotaBytes(PassRefPtr<WebStorageQuotaCallback>);
    void didGetWebStorageQuotaBytes(const uint32_t quota, uint64_t callbackID);
#endif
    void setWebStorageQuotaBytes(uint32_t quota);
#endif

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#if ENABLE(TOUCH_ADJUSTMENT)
    WebHitTestResult::Data hitTestResultAtPoint(const WebCore::IntPoint&, int hitTestMode = WebHitTestResult::HitTestModeDefault, const WebCore::IntSize& area = WebCore::IntSize());
#else
    WebHitTestResult::Data hitTestResultAtPoint(const WebCore::IntPoint&, int hitTestMode = WebHitTestResult::HitTestModeDefault);
#endif
#endif
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    void hideContextMenu();
    String contextMenuAbsoluteLinkURLString();
    String contextMenuAbsoluteImageURLString();
#if ENABLE(TIZEN_FOCUS_UI)
    void setContextMenuFocusable();
#endif
#endif
#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
    void setClipboardData(const String& data, const String& type);
    void clearClipboardData();
#endif
#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
    void didSelectPasteMenuFromContextMenu(const String& data, const String& type);
    void pasteContextMenuSelected();
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    void executePasteFromClipboardItem(const String& data, const String& type);
#endif
#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
    void createPagesToPDF(const WebCore::IntSize&, const WebCore::IntSize&, const String&);
    void didCreatePagesToPDF();
#endif

    void scale(double, const WebCore::IntPoint&);
    void scaleImage(double, const WebCore::IntPoint&);
    double scaleFactor();

#if ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
    void changeToMediumScaleImage(double, const WebCore::IntPoint&);
#endif

#if ENABLE(TIZEN_ORIENTATION_EVENTS)
    void sendOrientationChangeEvent(int orientation);
#endif
#if ENABLE(TIZEN_LOW_SCALE_LAYER)
    void setPaintOnlyLowScaleLayer(bool);
#endif

    void suspendPainting();
    void resumePainting();

    void suspendJavaScriptAndResource();
    void resumeJavaScriptAndResource();

    void suspendAnimations();
    void resumeAnimations();

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
    void suspendPlugin();
    void resumePlugin();
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    void purgeBackingStoresOfInactiveView();
#endif

#endif // #if PLATFORM(TIZEN)

#if USE(TILED_BACKING_STORE)
    void setViewportSize(const WebCore::IntSize&);
#endif

    void handleMouseEvent(const NativeWebMouseEvent&);
    void handleWheelEvent(const NativeWebWheelEvent&);
    void handleKeyboardEvent(const NativeWebKeyboardEvent&);
#if ENABLE(GESTURE_EVENTS)
    void handleGestureEvent(const WebGestureEvent&);
#endif
#if ENABLE(TOUCH_EVENTS)
    void handleTouchEvent(const NativeWebTouchEvent&);
#if PLATFORM(QT)
    void handlePotentialActivation(const WebCore::IntPoint& touchPoint, const WebCore::IntSize& touchArea);
#endif
#endif

    void scrollBy(WebCore::ScrollDirection, WebCore::ScrollGranularity);
    void centerSelectionInVisibleArea();

    String pageTitle() const;
    const String& toolTip() const { return m_toolTip; }

    void setUserAgent(const String&);
    const String& userAgent() const { return m_userAgent; }
    void setApplicationNameForUserAgent(const String&);
    const String& applicationNameForUserAgent() const { return m_applicationNameForUserAgent; }
    void setCustomUserAgent(const String&);
    const String& customUserAgent() const { return m_customUserAgent; }
    static String standardUserAgent(const String& applicationName = String());
#if ENABLE(TIZEN_CUSTOM_HEADERS)
    void addCustomHeader(const String&, const String&);
    void removeCustomHeader(const String&);
    void clearCustomHeaders();
#endif

    bool supportsTextEncoding() const;
    void setCustomTextEncodingName(const String&);
    String customTextEncodingName() const { return m_customTextEncodingName; }

    void resumeActiveDOMObjectsAndAnimations();
    void suspendActiveDOMObjectsAndAnimations();

    double estimatedProgress() const;

    void terminateProcess();

    typedef bool (*WebPageProxySessionStateFilterCallback)(WKPageRef, WKStringRef type, WKTypeRef object, void* context);
    PassRefPtr<WebData> sessionStateData(WebPageProxySessionStateFilterCallback, void* context) const;
    void restoreFromSessionStateData(WebData*);

    bool supportsTextZoom() const;
    double textZoomFactor() const { return m_mainFrameHasCustomRepresentation ? 1 : m_textZoomFactor; }
    void setTextZoomFactor(double);
    double pageZoomFactor() const;
    void setPageZoomFactor(double);
    void setPageAndTextZoomFactors(double pageZoomFactor, double textZoomFactor);

    void scalePage(double scale, const WebCore::IntPoint& origin);
    double pageScaleFactor() const { return m_pageScaleFactor; }

    float deviceScaleFactor() const;
    void setIntrinsicDeviceScaleFactor(float);
    void setCustomDeviceScaleFactor(float);
    void windowScreenDidChange(PlatformDisplayID);

    LayerHostingMode layerHostingMode() const { return m_layerHostingMode; }

    void setUseFixedLayout(bool);
    void setFixedLayoutSize(const WebCore::IntSize&);
    bool useFixedLayout() const { return m_useFixedLayout; };
    const WebCore::IntSize& fixedLayoutSize() const { return m_fixedLayoutSize; };

    bool hasHorizontalScrollbar() const { return m_mainFrameHasHorizontalScrollbar; }
    bool hasVerticalScrollbar() const { return m_mainFrameHasVerticalScrollbar; }

    bool isPinnedToLeftSide() const { return m_mainFrameIsPinnedToLeftSide; }
    bool isPinnedToRightSide() const { return m_mainFrameIsPinnedToRightSide; }

    void setPaginationMode(WebCore::Page::Pagination::Mode);
    WebCore::Page::Pagination::Mode paginationMode() const { return m_paginationMode; }
    void setPaginationBehavesLikeColumns(bool);
    bool paginationBehavesLikeColumns() const { return m_paginationBehavesLikeColumns; }
    void setPageLength(double);
    double pageLength() const { return m_pageLength; }
    void setGapBetweenPages(double);
    double gapBetweenPages() const { return m_gapBetweenPages; }
    unsigned pageCount() const { return m_pageCount; }

#if PLATFORM(MAC)
    // Called by the web process through a message.
    void registerWebProcessAccessibilityToken(const CoreIPC::DataReference&);
    // Called by the UI process when it is ready to send its tokens to the web process.
    void registerUIProcessAccessibilityTokens(const CoreIPC::DataReference& elemenToken, const CoreIPC::DataReference& windowToken);
    bool readSelectionFromPasteboard(const String& pasteboardName);
    String stringSelectionForPasteboard();
    PassRefPtr<WebCore::SharedBuffer> dataSelectionForPasteboard(const String& pasteboardType);
    void makeFirstResponder();

    ColorSpaceData colorSpace();
#endif

    void pageScaleFactorDidChange(double);

    void setMemoryCacheClientCallsEnabled(bool);

    // Find.
    void findString(const String&, FindOptions, unsigned maxMatchCount);
    void findStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    void getImageForFindMatch(int32_t matchIndex);
    void selectFindMatch(int32_t matchIndex);
    void didGetImageForFindMatch(const ShareableBitmap::Handle& contentImageHandle, uint32_t matchIndex);
    void hideFindUI();
    void countStringMatches(const String&, FindOptions, unsigned maxMatchCount);
    void didCountStringMatches(const String&, uint32_t matchCount);
    void setFindIndicator(const WebCore::FloatRect& selectionRectInWindowCoordinates, const Vector<WebCore::FloatRect>& textRectsInSelectionRectCoordinates, float contentImageScaleFactor, const ShareableBitmap::Handle& contentImageHandle, bool fadeOut, bool animate);
    void didFindString(const String&, uint32_t matchCount);
    void didFailToFindString(const String&);
    void didFindStringMatches(const String&, Vector<Vector<WebCore::IntRect> > matchRects, int32_t firstIndexAfterSelection);
#if PLATFORM(WIN)
    void didInstallOrUninstallPageOverlay(bool);
#endif

    void getContentsAsString(PassRefPtr<StringCallback>);
#if ENABLE(MHTML)
    void getContentsAsMHTMLData(PassRefPtr<DataCallback>, bool useBinaryEncoding);
#endif
    void getMainResourceDataOfFrame(WebFrameProxy*, PassRefPtr<DataCallback>);
    void getResourceDataFromFrame(WebFrameProxy*, WebURL*, PassRefPtr<DataCallback>);
    void getRenderTreeExternalRepresentation(PassRefPtr<StringCallback>);
    void getSelectionOrContentsAsString(PassRefPtr<StringCallback>);
    void getSourceForFrame(WebFrameProxy*, PassRefPtr<StringCallback>);
    void getWebArchiveOfFrame(WebFrameProxy*, PassRefPtr<DataCallback>);
    void runJavaScriptInMainFrame(const String&, PassRefPtr<ScriptValueCallback>);
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    void runJavaScriptInFocusedFrame(const String&, PassRefPtr<ScriptValueCallback>);
#endif
    void forceRepaint(PassRefPtr<VoidCallback>);

#if ENABLE(WEB_INTENTS)
    void deliverIntentToFrame(WebFrameProxy*, WebIntentData*);
#endif

    float headerHeight(WebFrameProxy*);
    float footerHeight(WebFrameProxy*);
    void drawHeader(WebFrameProxy*, const WebCore::FloatRect&);
    void drawFooter(WebFrameProxy*, const WebCore::FloatRect&);

#if ENABLE(SCREEN_ORIENTATION_SUPPORT) && ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
    void lockOrientation(int orientation, bool& result);
    void unlockOrientation();
#endif

#if PLATFORM(MAC)
    // Dictionary.
    void performDictionaryLookupAtLocation(const WebCore::FloatPoint&);
#endif

    void receivedPolicyDecision(WebCore::PolicyAction, WebFrameProxy*, uint64_t listenerID);

    void backForwardRemovedItem(uint64_t itemID);

#if ENABLE(DRAG_SUPPORT)    
    // Drag and drop support.
    void dragEntered(WebCore::DragData*, const String& dragStorageName = String());
    void dragUpdated(WebCore::DragData*, const String& dragStorageName = String());
    void dragExited(WebCore::DragData*, const String& dragStorageName = String());
#if ENABLE(TIZEN_DRAG_SUPPORT)
    void performDrag(WebCore::DragData*, const String& dragStorageName = String());
#else
    void performDrag(WebCore::DragData*, const String& dragStorageName, const SandboxExtension::Handle&, const SandboxExtension::HandleArray&);
#endif

    void didPerformDragControllerAction(WebCore::DragSession);
    void dragEnded(const WebCore::IntPoint& clientPosition, const WebCore::IntPoint& globalPosition, uint64_t operation);
#if PLATFORM(MAC)
    void setDragImage(const WebCore::IntPoint& clientPosition, const ShareableBitmap::Handle& dragImageHandle, bool isLinkDrag);
    void setPromisedData(const String& pasteboardName, const SharedMemory::Handle& imageHandle, uint64_t imageSize, const String& filename, const String& extension,
                         const String& title, const String& url, const String& visibleURL, const SharedMemory::Handle& archiveHandle, uint64_t archiveSize);
#endif
#if PLATFORM(WIN)
    void startDragDrop(const WebCore::IntPoint& imagePoint, const WebCore::IntPoint& dragPoint, uint64_t okEffect, const HashMap<UINT, Vector<String> >& dataMap, uint64_t fileSize, const String& pathname, const SharedMemory::Handle& fileContentHandle, const WebCore::IntSize& dragImageSize, const SharedMemory::Handle& dragImageHandle, bool isLinkDrag);
#endif
#if PLATFORM(QT) || PLATFORM(GTK) || ENABLE(TIZEN_DRAG_SUPPORT)
    void startDrag(const WebCore::DragData&, const ShareableBitmap::Handle& dragImage);
#endif
#endif

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*, OwnPtr<CoreIPC::ArgumentEncoder>&);

    void processDidBecomeUnresponsive();
    void interactionOccurredWhileProcessUnresponsive();
    void processDidBecomeResponsive();
    void processDidCrash();

#if USE(ACCELERATED_COMPOSITING)
    virtual void enterAcceleratedCompositingMode(const LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode();
    virtual void updateAcceleratedCompositingMode(const LayerTreeContext&);
#endif
    
    void didDraw();

    enum UndoOrRedo { Undo, Redo };
    void addEditCommand(WebEditCommandProxy*);
    void removeEditCommand(WebEditCommandProxy*);
    bool isValidEditCommand(WebEditCommandProxy*);
    void registerEditCommand(PassRefPtr<WebEditCommandProxy>, UndoOrRedo);

#if PLATFORM(MAC)
    void registerKeypressCommandName(const String& name) { m_knownKeypressCommandNames.add(name); }
    bool isValidKeypressCommandName(const String& name) const { return m_knownKeypressCommandNames.contains(name); }
#endif

    WebProcessProxy* process() const;
    PlatformProcessIdentifier processIdentifier() const;

    WebPageGroup* pageGroup() const { return m_pageGroup.get(); }

    bool isValid();

    const String& urlAtProcessExit() const { return m_urlAtProcessExit; }
    WebFrameProxy::LoadState loadStateAtProcessExit() const { return m_loadStateAtProcessExit; }

#if ENABLE(DRAG_SUPPORT)
    WebCore::DragSession dragSession() const { return m_currentDragSession; }
    void resetDragOperation() { m_currentDragSession = WebCore::DragSession(); }
#endif

    void preferencesDidChange();

#if ENABLE(CONTEXT_MENUS)
    // Called by the WebContextMenuProxy.
    void contextMenuItemSelected(const WebContextMenuItemData&);
#endif

    // Called by the WebOpenPanelResultListenerProxy.
    void didChooseFilesForOpenPanel(const Vector<String>&);
    void didCancelForOpenPanel();

    WebPageCreationParameters creationParameters() const;

#if PLATFORM(QT)
    void findZoomableAreaForPoint(const WebCore::IntPoint&, const WebCore::IntSize&);
    void didReceiveMessageFromNavigatorQtObject(const String&);
#endif

#if PLATFORM(QT) || PLATFORM(EFL)
    void handleDownloadRequest(DownloadProxy*);
#endif
#if PLATFORM(TIZEN)
    void findZoomableAreaForPoint(const WebCore::IntPoint&, const WebCore::IntSize&);
#endif // #if PLATFORM(TIZEN)

    void advanceToNextMisspelling(bool startBeforeSelection) const;
    void changeSpellingToWord(const String& word) const;
#if USE(APPKIT)
    void uppercaseWord();
    void lowercaseWord();
    void capitalizeWord();
#endif

#if PLATFORM(MAC)
    bool isSmartInsertDeleteEnabled() const { return m_isSmartInsertDeleteEnabled; }
    void setSmartInsertDeleteEnabled(bool);
#endif

    void setCanRunModal(bool);
    bool canRunModal();

    void beginPrinting(WebFrameProxy*, const PrintInfo&);
    void endPrinting();
    void computePagesForPrinting(WebFrameProxy*, const PrintInfo&, PassRefPtr<ComputedPagesCallback>);
#if PLATFORM(MAC) || PLATFORM(WIN)
    void drawRectToPDF(WebFrameProxy*, const PrintInfo&, const WebCore::IntRect&, PassRefPtr<DataCallback>);
    void drawPagesToPDF(WebFrameProxy*, const PrintInfo&, uint32_t first, uint32_t count, PassRefPtr<DataCallback>);
#elif PLATFORM(GTK)
    void drawPagesForPrinting(WebFrameProxy*, const PrintInfo&, PassRefPtr<PrintFinishedCallback>);
#endif

    const String& pendingAPIRequestURL() const { return m_pendingAPIRequestURL; }

    void flashBackingStoreUpdates(const Vector<WebCore::IntRect>& updateRects);

#if PLATFORM(MAC)
    void handleAlternativeTextUIResult(const String& result);
#endif

    static void setDebugPaintFlags(WKPageDebugPaintFlags flags) { s_debugPaintFlags = flags; }
    static WKPageDebugPaintFlags debugPaintFlags() { return s_debugPaintFlags; }

    // Color to be used with kWKDebugFlashViewUpdates.
    static WebCore::Color viewUpdatesFlashColor();

    // Color to be used with kWKDebugFlashBackingStoreUpdates.
    static WebCore::Color backingStoreUpdatesFlashColor();

    void saveDataToFileInDownloadsFolder(const String& suggestedFilename, const String& mimeType, const String& originatingURLString, WebData*);

    void linkClicked(const String&, const WebMouseEvent&);

    WebCore::IntRect visibleScrollerThumbRect() const { return m_visibleScrollerThumbRect; }

    uint64_t renderTreeSize() const { return m_renderTreeSize; }

    void setShouldSendEventsSynchronously(bool sync) { m_shouldSendEventsSynchronously = sync; };
#if ENABLE(TIZEN_INPUT_TAG_EXTENSION) || ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    void setFocusedInputElementValue(const String&, bool = true);
    String getFocusedInputElementValue();
#endif

#if ENABLE(TIZEN_DATALIST_ELEMENT)
    Vector<String> getFocusedInputElementDataList();
#endif

    void printMainFrame();

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
void recordingSurfaceSetEnableSet(bool enable);
#endif
    
#if ENABLE(TIZEN_REGISTER_PROTOCOL_HANDLER)
    void registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title);
#endif
#if ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)
    void isProtocolHandlerRegistered(const String& scheme, const String& baseURL, const String& url, unsigned int& result);
    void unregisterProtocolHandler(const String& scheme, const String& baseURL, const String& url);
#endif
#if ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)
    void registerContentHandler(const String& mimeType, const String& baseURL, const String& url, const String& title);
    void isContentHandlerRegistered(const String& mimeType, const String& baseURL, const String& url, unsigned int& result);
    void unregisterContentHandler(const String& mimeType, const String& baseURL, const String& url);
#endif

#if ENABLE(TIZEN_SEARCH_PROVIDER)
    void addSearchProvider(const String& baseURL, const String& engineURL);
    void isSearchProviderInstalled(const String& baseURL, const String& engineURL, uint64_t& result);
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    void getStandaloneStatus(bool& standalone);
    void getWebAppCapable(PassRefPtr<BooleanCallback>);
    void getWebAppIconURL(PassRefPtr<StringCallback>);
    void getWebAppIconURLs(PassRefPtr<DictionaryCallback>);
#endif

    void setMediaVolume(float);

#if PLATFORM(TIZEN)
    void replyJavaScriptAlert();
    void replyJavaScriptConfirm(bool result);
    void replyJavaScriptPrompt(const String& result);
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
    void replyBeforeUnloadConfirmPanel(bool result);
#endif
#endif

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    void replyReceiveAuthenticationChallengeInFrame(bool result);
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    void replyPolicyForCertificateError(bool result);
    void setCertificatePemFile(const String& certificate);
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    void replyBlockingToLoadForMalwareScan(bool result);
#endif

#if ENABLE(TIZEN_INTERCEPT_REQUEST)
    void setInterceptRequestEnabled(bool interceptRequestEnabled);
    void receivedInterceptResponseSet(String headers, PassRefPtr<WebData> body, WebFrameProxy* frame, uint64_t listenerID);
    void receivedInterceptResponseIgnore(WebFrameProxy* frame, uint64_t listenerID);
#endif

    // WebPopupMenuProxy::Client
    virtual NativeWebMouseEvent* currentlyProcessedMouseDownEvent();

#if ENABLE(TIZEN_PAGE_VISIBILITY_API)
    void setPageVisibility(WebCore::PageVisibilityState, bool isInitialState);
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    bool scrollOverflow(const WebCore::FloatPoint&);
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SPLIT)
    WebCore::FloatPoint splitScrollOverflow(const WebCore::FloatPoint&);
#endif
    bool setPressedNodeAtPoint(const WebCore::IntPoint&, bool checkOverflowLayer, WebLayerID&);
    void setLoadingFinished(const bool isLoadingFinished) { m_isLoadingFinished = isLoadingFinished; }
    bool isLoadingFinished() const { return m_isLoadingFinished; }
    bool askOverflow(const WebCore::IntPoint&);
    void setOverflowResult(bool pressed, WebLayerID webLayerID);
#endif

#if ENABLE(TIZEN_ISF_PORT)
    void prepareKeyDownEvent();
    void deleteSurroundingText(int, int);

    void didCancelComposition();
    void didRequestUpdatingEditorState();
    void recalcFilterEvent(const EditorState&, bool, bool&);
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    bool makeContextCurrent();
#endif

#if PLATFORM(TIZEN)
    void didRenderFrame();
#endif

#if PLATFORM(GTK) && USE(TEXTURE_MAPPER_GL)
    void widgetMapped(uint64_t nativeWindowId);
#endif

    void setSuppressVisibilityUpdates(bool flag) { m_suppressVisibilityUpdates = flag; }
    bool suppressVisibilityUpdates() { return m_suppressVisibilityUpdates; }

#if ENABLE(TIZEN_INPUT_COLOR_PICKER) // wait for upstream
#if ENABLE(INPUT_TYPE_COLOR)
    void setColorChooserColor(const WebCore::Color&);
    void endColorChooser();
#endif
#endif

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    void valueChangedForPopupMenuMultiple(WebPopupMenuProxy*, Vector<int32_t> newSelectedIndex);
#endif

#if ENABLE(TIZEN_NATIVE_MEMORY_SNAPSHOT)
    void dumpMemorySnapshot();
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    void saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName);
    void saveSubresourcesData(Vector<WebSubresourceTizen> subresourceData);
    void startOfflinePageSave(String subresourceFolderName);
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    bool selectClosestWord(const WebCore::IntPoint&, bool, bool&);
    int setLeftSelection(const WebCore::IntPoint&, const int);
    int setRightSelection(const WebCore::IntPoint&, const int);
    bool getSelectionHandlers(WebCore::IntRect&, WebCore::IntRect&);
    String getSelectionText();
    void selectionRangeClear();
    bool scrollContentByCharacter(const WebCore::IntPoint&, WebCore::SelectionDirection, bool extend);
    bool scrollContentByLine(const WebCore::IntPoint&, WebCore::SelectionDirection);
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
    void getLinkMagnifierRect(const WebCore::IntPoint&);
    void openLink(const WebCore::IntPoint&);
#endif

#if ENABLE(TIZEN_SCREEN_READER)
    bool moveScreenReaderFocus(bool);
    void moveScreenReaderFocusByPoint(const WebCore::IntPoint&, bool);
    void clearScreenReaderFocus();
    void raiseTapEvent(const WebCore::IntPoint&);
    void adjustScreenReaderFocusedObjectValue(bool);
    void clearScreenReader();
    bool extendParagraphOnScreenReader(int direction);

    void didScreenReaderTextChanged(const String&);
    void didScreenReaderRectsChanged(const Vector<WebCore::IntRect>&);
    void readOutPasswords(bool& enabled);
    void didRaiseTapEvent(bool result, EditorState editorState);
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    void didFocusedRectsChanged(const Vector<WebCore::IntRect>&);
    void recalcFocusedRects();
    void clearFocusedNode();
#endif

#if ENABLE(TIZEN_CSP)
    void setContentSecurityPolicy(const String& policy, WebCore::ContentSecurityPolicy::HeaderType type);
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
    void replyApplicationCachePermission(bool allow);
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
    void replyExceededIndexedDatabaseQuota(bool allow);
#endif

#if ENABLE(TIZEN_SQL_DATABASE)
    void replyExceededDatabaseQuota(bool allow);
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
    void replyExceededLocalFileSystemQuota(bool allow);
#endif

#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
    void setTouchEventTargetRects(const Vector<WebCore::IntRect>&);
    bool isPointInTouchEventTargetArea(const WebCore::IntPoint&);
#endif

#if ENABLE(TIZEN_USE_SETTINGS_FONT)
    void useSettingsFont();
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    void mediaControlsRequestRotate(const String&);
#if ENABLE(APP_LOGGING_FOR_MEDIA)
    void videoStreamingCount();
#endif
#endif

#if ENABLE(TIZEN_GET_COOKIES_FOR_URL)
    void getCookiesForURL(const String&, String&);
#endif

#if ENABLE(TIZEN_PROD_DETECT_CONTENTS)
    void didDetectContents(const WebCore::IntPoint& tapPosition, const String& detectedContents, const WebCore::IntRect& detectedContentsRect, const WebCore::Color& highlightColor);
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    void showContextMenuForDetectedContentsTimerFired(WebCore::Timer<WebPageProxy>*);
    void showContextMenuForDetectedContents(const WebCore::IntPoint& tapPosition, const String& detectedContents);
#endif
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    bool focusUIEnabled() const { return m_focusUIEnabled; }
    void setFocusUIEnabled(bool);
    void suspendFocusUI();
    void resumeFocusUI();
#endif

#if ENABLE(TIZEN_BROWSER_FONT)
    void setBrowserFont();
#endif

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
    void saveHistoryItem();
    void backForwardItemChanged();
#endif

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    void resizeEventDone();
#endif

#if ENABLE(TIZEN_WEBKIT2_TOUCH_EVENT_TIMER)
    void processDelayedTouchEvent();
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SPLIT)
    bool splitScrollOverflowEnabled() const { return m_splitScrollOverflowEnabled; }
    void setSplitScrollOverflowEnabled(bool enable) { m_splitScrollOverflowEnabled = enable; }
#endif

#if ENABLE(TIZEN_INPUT_COLOR_PICKER) // wait for upstream
    bool isColorPickerActive() { return m_colorPickerResultListener.get() ? true : false; }
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    void setGrabMediaKey(bool grab);
#endif

private:
    WebPageProxy(PageClient*, PassRefPtr<WebProcessProxy>, WebPageGroup*, uint64_t pageID);

    virtual Type type() const { return APIType; }

    // WebPopupMenuProxy::Client
    virtual void valueChangedForPopupMenu(WebPopupMenuProxy*, int32_t newSelectedIndex);
    virtual void setTextFromItemForPopupMenu(WebPopupMenuProxy*, int32_t index);
#if PLATFORM(GTK)
    virtual void failedToShowPopupMenu();
#endif
#if PLATFORM(QT)
    virtual void changeSelectedIndex(int32_t newSelectedIndex);
    virtual void closePopupMenu();
#endif

    // Implemented in generated WebPageProxyMessageReceiver.cpp
    void didReceiveWebPageProxyMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    void didReceiveSyncWebPageProxyMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*, OwnPtr<CoreIPC::ArgumentEncoder>&);

    void didCreateMainFrame(uint64_t frameID);
    void didCreateSubframe(uint64_t frameID, uint64_t parentFrameID);
    void didSaveFrameToPageCache(uint64_t frameID);
    void didRestoreFrameFromPageCache(uint64_t frameID, uint64_t parentFrameID);

    void didStartProvisionalLoadForFrame(uint64_t frameID, const String& url, const String& unreachableURL, CoreIPC::ArgumentDecoder*);
    void didReceiveServerRedirectForProvisionalLoadForFrame(uint64_t frameID, const String&, CoreIPC::ArgumentDecoder*);
    void didFailProvisionalLoadForFrame(uint64_t frameID, const WebCore::ResourceError&, CoreIPC::ArgumentDecoder*);
    void didCommitLoadForFrame(uint64_t frameID, const String& mimeType, bool frameHasCustomRepresentation, const PlatformCertificateInfo&, CoreIPC::ArgumentDecoder*);
    void didFinishDocumentLoadForFrame(uint64_t frameID, CoreIPC::ArgumentDecoder*);
    void didFinishLoadForFrame(uint64_t frameID, CoreIPC::ArgumentDecoder*);
#if ENABLE(TIZEN_CSS_THEME_COLOR)
    void didChangeThemeColor(uint64_t frameID, const WebCore::Color&);
#endif
    void didFailLoadForFrame(uint64_t frameID, const WebCore::ResourceError&, CoreIPC::ArgumentDecoder*);
    void didSameDocumentNavigationForFrame(uint64_t frameID, uint32_t sameDocumentNavigationType, const String&, CoreIPC::ArgumentDecoder*);
    void didReceiveTitleForFrame(uint64_t frameID, const String&, CoreIPC::ArgumentDecoder*);
    void didFirstLayoutForFrame(uint64_t frameID, CoreIPC::ArgumentDecoder*);
    void didFirstVisuallyNonEmptyLayoutForFrame(uint64_t frameID, CoreIPC::ArgumentDecoder*);
    void didNewFirstVisuallyNonEmptyLayout(CoreIPC::ArgumentDecoder*);
    void didRemoveFrameFromHierarchy(uint64_t frameID, CoreIPC::ArgumentDecoder*);
    void didDisplayInsecureContentForFrame(uint64_t frameID, CoreIPC::ArgumentDecoder*);
    void didRunInsecureContentForFrame(uint64_t frameID, CoreIPC::ArgumentDecoder*);
    void didDetectXSSForFrame(uint64_t frameID, CoreIPC::ArgumentDecoder*);
    void frameDidBecomeFrameSet(uint64_t frameID, bool);
    void didStartProgress();
    void didChangeProgress(double);
    void didFinishProgress();

#if ENABLE(TIZEN_PLUGIN_CUSTOM_REQUEST)
    void processPluginCustomRequest(const String& request, const String& msg);
#endif

#if ENABLE(WEB_INTENTS)
    void didReceiveIntentForFrame(uint64_t frameID, const IntentData&, CoreIPC::ArgumentDecoder*);
#endif
#if ENABLE(WEB_INTENTS_TAG)
    void registerIntentServiceForFrame(uint64_t frameID, const IntentServiceInfo&, CoreIPC::ArgumentDecoder*);
#endif

#if ENABLE(TIZEN_DOWNLOAD_ATTRIBUTE)
    // Called by the web process through a message when the download need to be initiated from the web process.
    void startDownload(const WebCore::ResourceRequest&, const String& suggestedName = String());
#endif

     void decidePolicyForNavigationAction(uint64_t frameID, uint32_t navigationType, uint32_t modifiers, int32_t mouseButton, const WebCore::ResourceRequest&, uint64_t listenerID, CoreIPC::ArgumentDecoder*, bool& receivedPolicyAction, uint64_t& policyAction, uint64_t& downloadID);
    void decidePolicyForNewWindowAction(uint64_t frameID, uint32_t navigationType, uint32_t modifiers, int32_t mouseButton, const WebCore::ResourceRequest&, const String& frameName, uint64_t listenerID, CoreIPC::ArgumentDecoder*);
#if ENABLE(TIZEN_POLICY_FOR_NEW_WINDOW_ACTION_SYNC)
    void decidePolicyForNewWindowActionSync(uint64_t frameID, uint32_t navigationType, uint32_t modifiers, int32_t mouseButton, const WebCore::ResourceRequest&, const String& frameName, uint64_t listenerID, CoreIPC::ArgumentDecoder*, bool& receivedPolicyAction, uint64_t& policyAction, uint64_t& downloadID);
#endif
    void decidePolicyForResponse(uint64_t frameID, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, uint64_t listenerID, CoreIPC::ArgumentDecoder* arguments, bool& receivedPolicyAction, uint64_t& policyAction, uint64_t& downloadID);
    void unableToImplementPolicy(uint64_t frameID, const WebCore::ResourceError&, CoreIPC::ArgumentDecoder* arguments);

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    void willSendSubmitEvent(uint64_t frameID, uint64_t sourceFrameID, const WebFormData::Data& formData);
    void willSubmitForm(uint64_t frameID, uint64_t sourceFrameID, const WebFormData::Data& formData, uint64_t listenerID, CoreIPC::ArgumentDecoder*);
#else
    void willSubmitForm(uint64_t frameID, uint64_t sourceFrameID, const StringPairVector& textFieldValues, uint64_t listenerID, CoreIPC::ArgumentDecoder*);
#endif

    // Resource load client
#if ENABLE(TIZEN_INTERCEPT_REQUEST)
    void shouldInterceptRequest(uint64_t frameID, const WebCore::ResourceRequest& request, const String& body, uint64_t listenerID);
#endif
    void didInitiateLoadForResource(uint64_t frameID, uint64_t resourceIdentifier, const WebCore::ResourceRequest&, bool pageIsProvisionallyLoading);
    void didSendRequestForResource(uint64_t frameID, uint64_t resourceIdentifier, const WebCore::ResourceRequest&, const WebCore::ResourceResponse& redirectResponse);
    void didReceiveResponseForResource(uint64_t frameID, uint64_t resourceIdentifier, const WebCore::ResourceResponse&);
    void didReceiveContentLengthForResource(uint64_t frameID, uint64_t resourceIdentifier, uint64_t contentLength);
    void didFinishLoadForResource(uint64_t frameID, uint64_t resourceIdentifier);
    void didFailLoadForResource(uint64_t frameID, uint64_t resourceIdentifier, const WebCore::ResourceError&);

    // UI client
    void createNewPage(const WebCore::ResourceRequest&, const WebCore::WindowFeatures&, uint32_t modifiers, int32_t mouseButton, uint64_t& newPageID, WebPageCreationParameters&);
    void showPage();
    void closePage(bool stopResponsivenessTimer);
#if PLATFORM(TIZEN)
    void runJavaScriptAlert(uint64_t frameID, const String&, PassRefPtr<Messages::WebPageProxy::RunJavaScriptAlert::DelayedReply>);
    void runJavaScriptConfirm(uint64_t frameID, const String&, PassRefPtr<Messages::WebPageProxy::RunJavaScriptConfirm::DelayedReply>);
    void runJavaScriptPrompt(uint64_t frameID, const String&, const String&, PassRefPtr<Messages::WebPageProxy::RunJavaScriptPrompt::DelayedReply>);
#else
    void runJavaScriptAlert(uint64_t frameID, const String&);
    void runJavaScriptConfirm(uint64_t frameID, const String&, bool& result);
    void runJavaScriptPrompt(uint64_t frameID, const String&, const String&, String& result);
#endif
    void shouldInterruptJavaScript(bool& result);
    void setStatusText(const String&);
    void mouseDidMoveOverElement(const WebHitTestResult::Data& hitTestResultData, uint32_t modifiers, CoreIPC::ArgumentDecoder*);
    void unavailablePluginButtonClicked(uint32_t opaquePluginUnavailabilityReason, const String& mimeType, const String& url, const String& pluginsPageURL);
    void setToolbarsAreVisible(bool toolbarsAreVisible);
    void getToolbarsAreVisible(bool& toolbarsAreVisible);
    void setMenuBarIsVisible(bool menuBarIsVisible);
    void getMenuBarIsVisible(bool& menuBarIsVisible);
    void setStatusBarIsVisible(bool statusBarIsVisible);
    void getStatusBarIsVisible(bool& statusBarIsVisible);
    void setIsResizable(bool isResizable);
    void getIsResizable(bool& isResizable);
    void setWindowFrame(const WebCore::FloatRect&);
    void getWindowFrame(WebCore::FloatRect&);
    void screenToWindow(const WebCore::IntPoint& screenPoint, WebCore::IntPoint& windowPoint);
    void windowToScreen(const WebCore::IntRect& viewRect, WebCore::IntRect& result);
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
    void runBeforeUnloadConfirmPanel(const String& message, uint64_t frameID, PassRefPtr<Messages::WebPageProxy::RunBeforeUnloadConfirmPanel::DelayedReply>);
#else
    void runBeforeUnloadConfirmPanel(const String& message, uint64_t frameID, bool& shouldClose);
#endif
    void didChangeViewportProperties(const WebCore::ViewportAttributes&);
    void pageDidScroll();
    void runOpenPanel(uint64_t frameID, const WebCore::FileChooserSettings&);
    void printFrame(uint64_t frameID);
#if ENABLE(TIZEN_SQL_DATABASE)
    void exceededDatabaseQuota(uint64_t frameID, const String& originIdentifier, const String& displayName, uint64_t expectedUsage, PassRefPtr<Messages::WebPageProxy::ExceededDatabaseQuota::DelayedReply>);
#else
    void exceededDatabaseQuota(uint64_t frameID, const String& originIdentifier, const String& databaseName, const String& displayName, uint64_t currentQuota, uint64_t currentOriginUsage, uint64_t currentDatabaseUsage, uint64_t expectedUsage, uint64_t& newQuota);
#endif
#if ENABLE(TIZEN_APPLICATION_CACHE)
    void requestApplicationCachePermission(uint64_t frameID, const String& originIdentifier, PassRefPtr<Messages::WebPageProxy::RequestApplicationCachePermission::DelayedReply>);
#endif
    void requestGeolocationPermissionForFrame(uint64_t geolocationID, uint64_t frameID, String originIdentifier);
    void runModal();
    void notifyScrollerThumbIsVisibleInRect(const WebCore::IntRect&);
#if HAVE(ACCESSIBILITY)
    void showHighlight(const WebCore::IntRect&);
    void clearHighlight();
#endif
    void recommendedScrollbarStyleDidChange(int32_t newStyle);
    void didChangeScrollbarsForMainFrame(bool hasHorizontalScrollbar, bool hasVerticalScrollbar);
    void didChangeScrollOffsetPinningForMainFrame(bool pinnedToLeftSide, bool pinnedToRightSide);
#if PLATFORM(EFL)
    void didChangeScrollPositionForMainFrame(const WebCore::IntPoint&);
#endif
    void didChangePageCount(unsigned);
    void didFailToInitializePlugin(const String& mimeType);
    void didBlockInsecurePluginVersion(const String& mimeType, const String& urlString);
    void setCanShortCircuitHorizontalWheelEvents(bool canShortCircuitHorizontalWheelEvents) { m_canShortCircuitHorizontalWheelEvents = canShortCircuitHorizontalWheelEvents; }

    void reattachToWebProcess();
    void reattachToWebProcessWithItem(WebBackForwardListItem*);

    void requestNotificationPermission(uint64_t notificationID, const String& originString);

    void showNotification(const String& title, const String& body, const String& iconURL, const String& tag, const String& originString, uint64_t notificationID);

#if ENABLE(TIZEN_MEDIA_STREAM)
    void requestUserMediaPermission(uint64_t userMediaID, const String& originString);
#endif

#if USE(TILED_BACKING_STORE)
    void pageDidRequestScroll(const WebCore::IntPoint&);
#endif

#if ENABLE(TIZEN_WEBKIT2_HISTORICAL_RESTORE_VISIBLE_CONTENT_RECT)
    void pageDidRequestRestoreVisibleContentRect(const WebCore::IntPoint&, float);
#endif

#if PLATFORM(QT) || PLATFORM(TIZEN)
    void didChangeContentsSize(const WebCore::IntSize&);
    void didFindZoomableArea(const WebCore::IntPoint&, const WebCore::IntRect&);
#endif
#if ENABLE(TOUCH_EVENTS)
    void needTouchEvents(bool);
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    void didGetWebAppCapable(const bool capable, uint64_t callbackID);
    void didGetWebAppIconURL(const String& iconURL, uint64_t callbackID);
    void didGetWebAppIconURLs(const StringPairVector& iconURLs, uint64_t callbackID);
#endif
#if ENABLE(TIZEN_ICON_DATABASE)
    void didReceiveIcon();
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    void showColorChooser(const WebCore::Color& initialColor);
#if !ENABLE(TIZEN_INPUT_COLOR_PICKER) // wait for upstream
    void setColorChooserColor(const WebCore::Color&);
    void endColorChooser();
#endif
    void didChooseColor(const WebCore::Color&);
    void didEndColorChooser();
#endif

    void editorStateChanged(const EditorState&);
    // Back/Forward list management
    void backForwardAddItem(uint64_t itemID);
    void backForwardGoToItem(uint64_t itemID, SandboxExtension::Handle&);
    void backForwardItemAtIndex(int32_t index, uint64_t& itemID);
    void backForwardBackListCount(int32_t& count);
    void backForwardForwardListCount(int32_t& count);
    void backForwardClear();

    // Undo management
    void registerEditCommandForUndo(uint64_t commandID, uint32_t editAction);
    void clearAllEditCommands();
    void canUndoRedo(uint32_t action, bool& result);
    void executeUndoRedo(uint32_t action, bool& result);

    // Keyboard handling
#if PLATFORM(MAC)
    void interpretQueuedKeyEvent(const EditorState&, bool& handled, Vector<WebCore::KeypressCommand>&);
    void executeSavedCommandBySelector(const String& selector, bool& handled);
#endif

#if PLATFORM(GTK)
    void getEditorCommandsForKeyEvent(const AtomicString&, Vector<String>&);
    void bindAccessibilityTree(const String&);
#endif
#if PLATFORM(EFL)
    void getEditorCommandsForKeyEvent(Vector<String>&);
#endif

#if PLATFORM(EFL) && HAVE(ACCESSIBILITY)
    void bindAccessibilityTree(const String&);
#endif

    // Popup Menu.
    void showPopupMenu(const WebCore::IntRect& rect, uint64_t textDirection, const Vector<WebPopupItem>& items, int32_t selectedIndex, const PlatformPopupMenuData&);
    void hidePopupMenu();
#if PLATFORM(WIN)
    void setPopupMenuSelectedIndex(int32_t);
#endif
#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
    void updatePopupMenu(uint64_t textDirection, const Vector<WebPopupItem>& items, int32_t selectedIndex);
#endif

#if ENABLE(CONTEXT_MENUS)
    // Context Menu.
    void showContextMenu(const WebCore::IntPoint& menuLocation, const WebHitTestResult::Data&, const Vector<WebContextMenuItemData>&, CoreIPC::ArgumentDecoder*);
    void internalShowContextMenu(const WebCore::IntPoint& menuLocation, const WebHitTestResult::Data&, const Vector<WebContextMenuItemData>&, CoreIPC::ArgumentDecoder*);
#endif

    // Search popup results
    void saveRecentSearches(const String&, const Vector<String>&);
    void loadRecentSearches(const String&, Vector<String>&);

#if PLATFORM(MAC)
    // Speech.
    void getIsSpeaking(bool&);
    void speak(const String&);
    void stopSpeaking();

    // Spotlight.
    void searchWithSpotlight(const String&);

    // Dictionary.
    void didPerformDictionaryLookup(const String&, const DictionaryPopupInfo&);
#endif

    // Spelling and grammar.
    int64_t spellDocumentTag();
#if USE(UNIFIED_TEXT_CHECKING)
    void checkTextOfParagraph(const String& text, uint64_t checkingTypes, Vector<WebCore::TextCheckingResult>& results);
#endif
    void checkSpellingOfString(const String& text, int32_t& misspellingLocation, int32_t& misspellingLength);
    void checkGrammarOfString(const String& text, Vector<WebCore::GrammarDetail>&, int32_t& badGrammarLocation, int32_t& badGrammarLength);
    void spellingUIIsShowing(bool&);
    void updateSpellingUIWithMisspelledWord(const String& misspelledWord);
    void updateSpellingUIWithGrammarString(const String& badGrammarPhrase, const WebCore::GrammarDetail&);
    void getGuessesForWord(const String& word, const String& context, Vector<String>& guesses);
#if PLATFORM(EFL)
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    void textChangeInTextField(const String&, const String&, bool isInputInForm, bool isForcePopupShow, bool isHandleEvent);
#else
    void textChangeInTextField(const String&, const String&);
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif
#endif
    void learnWord(const String& word);
    void ignoreWord(const String& word);

    void setFocus(bool focused);
#if ENABLE(TIZEN_FOCUS_UI)
    void canTakeFocus(uint32_t direction, bool& isPossible);
#endif
    void takeFocus(uint32_t direction);
    void setToolTip(const String&);
    void setCursor(const WebCore::Cursor&);
    void setCursorHiddenUntilMouseMoves(bool);

    void didReceiveEvent(uint32_t opaqueType, bool handled);
    void stopResponsivenessTimer();

    void voidCallback(uint64_t);
    void dataCallback(const CoreIPC::DataReference&, uint64_t);
    void stringCallback(const String&, uint64_t);
    void scriptValueCallback(const CoreIPC::DataReference&, uint64_t);
    void computedPagesCallback(const Vector<WebCore::IntRect>&, double totalScaleFactorForPrinting, uint64_t);
    void validateCommandCallback(const String&, bool, int, uint64_t);
#if PLATFORM(GTK)
    void printFinishedCallback(const WebCore::ResourceError&, uint64_t);
#endif

    void focusedFrameChanged(uint64_t frameID);
    void frameSetLargestFrameChanged(uint64_t frameID);

    void canAuthenticateAgainstProtectionSpaceInFrame(uint64_t frameID, const WebCore::ProtectionSpace&, bool& canAuthenticate);
#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    void didReceiveAuthenticationChallenge(uint64_t frameID, const WebCore::AuthenticationChallenge&, uint64_t challengeID, PassRefPtr<Messages::WebPageProxy::DidReceiveAuthenticationChallenge::DelayedReply>);
#else
    void didReceiveAuthenticationChallenge(uint64_t frameID, const WebCore::AuthenticationChallenge&, uint64_t challengeID);
#endif

    void didFinishLoadingDataForCustomRepresentation(const String& suggestedFilename, const CoreIPC::DataReference&);

#if PLATFORM(MAC)
    void pluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus);
    void setPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, uint64_t complexTextInputState);
#endif

    void clearPendingAPIRequestURL() { m_pendingAPIRequestURL = String(); }
    void setPendingAPIRequestURL(const String& pendingAPIRequestURL) { m_pendingAPIRequestURL = pendingAPIRequestURL; }

    bool maybeInitializeSandboxExtensionHandle(const WebCore::KURL&, SandboxExtension::Handle&);

#if PLATFORM(MAC)
    void substitutionsPanelIsShowing(bool&);
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
    void showCorrectionPanel(int32_t panelType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings);
    void dismissCorrectionPanel(int32_t reason);
    void dismissCorrectionPanelSoon(int32_t reason, String& result);
    void recordAutocorrectionResponse(int32_t responseType, const String& replacedString, const String& replacementString);
#endif // __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070

#if USE(DICTATION_ALTERNATIVES)
    void showDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext);
    void dismissDictationAlternativeUI();
    void removeDictationAlternatives(uint64_t dictationContext);
    void dictationAlternatives(uint64_t dictationContext, Vector<String>& result);
#endif
#endif // PLATFORM(MAC)

#if USE(SOUP)
    void didReceiveURIRequest(String uriString, uint64_t requestID);
#endif

    void clearLoadDependentCallbacks();

    void performDragControllerAction(DragControllerAction, WebCore::DragData*, const String& dragStorageName, const SandboxExtension::Handle&, const SandboxExtension::HandleArray&);

    void updateBackingStoreDiscardableState();

#if PLATFORM(WIN)
    void scheduleChildWindowGeometryUpdate(const WindowGeometry&);
#endif

    void setRenderTreeSize(uint64_t treeSize) { m_renderTreeSize = treeSize; }

#if PLUGIN_ARCHITECTURE(X11)
    void createPluginContainer(uint64_t& windowID);
    void windowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID);
#endif

    void processNextQueuedWheelEvent();
    void sendWheelEvent(const WebWheelEvent&);

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    void decidePolicyForCertificateError(const String& url, const String& certificate, int error, PassRefPtr<Messages::WebPageProxy::DecidePolicyForCertificateError::DelayedReply>);
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    void blockingToLoadForMalwareScan(const WebCore::ResourceError& error, PassRefPtr<Messages::WebPageProxy::BlockingToLoadForMalwareScan::DelayedReply>);
#endif

#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    bool isWaitingForJavaScriptPopupReply();
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
    void didGetLinkMagnifierRect(const WebCore::IntPoint&, const WebCore::IntRect&);
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
    void exceededIndexedDatabaseQuota(uint64_t frameID, const String& originIdentifier, int64_t currentUsage, PassRefPtr<Messages::WebPageProxy::ExceededIndexedDatabaseQuota::DelayedReply> reply);
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
    void exceededLocalFileSystemQuota(uint64_t frameID, const String& originIdentifier, int64_t currentUsage, PassRefPtr<Messages::WebPageProxy::ExceededLocalFileSystemQuota::DelayedReply> reply);
#endif

#if ENABLE(TIZEN_WEBKIT2_NOTIFY_SUSPEND_BY_REMOTE_WEB_INSPECTOR)
    void setContentSuspendedByInspector(bool);
#endif

#if ENABLE(TIZEN_POPUP_BLOCKED_NOTIFICATION)
    void popupBlocked(const String& urlString);
#endif

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
    void addMessageToConsole(uint32_t level, const String& message, uint32_t lineNumber, const String& source);
#endif

#if ENABLE(TIZEN_WEBKIT2_TOUCH_EVENT_TIMER)
    void updateTouchEventTimerFired(WebCore::Timer<WebPageProxy>*);
#endif

    PageClient* m_pageClient;
    WebLoaderClient m_loaderClient;
    WebPolicyClient m_policyClient;
    WebFormClient m_formClient;
    WebResourceLoadClient m_resourceLoadClient;
    WebUIClient m_uiClient;
    WebFindClient m_findClient;
    WebFindMatchesClient m_findMatchesClient;
#if ENABLE(CONTEXT_MENUS)
    WebPageContextMenuClient m_contextMenuClient;
#endif
#if PLATFORM(TIZEN)
    WebTizenClient m_tizenClient;
#endif

    OwnPtr<DrawingAreaProxy> m_drawingArea;
    RefPtr<WebProcessProxy> m_process;
    RefPtr<WebPageGroup> m_pageGroup;
    RefPtr<WebFrameProxy> m_mainFrame;
    RefPtr<WebFrameProxy> m_focusedFrame;
    RefPtr<WebFrameProxy> m_frameSetLargestFrame;

    String m_userAgent;
    String m_applicationNameForUserAgent;
    String m_customUserAgent;
    String m_customTextEncodingName;

#if ENABLE(TIZEN_USER_AGENT_WHITELIST)
    bool m_customUserAgentSet;
#endif

#if ENABLE(INSPECTOR)
    RefPtr<WebInspectorProxy> m_inspector;
#endif

#if ENABLE(FULLSCREEN_API)
    RefPtr<WebFullScreenManagerProxy> m_fullScreenManager;
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    HashMap<uint64_t, RefPtr<BooleanCallback> > m_booleanCallbacks;
    HashMap<uint64_t, RefPtr<DictionaryCallback> > m_dictionaryCallbacks;
#endif
    HashMap<uint64_t, RefPtr<VoidCallback> > m_voidCallbacks;
    HashMap<uint64_t, RefPtr<DataCallback> > m_dataCallbacks;
    HashMap<uint64_t, RefPtr<StringCallback> > m_stringCallbacks;
    HashSet<uint64_t> m_loadDependentStringCallbackIDs;
    HashMap<uint64_t, RefPtr<ScriptValueCallback> > m_scriptValueCallbacks;
    HashMap<uint64_t, RefPtr<ComputedPagesCallback> > m_computedPagesCallbacks;
    HashMap<uint64_t, RefPtr<ValidateCommandCallback> > m_validateCommandCallbacks;
#if PLATFORM(GTK)
    HashMap<uint64_t, RefPtr<PrintFinishedCallback> > m_printFinishedCallbacks;
#endif
#if ENABLE(TIZEN_WEB_STORAGE)
    HashMap<uint64_t, RefPtr<WebStorageQuotaCallback> > m_quotaCallbacks;
#endif

    HashSet<WebEditCommandProxy*> m_editCommandSet;

#if PLATFORM(MAC)
    HashSet<String> m_knownKeypressCommandNames;
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    bool m_formIsNavigating;
#endif
    RefPtr<WebPopupMenuProxy> m_activePopupMenu;
    RefPtr<WebContextMenuProxy> m_activeContextMenu;
    WebHitTestResult::Data m_activeContextMenuHitTestResultData;
    RefPtr<WebOpenPanelResultListenerProxy> m_openPanelResultListener;
    GeolocationPermissionRequestManagerProxy m_geolocationPermissionRequestManager;
    NotificationPermissionRequestManagerProxy m_notificationPermissionRequestManager;
#if ENABLE(TIZEN_MEDIA_STREAM)
    UserMediaPermissionRequestManagerProxy m_userMediaPermissionRequestManager;
#endif
    double m_estimatedProgress;

    // Whether the web page is contained in a top-level window.
    bool m_isInWindow;

    // Whether the page is visible; if the backing view is visible and inserted into a window.
    bool m_isVisible;

    bool m_canGoBack;
    bool m_canGoForward;
    RefPtr<WebBackForwardList> m_backForwardList;
    
    bool m_maintainsInactiveSelection;

    String m_toolTip;

    String m_urlAtProcessExit;
    WebFrameProxy::LoadState m_loadStateAtProcessExit;

    EditorState m_editorState;

    double m_textZoomFactor;
    double m_pageZoomFactor;
    double m_pageScaleFactor;
    float m_intrinsicDeviceScaleFactor;
    float m_customDeviceScaleFactor;

    LayerHostingMode m_layerHostingMode;

    bool m_drawsBackground;
    bool m_drawsTransparentBackground;

    bool m_areMemoryCacheClientCallsEnabled;

    bool m_useFixedLayout;
    WebCore::IntSize m_fixedLayoutSize;

    WebCore::Page::Pagination::Mode m_paginationMode;
    bool m_paginationBehavesLikeColumns;
    double m_pageLength;
    double m_gapBetweenPages;

    // If the process backing the web page is alive and kicking.
    bool m_isValid;

    // Whether WebPageProxy::close() has been called on this page.
    bool m_isClosed;

    // Whether it can run modal child web pages.
    bool m_canRunModal;

    bool m_isInPrintingMode;
    bool m_isPerformingDOMPrintOperation;

    bool m_inDecidePolicyForResponse;
    bool m_syncMimeTypePolicyActionIsValid;
    WebCore::PolicyAction m_syncMimeTypePolicyAction;
    uint64_t m_syncMimeTypePolicyDownloadID;

    bool m_inDecidePolicyForNavigationAction;
    bool m_syncNavigationActionPolicyActionIsValid;
    WebCore::PolicyAction m_syncNavigationActionPolicyAction;
    uint64_t m_syncNavigationActionPolicyDownloadID;

#if ENABLE(TIZEN_POLICY_FOR_NEW_WINDOW_ACTION_SYNC)
    bool m_inDecidePolicyForNewWindowActionSync;
    bool m_syncNewWindowActionSyncPolicyActionIsValid;
    WebCore::PolicyAction m_syncNewWindowActionSyncPolicyAction;
    uint64_t m_syncNewWindowActionSyncPolicyDownloadID;
#endif

#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
#if ENABLE(GESTURE_EVENTS)
    Deque<QueuedUIEvents<WebGestureEvent> > m_gestureEventQueue;
#endif
    Deque<QueuedUIEvents<NativeWebKeyboardEvent> > m_keyEventQueue;
#else
#if ENABLE(GESTURE_EVENTS)
    Deque<WebGestureEvent> m_gestureEventQueue;
#endif
    Deque<NativeWebKeyboardEvent> m_keyEventQueue;
#endif
    Deque<NativeWebWheelEvent> m_wheelEventQueue;
    Deque<OwnPtr<Vector<NativeWebWheelEvent> > > m_currentlyProcessedWheelEvents;

    bool m_processingMouseMoveEvent;
    OwnPtr<NativeWebMouseEvent> m_nextMouseMoveEvent;
    OwnPtr<NativeWebMouseEvent> m_currentlyProcessedMouseDownEvent;

#if ENABLE(TOUCH_EVENTS)
    bool m_needTouchEvents;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    Deque<QueuedUIEvents<NativeWebTouchEvent> > m_touchEventQueue;
#else
    Deque<QueuedTouchEvents> m_touchEventQueue;
#endif
#endif
#if ENABLE(INPUT_TYPE_COLOR)
    RefPtr<WebColorChooserProxy> m_colorChooser;
#if ENABLE(TIZEN_INPUT_COLOR_PICKER) // wait for upstream
    RefPtr<WebColorPickerResultListenerProxy> m_colorPickerResultListener;
#endif
#endif
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    bool m_isLoadingFinished;    //if the page loading is finished, then set to true.
#endif

    uint64_t m_pageID;

    bool m_isPageSuspended;

#if PLATFORM(MAC)
    bool m_isSmartInsertDeleteEnabled;
#endif

#if HAVE(ACCESSIBILITY) && PLATFORM(EFL)
    WKEinaSharedString m_accessibilityPlugID;
#endif

    int64_t m_spellDocumentTag;
    bool m_hasSpellDocumentTag;
    unsigned m_pendingLearnOrIgnoreWordMessageCount;

    bool m_mainFrameHasCustomRepresentation;

#if ENABLE(DRAG_SUPPORT)
    WebCore::DragSession m_currentDragSession;
#endif

    String m_pendingAPIRequestURL;

    bool m_mainFrameHasHorizontalScrollbar;
    bool m_mainFrameHasVerticalScrollbar;

    // Whether horizontal wheel events can be handled directly for swiping purposes.
    bool m_canShortCircuitHorizontalWheelEvents;

    bool m_mainFrameIsPinnedToLeftSide;
    bool m_mainFrameIsPinnedToRightSide;

#if PLATFORM(EFL)
    WebCore::IntPoint m_scrollPosition;
    WebCore::IntSize m_contentsSize;
#endif
    unsigned m_pageCount;

    WebCore::IntRect m_visibleScrollerThumbRect;

    uint64_t m_renderTreeSize;

    static WKPageDebugPaintFlags s_debugPaintFlags;

    bool m_shouldSendEventsSynchronously;

    bool m_suppressVisibilityUpdates;

    float m_mediaVolume;

#if PLATFORM(QT)
    WTF::HashSet<RefPtr<QtRefCountedNetworkRequestData> > m_applicationSchemeRequests;
#endif

#if ENABLE(PAGE_VISIBILITY_API)
    WebCore::PageVisibilityState m_visibilityState;
#endif

#if PLATFORM(TIZEN)
    RefPtr<Messages::WebPageProxy::RunJavaScriptAlert::DelayedReply> m_alertReply;
#if ENABLE(TIZEN_JAVASCRIPT_POPUP_REPLY_HANDLING)
    Deque<RefPtr<Messages::WebPageProxy::RunJavaScriptConfirm::DelayedReply> > m_confirmReply;
#else
    RefPtr<Messages::WebPageProxy::RunJavaScriptConfirm::DelayedReply> m_confirmReply;
#endif
    RefPtr<Messages::WebPageProxy::RunJavaScriptPrompt::DelayedReply> m_promptReply;
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
    RefPtr<Messages::WebPageProxy::RunBeforeUnloadConfirmPanel::DelayedReply> m_beforeUnloadConfirmPanelReply;
#endif
#if ENABLE(TIZEN_APPLICATION_CACHE)
    RefPtr<Messages::WebPageProxy::RequestApplicationCachePermission::DelayedReply> m_applicationCacheReply;
#endif
#if ENABLE(TIZEN_SQL_DATABASE)
    RefPtr<Messages::WebPageProxy::ExceededDatabaseQuota::DelayedReply> m_exceededDatabaseQuotaReply;
#endif
#endif

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    RefPtr<Messages::WebPageProxy::DidReceiveAuthenticationChallenge::DelayedReply> m_AuthReply;
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    RefPtr<Messages::WebPageProxy::DecidePolicyForCertificateError::DelayedReply> m_allowedReply;
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    RefPtr<Messages::WebPageProxy::BlockingToLoadForMalwareScan::DelayedReply> m_releaseBlockReply;
#endif

#if ENABLE(TIZEN_ISF_PORT)
    bool m_didCancelCompositionFromWebProcess;
#endif

#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
    Ecore_Timer* m_updateOnlyVisibleAreaStateTimer;
    bool m_updateOnlyVisibleAreaState;
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
    RefPtr<Messages::WebPageProxy::ExceededIndexedDatabaseQuota::DelayedReply> m_exceededIndexedDatabaseQuotaReply;
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
    RefPtr<Messages::WebPageProxy::ExceededLocalFileSystemQuota::DelayedReply> m_exceededLocalFileSystemQuotaReply;
#endif

#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
    Vector<WebCore::IntRect> m_touchEventTargetRects;
#endif

#if ENABLE(TIZEN_WEBKIT2_NOTIFY_SUSPEND_BY_REMOTE_WEB_INSPECTOR)
    bool m_contentSuspendedByInspector;
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    bool m_focusUIEnabled;
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SPLIT)
    bool m_splitScrollOverflowEnabled;
#endif

#if ENABLE(TIZEN_WEBKIT2_TOUCH_EVENT_TIMER)
    bool m_touchEventFired;
    unsigned m_untreatedtouchEventCount;
    unsigned m_preprocessedTouchEventCount;
    WebCore::Timer<WebPageProxy> m_touchEventTimer;
#endif

#if ENABLE(TIZEN_PROD_DETECT_CONTENTS)
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    WebCore::Timer<WebPageProxy> m_showContextMenuForDetectedContentsTimer;
#endif
    WebCore::IntPoint m_tapPositionForDetectedContents;
    String m_detectedContents;
#endif
};

} // namespace WebKit

#endif // WebPageProxy_h
