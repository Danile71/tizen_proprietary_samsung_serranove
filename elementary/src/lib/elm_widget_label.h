#ifndef ELM_WIDGET_LABEL_H
#define ELM_WIDGET_LABEL_H

#include "Elementary.h"

#include <Eio.h>

/* DO NOT USE THIS HEADER UNLESS YOU ARE PREPARED FOR BREAKING OF YOUR
 * CODE. THIS IS ELEMENTARY'S INTERNAL WIDGET API (for now) AND IS NOT
 * FINAL. CALL elm_widget_api_check(ELM_INTERNAL_API_VERSION) TO CHECK
 * IT AT RUNTIME.
 */

// TIZEN_ONLY(20150526): Support item with image path.
typedef struct _Sel Sel;
struct _Sel
{
   Evas_Textblock_Rectangle rect;
   Evas_Object *obj;
};

typedef struct _Elm_Label_Anchor_Data Elm_Label_Anchor_Data;
struct _Elm_Label_Anchor_Data
{
   Evas_Object          *obj;
   Eina_List            *sel;
   char                 *name;
   Evas_Textblock_Cursor *start, *end;

   Evas_Coord            down_x, down_y;
   Eina_Bool             mouse_down_flag : 1;
   Eina_Bool             item : 1;
};

typedef struct _Elm_Label_Item_Provider     Elm_Label_Item_Provider;
struct _Elm_Label_Item_Provider
{
   Evas_Object *(*func)(void *data, Evas_Object * entry, const char *item);
   void        *data;
};

//

/**
 * Base layout smart data extended with label instance data.
 */
typedef struct _Elm_Label_Data Elm_Label_Data;
struct _Elm_Label_Data
{
   const char           *format;
   double                slide_duration;
   double                slide_speed;
   Evas_Coord            lastw; /*<< lastly calculated resize object width.This is used to force the calculation on width changes. */
   Evas_Coord            wrap_w; /*<< wrap width by pixel for the line wrap support **/
   Elm_Wrap_Type         linewrap;
   Elm_Label_Slide_Mode  slide_mode;

   // TIZEN_ONLY(20150526): Support item with image path.
   Evas_Object          *tb;
   Eina_List            *anchors;
   Eina_List            *item_providers;
   Ecore_Timer          *longpress_timer;
   Evas_Coord            tx;
   Evas_Coord            ty;
   Evas_Coord            tw;
   Evas_Coord            th;
   Evas_Coord            down_x, down_y;
   Eina_Bool             on_clear : 1;
   Eina_Bool             anchors_force_update : 1;
   Eina_Bool             mouse_down_flag : 1;
   //

   Eina_Bool             ellipsis : 1;
   Eina_Bool             slide_ellipsis : 1;
   Eina_Bool             use_slide_speed : 1;
   Eina_Bool             slide_state : 1; /**< This will be marked as EINA_TRUE after elm_label_slide_go() is called. */
};

#define ELM_LABEL_DATA_GET(o, sd) \
  Elm_Label_Data * sd = eo_data_scope_get(o, ELM_LABEL_CLASS)

#define ELM_LABEL_DATA_GET_OR_RETURN(o, ptr)         \
  ELM_LABEL_DATA_GET(o, ptr);                        \
  if (EINA_UNLIKELY(!ptr))                           \
    {                                                \
       CRI("No widget data for object %p (%s)",      \
           o, evas_object_type_get(o));              \
       return;                                       \
    }

#define ELM_LABEL_DATA_GET_OR_RETURN_VAL(o, ptr, val) \
  ELM_LABEL_DATA_GET(o, ptr);                         \
  if (EINA_UNLIKELY(!ptr))                            \
    {                                                 \
       CRI("No widget data for object %p (%s)",       \
           o, evas_object_type_get(o));               \
       return val;                                    \
    }

#define ELM_LABEL_CHECK(obj)                              \
  if (EINA_UNLIKELY(!eo_isa((obj), ELM_LABEL_CLASS))) \
    return

#endif
