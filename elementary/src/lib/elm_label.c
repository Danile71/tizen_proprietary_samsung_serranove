#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_layout.h"
#include "elm_widget_label.h"
#include "ttrace.h"

#define MY_CLASS ELM_LABEL_CLASS

#define MY_CLASS_NAME "Elm_Label"
#define MY_CLASS_NAME_LEGACY "elm_label"

static const char SIG_SLIDE_END[] = "slide,end";
static const char SIG_ANCHOR_CLICKED[] = "anchor,clicked";
static const char SIG_ANCHOR_MOUSE_DOWN[] = "anchor,mouse,down";
static const char SIG_ANCHOR_MOUSE_UP[] = "anchor,mouse,up";
static const char SIG_CLICKED[] = "clicked";
static const char SIG_LONGPRESSED[] = "longpressed";

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_SLIDE_END, ""},
   {SIG_ANCHOR_CLICKED, ""},
   {SIG_ANCHOR_MOUSE_DOWN, ""},
   {SIG_ANCHOR_MOUSE_UP, ""},
   {SIG_CLICKED, ""},
   {SIG_LONGPRESSED, ""},
   {NULL, NULL}
};

static const Elm_Layout_Part_Alias_Description _text_aliases[] =
{
   {"default", "elm.text"},
   {NULL, NULL}
};

/////////////////////////////////////////////////////////////////
// TIZEN_ONLY(20150526): Support item with image path. ////////////
/////////////////////////////////////////////////////////////////
static void
_elm_label_anchors_clear(Evas_Object *obj, Eina_Bool free_cursors)
{
   Elm_Label_Anchor_Data *an;

   ELM_LABEL_DATA_GET(obj, sd);

   if (sd->on_clear)
     return;
   sd->on_clear = EINA_TRUE;

   while (sd->anchors)
     {
        an = sd->anchors->data;

        if (free_cursors)
          {
             evas_textblock_cursor_free(an->start);
             an->start = NULL;
             evas_textblock_cursor_free(an->end);
             an->end = NULL;
          }

        while (an->sel)
          {
             Sel *sel = an->sel->data;
             if (sel->obj) evas_object_del(sel->obj);
             free(sel);
             an->sel = eina_list_remove_list(an->sel, an->sel);
          }
        free(an->name);
        free(an);
        sd->anchors = eina_list_remove_list(sd->anchors, sd->anchors);
     }

   sd->on_clear = EINA_FALSE;
}

static Eina_Bool
_long_press_cb(void *data)
{
   ELM_LABEL_DATA_GET(data, sd);

   sd->longpress_timer = NULL;
   if (sd->mouse_down_flag)
     evas_object_smart_callback_call(data, SIG_LONGPRESSED, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static void
_label_mouse_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;

   ELM_LABEL_DATA_GET(data, sd);

   sd->mouse_down_flag = EINA_TRUE;
   sd->down_x = ev->canvas.x;
   sd->down_y = ev->canvas.y;
   if (ev->button == 1)
     {
        if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = ecore_timer_add
           (_elm_config->longpress_timeout, _long_press_cb, data);
     }
}

static void
_label_mouse_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;

   ELM_LABEL_DATA_GET(data, sd);

   if (ev->button == 1 && sd->longpress_timer)
     {
        ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = NULL;
     }

   if (sd->mouse_down_flag)
     evas_object_smart_callback_call(data, SIG_CLICKED, NULL);
   sd->mouse_down_flag = EINA_FALSE;
}

static void
_label_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord mx, my;
   Evas_Coord x, y, w, h;

   ELM_LABEL_DATA_GET(data, sd);

   if (!sd->mouse_down_flag)
     return;

   evas_object_geometry_get(data, &x, &y, &w, &h);

   if (ev->cur.canvas.x > sd->down_x)
     mx = ev->cur.canvas.x - sd->down_x;
   else
     mx = sd->down_x - ev->cur.canvas.x;

   if (ev->cur.canvas.y > sd->down_y)
     my = ev->cur.canvas.y - sd->down_y;
   else
     my = sd->down_y - ev->cur.canvas.y;

   if ((mx > _elm_config->finger_size) ||
       (my > _elm_config->finger_size))
     {
        sd->mouse_down_flag = EINA_FALSE;
        return;
     }

   if ((ev->cur.canvas.x < x) || (ev->cur.canvas.x > x + w) ||
       (ev->cur.canvas.y < y) || (ev->cur.canvas.y > y + h))
     sd->mouse_down_flag = EINA_FALSE;
}

static void
_label_anchor_mouse_down_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Elm_Label_Anchor_Data *an = (Elm_Label_Anchor_Data *)data;
   Evas_Event_Mouse_Down *ev = event_info;
   Elm_Label_Anchor_Info ai;

   an->mouse_down_flag = EINA_TRUE;
   ai.name = an->name;
   an->down_x = ev->canvas.x;
   an->down_y = ev->canvas.y;

   evas_object_smart_callback_call(an->obj, SIG_ANCHOR_MOUSE_DOWN, &ai);
}

static void
_label_anchor_mouse_up_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Elm_Label_Anchor_Data *an = (Elm_Label_Anchor_Data *)data;
   Elm_Label_Anchor_Info ai;

   ai.name = an->name;
   evas_object_smart_callback_call(an->obj, SIG_ANCHOR_MOUSE_UP, &ai);
   if (an->mouse_down_flag)
     evas_object_smart_callback_call(an->obj, SIG_ANCHOR_CLICKED, &ai);
   an->mouse_down_flag = EINA_FALSE;
}

static void
_label_anchor_mouse_move_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Elm_Label_Anchor_Data *an = (Elm_Label_Anchor_Data *)data;
   Evas_Coord mx, my;

   if (!an->mouse_down_flag)
     return;

   if (ev->cur.canvas.x > an->down_x)
     mx = ev->cur.canvas.x - an->down_x;
   else
     mx = an->down_x - ev->cur.canvas.x;

   if (ev->cur.canvas.y > an->down_y)
     my = ev->cur.canvas.y - an->down_y;
   else
     my = an->down_y - ev->cur.canvas.y;

   if ((mx > _elm_config->finger_size) ||
       (my > _elm_config->finger_size))
     {
        an->mouse_down_flag = EINA_FALSE;
        return;
     }
}

static void
_label_anchors_show(void *data,
                    Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   Eina_List *l, *ll;
   Elm_Label_Anchor_Data *an;
   Evas_Object *label_obj;
   Sel *sel;

   label_obj = (Evas_Object *)data;
   ELM_LABEL_DATA_GET(label_obj, sd);

   EINA_LIST_FOREACH(sd->anchors, l, an)
     {
        EINA_LIST_FOREACH(an->sel, ll, sel)
          {
             if (an->item)
               {
                  evas_object_show(sel->obj);
               }
          }
     }
}

static void
_label_anchors_hide(void *data,
                    Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   Eina_List *l, *ll;
   Elm_Label_Anchor_Data *an;
   Evas_Object *label_obj;
   Sel *sel;

   label_obj = (Evas_Object *)data;
   ELM_LABEL_DATA_GET(label_obj, sd);

   EINA_LIST_FOREACH(sd->anchors, l, an)
     {
        EINA_LIST_FOREACH(an->sel, ll, sel)
          {
             if (an->item)
               {
                  evas_object_hide(sel->obj);
               }
          }
     }
}

static void
_label_anchors_move(void *data,
                    Evas *e EINA_UNUSED,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   Evas_Coord x, y, w, h;
   Eina_List *l, *ll;
   Evas_Object *tb;
   Elm_Label_Anchor_Data *an;
   Sel *sel;

   ELM_LABEL_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

   tb = (Evas_Object *)edje_object_part_object_get(wd->resize_obj, "elm.text");

   evas_object_geometry_get(tb, &x, &y, &w, &h);

   EINA_LIST_FOREACH(sd->anchors, l, an)
     {
        EINA_LIST_FOREACH(an->sel, ll, sel)
          {
             if (an->item)
               {
                  Evas_Coord cx, cy, cw, ch;

                  if (!evas_textblock_cursor_format_item_geometry_get
                      (an->start, &cx, &cy, &cw, &ch))
                    continue;
                  evas_object_move(sel->obj, x + cx, y + cy);
                  evas_object_resize(sel->obj, cw, ch);
               }
             else
               {
                  if (sel->obj)
                    {
                       evas_object_move(sel->obj, x + sel->rect.x, y + sel->rect.y);
                       evas_object_resize(sel->obj, sel->rect.w, sel->rect.h);
                    }
               }
          }
     }
}

static Evas_Object *
_elm_label_item_get(Evas_Object *obj, char *name)
{
   Eina_List *l;
   Evas_Object *item_obj, *tb;
   Elm_Label_Item_Provider *ip;
   Evas *e;

   ELM_LABEL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, NULL);

   tb = (Evas_Object *)edje_object_part_object_get(wd->resize_obj, "elm.text");
   e = evas_object_evas_get(tb);

   EINA_LIST_FOREACH(sd->item_providers, l, ip)
     {
        item_obj = ip->func(ip->data, obj, name);
        if (item_obj) return item_obj;
     }

   if (!strncmp(name, "file://", 7))
     {
        const char *fname = name + 7;

        item_obj = evas_object_image_filled_add(e);
        // TIZEN_ONLY(20150819): Rotate image according to rotation info when image is loaded by entry, label.
        evas_object_image_load_orientation_set(item_obj, EINA_TRUE);
        //
        evas_object_image_file_set(item_obj, fname, NULL);
        if (evas_object_image_load_error_get(item_obj) != EVAS_LOAD_ERROR_NONE)
          {
             evas_object_del(item_obj);
             item_obj = edje_object_add(e);
             elm_widget_theme_object_set
                (obj, item_obj, "entry/emoticon", "wtf",
                 elm_widget_style_get(obj));
          }
        return item_obj;
     }

   item_obj = edje_object_add(e);
   if (!elm_widget_theme_object_set
       (obj, item_obj, "entry", name, elm_widget_style_get(obj)))
     elm_widget_theme_object_set
        (obj, item_obj, "entry/emoticon", "wtf", elm_widget_style_get(obj));
   return item_obj;
}

static void
_elm_label_anchors_update(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;
   Eina_List *l, *ll, *range;
   Evas_Object *tb;
   Evas_Object *smart, *clip;
   Elm_Label_Anchor_Data *an;
   Sel *sel;
   Evas *e;

   ELM_LABEL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->on_clear)
     return;

   tb = (Evas_Object *)edje_object_part_object_get(wd->resize_obj, "elm.text");
   smart = evas_object_smart_parent_get(tb);
   clip = evas_object_clip_get(tb);
   e = evas_object_evas_get(tb);

   evas_object_geometry_get(tb, &x, &y, &w, &h);

   EINA_LIST_FOREACH(sd->anchors, l, an)
     {
        if (an->item)
          {
             if (!an->sel)
               {
                  Evas_Object *item_obj;

                  sel = calloc(1, sizeof(Sel));
                  if (!sel)
                    break;

                  an->sel = eina_list_append(an->sel, sel);

                  item_obj = _elm_label_item_get(obj, an->name);
                  evas_object_smart_member_add(item_obj, smart);
                  evas_object_stack_above(item_obj, tb);
                  evas_object_clip_set(item_obj, clip);
                  evas_object_repeat_events_set(item_obj, EINA_TRUE);
                  evas_object_show(item_obj);
                  sel->obj = item_obj;
               }
          }
        else
          {
             range =
               evas_textblock_cursor_range_geometry_get(an->start, an->end);
             if (eina_list_count(range) != eina_list_count(an->sel))
               {
                 while (an->sel)
                   {
                      sel = an->sel->data;
                      if (sel->obj) evas_object_del(sel->obj);
                      free(sel);
                      an->sel = eina_list_remove_list(an->sel, an->sel);
                   }
                 for (ll = range; ll; ll = eina_list_next(ll))
                   {
                      Evas_Object *a_obj;

                      sel = calloc(1, sizeof(Sel));
                      an->sel = eina_list_append(an->sel, sel);

                      a_obj = evas_object_rectangle_add(e);
                      evas_object_color_set(a_obj, 0, 0, 0, 0);
                      evas_object_smart_member_add(a_obj, smart);
                      evas_object_stack_above(a_obj, tb);
                      evas_object_clip_set(a_obj, clip);
                      evas_object_repeat_events_set(a_obj, EINA_TRUE);
                      evas_object_event_callback_add(a_obj, EVAS_CALLBACK_MOUSE_DOWN, _label_anchor_mouse_down_cb, an);
                      evas_object_event_callback_add(a_obj, EVAS_CALLBACK_MOUSE_UP, _label_anchor_mouse_up_cb, an);
                      evas_object_event_callback_add(a_obj, EVAS_CALLBACK_MOUSE_MOVE, _label_anchor_mouse_move_cb, an);
                      evas_object_show(a_obj);
                      sel->obj = a_obj;
                   }
                }
          }

        EINA_LIST_FOREACH(an->sel, ll, sel)
          {
             if (an->item)
               {
                  Evas_Coord cx, cy, cw, ch;

                  if (!evas_textblock_cursor_format_item_geometry_get
                      (an->start, &cx, &cy, &cw, &ch))
                    {
                       evas_object_hide(sel->obj);
                       continue;
                    }

                  evas_object_move(sel->obj, x + cx, y + cy);
                  evas_object_resize(sel->obj, cw, ch);
                  evas_object_show(sel->obj);
               }
             else
               {
                  Evas_Textblock_Rectangle *r;

                  r = range->data;
                  *(&(sel->rect)) = *r;
                  if (sel->obj)
                    {
                       evas_object_move(sel->obj, x + r->x, y + r->y);
                       evas_object_resize(sel->obj, r->w, r->h);
                    }
                  range = eina_list_remove_list(range, range);
                  free(r);
                }

          }
     }
}

static void
_textblock_del_cb(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
   ELM_LABEL_DATA_GET(data, sd);
   if (sd->tb == obj) sd->tb = NULL;
   _elm_label_anchors_clear(data, EINA_FALSE);
}

static void
_elm_label_anchors_get(Evas_Object *obj)
{
   const Eina_List *anchors_item, *anchors_a;
   Evas_Object *tb;
   Elm_Label_Anchor_Data *an = NULL;

   ELM_LABEL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   _elm_label_anchors_clear(obj, EINA_TRUE);

   tb = (Evas_Object *)edje_object_part_object_get(wd->resize_obj, "elm.text");
   evas_object_event_callback_del_full(tb, EVAS_CALLBACK_DEL, _textblock_del_cb, obj);
   evas_object_event_callback_add(tb, EVAS_CALLBACK_DEL, _textblock_del_cb, obj);

   anchors_a = evas_textblock_node_format_list_get(tb, "a");
   anchors_item = evas_textblock_node_format_list_get(tb, "item");

   if (anchors_a)
     {
        const Evas_Object_Textblock_Node_Format *node;
        const Eina_List *itr;
        EINA_LIST_FOREACH(anchors_a, itr, node)
          {
             const char *s = evas_textblock_node_format_text_get(node);
             char *p;
             an = calloc(1, sizeof(Elm_Label_Anchor_Data));
             if (!an)
               break;

             an->obj = obj;
             p = strstr(s, "href=");
             if (p)
               {
                  an->name = strdup(p + 5);
               }
             sd->anchors = eina_list_append(sd->anchors, an);
             an->start = evas_object_textblock_cursor_new(tb);
             an->end = evas_object_textblock_cursor_new(tb);
             evas_textblock_cursor_at_format_set(an->start, node);
             evas_textblock_cursor_copy(an->start, an->end);

             /* Close the anchor, if the anchor was without text,
              * free it as well */
             node = evas_textblock_node_format_next_get(node);
             for (; node; node = evas_textblock_node_format_next_get(node))
               {
                  s = evas_textblock_node_format_text_get(node);
                  if ((!strcmp(s, "- a")) || (!strcmp(s, "-a")))
                    break;
               }

             if (node)
               {
                  evas_textblock_cursor_at_format_set(an->end, node);
               }
             else if (!evas_textblock_cursor_compare(an->start, an->end))
               {
                  if (an->name) free(an->name);
                  evas_textblock_cursor_free(an->start);
                  evas_textblock_cursor_free(an->end);
                  sd->anchors = eina_list_remove(sd->anchors, an);
                  free(an);
               }
             an = NULL;
          }
     }

   if (anchors_item)
     {
        const Evas_Object_Textblock_Node_Format *node;
        const Eina_List *itr;
        EINA_LIST_FOREACH(anchors_item, itr, node)
          {
             const char *s = evas_textblock_node_format_text_get(node);
             char *p;
             an = calloc(1, sizeof(Elm_Label_Anchor_Data));
             if (!an)
               break;

             an->obj = obj;
             an->item = 1;
             p = strstr(s, "href=");
             if (p)
               {
                  an->name = strdup(p + 5);
               }
             sd->anchors = eina_list_append(sd->anchors, an);
             an->start = evas_object_textblock_cursor_new(tb);
             an->end = evas_object_textblock_cursor_new(tb);
             evas_textblock_cursor_at_format_set(an->start, node);
             evas_textblock_cursor_copy(an->start, an->end);
             /* Although needed in textblock, don't bother with finding the end
              * here cause it doesn't really matter. */
          }
     }
}
/////////////////////////////////////////////////////////////////

static void
_recalc(void *data)
{
   ELM_LABEL_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw, w;

   evas_event_freeze(evas_object_evas_get(data));
   edje_object_size_min_calc(wd->resize_obj, &minw, NULL);
   evas_object_geometry_get(wd->resize_obj, NULL, NULL, &w, NULL);

   if (sd->wrap_w > minw)
     resw = sd->wrap_w;
   else if ((sd->wrap_w > 0) && (minw > sd->wrap_w))
     resw = minw;
   else
     resw = w;
   edje_object_size_min_restricted_calc(wd->resize_obj, &minw, &minh, resw, 0);

   /* This is a hack to workaround the way min size hints are treated.
    * If the minimum width is smaller than the restricted width, it means
    * the minimum doesn't matter. */
   if ((minw <= resw) && (minw != sd->wrap_w))
     {
        Evas_Coord ominw = -1;

        evas_object_size_hint_min_get(data, &ominw, NULL);
        minw = ominw;
     }

   evas_object_size_hint_min_set(data, minw, minh);
   // TIZEN_ONLY(20150724): fix wrapping calculation when wrap width is set.
   if (sd->wrap_w > 0)
     evas_object_size_hint_max_set(data, minw, minh);
   //

   evas_event_thaw(evas_object_evas_get(data));
   evas_event_thaw_eval(evas_object_evas_get(data));
}

static void
_label_format_set(Evas_Object *obj, const char *format)
{
   if (format)
     edje_object_part_text_style_user_push(obj, "elm.text", format);
   else
     edje_object_part_text_style_user_pop(obj, "elm.text");
}

static void
_label_slide_change(Evas_Object *obj)
{
   const Evas_Object *tb;
   char *plaintxt;
   int plainlen = 0;

   ELM_LABEL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (!sd->slide_state) return;

   edje_object_signal_emit(wd->resize_obj, "elm,state,slide,stop", "elm");

   //doesn't support multiline slide effect
   if (sd->linewrap)
     {
        WRN("Doesn't support slide effect for multiline! : label=%p", obj);
        return;
     }

   //stop if the text is none.
   plaintxt = _elm_util_mkup_to_text
      (edje_object_part_text_get(wd->resize_obj, "elm.text"));
   if (plaintxt)
     {
        plainlen = strlen(plaintxt);
        free(plaintxt);
     }
   if (plainlen < 1) return;

   //has slide effect.
   if (sd->slide_mode != ELM_LABEL_SLIDE_MODE_NONE)
     {
        Evas_Coord w, tb_w;

        tb = edje_object_part_object_get(wd->resize_obj, "elm.text");
        evas_object_textblock_size_native_get(tb, &tb_w, NULL);
        evas_object_geometry_get(wd->resize_obj, NULL, NULL, &w, NULL);

        if (sd->ellipsis)
          {
             sd->slide_ellipsis = EINA_TRUE;
             elm_label_ellipsis_set(obj, EINA_FALSE);
          }

        //slide only if the slide area is smaller than text width size.
        if (sd->slide_mode == ELM_LABEL_SLIDE_MODE_AUTO)
          {
            if ((tb_w > 0) && (tb_w < w))
              {
                if (sd->slide_ellipsis)
                  {
                     sd->slide_ellipsis = EINA_FALSE;
                     elm_label_ellipsis_set(obj, EINA_TRUE);
                  }
                return;
              }
          }

        // calculate speed or duration
        if (!strcmp(elm_object_style_get(obj), "slide_long"))
          w = tb_w + w;
        else // slide_short or slide_bounce
          w = tb_w - w;
        if (sd->use_slide_speed)
          {
             if (sd->slide_speed <= 0) sd->slide_speed = 1;
             sd->slide_duration = w / sd->slide_speed;
          }
        else
          {
             if (sd->slide_duration <= 0) sd->slide_duration = 1;
             sd->slide_speed = w / sd->slide_duration;
          }

        Edje_Message_Float_Set *msg =
          alloca(sizeof(Edje_Message_Float_Set) + (sizeof(double)));

        msg->count = 1;
        msg->val[0] = sd->slide_duration;

        edje_object_message_send(wd->resize_obj, EDJE_MESSAGE_FLOAT_SET, 0, msg);
        edje_object_signal_emit(wd->resize_obj, "elm,state,slide,start", "elm");
     }
   //no slide effect.
   else
     {
        if (sd->slide_ellipsis)
          {
             sd->slide_ellipsis = EINA_FALSE;
             elm_label_ellipsis_set(obj, EINA_TRUE);
          }
     }
}

EOLIAN static Eina_Bool
_elm_label_elm_widget_theme_apply(Eo *obj, Elm_Label_Data *sd)
{
   Eina_Bool int_ret = EINA_FALSE;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);

   evas_event_freeze(evas_object_evas_get(obj));

   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_theme_apply());
   if (!int_ret) {
     evas_event_thaw(evas_object_evas_get(obj));
     evas_event_thaw_eval(evas_object_evas_get(obj));
     return EINA_FALSE;
   }

   // TIZEN_ONLY(20150810): Update and clear anchors forcely when theme is applied.
   //sd->anchors_force_update = EINA_TRUE;
   //
   _label_format_set(wd->resize_obj, sd->format);
   _label_slide_change(obj);

   // TIZEN_ONLY(20150526): Support item with image path.
   Evas_Object *tb = (Evas_Object *)edje_object_part_object_get(wd->resize_obj, "elm.text");

   if (sd->tb != tb)
     {
        evas_object_event_callback_add(tb, EVAS_CALLBACK_SHOW, _label_anchors_show, obj);
        evas_object_event_callback_add(tb, EVAS_CALLBACK_HIDE, _label_anchors_hide, obj);
        evas_object_event_callback_add(tb, EVAS_CALLBACK_MOVE, _label_anchors_move, obj);
        evas_object_event_callback_add(tb, EVAS_CALLBACK_MOUSE_DOWN, _label_mouse_down_cb, obj);
        evas_object_event_callback_add(tb, EVAS_CALLBACK_MOUSE_UP, _label_mouse_up_cb, obj);
        evas_object_event_callback_add(tb, EVAS_CALLBACK_MOUSE_MOVE, _label_mouse_move_cb, obj);
     }
   sd->tb = tb;
   sd->lastw = 0;
   //

   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));

   elm_layout_sizing_eval(obj);

   return int_ret;
}

EOLIAN static void
_elm_label_elm_layout_sizing_eval(Eo *obj, Elm_Label_Data *_pd EINA_UNUSED)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord resw, resh;

   ELM_LABEL_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->linewrap)
     {
        evas_object_geometry_get(wd->resize_obj, NULL, NULL, &resw, &resh);
        // TIZEN_ONLY(20150526): Support item with image path.
        //if (resw == sd->lastw) return;
        if ((resw == sd->lastw) || ((resw == 0) && (sd->wrap_w <= 0))) goto end;
        //
        sd->lastw = resw;
        _recalc(obj);
     }
   else
     {
        evas_event_freeze(evas_object_evas_get(obj));
        edje_object_size_min_calc(wd->resize_obj, &minw, &minh);
        if (sd->wrap_w > 0 && minw > sd->wrap_w) minw = sd->wrap_w;
        evas_object_size_hint_min_set(obj, minw, minh);
        evas_event_thaw(evas_object_evas_get(obj));
        evas_event_thaw_eval(evas_object_evas_get(obj));
     }
   // TIZEN_ONLY(20150526): Support item with image path.
end:
   if (sd->tb)
     {
        Evas_Coord tx, ty, tw, th;
        evas_object_geometry_get(sd->tb, &tx, &ty, &tw, &th);

        if ((tx != sd->tx) || (ty != sd->ty) ||
            (tw != sd->tw) || (th != sd->th) || sd->anchors_force_update)
          {
             if (sd->anchors_force_update)
               {
                  sd->anchors_force_update = EINA_FALSE;
                  _elm_label_anchors_get(obj);
               }
             _elm_label_anchors_update(obj);
          }

        sd->tx = tx;
        sd->ty = ty;
        sd->tw = tw;
        sd->th = th;
     }
}

static void
_on_label_resize(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   ELM_LABEL_DATA_GET(data, sd);

   if (sd->slide_mode != ELM_LABEL_SLIDE_MODE_NONE) _label_slide_change(data);
   if (sd->linewrap) elm_layout_sizing_eval(data);
   // TIZEN_ONLY(20150526): Support item with image path.
   else _elm_label_anchors_update(data);
   //
}

static int
_get_value_in_key_string(const char *oldstring, const char *key, char **value)
{
   char *curlocater, *endtag;
   int firstindex = 0, foundflag = -1;

   curlocater = strstr(oldstring, key);
   if (curlocater)
     {
        int key_len = strlen(key);
        endtag = curlocater + key_len;
        if ((!endtag) || (*endtag != '='))
             return -1;

        firstindex = abs(oldstring - curlocater);
        firstindex += key_len + 1; // strlen("key") + strlen("=")
        *value = (char *)oldstring + firstindex;

        foundflag = 1;
     }
   else
     {
        foundflag = 0;
     }

   if (foundflag == 1) return 0;

   return -1;
}

static int
_strbuf_key_value_replace(Eina_Strbuf *srcbuf, const char *key, const char *value, int deleteflag)
{
   char *kvalue;
   const char *srcstring = NULL;

   srcstring = eina_strbuf_string_get(srcbuf);

   if (_get_value_in_key_string(srcstring, key, &kvalue) == 0)
     {
        const char *val_end;
        int val_end_idx = 0;
        int key_start_idx = 0;
        val_end = strchr(kvalue, ' ');

        if (val_end)
          val_end_idx = val_end - srcstring;
        else
          val_end_idx = kvalue - srcstring + strlen(kvalue) - 1;

        /* -1 is because of the '=' */
        key_start_idx = kvalue - srcstring - 1 - strlen(key);
        eina_strbuf_remove(srcbuf, key_start_idx, val_end_idx);
        if (!deleteflag)
          {
             eina_strbuf_insert_printf(srcbuf, "%s=%s", key_start_idx, key,
                                       value);
          }
     }
   else if (!deleteflag)
     {
        if (*srcstring)
          {
             /* -1 because we want it before the ' */
             eina_strbuf_insert_printf (srcbuf, " %s=%s",
                                eina_strbuf_length_get(srcbuf) - 1, key, value);
          }
        else
          {
             eina_strbuf_append_printf(srcbuf, "DEFAULT='%s=%s'", key, value);
          }
     }

   return 0;
}

static int
_stringshare_key_value_replace(const char **srcstring, const char *key, const char *value, int deleteflag)
{
   Eina_Strbuf *sharebuf = NULL;

   sharebuf = eina_strbuf_new();
   eina_strbuf_append(sharebuf, *srcstring);
   _strbuf_key_value_replace(sharebuf, key, value, deleteflag);
   eina_stringshare_del(*srcstring);
   *srcstring = eina_stringshare_add(eina_strbuf_string_get(sharebuf));
   eina_strbuf_free(sharebuf);

   return 0;
}

EOLIAN static Eina_Bool
_elm_label_elm_layout_text_set(Eo *obj, Elm_Label_Data *sd, const char *part, const char *label)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd, EINA_FALSE);
   Eina_Bool int_ret = EINA_FALSE;

   if (!label) label = "";
   _label_format_set(wd->resize_obj, sd->format);

   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_layout_text_set(part, label));
   if (int_ret)
     {
        sd->lastw = -1;
        // TIZEN_ONLY(20150526): Support item with image path.
        if (!part || !strcmp(part, "elm.text"))
          {
             sd->anchors_force_update = EINA_TRUE;
          }
        //
        eo_do(obj, elm_obj_layout_sizing_eval());
     }
   return int_ret;
}

static char *
_access_info_cb(void *data EINA_UNUSED, Evas_Object *obj)
{
   const char *txt = elm_widget_access_info_get(obj);

   if (!txt)
     return _elm_util_mkup_to_text(elm_layout_text_get(obj, NULL));
   else return strdup(txt);
}

static void
_on_slide_end(void *data, Evas_Object *obj EINA_UNUSED,
              const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   ELM_LABEL_DATA_GET(data, sd);

   if (sd->slide_ellipsis)
     eo_do(data, elm_obj_label_ellipsis_set(EINA_TRUE));

   evas_object_smart_callback_call(data, SIG_SLIDE_END, NULL);
}

//Tizen only(9/29/2015) Smart del added.
EOLIAN static void
_elm_label_evas_object_smart_del(Eo *obj, Elm_Label_Data *sd)
{
   evas_event_freeze(evas_object_evas_get(obj));
   if (sd->longpress_timer)
     ELM_SAFE_FREE(sd->longpress_timer, ecore_timer_del);
   _elm_label_anchors_clear(obj, EINA_TRUE);
   if (sd->item_providers)
     {
        Elm_Label_Item_Provider *ip;
        EINA_LIST_FREE(sd->item_providers, ip)
          {
             free(ip);
          }
     }
   evas_event_thaw(evas_object_evas_get(obj));
   evas_event_thaw_eval(evas_object_evas_get(obj));
   eo_do_super(obj, MY_CLASS, evas_obj_smart_del());
}
//

EOLIAN static void
_elm_label_evas_object_smart_add(Eo *obj, Elm_Label_Data *priv)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   eo_do_super(obj, MY_CLASS, evas_obj_smart_add());

   elm_widget_sub_object_parent_add(obj);

   priv->linewrap = ELM_WRAP_NONE;
   priv->wrap_w = -1;
   priv->slide_duration = 10;

   priv->format = eina_stringshare_add("");
   _label_format_set(wd->resize_obj, priv->format);

   evas_object_event_callback_add(wd->resize_obj, EVAS_CALLBACK_RESIZE,
                                  _on_label_resize, obj);

   edje_object_signal_callback_add(wd->resize_obj, "elm,state,slide,end", "elm",
                                   _on_slide_end, obj);

   /* access */
   elm_widget_can_focus_set(obj, _elm_config->access_mode);

   _elm_access_object_register(obj, wd->resize_obj);
   _elm_access_text_set(_elm_access_info_get(obj), ELM_ACCESS_TYPE,
                        E_("Label"));
   _elm_access_callback_set(_elm_access_info_get(obj), ELM_ACCESS_INFO,
                            _access_info_cb, NULL);

   if (!elm_layout_theme_set(obj, "label", "base", elm_widget_style_get(obj)))
     ERR("Failed to set layout!");

   elm_layout_text_set(obj, NULL, "<br>");
   elm_layout_sizing_eval(obj);

   // TIZEN_ONLY(20150526): Support item with image path.
   priv->anchors_force_update = EINA_FALSE;
   priv->tb = (Evas_Object *)edje_object_part_object_get(wd->resize_obj, "elm.text");
   priv->tx = 0;
   priv->ty = 0;
   priv->tw = 0;
   priv->th = 0;

   evas_object_event_callback_add(priv->tb, EVAS_CALLBACK_SHOW, _label_anchors_show, obj);
   evas_object_event_callback_add(priv->tb, EVAS_CALLBACK_HIDE, _label_anchors_hide, obj);
   evas_object_event_callback_add(priv->tb, EVAS_CALLBACK_MOVE, _label_anchors_move, obj);

   evas_object_event_callback_add(priv->tb, EVAS_CALLBACK_MOUSE_DOWN, _label_mouse_down_cb, obj);
   evas_object_event_callback_add(priv->tb, EVAS_CALLBACK_MOUSE_UP, _label_mouse_up_cb, obj);
   evas_object_event_callback_add(priv->tb, EVAS_CALLBACK_MOUSE_MOVE, _label_mouse_move_cb, obj);
   //
}

EAPI Evas_Object *
elm_label_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);

   traceBegin(TTRACE_TAG_VIEW, "elm_label_add: %p", parent);
   
   Evas_Object *obj = eo_add(MY_CLASS, parent);

   traceEnd(TTRACE_TAG_VIEW);

   return obj;
}

EOLIAN static void
_elm_label_eo_base_constructor(Eo *obj, Elm_Label_Data *_pd EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME_LEGACY),
         evas_obj_smart_callbacks_descriptions_set(_smart_callbacks),
         elm_interface_atspi_accessible_role_set(ELM_ATSPI_ROLE_LABEL));
}

EOLIAN static void
_elm_label_line_wrap_set(Eo *obj, Elm_Label_Data *sd, Elm_Wrap_Type wrap)
{
   const char *wrap_str, *text;
   int len;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->linewrap == wrap) return;

   sd->linewrap = wrap;
   text = elm_layout_text_get(obj, NULL);
   if (!text) return;

   len = strlen(text);
   if (len <= 0) return;

   switch (wrap)
     {
      case ELM_WRAP_CHAR:
        wrap_str = "char";
        break;

      case ELM_WRAP_WORD:
        wrap_str = "word";
        break;

      case ELM_WRAP_MIXED:
        wrap_str = "mixed";
        break;

      default:
        wrap_str = "none";
        break;
     }

   if (_stringshare_key_value_replace(&sd->format, "wrap", wrap_str, 0) == 0)
     {
        sd->lastw = -1;
        _label_format_set(wd->resize_obj, sd->format);
        elm_layout_sizing_eval(obj);
     }
}

EOLIAN static Elm_Wrap_Type
_elm_label_line_wrap_get(Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->linewrap;
}

EOLIAN static void
_elm_label_wrap_width_set(Eo *obj, Elm_Label_Data *sd, Evas_Coord w)
{
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (w < 0) w = 0;

   if (sd->wrap_w == w) return;

   if (sd->ellipsis)
     _label_format_set(wd->resize_obj, sd->format);
   sd->wrap_w = w;
   sd->lastw = -1;

   elm_layout_sizing_eval(obj);
}

EOLIAN static Evas_Coord
_elm_label_wrap_width_get(Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->wrap_w;
}

EOLIAN static void
_elm_label_ellipsis_set(Eo *obj, Elm_Label_Data *sd, Eina_Bool ellipsis)
{
   Eina_Strbuf *fontbuf = NULL;
   int len, removeflag = 0;
   const char *text;

   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->ellipsis == ellipsis) return;
   sd->ellipsis = ellipsis;

   text = elm_layout_text_get(obj, NULL);
   if (!text) return;

   len = strlen(text);
   if (len <= 0) return;

   if (ellipsis == EINA_FALSE) removeflag = 1;  // remove fontsize tag

   fontbuf = eina_strbuf_new();
   eina_strbuf_append_printf(fontbuf, "%f", 1.0);

   if (_stringshare_key_value_replace
         (&sd->format, "ellipsis", eina_strbuf_string_get
           (fontbuf), removeflag) == 0)
     {
        _label_format_set(wd->resize_obj, sd->format);
        elm_layout_sizing_eval(obj);
     }
   eina_strbuf_free(fontbuf);
}

EOLIAN static Eina_Bool
_elm_label_ellipsis_get(Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->ellipsis;
}

EOLIAN static void
_elm_label_slide_mode_set(Eo *obj EINA_UNUSED, Elm_Label_Data *sd, Elm_Label_Slide_Mode mode)
{
   sd->slide_mode = mode;
}

EOLIAN static Elm_Label_Slide_Mode
_elm_label_slide_mode_get(Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->slide_mode;
}

EINA_DEPRECATED EAPI void
elm_label_slide_set(Evas_Object *obj, Eina_Bool slide)
{
   if (slide)
     elm_label_slide_mode_set(obj, ELM_LABEL_SLIDE_MODE_ALWAYS);
   else
     elm_label_slide_mode_set(obj, ELM_LABEL_SLIDE_MODE_NONE);
}

EINA_DEPRECATED EAPI Eina_Bool
elm_label_slide_get(const Evas_Object *obj)
{
   Eina_Bool ret = EINA_FALSE;
   if (elm_label_slide_mode_get(obj) == ELM_LABEL_SLIDE_MODE_ALWAYS)
     ret = EINA_TRUE;
   return ret;
}

EOLIAN static void
_elm_label_slide_duration_set(Eo *obj EINA_UNUSED, Elm_Label_Data *sd, double duration)
{
   sd->slide_duration = duration;
   sd->use_slide_speed = EINA_FALSE;
}

EOLIAN static void
_elm_label_slide_speed_set(Eo *obj EINA_UNUSED, Elm_Label_Data *sd, double speed)
{
   sd->slide_speed = speed;
   sd->use_slide_speed = EINA_TRUE;
}

EOLIAN static double
_elm_label_slide_speed_get(Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->slide_speed;
}

EAPI void
elm_label_item_provider_append(Evas_Object *obj, Elm_Label_Item_Provider_Cb func, void *data)
{
  Elm_Label_Item_Provider *ip;

  ELM_LABEL_CHECK(obj);
  ELM_LABEL_DATA_GET(obj, sd);
  EINA_SAFETY_ON_NULL_RETURN(func);

  ip = calloc(1, sizeof(Elm_Label_Item_Provider));
  if (!ip) return;

  ip->func = func;
  ip->data = data;
  sd->item_providers = eina_list_append(sd->item_providers, ip);
}

EAPI void
elm_label_item_provider_prepend(Evas_Object *obj, Elm_Label_Item_Provider_Cb func, void *data)
{
  Elm_Label_Item_Provider *ip;

  ELM_LABEL_CHECK(obj);
  ELM_LABEL_DATA_GET(obj, sd);
  EINA_SAFETY_ON_NULL_RETURN(func);

  ip = calloc(1, sizeof(Elm_Label_Item_Provider));
  if (!ip) return;

  ip->func = func;
  ip->data = data;
  sd->item_providers = eina_list_prepend(sd->item_providers, ip);
}

EAPI void
elm_label_item_provider_remove(Evas_Object *obj, Elm_Label_Item_Provider_Cb func, void *data)
{
  Eina_List *l;
  Elm_Label_Item_Provider *ip;

  ELM_LABEL_CHECK(obj);
  ELM_LABEL_DATA_GET(obj, sd);
  EINA_SAFETY_ON_NULL_RETURN(func);

  EINA_LIST_FOREACH(sd->item_providers, l, ip)
    {
      if ((ip->func == func) && ((!data) || (ip->data == data)))
        {
          sd->item_providers = eina_list_remove_list(sd->item_providers, l);
          free(ip);
          return;
        }
    }
}

EAPI void
elm_label_anchor_access_provider_append(Evas_Object *obj, Elm_Label_Anchor_Access_Provider_Cb func, void *data)
{
}

EAPI void
elm_label_anchor_access_provider_prepend(Evas_Object *obj, Elm_Label_Anchor_Access_Provider_Cb func, void *data)
{
}


EAPI void
elm_label_anchor_access_provider_remove(Evas_Object *obj, Elm_Label_Anchor_Access_Provider_Cb func, void *data)
{
}

EOLIAN static void
_elm_label_slide_go(Eo *obj, Elm_Label_Data *sd)
{
   if (!sd->slide_state) sd->slide_state = EINA_TRUE;

   _label_slide_change(obj);
   elm_layout_sizing_eval(obj);
}

EOLIAN static double
_elm_label_slide_duration_get(Eo *obj EINA_UNUSED, Elm_Label_Data *sd)
{
   return sd->slide_duration;
}

EOLIAN static const Elm_Layout_Part_Alias_Description*
_elm_label_elm_layout_text_aliases_get(Eo *obj EINA_UNUSED, Elm_Label_Data *_pd EINA_UNUSED)
{
   return _text_aliases;
}

EOLIAN static Eina_Bool
_elm_label_elm_widget_focus_next_manager_is(Eo *obj EINA_UNUSED, Elm_Label_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

EOLIAN static Eina_Bool
_elm_label_elm_widget_focus_direction_manager_is(Eo *obj EINA_UNUSED, Elm_Label_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

EOLIAN static void
_elm_label_class_constructor(Eo_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

#include "elm_label.eo.c"
