#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED
//TIZEN_ONLY: mesh gradient effect
#define MESH_GRADIENT

#include <Elementary.h>

#include "elm_priv.h"
#include "elm_widget_notify.h"
#include "elm_widget_container.h"
#ifdef MESH_GRADIENT
#include <gxt_shared_image_buffer.h>
#endif

#define MY_CLASS ELM_NOTIFY_CLASS

#define MY_CLASS_NAME "Elm_Notify"
#define MY_CLASS_NAME_LEGACY "elm_notify"

static const char SIG_DISMISSED[] = "dismissed";
static const char SIG_BLOCK_CLICKED[] = "block,clicked";
static const char SIG_SHOW_FINISHED[] = "show,finished";
static const char SIG_TIMEOUT[] = "timeout";
static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_DISMISSED, ""},
   {SIG_SHOW_FINISHED, ""},
   {SIG_BLOCK_CLICKED, ""},
   {SIG_TIMEOUT, ""},
   {NULL, NULL}
};

static Eina_Bool
_notify_show_defer_cb(void *data)
{
   Evas_Object *obj = (Evas_Object*)data;
   ELM_NOTIFY_DATA_GET(obj, sd);

   edje_object_signal_emit(sd->notify, "show,defer,signal,received", "elm");
   edje_object_message_signal_process(sd->notify);
   sd->delay_animator = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static void
_notify_show_defer_emit_cb(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   Evas_Object *notify_sd = (Evas_Object*)data;
   ELM_NOTIFY_DATA_GET(notify_sd, sd);

   if (sd->delay_animator)
   {
        ecore_animator_del(sd->delay_animator);
        sd->delay_animator = NULL;
   }

   sd->delay_animator = ecore_animator_add(_notify_show_defer_cb, data);
}

static void
_notify_show_finished_cb(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   evas_object_smart_callback_call(data, SIG_SHOW_FINISHED, NULL);
}
static void
_hide_effect_finished_cb(void *data,
                         Evas_Object *obj EINA_UNUSED,
                         const char *emission EINA_UNUSED,
                         const char *source EINA_UNUSED)
{
   evas_object_smart_callback_call(data, SIG_DISMISSED, NULL);
}

#ifdef MESH_GRADIENT
static void
_create_shared_image_buffer(Evas_Object *obj, Evas_Object *bg)
{
   int iw = 0, ih = 0;
   unsigned char *fixed = (unsigned char *)ecore_evas_data_get(ecore_evas_ecore_evas_get(evas_object_evas_get(obj)), "fixed_theme");

   gxt_shared_image_buffer_h handler = gxt_shared_image_buffer_initialize();
   if (!handler) return NULL;

   tbm_surface_h tbm_surface;
   int ret = gxt_shared_image_buffer_get_tbm_surface(handler, &tbm_surface);
   if (ret != GXT_ERROR_NONE)
     {
        gxt_shared_image_buffer_deinitialize(handler);
        return NULL;
     }

   iw = tbm_surface_get_width(tbm_surface);
   ih = tbm_surface_get_height(tbm_surface);

   evas_object_resize(bg, iw, ih);
   evas_object_image_size_set(bg, iw, ih);

   if (fixed)
     evas_object_image_data_set(bg, fixed);
   else
     {
        Evas_Native_Surface surface;
        surface.version = EVAS_NATIVE_SURFACE_VERSION;
        surface.type = EVAS_NATIVE_SURFACE_TBM;
        surface.data.tbm.buffer = tbm_surface;
        surface.data.tbm.rot = 0;
        surface.data.tbm.ratio = 0;
        surface.data.tbm.flip = 0;
        evas_object_image_native_surface_set(bg, &surface);
     }
   evas_object_image_data_update_add(bg, 0, 0, iw, ih);

   evas_object_data_set(obj, "handler", handler);
   evas_object_data_set(obj, "tbm_surface", tbm_surface);
}

static void
_create_content_bg_image(Evas_Object *obj)
{
   Evas_Object *bg;
   int changeable;

   ELM_NOTIFY_DATA_GET(obj, sd);

   changeable = ecore_evas_data_get(ecore_evas_ecore_evas_get(evas_object_evas_get(obj)), "changeable_ui");

   if (!changeable)
     {
       Eina_List *eelist = ecore_evas_ecore_evas_list_get();
       if (eelist)
         {
           Eina_List* list = NULL;
           Ecore_Evas *ee = NULL;
           EINA_LIST_FOREACH(eelist, list, ee)
             {
                 changeable = ecore_evas_data_get(ee, "changeable_ui");
                 if (changeable) break;
             }
           eina_list_free(eelist);
         }
     }

   if (changeable)
     {
        if (edje_object_part_exists(sd->notify, "mesh_bg_changeable"))
          {
             int iw = 0, ih = 0;

             bg = edje_object_part_swallow_get(sd->notify, "mesh_bg_changeable");
             if (bg)
               {
                  tbm_surface_h tbm_surface;

                  tbm_surface = (tbm_surface_h)evas_object_data_get(sd->notify, "tbm_surface");
                  if (tbm_surface)
                     {
                        iw = tbm_surface_get_width(tbm_surface);
                        ih = tbm_surface_get_height(tbm_surface);
                        evas_object_image_data_update_add(bg, 0, 0, iw, ih);
                     }
               }
             else
               {
                  bg = evas_object_image_filled_add(evas_object_evas_get(obj));
                  _create_shared_image_buffer(sd->notify, bg);
               }

             edje_object_part_swallow(sd->notify, "mesh_bg_changeable", bg);
          }
        edje_object_signal_emit(sd->notify, "elm,state,changeable_ui,on", "elm");
     }
   else
     edje_object_signal_emit(sd->notify, "elm,state,changeable_ui,off", "elm");
}

static void
_content_color_class_changed_cb(void *data EINA_UNUSED,
                        Evas_Object *obj,
                        const char *emission EINA_UNUSED,
                        const char *source EINA_UNUSED)
{
   Evas_Object *bg;

   if (edje_object_part_exists(obj, "mesh_bg_changeable"))
     {
        bg = edje_object_part_swallow_get(obj, "mesh_bg_changeable");
        if (bg)
          _create_shared_image_buffer(obj, bg);
     }
}
#endif

static void
_notify_theme_apply(Evas_Object *obj)
{
   const char *style = elm_widget_style_get(obj);
   const char *position;
   double ax, ay;

   ELM_NOTIFY_DATA_GET(obj, sd);

   ax = sd->horizontal_align;
   ay = sd->vertical_align;
   if ((elm_widget_mirrored_get(obj)) && (ax != ELM_NOTIFY_ALIGN_FILL)) ax = 1.0 - ax;

   if (ay == 0.0)
     {
        if (ax == 0.0)
          position = "top_left";
        else if (ax == 1.0)
          position = "top_right";
        else
          position = "top";
     }
   else if (ay == 1.0)
     {
        if (ax == 0.0)
          position = "bottom_left";
        else if (ax == 1.0)
          position = "bottom_right";
        else
          position = "bottom";
     }
   else
     {
        if (ax == 0.0)
          position = "left";
        else if (ax == 1.0)
          position = "right";
        else
          position = "center";
     }

   elm_widget_theme_object_set(obj, sd->notify, "notify", position, style);
#ifdef MESH_GRADIENT
   _create_content_bg_image(obj);
#endif
}

/**
 * Moves notification to orientation.
 *
 * This function moves notification to orientation
 * according to object RTL orientation.
 *
 * @param obj notification object.
 *
 * @param orient notification orientation.
 *
 * @internal
 **/
static void
_notify_move_to_orientation(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord x, y, w, h;
   double ax, ay;

   ELM_NOTIFY_DATA_GET(obj, sd);

   evas_object_geometry_get(obj, &x, &y, &w, &h);
   edje_object_size_min_get(sd->notify, &minw, &minh);
   edje_object_size_min_restricted_calc(sd->notify, &minw, &minh, minw, minh);

   ax = sd->horizontal_align;
   ay = sd->vertical_align;
   if ((elm_widget_mirrored_get(obj)) && (ax != ELM_NOTIFY_ALIGN_FILL)) ax = 1.0 - ax;

   if (ax == ELM_NOTIFY_ALIGN_FILL) minw = w;
   if (ay == ELM_NOTIFY_ALIGN_FILL) minh = h;

   x = x + ((w - minw) * ax);
   y = y + ((h - minh) * ay);

   evas_object_move(sd->notify, x, y);
}

static void
_block_events_theme_apply(Evas_Object *obj)
{
   ELM_NOTIFY_DATA_GET(obj, sd);

   const char *style = elm_widget_style_get(obj);

   if (!elm_layout_theme_set
       (sd->block_events, "notify", "block_events", style))
     ERR("Failed to set layout!");
}

static void
_mirrored_set(Evas_Object *obj, Eina_Bool rtl)
{
   ELM_NOTIFY_DATA_GET(obj, sd);
   edje_object_mirrored_set(sd->notify, rtl);
   _notify_move_to_orientation(obj);
}

static void
_sizing_eval(Evas_Object *obj)
{
   Evas_Coord x, y, w, h;

   ELM_NOTIFY_DATA_GET(obj, sd);

   if (!sd->parent) return;
   evas_object_geometry_get(sd->parent, &x, &y, &w, &h);
   if (eo_isa(sd->parent, ELM_WIN_CLASS))
     {
        x = 0;
        y = 0;
     }
   evas_object_move(obj, x, y);
   evas_object_resize(obj, w, h);
}

EOLIAN static Eina_Bool
_elm_notify_elm_widget_theme_apply(Eo *obj, Elm_Notify_Data *sd)
{
   Eina_Bool int_ret = EINA_FALSE;
   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_theme_apply());
   if (!int_ret) return EINA_FALSE;

   _mirrored_set(obj, elm_widget_mirrored_get(obj));

   _notify_theme_apply(obj);
   if (sd->block_events) _block_events_theme_apply(obj);

   edje_object_scale_set
     (sd->notify, elm_widget_scale_get(obj) * elm_config_scale_get());

   _sizing_eval(obj);

   return EINA_TRUE;
}

static void
_calc(Evas_Object *obj)
{
   Evas_Coord minw = -1, minh = -1;
   Evas_Coord x, y, w, h;

   ELM_NOTIFY_DATA_GET(obj, sd);

   _sizing_eval(obj);

   evas_object_geometry_get(obj, &x, &y, &w, &h);

   edje_object_size_min_get(sd->notify, &minw, &minh);
   edje_object_size_min_restricted_calc(sd->notify, &minw, &minh, minw, minh);

   if (sd->horizontal_align == ELM_NOTIFY_ALIGN_FILL) minw = w;
   if (sd->vertical_align == ELM_NOTIFY_ALIGN_FILL) minh = h;

   if (sd->content)
     {
        _notify_move_to_orientation(obj);
        evas_object_resize(sd->notify, minw, minh);
     }
}

static void
_changed_size_hints_cb(void *data,
                       Evas *e EINA_UNUSED,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   _calc(data);
}

EOLIAN static Eina_Bool
_elm_notify_elm_widget_sub_object_del(Eo *obj, Elm_Notify_Data *sd, Evas_Object *sobj)
{
   Eina_Bool int_ret = EINA_FALSE;
   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_sub_object_del(sobj));
   if (!int_ret) return EINA_FALSE;

   if (sobj == sd->content)
     {
        evas_object_event_callback_del_full
          (sobj, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        sd->content = NULL;
     }

   return EINA_TRUE;
}

static void
_block_area_clicked_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       const char *emission EINA_UNUSED,
                       const char *source EINA_UNUSED)
{
   evas_object_smart_callback_call(data, SIG_BLOCK_CLICKED, NULL);
}

EOLIAN static void
_elm_notify_evas_object_smart_resize(Eo *obj, Elm_Notify_Data *sd EINA_UNUSED, Evas_Coord w, Evas_Coord h)
{
   eo_do_super(obj, MY_CLASS, evas_obj_smart_resize(w, h));

   _calc(obj);
}

EOLIAN static void
_elm_notify_evas_object_smart_move(Eo *obj, Elm_Notify_Data *sd EINA_UNUSED, Evas_Coord x, Evas_Coord y)
{
   eo_do_super(obj, MY_CLASS, evas_obj_smart_move(x, y));

   _calc(obj);
}

static Eina_Bool
_timer_cb(void *data)
{
   const char *hide_signal;
   Evas_Object *obj = data;

   ELM_NOTIFY_DATA_GET(obj, sd);

   sd->timer = NULL;
   if (!evas_object_visible_get(obj)) goto end;

   hide_signal = edje_object_data_get(sd->notify, "hide_finished_signal");
   if ((hide_signal) && (!strcmp(hide_signal, "on")))
     {
        sd->in_timeout = EINA_TRUE;
        edje_object_signal_emit(sd->notify, "elm,state,hide", "elm");
     }
   else //for backport supporting: edc without emitting hide finished signal
     {
        evas_object_hide(obj);
     }
   evas_object_smart_callback_call(obj, SIG_TIMEOUT, NULL);

end:
   return ECORE_CALLBACK_CANCEL;
}

static void
_timer_init(Evas_Object *obj,
            Elm_Notify_Data *sd)
{
   ecore_timer_del(sd->timer);
   if (sd->timeout > 0.0)
     sd->timer = ecore_timer_add(sd->timeout, _timer_cb, obj);
   else
     sd->timer = NULL;
}

EOLIAN static void
_elm_notify_evas_object_smart_show(Eo *obj, Elm_Notify_Data *sd)
{
   sd->had_hidden = EINA_FALSE;
   sd->in_timeout = EINA_FALSE;
   eo_do_super(obj, MY_CLASS, evas_obj_smart_show());

   evas_object_show(sd->notify);
   if (!sd->allow_events) evas_object_show(sd->block_events);
   _timer_init(obj, sd);
   elm_object_focus_set(obj, EINA_TRUE);
}

EOLIAN static void
_elm_notify_evas_object_smart_hide(Eo *obj, Elm_Notify_Data *sd)
{
   const char *hide_signal;

   if (sd->delay_animator)
   {
        ecore_animator_del(sd->delay_animator);
        sd->delay_animator = NULL;
   }

   if (sd->had_hidden && !sd->in_timeout)
     return;
   eo_do_super(obj, MY_CLASS, evas_obj_smart_hide());

   hide_signal = edje_object_data_get(sd->notify, "hide_finished_signal");
   if ((hide_signal) && (!strcmp(hide_signal, "on")))
     {
        if (!sd->in_timeout)
          edje_object_signal_emit(sd->notify, "elm,state,hide", "elm");
     }
   else //for backport supporting: edc without emitting hide finished signal
     {
        evas_object_hide(sd->notify);
        if (sd->allow_events) evas_object_hide(sd->block_events);
     }
   ELM_SAFE_FREE(sd->timer, ecore_timer_del);
}

static void
_parent_del_cb(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj EINA_UNUSED,
               void *event_info EINA_UNUSED)
{
   elm_notify_parent_set(data, NULL);
   evas_object_hide(data);
}

static void
_parent_hide_cb(void *data,
                Evas *e EINA_UNUSED,
                Evas_Object *obj EINA_UNUSED,
                void *event_info EINA_UNUSED)
{
   evas_object_hide(data);
}

EOLIAN static Eina_Bool
_elm_notify_elm_widget_focus_next_manager_is(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd EINA_UNUSED)
{
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_elm_notify_elm_widget_focus_next(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd, Elm_Focus_Direction dir, Evas_Object **next)
{
   Evas_Object *cur;

   if (!sd->content) return EINA_FALSE;

   cur = sd->content;

   /* Try to cycle focus on content */
   return elm_widget_focus_next_get(cur, dir, next);
}

EOLIAN static Eina_Bool
_elm_notify_elm_widget_focus_direction_manager_is(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd EINA_UNUSED)
{
   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_elm_notify_elm_widget_focus_direction(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd, const Evas_Object *base, double degree, Evas_Object **direction, double *weight)
{
   Evas_Object *cur;

   if (!sd->content) return EINA_FALSE;

   cur = sd->content;

   return elm_widget_focus_direction_get(cur, base, degree, direction, weight);
}

EOLIAN static Eina_Bool
_elm_notify_elm_container_content_set(Eo *obj, Elm_Notify_Data *sd, const char *part, Evas_Object *content)
{
   if (part && strcmp(part, "default")) return EINA_FALSE;
   if (sd->content == content) return EINA_TRUE;

   evas_object_del(sd->content);
   sd->content = content;

   if (content)
     {
        elm_widget_sub_object_add(obj, content);
        evas_object_event_callback_add
          (content, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        edje_object_part_swallow(sd->notify, "elm.swallow.content", content);
     }

   _sizing_eval(obj);
   _calc(obj);

   return EINA_TRUE;
}

EOLIAN static Evas_Object*
_elm_notify_elm_container_content_get(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd, const char *part)
{
   if (part && strcmp(part, "default")) return NULL;

   return sd->content;
}

EOLIAN static Evas_Object*
_elm_notify_elm_container_content_unset(Eo *obj, Elm_Notify_Data *sd, const char *part)
{
   Evas_Object *content;

   if (part && strcmp(part, "default")) return NULL;
   if (!sd->content) return NULL;

   content = sd->content;
   elm_widget_sub_object_del(obj, sd->content);
   edje_object_part_unswallow(sd->notify, content);

   return content;
}

static void
_hide_finished_cb(void *data,
                  Evas_Object *obj EINA_UNUSED,
                  const char *emission EINA_UNUSED,
                  const char *source EINA_UNUSED)
{
   ELM_NOTIFY_DATA_GET(data, sd);
   sd->had_hidden = EINA_TRUE;
   evas_object_hide(sd->notify);
   if (!sd->allow_events) evas_object_hide(sd->block_events);
}

EOLIAN static void
_elm_notify_evas_object_smart_add(Eo *obj, Elm_Notify_Data *priv)
{
   eo_do_super(obj, MY_CLASS, evas_obj_smart_add());
   elm_widget_sub_object_parent_add(obj);

   priv->allow_events = EINA_TRUE;
   priv->delay_animator = NULL;

   priv->notify = edje_object_add(evas_object_evas_get(obj));
   evas_object_smart_member_add(priv->notify, obj);

   edje_object_signal_callback_add
      (priv->notify, "elm,action,hide,finished", "elm", _hide_finished_cb, obj);
   edje_object_signal_callback_add
      (priv->notify, "elm,action,show,finished", "", _notify_show_finished_cb, obj);
   edje_object_signal_callback_add
      (priv->notify, "elm,action,show,defer,signal,emit", "", _notify_show_defer_emit_cb, obj);
   edje_object_signal_callback_add
      (priv->notify, "elm,state,hide,finished", "elm", _hide_effect_finished_cb, obj);

#ifdef MESH_GRADIENT
   edje_object_signal_callback_add(priv->notify, "color_class,set", "B011", _content_color_class_changed_cb, NULL);
#endif

   elm_widget_can_focus_set(obj, EINA_FALSE);
   elm_notify_align_set(obj, 0.5, 0.0);
}

EOLIAN static void
_elm_notify_evas_object_smart_del(Eo *obj, Elm_Notify_Data *sd)
{
   edje_object_signal_callback_del_full
      (sd->notify, "elm,action,hide,finished", "elm", _hide_finished_cb, obj);
   elm_notify_parent_set(obj, NULL);
   elm_notify_allow_events_set(obj, EINA_FALSE);
   ecore_timer_del(sd->timer);

   if (sd->delay_animator)
   {
        ecore_animator_del(sd->delay_animator);
        sd->delay_animator = NULL;
   }

#ifdef MESH_GRADIENT
   Evas_Object *bg;
   gxt_shared_image_buffer_h handler;

   if (edje_object_part_exists(sd->notify, "mesh_bg_changeable"))
     {
        bg = edje_object_part_swallow_get(sd->notify, "mesh_bg_changeable");
        if (bg)
          {
             handler = evas_object_data_get(sd->notify, "handler");
             gxt_shared_image_buffer_deinitialize(handler);

             edje_object_part_unswallow(sd->notify, bg);

             evas_object_image_native_surface_set(bg, NULL);
             evas_object_data_set(sd->notify, "handler", NULL);
             evas_object_data_set(sd->notify, "tbm_surface", NULL);

             ELM_SAFE_FREE(bg, evas_object_del);
          }
     }
#endif

   ELM_SAFE_FREE(sd->notify, evas_object_del);
   ELM_SAFE_FREE(sd->block_events, evas_object_del);
   eo_do_super(obj, MY_CLASS, evas_obj_smart_del());
}

EAPI Evas_Object *
elm_notify_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = eo_add(MY_CLASS, parent);
   return obj;
}

EOLIAN static void
_elm_notify_eo_base_constructor(Eo *obj, Elm_Notify_Data *sd EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME_LEGACY),
         elm_interface_atspi_accessible_role_set(ELM_ATSPI_ROLE_NOTIFICATION));
}

EAPI void
elm_notify_parent_set(Evas_Object *obj,
                      Evas_Object *parent)
{
   ELM_NOTIFY_CHECK(obj);
   eo_do(obj, elm_obj_widget_parent_set(parent));
}

EOLIAN static void
_elm_notify_elm_widget_parent_set(Eo *obj, Elm_Notify_Data *sd, Evas_Object *parent)
{
   if (sd->parent)
     {
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_RESIZE, _changed_size_hints_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_MOVE, _changed_size_hints_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_DEL, _parent_del_cb, obj);
        evas_object_event_callback_del_full
          (sd->parent, EVAS_CALLBACK_HIDE, _parent_hide_cb, obj);
        sd->parent = NULL;
     }

   if (parent)
     {
        sd->parent = parent;
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_CHANGED_SIZE_HINTS,
          _changed_size_hints_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_RESIZE, _changed_size_hints_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_MOVE, _changed_size_hints_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_DEL, _parent_del_cb, obj);
        evas_object_event_callback_add
          (parent, EVAS_CALLBACK_HIDE, _parent_hide_cb, obj);
        _sizing_eval(obj);
     }

   _calc(obj);
}

EAPI Evas_Object *
elm_notify_parent_get(const Evas_Object *obj)
{
   ELM_NOTIFY_CHECK(obj) NULL;
   Evas_Object *ret = NULL;
   eo_do((Eo *) obj, ret = elm_obj_widget_parent_get());
   return ret;
}

EOLIAN static Evas_Object*
_elm_notify_elm_widget_parent_get(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd)
{
   return sd->parent;
}

EINA_DEPRECATED EAPI void
elm_notify_orient_set(Evas_Object *obj,
                      Elm_Notify_Orient orient)
{
   double horizontal = 0, vertical = 0;

   switch (orient)
     {
      case ELM_NOTIFY_ORIENT_TOP:
         horizontal = 0.5; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_CENTER:
         horizontal = 0.5; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM:
         horizontal = 0.5; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LEFT:
         horizontal = 0.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_RIGHT:
         horizontal = 1.0; vertical = 0.5;
        break;

      case ELM_NOTIFY_ORIENT_TOP_LEFT:
         horizontal = 0.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_TOP_RIGHT:
         horizontal = 1.0; vertical = 0.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_LEFT:
         horizontal = 0.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_BOTTOM_RIGHT:
         horizontal = 1.0; vertical = 1.0;
        break;

      case ELM_NOTIFY_ORIENT_LAST:
        break;
     }
   elm_notify_align_set(obj, horizontal, vertical);
}

EINA_DEPRECATED EAPI Elm_Notify_Orient
elm_notify_orient_get(const Evas_Object *obj)
{
   Elm_Notify_Orient orient;
   double horizontal, vertical;

   elm_notify_align_get(obj, &horizontal, &vertical);

   if ((horizontal == 0.5) && (vertical == 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP;
   else if ((horizontal == 0.5) && (vertical == 0.5))
     orient = ELM_NOTIFY_ORIENT_CENTER;
   else if ((horizontal == 0.5) && (vertical == 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM;
   else if ((horizontal == 0.0) && (vertical == 0.5))
     orient = ELM_NOTIFY_ORIENT_LEFT;
   else if ((horizontal == 1.0) && (vertical == 0.5))
     orient = ELM_NOTIFY_ORIENT_RIGHT;
   else if ((horizontal == 0.0) && (vertical == 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_LEFT;
   else if ((horizontal == 1.0) && (vertical == 0.0))
     orient = ELM_NOTIFY_ORIENT_TOP_RIGHT;
   else if ((horizontal == 0.0) && (vertical == 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_LEFT;
   else if ((horizontal == 1.0) && (vertical == 1.0))
     orient = ELM_NOTIFY_ORIENT_BOTTOM_RIGHT;
   else
     orient = ELM_NOTIFY_ORIENT_TOP;

   return orient;
}

EOLIAN static void
_elm_notify_timeout_set(Eo *obj, Elm_Notify_Data *sd, double timeout)
{
   sd->timeout = timeout;
   _timer_init(obj, sd);
}

EOLIAN static double
_elm_notify_timeout_get(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd)
{
   return sd->timeout;
}

EOLIAN static void
_elm_notify_allow_events_set(Eo *obj, Elm_Notify_Data *sd, Eina_Bool allow)
{
   if (allow == sd->allow_events) return;
   sd->allow_events = allow;
   if (!allow)
     {
        Evas_Object *win;
        win = elm_widget_top_get(obj);
        if (win && !strcmp(evas_object_type_get(win), "elm_win"))
          {
             sd->block_events = elm_layout_add(win);
             evas_object_size_hint_weight_set(sd->block_events,
                                              EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
             elm_win_resize_object_add(win, sd->block_events);
             evas_object_smart_member_add(sd->block_events, obj);
          }
        else
          {
             sd->block_events = elm_layout_add(obj);
             elm_widget_resize_object_set(obj, sd->block_events, EINA_TRUE);
          }
        _block_events_theme_apply(obj);
        evas_object_stack_above(sd->notify, sd->block_events);
        elm_layout_signal_callback_add
          (sd->block_events, "elm,action,click", "elm",
          _block_area_clicked_cb, obj);
        elm_layout_signal_callback_add
          (sd->block_events, "elm,state,hide,finished", "elm",
          _hide_effect_finished_cb, obj);
     }
   else
     evas_object_del(sd->block_events);
}

EOLIAN static Eina_Bool
_elm_notify_allow_events_get(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd)
{
   return sd->allow_events;
}

EOLIAN static void
_elm_notify_align_set(Eo *obj, Elm_Notify_Data *sd, double horizontal, double vertical)
{
   sd->horizontal_align = horizontal;
   sd->vertical_align = vertical;

   _notify_theme_apply(obj);
   _calc(obj);
}

EOLIAN static void
_elm_notify_align_get(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd, double *horizontal, double *vertical)
{
   if (horizontal)
     *horizontal = sd->horizontal_align;
   if (vertical)
     *vertical = sd->vertical_align;
}

EOLIAN static void
_elm_notify_dismiss(Eo *obj EINA_UNUSED, Elm_Notify_Data *sd)
{
   elm_layout_signal_emit(sd->block_events, "elm,state,hide", "elm");
   edje_object_signal_emit(sd->notify, "elm,state,hide", "elm");
}

static void
_elm_notify_class_constructor(Eo_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);
}

#include "elm_notify.eo.c"
