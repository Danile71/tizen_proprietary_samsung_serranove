#ifndef ELM_WIDGET_BOX_H
#define ELM_WIDGET_BOX_H

#include "Elementary.h"

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
 * @section elm-box-class The Elementary Box Class
 *
 * Elementary, besides having the @ref Box widget, exposes its
 * foundation -- the Elementary Box Class -- in order to create
 * other widgets which are a box with some more logic on top.
 */

/**
 * Base widget smart data extended with box instance data.
 */
#define MAX_ITEMS_PER_VIEWPORT 10
#define MAX_COLORCLASS_LENGTH  20
//TIZEN_ONLY
typedef struct _Elm_Box_Banded_Item_Data        Elm_Box_Banded_Item_Data;
struct _Elm_Box_Banded_Item_Data
{
   Evas_Object                           *child_obj;
   Evas_Object                           * banded_bg;
   int                                   banded_index;
};

typedef enum
{
   ELM_BOX_PACK_START,
   ELM_BOX_PACK_END,
   ELM_BOX_PACK_BEFORE,
   ELM_BOX_PACK_AFTER
}Elm_Box_Pack_Type;

typedef struct _Elm_Box_Data        Elm_Box_Data;
struct _Elm_Box_Data
{
   Eina_Bool             homogeneous : 1;
   Eina_Bool             delete_me : 1;
   Eina_Bool             horizontal : 1;
   Eina_Bool             recalc : 1;
  #ifndef ELM_FEATURE_WEARABLE
    /*for banded ux*/
     Eina_List                             *children;
     Evas_Object                           *banded_bg_rect; /* banded color
                                                              background feature.
                                                              enabled only
                                                              un-scrollable. */
     Eina_Bool                             banded_bg_on : 1;
     int                                   item_color[MAX_ITEMS_PER_VIEWPORT][3];
     Eina_Bool                             color_calculated :1;
     Eina_Bool                             is_banded_style : 1;
     Evas_Object                           *scroller ;
     Elm_Box_Banded_Item_Data              *top_item ;
     int                                   item_color_r ;
     int                                   item_color_g ;
     int                                   item_color_b ;
     char                                  item_bg_color[MAX_COLORCLASS_LENGTH];
     char                                  list_bg_color[MAX_COLORCLASS_LENGTH];
     Evas_Coord                            svy;
     Evas_Coord                            svh;
  #endif

};

struct _Elm_Box_Transition
{
   double          initial_time;
   double          duration;
   Ecore_Animator *animator;

   struct
   {
      Evas_Object_Box_Layout layout;
      void                  *data;
      void                   (*free_data)(void *data);
   } start, end;

   void            (*transition_end_cb)(void *data);
   void           *transition_end_data;
   void            (*transition_end_free_data)(void *data);
   Eina_List      *objs;
   Evas_Object    *box;

   Eina_Bool       animation_ended : 1;
   Eina_Bool       recalculate : 1;
};

typedef struct _Transition_Animation_Data Transition_Animation_Data;
struct _Transition_Animation_Data
{
   Evas_Object *obj;
   struct
   {
      Evas_Coord x, y, w, h;
   } start, end;
};

/**
 * @}
 */

#define ELM_BOX_DATA_GET(o, sd) \
  Elm_Box_Data * sd = eo_data_scope_get(o, ELM_BOX_CLASS)

#define ELM_BOX_DATA_GET_OR_RETURN(o, ptr)           \
  ELM_BOX_DATA_GET(o, ptr);                          \
  if (EINA_UNLIKELY(!ptr))                           \
    {                                                \
       CRI("No widget data for object %p (%s)",      \
           o, evas_object_type_get(o));              \
       return;                                       \
    }

#define ELM_BOX_DATA_GET_OR_RETURN_VAL(o, ptr, val)  \
  ELM_BOX_DATA_GET(o, ptr);                          \
  if (EINA_UNLIKELY(!ptr))                           \
    {                                                \
       CRI("No widget data for object %p (%s)",      \
           o, evas_object_type_get(o));              \
       return val;                                   \
    }

#define ELM_BOX_CHECK(obj)                              \
  if (EINA_UNLIKELY(!eo_isa((obj), ELM_BOX_CLASS))) \
    return

#endif
