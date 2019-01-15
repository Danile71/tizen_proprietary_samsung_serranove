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

#ifndef InputPicker_h
#define InputPicker_h

#if PLATFORM(TIZEN) && ENABLE(TIZEN_INPUT_TAG_EXTENSION)
#include <ctime>

#include "ewk_view.h"

#if ENABLE(TIZEN_DATALIST_ELEMENT)
#include "ewk_view_private.h"
#endif

#if ENABLE(TIZEN_INPUT_PICKER_MAX_MIN)
class EwkViewImpl;
#endif
class Input_Picker_Layout {
public:
    Input_Picker_Layout(Evas_Object*);
    ~Input_Picker_Layout();

    Evas_Object* m_ewkView;
    Evas_Object* m_topWidget;
    Evas_Object* m_widgetWin;
    Evas_Object* conformant;
    Evas_Object* popup;
    Evas_Object* layout;
    Evas_Object* datePicker;
    Evas_Object* timePicker;
    Evas_Object* colorRect;
    Evas_Object* okButton;
    Evas_Object* dataListEditField;
    Evas_Object* setButton;
    Evas_Object* cancelButton;
    Ewk_Input_Type inputType;
    bool datetimeLocal;
#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    int initial_r;
    int initial_g;
    int initial_b;
#endif
};

namespace WebKit {

class InputPicker;

class InputPicker {
public:
    InputPicker(Evas_Object*);
    ~InputPicker();

    void show(Ewk_Input_Type, const char*);

#if ENABLE(TIZEN_INPUT_COLOR_PICKER)
    bool showColorPicker(int, int, int, int);
    bool ewk_color_popup(int, int, int);
#endif

    void ewk_datetime_picker(Ewk_Input_Type, const char*);
    void ewk_datetime_popup(const char*, bool);
    void removePickerDelayed();

private:
    bool createWin();
    void createDatetimePopup(struct tm*, bool);
    void createDatetimePicker(struct tm*, Ewk_Input_Type);
    void deletePopupLayout();

#if ENABLE(TIZEN_INPUT_COLOR_PICKER)
    void create_colorselector_layout();
    Evas_Object* create_colorselector(Evas_Object*);

    static void _color_selected_cb(void *, Evas_Object *, void *);
    static void _color_popup_set_btn_clicked_cb(void *, Evas_Object *, void *);
    static void _color_popup_cancel_btn_clicked_cb(void *, Evas_Object *, void *);
#endif

    static void _date_picker_set_button_cliked_cb(void*, Evas_Object*, void*);

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    static void _color_back_cb(void*, Evas_Object*, void*);
    static void _popup_back_cb(void*, Evas_Object*, void*);
    static void _edit_end_cb(void*, Evas_Object*, void*);
#endif
    static Eina_Bool removePicker(void*);
#if ENABLE(TIZEN_INPUT_PICKER_MAX_MIN)
    EwkViewImpl* m_viewImpl;
#endif
    Evas_Object* m_ewkView;
    Input_Picker_Layout* m_pickerLayout;
};

} // namespace WebKit

#endif // PLATFORM(TIZEN) && ENABLE(TIZEN_INPUT_TAG_EXTENSION)

#endif // InputPicker_h
