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

#include "isf_demo_efl.h"
#include "isf_return_key_type_efl.h"

struct _menu_item {
    const char *name;
    const char *guide_text;
    Elm_Input_Panel_Return_Key_Type return_key_type;
};

static struct _menu_item _menu_its[] = {
    { N_("DEFAULT"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT },
    { N_("DEFAULT"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT },
    { N_("DONE"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE },
    { N_("GO"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_GO },
    { N_("JOIN"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_JOIN },
    { N_("LOGIN"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_LOGIN },
    { N_("NEXT"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT },
    { N_("SEARCH"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH },
    { N_("SEND"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_SEND },
    { N_("SIGNIN"), N_("click to enter"), ELM_INPUT_PANEL_RETURN_KEY_TYPE_SIGNIN },

    /* do not delete below */
    { NULL, NULL, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT }
};

static Evas_Object *_create_ef_layout (Evas_Object *parent, const char *label, const char *guide_text, Elm_Input_Panel_Return_Key_Type type)
{
    Evas_Object *en;
    Evas_Object *ef = create_ef (parent, label, guide_text, &en);
    if (!ef || !en) return NULL;

    elm_entry_input_panel_return_key_type_set (en, type);

    return ef;
}

static Evas_Object * create_inner_layout (void *data)
{
    struct appdata *ad = (struct appdata *)data;
    Evas_Object *bx = NULL;
    Evas_Object *ef = NULL;
    int idx = 0;

    Evas_Object *parent = ad->naviframe;

    bx = elm_box_add (parent);
    evas_object_size_hint_weight_set (bx, EVAS_HINT_EXPAND, 0.0);
    evas_object_size_hint_align_set (bx, EVAS_HINT_FILL, 0.0);
    evas_object_show (bx);

    while (_menu_its[idx].name != NULL) {
        ef = _create_ef_layout (parent, _menu_its[idx].name, _menu_its[idx].guide_text, _menu_its[idx].return_key_type);
        elm_box_pack_end (bx, ef);
        ++idx;
    }

    return bx;
}

void ise_return_key_type_bt (void *data, Evas_Object *obj, void *event_info)
{
    Evas_Object *lay_inner = create_inner_layout (data);
    add_layout_to_naviframe (data, lay_inner, _("Return Key Type"));
}

/*
vi:ts=4:ai:nowrap:expandtab
*/
