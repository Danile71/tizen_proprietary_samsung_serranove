/*
 * Copyright (C) 2012 Samsung Electronics
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
 *
 */

#ifndef ewk_view_private_h
#define ewk_view_private_h

#include <tizen.h>
#include <Eina.h>
#include <Evas.h>
#include <WebKit2/WKBase.h>

Evas_Object* ewk_view_base_add(Evas* canvas, WKContextRef, WKPageGroupRef);

#if PLATFORM(TIZEN)
#include "WKPage.h"
#include "WebEvent.h"
#include "ewk_auth_challenge.h"
#include "ewk_context_menu.h"
#include "ewk_form_data.h"
#include "ewk_policy_decision.h"
#include "ewk_view.h"
#include <JavaScriptCore/JSValueRef.h>
#include <WebCore/TextDirection.h>
#include <wtf/Vector.h>

#if ENABLE(TIZEN_REGISTER_PROTOCOL_HANDLER) || ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)
#include "ewk_custom_handlers.h"
#endif

#if ENABLE(TIZEN_GEOLOCATION)
#include "ewk_geolocation.h"
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "ewk_user_media.h"
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "ewk_notification.h"
#include "ewk_notification_private.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
#include "ewk_text_style.h"
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
#include "ewk_certificate.h"
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
#include "ewk_content_screening_detection.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#include "ewk_touch.h"
#endif

#if ENABLE(TIZEN_INTERCEPT_REQUEST)
#include "ewk_intercept_request.h"
#endif

namespace WebCore {
class IntPoint;
}

namespace WebKit {
class WebPopupItem;
class WebPopupMenuProxyEfl;
#if ENABLE(TIZEN_INPUT_COLOR_PICKER)
class WebColorChooserProxyEfl;
#endif // ENABLE(TIZEN_INPUT_COLOR_PICKER)
class PageClientImpl;
}

typedef struct Ewk_Context Ewk_Context;

bool ewk_view_focused_node_adjust(Evas_Object* object, Eina_Bool adjustForExternalKeyboard = EINA_FALSE, Eina_Bool adjustForContentSizeChanged = EINA_FALSE);

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
void ewk_view_form_navigate(Evas_Object* ewkView, bool direction);
void ewk_view_form_navigation_disable(EwkViewImpl* impl);
#endif

void ewk_view_touch_event_handler_result_set(Evas_Object* ewkView, WebKit::WebEvent::Type type, bool wasHandled);

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
bool ewk_view_image_native_surface_set(Evas_Object* ewkView, Evas_Native_Surface* nativeSurface);
void ewk_view_mark_for_sync(Evas_Object* ewkView);
void ewk_view_data_update_add(Evas_Object* ewkView, int, int, int, int);
#endif

#if (ENABLE_TIZEN_REGISTER_PROTOCOL_HANDLER) || (ENABLE_TIZEN_REGISTER_CONTENT_HANDLER) || (ENABLE_TIZEN_CUSTOM_SCHEME_HANDLER)
Ewk_Custom_Handlers_Data* ewkCustomHandlersCreateData(const char* target, const char* baseUrl, const char* url, const char* title);
Ewk_Custom_Handlers_Data* ewkCustomHandlersCreateData(const char* target, const char* baseUrl, const char* url);
Ewk_Custom_Handlers_State ewkGetCustomHandlersDataResult(Ewk_Custom_Handlers_Data* customHandlersData);
void ewkCustomHandlersDeleteData(Ewk_Custom_Handlers_Data* customHandlersData);
#endif
#if ENABLE(TIZEN_REGISTER_PROTOCOL_HANDLER)
void ewkViewRegisterProtocolHandlers(Evas_Object* ewkView, const char* scheme, const char* baseURL, const char* url, const char* title);
#endif
#if ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)
Ewk_Custom_Handlers_State ewkViewIsProtocolHandlerRegistered(Evas_Object* ewkView, const char* scheme, const char* baseURL, const char* url);
void ewkViewUnregisterProtocolHandlers(Evas_Object* ewkView, const char* scheme, const char* baseUrl, const char* url);
#endif
#if ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)
void ewkViewRegisterContentHandlers(Evas_Object* ewkView, const char* mimeType, const char* baseURL, const char* url, const char* title);
Ewk_Custom_Handlers_State ewkViewIsContentHandlerRegistered(Evas_Object* ewkView, const char* mimeType, const char* baseURL, const char* url);
void ewkViewUnregisterContentHandlers(Evas_Object* ewkView, const char* mimeType, const char* baseUrl, const char* url);
#endif
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
bool ewkViewGetStandaloneStatus(Evas_Object* ewkView);
#endif

Evas_Object* ewk_view_touchscreen_add(Evas* canvas, WKContextRef, WKPageGroupRef);
WebKit::PageClientImpl* ewkViewGetPageClient(const Evas_Object* ewkView);

void ewkViewLoadCommitted(Evas_Object* ewkView);
#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
void ewkViewLoadError(Evas_Object* ewkView, WKErrorRef error);
#endif
void ewkViewDidFirstVisuallyNonEmptyLayout(Evas_Object* ewkView);
void ewkViewDidReceiveAuthenticationChallenge(Evas_Object* ewkView, Ewk_Auth_Challenge* authChallenge);
void ewk_view_process_crashed(Evas_Object* ewkView);
void ewkViewCustomizeContextMenu(Evas_Object* ewkView, Ewk_Context_Menu* menu);
void ewkViewCustomContextMenuItemSelected(Evas_Object* ewkView, Ewk_Context_Menu_Item* item);
uint64_t ewkContextGetDatabaseQuota(Ewk_Context* ewkContext);
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
void ewkViewNotifyPopupReplyWaitingState(Evas_Object* ewkView, bool isWaiting);
#endif
bool ewkViewRunJavaScriptAlert(Evas_Object* ewkView, WKStringRef alertText);
bool ewkViewRunJavaScriptConfirm(Evas_Object* ewkView, WKStringRef message);
bool ewkViewRunJavaScriptPrompt(Evas_Object* ewkView, WKStringRef message, WKStringRef defaultValue);
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
bool ewk_view_run_before_unload_confirm_panel(Evas_Object* ewkView, WKStringRef message);
#endif
bool ewkViewRunOpenPanel(Evas_Object* ewkView, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener);
void ewkViewClosePage(Evas_Object* ewkView);
WKPageRef ewkViewCreateNewPage(Evas_Object* ewkView);
JSGlobalContextRef ewkViewGetJavascriptGlobalContext(Evas_Object* ewkView);
void ewkViewFormSubmit(Evas_Object* ewkView, Ewk_Form_Data* formData);
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
Ewk_Form_Data* ewkFormDataCreate(WKFrameRef frame, WKFormDataRef formDataRef);
#else
Ewk_Form_Data* ewkFormDataCreate(WKFrameRef frame, WKDictionaryRef values);
#endif
void ewkFormDataDelete(Ewk_Form_Data* formData);
Ewk_Policy_Decision* ewkPolicyDecisionCreate(Evas_Object* ewkView, Ewk_Policy_Callback_Type callbackType, WKFramePolicyListenerRef listener, WKURLRequestRef request, WKFrameRef frame, WKURLResponseRef response, WKFrameNavigationType navigationType = kWKFrameNavigationTypeOther);
#if ENABLE(TIZEN_INTERCEPT_REQUEST)
Ewk_Intercept_Request* ewkInterceptDecisionCreate(WKURLRequestRef request, WKPageRef page, WKFrameRef frame, WKStringRef body, uint64_t listenerID);
void ewkInterceptRequestDelete(Ewk_Intercept_Request* interceptRequest);
bool ewkInterceptRequestDecided(Ewk_Intercept_Request* interceptRequest);
#endif
void ewkPolicyDecisionDelete(Ewk_Policy_Decision* policyDecision);
bool ewkPolicyDecisionDecided(Ewk_Policy_Decision* policyDecision);
bool ewkPolicyDecisionSuspended(Ewk_Policy_Decision* policyDecision);
void ewkViewPolicyNavigationDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision);
void ewkViewPolicyNewWindowDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision);
void ewkViewPolicyResponseDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision);

void ewkViewSendScrollEvent(Evas_Object* ewkView, int deltaX, int deltaY, int xVelocity, int yVelocity);
void ewkViewSendEdgeEvent(Evas_Object* ewkView, const WebCore::IntPoint& scrollPosition, int deltaX, int deltaY);
void ewkViewClearEdges(Evas_Object* ewkView);

void ewkViewFrameRendered(Evas_Object* ewkView);

#if ENABLE(TIZEN_SQL_DATABASE)
bool ewkViewExceededDatabaseQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, WKStringRef displayName, unsigned long long expectedUsage);
unsigned long long ewkContextGetNewQuotaForExceededQuota(Ewk_Context* context, Ewk_Context_Exceeded_Quota* exceededQuota);
#endif

#if ENABLE(TIZEN_GEOLOCATION)
bool ewkViewRequestGeolocationPermission(Evas_Object* ewkView, Ewk_Geolocation_Permission_Request*);
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
void ewkViewCancelNotification(Evas_Object* ewkView, uint64_t notificationID);
bool ewkViewRequestNotificationPermission(Evas_Object* ewkView, Ewk_Notification_Permission_Request*);
void ewkViewShowNotification(Evas_Object* ewkView, Ewk_Notification*);
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE_BACKUP_IMAGE)
void ewk_view_backup_image_set(Evas_Object* ewkView);
bool ewk_view_backup_image_show(Evas_Object* ewkView);
bool ewk_view_backup_image_hide(Evas_Object* ewkView);
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
bool ewkViewRequestUserMediaPermission(Evas_Object* ewkView, Ewk_User_Media_Permission_Request*);
#endif

#if ENABLE(TIZEN_JSBRIDGE_PLUGIN)
void ewkViewProcessJSBridgePlugin(Evas_Object* ewkView, WKStringRef request, WKStringRef message);
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
void ewkViewInputPickerRequest(Evas_Object* ewkView, Ewk_Input_Type inputType, const String& inputValue);
#endif

#if ENABLE(TIZEN_VIEWPORT_META_TAG)
void ewkViewGetWindowFrame(Evas_Object* ewkView, int *x, int *y, int *w, int *h);
#endif

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
void  ewkViewTextStyleState(Evas_Object* ewkView, const WebCore::IntPoint& startPoint, const WebCore::IntPoint& endPoint);
Ewk_Text_Style* ewkTextStyleCreate(const WebKit::EditorState editorState, const WebCore::IntPoint& startPoint, const WebCore::IntPoint& endPoint);
void ewkTextStyleDelete(Ewk_Text_Style* textStyle);
#endif

#if ENABLE(SCREEN_ORIENTATION_SUPPORT) && ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
bool ewk_view_orientation_lock(Evas_Object* ewkView, int willLockOrientation);
void ewk_view_orientation_unlock(Evas_Object* ewkView);
#endif

void ewk_view_zoomable_area_set(Evas_Object*, const WebCore::IntPoint&, const WebCore::IntRect&);

#if ENABLE(TIZEN_ICON_DATABASE)
void ewkViewIconReceived(Evas_Object* ewkView);
#endif

#endif // #if PLATFORM(TIZEN)

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
void ewkViewRequestCertificateConfirm(Evas_Object* ewkView, Ewk_Certificate_Policy_Decision* certificatePolicyDecision);
void ewkViewSetCertificatePem(Evas_Object* ewkView, const char* certificate);
Ewk_Certificate_Policy_Decision* ewkCertificatePolicyDecisionCreate(WKPageRef page, WKStringRef url, WKStringRef certificate, int error);
void ewkCertificatePolicyDecisionDelete(Ewk_Certificate_Policy_Decision* certificatePolicyDecision);
bool ewkCertificatePolicyDecisionSuspended(Ewk_Certificate_Policy_Decision* certificatePolicyDecision);
bool ewkCertificatePolicyDecisionDecided(Ewk_Certificate_Policy_Decision* certificatePolicyDecision);
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
void ewkViewRequestBlockedLoadingConfirm(Evas_Object* ewkView, Ewk_Content_Screening_Detection* contentScreeningDetection);
Ewk_Content_Screening_Detection* ewkContentScreeningDetectionCreate(WKPageRef page, WKErrorRef error);
void ewkContentScreeningDetectionDelete(Ewk_Content_Screening_Detection* contentScreeningDetection);
bool ewkContentScreeningDetectionSuspended(Ewk_Content_Screening_Detection* contentScreeningDetection);
bool ewkContentScreeningDetectionDecided(Ewk_Content_Screening_Detection* contentScreeningDetection);
#endif

void ewk_view_suspend_painting(Evas_Object* ewkView);
void ewk_view_resume_painting(Evas_Object* ewkView);

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
bool ewk_view_is_opengl_backend(Evas_Object* ewkView);
#endif

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
void ewkViewAddMessageToConsole(Evas_Object* ewkView, unsigned level, const String& message, unsigned int lineNumber, const String& source);
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
Eina_Bool ewkViewRequestApplicationCachePermission(Evas_Object* ewkView, WKSecurityOriginRef origin);
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
bool ewkViewExceededIndexedDatabaseQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, long long currentUsage);
#endif

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
void ewk_view_popup_menu_update(Evas_Object* ewkView, WebCore::TextDirection textDirection, const Vector<WebKit::WebPopupItem>& items, int32_t selectedIndex);
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
EXPORT_API Eina_Bool ewk_view_script_execute_in_focused_frame(Evas_Object* ewkView, const char* script, Ewk_View_Script_Execute_Callback callback, void* user_data);

void ewk_view_form_password_data_fill(Evas_Object* ewkView);
void ewk_view_form_data_add(Evas_Object* ewkView, WKFormDataRef formData);
void ewk_view_form_candidate_data_get(Evas_Object* ewkView, const String& name, Vector<String>& candidates);

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
void ewk_view_text_change_in_textfield(Evas_Object* ewkView, const String& name, const String& value, bool isInputInForm, bool isForcePopupShow, bool isHandleEvent);
void ewk_view_profile_form_candidate_data_get(Evas_Object* ewkView, const String& name, const String& value, Vector<std::pair<int, std::pair<String, String> > > & candidates);
void ewk_view_profile_form_candidate_set_to_form(Evas_Object* ewkView, const int& profileID);
#else
void ewk_view_text_change_in_textfield(Evas_Object* ewkView, const String& name, const String& value);
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
bool ewkViewExceededLocalFileSystemQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, long long currentUsage);
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void ewkViewMediaControlsRequestRotate(Evas_Object* o, const String& status);
bool ewkViewOpenMediaPlayer(Evas_Object* ewkView, const String& url, const String& cookies, const String& type);
#endif

#endif // ewk_view_private_h
