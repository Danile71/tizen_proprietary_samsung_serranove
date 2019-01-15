#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED
#define ELM_INTERFACE_ATSPI_COMPONENT_PROTECTED
#define ELM_INTERFACE_ATSPI_WIDGET_ACTION_PROTECTED
#define ELM_WIDGET_ITEM_PROTECTED

//TIZEN_ONLY: mesh gradient effect
#define MESH_GRADIENT

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_ctxpopup.h"

#ifdef MESH_GRADIENT
#include <gxt_shared_image_buffer.h>
#endif

#define MY_CLASS ELM_CTXPOPUP_CLASS

#define MY_CLASS_NAME "Elm_Ctxpopup"
#define MY_CLASS_NAME_LEGACY "elm_ctxpopup"


EAPI const char ELM_CTXPOPUP_SMART_NAME[] = "elm_ctxpopup";

#define ELM_PRIV_CTXPOPUP_SIGNALS(cmd) \
   cmd(SIG_DISMISSED, "dismissed", "") \

ELM_PRIV_CTXPOPUP_SIGNALS(ELM_PRIV_STATIC_VARIABLE_DECLARE);

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   ELM_PRIV_CTXPOPUP_SIGNALS(ELM_PRIV_SMART_CALLBACKS_DESC)
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   {NULL, NULL}
};
#undef ELM_PRIV_CTXPOPUP_SIGNALS

#define TTS_STR_MENU_POPUP dgettext(PACKAGE, "IDS_SCR_OPT_MENU_POP_UP_TTS")
#define TTS_STR_MENU_CLOSE dgettext(PACKAGE, "IDS_ST_BODY_DOUBLE_TAP_TO_CLOSE_THE_MENU_T_TTS")

static const char ACCESS_OUTLINE_PART[] = "access.outline";

static void _on_move(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED);
static void _item_sizing_eval(Elm_Ctxpopup_Item_Data *item);
static Eina_Bool _key_action_escape(Evas_Object *obj, const char *params EINA_UNUSED);
static void _create_mesh_bg_image(Evas_Object *obj, Evas_Object *container);

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_widget_translate(Eo *obj, Elm_Ctxpopup_Data *sd)
{
   Eina_List *l;
   Elm_Ctxpopup_Item_Data *it;

   if (sd->auto_hide) evas_object_hide(obj);

   EINA_LIST_FOREACH(sd->items, l, it)
     eo_do(EO_OBJ(it), elm_wdg_item_translate());

   eo_do_super(obj, MY_CLASS, elm_obj_widget_translate());

   return EINA_TRUE;
}

static Evas_Object *
_access_object_get(const Evas_Object *obj, const char* part)
{
   Evas_Object *po, *ao;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   po = (Evas_Object *)edje_object_part_object_get(wd->resize_obj, part);
   ao = evas_object_data_get(po, "_part_access_obj");

   return ao;
}

// FIXME: There are applications which do not use elm_win as top widget.
// This is workaround! Those could not use focus!
static Eina_Bool _focus_enabled(Evas_Object *obj)
{
   if (!elm_widget_focus_get(obj)) return EINA_FALSE;

   const Evas_Object *win = elm_widget_top_get(obj);
   const char *type = evas_object_type_get(win);

   if (type && !strcmp(type, "elm_win"))
     {
        return elm_win_focus_highlight_enabled_get(win);
     }
   return EINA_FALSE;
}

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_widget_focus_next(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd, Elm_Focus_Direction dir, Evas_Object **next)
{
   Eina_List *items = NULL;
   Evas_Object *ao;

   if (!sd)
     return EINA_FALSE;

   if (_elm_config->access_mode)
     {
        Eina_Bool ret;

        ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
        if (ao) items = eina_list_append(items, ao);

        /* scroller exists when ctxpopup has an item */
        if (sd->scr)
           items = eina_list_append(items, sd->scr);
        else
           items = eina_list_append(items, sd->box);

        _elm_access_auto_highlight_set(EINA_TRUE);
        ret = elm_widget_focus_list_next_get
                 (obj, items, eina_list_data_get, dir, next);
        _elm_access_auto_highlight_set(EINA_FALSE);
        return ret;
     }
   else
     {
        elm_widget_focus_next_get(sd->box, dir, next);
        if (!*next) *next = (Evas_Object *)obj;
        return EINA_TRUE;
     }
}

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_widget_focus_next_manager_is(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *_pd EINA_UNUSED)
{
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_widget_focus_direction_manager_is(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *_pd EINA_UNUSED)
{
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_widget_focus_direction(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd, const Evas_Object *base, double degree, Evas_Object **direction, double *weight)
{
   Eina_Bool int_ret;

   Eina_List *l = NULL;
   void *(*list_data_get)(const Eina_List *list);

   if (!sd)
     return EINA_FALSE;

   list_data_get = eina_list_data_get;

   l = eina_list_append(l, sd->box);

   int_ret = elm_widget_focus_list_direction_get
            (obj, base, l, list_data_get, degree, direction, weight);
   eina_list_free(l);

   return int_ret;
}

static Eina_Bool
_key_action_move(Evas_Object *obj, const char *params)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);
   const char *dir = params;

   if (!sd->box) return EINA_FALSE;

   if (!strcmp(dir, "previous"))
     elm_widget_focus_cycle(sd->box, ELM_FOCUS_PREVIOUS);
   else if (!strcmp(dir, "next"))
     elm_widget_focus_cycle(sd->box, ELM_FOCUS_NEXT);
   else if (!strcmp(dir, "left"))
     elm_widget_focus_cycle(sd->box, ELM_FOCUS_LEFT);
   else if (!strcmp(dir, "right"))
     elm_widget_focus_cycle(sd->box, ELM_FOCUS_RIGHT);
   else if (!strcmp(dir, "up"))
     elm_widget_focus_cycle(sd->box, ELM_FOCUS_UP);
   else if (!strcmp(dir, "down"))
     elm_widget_focus_cycle(sd->box, ELM_FOCUS_DOWN);
   else return EINA_FALSE;

   return EINA_TRUE;
}

static void
_x_pos_adjust(Evas_Coord_Point *pos,
              Evas_Coord_Point *base_size,
              Evas_Coord_Rectangle *hover_area)
{
   pos->x -= (base_size->x / 2);

   if (pos->x < hover_area->x)
     pos->x = hover_area->x;
   else if ((pos->x + base_size->x) > (hover_area->x + hover_area->w))
     pos->x = (hover_area->x + hover_area->w) - base_size->x;

   if (base_size->x > hover_area->w)
     base_size->x -= (base_size->x - hover_area->w);

   if (pos->x < hover_area->x)
     pos->x = hover_area->x;
}

static void
_y_pos_adjust(Evas_Coord_Point *pos,
              Evas_Coord_Point *base_size,
              Evas_Coord_Rectangle *hover_area)
{
   pos->y -= (base_size->y / 2);

   if (pos->y < hover_area->y)
     pos->y = hover_area->y;
   else if ((pos->y + base_size->y) > (hover_area->y + hover_area->h))
     pos->y = hover_area->y + hover_area->h - base_size->y;

   if (base_size->y > hover_area->h)
     base_size->y -= (base_size->y - hover_area->h);

   if (pos->y < hover_area->y)
     pos->y = hover_area->y;
}

static void
_item_select_cb(void *data,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   Elm_Ctxpopup_Item_Data *item = data;

   if (!item) return;
   if (eo_do(EO_OBJ(item), elm_wdg_item_disabled_get()))
     {
        if (_elm_config->access_mode)
          elm_access_say(E_("Disabled"));
        return;
     }

   if (item->wcb.org_func_cb)
     item->wcb.org_func_cb((void*)item->wcb.org_data, WIDGET(item), EO_OBJ(item));
}

static char *
_access_info_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   Elm_Ctxpopup_Item_Data *it = (Elm_Ctxpopup_Item_Data *)data;
   const char *txt = NULL;
   Evas_Object *icon = NULL;

   if (!it) return NULL;

   txt = it->label;
   icon = it->icon;

   if (txt) return strdup(txt);
   if (icon) return strdup(E_("icon"));
   return NULL;
}

static char *
_access_state_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   Elm_Ctxpopup_Item_Data *it = (Elm_Ctxpopup_Item_Data *)data;
   if (!it) return NULL;

   if (eo_do(EO_OBJ(it), elm_wdg_item_disabled_get()))
     return strdup(E_("Disabled"));

   return NULL;
}

static void
_access_focusable_button_register(Evas_Object *obj, Elm_Ctxpopup_Item_Data *it)
{
   Elm_Access_Info *ai;

   ai = _elm_access_info_get(obj);

   _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, it);
   _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, it);
   _elm_access_callback_set(ai, ELM_ACCESS_TYPE, NULL, NULL);

   it->base->access_obj = obj;
}

static void
_mouse_down_cb(Elm_Ctxpopup_Item_Data *it,
               Evas *evas EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               Evas_Event_Mouse_Down *ev)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);

   if (ev->button != 1) return;

   //If counter is not zero, it means all other multi down is not released.
   if (sd->multi_down != 0) return;
   sd->mouse_down = EINA_TRUE;
//******************** TIZEN Only
   sd->pressed = EINA_TRUE;
//****************************
}

//******************** TIZEN Only
static void
_mouse_move_cb(Elm_Ctxpopup_Item_Data *it,
               Evas *evas EINA_UNUSED,
               Evas_Object *obj,
               void *event_info)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);
   Evas_Event_Mouse_Move *ev = event_info;

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        if (sd->pressed)
          {
             edje_object_signal_emit(obj,"elm,action,unpressed", "elm");
             sd->pressed = EINA_FALSE;
          }
     }
   else
     {
        Evas_Coord x, y, w, h;
        evas_object_geometry_get(obj, &x, &y, &w, &h);
        if ((sd->pressed) && (ELM_RECTS_POINT_OUT(x, y, w, h, ev->cur.canvas.x, ev->cur.canvas.y)))
          {
             edje_object_signal_emit(obj,"elm,action,unpressed", "elm");
             edje_object_signal_emit(obj,"elm,action,mouse,out", "elm");
             sd->pressed = EINA_FALSE;
          }
     }
}
//****************************

static void
_mouse_up_cb(Elm_Ctxpopup_Item_Data *it,
             Evas *evas EINA_UNUSED,
             Evas_Object *obj EINA_UNUSED,
             Evas_Event_Mouse_Up *ev)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);

   if (ev->button != 1) return;

   if (!sd->mouse_down) return;
   sd->mouse_down = EINA_FALSE;
//******************** TIZEN Only
   sd->pressed = EINA_FALSE;
//****************************
}

static void
_multi_down_cb(void *data,
                    Evas *evas EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   Elm_Ctxpopup_Item_Data *it = data;
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);

   //Emitting signal to inform edje about multi down event.
   edje_object_signal_emit(VIEW(it), "elm,action,multi,down", "elm");
   sd->multi_down++;
}

static void
_multi_up_cb(void *data,
                    Evas *evas EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   Elm_Ctxpopup_Item_Data *it = data;
   ELM_CTXPOPUP_DATA_GET(WIDGET(it), sd);

   sd->multi_down--;
   if(sd->multi_down == 0)
     {
        //Emitting signal to inform edje about last multi up event.
        edje_object_signal_emit(VIEW(it), "elm,action,multi,up", "elm");
     }
}

static void
_item_new(Elm_Ctxpopup_Item_Data *item,
          char *group_name)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(item), sd);
   if (!sd) return;

   VIEW(item) = edje_object_add(evas_object_evas_get(sd->box));
   edje_object_mirrored_set(VIEW(item), elm_widget_mirrored_get(WIDGET(item)));
   _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", group_name,
                         elm_widget_style_get(WIDGET(item)));
   evas_object_size_hint_align_set(VIEW(item), EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(VIEW(item));

   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MOUSE_DOWN, (Evas_Object_Event_Cb)_mouse_down_cb,
     item);
//******************** TIZEN Only
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MOUSE_MOVE, (Evas_Object_Event_Cb)_mouse_move_cb,
     item);
//****************************
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MOUSE_UP, (Evas_Object_Event_Cb)_mouse_up_cb, item);

   /*Registering Multi down/up events to ignore mouse down/up events untill all
     multi down/up events are released.*/
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MULTI_DOWN, (Evas_Object_Event_Cb)_multi_down_cb,
     item);
   evas_object_event_callback_add
     (VIEW(item), EVAS_CALLBACK_MULTI_UP, (Evas_Object_Event_Cb)_multi_up_cb,
     item);
}

static void
_item_icon_set(Elm_Ctxpopup_Item_Data *item,
               Evas_Object *icon)
{
   if (item->icon)
     evas_object_del(item->icon);

   item->icon = icon;
   if (!icon) return;

   edje_object_part_swallow(VIEW(item), "elm.swallow.icon", item->icon);
}

static void
_item_label_set(Elm_Ctxpopup_Item_Data *item,
                const char *label)
{
   if (!eina_stringshare_replace(&item->label, label))
     return;

   ELM_CTXPOPUP_DATA_GET(WIDGET(item), sd);

   edje_object_part_text_set(VIEW(item), "elm.text", label);
   if (sd->visible) _item_sizing_eval(item);
}

static Evas_Object *
_item_in_focusable_button(Elm_Ctxpopup_Item_Data *item)
{
   Evas_Object *bt;

   bt = elm_button_add(WIDGET(item));
   elm_object_style_set(bt, "focus");
   evas_object_size_hint_align_set(bt, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_object_part_content_set(bt, "elm.swallow.content", VIEW(item));
   evas_object_smart_callback_add(bt, "clicked", _item_select_cb, item);
   evas_object_show(bt);

   return bt;
}

EOLIAN static Eina_Bool
_elm_ctxpopup_item_elm_widget_item_del_pre(Eo *eo_item EINA_UNUSED, Elm_Ctxpopup_Item_Data *item)
{
   Evas_Object *btn;

   ELM_CTXPOPUP_DATA_GET(WIDGET(item), sd);
   if (!sd) return EINA_FALSE;

   btn = item->btn;
   elm_box_unpack(sd->box, btn);
   evas_object_smart_callback_del_full(btn, "clicked", _item_select_cb, item);
   evas_object_del(btn);

   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   if (item->icon)
     evas_object_del(item->icon);
   if (VIEW(item))
     evas_object_del(VIEW(item));

   eina_stringshare_del(item->label);
   sd->items = eina_list_remove(sd->items, item);

   if (eina_list_count(sd->items) < 1)
     {
        evas_object_hide(WIDGET(item));
        return EINA_TRUE;
     }
   if (sd->visible) elm_layout_sizing_eval(WIDGET(item));

#ifndef ELM_FEATURE_WEARABLE
   if (item == sd->top_drawn_item) sd->top_drawn_item = NULL;
#endif
   return EINA_TRUE;
}

static void
_items_remove(Elm_Ctxpopup_Data *sd)
{
   Eina_List *l, *l_next;
   Elm_Ctxpopup_Item_Data *item;

   if (!sd->items) return;

   EINA_LIST_FOREACH_SAFE(sd->items, l, l_next, item)
      eo_do(EO_OBJ(item), elm_wdg_item_del());

   sd->items = NULL;
}

static void
_item_sizing_eval(Elm_Ctxpopup_Item_Data *item)
{
   Evas_Coord min_w = -1, min_h = -1, max_w = -1, max_h = -1;

   if (!item) return;

   edje_object_signal_emit(VIEW(item), "elm,state,text,default", "elm");
   edje_object_message_signal_process(VIEW(item));
   edje_object_size_min_restricted_calc(VIEW(item), &min_w, &min_h, min_w,
                                        min_h);
   evas_object_size_hint_min_set(VIEW(item), min_w, min_h);
   evas_object_size_hint_max_set(VIEW(item), max_w, max_h);
}

static Elm_Ctxpopup_Direction
_base_geometry_calc(Evas_Object *obj,
                    Evas_Coord_Rectangle *rect)
{
   Elm_Ctxpopup_Direction dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;
   Evas_Coord_Rectangle hover_area;
   Evas_Coord_Point pos = {0, 0};
   Evas_Coord_Point arrow_size;
   Evas_Coord_Point base_size;
   Evas_Coord_Point max_size;
   Evas_Coord_Point min_size;
   Evas_Coord_Point temp;
   int idx;
   const char *str;

   ELM_CTXPOPUP_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, ELM_CTXPOPUP_DIRECTION_DOWN);

   if (!rect) return ELM_CTXPOPUP_DIRECTION_DOWN;

   edje_object_part_geometry_get
     (sd->arrow, "ctxpopup_arrow", NULL, NULL, &arrow_size.x, &arrow_size.y);
   evas_object_resize(sd->arrow, arrow_size.x, arrow_size.y);

   //Initialize Area Rectangle.
   evas_object_geometry_get
     (sd->parent, &hover_area.x, &hover_area.y, &hover_area.w,
     &hover_area.h);

   evas_object_geometry_get(obj, &pos.x, &pos.y, NULL, NULL);

   //recalc the edje
   edje_object_size_min_calc
     (wd->resize_obj, &base_size.x, &base_size.y);
   evas_object_smart_calculate(wd->resize_obj);

   //Limit to Max Size
   evas_object_size_hint_max_get(obj, &max_size.x, &max_size.y);
   if ((max_size.x == -1) || (max_size.y == -1))
     {
        str = edje_object_data_get(sd->layout, "visible_maxw");
        if (str)
          max_size.x = (int)(atoi(str)
                             * elm_config_scale_get()
                             * elm_object_scale_get(obj)
                             / edje_object_base_scale_get(sd->layout));
        str = edje_object_data_get(sd->layout, "visible_maxh");
        if (str)
          max_size.y = (int)(atoi(str)
                             * elm_config_scale_get()
                             * elm_object_scale_get(obj)
                             / edje_object_base_scale_get(sd->layout));
     }

   if ((max_size.y > 0) && (base_size.y > max_size.y))
     base_size.y = max_size.y;

   if ((max_size.x > 0) && (base_size.x > max_size.x))
     base_size.x = max_size.x;

   //Limit to Min Size
   evas_object_size_hint_min_get(obj, &min_size.x, &min_size.y);
   if ((min_size.x == 0) || (min_size.y == 0))
     edje_object_size_min_get(sd->layout, &min_size.x, &min_size.y);

   if ((min_size.y > 0) && (base_size.y < min_size.y))
     base_size.y = min_size.y;

   if ((min_size.x > 0) && (base_size.x < min_size.x))
     base_size.x = min_size.x;

   //Check the Which direction is available.
   //If find a avaialble direction, it adjusts position and size.
   for (idx = 0; idx < 4; idx++)
     {
        switch (sd->dir_priority[idx])
          {
           case ELM_CTXPOPUP_DIRECTION_UNKNOWN:

           case ELM_CTXPOPUP_DIRECTION_UP:
             temp.y = (pos.y - base_size.y);
             if ((temp.y - arrow_size.y) < hover_area.y)
               continue;

             _x_pos_adjust(&pos, &base_size, &hover_area);
             pos.y -= base_size.y;
             dir = ELM_CTXPOPUP_DIRECTION_UP;
             break;

           case ELM_CTXPOPUP_DIRECTION_LEFT:
             temp.x = (pos.x - base_size.x);
             if ((temp.x - arrow_size.x) < hover_area.x)
               continue;

             _y_pos_adjust(&pos, &base_size, &hover_area);
             pos.x -= base_size.x;
             dir = ELM_CTXPOPUP_DIRECTION_LEFT;
             break;

           case ELM_CTXPOPUP_DIRECTION_RIGHT:
             temp.x = (pos.x + base_size.x);
             if ((temp.x + arrow_size.x) >
                 (hover_area.x + hover_area.w))
               continue;

             _y_pos_adjust(&pos, &base_size, &hover_area);
             dir = ELM_CTXPOPUP_DIRECTION_RIGHT;
             break;

           case ELM_CTXPOPUP_DIRECTION_DOWN:
             temp.y = (pos.y + base_size.y);
             if ((temp.y + arrow_size.y) >
                 (hover_area.y + hover_area.h))
               continue;

             _x_pos_adjust(&pos, &base_size, &hover_area);
             dir = ELM_CTXPOPUP_DIRECTION_DOWN;
             break;

           default:
             continue;
          }
        break;
     }

   //In this case, all directions are invalid because of lack of space.
   if (idx == 4)
     {
        Evas_Coord length[2];

        if (!sd->horizontal)
          {
             length[0] = pos.y - hover_area.y;
             length[1] = (hover_area.y + hover_area.h) - pos.y;

             // ELM_CTXPOPUP_DIRECTION_UP
             if (length[0] > length[1])
               {
                  _x_pos_adjust(&pos, &base_size, &hover_area);
                  pos.y -= base_size.y;
                  dir = ELM_CTXPOPUP_DIRECTION_UP;
                  if (pos.y < (hover_area.y + arrow_size.y))
                    {
                       base_size.y -= ((hover_area.y + arrow_size.y) - pos.y);
                       pos.y = hover_area.y + arrow_size.y;
                    }
               }
             //ELM_CTXPOPUP_DIRECTION_DOWN
             else
               {
                  _x_pos_adjust(&pos, &base_size, &hover_area);
                  dir = ELM_CTXPOPUP_DIRECTION_DOWN;
                  if ((pos.y + arrow_size.y + base_size.y) >
                      (hover_area.y + hover_area.h))
                    base_size.y -=
                      ((pos.y + arrow_size.y + base_size.y) -
                       (hover_area.y + hover_area.h));
               }
          }
        else
          {
             length[0] = pos.x - hover_area.x;
             length[1] = (hover_area.x + hover_area.w) - pos.x;

             //ELM_CTXPOPUP_DIRECTION_LEFT
             if (length[0] > length[1])
               {
                  _y_pos_adjust(&pos, &base_size, &hover_area);
                  pos.x -= base_size.x;
                  dir = ELM_CTXPOPUP_DIRECTION_LEFT;
                  if (pos.x < (hover_area.x + arrow_size.x))
                    {
                       base_size.x -= ((hover_area.x + arrow_size.x) - pos.x);
                       pos.x = hover_area.x + arrow_size.x;
                    }
               }
             //ELM_CTXPOPUP_DIRECTION_RIGHT
             else
               {
                  _y_pos_adjust(&pos, &base_size, &hover_area);
                  dir = ELM_CTXPOPUP_DIRECTION_RIGHT;
                  if (pos.x + (arrow_size.x + base_size.x) >
                      hover_area.x + hover_area.w)
                    base_size.x -=
                      ((pos.x + arrow_size.x + base_size.x) -
                       (hover_area.x + hover_area.w));
               }
          }
     }

   //Final position and size.
   rect->x = pos.x;
   rect->y = pos.y;
   rect->w = base_size.x;
   rect->h = base_size.y;

   return dir;
}

static void
_arrow_update(Evas_Object *obj,
              Elm_Ctxpopup_Direction dir,
              Evas_Coord_Rectangle base_size)
{
   Evas_Coord_Rectangle arrow_size;
   Evas_Coord x, y;
   double drag;
   Evas_Coord_Rectangle shadow_left_top, shadow_right_bottom, arrow_padding;

   ELM_CTXPOPUP_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_geometry_get(obj, &x, &y, NULL, NULL);
   evas_object_geometry_get
     (sd->arrow, NULL, NULL, &arrow_size.w, &arrow_size.h);

   /* tizen only : since ctxpopup of tizen has shadow, start and end padding of arrow, it should be put together when updating arrow
    * so there are some differences between open source and tizen */
   edje_object_part_geometry_get(wd->resize_obj, "frame_shadow_left_top_padding", NULL, NULL, &shadow_left_top.w, &shadow_left_top.h);
   edje_object_part_geometry_get(wd->resize_obj, "frame_shadow_right_bottom_padding", NULL, NULL, &shadow_right_bottom.w, &shadow_right_bottom.h);
   edje_object_part_geometry_get(wd->resize_obj, "ctxpopup_frame_left_top", NULL, NULL, &arrow_padding.w, &arrow_padding.h);

   /* arrow is not being kept as sub-object on purpose, here. the
    * design of the widget does not help with the contrary */

   switch (dir)
     {
      case ELM_CTXPOPUP_DIRECTION_RIGHT:
        edje_object_signal_emit(sd->arrow, "elm,state,left", "elm");
        edje_object_part_swallow
           (wd->resize_obj,
            (elm_widget_mirrored_get(obj) ? "elm.swallow.arrow_right" :
             "elm.swallow.arrow_left"), sd->arrow);

        if (base_size.h > 0)
          {
             if (y <= ((arrow_size.h * 0.5) + base_size.y + shadow_left_top.h + arrow_padding.h))
               y = 0;
             else if (y >= (base_size.y + base_size.h - ((arrow_size.h * 0.5) + shadow_right_bottom.h + arrow_padding.h)))
               y = base_size.h - (arrow_size.h + shadow_right_bottom.h + shadow_left_top.h + (arrow_padding.h * 2));
             else
               y = y - base_size.y - ((arrow_size.h * 0.5) + shadow_left_top.h + arrow_padding.h);
             drag = (double)(y) / (double)(base_size.h - (arrow_size.h + shadow_right_bottom.h + shadow_left_top.h + (arrow_padding.h * 2)));
             edje_object_part_drag_value_set
                (wd->resize_obj,
                 (elm_widget_mirrored_get(obj) ? "elm.swallow.arrow_right" :
                  "elm.swallow.arrow_left"), 1, drag);
          }
        break;

      case ELM_CTXPOPUP_DIRECTION_LEFT:
        edje_object_signal_emit(sd->arrow, "elm,state,right", "elm");
        edje_object_part_swallow
           (wd->resize_obj,
            (elm_widget_mirrored_get(obj) ? "elm.swallow.arrow_left" :
             "elm.swallow.arrow_right"), sd->arrow);

        if (base_size.h > 0)
          {
             if (y <= ((arrow_size.h * 0.5) + base_size.y + shadow_left_top.h + arrow_padding.h))
               y = 0;
             else if (y >= (base_size.y + base_size.h - ((arrow_size.h * 0.5) + shadow_right_bottom.h + arrow_padding.h)))
               y = base_size.h - (arrow_size.h + shadow_right_bottom.h + shadow_left_top.h + (arrow_padding.h * 2));
             else
               y = y - base_size.y - ((arrow_size.h * 0.5) + shadow_left_top.h + arrow_padding.h);
             drag = (double)(y) / (double)(base_size.h - (arrow_size.h + shadow_right_bottom.h + shadow_left_top.h + (arrow_padding.h * 2)));
             edje_object_part_drag_value_set
                (wd->resize_obj,
                 (elm_widget_mirrored_get(obj) ? "elm.swallow.arrow_left" :
                  "elm.swallow.arrow_right"), 0, drag);
          }
        break;

      case ELM_CTXPOPUP_DIRECTION_DOWN:
        edje_object_signal_emit(sd->arrow, "elm,state,top", "elm");
        edje_object_part_swallow
          (wd->resize_obj, "elm.swallow.arrow_up",
          sd->arrow);

        if (base_size.w > 0)
          {
             if (x <= ((arrow_size.w * 0.5) + base_size.x + shadow_left_top.w + arrow_padding.w))
               x = 0;
             else if (x >= (base_size.x + base_size.w - ((arrow_size.w * 0.5) + shadow_right_bottom.w + arrow_padding.w)))
               x = base_size.w - (arrow_size.w + shadow_right_bottom.w + shadow_left_top.w + (arrow_padding.w * 2));
             else
               x = x - base_size.x - ((arrow_size.w * 0.5) + shadow_left_top.w + arrow_padding.w);
             drag = (double)(x) / (double)(base_size.w - (arrow_size.w + shadow_right_bottom.w + shadow_left_top.w + (arrow_padding.w * 2)));
             edje_object_part_drag_value_set
               (wd->resize_obj, "elm.swallow.arrow_up",
               drag, 1);
          }
        break;

      case ELM_CTXPOPUP_DIRECTION_UP:
        edje_object_signal_emit(sd->arrow, "elm,state,bottom", "elm");
        edje_object_part_swallow
          (wd->resize_obj, "elm.swallow.arrow_down",
          sd->arrow);

        if (base_size.w > 0)
          {
             if (x <= ((arrow_size.w * 0.5) + base_size.x + shadow_left_top.w + arrow_padding.w))
               x = 0;
             else if (x >= (base_size.x + base_size.w - ((arrow_size.w * 0.5) + shadow_right_bottom.w + arrow_padding.w)))
               x = base_size.w - (arrow_size.w + shadow_right_bottom.w + shadow_left_top.w + (arrow_padding.w * 2));
             else
               x = x - base_size.x - ((arrow_size.w * 0.5) + shadow_left_top.w + arrow_padding.w);
             drag = (double)(x) / (double)(base_size.w - (arrow_size.w + shadow_right_bottom.w + shadow_left_top.w + (arrow_padding.w * 2)));
             edje_object_part_drag_value_set
               (wd->resize_obj, "elm.swallow.arrow_down",
               drag, 0);
          }
        break;

      default:
        break;
     }

   //should be here for getting accurate geometry value
   evas_object_smart_calculate(wd->resize_obj);
}

//TIZEN_ONLY(20150923): Using ecore idle enterer for better performance
//static void
//_show_signals_emit(Evas_Object *obj,
//                   Elm_Ctxpopup_Direction dir)
static Eina_Bool
_show_signals_emit(void *data)
{
   Evas_Object *obj = (Evas_Object *)data;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd->visible) return ECORE_CALLBACK_RENEW;

   switch (sd->dir)
     {
      case ELM_CTXPOPUP_DIRECTION_UP:
        edje_object_signal_emit(sd->layout, "elm,state,show,up", "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_LEFT:
        edje_object_signal_emit(sd->layout, (elm_widget_mirrored_get(obj) ? "elm,state,show,right" :
               "elm,state,show,left"), "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_RIGHT:
        edje_object_signal_emit(sd->layout, (elm_widget_mirrored_get(obj) ? "elm,state,show,left" :
               "elm,state,show,right"), "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_DOWN:
        edje_object_signal_emit(sd->layout, "elm,state,show,down", "elm");
        break;

      default:
        break;
     }

   edje_object_signal_emit(sd->bg, "elm,state,show", "elm");
   sd->on_dismiss = EINA_FALSE;

   return ECORE_CALLBACK_CANCEL;
}
//

static void
_hide_signals_emit(Evas_Object *obj,
                   Elm_Ctxpopup_Direction dir)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd->visible) return;

   switch (dir)
     {
      case ELM_CTXPOPUP_DIRECTION_UP:
        edje_object_signal_emit(sd->layout, "elm,state,hide,up", "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_LEFT:
        edje_object_signal_emit(sd->layout, (elm_widget_mirrored_get(obj) ? "elm,state,hide,right" :
               "elm,state,hide,left"), "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_RIGHT:
        edje_object_signal_emit(sd->layout, (elm_widget_mirrored_get(obj) ? "elm,state,hide,left" :
               "elm,state,hide,right"), "elm");
        break;

      case ELM_CTXPOPUP_DIRECTION_DOWN:
        edje_object_signal_emit(sd->layout, "elm,state,hide,down", "elm");
        break;

      default:
        break;
     }

   edje_object_signal_emit(sd->bg, "elm,state,hide", "elm");
}

static void
_base_shift_by_arrow(Evas_Object *obj,
                     Evas_Object *arrow,
                     Elm_Ctxpopup_Direction dir,
                     Evas_Coord_Rectangle *rect)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   Evas_Coord arrow_w, arrow_h, diff_w, diff_h;
   Evas_Coord_Rectangle shadow_left_top, shadow_right_bottom;

   evas_object_geometry_get(arrow, NULL, NULL, &arrow_w, &arrow_h);
   /* tizen only: since ctxpopup of tizen has shadow parts, start and end padding of arrow, it should be put together when shifting ctxpopup by arrow
    * so there are some differences between opensource and tizen*/
   edje_object_part_geometry_get(wd->resize_obj, "frame_shadow_left_top_padding", NULL, NULL, &shadow_left_top.w, &shadow_left_top.h);
   edje_object_part_geometry_get(wd->resize_obj, "frame_shadow_right_bottom_padding", NULL, NULL, &shadow_right_bottom.w, &shadow_right_bottom.h);
   //

   switch (dir)
     {
      case ELM_CTXPOPUP_DIRECTION_RIGHT:
        diff_w = arrow_w - shadow_right_bottom.w;
        rect->x += diff_w;
        break;

      case ELM_CTXPOPUP_DIRECTION_LEFT:
        diff_w = arrow_w - shadow_left_top.w;
        rect->x -= diff_w;
        break;

      case ELM_CTXPOPUP_DIRECTION_DOWN:
        diff_h = arrow_h - shadow_left_top.h;
        rect->y += diff_h;
        break;

      case ELM_CTXPOPUP_DIRECTION_UP:
        diff_h = arrow_h - shadow_right_bottom.h;
        rect->y -= diff_h;
        break;

      default:
         break;
     }
}

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_layout_sub_object_add_enable(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_widget_sub_object_add(Eo *obj, Elm_Ctxpopup_Data *_pd EINA_UNUSED, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;

   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_sub_object_add(sobj));

   return int_ret;
}

EOLIAN static void
_elm_ctxpopup_elm_layout_sizing_eval(Eo *obj, Elm_Ctxpopup_Data *sd)
{
   Eina_List *elist;
   Elm_Ctxpopup_Item_Data *item;
   Evas_Coord_Rectangle rect = { 0, 0, 1, 1 };
   Evas_Coord_Point box_size = { 0, 0 };
   Evas_Coord_Point _box_size = { 0, 0 };
   Evas_Coord maxw = 0;
   Evas_Coord x, y, w, h;
   const char *str;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!sd->parent || !(sd->items || sd->content)) return;

   //Box, Scroller
   EINA_LIST_FOREACH(sd->items, elist, item)
     {
        _item_sizing_eval(item);
        evas_object_size_hint_min_get(VIEW(item), &_box_size.x, &_box_size.y);
#ifndef ELM_FEATURE_WEARABLE
   //Kiran only
   if (!sd->color_calculated)
     {
        int item_r, item_g, item_b, item_a = 0;
        int bg_banded_a = 0;
        const char *item_bg_color = NULL;
        int i = 0;
        item_bg_color = edje_object_data_get(VIEW(item), "bg_color");
        if (item_bg_color != NULL)
          {
             edje_color_class_get (item_bg_color, &item_r, &item_g, &item_b, &item_a, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
             for (i = 0; i < MAX_ITEMS_PER_VIEWPORT; i++)
                {
                   sd->item_color[i][0] = item_r;
                   sd->item_color[i][1] = item_g;
                   sd->item_color[i][2] = item_b;
                   sd->item_color[i][3] = 255 - (i * 10); //10 means 255*0.04
                }
             sd->color_calculated = EINA_TRUE;
          }
     }
#endif
        str = edje_object_data_get(VIEW(item), "item_max_size");
        if (str)
          {
             maxw = atoi(str);
             maxw = maxw * elm_config_scale_get()
                             * elm_widget_scale_get(obj)
                             / edje_object_base_scale_get(VIEW(item));

             if (_box_size.x > maxw)
               {
                  edje_object_signal_emit(VIEW(item), "elm,state,text,ellipsis", "elm");
                  edje_object_message_signal_process(VIEW(item));
               }
          }

        if (!sd->horizontal)
          {
             if (_box_size.x > box_size.x)
               box_size.x = _box_size.x;
             if (_box_size.y != -1)
               box_size.y += _box_size.y;
          }
        else
          {
             if (_box_size.x != -1)
               box_size.x += _box_size.x;
             if (_box_size.y > box_size.y)
               box_size.y = _box_size.y;
          }
     }

//   if (!sd->arrow) return;  /* simple way to flag "under deletion" */

   if ((!sd->content) && (sd->scr))
     {
        elm_scroller_content_min_limit(sd->scr, EINA_TRUE, EINA_TRUE);
        evas_object_size_hint_min_set(sd->scr, box_size.x, box_size.y);
        evas_object_size_hint_min_set(sd->box, box_size.x, box_size.y);
     }

   //Base
   sd->dir = _base_geometry_calc(obj, &rect);

   _arrow_update(obj, sd->dir, rect);
   _base_shift_by_arrow(obj, sd->arrow, sd->dir, &rect);

   //resize scroller according to final size
   if ((!sd->content) && (sd->scr))
     {
        elm_scroller_content_min_limit(sd->scr, EINA_FALSE, EINA_FALSE);
        evas_object_smart_calculate(sd->scr);
     }

#ifdef ELM_FEATURE_DESKTOP
   //TIZEN ONLY(20150829): Position calculation for Tizen SDK tools is supported.
   evas_object_move(wd->resize_obj, rect.x, rect.y);
   evas_object_resize(wd->resize_obj, rect.w, rect.h);
#else
   //TIZEN ONLY(20150829): Positions of ctxpopup layout and bg are calculated.
   evas_object_size_hint_min_set(wd->resize_obj, rect.w, rect.h);
   evas_object_resize(wd->resize_obj, rect.w, rect.h);

   evas_object_resize(sd->layout, rect.w, rect.h);
   evas_object_move(sd->layout, rect.x, rect.y);

   evas_object_geometry_get(sd->parent, &x, &y, &w, &h);
   evas_object_move(sd->bg, x, y);
   evas_object_resize(sd->bg, w, h);
   //
#endif
}

static void
_on_parent_del(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   evas_object_del(data);
}

static void
_on_parent_move(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   ELM_CTXPOPUP_DATA_GET(data, sd);


   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   if (sd->visible)
     elm_layout_sizing_eval(data);
}

static void
_on_parent_resize(void *data,
                  Evas *e EINA_UNUSED,
                  Evas_Object *obj EINA_UNUSED,
                  void *event_info EINA_UNUSED)
{
   ELM_CTXPOPUP_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

   if (sd->auto_hide)
     {
        _hide_signals_emit(data, sd->dir);

        sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

        evas_object_hide(data);
        evas_object_smart_callback_call(data, SIG_DISMISSED, NULL);
     }
   else
     {
#ifdef MESH_GRADIENT
        Evas_Object *bg;
        if (edje_object_part_exists(sd->layout, "mesh_bg_changeable"))
          {
             bg = edje_object_part_swallow_get(sd->layout, "mesh_bg_changeable");
             if (bg)
               {
                  edje_object_part_unswallow(sd->layout, bg);
                  ELM_SAFE_FREE(bg, evas_object_del);
               }
          }
#endif
        if (wd->orient_mode == 90 || wd->orient_mode == 270)
         elm_widget_theme_object_set
           (data, sd->layout, "ctxpopup", "layout/landscape", elm_widget_style_get(data));
        else
         elm_widget_theme_object_set
           (data, sd->layout, "ctxpopup", "layout", elm_widget_style_get(data));

#ifdef MESH_GRADIENT
        _create_mesh_bg_image(data, sd->layout);
#endif

        if (sd->visible)
          elm_layout_sizing_eval(data);

        //TIZEN_ONLY(20150923): Using ecore idle enterer for better performance
        //_show_signals_emit(data, sd->dir);
        _show_signals_emit(data);
        //
     }
}

static void
_parent_detach(Evas_Object *obj)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd->parent) return;

   evas_object_event_callback_del_full
     (sd->parent, EVAS_CALLBACK_DEL, _on_parent_del, obj);
   evas_object_event_callback_del_full
     (sd->parent, EVAS_CALLBACK_MOVE, _on_parent_move, obj);
   evas_object_event_callback_del_full
     (sd->parent, EVAS_CALLBACK_RESIZE, _on_parent_resize, obj);
}

static void
_on_content_resized(void *data,
                    Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   elm_layout_sizing_eval(data);
}

static void
_access_outline_activate_cb(void *data,
                        Evas_Object *part_obj EINA_UNUSED,
                        Elm_Object_Item *item EINA_UNUSED)
{
   evas_object_hide(data);
   evas_object_smart_callback_call(data, SIG_DISMISSED, NULL);
}

static void
_access_obj_process(Evas_Object *obj, Eina_Bool is_access)
{
   Evas_Object *ao;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (is_access)
     {
        ao = _access_object_get(obj, ACCESS_OUTLINE_PART);
        if (!ao)
          {
             ao = _elm_access_edje_object_part_object_register
                (obj, wd->resize_obj, ACCESS_OUTLINE_PART);

             const char *style = elm_widget_style_get(obj);
             if (!strcmp(style, "more/default"))
               {
                  elm_access_info_set(ao, ELM_ACCESS_TYPE, TTS_STR_MENU_POPUP);
                  elm_access_info_set(ao, ELM_ACCESS_CONTEXT_INFO, TTS_STR_MENU_CLOSE);
               }
             else
               {
                  elm_access_info_set(ao, ELM_ACCESS_TYPE, E_("Contextual popup"));
                  elm_access_info_set(ao, ELM_ACCESS_CONTEXT_INFO, E_("Double tap to close popup"));
               }
             _elm_access_activate_callback_set
                (_elm_access_info_get(ao), _access_outline_activate_cb, obj);
          }
     }
   else
     {
        _elm_access_edje_object_part_object_unregister
               (obj, wd->resize_obj, ACCESS_OUTLINE_PART);
     }
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   edje_object_mirrored_set(sd->layout, rtl);
   edje_object_mirrored_set(sd->arrow, rtl);
   edje_object_mirrored_set(wd->resize_obj, rtl);
}

#ifdef MESH_GRADIENT
static void*
_create_shared_image_buffer(Evas_Object *obj, Evas_Object *bg)
{
   if (bg)
     {
        tbm_surface_h tbm_surface = (tbm_surface_h)evas_object_data_get(obj, "tbm_surface");

        if (tbm_surface)
          {
             int w = tbm_surface_get_width(tbm_surface);
             int h = tbm_surface_get_height(tbm_surface);
             evas_object_image_data_update_add(bg, 0, 0, w, h);
          }
     }
/*   gxt_shared_image_buffer_h handler = gxt_shared_image_buffer_initialize();
   if (!handler) return NULL;

   tbm_surface_h tbm_surface;
   int ret = gxt_shared_image_buffer_get_tbm_surface(handler, &tbm_surface);
   if (ret != GXT_ERROR_NONE)
     {
        gxt_shared_image_buffer_deinitialize(handler);
        return NULL;
     }

   tbm_surface_info_s info;
   ret = tbm_surface_map(tbm_surface, TBM_SURF_OPTION_READ, &info);
   if (ret != GXT_ERROR_NONE)
     {
        gxt_shared_image_buffer_deinitialize(handler);
        return NULL;
     }

   Evas_Coord w, h;
   Ecore_X_Window root_win = ecore_x_window_root_first_get();
   ecore_x_window_geometry_get(root_win, NULL, NULL, &w, &h);

   unsigned char *buffer = (unsigned char *)ecore_evas_data_get(ecore_evas_ecore_evas_get(evas_object_evas_get(obj)), "fixed_theme");
   if (buffer)
     {
        evas_object_image_data_set(bg, buffer);
        evas_object_image_data_update_add(bg, 0, 0, w, h);
     }
   else
     {
        unsigned char *data = (unsigned char *)evas_object_image_data_get(bg, EINA_TRUE);
        int stride = evas_object_image_stride_get(bg);
        if (data && (stride == info.planes[0].stride))
          memcpy(data, info.planes[0].ptr, info.planes[0].size);
     }

   tbm_surface_unmap(tbm_surface);
   gxt_shared_image_buffer_deinitialize(handler);
*/
   return NULL;
}

static void
_create_mesh_bg_image(Evas_Object *obj, Evas_Object *container)
{
   Evas_Object *bg = NULL;
   int changeable;

   changeable = (int)ecore_evas_data_get(ecore_evas_ecore_evas_get(evas_object_evas_get(obj)), "changeable_ui");

   if (changeable)
     {
        if (edje_object_part_exists(container, "mesh_bg_changeable"))
          {
             bg = edje_object_part_swallow_get(container, "mesh_bg_changeable");
             if (bg)
               {
                  edje_object_part_unswallow(container, bg);
                  evas_object_del(bg);
               }

             gxt_shared_image_buffer_h handler = gxt_shared_image_buffer_initialize();
             if (handler == NULL) return;

             tbm_surface_h tbm_surface;
             int result = gxt_shared_image_buffer_get_tbm_surface(handler, &tbm_surface);
             if (result != GXT_ERROR_NONE)
               {
                  gxt_shared_image_buffer_deinitialize(handler);
                  ERR("failed to get tbm surface");
                  return;
               }

             int w = tbm_surface_get_width(tbm_surface);
             int h = tbm_surface_get_height(tbm_surface);

             bg = evas_object_image_filled_add(evas_object_evas_get(obj));
             if (!bg)
               {
                  gxt_shared_image_buffer_deinitialize(handler);
                  return;
               }

             evas_object_move(bg, 0, 0);
             evas_object_resize(bg, w, h);
             evas_object_image_size_set(bg, w, h);
             evas_object_show(bg);

             Evas_Native_Surface surface;
             surface.version = EVAS_NATIVE_SURFACE_VERSION;
             surface.type = EVAS_NATIVE_SURFACE_TBM;
             surface.data.tbm.buffer = tbm_surface;
             surface.data.tbm.rot = 0;
             surface.data.tbm.ratio = 0;
             surface.data.tbm.flip = 0;
             evas_object_image_native_surface_set(bg, &surface);
             evas_object_image_data_update_add(bg, 0, 0, w, h);
//             evas_object_image_fill_set(bg, 0, 0, w, h);
  //           evas_object_image_colorspace_set(bg, EVAS_COLORSPACE_ARGB8888);

//             _create_shared_image_buffer(obj, bg);

             evas_object_data_set(container, "handler", handler);
             evas_object_data_set(container, "tbm_surface", tbm_surface);

             edje_object_part_swallow(container, "mesh_bg_changeable", bg);
          }
        edje_object_signal_emit(container, "elm,state,changeable_ui,on", "elm");
     }
   else
     {
        edje_object_signal_emit(container, "elm,state,changeable_ui,off", "elm");
     }
}

static void
_remove_mesh_bg_image(Evas_Object *obj)
{
   Evas_Object *bg = NULL;

   if (edje_object_part_exists(obj, "mesh_bg_changeable"))
     {
        bg = edje_object_part_swallow_get(obj, "mesh_bg_changeable");
        if (bg)
          {
             gxt_shared_image_buffer_h handler = evas_object_data_get(obj, "handler");
             gxt_shared_image_buffer_deinitialize(handler);
             edje_object_part_unswallow(obj, bg);
             evas_object_image_native_surface_set(bg, NULL);
             evas_object_data_set(obj, "handler", NULL);
             evas_object_data_set(obj, "tbm_surface", NULL);
             ELM_SAFE_FREE(bg, evas_object_del);
          }
     }
}

static void
_color_class_changed_cb(void *data EINA_UNUSED,
                        Evas_Object *obj,
                        const char *emission EINA_UNUSED,
                        const char *source EINA_UNUSED)
{
   Evas_Object *bg;

   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (edje_object_part_exists(sd->layout, "mesh_bg_changeable"))
     {
        bg = edje_object_part_swallow_get(sd->layout, "mesh_bg_changeable");
        if (bg)
          _create_shared_image_buffer(sd->layout, bg);
     }
}
#endif

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_widget_event(Eo *obj, Elm_Ctxpopup_Data *sd, Evas_Object *src EINA_UNUSED, Evas_Callback_Type type, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;

   // TIZEN ONLY(20131221) : When access mode, focused ui is disabled.
   if (_elm_config->access_mode) return EINA_FALSE;

   if (elm_widget_disabled_get(obj)) return EINA_FALSE;
   if (type != EVAS_CALLBACK_KEY_DOWN) return EINA_FALSE;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
   if (!_focus_enabled(obj)) return EINA_FALSE;

   //FIXME: for this key event, _elm_ctxpopup_smart_focus_next should be done first
   if ((!strcmp(ev->keyname, "Tab")) ||
       (!strcmp(ev->keyname, "ISO_Left_Tab")))
     {
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   /////
   if (((!strcmp(ev->keyname, "Left")) ||
        (!strcmp(ev->keyname, "KP_Left")) ||
        (!strcmp(ev->keyname, "Right")) ||
        (!strcmp(ev->keyname, "KP_Right")) ||
        (!strcmp(ev->keyname, "Up")) ||
        (!strcmp(ev->keyname, "KP_Up")) ||
        (!strcmp(ev->keyname, "Down")) ||
        (!strcmp(ev->keyname, "KP_Down"))) && (!ev->string))
     {
        double degree = 0.0;

        if ((!strcmp(ev->keyname, "Left")) ||
            (!strcmp(ev->keyname, "KP_Left")))
          degree = 270.0;
        else if ((!strcmp(ev->keyname, "Right")) ||
                 (!strcmp(ev->keyname, "KP_Right")))
          degree = 90.0;
        else if ((!strcmp(ev->keyname, "Up")) ||
                 (!strcmp(ev->keyname, "KP_Up")))
          degree = 0.0;
        else if ((!strcmp(ev->keyname, "Down")) ||
                 (!strcmp(ev->keyname, "KP_Down")))
          degree = 180.0;

        elm_widget_focus_direction_go(sd->box, degree);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if (((!strcmp(ev->keyname, "Home")) ||
             (!strcmp(ev->keyname, "KP_Home")) ||
             (!strcmp(ev->keyname, "Prior")) ||
             (!strcmp(ev->keyname, "KP_Prior"))) && (!ev->string))
     {
        Elm_Ctxpopup_Item_Data *it = eina_list_data_get(sd->items);
        Evas_Object *btn = it->btn;
        elm_object_focus_set(btn, EINA_TRUE);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }
   else if (((!strcmp(ev->keyname, "End")) ||
             (!strcmp(ev->keyname, "KP_End")) ||
             (!strcmp(ev->keyname, "Next")) ||
             (!strcmp(ev->keyname, "KP_Next"))) && (!ev->string))
     {
        Elm_Ctxpopup_Item_Data *it = eina_list_data_get(eina_list_last(sd->items));
        Evas_Object *btn = it->btn;
        elm_object_focus_set(btn, EINA_TRUE);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
        return EINA_TRUE;
     }

   // TIZEN ONLY : 20130530 : ctxpopup will be dismissed by user
   //if (strcmp(ev->keyname, "Escape")) return EINA_FALSE;
   return EINA_FALSE;

/*
   _hide_signals_emit(obj, sd->dir);

   ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
   return EINA_TRUE;
*/
}

//FIXME: lost the content size when theme hook is called.
EOLIAN static Eina_Bool
_elm_ctxpopup_elm_widget_theme_apply(Eo *obj, Elm_Ctxpopup_Data *sd)
{
   Eina_List *elist;
   Elm_Ctxpopup_Item_Data *item;
   int idx = 0;
   Eina_Bool rtl;
   Eina_Bool int_ret = EINA_FALSE;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);

   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_theme_apply());
   if (!int_ret) return EINA_FALSE;

   rtl = elm_widget_mirrored_get(obj);

   elm_widget_theme_object_set
     (obj, sd->bg, "ctxpopup", "bg", elm_widget_style_get(obj));


   elm_widget_theme_object_set
     (obj, sd->arrow, "ctxpopup", "arrow", elm_widget_style_get(obj));

   if (wd->orient_mode == 90 || wd->orient_mode == 270)
     elm_widget_theme_object_set
       (obj, sd->layout, "ctxpopup", "layout/landscape", elm_widget_style_get(obj));
   else
     elm_widget_theme_object_set
       (obj, sd->layout, "ctxpopup", "layout", elm_widget_style_get(obj));

#ifdef MESH_GRADIENT
   _create_mesh_bg_image(obj, sd->layout);
   elm_layout_signal_callback_add(obj, "color_class,set", "B011", _color_class_changed_cb, NULL);
#endif

   _mirrored_set(obj, rtl);

   //Items
   EINA_LIST_FOREACH(sd->items, elist, item)
     {
        edje_object_mirrored_set(VIEW(item), rtl);

        if (item->label && item->icon)
          {
             if (!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "icon_text_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "icon_text_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }
        else if (item->label)
          {
             if (!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }
        else if (item->icon)
          {
             if (!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "icon_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "icon_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }

        if (item->label)
          edje_object_part_text_set(VIEW(item), "elm.text", item->label);

        if (eo_do(EO_OBJ(item), elm_wdg_item_disabled_get()))
          edje_object_signal_emit(VIEW(item), "elm,state,disabled", "elm");

       /*
        *  For separator, if the first item has visible separator,
        *  then it should be aligned with edge of the base part.
        *  In some cases, it gives improper display. Ex) rounded corner
        *  So the first item separator should be invisible.
        */
        if ((idx++) == 0)
          edje_object_signal_emit(VIEW(item), "elm,state,default", "elm");
        else
          edje_object_signal_emit(VIEW(item), "elm,state,separator", "elm");

        // reset state of text to be default
        edje_object_signal_emit(VIEW(item), "elm,state,text,default", "elm");
        edje_object_message_signal_process(VIEW(item));
     }

   if (evas_object_visible_get(sd->bg))
     edje_object_signal_emit(sd->bg, "elm,state,show", "elm");

   if (sd->scr)
     {
        elm_object_style_set(sd->scr, "list_effect");

        if (sd->horizontal)
          elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
        else
          elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
     }

   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   if (sd->visible)
     elm_layout_sizing_eval(obj);

   /* access */
  if (_elm_config->access_mode) _access_obj_process(obj, EINA_TRUE);

   return EINA_TRUE;
}

/* kind of a big and tricky override here: an internal box will hold
 * the actual content. content aliases won't be of much help here */
EOLIAN static Eina_Bool
_elm_ctxpopup_elm_container_content_set(Eo *obj, Elm_Ctxpopup_Data *sd, const char *part, Evas_Object *content)
{
   Evas_Coord min_w = -1, min_h = -1;
   Eina_Bool int_ret = EINA_TRUE;

   if ((part) && (strcmp(part, "default")))
     {
        eo_do_super(obj, MY_CLASS, int_ret = elm_obj_container_content_set(part, content));
        return int_ret;
     }

   if (!content) return EINA_FALSE;

   if (content == sd->content) return EINA_TRUE;

   if (sd->items) elm_ctxpopup_clear(obj);
   if (sd->content) evas_object_del(sd->content);

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);

   evas_object_event_callback_add
      (sd->box, EVAS_CALLBACK_RESIZE, _on_content_resized, obj);
   edje_object_part_swallow(wd->resize_obj, "elm.swallow.content", sd->box);

   evas_object_size_hint_weight_set
     (content, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_fill_set
     (content, EVAS_HINT_FILL, EVAS_HINT_FILL);

   /* since it's going to be a box content, not a layout's... */
   evas_object_show(content);

   evas_object_size_hint_min_get(content, &min_w, &min_h);
   evas_object_size_hint_min_set(sd->box, min_w, min_h);
   elm_box_pack_end(sd->box, content);

   sd->content = content;
   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   if (sd->visible) elm_layout_sizing_eval(obj);

   return EINA_TRUE;
}

EOLIAN static Evas_Object*
_elm_ctxpopup_elm_container_content_get(Eo *obj, Elm_Ctxpopup_Data *sd, const char *part)
{

   if ((part) && (strcmp(part, "default")))
     {
        Evas_Object *ret = NULL;
        eo_do_super(obj, MY_CLASS, ret = elm_obj_container_content_get(part));
        return ret;
     }

   return sd->content;
}

EOLIAN static Evas_Object*
_elm_ctxpopup_elm_container_content_unset(Eo *obj, Elm_Ctxpopup_Data *sd, const char *part)
{
   Evas_Object *content = NULL;

   if ((part) && (strcmp(part, "default")))
     {
        eo_do_super(obj, MY_CLASS, content = elm_obj_container_content_unset(part));
        return content;
     }

   content = sd->content;
   if (!content) return content;

   elm_box_unpack(sd->box, content);
   sd->content = NULL;
   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   if (sd->visible) elm_layout_sizing_eval(obj);

   return content;
}

EOLIAN static void
_elm_ctxpopup_item_elm_widget_item_part_text_set(Eo *eo_ctxpopup_it EINA_UNUSED,
                                                 Elm_Ctxpopup_Item_Data *ctxpopup_it,
                                                 const char *part,
                                                 const char *label)
{
   if ((part) && (strcmp(part, "default"))) return;

   _item_label_set(ctxpopup_it, label);
}

EOLIAN static const char *
_elm_ctxpopup_item_elm_widget_item_part_text_get(Eo *eo_ctxpopup_it EINA_UNUSED,
                                                 Elm_Ctxpopup_Item_Data *ctxpopup_it,
                                                 const char *part)
{
   if (part && strcmp(part, "default")) return NULL;

   return ctxpopup_it->label;
}

EOLIAN static void
_elm_ctxpopup_item_elm_widget_item_part_content_set(Eo *eo_ctxpopup_it EINA_UNUSED,
                                                    Elm_Ctxpopup_Item_Data *ctxpopup_it,
                                                    const char *part,
                                                    Evas_Object *content)
{
   if ((part) && (strcmp(part, "icon"))) return;

   ELM_CTXPOPUP_DATA_GET(WIDGET(ctxpopup_it), sd);

   _item_icon_set(ctxpopup_it, content);
   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   if (sd->visible)
     elm_layout_sizing_eval(WIDGET(ctxpopup_it));
}

EOLIAN static Evas_Object *
_elm_ctxpopup_item_elm_widget_item_part_content_get(Eo *eo_ctxpopup_it EINA_UNUSED,
                                                    Elm_Ctxpopup_Item_Data *ctxpopup_it,
                                                    const char *part)
{
   if (part && strcmp(part, "icon")) return NULL;

   return ctxpopup_it->icon;
}

EOLIAN static void
_elm_ctxpopup_item_elm_widget_item_disable(Eo *eo_ctxpopup_it,
                                           Elm_Ctxpopup_Item_Data *ctxpopup_it)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(ctxpopup_it), sd);
   if (!sd) return;

   if (eo_do(eo_ctxpopup_it, elm_wdg_item_disabled_get()))
     edje_object_signal_emit(VIEW(ctxpopup_it), "elm,state,disabled", "elm");
   else
     edje_object_signal_emit(VIEW(ctxpopup_it), "elm,state,enabled", "elm");
}

EOLIAN static void
_elm_ctxpopup_item_elm_widget_item_signal_emit(Eo *eo_ctxpopup_it EINA_UNUSED,
                                               Elm_Ctxpopup_Item_Data *ctxpopup_it,
                                               const char *emission,
                                               const char *source)
{
   edje_object_signal_emit(VIEW(ctxpopup_it), emission, source);
}

EOLIAN static void
_elm_ctxpopup_item_elm_widget_item_style_set(Eo *eo_item EINA_UNUSED,
                                             Elm_Ctxpopup_Item_Data *item,
                                             const char *style)
{
   ELM_CTXPOPUP_DATA_GET(WIDGET(item), sd);

   if (item->icon && item->label)
     {
        if (sd->horizontal)
          _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "icon_text_style_item_horizontal", style);
        else
          _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "icon_text_style_item", style);
     }
   else if (item->label)
     {
        if (sd->horizontal)
          _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "text_style_item_horizontal", style);
        else
          _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "text_style_item", style);
     }
   else if (item->icon)
     {
        if (sd->horizontal)
          _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "icon_style_item_horizontal", style);
        else
          _elm_theme_object_set(WIDGET(item), VIEW(item), "ctxpopup", "icon_style_item", style);
     }

   if (sd->visible) elm_layout_sizing_eval(WIDGET(item));
}

static void
_bg_clicked_cb(void *data,
               Evas_Object *obj EINA_UNUSED,
               const char *emission EINA_UNUSED,
               const char *source EINA_UNUSED)
{
   ELM_CTXPOPUP_DATA_GET(data, sd);

   _hide_signals_emit(data, sd->dir);
}

static void
_on_show(void *data EINA_UNUSED,
         Evas *e EINA_UNUSED,
         Evas_Object *obj,
         void *event_info EINA_UNUSED)
{
   Eina_List *elist;
   Elm_Ctxpopup_Item_Data *item;
   int idx = 0;

   ELM_CTXPOPUP_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if ((!sd->items) && (!sd->content)) return;

   sd->visible = EINA_TRUE;
   sd->on_dismiss = EINA_FALSE;

   elm_layout_signal_emit(obj, "elm,state,show", "elm");

   if (wd->orient_mode == 90 || wd->orient_mode == 270)
     elm_widget_theme_object_set
       (obj, sd->layout, "ctxpopup", "layout/landscape", elm_widget_style_get(obj));
   else
     elm_widget_theme_object_set
       (obj, sd->layout, "ctxpopup", "layout", elm_widget_style_get(obj));

   EINA_LIST_FOREACH(sd->items, elist, item)
     {
        if (item->label && !item->icon)
          {
             if (!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }
        else if (item->label)
          {
             if (!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "label_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "label_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }
        else if (item->icon)
          {
             if (!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "icon_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "icon_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }

        if (idx++ == 0)
          edje_object_signal_emit(VIEW(item), "elm,state,default", "elm");
        else
          edje_object_signal_emit(VIEW(item), "elm,state,separator", "elm");
     }

   elm_layout_sizing_eval(obj);

#ifndef ELM_FEATURE_WEARABLE
   const char *color= NULL;
   int i = 0;
   Elm_Ctxpopup_Item_Data *it;

   EINA_LIST_FOREACH(sd->items, elist, it)
     {
        if (i < MAX_ITEMS_PER_VIEWPORT)
          {
             color = edje_object_data_get(VIEW(it), "bg_color");
             edje_object_color_class_set(VIEW(it), color,
                                         sd->item_color[i][0], sd->item_color[i][1], sd->item_color[i][2], sd->item_color[i][3],
                                         255, 255, 255, 255, 255, 255, 255, 255);
             i++;
          }
     }
#endif

   elm_object_focus_set(obj, EINA_TRUE);

   //TIZEN_ONLY(20150923): Using ecore idle enterer for better performance
   //_show_signals_emit(obj, sd->dir);
   if (sd->idle_enterer)
     {
        ecore_idle_enterer_del(sd->idle_enterer);
        sd->idle_enterer = NULL;
     }
   sd->idle_enterer = ecore_idle_enterer_add(_show_signals_emit, obj);
   //
}

EOLIAN static void
_elm_ctxpopup_item_elm_widget_item_focus_set(Eo *eo_ctxpopup_it EINA_UNUSED,
                                             Elm_Ctxpopup_Item_Data *ctxpopup_it,
                                             Eina_Bool focused)
{
   elm_object_focus_set(ctxpopup_it->btn, focused);
}

EOLIAN static Eina_Bool
_elm_ctxpopup_item_elm_widget_item_focus_get(Eo *eo_ctxpopup_it EINA_UNUSED,
                                             Elm_Ctxpopup_Item_Data *ctxpopup_it)
{
   return elm_object_focus_get(ctxpopup_it->btn);
}

static void
_on_hide(void *data EINA_UNUSED,
         Evas *e EINA_UNUSED,
         Evas_Object *obj,
         void *event_info EINA_UNUSED)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (!sd->visible) return;
   sd->on_dismiss = EINA_FALSE;

   sd->visible = EINA_FALSE;
}

static void
_on_move(void *data EINA_UNUSED,
         Evas *e EINA_UNUSED,
         Evas_Object *obj,
         void *event_info EINA_UNUSED)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   if (sd->visible) elm_layout_sizing_eval(obj);
}

static void
_hide_finished_cb(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   evas_object_hide(data);
   evas_object_smart_callback_call(data, SIG_DISMISSED, NULL);
}

static void
_list_del(Eo *obj, Elm_Ctxpopup_Data *sd)
{
   if (!sd->scr) return;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   edje_object_part_unswallow(wd->resize_obj, sd->scr);
   evas_object_del(sd->scr);
   sd->scr = NULL;
   evas_object_del(sd->box);
   sd->box = NULL;
}

static void
scroll_up_down_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
#ifndef ELM_FEATURE_WEARABLE
   ELM_CTXPOPUP_DATA_GET(data, sd);
   if (!sd) return;
   Evas_Coord x, y, w, h;
   Evas_Coord x2, y2, w2, h2;
   const char *color = NULL;
   int i = 0;
   Eina_List *elist;
   Elm_Ctxpopup_Item_Data *item , *top_drawn_item = NULL;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   evas_object_geometry_get(wd->resize_obj, &x2, &y2, &w2, &h2);
   EINA_LIST_FOREACH(sd->items, elist, item) {
     evas_object_geometry_get(VIEW(item), &x, &y, &w, &h);
     if ( y <= y2 && ELM_RECTS_INTERSECT(x,y,w,h,x2,y2,w2,h2))
       top_drawn_item = item;
     if( top_drawn_item && ( top_drawn_item != sd->top_drawn_item  )) {
       if( i < MAX_ITEMS_PER_VIEWPORT ) {
          color = edje_object_data_get(VIEW(item), "bg_color");
          edje_object_color_class_set(VIEW(item), color, sd->item_color[i][0], sd->item_color[i][1], sd->item_color[i][2], sd->item_color[i][3],
                                      255, 255, 255, 255, 255, 255, 255, 255);
          i++;
       }
     }
   }
   sd->top_drawn_item = top_drawn_item;
#endif
}

static void
_list_new(Evas_Object *obj)
{
   ELM_CTXPOPUP_DATA_GET(obj, sd);
   if (!sd) return;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);
   //scroller
   sd->scr = elm_scroller_add(obj);
   elm_object_style_set(sd->scr, "list_effect");
   evas_object_size_hint_align_set(sd->scr, EVAS_HINT_FILL, EVAS_HINT_FILL);
#ifndef ELM_FEATURE_WEARABLE
   evas_object_smart_callback_add(sd->scr, "scroll,up", scroll_up_down_cb, obj);
   evas_object_smart_callback_add(sd->scr, "scroll,down", scroll_up_down_cb, obj);
#endif
   if (sd->horizontal)
     elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
   else
     elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);

   edje_object_part_swallow(wd->resize_obj, "elm.swallow.content", sd->scr);

   elm_object_content_set(sd->scr, sd->box);
   elm_ctxpopup_horizontal_set(obj, sd->horizontal);
}

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_widget_disable(Eo *obj, Elm_Ctxpopup_Data *sd)
{
   Eina_List *l;
   Elm_Ctxpopup_Item_Data *it;
   Eina_Bool int_ret = EINA_FALSE;

   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_disable());
   if (!int_ret) return EINA_FALSE;

   EINA_LIST_FOREACH(sd->items, l, it)
     elm_object_item_disabled_set(EO_OBJ(it), elm_widget_disabled_get(obj));

   return EINA_TRUE;
}

EOLIAN static void
_elm_ctxpopup_evas_object_smart_add(Eo *obj, Elm_Ctxpopup_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   eo_do_super(obj, MY_CLASS, evas_obj_smart_add());

   if (!elm_widget_sub_object_parent_add(obj))
     ERR("could not add %p as sub object of %p", obj, elm_object_parent_widget_get(obj));

   elm_layout_theme_set(obj, "ctxpopup", "base", elm_widget_style_get(obj));
   elm_layout_signal_callback_add
     (obj, "elm,action,hide,finished", "", _hide_finished_cb, obj);

   //Background
   priv->bg = edje_object_add(evas_object_evas_get(obj));
   elm_widget_theme_object_set(obj, priv->bg, "ctxpopup", "bg", "default");
   edje_object_signal_callback_add
     (priv->bg, "elm,action,click", "", _bg_clicked_cb, obj);
   evas_object_smart_member_add(priv->bg, obj);
   evas_object_stack_below(priv->bg, wd->resize_obj);
#ifndef ELM_FEATURE_WEARABLE
   priv->color_calculated = EINA_FALSE;
   priv->top_drawn_item = NULL;
#endif
   //Arrow
   priv->arrow = edje_object_add(evas_object_evas_get(obj));
   elm_widget_theme_object_set
     (obj, priv->arrow, "ctxpopup", "arrow", "default");

   priv->dir_priority[0] = ELM_CTXPOPUP_DIRECTION_UP;
   priv->dir_priority[1] = ELM_CTXPOPUP_DIRECTION_LEFT;
   priv->dir_priority[2] = ELM_CTXPOPUP_DIRECTION_RIGHT;
   priv->dir_priority[3] = ELM_CTXPOPUP_DIRECTION_DOWN;
   priv->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   priv->auto_hide = EINA_TRUE;
   priv->mouse_down = EINA_FALSE;
   priv->multi_down = 0;

   priv->box = elm_box_add(obj);
   evas_object_size_hint_weight_set
     (priv->box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   priv->layout = edje_object_add(evas_object_evas_get(obj));
   if (wd->orient_mode == 90 || wd->orient_mode == 270)
     elm_widget_theme_object_set(obj, priv->layout, "ctxpopup", "layout/landscape", "default");
   else
     elm_widget_theme_object_set(obj, priv->layout, "ctxpopup", "layout", "default");
   evas_object_smart_member_add(priv->layout, obj);

   edje_object_signal_callback_add
     (priv->layout, "elm,action,hide,finished", "", _hide_finished_cb, obj);
   edje_object_part_swallow(priv->layout, "swallow", wd->resize_obj);
   evas_object_size_hint_weight_set
     (priv->layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

   evas_object_event_callback_add(obj, EVAS_CALLBACK_SHOW, _on_show, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_HIDE, _on_hide, NULL);
   evas_object_event_callback_add(obj, EVAS_CALLBACK_MOVE, _on_move, NULL);

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   elm_widget_can_focus_set(obj, EINA_TRUE);
   elm_ctxpopup_hover_parent_set(obj, elm_object_parent_widget_get(obj));
   /* access */
   if (_elm_config->access_mode) _access_obj_process(obj, EINA_TRUE);

   /* access: parent could be any object such as elm_list which does
      not know elc_ctxpopup as its child object in the focus_next(); */

   wd->highlight_root = EINA_TRUE;

   //Tizen Only: This should be removed when eo is applied.
   wd->on_create = EINA_FALSE;
}

EOLIAN static void
_elm_ctxpopup_evas_object_smart_del(Eo *obj, Elm_Ctxpopup_Data *sd)
{
   evas_object_event_callback_del_full
     (sd->box, EVAS_CALLBACK_RESIZE, _on_content_resized, obj);
   _parent_detach(obj);

   if (sd->items)
     {
        _items_remove(sd);
        _list_del(obj, sd);
     }
   else
     {
        evas_object_del(sd->box);
        sd->box = NULL;
     }

   evas_object_del(sd->arrow);
   sd->arrow = NULL; /* stops _sizing_eval() from going on on deletion */

#ifdef MESH_GRADIENT
   _remove_mesh_bg_image(sd->layout);
#endif
   evas_object_del(sd->bg);
   sd->bg = NULL;

   evas_object_del(sd->layout);
   sd->layout = NULL;

   //TIZEN_ONLY(20150923): Using ecore idle enterer for better performance
   if (sd->idle_enterer)
     {
        ecore_idle_enterer_del(sd->idle_enterer);
        sd->idle_enterer = NULL;
     }
   //

   eo_do_super(obj, MY_CLASS, evas_obj_smart_del());
}

EOLIAN static void
_elm_ctxpopup_elm_widget_parent_set(Eo *obj, Elm_Ctxpopup_Data *_pd EINA_UNUSED, Evas_Object *parent)
{
   //default parent is to be hover parent
   elm_ctxpopup_hover_parent_set(obj, parent);
}

/*
static void
_elm_ctxpopup_smart_access(Evas_Object *obj, Eina_Bool is_access)
{
   ELM_CTXPOPUP_CHECK(obj);

   _access_obj_process(obj, is_access);

   evas_object_smart_callback_call(obj, SIG_ACCESS_CHANGED, NULL);
}

static Evas_Object *
_elm_ctxpopup_smart_access_object_get(Evas_Object *obj, char *part)
{
   ELM_CTXPOPUP_CHECK(obj) NULL;

   return _access_object_get(obj, part);
}
*/

EOLIAN static void
_elm_ctxpopup_class_constructor(Eo_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

EAPI Evas_Object *
elm_ctxpopup_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = eo_add(MY_CLASS, parent);

   /* access: parent could be any object such as elm_list which does
      not know elc_ctxpopup as its child object in the focus_next(); */
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);
   wd->highlight_root = EINA_TRUE;

   return obj;
}

EOLIAN static void
_elm_ctxpopup_eo_base_constructor(Eo *obj, Elm_Ctxpopup_Data *_pd EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME_LEGACY),
         evas_obj_smart_callbacks_descriptions_set(_smart_callbacks),
         elm_interface_atspi_accessible_role_set(ELM_ATSPI_ROLE_POPUP_MENU));
}


EOLIAN static void
_elm_ctxpopup_hover_parent_set(Eo *obj, Elm_Ctxpopup_Data *sd, Evas_Object *parent)
{
   Evas_Coord x, y, w, h;

   if (!parent) return;

   _parent_detach(obj);

   evas_object_event_callback_add
     (parent, EVAS_CALLBACK_DEL, _on_parent_del, obj);
   evas_object_event_callback_add
     (parent, EVAS_CALLBACK_MOVE, _on_parent_move, obj);
   evas_object_event_callback_add
     (parent, EVAS_CALLBACK_RESIZE, _on_parent_resize, obj);

   sd->parent = parent;

   //Update Background
   evas_object_geometry_get(parent, &x, &y, &w, &h);
   evas_object_move(sd->bg, x, y);
   evas_object_resize(sd->bg, w, h);

   if (sd->visible) elm_layout_sizing_eval(obj);
}

EOLIAN static Evas_Object*
_elm_ctxpopup_hover_parent_get(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd)
{
   return sd->parent;
}

EOLIAN static void
_elm_ctxpopup_clear(Eo *obj, Elm_Ctxpopup_Data *sd)
{
   _items_remove(sd);

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   elm_object_content_unset(sd->scr);
   edje_object_part_unswallow(wd->resize_obj, sd->scr);
   evas_object_del(sd->scr);
   sd->scr = NULL;
   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;
}

EOLIAN static void
_elm_ctxpopup_horizontal_set(Eo *obj, Elm_Ctxpopup_Data *sd, Eina_Bool horizontal)
{
   Eina_List *elist;
   Elm_Ctxpopup_Item_Data *item;
   int idx = 0;

   sd->horizontal = !!horizontal;

   if (!sd->scr)
      return;

  if (!horizontal)
     {
        elm_box_horizontal_set(sd->box, EINA_FALSE);
        elm_scroller_bounce_set(sd->scr, EINA_FALSE, EINA_TRUE);
        elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_OFF, ELM_SCROLLER_POLICY_AUTO);
     }
   else
     {
        elm_box_horizontal_set(sd->box, EINA_TRUE);
        elm_scroller_bounce_set(sd->scr, EINA_TRUE, EINA_FALSE);
        elm_scroller_policy_set(sd->scr, ELM_SCROLLER_POLICY_AUTO, ELM_SCROLLER_POLICY_OFF);
     }

   EINA_LIST_FOREACH(sd->items, elist, item)
     {
        if (item->label && !item->icon)
          {
             if (!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "text_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }
        else if (item->label)
          {
             if (!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "label_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "label_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }
        else if (item->icon)
          {
             if (!sd->horizontal)
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "icon_style_item",
                                     elm_widget_style_get(obj));
             else
               _elm_theme_object_set(obj, VIEW(item), "ctxpopup",
                                     "icon_style_item_horizontal",
                                     elm_widget_style_get(obj));
          }
        if (idx++ == 0)
          edje_object_signal_emit(VIEW(item), "elm,state,default", "elm");
        else
          edje_object_signal_emit(VIEW(item), "elm,state,separator", "elm");

        eo_do(EO_OBJ(item), elm_wdg_item_disable());
     }

   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   if (sd->visible) elm_layout_sizing_eval(obj);
}

EOLIAN static Eina_Bool
_elm_ctxpopup_horizontal_get(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd)
{
   return sd->horizontal;
}

EOLIAN static void
_elm_ctxpopup_item_eo_base_constructor(Eo *obj, Elm_Ctxpopup_Item_Data *it)
{
   eo_do_super(obj, ELM_CTXPOPUP_ITEM_CLASS, eo_constructor());
   it->base = eo_data_scope_get(obj, ELM_WIDGET_ITEM_CLASS);
//TIZEN ONLY(20150710)ctxpopup: Accessible methods for children_get, extents_get and item name_get
   eo_do(obj, elm_interface_atspi_accessible_role_set(ELM_ATSPI_ROLE_LIST_ITEM));
//
}

EOLIAN static void
_elm_ctxpopup_item_eo_base_destructor(Eo *eo_ctxpopup_it,
                                      Elm_Ctxpopup_Item_Data *ctxpopup_it EINA_UNUSED)
{
   eo_do_super(eo_ctxpopup_it, ELM_CTXPOPUP_ITEM_CLASS, eo_destructor());
}

EOLIAN static Elm_Object_Item*
_elm_ctxpopup_item_append(Eo *obj, Elm_Ctxpopup_Data *sd, const char *label, Evas_Object *icon, Evas_Smart_Cb func, const void *data)
{
   Elm_Ctxpopup_Item_Data *it;
   Evas_Object *content, *focus_bt;
   int idx = 0;
   Eina_List *elist;
   Eo *eo_item;

   eo_item = eo_add(ELM_CTXPOPUP_ITEM_CLASS, obj, elm_obj_ctxpopup_item_init(func, data));
   if (!eo_item) return NULL;

   ELM_CTXPOPUP_ITEM_DATA_GET(eo_item, item);

   //The first item is appended.
   content = elm_object_content_unset(obj);
   if (content) evas_object_del(content);

   if (!sd->items)
     _list_new(obj);

   if (icon && label)
     {
        if (!sd->horizontal)
          _item_new(item, "icon_text_style_item");
        else
          _item_new(item, "icon_text_style_item_horizontal");
     }
   else if (label)
     {
        if (!sd->horizontal)
          _item_new(item, "text_style_item");
        else
          _item_new(item, "text_style_item_horizontal");
     }
   else
     {
        if (!sd->horizontal)
          _item_new(item, "icon_style_item");
        else
          _item_new(item, "icon_style_item_horizontal");
     }

   _item_icon_set(item, icon);
   _item_label_set(item, label);
   focus_bt = _item_in_focusable_button(item);
   elm_box_pack_end(sd->box, focus_bt);
   sd->items = eina_list_append(sd->items, item);
   item->btn = focus_bt;

   sd->dir = ELM_CTXPOPUP_DIRECTION_UNKNOWN;

   if (sd->visible)
     {
        EINA_LIST_FOREACH(sd->items, elist, it)
          {
             if (idx++ == 0)
               edje_object_signal_emit(VIEW(it), "elm,state,default", "elm");
             else
               edje_object_signal_emit(VIEW(it), "elm,state,separator", "elm");
          }
        elm_layout_sizing_eval(obj);
     }

   if (_elm_config->access_mode) _access_focusable_button_register(focus_bt, item);

   return EO_OBJ(item);
}

EOLIAN static void
_elm_ctxpopup_direction_priority_set(Eo *obj, Elm_Ctxpopup_Data *sd, Elm_Ctxpopup_Direction first, Elm_Ctxpopup_Direction second, Elm_Ctxpopup_Direction third, Elm_Ctxpopup_Direction fourth)
{
   sd->dir_priority[0] = first;
   sd->dir_priority[1] = second;
   sd->dir_priority[2] = third;
   sd->dir_priority[3] = fourth;

   if (sd->visible) elm_layout_sizing_eval(obj);
}

EOLIAN static void
_elm_ctxpopup_direction_priority_get(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd, Elm_Ctxpopup_Direction *first, Elm_Ctxpopup_Direction *second, Elm_Ctxpopup_Direction *third, Elm_Ctxpopup_Direction *fourth)
{
   if (first) *first = sd->dir_priority[0];
   if (second) *second = sd->dir_priority[1];
   if (third) *third = sd->dir_priority[2];
   if (fourth) *fourth = sd->dir_priority[3];
}

EOLIAN static Elm_Ctxpopup_Direction
_elm_ctxpopup_direction_get(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd)
{
   return sd->dir;
}

EAPI Eina_Bool
elm_ctxpopup_direction_available_get(Evas_Object *obj, Elm_Ctxpopup_Direction direction)
{
   ELM_CTXPOPUP_CHECK(obj) EINA_FALSE;
   ELM_CTXPOPUP_DATA_GET(obj, sd);

   elm_layout_sizing_eval(obj);

   if (sd->dir == direction) return EINA_TRUE;
   return EINA_FALSE;
}

EOLIAN static void
_elm_ctxpopup_dismiss(Eo *obj, Elm_Ctxpopup_Data *sd)
{
   if (sd->on_dismiss) return;
   sd->on_dismiss = EINA_TRUE;
   _hide_signals_emit(obj, sd->dir);
}

EOLIAN static void
_elm_ctxpopup_auto_hide_disabled_set(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd, Eina_Bool disabled)
{
   disabled = !!disabled;
   if (sd->auto_hide == !disabled) return;
   sd->auto_hide = !disabled;
}

EOLIAN static void
_elm_ctxpopup_item_init(Eo *eo_item,
          Elm_Ctxpopup_Item_Data *item,
          Evas_Smart_Cb func,
          const void *data)
{
   Eo *obj = eo_do(eo_item, eo_parent_get());

   item->wcb.org_func_cb = func;
   item->wcb.org_data = data;
   item->wcb.cobj = obj;
}

EOLIAN static const Elm_Atspi_Action*
_elm_ctxpopup_elm_interface_atspi_widget_action_elm_actions_get(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd EINA_UNUSED)
{
   static Elm_Atspi_Action atspi_actions[] = {
          { "escape", "escape", NULL, _key_action_escape},
          { "move,previous", "move", "previous", _key_action_move},
          { "move,next", "move", "next", _key_action_move},
          { "move,left", "move", "left", _key_action_move},
          { "move,right", "move", "right", _key_action_move},
          { "move,up", "move", "up", _key_action_move},
          { "move,down", "move", "down", _key_action_move},
          { NULL, NULL, NULL, NULL }
   };
   return &atspi_actions[0];
}

EOLIAN static Elm_Atspi_State_Set
_elm_ctxpopup_elm_interface_atspi_accessible_state_set_get(Eo *obj, Elm_Ctxpopup_Data *sd EINA_UNUSED)
{
   Elm_Atspi_State_Set ret;
   eo_do_super(obj, MY_CLASS, ret = elm_interface_atspi_accessible_state_set_get());

   STATE_TYPE_SET(ret, ELM_ATSPI_STATE_MODAL);

   return ret;
}
//TIZEN ONLY(20150710): ctxpopup: Accessible methods for children_get, extents_get and item name_get
EOLIAN Eina_List*
_elm_ctxpopup_elm_interface_atspi_accessible_children_get(Eo *eo_item EINA_UNUSED, Elm_Ctxpopup_Data *sd)
{
   Eina_List *ret = NULL;
   Eina_List *l = NULL;
   Elm_Ctxpopup_Item_Data *it;

   EINA_LIST_FOREACH(sd->items, l, it)
      ret = eina_list_append(ret, EO_OBJ(it));

   return ret;
}

EOLIAN static void
_elm_ctxpopup_elm_interface_atspi_component_extents_get(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd, Eina_Bool screen_coords, int *x, int *y, int *w, int *h)
{
   int ee_x, ee_y;

   if (!sd->scr)
     {
        if (x) *x = -1;
        if (y) *y = -1;
        if (w) *w = -1;
        if (h) *h = -1;
        return;
     }
   evas_object_geometry_get(sd->scr, x, y, w, h);

   if (screen_coords)
     {
        Ecore_Evas *ee = ecore_evas_ecore_evas_get(evas_object_evas_get(sd->scr));
        if (!ee) return;
        ecore_evas_geometry_get(ee, &ee_x, &ee_y, NULL, NULL);
        if (x) *x += ee_x;
        if (y) *y += ee_y;
     }

}
//
//TIZEN ONLY(20150708): popup and ctxpopup accessibility highlight impementation
EOLIAN static Eina_Bool
_elm_ctxpopup_elm_interface_atspi_component_highlight_grab(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd)
{
   if (sd->scr)
     {
        elm_object_accessibility_highlight_set(sd->scr, EINA_TRUE);
        ///TIZEN_ONLY(20170717) : expose highlight information on atspi
        elm_interface_atspi_accessible_state_changed_signal_emit(obj, ELM_ATSPI_STATE_HIGHLIGHTED, EINA_TRUE);
        ///
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

EOLIAN static Eina_Bool
_elm_ctxpopup_elm_interface_atspi_component_highlight_clear(Eo *obj EINA_UNUSED, Elm_Ctxpopup_Data *sd)
{
   if (sd->scr)
     {
        elm_object_accessibility_highlight_set(sd->scr, EINA_TRUE);
        ///TIZEN_ONLY(20170717) : expose highlight information on atspi
        elm_interface_atspi_accessible_state_changed_signal_emit(obj, ELM_ATSPI_STATE_HIGHLIGHTED, EINA_FALSE);
        ///
        return EINA_TRUE;
     }
   return EINA_FALSE;
}
//

//TIZEN ONLY(20150710)ctxpopup: Accessible methods for children_get, extents_get and item name_get
EOLIAN char *
_elm_ctxpopup_item_elm_interface_atspi_accessible_name_get(Eo *eo_it EINA_UNUSED, Elm_Ctxpopup_Item_Data *item)
{
   return item->label ? strdup(item->label) : NULL;
}
//

static Eina_Bool
_key_action_escape(Evas_Object *obj, const char *params EINA_UNUSED)
{
   elm_ctxpopup_dismiss(obj);
   return EINA_TRUE;
}

#include "elm_ctxpopup_item.eo.c"
#include "elm_ctxpopup.eo.c"
