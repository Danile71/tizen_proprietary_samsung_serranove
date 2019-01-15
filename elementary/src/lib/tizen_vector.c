#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_radio.h"

#define EFL_BETA_API_SUPPORT 1

#ifdef TIZEN_VECTOR_UX

static const char *vg_key = "_tizen_vg";

static double
ELM_VG_SCALE_SIZE(Evas_Object* obj, double x)
{
   Evas_Object *edje = elm_layout_edje_get(obj);

   return ((x) * elm_config_scale_get() / edje_object_base_scale_get(edje) * elm_widget_scale_get(obj));
}
/////////////////////////////////////////////////////////////////////////
/* Radio */
/////////////////////////////////////////////////////////////////////////
typedef struct vg_radio_s
{
   Evas_Object *vg[3];       //0: bg, 1: outline, 2: icon
   Efl_VG_Shape *shape[3];   //0: bg, 1: outline, 2: icon
   Elm_Transit *transit;
   Evas_Object *obj;
   Eina_Bool init : 1;
} vg_radio;

static void
radio_init(vg_radio *vd)
{
   if (vd->init) return;
   vd->init = EINA_TRUE;

   // bg
   vd->shape[0] = evas_vg_shape_add(evas_object_vg_root_node_get(vd->vg[0]));
   evas_vg_node_color_set(vd->shape[0], 255, 255, 255, 255);

   //Outline Shape
   vd->shape[1] = evas_vg_shape_add(evas_object_vg_root_node_get(vd->vg[1]));
   evas_vg_shape_stroke_color_set(vd->shape[1], 255, 255, 255, 255);

   //Iconic Circle (Center Point)
   vd->shape[2] = evas_vg_shape_add(evas_object_vg_root_node_get(vd->vg[2]));
   evas_vg_node_color_set(vd->shape[2], 255, 255, 255, 255);

}

static void
_radio_bg_update(vg_radio *vd)
{
   Evas_Coord w, h;
   ELM_RADIO_DATA_GET(vd->obj, rd);
   Eina_Bool on_case = rd->state;
   Efl_VG * shape = vd->shape[0];
   evas_object_geometry_get(vd->vg[0], NULL, NULL, &w, &h);
   evas_vg_shape_shape_reset(shape);
   double r = w/2;
   if (on_case)
     r -= 1; // 1 pixel margin
   else
     r -= 2; // 2 pixel margin

   evas_vg_shape_shape_append_circle(shape, w/2, h/2, r);
}

static void
_radio_icon_update(vg_radio *vd, double progress)
{
   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[0], NULL, NULL, &w, &h);
   double center_x = ((double)w / 2);
   double center_y = ((double)h / 2);
   double offset = 1;
   double outline_stroke;
   ELM_RADIO_DATA_GET(vd->obj, rd);
   if (!rd->state)
     progress = 1 - progress;
   else
     offset = 2; // on case 2 pixel margin

   outline_stroke = ELM_VG_SCALE_SIZE(vd->obj, 1) + progress * ELM_VG_SCALE_SIZE(vd->obj, 1.5);
   double radius = (center_x < center_y ? center_x : center_y) - outline_stroke - offset;

   //Iconic Circle (Outline)
   evas_vg_shape_stroke_width_set(vd->shape[1], outline_stroke);
   evas_vg_shape_shape_reset(vd->shape[1]);
   evas_vg_shape_shape_append_circle(vd->shape[1], center_x, center_y, radius);

   //Iconic Circle (Center)
   radius = (radius - outline_stroke - 4) * progress;
   evas_vg_shape_shape_reset(vd->shape[2]);
   evas_vg_shape_shape_append_circle(vd->shape[2], center_x, center_y, radius);
}

static void
transit_radio_op(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED,
                 double progress)
{
   vg_radio *vd = effect;
   _radio_icon_update(vd, progress);
}

static void
transit_radio_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   vg_radio *vd = data;
   vd->transit = NULL;
}

static void
radio_action_toggle_cb(void *data, Evas_Object *obj EINA_UNUSED,
                       const char *emission EINA_UNUSED,
                       const char *source)
{
   vg_radio *vd = data;
   if (!source) return;
   if (strcmp(source, "tizen_vg")) return;

   //Circle Effect
   elm_transit_del(vd->transit);
   vd->transit = elm_transit_add();
   elm_transit_effect_add(vd->transit, transit_radio_op, vd,
                          NULL);
   elm_transit_del_cb_set(vd->transit, transit_radio_del_cb, vd);
   elm_transit_tween_mode_set(vd->transit,
                              ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   elm_transit_duration_set(vd->transit, 0.2);
   elm_transit_go(vd->transit);

   _radio_bg_update(vd);
}

static void
radio_state_toggle_cb(void *data, Evas_Object *obj EINA_UNUSED,
                      const char *emission EINA_UNUSED,
                      const char *source)
{
   vg_radio *vd = data;
   if (!source) return;
   if (strcmp(source, "tizen_vg")) return;

   //Circle Effect
   _radio_icon_update(vd, 1.0);
   _radio_bg_update(vd);
}

static void
radio_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
             void *event_info EINA_UNUSED)
{
   vg_radio *vd = data;
   evas_object_data_set(vd->obj, vg_key, NULL);
   elm_object_signal_callback_del(vd->obj, "elm,radio,state,toggle", "tizen_vg", radio_state_toggle_cb);
   elm_object_signal_callback_del(vd->obj, "elm,radio,action,toggle", "tizen_vg", radio_action_toggle_cb);
   elm_transit_del(vd->transit);
   evas_object_del(vd->vg[1]);
   evas_object_del(vd->vg[2]);
   free(vd);
}

static void
radio_base_resize_cb(void *data, Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED,
                     void *event_info EINA_UNUSED)
{
   vg_radio *vd = data;

   radio_init(vd);
   _radio_icon_update(vd, 1.0);
}

static void
radio_vg_bg_resize_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   vg_radio *vd = data;
   radio_init(vd);
   _radio_bg_update(vd);
}

void
tizen_vg_radio_set(Elm_Radio *obj)
{
   vg_radio *vd = evas_object_data_get(obj, vg_key);
   if (vd) evas_object_del(vd->vg[0]);

   //Apply vector ux only theme has "vector_ux"
   const char *str = elm_layout_data_get(obj, "vector_ux");
   if (!str) return;

   vd = calloc(1, sizeof(vg_radio));
   if (!vd)
     {
        ERR("Failed to allocate vector graphics data memory");
        return;
     }
   evas_object_data_set(obj, vg_key, vd);
   elm_object_signal_callback_add(obj, "elm,radio,state,toggle", "tizen_vg", radio_state_toggle_cb, vd);
   elm_object_signal_callback_add(obj, "elm,radio,action,toggle", "tizen_vg", radio_action_toggle_cb, vd);


   //Vector Graphics Object
   Evas *e = evas_object_evas_get(obj);

   vd->obj = obj;

   //Check bg
   vd->vg[0] = evas_object_vg_add(evas_object_evas_get(obj));
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_RESIZE,
                                  radio_vg_bg_resize_cb, vd);

   //Outline VG
   vd->vg[1] = evas_object_vg_add(e);
   evas_object_event_callback_add(vd->vg[1], EVAS_CALLBACK_DEL,
                                  radio_del_cb, vd);
   evas_object_event_callback_add(vd->vg[1], EVAS_CALLBACK_RESIZE,
                                  radio_base_resize_cb, vd);

   //Iconic Circle
   vd->vg[2] = evas_object_vg_add(e);

   elm_object_part_content_set(obj, "tizen_vg_shape_bg", vd->vg[0]);
   elm_object_part_content_set(obj, "tizen_vg_shape", vd->vg[1]);
   elm_object_part_content_set(obj, "tizen_vg_shape2", vd->vg[2]);
}


/////////////////////////////////////////////////////////////////////////
/* Check: Favorite */
/////////////////////////////////////////////////////////////////////////
typedef struct check_favorite_s
{
   Evas_Object *vg[2]; //0: outline, 1: inner body
   Efl_VG_Shape *shape[2];  //0: outline, 1: inner body
   Elm_Transit *transit;
   Evas_Object *obj;
   Eina_Bool init : 1;
   double scale_x;
   double scale_y;
} check_favorite;

//favorite off svg path info
static const char *star_off = "M26.003,38.931c0.29,0,0.58,0.084,0.832,0.252\
  l12.212,8.142c0.367,0.245,0.859,0.229,1.199-0.021c0.359-0.262,0.516-0.721,0.394-1.146l-4.104-14.367\
  c-0.158-0.557,0.017-1.156,0.452-1.538l10.174-8.949c0.313-0.284,0.424-0.74,0.267-1.148c-0.159-0.415-0.546-0.681-0.987-0.681\
  H33.433c-0.623,0-1.181-0.385-1.402-0.967L26.966,5.17c-0.153-0.398-0.549-0.67-0.98-0.67C25.55,4.501,25.154,4.773,25,5.177\
  l-5.034,13.328c-0.221,0.583-0.779,0.97-1.404,0.97H5.557c-0.44,0-0.827,0.266-0.985,0.677c-0.158,0.413-0.049,0.869,0.279,1.164\
  l10.158,8.937c0.434,0.383,0.609,0.981,0.451,1.538l-4.104,14.367c-0.123,0.425,0.033,0.883,0.387,1.14\
  c0.357,0.258,0.85,0.271,1.211,0.028l12.217-8.144C25.423,39.015,25.713,38.931,26.003,38.931z";

//favorite on svg path info
static const char *star_on = "M39.633,49c-0.492,0-0.987-0.145-1.417-0.432l-12.212-8.141l-12.215,8.141\
  c-0.892,0.598-2.059,0.57-2.926-0.061c-0.866-0.629-1.245-1.734-0.949-2.766l4.104-14.365L3.846,22.429\
  c-0.788-0.71-1.055-1.828-0.676-2.814c0.378-0.987,1.326-1.641,2.385-1.641h13.007l5.036-13.331c0.377-0.989,1.326-1.64,2.384-1.643\
  h0.003c1.056,0,2.003,0.651,2.384,1.637l5.063,13.337h13.011c1.061,0,2.007,0.652,2.387,1.641c0.38,0.988,0.109,2.104-0.677,2.814\
  L37.98,31.377l4.104,14.365c0.294,1.031-0.085,2.137-0.947,2.766C40.692,48.834,40.162,49,39.633,49z";

static void
check_favorite_init(check_favorite *vd)
{
   if (vd->init) return;
   vd->init = EINA_TRUE;

   //Outline Star
   vd->shape[0] = evas_vg_shape_add(evas_object_vg_root_node_get(vd->vg[0]));
   evas_vg_node_origin_set(vd->shape[0], 14, 14);
   evas_vg_shape_stroke_color_set(vd->shape[0], 255, 255, 255, 255);
   evas_vg_shape_stroke_width_set(vd->shape[0], ELM_VG_SCALE_SIZE(vd->obj, 1.5));
   evas_vg_shape_shape_append_svg_path(vd->shape[0], star_off);

   //Inner Body Star
   vd->shape[1] = evas_vg_shape_add(evas_object_vg_root_node_get(vd->vg[1]));
   evas_vg_node_origin_set(vd->shape[1], 14, 14);
   evas_vg_shape_shape_append_svg_path(vd->shape[1], star_on);
   evas_vg_node_color_set(vd->shape[1], 255, 255, 255, 255);
}

static void
transit_check_favorite_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   check_favorite *vd = data;
   vd->transit = NULL;
}

static void
_check_favorite(check_favorite *vd, double progress)
{
   // star on svg has viewbox as -14 -14 80 80, so the center
   // of the shape is (80 - 2 *14) = 26
   double center_x = 26;
   double center_y = 26;

   if (!elm_check_state_get(vd->obj)) progress = 1 - progress;

   Eina_Matrix3 m;
   eina_matrix3_identity(&m);
   eina_matrix3_scale(&m, vd->scale_x, vd->scale_y);
   eina_matrix3_translate(&m, center_x, center_y);
   eina_matrix3_scale(&m, progress, progress);
   eina_matrix3_translate(&m, -center_x, -center_y);
   evas_vg_node_transformation_set(vd->shape[1], &m);
}

static void
transit_check_favorite_op(Elm_Transit_Effect *effect,
                          Elm_Transit *transit EINA_UNUSED, double progress)
{
   check_favorite *vd = effect;
   _check_favorite(vd, progress);
}

static void
_transit_check_favorite_animation_finished(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED)
{
   check_favorite *vd = effect;

   elm_layout_signal_emit(vd->obj, "elm,action,animation,finished", "elm");
}

static void
check_favorite_vg_resize_cb(void *data, Evas *e EINA_UNUSED,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   check_favorite *vd = data;
   check_favorite_init(vd);
   // star on svg has viewbox as -14 -14 80 80, so the origin
   // is shifted by 14.
   double originx = 14;
   double originy = 14;

   evas_object_geometry_get(vd->vg[0], NULL, NULL, &w, &h);
   vd->scale_x = w/80.0;
   vd->scale_y = h/80.0;

   originx = originx * vd->scale_x;
   originy = originy * vd->scale_y;

   // update origin
   evas_vg_node_origin_set(vd->shape[0], originx, originy);
   evas_vg_node_origin_set(vd->shape[1], originx, originy);

   // apply it to outline star
   Eina_Matrix3 m;
   eina_matrix3_identity(&m);
   eina_matrix3_scale(&m, vd->scale_x, vd->scale_y);
   evas_vg_node_transformation_set(vd->shape[0], &m);

   // update the inner star
   _check_favorite(vd, 1);
}

static void
check_favorite_action_toggle_cb(void *data, Evas_Object *obj EINA_UNUSED,
                                const char *emission EINA_UNUSED,
                                const char *source EINA_UNUSED)
{
   check_favorite *vd = data;
   if (!source) return;
   if (strcmp(source, "tizen_vg")) return;

   check_favorite_init(vd);

   Eina_Bool check = elm_check_state_get(obj);

   //Circle Effect
   elm_transit_del(vd->transit);
   vd->transit = elm_transit_add();
   elm_transit_effect_add(vd->transit, transit_check_favorite_op, vd,
                          _transit_check_favorite_animation_finished);
   elm_transit_del_cb_set(vd->transit, transit_check_favorite_del_cb, vd);
   elm_transit_tween_mode_set(vd->transit,
                              ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   if (check)
     {
        elm_transit_duration_set(vd->transit, 0.3);
        elm_transit_go_in(vd->transit, 0.1);
     }
   else
     {
        elm_transit_duration_set(vd->transit, 0.3);
        elm_transit_go(vd->transit);
     }
}

static void
check_favorite_state_toggle_cb(void *data, Evas_Object *obj EINA_UNUSED,
                               const char *emission EINA_UNUSED,
                               const char *source EINA_UNUSED)
{
   check_favorite *vd = data;
   if (!source) return;
   if (strcmp(source, "tizen_vg")) return;

   check_favorite_init(vd);

   _check_favorite(vd, 1.0);
}

static void
check_favorite_del_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   check_favorite *vd = data;
   evas_object_data_set(vd->obj, vg_key, NULL);
   elm_object_signal_callback_del(vd->obj, "elm,check,state,toggle", "tizen_vg", check_favorite_state_toggle_cb);
   elm_object_signal_callback_del(vd->obj, "elm,check,action,toggle", "tizen_vg", check_favorite_action_toggle_cb);
   elm_transit_del(vd->transit);
   evas_object_del(vd->vg[1]);
   free(vd);
}

void
tizen_vg_check_favorite_set(Elm_Check *obj)
{
   check_favorite *vd = calloc(1, sizeof(check_favorite));
   if (!vd)
     {
        ERR("Failed to allocate vector graphics data memory");
        return;
     }
   evas_object_data_set(obj, vg_key, vd);
   elm_object_signal_callback_add(obj, "elm,check,state,toggle", "tizen_vg", check_favorite_state_toggle_cb, vd);
   elm_object_signal_callback_add(obj, "elm,check,action,toggle", "tizen_vg", check_favorite_action_toggle_cb, vd);
   vd->obj = obj;

   //Outline Star
   vd->vg[0] = evas_object_vg_add(evas_object_evas_get(obj));
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_DEL,
                                  check_favorite_del_cb, vd);
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_RESIZE,
                                  check_favorite_vg_resize_cb, vd);
   //Inner Body Star
   vd->vg[1] = evas_object_vg_add(evas_object_evas_get(obj));

   elm_object_part_content_set(obj, "tizen_vg_shape", vd->vg[0]);
   elm_object_part_content_set(obj, "tizen_vg_shape2", vd->vg[1]);
}


/////////////////////////////////////////////////////////////////////////
/* Check: OnOff */
/////////////////////////////////////////////////////////////////////////
typedef struct check_onoff_s
{
   Evas_Object *vg[3]; //0: bg, 1: overlapped circle, 2: line-circle
   Efl_VG_Shape *shape[4];  //0: bg, 1: overlapped circle, 2: line, 3: circle
   Elm_Transit *transit[3];  //0: circle, 1: line, 2: overlapped circle
   Evas_Object *obj;
   Eina_Bool init : 1;
} check_onoff;


static void
check_onoff_init(check_onoff *vd)
{
   if (vd->init) return;
   vd->init = EINA_TRUE;

   //BG Circle
   vd->shape[0] = evas_vg_shape_add(evas_object_vg_root_node_get(vd->vg[0]));
   evas_vg_node_color_set(vd->shape[0], 255, 255, 255, 255);

   //Overlap Circle
   vd->shape[1] = evas_vg_shape_add(evas_object_vg_root_node_get(vd->vg[1]));
   evas_vg_node_color_set(vd->shape[1], 255, 255, 255, 255);

   //Line Shape
   vd->shape[2] = evas_vg_shape_add(evas_object_vg_root_node_get(vd->vg[2]));
   evas_vg_shape_stroke_color_set(vd->shape[2], 255, 255, 255, 255);
   evas_vg_shape_stroke_width_set(vd->shape[2], ELM_VG_SCALE_SIZE(vd->obj, 3));
   evas_vg_shape_stroke_cap_set(vd->shape[2], EFL_GFX_CAP_ROUND);

   //Circle Shape
   vd->shape[3] = evas_vg_shape_add(evas_object_vg_root_node_get(vd->vg[2]));
   evas_vg_shape_stroke_color_set(vd->shape[3], 255, 255, 255, 255);
   evas_vg_shape_stroke_width_set(vd->shape[3], ELM_VG_SCALE_SIZE(vd->obj, 3));
}

static void
_check_onoff_circle(check_onoff *vd, double progress)
{
   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[2], NULL, NULL, &w, &h);
   double center_x = ((double)w / 2);
   double center_y = ((double)h / 2);

   evas_vg_shape_shape_reset(vd->shape[3]);

   double radius = (center_x < center_y ? center_x : center_y) -
                   (ELM_VG_SCALE_SIZE(vd->obj, 3));

   if (elm_check_state_get(vd->obj)) progress = 1 - progress;

   evas_vg_shape_shape_append_circle(vd->shape[3], center_x, center_y,
                                     radius * progress);
}

static void
transit_check_onoff_circle_op(Elm_Transit_Effect *effect,
                              Elm_Transit *transit EINA_UNUSED, double progress)
{
   check_onoff *vd = effect;
   _check_onoff_circle(vd, progress);
}

static void
transit_check_onoff_circle_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   check_onoff *vd = data;
   vd->transit[0] = NULL;
}

static void
_check_onoff_line(check_onoff *vd, double progress)
{
   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[2], NULL, NULL, &w, &h);
   double center_x = ((double)w / 2);
   double center_y = ((double)h / 2);

   evas_vg_shape_shape_reset(vd->shape[2]);

   if (!elm_check_state_get(vd->obj)) progress = 1 - progress;

   double diff = center_y - ELM_VG_SCALE_SIZE(vd->obj, 3) - 1;

   evas_vg_shape_shape_append_move_to(vd->shape[2], center_x,
                                      (center_y - (diff * progress)));
   evas_vg_shape_shape_append_line_to(vd->shape[2], center_x,
                                      (center_y + (diff * progress)));
}

static void
transit_check_onoff_line_op(void *data, Elm_Transit *transit EINA_UNUSED,
                            double progress)
{
   check_onoff *vd = data;
   _check_onoff_line(vd, progress);
}

static void
_transit_check_onoff_animation_finished(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED)
{
   check_onoff *vd = effect;

   elm_layout_signal_emit(vd->obj, "elm,action,animation,finished", "elm");
}

static void
transit_check_onoff_line_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   check_onoff *vd = data;
   vd->transit[1] = NULL;
}

static void
_check_onoff_sizing(check_onoff *vd, double progress)
{
   check_onoff_init(vd);

   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[1], NULL, NULL, &w, &h);
   double center_x = ((double)w / 2);
   double center_y = ((double)h / 2);

   if (!elm_check_state_get(vd->obj)) progress = 1 - progress;
   progress *= 0.3;

   evas_vg_shape_shape_reset(vd->shape[1]);
   evas_vg_shape_shape_append_circle(vd->shape[1],
                                     center_x, center_y,
                                     (0.7 + progress) * center_x);
}

static void
transit_check_onoff_sizing_op(void *data, Elm_Transit *transit EINA_UNUSED,
                              double progress)
{
   check_onoff *vd = data;
   _check_onoff_sizing(vd, progress);
}

static void
transit_check_onoff_sizing_del_cb(void *data,
                                  Elm_Transit *transit EINA_UNUSED)
{
   check_onoff *vd = data;
   vd->transit[2] = NULL;
}

static void
check_onoff_action_toggle_cb(void *data, Evas_Object *obj EINA_UNUSED,
                             const char *emission EINA_UNUSED,
                             const char *source EINA_UNUSED)
{
   check_onoff *vd = data;
   if (!source) return;
   if (strcmp(source, "tizen_vg")) return;

   check_onoff_init(vd);

   Eina_Bool check = elm_check_state_get(obj);

   //Circle Effect
   elm_transit_del(vd->transit[0]);
   vd->transit[0] = elm_transit_add();
   elm_transit_effect_add(vd->transit[0], transit_check_onoff_circle_op, vd,
                          NULL);
   elm_transit_del_cb_set(vd->transit[0], transit_check_onoff_circle_del_cb,
                          vd);
   elm_transit_tween_mode_set(vd->transit[0],
                              ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   if (check)
     {
        elm_transit_duration_set(vd->transit[0], 0.1);
        elm_transit_go(vd->transit[0]);
     }
   else
     {
        elm_transit_duration_set(vd->transit[0], 0.25);
        elm_transit_go_in(vd->transit[0], 0.05);
     }

   //Line Effect
   elm_transit_del(vd->transit[1]);
   vd->transit[1] = elm_transit_add();
   elm_transit_effect_add(vd->transit[1], transit_check_onoff_line_op, vd,
                          NULL);
   elm_transit_del_cb_set(vd->transit[1], transit_check_onoff_line_del_cb,
                          vd);
   elm_transit_tween_mode_set(vd->transit[1],
                              ELM_TRANSIT_TWEEN_MODE_DECELERATE);
   if (check)
     {
        elm_transit_duration_set(vd->transit[1], 0.25);
        elm_transit_go_in(vd->transit[1], 0.05);

        //Overlap Circle Sizing Effect
        elm_transit_del(vd->transit[2]);
        vd->transit[2] = elm_transit_add();
        elm_transit_effect_add(vd->transit[2], transit_check_onoff_sizing_op, vd,
                               _transit_check_onoff_animation_finished);
        elm_transit_del_cb_set(vd->transit[2], transit_check_onoff_sizing_del_cb,
                               vd);
        elm_transit_tween_mode_set(vd->transit[2],
                                   ELM_TRANSIT_TWEEN_MODE_DECELERATE);
        elm_transit_duration_set(vd->transit[2], 0.3);
        elm_transit_go(vd->transit[2]);
     }
   else
     {
        elm_transit_duration_set(vd->transit[1], 0.1);
        elm_transit_go(vd->transit[1]);
     }
}

static void
check_onoff_state_toggle_cb(void *data, Evas_Object *obj EINA_UNUSED,
                            const char *emission EINA_UNUSED,
                            const char *source EINA_UNUSED)
{
   check_onoff *vd = data;
   if (!source) return;
   if (strcmp(source, "tizen_vg")) return;

   check_onoff_init(vd);

   _check_onoff_circle(vd, 1.0);
   _check_onoff_line(vd, 1.0);
   _check_onoff_sizing(vd, 1.0);
}

static void
check_onoff_del_cb(void *data, Evas *e EINA_UNUSED,
                   Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   check_onoff *vd = data;
   evas_object_data_set(vd->obj, vg_key, NULL);
   elm_object_signal_callback_del(vd->obj, "elm,check,state,toggle", "tizen_vg", check_onoff_state_toggle_cb);
   elm_object_signal_callback_del(vd->obj, "elm,check,action,toggle", "tizen_vg", check_onoff_action_toggle_cb);
   elm_transit_del(vd->transit[0]);
   elm_transit_del(vd->transit[1]);
   elm_transit_del(vd->transit[2]);
   evas_object_del(vd->vg[1]);
   evas_object_del(vd->vg[2]);
   free(vd);
}

static void
check_onoff_vg_resize_cb(void *data, Evas *e EINA_UNUSED,
                         Evas_Object *obj EINA_UNUSED,
                         void *event_info EINA_UNUSED)
{
   check_onoff *vd = data;

   check_onoff_init(vd);

   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[0], NULL, NULL, &w, &h);
   Evas_Coord center_x = (w / 2);
   Evas_Coord center_y = (h / 2);

   evas_vg_shape_shape_reset(vd->shape[0]);
   evas_vg_shape_shape_append_circle(vd->shape[0], center_x, center_y,
                                     center_x);
}

static void
check_onoff_vg2_resize_cb(void *data, Evas *e EINA_UNUSED,
                          Evas_Object *obj EINA_UNUSED,
                          void *event_info EINA_UNUSED)
{
   check_onoff *vd = data;

   check_onoff_init(vd);

   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[1], NULL, NULL, &w, &h);
   double center_x = ((double)w / 2);
   double center_y = ((double)h / 2);

   evas_vg_shape_shape_reset(vd->shape[1]);
   evas_vg_shape_shape_append_circle(vd->shape[1],
                                     center_x, center_y,
                                     center_x);
}

static void
check_onoff_vg3_resize_cb(void *data, Evas *e EINA_UNUSED,
                          Evas_Object *obj EINA_UNUSED,
                          void *event_info EINA_UNUSED)
{
   check_onoff *vd = data;

   check_onoff_init(vd);

   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[2], NULL, NULL, &w, &h);
   double center_x = ((double)w / 2);
   double center_y = ((double)h / 2);

   evas_vg_shape_shape_reset(vd->shape[2]);
   evas_vg_shape_shape_reset(vd->shape[3]);

   //Line
   if (elm_check_state_get(vd->obj))
     {
        double diff = ELM_VG_SCALE_SIZE(vd->obj, 3) - 1;

        evas_vg_shape_shape_append_move_to(vd->shape[2], center_x, diff);
        evas_vg_shape_shape_append_line_to(vd->shape[2], center_x, h - diff);
     }
   //Circle
   else
     {
        double radius = (center_x < center_y ? center_x : center_y) -
                        (ELM_VG_SCALE_SIZE(vd->obj, 3));
        evas_vg_shape_shape_append_circle(vd->shape[3],
                                          center_x, center_y, radius);
     }
}

void
tizen_vg_check_onoff_set(Elm_Check *obj)
{
   check_onoff *vd = calloc(1, sizeof(check_onoff));
   if (!vd)
     {
        ERR("Failed to allocate vector graphics data memory");
        return;
     }
   evas_object_data_set(obj, vg_key, vd);

   elm_object_signal_callback_add(obj, "elm,check,state,toggle", "tizen_vg", check_onoff_state_toggle_cb, vd);
   elm_object_signal_callback_add(obj, "elm,check,action,toggle", "tizen_vg", check_onoff_action_toggle_cb, vd);

   vd->obj = obj;

   //Base (BG) VG
   vd->vg[0] = evas_object_vg_add(evas_object_evas_get(obj));
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_DEL,
                                  check_onoff_del_cb, vd);
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_RESIZE,
                                  check_onoff_vg_resize_cb, vd);
   //Overlapped Circle VG
   vd->vg[1] = evas_object_vg_add(evas_object_evas_get(obj));
   evas_object_event_callback_add(vd->vg[1], EVAS_CALLBACK_RESIZE,
                                  check_onoff_vg2_resize_cb, vd);
   //Line-Circle VG
   vd->vg[2] = evas_object_vg_add(evas_object_evas_get(obj));
   evas_object_event_callback_add(vd->vg[2], EVAS_CALLBACK_RESIZE,
                                  check_onoff_vg3_resize_cb, vd);

   elm_object_part_content_set(obj, "tizen_vg_shape", vd->vg[0]);
   elm_object_part_content_set(obj, "tizen_vg_shape2", vd->vg[1]);
   elm_object_part_content_set(obj, "tizen_vg_shape3", vd->vg[2]);
}



/////////////////////////////////////////////////////////////////////////
/* Check: Default */
/////////////////////////////////////////////////////////////////////////
typedef struct check_default_s
{
   Evas_Object *vg[3]; //0: base, 1: line 2: bg
   Efl_VG_Shape *shape[5];  //0: outline, 1: bg, 2: left line, 3: right line 4: bg
   Elm_Transit *transit[3]; //0: bg color, 1: bg scale, 2: check lines
   Evas_Object *obj;
   double left_move_to[2];
   double left_line_to[2];
   double right_move_to[2];
   double right_line_to[2];
   Eina_Bool init : 1;
} check_default;

static const char *check_default_fill = "M7.279,2h35.442C45.637,2,48,4.359,48,7.271v35.455C48,45.639,45.637,48,42.723,48\
  H7.279C4.362,47.997,2,45.639,2,42.727V7.271C2,4.359,4.362,2,7.279,2z";

static const char *check_default_outline= "M-2.073-7h36.147C36.796-7,39-4.793,39-2.073v36.146\
  C39,36.796,36.796,39,34.074,39H-2.073C-4.793,39-7,36.796-7,34.072V-2.073C-7-4.793-4.793-7-2.073-7z";

static void
check_default_init(check_default *vd)
{
   if (vd->init) return;
   vd->init = EINA_TRUE;

   // bg sahpe
   Efl_VG *bg_root = evas_object_vg_root_node_get(vd->vg[2]);
   vd->shape[4] = evas_vg_shape_add(bg_root);


   Efl_VG *base_root = evas_object_vg_root_node_get(vd->vg[0]);
   //outline
   vd->shape[0] = evas_vg_shape_add(base_root);
   //effect
   vd->shape[1] = evas_vg_shape_add(base_root);

   // check icon
   Efl_VG *line_root = evas_object_vg_root_node_get(vd->vg[1]);
   //Left Line Shape
   vd->shape[2] = evas_vg_shape_add(line_root);
   evas_vg_shape_stroke_width_set(vd->shape[2], ELM_VG_SCALE_SIZE(vd->obj, 1.75));
   evas_vg_shape_stroke_color_set(vd->shape[2], 255, 255, 255, 255);
   evas_vg_shape_stroke_cap_set(vd->shape[2], EFL_GFX_CAP_ROUND);

   //Right Line Shape
   vd->shape[3] = evas_vg_shape_add(line_root);
   evas_vg_shape_stroke_width_set(vd->shape[3], ELM_VG_SCALE_SIZE(vd->obj, 1.75));
   evas_vg_shape_stroke_color_set(vd->shape[3], 255, 255, 255, 255);
   evas_vg_shape_stroke_cap_set(vd->shape[3], EFL_GFX_CAP_ROUND);

   vd->left_move_to[0] = ELM_VG_SCALE_SIZE(vd->obj, -5);
   vd->left_move_to[1] = ELM_VG_SCALE_SIZE(vd->obj, 10);
   vd->left_line_to[0] = ELM_VG_SCALE_SIZE(vd->obj, -8);
   vd->left_line_to[1] = ELM_VG_SCALE_SIZE(vd->obj, -8);

   vd->right_move_to[0] = ELM_VG_SCALE_SIZE(vd->obj, -5);
   vd->right_move_to[1] = ELM_VG_SCALE_SIZE(vd->obj, 10);
   vd->right_line_to[0] = ELM_VG_SCALE_SIZE(vd->obj, 18);
   vd->right_line_to[1] = ELM_VG_SCALE_SIZE(vd->obj, -18);
}

static void
_update_default_check_shape(check_default *vd, Efl_VG *shape, Eina_Bool outline)
{
   evas_vg_shape_shape_reset(shape);
   if (outline)
     {
        // outline
        Evas_Coord w, h;
        evas_object_geometry_get(vd->vg[0], NULL, NULL, &w, &h);
        double scale_x = w/50.0;
        double scale_y = h/50.0;

        evas_vg_shape_shape_append_svg_path(shape, check_default_outline);
        evas_vg_node_origin_set(shape, 9 * scale_x, 9 * scale_y);
        Eina_Matrix3 m;
        eina_matrix3_identity(&m);
        eina_matrix3_scale(&m, scale_x, scale_y);
        evas_vg_node_transformation_set(shape, &m);

        evas_vg_shape_stroke_width_set(shape, 1.5);

        // update color
        evas_vg_node_color_set(shape, 0, 0, 0, 0);
        evas_vg_shape_stroke_color_set(shape, 255, 255, 255, 255);
     }
   else
     {
        // fill
        evas_vg_shape_shape_append_svg_path(shape, check_default_fill);
        evas_vg_node_origin_set(shape, 0, 0);
        evas_vg_node_transformation_set(shape, NULL);

        // update color
        evas_vg_node_color_set(shape, 255, 255, 255, 255);
        evas_vg_shape_stroke_color_set(shape, 0, 0, 0, 0);
     }
}

static void
check_default_vg_bg_resize_cb(void *data, Evas *e EINA_UNUSED,
                                Evas_Object *obj EINA_UNUSED,
                                void *event_info EINA_UNUSED)
{
   check_default *vd = data;
   check_default_init(vd);
   _update_default_check_shape(vd, vd->shape[4], elm_check_state_get(vd->obj));
}

static void
check_default_vg_resize_cb(void *data, Evas *e EINA_UNUSED,
                           Evas_Object *obj EINA_UNUSED,
                           void *event_info EINA_UNUSED)
{
   check_default *vd = data;

   check_default_init(vd);

   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[0], NULL, NULL, &w, &h);
   double center_x = ((double)w / 2);
   double center_y = ((double)h / 2);

   //Update Outline Shape
   _update_default_check_shape(vd, vd->shape[0], !elm_check_state_get(vd->obj));

   //Update BG Shape
   _update_default_check_shape(vd, vd->shape[1], EINA_FALSE);

   if (elm_check_state_get(vd->obj))
     evas_vg_node_color_set(vd->shape[1], 255, 255, 255, 255);
   else
     evas_vg_node_color_set(vd->shape[1], 0, 0, 0, 0);

   //Update Line Shape
   if (elm_check_state_get(vd->obj))
     {
        //Left
        evas_vg_shape_shape_reset(vd->shape[2]);
        evas_vg_shape_shape_append_move_to(vd->shape[2],
                                           center_x + vd->left_move_to[0],
                                           center_y + vd->left_move_to[1]);
        evas_vg_shape_shape_append_line_to(vd->shape[2],
                                           (center_x + vd->left_move_to[0]) +
                                           (vd->left_line_to[0]),
                                           (center_y + vd->left_move_to[1]) +
                                           (vd->left_line_to[1]));
        //Right
        evas_vg_shape_shape_reset(vd->shape[3]);
        evas_vg_shape_shape_append_move_to(vd->shape[3],
                                           center_x + vd->right_move_to[0],
                                           center_y + vd->right_move_to[1]);
        evas_vg_shape_shape_append_line_to(vd->shape[3],
                                           (center_x + vd->right_move_to[0]) +
                                           (vd->right_line_to[0]),
                                           (center_y + vd->right_move_to[1]) +
                                           (vd->right_line_to[1]));
     }
}

static void
transit_check_default_bg_color_del_cb(Elm_Transit_Effect *effect,
                                      Elm_Transit *transit EINA_UNUSED)
{
   check_default *vd = effect;
   vd->transit[0] = NULL;
}


static void
_check_default_bg_color(check_default *vd, double progress)
{
   int color;

   if (elm_check_state_get(vd->obj)) color = 255 * progress;
   else color = 255 * (1 - progress);

   evas_vg_node_color_set(vd->shape[1], color, color, color, color);
   evas_vg_shape_stroke_color_set(vd->shape[0], 255 - color, 255 - color, 255 - color, 255 -color);
}

static void
transit_check_default_bg_color_op(Elm_Transit_Effect *effect,
                                  Elm_Transit *transit EINA_UNUSED,
                                  double progress)
{
   check_default *vd = effect;
   _check_default_bg_color(vd, progress);
}

static void
_transit_check_default_animation_finished(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED)
{
   check_default *vd = effect;

   elm_layout_signal_emit(vd->obj, "elm,action,animation,finished", "elm");
}

static void
transit_check_default_bg_scale_del_cb(Elm_Transit_Effect *effect,
                                      Elm_Transit *transit EINA_UNUSED)
{
   check_default *vd = effect;
   vd->transit[1] = NULL;
}

static void
_check_default_bg_scale(check_default *vd, double progress)
{
   // as the viewbox of the check_on svg is 0 0 50 50
   // center of the svg is 25 25
   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[0], NULL, NULL, &w, &h);
   double scale_x = w/50.0;
   double scale_y = h/50.0;

   Eina_Matrix3 m;
   eina_matrix3_identity(&m);
   eina_matrix3_scale(&m, scale_x, scale_y);
   eina_matrix3_translate(&m, 25, 25);
   eina_matrix3_scale(&m, progress, progress);
   eina_matrix3_translate(&m, -25, -25);
   evas_vg_node_transformation_set(vd->shape[1], &m);
}
static void
transit_check_default_bg_scale_op(Elm_Transit_Effect *effect,
                                  Elm_Transit *transit EINA_UNUSED,
                                  double progress)
{
   check_default *vd = effect;
   _check_default_bg_scale(vd, progress);
}

static void
transit_check_default_line_del_cb(Elm_Transit_Effect *effect,
                                  Elm_Transit *transit EINA_UNUSED)
{
   check_default *vd = effect;
   vd->transit[2] = NULL;
}

static void
_check_default_line(check_default *vd, double progress)
{
   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[1], NULL, NULL, &w, &h);
   double center_x = ((double)w / 2);
   double center_y = ((double)h / 2);

   //Update Line Shape
   if (!elm_check_state_get(vd->obj)) progress = 1 - progress;

   //Left
   evas_vg_shape_shape_reset(vd->shape[2]);
   evas_vg_shape_shape_append_move_to(vd->shape[2],
                                      center_x + vd->left_move_to[0],
                                      center_y + vd->left_move_to[1]);
   evas_vg_shape_shape_append_line_to(vd->shape[2],
                                      (center_x + vd->left_move_to[0]) +
                                      (vd->left_line_to[0] * progress),
                                      (center_y + vd->left_move_to[1]) +
                                      (vd->left_line_to[1] * progress));

   //Right
   evas_vg_shape_shape_reset(vd->shape[3]);
   evas_vg_shape_shape_append_move_to(vd->shape[3],
                                      center_x + vd->right_move_to[0],
                                      center_y + vd->right_move_to[1]);
   evas_vg_shape_shape_append_line_to(vd->shape[3],
                                      (center_x + vd->right_move_to[0]) +
                                      (vd->right_line_to[0] * progress),
                                      (center_y + vd->right_move_to[1]) +
                                      (vd->right_line_to[1] * progress));
}

static void
transit_check_default_line_op(Elm_Transit_Effect *effect,
                              Elm_Transit *transit EINA_UNUSED, double progress)
{
   check_default *vd = effect;
   _check_default_line(vd, progress);
}

static void
check_default_action_toggle_cb(void *data, Evas_Object *obj EINA_UNUSED,
                               const char *emission EINA_UNUSED,
                               const char *source EINA_UNUSED)
{
   check_default *vd = data;
   if (!source) return;
   if (strcmp(source, "tizen_vg")) return;

   check_default_init(vd);

   Eina_Bool check = elm_check_state_get(obj);

   //BG Color Effect
   elm_transit_del(vd->transit[0]);
   vd->transit[0] = elm_transit_add();
   elm_transit_effect_add(vd->transit[0], transit_check_default_bg_color_op, vd,
                          _transit_check_default_animation_finished);
   elm_transit_del_cb_set(vd->transit[0], transit_check_default_bg_color_del_cb,
                          vd);
   elm_transit_tween_mode_set(vd->transit[0],
                              ELM_TRANSIT_TWEEN_MODE_DECELERATE);

   if (check)
     {
        elm_transit_duration_set(vd->transit[0], 0.3);
        elm_transit_go(vd->transit[0]);
     }
   else
     {
        elm_transit_duration_set(vd->transit[0], 0.2);
        elm_transit_go_in(vd->transit[0], 0.15);
     }

   //BG Size Effect
   elm_transit_del(vd->transit[1]);

   if (check)
     {
        vd->transit[1] = elm_transit_add();
        elm_transit_effect_add(vd->transit[1],
                               transit_check_default_bg_scale_op, vd, NULL);
        elm_transit_del_cb_set(vd->transit[1],
                               transit_check_default_bg_scale_del_cb, vd);
        elm_transit_tween_mode_set(vd->transit[1],
                                   ELM_TRANSIT_TWEEN_MODE_DECELERATE);
        elm_transit_duration_set(vd->transit[1], 0.15);
        elm_transit_go(vd->transit[1]);
     }

   //Draw Line
   elm_transit_del(vd->transit[2]);

   vd->transit[2] = elm_transit_add();
   elm_transit_effect_add(vd->transit[2], transit_check_default_line_op, vd,
                          NULL);
   elm_transit_del_cb_set(vd->transit[2], transit_check_default_line_del_cb,
                          vd);
   elm_transit_tween_mode_set(vd->transit[2],
                              ELM_TRANSIT_TWEEN_MODE_SINUSOIDAL);
   elm_transit_duration_set(vd->transit[2], 0.15);

   if (check)
     elm_transit_go_in(vd->transit[2], 0.15);
   else
     elm_transit_go(vd->transit[2]);

   _update_default_check_shape(vd, vd->shape[4], elm_check_state_get(vd->obj));
}

static void
check_default_state_toggle_cb(void *data, Evas_Object *obj EINA_UNUSED,
                              const char *emission EINA_UNUSED,
                              const char *source EINA_UNUSED)
{
   check_default *vd = data;
   if (!source) return;
   if (strcmp(source, "tizen_vg")) return;

   check_default_init(vd);

   _check_default_bg_color(vd, 1.0);
   _check_default_bg_scale(vd, 1.0);
   _check_default_line(vd, 1.0);
   _update_default_check_shape(vd, vd->shape[4], elm_check_state_get(vd->obj));

   // update outline color
   if (elm_check_state_get(vd->obj))
     evas_vg_shape_stroke_color_set(vd->shape[0], 0, 0, 0, 0);
   else
     evas_vg_shape_stroke_color_set(vd->shape[0], 255, 255, 255, 255);
}

static void
check_default_del_cb(void *data, Evas *e EINA_UNUSED,
                     Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   check_default *vd = data;
   evas_object_data_set(vd->obj, vg_key, NULL);
   elm_object_signal_callback_del(vd->obj, "elm,check,state,toggle", "tizen_vg", check_default_state_toggle_cb);
   elm_object_signal_callback_del(vd->obj, "elm,check,action,toggle", "tizen_vg", check_default_action_toggle_cb);
   elm_transit_del(vd->transit[0]);
   elm_transit_del(vd->transit[1]);
   elm_transit_del(vd->transit[2]);
   evas_object_del(vd->vg[1]);
   evas_object_del(vd->vg[2]);
   free(vd);
}

void
tizen_vg_check_default_set(Elm_Check *obj)
{
   check_default *vd = calloc(1, sizeof(check_default));
   if (!vd)
     {
        ERR("Failed to allocate vector graphics data memory");
        return;
     }
   evas_object_data_set(obj, vg_key, vd);

   elm_object_signal_callback_add(obj, "elm,check,state,toggle", "tizen_vg", check_default_state_toggle_cb, vd);
   elm_object_signal_callback_add(obj, "elm,check,action,toggle", "tizen_vg", check_default_action_toggle_cb, vd);

   vd->obj = obj;

   //Check bg
   vd->vg[2] = evas_object_vg_add(evas_object_evas_get(obj));
   evas_object_event_callback_add(vd->vg[2], EVAS_CALLBACK_RESIZE,
                                  check_default_vg_bg_resize_cb, vd);

   //Base VG
   vd->vg[0] = evas_object_vg_add(evas_object_evas_get(obj));
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_DEL,
                                  check_default_del_cb, vd);
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_RESIZE,
                                  check_default_vg_resize_cb, vd);
   //Check Line VG
   vd->vg[1] = evas_object_vg_add(evas_object_evas_get(obj));

   elm_object_part_content_set(obj, "tizen_vg_shape_bg", vd->vg[2]);
   elm_object_part_content_set(obj, "tizen_vg_shape", vd->vg[0]);
   elm_object_part_content_set(obj, "tizen_vg_shape2", vd->vg[1]);
}

void
tizen_vg_check_set(Elm_Check *obj)
{
   check_default *vd = evas_object_data_get(obj, vg_key);
   if (vd) evas_object_del(vd->vg[0]);

   //Apply vector ux only theme has "vector_ux"
   const char *str = elm_layout_data_get(obj, "vector_ux");
   if (!str) return;

   if (!strcmp(str, "default"))
     tizen_vg_check_default_set(obj);
   else if (!strcmp(str, "on&off"))
     tizen_vg_check_onoff_set(obj);
   else if (!strcmp(str, "favorite"))
     tizen_vg_check_favorite_set(obj);
}

void
tizen_vg_check_state_set(Elm_Check *obj)
{
   // Fix for the state change visual change skips one frame.
   // For vector enabled checkbox , state change visual change is handled in the
   // code and it depends on "elm,check,action,toggle" signal.
   // as edje signal emit is asyn force one more message_signal to make sure
   // state change related visual change occurs in the same frame.
   edje_object_message_signal_process(elm_layout_edje_get(obj));
}

/////////////////////////////////////////////////////////////////////////
/* Button */
/////////////////////////////////////////////////////////////////////////
typedef struct vg_button_s
{
   Evas_Object *vg[2];       //0: base, 1: effect
   Efl_VG_Shape *shape[2];   //0: base, 1: effect
   Evas_Object *obj;
   Evas_Coord corner;
   Eina_Bool init : 1;
   Eina_Bool is_circle : 1;
} vg_button;

static void
button_no_bg_init(vg_button *vd)
{
   if (vd->init) return;
   vd->init = EINA_TRUE;

   //Effect Shape
   Efl_VG *effect_root = evas_object_vg_root_node_get(vd->vg[0]);
   vd->shape[0] = evas_vg_shape_add(effect_root);
   evas_vg_node_color_set(vd->shape[0], 255, 255, 255, 255);
}

static void
button_effect_no_bg_resize_cb(void *data, Evas *e EINA_UNUSED,
                        Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   vg_button *vd = data;

   button_no_bg_init(vd);

   //Update Effect Shape
   Evas_Coord x, y, w, h;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   evas_vg_shape_shape_reset(vd->shape[0]);
   if (vd->is_circle)
     {
        int radius_w = w / 2;
        int radius_h = h / 2;
        evas_vg_shape_shape_append_circle(vd->shape[0], radius_w, radius_h, radius_w);
     }
   else
     {
        Evas_Coord w2, h2;
        double effect_ratio;

        if (!edje_object_part_geometry_get(elm_layout_edje_get(vd->obj), "effect_spacer", NULL, NULL, &w2, &h2))
          evas_object_geometry_get(vd->obj, NULL, NULL, &w2, &h2);
        effect_ratio = (w2 < h2) ? (double)w / w2 : (double)h / h2;
        evas_vg_shape_shape_append_rect(vd->shape[0], 0, 0, w, h, vd->corner * effect_ratio, vd->corner * effect_ratio);
     }
}

static void
button_no_bg_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   vg_button *vd = data;
   evas_object_data_del(vd->obj, vg_key);
   free(vd);
}

void
tizen_vg_button_no_bg_set(Elm_Button *obj)
{
   vg_button *vd = evas_object_data_get(obj, vg_key);
   if (vd)
     {
        elm_object_part_content_unset(obj, "tizen_vg_shape");
        evas_object_del(vd->vg[0]);
     }

   vd = calloc(1, sizeof(vg_button));
   if (!vd)
     {
        ERR("Failed to allocate vector graphics data memory");
        return;
     }
   evas_object_data_set(obj, vg_key, vd);

   //Vector Graphics Object
   Evas *e = evas_object_evas_get(obj);

   vd->obj = obj;

   //Effect VG
   vd->vg[0] = evas_object_vg_add(e);
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_DEL,
                                  button_no_bg_del_cb, vd);

   // Check whether the button has circle shape.
   // When the button has no background and has circle shape,
   // the vector_ux will be "no_bg/circle".
   const char *str = elm_layout_data_get(obj, "vector_ux");
   if (strstr(str, "circle"))
     vd->is_circle = EINA_TRUE;
   else
     {
        // Since it is not circle, it has rectangle shape.
        str = elm_layout_data_get(obj, "corner_radius");
        if (str) vd->corner = ELM_VG_SCALE_SIZE(obj, atoi(str));
     }

   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_RESIZE,
                                  button_effect_no_bg_resize_cb, vd);

   elm_object_part_content_set(obj, "tizen_vg_shape", vd->vg[0]);
}

static void
button_init(vg_button *vd)
{
   if (vd->init) return;
   vd->init = EINA_TRUE;

   //Base Shape
   Efl_VG *base_root = evas_object_vg_root_node_get(vd->vg[0]);
   vd->shape[0] = evas_vg_shape_add(base_root);
   evas_vg_node_color_set(vd->shape[0], 255, 255, 255, 255);

   //Effect Shape
   Efl_VG *effect_root = evas_object_vg_root_node_get(vd->vg[1]);
   vd->shape[1] = evas_vg_shape_add(effect_root);
   evas_vg_node_color_set(vd->shape[1], 255, 255, 255, 255);
}

static void
button_effect_resize_cb(void *data, Evas *e EINA_UNUSED,
                        Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   vg_button *vd = data;

   button_init(vd);

   //Update Effect Shape
   Evas_Coord x, y, w, h;
   evas_object_geometry_get(vd->vg[1], &x, &y, &w, &h);
   evas_vg_shape_shape_reset(vd->shape[1]);
   if (vd->is_circle)
     {
        int radius_w = w / 2;
        int radius_h = h / 2;
        evas_vg_shape_shape_append_circle(vd->shape[1], radius_w, radius_h, radius_w);
     }
   else
     {
        Evas_Coord w2, h2;
        double effect_ratio;

        if (!edje_object_part_geometry_get(elm_layout_edje_get(vd->obj), "effect_spacer", NULL, NULL, &w2, &h2))
          evas_object_geometry_get(vd->vg[0], NULL, NULL, &w2, &h2);
        effect_ratio = (w2 < h2) ? (double)w / w2 : (double)h / h2;
        evas_vg_shape_shape_append_rect(vd->shape[1], 0, 0, w, h, vd->corner * effect_ratio, vd->corner * effect_ratio);
     }
}

static void
button_base_resize_cb(void *data, Evas *e EINA_UNUSED,
                      Evas_Object *obj EINA_UNUSED,
                      void *event_info EINA_UNUSED)
{
   vg_button *vd = data;

   button_init(vd);

   //Update Base Shape
   Evas_Coord w, h;
   evas_object_geometry_get(vd->vg[0], NULL, NULL, &w, &h);
   evas_vg_shape_shape_reset(vd->shape[0]);
   if (vd->is_circle)
     {
        int radius_w = w / 2;
        int radius_h = h / 2;
        evas_vg_shape_shape_append_circle(vd->shape[0], radius_w, radius_h, radius_w);
     }
   else
     evas_vg_shape_shape_append_rect(vd->shape[0], 0, 0, w, h, vd->corner, vd->corner);
}

static void
button_del_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
              void *event_info EINA_UNUSED)
{
   vg_button *vd = data;
   evas_object_data_del(vd->obj, vg_key);
   evas_object_del(vd->vg[1]);
   free(vd);
}

void
tizen_vg_button_default_set(Elm_Button *obj)
{
   vg_button *vd = evas_object_data_get(obj, vg_key);
   if (vd)
     {
        elm_object_part_content_unset(obj, "tizen_vg_shape");
        elm_object_part_content_unset(obj, "tizen_vg_shape2");
        evas_object_del(vd->vg[0]);
     }
   vd = calloc(1, sizeof(vg_button));
   if (!vd)
     {
        ERR("Failed to allocate vector graphics data memory");
        return;
     }
   evas_object_data_set(obj, vg_key, vd);

   //Vector Graphics Object
   Evas *e = evas_object_evas_get(obj);

   vd->obj = obj;

   // Check whether the button has circle shape.
   // When the button has no background and has circle shape,
   // the vector_ux will be "no_bg/circle".
   const char *str = elm_layout_data_get(obj, "vector_ux");
   if (strstr(str, "circle"))
     vd->is_circle = EINA_TRUE;
   else
     {
        // Since it is not circle, it has rectangle shape.
        str = elm_layout_data_get(obj, "corner_radius");
        if (str) vd->corner = ELM_VG_SCALE_SIZE(obj, atoi(str));
     }

   //Base VG
   vd->vg[0] = evas_object_vg_add(e);
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_DEL,
                                  button_del_cb, vd);
   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_RESIZE,
                                  button_base_resize_cb, vd);
   //Effect VG
   vd->vg[1] = evas_object_vg_add(e);
   evas_object_event_callback_add(vd->vg[1], EVAS_CALLBACK_RESIZE,
                                  button_effect_resize_cb, vd);

   elm_object_part_content_set(obj, "tizen_vg_shape", vd->vg[0]);
   elm_object_part_content_set(obj, "tizen_vg_shape2", vd->vg[1]);
}

void
tizen_vg_button_set(Elm_Button *obj)
{
   //Apply vector ux only theme has "vector_ux"
   const char *str = elm_layout_data_get(obj, "vector_ux");
   if (!str) return;

   // Check whether the button has a background.
   if (!strncmp(str, "no_bg", strlen("no_bg")))
     tizen_vg_button_no_bg_set(obj);
   else
     tizen_vg_button_default_set(obj);
}

/////////////////////////////////////////////////////////////////////////
/* Progressbar */
/////////////////////////////////////////////////////////////////////////

typedef struct vg_progressbar_s
{
   Evas_Object *vg[3];       //0: base, 1: layer1, 2:layer2
   Efl_VG_Shape *shape[3];   //0: base, 1: layer1, 2: layer2
   Elm_Transit  *transit[8];
   Evas_Object *obj;
   Evas_Coord x, w, h;       // for normal style animation data
   double stroke_width;
   double shrink;
   double shift;
   Ecore_Job *pulse_job;
   Eina_Bool continued : 1;
   Eina_Bool pulse : 1;
} vg_progressbar;

static void
transit0_progress_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   vg_progressbar *vd = data;
   vd->transit[0] = NULL;
}

static void
transit1_progress_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   vg_progressbar *vd = data;
   vd->transit[1] = NULL;
}

static void
transit2_progress_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   vg_progressbar *vd = data;
   vd->transit[2] = NULL;
}

static void
transit3_progress_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   vg_progressbar *vd = data;
   vd->transit[3] = NULL;
}

static void
transit4_progress_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   vg_progressbar *vd = data;
   vd->transit[4] = NULL;
}

static void
transit5_progress_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   vg_progressbar *vd = data;
   vd->transit[5] = NULL;
}

static void
transit6_progress_del_cb(void *data, Elm_Transit *transit EINA_UNUSED)
{
   vg_progressbar *vd = data;
   vd->transit[6] = NULL;
}

static void
_pulse_stop(vg_progressbar *vd)
{
   int i;

   vd->pulse = EINA_FALSE;

   for (i = 0; i < 8 ; i++)
     {
        elm_transit_del(vd->transit[i]);
        vd->transit[i] = NULL;
     }

   // delete if any pul_job is scheduled.
   if (vd->pulse_job)
     ecore_job_del(vd->pulse_job);
   vd->pulse_job = NULL;
}

static void
progressbar_hide_cb(void *data, Evas *e EINA_UNUSED, Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   _pulse_stop(data);
}

static void
transit_progressbar_normal_op1(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;

   evas_vg_node_color_set(vd->shape[1], 255, 255, 255, 255);
   evas_vg_shape_shape_reset(vd->shape[1]);
   Evas_Coord start_width = vd->x + vd->h;
   evas_vg_shape_shape_append_rect(vd->shape[1], 0, 0, start_width + (vd->w - start_width)* progress, vd->h, vd->h/2, vd->h/2);

   evas_vg_node_color_set(vd->shape[2], 255, 255, 255, 255);
   evas_vg_shape_shape_reset(vd->shape[2]);
   Evas_Coord delta_width = (vd->w - vd->x - vd->h) * progress;
   evas_vg_shape_shape_append_rect(vd->shape[2], vd->x, 0, vd->h + delta_width, vd->h, vd->h/2, vd->h/2);

}

static void
transit_progressbar_normal_op2(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;
   evas_vg_node_color_set(vd->shape[2], 255, 255, 255, 255);
   evas_vg_shape_shape_reset(vd->shape[2]);
   Evas_Coord delta_move =  (vd->w - vd->x - vd->h) * progress;
   evas_vg_shape_shape_append_rect(vd->shape[2], vd->x + delta_move, 0, vd->w - vd->x - delta_move, vd->h, vd->h/2, vd->h/2);
}

static void
transit_progressbar_normal_op3(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;
   int c = 255 - 255*progress;
   evas_vg_node_color_set(vd->shape[2], c, c, c, c);
}

static void
_transit_progressbar_normal_animation_finished(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED)
{
   vg_progressbar *vd = effect;
   vd->transit[2] = NULL;
   if (vd->continued)
     vd->continued = EINA_FALSE;
   else
     elm_layout_signal_emit(vd->obj, "elm,action,animation,finished", "elm");
}

static void
progressbar_normal_bg_resize_cb(void *data , Evas *e EINA_UNUSED,
                                Evas_Object *obj EINA_UNUSED,
                                void *event_info EINA_UNUSED)
{
   Evas_Coord x, y, w, h;
   vg_progressbar *vd = data;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   evas_vg_node_color_set(vd->shape[0], 255, 255, 255, 255);
   evas_vg_shape_shape_reset(vd->shape[0]);
   evas_vg_shape_shape_append_rect(vd->shape[0], 0, 0, w, h, h/2, h/2);
}

static void
progressbar_normal_bg_show_cb(void *data , Evas *e EINA_UNUSED,
                                Evas_Object *obj EINA_UNUSED,
                                void *event_info EINA_UNUSED)
{
   Evas_Coord x, y, w, h;
   vg_progressbar *vd = data;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   evas_vg_node_color_set(vd->shape[0], 255, 255, 255, 255);
   evas_vg_shape_shape_reset(vd->shape[0]);
   evas_vg_shape_shape_append_rect(vd->shape[0], 0, 0, w, h, h/2, h/2);
}

static void
progressbar_normal_fg_resize_cb(void *data, Evas *e EINA_UNUSED,
                                Evas_Object *obj EINA_UNUSED,
                                void *event_info EINA_UNUSED)
{
   Evas_Coord x, y, w, h;
   static double ease_out_factor[4] = {0.25, 0.46, 0.45, 1.0};
   vg_progressbar *vd = data;
   evas_object_geometry_get(vd->vg[1], &x, &y, &w, &h);
   if (w < 2) return;

   if (vd->w >= w) vd->x = 0;
   else vd->x = vd->w - vd->h;
   vd->w = w;
   vd->h = h;
   elm_transit_del(vd->transit[0]);
   vd->transit[0] = elm_transit_add();
   elm_transit_object_add(vd->transit[0], obj);
   elm_transit_effect_add(vd->transit[0], transit_progressbar_normal_op1, vd, transit0_progress_del_cb);
   elm_transit_tween_mode_set(vd->transit[0], ELM_TRANSIT_TWEEN_MODE_BEZIER_CURVE);
   elm_transit_tween_mode_factor_n_set(vd->transit[0], 4, ease_out_factor);
   elm_transit_duration_set(vd->transit[0], 0.8);
   elm_transit_objects_final_state_keep_set(vd->transit[0], EINA_TRUE);

   elm_transit_del(vd->transit[1]);
   vd->transit[1] = elm_transit_add();
   elm_transit_object_add(vd->transit[1], obj);
   elm_transit_effect_add(vd->transit[1], transit_progressbar_normal_op2, vd, transit1_progress_del_cb);
   elm_transit_tween_mode_set(vd->transit[1], ELM_TRANSIT_TWEEN_MODE_BEZIER_CURVE);
   elm_transit_tween_mode_factor_n_set(vd->transit[1], 4, ease_out_factor);
   elm_transit_duration_set(vd->transit[1], 0.7);
   elm_transit_objects_final_state_keep_set(vd->transit[1], EINA_TRUE);

   if (vd->transit[2]) vd->continued = EINA_TRUE;

   elm_transit_del(vd->transit[2]);
   vd->transit[2] = elm_transit_add();
   elm_transit_object_add(vd->transit[2], obj);
   elm_transit_effect_add(vd->transit[2], transit_progressbar_normal_op3, vd, _transit_progressbar_normal_animation_finished);
   elm_transit_tween_mode_set(vd->transit[2], ELM_TRANSIT_TWEEN_MODE_BEZIER_CURVE);
   elm_transit_tween_mode_factor_n_set(vd->transit[2], 4, ease_out_factor);
   elm_transit_duration_set(vd->transit[2], 0.3);
   elm_transit_objects_final_state_keep_set(vd->transit[2], EINA_TRUE);

   elm_transit_chain_transit_add(vd->transit[0], vd->transit[1]);
   elm_transit_chain_transit_add(vd->transit[1], vd->transit[2]);

   elm_transit_go(vd->transit[0]);

}

static void
_progressbar_normal_style(vg_progressbar *vd)
{
   Efl_VG *root;
   int i;
   Evas *e = evas_object_evas_get(vd->obj);

   for(i=0; i < 3; i++)
     {
        vd->vg[i] = evas_object_vg_add(e);
        root = evas_object_vg_root_node_get(vd->vg[i]);
        vd->shape[i] = evas_vg_shape_add(root);
     }

   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_RESIZE,
                                  progressbar_normal_bg_resize_cb, vd);
   evas_object_event_callback_add(vd->obj, EVAS_CALLBACK_SHOW,
                                  progressbar_normal_bg_show_cb, vd);
   evas_object_event_callback_add(vd->vg[1], EVAS_CALLBACK_RESIZE,
                                  progressbar_normal_fg_resize_cb, vd);

   //unset
   elm_object_part_content_unset(vd->obj, "tizen_vg_shape1");
   elm_object_part_content_unset(vd->obj, "tizen_vg_shape2");
   elm_object_part_content_unset(vd->obj, "tizen_vg_shape3");

   elm_object_part_content_set(vd->obj, "tizen_vg_shape1", vd->vg[0]);
   elm_object_part_content_set(vd->obj, "tizen_vg_shape2", vd->vg[1]);
   elm_object_part_content_set(vd->obj, "tizen_vg_shape3", vd->vg[2]);
}

static void
progressbar_process_resize_cb(void *data , Evas *e EINA_UNUSED,
                                Evas_Object *obj EINA_UNUSED,
                                void *event_info EINA_UNUSED)
{
   Evas_Coord x, y, w, h,i;
   vg_progressbar *vd = data;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   for(i=0; i < 3; i++)
     {
        evas_vg_shape_shape_reset(vd->shape[i]);
        evas_vg_shape_stroke_color_set(vd->shape[i], 255, 255, 255, 255);
        evas_vg_shape_shape_append_arc(vd->shape[i], 0, 0, w - vd->shrink, h - vd->shrink, 90.5, -1);
        evas_vg_shape_stroke_width_set(vd->shape[i], vd->stroke_width);
        evas_vg_node_origin_set(vd->shape[i], vd->shift, vd->shift);
     }
}

static void
transit_progressbar_process_A_op1(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;
   Evas_Coord x, y, w, h;
   double s_a = 90.5, e_a = 90.5, s_l = -1 , e_l = -126;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   double start_angle = s_a + (e_a - s_a) * progress;
   double sweep_length = s_l + (e_l - s_l) * progress;
   evas_vg_shape_shape_reset(vd->shape[2]);
   evas_vg_shape_shape_append_arc(vd->shape[2], 0, 0, w - vd->shrink, h - vd->shrink, start_angle, sweep_length);
}

static void
transit_progressbar_process_A_op2(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;
   Evas_Coord x, y, w, h;
   double s_a = 90.5, e_a = -170, s_l = -126 , e_l = -100;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   double start_angle = s_a + (e_a - s_a) * progress;
   double sweep_length = s_l + (e_l - s_l) * progress;
   evas_vg_shape_shape_reset(vd->shape[2]);
   evas_vg_shape_shape_append_arc(vd->shape[2], 0, 0, w - vd->shrink, h - vd->shrink, start_angle, sweep_length);
}

static void
transit_progressbar_process_A_op3(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;
   Evas_Coord x, y, w, h;
   double s_a = -170, e_a = -269.5, s_l = -100 , e_l = -1;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   double start_angle = s_a + (e_a - s_a) * progress;
   double sweep_length = s_l + (e_l - s_l) * progress;
   evas_vg_shape_shape_reset(vd->shape[2]);
   evas_vg_shape_shape_append_arc(vd->shape[2], 0, 0, w - vd->shrink, h - vd->shrink, start_angle, sweep_length);
}

static void
transit_progressbar_process_B_op1(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;
   Evas_Coord x, y, w, h;
   double s_a = 90.5, e_a = 90.5, s_l = -1 , e_l = -172.8;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   double start_angle = s_a + (e_a - s_a) * progress;
   double sweep_length = s_l + (e_l - s_l) * progress;
   evas_vg_shape_shape_reset(vd->shape[1]);
   evas_vg_shape_shape_append_arc(vd->shape[1], 0, 0, w - vd->shrink, h - vd->shrink, start_angle, sweep_length);
}

static void
transit_progressbar_process_B_op2(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;
   Evas_Coord x, y, w, h;
   double s_a = 90.5, e_a = -129.7, s_l = -172.8 , e_l = -140.3;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   double start_angle = s_a + (e_a - s_a) * progress;
   double sweep_length = s_l + (e_l - s_l) * progress;
   evas_vg_shape_shape_reset(vd->shape[1]);
   evas_vg_shape_shape_append_arc(vd->shape[1], 0, 0, w - vd->shrink, h - vd->shrink, start_angle, sweep_length);
}

static void
transit_progressbar_process_B_op3(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;
   Evas_Coord x, y, w, h;
   double s_a = -129.7, e_a = -269.5, s_l = -140.3 , e_l = -1;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   double start_angle = s_a + (e_a - s_a) * progress;
   double sweep_length = s_l + (e_l - s_l) * progress;
   evas_vg_shape_shape_reset(vd->shape[1]);
   evas_vg_shape_shape_append_arc(vd->shape[1], 0, 0, w - vd->shrink, h - vd->shrink, start_angle, sweep_length);
}

static void
transit_progressbar_process_C_op1(Elm_Transit_Effect *effect EINA_UNUSED, Elm_Transit *transit EINA_UNUSED, double progress EINA_UNUSED)
{
}

static void
transit_progressbar_process_C_op2(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED, double progress)
{
   vg_progressbar *vd = effect;
   Evas_Coord x, y, w, h;
   double s_a = 89.5, e_a = -270.5, s_l = -1 , e_l = -1;
   evas_object_geometry_get(vd->vg[0], &x, &y, &w, &h);
   double start_angle = s_a + (e_a - s_a) * progress;
   double sweep_length = s_l + (e_l - s_l) * progress;
   evas_vg_shape_shape_reset(vd->shape[0]);
   evas_vg_shape_shape_append_arc(vd->shape[0], 0, 0, w - vd->shrink, h - vd->shrink, start_angle, sweep_length);
}

static void _transit_progressbar_process_end(Elm_Transit_Effect *effect, Elm_Transit *transit);

static void
_progressbar_process_pulse_start_helper(void *data)
{
   vg_progressbar *vd = data;

   vd->pulse_job = NULL;

   // For Layer A animation
   elm_transit_del(vd->transit[0]);
   vd->transit[0] = elm_transit_add();
   elm_transit_object_add(vd->transit[0], vd->obj);
   elm_transit_effect_add(vd->transit[0], transit_progressbar_process_A_op1, vd, transit0_progress_del_cb);
   elm_transit_duration_set(vd->transit[0], 0.35);
   elm_transit_objects_final_state_keep_set(vd->transit[0], EINA_TRUE);

   elm_transit_del(vd->transit[1]);
   vd->transit[1] = elm_transit_add();
   elm_transit_object_add(vd->transit[1], vd->obj);
   elm_transit_effect_add(vd->transit[1], transit_progressbar_process_A_op2, vd, transit1_progress_del_cb);
   elm_transit_duration_set(vd->transit[1], 0.65);
   elm_transit_objects_final_state_keep_set(vd->transit[1], EINA_TRUE);

   elm_transit_del(vd->transit[2]);
   vd->transit[2] = elm_transit_add();
   elm_transit_object_add(vd->transit[2], vd->obj);
   elm_transit_effect_add(vd->transit[2], transit_progressbar_process_A_op3, vd, transit2_progress_del_cb);
   elm_transit_duration_set(vd->transit[2], 0.25);
   elm_transit_objects_final_state_keep_set(vd->transit[2], EINA_TRUE);

   elm_transit_chain_transit_add(vd->transit[0], vd->transit[1]);
   elm_transit_chain_transit_add(vd->transit[1], vd->transit[2]);

   elm_transit_go(vd->transit[0]);

   // For Layer B Animation
   elm_transit_del(vd->transit[3]);
   vd->transit[3] = elm_transit_add();
   elm_transit_object_add(vd->transit[3], vd->obj);
   elm_transit_effect_add(vd->transit[3], transit_progressbar_process_B_op1, vd, transit3_progress_del_cb);
   elm_transit_duration_set(vd->transit[3], 0.48);
   elm_transit_objects_final_state_keep_set(vd->transit[3], EINA_TRUE);

   elm_transit_del(vd->transit[4]);
   vd->transit[4] = elm_transit_add();
   elm_transit_object_add(vd->transit[4], vd->obj);
   elm_transit_effect_add(vd->transit[4], transit_progressbar_process_B_op2, vd, transit4_progress_del_cb);
   elm_transit_duration_set(vd->transit[4], 0.52);
   elm_transit_objects_final_state_keep_set(vd->transit[4], EINA_TRUE);

   elm_transit_del(vd->transit[5]);
   vd->transit[5] = elm_transit_add();
   elm_transit_object_add(vd->transit[5], vd->obj);
   elm_transit_effect_add(vd->transit[5], transit_progressbar_process_B_op3, vd, transit5_progress_del_cb);
   elm_transit_duration_set(vd->transit[5], 0.33);
   elm_transit_objects_final_state_keep_set(vd->transit[5], EINA_TRUE);

   elm_transit_chain_transit_add(vd->transit[3], vd->transit[4]);
   elm_transit_chain_transit_add(vd->transit[4], vd->transit[5]);

   elm_transit_go(vd->transit[3]);

   // This transition is just a dummy one
   // because transit_go_in dosen't guarentee the exact time transition will start.
   // this fixed the tail dot animation delay issue in 1st frame.
   elm_transit_del(vd->transit[6]);
   vd->transit[6] = elm_transit_add();
   elm_transit_object_add(vd->transit[6], vd->obj);
   elm_transit_effect_add(vd->transit[6], transit_progressbar_process_C_op1, vd, transit6_progress_del_cb);
   elm_transit_duration_set(vd->transit[6], .54);
   elm_transit_objects_final_state_keep_set(vd->transit[6], EINA_TRUE);
   elm_transit_go_in(vd->transit[6], .54);

   // For Layer C Animation
   elm_transit_del(vd->transit[7]);
   vd->transit[7] = elm_transit_add();
   elm_transit_object_add(vd->transit[7], vd->obj);
   elm_transit_effect_add(vd->transit[7], transit_progressbar_process_C_op2, vd, _transit_progressbar_process_end);
   elm_transit_duration_set(vd->transit[7], 0.85);
   elm_transit_objects_final_state_keep_set(vd->transit[7], EINA_TRUE);

   elm_transit_chain_transit_add(vd->transit[6], vd->transit[7]);

   elm_transit_go(vd->transit[6]);
}

static void
_transit_progressbar_process_end(Elm_Transit_Effect *effect, Elm_Transit *transit EINA_UNUSED)
{
   vg_progressbar *vd = effect;
   vd->transit[7] = NULL;

   // restart the pulse
   if (vd->pulse)
     vd->pulse_job = ecore_job_add(_progressbar_process_pulse_start_helper, vd);
}

static void
_progressbar_process_pulse_start(void *data,
                                 Evas_Object *obj EINA_UNUSED,
                                 const char *emission EINA_UNUSED,
                                 const char *source EINA_UNUSED)
{
   vg_progressbar *vd = data;
   if (vd->pulse) return;

   vd->pulse = EINA_TRUE;
   _progressbar_process_pulse_start_helper(data);
}

static void
_progressbar_process_pulse_stop(void *data,
                                Evas_Object *obj EINA_UNUSED,
                                const char *emission EINA_UNUSED,
                                const char *source EINA_UNUSED)
{
   _pulse_stop(data);
}

static void
progressbar_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
                   void *event_info EINA_UNUSED)
{
   vg_progressbar *vd = evas_object_data_get(obj, vg_key);

   _pulse_stop(vd);
   evas_object_data_set(obj, vg_key, NULL);
   elm_object_signal_callback_del(vd->obj, "elm,state,pulse,start",
                                  "*", _progressbar_process_pulse_start);
   elm_object_signal_callback_del(vd->obj, "elm,state,pulse,stop",
                                  "*", _progressbar_process_pulse_stop);
   evas_object_del(vd->vg[0]);
   evas_object_del(vd->vg[1]);
   evas_object_del(vd->vg[2]);
   free(vd);
}

static void
_progressbar_process_style(vg_progressbar *vd)
{
   Efl_VG *root;
   int i;
   Evas *e = evas_object_evas_get(vd->obj);

   for(i=0; i < 3; i++)
     {
        vd->vg[i] = evas_object_vg_add(e);
        root = evas_object_vg_root_node_get(vd->vg[i]);
        vd->shape[i] = evas_vg_shape_add(root);
        evas_vg_shape_stroke_color_set(vd->shape[i], 255, 255, 255, 255);
        evas_vg_shape_stroke_cap_set(vd->shape[i], EFL_GFX_CAP_ROUND);
     }

   evas_object_event_callback_add(vd->vg[0], EVAS_CALLBACK_RESIZE,
                                  progressbar_process_resize_cb, vd);

   elm_object_part_content_set(vd->obj, "tizen_vg_shape1", vd->vg[0]);
   elm_object_part_content_set(vd->obj, "tizen_vg_shape2", vd->vg[1]);
   elm_object_part_content_set(vd->obj, "tizen_vg_shape3", vd->vg[2]);

   elm_object_signal_callback_add(vd->obj, "elm,state,pulse,start",
                                  "*", _progressbar_process_pulse_start, vd);

   elm_object_signal_callback_add(vd->obj, "elm,state,pulse,stop",
                                   "*", _progressbar_process_pulse_stop, vd);

}

void
tizen_vg_progressbar_set(Elm_Progressbar *obj)
{
   vg_progressbar *vd = evas_object_data_get(obj, vg_key);
   //Apply vector ux only theme has "vector_ux"
   const char *str = elm_layout_data_get(obj, "vector_ux");
   if (vd)
     {
        if (vd->vg[0]) evas_object_del(vd->vg[0]);
        if (vd->vg[1]) evas_object_del(vd->vg[1]);
        if (vd->vg[2]) evas_object_del(vd->vg[2]);
        vd->x = 0;
        if (progressbar_normal_bg_show_cb)
          evas_object_event_callback_del(vd->obj, EVAS_CALLBACK_SHOW, progressbar_normal_bg_show_cb);
        if (!str)
          {
             _pulse_stop(vd);
             evas_object_data_set(vd->obj, vg_key, NULL);
             evas_object_event_callback_del(vd->obj, EVAS_CALLBACK_DEL,
                                            progressbar_del_cb);
             evas_object_event_callback_del(vd->obj, EVAS_CALLBACK_HIDE,
                                            progressbar_hide_cb);
             elm_object_signal_callback_del(vd->obj, "elm,state,pulse,start",
                                            "*", _progressbar_process_pulse_start);

             elm_object_signal_callback_del(vd->obj, "elm,state,pulse,stop",
                                            "*", _progressbar_process_pulse_stop);
             free(vd);

             return;
          }
     }

   if (!vd)
     {
        vd = calloc(1, sizeof(vg_progressbar));
        if (!vd)
          {
             ERR("Failed to allocate vector graphics data memory");
             return;
          }
        evas_object_data_set(obj, vg_key, vd);
        vd->obj = obj;
        // callback to free vd data
        evas_object_event_callback_add(vd->obj, EVAS_CALLBACK_DEL,
                                       progressbar_del_cb, NULL);
        evas_object_event_callback_add(vd->obj, EVAS_CALLBACK_HIDE,
                                       progressbar_hide_cb, vd);
     }

   if (str && !strcmp(str, "default"))
     {
        _progressbar_normal_style(vd);
        return;
     }
   if (str && (!strcmp(str, "process_large") ||
       !strcmp(str, "process_medium") ||
       !strcmp(str, "process_small")))
     {
        vd->stroke_width = ELM_VG_SCALE_SIZE(vd->obj, 5);
        if (!strcmp(str, "process_medium"))
          {
             vd->stroke_width = ELM_VG_SCALE_SIZE(vd->obj, 3);
          }
        if (!strcmp(str, "process_small"))
          {
             vd->stroke_width = ELM_VG_SCALE_SIZE(vd->obj, 2);
          }
        vd->shrink = 2 * vd->stroke_width;
        vd->shift = vd->stroke_width + 0.5;
        _progressbar_process_style(vd);
        return;
     }
}

/////////////////////////////////////////////////////////////////////////
/* Slider */
/////////////////////////////////////////////////////////////////////////

typedef struct vg_slider_s
{
   Evas_Object *vg[7];
   Efl_VG_Shape *shape[7];
   Evas_Object *obj;
   Evas_Object *popup;
   Eina_Stringshare *style;
} vg_slider;

static int slider_base = 0;
static int slider_level = 1;
static int slider_level2 = 2;
static int slider_level_rest = 3;
static int slider_center = 4;
static int slider_handle = 5;
static int slider_handle_pressed = 6;

static void
slider_del_cb(void *data EINA_UNUSED, Evas *e EINA_UNUSED, Evas_Object *obj,
              void *event_info EINA_UNUSED)
{
   vg_slider *vd = evas_object_data_get(obj, vg_key);
   if (vd)
     {
        evas_object_data_set(obj, vg_key, NULL);
        free(vd);
     }
}

static void
_append_circle(Efl_VG *shape, double cx, double cy, double r)
{
   evas_vg_shape_shape_reset(shape);
   evas_vg_shape_shape_append_circle(shape, cx, cy, r);
}

static void
_append_round_rect(Efl_VG *shape, int w, int h)
{
   double radius = w/2 > h/2 ? h/2 : w/2;
   evas_vg_shape_shape_reset(shape);
   evas_vg_shape_shape_append_rect(shape, 0, 0, w, h, radius, radius);
}

static void
slider_vg_handle_normal_resize_cb(void *data , Evas *e EINA_UNUSED,
                                  Evas_Object *obj EINA_UNUSED,
                                  void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   vg_slider *vd = data;
   evas_object_geometry_get(vd->vg[slider_handle], NULL, NULL, &w, &h);
   _append_circle(vd->shape[slider_handle], w/2, h/2, w/2);
}

static void
slider_vg_handle_pressed_resize_cb(void *data , Evas *e EINA_UNUSED,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   vg_slider *vd = data;
   evas_object_geometry_get(vd->vg[slider_handle_pressed], NULL, NULL, &w, &h);
   if (w == h)
     _append_circle(vd->shape[slider_handle_pressed], w/2, h/2, w/2);
   else
     {
        if (elm_slider_indicator_show_get(vd->obj) &&
            elm_slider_indicator_format_get(vd->obj))
          _append_round_rect(vd->shape[slider_handle_pressed], w, h);
        else
          _append_circle(vd->shape[slider_handle_pressed], w/2, h - w/2, w/2.5);
     }
}

static void
slider_base_resize_cb(void *data , Evas *e EINA_UNUSED,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   vg_slider *vd = data;
   evas_object_geometry_get(vd->vg[slider_base], NULL, NULL, &w, &h);
   _append_round_rect(vd->shape[slider_base], w, h);
}

static void
slider_center_resize_cb(void *data , Evas *e EINA_UNUSED,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   vg_slider *vd = data;
   evas_object_geometry_get(vd->vg[slider_center], NULL, NULL, &w, &h);
   _append_round_rect(vd->shape[slider_center], w, h);
}

static void
slider_level_resize_cb(void *data , Evas *e EINA_UNUSED,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   vg_slider *vd = data;
   evas_object_geometry_get(vd->vg[slider_level], NULL, NULL, &w, &h);
   _append_round_rect(vd->shape[slider_level], w, h);
}

static void
slider_level2_resize_cb(void *data , Evas *e EINA_UNUSED,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   vg_slider *vd = data;
   evas_object_geometry_get(vd->vg[slider_level2], NULL, NULL, &w, &h);
   _append_round_rect(vd->shape[slider_level2], w, h);
}

static void
slider_level_rest_resize_cb(void *data , Evas *e EINA_UNUSED,
                            Evas_Object *obj EINA_UNUSED,
                            void *event_info EINA_UNUSED)
{
   Evas_Coord w, h;
   vg_slider *vd = data;
   evas_object_geometry_get(vd->vg[slider_level_rest], NULL, NULL, &w, &h);
   _append_round_rect(vd->shape[slider_level_rest], w, h);
}

//TIZEN_ONLY(20150915): slider: fix slider's handler bug
static void
slider_unfocused_cb(void *data,
                   Evas_Object *obj EINA_UNUSED,
                   void *event_info EINA_UNUSED)
{
   vg_slider *vd = evas_object_data_get(obj, vg_key);
   if (vd->popup)
     evas_object_hide(vd->popup);
}
//

static void
_slider_create_handle(vg_slider *vd)
{
   Efl_VG *root;
   int i;
   Evas *e = evas_object_evas_get(vd->obj);
   for(i=0; i < 7; i++)
     {
        vd->vg[i] = evas_object_vg_add(e);
        root = evas_object_vg_root_node_get(vd->vg[i]);
        vd->shape[i] = evas_vg_shape_add(root);
        evas_vg_node_color_set(vd->shape[i], 255, 255, 255, 255);

     }
   // slider base
   evas_object_event_callback_add(vd->vg[slider_base], EVAS_CALLBACK_RESIZE,
                                  slider_base_resize_cb, vd);
   elm_object_part_content_set(vd->obj, "elm.swallow.tizen_vg_shape1", vd->vg[slider_base]);

   // level
   evas_object_event_callback_add(vd->vg[slider_level], EVAS_CALLBACK_RESIZE,
                                  slider_level_resize_cb, vd);
   elm_object_part_content_set(vd->obj, "elm.swallow.tizen_vg_shape2", vd->vg[slider_level]);

   // level2
   evas_object_event_callback_add(vd->vg[slider_level2], EVAS_CALLBACK_RESIZE,
                                  slider_level2_resize_cb, vd);
   elm_object_part_content_set(vd->obj, "elm.swallow.tizen_vg_shape3", vd->vg[slider_level2]);

   // level rest only for warning type
   evas_object_event_callback_add(vd->vg[slider_level_rest], EVAS_CALLBACK_RESIZE,
                                  slider_level_rest_resize_cb, vd);
   elm_object_part_content_set(vd->obj, "elm.swallow.tizen_vg_shape4", vd->vg[slider_level_rest]);

   // center point
   evas_object_event_callback_add(vd->vg[slider_center], EVAS_CALLBACK_RESIZE,
                                  slider_center_resize_cb, vd);
   elm_object_part_content_set(vd->obj, "elm.swallow.tizen_vg_shape5", vd->vg[slider_center]);

   // slider handle
   evas_object_event_callback_add(vd->vg[slider_handle], EVAS_CALLBACK_RESIZE,
                                  slider_vg_handle_normal_resize_cb, vd);
   evas_object_event_callback_add(vd->vg[slider_handle_pressed], EVAS_CALLBACK_RESIZE,
                                  slider_vg_handle_pressed_resize_cb, vd);
   elm_object_part_content_set(vd->obj, "elm.dragable.slider:elm.swallow.tizen_vg_shape1", vd->vg[slider_handle]);
   if (vd->popup)
     {
        edje_object_part_swallow(vd->popup, "elm.swallow.tizen_vg_shape2", vd->vg[slider_handle_pressed]);
        elm_widget_sub_object_add(vd->obj, vd->vg[slider_handle_pressed]);
     }
}

void
tizen_vg_slider_set(Elm_Slider *obj, Evas_Object *popup)
{
   vg_slider *vd = evas_object_data_get(obj, vg_key);
   if (vd)
     {
        int i;
        for(i=0; i < 7; i++)
          if (vd->vg[i]) evas_object_del(vd->vg[i]);
        eina_stringshare_del(vd->style);
     }

   //Apply vector ux only theme has "vector_ux" "on"
   const char *str = elm_layout_data_get(obj, "vector_ux");
   if (!str) return;

   if (!vd)
     {
        vd = calloc(1, sizeof(vg_slider));
        evas_object_data_set(obj, vg_key, vd);
        vd->obj = obj;
        vd->popup = popup;
        // callback to free vd data
        evas_object_event_callback_add(vd->obj, EVAS_CALLBACK_DEL,
                                       slider_del_cb, NULL);

        //TIZEN_ONLY(20150915): slider: fix slider's handler bug
        evas_object_smart_callback_add(vd->obj, SIG_LAYOUT_UNFOCUSED, slider_unfocused_cb, NULL);
        //
     }
   if (!vd)
     {
        ERR("Failed to allocate vector graphics data memory");
        return;
     }

   vd->style = eina_stringshare_add(str);

   _slider_create_handle(vd);
}
#endif
