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

#include "config.h"
#include "EwkViewImpl.h"

#include "EflScreenUtilities.h"
#include "FindClientEfl.h"
#include "FormClientEfl.h"
#include "InputMethodContextEfl.h"
#include "PageClientImpl.h"
#include "PageLoadClientEfl.h"
#include "PagePolicyClientEfl.h"
#include "PageUIClientEfl.h"
#if ENABLE(WAIT_UPVERSION)
#include "PageViewportController.h"
#include "PageViewportControllerClientEfl.h"
#endif
#include "ResourceLoadClientEfl.h"
#include "WKColorPickerResultListener.h"
#include "WKString.h"
#include "WebPageProxy.h"
#include "WebPopupMenuProxyEfl.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_context_private.h"
#include "ewk_favicon_database_private.h"
#include "ewk_popup_menu_item_private.h"
#include "ewk_private.h"
#include "ewk_settings_private.h"
#include "ewk_view.h"
#include "ewk_view_private.h"
#include <Ecore_Evas.h>
#include <Edje.h>
#include <WebCore/Cursor.h>

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_GEOLOCATION)
#include "ewk_geolocation_private.h"
#endif
#if ENABLE(TIZEN_MEDIA_STREAM)
#include "ewk_user_media_private.h"
#endif
#if ENABLE(TIZEN_NOTIFICATIONS)
#include "ewk_notification_private.h"
#endif
#if ENABLE(TIZEN_WEBKIT2_CREATE_VIEW_WITH_CREATED_PAGE_GROUP_WITH_IDENTIFIER)
#include "WebPageGroup.h"
#endif
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
#include "ewk_web_application_icon_data_private.h"
#endif
#endif

using namespace WebCore;
using namespace WebKit;

static const int defaultCursorSize = 16;

EwkViewImpl::PageViewMap EwkViewImpl::pageViewMap;

void EwkViewImpl::addToPageViewMap(const Evas_Object* ewkView)
{
    EwkViewImpl* viewImpl = EwkViewImpl::fromEvasObject(ewkView);

    PageViewMap::AddResult result = pageViewMap.add(viewImpl->wkPage(), ewkView);
    ASSERT_UNUSED(result, result.isNewEntry);
}

void EwkViewImpl::removeFromPageViewMap(const Evas_Object* ewkView)
{
    EwkViewImpl* viewImpl = EwkViewImpl::fromEvasObject(ewkView);

    ASSERT(pageViewMap.contains(viewImpl->wkPage()));

    if (!pageViewMap.contains(viewImpl->wkPage())) {
        TIZEN_LOGE("ewkview was already deleted");
        EINA_LOG_CRIT("ewkview was already deleted");
        return;
    }
    pageViewMap.remove(viewImpl->wkPage());
}

const Evas_Object* EwkViewImpl::viewFromPageViewMap(const WKPageRef page)
{
    ASSERT(page);

    return pageViewMap.get(page);
}

const Evas_Object* EwkViewImpl::visibleViewFromPageViewMap()
{
    for (PageViewMap::iterator it = pageViewMap.begin(); it != pageViewMap.end(); ++it)
        if (fromEvasObject(it->second)->pageClient->isVisible())
            return it->second;

    return 0;
}

#if ENABLE(TIZEN_WEBKIT2_CREATE_VIEW_WITH_CREATED_PAGE_GROUP_WITH_IDENTIFIER)
static uint64_t generatePageGroupIdentifierID()
{
    static uint64_t uniquePageGroupIdentifierID = 1;
    return uniquePageGroupIdentifierID++;
}
#endif

EwkViewImpl::EwkViewImpl(Evas_Object* view)
    : popupMenuProxy(0)
    , popupMenuItems(0)
#if USE(ACCELERATED_COMPOSITING)
    , evasGl(0)
    , evasGlContext(0)
    , evasGlSurface(0)
#endif
#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_ORIENTATION_EVENTS)
    , orientation(0)
#endif
    , javascriptGlobalContext(0)
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    , webAppIconURLs(0)
#endif
    , isWaitingForJavaScriptPopupReply(false)
    , authChallenge(0)
    , policyDecision(0)
    , openPanelListener(0)
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    , certificatePolicyDecision(0)
#endif
#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    , contentScreeningDetection(0)
#endif
    , suspendRequested(false)
    , suspendedPainting(false)
    , suspendedResources(false)
#if ENABLE(TIZEN_NOTIFICATIONS)
    , notifications(0)
#endif
#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
    , popupPicker(0)
#endif
    , isVerticalEdge(false)
    , isHorizontalEdge(false)
#if ENABLE(TIZEN_GESTURE)
#if ENABLE(TOUCH_EVENTS)
    , exceedTouchMoveThreshold(false)
    , exceedTouchTapThreshold(false)
    , wasHandledTouchStart(false)
    , wasHandledTouchMove(false)
    , wasHandledFirstTouchMove(false)
#endif
    , holdHorizontalPanning(false)
    , holdVerticalPanning(false)
#endif // #if ENABLE(TIZEN_GESTURE)
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    , compositionAnimator(0)
#endif
#if ENABLE(TIZEN_DATALIST_ELEMENT)
    , dataList(0)
#endif
#if ENABLE(TIZEN_APPLICATION_CACHE)
    , applicationCachePermissionOrigin(0)
    , isWaitingForApplicationCachePermission(false)
#endif
    , exceededQuotaOrigin(0)
    , isWaitingForExceededQuotaPopupReply(false)
#endif // #if PLATFORM(TIZEN)
    , m_view(view)
    , m_settings(Ewk_Settings::create(this))
    , m_mouseEventsEnabled(false)
#if ENABLE(TOUCH_EVENTS)
    , m_touchEventsEnabled(false)
#endif
    , m_displayTimer(this, &EwkViewImpl::displayTimerFired)
    , m_inputMethodContext(InputMethodContextEfl::create(this, smartData()->base.evas))
#if PLATFORM(TIZEN)
#if USE(TILED_BACKING_STORE)
    , m_scaleFactor(2)
#endif
#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
    , m_currentAngle(INVALID_ANGLE)
    , m_screenShotForRotation(0)
#endif
#if ENABLE(TIZEN_EDGE_SUPPORT)
    , m_edgeEffect(EdgeEffect::create(view))
#endif
#if ENABLE(TIZEN_FOCUS_UI)
    , m_deferKeyDownEventTimer(this, &EwkViewImpl::deferKeyDownEventTimerFired)
    , m_keyDownEventDeferred(false)
#endif
#endif // #if PLATFORM(TIZEN)
{
    ASSERT(view);

    // Enable mouse events by default
    setMouseEventsEnabled(true);

#if PLATFORM(TIZEN)
    javascriptPopup = adoptPtr<JavaScriptPopup>(new JavaScriptPopup(m_view));
    permissionPopupManager = adoptPtr<PermissionPopupManager>(new PermissionPopupManager(m_view));
#if ENABLE(TIZEN_NOTIFICATIONS)
    notificationManagerEfl = adoptPtr<NotificationManagerEfl>(new NotificationManagerEfl(m_view, this));
#endif
#if ENABLE(TIZEN_OPEN_PANEL)
    openPanel = adoptPtr<OpenPanel>(new OpenPanel(m_view));
    openMediaPlayer = adoptPtr<OpenMediaPlayer>(new OpenMediaPlayer(m_view));
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
    inputPicker = adoptPtr<InputPicker>(new InputPicker(m_view));
#endif

#if ENABLE(TIZEN_GESTURE)
#if ENABLE(TOUCH_EVENTS)
    touchDownPoint.x = 0;
    touchDownPoint.y = 0;
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
    isRunningSetFocusManually = false;
#endif


#if ENABLE(TIZEN_GESTURE)
    gestureRecognizer = GestureRecognizer::create(m_view);
    gestureClient = GestureClient::create(this);
#endif

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    inputFieldZoom = InputFieldZoom::create(this);
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
    const char* hideScrollbar = getenv("TIZEN_WEBKIT2_TILED_SCROLLBAR_HIDE");
    if (hideScrollbar && atoi(hideScrollbar) == 1)
        mainFrameScrollbarVisibility = false;
    else
        mainFrameScrollbarVisibility = true;
#endif

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT_INTERNAL)
    orientationLock.callback = 0;
    orientationLock.data = 0;
#endif

#if ENABLE(TIZEN_WEBKIT2_CREATE_VIEW_WITH_CREATED_PAGE_GROUP_WITH_IDENTIFIER)
    String pageGroupIdentifierID = String::number(generatePageGroupIdentifierID());
    String pageGroupIdentifier = String::format("PageGroup%s", pageGroupIdentifierID.utf8().data());

    WKRetainPtr<WKStringRef> pageGroupIdentifierRef(AdoptWK, WKStringCreateWithUTF8CString(pageGroupIdentifier.utf8().data()));
    pageGroup = WebPageGroup::create(toWTFString(pageGroupIdentifierRef.get()));
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    m_focusRing = FocusRing::create(this);
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    m_textSelection = TextSelection::create(this);
#endif
#endif // #if PLATFORM(TIZEN)
}

EwkViewImpl::~EwkViewImpl()
{
    TIZEN_LOGI("EwkViewImpl [%p]", this);
    void* item;
    EINA_LIST_FREE(popupMenuItems, item)
        delete static_cast<Ewk_Popup_Menu_Item*>(item);

#if PLATFORM(TIZEN)
    if (javascriptGlobalContext)
        JSGlobalContextRelease(javascriptGlobalContext);

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    if (webAppIconURLs) {
        void* data = 0;
        EINA_LIST_FREE(webAppIconURLs, data)
            ewkWebAppIconDataDelete(static_cast<Ewk_Web_App_Icon_Data*>(data));
    }
#endif

    if (authChallenge)
        ewkAuthChallengeDelete(authChallenge);
    if (policyDecision)
        ewkPolicyDecisionDelete(policyDecision);

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    if (certificatePolicyDecision)
        ewkCertificatePolicyDecisionDelete(certificatePolicyDecision);
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    if (contentScreeningDetection)
        ewkContentScreeningDetectionDelete(contentScreeningDetection);
#endif

#if ENABLE(TIZEN_DATALIST_ELEMENT)
    deleteDataList();
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
    if (notifications)
        ewkNotificationDeleteNotificationList(notifications);

    if (notificationManagerEfl)
        notificationManagerEfl->deleteAllNotifications();
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    if (compositionAnimator)
        ecore_animator_del(compositionAnimator);
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
    if (applicationCachePermissionOrigin)
        deleteSecurityOrigin(applicationCachePermissionOrigin);
#endif

    if (exceededQuotaOrigin)
        deleteSecurityOrigin(exceededQuotaOrigin);

#if ENABLE(TIZEN_OPEN_PANEL)
    if (openPanel)
        openPanel->close();
#endif

    permissionPopupManager->deleteAllPermissionRequest();

    if(page()->isClosed())
        return;
    page()->close();
#endif // #if PLATFORM(TIZEN)
}

Ewk_View_Smart_Data* EwkViewImpl::smartData() const
{
    return static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(m_view));
}

EwkViewImpl* EwkViewImpl::fromEvasObject(const Evas_Object* view)
{
    ASSERT(view);
    Ewk_View_Smart_Data* sd = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(view));
    ASSERT(sd);
    ASSERT(sd->priv);
    return sd->priv;
}

/**
 * @internal
 * Retrieves the internal WKPage for this view.
 */
WKPageRef EwkViewImpl::wkPage()
{
    return toAPI(pageProxy.get());
}

void EwkViewImpl::setCursor(const Cursor& cursor)
{
    Ewk_View_Smart_Data* sd = smartData();

    const char* group = cursor.platformCursor();
    if (!group || group == m_cursorGroup)
        return;

    m_cursorGroup = group;
    m_cursorObject = adoptRef(edje_object_add(sd->base.evas));

    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
    if (!m_theme || !edje_object_file_set(m_cursorObject.get(), m_theme, group)) {
        m_cursorObject.clear();

        ecore_evas_object_cursor_set(ecoreEvas, 0, 0, 0, 0);
#ifdef HAVE_ECORE_X
        if (WebCore::isUsingEcoreX(sd->base.evas))
            WebCore::applyFallbackCursor(ecoreEvas, group);
#endif
        return;
    }

    Evas_Coord width, height;
    edje_object_size_min_get(m_cursorObject.get(), &width, &height);
    if (width <= 0 || height <= 0)
        edje_object_size_min_calc(m_cursorObject.get(), &width, &height);
    if (width <= 0 || height <= 0) {
        width = defaultCursorSize;
        height = defaultCursorSize;
    }
    evas_object_resize(m_cursorObject.get(), width, height);

    const char* data;
    int hotspotX = 0;
    data = edje_object_data_get(m_cursorObject.get(), "hot.x");
    if (data)
        hotspotX = atoi(data);

    int hotspotY = 0;
    data = edje_object_data_get(m_cursorObject.get(), "hot.y");
    if (data)
        hotspotY = atoi(data);

    ecore_evas_object_cursor_set(ecoreEvas, m_cursorObject.get(), EVAS_LAYER_MAX, hotspotX, hotspotY);
}

void EwkViewImpl::displayTimerFired(WebCore::Timer<EwkViewImpl>*)
{
    Ewk_View_Smart_Data* sd = smartData();

    if (!sd->image)
        return;

#if USE(COORDINATED_GRAPHICS)
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, viewImpl);
#endif

    Region dirtyRegion;
    for (Vector<IntRect>::iterator it = m_dirtyRects.begin(); it != m_dirtyRects.end(); ++it)
        dirtyRegion.unite(*it);

    m_dirtyRects.clear();

    Vector<IntRect> rects = dirtyRegion.rects();
    Vector<IntRect>::iterator end = rects.end();

    for (Vector<IntRect>::iterator it = rects.begin(); it != end; ++it) {
        IntRect rect = *it;
#if USE(COORDINATED_GRAPHICS)
        evas_gl_make_current(viewImpl->evasGl, viewImpl->evasGlSurface, viewImpl->evasGlContext);
        viewImpl->pageViewportControllerClient->display(rect, IntPoint(sd->view.x, sd->view.y));
#endif

        evas_object_image_data_update_add(sd->image, rect.x(), rect.y(), rect.width(), rect.height());
    }
}

void EwkViewImpl::redrawRegion(const IntRect& rect)
{
    if (!m_displayTimer.isActive())
        m_displayTimer.startOneShot(0);
    m_dirtyRects.append(rect);
}

/**
 * @internal
 * A download for that view was cancelled.
 *
 * Emits signal: "download,cancelled" with pointer to a Ewk_Download_Job.
 */
void EwkViewImpl::informDownloadJobCancelled(Ewk_Download_Job* download)
{
    evas_object_smart_callback_call(m_view, "download,cancelled", download);
}

/**
 * @internal
 * A download for that view has failed.
 *
 * Emits signal: "download,failed" with pointer to a Ewk_Download_Job_Error.
 */
void EwkViewImpl::informDownloadJobFailed(Ewk_Download_Job* download, Ewk_Error* error)
{
    Ewk_Download_Job_Error downloadError = { download, error };
    evas_object_smart_callback_call(m_view, "download,failed", &downloadError);
}

/**
 * @internal
 * A download for that view finished successfully.
 *
 * Emits signal: "download,finished" with pointer to a Ewk_Download_Job.
 */
void EwkViewImpl::informDownloadJobFinished(Ewk_Download_Job* download)
{
    evas_object_smart_callback_call(m_view, "download,finished", download);
}

/**
 * @internal
 * A new download has been requested for that view.
 *
 * Emits signal: "download,request" with pointer to a Ewk_Download_Job.
 */
void EwkViewImpl::informDownloadJobRequested(Ewk_Download_Job* download)
{
    evas_object_smart_callback_call(m_view, "download,request", download);
}

/**
 * @internal
 * informs that a form request is about to be submitted.
 *
 * Emits signal: "form,submission,request" with pointer to Ewk_Form_Submission_Request.
 */
void EwkViewImpl::informNewFormSubmissionRequest(Ewk_Form_Submission_Request* request)
{
    evas_object_smart_callback_call(m_view, "form,submission,request", request);
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO) && ENABLE(APP_LOGGING_FOR_MEDIA)
void EwkViewImpl::videoStreamingCount()
{
    evas_object_smart_callback_call(m_view, "video,streaming,count", 0);
}
#endif

#if ENABLE(FULLSCREEN_API)
/**
 * @internal
 * Calls fullscreen_enter callback or falls back to default behavior and enables fullscreen mode.
 */
void EwkViewImpl::enterFullScreen()
{
    Ewk_View_Smart_Data* sd = smartData();

    if (!sd->api->fullscreen_enter || !sd->api->fullscreen_enter(sd)) {
        Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_fullscreen_set(ecoreEvas, true);
    }

#if ENABLE(TIZEN_FULLSCREEN_API)
    // Height of contents could be shorter than height of visible rect in fullscreen.
    // In this case, part of fullscreen is not drawn on screen.
    // Increase scale factor to fill up visible rect with contents.
    const int defautWidthForDesktopPage = 980;
    WebCore::IntRect visibleRect = pageClient->visibleContentRect();
    if (visibleRect.width() / pageClient->scaleFactor() > defautWidthForDesktopPage) {
        pageProxy->fullScreenManager()->setScaleFactorToRestore(pageClient->scaleFactor());

        float newScale = (float)visibleRect.width() / defautWidthForDesktopPage;
        pageClient->setVisibleContentRect(visibleRect, newScale);
    }
#endif
}

/**
 * @internal
 * Calls fullscreen_exit callback or falls back to default behavior and disables fullscreen mode.
 */
void EwkViewImpl::exitFullScreen()
{
    Ewk_View_Smart_Data* sd = smartData();

    if (!sd->api->fullscreen_exit || !sd->api->fullscreen_exit(sd)) {
        Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
        ecore_evas_fullscreen_set(ecoreEvas, false);
    }
}

#if ENABLE(TIZEN_FULLSCREEN_API)
void EwkViewImpl::restoreScaleFactorAfterExitingFullScreen(float scale)
{
    pageClient->setVisibleContentRect(pageClient->visibleContentRect(), scale);
}
#endif
#endif

void EwkViewImpl::setImageData(void* imageData, const IntSize& size)
{
    Ewk_View_Smart_Data* sd = smartData();
    if (!imageData || !sd->image)
        return;

    evas_object_resize(sd->image, size.width(), size.height());
    evas_object_image_size_set(sd->image, size.width(), size.height());
    evas_object_image_data_copy_set(sd->image, imageData);
}

#if ENABLE(INPUT_TYPE_COLOR)
bool EwkViewImpl::setColorPickerColor(const WebCore::Color& color)
{
    if (!m_colorPickerResultListener)
        return false;

    WKRetainPtr<WKStringRef> colorString(AdoptWK, WKStringCreateWithUTF8CString(color.serialized().utf8().data()));
    WKColorPickerResultListenerSetColor(m_colorPickerResultListener.get(), colorString.get());
    m_colorPickerResultListener.clear();

    return true;
}
#endif

/**
 * @internal
 * informs load failed with error information.
 *
 * Emits signal: "load,error" with pointer to Ewk_Error.
 */
void EwkViewImpl::informLoadError(Ewk_Error* error)
{
    evas_object_smart_callback_call(m_view, "load,error", error);
}

/**
 * @internal
 * informs load finished.
 *
 * Emits signal: "load,finished".
 */
void EwkViewImpl::informLoadFinished()
{
    informURLChange();
    evas_object_smart_callback_call(m_view, "load,finished", 0);

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    pageProxy->setLoadingFinished(true);
#endif
    if (pageClient->wasViewportFitsToContent() && !pageClient->viewportConstraints().fixedInitialScale)
        pageClient->fitViewportToContent();

#if ENABLE(TIZEN_WEBKIT2_RESTORE_SCROLLPOINT_ON_FRAME_LOAD_FINISH)
    pageClient->restoreVisibleContentRect();
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    ewk_view_form_password_data_fill(m_view);
#endif

    if (!suspendRequested)
        return;

    suspendRequested = false;

    if (!suspendedPainting) {
        pageProxy->suspendPainting();
        suspendedPainting = true;
    }
    if (!suspendedResources) {
        pageProxy->suspendJavaScriptAndResource();
        suspendedResources = true;
    }
#endif
}

/**
 * @internal
 * informs Change Color for Theme.
 *
 * Emits signal: "theme,color" with the theme color.
 */
#if ENABLE(TIZEN_CSS_THEME_COLOR)
void EwkViewImpl::informThemeColor(const WebCore::Color& theme_color)
{
        Ewk_Color colorChange;
        colorChange.r = theme_color.red();
        colorChange.g = theme_color.green();
        colorChange.b = theme_color.blue();
        colorChange.a = theme_color.alpha();

        evas_object_smart_callback_call(m_view, "theme,color", &colorChange);
}
#endif

/**
 * @internal
 * informs load progress changed.
 *
 * Emits signal: "load,progress" with pointer to a double from 0.0 to 1.0.
 */
void EwkViewImpl::informLoadProgress(double progress)
{
    evas_object_smart_callback_call(m_view, "load,progress", &progress);
}

/**
 * @internal
 * informs view provisional load failed with error information.
 *
 * Emits signal: "load,provisional,failed" with pointer to Ewk_Error.
 */
void EwkViewImpl::informProvisionalLoadFailed(Ewk_Error* error)
{
    evas_object_smart_callback_call(m_view, "load,provisional,failed", error);
}

#if ENABLE(WAIT_UPVERSION)
#if USE(TILED_BACKING_STORE)
void EwkViewImpl::informLoadCommitted()
{
    pageViewportController->didCommitLoad();
}
#endif
#endif

/**
 * @internal
 * informs view received redirect for provisional load.
 *
 * Emits signal: "load,provisional,redirect".
 */
void EwkViewImpl::informProvisionalLoadRedirect()
{
    informURLChange();
    evas_object_smart_callback_call(m_view, "load,provisional,redirect", 0);
}

/**
 * @internal
 * informs view provisional load started.
 *
 * Emits signal: "load,provisional,started".
 */
void EwkViewImpl::informProvisionalLoadStarted()
{
#if PLATFORM(TIZEN)
    evas_object_smart_callback_call(m_view, "load,started", 0);
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
    if (pageClient->isDragMode())
        pageClient->setDragMode(false);
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
    pageProxy->setLoadingFinished(false);
#endif

    informURLChange();
    evas_object_smart_callback_call(m_view, "load,provisional,started", 0);
}

/**
 * @internal
 * informs that a navigation policy decision should be taken.
 *
 * Emits signal: "policy,decision,navigation".
 */
void EwkViewImpl::informNavigationPolicyDecision(Ewk_Navigation_Policy_Decision* decision)
{
    evas_object_smart_callback_call(m_view, "policy,decision,navigation", decision);
}

/**
 * @internal
 * informs that a new window policy decision should be taken.
 *
 * Emits signal: "policy,decision,new,window".
 */
void EwkViewImpl::informNewWindowPolicyDecision(Ewk_Navigation_Policy_Decision* decision)
{
    evas_object_smart_callback_call(m_view, "policy,decision,new,window", decision);
}

/**
 * @internal
 * Load was initiated for a resource in the view.
 *
 * Emits signal: "resource,request,new" with pointer to resource request.
 */
void EwkViewImpl::informResourceLoadStarted(Ewk_Resource* resource, Ewk_Url_Request* request)
{
    Ewk_Resource_Request resourceRequest = {resource, request, 0};

    evas_object_smart_callback_call(m_view, "resource,request,new", &resourceRequest);
}

/**
 * @internal
 * Received a response to a resource load request in the view.
 *
 * Emits signal: "resource,request,response" with pointer to resource response.
 */
void EwkViewImpl::informResourceLoadResponse(Ewk_Resource* resource, Ewk_Url_Response* response)
{
    Ewk_Resource_Load_Response resourceLoadResponse = {resource, response};
    evas_object_smart_callback_call(m_view, "resource,request,response", &resourceLoadResponse);
}

/**
 * @internal
 * Failed loading a resource in the view.
 *
 * Emits signal: "resource,request,finished" with pointer to the resource load error.
 */
void EwkViewImpl::informResourceLoadFailed(Ewk_Resource* resource, Ewk_Error* error)
{
    Ewk_Resource_Load_Error resourceLoadError = {resource, error};
    evas_object_smart_callback_call(m_view, "resource,request,failed", &resourceLoadError);
}

/**
 * @internal
 * Finished loading a resource in the view.
 *
 * Emits signal: "resource,request,finished" with pointer to the resource.
 */
void EwkViewImpl::informResourceLoadFinished(Ewk_Resource* resource)
{
    evas_object_smart_callback_call(m_view, "resource,request,finished", resource);
}

/**
 * @internal
 * Request was sent for a resource in the view.
 *
 * Emits signal: "resource,request,sent" with pointer to resource request and possible redirect response.
 */
void EwkViewImpl::informResourceRequestSent(Ewk_Resource* resource, Ewk_Url_Request* request, Ewk_Url_Response* redirectResponse)
{
    Ewk_Resource_Request resourceRequest = {resource, request, redirectResponse};
    evas_object_smart_callback_call(m_view, "resource,request,sent", &resourceRequest);
}

#if ENABLE(TIZEN_INTERCEPT_REQUEST)
/**
 * @internal
 * Request sent to notify intercept request.
 *
 * Emits signal: "intercept,request" with pointer to intercept request.
 */
void EwkViewImpl::shouldInterceptRequest(Ewk_Intercept_Request* interceptRequest)
{
    if (!interceptRequestContext || !interceptRequestContext->interceptRequestCallback)
        return;

    EINA_SAFETY_ON_NULL_RETURN(interceptRequestContext->ewkView == m_view);

    interceptRequestContext->interceptRequestCallback(m_view, interceptRequest , interceptRequestContext->userData);
}
#endif

/**
 * @internal
 * The view title was changed by the frame loader.
 *
 * Emits signal: "title,changed" with pointer to new title string.
 */
void EwkViewImpl::informTitleChange(const String& title)
{
    evas_object_smart_callback_call(m_view, "title,changed", const_cast<char*>(title.utf8().data()));
}

/**
 * @internal
 */
void EwkViewImpl::informTooltipTextChange(const String& text)
{
    if (text.isEmpty())
        evas_object_smart_callback_call(m_view, "tooltip,text,unset", 0);
    else
        evas_object_smart_callback_call(m_view, "tooltip,text,set", const_cast<char*>(text.utf8().data()));

}

/**
 * @internal
 * informs that the requested text was found.
 *
 * Emits signal: "text,found" with the number of matches.
 */
void EwkViewImpl::informTextFound(unsigned matchCount)
{
    evas_object_smart_callback_call(m_view, "text,found", &matchCount);
}

IntSize EwkViewImpl::size() const
{
    int width, height;
    evas_object_geometry_get(m_view, 0, 0, &width, &height);
    return IntSize(width, height);
}

bool EwkViewImpl::isFocused() const
{
    return evas_object_focus_get(m_view);
}

bool EwkViewImpl::isVisible() const
{
    return evas_object_visible_get(m_view);
}

const char* EwkViewImpl::title() const
{
    m_title = pageProxy->pageTitle().utf8().data();

    return m_title;
}

/**
 * @internal
 * This function may return @c NULL.
 */
InputMethodContextEfl* EwkViewImpl::inputMethodContext()
{
    return m_inputMethodContext.get();
}

const char* EwkViewImpl::themePath() const
{
    return m_theme;
}

void EwkViewImpl::setThemePath(const char* theme)
{
    if (m_theme != theme) {
        m_theme = theme;
        pageProxy->setThemePath(theme);
    }
}

const char* EwkViewImpl::customTextEncodingName() const
{
    String customEncoding = pageProxy->customTextEncodingName();
    if (customEncoding.isEmpty())
        return 0;

    m_customEncoding = customEncoding.utf8().data();

    return m_customEncoding;
}

void EwkViewImpl::setCustomTextEncodingName(const char* encoding)
{
    m_customEncoding = encoding;
    pageProxy->setCustomTextEncodingName(encoding ? encoding : String());
}

void EwkViewImpl::setMouseEventsEnabled(bool enabled)
{
    if (m_mouseEventsEnabled == enabled)
        return;

    m_mouseEventsEnabled = enabled;
    if (enabled) {
        Ewk_View_Smart_Data* sd = smartData();
        evas_object_event_callback_add(m_view, EVAS_CALLBACK_MOUSE_DOWN, onMouseDown, sd);
        evas_object_event_callback_add(m_view, EVAS_CALLBACK_MOUSE_UP, onMouseUp, sd);
        evas_object_event_callback_add(m_view, EVAS_CALLBACK_MOUSE_MOVE, onMouseMove, sd);
    } else {
        evas_object_event_callback_del(m_view, EVAS_CALLBACK_MOUSE_DOWN, onMouseDown);
        evas_object_event_callback_del(m_view, EVAS_CALLBACK_MOUSE_UP, onMouseUp);
        evas_object_event_callback_del(m_view, EVAS_CALLBACK_MOUSE_MOVE, onMouseMove);
    }
}

#if ENABLE(TOUCH_EVENTS)
void EwkViewImpl::setTouchEventsEnabled(bool enabled)
{
    if (m_touchEventsEnabled == enabled)
        return;

    m_touchEventsEnabled = enabled;

    if (enabled) {
        // FIXME: We have to connect touch callbacks with mouse and multi events
        // because the Evas creates mouse events for first touch and multi events
        // for second and third touches. Below codes should be fixed when the Evas
        // supports the touch events.
        // See https://bugs.webkit.org/show_bug.cgi?id=97785 for details.
        Ewk_View_Smart_Data* sd = smartData();
        evas_object_event_callback_add(m_view, EVAS_CALLBACK_MOUSE_DOWN, onTouchDown, sd);
        evas_object_event_callback_add(m_view, EVAS_CALLBACK_MOUSE_UP, onTouchUp, sd);
        evas_object_event_callback_add(m_view, EVAS_CALLBACK_MOUSE_MOVE, onTouchMove, sd);
        evas_object_event_callback_add(m_view, EVAS_CALLBACK_MULTI_DOWN, onTouchDown, sd);
        evas_object_event_callback_add(m_view, EVAS_CALLBACK_MULTI_UP, onTouchUp, sd);
        evas_object_event_callback_add(m_view, EVAS_CALLBACK_MULTI_MOVE, onTouchMove, sd);
    } else {
        evas_object_event_callback_del(m_view, EVAS_CALLBACK_MOUSE_DOWN, onTouchDown);
        evas_object_event_callback_del(m_view, EVAS_CALLBACK_MOUSE_UP, onTouchUp);
        evas_object_event_callback_del(m_view, EVAS_CALLBACK_MOUSE_MOVE, onTouchMove);
        evas_object_event_callback_del(m_view, EVAS_CALLBACK_MULTI_DOWN, onTouchDown);
        evas_object_event_callback_del(m_view, EVAS_CALLBACK_MULTI_UP, onTouchUp);
        evas_object_event_callback_del(m_view, EVAS_CALLBACK_MULTI_MOVE, onTouchMove);
    }
}
#endif

/**
 * @internal
 * Update the view's favicon and emits a "icon,changed" signal if it has
 * changed.
 *
 * This function is called whenever the URL has changed or when the icon for
 * the current page URL has changed.
 */
void EwkViewImpl::informIconChange()
{
#if ENABLE(TIZEN_ICON_DATABASE)
    return;
#endif
    Ewk_Favicon_Database* iconDatabase = context->faviconDatabase();
    ASSERT(iconDatabase);

    m_faviconURL = ewk_favicon_database_icon_url_get(iconDatabase, m_url);
    evas_object_smart_callback_call(m_view, "icon,changed", 0);
}

#if ENABLE(WEB_INTENTS)
/**
 * @internal
 * The view received a new intent request.
 *
 * Emits signal: "intent,request,new" with pointer to a Ewk_Intent.
 */
void EwkViewImpl::informIntentRequest(Ewk_Intent* ewkIntent)
{
    evas_object_smart_callback_call(m_view, "intent,request,new", ewkIntent);
}
#endif

#if ENABLE(WEB_INTENTS_TAG)
/**
 * @internal
 * The view received a new intent service registration.
 *
 * Emits signal: "intent,service,register" with pointer to a Ewk_Intent_Service.
 */
void EwkViewImpl::informIntentServiceRegistration(Ewk_Intent_Service* ewkIntentService)
{
    evas_object_smart_callback_call(m_view, "intent,service,register", ewkIntentService);
}
#endif // ENABLE(WEB_INTENTS_TAG)

#if USE(ACCELERATED_COMPOSITING)
bool EwkViewImpl::createGLSurface(const IntSize& viewSize)
{
    Ewk_View_Smart_Data* sd = smartData();

    Evas_GL_Config evasGlConfig = {
        EVAS_GL_RGBA_8888,
        EVAS_GL_DEPTH_BIT_8,
        EVAS_GL_STENCIL_NONE,
        EVAS_GL_OPTIONS_NONE,
        EVAS_GL_MULTISAMPLE_NONE
    };

    ASSERT(!evasGlSurface);
    evasGlSurface = evas_gl_surface_create(evasGl, &evasGlConfig, viewSize.width(), viewSize.height());
    if (!evasGlSurface)
        return false;

    Evas_Native_Surface nativeSurface;
    evas_gl_native_surface_get(evasGl, evasGlSurface, &nativeSurface);
    evas_object_image_native_surface_set(sd->image, &nativeSurface);

    return true;
}

bool EwkViewImpl::enterAcceleratedCompositingMode()
{
    if (evasGl) {
        EINA_LOG_DOM_WARN(_ewk_log_dom, "Accelerated compositing mode already entered.");
        return false;
    }

    Evas* evas = evas_object_evas_get(m_view);
    evasGl = evas_gl_new(evas);
    if (!evasGl)
        return false;

    evasGlContext = evas_gl_context_create(evasGl, 0);
    if (!evasGlContext) {
        evas_gl_free(evasGl);
        evasGl = 0;
        return false;
    }

    if (!createGLSurface(size())) {
        evas_gl_context_destroy(evasGl, evasGlContext);
        evasGlContext = 0;

        evas_gl_free(evasGl);
        evasGl = 0;
        return false;
    }

#if ENABLE(WAIT_UPVERSION)
    pageViewportControllerClient->setRendererActive(true);
#endif
    return true;
}

bool EwkViewImpl::exitAcceleratedCompositingMode()
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(evasGl, false);

    if (evasGlSurface) {
        evas_gl_surface_destroy(evasGl, evasGlSurface);
        evasGlSurface = 0;
    }

    if (evasGlContext) {
        evas_gl_context_destroy(evasGl, evasGlContext);
        evasGlContext = 0;
    }

    evas_gl_free(evasGl);
    evasGl = 0;

    return true;
}
#endif

#if ENABLE(INPUT_TYPE_COLOR)
/**
 * @internal
 * Requests to show external color picker.
 */
void EwkViewImpl::requestColorPicker(int r, int g, int b, int a, WKColorPickerResultListenerRef listener)
{
    Ewk_View_Smart_Data* sd = smartData();
    EINA_SAFETY_ON_NULL_RETURN(sd->api->input_picker_color_request);

    m_colorPickerResultListener = listener;

#if ENABLE(TIZEN_INPUT_COLOR_PICKER)
    if (!sd->api->input_picker_color_request(sd, r, g, b, a))
        page()->endColorChooser();
#else
    sd->api->input_picker_color_request(sd, r, g, b, a);
#endif
}

/**
 * @internal
 * Requests to hide external color picker.
 */
void EwkViewImpl::dismissColorPicker()
{
    Ewk_View_Smart_Data* sd = smartData();
    EINA_SAFETY_ON_NULL_RETURN(sd->api->input_picker_color_dismiss);

    m_colorPickerResultListener.clear();

    sd->api->input_picker_color_dismiss(sd);
}
#endif

/**
 * @internal
 * informs that the view's back / forward list has changed.
 *
 * Emits signal: "back,forward,list,changed".
 */
void EwkViewImpl::informBackForwardListChange()
{
    evas_object_smart_callback_call(m_view, "back,forward,list,changed", 0);
}

/**
 * @internal
 * Web process has crashed.
 *
 * Emits signal: "webprocess,crashed" with pointer to crash handling boolean.
 */
void EwkViewImpl::informWebProcessCrashed()
{
    bool handled = false;
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("Notify m_view of crash: %x", m_view);
#endif
    evas_object_smart_callback_call(m_view, "webprocess,crashed", &handled);
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("crash has been handled[%d]", handled);
#endif
    if (!handled) {
        CString url = pageProxy->urlAtProcessExit().utf8();
        WARN("WARNING: The web process experienced a crash on '%s'.\n", url.data());

        // Display an error page
        ewk_view_html_string_load(m_view, "The web process has crashed.", 0, url.data());
    }
}

void EwkViewImpl::informContentsSizeChange(const IntSize& size)
{
#if USE(COORDINATED_GRAPHICS)
    pageViewportControllerClient->didChangeContentsSize(size);
#else
    UNUSED_PARAM(size);
#endif
    evas_object_smart_callback_call(m_view, "contents,size,changed", nullptr);
}

COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_RIGHT_TO_LEFT, RTL);
COMPILE_ASSERT_MATCHING_ENUM(EWK_TEXT_DIRECTION_LEFT_TO_RIGHT, LTR);

#if ENABLE(TIZEN_MULTIPLE_SELECT)
void EwkViewImpl::requestPopupMenu(WebPopupMenuProxyEfl* popupMenu, const IntRect& rect, TextDirection textDirection, double pageScaleFactor, const Vector<WebPopupItem>& items, int32_t selectedIndex, bool multiple)
#else
void EwkViewImpl::requestPopupMenu(WebPopupMenuProxyEfl* popupMenu, const IntRect& rect, TextDirection textDirection, double pageScaleFactor, const Vector<WebPopupItem>& items, int32_t selectedIndex)
#endif
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    ASSERT(popupMenu);

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    if (multiple ? !sd->api->multiple_popup_menu_show : !sd->api->popup_menu_show)
#else
    if (!sd->api->popup_menu_show)
#endif
        return;

    if (popupMenuProxy)
        ewk_view_popup_menu_close(m_view);
    popupMenuProxy = popupMenu;

    Eina_List* popupItems = 0;
    const size_t size = items.size();
    for (size_t i = 0; i < size; ++i)
        popupItems = eina_list_append(popupItems, Ewk_Popup_Menu_Item::create(items[i]).leakPtr());
    popupMenuItems = popupItems;

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
#if ENABLE(TIZEN_MULTIPLE_SELECT)
    if (multiple ? sd->api->multiple_popup_menu_show(sd, rect, static_cast<Ewk_Text_Direction>(textDirection), pageScaleFactor, popupMenuItems)
        : sd->api->popup_menu_show(sd, rect, static_cast<Ewk_Text_Direction>(textDirection), pageScaleFactor, popupMenuItems, selectedIndex)) {
#else
    if (sd->api->popup_menu_show(sd, rect, static_cast<Ewk_Text_Direction>(textDirection), pageScaleFactor, popupMenuItems, selectedIndex)) {
#endif

#if ENABLE(TIZEN_ISF_PORT)
        //If keyboard is shown hide it, As we are going to show popupMenu.
        if (inputMethodContext()->isShow())
            inputMethodContext()->hideIMFContext();
#endif

        /* maps.google.com generate mouse event in touch down without preventDefault.
         * So, popup menu is opend in touch down event and closed via fake mouse down
         * which generated by endTap.
         * In order to fix select of maps.google.com (based on touch behavior),
         * We should disable touch events when select popup is open.
         */

#if ENABLE(TIZEN_GESTURE)
        gestureClient->reset();
#endif
        ewk_view_touch_events_enabled_set(m_view, false);
    }
#else
    sd->api->popup_menu_show(sd, rect, static_cast<Ewk_Text_Direction>(textDirection), pageScaleFactor, popupItems, selectedIndex);
#endif
}

/**
 * @internal
 * Calls a smart member function for javascript alert().
 */
void EwkViewImpl::requestJSAlertPopup(const WKEinaSharedString& message)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->run_javascript_alert)
        return;

    sd->api->run_javascript_alert(sd, message);
}

/**
 * @internal
 * Calls a smart member function for javascript confirm() and returns a value from the function. Returns false by default.
 */
bool EwkViewImpl::requestJSConfirmPopup(const WKEinaSharedString& message)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->run_javascript_confirm)
        return false;

    return sd->api->run_javascript_confirm(sd, message);
}

/**
 * @internal
 * Calls a smart member function for javascript prompt() and returns a value from the function. Returns null string by default.
 */
WKEinaSharedString EwkViewImpl::requestJSPromptPopup(const WKEinaSharedString& message, const WKEinaSharedString& defaultValue)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    if (!sd->api->run_javascript_prompt)
        return WKEinaSharedString();

    return WKEinaSharedString::adopt(sd->api->run_javascript_prompt(sd, message, defaultValue));
}

#if ENABLE(SQL_DATABASE)
/**
 * @internal
 * Calls exceeded_database_quota callback or falls back to default behavior returns default database quota.
 */
unsigned long long EwkViewImpl::informDatabaseQuotaReached(const String& databaseName, const String& displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage)
{
    Ewk_View_Smart_Data* sd = smartData();
    ASSERT(sd->api);

    static const unsigned long long defaultQuota = 5 * 1024 * 1204; // 5 MB
    if (sd->api->exceeded_database_quota)
        return sd->api->exceeded_database_quota(sd, databaseName.utf8().data(), displayName.utf8().data(), currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);

    return defaultQuota;
}
#endif

/**
 * @internal
 * The url of view was changed by the frame loader.
 *
 * Emits signal: "url,changed" with pointer to new url string.
 */
void EwkViewImpl::informURLChange()
{
    String activeURL = pageProxy->activeURL();
    if (activeURL.isEmpty())
        return;

    CString rawActiveURL = activeURL.utf8();
    if (m_url == rawActiveURL.data())
        return;

    m_url = rawActiveURL.data();
    const char* callbackArgument = static_cast<const char*>(m_url);
    evas_object_smart_callback_call(m_view, "url,changed", const_cast<char*>(callbackArgument));
#if PLATFORM(TIZEN)
    evas_object_smart_callback_call(m_view, "uri,changed", const_cast<char*>(callbackArgument));
#endif

    // Update the view's favicon.
    informIconChange();
}

WKPageRef EwkViewImpl::createNewPage()
{
    Evas_Object* newEwkView = 0;
    evas_object_smart_callback_call(m_view, "create,window", &newEwkView);

    if (!newEwkView)
        return 0;

    EwkViewImpl* newViewImpl = EwkViewImpl::fromEvasObject(newEwkView);
    ASSERT(newViewImpl);

    return static_cast<WKPageRef>(WKRetain(newViewImpl->page()));
}

void EwkViewImpl::closePage()
{
    evas_object_smart_callback_call(m_view, "close,window", 0);
}

void EwkViewImpl::onMouseDown(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Down* downEvent = static_cast<Evas_Event_Mouse_Down*>(eventInfo);
    Ewk_View_Smart_Data* sd = static_cast<Ewk_View_Smart_Data*>(data);
    EINA_SAFETY_ON_NULL_RETURN(sd->api);
    EINA_SAFETY_ON_NULL_RETURN(sd->api->mouse_down);
    sd->api->mouse_down(sd, downEvent);
}

void EwkViewImpl::onMouseUp(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Up* upEvent = static_cast<Evas_Event_Mouse_Up*>(eventInfo);
    Ewk_View_Smart_Data* sd = static_cast<Ewk_View_Smart_Data*>(data);
    EINA_SAFETY_ON_NULL_RETURN(sd->api);
    EINA_SAFETY_ON_NULL_RETURN(sd->api->mouse_up);
    sd->api->mouse_up(sd, upEvent);
}

void EwkViewImpl::onMouseMove(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Move* moveEvent = static_cast<Evas_Event_Mouse_Move*>(eventInfo);
    Ewk_View_Smart_Data* sd = static_cast<Ewk_View_Smart_Data*>(data);
    EINA_SAFETY_ON_NULL_RETURN(sd->api);
    EINA_SAFETY_ON_NULL_RETURN(sd->api->mouse_move);
    sd->api->mouse_move(sd, moveEvent);
}

#if ENABLE(TOUCH_EVENTS)
void EwkViewImpl::feedTouchEvents(Ewk_Touch_Event_Type type)
{
    Ewk_View_Smart_Data* sd = smartData();

    unsigned count = evas_touch_point_list_count(sd->base.evas);
    if (!count)
        return;

    Eina_List* points = 0;
    for (unsigned i = 0; i < count; ++i) {
        Ewk_Touch_Point* point = new Ewk_Touch_Point;
        point->id = evas_touch_point_list_nth_id_get(sd->base.evas, i);
        evas_touch_point_list_nth_xy_get(sd->base.evas, i, &point->x, &point->y);
        point->state = evas_touch_point_list_nth_state_get(sd->base.evas, i);
#if ENABLE(TOUCH_EVENTS) && ENABLE(TIZEN_GESTURE)
        if (type == EWK_TOUCH_CANCEL)
            point->state = EVAS_TOUCH_POINT_CANCEL;
#endif
        points = eina_list_append(points, point);
    }

    ewk_view_feed_touch_event(m_view, type, points, evas_key_modifier_get(sd->base.evas));

    void* data;
    EINA_LIST_FREE(points, data)
        delete static_cast<Ewk_Touch_Point*>(data);
}

void EwkViewImpl::onTouchDown(void* data, Evas* /* canvas */, Evas_Object* ewkView, void* /* eventInfo */)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    Ewk_View_Smart_Data* sd = static_cast<Ewk_View_Smart_Data*>(data);

    TIZEN_LOGI("[Touch] onTouchDown is called : Touch Point Count [%d]", evas_touch_point_list_count(sd->base.evas));
#endif

    EwkViewImpl* viewImpl = EwkViewImpl::fromEvasObject(ewkView);
    viewImpl->feedTouchEvents(EWK_TOUCH_START);
}

void EwkViewImpl::onTouchUp(void* data, Evas* /* canvas */, Evas_Object* ewkView, void* eventInfo)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    Ewk_View_Smart_Data* sd = static_cast<Ewk_View_Smart_Data*>(data);

    TIZEN_LOGI("[Touch] onTouchUp is called : Touch Point Count [%d]", evas_touch_point_list_count(sd->base.evas));
#endif

    EwkViewImpl* viewImpl = EwkViewImpl::fromEvasObject(ewkView);

#if ENABLE(TIZEN_WEBKIT2_TOUCH_CANCEL)
    if (static_cast<Evas_Event_Mouse_Up*>(eventInfo)->event_flags & EVAS_EVENT_FLAG_ON_HOLD) {
        viewImpl->feedTouchEvents(EWK_TOUCH_CANCEL);
        return;
    }
#else
    UNUSED_PARAM(eventInfo);
#endif
    viewImpl->feedTouchEvents(EWK_TOUCH_END);
}

void EwkViewImpl::onTouchMove(void* /* data */, Evas* /* canvas */, Evas_Object* ewkView, void* /* eventInfo */)
{
    EwkViewImpl* viewImpl = EwkViewImpl::fromEvasObject(ewkView);
    viewImpl->feedTouchEvents(EWK_TOUCH_MOVE);
}
#endif

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
AffineTransform EwkViewImpl::transformFromView() const
{
    AffineTransform transform;
    transform.scale(1 / m_scaleFactor);
    transform.translate(m_scrollPosition.x(), m_scrollPosition.y());

    return transform;
}

AffineTransform EwkViewImpl::transformToView() const
{
    return transformFromView().inverse();
}
#endif

AffineTransform EwkViewImpl::transformFromScene() const
{
    AffineTransform transform;

#if USE(TILED_BACKING_STORE)
    // FIXME: We have to scale firstly unlike open source, because we are using scaled scroll position.
    //transform.translate(m_scrollPosition.x(), m_scrollPosition.y());
    //transform.scale(1 / m_scaleFactor);
    transform.scale(1 / m_scaleFactor);
    transform.translate(m_scrollPosition.x(), m_scrollPosition.y());
#endif

    Ewk_View_Smart_Data* sd = smartData();
    transform.translate(-sd->view.x, -sd->view.y);

    return transform;
}

AffineTransform EwkViewImpl::transformToScene() const
{
    return transformFromScene().inverse();
}

AffineTransform EwkViewImpl::transformToScreen() const
{
    AffineTransform transform;

    int windowGlobalX = 0;
    int windowGlobalY = 0;

    Ewk_View_Smart_Data* sd = smartData();

#ifdef HAVE_ECORE_X
    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(sd->base.evas);
    const char* engine = ecore_evas_engine_name_get(ecoreEvas);
    Ecore_X_Window window = 0;
#if USE(ACCELERATED_COMPOSITING)
    if (engine && !strcmp(engine, "opengl_x11"))
        window = ecore_evas_gl_x11_window_get(ecoreEvas);
    if (!window)
#endif
    {
        if (engine && !strcmp(engine, "software_x11"))
            window = ecore_evas_software_x11_window_get(ecoreEvas);
    }

    int x, y; // x, y are relative to parent (in a reparenting window manager).
    while (window) {
        ecore_x_window_geometry_get(window, &x, &y, 0, 0);
        windowGlobalX += x;
        windowGlobalY += y;
        window = ecore_x_window_parent_get(window);
    }
#endif

    transform.translate(-sd->view.x, -sd->view.y);
    transform.translate(windowGlobalX, windowGlobalY);

    return transform;
}

#if ENABLE(TIZEN_WEBKIT2_SEPERATE_LOAD_PROGRESS)
void EwkViewImpl::informLoadProgressStarted()
{
    evas_object_smart_callback_call(m_view, "load,progress,started", 0);
}

void EwkViewImpl::informLoadProgressFinished()
{
    evas_object_smart_callback_call(m_view, "load,progress,finished", 0);
}
#endif

#if ENABLE(TIZEN_DATALIST_ELEMENT)
void EwkViewImpl::deleteDataList()
{
    if (!dataList)
        return;

    void* item;
    EINA_LIST_FREE(dataList, item)
        eina_stringshare_del(static_cast<char*>(item));

    dataList = 0;
}
#endif

#if ENABLE(TIZEN_GEOLOCATION)
bool EwkViewImpl::isValidLocationService()
{
    TIZEN_LOGI("geolocation,valid");

    bool valid = true;
    evas_object_smart_callback_call(m_view, "geolocation,valid", &valid);
    return valid;
}
#endif

#if ENABLE(TIZEN_FOCUS_UI)
bool EwkViewImpl::canTakeFocus(WKFocusDirection direction)
{
    if (!unfocusAllowContext || !unfocusAllowContext->unfocusAllowCallback)
        return false;

    if (unfocusAllowContext->ewkView != m_view)
        return false;

    return unfocusAllowContext->unfocusAllowCallback(m_view, static_cast<Ewk_Unfocus_Direction>(direction), unfocusAllowContext->userData);
}
#endif

#if ENABLE(TIZEN_GESTURE)
#if ENABLE(TOUCH_EVENTS)
void EwkViewImpl::feedTouchEventsByType(Ewk_Touch_Event_Type type)
{
    feedTouchEvents(type);
}
#endif

void EwkViewImpl::setDoubleTapEnabled(bool enabled)
{
    gestureRecognizer->setDoubleTapEnabled(enabled);
}
#endif

#if ENABLE(TIZEN_FOCUS_UI)
void EwkViewImpl::pages(Vector<RefPtr<WebPageProxy> >& pages)
{
    pages.resize(pageViewMap.size());

    PageViewMap::const_iterator::Keys it = pageViewMap.begin().keys();
    PageViewMap::const_iterator::Keys end = pageViewMap.end().keys();
    for (unsigned i = 0; it != end; ++it, ++i)
        pages[i] = toImpl(*it);
}

static void handleDeferedKeyDownEvent(WebPageProxy* page, InputMethodContextEfl* inputMethodContext)
{
    Evas_Event_Key_Down downEvent;
    memset(&downEvent, 0, sizeof(Evas_Event_Key_Down));

    downEvent.keyname = const_cast<char*>("Return");
    downEvent.key = const_cast<char*>("Return");
    downEvent.string = const_cast<char*>("\r");
    downEvent.compose = const_cast<char*>("\r");

    bool isFiltered = false;
    if (inputMethodContext)
        inputMethodContext->handleKeyDownEvent(&downEvent, &isFiltered);

    NativeWebKeyboardEvent nativeEvent(&downEvent, isFiltered);
    nativeEvent.setInputMethodContextID(page->editorState().inputMethodContextID);
    page->handleKeyboardEvent(nativeEvent);
}

bool EwkViewImpl::shouldHandleKeyDownEvent(const Evas_Event_Key_Down* downEvent)
{
    if (!page()->focusUIEnabled() || (strcmp(downEvent->keyname, "Return") && strcmp(downEvent->keyname, "KP_Enter")))
        return true;

    const EditorState& editorState = page()->editorState();
    if (editorState.isContentRichlyEditable || (editorState.inputMethodHints ==  "textarea"))
        return true;

    if (!m_keyDownEventDeferred && !m_deferKeyDownEventTimer.isActive()) {
        m_deferKeyDownEventTimer.startOneShot(1);
        m_keyDownEventDeferred = true;
    }

    return false;
}

bool EwkViewImpl::shouldHandleKeyUpEvent(const Evas_Event_Key_Up* upEvent)
{
    if (!page()->focusUIEnabled() || (strcmp(upEvent->keyname, "Return") && strcmp(upEvent->keyname, "KP_Enter")))
        return true;

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    if (!strcmp(upEvent->keyname, "Return") || !strcmp(upEvent->keyname, "KP_Enter"))
        pageProxy->setContextMenuFocusable();
#endif

    m_keyDownEventDeferred = false;

    if (!m_deferKeyDownEventTimer.isActive())
        return false;

    m_deferKeyDownEventTimer.stop();
    handleDeferedKeyDownEvent(page(), m_inputMethodContext.get());
    return true;
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void EwkViewImpl::hideCandidatePopup()
{
    Ewk_View_Smart_Data* sd = smartData();
    EINA_SAFETY_ON_NULL_RETURN(sd->api);
    EINA_SAFETY_ON_NULL_RETURN(sd->api->formdata_candidate_hide);

    sd->api->formdata_candidate_hide(sd);
}
#endif

void EwkViewImpl::deferKeyDownEventTimerFired(WebCore::Timer<EwkViewImpl>*)
{
    if (page()->editorState().isContentEditable)
        return;

    Ewk_View_Smart_Data* sd = smartData();
    EINA_SAFETY_ON_NULL_RETURN(sd->api);
    EINA_SAFETY_ON_NULL_RETURN(sd->api->gesture_start);

    IntPoint point = m_focusRing->centerPointInScreen();

    Ewk_Event_Gesture gestureEvent;
    memset(&gestureEvent, 0, sizeof(Ewk_Event_Gesture));
    gestureEvent.type = EWK_GESTURE_LONG_PRESS;
    gestureEvent.position.x = point.x();
    gestureEvent.position.y = point.y();
    gestureEvent.scale = 1;
    gestureEvent.count = 1;
    gestureEvent.timestamp = ecore_time_get() * 1000;

    wasHandledTouchStart = false;
    gestureClient->setGestureEnabled(true);
    sd->api->gesture_start(sd, &gestureEvent);
}
#endif

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
void EwkViewImpl::saveSessionData() const
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[BackForward] save,session,data m_view = %x", m_view);
#endif
    if(m_view)
        evas_object_smart_callback_call(m_view, "save,session,data", 0);
}
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
void EwkViewImpl::hwVideoOverlayEnabled()
{
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    m_focusRing->hide(false);
#endif
#if ENABLE(TIZEN_FOCUS_UI)
    page()->setFocusUIEnabled(false);
#endif

    evas_object_smart_callback_call(m_view, "video,hwoverlay,enabled", 0);
}

void EwkViewImpl::hwVideoOverlayDisabled()
{
#if ENABLE(TIZEN_FOCUS_UI)
    Evas_Object* win = elm_object_top_widget_get(elm_object_parent_widget_get(m_view));
    const char* type = evas_object_type_get(win);
    if (type && !strcmp(type, "elm_win") && elm_win_focus_highlight_enabled_get(win))
        elm_win_focus_highlight_enabled_set(win, false);
#endif

    evas_object_smart_callback_call(m_view, "video,hwoverlay,disabled", 0);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
void EwkViewImpl::setFocusManually()
{
    if (evas_object_focus_get(m_view))
        return;

    isRunningSetFocusManually = true;
    evas_object_focus_set(m_view, true);
    isRunningSetFocusManually = false;
}
#endif

IntRect EwkViewImpl::selectionRect(bool toScene)
{
#if ENABLE(TIZEN_ISF_PORT)
    return toScene ? transformToScene().mapRect(page()->editorState().selectionRect) : page()->editorState().selectionRect;
#endif
    return IntRect();
}

IntRect EwkViewImpl::caretRect(bool toScene)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION) && ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    return toScene ? transformToScene().mapRect(page()->editorState().caretRect) : page()->editorState().caretRect;
#endif
    return IntRect();
}

IntRect EwkViewImpl::focusedNodeRect(bool toScene)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION) && ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    return toScene ? transformToScene().mapRect(page()->editorState().focusedNodeRect) : page()->editorState().focusedNodeRect;
#endif
    return IntRect();
}

#if ENABLE(TIZEN_BACKGROUND_COLOR_API)
void EwkViewImpl::setBackgroundColor(int red, int green, int blue, int alpha)
{
    if (red == 255 && green == 255 && blue == 255 && alpha == 255)
        page()->setDrawsBackground(true);
    else
        page()->setDrawsBackground(false);

    int objectAlpha;
    Evas_Object* image = smartData()->image;
    evas_object_color_get(image, nullptr, nullptr, nullptr, &objectAlpha);
    evas_object_image_alpha_set(image, alpha < 255 || objectAlpha < 255);

    pageClient->setBackgroundColor(WebCore::Color(red, green, blue, alpha));
}

void EwkViewImpl::getBackgroundColor(int* red, int* green, int* blue, int* alpha)
{
    WebCore::Color color = pageClient->backgroundColor();

    if (red)
        *red = color.red();
    if (green)
        *green = color.green();
    if (blue)
        *blue = color.blue();
    if (alpha)
        *alpha = color.alpha();
}
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
unsigned EwkViewImpl::getXWindow()
{
    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(smartData()->base.evas);
    const char* engine = ecore_evas_engine_name_get(ecoreEvas);

    if (!strcmp(engine, "opengl_x11"))
        return ecore_evas_gl_x11_window_get(ecoreEvas);

    if (!strcmp(engine, "software_x11"))
        return ecore_evas_software_x11_window_get(ecoreEvas);

    return 0;
}
#endif

#endif //#if PLATFORM(TIZEN)
