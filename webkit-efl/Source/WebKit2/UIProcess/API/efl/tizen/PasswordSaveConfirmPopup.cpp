/*
 * Copyright (C) 2013 Samsung Electronics All rights reserved.
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

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#include "PasswordSaveConfirmPopup.h"

#include "EwkViewImpl.h"
#include "LocalizedStrings.h"

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
#include <dlfcn.h>
#include <efl_assist.h>
extern void* EflAssistHandle;
#endif

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
static void passwordSaveConfirmBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    PasswordSaveConfirmPopup* popup = static_cast<PasswordSaveConfirmPopup*>(data);
    if (!popup)
        return;
    popup->hide();
    ewk_context_password_confirm_popup_reply(popup->m_context, EWK_CONTEXT_PASSWORD_POPUP_NOT_NOW);
}
#endif

static void passwordSaveConfirmNeverCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    PasswordSaveConfirmPopup* popup = static_cast<PasswordSaveConfirmPopup*>(data);
    if (!popup)
        return;
    ewk_context_password_confirm_popup_reply(popup->m_context, EWK_CONTEXT_PASSWORD_POPUP_NEVER);
    popup->hide();
}

static void passwordSaveConfirmSaveCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    PasswordSaveConfirmPopup* popup = static_cast<PasswordSaveConfirmPopup*>(data);
    if (!popup)
        return;
    ewk_context_password_confirm_popup_reply(popup->m_context, EWK_CONTEXT_PASSWORD_POPUP_SAVE);

    popup->hide();
}

static void passwordSaveConfirmLaterCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    PasswordSaveConfirmPopup* popup = static_cast<PasswordSaveConfirmPopup*>(data);
    if (!popup)
        return;
    ewk_context_password_confirm_popup_reply(popup->m_context, EWK_CONTEXT_PASSWORD_POPUP_NOT_NOW);
    popup->hide();
}

PasswordSaveConfirmPopup::PasswordSaveConfirmPopup(Ewk_Context* context)
    : m_context(context)
    , m_view(0)
    , m_widgetWin(0)
    , m_popup(0)
    , m_isShow(false)
{
}

PasswordSaveConfirmPopup::~PasswordSaveConfirmPopup()
{
    if (m_widgetWin) {
        evas_object_del(m_widgetWin);
        m_widgetWin = 0;
        m_popup = 0;
    }
}

void PasswordSaveConfirmPopup::hide()
{
    if (m_popup) {
        evas_object_hide(m_widgetWin);
        evas_object_hide(m_popup);

        evas_object_del(m_widgetWin);
        evas_object_del(m_popup);
        m_popup = 0;
    }
    m_isShow = false;
}

void PasswordSaveConfirmPopup::show(Evas_Object* ewkView)
{
    if (!ewkView || m_isShow)
        return;

    m_view = ewkView;

    if (!m_popup) {
        Evas_Object* topWidget = elm_object_top_widget_get(elm_object_parent_widget_get(ewkView));

        if (!topWidget)
            topWidget = ewkView;

        m_widgetWin = elm_win_add(topWidget, "WebKit Center Popup", ELM_WIN_DIALOG_BASIC);

        if (!m_widgetWin)
            return;

        elm_win_alpha_set(m_widgetWin, EINA_TRUE);
        ecore_x_icccm_name_class_set(elm_win_xwindow_get(m_widgetWin), "APP_POPUP", "APP_POPUP");

        if (elm_win_wm_rotation_supported_get(topWidget)) {
            int preferredRotation = elm_win_wm_rotation_preferred_rotation_get(topWidget);
            if (preferredRotation == -1) {
                int rots[4] = {0, 90, 180, 270};
                elm_win_wm_rotation_available_rotations_set(m_widgetWin, rots, 4);
            } else
                elm_win_wm_rotation_available_rotations_set(m_widgetWin, &preferredRotation, 1);
        }

        m_popup = elm_popup_add(m_widgetWin);

        if (!m_popup)
            return;

        elm_popup_align_set(m_popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
        evas_object_size_hint_weight_set(m_popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_resize(m_widgetWin, WebCore::getDefaultScreenResolution().width(), WebCore::getDefaultScreenResolution().height());

        elm_object_domain_translatable_part_text_set(m_popup, "title,text","WebKit", "IDS_BR_HEADER_SAVE_SIGN_IN_INFO_ABB");

        elm_object_domain_translatable_text_set(m_popup, "WebKit", "IDS_BR_POP_SAVE_YOUR_USERNAMES_AND_PASSWORDS_FOR_WEBSITES_Q");

        Evas_Object* neverButton = elm_button_add(m_popup);
        elm_object_style_set(neverButton, "popup");
        elm_object_domain_translatable_part_text_set(neverButton, 0, "WebKit", "IDS_WEBVIEW_BUTTON2_NEVER");
        elm_object_part_content_set(m_popup, "button1", neverButton);
        evas_object_smart_callback_add(neverButton, "clicked", passwordSaveConfirmNeverCallback, this);

        Evas_Object* laterButton = elm_button_add(m_popup);
        elm_object_style_set(laterButton, "popup");
        elm_object_domain_translatable_part_text_set(laterButton, 0, "WebKit", "IDS_WEBVIEW_BUTTON_LATER_ABB");
        elm_object_part_content_set(m_popup, "button2", laterButton);
        evas_object_smart_callback_add(laterButton, "clicked", passwordSaveConfirmLaterCallback, this);

        Evas_Object* saveButton = elm_button_add(m_popup);
        elm_object_style_set(saveButton, "popup");
        elm_object_domain_translatable_part_text_set(saveButton, 0, "WebKit", "IDS_WEBVIEW_BUTTON_SAVE");
        elm_object_part_content_set(m_popup, "button3", saveButton);
        evas_object_smart_callback_add(saveButton, "clicked", passwordSaveConfirmSaveCallback, this);

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
        if (EflAssistHandle) {
            void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
            webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
            (*webkit_ea_object_event_callback_add)(m_popup, EA_CALLBACK_BACK, passwordSaveConfirmBackKeyCallback, this);
        }
#endif
    }
    evas_object_show(m_widgetWin);
    evas_object_show(m_popup);
    m_isShow = true;
}

void PasswordSaveConfirmPopup::setPasswordData(const String& url, WKFormDataRef formData)
{
    if (m_isShow)
        return;

    m_url = url;
    m_formData = WebFormData::create(toImpl(formData)->data());
}

void PasswordSaveConfirmPopup::savePasswordData()
{
    if (m_formData->values().isEmpty() || m_url.isEmpty())
        return;

    FormDatabase* formDatabae =  m_context->formDatabase();

    formDatabae->addPasswordFormData(m_url , toAPI(m_formData.get()), false);

    m_url = emptyString();
    m_formData.clear();
}

void PasswordSaveConfirmPopup::clearPasswordData()
{
    m_url = emptyString();
    m_formData.clear();
}

void PasswordSaveConfirmPopup::neverSavePasswordData()
{
    if(!m_url.length())
        return;

    FormDatabase* formDatabae =  m_context->formDatabase();
    formDatabae->addURLForPasswordNever(m_url);
    m_url = emptyString();
    m_formData.clear();
}

} // namespace WebKit

#endif // TIZEN_WEBKIT2_FORM_DATABASE
