#ifndef ELM_WIDGET_CTXPOPUP_H
#define ELM_WIDGET_CTXPOPUP_H

#include "elm_widget_layout.h"

#define MAX_ITEMS_PER_VIEWPORT 8

/* DO NOT USE THIS HEADER UNLESS YOU ARE PREPARED FOR BREAKING OF YOUR
 * CODE. THIS IS ELEMENTARY'S INTERNAL WIDGET API (for now) AND IS NOT
 * FINAL. CALL elm_widget_api_check(ELM_INTERNAL_API_VERSION) TO CHECK
 * IT AT RUNTIME.
 */

/**
 * @internal
 * @addtogroup Widget
 * @{
 *
 * @section elm-ctxpopup-class The Elementary Ctxpopup Class
 *
 * Elementary, besides having the @ref Ctxpopup widget, exposes its
 * foundation -- the Elementary Ctxpopup Class -- in order to create other
 * widgets which are a ctxpopup with some more logic on top.
 */

typedef struct _Elm_Ctxpopup_Item_Data       Elm_Ctxpopup_Item_Data;

/**
 * Base widget smart data extended with ctxpopup instance data.
 */
typedef struct _Elm_Ctxpopup_Data Elm_Ctxpopup_Data;
struct _Elm_Ctxpopup_Item_Data
{
   Elm_Widget_Item_Data *base;

   const char   *label;
   Evas_Object  *icon;
   Evas_Object  *btn;

   struct
     {
        Evas_Smart_Cb org_func_cb;
        const void    *org_data;
        Evas_Object   *cobj;
     } wcb;
};

struct _Elm_Ctxpopup_Data
{
   Evas_Object           *parent;
   Evas_Object           *box;

   Evas_Object           *layout;
   Evas_Object           *arrow;
   Evas_Object           *scr;
   Evas_Object           *bg;
   Evas_Object           *content;
   Eina_List             *items;

   Elm_Ctxpopup_Direction dir;
   Elm_Ctxpopup_Direction dir_priority[4];

   int                    multi_down;

   Eina_Bool              horizontal : 1;
   Eina_Bool              visible : 1;
   Eina_Bool              auto_hide : 1;
   Eina_Bool              mouse_down : 1;
//******************** TIZEN Only
   Eina_Bool              pressed : 1;
   Eina_Bool              on_dismiss: 1;
   //TIZEN_ONLY(20150923): Using ecore idle enterer for better performance
   Ecore_Idle_Enterer    *idle_enterer;
//****************************
#ifndef ELM_FEATURE_WEARABLE
  /*for banded ux*/
   int                    item_color[MAX_ITEMS_PER_VIEWPORT][4];
   Elm_Ctxpopup_Item_Data *top_drawn_item;
   Eina_Bool              color_calculated : 1;
#endif
};

/**
 * @}
 */

#define ELM_CTXPOPUP_DATA_GET(o, sd) \
  Elm_Ctxpopup_Data * sd = eo_data_scope_get(o, ELM_CTXPOPUP_CLASS)

#define ELM_CTXPOPUP_DATA_GET_OR_RETURN(o, ptr)      \
  ELM_CTXPOPUP_DATA_GET(o, ptr);                     \
  if (EINA_UNLIKELY(!ptr))                           \
    {                                                \
       CRI("No widget data for object %p (%s)",      \
           o, evas_object_type_get(o));              \
       return;                                       \
    }

#define ELM_CTXPOPUP_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  ELM_CTXPOPUP_DATA_GET(o, ptr);                         \
  if (EINA_UNLIKELY(!ptr))                               \
    {                                                    \
       CRI("No widget data for object %p (%s)",          \
           o, evas_object_type_get(o));                  \
       return val;                                       \
    }

#define ELM_CTXPOPUP_CHECK(obj)                              \
  if (EINA_UNLIKELY(!eo_isa((obj), ELM_CTXPOPUP_CLASS))) \
    return

#define ELM_CTXPOPUP_ITEM_CHECK_OR_RETURN(it, ...)                 \
  ELM_WIDGET_ITEM_CHECK_OR_RETURN(it->base, __VA_ARGS__); \
  ELM_CTXPOPUP_CHECK(it->base->widget) __VA_ARGS__;

#define ELM_CTXPOPUP_ITEM_DATA_GET(o, sd) \
  Elm_Ctxpopup_Item_Data *sd = eo_data_scope_get(o, ELM_CTXPOPUP_ITEM_CLASS)

#endif
