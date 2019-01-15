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
 */

#include "config.h"

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
#include "ewk_popup_picker.h"

#include "LocalizedStrings.h"
#include "WebPopupMenuProxyEfl.h"
#include "ewk_view.h"
#include "ewk_view_private.h"
#include "ewk_popup_menu_item.h"
#include <Elementary.h>
#include <libintl.h>
#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
#include <dlfcn.h>
#include <efl_assist.h>
extern void* EflAssistHandle;
#endif

#if ENABLE(TIZEN_MULTIPLE_SELECT)
int compareChangedItems(const void* a, const void* b)
{
    const int* left = static_cast<const int*>(a);
    const int* right = static_cast<const int*>(b);
    return (*left - *right);
}
#endif

static void showPickerFinishedCallback(void* data, Evas_Object* obj, const char* emission, const char* source)
{
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);

    if (!picker)
        return;

    elm_win_prop_focus_skip_set(picker->win, EINA_FALSE);
    elm_win_activate(picker->win);

    if (picker->m_genlist_callback_data_list) {
        Eina_List* list;
        void* data;

        EINA_LIST_FOREACH(picker->m_genlist_callback_data_list, list, data) {
            Elm_Object_Item* item = (static_cast<genlist_callback_data*>(data))->it;
            if (picker->selectedIndex == (elm_genlist_item_index_get(item) - 1)) {
                elm_object_focus_set(picker->popupList, true);
                elm_genlist_item_show(item, ELM_GENLIST_ITEM_SCROLLTO_TOP);
                break;
            }
        }
    }
}

static void __picker_list_realized_cb(void* data, Evas_Object* obj, void* event_info)
{
    if (!event_info)
        return;

    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);
    Elm_Object_Item* item = static_cast<Elm_Object_Item*>(event_info);
    Evas_Object* content = elm_object_item_part_content_get(item, "elm.icon.1");
    Evas_Object* icon = elm_object_part_content_get(content, "elm.swallow.content");

    if ((elm_genlist_item_index_get(item) - 1) == picker->selectedIndex) {
#if ENABLE(TIZEN_MULTIPLE_SELECT)
        if (!picker->multiSelect)
#endif
            elm_radio_value_set(icon, picker->selectedIndex);

#if ENABLE(TIZEN_SCREEN_READER)
        elm_access_highlight_set(elm_object_item_access_object_get(item));
#endif
    }
}

#if ENABLE(TIZEN_POPUP_PICKER_WITH_BUTTONS)
#if ENABLE(TIZEN_SCREEN_READER)
static void __picker_access_list_close_cb(void* data, Evas_Object* obj, Elm_Object_Item* item)
{
    Evas_Object* container = static_cast<Evas_Object*>(data);
    edje_object_signal_emit(elm_layout_edje_get(container), "mouse,clicked,1", "elm.button.done");
}

static char* __picker_access_info_cb(void* data, Evas_Object* obj)
{
    if (data) {
        String* text = static_cast<String*>(data);
        return strdup(text->utf8().data());
    }

    return 0;
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
static void __picker_access_navigate_to_next_cb(void* data, Evas_Object* obj, Elm_Object_Item* item)
{
    Evas_Object* container = static_cast<Evas_Object*>(data);
    edje_object_signal_emit(elm_layout_edje_get(container), "mouse,clicked,1", "elm.button.next");
}

static void __picker_access_navigate_to_prev_cb(void* data, Evas_Object* obj, Elm_Object_Item* item)
{
    Evas_Object* container = static_cast<Evas_Object*>(data);
    edje_object_signal_emit(elm_layout_edje_get(container), "mouse,clicked,1", "elm.button.prev");
}
#endif

static void __picker_icon_changed_cb(void* data, Evas_Object* obj, void* event_info)
{
    genlist_callback_data* callback_data = static_cast<genlist_callback_data*>(data);
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(callback_data->user_data);

    if (picker->multiSelect) {
        Elm_Object_Item* item = callback_data->it;
        int index = elm_genlist_item_index_get(item) - 1;
        int pos = eina_inarray_search(picker->changedList, &index, compareChangedItems);

        if (pos == -1)
            eina_inarray_push(picker->changedList, &index);
        else
            eina_inarray_remove(picker->changedList, &index);
    } else {
        int index = elm_radio_value_get(picker->radioMain);
        picker->selectedIndex = index;
        ewk_view_popup_menu_select(picker->parent, index);
    }
}

static char* __picker_label_get_cb(void* data, Evas_Object* obj, const char* part)
{
    genlist_callback_data* callback_data = static_cast<genlist_callback_data*>(data);
    Ewk_Popup_Menu_Item* menuItem = static_cast<Ewk_Popup_Menu_Item*>(callback_data->menuItem);

    if (!strncmp(part, "elm.text.main.left", strlen("elm.text.main.left")))
        return strdup(ewk_popup_menu_item_text_get(menuItem));

    return 0;
}

static Evas_Object* __picker_icon_get_cb(void* data, Evas_Object* obj, const char* part)
{
    genlist_callback_data* callback_data = static_cast<genlist_callback_data*>(data);
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(callback_data->user_data);
    Ewk_Popup_Menu_Item* menuItem = static_cast<Ewk_Popup_Menu_Item*>(callback_data->menuItem);

    if (menuItem->isLabel)
        return 0;

    if (!strcmp(part, "elm.icon.1")) {
        Evas_Object* content = elm_layout_add(obj);
        elm_layout_theme_set(content, "layout", "list/B/type.4", "default");

        Evas_Object* icon = 0;
        if (picker->multiSelect) {
            icon = elm_check_add(content);
            if (menuItem->isSelected)
                elm_check_state_set(icon, true);
        } else {
            icon = elm_radio_add(content);
            elm_radio_state_value_set(icon, callback_data->index);
            elm_radio_group_add(icon, picker->radioMain);
            elm_radio_value_set(picker->radioMain, picker->selectedIndex);
        }
#if ENABLE(TIZEN_SCREEN_READER)
        elm_access_object_unregister(icon);
#endif

        evas_object_size_hint_weight_set(icon, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(icon, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_propagate_events_set(icon, EINA_FALSE);
        evas_object_smart_callback_add(icon, "changed", __picker_icon_changed_cb, (void*)data);

        elm_layout_content_set(content, "elm.swallow.content", icon);
        return content;
    }
    return 0;
}

static void __picker_item_selected_cb(void* data, Evas_Object* obj, void* event_info)
{
    genlist_callback_data* callback_data = static_cast<genlist_callback_data*>(data);
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(callback_data->user_data);
    Elm_Object_Item* item = static_cast<Elm_Object_Item*>(event_info);
    Evas_Object* content = elm_object_item_part_content_get(item, "elm.icon.1");
    Evas_Object* icon = elm_object_part_content_get(content, "elm.swallow.content");

    elm_genlist_item_selected_set(item, EINA_FALSE);

    if (picker->multiSelect) {
        Eina_Bool state = elm_check_state_get(icon);
        elm_check_state_set(icon, !state);

        int index = elm_genlist_item_index_get(item) - 1;
        int pos = eina_inarray_search(picker->changedList, &index, compareChangedItems);
        if (pos == -1)
            eina_inarray_push(picker->changedList, &index);
        else
            eina_inarray_remove(picker->changedList, &index);
    } else {
        int index = callback_data->index;
        elm_radio_value_set(icon, callback_data->index);
        picker->selectedIndex = index;
        ewk_view_popup_menu_select(picker->parent, index);
    }
}

void clear_genlist_callback_data(Ewk_Popup_Picker* picker)
{
    if (picker && picker->m_genlist_callback_data_list) {
        Eina_List* list;
        void* data;

        EINA_LIST_FOREACH(picker->m_genlist_callback_data_list, list, data) {
            if (data) {
                elm_object_item_del((static_cast<genlist_callback_data*>(data))->it);
                (static_cast<genlist_callback_data*>(data))->it = 0;
                (static_cast<genlist_callback_data*>(data))->menuItem = 0;
                (static_cast<genlist_callback_data*>(data))->user_data = 0;
                free (static_cast<genlist_callback_data*>(data));
            }
        }
        eina_list_free(picker->m_genlist_callback_data_list);
        picker->m_genlist_callback_data_list = 0;
    }
}
#endif

void menuItemActivated(void* data, Evas_Object* obj, void* event_info)
{
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);
    Elm_Object_Item* selected = static_cast<Elm_Object_Item*>(event_info);
    int index = elm_genlist_item_index_get(selected) - 1;

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    if (picker->multiSelect) {
        int pos = eina_inarray_search(picker->changedList, &index, compareChangedItems);
        if (pos == -1)
            eina_inarray_push(picker->changedList, &index);
        else
            eina_inarray_remove(picker->changedList, &index);

        return;
    }
#endif
    picker->selectedIndex = index;
    ewk_view_popup_menu_select(picker->parent, index);
}

#if ENABLE(TIZEN_MULTIPLE_SELECT)
void menuItemDeactivated(void* data, Evas_Object* obj, void* event_info)
{
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);
    Elm_Object_Item* deselectedItem = static_cast<Elm_Object_Item*>(event_info);

    int deselectedIndex = elm_genlist_item_index_get(deselectedItem) - 1;
    int pos = eina_inarray_search(picker->changedList, &deselectedIndex, compareChangedItems);
    if (pos == -1)
        eina_inarray_push(picker->changedList, &deselectedIndex);
    else
        eina_inarray_remove(picker->changedList, &deselectedIndex);
}
#endif

void listClosed(void* data, Evas_Object* obj, void* event_info)
{
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    if (picker->multiSelect)
        ewk_view_popup_menu_multiple_select(picker->parent, picker->changedList);
    else
        ewk_view_popup_menu_select(picker->parent, picker->selectedIndex);
    eina_inarray_free(picker->changedList);
#else
    ewk_view_popup_menu_select(picker->parent, picker->selectedIndex);

#endif
#if ENABLE(TIZEN_POPUP_PICKER_WITH_BUTTONS)
    clear_genlist_callback_data(picker);
#endif
    ewk_view_popup_menu_close(picker->parent);
}

void dimPressed(void* data, Evas_Object* obj, const char* emission, const char* source)
{
    listClosed(data, obj, 0);
}

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
void navigateToNext(void* data, Evas_Object* obj, void* event_info)
{
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);
    ewk_view_form_navigate(picker->parent, true);
}

void navigateToPrev(void* data, Evas_Object* obj, void* event_info)
{
    Ewk_Popup_Picker* picker = static_cast<Ewk_Popup_Picker*>(data);
    ewk_view_form_navigate(picker->parent, false);
}
#endif

#if !ENABLE(TIZEN_POPUP_PICKER_WITH_BUTTONS)
static char* _listLabelGet(void* data, Evas_Object* obj, const char* part)
{
    Ewk_Popup_Menu_Item* menuItem = static_cast<Ewk_Popup_Menu_Item*>(data);

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    if (!strncmp(part, "elm.text", strlen("elm.text")) && ewk_popup_menu_item_text_get(menuItem))
#else
    if (!strncmp(part, "elm.text", strlen("elm.text")))
#endif
        return strdup(ewk_popup_menu_item_text_get(menuItem));
    return 0;
}
#endif

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
static void popupMenuHwBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo)
{
   listClosed(data, obj, 0);
}
#endif

static void createAndShowPopupList(Evas_Object* win, Ewk_Popup_Picker* picker, Eina_List* items, int selectedIndex, bool isPickerExists = false)
{
    picker->selectedIndex = selectedIndex;
    picker->popupList = elm_genlist_add(picker->container);
    elm_genlist_homogeneous_set(picker->popupList, EINA_TRUE);
    elm_genlist_mode_set(picker->popupList, ELM_LIST_COMPRESS);
    evas_object_size_hint_weight_set(picker->popupList, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(picker->popupList, EVAS_HINT_FILL, EVAS_HINT_FILL);

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    if (picker->multiSelect) {
        eina_inarray_flush(picker->changedList);
        elm_genlist_multi_select_set(picker->popupList, true);
    }
#endif

    static Elm_Genlist_Item_Class itemClass;
    void* itemv;
    const Eina_List* l;
    picker->firstItem = 0;
#if ENABLE(TIZEN_POPUP_PICKER_WITH_BUTTONS)
    picker->m_genlist_callback_data_list = 0;
    itemClass.item_style = "1line";
    itemClass.func.text_get = __picker_label_get_cb;
    itemClass.func.content_get = __picker_icon_get_cb;
    itemClass.func.state_get = 0;
    itemClass.func.del = 0;

    if (picker->multiSelect) {
        picker->selectedIndex = -1;
        elm_genlist_multi_select_set(picker->popupList, true);
        picker->checkboxMain = elm_check_add(picker->popupList);
        if (!picker->checkboxMain) {
            LOG_ERROR("elm_radio_add failed.");
            return;
        }

        int index = 0;
        Eina_Bool has_problem = EINA_FALSE;
        EINA_LIST_FOREACH(items, l, itemv) {
            genlist_callback_data* data = static_cast<genlist_callback_data*>(malloc(sizeof(genlist_callback_data)));
            if (!data) {
                has_problem = EINA_TRUE;
                break;
            }

            memset(data, 0x00, sizeof(genlist_callback_data));
            data->index = index;
            data->user_data = picker;
            data->menuItem = static_cast<Ewk_Popup_Menu_Item*>(itemv);
            data->menuItem->text = evas_textblock_text_utf8_to_markup(NULL, data->menuItem->text);
            data->it =  elm_genlist_item_append(picker->popupList, &itemClass, data, 0, ELM_GENLIST_ITEM_NONE, __picker_item_selected_cb, data);

            if (data->menuItem->isLabel || !data->menuItem->isEnabled)
                elm_object_item_disabled_set(data->it, EINA_TRUE);

            picker->m_genlist_callback_data_list = eina_list_append(picker->m_genlist_callback_data_list, data);

            if (!index)
                picker->firstItem = data->it;

            // Animatedly brings the first selected item to the visible area of a genlist, if selection type is multiple
            if (data->menuItem->isSelected) {
                if (picker->selectedIndex < 0) {
                    if (index == 0)
                        elm_genlist_item_bring_in(data->it, ELM_GENLIST_ITEM_SCROLLTO_TOP);
                    else
                        elm_genlist_item_bring_in(data->it, ELM_GENLIST_ITEM_SCROLLTO_MIDDLE);
                    picker->selectedIndex = index;
                }
            }

            index++;
        }
        if (has_problem == EINA_TRUE) {
            clear_genlist_callback_data(picker);
            return;
        }
    } else {
        picker->radioMain = elm_radio_add(picker->popupList);
        if (!picker->radioMain) {
            LOG_ERROR("elm_radio_add failed.");
            return;
        }

        elm_radio_state_value_set(picker->radioMain, 10000);
        elm_radio_value_set(picker->radioMain, 0);

        int index = 0;
        Eina_Bool has_problem = EINA_FALSE;
        EINA_LIST_FOREACH(items, l, itemv) {
            genlist_callback_data* data = static_cast<genlist_callback_data*>(malloc(sizeof(genlist_callback_data)));
            if (!data) {
                has_problem = EINA_TRUE;
                break;
            }

            memset(data, 0x00, sizeof(genlist_callback_data));
            data->index = index;
            data->user_data = picker;
            data->menuItem = static_cast<Ewk_Popup_Menu_Item*>(itemv);
            data->menuItem->text = evas_textblock_text_utf8_to_markup(NULL, data->menuItem->text);
            data->it =  elm_genlist_item_append(picker->popupList, &itemClass, data, 0, ELM_GENLIST_ITEM_NONE, __picker_item_selected_cb, data);
            if (data->menuItem->isLabel || !data->menuItem->isEnabled)
                elm_object_item_disabled_set(data->it, EINA_TRUE);

            picker->m_genlist_callback_data_list = eina_list_append(picker->m_genlist_callback_data_list, data);

            if (!index)
                picker->firstItem = data->it;

            if (selectedIndex == index) {
                elm_radio_value_set(picker->radioMain, index);
                if (isPickerExists)
                  elm_genlist_item_bring_in(data->it, ELM_GENLIST_ITEM_SCROLLTO_TOP);
            }

            index++;
        }

        if (has_problem == EINA_TRUE) {
            clear_genlist_callback_data(picker);
            return;
        }
    }
#else
    itemClass.item_style = "1text";
    itemClass.func.text_get = _listLabelGet;
    itemClass.func.content_get = 0;
    itemClass.func.state_get = 0;
    itemClass.func.del = 0;

    int index = 0;
    EINA_LIST_FOREACH(items, l, itemv) {
        Ewk_Popup_Menu_Item* menuItem = static_cast<Ewk_Popup_Menu_Item*>(itemv);

        Elm_Object_Item* itemObject = elm_genlist_item_append(picker->popupList, &itemClass, menuItem, 0, ELM_GENLIST_ITEM_NONE, 0, 0);
        if (!index)
            picker->firstItem = itemObject;
#if ENABLE(TIZEN_MULTIPLE_SELECT)
        if (menuItem->isSelected)
#else
        if (selectedIndex == index)
#endif
            elm_genlist_item_selected_set(itemObject, true);
        if (!menuItem->isEnabled)
            elm_object_item_disabled_set(itemObject, EINA_TRUE);
        index++;
    }

    elm_win_prop_focus_skip_set(picker->win, EINA_TRUE);

    evas_object_smart_callback_add(picker->popupList, "selected", menuItemActivated, picker);
#if ENABLE(TIZEN_MULTIPLE_SELECT)
    if (picker->multiSelect)
        evas_object_smart_callback_add(picker->popupList, "unselected", menuItemDeactivated, picker);
#endif
#endif // ENABLE(TIZEN_POPUP_PICKER_WITH_BUTTONS)

    elm_scroller_policy_set(picker->popupList, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
    evas_object_show(picker->popupList);

    Evas_Object *bgcolor=elm_bg_add(picker->container);
    elm_object_part_content_set(picker->container, "bg", bgcolor);
    elm_object_part_content_set(picker->container, "elm.swallow.content", picker->popupList);

#if ENABLE(TIZEN_FOCUS_UI) || ENABLE(TIZEN_SCREEN_READER)
    Eina_List* pickerChain = 0;

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    pickerChain = eina_list_append(pickerChain, picker->prevButton);
    pickerChain = eina_list_append(pickerChain, picker->nextButton);
#endif
    pickerChain = eina_list_append(pickerChain, picker->doneButton);

    elm_object_focus_custom_chain_set(picker->container, pickerChain);
    elm_object_focus_custom_chain_append(picker->container, picker->popupList, 0);
#endif
}

static void resizeAndShowPicker(Ewk_Popup_Picker* picker)
{
    /* resize picker to window */
    Eina_Rectangle windowRect;
    Ecore_Evas* ee = ecore_evas_ecore_evas_get(evas_object_evas_get(picker->container));
    ecore_evas_geometry_get(ee, &windowRect.x, &windowRect.y, &windowRect.w, &windowRect.h);

    evas_object_resize(picker->container, windowRect.w, windowRect.h);
    evas_object_move(picker->container, windowRect.x, windowRect.y);
    evas_object_show(picker->container);
}

#if ENABLE(TIZEN_MULTIPLE_SELECT)
Ewk_Popup_Picker* ewk_popup_picker_new(Evas_Object* parent, Eina_List* items, int selectedIndex, Eina_Bool multiple)
#else
Ewk_Popup_Picker* ewk_popup_picker_new(Evas_Object* parent, Eina_List* items, int selectedIndex)
#endif
{
    Ewk_Popup_Picker* picker = new Ewk_Popup_Picker;
    picker->parent = parent;

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    picker->multiSelect = multiple;
    picker->changedList = eina_inarray_new(sizeof(int), 0);
#endif

    Evas_Object* topParent = elm_object_top_widget_get(elm_object_parent_widget_get(parent));
    picker->win = elm_win_add(topParent, "combo_box Window", ELM_WIN_DIALOG_BASIC);

    if (!picker->win) {
        ewk_popup_picker_del (picker);
        return 0;
    }

    elm_win_alpha_set(picker->win, EINA_TRUE);
    ecore_x_icccm_name_class_set(elm_win_xwindow_get(picker->win), "APP_POPUP", "APP_POPUP");

    if (elm_win_wm_rotation_supported_get(topParent)) {
        int preferredRotation = elm_win_wm_rotation_preferred_rotation_get(topParent);
        if (preferredRotation == -1) {
            int rots[4] = {0, 90, 180, 270};
            elm_win_wm_rotation_available_rotations_set(picker->win, rots, 4);
        } else
            elm_win_wm_rotation_available_rotations_set(picker->win, &preferredRotation, 1);
    }

    evas_object_resize(picker->win, WebCore::getDefaultScreenResolution().width(), WebCore::getDefaultScreenResolution().height());
    evas_object_move(picker->win, 0, 0);
    evas_object_show(picker->win);

    picker->container = elm_layout_add(picker->win);
    evas_object_size_hint_weight_set(picker->container, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(picker->win, picker->container);

    if (!elm_layout_file_set(picker->container, EDJE_DIR"/control.edj", "elm/picker"))
        LOG_ERROR("Error! : %s, %s\n", EDJE_DIR"/control.edj", "elm/picker");
    else {
        picker->doneButton = elm_button_add(picker->container);
        elm_object_part_content_set(picker->container, "elm.button.done", picker->doneButton);
        elm_object_domain_translatable_part_text_set(picker->doneButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_DONE");
        evas_object_smart_callback_add(picker->doneButton, "clicked", listClosed, picker);
        edje_object_signal_callback_add(elm_layout_edje_get(picker->container),
            "mouse,clicked,1", "dim", dimPressed, picker);
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
        picker->prevButton = elm_button_add(picker->container);
        elm_object_part_content_set(picker->container, "elm.button.prev", picker->prevButton);
        elm_object_domain_translatable_part_text_set(picker->prevButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_PREV_ABB");
        picker->nextButton = elm_button_add(picker->container);
        evas_object_smart_callback_add(picker->prevButton, "clicked", navigateToPrev, picker);

        elm_object_part_content_set(picker->container, "elm.button.next", picker->nextButton);
        elm_object_domain_translatable_part_text_set(picker->nextButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_NEXT_ABB3");
        evas_object_smart_callback_add(picker->nextButton, "clicked", navigateToNext, picker);
#endif
    }

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
        if (EflAssistHandle) {
            void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
            webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
            (*webkit_ea_object_event_callback_add)(picker->container, EA_CALLBACK_BACK, popupMenuHwBackKeyCallback, picker);
        }
#endif

    createAndShowPopupList(picker->win, picker, items, selectedIndex);
    evas_object_propagate_events_set(picker->container, false);
    resizeAndShowPicker(picker);
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    String* prevButtonString = new String("Prev ");
    elm_access_info_cb_set(picker->prevButton, ELM_ACCESS_INFO, __picker_access_info_cb, prevButtonString);
    elm_access_activate_cb_set(picker->prevButton, __picker_access_navigate_to_prev_cb, picker->container);

    String* nextButtonString = new String("Next ");
    elm_access_info_cb_set(picker->nextButton, ELM_ACCESS_INFO, __picker_access_info_cb, nextButtonString);
    elm_access_activate_cb_set(picker->nextButton, __picker_access_navigate_to_next_cb, picker->container);
#endif
#if ENABLE(TIZEN_SCREEN_READER)
    String* doneButtonString = new String(popupPickerDone().utf8().data());
    elm_access_info_cb_set(picker->doneButton, ELM_ACCESS_INFO, __picker_access_info_cb, doneButtonString);
    elm_access_activate_cb_set(picker->doneButton, __picker_access_list_close_cb, picker->container);
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    edje_object_signal_emit(elm_layout_edje_get(picker->container), "show,prev_button,signal", "");
    edje_object_signal_emit(elm_layout_edje_get(picker->container), "show,next_button,signal", "");
#endif

    edje_object_signal_emit(elm_layout_edje_get(picker->container), "show,picker,signal", "");
    evas_object_smart_callback_add(picker->popupList, "realized", __picker_list_realized_cb, picker);
    elm_object_signal_callback_add(picker->container, "show,picker,finished", "elm", showPickerFinishedCallback, picker);

    return picker;
}

void ewk_popup_picker_resize(Ewk_Popup_Picker* picker)
{
    resizeAndShowPicker(picker);
}

void ewk_popup_picker_del(Ewk_Popup_Picker* picker)
{
#if ENABLE(TIZEN_POPUP_PICKER_WITH_BUTTONS)
    evas_object_smart_callback_del(picker->popupList, "changed", __picker_icon_changed_cb);
    clear_genlist_callback_data(picker);
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    evas_object_smart_callback_del(picker->prevButton, "clicked", navigateToPrev);
    evas_object_smart_callback_del(picker->nextButton, "clicked", navigateToNext);
    evas_object_del(picker->prevButton);
    evas_object_del(picker->nextButton);
    picker->prevButton = 0;
    picker->nextButton = 0;
#endif

    evas_object_smart_callback_del(picker->popupList, "selected", menuItemActivated);
    evas_object_smart_callback_del(picker->doneButton, "clicked", listClosed);
    evas_object_del(picker->popupList);
    evas_object_del(picker->doneButton);
    evas_object_del(picker->container);
    evas_object_del(picker->win);
    picker->firstItem = 0;
    picker->popupList = 0;
    picker->doneButton = 0;
    picker->container = 0;
    picker->win = 0;

    delete picker;
}

void ewk_popup_picker_update(Evas_Object* parent, Ewk_Popup_Picker* picker, Eina_List* items, int selectedIndex)
{
    // FIXME: A check should be made if the items are changed instead of directly changing them.
    // Another issue is that if the list is scrolled, the old scroll position is not retained as
    // the list is replaced directly.
    evas_object_del(picker->popupList);

#if ENABLE(TIZEN_MULTIPLE_SELECT)
    picker->changedList = eina_inarray_new(sizeof(int), 0);
#endif

    createAndShowPopupList(picker->win, picker, items, selectedIndex, true);
}

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
void ewk_popup_picker_buttons_update(Ewk_Popup_Picker* picker, int position, int count, bool enable)
{
    Evas_Object* layoutObj = elm_layout_edje_get(picker->container);
    if (enable) {
        if (position == 0) {
            elm_object_disabled_set(picker->prevButton, EINA_FALSE);
            evas_object_smart_callback_add(picker->prevButton, "clicked", navigateToPrev, picker);
        }
        if (position == count - 1) {
            elm_object_disabled_set(picker->nextButton, EINA_FALSE);
            evas_object_smart_callback_add(picker->nextButton, "clicked", navigateToNext, picker);
        }
    }
    else {
        if (position == 0) {
            elm_object_disabled_set(picker->prevButton, EINA_TRUE);
            evas_object_smart_callback_del(picker->prevButton, "clicked", navigateToPrev);
        }
        if (position == count - 1) {
            elm_object_disabled_set(picker->nextButton, EINA_TRUE);
            evas_object_smart_callback_del(picker->nextButton, "clicked", navigateToNext);
        }
    }
}
#endif
#endif
