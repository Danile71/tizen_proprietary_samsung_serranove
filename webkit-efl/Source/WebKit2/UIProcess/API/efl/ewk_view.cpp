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
#include "ewk_view.h"

#include "EwkViewImpl.h"
#include "FindClientEfl.h"
#include "FormClientEfl.h"
#include "LayerTreeCoordinatorProxy.h"
#include "LocalizedStrings.h"
#include "InputMethodContextEfl.h"
#include "NativeWebKeyboardEvent.h"
#include "NativeWebMouseEvent.h"
#include "NativeWebWheelEvent.h"
#include "PageClientImpl.h"
#include "PageLoadClientEfl.h"
#include "PagePolicyClientEfl.h"
#include "PageUIClientEfl.h"
#include "ResourceLoadClientEfl.h"
#include "WKAPICast.h"
#include "WKEinaSharedString.h"
#include "WKFindOptions.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "WebContext.h"
#include "WebData.h"
#include "WebPageGroup.h"
#include "WebPopupItem.h"
#include "WebPopupMenuProxyEfl.h"
#include "WebPreferences.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_context.h"
#include "ewk_context_private.h"
#include "ewk_favicon_database_private.h"
#include "ewk_intent_private.h"
#include "ewk_popup_menu_item_private.h"
#include "ewk_private.h"
#include "ewk_settings_private.h"
#include "ewk_view_private.h"
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <WebKit2/WKPageGroup.h>
#include <wtf/text/CString.h>

#if ENABLE(FULLSCREEN_API)
#include "WebFullScreenManagerProxy.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_DDK_CHECK)
namespace EGL {
#include <EGL/egl.h>
}
#endif

#if PLATFORM(TIZEN)
#include "DrawingAreaProxyImpl.h"
#include "WKArray.h"
#include "WKData.h"
#include "WKDownload.h"
#include "WKError.h"
#include "WKGeolocationPermissionRequest.h"
#include "WKImageCairo.h"
#include "WKOpenPanelParameters.h"
#include "WKOpenPanelResultListener.h"
#include "WKPage.h"
#include "WKPageGroup.h"
#include "WKView.h"
#include "WKPageTizen.h"
#include "WKPreferences.h"
#include "WKSerializedScriptValue.h"
#include "WKString.h"
#include "WKURLRequest.h"
#include "ewk_context_menu_private.h"
#include "ewk_error.h"
#include "ewk_error_private.h"
#include "ewk_history_private.h"
#include "ewk_popup_menu_item.h"
#include "ewk_popup_menu_item_private.h"
#include "ewk_view_context_menu_client.h"
#include "ewk_view_icondatabase_client.h"
#include "ewk_view_tizen_client.h"
#include "ewk_view_utilx.h"
#include <Ecore.h>
#include <Elementary.h>
#include <WebCore/EflScreenUtilities.h>
#include <WebCore/NotImplemented.h>
#include <cairo.h>

#if ENABLE(TIZEN_ICON_DATABASE)
#include "WKContextPrivate.h"
#endif

#if ENABLE(TIZEN_GEOLOCATION)
#include "GeolocationPermissionPopup.h"
#include "ewk_geolocation_private.h"
#include "ewk_geolocation_provider_private.h"
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
#include "ApplicationCachePermissionPopup.h"
#endif

#if ENABLE(TIZEN_SQL_DATABASE)
#include "ExceededDatabaseQuotaPermissionPopup.h"
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
#include "ExceededIndexedDatabaseQuotaPermissionPopup.h"
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
#include "ExceededLocalFileSystemQuotaPermissionPopup.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#include "NativeWebTouchEvent.h"
#include "WebEvent.h"
#endif

#if ENABLE(TIZEN_ISF_PORT)
#include <Ecore_IMF.h>
#include <WebCore/EflKeyboardUtilities.h>
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "NotificationPermissionPopup.h"
#include "WKArray.h"
#include "WKNotificationManager.h"
#include "WKNumber.h"
#include "ewk_notification_private.h"
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "UserMediaPermissionPopup.h"
#include "WKUserMediaPermissionRequest.h"
#include "ewk_user_media_private.h"
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
#include "WebPageGroup.h"
#include "WebPreferences.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#include "ewk_hit_test_private.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
#include "ewk_text_style.h"
#endif

#if ENABLE(TIZEN_SCREEN_READER)
#include "ScreenReaderProxy.h"
#endif

#if ENABLE(TIZEN_CSP)
#include <WebCore/ContentSecurityPolicy.h>
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
#include "WKDictionary.h"
#include "ewk_web_application_icon_data_private.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#include "AutoFillPopup.h"
#endif

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
#include "ewk_console_message.h"
#include "ewk_console_message_private.h"
#endif

#if ENABLE(TIZEN_ORIENTATION_EVENTS)
#include <vconf.h>
#include "WKContextTizen.h"
#include "TizenExtensibleAPI.h"
#endif

#endif // #if PLATFORM(TIZEN)

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
#include "WebBackForwardList.h"
#endif

#if ENABLE(TIZEN_FOCUS_UI)
#include <WebCore/WindowsKeyboardCodes.h>
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
#include "LinkMagnifierProxy.h"
#endif

#if ENABLE(TIZEN_OPEN_PANEL)
#include "ewk_settings.h"
#endif

using namespace WebKit;
using namespace WebCore;

static const char EWK_VIEW_TYPE_STR[] = "EWK2_View";

#define EWK_VIEW_TYPE_CHECK(ewkView, result)                                   \
    bool result = true;                                                        \
    do {                                                                       \
        if (!ewkView) {                                                        \
            EINA_LOG_ERR("null is not a ewk_view");                            \
            result = false;                                                    \
            break;                                                             \
        }                                                                      \
        const char* _tmp_otype = evas_object_type_get(ewkView);                \
        const Evas_Smart* _tmp_s = evas_object_smart_smart_get(ewkView);       \
        if (EINA_UNLIKELY(!_tmp_s)) {                                          \
            EINA_LOG_ERR                                                       \
                ("%p (%s) is not a smart object!",                             \
                 ewkView, _tmp_otype ? _tmp_otype : "(null)");                 \
            result = false;                                                    \
            break;                                                             \
        }                                                                      \
        const Evas_Smart_Class* _tmp_sc = evas_smart_class_get(_tmp_s);        \
        if (EINA_UNLIKELY(!_tmp_sc)) {                                         \
            EINA_LOG_ERR                                                       \
                ("%p (%s) is not a smart object!",                             \
                 ewkView, _tmp_otype ? _tmp_otype : "(null)");                 \
            result = false;                                                    \
            break;                                                             \
        }                                                                      \
        else if (EINA_UNLIKELY(_tmp_sc->data != EWK_VIEW_TYPE_STR)) {          \
            EINA_LOG_ERR                                                       \
                ("%p (%s) is not of an ewk_view (need %p, got %p)!",           \
                 ewkView, _tmp_otype ? _tmp_otype : "(null)",                  \
                 EWK_VIEW_TYPE_STR, _tmp_sc->data);                            \
            result = false;                                                    \
        }                                                                      \
    } while (0)

#define EWK_VIEW_SD_GET(ewkView, smartData)                                    \
    EWK_VIEW_TYPE_CHECK(ewkView, _tmp_result);                                 \
    Ewk_View_Smart_Data* smartData = 0;                                        \
    if (_tmp_result)                                                           \
        smartData = (Ewk_View_Smart_Data*)evas_object_smart_data_get(ewkView)

#define EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, ...)                     \
    EWK_VIEW_SD_GET(ewkView, smartData);                                       \
    do {                                                                       \
        if (!smartData) {                                                      \
            EINA_LOG_ERR("no smart data for object %p (%s)",                   \
                     ewkView, evas_object_type_get(ewkView));                  \
            return __VA_ARGS__;                                                \
        }                                                                      \
    } while (0)

#if PLATFORM(TIZEN)
static Eina_Bool _ewk_view_default_javascript_alert(Evas_Object*, const char* alertText, void* userData);
static Eina_Bool _ewk_view_default_javascript_confirm(Evas_Object*, const char* message, void* userData);
static Eina_Bool _ewk_view_default_javascript_prompt(Evas_Object*, const char* message, const char* defaultValue, void* userData);
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
static Eina_Bool _ewk_view_default_before_unload_confirm_panel(Evas_Object*, const char* message, void* userData);
#endif
static Eina_Bool _ewk_view_default_open_panel(Evas_Object*, Eina_Bool allow_multiple_files, Eina_List *accepted_mime_types, const char* capture, void* userData);

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
Eina_Bool _ewk_view_popup_menu_show(Ewk_View_Smart_Data*, Eina_Rectangle, Ewk_Text_Direction, double page_scale_factor, Eina_List* items, int selectedIndex);
#if ENABLE(TIZEN_MULTIPLE_SELECT)
Eina_Bool _ewk_view_multiple_popup_menu_show(Ewk_View_Smart_Data*, Eina_Rectangle, Ewk_Text_Direction, double page_scale_factor, Eina_List* items);
#endif
Eina_Bool _ewk_view_popup_menu_hide(Ewk_View_Smart_Data*);
Eina_Bool _ewk_view_popup_menu_update(Ewk_View_Smart_Data*, Eina_Rectangle, Ewk_Text_Direction, Eina_List*, int);
#endif
#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT_INTERNAL)
Eina_Bool _ewk_orientation_lock(Ewk_View_Smart_Data *sd, int orientations);
void _ewk_orientation_unlock(Ewk_View_Smart_Data *sd);
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
static Eina_Bool _ewk_view_input_picker_show(Ewk_View_Smart_Data*, Ewk_Input_Type, const char* inputValue);
#endif

#if ENABLE(TIZEN_INPUT_COLOR_PICKER)
static Eina_Bool _ewk_input_picker_color_request(Ewk_View_Smart_Data*, int, int, int, int);
static Eina_Bool _ewk_input_picker_color_dismiss(Ewk_View_Smart_Data*);
#endif

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
static void ewk_view_session_data_restore(Evas_Object* ewkView, const char* data, unsigned length);
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
static Eina_Bool _ewk_view_default_application_cache_permission(Evas_Object* ewkView, Ewk_Security_Origin* origin, void* userData);
#endif

#if ENABLE(TIZEN_SQL_DATABASE)
static Eina_Bool _ewk_view_default_exceeded_database_quota_permission(Evas_Object* ewkView, Ewk_Security_Origin* origin, const char* databaseName, unsigned long long expectedQuota, void* userData);
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
static Eina_Bool _ewk_view_default_exceeded_indexed_database_quota_permission(Evas_Object*, Ewk_Security_Origin* origin, long long currentQuota, void* userData);
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
static Eina_Bool _ewk_view_default_exceeded_local_file_system_quota_permission(Evas_Object* ewkView, Ewk_Security_Origin* origin, long long currentQuota, void* userData);
#endif

#if ENABLE(TIZEN_GEOLOCATION)
static Eina_Bool _ewk_view_default_geolocation_permission(Evas_Object*, Ewk_Geolocation_Permission_Request* geolocationPermissionRequest, void* userData);
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
static Eina_Bool _ewk_view_default_show_notification(Evas_Object* ewkView, Ewk_Notification* ewkNotification, void* userData);
static Eina_Bool _ewk_view_default_cancel_notification(Evas_Object* ewkView, uint64_t ewkNotificationID, void* userData);
static Eina_Bool _ewk_view_default_notification_permission(Evas_Object*, Ewk_Notification_Permission_Request* notificationPermissionRequest, void* userData);
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
static Eina_Bool _ewk_view_default_user_media_permission(Evas_Object*, Ewk_User_Media_Permission_Request* userMediaPermissionRequest, void* userData);
#endif

#endif // #if PLATFORM(TIZEN)

static void _ewk_view_smart_changed(Ewk_View_Smart_Data* smartData)
{
    if (smartData->changed.any)
        return;
    smartData->changed.any = true;
    evas_object_smart_changed(smartData->self);
}

#if !ENABLE(TIZEN_ICON_DATABASE)
static void _ewk_view_on_favicon_changed(const char* pageURL, void* eventInfo)
{
    Evas_Object* ewkView = static_cast<Evas_Object*>(eventInfo);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    const char* viewURL = ewk_view_url_get(ewkView);
    if (!viewURL || strcasecmp(viewURL, pageURL))
        return;

    impl->informIconChange();
}
#endif

#if ENABLE(TIZEN_GEOLOCATION)
static Eina_Bool _ewk_view_geolocation_validity_check(void* eventInfo)
{
    Evas_Object* ewkView = static_cast<Evas_Object*>(eventInfo);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, true);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, true);

    return impl->isValidLocationService();
}
#endif

// Default Event Handling.
static Eina_Bool _ewk_view_smart_focus_in(Ewk_View_Smart_Data* smartData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    TIZEN_LOGI("");

#if PLATFORM(TIZEN)
    // When page is closed should not handle this event.
    if (impl->page()->isClosed())
        return true;

    impl->pageClient->setViewFocused(true);
#endif // #if PLATFORM(TIZEN)

    impl->pageProxy->viewStateDidChange(WebPageProxy::ViewIsFocused | WebPageProxy::ViewWindowIsActive);
#if ENABLE(TIZEN_ISF_PORT)
#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
    if (!impl->isRunningSetFocusManually)
#endif
    if (impl->inputMethodContext())
        impl->inputMethodContext()->onFocusIn();
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    impl->page()->resumeFocusUI();
#endif

    return true;
}

static Eina_Bool _ewk_view_smart_focus_out(Ewk_View_Smart_Data* smartData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    TIZEN_LOGI("");

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_ISF_PORT)
    // Keypad should be hidden rapidly when moving focus on elementary
    // because Ecore-ime doesn't support it.
    if (impl->inputMethodContext())
        impl->inputMethodContext()->onFocusOut();
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    impl->page()->suspendFocusUI();
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (impl->textSelection()->isTextSelectionMode()) {
        if (ewk_settings_clear_text_selection_automatically_get(impl->settings()))
            impl->textSelection()->setIsTextSelectionMode(TextSelection::ModeNone);
        else {
            if (ewk_settings_text_selection_enabled_get(impl->settings()))
                impl->textSelection()->updateVisible(false);
        }
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    if (impl->pageClient->isClipboardWindowOpened())
        impl->pageClient->closeClipboardWindow();
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    if (smartData->api->formdata_candidate_is_showing(smartData))
        smartData->api->formdata_candidate_hide(smartData);
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
    LinkMagnifierProxy::linkMagnifier().hide();
#endif

    impl->pageClient->setViewFocused(false);
#endif // #if PLATFORM(TIZEN)
    impl->pageProxy->viewStateDidChange(WebPageProxy::ViewIsFocused | WebPageProxy::ViewWindowIsActive);
    return true;
}

static Eina_Bool _ewk_view_smart_mouse_wheel(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Wheel* wheelEvent)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->page()->handleWheelEvent(NativeWebWheelEvent(wheelEvent, impl->transformFromScene(), impl->transformToScreen()));
    return true;
}

static Eina_Bool _ewk_view_smart_mouse_down(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Down* downEvent)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->page()->handleMouseEvent(NativeWebMouseEvent(downEvent, impl->transformFromScene(), impl->transformToScreen()));
    return true;
}

static Eina_Bool _ewk_view_smart_mouse_up(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Up* upEvent)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->page()->handleMouseEvent(NativeWebMouseEvent(upEvent, impl->transformFromScene(), impl->transformToScreen()));

    InputMethodContextEfl* inputMethodContext = impl->inputMethodContext();
    if (inputMethodContext)
        inputMethodContext->handleMouseUpEvent(upEvent);

    return true;
}

static Eina_Bool _ewk_view_smart_mouse_move(Ewk_View_Smart_Data* smartData, const Evas_Event_Mouse_Move* moveEvent)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    // FIXME: impl->page() is used in the webkit opensource, but tizen webkit does not use it yet.
    impl->page()->handleMouseEvent(NativeWebMouseEvent(moveEvent, impl->transformFromScene(), impl->transformToScreen()));
    return true;
}

#if ENABLE(TIZEN_FOCUS_UI)
template<typename KeyEventType>
static bool isHWKeyboardEvent(KeyEventType event)
{
    return (event->timestamp > 1);
}

static bool isFocusUIActivationKeyCode(int32_t keycode)
{
    return (keycode == VK_UP || keycode == VK_DOWN || keycode == VK_LEFT || keycode == VK_RIGHT
           || keycode == VK_TAB || keycode == VK_RETURN);
}

static bool isFocusUIFunctionKeyCode(int32_t keycode)
{
    return (keycode == VK_CONTROL || keycode == VK_C || keycode == VK_A);
}
#endif

static Eina_Bool _ewk_view_smart_key_down(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Down* downEvent)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_ISF_PORT)
    NativeWebKeyboardEvent nativeEvent(downEvent, false);
    String key = downEvent->key;
    if ((key.isEmpty() || key.startsWith("XF86")) && key != "XF86AudioRaiseVolume" && key != "XF86AudioLowerVolume")
        return false;
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    WebPageProxy* page = impl->page();
    if (isHWKeyboardEvent(downEvent) && !page->focusUIEnabled()) {
        bool isActivationEvent = isFocusUIActivationKeyCode(nativeEvent.windowsVirtualKeyCode());
        bool isFunctionEvent = isFocusUIFunctionKeyCode(nativeEvent.windowsVirtualKeyCode());
        bool isFullscreen = false;

#if ENABLE(TIZEN_FULLSCREEN_API)
        isFullscreen = page->fullScreenManager()->isFullScreen();
#endif
        if (!isFullscreen && (isActivationEvent && !isFunctionEvent))
            page->setFocusUIEnabled(true);
    }

    if (page->focusUIEnabled() && !impl->shouldHandleKeyDownEvent(downEvent))
        return true;
#endif

    bool isFiltered = false;
    InputMethodContextEfl* inputMethodContext = impl->inputMethodContext();
    if (inputMethodContext)
        inputMethodContext->handleKeyDownEvent(downEvent, &isFiltered);

#if ENABLE(TIZEN_KEYPRESS_KEYCODE_FIX)
    std::string preeditCopy (inputMethodContext->preeditString.utf8().data());
    if (!preeditCopy.empty())
    {
        Evas_Event_Key_Down overridenDownEvent;
        memcpy(&overridenDownEvent, downEvent, sizeof(Evas_Event_Key_Down));
        const char * preeditKey = preeditCopy.c_str();
        overridenDownEvent.key     = preeditKey;
        overridenDownEvent.keyname = const_cast<char*>(preeditKey);
        overridenDownEvent.string  = preeditKey;
        overridenDownEvent.compose = preeditKey;
        nativeEvent = NativeWebKeyboardEvent(&overridenDownEvent, false);
    }
#endif
#if ENABLE(TIZEN_ISF_PORT)
    nativeEvent.setIsFiltered(isFiltered);
    nativeEvent.setInputMethodContextID(impl->pageProxy->editorState().inputMethodContextID);
    impl->pageProxy->handleKeyboardEvent(nativeEvent);
#else
    impl->pageProxy->handleKeyboardEvent(NativeWebKeyboardEvent(downEvent, isFiltered));
#endif

    return true;
}

static Eina_Bool _ewk_view_smart_key_up(Ewk_View_Smart_Data* smartData, const Evas_Event_Key_Up* upEvent)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    String key = upEvent->key;
#if ENABLE(TIZEN_FOCUS_UI)
    WebPageProxy* page = impl->page();
    if (isHWKeyboardEvent(upEvent) && !page->focusUIEnabled() && !(key.startsWith("XF86Audio")))
        return false;
    if (page->focusUIEnabled() && !impl->shouldHandleKeyUpEvent(upEvent))
        return true;
#endif
#if ENABLE(TIZEN_ISF_PORT)
    NativeWebKeyboardEvent nativeEvent(upEvent);
    if ((key.isEmpty() || key.startsWith("XF86")) && key != "XF86AudioRaiseVolume" && key != "XF86AudioLowerVolume")
        return false;

    InputMethodContextEfl* inputMethodContext = impl->inputMethodContext();
    if (inputMethodContext)
        inputMethodContext->handleKeyUpEvent(upEvent);

    impl->pageProxy->handleKeyboardEvent(nativeEvent);
#else
    impl->pageProxy->handleKeyboardEvent(NativeWebKeyboardEvent(upEvent));
#endif
    return true;
}

#if PLATFORM(TIZEN)
static Eina_Bool _ewk_view_smart_gesture_start(Ewk_View_Smart_Data* smartData, const Ewk_Event_Gesture* event)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_LINK_MAGNIFIER)
    LinkMagnifierProxy::linkMagnifier().setClickedExactly(false);
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    FocusRing* focusRing = impl->focusRing();
    if (focusRing) {
        if (event->type == EWK_GESTURE_TAP) {
            if (event->count == 1)
                focusRing->requestToShow(IntPoint(event->position.x, event->position.y));
        } else if (event->type == EWK_GESTURE_PAN) {
            if (impl->exceedTouchMoveThreshold)
                focusRing->requestToHide();
        } else if (event->type != EWK_GESTURE_LONG_PRESS) {
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
            if (!impl->pageClient->isContextMenuVisible())
#endif
                focusRing->requestToHide();
        }
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    if (smartData->api->formdata_candidate_is_showing(smartData))
        smartData->api->formdata_candidate_hide(smartData);
#endif

    if (impl->focusedNodeRect(true).intersects(IntRect(event->position.x, event->position.y,1,1)) && impl->pageProxy->editorState().hasComposition)
        impl->inputMethodContext()->resetIMFContext(false);

    switch (event->type) {
    case EWK_GESTURE_TAP:
        if (event->count == 1)
            impl->gestureClient->startTap(IntPoint(event->position.x, event->position.y));
        break;
    case EWK_GESTURE_LONG_PRESS: {
#if ENABLE(TIZEN_WEBKIT2_PREVENT_LONG_PRESS)
        // If longpress is disable, longpress should not be working.
        if (!ewk_settings_extra_feature_get(impl->settings(), "longpress,enable"))
            break;
#endif

#if ENABLE(TOUCH_EVENTS)
        // If the preventDefault method is called on touchstart event, it should prevent text selection and context menu.
        if (impl->wasHandledTouchStart)
            break;
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        //When javascript popup is shown then longpress will not respond.
        if (impl->isWaitingForJavaScriptPopupReply)
            break;
#endif
        // If gesture is working, text selection should not be working and context menu should not be shown.
        if (impl->gestureClient->isGestureWorking())
            break;

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (ewk_settings_text_selection_enabled_get(impl->settings()))
            impl->textSelection()->setIsTextSelectionMode(TextSelection::ModeNone);
#endif

        IntPoint scenePoint(event->position.x, event->position.y);
        IntPoint contentsPoint = impl->transformFromScene().mapPoint(scenePoint);
        WebHitTestResult::Data hitTestResultData = impl->pageProxy->hitTestResultAtPoint(contentsPoint, WebHitTestResult::HitTestModeDefault | WebHitTestResult::HitTestModeSetFocus);
        if (!hitTestResultData.absoluteMediaURL.isEmpty())
            break;

        bool contextMenuRequired = false;
        contextMenuRequired = !hitTestResultData.absoluteImageURL.isEmpty() || !hitTestResultData.absoluteLinkURL.isEmpty();
#if ENABLE(TIZEN_DRAG_SUPPORT)
        contextMenuRequired = contextMenuRequired || hitTestResultData.isDragSupport;
#endif
        contextMenuRequired = contextMenuRequired && !hitTestResultData.isContentEditable;

        if (contextMenuRequired) {
#if ENABLE(TIZEN_DRAG_SUPPORT)
            if (hitTestResultData.isDragSupport) {
                impl->pageClient->setDragPoint(scenePoint);
                if (impl->pageClient->isDragMode())
                    impl->pageClient->setDragMode(false);
            }
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
            if (focusRing)
                focusRing->showWithContextMenu(scenePoint, hitTestResultData.focusedColor, hitTestResultData.focusedRects);
#endif
            break;
        }

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        // 3. Check for text selection.
        if (ewk_settings_text_selection_enabled_get(impl->settings())) {
            if (impl->pageClient->textSelectionDown(scenePoint)) {
                impl->gestureClient->setGestureEnabled(false);
                impl->feedTouchEventsByType(EWK_TOUCH_CANCEL);
            }
        }
#endif
        break;
#endif // #if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    }
    case EWK_GESTURE_PAN:
        impl->gestureClient->startPan(IntPoint(event->position.x, event->position.y));
        break;
    case EWK_GESTURE_FLICK:
        impl->gestureClient->startFlick(IntPoint(event->position.x, event->position.y), IntPoint(event->velocity.x, event->velocity.y));
        break;
    case EWK_GESTURE_PINCH:
#if ENABLE(TIZEN_WEBKIT2_PREVENT_ZOOM)
        if (!ewk_settings_extra_feature_get(impl->settings(), "zoom,enable"))
            break;
#endif
        impl->gestureClient->startPinch(IntPoint(event->position.x, event->position.y), event->scale);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    return true;
#else
    return false;
#endif // #if ENABLE(TIZEN_GESTURE)
}

static Eina_Bool _ewk_view_smart_gesture_end(Ewk_View_Smart_Data* smartData, const Ewk_Event_Gesture* event)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_EDGE_SUPPORT)
    impl->edgeEffect()->hide();
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    FocusRing* focusRing = impl->focusRing();
    if (focusRing) {
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
        if (!impl->pageClient->isContextMenuVisible() || impl->textSelection()->isTextSelectionMode())
#endif
            focusRing->requestToHide();
    }
#endif

    switch (event->type) {
    case EWK_GESTURE_TAP:
        if (event->count == 1) {
#if ENABLE(TIZEN_DRAG_SUPPORT)
        if (impl->pageClient->isDragMode())
            impl->pageClient->setDragMode(false);
#endif

#if ENABLE(TIZEN_ISF_PORT)
            if (impl->inputMethodContext()->isIMEPostion(event->position.x, event->position.y))
                return false;
#endif

            int viewX, viewY;
            evas_object_geometry_get(impl->view(), &viewX, &viewY, 0, 0);
            IntPoint eventPosition(event->position.x, event->position.y);
            eventPosition = eventPosition - (impl->viewPositionAtTouchStart - IntPoint(viewX, viewY));
            impl->gestureClient->endTap(eventPosition);
        } else if (event->count == 2) {
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
            if (focusRing)
                focusRing->requestToHide(true);
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_DOUBLE_TAP)
            // If doubletap is disable, doubletap should not be working.
            if (!ewk_settings_extra_feature_get(impl->settings(), "doubletap,enable"))
                break;

            impl->gestureClient->endDoubleTap(IntPoint(event->position.x, event->position.y));
#endif
        }
        break;
    case EWK_GESTURE_LONG_PRESS: {
#if ENABLE(TIZEN_WEBKIT2_PREVENT_LONG_PRESS)
        // If longpress is disable, longpress should not be working.
        if (!ewk_settings_extra_feature_get(impl->settings(), "longpress,enable"))
            break;
#endif

        bool enableEndTap = true;
        // Prcess endTap for LONG_PRESS gesture if text-selection and context menu did not work
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (impl->textSelection()->isTextSelectionMode())
            enableEndTap = false;
#endif
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
        if (impl->pageClient->isContextMenuVisible())
            enableEndTap = false;
#endif
        if (enableEndTap)
            impl->gestureClient->endTap(IntPoint(event->position.x, event->position.y));
        else
            return true;
    }
        break;
    case EWK_GESTURE_PAN:
#if ENABLE(TIZEN_WEBKIT2_TOUCH_EVENT_TIMER)
        impl->pageProxy->processDelayedTouchEvent();
#endif
        impl->gestureClient->endPan(IntPoint(event->position.x, event->position.y));
        break;
    case EWK_GESTURE_FLICK:
        impl->gestureClient->endFlick(IntPoint(event->position.x, event->position.y), IntPoint(event->velocity.x, event->velocity.y));
        break;
    case EWK_GESTURE_PINCH:
        impl->gestureClient->endPinch(IntPoint(event->position.x, event->position.y), event->scale);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    return true;
#else
    return false;
#endif // #if ENABLE(TIZEN_GESTURE)
}

static Eina_Bool _ewk_view_smart_gesture_move(Ewk_View_Smart_Data* smartData, const Ewk_Event_Gesture* event)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    FocusRing* focusRing = impl->focusRing();
    if (focusRing && !(event->type == EWK_GESTURE_PAN && !impl->exceedTouchMoveThreshold)) {
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
        if (!impl->pageClient->isContextMenuVisible())
#endif
            focusRing->requestToHide(true);
    }
#endif

    switch (event->type) {
    case EWK_GESTURE_PAN:
        impl->gestureClient->movePan(IntPoint(event->position.x, event->position.y));
        break;
    case EWK_GESTURE_TAP:
    case EWK_GESTURE_LONG_PRESS:
    case EWK_GESTURE_FLICK:
        break;
    case EWK_GESTURE_PINCH:
        impl->gestureClient->movePinch(IntPoint(event->position.x, event->position.y), event->scale);
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    return true;
#else
    return false;
#endif // #if ENABLE(TIZEN_GESTURE)
}
#endif //#if PLATFORM(TIZEN)

// Event Handling.
static void _ewk_view_on_focus_in(void* data, Evas* canvas, Evas_Object* ewkView, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->focus_in);
    smartData->api->focus_in(smartData);
}

static void _ewk_view_on_focus_out(void* data, Evas* canvas, Evas_Object* ewkView, void* eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->focus_out);
    smartData->api->focus_out(smartData);
}

static void _ewk_view_on_mouse_wheel(void* data, Evas* canvas, Evas_Object* ewkView, void* eventInfo)
{
    Evas_Event_Mouse_Wheel* wheelEvent = static_cast<Evas_Event_Mouse_Wheel*>(eventInfo);
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->mouse_wheel);
    smartData->api->mouse_wheel(smartData, wheelEvent);
}

static void _ewk_view_on_key_down(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Key_Down* downEvent = static_cast<Evas_Event_Key_Down*>(eventInfo);
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->key_down);
    smartData->api->key_down(smartData, downEvent);
}

static void _ewk_view_on_key_up(void* data, Evas* canvas, Evas_Object* ewkView, void* eventInfo)
{
    Evas_Event_Key_Up* upEvent = static_cast<Evas_Event_Key_Up*>(eventInfo);
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->key_up);
    smartData->api->key_up(smartData, upEvent);
}

static void _ewk_view_on_show(void* data, Evas*, Evas_Object*, void* /*eventInfo*/)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    impl->pageProxy->viewStateDidChange(WebPageProxy::ViewIsVisible);
}

static void _ewk_view_on_hide(void* data, Evas*, Evas_Object*, void* /*eventInfo*/)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    // This call may look wrong, but we really need to pass ViewIsVisible here.
    // viewStateDidChange() itself is responsible for actually setting the visibility to Visible or Hidden
    // depending on what WebPageProxy::isViewVisible() returns, this simply triggers the process.
    impl->pageProxy->viewStateDidChange(WebPageProxy::ViewIsVisible);
}

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_FULLSCREEN_API)
Eina_Bool _ewk_view_smart_fullscreen_enter(Ewk_View_Smart_Data* smartData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    ewk_view_main_frame_scrollbar_visible_set(impl->view(), false);
    bool isNeed = impl->page()->fullScreenManager()->exitFullScreenByHwBackKey();
    evas_object_smart_callback_call(impl->view(), "fullscreen,enterfullscreen", &isNeed);
    return true;
}

Eina_Bool _ewk_view_smart_fullscreen_exit(Ewk_View_Smart_Data* smartData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    ewk_view_main_frame_scrollbar_visible_set(impl->view(), true);
    evas_object_smart_callback_call(impl->view(), "fullscreen,exitfullscreen", 0);
    return true;
}
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
Eina_Bool _ewk_view_text_selection_down(Ewk_View_Smart_Data* smartData, int x, int y)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->pageClient->textSelectionDown(IntPoint(x, y));
}

Eina_Bool _ewk_view_text_selection_move(Ewk_View_Smart_Data* smartData, int x, int y)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    IntPoint point(x, y);
    impl->pageClient->textSelectionMove(point);

    return true;
}

Eina_Bool _ewk_view_text_selection_up(Ewk_View_Smart_Data* smartData, int x, int y)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    IntPoint point(x, y);
    impl->pageClient->textSelectionUp(point, true);

    return true;
}

void _ewk_view_selecton_handle_down(Ewk_View_Smart_Data* smartData, Ewk_Selection_Handle_Type hanleType, int x, int y)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->textSelection()->handleDown(static_cast<TextSelectionHandle::HandleType>(hanleType), IntPoint(x, y));
}

void _ewk_view_selecton_handle_move(Ewk_View_Smart_Data* smartData, Ewk_Selection_Handle_Type hanleType, int x, int y)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->textSelection()->handleMove(static_cast<TextSelectionHandle::HandleType>(hanleType), IntPoint(x, y));
}


void _ewk_view_selecton_handle_up(Ewk_View_Smart_Data* smartData, Ewk_Selection_Handle_Type hanleType, int x, int y)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->textSelection()->handleUp(static_cast<TextSelectionHandle::HandleType>(hanleType), IntPoint(x, y));
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
Eina_Bool _ewk_view_smart_formdata_candidate_show(Ewk_View_Smart_Data* smartData, int x, int y, int w, int h)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->pageClient->showAutoFillPopup(IntRect(x, y, w, h));

    return true;
}

Eina_Bool _ewk_view_smart_formdata_candidate_hide(Ewk_View_Smart_Data* smartData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->pageClient->hideAutoFillPopup();

    return true;
}

Eina_Bool _ewk_view_smart_formdata_candidate_update_data(Ewk_View_Smart_Data* smartData, Eina_List* dataList)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    Vector<AutoFillPopupItem> formData;
    Eina_List* list;
    void* data;
    EINA_LIST_FOREACH(dataList, list, data)
        formData.append(AutoFillPopupItem(String::fromUTF8(static_cast<char*>(data)), candidateAutoFill));

    impl->pageClient->updateAutoFillPopup(formData);

    return true;
}

Eina_Bool _ewk_view_smart_formdata_candidate_is_showing(Ewk_View_Smart_Data* smartData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->pageClient->isShowingAutoFillPopup();
}
#endif

#if ENABLE(TIZEN_SCREEN_READER)
Eina_Bool _ewk_view_screen_reader_action_execute(Ewk_View_Smart_Data* smartData, void* actionInfo)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return ScreenReaderProxy::executeAction(impl, 0, static_cast<Elm_Access_Action_Info*>(actionInfo));
}
#endif
#endif // #if PLATFORM(TIZEN)

static Evas_Smart_Class g_parentSmartClass = EVAS_SMART_CLASS_INIT_NULL;

static void _ewk_view_impl_del(EwkViewImpl* impl)
{
#if !ENABLE(TIZEN_ICON_DATABASE)
    /* Unregister icon change callback */
    Ewk_Favicon_Database* iconDatabase = impl->context->faviconDatabase();
    iconDatabase->unwatchChanges(_ewk_view_on_favicon_changed);
#endif

    delete impl;
}

static void _ewk_view_smart_add(Evas_Object* ewkView)
{
    const Evas_Smart* smart = evas_object_smart_smart_get(ewkView);
    const Evas_Smart_Class* smartClass = evas_smart_class_get(smart);
    const Ewk_View_Smart_Class* api = reinterpret_cast<const Ewk_View_Smart_Class*>(smartClass);
    EWK_VIEW_SD_GET(ewkView, smartData);

    if (!smartData) {
        smartData = static_cast<Ewk_View_Smart_Data*>(calloc(1, sizeof(Ewk_View_Smart_Data)));
        if (!smartData) {
            EINA_LOG_CRIT("could not allocate Ewk_View_Smart_Data");
            return;
        }
        evas_object_smart_data_set(ewkView, smartData);
    }

    smartData->self = ewkView;
    smartData->api = api;

    g_parentSmartClass.add(ewkView);

    smartData->priv = new EwkViewImpl(ewkView);
    if (!smartData->priv) {
        EINA_LOG_CRIT("could not allocate EwkViewImpl");
        evas_object_smart_data_set(ewkView, 0);
        free(smartData);
        return;
    }

    // Create evas_object_image to draw web contents.
    smartData->image = evas_object_image_add(smartData->base.evas);
    evas_object_image_alpha_set(smartData->image, false);
    evas_object_image_filled_set(smartData->image, true);
    evas_object_smart_member_add(smartData->image, ewkView);

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(smartData->base.evas);
    const char *engine = ecore_evas_engine_name_get(ee);
    if (engine && !strcmp(engine, "opengl_x11"))
        evas_object_image_content_hint_set(smartData->image, EVAS_IMAGE_CONTENT_HINT_DYNAMIC);
#endif
    ewk_view_mouse_events_enabled_set(ewkView, false);
    ewk_view_touch_events_enabled_set(ewkView, true);

    // FIXME: This code should be removed if side effect occur.
    // WebView does not have focus after url loading.
    // If focus is set as true to webview,
    // elementary steal webview's focus during mouse up event
    // So, added code that events are not propagated to smart parent according to guide from EFL
    evas_object_propagate_events_set(ewkView, false);
#endif // #if PLATFORM(TIZEN)

#define CONNECT(s, c) evas_object_event_callback_add(ewkView, s, c, smartData)
    CONNECT(EVAS_CALLBACK_FOCUS_IN, _ewk_view_on_focus_in);
    CONNECT(EVAS_CALLBACK_FOCUS_OUT, _ewk_view_on_focus_out);
    CONNECT(EVAS_CALLBACK_MOUSE_WHEEL, _ewk_view_on_mouse_wheel);
    CONNECT(EVAS_CALLBACK_KEY_DOWN, _ewk_view_on_key_down);
    CONNECT(EVAS_CALLBACK_KEY_UP, _ewk_view_on_key_up);
    CONNECT(EVAS_CALLBACK_SHOW, _ewk_view_on_show);
    CONNECT(EVAS_CALLBACK_HIDE, _ewk_view_on_hide);
#undef CONNECT
}

static void _ewk_view_smart_del(Evas_Object* ewkView)
{
    TIZEN_LOGI("_ewk_view_smart_del -> [%p]", ewkView);
    EwkViewImpl::removeFromPageViewMap(ewkView);
    EWK_VIEW_SD_GET(ewkView, smartData);
#if ENABLE(TIZEN_SCREEN_READER)
    ScreenReaderProxy::screenReader().finalize(smartData->priv);
#endif
#if ENABLE(TIZEN_ICON_DATABASE)
    if (smartData) {
        EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
        ewk_view_icondatabase_client_detach(impl->context->wkContext());
    }
#endif

#if ENABLE(TIZEN_GEOLOCATION)
    if (smartData) {
        EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
        Ewk_Geolocation_Provider* geolocationProvider = impl->context->geolocationProvider();
        TIZEN_LOGI("_ewk_view_smart_del:: unwatch validity");
        geolocationProvider->unwatchValidity(ewkView);
    }
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    if (!ewk_view_is_opengl_backend(ewkView) && smartData) {
        EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
        if (impl->compositionAnimator)
            ecore_animator_del(impl->compositionAnimator);
    }
#endif

    if (smartData && smartData->priv)
        _ewk_view_impl_del(smartData->priv);

    g_parentSmartClass.del(ewkView);
}

static Eina_Bool _ewk_view_composite(void* data);

static void _ewk_view_smart_resize(Evas_Object* ewkView, Evas_Coord width, Evas_Coord height)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);

    evas_object_resize(smartData->image, width, height);
    evas_object_image_size_set(smartData->image, width, height);
    evas_object_image_fill_set(smartData->image, 0, 0, width, height);
#if PLATFORM(TIZEN)
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
    evas_object_image_native_surface_set(smartData->image, 0);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("smartData: [%d, %d], resize: [%d, %d]", smartData->view.w, smartData->view.h, width, height);
#endif

#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    impl->gestureClient->resetUrlBarTriggerPoint();
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    if (!ewk_view_is_opengl_backend(ewkView))
        _ewk_view_composite(smartData);
    else // OpenGL backend
#endif
    impl->pageClient->displayViewport();
#endif
#endif // #if PLATFORM(TIZEN)

    smartData->changed.size = true;
    _ewk_view_smart_changed(smartData);
}

static void _ewk_view_smart_move(Evas_Object* ewkView, Evas_Coord x, Evas_Coord y)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);

    smartData->changed.position = true;
    _ewk_view_smart_changed(smartData);
}

static void _ewk_view_smart_calculate(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    Evas_Coord x, y, width, height;

    smartData->changed.any = false;

    evas_object_geometry_get(ewkView, &x, &y, &width, &height);

    if (smartData->changed.size) {
#if ENABLE(TIZEN_LINK_MAGNIFIER)
        if (ewk_settings_extra_feature_get(impl->settings(), "link,magnifier"))
            LinkMagnifierProxy::linkMagnifier().hide();
#endif
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("changed.size: [%d, %d]", width, height);
#endif
#if !PLATFORM(TIZEN)
#if USE(COORDINATED_GRAPHICS)
        impl->pageViewportControllerClient->updateViewportSize(IntSize(width, height));
#endif
#if USE(ACCELERATED_COMPOSITING)
        needsNewSurface = impl->evasGlSurface;
#endif

        if (impl->pageProxy->drawingArea())
            impl->pageProxy->drawingArea()->setSize(IntSize(width, height), IntSize());

#if USE(ACCELERATED_COMPOSITING)
        if (!impl->evasGlSurface)
            return;
        evas_gl_surface_destroy(impl->evasGl, impl->evasGlSurface);
        impl->evasGlSurface = 0;
        ewk_view_create_gl_surface(ewkView, IntSize(width, height));
        ewk_view_display(ewkView, IntRect(IntPoint(), IntSize(width, height)));
#endif
#endif // #if !PLATFORM(TIZEN)

#if PLATFORM(TIZEN) && ENABLE(TIZEN_PREVENT_RESIZE_WHILE_ROTATING_WITH_SAME_WIDTH)
        int previousViewWidth = smartData->view.w;
        bool canPreventResize = (width == smartData->view.w && height != smartData->view.h);
#endif
        smartData->view.w = width;
        smartData->view.h = height;
        smartData->changed.size = false;

#if PLATFORM(TIZEN)
        if (impl->pageClient) {
            if (DrawingAreaProxy* drawingArea = impl->pageProxy->drawingArea()) {
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE) && !ENABLE(TIZEN_WEBKIT2_EFL_WTR) || ENABLE(TIZEN_PREVENT_RESIZE_WHILE_ROTATING_WITH_SAME_WIDTH)
                Ecore_Evas* ee = ecore_evas_ecore_evas_get(smartData->base.evas);
                int newAngle = ecore_evas_rotation_get(ee);
#endif
#if ENABLE(TIZEN_PREVENT_RESIZE_WHILE_ROTATING_WITH_SAME_WIDTH)
                if (impl->pageProxy->isViewVisible() && ((impl->currentAngle() + newAngle) % 180 != 0) && canPreventResize)
                    return;
#endif
                impl->pageClient->updateViewportSize(IntSize(width, height));
                impl->pageClient->updateVisibleContentRectSize(IntSize(width, height));

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
                drawingArea->setIsShowingInputMethod(impl->inputMethodContext()->isShow());
#endif

                drawingArea->setSize(IntSize(width, height), IntSize());

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE) && !ENABLE(TIZEN_WEBKIT2_EFL_WTR)
#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
                bool prerenderingForRotation = false;
#if ENABLE(TIZEN_EXTENSIBLE_API)
                prerenderingForRotation = WKContextGetTizenExtensibleAPI(impl->context->wkContext(), static_cast<WKTizenExtensibleAPI>(EWK_EXTENSIBLE_API_PRERENDERING_FOR_ROTATION));
#endif
                bool isFullscreen = false;
#if ENABLE(TIZEN_FULLSCREEN_API)
                isFullscreen = impl->page()->fullScreenManager()->isFullScreen();
#endif

                if (impl->currentAngle() == INVALID_ANGLE)
                    impl->setCurrentAngle(newAngle);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
                impl->textSelection()->hideMagnifierOnRotation();

                if (impl->textSelection()->isTextSelectionMode()) {
                    impl->textSelection()->updateVisible(false);
                    if (width != previousViewWidth)
                        impl->textSelection()->setRelayoutRequired(true); // Update Text Selection when we are done with setting the visible content rect.
                }
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
                if (impl->pageClient->isDragMode() && (impl->currentAngle() != newAngle)) {
                    impl->pageClient->updateDragPosition();
                }
#endif

                if (impl->pageProxy->isViewVisible() && (prerenderingForRotation || isFullscreen))
                    if ((impl->currentAngle() + newAngle) % 180 != 0)
                        if (impl->pageClient->notifiedNonemptyLayout() && (impl->pageClient->pageDidRendered() && impl->page()->estimatedProgress() > 0.1)) {
                            impl->pageClient->resetPreparingRotationStatus();
                            evas_object_image_pixels_dirty_set(smartData->image, false);
                            impl->pageClient->setPreparingRotation(true);
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
                            if (!impl->page()->fullScreenManager()->isUsingHwVideoOverlay())
#endif
                            ewkViewScreenShotForRotationDisplay(ewkView, newAngle);
                            ewk_view_suspend_painting(ewkView);
                        }
                impl->setCurrentAngle(newAngle);
#endif
#endif
            }
#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
            impl->pageClient->frameRectChanged();
#endif
        }
#endif // #if PLATFORM(TIZEN)
    }

    if (smartData->changed.position) {
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        IntSize diff(x - smartData->view.x, y - smartData->view.y);
#endif

        evas_object_move(smartData->image, x, y);
        smartData->view.x = x;
        smartData->view.y = y;
        smartData->changed.position = false;
#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
        impl->pageClient->frameRectChanged();
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (impl->textSelection()->isTextSelectionMode()) {
            impl->textSelection()->updateVisible(false);

            if (!impl->textSelection()->relayoutRequired())
                impl->textSelection()->requestToShow();
        }
#endif
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        if (impl->focusRing())
            impl->focusRing()->moveBy(diff);
#endif
#if ENABLE(TIZEN_SCREEN_READER)
        if (ScreenReaderProxy::screenReader().focusRing())
            ScreenReaderProxy::screenReader().focusRing()->moveBy(diff);
#endif
#endif // #if PLATFORM(TIZEN)
    }
#if PLATFORM(TIZEN)
    if (impl->popupPicker)
        ewk_popup_picker_resize(impl->popupPicker);
#endif // #if PLATFORM(TIZEN)
}

static void _ewk_view_smart_show(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (evas_object_clipees_get(smartData->base.clipper))
        evas_object_show(smartData->base.clipper);
    evas_object_show(smartData->image);

#if ENABLE(TIZEN_SCREEN_READER)
    ScreenReaderProxy::screenReader().initialize(impl);
#endif
}

static void _ewk_view_smart_hide(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);

    evas_object_hide(smartData->base.clipper);
    evas_object_hide(smartData->image);
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    if (impl->textSelection()->isTextSelectionMode())
        impl->textSelection()->setIsTextSelectionMode(TextSelection::ModeNone);
#endif

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
    ewk_view_popup_menu_close(ewkView);
#endif
}

static void _ewk_view_smart_color_set(Evas_Object* ewkView, int red, int green, int blue, int alpha)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (alpha < 0)
        alpha = 0;
    else if (alpha > 255)
        alpha = 255;

#define CHECK_COLOR(color, alpha) \
    if (color < 0)                \
        color = 0;                \
    else if (color > alpha)       \
        color = alpha;
    CHECK_COLOR(red, alpha);
    CHECK_COLOR(green, alpha);
    CHECK_COLOR(blue, alpha);
#undef CHECK_COLOR

    evas_object_image_alpha_set(smartData->image, alpha < 255);
    impl->pageProxy->setDrawsBackground(red || green || blue);
    impl->pageProxy->setDrawsTransparentBackground(alpha < 255);

    if (impl->pageClient->drawingArea() && impl->pageClient->drawingArea()->layerTreeCoordinatorProxy()) {
        WebCore::Color backgroundColor(red, green, blue, alpha);
        impl->pageClient->drawingArea()->layerTreeCoordinatorProxy()->setBackgroundColor(backgroundColor);
    }

#if !PLATFORM(TIZEN)
    g_parentSmartClass.color_set(ewkView, red, green, blue, alpha);
#endif
}

Eina_Bool ewk_view_smart_class_set(Ewk_View_Smart_Class* api)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(api, false);

    if (api->version != EWK_VIEW_SMART_CLASS_VERSION) {
        EINA_LOG_CRIT("Ewk_View_Smart_Class %p is version %lu while %lu was expected.",
             api, api->version, EWK_VIEW_SMART_CLASS_VERSION);
        return false;
    }

    if (EINA_UNLIKELY(!g_parentSmartClass.add))
        evas_object_smart_clipped_smart_set(&g_parentSmartClass);

    evas_object_smart_clipped_smart_set(&api->sc);

    // Set Evas_Smart_Class functions.
    api->sc.add = _ewk_view_smart_add;
    api->sc.del = _ewk_view_smart_del;
    api->sc.move = _ewk_view_smart_move;
    api->sc.resize = _ewk_view_smart_resize;
    api->sc.show = _ewk_view_smart_show;
    api->sc.hide = _ewk_view_smart_hide;
    api->sc.color_set = _ewk_view_smart_color_set;
    api->sc.calculate = _ewk_view_smart_calculate;
    api->sc.data = EWK_VIEW_TYPE_STR; // It is used by type checking.

    // Set Ewk_View_Smart_Class functions.
    api->focus_in = _ewk_view_smart_focus_in;
    api->focus_out = _ewk_view_smart_focus_out;
    api->mouse_wheel = _ewk_view_smart_mouse_wheel;
    api->mouse_down = _ewk_view_smart_mouse_down;
    api->mouse_up = _ewk_view_smart_mouse_up;
    api->mouse_move = _ewk_view_smart_mouse_move;
    api->key_down = _ewk_view_smart_key_down;
    api->key_up = _ewk_view_smart_key_up;
#if PLATFORM(TIZEN)
    api->gesture_start = _ewk_view_smart_gesture_start;
    api->gesture_end = _ewk_view_smart_gesture_end;
    api->gesture_move = _ewk_view_smart_gesture_move;

#if ENABLE(TIZEN_FULLSCREEN_API)
    api->fullscreen_enter = _ewk_view_smart_fullscreen_enter;
    api->fullscreen_exit = _ewk_view_smart_fullscreen_exit;
#endif

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
    api->popup_menu_show = _ewk_view_popup_menu_show;
#if ENABLE(TIZEN_MULTIPLE_SELECT)
    api->multiple_popup_menu_show = _ewk_view_multiple_popup_menu_show;
#endif
    api->popup_menu_hide = _ewk_view_popup_menu_hide;
    api->popup_menu_update = _ewk_view_popup_menu_update;
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    api->text_selection_down = _ewk_view_text_selection_down;
    api->text_selection_move = _ewk_view_text_selection_move;
    api->text_selection_up = _ewk_view_text_selection_up;
#endif
#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
    api->input_picker_show = _ewk_view_input_picker_show;
#endif
#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT_INTERNAL)
    api->orientation_lock = _ewk_orientation_lock;
    api->orientation_unlock = _ewk_orientation_unlock;
#endif
#if ENABLE(TIZEN_INPUT_COLOR_PICKER)
    api->input_picker_color_request = _ewk_input_picker_color_request;
    api->input_picker_color_dismiss = _ewk_input_picker_color_dismiss;
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    api->formdata_candidate_show = _ewk_view_smart_formdata_candidate_show;
    api->formdata_candidate_hide = _ewk_view_smart_formdata_candidate_hide;
    api->formdata_candidate_update_data = _ewk_view_smart_formdata_candidate_update_data;
    api->formdata_candidate_is_showing = _ewk_view_smart_formdata_candidate_is_showing;
#endif
#if ENABLE(TIZEN_SCREEN_READER)
    api->screen_reader_action_execute = _ewk_view_screen_reader_action_execute;
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    api->selection_handle_down = _ewk_view_selecton_handle_down;
    api->selection_handle_move = _ewk_view_selecton_handle_move;
    api->selection_handle_up = _ewk_view_selecton_handle_up;
#endif
#endif //#if PLATFORM(TIZEN)

    return true;
}

static inline Evas_Smart* _ewk_view_smart_class_new(void)
{
    static Ewk_View_Smart_Class api = EWK_VIEW_SMART_CLASS_INIT_NAME_VERSION("Ewk_View");
    static Evas_Smart* smart = 0;

    if (EINA_UNLIKELY(!smart)) {
        ewk_view_smart_class_set(&api);
        smart = evas_smart_class_new(&api.sc);
    }

    return smart;
}

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
static void _ewk_view_initialize(Evas_Object* ewkView, PassRefPtr<Ewk_Context> context, WKPageGroupRef pageGroupRef, const char* data = 0, unsigned length = 0)
#else
static void _ewk_view_initialize(Evas_Object* ewkView, PassRefPtr<Ewk_Context> context, WKPageGroupRef pageGroupRef)
#endif
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    EINA_SAFETY_ON_NULL_RETURN(context);

    if (impl->pageClient)
        return;

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    bool isOpenGL = ewk_view_is_opengl_backend(ewkView);
    if (isOpenGL) {
        TIZEN_LOGI("OpenGL(ES) Backend Mode");
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
        impl->pageClient = PageClientEvasGL::create(impl, context->pixmap());
#else
        impl->pageClient = PageClientEvasGL::create(impl);
#endif
    }
    else {
        TIZEN_LOGI("Software Backend Mode");
        impl->pageClient = PageClientImpl::create(impl);
    }
#else
    impl->pageClient = PageClientImpl::create(impl);
#endif

    if (pageGroupRef)
        impl->pageProxy = toImpl(context->wkContext())->createWebPage(impl->pageClient.get(), toImpl(pageGroupRef));
    else
        impl->pageProxy = toImpl(context->wkContext())->createWebPage(impl->pageClient.get(), WebPageGroup::create().get());

    EwkViewImpl::addToPageViewMap(ewkView);

#if PLATFORM(TIZEN)
    impl->pageProxy->pageGroup()->preferences()->setAcceleratedCompositingEnabled(true);
    impl->pageProxy->pageGroup()->preferences()->setForceCompositingMode(true);
    impl->pageProxy->pageGroup()->preferences()->setFrameFlatteningEnabled(true);
    impl->pageProxy->pageGroup()->preferences()->setAllowUniversalAccessFromFileURLs(true);
#endif

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
    if(data && length)
        ewk_view_session_data_restore(ewkView, data, length);
#endif
    impl->pageProxy->initializeWebPage();

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
    WebBackForwardListItem* item = impl->pageProxy->backForwardList()->currentItem();
    if (item)
        impl->pageProxy->goToBackForwardItem(item);
#endif

#if ENABLE(TIZEN_VIEWPORT_META_TAG)
    impl->pageProxy->setCustomDeviceScaleFactor((float)getMobileDPI() / 160);
#else
    impl->pageProxy->setCustomDeviceScaleFactor((float)getDPI() / 160);
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE) && !ENABLE(TIZEN_WEBKIT2_EFL_WTR)
    impl->pageProxy->setUseFixedLayout(true);
#endif
#if ENABLE(FULLSCREEN_API)
    impl->pageProxy->fullScreenManager()->setWebView(ewkView);
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    if (isOpenGL)
        impl->pageProxy->drawingArea()->layerTreeCoordinatorProxy()->initializeAcceleratedCompositingMode(true);
    else {
        impl->pageProxy->pageGroup()->preferences()->setAcceleratedCompositingEnabled(false);
        impl->pageProxy->pageGroup()->preferences()->setWebGLEnabled(false);
        impl->pageProxy->drawingArea()->layerTreeCoordinatorProxy()->initializeAcceleratedCompositingMode(false);
    }
#endif

    impl->backForwardList = Ewk_Back_Forward_List::create(toAPI(impl->pageProxy->backForwardList()));

    impl->context = context;

#if PLATFORM(TIZEN)
    ewkViewContextMenuClientAttachClient(ewkView);
    ewkViewTizenClientAttachClient(ewkView);

#if ENABLE(TIZEN_APPLICATION_CACHE)
    #if 0
        // Code is made unreachable as per requirement to disable popup
        ewk_view_application_cache_permission_callback_set(ewkView, _ewk_view_default_application_cache_permission, 0);
    #endif
#endif

#if ENABLE(TIZEN_SQL_DATABASE)
    ewk_view_exceeded_database_quota_callback_set(ewkView, _ewk_view_default_exceeded_database_quota_permission, 0);
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
    ewk_view_exceeded_indexed_database_quota_callback_set(ewkView, _ewk_view_default_exceeded_indexed_database_quota_permission, 0);
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
    ewk_view_exceeded_local_file_system_quota_callback_set(ewkView, _ewk_view_default_exceeded_local_file_system_quota_permission, 0);
#endif

#if ENABLE(TIZEN_GEOLOCATION)
    Ewk_Geolocation_Provider* geolocationProvider = impl->context->geolocationProvider();
    geolocationProvider->watchValidity(GeolocationValidityCallbackData(_ewk_view_geolocation_validity_check, ewkView));
    ewk_view_geolocation_permission_callback_set(ewkView, _ewk_view_default_geolocation_permission, 0);
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
    ewk_view_notification_permission_callback_set(ewkView, _ewk_view_default_notification_permission, 0);
    ewk_view_notification_callback_set(ewkView, _ewk_view_default_show_notification, _ewk_view_default_cancel_notification, 0);
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
    ewk_view_user_media_permission_callback_set(ewkView, _ewk_view_default_user_media_permission, 0);
#endif

#if ENABLE(TIZEN_ICON_DATABASE)
    ewk_view_icondatabase_client_attach(ewkView, impl->context->wkContext());
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    ewk_view_password_confirm_popup_callback_set(ewkView, 0, 0);
#endif
    ewk_view_javascript_alert_callback_set(ewkView, _ewk_view_default_javascript_alert, 0);
    ewk_view_javascript_confirm_callback_set(ewkView, _ewk_view_default_javascript_confirm, 0);
    ewk_view_javascript_prompt_callback_set(ewkView, _ewk_view_default_javascript_prompt, 0);
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
    ewk_view_before_unload_confirm_panel_callback_set(ewkView, _ewk_view_default_before_unload_confirm_panel, 0);
#endif
    ewk_view_open_panel_callback_set(ewkView, _ewk_view_default_open_panel, 0);
#else // #if PLATFORM(TIZEN)
#if USE(COORDINATED_GRAPHICS)
    impl->viewportHandler = EflViewportHandler::create(impl->pageClient.get());
#endif
#endif // #if PLATFORM(TIZEN)

#if ENABLE(FULLSCREEN_API)
    impl->pageProxy->fullScreenManager()->setWebView(ewkView);
    impl->pageProxy->pageGroup()->preferences()->setFullScreenEnabled(true);
#endif

    // Initialize page clients.
    impl->pageLoadClient = PageLoadClientEfl::create(impl);
    impl->pagePolicyClient = PagePolicyClientEfl::create(impl);
    impl->pageUIClient = PageUIClientEfl::create(impl);
    impl->resourceLoadClient = ResourceLoadClientEfl::create(impl);
    impl->findClient = FindClientEfl::create(impl);
    impl->formClient = FormClientEfl::create(impl);
#if !ENABLE(TIZEN_ICON_DATABASE)
    /* Listen for favicon changes */
    Ewk_Favicon_Database* iconDatabase = impl->context->faviconDatabase();
    iconDatabase->watchChanges(IconChangeCallbackData(_ewk_view_on_favicon_changed, ewkView));
#endif
#if ENABLE(TIZEN_WEBKIT2_THEME_SET_INTERNAL)
    ewk_view_theme_set(ewkView, "/usr/share/edje/webkit.edj");
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    if (!impl->context->xWindow())
        impl->context->setXWindow(impl->getXWindow());
#endif

#if ENABLE(TIZEN_WEBKIT2_PROXY)
    impl->context->setProxy();
#endif
}

static Evas_Object* _ewk_view_add_with_smart(Evas* canvas, Evas_Smart* smart)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smart, 0);

#if ENABLE(TIZEN_WEBKIT2_DDK_CHECK)
    {
        using namespace EGL;
        if(!eglGetDisplay(EGL_DEFAULT_DISPLAY)) {
            EINA_LOG_CRIT("Fail in initiziling view because No DDK is installed.");
            return 0;
        }
    }
#endif

    Evas_Object* ewkView = evas_object_smart_add(canvas, smart);
    if (!ewkView)
        return 0;

    EWK_VIEW_SD_GET(ewkView, smartData);
    if (!smartData) {
        evas_object_del(ewkView);
        return 0;
    }

    EWK_VIEW_IMPL_GET(smartData, impl);
    if (!impl) {
        evas_object_del(ewkView);
        return 0;
    }

    return ewkView;
}

/**
 * @internal
 * Constructs a ewk_view Evas_Object with WKType parameters.
 */
Evas_Object* ewk_view_base_add(Evas* canvas, WKContextRef contextRef, WKPageGroupRef pageGroupRef)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(contextRef, 0);

    Evas_Object* ewkView = _ewk_view_add_with_smart(canvas, _ewk_view_smart_class_new());
    if (!ewkView)
        return 0;

    _ewk_view_initialize(ewkView, Ewk_Context::create(contextRef), pageGroupRef);

    return ewkView;
}

Evas_Object* ewk_view_add_with_session_data(Evas* canvas, const char* data, unsigned length)
{
#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
    Ewk_Context* context = ewk_context_default_get();
    Evas_Smart* smart = _ewk_view_smart_class_new();

    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smart, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(context, 0);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[BackForward] length = %u", length);
#endif

    Evas_Object* ewkView = _ewk_view_add_with_smart(canvas, smart);
    if (!ewkView)
        return 0;

#if ENABLE(TIZEN_WEBKIT2_CREATE_VIEW_WITH_CREATED_PAGE_GROUP_WITH_IDENTIFIER)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);
    _ewk_view_initialize(ewkView, context, toAPI(impl->pageGroup.get()), data, length);
#else
    _ewk_view_initialize(ewkView, context, 0, data, length);
#endif

    return ewkView;
#else // ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
    UNUSED_PARAM(data);
    UNUSED_PARAM(length);
    return ewk_view_add(canvas);
#endif // ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
}

Evas_Object* ewk_view_smart_add(Evas* canvas, Evas_Smart* smart, Ewk_Context* context)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smart, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(context, 0);

    Evas_Object* ewkView = _ewk_view_add_with_smart(canvas, smart);
    if (!ewkView)
        return 0;

#if ENABLE(TIZEN_WEBKIT2_CREATE_VIEW_WITH_CREATED_PAGE_GROUP_WITH_IDENTIFIER)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);
    _ewk_view_initialize(ewkView, context, toAPI(impl->pageGroup.get()));
#else
    _ewk_view_initialize(ewkView, context, 0);
#endif

    return ewkView;
}

Evas_Object* ewk_view_add_with_context(Evas* canvas, Ewk_Context* context)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(context, 0);
    return ewk_view_smart_add(canvas, _ewk_view_smart_class_new(), context);
}

Evas_Object* ewk_view_add(Evas* canvas)
{
    return ewk_view_add_with_context(canvas, ewk_context_default_get());
}

Evas_Object* ewk_view_add_in_incognito_mode(Evas* canvas)
{
    Evas_Object* incognitoView = ewk_view_add_with_context(canvas, ewk_context_default_get());
    ewk_settings_private_browsing_enabled_set(ewk_view_settings_get(incognitoView), true);
    return incognitoView;
}

Ewk_Context* ewk_view_context_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return impl->ewkContext();
}

Eina_Bool ewk_view_url_set(Evas_Object* ewkView, const char* url)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(url, false);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_SECURE_LOGI("[Loader] url[%.256s]", url);
#endif

    impl->pageProxy->loadURL(String::fromUTF8(url));
    impl->informURLChange();

#if ENABLE(TIZEN_EDGE_SUPPORT)
    impl->edgeEffect()->hide(String::fromUTF8("edge,top"));
#endif

    return true;
}

const char* ewk_view_url_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return impl->url();
}

const char *ewk_view_icon_url_get(const Evas_Object *ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return impl->faviconURL();
}

Eina_Bool ewk_view_reload(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] reload is called");
#endif

    impl->pageClient->prepareRestoredVisibleContectRect();
    impl->pageProxy->reload(/*reloadFromOrigin*/ false);
    impl->informURLChange();

    return true;
}

Eina_Bool ewk_view_reload_bypass_cache(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] reload by pass is called");
#endif

    impl->pageProxy->reload(/*reloadFromOrigin*/ true);
    impl->informURLChange();

    return true;
}

Eina_Bool ewk_view_stop(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] stop loading is called");
#endif

    impl->pageProxy->stopLoading();

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    ewk_view_form_password_data_fill(ewkView);
#endif

    return true;
}

Ewk_Settings* ewk_view_settings_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return impl->settings();
}

const char* ewk_view_title_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return impl->title();
}

/**
 * @internal
 * Reports that the requested text was found.
 *
 * Emits signal: "text,found" with the number of matches.
 */
void ewk_view_text_found(Evas_Object* ewkView, unsigned int matchCount)
{
    evas_object_smart_callback_call(ewkView, "text,found", &matchCount);
}

double ewk_view_load_progress_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, -1.0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, -1.0);

    return impl->pageProxy->estimatedProgress();
}

Eina_Bool ewk_view_scale_set(Evas_Object* ewkView, double scaleFactor, int x, int y)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if PLATFORM(TIZEN)
    // FIXME
    int centerX = x;
    int centerY = y;
    scaleFactor = impl->pageClient->adjustScaleWithViewport(scaleFactor);

    IntPoint scrollPosition = impl->pageClient->scrollPosition();
    double scaleDifference = scaleFactor / impl->pageClient->scaleFactor();
    int newScrollX = (scrollPosition.x() + centerX - smartData->view.x) * scaleDifference - (centerX - smartData->view.x);
    int newScrollY = (scrollPosition.y() + centerY - smartData->view.y) * scaleDifference - (centerY - smartData->view.y);

    impl->pageProxy->scale(scaleFactor, IntPoint(newScrollX, newScrollY));
#else
    impl->pageProxy->scalePage(scaleFactor, IntPoint(x, y));
#endif
    return true;
}

double ewk_view_scale_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, -1);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, -1);

#if PLATFORM(TIZEN)
    return impl->pageProxy->scaleFactor();
#else
    return impl->pageProxy->pageScaleFactor();
#endif
}

Eina_Bool ewk_view_device_pixel_ratio_set(Evas_Object* ewkView, float ratio)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->pageProxy->setCustomDeviceScaleFactor(ratio);

    return true;
}

float ewk_view_device_pixel_ratio_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, -1.0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, -1.0);

    return impl->pageProxy->deviceScaleFactor();
}

void ewk_view_theme_set(Evas_Object* ewkView, const char* path)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->setThemePath(path);
}

const char* ewk_view_theme_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return impl->themePath();
}

Eina_Bool ewk_view_back(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] ewk_view_back is called");
#endif

    WebPageProxy* page = impl->pageProxy.get();
    if (page->canGoBack()) {
#if ENABLE(TIZEN_SCREEN_READER)
        // TTS mode is ON and page update happens from the H/W back key need to clear the Screen reader rect.
        if (ScreenReaderProxy::screenReader().focusRing()) {
            page->clearScreenReaderFocus();
            ScreenReaderProxy::screenReader().focusRing()->hide(false);
        }
#endif
        page->goBack();
        return true;
    }

    return false;
}

Eina_Bool ewk_view_forward(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] ewk_view_forward is called");
#endif

    WebPageProxy* page = impl->pageProxy.get();
    if (page->canGoForward()) {
        page->goForward();
        return true;
    }

    return false;
}

Eina_Bool ewk_view_intent_deliver(Evas_Object* ewkView, Ewk_Intent* intent)
{
#if ENABLE(WEB_INTENTS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(intent, false);

    WebPageProxy* page = impl->pageProxy.get();
    page->deliverIntentToFrame(page->mainFrame(), intent->webIntentData());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_back_possible(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->pageProxy->canGoBack();
}

Eina_Bool ewk_view_forward_possible(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->pageProxy->canGoForward();
}

Ewk_Back_Forward_List* ewk_view_back_forward_list_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return impl->backForwardList.get();
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
void ewk_view_text_change_in_textfield(Evas_Object* ewkView, const String& name, const String& value, bool isInputInForm, bool isForcePopupShow, bool isHandleEvent)
#else
void ewk_view_text_change_in_textfield(Evas_Object* ewkView, const String& name, const String& value)
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if ((!isForcePopupShow && value.isEmpty()) || impl->pageProxy->editorState().isInPasswordField) {
        smartData->api->formdata_candidate_hide(smartData);
        return;
    }

    Eina_List* optionList = 0;
    Vector<String> popupOptionLists = impl->pageProxy->getFocusedInputElementDataList();
    std::sort(popupOptionLists.begin(), popupOptionLists.end(), WTF::codePointCompareLessThan);
    for (size_t i = 0; i < popupOptionLists.size(); ++i) {
        if (value.isEmpty() || popupOptionLists[i].startsWith(value, false))
            optionList = eina_list_append(optionList, eina_stringshare_add(popupOptionLists[i].utf8().data()));
    }

    Eina_List* candidateList = 0;
    if (ewk_settings_form_candidate_data_enabled_get(ewk_view_settings_get(ewkView))) {
        Vector<String> popupCandidates;
        ewk_view_form_candidate_data_get(ewkView, name, popupCandidates);
        std::sort(popupCandidates.begin(), popupCandidates.end(), WTF::codePointCompareLessThan);

        for (size_t i = 0; i < popupCandidates.size(); ++i) {
            if (value.isEmpty() || popupCandidates[i].startsWith(value, false))
                candidateList = eina_list_append(candidateList, eina_stringshare_add(popupCandidates[i].utf8().data()));
        }
    }

    Vector<AutoFillPopupItem> formData;

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    if (ewk_settings_form_profile_data_enabled_get(ewk_view_settings_get(ewkView)) && isInputInForm) {
        Vector<std::pair<int, std::pair<String, String> > > profileFormCandidates;
        ewk_view_profile_form_candidate_data_get(ewkView, name, value, profileFormCandidates);
        for (size_t ittr = 0; ittr < profileFormCandidates.size(); ++ittr) {
            std::pair<int, std::pair<String, String> > fullData(profileFormCandidates[ittr]);
            std::pair<String, String> dispStrings(fullData.second);
            formData.append(AutoFillPopupItem(dispStrings.first, dispStrings.second, fullData.first, profileAutoFill));
        }
    }
#endif

    Eina_List* list;
    void* data;
    if (eina_list_count(optionList)) {
        EINA_LIST_FOREACH(optionList, list, data)
            formData.append(AutoFillPopupItem(String::fromUTF8(static_cast<char*>(data)), dataListAutoFill));
    }

    if (eina_list_count(candidateList) && ewk_settings_form_candidate_data_enabled_get(ewk_view_settings_get(ewkView))) {
        EINA_LIST_FOREACH(candidateList, list, data)
            formData.append(AutoFillPopupItem(String::fromUTF8(static_cast<char*>(data)), candidateAutoFill));
    }

    if (!formData.size()) {
        if(smartData->api->formdata_candidate_is_showing(smartData))
            smartData->api->formdata_candidate_hide(smartData);
        return;
    }

    if (formData[0].itemtype == profileAutoFill)
        impl->pageClient->updateAutoFillPopup(formData);
    else {
        Eina_List* autoFillList = 0;

        if (eina_list_count(optionList)) {
            EINA_LIST_FOREACH(optionList, list, data)
                autoFillList = eina_list_append(autoFillList, eina_stringshare_add(static_cast<char*>(data)));
        }
        if (eina_list_count(candidateList)) {
            EINA_LIST_FOREACH(candidateList, list, data)
                autoFillList = eina_list_append(autoFillList, eina_stringshare_add(static_cast<char*>(data)));
        }
        smartData->api->formdata_candidate_update_data(smartData, autoFillList);
    }

    if (isHandleEvent) {
        IntRect inputFieldRect = impl->focusedNodeRect(true);
        smartData->api->formdata_candidate_show(smartData, inputFieldRect.x(), inputFieldRect.y(), inputFieldRect.width(), inputFieldRect.height());
    }
}

void ewk_view_form_data_add(Evas_Object* ewkView, WKFormDataRef formData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    // if private browsing is enabled, return
    if (ewk_settings_private_browsing_enabled_get(ewk_view_settings_get(ewkView)))
        return;

    // if isAutoComplete is off then return
    if (!toImpl(formData)->shouldAutocomplete())
        return;

    FormDatabase* formDatabase = ewk_view_context_get(ewkView)->formDatabase();

    // Save Candidate Form Data
    if (ewk_settings_form_candidate_data_enabled_get(ewk_view_settings_get(ewkView)))
        formDatabase->addCandidateFormData(formData);

    // if it's password form data and autofill password form is enabled, then return
    if (!toImpl(formData)->containsPassword() || !ewk_settings_autofill_password_form_enabled_get(ewk_view_settings_get(ewkView)))
        return;

    // Check CAPTCHA
    size_t size = toImpl(formData)->values().size();
    bool didFoundPassword = false;
    for (size_t i = 0; i < size; i++) {
        if (toImpl(formData)->values()[i].m_type.isEmpty())
            continue;

        if (toImpl(formData)->values()[i].m_type == String::fromUTF8("password")) {
            didFoundPassword = true;
            continue;
        }

        if (!didFoundPassword)
            continue;

        if (toImpl(formData)->values()[i].m_type != String::fromUTF8("text"))
            continue;
        if (toImpl(formData)->values()[i].m_name.isEmpty())
            continue;
        if (toImpl(formData)->values()[i].m_name.contains(String::fromUTF8("captcha"), false))
            return;
    }

    // For candidate form data add
    String url = String::fromUTF8(ewk_view_url_get(ewkView));
    KURL kurl(KURL(),url);
    url = kurl.host();

    // If url exists in NeverPassword table
    Vector <String> urlList;
    if (ewk_view_context_get(ewkView)->formDatabase()->getPasswordNeverURLs(urlList)) {
        for (size_t i = 0; i < urlList.size(); i++)
            if (url == urlList[i])
                return;
    }

    // For password form data add
    if (!impl->passwordContext)
        return;

    // For Checking password form have this data already.
    Vector<WebFormData::Data::ValueData> passwordFormData;
    String formID;
    String sourceFrameURL;
    bool useFingerPrint = false;
    String buttonID;
    formDatabase->getPasswordFormValuesForURL(url.utf8().data(), formID, sourceFrameURL, useFingerPrint, buttonID, passwordFormData);

    WebFormData* data = toImpl(formData);
    if (passwordFormData.size() == data->values().size()) {
        size_t passFormSize = passwordFormData.size();
        size_t formDataSize = data->values().size();
        size_t entryCount = 0;
        for (size_t i = 0; i < passFormSize; i++)
            for (size_t j = 0; j < formDataSize; j++)
                if ((passwordFormData[i].m_name == data->values()[j].m_name)
                    && (passwordFormData[i].m_type == (data->values()[j].m_type.isEmpty() ? String("text") : data->values()[j].m_type))
                    && (passwordFormData[i].m_value == data->values()[j].m_value)) {
                    entryCount++;
                    break;
                }

        if (entryCount == passwordFormData.size())
            return; //entry is already present in the database.
    }

    ewk_view_context_get(ewkView)->setPasswordData(url, formData);
    if (impl->passwordContext->passwordConfirmPopupCallback)
        impl->passwordContext->passwordConfirmPopupCallback(impl->passwordContext->ewkView, 0, impl->passwordContext->userData);
    else
        ewk_view_context_get(ewkView)->passworSaveConfirmPopupCallbackCall(ewkView);
}

#define JS_FORM_PASWORD_FILL \
    "try { " \
        "function EwkfillPasswordInForm(doc) { " \
            "var formTags = doc.forms; " \
            "for (var i = 0; i < formTags.length; i++) { " \
                "var inputField;" \
                "%s " \
            "} " \
        "} " \
        "function EwkfillPasswordInDocument(doc) { " \
            "EwkfillPasswordInForm(doc); " \
            "var iframeTags = doc.getElementsByTagName(\"iframe\"); " \
            "for (var i = 0; i < iframeTags.length; i++) " \
                "try { EwkfillPasswordInDocument(iframeTags[i].contentDocument); } catch(e) { continue; } " \
        "} " \
        "EwkfillPasswordInDocument(document); " \
    "} catch(e) { } "

#define JS_FORM_PASSWORD_FILL_VALUE_SET \
    "inputField = formTags[i][\"%s\"]; " \
    "if (typeof inputField != \"undefined\" " \
        "&& (inputField.tagName && inputField.tagName.toLowerCase() in {\"input\":1, \"select\":1}) " \
        "&& (inputField.type && inputField.type.toLowerCase() in {\"text\":1, \"password\":1, \"email\":1, \"select-one\":1})) {" \
        "inputField.focus(); " \
        "inputField.value = \"%s\"; " \
        "document.execCommand(\"InsertText\", false, \"\"); " \
        "inputField.blur(); " \
    "} "

Eina_Bool ewk_view_web_login_request(Evas_Object* ewkView)
{
    return false;
}

void ewk_view_form_password_data_fill(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!ewk_settings_autofill_password_form_enabled_get(ewk_view_settings_get(ewkView)) || ewk_settings_private_browsing_enabled_get(ewk_view_settings_get(ewkView)))
        return;

    String url = String::fromUTF8(ewk_view_url_get(ewkView));
    KURL kurl(KURL(),url);
    url = kurl.host();

    Vector<WebFormData::Data::ValueData> passwordFormData;
    FormDatabase* formDatabase = ewk_view_context_get(ewkView)->formDatabase();
    String formID;
    String sourceFrameURL;
    bool useFingerPrint = false;
    String buttonID;
    formDatabase->getPasswordFormValuesForURL(url.utf8().data(), formID, sourceFrameURL, useFingerPrint, buttonID, passwordFormData);

    if (!passwordFormData.size())
        return;

    String jsSetValue;
    for (size_t i = 0; i < passwordFormData.size(); i++)
        jsSetValue += String::format(JS_FORM_PASSWORD_FILL_VALUE_SET, passwordFormData[i].m_name.utf8().data(), passwordFormData[i].m_value.utf8().data());

    String passwordFormAutofill = String::format(JS_FORM_PASWORD_FILL, jsSetValue.utf8().data());
    ewk_view_script_execute_in_focused_frame(ewkView, passwordFormAutofill.utf8().data(), 0, 0);
}

void ewk_view_form_candidate_data_get(Evas_Object* ewkView, const String& name, Vector<String>& candidates)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    ewk_view_context_get(ewkView)->formDatabase()->getCandidateFormDataForName(name, candidates);
}

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
void ewk_view_profile_form_candidate_data_get(Evas_Object* ewkView, const String& name, const String& value, Vector<std::pair<int, std::pair<String, String> > > & candidates)
{
    ewk_view_context_get(ewkView)->candidateProfileFormData(name, value, candidates);
}

void ewk_view_profile_form_candidate_set_to_form(Evas_Object* ewkView, const int& profileID)
{
    String formAutoFillString;
    ewk_view_context_get(ewkView)->setProfileFormCanditateToForm(profileID, formAutoFillString);
    if (!(formAutoFillString.isEmpty()))
        ewk_view_script_execute(ewkView, formAutoFillString.utf8().data(), 0, 0);
}
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif

Eina_Bool ewk_view_html_string_load(Evas_Object* ewkView, const char* html, const char* base_url, const char* unreachable_url)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(html, false);

    if (unreachable_url && *unreachable_url)
        impl->pageProxy->loadAlternateHTMLString(String::fromUTF8(html), base_url ? String::fromUTF8(base_url) : "", String::fromUTF8(unreachable_url));
    else
        impl->pageProxy->loadHTMLString(String::fromUTF8(html), base_url ? String::fromUTF8(base_url) : "");

    impl->informURLChange();

    return true;
}

const char* ewk_view_custom_encoding_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return impl->customTextEncodingName();
}

Eina_Bool ewk_view_custom_encoding_set(Evas_Object* ewkView, const char* encoding)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->setCustomTextEncodingName(encoding);

    return true;
}

#if PLATFORM(TIZEN)
// FIXME: It should be removed.
WKPageRef ewk_view_WKPage_get(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return toAPI(impl->pageProxy.get());
}

Eina_Bool ewk_view_mouse_events_enabled_set(Evas_Object* ewkView, Eina_Bool enabled)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->setMouseEventsEnabled(!!enabled);

    return true;
}

Eina_Bool ewk_view_mouse_events_enabled_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->mouseEventsEnabled();
}

Eina_Bool ewk_view_color_picker_color_set(Evas_Object* ewkView, int r, int g, int b, int a)
{
#if ENABLE(INPUT_TYPE_COLOR)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->setColorPickerColor(WebCore::Color(r, g, b, a));
#else
    return false;
#endif
}

static Eina_Bool _ewk_view_default_javascript_alert(Evas_Object* ewkView, const char* alertText, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->javascriptPopup->alert(alertText);
}

static Eina_Bool _ewk_view_default_javascript_confirm(Evas_Object* ewkView, const char* message, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->javascriptPopup->confirm(message);
}

static Eina_Bool _ewk_view_default_javascript_prompt(Evas_Object* ewkView, const char* message, const char* defaultValue, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->javascriptPopup->prompt(message, defaultValue);
}

#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
static Eina_Bool _ewk_view_default_before_unload_confirm_panel(Evas_Object* ewkView, const char* message, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->javascriptPopup->beforeUnloadConfirmPanel(message);
}
#endif

static Eina_Bool _ewk_view_default_open_panel(Evas_Object* ewkView, Eina_Bool allow_multiple_files, Eina_List *accepted_mime_types, const char* capture, void* userData)
{
#if ENABLE(TIZEN_OPEN_PANEL)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!ewk_settings_extra_feature_get(impl->settings(), "openpanel,enable"))
        return false;
    else
        return impl->openPanel->openPanel(ewkView, allow_multiple_files, accepted_mime_types, capture, impl);
#else
    return false;
#endif
}

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
Eina_Bool _ewk_view_popup_menu_show(Ewk_View_Smart_Data* smartData, Eina_Rectangle rect, Ewk_Text_Direction text_direction, double page_scale_factor, Eina_List* items, int selectedIndex)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    if (impl->pageProxy->formIsNavigating()) {
        impl->popupPicker->multiSelect = false;
        _ewk_view_popup_menu_update(smartData, rect, text_direction, items, selectedIndex);
        impl->pageProxy->setFormIsNavigating(false);
        return false;
    }
#endif

    if (impl->popupPicker)
        ewk_popup_picker_del(impl->popupPicker);

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    impl->popupPicker = ewk_popup_picker_new(smartData->self, items, selectedIndex, false);
#else
    impl->popupPicker = ewk_popup_picker_new(smartData->self, items, selectedIndex);
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    ewk_popup_picker_buttons_update(impl->popupPicker, impl->formNavigation.position, impl->formNavigation.count, false);
#endif

    return true;
}

#if ENABLE(TIZEN_MULTIPLE_SELECT)
Eina_Bool _ewk_view_multiple_popup_menu_show(Ewk_View_Smart_Data* smartData, Eina_Rectangle rect, Ewk_Text_Direction text_direction, double page_scale_factor, Eina_List* items)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    if (impl->pageProxy->formIsNavigating()) {
        impl->popupPicker->multiSelect = true;
        _ewk_view_popup_menu_update(smartData, rect, text_direction, items, 0);
        impl->pageProxy->setFormIsNavigating(false);
        return false;
    }
#endif

    if (impl->popupPicker)
        ewk_popup_picker_del(impl->popupPicker);

    impl->popupPicker = ewk_popup_picker_new(smartData->self, items, 0, true);

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    ewk_popup_picker_buttons_update(impl->popupPicker, impl->formNavigation.position, impl->formNavigation.count, false);
#endif

    return true;
}
#endif

Eina_Bool _ewk_view_popup_menu_hide(Ewk_View_Smart_Data* smartData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->popupPicker)
        return false;

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    if (impl->pageProxy->formIsNavigating())
        return true;
#endif

    ewk_popup_picker_del(impl->popupPicker);
    impl->popupPicker = 0;

    return true;
}

Eina_Bool _ewk_view_popup_menu_update(Ewk_View_Smart_Data* smartData, Eina_Rectangle rect, Ewk_Text_Direction text_direction, Eina_List* items, int selectedIndex)
{
    // FIXME: The rect should be updated if it was changed

    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->popupPicker)
        return false;

    ewk_popup_picker_update(smartData->self, impl->popupPicker, items, selectedIndex);
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    ewk_popup_picker_buttons_update(impl->popupPicker, impl->formNavigation.position, impl->formNavigation.count, false);
#endif

    return true;
}
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
static Eina_Bool _ewk_view_input_picker_show(Ewk_View_Smart_Data* smartData, Ewk_Input_Type inputType, const char* inputValue)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->inputPicker->show(inputType, inputValue);
    return true;
}
#endif

#if ENABLE(TIZEN_INPUT_COLOR_PICKER)
static Eina_Bool _ewk_input_picker_color_request(Ewk_View_Smart_Data* smartData, int r, int g, int b, int a)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->inputPicker->showColorPicker(r, g, b, a);
}

static Eina_Bool _ewk_input_picker_color_dismiss(Ewk_View_Smart_Data* smartData)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->inputPicker->removePickerDelayed();
    return true;
}
#endif

PageClientImpl* ewkViewGetPageClient(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return impl->pageClient.get();
}

double ewk_view_text_zoom_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 1);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 1);

    return WKPageGetTextZoomFactor(toAPI(impl->pageProxy.get()));
}

Eina_Bool ewk_view_text_zoom_set(Evas_Object* ewkView, double textZoomFactor)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    WKPageSetTextZoomFactor(toAPI(impl->pageProxy.get()), textZoomFactor);
    return true;
}

Ewk_Frame_Ref ewk_view_main_frame_get(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return static_cast<Ewk_Frame_Ref>(WKPageGetMainFrame(toAPI(impl->pageProxy.get())));
}

Ewk_Frame_Ref ewk_view_focused_frame_get(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    return static_cast<Ewk_Frame_Ref>(WKPageGetFocusedFrame(toAPI(impl->pageProxy.get())));
}

JSGlobalContextRef ewkViewGetJavascriptGlobalContext(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    if (!impl->javascriptGlobalContext)
        impl->javascriptGlobalContext = JSGlobalContextCreate(0);
    return impl->javascriptGlobalContext;
}

#if ENABLE(TIZEN_ALWAYS_TOUCH_EVENT)
#define JS_SET_ALWASY_TOUCHEVENT_ENABLED \
    "try { " \
        "var tizenTouchEventEnabledRetryCount=0; " \
        "function setTizenAllwaysTouchEvent() { " \
            "if (document && document.body) { " \
                "document.body.addEventListener('touchstart', function(){}); " \
            "} else { " \
                "tizenTouchEventEnabledRetryCount++; " \
                "if (tizenTouchEventEnabledRetryCount<3) setTimeout(setTizenAllwaysTouchEvent, 500); " \
            "} " \
        "} " \
        "setTizenAllwaysTouchEvent(); " \
    "} catch(e) { } "
#endif

void ewkViewLoadCommitted(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    impl->gestureClient->reset();
#endif
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    if (impl->focusRing())
        impl->focusRing()->hide(false);
#endif
#if ENABLE(TIZEN_SCREEN_READER)
    if (ScreenReaderProxy::screenReader().focusRing()) {
        WebPageProxy* page = impl->pageProxy.get();
        if (page)
            page->clearScreenReaderFocus();
        ScreenReaderProxy::screenReader().focusRing()->hide(false);
    }
#endif

#if ENABLE(TIZEN_ISF_PORT)
    impl->inputMethodContext()->hideIMFContext();
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    if(smartData->api->formdata_candidate_is_showing(smartData))
        smartData->api->formdata_candidate_hide(smartData);
#endif
#if ENABLE(TIZEN_ALWAYS_TOUCH_EVENT)
    ewk_view_script_execute(ewkView, JS_SET_ALWASY_TOUCHEVENT_ENABLED, 0, 0);
#endif
    impl->informURLChange();
    evas_object_smart_callback_call(ewkView, "load,committed", 0);
}

void ewkViewLoadError(Evas_Object* ewkView, WKErrorRef error)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    OwnPtr<Ewk_Error> ewkError = Ewk_Error::create(error);
    ewk_error_load_error_page(ewkError.get(), toAPI(impl->pageProxy.get()));

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] evas_object_smart_callback_call load,error");
#endif

    evas_object_smart_callback_call(ewkView, "load,error", ewkError.get());
}

void ewkViewDidFirstVisuallyNonEmptyLayout(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    impl->gestureClient->reset();
#endif
    evas_object_smart_callback_call(ewkView, "load,nonemptylayout,finished", 0);
}

void ewkViewDidReceiveAuthenticationChallenge(Evas_Object* ewkView, Ewk_Auth_Challenge* authChallenge)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (impl->authChallenge)
        ewkAuthChallengeDelete(impl->authChallenge);
    impl->authChallenge = authChallenge;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] evas_object_smart_callback_call authentication,challenge");
#endif

    evas_object_smart_callback_call(ewkView, "authentication,challenge", impl->authChallenge);
}

void ewk_view_process_crashed(Evas_Object* ewkView)
{
    bool handled = false;
    evas_object_smart_callback_call(ewkView, "process,crashed", &handled);

    if (!handled)
        exit(0);
}

#if ENABLE(TIZEN_SQL_DATABASE)
static Eina_Bool _ewk_view_default_exceeded_database_quota_permission(Evas_Object* ewkView, Ewk_Security_Origin* origin, const char* databaseName, unsigned long long expectedQuota, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    ExceededDatabaseQuotaPermissionPopup* popup = new ExceededDatabaseQuotaPermissionPopup(origin, exceededDatabaseQuotaPermissionText(String::fromUTF8(ewk_security_origin_host_get(origin))));
    impl->permissionPopupManager->addPermissionRequest(popup);
    return true;
}

bool ewkViewExceededDatabaseQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, WKStringRef databaseName, unsigned long long expectedUsage)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->exceededDatabaseQuotaContext || !impl->exceededDatabaseQuotaContext->exceededDatabaseQuotaCallback)
        return false;

    TIZEN_LOGI("No error in prameter. Request to display user confirm popup. expectedUsage(%llu)", expectedUsage);
    impl->isWaitingForExceededQuotaPopupReply = true;
    if (impl->exceededQuotaOrigin)
        deleteSecurityOrigin(impl->exceededQuotaOrigin);
    impl->exceededQuotaOrigin = createSecurityOrigin(origin);

    int length = WKStringGetMaximumUTF8CStringSize(databaseName);
    OwnArrayPtr<char> databaseNameBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(databaseName, databaseNameBuffer.get(), length);

    return impl->exceededDatabaseQuotaContext->exceededDatabaseQuotaCallback(ewkView, impl->exceededQuotaOrigin, databaseNameBuffer.get(), expectedUsage, impl->exceededDatabaseQuotaContext->userData) == EINA_TRUE;
}
#endif

void ewk_view_notification_callback_set(Evas_Object* ewkView, Ewk_Notification_Show_Callback showCallback, Ewk_Notification_Cancel_Callback cancelCallback, void* userData)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->notificationShowContext)
        impl->notificationShowContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);

    if (!impl->notificationCancelContext)
        impl->notificationCancelContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);

    // showCallback
    impl->notificationShowContext->notificationShowCallback = showCallback;
    impl->notificationShowContext->ewkView = ewkView;
    impl->notificationShowContext->userData = userData;

    // cancelCallback
    impl->notificationCancelContext->notificationCancelCallback = cancelCallback;
    impl->notificationCancelContext->ewkView = ewkView;
    impl->notificationCancelContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(showCallback);
    UNUSED_PARAM(cancelCallback);
    UNUSED_PARAM(userData);
#endif
}

void ewk_view_notification_permission_callback_set(Evas_Object* ewkView, Ewk_View_Notification_Permission_Callback callback, void* userData)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->notificationPermissionContext)
        impl->notificationPermissionContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->notificationPermissionContext->notificationPermissionCallback = callback;
    impl->notificationPermissionContext->ewkView = ewkView;
    impl->notificationPermissionContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
#endif
}

#if ENABLE(TIZEN_NOTIFICATIONS)
void ewkViewCancelNotification(Evas_Object* ewkView, uint64_t notificationID)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    TIZEN_LOGI("notification,cancel");

    if (!impl->notificationCancelContext || !impl->notificationCancelContext->notificationCancelCallback)
        return;

    impl->notificationCancelContext->notificationCancelCallback(impl->notificationCancelContext->ewkView, notificationID, impl->notificationCancelContext->userData);
}

static Eina_Bool _ewk_view_default_show_notification(Evas_Object* ewkView, Ewk_Notification* ewkNotification, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->notificationManagerEfl->showNotification(ewkNotification);
    return true;
}

static Eina_Bool _ewk_view_default_cancel_notification(Evas_Object* ewkView, uint64_t ewkNotificationID, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->notificationManagerEfl->cancelNotification(ewkNotificationID);
    return true;
}

static Eina_Bool _ewk_view_default_notification_permission(Evas_Object* ewkView, Ewk_Notification_Permission_Request* notificationPermissionRequest, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    NotificationPermissionPopup* popup = new NotificationPermissionPopup(notificationPermissionRequest, ewk_notification_permission_request_origin_get(notificationPermissionRequest), notificationPermissionText(String::fromUTF8(ewk_security_origin_host_get(ewk_notification_permission_request_origin_get(notificationPermissionRequest)))));
    impl->permissionPopupManager->addPermissionRequest(popup);
    return true;
}

bool ewkViewRequestNotificationPermission(Evas_Object* ewkView, Ewk_Notification_Permission_Request* notificationPermissionRequest)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->notificationPermissionContext || !impl->notificationPermissionContext->notificationPermissionCallback)
        return false;

    impl->notificationPermissionContext->notificationPermissionCallback(impl->notificationPermissionContext->ewkView, notificationPermissionRequest, impl->notificationPermissionContext->userData);
    return true;
}

void ewkViewShowNotification(Evas_Object* ewkView, Ewk_Notification* notification)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    TIZEN_LOGI("notification,show");
    if (!impl->notificationShowContext || !impl->notificationShowContext->notificationShowCallback)
        return;

    Eina_List* listIterator=0;
    void* data=0;
    const char* replaceID = ewkNotificationGetReplaceID(notification);
    if(strlen(replaceID)) {
        EINA_LIST_FOREACH(impl->notifications, listIterator, data) {
            Ewk_Notification* notificationForReplace = static_cast<Ewk_Notification*>(data);
            if(!strcmp(ewkNotificationGetReplaceID(notificationForReplace), replaceID))
                ewkViewCancelNotification(ewkView, ewk_notification_id_get(notificationForReplace));
        }
    }
    impl->notifications = eina_list_append(impl->notifications, notification);
    impl->notificationShowContext->notificationShowCallback(impl->notificationShowContext->ewkView, notification, impl->notificationShowContext->userData);
}
#endif

#if ENABLE(TIZEN_REGISTER_PROTOCOL_HANDLER)
void ewkViewRegisterProtocolHandlers(Evas_Object* ewkView, const char* scheme, const char* baseUrl, const char* url, const char* title)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkView);

    TIZEN_LOGI("protocolhandler,registration,requested");
    Ewk_Custom_Handlers_Data* customHandlersData = ewkCustomHandlersCreateData(scheme, baseUrl, url, title);
    evas_object_smart_callback_call(ewkView, "protocolhandler,registration,requested", static_cast<void*>(customHandlersData));
    ewkCustomHandlersDeleteData(customHandlersData);
}
#endif

#if ENABLE(TIZEN_CUSTOM_SCHEME_HANDLER)
Ewk_Custom_Handlers_State ewkViewIsProtocolHandlerRegistered(Evas_Object* ewkView, const char* scheme, const char* baseURL, const char* url)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkView, EWK_CUSTOM_HANDLERS_DECLINED);

    TIZEN_LOGI("protocolhandler,isregistered");
    Ewk_Custom_Handlers_Data* customHandlersData = ewkCustomHandlersCreateData(scheme, baseURL, url);
    evas_object_smart_callback_call(ewkView, "protocolhandler,isregistered", static_cast<void*>(customHandlersData));

    Ewk_Custom_Handlers_State result;
    result = ewkGetCustomHandlersDataResult(customHandlersData);
    ewkCustomHandlersDeleteData(customHandlersData);

    return result;
}

void ewkViewUnregisterProtocolHandlers(Evas_Object* ewkView, const char* scheme, const char* baseURL, const char* url)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkView);

    TIZEN_LOGI("protocolhandler,unregistration,requested");
    Ewk_Custom_Handlers_Data* customHandlersData = ewkCustomHandlersCreateData(scheme, baseURL, url);
    evas_object_smart_callback_call(ewkView, "protocolhandler,unregistration,requested", static_cast<void*>(customHandlersData));
    ewkCustomHandlersDeleteData(customHandlersData);
}
#endif

#if ENABLE(TIZEN_REGISTER_CONTENT_HANDLER)
void ewkViewRegisterContentHandlers(Evas_Object* ewkView, const char* mimeType, const char* baseUrl, const char* url, const char* title)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkView);

    TIZEN_LOGI("contenthandler,registration,requested");
    Ewk_Custom_Handlers_Data* customHandlersData = ewkCustomHandlersCreateData(mimeType, baseUrl, url, title);
    evas_object_smart_callback_call(ewkView, "contenthandler,registration,requested", static_cast<void*>(customHandlersData));
    ewkCustomHandlersDeleteData(customHandlersData);
}

Ewk_Custom_Handlers_State ewkViewIsContentHandlerRegistered(Evas_Object* ewkView, const char* mimeType, const char* baseURL, const char* url)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkView, EWK_CUSTOM_HANDLERS_DECLINED);

    TIZEN_LOGI("contenthandler,isregistered");
    Ewk_Custom_Handlers_Data* customHandlersData = ewkCustomHandlersCreateData(mimeType, baseURL, url);
    evas_object_smart_callback_call(ewkView, "contenthandler,isregistered", static_cast<void*>(customHandlersData));

    Ewk_Custom_Handlers_State result;
    result = ewkGetCustomHandlersDataResult(customHandlersData);
    ewkCustomHandlersDeleteData(customHandlersData);

    return result;
}

void ewkViewUnregisterContentHandlers(Evas_Object* ewkView, const char* mimeType, const char* baseURL, const char* url)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkView);

    TIZEN_LOGI("contenthandler,unregistration,requested");
    Ewk_Custom_Handlers_Data* customHandlersData = ewkCustomHandlersCreateData(mimeType, baseURL, url);
    evas_object_smart_callback_call(ewkView, "contenthandler,unregistration,requested", static_cast<void*>(customHandlersData));
    ewkCustomHandlersDeleteData(customHandlersData);
}
#endif

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
bool ewkViewGetStandaloneStatus(Evas_Object* ewkView)
{
    TIZEN_LOGI("webapp,metatag,standalone");
    bool standalone = true;
    evas_object_smart_callback_call(ewkView, "webapp,metatag,standalone", (void*)&standalone);
    return standalone;
}
#endif

void ewk_view_user_media_permission_callback_set(Evas_Object* ewkView, Ewk_View_User_Media_Permission_Callback callback, void* userData)
{
#if ENABLE(TIZEN_MEDIA_STREAM)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->userMediaPermissionContext)
        impl->userMediaPermissionContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->userMediaPermissionContext->userMediaPermissionCallback = callback;
    impl->userMediaPermissionContext->ewkView = ewkView;
    impl->userMediaPermissionContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
#endif
}

#if ENABLE(TIZEN_MEDIA_STREAM)
static Eina_Bool _ewk_view_default_user_media_permission(Evas_Object* ewkView, Ewk_User_Media_Permission_Request* userMediaPermissionRequest, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    UserMediaPermissionPopup* popup = new UserMediaPermissionPopup(userMediaPermissionRequest, ewk_user_media_permission_request_origin_get(userMediaPermissionRequest), userMediaPermissionText(String::fromUTF8(ewk_security_origin_host_get(ewk_user_media_permission_request_origin_get(userMediaPermissionRequest)))));
    impl->permissionPopupManager->addPermissionRequest(popup);
    return true;
}

bool ewkViewRequestUserMediaPermission(Evas_Object* ewkView, Ewk_User_Media_Permission_Request* userMediaPermission)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->userMediaPermissionContext || !impl->userMediaPermissionContext->userMediaPermissionCallback)
        return false;

    impl->userMediaPermissionContext->userMediaPermissionCallback(impl->userMediaPermissionContext->ewkView, userMediaPermission, impl->userMediaPermissionContext->userData);
    return true;
}
#endif

#if ENABLE(TIZEN_JSBRIDGE_PLUGIN)
void ewkViewProcessJSBridgePlugin(Evas_Object* ewkView, WKStringRef request, WKStringRef message)
{
    int requestLength = WKStringGetMaximumUTF8CStringSize(request);
    OwnArrayPtr<char> requestBuffer = adoptArrayPtr(new char[requestLength]);
    int messageLength = WKStringGetMaximumUTF8CStringSize(message);
    OwnArrayPtr<char> messageBuffer = adoptArrayPtr(new char[messageLength]);
    WKStringGetUTF8CString(request, requestBuffer.get(), requestLength);
    WKStringGetUTF8CString(message, messageBuffer.get(), messageLength);

    evas_object_smart_callback_call(ewkView, requestBuffer.get(), static_cast<void*>(messageBuffer.get()));
}
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
void ewkViewSetCertificatePem(Evas_Object* ewkView, const char* certificate)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, sd);
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, impl);

    evas_object_smart_callback_call(ewkView, "certificate,pem,set", const_cast<char *>(certificate));
}

void ewkViewRequestCertificateConfirm(Evas_Object* ewkView, Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, sd);
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, impl);
    if (impl->certificatePolicyDecision)
        ewkCertificatePolicyDecisionDelete(impl->certificatePolicyDecision);
    impl->certificatePolicyDecision = certificatePolicyDecision;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] evas_object_smart_callback_call request,certificate,confirm");
#endif

    evas_object_smart_callback_call(ewkView, "request,certificate,confirm", certificatePolicyDecision);
}
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
void ewkViewRequestBlockedLoadingConfirm(Evas_Object* ewkView, Ewk_Content_Screening_Detection* contentScreeningDetection)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, sd);
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, impl);
    if (impl->contentScreeningDetection)
        ewkContentScreeningDetectionDelete(impl->contentScreeningDetection);
    impl->contentScreeningDetection = contentScreeningDetection;

    evas_object_smart_callback_call(ewkView, "request,malware,confirm", contentScreeningDetection);
}
#endif

void ewkViewCustomizeContextMenu(Evas_Object* ewkView, Ewk_Context_Menu* menu)
{
    evas_object_smart_callback_call(ewkView, "contextmenu,customize", static_cast<void*>(menu));
}

void ewkViewCustomContextMenuItemSelected(Evas_Object* ewkView, Ewk_Context_Menu_Item* item)
{
    evas_object_smart_callback_call(ewkView, "contextmenu,selected", static_cast<void*>(item));
}

void ewk_view_geolocation_permission_callback_set(Evas_Object* ewkView, Ewk_View_Geolocation_Permission_Callback callback, void* userData)
{
#if ENABLE(TIZEN_GEOLOCATION)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->geolocationPermissionContext)
        impl->geolocationPermissionContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->geolocationPermissionContext->geolocationPermissionCallback = callback;
    impl->geolocationPermissionContext->ewkView = ewkView;
    impl->geolocationPermissionContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
#endif
}

#if ENABLE(TIZEN_GEOLOCATION)
static Eina_Bool _ewk_view_default_geolocation_permission(Evas_Object* ewkView, Ewk_Geolocation_Permission_Request* geolocationPermissionRequest, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    GeolocationPermissionPopup* popup = new GeolocationPermissionPopup(geolocationPermissionRequest, ewk_geolocation_permission_request_origin_get(geolocationPermissionRequest), geolocationPermissionText(String::fromUTF8(ewk_security_origin_host_get(ewk_geolocation_permission_request_origin_get(geolocationPermissionRequest)))));
    impl->permissionPopupManager->addPermissionRequest(popup);
    return true;
}

bool ewkViewRequestGeolocationPermission(Evas_Object* ewkView, Ewk_Geolocation_Permission_Request* geolocationPermissionRequest)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->geolocationPermissionContext || !impl->geolocationPermissionContext->geolocationPermissionCallback)
        return false;

    impl->geolocationPermissionContext->geolocationPermissionCallback(impl->geolocationPermissionContext->ewkView, geolocationPermissionRequest, impl->geolocationPermissionContext->userData);
    return true;
}
#endif

void ewkViewFormSubmit(Evas_Object* ewkView, Ewk_Form_Data* formData)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] evas_object_smart_callback_call form,submit");
#endif

    evas_object_smart_callback_call(ewkView, "form,submit", formData);
}

void ewkViewPolicyNavigationDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, sd);
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, impl);
    if (impl->policyDecision)
        ewkPolicyDecisionDelete(impl->policyDecision);
    impl->policyDecision = policyDecision;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] evas_object_smart_callback_call policy,navigation,decide");
#endif

    evas_object_smart_callback_call(ewkView, "policy,navigation,decide", impl->policyDecision);
}

void ewkViewPolicyNewWindowDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, sd);
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, impl);
    if (impl->policyDecision)
        ewkPolicyDecisionDelete(impl->policyDecision);
    impl->policyDecision = policyDecision;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] evas_object_smart_callback_call policy,newwindow,decide");
#endif

    evas_object_smart_callback_call(ewkView, "policy,newwindow,decide", impl->policyDecision);
}

void ewkViewPolicyResponseDecide(Evas_Object* ewkView, Ewk_Policy_Decision* policyDecision)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, sd);
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, impl);
    if (impl->policyDecision)
        ewkPolicyDecisionDelete(impl->policyDecision);
    impl->policyDecision = policyDecision;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[Loader] evas_object_smart_callback_call policy,response,decide");
#endif

    evas_object_smart_callback_call(ewkView, "policy,response,decide", impl->policyDecision);
}

void ewkViewSendScrollEvent(Evas_Object* ewkView, int deltaX, int deltaY, int xVelocity, int yVelocity)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    IntPoint scrollPosition = impl->pageClient->scrollPosition();
    IntSize contentsSize = impl->pageProxy->contentsSize();
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    contentsSize.scale(impl->pageClient->scaleFactor());
#endif

    // some website's contents size is smaller than view size,
    // so we have to adjust contents size to view size for that case.
    if (contentsSize.width() < smartData->view.w)
        contentsSize.setWidth(smartData->view.w);
    if (contentsSize.height() < smartData->view.h)
        contentsSize.setHeight(smartData->view.h);

    // Call "scroll,down" if webview can be scrolled down.
    if (deltaY > 0 && (scrollPosition.y() + smartData->view.h) < contentsSize.height()) {
        evas_object_smart_callback_call(ewkView, "scroll,down", &yVelocity);
#if ENABLE(TIZEN_EDGE_SUPPORT)
        impl->edgeEffect()->hide(String::fromUTF8("edge,top"));
#endif
    // Call "scroll,up" if webview can be scrolled up.
    } else if (deltaY < 0 && scrollPosition.y() > 0) {
        evas_object_smart_callback_call(ewkView, "scroll,up", &yVelocity);
#if ENABLE(TIZEN_EDGE_SUPPORT)
        impl->edgeEffect()->hide(String::fromUTF8("edge,bottom"));
#endif
    }

    // Call "scroll,right" if webview can be scrolled down.
    if (deltaX > 0 && (scrollPosition.x() + smartData->view.w) < contentsSize.width()) {
        evas_object_smart_callback_call(ewkView, "scroll,right", &xVelocity);
#if ENABLE(TIZEN_EDGE_SUPPORT)
        impl->edgeEffect()->hide(String::fromUTF8("edge,left"));
#endif
    // Call "scroll,left" if webview can be scrolled up.
    } else if (deltaX < 0 && scrollPosition.x() > 0) {
        evas_object_smart_callback_call(ewkView, "scroll,left", &xVelocity);
#if ENABLE(TIZEN_EDGE_SUPPORT)
        impl->edgeEffect()->hide(String::fromUTF8("edge,right"));
#endif
    }
}

void ewkViewSendEdgeEvent(Evas_Object* ewkView, const IntPoint& scrollPosition, int deltaX, int deltaY)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

#if ENABLE(TIZEN_EDGE_SUPPORT)
    IntSize contentsSize = impl->pageProxy->contentsSize();
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    contentsSize.scale(impl->pageClient->scaleFactor());
#endif
    String scrollEdge;
    int maxScrollY = contentsSize.height() - smartData->view.h;
    if (deltaY < 0 && (scrollPosition.y() + deltaY) <= 0) {
        evas_object_smart_callback_call(ewkView, "edge,top", NULL);
        impl->isVerticalEdge = true;
        scrollEdge = String::fromUTF8("edge,top");
    } else if (deltaY > 0 && (scrollPosition.y() + deltaY) >= maxScrollY) {
        evas_object_smart_callback_call(ewkView, "edge,bottom", NULL);
        impl->isVerticalEdge = true;
        scrollEdge = String::fromUTF8("edge,bottom");
    }
    if (!scrollEdge.isEmpty() && maxScrollY > 0)
        impl->edgeEffect()->show(scrollEdge);

    scrollEdge = "";
    int maxScrollX = contentsSize.width() - smartData->view.w;
    if (deltaX < 0 && (scrollPosition.x() + deltaX) <= 0) {
        evas_object_smart_callback_call(ewkView, "edge,left", NULL);
        impl->isHorizontalEdge = true;
        scrollEdge = String::fromUTF8("edge,left");
    } else if (deltaX > 0 && (scrollPosition.x() + deltaX) >= maxScrollX) {
        evas_object_smart_callback_call(ewkView, "edge,right", NULL);
        impl->isHorizontalEdge = true;
        scrollEdge = String::fromUTF8("edge,right");
    }
    if (!scrollEdge.isEmpty() && maxScrollX > 0)
        impl->edgeEffect()->show(scrollEdge);
#endif
}

void ewkViewClearEdges(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_EDGE_SUPPORT)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->isVerticalEdge = false;
    impl->isHorizontalEdge = false;
#endif
}

void ewk_view_scale_range_get(Evas_Object* ewkView, double* minimumScale, double* maximumScale)
{
    EWK_VIEW_SD_GET(ewkView, smartData);
    if (!smartData || !smartData->priv) {
        if (minimumScale)
            *minimumScale = -1;
        if (maximumScale)
            *maximumScale = -1;
        return;
    }

    PageClientImpl::ViewportConstraints constraints = smartData->priv->pageClient->viewportConstraints();
    if (minimumScale)
        *minimumScale = constraints.minimumScale;
    if (maximumScale)
        *maximumScale = constraints.maximumScale;
}
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
static void popupPickerFocusInCallbackPrev(void *data, Evas_Object *obj, void *eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    evas_object_smart_callback_del(obj, "focus,in", popupPickerFocusInCallbackPrev);
    impl->pageProxy->moveFocus(impl->formNavigation.position - 1);
}

static void popupPickerFocusInCallbackNext(void *data, Evas_Object *obj, void *eventInfo)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    evas_object_smart_callback_del(obj, "focus,in", popupPickerFocusInCallbackNext);
    impl->pageProxy->moveFocus(impl->formNavigation.position + 1);
}

void ewk_view_form_navigate(Evas_Object* ewkView, bool direction)
{
    EWK_VIEW_SD_GET(ewkView, smartData);
    EWK_VIEW_IMPL_GET(smartData, impl);

    if (impl->pageProxy->formIsNavigating())
        return;

    ewk_popup_picker_buttons_update(impl->popupPicker, impl->formNavigation.position, impl->formNavigation.count, true);

    if ((direction && impl->formNavigation.nextState) || (!direction && impl->formNavigation.prevState))
         impl->pageProxy->setFormIsNavigating(true);

    if (!impl->pageProxy->formIsNavigating()) {
        if (direction)
            evas_object_smart_callback_add(elm_object_top_widget_get(elm_object_parent_widget_get(impl->popupPicker->parent)), "focus,in", popupPickerFocusInCallbackNext, smartData);
        else
            evas_object_smart_callback_add(elm_object_top_widget_get(elm_object_parent_widget_get(impl->popupPicker->parent)), "focus,in", popupPickerFocusInCallbackPrev, smartData);

        listClosed(impl->popupPicker, 0, 0);
        return;
    }

    listClosed(impl->popupPicker, 0, 0);
    if (direction)
        impl->pageProxy->moveFocus(impl->formNavigation.position + 1);
    else
        impl->pageProxy->moveFocus(impl->formNavigation.position - 1);
}
#endif

bool ewk_view_focused_node_adjust(Evas_Object* ewkView, Eina_Bool adjustForExternalKeyboard, Eina_Bool adjustForContentSizeChanged)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_ISF_PORT)
    // We should treat both of ECORE_IMF_INPUT_PANEL_STATE_SHOW and ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW
    // as IME is shown. ECORE_IMF_INPUT_PANEL_STATE_HIDE is ignored at here.
    // input field zoom should not work with below conditions
    // 1. view is not focused
    // 2. external keyboard is not connected(if adjustForExternalKeyboard is true)
    // 3. imfContext is null(if adjustForExternalKeyboard is false)
    // 4. input panel state is hidden(if adjustForExternalKeyboard is false)
    if ((!(static_cast<PageClient*>(impl->pageClient.get()))->isViewFocused()
        || (!adjustForExternalKeyboard && (!impl->inputMethodContext() || !impl->inputMethodContext()->isShow()))
        || (adjustForExternalKeyboard && !impl->inputMethodContext()->isShow())
        || (!adjustForContentSizeChanged && impl->pageProxy->editorState().isContentRichlyEditable))
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
        && !(impl->pageClient->isClipboardWindowOpened())
#endif
        )
        return false;
#endif

    if (impl->caretRect(false).isEmpty() || impl->focusedNodeRect(false).isEmpty())
        return false;

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    if (impl->focusRing())
        impl->focusRing()->hide();
#endif // #if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    return impl->inputFieldZoom->scheduleInputFieldZoom();
#endif
    return false;
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
static Eina_Bool _ewk_view_composite(void* data)
{
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(data);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!evas_object_visible_get(smartData->image)) {
        if (impl->compositionAnimator) {
            ecore_animator_del(impl->compositionAnimator);
            impl->compositionAnimator = 0;
        }
        return ECORE_CALLBACK_CANCEL;
    }

    Evas_Coord ow, oh;
    evas_object_image_size_get(smartData->image, &ow, &oh);
    uint8_t* pixels = static_cast<uint8_t*>(evas_object_image_data_get(smartData->image, true));

    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_ARGB32, ow, oh, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, ow)));
    RefPtr<cairo_t> context = adoptRef(cairo_create(surface.get()));

    cairo_save(context.get());
    cairo_set_operator(context.get(), CAIRO_OPERATOR_CLEAR);
    cairo_rectangle(context.get(), 0, 0, ow, oh);
    cairo_fill(context.get());
    cairo_restore(context.get());

    impl->pageClient->drawContents(context.get());

    evas_object_image_data_set(smartData->image, pixels);
    evas_object_image_data_update_add(smartData->image, 0, 0, ow, oh);

    if (impl->pageClient->notifiedNonemptyLayout() && impl->pageClient->canUpdateVisibleContentRect())
        ewkViewFrameRendered(smartData->self);

    if (impl->compositionAnimator) {
        ecore_animator_del(impl->compositionAnimator);
        impl->compositionAnimator = 0;
    }

    return ECORE_CALLBACK_CANCEL;
}
#endif

void ewk_view_mark_for_sync(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EINA_SAFETY_ON_NULL_RETURN(smartData->image);

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!ewk_view_is_opengl_backend(ewkView)) {
        if (!impl->compositionAnimator)
            impl->compositionAnimator = ecore_animator_add(_ewk_view_composite, smartData);
        return;
    }
#endif

    evas_object_image_pixels_dirty_set(smartData->image, true);
}

void ewk_view_data_update_add(Evas_Object* ewkView, int x, int y, int w, int h)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EINA_SAFETY_ON_NULL_RETURN(smartData->image);

    evas_object_image_data_update_add(smartData->image, x, y, w, h);
}

static void on_pixels_for_accelerated_compositing(void* data, Evas_Object* obj)
{
    Evas_Object* ewkView = static_cast<Evas_Object*>(data);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    impl->pageClient->drawContents();
}

bool ewk_view_image_native_surface_set(Evas_Object* ewkView, Evas_Native_Surface* nativeSurface)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!smartData->image)
        return false;

    evas_object_image_native_surface_set(smartData->image, nativeSurface);
    if (nativeSurface)
        evas_object_image_pixels_get_callback_set(smartData->image, on_pixels_for_accelerated_compositing, ewkView);
    else
        evas_object_image_pixels_get_callback_set(smartData->image, 0, 0);

    return true;
}
#endif

void ewk_view_suspend_painting(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->suspendedPainting) {
        impl->pageProxy->suspendPainting();
        impl->suspendedPainting = true;
    }
}

void ewk_view_resume_painting(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (impl->suspendedPainting) {
        impl->pageProxy->resumePainting();
        impl->suspendedPainting = false;
    }
}

void ewk_view_suspend(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    ewk_view_context_get(ewkView)->hidePasswordSaveConfirmPopup();
#endif

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
    if (impl->popupPicker)
        listClosed(impl->popupPicker, 0, 0);
#endif

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    if (impl->pageClient && impl->pageClient->isContextMenuVisible() && impl->pageProxy)
        impl->pageProxy->hideContextMenu();
#endif

    if (impl->pageProxy->estimatedProgress() < 1.0) {
        impl->suspendRequested = true;
        return;
    }
    impl->suspendRequested = false;

    ewk_view_suspend_painting(ewkView);

    // FIXME Workaround for suspend/resume while javascript popup is displayed.
    // condition ' && !private->isWaitingForJavaScriptPopupReply' is added to
    // skip suspendJavaScriptAndResource to prevent multiple ActiveDOMObject
    // suspend.
    // During the javascript popup is displayed, PageGroupLoadDeferrer is activated,
    // which suspends resource loading and scheduled tasks, including ActiveDOMObject.
    // When ewk_view_suspend() is called during the javascript popup, scheduled tasks
    // will be suspended again.
    // Multiple suspend of ActiveDOMObject makes unintended suspend/resume status of
    // the ActiveDOMObject.
    if (!impl->suspendedResources && (!impl->isWaitingForJavaScriptPopupReply || !impl->isWaitingForApplicationCachePermission || !impl->isWaitingForExceededQuotaPopupReply)) {
        impl->pageProxy->suspendAnimations();
        impl->pageProxy->suspendJavaScriptAndResource();
        impl->suspendedResources = true;
    }

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
    impl->pageProxy->suspendPlugin();
#endif

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
    // Since webview is suspended this is good time to save current history item
    // This will save current view state (scroll position and scale factor)
    impl->pageProxy->saveHistoryItem();
#endif
}

void ewk_view_resume(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (impl->suspendRequested) {
        impl->suspendRequested = false;
        return;
    }

    ewk_view_resume_painting(ewkView);

    // FIXME Workaround for suspend/resume while javascript popup is displayed.
    // condition ' && !private->isWaitingForJavaScriptPopupReply' is added to
    // skip suspendJavaScriptAndResource to prevent multiple ActiveDOMObject
    // suspend.
    // During the javascript popup is displayed, PageGroupLoadDeferrer is activated,
    // which suspends resource loading and scheduled tasks, including ActiveDOMObject.
    // When ewk_view_suspend() is called during the javascript popup, scheduled tasks
    // will be suspended again.
    // Multiple suspend of ActiveDOMObject makes unintended suspend/resume status of
    // the ActiveDOMObject.
    if (impl->suspendedResources && (!impl->isWaitingForJavaScriptPopupReply || !impl->isWaitingForApplicationCachePermission || !impl->isWaitingForExceededQuotaPopupReply)) {
        impl->pageProxy->resumeAnimations();
        impl->pageProxy->resumeJavaScriptAndResource();
        impl->suspendedResources = false;
    }

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
    impl->pageProxy->resumePlugin();
#endif
}

Eina_Bool ewk_view_url_request_set(Evas_Object* ewkView, const char* url, Ewk_Http_Method method, Eina_Hash* headers, const char* body)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(url, false);

    ResourceRequest request(String::fromUTF8(url));

    switch (method) {
    case EWK_HTTP_METHOD_GET:
        request.setHTTPMethod("GET");
        break;
    case EWK_HTTP_METHOD_HEAD:
        request.setHTTPMethod("HEAD");
        break;
    case EWK_HTTP_METHOD_POST:
        request.setHTTPMethod("POST");
        break;
    case EWK_HTTP_METHOD_PUT:
        request.setHTTPMethod("PUT");
        break;
    case EWK_HTTP_METHOD_DELETE:
        request.setHTTPMethod("DELETE");
        break;
    default:
        return false;
    }

    if (headers) {
        Eina_Iterator* it = eina_hash_iterator_tuple_new(headers);
        void* data;
        while (eina_iterator_next(it, &data)) {
            Eina_Hash_Tuple* t = static_cast<Eina_Hash_Tuple*>(data);
            const char* name = static_cast<const char*>(t->key);
            const char* value = static_cast<const char*>(t->data);
            request.addHTTPHeaderField(name, value);
        }
        eina_iterator_free(it);
    }

    if (body)
        request.setHTTPBody(FormData::create(body));

    WKRetainPtr<WKURLRequestRef> urlRequest(AdoptWK,toAPI(WebURLRequest::create(request).leakRef()));
    WKPageLoadURLRequest(toAPI(impl->pageProxy.get()), urlRequest.get());

    return true;
}

Eina_Bool ewk_view_plain_text_set(Evas_Object* ewkView, const char* plainText)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    WKRetainPtr<WKStringRef> plainTextRef(AdoptWK, WKStringCreateWithUTF8CString(plainText));
    WKPageLoadPlainTextString(toAPI(impl->pageProxy.get()), plainTextRef.get());

    return true;
}

Eina_Bool ewk_view_contents_set(Evas_Object* ewkView, const char* contents, size_t contentsSize, char* mimeType, char* encoding, char* baseUri)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(contents, false);

    if (contentsSize == 0)
        return false;

    String mimeTypeString;
    if (mimeType)
        mimeTypeString = String::fromUTF8(mimeType);
    else
        mimeTypeString = String::fromUTF8("text/html");

    String encodingString;
    if (encoding)
        encodingString = String::fromUTF8(encoding);
    else
        encodingString = String::fromUTF8("UTF-8");

    String baseUriString;
    if (baseUri)
        baseUriString = String::fromUTF8(baseUri);
    else
        baseUriString = String::fromUTF8("about:blank");

    WKRetainPtr<WKDataRef> contentsRef(AdoptWK, WKDataCreate(reinterpret_cast<const unsigned char*>(contents), contentsSize));
    impl->pageProxy->loadContentsbyMimeType(toImpl(contentsRef.get()), mimeTypeString, encodingString, baseUriString);

    return true;
}

Eina_Bool ewk_view_html_contents_set(Evas_Object* ewkView, const char* html, const char* baseUri)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    WKRetainPtr<WKStringRef> htmlString(AdoptWK, WKStringCreateWithUTF8CString(html));
    WKRetainPtr<WKURLRef> baseURL(AdoptWK, WKURLCreateWithUTF8CString(baseUri));

    WKPageLoadHTMLString(toAPI(impl->pageProxy.get()), htmlString.get(), baseURL.get());

    return true;
}

Eina_Bool ewk_view_page_visibility_state_set(Evas_Object* ewkView, Ewk_Page_Visibility_State pageVisibilityState, Eina_Bool initialState)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    TIZEN_LOGI("initialState (%d)", initialState);
#if ENABLE(TIZEN_PAGE_VISIBILITY_API)
    WKPageSetPageVisibility(toAPI(impl->pageProxy.get()), static_cast<WKPageVisibilityState>(pageVisibilityState), initialState);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_user_agent_set(Evas_Object* ewkView, const char* userAgent)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->userAgent = userAgent;

    WKRetainPtr<WKStringRef> userAgentString(AdoptWK, WKStringCreateWithUTF8CString(userAgent));
    WKPageSetCustomUserAgent(toAPI(impl->pageProxy.get()), userAgentString.get());

    return true;
}

const char* ewk_view_user_agent_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

#if PLATFORM(TIZEN)
    WKRetainPtr<WKStringRef> userAgentString(AdoptWK, WKPageCopyUserAgent(toAPI(impl->pageProxy.get())));

    int length = WKStringGetMaximumUTF8CStringSize(userAgentString.get());
    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(userAgentString.get(), buffer.get(), length);
    impl->userAgent = buffer.get();
#else
    if (!impl->userAgent) {
        WKRetainPtr<WKStringRef> userAgentString(AdoptWK, WKPageCopyUserAgent(toAPI(impl->pageProxy.get())));

        int length = WKStringGetMaximumUTF8CStringSize(userAgentString.get());
        OwnArrayPtr<char> buffer = adoptArrayPtr(new char[length]);
        WKStringGetUTF8CString(userAgentString.get(), buffer.get(), length);
        impl->userAgent = buffer.get();
    }
#endif

    return impl->userAgent;
}

#if PLATFORM(TIZEN)
Eina_Bool ewk_view_application_name_for_user_agent_set(Evas_Object* ewkView, const char* applicationName)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    WKRetainPtr<WKStringRef> applicationNameString(AdoptWK, WKStringCreateWithUTF8CString(applicationName));
    WKPageSetApplicationNameForUserAgent(toAPI(impl->pageProxy.get()), applicationNameString.get());

    return true;
}

const char* ewk_view_application_name_for_user_agent_get(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    WKRetainPtr<WKStringRef> applicationNameString(AdoptWK, WKPageCopyApplicationNameForUserAgent(toAPI(impl->pageProxy.get())));

    int length = WKStringGetMaximumUTF8CStringSize(applicationNameString.get());
    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(applicationNameString.get(), buffer.get(), length);
    impl->applicationName = buffer.get();

    return impl->applicationName;
}
#endif

Eina_Bool ewk_view_custom_header_add(const Evas_Object* ewkView, const char* name, const char* value)
{
#if ENABLE(TIZEN_CUSTOM_HEADERS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    WKRetainPtr<WKStringRef> customHeaderName(AdoptWK, WKStringCreateWithUTF8CString(name));
    WKRetainPtr<WKStringRef> customHeaderValue(AdoptWK, WKStringCreateWithUTF8CString(value));

    WKPageAddCustomHeader(toAPI(impl->pageProxy.get()), customHeaderName.get(), customHeaderValue.get());
    return true;
#else
    ERR("TIZEN_CUSTOM_HEADERS not enabled!");
    return false;
#endif
}

Eina_Bool ewk_view_custom_header_remove(const Evas_Object* ewkView, const char* name)
{
#if ENABLE(TIZEN_CUSTOM_HEADERS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    WKRetainPtr<WKStringRef> customHeaderName(AdoptWK, WKStringCreateWithUTF8CString(name));

    WKPageRemoveCustomHeader(toAPI(impl->pageProxy.get()), customHeaderName.get());
    return true;
#else
    ERR("TIZEN_CUSTOM_HEADERS not enabled!");
    return false;
#endif
}

Eina_Bool ewk_view_custom_header_clear(const Evas_Object* ewkView)
{
#if ENABLE(TIZEN_CUSTOM_HEADERS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    WKPageClearCustomHeaders(toAPI(impl->pageProxy.get()));
    return true;
#else
    ERR("TIZEN_CUSTOM_HEADERS not enabled!");
    return false;
#endif
}

#if ENABLE(TIZEN_WEBKIT2_VIEW_VISIBILITY)
Eina_Bool ewk_view_visibility_set(Evas_Object* ewkView, Eina_Bool enable)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_DRAG_SUPPORT)
    if (impl->pageClient->isDragMode())
        impl->pageClient->setDragMode(false);
#endif

#if ENABLE(TIZEN_FOCUS_UI)
    if (!enable)
        impl->page()->suspendFocusUI();
#endif

    impl->pageClient->setIsVisible(enable);
    return true;
}
#endif

Eina_Bool ewk_view_foreground_set(Evas_Object* ewkView, Eina_Bool enable)
{
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->pageClient->setIsForeground(enable);
    return true;
#else
    ERR("TIZEN_BACKGROUND_DISK_CACHE not enabled!");
    return false;
#endif
}

Evas_Object* ewk_view_screenshot_contents_get(const Evas_Object* ewkView, Eina_Rectangle viewArea, float scaleFactor, Evas* canvas)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);

    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    WKRect rect;
    rect.origin.x = viewArea.x;
    rect.origin.y = viewArea.y;
    rect.size.width = viewArea.w;
    rect.size.height = viewArea.h;

    WKRetainPtr<WKImageRef> snapshot(AdoptWK, WKPageCreateSnapshot(toAPI(impl->pageProxy.get()), rect, scaleFactor));
    if (!snapshot.get())
        return 0;

    RefPtr<cairo_surface_t> screenshotSurface = adoptRef(WKImageCreateCairoSurface(snapshot.get()));

    Evas_Object* screenshotImage = evas_object_image_add(canvas);
    int surfaceWidth = cairo_image_surface_get_width(screenshotSurface.get());
    int surfaceHeight = cairo_image_surface_get_height(screenshotSurface.get());
    evas_object_image_size_set(screenshotImage, surfaceWidth, surfaceHeight);
    evas_object_image_colorspace_set(screenshotImage, EVAS_COLORSPACE_ARGB8888);

    uint8_t* pixels = static_cast<uint8_t*>(evas_object_image_data_get(screenshotImage, true));

    RefPtr<cairo_surface_t> imageSurface = adoptRef(cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_RGB24, surfaceWidth, surfaceHeight, cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, surfaceWidth)));
    RefPtr<cairo_t> cairo = adoptRef(cairo_create(imageSurface.get()));

    cairo_set_source_surface(cairo.get(), screenshotSurface.get(), 0, 0);
    cairo_rectangle(cairo.get(), 0, 0, surfaceWidth, surfaceHeight);
    cairo_fill(cairo.get());

    evas_object_image_smooth_scale_set(screenshotImage, true);
    evas_object_size_hint_min_set(screenshotImage, surfaceWidth, surfaceHeight);
    evas_object_resize(screenshotImage, surfaceWidth, surfaceHeight);
    evas_object_image_fill_set(screenshotImage, 0, 0, surfaceWidth, surfaceHeight);
    evas_object_image_data_set(screenshotImage, pixels);

    return screenshotImage;
}

#if ENABLE(TIZEN_CACHE_IMAGE_GET)
Evas_Object* ewk_view_cache_image_get(const Evas_Object* ewkView, const char* imageUrl, Evas* canvas)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);

    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(imageUrl, false);

    WKRetainPtr<WKImageRef> cacheImageRef(AdoptWK, WKPageCreateCacheImage(toAPI(impl->pageProxy.get()), WKStringCreateWithUTF8CString(imageUrl)));
    if (!cacheImageRef.get())
        return 0;

    RefPtr<cairo_surface_t> cacheImageSurface = adoptRef(WKImageCreateCairoSurface(cacheImageRef.get()));

    Evas_Object* cacheImage = evas_object_image_add(canvas);
    int surfaceWidth = cairo_image_surface_get_width(cacheImageSurface.get());
    int surfaceHeight = cairo_image_surface_get_height(cacheImageSurface.get());
    evas_object_image_size_set(cacheImage, surfaceWidth, surfaceHeight);
    evas_object_image_colorspace_set(cacheImage, EVAS_COLORSPACE_ARGB8888);

    uint8_t* pixels = static_cast<uint8_t*>(evas_object_image_data_get(cacheImage, true));

    RefPtr<cairo_surface_t> imageSurface = adoptRef(cairo_image_surface_create_for_data(pixels, CAIRO_FORMAT_RGB24, surfaceWidth, surfaceHeight, cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, surfaceWidth)));
    RefPtr<cairo_t> cairo = adoptRef(cairo_create(imageSurface.get()));

    cairo_set_source_surface(cairo.get(), cacheImageSurface.get(), 0, 0);
    cairo_rectangle(cairo.get(), 0, 0, surfaceWidth, surfaceHeight);
    cairo_fill(cairo.get());

    evas_object_image_smooth_scale_set(cacheImage, true);
    evas_object_size_hint_min_set(cacheImage, surfaceWidth, surfaceHeight);
    evas_object_resize(cacheImage, surfaceWidth, surfaceHeight);
    evas_object_image_fill_set(cacheImage, 0, 0, surfaceWidth, surfaceHeight);
    evas_object_image_data_set(cacheImage, pixels);

    return cacheImage;
}
#endif

void ewk_view_scroll_by(Evas_Object* ewkView, int deltaX, int deltaY)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    WKPageScrollBy(toAPI(impl->pageProxy.get()), toAPI(IntSize(deltaX, deltaY)));
}

Eina_Bool ewk_view_scroll_pos_get(Evas_Object* ewkView, int* x, int* y)
{
    if (x)
        *x = 0;
    if (y)
        *y = 0;
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    IntPoint scrollPosition = impl->pageClient->scrollPosition();
    if (x)
        *x = scrollPosition.x();
    if (y)
        *y = scrollPosition.y();

    return true;
}

Eina_Bool ewk_view_scroll_set(Evas_Object* ewkView, int x, int y)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->pageProxy->scrollMainFrameTo(IntPoint(x, y));
    return true;
}

Eina_Bool ewk_view_scroll_size_get(const Evas_Object* ewkView, int* width, int* height)
{
    if (width)
        *width = 0;
    if (height)
        *height = 0;
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    IntSize contentsSize = impl->pageProxy->contentsSize();
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    contentsSize.scale(impl->pageClient->scaleFactor());
#endif

    if (width && contentsSize.width() > smartData->view.w)
        *width = contentsSize.width() - smartData->view.w;
    if (height && contentsSize.height() > smartData->view.h)
        *height = contentsSize.height() - smartData->view.h;
    return true;
}

#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
void ewkViewNotifyPopupReplyWaitingState(Evas_Object* ewkView, bool isWaiting)
{
    if (isWaiting)
        evas_object_smart_callback_call(ewkView, "popup,reply,wait,start", 0);
    else
        evas_object_smart_callback_call(ewkView, "popup,reply,wait,finish", 0);
}
#endif

bool ewkViewRunJavaScriptAlert(Evas_Object* ewkView, WKStringRef alertText)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    if (!impl->alertContext || !impl->alertContext->javascriptAlertCallback)
        return false;
    EINA_SAFETY_ON_FALSE_RETURN_VAL(impl->alertContext->ewkView == ewkView, false);

    impl->isWaitingForJavaScriptPopupReply = true;
    int length = WKStringGetMaximumUTF8CStringSize(alertText);
    OwnArrayPtr<char> alertTextBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(alertText, alertTextBuffer.get(), length);
    return impl->alertContext->javascriptAlertCallback(impl->alertContext->ewkView, alertTextBuffer.get(), impl->alertContext->userData) == EINA_TRUE;
}

bool ewkViewRunJavaScriptConfirm(Evas_Object* ewkView, WKStringRef message)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    if (!impl->confirmContext || !impl->confirmContext->javascriptConfirmCallback)
        return false;
    EINA_SAFETY_ON_FALSE_RETURN_VAL(impl->confirmContext->ewkView == ewkView, false);

    impl->isWaitingForJavaScriptPopupReply = true;
    int length = WKStringGetMaximumUTF8CStringSize(message);
    OwnArrayPtr<char> messageBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(message, messageBuffer.get(), length);
    return impl->confirmContext->javascriptConfirmCallback(impl->confirmContext->ewkView, messageBuffer.get(), impl->confirmContext->userData) == EINA_TRUE;
}

bool ewkViewRunJavaScriptPrompt(Evas_Object* ewkView, WKStringRef message, WKStringRef defaultValue)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    if (!impl->promptContext || !impl->promptContext->javascriptPromptCallback)
        return false;
    EINA_SAFETY_ON_FALSE_RETURN_VAL(impl->promptContext->ewkView == ewkView, false);

    impl->isWaitingForJavaScriptPopupReply = true;
    int length = WKStringGetMaximumUTF8CStringSize(message);
    OwnArrayPtr<char> messageBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(message, messageBuffer.get(), length);
    length = WKStringGetMaximumUTF8CStringSize(defaultValue);
    OwnArrayPtr<char> defaultValueBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(defaultValue, defaultValueBuffer.get(), length);
    return impl->promptContext->javascriptPromptCallback(impl->promptContext->ewkView, messageBuffer.get(), defaultValueBuffer.get(), impl->promptContext->userData) == EINA_TRUE;
}

#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
bool ewk_view_run_before_unload_confirm_panel(Evas_Object* ewkView, WKStringRef message)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    if (!impl->beforeUnloadConfirmPanelContext || !impl->beforeUnloadConfirmPanelContext->beforeUnloadConfirmPanelCallback)
        return false;
    EINA_SAFETY_ON_FALSE_RETURN_VAL(impl->beforeUnloadConfirmPanelContext->ewkView == ewkView, false);

    impl->isWaitingForJavaScriptPopupReply = true;
    int length = WKStringGetMaximumUTF8CStringSize(message);
    OwnArrayPtr<char> messageBuffer = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(message, messageBuffer.get(), length);
    bool result = impl->beforeUnloadConfirmPanelContext->beforeUnloadConfirmPanelCallback(impl->beforeUnloadConfirmPanelContext->ewkView, messageBuffer.get(), impl->beforeUnloadConfirmPanelContext->userData) == EINA_TRUE;
    return result;
}
#endif

bool ewkViewRunOpenPanel(Evas_Object* ewkView, WKOpenPanelParametersRef parameters, WKOpenPanelResultListenerRef listener)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    if (!impl->openpanelContext || !impl->openpanelContext->openPanelCallback)
        return false;

    EINA_SAFETY_ON_FALSE_RETURN_VAL(impl->openpanelContext->ewkView == ewkView, false);

    impl->openPanelListener = listener;

    Eina_Bool allowMultipleFiles = WKOpenPanelParametersGetAllowsMultipleFiles(parameters) ? EINA_TRUE : EINA_FALSE;
    Eina_List* acceptedMimeTypes = 0;
    WKRetainPtr<WKArrayRef> array(AdoptWK, WKOpenPanelParametersCopyAcceptedMIMETypes(parameters));
    size_t len = WKArrayGetSize(array.get());
    for (size_t i = 0; i < len; i++) {
        WKTypeRef item = WKArrayGetItemAtIndex(array.get(), i);
        if (WKGetTypeID(item) != WKStringGetTypeID())
            continue;
        WKStringRef mime = static_cast<WKStringRef>(item);
        int length = WKStringGetMaximumUTF8CStringSize(mime);
        char* buffer = new char[length];
        WKStringGetUTF8CString(mime, buffer, length);
        acceptedMimeTypes = eina_list_append(acceptedMimeTypes, static_cast<const void*>(const_cast<const char*>(buffer)));
    }
    const char* capture = 0;
#if ENABLE(MEDIA_CAPTURE)
    WKRetainPtr<WKStringRef> captureRef(AdoptWK, WKOpenPanelParametersCopyCapture(parameters));
    capture = eina_stringshare_add(toImpl(captureRef.get())->string().utf8().data());
#endif
    bool result = impl->openpanelContext->openPanelCallback(impl->openpanelContext->ewkView, allowMultipleFiles, acceptedMimeTypes, capture, 0);
    if (!result)
        impl->openPanelListener = 0;

    if (!acceptedMimeTypes)
        return result;

    Eina_List* list;
    void* data = 0;
    EINA_LIST_FOREACH(acceptedMimeTypes, list, data)
        delete[] (char*)data;
    eina_list_free(acceptedMimeTypes);
    return result;
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void ewk_view_password_confirm_popup_callback_set(Evas_Object* ewkView, Ewk_View_Password_Confirm_Popup_Callback callback, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->passwordContext)
        impl->passwordContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->passwordContext->passwordConfirmPopupCallback = callback;
    impl->passwordContext->ewkView = ewkView;
    impl->passwordContext->userData = userData;
}

void ewk_view_password_confirm_popup_reply(Evas_Object* ewkView, Ewk_Password_Popup_Option result)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    switch (result) {
    case EWK_PASSWORD_POPUP_SAVE:
        ewk_view_context_get(ewkView)->allowSavePassword();
        break;
    case EWK_PASSWORD_POPUP_NOT_NOW:
        ewk_view_context_get(ewkView)->cancelSavePassword();
        break;
    case EWK_PASSWORD_POPUP_NEVER:
        ewk_view_context_get(ewkView)->neverSavePassword();
        break;
    default:
        break;
    }
}
#endif

void ewk_view_javascript_alert_callback_set(Evas_Object* ewkView, Ewk_View_JavaScript_Alert_Callback callback, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->alertContext)
        impl->alertContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->alertContext->javascriptAlertCallback = callback;
    impl->alertContext->ewkView = ewkView;
    impl->alertContext->userData = userData;
}

void ewk_view_javascript_alert_reply(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    WKPageReplyJavaScriptAlert(toAPI(impl->page()));
    impl->isWaitingForJavaScriptPopupReply = false;
}

void ewk_view_javascript_confirm_callback_set(Evas_Object* ewkView, Ewk_View_JavaScript_Confirm_Callback callback, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->confirmContext)
        impl->confirmContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->confirmContext->javascriptConfirmCallback = callback;
    impl->confirmContext->ewkView = ewkView;
    impl->confirmContext->userData = userData;
}

void ewk_view_javascript_confirm_reply(Evas_Object* ewkView, Eina_Bool result)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    WKPageReplyJavaScriptConfirm(toAPI(impl->page()), result == EINA_TRUE);
    impl->isWaitingForJavaScriptPopupReply = false;
}

void ewk_view_javascript_prompt_callback_set(Evas_Object* ewkView, Ewk_View_JavaScript_Prompt_Callback callback, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->promptContext)
        impl->promptContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->promptContext->javascriptPromptCallback = callback;
    impl->promptContext->ewkView = ewkView;
    impl->promptContext->userData = userData;
}

void ewk_view_javascript_prompt_reply(Evas_Object* ewkView, const char* result)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    WKRetainPtr<WKStringRef> resultString(AdoptWK, WKStringCreateWithUTF8CString(result));
    WKPageReplyJavaScriptPrompt(toAPI(impl->page()), result ? resultString.get() : 0);
    impl->isWaitingForJavaScriptPopupReply = false;
}

#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
void ewk_view_before_unload_confirm_panel_callback_set(Evas_Object* ewkView, Ewk_View_Before_Unload_Confirm_Panel_Callback callback, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->beforeUnloadConfirmPanelContext)
        impl->beforeUnloadConfirmPanelContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->beforeUnloadConfirmPanelContext->beforeUnloadConfirmPanelCallback = callback;
    impl->beforeUnloadConfirmPanelContext->ewkView = ewkView;
    impl->beforeUnloadConfirmPanelContext->userData = userData;
}

void ewk_view_before_unload_confirm_panel_reply(Evas_Object* ewkView, Eina_Bool result)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    WKPageReplyBeforeUnloadConfirmPanel(toAPI(impl->page()), result == EINA_TRUE);
    impl->isWaitingForJavaScriptPopupReply = false;
}
#endif

void ewk_view_open_panel_callback_set(Evas_Object* ewkView, Ewk_View_Open_Panel_Callback callback, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->openpanelContext)
        impl->openpanelContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);

    impl->openpanelContext->openPanelCallback = callback;
    impl->openpanelContext->ewkView = ewkView;
    impl->openpanelContext->userData = userData;
}

void ewk_view_open_panel_reply(Evas_Object* ewkView, Eina_List* fileUrls, Eina_Bool result)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->openPanelListener)
        return;

    unsigned int size = eina_list_count(fileUrls);
    if ((result == EINA_FALSE) || (size == 0)) {
        WKOpenPanelResultListenerCancel(impl->openPanelListener);
        impl->openPanelListener = 0;
        return;
    }

    WKTypeRef* items = new WKTypeRef[size];
    Eina_List* list;
    void* data;
    unsigned int i = 0;
    KURL base(KURL(), "file://");
    EINA_LIST_FOREACH(fileUrls, list, data) {
        KURL url(base, String::fromUTF8(static_cast<char*>(data)));
        items[i++] = WKURLCreateWithUTF8CString(url.string().utf8().data());
    }
    WKRetainPtr<WKArrayRef> filesArray(AdoptWK, WKArrayCreate(items, size));
    WKOpenPanelResultListenerChooseFiles(impl->openPanelListener, filesArray.get());
    impl->openPanelListener = 0;
    delete [] items;
}

#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
static void didGetWebAppCapable(WKBooleanRef capable, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(capable);
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_View_Callback_Context* webAppContext = static_cast<Ewk_View_Callback_Context*>(context);

    ASSERT(webAppContext->webAppCapableCallback);

    TIZEN_LOGI("webAppCapableCallback exist. capable(%b)", capable);
    if (capable)
        webAppContext->webAppCapableCallback(toImpl(capable)->value(), webAppContext->userData);
    else
        webAppContext->webAppCapableCallback(0, webAppContext->userData);

    delete webAppContext;
}

static void didGetWebAppIconURL(WKStringRef iconURL, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(iconURL);
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_View_Callback_Context* webAppContext = static_cast<Ewk_View_Callback_Context*>(context);

    EWK_VIEW_SD_GET_OR_RETURN(webAppContext->ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    ASSERT(webAppContext->webAppIconURLCallback);

    if (iconURL) {
        impl->webAppIconURL = toImpl(iconURL)->string().utf8().data();
        webAppContext->webAppIconURLCallback(impl->webAppIconURL, webAppContext->userData);
    } else
        webAppContext->webAppIconURLCallback(0, webAppContext->userData);

    delete webAppContext;
}

static void didGetWebAppIconURLs(WKDictionaryRef iconURLs, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(iconURLs);
    EINA_SAFETY_ON_NULL_RETURN(context);

    Ewk_View_Callback_Context* webAppContext = static_cast<Ewk_View_Callback_Context*>(context);

    EWK_VIEW_SD_GET_OR_RETURN(webAppContext->ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    ASSERT(webAppContext->webAppIconURLsCallback);

    if (impl->webAppIconURLs) {
        void* data = 0;
        EINA_LIST_FREE(impl->webAppIconURLs, data)
            ewkWebAppIconDataDelete(static_cast<Ewk_Web_App_Icon_Data*>(data));
    }

    WKRetainPtr<WKArrayRef> wkKeys(AdoptWK, WKDictionaryCopyKeys(iconURLs));
    size_t iconURLCount = WKArrayGetSize(wkKeys.get());
    for (size_t i = 0; i < iconURLCount; i++) {
        WKStringRef urlRef = static_cast<WKStringRef>(WKArrayGetItemAtIndex(wkKeys.get(), i));
        WKStringRef sizeRef = static_cast<WKStringRef>(WKDictionaryGetItemForKey(iconURLs, urlRef));
        impl->webAppIconURLs = eina_list_append(impl->webAppIconURLs, ewkWebAppIconDataCreate(sizeRef, urlRef));
    }
    TIZEN_LOGI("webAppIconURLsCallback exist. found %d icon urls", iconURLCount);

    webAppContext->webAppIconURLsCallback(impl->webAppIconURLs, webAppContext->userData);
    delete webAppContext;
}
#endif

Eina_Bool ewk_view_web_application_capable_get(Evas_Object* ewkView, Ewk_Web_App_Capable_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    TIZEN_LOGI("callback(%d), userData(%d)", callback, userData);
    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->webAppCapableCallback = callback;
    context->ewkView = ewkView;
    context->userData = userData;

    WKPageGetWebAppCapable(toAPI(impl->pageProxy.get()), context, didGetWebAppCapable);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_web_application_icon_url_get(Evas_Object* ewkView, Ewk_Web_App_Icon_URL_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->webAppIconURLCallback = callback;
    context->ewkView = ewkView;
    context->userData = userData;

    WKPageGetWebAppIconURL(toAPI(impl->page()), context, didGetWebAppIconURL);

    return true;
#else
    return 0;
#endif
}

Eina_Bool ewk_view_web_application_icon_urls_get(Evas_Object* ewkView, Ewk_Web_App_Icon_URLs_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    TIZEN_LOGI("callback(%d), userData(%d)", callback, userData);
    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->webAppIconURLsCallback = callback;
    context->ewkView = ewkView;
    context->userData = userData;

    WKPageGetWebAppIconURLs(toAPI(impl->page()), context, didGetWebAppIconURLs);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
    return 0;
#endif
}

Eina_Bool ewk_view_command_execute(Evas_Object* ewkView, const char* command, const char* value)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(command, false);

    WKRetainPtr<WKStringRef> commandString(AdoptWK, WKStringCreateWithUTF8CString(command));
    WKRetainPtr<WKStringRef> valueString(AdoptWK, WKStringCreateWithUTF8CString(value));
    WKPageExecuteCommandWithArgument(toAPI(impl->pageProxy.get()), commandString.get(), valueString.get());

    return true;
}

Eina_Bool ewk_view_contents_size_get(const Evas_Object* ewkView, Evas_Coord* width, Evas_Coord* height)
{
    if (width)
        *width = 0;
    if (height)
        *height = 0;

    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    IntSize contentsSize = impl->pageProxy->contentsSize();

    if (width)
        *width = contentsSize.width();
    if (height)
        *height = contentsSize.height();

    return true;
}

Eina_Bool ewk_view_contents_pdf_get(Evas_Object* ewkView, int width, int height, const char* fileName)
{
#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(fileName, false);

    IntSize contentsSize = impl->pageProxy->contentsSize();
    WKPageGetSnapshotPdfFile(toAPI(impl->pageProxy.get()), toAPI(IntSize(width, height)), toAPI(IntSize(contentsSize.width(), contentsSize.height())), WKStringCreateWithUTF8CString(fileName));

    return true;
#else
    return false;
#endif
}

static void runJavaScriptCallback(WKSerializedScriptValueRef scriptValue, WKErrorRef error, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);
    Ewk_View_Callback_Context* callbackContext = static_cast<Ewk_View_Callback_Context*>(context);

    if (!callbackContext->scriptExecuteCallback) {
        delete callbackContext;
        return;
    }

    JSGlobalContextRef jsGlobalContext = ewkViewGetJavascriptGlobalContext(callbackContext->ewkView);

    // This is early return in case smartData of ewkView is null.
    if (!jsGlobalContext) {
        delete callbackContext;
        return;
    }

    if (scriptValue) {
        JSValueRef value = WKSerializedScriptValueDeserialize(scriptValue, jsGlobalContext, 0);
        JSRetainPtr<JSStringRef> jsStringValue(Adopt, JSValueToStringCopy(jsGlobalContext, value, 0));
        size_t length = JSStringGetMaximumUTF8CStringSize(jsStringValue.get());
        OwnArrayPtr<char> buffer = adoptArrayPtr(new char[length]);
        JSStringGetUTF8CString(jsStringValue.get(), buffer.get(), length);
        callbackContext->scriptExecuteCallback(callbackContext->ewkView, buffer.get(), callbackContext->userData);
    } else
        callbackContext->scriptExecuteCallback(callbackContext->ewkView, 0, callbackContext->userData);

    delete callbackContext;
}

Eina_Bool ewk_view_script_execute(Evas_Object* ewkView, const char* script, Ewk_View_Script_Execute_Callback callback, void* user_data)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(script, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(impl->pageClient, false);

    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->scriptExecuteCallback = callback;
    context->ewkView = ewkView;
    context->userData = user_data;
    WKRetainPtr<WKStringRef> scriptString(AdoptWK, WKStringCreateWithUTF8CString(script));
    WKPageRunJavaScriptInMainFrame(toAPI(impl->pageProxy.get()), scriptString.get(), context, runJavaScriptCallback);

    return true;
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
Eina_Bool ewk_view_script_execute_in_focused_frame(Evas_Object* ewkView, const char* script, Ewk_View_Script_Execute_Callback callback, void* user_data)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(script, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(impl->pageClient, false);

    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->scriptExecuteCallback = callback;
    context->ewkView = ewkView;
    context->userData = user_data;
    WKRetainPtr<WKStringRef> scriptString(AdoptWK, WKStringCreateWithUTF8CString(script));
    WKPageRunJavaScriptInFocusedFrame(toAPI(impl->pageProxy.get()), scriptString.get(), context, runJavaScriptCallback);

    return true;
}
#endif

#if ENABLE(TIZEN_WEB_STORAGE) && ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
static void didGetWebStorageQuota(WKUInt32Ref quota, WKErrorRef error, void* context)
{
    Ewk_View_Callback_Context* storageContext = static_cast<Ewk_View_Callback_Context*>(context);

    if (quota)
        storageContext->webStorageQuotaCallback(toImpl(quota)->value(), storageContext->userData);
    else
        storageContext->webStorageQuotaCallback(0, storageContext->userData);

    delete storageContext;
}
#endif

Eina_Bool ewk_view_web_storage_quota_get(const Evas_Object* ewkView, Ewk_Web_Storage_Quota_Get_Callback resultCallback, void* userData)
{
#if ENABLE(TIZEN_WEB_STORAGE) && ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkView, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(resultCallback, false);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    TIZEN_LOGI("resultCallback (%p)", resultCallback);

    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->webStorageQuotaCallback = resultCallback;
    context->userData = userData;

    WKPageRef pageRef = toAPI(impl->page());
    WKPageGetWebStorageQuota(pageRef, context, didGetWebStorageQuota);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_web_storage_quota_set(Evas_Object* ewkView, uint32_t quota)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkView, false);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    TIZEN_LOGI("quota (%d)", quota);
    WKPageRef pageRef = toAPI(impl->page());
    WKPageSetWebStorageQuota(pageRef, quota);

    return true;
#else
    return false;
#endif
}

static void getContentsAsStringCallback(WKStringRef plain_text, WKErrorRef error, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);
    Ewk_View_Callback_Context* callbackContext = static_cast<Ewk_View_Callback_Context*>(context);

    ASSERT(callbackContext->plainTextGetCallback);

    if (plain_text) {
        size_t length = WKStringGetMaximumUTF8CStringSize(plain_text);
        OwnArrayPtr<char> buffer = adoptArrayPtr(new char[length]);
        WKStringGetUTF8CString(plain_text, buffer.get(), length);

        callbackContext->plainTextGetCallback(callbackContext->ewkView, buffer.get(), callbackContext->userData);
    } else
        callbackContext->plainTextGetCallback(callbackContext->ewkView, 0, callbackContext->userData);

    delete callbackContext;
}

Eina_Bool ewk_view_plain_text_get(Evas_Object* ewkView, Ewk_View_Plain_Text_Get_Callback callback, void* user_data)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(impl->pageClient, false);

    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->plainTextGetCallback = callback;
    context->ewkView = ewkView;
    context->userData = user_data;
    WKPageGetContentsAsString(toAPI(impl->pageProxy.get()), context, getContentsAsStringCallback);

    return true;
}

#if ENABLE(TIZEN_SUPPORT_MHTML)
/**
 * @internal
 * Callback function used for ewk_view_mhtml_data_get().
 */
static void getContentsAsMHTMLCallback(WKDataRef wkData, WKErrorRef, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);
    Ewk_View_Callback_Context* callbackContext = static_cast<Ewk_View_Callback_Context*>(context);

    ASSERT(callbackContext->mhtmlDataGetCallback);

    if (wkData)
        callbackContext->mhtmlDataGetCallback(callbackContext->ewkView, reinterpret_cast<const char*>(WKDataGetBytes(wkData)), callbackContext->userData);
    else
        callbackContext->mhtmlDataGetCallback(callbackContext->ewkView, 0, callbackContext->userData);

    delete callbackContext;
}

Eina_Bool ewk_view_mhtml_data_get(Evas_Object* ewkView, Ewk_View_MHTML_Data_Get_Callback callback, void* user_data)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    Ewk_View_Callback_Context* context = new Ewk_View_Callback_Context;
    context->mhtmlDataGetCallback = callback;
    context->ewkView = ewkView;
    context->userData = user_data;

    WKPageGetContentsAsMHTMLData(toAPI(impl->page()), false, context, getContentsAsMHTMLCallback);

    return true;
}
#endif // ENABLE(TIZEN_SUPPORT_MHTML)

Ewk_Hit_Test* ewk_view_hit_test_new(Evas_Object* ewkView, int x, int y, int hitTestMode)
{
#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    IntPoint pointForHitTest = impl->transformFromScene().mapPoint(IntPoint(x, y));
    WebHitTestResult::Data hitTestResultData = impl->pageProxy->hitTestResultAtPoint(pointForHitTest, hitTestMode);
    Ewk_Hit_Test* hitTest = ewkHitTestCreate(hitTestResultData);

    return hitTest;
#else
    return 0;
#endif
}

Ewk_History* ewk_view_history_get(Evas_Object* ewkView)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkView, 0);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    WebPageProxy* page = impl->page();
    EINA_SAFETY_ON_NULL_RETURN_VAL(page, 0);

    return ewkHistoryCreate(WKPageGetBackForwardList(toAPI(page)));
}

Eina_Bool ewk_view_notification_closed(Evas_Object* ewkView, Eina_List* ewkNotifications)
{
#if ENABLE(TIZEN_NOTIFICATIONS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(impl->context, false);

    TIZEN_LOGI("ewkNotifications (%p)", ewkNotifications);
    if (!eina_list_count(ewkNotifications))
        return false;

    Eina_List* listIterator = 0;
    void* data = 0;
    Vector<WKTypeRef> ids;
    EINA_LIST_FOREACH(ewkNotifications, listIterator, data) {
        Ewk_Notification* notification = static_cast<Ewk_Notification*>(data);
        WKUInt64Ref idRef = WKUInt64Create(ewk_notification_id_get(notification));
        ids.append(idRef);
        impl->notifications = eina_list_remove(impl->notifications, notification);
        deleteEwkNotification(notification);
    }

    WKRetainPtr<WKArrayRef> notificationIDsArray(AdoptWK, WKArrayCreate(ids.data(), ids.size()));
    WKNotificationManagerRef notificationManager = WKContextGetNotificationManager(impl->context->wkContext());
    WKNotificationManagerProviderDidCloseNotifications(notificationManager, notificationIDsArray.get());

    if(impl->notificationManagerEfl)
        impl->notificationManagerEfl->deleteAllNotifications();

    return true;
#else
    return false;
#endif
}

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
static Eina_List* createPopupMenuList(const Vector<WebPopupItem>& items)
{
    Eina_List* popupItems = 0;
    size_t size = items.size();
    for (size_t i = 0; i < size; ++i)
        popupItems = eina_list_append(popupItems, Ewk_Popup_Menu_Item::create(items[i]).leakPtr());
    TIZEN_LOGI("size : %d", size);
    return popupItems;
}

static void releasePopupMenuList(Eina_List* popupMenuItems)
{
    if (!popupMenuItems)
        return;

    void* item;
    EINA_LIST_FREE(popupMenuItems, item)
        delete static_cast<Ewk_Popup_Menu_Item*>(item);
}
#endif

namespace {
Eina_Bool postponedFocusIn(void* data)
{
    Evas_Object* ewkView = static_cast<Evas_Object*>(data);
    elm_object_focus_set(ewkView, true);
    return 0;
}
}

Eina_Bool ewk_view_popup_menu_close(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->api, false);
#if PLATFORM(TIZEN)
    TIZEN_LOGI("proxy : %p", impl->popupMenuProxy);
#endif

    if (!impl->popupMenuProxy)
        return false;

    impl->popupMenuProxy = 0;

    // FIXME: Workaround for focus_out on ewkView
    Ecore_Timer* timer = ecore_timer_add(0.0, postponedFocusIn, ewkView);

    if (smartData->api->popup_menu_hide)
        smartData->api->popup_menu_hide(smartData);
#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
    ewk_view_touch_events_enabled_set(ewkView, true);

    releasePopupMenuList(impl->popupMenuItems);
    impl->popupMenuItems = 0;
#else
    void* item;
    EINA_LIST_FREE(impl->popupMenuItems, item)
        delete static_cast<Ewk_Popup_Menu_Item*>(item);
#endif

    return true;
}

Eina_Bool ewk_view_popup_menu_select(Evas_Object* ewkView, unsigned int selectedIndex)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(impl->popupMenuProxy, false);
#if PLATFORM(TIZEN)
    TIZEN_LOGI("proxy : %p / index : %d", impl->popupMenuProxy, selectedIndex);

    if (!impl->popupMenuItems)
        return false;
#endif

    //When user select empty space then no index is selected, so selectedIndex value is -1
    //In that case we should call valueChanged() with -1 index.That in turn call popupDidHide()
    //in didChangeSelectedIndex() for reseting the value of m_popupIsVisible in RenderMenuList.
    if (selectedIndex != static_cast<unsigned int>(-1) && selectedIndex >= eina_list_count(impl->popupMenuItems))
        return false;

    impl->popupMenuProxy->valueChanged(selectedIndex);

    return true;
}

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
void ewk_view_popup_menu_update(Evas_Object* ewkView, TextDirection textDirection, const Vector<WebPopupItem>& items, int32_t selectedIndex)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    TIZEN_LOGI("proxy : %p", impl->popupMenuProxy);

    if (!impl->popupMenuProxy)
        return;

    if (!smartData->api->popup_menu_update)
        return;

    releasePopupMenuList(impl->popupMenuItems);
    impl->popupMenuItems = createPopupMenuList(items);

    // TODO: Instead of passing a dummy rect, updated rect should be coming from WebProcess
    smartData->api->popup_menu_update(smartData, IntRect(), static_cast<Ewk_Text_Direction>(textDirection), impl->popupMenuItems, selectedIndex);
}
#endif

#if ENABLE(TIZEN_MULTIPLE_SELECT)
Eina_Bool ewk_view_popup_menu_multiple_select(Evas_Object* ewkView, Eina_Inarray* changeList)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(impl->popupMenuProxy, false);

    Vector<int> selectedIndex;

    if (!impl->popupMenuItems)
        return false;

    Eina_Iterator* itr;
    void* data;
    itr = eina_inarray_iterator_new(changeList);
    EINA_ITERATOR_FOREACH(itr, data)
    {
        int* pData = static_cast<int*>(data);
        selectedIndex.append(*pData);
    }
    eina_iterator_free(itr);

    impl->popupMenuProxy->multipleValueChanged(selectedIndex);
    return true;
}
#endif

void ewk_view_orientation_send(Evas_Object* ewkView, int orientation)
{
#if ENABLE(TIZEN_ORIENTATION_EVENTS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    EINA_SAFETY_ON_NULL_RETURN(impl->context);
    TIZEN_LOGI("orientation: %d", orientation);

    if (orientation != 0 && orientation != 90 && orientation != -90 && orientation != 180) {
        TIZEN_LOGE("orientation: %d", orientation);
        return;
    }

    if (impl->orientation == orientation)
        return;

    impl->orientation = orientation;

    impl->pageProxy->sendOrientationChangeEvent(orientation);
#endif
#if ENABLE(TIZEN_FULLSCREEN_API)
    if (impl->page()->fullScreenManager()->isFullScreen())
        impl->page()->fullScreenManager()->willRotateScreen();
#endif
}

void ewkViewFrameRendered(Evas_Object* ewkView)
{
    evas_object_smart_callback_call(ewkView, "frame,rendered", 0);
}

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
Eina_Bool ewk_view_text_selection_range_get(Evas_Object* ewkView, Eina_Rectangle* leftRect, Eina_Rectangle* rightRect)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    IntRect leftSelectionRect;
    IntRect rightSelectionRect;
    if (!impl->pageProxy->getSelectionHandlers(leftSelectionRect, rightSelectionRect)) {
        leftRect->x = 0;
        leftRect->y = 0;
        leftRect->w = 0;
        leftRect->h = 0;

        rightRect->x = 0;
        rightRect->y = 0;
        rightRect->w = 0;
        rightRect->h = 0;
        return false;
    }

    AffineTransform contentToScreen = impl->transformToScene();
    leftSelectionRect = contentToScreen.mapRect(leftSelectionRect);
    rightSelectionRect = contentToScreen.mapRect(rightSelectionRect);

    leftRect->x = leftSelectionRect.x();
    leftRect->y = leftSelectionRect.y();
    leftRect->w = leftSelectionRect.width();
    leftRect->h = leftSelectionRect.height();

    rightRect->x = rightSelectionRect.x();
    rightRect->y = rightSelectionRect.y();
    rightRect->w = rightSelectionRect.width();
    rightRect->h = rightSelectionRect.height();

    return true;;
}

const char* ewk_view_text_selection_text_get(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    const CString selectedString = impl->pageProxy->getSelectionText().utf8();
    impl->selectedText = selectedString.data();

    return impl->selectedText;
}

Eina_Bool ewk_view_text_selection_clear(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    bool isSelectionClear = false;
    EditorState editorState = impl->page()->editorState();

    if (impl->textSelection()->isTextSelectionMode() && (impl->pageClient->isContextMenuVisible() || editorState.selectionIsRange))
        isSelectionClear = true;

    impl->textSelection()->setIsTextSelectionMode(TextSelection::ModeNone);
    return isSelectionClear;
}
#endif // #if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
void ewkViewInputPickerRequest(Evas_Object* ewkView, Ewk_Input_Type inputType, const String& inputValue)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api->input_picker_show);

    smartData->api->input_picker_show(smartData, inputType, inputValue.utf8().data());
}
#endif

#if ENABLE(TIZEN_VIEWPORT_META_TAG)
void ewkViewGetWindowFrame(Evas_Object* ewkView, int *x, int *y, int *w, int *h)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    IntPoint drawingPosition = (IntPoint)(impl->pageClient->visibleContentRect().location() - impl->pageClient->scrollPosition());
    if (x)
        *x = drawingPosition.x();
    if (y)
        *y = drawingPosition.y();
    if (w)
        *w = impl->pageClient->visibleContentRect().width();
    if (h)
        *h = impl->pageClient->visibleContentRect().height();
}
#endif

void ewk_view_focused_input_element_value_set(Evas_Object* ewkView, const char* value)
{
#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->pageProxy->setFocusedInputElementValue(String::fromUTF8(value));
#endif // ENABLE(TIZEN_INPUT_TAG_EXTENSION)
}

const char* ewk_view_focused_input_element_value_get(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, 0);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, 0);

    impl->inputValue = impl->pageProxy->getFocusedInputElementValue().utf8().data();
    return impl->inputValue;
#else
    return 0;
#endif // ENABLE(TIZEN_INPUT_TAG_EXTENSION)
}

Eina_Bool ewk_view_horizontal_panning_hold_get(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    return impl->holdHorizontalPanning;
#else
    return false;
#endif
}

void ewk_view_horizontal_panning_hold_set(Evas_Object* ewkView, Eina_Bool hold)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    impl->holdHorizontalPanning = hold;
#endif
}

Eina_Bool ewk_view_vertical_panning_hold_get(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    return impl->holdVerticalPanning;
#else
    return false;
#endif
}

void ewk_view_vertical_panning_hold_set(Evas_Object* ewkView, Eina_Bool hold)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    impl->holdVerticalPanning = hold;
#endif
}

void ewk_view_touch_event_handler_result_set(Evas_Object* ewkView, WebKit::WebEvent::Type type, bool wasHandled)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    static unsigned touchMoveLogCount = 0;
    if (type == WebEvent::TouchStart)
        touchMoveLogCount = 0;
    else if (type == WebEvent::TouchMove)
        touchMoveLogCount++;
    if (type != WebEvent::TouchMove || (type == WebEvent::TouchMove && touchMoveLogCount < 10))
        TIZEN_LOGI("[Touch] type[%d] wasHandled[%d]", type, wasHandled);
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    if (impl->focusRing() && wasHandled)
        impl->focusRing()->hide();
#endif // #if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)

    if (impl->mouseEventsEnabled())
        return;

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (impl->textSelection()->isTextSelectionMode()) {
        if (type == WebEvent::TouchStart)
            impl->textSelection()->updateVisible(false);
        else if (type == WebEvent::TouchMove)
            impl->textSelection()->updateVisible(false);
        else if (type == WebEvent::TouchEnd) {
            if (impl->wasHandledTouchStart || wasHandled || impl->exceedTouchTapThreshold)
                impl->textSelection()->requestToShow();
        }
    }
#endif

    // We have to check TouchStart, TouchMove and TouchEnd with handled.
    // The Pan and Flick will be enabled if Touch Start was not handled,
    // and Touch Move was not handled or did not occur.
    // Tap will be enabled if Touch Start and End was not handled
    // and Touch Move did not occur.
    // The gestures are disabled as a default.
    // o: handled, x: not handled
    // ------------------------------------------------------------
    // Touch Start | Touch Move | Touch End ||   Tap   | Pan, Flick
    // ------------------------------------------------------------
    //      o      |   o or x   |   o or x  || disable |  disable
    //      x      |      o     |   o or x  || disable |  disable
    //      x      |      x     |   o or x  || disable |  enable
    //      x      |not occured |     x     || enable  |  enable
    // ------------------------------------------------------------
    if (type == WebEvent::TouchStart) {
        impl->gestureClient->setGestureEnabled(!wasHandled);

        // Notify the result of touchstart to applications
        // in order to make applications to choose whether handling at the beginnig of touch.
        evas_object_smart_callback_call(ewkView, "touchstart,handled", static_cast<void*>(&wasHandled));

        impl->wasHandledTouchStart = wasHandled;
    } else if (type == WebEvent::TouchMove) {
        impl->gestureClient->setMovingEnabled(!wasHandled);

        // We have to set wasHandled to true if touchstart was handled even though current touchmove was not handled.
        if (impl->wasHandledTouchStart)
            wasHandled = true;

        // Notify the result of touchmove to applications when handled value is set for the first time or changed
        // in order to make applications to choose whether scrolling its scrollable objects or not.
        if (!impl->wasHandledFirstTouchMove) {
            evas_object_smart_callback_call(ewkView, "touchmove,handled", static_cast<void*>(&wasHandled));
            impl->wasHandledFirstTouchMove = true;
        }
        else if (impl->wasHandledTouchMove != wasHandled)
            evas_object_smart_callback_call(ewkView, "touchmove,handled", static_cast<void*>(&wasHandled));

        impl->wasHandledTouchMove = wasHandled;
    } else if (type == WebEvent::TouchEnd) {
        if (!impl->exceedTouchTapThreshold) {
            impl->gestureClient->setTapEnabled(!wasHandled);
#if ENABLE(TIZEN_ISF_PORT)
            if (wasHandled && impl->pageProxy->isViewVisible() && impl->inputMethodContext()) {
                IntPoint pointForHitTest = impl->transformFromScene().mapPoint(IntPoint(impl->touchDownPoint.x, impl->touchDownPoint.y));
                WebHitTestResult::Data hitTestResultData = impl->pageProxy->hitTestResultAtPoint(pointForHitTest);
                if (!equalIgnoringCase(hitTestResultData.nodeData.tagName, "IMG")) {
                    if (hitTestResultData.isContentEditable)
                        impl->inputMethodContext()->updateTextInputStateByUserAction(true);
                    else
                        impl->inputMethodContext()->requestUpdateTextInputStateByUserAction();
                }
            }
#endif
        }
        impl->wasHandledFirstTouchMove = false;
    }
#endif
}

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
void  ewkViewTextStyleState(Evas_Object* ewkView, const IntPoint& startPoint, const IntPoint& endPoint)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    EditorState editorState = impl->page()->editorState();
    Ewk_Text_Style* textStyle = ewkTextStyleCreate(editorState, startPoint, endPoint);
    evas_object_smart_callback_call(ewkView, "text,style,state", static_cast<void*>(textStyle));
    ewkTextStyleDelete(textStyle);
}
#endif

#if ENABLE(SCREEN_ORIENTATION_SUPPORT) && ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT)
bool ewk_view_orientation_lock(Evas_Object* ewkView, int willLockOrientation)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(smartData->api, false);

    TIZEN_LOGI("willLockOrientation (%d)", willLockOrientation);
    if (!smartData->api->orientation_lock) {
        TIZEN_LOGE("fail");
        return false;
    }

    return smartData->api->orientation_lock(smartData, willLockOrientation);
}

void ewk_view_orientation_unlock(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EINA_SAFETY_ON_NULL_RETURN(smartData->api);

    TIZEN_LOGI("ewkView (%p)", ewkView);
    if (!smartData->api->orientation_unlock) {
        TIZEN_LOGE("fail");
        return;
    }

    smartData->api->orientation_unlock(smartData);
}
#endif

void ewk_view_orientation_lock_callback_set(Evas_Object* ewkView, Ewk_Orientation_Lock_Cb func, void* data)
{
#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT_INTERNAL)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);
    TIZEN_LOGI("callbacks: %p / data: %p", func, data);

    impl->orientationLock.callback = func;
    impl->orientationLock.data = data;
#endif
}

#if ENABLE(TIZEN_SCREEN_ORIENTATION_SUPPORT_INTERNAL)
Eina_Bool _ewk_orientation_lock(Ewk_View_Smart_Data *sd, int orientations)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, impl, false);
    TIZEN_LOGI("locked orientations : %d", orientations);

    if (impl->orientationLock.callback)
        return impl->orientationLock.callback(sd->self, true, orientations, impl->orientationLock.data);

    return false;
}

void _ewk_orientation_unlock(Ewk_View_Smart_Data *sd)
{
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, impl);
    TIZEN_LOGI("unlock is requested");

    if (impl->orientationLock.callback)
        impl->orientationLock.callback(sd->self, false, 0, impl->orientationLock.data);
}
#endif

#if ENABLE(TIZEN_RUNTIME_BACKEND_SELECTION)
bool ewk_view_is_opengl_backend(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);

    Ecore_Evas* ee = ecore_evas_ecore_evas_get(smartData->base.evas);
    const char *engine = ecore_evas_engine_name_get(ee);
    if (engine && !strcmp(engine, "opengl_x11"))
        return true;
    return false;
}
#endif

void ewk_view_zoomable_area_set(Evas_Object* ewkView, const IntPoint& target, const IntRect& area)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->gestureClient->setZoomableArea(target, area);
#endif
}

#if ENABLE(TIZEN_BACKFORWARD_LIST_CLEAR)
void ewk_view_back_forward_list_clear(const Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    WKBackForwardListClearBackForwardList(WKPageGetBackForwardList(toAPI(impl->pageProxy.get())));
}
#endif

#if ENABLE(TIZEN_ICON_DATABASE)
void ewkViewIconReceived(Evas_Object* ewkView)
{
    if (!ewkView)
        return;
    evas_object_smart_callback_call(ewkView, "icon,received", 0);
}
#endif

void ewk_view_motion_set(Evas_Object* ewkView, Eina_Bool enable, unsigned int sensitivity)
{
    notImplemented();
}

#endif // #if PLATFORM(TIZEN)

Eina_Bool ewk_view_feed_touch_event(Evas_Object* ewkView, Ewk_Touch_Event_Type type, const Eina_List* points, const Evas_Modifier* modifiers)
{
#if ENABLE(TOUCH_EVENTS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(points, false);
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

#if ENABLE(TIZEN_FOCUS_UI)
    if (type == EWK_TOUCH_START)
        impl->page()->setFocusUIEnabled(false);
#endif

#if ENABLE(TIZEN_GESTURE)
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    // We don't want to process touch event when context menu is shown.
    if ((impl->pageClient->isContextMenuVisible() && type != EWK_TOUCH_CANCEL)
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        && !impl->textSelection()->isTextSelectionMode()
#endif
        )
        return true;
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (impl->textSelection()->isTextSelectionDowned() && impl->textSelection()->isTextSelectionMode() && type != EWK_TOUCH_CANCEL)
        return true;
#endif

#if ENABLE(TIZEN_INPUT_COLOR_PICKER) // wait for upstream
    // We don't want to process touch event when color picker is shown.
    if (impl->page()->isColorPickerActive())
        return true;
#endif

    if (type == EWK_TOUCH_START) {
        if (eina_list_count(points) == 1) {
            impl->gestureClient->reset();
            Ewk_Touch_Point* point = static_cast<Ewk_Touch_Point*>(eina_list_data_get(points));
            impl->touchDownPoint.x = point->x;
            impl->touchDownPoint.y = point->y;
            impl->exceedTouchMoveThreshold = false;
            impl->exceedTouchTapThreshold = false;

            int viewX, viewY;
            evas_object_geometry_get(ewkView, &viewX, &viewY, 0, 0);
            impl->viewPositionAtTouchStart = IntPoint(viewX, viewY);

#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
            if (impl->settings()->touchFocusEnabled())
                impl->setFocusManually();
#endif
        }
    } else if (type == EWK_TOUCH_MOVE) {
        if (!impl->exceedTouchMoveThreshold) {
            if (eina_list_count(points) == 1) {
                unsigned int moveThreshold = elm_config_scroll_thumbscroll_threshold_get();
                Ewk_Touch_Point* point = static_cast<Ewk_Touch_Point*>(eina_list_data_get(points));
                int diffX = impl->touchDownPoint.x - point->x;
                int diffY = impl->touchDownPoint.y - point->y;
                if (static_cast<unsigned int>(diffX * diffX + diffY * diffY) > moveThreshold * moveThreshold)
                    impl->exceedTouchMoveThreshold = true;
                else
                    return true;
            } else {
                impl->exceedTouchMoveThreshold = true;
            }
        } else if (!impl->exceedTouchTapThreshold) {
            if (eina_list_count(points) == 1) {
                // FIXME: we should change tapThreshold value as a common fomular to support other models.
                unsigned int tapThreshold = 32;
                Ewk_Touch_Point* point = static_cast<Ewk_Touch_Point*>(eina_list_data_get(points));
                int diffX = impl->touchDownPoint.x - point->x;
                int diffY = impl->touchDownPoint.y - point->y;
                if (static_cast<unsigned int>(diffX * diffX + diffY * diffY) > tapThreshold * tapThreshold)
                    impl->exceedTouchTapThreshold = true;
                else
                    return true;
            } else {
                impl->exceedTouchTapThreshold = true;
            }
        }
    }
#endif // #if ENABLE(TIZEN_GESTURE)

    // FIXME: impl is used in the webkit opensource, but tizen webkit does not use it yet.
    //impl->page()->handleTouchEvent(NativeWebTouchEvent(type, points, modifiers, impl->transformFromScene(), impl->transformToScreen(), ecore_time_get()));
    impl->pageProxy->handleTouchEvent(NativeWebTouchEvent(type, points, modifiers, impl->transformFromScene(), impl->transformToScreen(), ecore_time_get()));

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_touch_events_enabled_set(Evas_Object* ewkView, Eina_Bool enabled)
{
#if ENABLE(TOUCH_EVENTS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->setTouchEventsEnabled(!!enabled);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_touch_events_enabled_get(const Evas_Object* ewkView)
{
#if ENABLE(TOUCH_EVENTS)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->touchEventsEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_view_main_frame_scrollbar_visible_set(Evas_Object* ewkView, Eina_Bool visible)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->mainFrameScrollbarVisibility = visible;
    impl->pageClient->updateVisibility();

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_view_main_frame_scrollbar_visible_get(const Evas_Object* ewkView)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->mainFrameScrollbarVisibility;
#else
    return false;
#endif
}

Eina_Bool ewk_view_page_save(Evas_Object* ewkView, const char* path)
{
#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    String title = ewk_view_title_get(ewkView);
    String url = ewk_view_url_get(ewkView);
    String directoryPath(path);
    impl->pageClient->startOfflinePageSave(directoryPath, url, title);

    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(path);
    return false;
#endif
}

Eina_Bool ewk_view_animated_scroll_set(Evas_Object* ewkView, int x, int y)
{
#if ENABLE(TIZEN_GESTURE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->gestureClient->scrollToWithAnimation(x, y);
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(x);
    UNUSED_PARAM(y);
    return false;
#endif
}

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
void ewkViewAddMessageToConsole(Evas_Object* ewkView, unsigned level, const String& message, unsigned int lineNumber, const String& source)
{
    OwnPtr<Ewk_Console_Message> ewkConsoleMessage = Ewk_Console_Message::create(level, message, lineNumber, source);
    evas_object_smart_callback_call(ewkView, "console,message", ewkConsoleMessage.get());
}
#endif

void ewk_view_content_security_policy_set(Evas_Object* ewkView, const char* policy, Ewk_CSP_Header_Type type)
{
#if ENABLE(TIZEN_CSP)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, sd);
    EWK_VIEW_IMPL_GET_OR_RETURN(sd, impl);

    TIZEN_LOGI("policy(%s), type(%d)\n", policy, type);
    impl->pageProxy->setContentSecurityPolicy(String::fromUTF8(policy), static_cast<WebCore::ContentSecurityPolicy::HeaderType>(type));
#endif
}

#if ENABLE(TIZEN_APPLICATION_CACHE)
static Eina_Bool _ewk_view_default_application_cache_permission(Evas_Object* ewkView, Ewk_Security_Origin* origin, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    ApplicationCachePermissionPopup* popup = new ApplicationCachePermissionPopup(origin, applicationCachePermissionText(String::fromUTF8(ewk_security_origin_host_get(origin))));
    impl->permissionPopupManager->addPermissionRequest(popup);
    return true;
}

Eina_Bool ewkViewRequestApplicationCachePermission(Evas_Object* ewkView, WKSecurityOriginRef origin)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->applicationCachePermissionContext || !impl->applicationCachePermissionContext->applicationCachePermissionCallback)
        return false;

    impl->isWaitingForApplicationCachePermission = true;
    if (impl->applicationCachePermissionOrigin)
        deleteSecurityOrigin(impl->applicationCachePermissionOrigin);
    impl->applicationCachePermissionOrigin = createSecurityOrigin(origin);

    return impl->applicationCachePermissionContext->applicationCachePermissionCallback(ewkView, impl->applicationCachePermissionOrigin, impl->applicationCachePermissionContext->userData) == EINA_TRUE;
}
#endif

void ewk_view_application_cache_permission_callback_set(Evas_Object* ewkView, Ewk_View_Applicacion_Cache_Permission_Callback callback, void* userData)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->applicationCachePermissionContext)
        impl->applicationCachePermissionContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->applicationCachePermissionContext->applicationCachePermissionCallback = callback;
    impl->applicationCachePermissionContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
#endif
}

void ewk_view_application_cache_permission_reply(Evas_Object* ewkView, Eina_Bool allow)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    TIZEN_LOGI("allow %d", allow);
    WKPageReplyApplicationCachePermission(toAPI(impl->page()), allow);
    if (impl->applicationCachePermissionOrigin)
        deleteSecurityOrigin(impl->applicationCachePermissionOrigin);
    impl->applicationCachePermissionOrigin = 0;
    impl->isWaitingForApplicationCachePermission = false;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(allow);
#endif
}

#if ENABLE(TIZEN_INDEXED_DATABASE)
static Eina_Bool _ewk_view_default_exceeded_indexed_database_quota_permission(Evas_Object* ewkView, Ewk_Security_Origin* origin, long long currentQuota, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    ExceededIndexedDatabaseQuotaPermissionPopup* popup = new ExceededIndexedDatabaseQuotaPermissionPopup(origin, exceededIndexedDatabaseQuotaPermissionText(String::fromUTF8(ewk_security_origin_host_get(origin))));
    impl->permissionPopupManager->addPermissionRequest(popup);
    return true;
}

bool ewkViewExceededIndexedDatabaseQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, long long currentUsage)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->exceededIndexedDatabaseQuotaContext || !impl->exceededIndexedDatabaseQuotaContext->exceededIndexedDatabaseQuotaCallback)
        return false;

    impl->isWaitingForExceededQuotaPopupReply = true;
    if (impl->exceededQuotaOrigin)
        deleteSecurityOrigin(impl->exceededQuotaOrigin);
    impl->exceededQuotaOrigin = createSecurityOrigin(origin);

    TIZEN_LOGI("currentUsage(%lld)", currentUsage);

    return impl->exceededIndexedDatabaseQuotaContext->exceededIndexedDatabaseQuotaCallback(ewkView, impl->exceededQuotaOrigin, currentUsage, impl->exceededIndexedDatabaseQuotaContext->userData) == EINA_TRUE;
}
#endif

void ewk_view_exceeded_indexed_database_quota_callback_set(Evas_Object* ewkView, Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback, void* userData)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->exceededIndexedDatabaseQuotaContext)
        impl->exceededIndexedDatabaseQuotaContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->exceededIndexedDatabaseQuotaContext->exceededIndexedDatabaseQuotaCallback = callback;
    impl->exceededIndexedDatabaseQuotaContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
#endif
}

void ewk_view_exceeded_indexed_database_quota_reply(Evas_Object* ewkView, Eina_Bool allow)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    TIZEN_LOGI("allow %d", allow);
    WKPageReplyExceededIndexedDatabaseQuota(toAPI(impl->page()), allow == EINA_TRUE);
    if (impl->exceededQuotaOrigin)
        deleteSecurityOrigin(impl->exceededQuotaOrigin);
    impl->exceededQuotaOrigin = 0;
    impl->isWaitingForExceededQuotaPopupReply = false;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(allow);
#endif
}

// EwkFindOptions should be matched up orders with WkFindOptions.
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_CASE_INSENSITIVE, kWKFindOptionsCaseInsensitive);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_AT_WORD_STARTS, kWKFindOptionsAtWordStarts);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_TREAT_MEDIAL_CAPITAL_AS_WORD_START, kWKFindOptionsTreatMedialCapitalAsWordStart);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_BACKWARDS, kWKFindOptionsBackwards);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_WRAP_AROUND, kWKFindOptionsWrapAround);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_SHOW_OVERLAY, kWKFindOptionsShowOverlay);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_SHOW_FIND_INDICATOR, kWKFindOptionsShowFindIndicator);
COMPILE_ASSERT_MATCHING_ENUM(EWK_FIND_OPTIONS_SHOW_HIGHLIGHT, kWKFindOptionsShowHighlight);

Eina_Bool ewk_view_text_find(Evas_Object* ewkView, const char* text, Ewk_Find_Options options, unsigned maxMatchCount)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(text, false);

    impl->pageProxy->findString(String::fromUTF8(text), static_cast<WebKit::FindOptions>(options), maxMatchCount);

    return true;
}

Eina_Bool ewk_view_text_find_highlight_clear(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->pageProxy->hideFindUI();

    return true;
}

Eina_Bool ewk_view_text_matches_count(Evas_Object* ewkView, const char* text, Ewk_Find_Options options, unsigned maxMatchCount)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(text, false);

    impl->pageProxy->countStringMatches(String::fromUTF8(text), static_cast<WebKit::FindOptions>(options), maxMatchCount);

    return true;
}

void ewk_view_exceeded_database_quota_callback_set(Evas_Object* ewkView, Ewk_View_Exceeded_Database_Quota_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->exceededDatabaseQuotaContext)
        impl->exceededDatabaseQuotaContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->exceededDatabaseQuotaContext->exceededDatabaseQuotaCallback = callback;
    impl->exceededDatabaseQuotaContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
#endif
}

void ewk_view_exceeded_database_quota_reply(Evas_Object* ewkView, Eina_Bool allow)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    TIZEN_LOGI("allow %d", allow);
    WKPageReplyExceededDatabaseQuota(toAPI(impl->page()), allow == EINA_TRUE);
    if (impl->exceededQuotaOrigin)
        deleteSecurityOrigin(impl->exceededQuotaOrigin);
    impl->exceededQuotaOrigin = 0;
    impl->isWaitingForExceededQuotaPopupReply = false;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(allow);
#endif
}

#if ENABLE(TIZEN_FILE_SYSTEM)
static Eina_Bool _ewk_view_default_exceeded_local_file_system_quota_permission(Evas_Object* ewkView, Ewk_Security_Origin* origin, long long currentQuota, void* userData)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    ExceededLocalFileSystemQuotaPermissionPopup* popup = new ExceededLocalFileSystemQuotaPermissionPopup(origin, exceededLocalFileSystemQuotaPermissionText(String::fromUTF8(ewk_security_origin_host_get(origin))));
    impl->permissionPopupManager->addPermissionRequest(popup);
    return true;
}

bool ewkViewExceededLocalFileSystemQuota(Evas_Object* ewkView, WKSecurityOriginRef origin, long long currentUsage)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->exceededLocalFileSystemQuotaContext || !impl->exceededLocalFileSystemQuotaContext->exceededLocalFileSystemQuotaCallback)
        return false;

    impl->isWaitingForExceededQuotaPopupReply = true;
    if (impl->exceededQuotaOrigin)
        deleteSecurityOrigin(impl->exceededQuotaOrigin);
    impl->exceededQuotaOrigin = createSecurityOrigin(origin);

    TIZEN_LOGI("currentUsage(%lld)", currentUsage);

    return impl->exceededLocalFileSystemQuotaContext->exceededLocalFileSystemQuotaCallback(ewkView, impl->exceededQuotaOrigin , currentUsage, impl->exceededLocalFileSystemQuotaContext->userData) == EINA_TRUE;
}
#endif

void ewk_view_exceeded_local_file_system_quota_callback_set(Evas_Object* ewkView, Ewk_View_Exceeded_Indexed_Database_Quota_Callback callback, void* userData)
{
#if ENABLE(TIZEN_FILE_SYSTEM)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->exceededLocalFileSystemQuotaContext)
        impl->exceededLocalFileSystemQuotaContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->exceededLocalFileSystemQuotaContext->exceededLocalFileSystemQuotaCallback = callback;
    impl->exceededLocalFileSystemQuotaContext->userData = userData;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);
#endif
}

void ewk_view_exceeded_local_file_system_quota_reply(Evas_Object* ewkView, Eina_Bool allow)
{
#if ENABLE(TIZEN_FILE_SYSTEM)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    TIZEN_LOGI("allow %d", allow);
    WKPageReplyExceededLocalFileSystemQuota(toAPI(impl->page()), allow == EINA_TRUE);
    if (impl->exceededQuotaOrigin)
        deleteSecurityOrigin(impl->exceededQuotaOrigin);
    impl->exceededQuotaOrigin = 0;
    impl->isWaitingForExceededQuotaPopupReply = false;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(allow);
#endif
}

#if ENABLE(TIZEN_INTERCEPT_REQUEST)
void ewk_view_intercept_request_callback_set (Evas_Object* ewkView, Ewk_View_Intercept_Request_Callback callback, void* user_data)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->interceptRequestContext)
        impl->interceptRequestContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->interceptRequestContext->interceptRequestCallback = callback;
    impl->interceptRequestContext->userData = user_data;
    impl->interceptRequestContext->ewkView = ewkView;

    if (callback)
        impl->pageProxy->setInterceptRequestEnabled(true);
    else
        impl->pageProxy->setInterceptRequestEnabled(false);
}
#endif

#if ENABLE(TIZEN_FOCUS_UI)
void ewk_view_unfocus_allow_callback_set(Evas_Object* ewkView, Ewk_View_Unfocus_Allow_Callback callback, void* user_data)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    if (!impl->unfocusAllowContext)
        impl->unfocusAllowContext = adoptPtr<Ewk_View_Callback_Context>(new Ewk_View_Callback_Context);
    impl->unfocusAllowContext->unfocusAllowCallback = callback;
    impl->unfocusAllowContext->userData = user_data;
    impl->unfocusAllowContext->ewkView = ewkView;
}
#endif

void ewk_view_use_settings_font(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_USE_SETTINGS_FONT)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->pageProxy->useSettingsFont();
#endif
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void ewkViewMediaControlsRequestRotate(Evas_Object* ewkView, const String& status)
{
    TIZEN_LOGI("status %s", status.utf8().data());

    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    String rotateStatus("mediacontrol,rotate,");
    rotateStatus.append(status);
    TIZEN_LOGI("callbackName (%s)", rotateStatus.utf8().data());
    evas_object_smart_callback_call(impl->view(), rotateStatus.utf8().data(), 0);
}

bool ewkViewOpenMediaPlayer(Evas_Object* ewkView, const String& url, const String& cookies, const String& type)
{
#if ENABLE(TIZEN_OPEN_PANEL)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->openMediaPlayer->open(ewkView, url, cookies, type);
#else
    return false;
#endif
}
#endif

char* ewk_view_get_cookies_for_url(Evas_Object* ewkView, const char* url)
{
#if ENABLE(TIZEN_GET_COOKIES_FOR_URL)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, NULL);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, NULL);

    String cookiesForURL;

    impl->pageProxy->getCookiesForURL(String::fromUTF8(url), cookiesForURL);
    if (cookiesForURL.isEmpty())
        return NULL;
    char* result = new char[cookiesForURL.length() + 1]();
    strncpy(result, cookiesForURL.utf8().data(), cookiesForURL.length());
    return result;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(url);
#endif
}

Eina_Bool ewk_view_fullscreen_exit(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_FULLSCREEN_API)
    TIZEN_LOGI("");

    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    if (!impl->pageProxy->fullScreenManager()->isFullScreen())
        return false;

    impl->pageProxy->fullScreenManager()->requestExitFullScreen();
    return true;
#else
    UNUSED_PARAM(ewkView);
return false;
#endif
}

Eina_Bool ewk_view_draws_transparent_background_set(Evas_Object* ewkView, Eina_Bool enabled)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    impl->pageProxy->setDrawsTransparentBackground(enabled);
    evas_object_image_alpha_set(smartData->image, enabled);

    return true;
}

Eina_Bool ewk_view_draws_transparent_background_get(Evas_Object* ewkView)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);

    return impl->pageProxy->drawsTransparentBackground();
}

void ewk_view_browser_font_set(Evas_Object* ewkView)
{
#if ENABLE(TIZEN_BROWSER_FONT)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->pageProxy->setBrowserFont();
#endif
}

void ewk_view_session_data_get(Evas_Object* ewkView, const char** data, unsigned* length)
{
#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    WebPageProxy::WebPageProxySessionStateFilterCallback dummyCallback;
    RefPtr<WebData> sessionData = impl->pageProxy->sessionStateData(dummyCallback, 0);
    if (sessionData.get() == NULL) {
        *data = NULL;
        *length = 0;
	return;
    }
    *data = reinterpret_cast<const char*>(sessionData->bytes());
    *length = sessionData->size();
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[BackForward] length = %u", sessionData->size());
#endif
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(data);
    UNUSED_PARAM(length);
#endif
}

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
static void ewk_view_session_data_restore(Evas_Object* ewkView, const char* data, unsigned length)
{
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[BackForward] length = %u", length);
#endif

    RefPtr<WebData> sessionData = WebData::create(reinterpret_cast<const unsigned char*>(data), length);
    impl->pageProxy->restoreFromSessionStateData(sessionData.get());
}
#endif

Eina_Bool ewk_view_split_scroll_overflow_enabled_get(const Evas_Object* ewkView)
{
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SPLIT)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    return impl->pageProxy->splitScrollOverflowEnabled();
#else
    UNUSED_PARAM(ewkView);
    return false;
#endif
}

Eina_Bool ewk_view_split_scroll_overflow_enabled_set(Evas_Object* ewkView, const Eina_Bool enabled)
{
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SPLIT)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, false);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, false);
    impl->pageProxy->setSplitScrollOverflowEnabled(enabled);
    return true;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(enabled);
    return false;
#endif
}

void ewk_view_bg_color_set(Evas_Object* ewkView, int red, int green, int blue, int alpha)
{
#if ENABLE(TIZEN_BACKGROUND_COLOR_API)
    if (EINA_UNLIKELY(alpha < 0 || alpha > 255)) {
        EINA_LOG_CRIT("Alpha should be between 0 and 255");
        return;
    }

    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->setBackgroundColor(red, green, blue, alpha);
#endif
}

void ewk_view_bg_color_get(Evas_Object* ewkView, int* red, int* green, int* blue, int* alpha)
{
#if ENABLE(TIZEN_BACKGROUND_COLOR_API)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl);

    impl->getBackgroundColor(red, green, blue, alpha);
#endif
}

char* ewk_view_cookies_get(Evas_Object* ewkView, const char* url)
{
#if ENABLE(TIZEN_GET_COOKIES_FOR_URL)
    EWK_VIEW_SD_GET_OR_RETURN(ewkView, smartData, NULL);
    EWK_VIEW_IMPL_GET_OR_RETURN(smartData, impl, NULL);

    String cookiesForURL;

    impl->pageProxy->getCookiesForURL(String::fromUTF8(url), cookiesForURL);
    if (cookiesForURL.isEmpty())
        return NULL;
    char* result = new char[cookiesForURL.length() + 1]();
    strncpy(result, cookiesForURL.utf8().data(), cookiesForURL.length());
    return result;
#else
    UNUSED_PARAM(ewkView);
    UNUSED_PARAM(url);
#endif
}
