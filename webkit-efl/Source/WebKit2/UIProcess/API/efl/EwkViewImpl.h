/*
   Copyright (C) 2011 Samsung Electronics
   Copyright (C) 2012 Intel Corporation. All rights reserved.

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

#ifndef EwkViewImpl_h
#define EwkViewImpl_h

#include "RefPtrEfl.h"
#include "WKEinaSharedString.h"
#include "WKRetainPtr.h"
#include <Evas.h>
#include <WebCore/TextDirection.h>
#include <WebCore/Timer.h>
#include <WebKit2/WKBase.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

#if ENABLE(TOUCH_EVENTS)
#include "ewk_touch.h"
#endif

#if USE(ACCELERATED_COMPOSITING)
#include <Evas_GL.h>
#endif

#if PLATFORM(TIZEN)
#include "JavaScriptPopup.h"
#include "OpenMediaPlayer.h"
#include "PermissionPopupManager.h"
#include "ewk_auth_challenge_private.h"
#include <JavaScriptCore/JSRetainPtr.h>
#include <WebCore/IntPoint.h>

#if ENABLE(TIZEN_OPEN_PANEL)
#include "OpenPanel.h"
#endif

#if ENABLE(TIZEN_GESTURE)
#include "GestureRecognizer.h"
#include "GestureClient.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
#include "InputFieldZoom.h"
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
#include "InputPicker.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
#include "FocusRing.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
#include "ewk_popup_picker.h"
#endif

#if ENABLE(TIZEN_EDGE_SUPPORT)
#include "EdgeEffect.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
#include "TextSelection.h"
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "NotificationManagerEfl.h"
#endif
#endif // #if PLATFORM(TIZEN)

#define EWK_VIEW_IMPL_GET(smartData, impl)                                     \
    EwkViewImpl* impl = smartData->priv

#define EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, ...)                      \
    if (!smartData) {                                                          \
        EINA_LOG_ERR("smart data is null");                                    \
        return __VA_ARGS__;                                                    \
    }                                                                          \
    EWK_VIEW_IMPL_GET(smartData, impl);                                        \
    do {                                                                       \
        if (!impl) {                                                           \
            EINA_LOG_ERR("no private data for object %p (%s)",                 \
                smartData->self, evas_object_type_get(smartData->self));       \
            return __VA_ARGS__;                                                \
        }                                                                      \
    } while (0)

namespace WebKit {
class FindClientEfl;
class FormClientEfl;
class InputMethodContextEfl;
class PageClientImpl;
class PageLoadClientEfl;
class PagePolicyClientEfl;
class PageUIClientEfl;
class PageViewportControllerClientEfl;
class PageViewportController;
class ResourceLoadClientEfl;
class WebPageProxy;
class WebPopupItem;
class WebPopupMenuProxyEfl;
class NotificationManagerEfl;
}

namespace WebCore {
class Color;
class Cursor;
class IntRect;
class IntSize;
}

class Ewk_Back_Forward_List;
class Ewk_Context;
class Ewk_Download_Job;
class Ewk_Error;
class Ewk_Form_Submission_Request;
class Ewk_Intent;
class Ewk_Intent_Service;
class Ewk_Navigation_Policy_Decision;
class Ewk_Resource;
class Ewk_Settings;
class Ewk_Url_Request;
class Ewk_Url_Response;

typedef struct Ewk_View_Smart_Data Ewk_View_Smart_Data;

#if PLATFORM(TIZEN)
namespace WebCore {
class AffineTransform;
}

// FIXME: we have to include ewk_view.h instead of typedef,
// because there are "circular include" in the local code unlike open source,
// so we can not do typedef again here.
//typedef struct Ewk_View_Smart_Data Ewk_View_Smart_Data;
#include "ewk_view.h"

struct Ewk_View_Callback_Context {
    union {
        Ewk_Web_App_Capable_Get_Callback webAppCapableCallback;
        Ewk_Web_App_Icon_URL_Get_Callback webAppIconURLCallback;
        Ewk_Web_App_Icon_URLs_Get_Callback webAppIconURLsCallback;
#if ENABLE(TIZEN_WEB_STORAGE) && ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
        Ewk_Web_Storage_Quota_Get_Callback webStorageQuotaCallback;
#endif
        Ewk_View_Script_Execute_Callback scriptExecuteCallback;
        Ewk_View_Plain_Text_Get_Callback plainTextGetCallback;
#if ENABLE(TIZEN_SUPPORT_MHTML)
        Ewk_View_MHTML_Data_Get_Callback mhtmlDataGetCallback;
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
        Ewk_View_Password_Confirm_Popup_Callback passwordConfirmPopupCallback;
#endif
        Ewk_View_JavaScript_Alert_Callback javascriptAlertCallback;
        Ewk_View_JavaScript_Confirm_Callback javascriptConfirmCallback;
        Ewk_View_JavaScript_Prompt_Callback javascriptPromptCallback;
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
        Ewk_View_Before_Unload_Confirm_Panel_Callback beforeUnloadConfirmPanelCallback;
#endif
        Ewk_View_Open_Panel_Callback openPanelCallback;
#if ENABLE(TIZEN_APPLICATION_CACHE)
        Ewk_View_Applicacion_Cache_Permission_Callback applicationCachePermissionCallback;
#endif
#if ENABLE(TIZEN_INDEXED_DATABASE)
        Ewk_View_Exceeded_Indexed_Database_Quota_Callback exceededIndexedDatabaseQuotaCallback;
#endif
#if ENABLE(TIZEN_SQL_DATABASE)
        Ewk_View_Exceeded_Database_Quota_Callback exceededDatabaseQuotaCallback;
#endif
#if ENABLE(TIZEN_FILE_SYSTEM)
        Ewk_View_Exceeded_Local_File_System_Quota_Callback exceededLocalFileSystemQuotaCallback;
#endif
#if ENABLE(TIZEN_INTERCEPT_REQUEST)
        Ewk_View_Intercept_Request_Callback interceptRequestCallback;
#endif
#if ENABLE(TIZEN_FOCUS_UI)
        Ewk_View_Unfocus_Allow_Callback unfocusAllowCallback;
#endif
#if ENABLE(TIZEN_GEOLOCATION)
        Ewk_View_Geolocation_Permission_Callback geolocationPermissionCallback;
#endif
#if ENABLE(TIZEN_NOTIFICATIONS)
        Ewk_View_Notification_Permission_Callback notificationPermissionCallback;
        Ewk_Notification_Show_Callback notificationShowCallback;
        Ewk_Notification_Cancel_Callback notificationCancelCallback;
#endif
#if ENABLE(TIZEN_MEDIA_STREAM)
        Ewk_View_User_Media_Permission_Callback userMediaPermissionCallback;
#endif
    };

    Evas_Object* ewkView;
    void* userData;
};
typedef struct Ewk_View_Callback_Context Ewk_View_Callback_Context;
#endif

class EwkViewImpl {
public:
    explicit EwkViewImpl(Evas_Object* view);
    ~EwkViewImpl();

    static EwkViewImpl* fromEvasObject(const Evas_Object* view);

    inline Evas_Object* view() { return m_view; }
    WKPageRef wkPage();
    inline WebKit::WebPageProxy* page() { return pageProxy.get(); }
    Ewk_Context* ewkContext() { return context.get(); }
    Ewk_Settings* settings() { return m_settings.get(); }

    WebCore::IntSize size() const;
    bool isFocused() const;
    bool isVisible() const;

    const char* url() const { return m_url; }
    const char* faviconURL() const { return m_faviconURL; }
    const char* title() const;
    WebKit::InputMethodContextEfl* inputMethodContext();

    const char* themePath() const;
    void setThemePath(const char* theme);
    const char* customTextEncodingName() const;
    void setCustomTextEncodingName(const char* encoding);

    bool mouseEventsEnabled() const { return m_mouseEventsEnabled; }
    void setMouseEventsEnabled(bool enabled);
#if ENABLE(TOUCH_EVENTS)
    bool touchEventsEnabled() const { return m_touchEventsEnabled; }
    void setTouchEventsEnabled(bool enabled);
#endif

    void setCursor(const WebCore::Cursor& cursor);
    void redrawRegion(const WebCore::IntRect& rect);
    void setImageData(void* imageData, const WebCore::IntSize& size);

#if ENABLE(INPUT_TYPE_COLOR)
    bool setColorPickerColor(const WebCore::Color& color);
#endif

    static void addToPageViewMap(const Evas_Object* ewkView);
    static void removeFromPageViewMap(const Evas_Object* ewkView);
    static const Evas_Object* viewFromPageViewMap(const WKPageRef);
    static const Evas_Object* visibleViewFromPageViewMap();

#if ENABLE(TIZEN_GSTREAMER_VIDEO) && ENABLE(APP_LOGGING_FOR_MEDIA)
    void videoStreamingCount();
#endif

#if ENABLE(FULLSCREEN_API)
    void enterFullScreen();
    void exitFullScreen();
#if ENABLE(TIZEN_FULLSCREEN_API)
    void restoreScaleFactorAfterExitingFullScreen(float scale);
#endif
#endif

#if USE(ACCELERATED_COMPOSITING)
    bool createGLSurface(const WebCore::IntSize& viewSize);
    bool enterAcceleratedCompositingMode();
    bool exitAcceleratedCompositingMode();
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    void requestColorPicker(int r, int g, int b, int a, WKColorPickerResultListenerRef listener);
    void dismissColorPicker();
#endif

    WKPageRef createNewPage();
    void closePage();

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    void requestPopupMenu(WebKit::WebPopupMenuProxyEfl*, const WebCore::IntRect&, WebCore::TextDirection, double pageScaleFactor, const Vector<WebKit::WebPopupItem>& items, int32_t selectedIndex, bool);
#else
    void requestPopupMenu(WebKit::WebPopupMenuProxyEfl*, const WebCore::IntRect&, WebCore::TextDirection, double pageScaleFactor, const Vector<WebKit::WebPopupItem>& items, int32_t selectedIndex);
#endif
    void updateTextInputState();

    void requestJSAlertPopup(const WKEinaSharedString& message);
    bool requestJSConfirmPopup(const WKEinaSharedString& message);
    WKEinaSharedString requestJSPromptPopup(const WKEinaSharedString& message, const WKEinaSharedString& defaultValue);

    void informDownloadJobCancelled(Ewk_Download_Job* download);
    void informDownloadJobFailed(Ewk_Download_Job* download, Ewk_Error* error);
    void informDownloadJobFinished(Ewk_Download_Job* download);
    void informDownloadJobRequested(Ewk_Download_Job* download);

    void informNewFormSubmissionRequest(Ewk_Form_Submission_Request* request);
    void informLoadError(Ewk_Error* error);
    void informLoadFinished();
#if ENABLE(TIZEN_CSS_THEME_COLOR)
    void informThemeColor(const WebCore::Color&);
#endif
    void informLoadProgress(double progress);
    void informProvisionalLoadFailed(Ewk_Error* error);
#if USE(TILED_BACKING_STORE)
    void informLoadCommitted();
#endif
    void informProvisionalLoadRedirect();
    void informProvisionalLoadStarted();

    void informResourceLoadStarted(Ewk_Resource* resource, Ewk_Url_Request* request);
    void informResourceLoadResponse(Ewk_Resource* resource, Ewk_Url_Response* response);
    void informResourceLoadFailed(Ewk_Resource* resource, Ewk_Error* error);
    void informResourceLoadFinished(Ewk_Resource* resource);
    void informResourceRequestSent(Ewk_Resource* resource, Ewk_Url_Request* request, Ewk_Url_Response* redirectResponse);
#if ENABLE(TIZEN_INTERCEPT_REQUEST)
    void shouldInterceptRequest(Ewk_Intercept_Request* interceptRequest);
#endif

    void informNavigationPolicyDecision(Ewk_Navigation_Policy_Decision* decision);
    void informNewWindowPolicyDecision(Ewk_Navigation_Policy_Decision* decision);
    void informBackForwardListChange();

    void informTitleChange(const String& title);
    void informTooltipTextChange(const String& text);
    void informTextFound(unsigned matchCount);
    void informIconChange();
    void informWebProcessCrashed();
    void informContentsSizeChange(const WebCore::IntSize& size);
    unsigned long long informDatabaseQuotaReached(const String& databaseName, const String& displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage);
    void informURLChange();

#if ENABLE(WEB_INTENTS)
    void informIntentRequest(Ewk_Intent* ewkIntent);
#endif
#if ENABLE(WEB_INTENTS_TAG)
    void informIntentServiceRegistration(Ewk_Intent_Service* ewkIntentService);
#endif

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    WebCore::AffineTransform transformFromView() const;
    WebCore::AffineTransform transformToView() const;
#endif
    WebCore::AffineTransform transformToScene() const;
    WebCore::AffineTransform transformFromScene() const;
    WebCore::AffineTransform transformToScreen() const;

#if ENABLE(TIZEN_WEBKIT2_SEPERATE_LOAD_PROGRESS)
    void informLoadProgressStarted();
    void informLoadProgressFinished();
#endif

#if ENABLE(TIZEN_DATALIST_ELEMENT)
    void deleteDataList();
#endif

#if ENABLE(TIZEN_GEOLOCATION)
    bool isValidLocationService();
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    bool canTakeFocus(WKFocusDirection);
#endif

#if USE(TILED_BACKING_STORE)
    void setScaleFactor(float scaleFactor) { m_scaleFactor = scaleFactor; }
    float scaleFactor() const { return m_scaleFactor; }

    void setScrollPosition(WebCore::IntPoint position) { m_scrollPosition = position; }
    const WebCore::IntPoint scrollPosition() const { return m_scrollPosition; }
#endif

#if ENABLE(TIZEN_GESTURE)
#if ENABLE(TOUCH_EVENTS)
    void feedTouchEventsByType(Ewk_Touch_Event_Type);
#endif
    void setDoubleTapEnabled(bool);
#endif

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
#define INVALID_ANGLE   -1
    void setCurrentAngle(int newAngle) { m_currentAngle = newAngle; }
    int currentAngle() { return m_currentAngle; }

    void setScreenShotForRotation(Evas_Object* screenShotForRotation) { m_screenShotForRotation = screenShotForRotation; }
    Evas_Object* screenShotForRotation() { return m_screenShotForRotation; }
#endif

#if ENABLE(TIZEN_EDGE_SUPPORT)
    RefPtr<WebKit::EdgeEffect> edgeEffect() { return m_edgeEffect; }
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    FocusRing* focusRing() { return m_focusRing.get(); }
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    WebKit::TextSelection* textSelection() { return m_textSelection.get(); }
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    static void pages(Vector<RefPtr<WebKit::WebPageProxy> >&);
    bool shouldHandleKeyDownEvent(const Evas_Event_Key_Down*);
    bool shouldHandleKeyUpEvent(const Evas_Event_Key_Up*);
    void clearKeyDownEventDeferred() { m_keyDownEventDeferred = false; }
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    void hideCandidatePopup();
#endif

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
    void saveSessionData() const;
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    void hwVideoOverlayEnabled();
    void hwVideoOverlayDisabled();
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
    void setFocusManually();
#endif

    WebCore::IntRect caretRect(bool);
    WebCore::IntRect selectionRect(bool);
    WebCore::IntRect focusedNodeRect(bool);

#if ENABLE(TIZEN_BACKGROUND_COLOR_API)
    void getBackgroundColor(int* red, int* green, int* blue, int* alpha);
    void setBackgroundColor(int red, int green, int blue, int alpha);
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    unsigned getXWindow();
#endif
#endif // #if PLATFORM(TIZEN)

    // FIXME: Make members private for encapsulation.
    OwnPtr<WebKit::PageClientImpl> pageClient;
#if ENABLE(WAIT_UPVERSION)
#if USE(TILED_BACKING_STORE)
    OwnPtr<WebKit::PageViewportControllerClientEfl> pageViewportControllerClient;
    OwnPtr<WebKit::PageViewportController> pageViewportController;
#endif
#endif
    RefPtr<WebKit::WebPageProxy> pageProxy;
    OwnPtr<WebKit::PageLoadClientEfl> pageLoadClient;
    OwnPtr<WebKit::PagePolicyClientEfl> pagePolicyClient;
    OwnPtr<WebKit::PageUIClientEfl> pageUIClient;
    OwnPtr<WebKit::ResourceLoadClientEfl> resourceLoadClient;
    OwnPtr<WebKit::FindClientEfl> findClient;
    OwnPtr<WebKit::FormClientEfl> formClient;

    OwnPtr<Ewk_Back_Forward_List> backForwardList;
    RefPtr<Ewk_Context> context;

    WebKit::WebPopupMenuProxyEfl* popupMenuProxy;
    Eina_List* popupMenuItems;

#if USE(ACCELERATED_COMPOSITING)
    Evas_GL* evasGl;
    Evas_GL_Context* evasGlContext;
    Evas_GL_Surface* evasGlSurface;
#endif

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_ORIENTATION_EVENTS)
    int orientation;
#endif

    JSGlobalContextRef javascriptGlobalContext;

    WKEinaSharedString userAgent;

#if PLATFORM(TIZEN)
    WKEinaSharedString applicationName;
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    WKEinaSharedString webAppIconURL;
    Eina_List* webAppIconURLs;
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    OwnPtr<Ewk_View_Callback_Context> passwordContext;
#endif
    OwnPtr<Ewk_View_Callback_Context> alertContext;
    OwnPtr<Ewk_View_Callback_Context> confirmContext;
    OwnPtr<Ewk_View_Callback_Context> promptContext;

#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
    OwnPtr<Ewk_View_Callback_Context> beforeUnloadConfirmPanelContext;
#endif

    OwnPtr<Ewk_View_Callback_Context> openpanelContext;
    OwnPtr<WebKit::JavaScriptPopup> javascriptPopup;
    bool isWaitingForJavaScriptPopupReply;

#if ENABLE(TIZEN_OPEN_PANEL)
    OwnPtr<WebKit::OpenPanel> openPanel;
    OwnPtr<WebKit::OpenMediaPlayer> openMediaPlayer;
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
    OwnPtr<WebKit::InputPicker> inputPicker;
    WKEinaSharedString inputValue;
#endif

    Ewk_Auth_Challenge* authChallenge;
    Ewk_Policy_Decision* policyDecision;
    WKOpenPanelResultListenerRef openPanelListener;

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    Ewk_Certificate_Policy_Decision* certificatePolicyDecision;
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    Ewk_Content_Screening_Detection* contentScreeningDetection;
#endif

    OwnPtr<WebKit::PermissionPopupManager> permissionPopupManager;

#if ENABLE(TIZEN_GEOLOCATION)
    OwnPtr<Ewk_View_Callback_Context> geolocationPermissionContext;
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
    OwnPtr<Ewk_View_Callback_Context> notificationPermissionContext;
    OwnPtr<Ewk_View_Callback_Context> notificationShowContext;
    OwnPtr<Ewk_View_Callback_Context> notificationCancelContext;
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
    OwnPtr<Ewk_View_Callback_Context> userMediaPermissionContext;
#endif

    bool suspendRequested;
    bool suspendedPainting;
    bool suspendedResources;

#if ENABLE(TIZEN_NOTIFICATIONS)
    Eina_List* notifications;
    OwnPtr<WebKit::NotificationManagerEfl> notificationManagerEfl;
#endif

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
    Ewk_Popup_Picker* popupPicker;
#endif

    bool isVerticalEdge;
    bool isHorizontalEdge;

#if ENABLE(TIZEN_GESTURE)
    OwnPtr<WebKit::GestureRecognizer> gestureRecognizer;
    OwnPtr<WebKit::GestureClient> gestureClient;
#if ENABLE(TOUCH_EVENTS)
    Evas_Coord_Point touchDownPoint;
    bool exceedTouchMoveThreshold;
    bool exceedTouchTapThreshold;
    bool wasHandledTouchStart;
    bool wasHandledTouchMove;
    bool wasHandledFirstTouchMove;
    WebCore::IntPoint viewPositionAtTouchStart;
#endif
    bool holdHorizontalPanning;
    bool holdVerticalPanning;
#endif // #if ENABLE(TIZEN_GESTURE)

#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
    bool isRunningSetFocusManually;
#endif

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    OwnPtr<WebKit::InputFieldZoom> inputFieldZoom;
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
    bool mainFrameScrollbarVisibility;
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    Ecore_Animator* compositionAnimator;
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    WKEinaSharedString selectedText;
#endif

#if ENABLE(TIZEN_DATALIST_ELEMENT)
    Eina_List* dataList;
#endif

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT_INTERNAL)
    struct {
        Ewk_Orientation_Lock_Cb callback;
        void* data;
    } orientationLock;
#endif

#if ENABLE(TIZEN_WEBKIT2_CREATE_VIEW_WITH_CREATED_PAGE_GROUP_WITH_IDENTIFIER)
    RefPtr<WebKit::WebPageGroup> pageGroup;
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
    OwnPtr<Ewk_View_Callback_Context> applicationCachePermissionContext;
    Ewk_Security_Origin* applicationCachePermissionOrigin;
    bool isWaitingForApplicationCachePermission;
#endif
#if ENABLE(TIZEN_INDEXED_DATABASE)
    OwnPtr<Ewk_View_Callback_Context> exceededIndexedDatabaseQuotaContext;
#endif
#if ENABLE(TIZEN_SQL_DATABASE)
    OwnPtr<Ewk_View_Callback_Context> exceededDatabaseQuotaContext;
#endif
#if ENABLE(TIZEN_FILE_SYSTEM)
    OwnPtr<Ewk_View_Callback_Context> exceededLocalFileSystemQuotaContext;
#endif
#if ENABLE(TIZEN_INTERCEPT_REQUEST)
    OwnPtr<Ewk_View_Callback_Context> interceptRequestContext;
#endif
#if ENABLE(TIZEN_FOCUS_UI)
    OwnPtr<Ewk_View_Callback_Context> unfocusAllowContext;
#endif
    Ewk_Security_Origin* exceededQuotaOrigin;
    bool isWaitingForExceededQuotaPopupReply;
#endif // #if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    struct {
        int count;
        int position;
        bool prevState;
        bool nextState;
    } formNavigation;
#endif

private:
    inline Ewk_View_Smart_Data* smartData() const;
    void displayTimerFired(WebCore::Timer<EwkViewImpl>*);

    static void onMouseDown(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void onMouseUp(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void onMouseMove(void* data, Evas*, Evas_Object*, void* eventInfo);
#if ENABLE(TOUCH_EVENTS)
    void feedTouchEvents(Ewk_Touch_Event_Type type);
    static void onTouchDown(void* data, Evas* /* canvas */, Evas_Object* ewkView, void* /* eventInfo */);
    static void onTouchUp(void* data, Evas* /* canvas */, Evas_Object* ewkView, void* /* eventInfo */);
    static void onTouchMove(void* /* data */, Evas* /* canvas */, Evas_Object* ewkView, void* /* eventInfo */);
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    void deferKeyDownEventTimerFired(WebCore::Timer<EwkViewImpl>*);
#endif

    Evas_Object* m_view;
    OwnPtr<Ewk_Settings> m_settings;
    RefPtr<Evas_Object> m_cursorObject;
    WKEinaSharedString m_cursorGroup;
    WKEinaSharedString m_faviconURL;
    WKEinaSharedString m_url;
    mutable WKEinaSharedString m_title;
    WKEinaSharedString m_theme;
    mutable WKEinaSharedString m_customEncoding;
    bool m_mouseEventsEnabled;
#if ENABLE(TOUCH_EVENTS)
    bool m_touchEventsEnabled;
#endif
    WKRetainPtr<WKColorPickerResultListenerRef> m_colorPickerResultListener;
    WebCore::Timer<EwkViewImpl> m_displayTimer;
    WTF::Vector <WebCore::IntRect> m_dirtyRects;
    OwnPtr<WebKit::InputMethodContextEfl> m_inputMethodContext;

    typedef HashMap<WKPageRef, const Evas_Object*> PageViewMap;
    static PageViewMap pageViewMap;

#if PLATFORM(TIZEN)
#if USE(TILED_BACKING_STORE)
    float m_scaleFactor;
    WebCore::IntPoint m_scrollPosition;
#endif

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    int m_currentAngle;
    Evas_Object* m_screenShotForRotation;
#endif

#if ENABLE(TIZEN_EDGE_SUPPORT)
    RefPtr<WebKit::EdgeEffect> m_edgeEffect;
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    OwnPtr<FocusRing> m_focusRing;
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    OwnPtr<WebKit::TextSelection> m_textSelection;
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    WebCore::Timer<EwkViewImpl> m_deferKeyDownEventTimer;
    bool m_keyDownEventDeferred;
#endif
#endif
};

#endif // EwkViewImpl_h
