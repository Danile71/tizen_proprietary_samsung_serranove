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
#include "AutoFillPopup.h"
#include "EwkViewImpl.h"

#include <Edje.h>
#include <Elementary.h>
#include <WebCore/EflScreenUtilities.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {
static Elm_Genlist_Item_Class* s_listItem = 0;
static Vector<AutoFillPopupItem> s_autoFillData;
static int s_autoFillItemHeight = 0;
const double s_autoFillPopupScale = 0.7;
static double s_autoFillPopupVisibleItem = 3.5;
const int s_autoFillPopupRGB = 200;

static void autoFillPopupListTrackResize(void* data, Evas* e, Evas_Object* obj, void* event_info)
{
    int height;
    evas_object_geometry_get(obj, 0, 0, 0, &height);

    if (s_autoFillItemHeight == height)
        return;

    s_autoFillItemHeight = height;

    AutoFillPopup* autoFillPopup = static_cast<AutoFillPopup*>(data);
    autoFillPopup->updateAutoFillPopup(autoFillPopup->m_listWidth);
}

static void autoFillPopupListRealized(void* data, Evas_Object* obj, void* event_info)
{
    if (!event_info)
        return;

    Elm_Object_Item* item = static_cast<Elm_Object_Item*>(event_info);

    Evas_Object *track = elm_object_item_track(item);
    evas_object_event_callback_add(track, EVAS_CALLBACK_RESIZE, autoFillPopupListTrackResize, data);
}

static char* autoFillPopupGetItemLabel(void* data, Evas_Object* obj, const char* part)
{
    char* label;
    String lebelText;
    if (s_autoFillData[(int)data].itemtype == profileAutoFill) {
        lebelText = String::fromUTF8(s_autoFillData[(int)data].maintext.utf8().data()) + "("
            + String::fromUTF8(s_autoFillData[(int)data].subtext.utf8().data()) + ")";
    } else
        lebelText = String::fromUTF8(s_autoFillData[(int)data].maintext.utf8().data());

    label = new char[lebelText.utf8().length()+1];
    strcpy(label, lebelText.utf8().data());

    return label;
}

static void autoFillPopupItemSelectCb(void* data, Evas_Object* obj, void* event_info)
{
    Elm_Object_Item* selected_item = static_cast<Elm_Object_Item*>(event_info);
    int index = elm_genlist_item_index_get(selected_item) - 1;
    elm_genlist_item_selected_set(selected_item, EINA_FALSE);
    if (index < 0) {
        TIZEN_LOGE("Autofill item index should never be negative, perhaps wrong object?");
        return;
    }

     AutoFillPopup* autoFillPopup = static_cast<AutoFillPopup*>(data);
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    if (s_autoFillData[index].itemtype == profileAutoFill) {
        autoFillPopup->setValueForProfile(s_autoFillData[index].id);
        return;
    }
#endif
    autoFillPopup->setValueForInputElement(s_autoFillData[index].maintext);
}

AutoFillPopup::AutoFillPopup(EwkViewImpl* viewImpl)
    : m_listWidth(0)
    , m_viewImpl(viewImpl)
    , m_icon(0)
    , m_list(0)
    , m_borderWidth(0)
    , m_isShowing(false)
{
    m_icon = elm_layout_add(viewImpl->view());

    if (!m_icon)
        return;

    if (!elm_layout_file_set(m_icon, EDJE_DIR"/FormDataPopup.edj", "formdata_list"))
        return;

    m_list = elm_genlist_add(m_icon);

    m_borderWidth = (720 / WebCore::getDefaultScreenResolution().width()) * 3;
}

AutoFillPopup::~AutoFillPopup()
{
    if (m_icon)
        evas_object_del(m_icon);
}

void AutoFillPopup::move(const IntRect& inputFieldRect)
{
    if (m_icon)
        evas_object_move(m_icon, inputFieldRect.x(), inputFieldRect.y() + inputFieldRect.height());
}

void AutoFillPopup::show(const WebCore::IntRect& rect)
{
    m_isShowing = true;
    m_listWidth = rect.width();

    updateAutoFillPopup(m_listWidth);
    move(rect);

    if (m_icon)
        evas_object_show(m_icon);
}

void AutoFillPopup::hide()
{
    m_isShowing = false;
    if (m_icon)
        evas_object_hide(m_icon);
}

void AutoFillPopup::updateFormData(const Vector<AutoFillPopupItem>& data)
{
    s_autoFillData = data;
}

void AutoFillPopup::updateAutoFillPopup(int inputFieldWidth)
{
    unsigned int noOfItems = 0;
    if (m_list)
        noOfItems = elm_genlist_items_count(m_list);

    Elm_Object_Item* item = elm_genlist_first_item_get(m_list);

    if (!s_listItem) {
        s_listItem = elm_genlist_item_class_new();
        s_listItem->item_style = "default";
        s_listItem->func.text_get = autoFillPopupGetItemLabel;
        s_listItem->func.content_get = 0;
        s_listItem->func.state_get = 0;
        s_listItem->func.del = 0;
        for (size_t i = 0; i < s_autoFillData.size(); ++i)
            elm_genlist_item_append(m_list, s_listItem, (void*) i, 0, ELM_GENLIST_ITEM_NONE, autoFillPopupItemSelectCb, (void*)this);
    } else {
        if (s_autoFillData.size() == noOfItems) {
            for (size_t i = 0; i < s_autoFillData.size(); ++i) {
                elm_genlist_item_fields_update(item, "elm.text", ELM_GENLIST_ITEM_FIELD_TEXT);
                elm_genlist_item_selected_set(item, EINA_FALSE);
                item = elm_genlist_item_next_get(item);
            }
        }
        else if (s_autoFillData.size() > noOfItems) {
            for (size_t i = noOfItems; i < s_autoFillData.size(); ++i)
                elm_genlist_item_append(m_list, s_listItem, (void*) i, 0, ELM_GENLIST_ITEM_NONE, autoFillPopupItemSelectCb, (void*)this);
            for (size_t i = 0; i < s_autoFillData.size(); ++i) {
                elm_genlist_item_fields_update(item, "elm.text", ELM_GENLIST_ITEM_FIELD_TEXT);
                elm_genlist_item_selected_set(item, EINA_FALSE);
                item = elm_genlist_item_next_get(item);
            }
        }
        else if (s_autoFillData.size() < noOfItems) {
            for (size_t i = 0; i < s_autoFillData.size(); ++i) {
                elm_genlist_item_fields_update(item, "elm.text", ELM_GENLIST_ITEM_FIELD_TEXT);
                elm_genlist_item_selected_set(item, EINA_FALSE);
                item = elm_genlist_item_next_get(item);
            }
            item = elm_genlist_last_item_get(m_list);
            Elm_Object_Item* temp = NULL;
            for (size_t i = 0; i < noOfItems - s_autoFillData.size(); ++i) {
                temp = elm_genlist_item_prev_get(item);
                elm_object_item_del(item);
                item = temp;
            }
        }
    }

    item = elm_genlist_first_item_get(m_list);

    for (size_t i = 0; i < s_autoFillData.size(); ++i) {
        if (i == s_autoFillData.size() - 1)
            elm_object_item_signal_emit(item, "elm,state,bottomline,hide", "");
        else
            elm_object_item_signal_emit(item, "elm,state,bottomline,show", "");
        item = elm_genlist_item_next_get(item);
    }

    elm_object_scale_set(m_list, s_autoFillPopupScale);

    evas_object_smart_callback_add(m_list, "realized", autoFillPopupListRealized, this);
    evas_object_show(m_list);

    elm_object_part_content_set(m_icon, "list_container", m_list);

    int orientation =  m_viewImpl->orientation;
    s_autoFillPopupVisibleItem = (orientation == 90 || orientation == -90) ? 1.2 : 3.5;

    if (s_autoFillItemHeight) {
        int listHeight = s_autoFillItemHeight * s_autoFillData.size();
        int listWidth = inputFieldWidth - m_borderWidth;

        if(listHeight > s_autoFillItemHeight * s_autoFillPopupVisibleItem)
            listHeight = s_autoFillItemHeight * s_autoFillPopupVisibleItem;
        if(listWidth < WebCore::getDefaultScreenResolution().width() / 2)
            listWidth = (WebCore::getDefaultScreenResolution().width() / 2) - m_borderWidth;

        Evas_Object* borderUp = elm_bg_add(m_icon);
        Evas_Object* borderDown = elm_bg_add(m_icon);
        Evas_Object* borderLeft = elm_bg_add(m_icon);
        Evas_Object* borderRight = elm_bg_add(m_icon);

        elm_bg_color_set(borderUp, s_autoFillPopupRGB, s_autoFillPopupRGB, s_autoFillPopupRGB);
        evas_object_size_hint_min_set(borderUp, listWidth, m_borderWidth);
        evas_object_show(borderUp);

        elm_bg_color_set(borderDown, s_autoFillPopupRGB, s_autoFillPopupRGB, s_autoFillPopupRGB);
        evas_object_size_hint_min_set(borderDown, listWidth, m_borderWidth);
        evas_object_show(borderDown);

        elm_bg_color_set(borderLeft, s_autoFillPopupRGB, s_autoFillPopupRGB, s_autoFillPopupRGB);
        evas_object_size_hint_min_set(borderLeft, m_borderWidth, listHeight);
        evas_object_show(borderLeft);

        elm_bg_color_set(borderRight, s_autoFillPopupRGB, s_autoFillPopupRGB, s_autoFillPopupRGB);
        evas_object_size_hint_min_set(borderRight, m_borderWidth, listHeight);
        evas_object_show(borderRight);

        elm_object_part_content_set(m_icon, "border_up", borderUp);
        elm_object_part_content_set(m_icon, "border_down", borderDown);
        elm_object_part_content_set(m_icon, "border_left", borderLeft);
        elm_object_part_content_set(m_icon, "border_right", borderRight);

        evas_object_size_hint_min_set(m_icon, listWidth, listHeight);
        evas_object_resize(m_icon, listWidth, listHeight);

        evas_object_propagate_events_set(m_icon, false);
    } else {
        // Set the size temporarily for calculate the s_autoFillItemHeight.
        evas_object_size_hint_min_set(m_icon, 1, 1);
        evas_object_resize(m_icon, 1, 1);

        elm_genlist_item_show(elm_genlist_first_item_get(m_list), ELM_GENLIST_ITEM_SCROLLTO_TOP);
    }
}

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
void AutoFillPopup::setValueForProfile(int& id)
{
    ewk_view_profile_form_candidate_set_to_form(m_viewImpl->view(), id);
    hide();
}
#endif
void AutoFillPopup::setValueForInputElement(String& value)
{
    m_viewImpl->page()->setFocusedInputElementValue(value, false);
    hide();
    m_candidateValue = emptyString();
}

} // namespace WebKit

#endif // TIZEN_WEBKIT2_FORM_DATABASE
