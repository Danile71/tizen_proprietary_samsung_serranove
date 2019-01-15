/*
   Copyright (C) 2012 Samsung Electronics

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
#include "JavaScriptPopup.h"

#include "ewk_view.h"
#include "LocalizedStrings.h"
#include <Elementary.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WebCore/EflScreenUtilities.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
#include <dlfcn.h>
#include <efl_assist.h>
extern void* EflAssistHandle;
#endif

#if PLATFORM(TIZEN)

namespace WebKit {

JavaScriptPopup::JavaScriptPopup(Evas_Object* ewkView)
    : m_popup(0)
    , m_popupMessage(String())
    , m_entry(0)
    , m_ewkView(ewkView)
    , m_widgetWin(0)
    , m_topWidget(0)
    , m_isConfirm(0)
    , m_promptReply(0)
{
}

JavaScriptPopup::~JavaScriptPopup()
{
    close();
}

static void alertPopupFocusInCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    evas_object_smart_callback_del(obj, "focus,in", alertPopupFocusInCallback);
    if (!popup)
        return;
    popup->close();
    ewk_view_javascript_alert_reply(popup->ewkView());
}


static void alertResponseCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;
    evas_object_smart_callback_add(popup->getTopWidget(), "focus,in", alertPopupFocusInCallback, data);
    popup->hidePopup();
}

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
static void alertHwBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;
    evas_object_smart_callback_add(popup->getTopWidget(), "focus,in", alertPopupFocusInCallback, data);
    popup->hidePopup();
}
#endif

Evas_Object* JavaScriptPopup::popupAdd()
{
    m_topWidget = elm_object_top_widget_get(elm_object_parent_widget_get(m_ewkView));
    if (!m_topWidget)
        return 0;

    m_widgetWin = elm_win_add(m_topWidget, "WebKit JavaScript Popup", ELM_WIN_DIALOG_BASIC);
    if (!m_widgetWin)
        return 0;

    elm_win_alpha_set(m_widgetWin, EINA_TRUE);
    ecore_x_icccm_name_class_set(elm_win_xwindow_get(m_widgetWin), "APP_POPUP", "APP_POPUP");

    if (elm_win_wm_rotation_supported_get(m_topWidget)) {
        int preferredRotation = elm_win_wm_rotation_preferred_rotation_get(m_topWidget);
        if (preferredRotation == -1) {
            int rots[4] = {0, 90, 180, 270};
            elm_win_wm_rotation_available_rotations_set(m_widgetWin, rots, 4);
        } else
            elm_win_wm_rotation_available_rotations_set(m_widgetWin, &preferredRotation, 1);
    }

    evas_object_resize(m_widgetWin, WebCore::getDefaultScreenResolution().width(), WebCore::getDefaultScreenResolution().height());

    Evas_Object* conformant = elm_conformant_add(m_widgetWin);
    if (!conformant)
        return 0;

    evas_object_size_hint_weight_set(conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(m_widgetWin, conformant);
    evas_object_show(conformant);

    Evas_Object* layout = elm_layout_add(conformant);
    if (!layout)
        return 0;

    elm_layout_theme_set(layout, "layout", "application", "default");
    evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(layout);

    elm_object_content_set(conformant, layout);
    elm_win_conformant_set(m_widgetWin, EINA_TRUE);

    evas_object_show(m_widgetWin);

    /* This is workaround for platform issue, popup was not displayed sometimes
       http://suprem.sec.samsung.net/jira/browse/CBBROWSER-806*/
    elm_win_render(m_widgetWin);

    Evas_Object* popup = elm_popup_add(layout);
    elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);

    return popup;
}

String JavaScriptPopup::getTitle()
{
    String url = String::fromUTF8(ewk_view_url_get(m_ewkView));
    KURL kurl(KURL(), url);
    url = kurl.host();

    if (url.isEmpty())
        url = String::fromUTF8(ewk_view_title_get(m_ewkView));

    String jsPopupTitle = javaScriptPopupTitle();
    jsPopupTitle = jsPopupTitle.replace("%s", url);

    return jsPopupTitle;
}

bool JavaScriptPopup::setLabelText(const char* message)
{
    if (!message)
        return false;

    m_popupMessage = String::fromUTF8(elm_entry_utf8_to_markup(message));
    m_popupMessage.replace("\n", "</br>");

    elm_object_text_set(m_popup, m_popupMessage.utf8().data());

    return true;
}

bool JavaScriptPopup::alert(const char* message)
{
    if (m_popup)
        return false;

    m_popup = popupAdd();
    if (!m_popup)
        return false;

    if (!setLabelText(message))
        return false;

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    if (EflAssistHandle) {
        void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
        webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
        (*webkit_ea_object_event_callback_add)(m_popup, EA_CALLBACK_BACK, alertHwBackKeyCallback, this);
    }
#endif

    Evas_Object* okButton = elm_button_add(m_popup);
    elm_object_style_set(okButton, "popup");
    elm_object_domain_translatable_part_text_set(okButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_OK_ABB4");
    elm_object_part_content_set(m_popup, "button1", okButton);
    evas_object_focus_set(okButton, true);
    evas_object_smart_callback_add(okButton, "clicked", alertResponseCallback, this);

    elm_object_part_text_set(m_popup, "title,text", getTitle().utf8().data());
    evas_object_show(m_popup);

    return true;
}

static void confirmPopupFocusInCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    evas_object_smart_callback_del(obj, "focus,in", confirmPopupFocusInCallback);
    if (!popup)
        return;
    popup->close();
    ewk_view_javascript_confirm_reply(popup->ewkView(), popup->isConfirm());
}

static void confirmOkCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;
    popup->setIsConfirm(true);
    evas_object_smart_callback_add(popup->getTopWidget(), "focus,in", confirmPopupFocusInCallback, data);
    popup->hidePopup();
}

static void confirmCancelCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;
    popup->setIsConfirm(false);
    evas_object_smart_callback_add(popup->getTopWidget(), "focus,in", confirmPopupFocusInCallback, data);
    popup->hidePopup();
}

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
static void confirmHwBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;
    popup->setIsConfirm(false);
    evas_object_smart_callback_add(popup->getTopWidget(), "focus,in", confirmPopupFocusInCallback, data);
    popup->hidePopup();
}
#endif

bool JavaScriptPopup::confirm(const char* message)
{
    if (m_popup)
        return false;

    m_popup = popupAdd();
    if (!m_popup)
        return false;

    if (!setLabelText(message))
        return false;

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    if (EflAssistHandle) {
        void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
        webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
        (*webkit_ea_object_event_callback_add)(m_popup, EA_CALLBACK_BACK, confirmHwBackKeyCallback, this);
    }
#endif

    Evas_Object* cancelButton = elm_button_add(m_popup);
    elm_object_style_set(cancelButton, "popup");
    elm_object_domain_translatable_part_text_set(cancelButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_CANCEL_ABB4");
    elm_object_part_content_set(m_popup, "button1", cancelButton);
    evas_object_smart_callback_add(cancelButton, "clicked", confirmCancelCallback, this);

    Evas_Object* okButton = elm_button_add(m_popup);
    elm_object_style_set(okButton, "popup");
    elm_object_domain_translatable_part_text_set(okButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_OK_ABB4");
    elm_object_part_content_set(m_popup, "button2", okButton);
    evas_object_focus_set(okButton, true);
    evas_object_smart_callback_add(okButton, "clicked", confirmOkCallback, this);

    elm_object_part_text_set(m_popup, "title,text", getTitle().utf8().data());
    evas_object_show(m_popup);

    return true;
}

static void promptEntryChanged(void* data, Ecore_IMF_Context* ctx, int value)
{
    if (value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        Evas_Object* entry = static_cast<Evas_Object*>(data);
        elm_object_focus_set(entry, false);
    }
}

static void promptEnterKeyDownCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    elm_entry_input_panel_hide(obj);
}

static void promptPopupFocusInCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    evas_object_smart_callback_del(obj, "focus,in", promptPopupFocusInCallback);
    if (!popup)
        return;
    popup->close();
    char* result = popup->getPromptReply();
    ewk_view_javascript_prompt_reply(popup->ewkView(), result);
    if (result)
        free(result);
    popup->setPromptReply(0);
}

static void promptCancelCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;
    evas_object_smart_callback_add(popup->getTopWidget(), "focus,in", promptPopupFocusInCallback, data);
    popup->hidePopup();
}

static void promptOkCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;
    char* result = elm_entry_markup_to_utf8(elm_entry_entry_get(popup->entry()));
    popup->setPromptReply(result);
    evas_object_smart_callback_add(popup->getTopWidget(), "focus,in", promptPopupFocusInCallback, data);
    popup->hidePopup();
}

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
static void promptHwBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;
    evas_object_smart_callback_add(popup->getTopWidget(), "focus,in", promptPopupFocusInCallback, data);
    popup->hidePopup();
}
#endif

bool JavaScriptPopup::prompt(const char* message, const char* defaultValue)
{
    if (m_popup)
        return false;

    m_popup = popupAdd();
    if (!m_popup)
        return false;

    if (message)
        elm_object_part_text_set(m_popup, "title,text", message);
    else
        elm_object_part_text_set(m_popup, "title,text", getTitle().utf8().data());

    Evas_Object* layout = elm_layout_add(m_popup);
    elm_layout_file_set(layout, EDJE_DIR"/JavaScriptPopup.edj", "prompt");
    evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    m_entry = elm_entry_add(m_popup);
    Ecore_IMF_Context* imfContext = static_cast<Ecore_IMF_Context*>(elm_entry_imf_context_get(m_entry));
    ecore_imf_context_input_panel_event_callback_add(imfContext, ECORE_IMF_INPUT_PANEL_STATE_EVENT, promptEntryChanged, 0);
    elm_entry_single_line_set(m_entry, EINA_TRUE);
    elm_entry_scrollable_set(m_entry, EINA_TRUE);
    elm_entry_input_panel_return_key_type_set(m_entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE );
    evas_object_smart_callback_add(m_entry, "activated", promptEnterKeyDownCallback, 0);
    elm_object_text_set(m_entry, defaultValue);
    elm_entry_cursor_end_set(m_entry);

    elm_object_part_content_set(layout, "prompt_container", m_entry);
    elm_object_content_set(m_popup, layout);

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    if (EflAssistHandle) {
        void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
        webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
        (*webkit_ea_object_event_callback_add)(m_popup, EA_CALLBACK_BACK, promptHwBackKeyCallback, this);
    }
#endif
    Evas_Object* cancelButton = elm_button_add(m_popup);
    elm_object_style_set(cancelButton, "popup");
    elm_object_domain_translatable_part_text_set(cancelButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_CANCEL_ABB4");
    elm_object_part_content_set(m_popup, "button1", cancelButton);
    evas_object_smart_callback_add(cancelButton, "clicked", promptCancelCallback, this);

    Evas_Object* okButton = elm_button_add(m_popup);
    elm_object_style_set(okButton, "popup");
    elm_object_domain_translatable_part_text_set(okButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_OK_ABB4");
    elm_object_part_content_set(m_popup, "button2", okButton);
    evas_object_smart_callback_add(okButton, "clicked", promptOkCallback, this);
    evas_object_focus_set(okButton, true);

    evas_object_show(m_popup);
    return true;
}

#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
static void beforeUnloadConfirmPanelLeaveCallback(void* data, Evas_Object*, void*)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;

    popup->close();
    ewk_view_before_unload_confirm_panel_reply(popup->ewkView(), true);
}

static void beforeUnloadConfirmPanelStayCallback(void* data, Evas_Object*, void*)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;

    popup->close();
    ewk_view_before_unload_confirm_panel_reply(popup->ewkView(), false);
}

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
static void beforeUnloadConfirmPanelHwBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    JavaScriptPopup* popup = static_cast<JavaScriptPopup*>(data);
    if (!popup)
        return;

    popup->close();
    ewk_view_before_unload_confirm_panel_reply(popup->ewkView(), false);
}
#endif

bool JavaScriptPopup::beforeUnloadConfirmPanel(const char* message)
{
    if (m_popup)
        return false;

    m_popup = popupAdd();
    if (!m_popup)
        return false;

    String popupMessage;
    if (message)
        popupMessage = String::fromUTF8(message) + "\n" + beforeUnloadConfirmPopupMessage();
    else
        popupMessage = beforeUnloadConfirmPopupMessage();

    if (!setLabelText(popupMessage.utf8().data()))
        return false;

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    if (EflAssistHandle) {
        void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
        webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
        (*webkit_ea_object_event_callback_add)(m_popup, EA_CALLBACK_BACK, beforeUnloadConfirmPanelHwBackKeyCallback, this);
    }
#endif

    Evas_Object* leaveButton = elm_button_add(m_popup);
    elm_object_style_set(leaveButton, "popup");
    // FIXME: Need to apply i18n for the button label
    elm_object_domain_translatable_part_text_set(leaveButton, 0, "WebKit", "IDS_WEBVIEW_BUTTON_LEAVE");
    elm_object_part_content_set(m_popup, "button1", leaveButton);
    evas_object_smart_callback_add(leaveButton, "clicked", beforeUnloadConfirmPanelLeaveCallback, this);

    Evas_Object* stayButton = elm_button_add(m_popup);
    elm_object_style_set(stayButton, "popup");
    // FIXME: Need to apply i18n for the button label
    elm_object_domain_translatable_part_text_set(stayButton, 0, "WebKit", "IDS_WEBVIEW_BUTTON_STAY");
    elm_object_part_content_set(m_popup, "button2", stayButton);
    evas_object_smart_callback_add(stayButton, "clicked", beforeUnloadConfirmPanelStayCallback, this);

    elm_object_part_text_set(m_popup, "title,text", getTitle().utf8().data());
    evas_object_show(m_popup);

    return true;
}
#endif

Evas_Object* JavaScriptPopup::ewkView()
{
    return m_ewkView;
}

Evas_Object* JavaScriptPopup::entry()
{
    return m_entry;
}

void JavaScriptPopup::close()
{
    if (m_widgetWin) {
        evas_object_del(m_widgetWin);
        m_popup = 0;
        m_widgetWin = 0;
    }
    m_entry = 0;
}

void JavaScriptPopup::hidePopup(){
    if (m_popup)
        evas_object_hide(m_popup);

    if (m_widgetWin)
        evas_object_hide(m_widgetWin);
}

} // namespace WebKit

#endif // PLATFORM(TIZEN)
