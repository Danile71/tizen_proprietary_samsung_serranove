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
#include "InputPicker.h"

#include "ewk_view.h"
#include "LocalizedStrings.h"

#if ENABLE(TIZEN_INPUT_PICKER_MAX_MIN)
#include "WKString.h"
#endif

#include <Elementary.h>
#include <stdlib.h>
#include <string.h>
#include <vconf/vconf.h>

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
#include <dlfcn.h>
#include <efl_assist.h>
extern void* EflAssistHandle;
#endif

#if PLATFORM(TIZEN) && ENABLE(TIZEN_INPUT_TAG_EXTENSION)

static const unsigned maxDatetimeLength = 32;
static const unsigned maxDateStringLength = 10;
static const char defaultDatetimeFormat[] = "%Y/%m/%d %H:%M";

struct Input_Date {
    int year;
    int mon;
    int day;
    int hour;
    int min;
    int sec;
};

struct Input_Date_Str {
    char year[maxDateStringLength];
    char mon[maxDateStringLength];
    char day[maxDateStringLength];
    char hour[maxDateStringLength];
    char min[maxDateStringLength];
    char sec[maxDateStringLength];
};

static char* datetimeFormat()
{
    char* datetimeFormat = NULL;
    char* regionFormat = NULL;
    char* language = NULL;
    char buf[256] = { 0, };
    int timeValue = 0;
    int isHour24 = 0;
    int returnValue = 0;

    language = getenv("LANGUAGE");
    setenv("LANGUAGE", "en_US", 1);

    regionFormat = vconf_get_str(VCONFKEY_REGIONFORMAT);
    if (!regionFormat)
        return 0;

    returnValue = vconf_get_int(VCONFKEY_REGIONFORMAT_TIME1224, &timeValue);
    if (returnValue < 0)
        isHour24 = 0;
    else if(timeValue == VCONFKEY_TIME_FORMAT_12 || timeValue == VCONFKEY_TIME_FORMAT_24)
        isHour24 = timeValue - 1;

    if (isHour24)
        snprintf(buf, sizeof(buf), "%s_DTFMT_24HR", regionFormat);
    else
        snprintf(buf, sizeof(buf), "%s_DTFMT_12HR", regionFormat);

    free(regionFormat);

    // FIXME: Workaround fix for region format.
    int bufLength = strlen(buf);
    for (int i = 0; i < bufLength - 4; i++) {
        if (buf[i] == 'u' && buf[i + 1] == 't' && buf[i + 2] == 'f') {
            if (buf[i + 3] == '8') {
                // utf8 -> UTF-8
                TIZEN_LOGI("Replace utf8 with UTF-8");
                for (int j = bufLength; j > i + 3; j--)
                    buf[j] = buf[j - 1];
                buf[i + 3] = '-';
                buf[bufLength + 1] = '\0';
            } else if (buf[i + 3] == '-' && buf[i + 4] == '8') {
                // utf-8 -> UTF-8
                TIZEN_LOGI("Replace utf-8 with UTF-8");
            } else {
                TIZEN_LOGI("datetime Format: %s", buf);
                break;
            }

            buf[i] = 'U';
            buf[i + 1] = 'T';
            buf[i + 2] = 'F';
            break;
        }
    }

    datetimeFormat = dgettext("dt_fmt", buf);

    if(!language || !strcmp(language, ""))
        unsetenv("LANGUAGE");
    else
        setenv("LANGUAGE", language, 1);

    // FIXME: Workaround fix for not supported dt_fmt. Use default format if dt_fmt string is not exist.
    if (strlen(datetimeFormat) == strlen(buf) && !strncmp(datetimeFormat, buf, strlen(buf))) {
        TIZEN_LOGI("datetime format: %s", datetimeFormat);
        return 0;
    }

    return strdup(datetimeFormat);
}

Input_Picker_Layout::Input_Picker_Layout(Evas_Object* ewkView)
    : m_ewkView(ewkView)
    , m_topWidget(0)
    , m_widgetWin(0)
    , conformant(0)
    , popup(0)
    , layout(0)
    , datePicker(0)
    , timePicker(0)
    , colorRect(0)
    , okButton(0)
    , dataListEditField(0)
    , setButton(0)
    , cancelButton(0)
    , inputType(EWK_INPUT_TYPE_DATE)
    , datetimeLocal(false)
#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    , initial_r(0)
    , initial_g(0)
    , initial_b(0)
#endif
{
    evas_object_focus_set(m_ewkView, false);

    /* FIXME : Workaround. OSP requirement.
       OSP want to block own touch event while webkit internal picker is running. */
    evas_object_smart_callback_call(m_ewkView, "input,picker,show", 0);
}

Input_Picker_Layout::~Input_Picker_Layout()
{
    /* FIXME : Workaround. OSP requirement.
       OSP want to block own touch event while webkit internal picker is running. */
    evas_object_smart_callback_call(m_ewkView, "input,picker,hide", 0);
}

namespace WebKit {

InputPicker::InputPicker(Evas_Object* ewkView)
    : m_ewkView(ewkView)
    , m_pickerLayout(0)
{
}

InputPicker::~InputPicker()
{
    deletePopupLayout();
}

void InputPicker::show(Ewk_Input_Type inputType, const char* inputValue)
{
    TIZEN_SECURE_LOGI("input value: %s", inputValue);

    ewk_view_command_execute(m_ewkView, "Unselect", 0);
    switch (inputType) {
        case EWK_INPUT_TYPE_DATETIME:
            ewk_datetime_popup(inputValue, false);
            break;
        case EWK_INPUT_TYPE_DATETIMELOCAL:
            ewk_datetime_popup(inputValue, true);
            break;
        case EWK_INPUT_TYPE_DATE:
        case EWK_INPUT_TYPE_MONTH:
        case EWK_INPUT_TYPE_TIME:
        case EWK_INPUT_TYPE_WEEK:
            ewk_datetime_picker(inputType, inputValue);
            break;
        default:
            if (!m_pickerLayout)
                return;
            deletePopupLayout();
    };
}

bool InputPicker::createWin()
{
    m_pickerLayout->m_topWidget = elm_object_top_widget_get(elm_object_parent_widget_get(m_ewkView));
    if (!m_pickerLayout->m_topWidget)
        return false;

    m_pickerLayout->m_widgetWin = elm_win_add(m_pickerLayout->m_topWidget, "WebKit InputPicker Popup", ELM_WIN_DIALOG_BASIC);
    if (!m_pickerLayout->m_widgetWin)
        return false;

    elm_win_alpha_set(m_pickerLayout->m_widgetWin, EINA_TRUE);
    ecore_x_icccm_name_class_set(elm_win_xwindow_get(m_pickerLayout->m_widgetWin), "APP_POPUP", "APP_POPUP");

    if (elm_win_wm_rotation_supported_get(m_pickerLayout->m_topWidget)) {
        int preferredRotation = elm_win_wm_rotation_preferred_rotation_get(m_pickerLayout->m_topWidget);
        if (preferredRotation == -1) {
            int rots[4] = {0, 90, 180, 270};
            elm_win_wm_rotation_available_rotations_set(m_pickerLayout->m_widgetWin, rots, 4);
        } else
            elm_win_wm_rotation_available_rotations_set(m_pickerLayout->m_widgetWin, &preferredRotation, 1);
    }

    m_pickerLayout->conformant = elm_conformant_add(m_pickerLayout->m_widgetWin);
    if (!m_pickerLayout->conformant)
        return 0;

    evas_object_size_hint_weight_set(m_pickerLayout->conformant, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_win_resize_object_add(m_pickerLayout->m_widgetWin, m_pickerLayout->conformant);
    evas_object_show(m_pickerLayout->conformant);

    m_pickerLayout->layout = elm_layout_add(m_pickerLayout->conformant);
    if (!m_pickerLayout->layout)
        return 0;

    elm_layout_theme_set(m_pickerLayout->layout, "layout", "application", "default");
    evas_object_size_hint_weight_set(m_pickerLayout->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(m_pickerLayout->layout);

    elm_object_content_set(m_pickerLayout->conformant, m_pickerLayout->layout);
    elm_win_conformant_set(m_pickerLayout->m_widgetWin, EINA_TRUE);

    evas_object_resize(m_pickerLayout->m_widgetWin, WebCore::getDefaultScreenResolution().width(), WebCore::getDefaultScreenResolution().height());
    evas_object_show(m_pickerLayout->m_widgetWin);
    // Workaround. Platform issue, new created window is not shown.
    elm_win_render(m_pickerLayout->m_widgetWin);

    return true;
}

#if ENABLE(TIZEN_INPUT_COLOR_PICKER)
bool InputPicker::showColorPicker(int r, int g, int b, int)
{
    return ewk_color_popup(r, g, b);
}

void InputPicker::_color_selected_cb(void *data, Evas_Object *obj, void *event_info)
{
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;

    Elm_Object_Item *color_it = (Elm_Object_Item *) event_info;
    elm_colorselector_palette_item_color_get(color_it, &r, &g, &b, &a);
    evas_object_color_set((Evas_Object*)data, r, g, b, a);
}

void InputPicker::_color_popup_set_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
    InputPicker* inputPicker = static_cast<InputPicker*>(data);
    int r = 0, g = 0, b = 0 ,a = 0;

    evas_object_color_get(inputPicker->m_pickerLayout->colorRect, &r, &g, &b, &a);
    ewk_view_color_picker_color_set(inputPicker->m_ewkView, r, g, b, a);

    inputPicker->removePickerDelayed();
}

void InputPicker::_color_popup_cancel_btn_clicked_cb(void *data, Evas_Object *obj, void *event_info)
{
    InputPicker* inputPicker = static_cast<InputPicker*>(data);

    int r = inputPicker->m_pickerLayout->initial_r;
    int g = inputPicker->m_pickerLayout->initial_g;
    int b = inputPicker->m_pickerLayout->initial_b;
    int a = 255;
    ewk_view_color_picker_color_set(inputPicker->m_ewkView, r, g, b, a);

    inputPicker->removePickerDelayed();
}

Evas_Object* InputPicker::create_colorselector(Evas_Object *parent)
{
    /* add color palette widget */
    Evas_Object *colorselector;
    Elm_Object_Item *it = NULL;
    Eina_List *color_list;

    colorselector = elm_colorselector_add(parent);
    elm_colorselector_mode_set(colorselector, ELM_COLORSELECTOR_PALETTE);
    evas_object_size_hint_fill_set(colorselector, EVAS_HINT_FILL, EVAS_HINT_FILL);
    evas_object_size_hint_weight_set(colorselector, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    color_list = const_cast<Eina_List*>(elm_colorselector_palette_items_get(colorselector));
    Eina_List* list = 0;
    void* item = 0;
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;

    EINA_LIST_FOREACH(color_list, list, item) {
        if (item) {
            elm_colorselector_palette_item_color_get((Elm_Object_Item*)item, &r, &g, &b, &a);
            if (r == m_pickerLayout->initial_r && g == m_pickerLayout->initial_g && b == m_pickerLayout->initial_b) {
                it = (Elm_Object_Item*)item;
                break;
            }
        }
    }

    if (!it)
        it = (Elm_Object_Item*)eina_list_nth((Eina_List*)color_list, 0);

    elm_object_item_signal_emit((Elm_Object_Item*)it, "elm,state,selected", "elm");

    return colorselector;
}

void InputPicker::create_colorselector_layout()
{
    Evas_Object* colorselector, *layout;

    layout = elm_layout_add(m_pickerLayout->popup);

    elm_layout_file_set(layout, EDJE_DIR"/control.edj", "colorselector_popup_layout");
    evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

    m_pickerLayout->colorRect = evas_object_rectangle_add(evas_object_evas_get(layout));
    evas_object_size_hint_weight_set(m_pickerLayout->colorRect, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_show(m_pickerLayout->colorRect);
    evas_object_color_set(m_pickerLayout->colorRect, m_pickerLayout->initial_r, m_pickerLayout->initial_g, m_pickerLayout->initial_b, 255);
    elm_object_part_content_set(m_pickerLayout->layout, "rect", m_pickerLayout->colorRect);

    colorselector = create_colorselector(layout);
    elm_object_part_content_set(layout, "colorpalette", colorselector);
    evas_object_smart_callback_add(colorselector, "color,item,selected", _color_selected_cb, m_pickerLayout->colorRect);
    elm_object_content_set(m_pickerLayout->popup, layout);

    evas_object_show(layout);
}

bool InputPicker::ewk_color_popup(int r, int g, int b)
{
    if (m_pickerLayout)
        return false;

    m_pickerLayout = new Input_Picker_Layout(m_ewkView);
    if (!createWin())
        return false;

    m_pickerLayout->popup = elm_popup_add(m_pickerLayout->m_widgetWin);
    elm_popup_align_set(m_pickerLayout->popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
    elm_object_domain_translatable_part_text_set(m_pickerLayout->popup, "title,text", "WebKit", "IDS_WEBVIEW_HEADER_SELECT_COLOUR");
    evas_object_size_hint_weight_set(m_pickerLayout->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    m_pickerLayout->initial_r = r;
    m_pickerLayout->initial_g = g;
    m_pickerLayout->initial_b = b;
    if (EflAssistHandle) {
        void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
        webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
        (*webkit_ea_object_event_callback_add)(m_pickerLayout->popup, EA_CALLBACK_BACK, _color_back_cb, this);
    }
#endif

    create_colorselector_layout();

    m_pickerLayout->cancelButton = elm_button_add(m_pickerLayout->popup);
    elm_object_style_set(m_pickerLayout->cancelButton, "popup");
    elm_object_domain_translatable_part_text_set(m_pickerLayout->cancelButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_CANCEL_ABB4");
    elm_object_part_content_set(m_pickerLayout->popup, "button1", m_pickerLayout->cancelButton);
    evas_object_smart_callback_add(m_pickerLayout->cancelButton, "clicked", _color_popup_cancel_btn_clicked_cb, this);

    m_pickerLayout->setButton = elm_button_add(m_pickerLayout->popup);
    elm_object_style_set(m_pickerLayout->setButton, "popup");
    elm_object_domain_translatable_part_text_set(m_pickerLayout->setButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_SET_ABB2");
    elm_object_part_content_set(m_pickerLayout->popup, "button2", m_pickerLayout->setButton);
    evas_object_smart_callback_add(m_pickerLayout->setButton, "clicked", _color_popup_set_btn_clicked_cb, this);

    evas_object_show(m_pickerLayout->popup);

    return true;
}
#endif

#define isLeapYear(year) ((!(year % 4) && (year % 100)) || (!(year % 400) && (year % 1000)))
static int _calculateMonthAndDay(int year, int totalDays, int* remains)
{
    // Jan.
    if (totalDays - 31 < 0) {
    	*remains = totalDays;
        return 0;
    }

    totalDays = totalDays - 31;

    // Feb.
    if (isLeapYear(year)) {
        if (totalDays - 29 < 0) {
            *remains = totalDays;
            return 1;
        }
        totalDays = totalDays - 29;
    } else {
        if (totalDays - 28 < 0) {
            *remains = totalDays;
            return 1;
        }
        totalDays = totalDays - 28;
    }

    // Mar.
    if (totalDays - 31 < 0) {
        *remains = totalDays;
        return 2;
    }
    totalDays = totalDays - 31;

    // Apr.
    if (totalDays - 30 < 0) {
        *remains = totalDays;
        return 3;
    }
    totalDays = totalDays - 30;

    // May
    if (totalDays - 31 < 0) {
        *remains = totalDays;
        return 4;
    }
    totalDays = totalDays - 31;

    // Jun.
    if (totalDays - 30 < 0) {
        *remains = totalDays;
        return 5;
    }
    totalDays = totalDays - 30;

    // Jul.
    if (totalDays - 31 < 0) {
        *remains = totalDays;
        return 6;
    }
    totalDays = totalDays - 31;

    // Aug.
    if (totalDays - 31 < 0) {
        *remains = totalDays;
        return 7;
    }
    totalDays = totalDays - 31;

    // Sept.
    if (totalDays - 30 < 0) {
        *remains = totalDays;
        return 8;
    }
    totalDays = totalDays - 30;

    // Oct.
    if (totalDays - 31 < 0) {
        *remains = totalDays;
        return 9;
    }
    totalDays = totalDays - 31;

    // Nov.
    if (totalDays - 30 < 0) {
        *remains = totalDays;
        return 10;
    }
    totalDays = totalDays - 30;

    *remains = totalDays;
    return 11;
}

tm* convertWeekString(const char* value)
{
    struct tm* convertedTime;
    time_t cur_time;
    time(&cur_time);
    convertedTime = localtime(&cur_time);

    Input_Date_Str dateStr;
    memset(&dateStr, 0, sizeof(Input_Date_Str));

    char tmpinputValue[maxDatetimeLength] = { 0, };

    snprintf(tmpinputValue, maxDatetimeLength, "%s", value);
    char* token = strtok(tmpinputValue,"-");
    if (token)
        strncpy(dateStr.year, token, maxDateStringLength);
    const char* week = strstr(value, "W");
    int weekNum = 1;
    if (week)
        weekNum = atoi(week + 1);

    if (dateStr.year)
        convertedTime->tm_year = atoi(dateStr.year);
    convertedTime->tm_year = convertedTime->tm_year - 1900;

    struct tm firtTimeOfyear;
    memset(&firtTimeOfyear, 0, sizeof(struct tm));
    firtTimeOfyear.tm_year = convertedTime->tm_year;
    firtTimeOfyear.tm_mon = 0;
    firtTimeOfyear.tm_mday = 1;
    mktime(&firtTimeOfyear);

    char firstWeek[10] = {0, };
    strftime(firstWeek, 10, "%w", &firtTimeOfyear);
    int firstWeekCount = atoi(firstWeek);

    int totalDays = 1;

    totalDays = weekNum * 7 - firstWeekCount;

    int days = 0;
    int month = _calculateMonthAndDay(convertedTime->tm_year, totalDays, &days);

    convertedTime->tm_mon = month;
    convertedTime->tm_mday = days;

    return convertedTime;
}

tm* convertDateString(const char* value)
{
    struct tm* convertedTime;
    time_t cur_time;
    time(&cur_time);
    convertedTime = localtime(&cur_time);

    Input_Date_Str dateStr;
    memset(&dateStr, 0, sizeof(Input_Date_Str));

    char tmpinputValue[maxDatetimeLength] = { 0, };

    snprintf(tmpinputValue, maxDatetimeLength, "%s", value);
    char* token = strtok(tmpinputValue,"-");
    if (token)
        strncpy(dateStr.year, token, maxDateStringLength);
    token = strtok(0, "-");
    if (token)
        strncpy(dateStr.mon, token, maxDateStringLength);
    token = strtok(0, "-");
    if (token)
        strncpy(dateStr.day, token, maxDateStringLength);

    if (dateStr.year)
        convertedTime->tm_year = atoi(dateStr.year);
    if (dateStr.mon)
        convertedTime->tm_mon = atoi(dateStr.mon);
    if (dateStr.day)
        convertedTime->tm_mday = atoi(dateStr.day);

    convertedTime->tm_year = convertedTime->tm_year - 1900;
    convertedTime->tm_mon = convertedTime->tm_mon - 1;

    return convertedTime;
}

tm* convertTimeString(const char* value)
{
    struct tm* convertedTime;
    time_t cur_time;
    time(&cur_time);
    convertedTime = localtime(&cur_time);

    Input_Date_Str dateStr;
    memset(&dateStr, 0, sizeof(Input_Date_Str));

    char tmpinputValue[maxDatetimeLength] = { 0, };

    snprintf(tmpinputValue, maxDatetimeLength, "%s", value);
    char* token = strtok(tmpinputValue,":");
    if (token)
        strncpy(dateStr.hour, token, maxDateStringLength);
    token = strtok(0, ":");
    if (token)
        strncpy(dateStr.min, token, maxDateStringLength);

    if (dateStr.hour)
        convertedTime->tm_hour = atoi(dateStr.hour);
    if (dateStr.min)
        convertedTime->tm_min = atoi(dateStr.min);

    return convertedTime;
}

tm* convertMonthString(const char* value)
{
    struct tm* convertedTime;
    time_t cur_time;
    time(&cur_time);
    convertedTime = localtime(&cur_time);

    Input_Date_Str dateStr;
    memset(&dateStr, 0, sizeof(Input_Date_Str));

    char tmpInputValue[maxDatetimeLength] = { 0, };

    snprintf(tmpInputValue, maxDatetimeLength, "%s", value);
    char* token = strtok(tmpInputValue,"-");
    if (token)
        strncpy(dateStr.year, token, maxDateStringLength);
    token = strtok(0, "-");
    if (token)
        strncpy(dateStr.mon, token, maxDateStringLength);

    if (dateStr.year)
        convertedTime->tm_year = atoi(dateStr.year);
    if (dateStr.mon)
        convertedTime->tm_mon = atoi(dateStr.mon);

    convertedTime->tm_year = convertedTime->tm_year - 1900;
    convertedTime->tm_mon = convertedTime->tm_mon - 1;

    return convertedTime;
}

tm* convertDateTimeString(const char* value, bool local)
{
    struct tm* convertedTime;
    time_t  cur_time;
    time(&cur_time);
    convertedTime = localtime(&cur_time);

    Input_Date_Str dateStr;
    memset(&dateStr, 0, sizeof(Input_Date_Str));

    char tmpInputValue[maxDatetimeLength] = { 0, };

    snprintf(tmpInputValue, maxDatetimeLength, "%s", value);
    char* token = strtok(tmpInputValue,"-");
    if (token)
        strncpy(dateStr.year, token, maxDateStringLength);
    token = strtok(0, "-");
    if (token)
        strncpy(dateStr.mon, token, maxDateStringLength);
    token = strtok(0, "T");
    if (token)
        strncpy(dateStr.day, token, maxDateStringLength);
    token = strtok(0, ":");
    if (token)
        strncpy(dateStr.hour, token, maxDateStringLength);

    if (local) {
        token = strtok(0, "Z");
        if (token)
            strncpy(dateStr.min, token, maxDateStringLength);
    } else {
        token = strtok(0, ":");
        if (token)
            strncpy(dateStr.min, token, maxDateStringLength);
    }

    if (dateStr.year)
        convertedTime->tm_year = atoi(dateStr.year);
    if (dateStr.mon)
        convertedTime->tm_mon = atoi(dateStr.mon);
    if (dateStr.day)
        convertedTime->tm_mday = atoi(dateStr.day);
    if (dateStr.hour)
        convertedTime->tm_hour = atoi(dateStr.hour);
    if (dateStr.min)
        convertedTime->tm_min = atoi(dateStr.min);

    convertedTime->tm_year = convertedTime->tm_year - 1900;
    convertedTime->tm_mon = convertedTime->tm_mon - 1;

    return convertedTime;
}

void InputPicker::ewk_datetime_picker(Ewk_Input_Type inputType, const char* inputValue)
{
    struct tm* currentTime;
    time_t cur_time;
    time(&cur_time);
    currentTime = localtime(&cur_time);

    if (inputValue && strlen(inputValue)) {
        if(inputType == EWK_INPUT_TYPE_DATE)
            currentTime = convertDateString(inputValue);
        else if(inputType == EWK_INPUT_TYPE_TIME)
            currentTime = convertTimeString(inputValue);
        else if(inputType == EWK_INPUT_TYPE_MONTH)
            currentTime = convertMonthString(inputValue);
        else if(inputType == EWK_INPUT_TYPE_WEEK)
            currentTime = convertWeekString(inputValue);
    }

    if (m_pickerLayout) {
        // Just update the value.
        elm_datetime_value_set(m_pickerLayout->datePicker, currentTime);
        elm_object_focus_set(m_pickerLayout->popup, true);
        return;
    }

    m_pickerLayout = new Input_Picker_Layout(m_ewkView);

    m_pickerLayout->inputType = inputType;
    createDatetimePicker(currentTime, inputType);

    evas_object_smart_callback_add(m_pickerLayout->cancelButton, "clicked", _edit_end_cb, this);
    evas_object_smart_callback_add(m_pickerLayout->setButton, "clicked", _date_picker_set_button_cliked_cb, this);
    evas_object_size_hint_align_set(m_pickerLayout->datePicker, 0.5 , 0.5);
    Evas_Object *box = elm_box_add(m_pickerLayout->popup);
    evas_object_show(m_pickerLayout->datePicker);
    elm_box_pack_end(box, m_pickerLayout->datePicker);
    elm_object_content_set(m_pickerLayout->popup, box);
    evas_object_show(m_pickerLayout->popup);
}

void InputPicker::ewk_datetime_popup(const char* inputValue, bool local)
{
    struct tm* currentTime;
    time_t  cur_time;
    time(&cur_time);
    currentTime = localtime(&cur_time);

    if (inputValue && strlen(inputValue))
        currentTime = convertDateTimeString(inputValue, local);

    if (m_pickerLayout) {
        // Just update the value.
        m_pickerLayout->datetimeLocal = local;

        elm_datetime_value_set(m_pickerLayout->datePicker, currentTime);
        evas_object_focus_set(m_pickerLayout->setButton, true);
        return;
    }

    m_pickerLayout = new Input_Picker_Layout(m_ewkView);

    if (local)
        m_pickerLayout->inputType = EWK_INPUT_TYPE_DATETIMELOCAL;
    else
        m_pickerLayout->inputType = EWK_INPUT_TYPE_DATETIME;

    m_pickerLayout->datetimeLocal = local;

    createDatetimePopup(currentTime, local);

    evas_object_smart_callback_add(m_pickerLayout->cancelButton, "clicked", _edit_end_cb, this);
    evas_object_smart_callback_add(m_pickerLayout->setButton, "clicked", _date_picker_set_button_cliked_cb, this);

    evas_object_show(m_pickerLayout->popup);

}

Eina_Bool InputPicker::removePicker(void* data)
{
    InputPicker* inputPicker = static_cast<InputPicker*>(data);

    if (!inputPicker->m_pickerLayout)
        return ECORE_CALLBACK_CANCEL;

    ewk_view_command_execute(inputPicker->m_ewkView, "Unselect", 0);

    inputPicker->deletePopupLayout();

    return ECORE_CALLBACK_CANCEL;
}

void InputPicker::removePickerDelayed()
{
    ecore_timer_add(0.1, removePicker, this);
}

void InputPicker::createDatetimePicker(struct tm* currentTime, Ewk_Input_Type inputType)
{
    if (!createWin())
        return;

    m_pickerLayout->popup = elm_popup_add(m_pickerLayout->layout);
    elm_popup_align_set(m_pickerLayout->popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
    evas_object_size_hint_weight_set(m_pickerLayout->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_size_hint_align_set(m_pickerLayout->popup, EVAS_HINT_FILL, 0.5);

    if (inputType == EWK_INPUT_TYPE_TIME)
      elm_object_domain_translatable_part_text_set(m_pickerLayout->popup, "title,text", "WebKit", "IDS_WEBVIEW_HEADER_SET_TIME");
    else
      elm_object_domain_translatable_part_text_set(m_pickerLayout->popup, "title,text", "WebKit", "IDS_WEBVIEW_HEADER_SET_DATE");

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    if (EflAssistHandle) {
        void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
        webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
        (*webkit_ea_object_event_callback_add)(m_pickerLayout->popup, EA_CALLBACK_BACK, _popup_back_cb, this);
    }
#endif

    m_pickerLayout->datePicker= elm_datetime_add(m_pickerLayout->popup);

    if (inputType == EWK_INPUT_TYPE_TIME)
        elm_object_style_set(m_pickerLayout->datePicker, "time_layout");
    else
        elm_object_style_set(m_pickerLayout->datePicker, "date_layout");

    char* format = datetimeFormat();
    if (format) {
        elm_datetime_format_set(m_pickerLayout->datePicker, format);
        free(format);
    } else
        elm_datetime_format_set(m_pickerLayout->datePicker, defaultDatetimeFormat);

    elm_datetime_value_set(m_pickerLayout->datePicker, currentTime);

#if ENABLE(TIZEN_INPUT_PICKER_MAX_MIN)
    struct tm* minTime = 0;
    struct tm* maxTime = 0;

    m_viewImpl = ewkViewGetPageClient(m_ewkView)->viewImpl();

    if (m_viewImpl->page()->editorState().minDate.utf8().data() && strlen(m_viewImpl->page()->editorState().minDate.utf8().data())) {
        if (inputType == EWK_INPUT_TYPE_DATE)
            minTime = convertDateString(m_viewImpl->page()->editorState().minDate.utf8().data());
        else if (inputType == EWK_INPUT_TYPE_MONTH)
            minTime = convertMonthString(m_viewImpl->page()->editorState().minDate.utf8().data());
        else if (inputType == EWK_INPUT_TYPE_WEEK)
            minTime = convertWeekString(m_viewImpl->page()->editorState().minDate.utf8().data());
        else if (inputType == EWK_INPUT_TYPE_TIME) {
            minTime = convertTimeString(m_viewImpl->page()->editorState().minDate.utf8().data());
        }
        if (minTime)
            elm_datetime_value_min_set(m_pickerLayout->datePicker, minTime);
    }

    if (m_viewImpl->page()->editorState().maxDate.utf8().data() && strlen(m_viewImpl->page()->editorState().maxDate.utf8().data())) {
        if (inputType == EWK_INPUT_TYPE_DATE)
            maxTime = convertDateString(m_viewImpl->page()->editorState().maxDate.utf8().data());
        else if (inputType == EWK_INPUT_TYPE_MONTH)
            maxTime = convertMonthString(m_viewImpl->page()->editorState().maxDate.utf8().data());
        else if (inputType == EWK_INPUT_TYPE_WEEK)
            maxTime = convertWeekString(m_viewImpl->page()->editorState().maxDate.utf8().data());
        else if (inputType == EWK_INPUT_TYPE_TIME) {
            maxTime = convertTimeString(m_viewImpl->page()->editorState().maxDate.utf8().data());
        }
        if (maxTime)
            elm_datetime_value_max_set(m_pickerLayout->datePicker, maxTime);
    }
#endif

    m_pickerLayout->cancelButton = elm_button_add(m_pickerLayout->popup);
    elm_object_style_set(m_pickerLayout->cancelButton, "popup");
    elm_object_domain_translatable_part_text_set(m_pickerLayout->cancelButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_CANCEL_ABB4");
    elm_object_part_content_set(m_pickerLayout->popup, "button1", m_pickerLayout->cancelButton);

    m_pickerLayout->setButton = elm_button_add(m_pickerLayout->popup);
    elm_object_style_set(m_pickerLayout->setButton, "popup");
    elm_object_domain_translatable_part_text_set(m_pickerLayout->setButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_SET_ABB2");
    elm_object_part_content_set(m_pickerLayout->popup, "button2", m_pickerLayout->setButton);

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    evas_object_smart_callback_add(m_pickerLayout->popup, "edit,end", _edit_end_cb, this);
#endif
}

void InputPicker::createDatetimePopup(struct tm* currentTime, bool local)
{
    if (!createWin())
        return;

    m_pickerLayout->popup = elm_popup_add(m_pickerLayout->layout);
    elm_popup_align_set(m_pickerLayout->popup, ELM_NOTIFY_ALIGN_FILL, 1.0);
    evas_object_size_hint_weight_set(m_pickerLayout->popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    elm_object_domain_translatable_part_text_set(m_pickerLayout->popup, "title,text", "WebKit", "IDS_WEBVIEW_HEADER_SET_DATE_AND_TIME");

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    if (EflAssistHandle) {
        void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
        webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
        (*webkit_ea_object_event_callback_add)(m_pickerLayout->popup, EA_CALLBACK_BACK, _popup_back_cb, this);
    }
#endif

    Evas_Object* box = elm_box_add(m_pickerLayout->popup);
    elm_box_padding_set(box, 0, 10);

    m_pickerLayout->datePicker = elm_datetime_add(m_pickerLayout->popup);
    m_pickerLayout->timePicker = elm_datetime_add(m_pickerLayout->popup);

    elm_object_style_set(m_pickerLayout->datePicker, "date_layout");
    elm_object_style_set(m_pickerLayout->timePicker, "time_layout");

    char* format = datetimeFormat();

    elm_datetime_value_set(m_pickerLayout->datePicker, currentTime);
    elm_datetime_value_set(m_pickerLayout->timePicker, currentTime);

    if (format) {
        elm_datetime_format_set(m_pickerLayout->datePicker, format);
        elm_datetime_format_set(m_pickerLayout->timePicker, format);
        free(format);
    } else {
        elm_datetime_format_set(m_pickerLayout->datePicker, defaultDatetimeFormat);
        elm_datetime_format_set(m_pickerLayout->timePicker, defaultDatetimeFormat);
    }

    evas_object_size_hint_align_set(m_pickerLayout->datePicker, 0.5 , 0.5);
    evas_object_size_hint_align_set(m_pickerLayout->timePicker, 0.5 , 0.5);

#if ENABLE(TIZEN_INPUT_PICKER_MAX_MIN)
    struct tm* minTime;
    struct tm* maxTime;

    m_viewImpl = ewkViewGetPageClient(m_ewkView)->viewImpl();

    if (m_viewImpl->page()->editorState().minDate.utf8().data() && strlen(m_viewImpl->page()->editorState().minDate.utf8().data())) {
        minTime = convertDateTimeString(m_viewImpl->page()->editorState().minDate.utf8().data(), local);

        elm_datetime_value_min_set(m_pickerLayout->datePicker, minTime);
    }
    if (m_viewImpl->page()->editorState().maxDate.utf8().data() && strlen(m_viewImpl->page()->editorState().maxDate.utf8().data())) {
        maxTime = convertDateTimeString(m_viewImpl->page()->editorState().maxDate.utf8().data(), local);

        elm_datetime_value_max_set(m_pickerLayout->datePicker, maxTime);
    }
#endif

    m_pickerLayout->cancelButton = elm_button_add(m_pickerLayout->popup);
    elm_object_style_set(m_pickerLayout->cancelButton, "popup");
    elm_object_domain_translatable_part_text_set(m_pickerLayout->cancelButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_CANCEL_ABB4");
    elm_object_part_content_set(m_pickerLayout->popup, "button1", m_pickerLayout->cancelButton);

    m_pickerLayout->setButton = elm_button_add(m_pickerLayout->popup);
    elm_object_style_set(m_pickerLayout->setButton, "popup");
    elm_object_domain_translatable_part_text_set(m_pickerLayout->setButton, NULL, "WebKit", "IDS_WEBVIEW_BUTTON_SET_ABB2");
    elm_object_part_content_set(m_pickerLayout->popup, "button2", m_pickerLayout->setButton);

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    evas_object_smart_callback_add(m_pickerLayout->datePicker, "edit,end", _edit_end_cb, 0);
#endif

    evas_object_show(m_pickerLayout->datePicker);
    evas_object_show(m_pickerLayout->timePicker);
    elm_box_pack_end(box, m_pickerLayout->datePicker);
    elm_box_pack_end(box, m_pickerLayout->timePicker);

    elm_object_content_set(m_pickerLayout->popup, box);
}

void InputPicker::deletePopupLayout()
{
    if (!m_pickerLayout)
        return;

    if(m_pickerLayout->m_widgetWin){
        evas_object_del(m_pickerLayout->m_widgetWin);
        m_pickerLayout->m_widgetWin = 0;
        evas_object_hide(m_pickerLayout->popup);
        m_pickerLayout->popup = 0;
    }

    delete m_pickerLayout;
    m_pickerLayout = 0;
}

void InputPicker::_date_picker_set_button_cliked_cb(void* data,  Evas_Object* obj, void* event_info)
{
    struct tm currentTime;
    memset(&currentTime, 0, sizeof(struct tm));

    InputPicker* inputPicker = static_cast<InputPicker*>(data);

    elm_datetime_value_get(inputPicker->m_pickerLayout->datePicker, &currentTime);
    mktime(&currentTime);

    char dateStr[20] = { 0, };

    if (inputPicker->m_pickerLayout->inputType == EWK_INPUT_TYPE_DATE)
        strftime(dateStr, 20, "%F" , &currentTime);
    else if (inputPicker->m_pickerLayout->inputType == EWK_INPUT_TYPE_TIME)
        strftime(dateStr, 20, "%R", &currentTime);
    else if (inputPicker->m_pickerLayout->inputType == EWK_INPUT_TYPE_MONTH)
        strftime(dateStr, 20, "%Y-%m", &currentTime);
    else if (inputPicker->m_pickerLayout->inputType == EWK_INPUT_TYPE_WEEK)
        strftime(dateStr, 20, "%G-W%V", &currentTime);
    else {
        struct tm time;
        memset(&time, 0, sizeof(struct tm));

        elm_datetime_value_get(inputPicker->m_pickerLayout->timePicker, &time);
        currentTime.tm_hour = time.tm_hour;
        currentTime.tm_min = time.tm_min;

        if(inputPicker->m_pickerLayout->inputType == EWK_INPUT_TYPE_DATETIME)
            strftime(dateStr, 50, "%FT%RZ", &currentTime);
        else if (inputPicker->m_pickerLayout->inputType == EWK_INPUT_TYPE_DATETIMELOCAL)
            strftime(dateStr, 50, "%FT%R", &currentTime);
    }

    ewk_view_focused_input_element_value_set(inputPicker->m_ewkView, dateStr);
    inputPicker->removePickerDelayed();
}

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
void InputPicker::_color_back_cb(void* data,  Evas_Object* obj, void* event_info)
{
    InputPicker* inputPicker = static_cast<InputPicker*>(data);

    int r = inputPicker->m_pickerLayout->initial_r;
    int g = inputPicker->m_pickerLayout->initial_g;
    int b = inputPicker->m_pickerLayout->initial_b;
    int a = 255;
    ewk_view_color_picker_color_set(inputPicker->m_ewkView, r, g, b, a);

    inputPicker->removePickerDelayed();
}

void InputPicker::_popup_back_cb(void* data, Evas_Object* obj, void* event_info)
{
    removePicker(data);
}

void InputPicker::_edit_end_cb(void* data, Evas_Object* obj, void* event_info)
{
    WebKit::InputPicker* inputPicker = static_cast<WebKit::InputPicker*>(data);
    if (inputPicker)
        inputPicker->removePickerDelayed();
}
#endif

} // namespace WebKit

#endif // PLATFORM(TIZEN) && ENABLE(TIZEN_INPUT_TAG_EXTENSION)
