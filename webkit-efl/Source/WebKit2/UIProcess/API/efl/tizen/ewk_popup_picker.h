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

#ifndef ewk_popup_picker_h
#define ewk_popup_picker_h

typedef Eo Elm_Object_Item;
#if ENABLE(TIZEN_POPUP_PICKER_WITH_BUTTONS)
typedef struct Ewk_Popup_Menu_Item Ewk_Popup_Menu_Item;
#endif
#if ENABLE(TIZEN_MULTIPLE_SELECT)
typedef struct _Eina_Inarray Eina_Inarray;
#endif

namespace WebKit {
class WebPopupMenuProxyEfl;
}

struct _Ewk_Popup_Picker {
    Evas_Object* container;
    Evas_Object* parent;
    Evas_Object* popupList;
    Evas_Object* win;
#if ENABLE(TIZEN_POPUP_PICKER_WITH_BUTTONS)
    Evas_Object* radioMain;
    Evas_Object* checkboxMain;
    Eina_List* m_genlist_callback_data_list;
    Evas_Object* doneButton;
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
    Evas_Object* prevButton;
    Evas_Object* nextButton;
#endif
    Elm_Object_Item* firstItem;
    int selectedIndex;  // first selected option if selection type is multiple
#if ENABLE(TIZEN_MULTIPLE_SELECT)
    Eina_Bool multiSelect;
    Eina_Inarray* changedList;
#endif
};
typedef struct _Ewk_Popup_Picker Ewk_Popup_Picker;

#if ENABLE(TIZEN_POPUP_PICKER_WITH_BUTTONS)
typedef struct _genlist_callback_data {
    int index;
    void *user_data;
    Elm_Object_Item *it;
    Ewk_Popup_Menu_Item* menuItem;
} genlist_callback_data;
#endif

#if ENABLE(TIZEN_MULTIPLE_SELECT)
Ewk_Popup_Picker* ewk_popup_picker_new(Evas_Object* parent, Eina_List* items, int selectedIndex, Eina_Bool multiple);
#else
Ewk_Popup_Picker* ewk_popup_picker_new(Evas_Object* parent, Eina_List* items, int selectedIndex);
#endif
void ewk_popup_picker_resize(Ewk_Popup_Picker* picker);
void ewk_popup_picker_del(Ewk_Popup_Picker* picker);

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
void ewk_popup_picker_update(Evas_Object* parent, Ewk_Popup_Picker* picker, Eina_List* items, int selectedIndex);
void listClosed(void* data, Evas_Object* obj, void* event_info);
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
void ewk_popup_picker_buttons_update(Ewk_Popup_Picker* picker, int position, int count, bool enable);
#endif

#endif // ewk_popup_picker_h
