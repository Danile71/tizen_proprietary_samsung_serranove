/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "PageUIClientEfl.h"

#include "EwkViewImpl.h"
#include "WKAPICast.h"
#include "WKEvent.h"
#include "WKString.h"
#include <Ecore_Evas.h>
#include <WebCore/Color.h>

#if PLATFORM(TIZEN)
#include "WKGeolocationPermissionRequest.h"
#include "WKNotificationPermissionRequest.h"
#include "WKPage.h"
#include "WKRetainPtr.h"
#include "WKSecurityOrigin.h"
#include "ewk_geolocation_private.h"
#include "ewk_notification.h"
#include "ewk_notification_private.h"
#include "ewk_view_private.h"
#include "ewk_view.h"
#endif

namespace WebKit {

static inline PageUIClientEfl* toPageUIClientEfl(const void* clientInfo)
{
    return static_cast<PageUIClientEfl*>(const_cast<void*>(clientInfo));
}

void PageUIClientEfl::closePage(WKPageRef, const void* clientInfo)
{
    toPageUIClientEfl(clientInfo)->m_viewImpl->closePage();
}

WKPageRef PageUIClientEfl::createNewPage(WKPageRef, WKURLRequestRef, WKDictionaryRef, WKEventModifiers, WKEventMouseButton, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_viewImpl->createNewPage();
}

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
void PageUIClientEfl::notifyPopupReplyWaitingState(WKPageRef, bool isWaiting, const void* clientInfo)
{
    ewkViewNotifyPopupReplyWaitingState(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), isWaiting);
}
#endif

bool PageUIClientEfl::runJavaScriptAlert(WKPageRef, WKStringRef alertText, WKFrameRef, const void* clientInfo)
{
    return ewkViewRunJavaScriptAlert(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), alertText);
}

bool PageUIClientEfl::runJavaScriptConfirm(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    return ewkViewRunJavaScriptConfirm(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), message);
}

bool PageUIClientEfl::runJavaScriptPrompt(WKPageRef, WKStringRef message, WKStringRef defaultValue, WKFrameRef, const void* clientInfo)
{
    return ewkViewRunJavaScriptPrompt(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), message, defaultValue);
}
#else
void PageUIClientEfl::runJavaScriptAlert(WKPageRef, WKStringRef alertText, WKFrameRef, const void* clientInfo)
{
    toPageUIClientEfl(clientInfo)->m_viewImpl->requestJSAlertPopup(WKEinaSharedString(alertText));
}

bool PageUIClientEfl::runJavaScriptConfirm(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_viewImpl->requestJSConfirmPopup(WKEinaSharedString(message));
}

WKStringRef PageUIClientEfl::runJavaScriptPrompt(WKPageRef, WKStringRef message, WKStringRef defaultValue, WKFrameRef, const void* clientInfo)
{
    WKEinaSharedString value = toPageUIClientEfl(clientInfo)->m_viewImpl->requestJSPromptPopup(WKEinaSharedString(message), WKEinaSharedString(defaultValue));
    return value ? WKStringCreateWithUTF8CString(value) : 0;
}
#endif

#if ENABLE(INPUT_TYPE_COLOR)
void PageUIClientEfl::showColorPicker(WKPageRef, WKStringRef initialColor, WKColorPickerResultListenerRef listener, const void* clientInfo)
{
    PageUIClientEfl* pageUIClient = toPageUIClientEfl(clientInfo);
    WebCore::Color color = WebCore::Color(WebKit::toWTFString(initialColor));
    pageUIClient->m_viewImpl->requestColorPicker(color.red(), color.green(), color.blue(), color.alpha(), listener);
}

void PageUIClientEfl::hideColorPicker(WKPageRef, const void* clientInfo)
{
    PageUIClientEfl* pageUIClient = toPageUIClientEfl(clientInfo);
    pageUIClient->m_viewImpl->dismissColorPicker();
}
#endif

#if ENABLE(SQL_DATABASE)
#if ENABLE(TIZEN_SQL_DATABASE)
bool PageUIClientEfl::exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef origin, WKStringRef displayName, unsigned long long expectedUsage, const void *clientInfo)
{
    return ewkViewExceededDatabaseQuota(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), origin, displayName, expectedUsage);
}
#else
unsigned long long PageUIClientEfl::exceededDatabaseQuota(WKPageRef, WKFrameRef, WKSecurityOriginRef, WKStringRef databaseName, WKStringRef displayName, unsigned long long currentQuota, unsigned long long currentOriginUsage, unsigned long long currentDatabaseUsage, unsigned long long expectedUsage, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageUIClientEfl(clientInfo)->m_viewImpl;
    return viewImpl->informDatabaseQuotaReached(toImpl(databaseName)->string(), toImpl(displayName)->string(), currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);
}
#endif
#endif

void PageUIClientEfl::focus(WKPageRef, const void* clientInfo)
{
    evas_object_focus_set(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), true);
}

void PageUIClientEfl::unfocus(WKPageRef, const void* clientInfo)
{
    evas_object_focus_set(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), false);
}

#if ENABLE(TIZEN_FOCUS_UI)
bool PageUIClientEfl::canTakeFocus(WKPageRef, WKFocusDirection direction, const void* clientInfo)
{
    return toPageUIClientEfl(clientInfo)->m_viewImpl->canTakeFocus(direction);
}
#endif

void PageUIClientEfl::takeFocus(WKPageRef, WKFocusDirection direction, const void* clientInfo)
{
    // FIXME: this is only a partial implementation.
    evas_object_focus_set(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), false);
#if ENABLE(TIZEN_FOCUS_UI)
    evas_object_smart_callback_call(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), "unfocus,direction", &direction);
#endif
}

WKRect PageUIClientEfl::getWindowFrame(WKPageRef, const void* clientInfo)
{
    int x, y, width, height;

#if ENABLE(TIZEN_VIEWPORT_META_TAG)
    ewkViewGetWindowFrame(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), &x, &y, &width, &height);
    return WKRectMake(x, y, width, height);
#endif
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_object_evas_get(toPageUIClientEfl(clientInfo)->m_viewImpl->view()));
    ecore_evas_request_geometry_get(ee, &x, &y, &width, &height);

    return WKRectMake(x, y, width, height);
}

void PageUIClientEfl::setWindowFrame(WKPageRef, WKRect frame, const void* clientInfo)
{
#if PLATFORM(TIZEN)
    // FIXME : It is not required just for Mobile Browser. But the solution for desktop browser is required.
    return;
#endif
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_object_evas_get(toPageUIClientEfl(clientInfo)->m_viewImpl->view()));
    ecore_evas_move_resize(ee, frame.origin.x, frame.origin.y, frame.size.width, frame.size.height);
}

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
bool PageUIClientEfl::runBeforeUnloadConfirmPanel(WKPageRef, WKStringRef message, WKFrameRef, const void* clientInfo)
{
    return ewk_view_run_before_unload_confirm_panel(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), message);
}
#endif

bool PageUIClientEfl::runOpenPanel(WKPageRef, WKFrameRef, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener, const void* clientInfo)
{
    return ewkViewRunOpenPanel(toPageUIClientEfl(clientInfo)->m_viewImpl->view(), parameters, listener);
}

void PageUIClientEfl::decidePolicyForGeolocationPermissionRequest(WKPageRef page, WKFrameRef, WKSecurityOriginRef origin, WKGeolocationPermissionRequestRef permissionRequest, const void* clientInfo)
{
#if ENABLE(TIZEN_GEOLOCATION)
    Evas_Object* ewkView = toPageUIClientEfl(clientInfo)->m_viewImpl->view();
    Ewk_Geolocation_Permission_Request* geolocationPermissionRequest = ewkGeolocationCreatePermissionRequest(page, permissionRequest, origin);

    if (!ewkViewRequestGeolocationPermission(ewkView, geolocationPermissionRequest))
        WKGeolocationPermissionRequestDeny(permissionRequest);
#endif
}

#if ENABLE(TIZEN_NOTIFICATIONS)
void PageUIClientEfl::decidePolicyForNotificationPermissionRequest(WKPageRef page, WKSecurityOriginRef origin, WKNotificationPermissionRequestRef permissionRequest, const void* clientInfo)
{
    Evas_Object* ewkView = toPageUIClientEfl(clientInfo)->m_viewImpl->view();
    Ewk_Notification_Permission_Request* ewkNotificationPermissionRequest = ewkNotificationCreatePermissionRequest(page, permissionRequest, origin);

    if (!ewkViewRequestNotificationPermission(ewkView, ewkNotificationPermissionRequest))
        WKNotificationPermissionRequestDeny(ewkNotificationGetWKNotificationPermissionRequest(ewkNotificationPermissionRequest));
}
#endif
#endif // #if PLATFORM(TIZEN)

PageUIClientEfl::PageUIClientEfl(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
{
    WKPageRef pageRef = m_viewImpl->wkPage();
    ASSERT(pageRef);

    WKPageUIClient uiClient;
    memset(&uiClient, 0, sizeof(WKPageUIClient));
    uiClient.version = kWKPageUIClientCurrentVersion;
    uiClient.clientInfo = this;
    uiClient.close = closePage;
    uiClient.createNewPage = createNewPage;
    uiClient.runJavaScriptAlert = runJavaScriptAlert;
    uiClient.runJavaScriptConfirm = runJavaScriptConfirm;
    uiClient.runJavaScriptPrompt = runJavaScriptPrompt;
#if ENABLE(TIZEN_FOCUS_UI)
    uiClient.canTakeFocus = canTakeFocus;
#endif
    uiClient.takeFocus = takeFocus;
    uiClient.focus = focus;
    uiClient.unfocus = unfocus;
    uiClient.getWindowFrame = getWindowFrame;
    uiClient.setWindowFrame = setWindowFrame;
#if ENABLE(SQL_DATABASE)
    uiClient.exceededDatabaseQuota = exceededDatabaseQuota;
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    uiClient.showColorPicker = showColorPicker;
    uiClient.hideColorPicker = hideColorPicker;
#endif

#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
    uiClient.notifyPopupReplyWaitingState = notifyPopupReplyWaitingState;
#endif

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
    uiClient.runBeforeUnloadConfirmPanel = runBeforeUnloadConfirmPanel;
#endif
    uiClient.runOpenPanel = runOpenPanel;
    uiClient.decidePolicyForGeolocationPermissionRequest = decidePolicyForGeolocationPermissionRequest;
#if ENABLE(TIZEN_NOTIFICATIONS)
    uiClient.decidePolicyForNotificationPermissionRequest = decidePolicyForNotificationPermissionRequest;
#endif
#endif

    WKPageSetPageUIClient(pageRef, &uiClient);
}

} // namespace WebKit
