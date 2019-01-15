/*
   Copyright (C) 2014 Samsung Electronics

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
#include "PermissionPopupManager.h"

#include "ewk_view.h"
#include "LocalizedStrings.h"
#include "PermissionPopup.h"
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


namespace WebKit {

PermissionPopupManager::PermissionPopupManager(Evas_Object* ewkView)
    : m_isDecided(false)
    , m_permissionPopups(0)
    , m_ewkView(ewkView)
    , m_popup(0)
    , m_widgetWin(0)
    , m_topWidget(0)
{
}

PermissionPopupManager::~PermissionPopupManager()
{
    closePopup();
    deleteAllPermissionRequest();
}

void PermissionPopupManager::addPermissionRequest(PermissionPopup* popup)
{
    m_permissionPopups = eina_list_append(m_permissionPopups, popup);
    showPermissionPopup(static_cast<PermissionPopup*>(eina_list_data_get(m_permissionPopups)));
}

void PermissionPopupManager::deletePermissionRequest(PermissionPopup* popup)
{
    m_permissionPopups = eina_list_remove(m_permissionPopups, popup);
    delete popup;

    if (eina_list_count(m_permissionPopups) == 0)
        return;

    showPermissionPopup(static_cast<PermissionPopup*>(eina_list_data_get(m_permissionPopups)));
}

void PermissionPopupManager::deleteAllPermissionRequest()
{
    void* data;
    EINA_LIST_FREE(m_permissionPopups, data)
        delete static_cast<PermissionPopup*>(data);
}

Evas_Object* PermissionPopupManager::createPopup()
{
    m_topWidget = elm_object_top_widget_get(elm_object_parent_widget_get(m_ewkView));
    if (!m_topWidget)
        return 0;

    m_widgetWin = elm_win_add(m_topWidget, "WebKit Permission Popup", ELM_WIN_BASIC);
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
    evas_object_show(m_widgetWin);

    elm_object_content_set(conformant, layout);
    elm_win_conformant_set(m_widgetWin, EINA_TRUE);

    Evas_Object* popup = elm_popup_add(layout);
    elm_popup_align_set(popup, ELM_NOTIFY_ALIGN_FILL, 1.0);

    return popup;
}


static void permissionPopupFocusInCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    evas_object_smart_callback_del(obj, "focus,in", permissionPopupFocusInCallback);

    PermissionPopupManager* manager = static_cast<PermissionPopupManager*>(data);
    if (!manager)
        return;
    manager->closePopup();

    PermissionPopup* popup = manager->getPermissionPopup();
    if (!popup)
        return;

    popup->sendDecidedPermission(manager->ewkView(), manager->isDecided());
    manager->deletePermissionRequest(popup);
}

static void permissionOkCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    PermissionPopupManager* manager = static_cast<PermissionPopupManager*>(data);
    if (!manager)
        return;

    evas_object_smart_callback_add(manager->getTopWidget(), "focus,in", permissionPopupFocusInCallback, data);
    manager->hidePopup();
    manager->setIsDecided(true);
}

static void permissionCancelCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    PermissionPopupManager* manager = static_cast<PermissionPopupManager*>(data);
    if (!manager)
        return;

    evas_object_smart_callback_add(manager->getTopWidget(), "focus,in", permissionPopupFocusInCallback, data);
    manager->hidePopup();
    manager->setIsDecided(false);
}

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
static void permissionHwBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    PermissionPopupManager* manager = static_cast<PermissionPopupManager*>(data);
    if (!manager)
        return;

    evas_object_smart_callback_add(manager->getTopWidget(), "focus,in", permissionPopupFocusInCallback, data);
    manager->hidePopup();
}
#endif

void PermissionPopupManager::showPermissionPopup(PermissionPopup* popup)
{
    if (m_popup)
        return;

    m_popup = createPopup();

    if (!m_popup)
        return;

    if (!setLabelText(popup->getMessage()))
        return;

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    if (EflAssistHandle) {
        void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
        webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
        (*webkit_ea_object_event_callback_add)(m_popup, EA_CALLBACK_BACK, permissionHwBackKeyCallback, this);
    }
#endif

    Evas_Object* cancelButton = elm_button_add(m_popup);
    elm_object_style_set(cancelButton, "popup");
    elm_object_domain_translatable_part_text_set(cancelButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_CANCEL_ABB4");
    elm_object_part_content_set(m_popup, "button1", cancelButton);
    evas_object_smart_callback_add(cancelButton, "clicked", permissionCancelCallback, this);

    Evas_Object* okButton = elm_button_add(m_popup);
    elm_object_style_set(okButton, "popup");
    elm_object_domain_translatable_part_text_set(okButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_OK_ABB4");
    elm_object_part_content_set(m_popup, "button2", okButton);
    evas_object_focus_set(okButton, true);
    evas_object_smart_callback_add(okButton, "clicked", permissionOkCallback, this);

    elm_object_part_text_set(m_popup, "title,text", popup->getOriginHost().utf8().data());
    evas_object_show(m_popup);
}

String PermissionPopupManager::getTitle(PermissionPopup* popup)
{
    return String::format(permissionPopupTitle().utf8().data(), popup->getOriginHost().utf8().data());
}

bool PermissionPopupManager::setLabelText(String message)
{
    if (!message)
        return false;

    message.replace("\n", "</br>");
    elm_object_text_set(m_popup, message.utf8().data());

    return true;
}

Evas_Object* PermissionPopupManager::ewkView()
{
    return m_ewkView;
}

void PermissionPopupManager::closePopup()
{
    if (!m_widgetWin)
        return;

    evas_object_del(m_widgetWin);
    m_popup = 0;
    m_widgetWin = 0;
}

void PermissionPopupManager::hidePopup(){
    if (m_popup)
        evas_object_hide(m_popup);

    if (m_widgetWin)
        evas_object_hide(m_widgetWin);
}

PermissionPopup* PermissionPopupManager::getPermissionPopup()
{
    return static_cast<PermissionPopup*>(eina_list_data_get(m_permissionPopups));
}

} // namespace WebKit
