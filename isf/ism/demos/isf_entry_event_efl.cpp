/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Shuo Liu <shuo0805.liu@samsung.com>, Jihoon Kim <jihoon48.kim@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <Ecore_IMF.h>
#include "isf_demo_efl.h"

static Evas_Object * _entry1                 = NULL;
static Evas_Object * _entry2                 = NULL;
static Evas_Object * _key_event_label        = NULL;
static Evas_Object * _preedit_event_label    = NULL;
static Evas_Object * _commit_event_label     = NULL;

static void _input_panel_event_callback (void *data, Ecore_IMF_Context *ctx, int value)
{
    if (value == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
        LOGD ("ECORE_IMF_INPUT_PANEL_STATE_SHOW\n");
    } else if (value == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
        LOGD ("ECORE_IMF_INPUT_PANEL_STATE_HIDE\n");
    }
}

static void _evas_key_up_cb (void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    static char str [100];
    Evas_Event_Key_Up *ev = (Evas_Event_Key_Up *) event_info;

    if (obj == _entry1) {
        snprintf (str, sizeof (str), "entry 1  get keyEvent: %s", (char *)(ev->keyname));
        elm_object_text_set (_key_event_label, str);
    } else if (obj == _entry2) {
        snprintf (str, sizeof (str), "entry 2  get keyEvent: %s", (char *)(ev->keyname));
        elm_object_text_set (_key_event_label, str);
    }
}

static void _ecore_imf_event_changed_cb (void *data, Ecore_IMF_Context *ctx, void *event)
{
    int len;
    static char str [100];
    char *preedit_string           = NULL;
    Ecore_IMF_Context *imf_context = NULL;

    if (elm_object_focus_get (_entry1) == EINA_TRUE) {
        imf_context = (Ecore_IMF_Context *)elm_entry_imf_context_get (_entry1);
        ecore_imf_context_preedit_string_get (imf_context, &preedit_string, &len);
        snprintf (str, sizeof (str), "entry 1 get preedit string: %s", preedit_string);
        elm_object_text_set (_preedit_event_label, str);
    } else if (elm_object_focus_get (_entry2) == EINA_TRUE) {
        imf_context = (Ecore_IMF_Context *)elm_entry_imf_context_get (_entry2);
        ecore_imf_context_preedit_string_get (imf_context, &preedit_string, &len);
        snprintf (str, sizeof (str), "entry 2 get preedit string: %s", preedit_string);
        elm_object_text_set (_preedit_event_label, str);
    }
}

static void _ecore_imf_event_commit_cb (void *data, Ecore_IMF_Context *ctx, void *event)
{
    static char str [100];
    char *commit_str = (char *)event;

    if (elm_object_focus_get (_entry1) == EINA_TRUE) {
        snprintf (str, sizeof (str), "entry 1 get commit string: %s", commit_str);
        elm_object_text_set (_commit_event_label, str);
    } else if (elm_object_focus_get (_entry2) == EINA_TRUE) {
        snprintf (str, sizeof (str), "entry 2 get commit string: %s", commit_str);
        elm_object_text_set (_commit_event_label, str);
    }
}

void isf_entry_event_demo_bt (void *data, Evas_Object *obj, void *event_info)
{
    struct appdata *ad = (struct appdata *)data;
    Ecore_IMF_Context *ic = NULL;

    Evas_Object *bx = elm_box_add (ad->naviframe);
    evas_object_size_hint_weight_set (bx, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (bx, EVAS_HINT_FILL, 0.0);
    evas_object_show (bx);

    /* Entry 1 */
    _entry1 = elm_entry_add (bx);
    elm_entry_entry_set (_entry1, "ENTRY 1");
    evas_object_size_hint_weight_set (_entry1, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (_entry1, EVAS_HINT_FILL, 0.0);
    evas_object_show (_entry1);
    evas_object_event_callback_add (_entry1, EVAS_CALLBACK_KEY_UP, _evas_key_up_cb, (void *)NULL);
    elm_box_pack_end (bx, _entry1);

    ic = (Ecore_IMF_Context *)elm_entry_imf_context_get (_entry1);
    if (ic != NULL) {
        ecore_imf_context_event_callback_add(ic, ECORE_IMF_CALLBACK_COMMIT, _ecore_imf_event_commit_cb, NULL);
        ecore_imf_context_event_callback_add(ic, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, _ecore_imf_event_changed_cb, NULL);
        ecore_imf_context_input_panel_event_callback_add (ic, ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback, NULL);
    }

    /* Entry 2 */
    _entry2 = elm_entry_add (bx);
    elm_entry_entry_set (_entry2, "ENTRY 2");
    evas_object_size_hint_weight_set (_entry2, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (_entry2, EVAS_HINT_FILL, 0.0);
    evas_object_show (_entry2);
    evas_object_event_callback_add (_entry2, EVAS_CALLBACK_KEY_UP, _evas_key_up_cb, (void *)NULL);
    elm_box_pack_end (bx, _entry2);

    ic = (Ecore_IMF_Context *)elm_entry_imf_context_get (_entry2);
    if (ic != NULL) {
        ecore_imf_context_event_callback_add(ic, ECORE_IMF_CALLBACK_COMMIT, _ecore_imf_event_commit_cb, NULL);
        ecore_imf_context_event_callback_add(ic, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, _ecore_imf_event_changed_cb, NULL);
        ecore_imf_context_input_panel_event_callback_add (ic, ECORE_IMF_INPUT_PANEL_STATE_EVENT, _input_panel_event_callback, NULL);
    }

    /* key event */
    _key_event_label = create_button (bx, "KEY EVENT");
    elm_box_pack_end (bx, _key_event_label);

    /* preedit event */
    _preedit_event_label = create_button (bx, "PREEDIT EVENT");
    elm_box_pack_end (bx, _preedit_event_label);

    /* commit event */
    _commit_event_label = create_button (bx, "COMMIT EVENT");
    elm_box_pack_end (bx, _commit_event_label);

    add_layout_to_naviframe (ad, bx, _("Entry Event"));
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
