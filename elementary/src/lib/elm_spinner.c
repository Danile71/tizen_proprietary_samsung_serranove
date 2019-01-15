#ifdef HAVE_CONFIG_H
# include "elementary_config.h"
#endif

#define ELM_INTERFACE_ATSPI_ACCESSIBLE_PROTECTED
#define ELM_INTERFACE_ATSPI_VALUE_PROTECTED
#define ELM_INTERFACE_ATSPI_WIDGET_ACTION_PROTECTED

#include <Elementary.h>
#include <ctype.h>

#include "elm_priv.h"
#include "elm_widget_spinner.h"

#include "Eo.h"

#define MY_CLASS ELM_SPINNER_CLASS

#define MY_CLASS_NAME "Elm_Spinner"
#define MY_CLASS_NAME_LEGACY "elm_spinner"

#define ELM_SPINNER_DELAY_CHANGE_TIME 0.2

static const char SIG_CHANGED[] = "changed";
static const char SIG_DRAG_START[] = "spinner,drag,start";
static const char SIG_DRAG_STOP[] = "spinner,drag,stop";
static const char SIG_DELAY_CHANGED[] = "delay,changed";
//TIZEN_ONLY(20150715): Spinner entry changed callback support for datetime UX.
static const char SIG_ENTRY_CHANGED[] = "entry,changed";
//
//TIZEN_ONLY(20150722): Spinner entry callbacks support for datetime UX.
static const char SIG_ENTRY_SHOW[] = "entry,show";
static const char SIG_ENTRY_VAL_APPLY[] = "entry,apply";
//

static const Evas_Smart_Cb_Description _smart_callbacks[] = {
   {SIG_CHANGED, ""},
   {SIG_DELAY_CHANGED, ""},
   {SIG_DRAG_START, ""},
   {SIG_DRAG_STOP, ""},
   //TIZEN_ONLY(20150715): Spinner entry changed callback support for datetime UX.
   {SIG_ENTRY_CHANGED, ""},
   //
   {SIG_WIDGET_LANG_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_WIDGET_ACCESS_CHANGED, ""}, /**< handled by elm_widget */
   {SIG_LAYOUT_FOCUSED, ""}, /**< handled by elm_layout */
   {SIG_LAYOUT_UNFOCUSED, ""}, /**< handled by elm_layout */
   //TIZEN_ONLY(20150722): Spinner entry callbacks support for datetime UX.
   {SIG_ENTRY_VAL_APPLY, ""},
   {SIG_ENTRY_SHOW, ""},
   //
   {NULL, NULL}
};

static Eina_Bool _key_action_spin(Evas_Object *obj, const char *params);
static Eina_Bool _key_action_toggle(Evas_Object *obj, const char *params);

static const Elm_Action key_actions[] = {
   {"spin", _key_action_spin},
   {"toggle", _key_action_toggle},
   {NULL, NULL}
};

static void
_entry_show(Elm_Spinner_Data *sd)
{
   Eina_List *l;
   Elm_Spinner_Special_Value *sv;
   char buf[32], fmt[32] = "%0.f";

   EINA_LIST_FOREACH(sd->special_values, l, sv)
     {
        if (sv->value == sd->val)
          {
             snprintf(buf, sizeof(buf), "%s", sv->label);
             goto apply;
          }
     }
   /* try to construct just the format from given label
    * completely ignoring pre/post words
    */
   if (sd->label)
     {
        const char *start = strchr(sd->label, '%');
        while (start)
          {
             /* handle %% */
             if (start[1] != '%')
               break;
             else
               start = strchr(start + 2, '%');
          }

        if (start)
          {
             const char *itr, *end = NULL;
             for (itr = start + 1; *itr != '\0'; itr++)
               {
                  /* allowing '%d' is quite dangerous, remove it? */
                  if ((*itr == 'd') || (*itr == 'f'))
                    {
                       end = itr + 1;
                       break;
                    }
               }

             if ((end) && ((size_t)(end - start + 1) < sizeof(fmt)))
               {
                  memcpy(fmt, start, end - start);
                  fmt[end - start] = '\0';
               }
          }
     }
   snprintf(buf, sizeof(buf), fmt, sd->val);

apply:
   elm_object_text_set(sd->ent, buf);
}

static void
_label_write(Evas_Object *obj)
{
   Eina_List *l;
   char buf[1024];
   Elm_Spinner_Special_Value *sv;

   ELM_SPINNER_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->special_values, l, sv)
     {
        if (sv->value == sd->val)
          {
             snprintf(buf, sizeof(buf), "%s", sv->label);
             goto apply;
          }
     }
   if (sd->label)
     snprintf(buf, sizeof(buf), sd->label, sd->val);
   else
     snprintf(buf, sizeof(buf), "%.0f", sd->val);

apply:
   //TIZEN_ONLY(20150121): spin implementation merge into 2.4
   //elm_layout_text_set(obj, "elm.text", buf);
   elm_object_text_set(sd->text_button, buf);
   //////////////////////////////////////////////////////////
   elm_interface_atspi_accessible_name_changed_signal_emit(obj);
   if (sd->entry_visible) _entry_show(sd);
}

static Eina_Bool
_delay_change_timer_cb(void *data)
{
   ELM_SPINNER_DATA_GET(data, sd);

   sd->delay_change_timer = NULL;
   evas_object_smart_callback_call(data, SIG_DELAY_CHANGED, NULL);

   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_value_set(Evas_Object *obj,
           double new_val)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   if (sd->round > 0)
     new_val = sd->val_base +
       (double)((((int)(new_val - sd->val_base)) / sd->round) * sd->round);

   if (sd->wrap)
     {
        if (new_val < sd->val_min)
          new_val = sd->val_max;
        else if (new_val > sd->val_max)
          new_val = sd->val_min;
     }
   else
     {
        if (new_val < sd->val_min)
          new_val = sd->val_min;
        else if (new_val > sd->val_max)
          new_val = sd->val_max;
     }

   if (new_val == sd->val) return EINA_FALSE;
   sd->val = new_val;

   evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
   elm_interface_atspi_accessible_value_changed_signal_emit(obj);
   ecore_timer_del(sd->delay_change_timer);
   sd->delay_change_timer = ecore_timer_add(ELM_SPINNER_DELAY_CHANGE_TIME,
                                            _delay_change_timer_cb, obj);

   return EINA_TRUE;
}

static void
_val_set(Evas_Object *obj)
{
   double pos = 0.0;

   ELM_SPINNER_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->val_max > sd->val_min)
     pos = ((sd->val - sd->val_min) / (sd->val_max - sd->val_min));
   if (pos < 0.0) pos = 0.0;
   else if (pos > 1.0)
     pos = 1.0;
   edje_object_part_drag_value_set
     (wd->resize_obj, "elm.dragable.slider", pos, pos);
}

static void
_drag_cb(void *data,
         Evas_Object *_obj EINA_UNUSED,
         const char *emission EINA_UNUSED,
         const char *source EINA_UNUSED)
{
   double pos = 0.0, delta;
   Evas_Object *obj = data;

   ELM_SPINNER_DATA_GET(obj, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   if (sd->entry_visible) return;
   eo_do((Eo *)wd->resize_obj,
         edje_obj_part_drag_value_get("elm.dragable.slider", &pos, NULL));

   delta = pos * sd->step * _elm_config->scale;
   /* If we are on rtl mode, change the delta to be negative on such changes */
   if (elm_widget_mirrored_get(obj)) delta *= -1;
   if (_value_set(data, sd->drag_start_val + delta)) _label_write(data);
   sd->dragging = 1;
}

static void
_drag_start_cb(void *data,
               Evas_Object *obj EINA_UNUSED,
               const char *emission EINA_UNUSED,
               const char *source EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   sd->drag_start_val = sd->val;

   evas_object_smart_callback_call(obj, SIG_DRAG_START, NULL);
}

static void
_drag_stop_cb(void *data,
              Evas_Object *obj EINA_UNUSED,
              const char *emission EINA_UNUSED,
              const char *source EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);
   ELM_WIDGET_DATA_GET_OR_RETURN(data, wd);

   sd->drag_start_val = 0;
   edje_object_part_drag_value_set
     (wd->resize_obj, "elm.dragable.slider", 0.0, 0.0);

   evas_object_smart_callback_call(obj, SIG_DRAG_STOP, NULL);
}

static void
_entry_hide(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   elm_layout_signal_emit(obj, "elm,state,inactive", "elm");
   sd->entry_visible = EINA_FALSE;
}

static void
_entry_value_apply(Evas_Object *obj)
{
   const char *str;
   double val;
   char *end;

   ELM_SPINNER_DATA_GET(obj, sd);

   if (!sd->entry_visible) return;

   _entry_hide(obj);
   str = elm_object_text_get(sd->ent);
   if (!str) return;
   val = strtod(str, &end);
   if ((*end != '\0') && (!isspace(*end))) return;
   //TIZEN_ONLY(20150722): Spinner entry apply callback support for datetime UX.
   evas_object_smart_callback_call(obj, SIG_ENTRY_VAL_APPLY, &val);
   //
   elm_spinner_value_set(obj, val);
}

static void
_entry_activated_cb(void *data,
                    Evas_Object *obj EINA_UNUSED,
                    void *event_info EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   _entry_value_apply(data);
   evas_object_smart_callback_call(data, SIG_CHANGED, NULL);
   ecore_timer_del(sd->delay_change_timer);
   sd->delay_change_timer = ecore_timer_add(ELM_SPINNER_DELAY_CHANGE_TIME,
                                            _delay_change_timer_cb, data);
}

//TIZEN_ONLY(20150715): Spinner entry changed callback support for datetime UX.
static void
_entry_changed_user_cb(void *data,
                       Evas_Object *obj,
                       void *event_info EINA_UNUSED)
{
   Evas_Object *spinner;
   const char *str;
   int len, max_len;
   double min, max, val;

   spinner = data;
   str = elm_object_text_get(obj);
   len = strlen(str);
   if (len < 1) return;
   val = atof(str);
   elm_spinner_min_max_get(spinner, &min, &max);
   max_len = log10(abs(max)) + 1;
   if (max_len == len)
     {
        val = (val < min) ? min : (val > max ? max : val);
        elm_spinner_value_set(spinner, val);
        _label_write(spinner);
        elm_entry_cursor_end_set(obj);
        str = elm_object_text_get(obj);
     }

   evas_object_smart_callback_call(spinner, SIG_ENTRY_CHANGED, (void *)str);
}
//

//TIZEN_ONLY(20150305): Added entry filter feature.
static void
_entry_filter_add(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;

   if (!sd->ent) return;

   elm_entry_markup_filter_remove(sd->ent, elm_entry_filter_accept_set, &digits_filter_data);
   elm_entry_markup_filter_remove(sd->ent, elm_entry_filter_limit_size, &limit_filter_data);

   int num_digit = log10(abs(sd->val_max)) + 1;
   limit_filter_data.max_byte_count = 0;

   digits_filter_data.accepted = "0123456789";
   digits_filter_data.rejected = NULL;

   if (sd->decimal_points > 0)
     {
        digits_filter_data.accepted = ".0123456789";
        num_digit += (sd->decimal_points + 1);
     }

   limit_filter_data.max_char_count = num_digit;

   elm_entry_markup_filter_append(sd->ent, elm_entry_filter_accept_set, &digits_filter_data);
   elm_entry_markup_filter_append(sd->ent, elm_entry_filter_limit_size, &limit_filter_data);
}
/////////////////////////////////////////////////////

// TIZEN_ONLY(20150226): Fix spinner widget's entry show/hide issues.
static void
_entry_show_cb(void *data,
               Evas *e EINA_UNUSED,
               Evas_Object *obj,
               void *event_info EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);
   //TIZEN_ONLY(20150722): Spinner entry show callback support for datetime UX.
   evas_object_smart_callback_call(data, SIG_ENTRY_SHOW, NULL);
   //

   _entry_show(sd);
   elm_object_focus_set(obj, EINA_TRUE);
   elm_entry_select_all(obj);
   sd->entry_visible = EINA_TRUE;
}
//

// TIZEN_ONLY(20150711): Select text on click
static void
_entry_clicked_cb(void *data EINA_UNUSED,
                  Evas_Object *obj,
                  void *event_info EINA_UNUSED)
{
   elm_entry_select_all(obj);
}
//

static void
_toggle_entry(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   if (sd->dragging)
     {
        sd->dragging = 0;
        return;
     }
   if (elm_widget_disabled_get(obj)) return;
   if (!sd->editable) return;
   if (sd->entry_visible) _entry_value_apply(obj);
   else
     {
        // TIZEN_ONLY(20150729): To defer the entry creation for performance
        /*if (!sd->ent)
          {
             sd->ent = elm_entry_add(obj);
             elm_entry_single_line_set(sd->ent, EINA_TRUE);
             // TIZEN_ONLY(20150527): Disable ctxpopup for copy & paste.
             elm_entry_context_menu_disabled_set(sd->ent, EINA_TRUE);
             elm_entry_cnp_mode_set(sd->ent, ELM_CNP_MODE_PLAINTEXT);
             //
             //TIZEN_ONLY(20150711): Disable selection handler
             elm_entry_selection_handler_disabled_set(sd->ent, EINA_TRUE);
             //
             //TIZEN_ONLY(20150711): Use below Internal Tizen only APIs
             _elm_entry_drag_enabled_set(sd->ent, EINA_FALSE);
             _elm_entry_magnifier_enabled_set(sd->ent, EINA_FALSE);
             _elm_entry_cursor_handler_enabled_set(sd->ent, EINA_FALSE);
             //
             // TIZEN_ONLY(20150226): Apply default properties for entry widget in the spinner.
             Eina_Strbuf *buf = eina_strbuf_new();
             eina_strbuf_append_printf(buf, "spinner/%s", elm_widget_style_get(obj));
             elm_widget_style_set(sd->ent, eina_strbuf_string_get(buf));
             eina_strbuf_free(buf);
             elm_entry_cnp_mode_set(sd->ent, ELM_CNP_MODE_PLAINTEXT);
             elm_entry_input_panel_layout_set(sd->ent, ELM_INPUT_PANEL_LAYOUT_NUMBERONLY);
             elm_entry_prediction_allow_set(sd->ent, EINA_FALSE);
             _entry_filter_add(obj);
             //
             evas_object_smart_callback_add
                (sd->ent, "activated", _entry_activated_cb, obj);
             // TIZEN_ONLY(20150226): Fix spinner widget's entry show/hide issues.
             evas_object_event_callback_add
                (sd->ent, EVAS_CALLBACK_SHOW, _entry_show_cb, obj);
             //
             //TIZEN_ONLY(20150715): Spinner entry changed callback support for datetime UX.
             evas_object_smart_callback_add
                (sd->ent, "changed,user", _entry_changed_user_cb, obj);
             //
             // TIZEN_ONLY(20150711): Select text on click
             evas_object_smart_callback_add
                (sd->ent, "clicked", _entry_clicked_cb, obj);
             //
             elm_layout_content_set(obj, "elm.swallow.entry", sd->ent);
          }
          */
        //
        elm_layout_signal_emit(obj, "elm,state,active", "elm");
        // TIZEN_ONLY(20150226): Fix spinner widget's entry show/hide issues.
        //_entry_show(sd);
        //elm_entry_select_all(sd->ent);
        //elm_widget_focus_set(sd->ent, EINA_TRUE);
        //sd->entry_visible = EINA_TRUE;
     }
}

// TIZEN_ONLY(20150729): To defer the entry creation for performance
static void
_entry_create_cb(void *data)
{
   Evas_Object *obj = data;

   ELM_SPINNER_DATA_GET(obj, sd);

   if (!sd->ent)
     {
        sd->ent = elm_entry_add(obj);
        elm_entry_single_line_set(sd->ent, EINA_TRUE);
        // TIZEN_ONLY(20150527): Disable ctxpopup for copy & paste.
        elm_entry_context_menu_disabled_set(sd->ent, EINA_TRUE);
        elm_entry_cnp_mode_set(sd->ent, ELM_CNP_MODE_PLAINTEXT);
        //
        //TIZEN_ONLY(20150711): Disable selection handler
        elm_entry_selection_handler_disabled_set(sd->ent, EINA_TRUE);
        //
        //TIZEN_ONLY(20150711): Use below Internal Tizen only APIs
        _elm_entry_drag_enabled_set(sd->ent, EINA_FALSE);
        _elm_entry_magnifier_enabled_set(sd->ent, EINA_FALSE);
        _elm_entry_cursor_handler_enabled_set(sd->ent, EINA_FALSE);
        //
        // TIZEN_ONLY(20150226): Apply default properties for entry widget in the spinner.
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "spinner/%s", elm_widget_style_get(obj));
        elm_widget_style_set(sd->ent, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
        elm_entry_cnp_mode_set(sd->ent, ELM_CNP_MODE_PLAINTEXT);
        elm_entry_input_panel_layout_set(sd->ent, ELM_INPUT_PANEL_LAYOUT_NUMBERONLY);
        elm_entry_prediction_allow_set(sd->ent, EINA_FALSE);
        _entry_filter_add(obj);
        //
        evas_object_smart_callback_add
           (sd->ent, "activated", _entry_activated_cb, obj);
        // TIZEN_ONLY(20150226): Fix spinner widget's entry show/hide issues.
        evas_object_event_callback_add
           (sd->ent, EVAS_CALLBACK_SHOW, _entry_show_cb, obj);
        //
        //TIZEN_ONLY(20150715): Spinner entry changed callback support for datetime UX.
        evas_object_smart_callback_add
           (sd->ent, "changed,user", _entry_changed_user_cb, obj);
        //
        // TIZEN_ONLY(20150711): Select text on click
        evas_object_smart_callback_add
           (sd->ent, "clicked", _entry_clicked_cb, obj);
        //
        elm_layout_content_set(obj, "elm.swallow.entry", sd->ent);
     }
   sd->ent_create_job = NULL;
}
//

//TIZEN_ONLY(20150121): spin implementation merge into 2.4
static void
_text_button_clicked_cb(void *data,
                        Evas_Object *obj EINA_UNUSED,
                        void *event_info EINA_UNUSED)
{
   _toggle_entry(data);
}
//////////////////////////////////////////////////////////

static void
_spin_stop(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->interval = sd->first_interval;
   sd->spin_speed = 0;
   ELM_SAFE_FREE(sd->spin_timer, ecore_timer_del);
}

static void
_entry_toggle_cb(void *data EINA_UNUSED,
                 Evas_Object *obj,
                 const char *emission  EINA_UNUSED,
                 const char *source EINA_UNUSED)
{
   _toggle_entry(obj);
}

static Eina_Bool
_spin_value(void *data)
{
   ELM_SPINNER_DATA_GET(data, sd);
   double real_speed = sd->spin_speed;

   /* Sanity check: our step size should be at least as large as our rounding value */
   if ((sd->spin_speed != 0.0) && (abs(sd->spin_speed) < sd->round))
     {
        WRN("The spinning step is smaller than the rounding value, please check your code");
        real_speed = sd->spin_speed > 0 ? sd->round : -sd->round;
     }

   //TIZEN_ONLY(20150121): spin implementation merge into 2.4
   /*
   sd->interval = sd->interval / 1.05;

   // spin_timer does not exist when _spin_value() is called from wheel event
   if (sd->spin_timer)
    ecore_timer_interval_set(sd->spin_timer, sd->interval);
   */
   //////////////////////////////////////////////////////////
   if (_value_set(data, sd->val + real_speed)) _label_write(data);

   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_val_inc_start(void *data)
{
   ELM_SPINNER_DATA_GET(data, sd);

   sd->interval = sd->first_interval;
   sd->spin_speed = sd->step;
   sd->longpress_timer = NULL;
   //TIZEN_ONLY(20150121): spin implementation merge into 2.4
   //ecore_timer_del(sd->spin_timer);
   elm_layout_signal_emit(data, "elm,action,longpress", "elm");
   if (sd->spin_timer) ecore_timer_del(sd->spin_timer);
   //////////////////////////////////////////////////////////
   sd->spin_timer = ecore_timer_add(sd->interval, _spin_value, data);
   _spin_value(data);

   //TIZEN_ONLY(20150922): Prevent scroll in longpressed state.
   elm_widget_scroll_freeze_push(data);
   //
   return ECORE_CALLBACK_CANCEL;
}

//TIZEN_ONLY(20150121): spin implementation merge into 2.4
static void
_val_inc_stop(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->spin_speed = 0;
   if (sd->spin_timer) ecore_timer_del(sd->spin_timer);
   sd->spin_timer = NULL;

   //TIZEN_ONLY(20150922): Prevent scroll in longpressed state.
   elm_widget_scroll_freeze_pop(obj);
   //
}
//////////////////////////////////////////////////////////

static Eina_Bool
_val_dec_start(void *data)
{
   ELM_SPINNER_DATA_GET(data, sd);

   sd->interval = sd->first_interval;
   sd->spin_speed = -sd->step;
   sd->longpress_timer = NULL;
   //TIZEN_ONLY(20150121): spin implementation merge into 2.4
   //ecore_timer_del(sd->spin_timer);
   elm_layout_signal_emit(data, "elm,action,longpress", "elm");
   if (sd->spin_timer) ecore_timer_del(sd->spin_timer);
   //////////////////////////////////////////////////////////
   sd->spin_timer = ecore_timer_add(sd->interval, _spin_value, data);
   _spin_value(data);

   //TIZEN_ONLY(20150922): Prevent scroll in longpressed state.
   elm_widget_scroll_freeze_push(data);
   //
   return ECORE_CALLBACK_CANCEL;
}

//TIZEN_ONLY(20150121): spin implementation merge into 2.4
/*
static void
_button_inc_start_cb(void *data,
                     Evas_Object *obj,
                     const char *emission EINA_UNUSED,
                     const char *source EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->entry_visible)
     {
        _entry_value_apply(obj);
        if ((sd->val_updated) && (sd->val == sd->val_min)) return;
     }
   ecore_timer_del(sd->longpress_timer);
   sd->longpress_timer = ecore_timer_add
      (_elm_config->longpress_timeout, _val_inc_start, data);
}

static void
_button_inc_stop_cb(void *data,
                    Evas_Object *obj EINA_UNUSED,
                    const char *emission EINA_UNUSED,
                    const char *source EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);
   if (sd->longpress_timer)
     {
        ELM_SAFE_FREE(sd->longpress_timer, ecore_timer_del);
        sd->spin_speed = sd->step;
        _spin_value(data);
     }
   _spin_stop(data);
}

static void
_button_dec_start_cb(void *data,
                     Evas_Object *obj EINA_UNUSED,
                     const char *emission EINA_UNUSED,
                     const char *source EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->entry_visible)
     {
        _entry_value_apply(obj);
        if ((sd->val_updated) && (sd->val == sd->val_max)) return;
     }
   ecore_timer_del(sd->longpress_timer);
   sd->longpress_timer = ecore_timer_add
      (_elm_config->longpress_timeout, _val_dec_start, data);
}
static void
_button_dec_stop_cb(void *data,
                    Evas_Object *obj EINA_UNUSED,
                    const char *emission EINA_UNUSED,
                    const char *source EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);
   if (sd->longpress_timer)
     {
        ELM_SAFE_FREE(sd->longpress_timer, ecore_timer_del);
        sd->spin_speed = -sd->step;
        _spin_value(data);
     }
   _spin_stop(data);
}
*/

static void
_val_dec_stop(Evas_Object *obj)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   sd->spin_speed = 0;
   if (sd->spin_timer) ecore_timer_del(sd->spin_timer);
   sd->spin_timer = NULL;

   //TIZEN_ONLY(20150922): Prevent scroll in longpressed state.
   elm_widget_scroll_freeze_pop(obj);
   //
}

static void
_inc_button_clicked_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   _val_inc_stop(data);
   sd->spin_speed = sd->step;
   _spin_value(data);

   if (sd->entry_visible) _entry_value_apply(data);

   /*FIXME: _access_increment_decrement_info_say function should be implemented.
   if (_elm_config->access_mode)
     _access_increment_decrement_info_say(data, EINA_TRUE);
   */
}

static void
_inc_button_pressed_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   sd->longpress_timer = ecore_timer_add
      (_elm_config->longpress_timeout,
       _val_inc_start, data);

   if (sd->entry_visible) _entry_value_apply(data);
}

static void
_inc_button_unpressed_cb(void *data,
                         Evas_Object *obj EINA_UNUSED,
                         void *event_info EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->longpress_timer)
     {
        ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = NULL;
     }
   _val_inc_stop(data);
}

static void
_dec_button_clicked_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   _val_dec_stop(data);
   sd->spin_speed = -sd->step;
   _spin_value(data);

   if (sd->entry_visible) _entry_value_apply(data);

   /*FIXME: _access_increment_decrement_info_say function should be implemented.
   if (_elm_config->access_mode)
     _access_increment_decrement_info_say(data, EINA_FALSE);
   */
}

static void
_dec_button_pressed_cb(void *data,
                       Evas_Object *obj EINA_UNUSED,
                       void *event_info EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->longpress_timer) ecore_timer_del(sd->longpress_timer);
   sd->longpress_timer = ecore_timer_add
      (_elm_config->longpress_timeout,
       _val_dec_start, data);

   if (sd->entry_visible) _entry_value_apply(data);
}

static void
_dec_button_unpressed_cb(void *data,
                         Evas_Object *obj EINA_UNUSED,
                         void *event_info EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(data, sd);

   if (sd->longpress_timer)
     {
        ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = NULL;
     }
   _val_dec_stop(data);
}
//////////////////////////////////////////////////////////

//TIZEN_ONLY(20150922): Delete longpress_timer in scroll enabled case.
static void
_inc_dec_button_mouse_move_cb(void *data,
                              Evas *evas EINA_UNUSED,
                              Evas_Object *obj EINA_UNUSED,
                              void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   ELM_SPINNER_DATA_GET(data, sd);

   if ((ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) && sd->longpress_timer)
     {
        ecore_timer_del(sd->longpress_timer);
        sd->longpress_timer = NULL;
     }
}
//

EOLIAN static void
_elm_spinner_elm_layout_sizing_eval(Eo *obj, Elm_Spinner_Data *_pd EINA_UNUSED)
{
   Evas_Coord minw = -1, minh = -1;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   edje_object_size_min_restricted_calc
     (wd->resize_obj, &minw, &minh, minw, minh);
   elm_coords_finger_size_adjust(1, &minw, 1, &minh);
   evas_object_size_hint_min_set(obj, minw, minh);
   evas_object_size_hint_max_set(obj, -1, -1);
}

static Eina_Bool
_key_action_spin(Evas_Object *obj, const char *params)
{
   const char *dir = params;
   Eina_Bool horz = !!strncmp(elm_widget_style_get(obj), "vertical", 8);

   if (((!strcmp(dir, "left")) && horz) ||
       ((!strcmp(dir, "down")) && !horz))
     {
        _val_dec_start(obj);
        elm_layout_signal_emit(obj, "elm,left,anim,activate", "elm");
     }
   else if (((!strcmp(dir, "right")) && horz) ||
            ((!strcmp(dir, "up")) && !horz))
     {
        _val_inc_start(obj);
        elm_layout_signal_emit(obj, "elm,right,anim,activate", "elm");
     }
   else return EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_key_action_toggle(Evas_Object *obj, const char *params EINA_UNUSED)
{
   ELM_SPINNER_DATA_GET(obj, sd);

   if (sd->spin_timer) _spin_stop(obj);
   else if (sd->entry_visible) _entry_toggle_cb(NULL, obj, NULL, NULL);
   return EINA_FALSE;
}

EOLIAN static Eina_Bool
_elm_spinner_elm_widget_event(Eo *obj, Elm_Spinner_Data *sd EINA_UNUSED, Evas_Object *src, Evas_Callback_Type type, void *event_info)
{
   Evas_Event_Key_Down *ev = event_info;
   Evas_Event_Mouse_Wheel *mev;
   (void) src;

   if (type == EVAS_CALLBACK_KEY_DOWN)
     {
        Eina_Bool ret;

        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
        ret = _elm_config_key_binding_call(obj, ev, key_actions);
        if (!ret)
          {
             if (sd->spin_timer) _spin_stop(obj);
             else return EINA_FALSE;
          }
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (type == EVAS_CALLBACK_KEY_UP)
     {
        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
        if (sd->spin_timer) _spin_stop(obj);
        else return EINA_FALSE;
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (type == EVAS_CALLBACK_MOUSE_WHEEL)
     {
        if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return EINA_FALSE;
        mev = event_info;
        sd->interval = sd->first_interval;
        if (mev->z < 0)
          {
             sd->spin_speed = sd->step;
             elm_layout_signal_emit(obj, "elm,right,anim,activate", "elm");
          }
        else
          {
             sd->spin_speed = -sd->step;
             elm_layout_signal_emit(obj, "elm,left,anim,activate", "elm");
          }
        _spin_value(obj);
        mev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else return EINA_FALSE;

   return EINA_TRUE;
}

EOLIAN static Eina_Bool
_elm_spinner_elm_widget_on_focus(Eo *obj, Elm_Spinner_Data *sd)
{
   Eina_Bool int_ret = EINA_FALSE;
   eo_do_super(obj, MY_CLASS, int_ret = elm_obj_widget_on_focus());
   if (!int_ret) return EINA_FALSE;

   if (!elm_widget_focus_get(obj))
     {
        ELM_SAFE_FREE(sd->delay_change_timer, ecore_timer_del);
        ELM_SAFE_FREE(sd->spin_timer, ecore_timer_del);
        //TIZEN_ONLY(20150707): entry visibility check added
        if (sd->entry_visible && !evas_focus_state_get(evas_object_evas_get(obj)))
          sd->entry_was_visible = EINA_TRUE;
        //

        _entry_value_apply(obj);
     }
   //TIZEN_ONLY(20150707): entry visibility check added
   else
     {
        if (sd->entry_was_visible)
          {
             _toggle_entry(obj);
             sd->entry_was_visible = EINA_FALSE;
          }
     }
   //

   return EINA_TRUE;
}

static char *
_access_info_cb(void *data, Evas_Object *obj)
{
   Evas_Object *spinner;
   const char *txt = elm_widget_access_info_get(obj);

   spinner = data;
   if (!txt) txt = elm_layout_text_get(spinner, "elm.text");
   if (txt) return strdup(txt);

   return NULL;
}

static char *
_access_state_cb(void *data, Evas_Object *obj EINA_UNUSED)
{
   if (elm_widget_disabled_get(data))
     return strdup(E_("State: Disabled"));

   return NULL;
}

static void
_access_activate_cb(void *data,
                    Evas_Object *part_obj,
                    Elm_Object_Item *item EINA_UNUSED)
{
   char *text;
   Eina_Strbuf *buf;
   Evas_Object *eo, *inc_btn;
   const char* increment_part;

   if (!strncmp(elm_widget_style_get(data), "vertical", 8))
     increment_part = "up_bt";
   else
     increment_part = "right_bt";

   eo = elm_layout_edje_get(data);
   inc_btn = (Evas_Object *)edje_object_part_object_get(eo, increment_part);

   if (part_obj != inc_btn)
     {
        _val_dec_start(data);
        elm_layout_signal_emit(data, "elm,left,anim,activate", "elm");
        _spin_stop(data);
        text = "decremented";
     }
   else
     {
        _val_inc_start(data);
        elm_layout_signal_emit(data, "elm,right,anim,activate", "elm");
        _spin_stop(data);
        text = "incremented";
     }

   buf = eina_strbuf_new();

   eina_strbuf_append_printf(buf, "%s, %s", text,
            elm_layout_text_get(data, "elm.text"));

   text = eina_strbuf_string_steal(buf);
   eina_strbuf_free(buf);

   _elm_access_say(text);
}

static void
_access_spinner_register(Evas_Object *obj, Eina_Bool is_access)
{
   Evas_Object *ao;
   Elm_Access_Info *ai;
   const char* increment_part;
   const char* decrement_part;

   if (!strncmp(elm_widget_style_get(obj), "vertical", 8))
     {
        increment_part = "up_bt";
        decrement_part = "down_bt";
     }
   else
     {
        increment_part = "right_bt";
        decrement_part = "left_bt";
     }

   if (!is_access)
     {
        /* unregister increment button, decrement button and spinner label */
        _elm_access_edje_object_part_object_unregister
          (obj, elm_layout_edje_get(obj), increment_part);

        _elm_access_edje_object_part_object_unregister
          (obj, elm_layout_edje_get(obj), decrement_part);

        _elm_access_edje_object_part_object_unregister
          (obj, elm_layout_edje_get(obj), "access.text");

        return;
     }

   /* register increment button */
   ao = _elm_access_edje_object_part_object_register
          (obj, elm_layout_edje_get(obj), increment_part);

   ai = _elm_access_info_get(ao);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE,
                        E_("spinner increment button"));
   _elm_access_activate_callback_set(ai, _access_activate_cb, obj);

   /* register decrement button */
   ao = _elm_access_edje_object_part_object_register
          (obj, elm_layout_edje_get(obj), decrement_part);

   ai = _elm_access_info_get(ao);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE,
                        E_("spinner decrement button"));
   _elm_access_activate_callback_set(ai, _access_activate_cb, obj);

   /* register spinner label */
   ao = _elm_access_edje_object_part_object_register
          (obj, elm_layout_edje_get(obj), "access.text");

   ai = _elm_access_info_get(ao);
   _elm_access_text_set(ai, ELM_ACCESS_TYPE, E_("spinner"));
   _elm_access_callback_set(ai, ELM_ACCESS_INFO, _access_info_cb, obj);
   _elm_access_callback_set(ai, ELM_ACCESS_STATE, _access_state_cb, obj);
}

EOLIAN static void
_elm_spinner_evas_object_smart_add(Eo *obj, Elm_Spinner_Data *priv)
{
   Evas_Object *spinner = NULL;
   char *spinner_style = NULL;
   ELM_WIDGET_DATA_GET_OR_RETURN(obj, wd);

   eo_do_super(obj, MY_CLASS, evas_obj_smart_add());
   elm_widget_sub_object_parent_add(obj);

   priv->val_max = 100.0;
   priv->step = 1.0;
   priv->first_interval = 0.85;
   //TIZEN_ONLY(20150722): Maintain compatibility with Tizen_2.3
   priv->editable = EINA_TRUE;
   //

   spinner = elm_object_parent_widget_get(obj);
   spinner_style = (char *)evas_object_data_get(spinner, "ELM_SPINNER");

   if (!spinner_style)
     spinner_style = elm_widget_style_get(obj);

   if (!elm_layout_theme_set(obj, "spinner", "base", spinner_style))
     ERR("Failed to set layout!");

   elm_layout_signal_callback_add(obj, "drag", "*", _drag_cb, obj);
   elm_layout_signal_callback_add(obj, "drag,start", "*", _drag_start_cb, obj);
   elm_layout_signal_callback_add(obj, "drag,stop", "*", _drag_stop_cb, obj);
   elm_layout_signal_callback_add(obj, "drag,step", "*", _drag_stop_cb, obj);
   elm_layout_signal_callback_add(obj, "drag,page", "*", _drag_stop_cb, obj);

   //TIZEN_ONLY(20150121): spin implementation merge into 2.4
   /*
   elm_layout_signal_callback_add
        (obj, "elm,action,increment,start", "*", _button_inc_start_cb, obj);
   elm_layout_signal_callback_add
        (obj, "elm,action,increment,stop", "*", _button_inc_stop_cb, obj);
   elm_layout_signal_callback_add
        (obj, "elm,action,decrement,start", "*", _button_dec_start_cb, obj);
   elm_layout_signal_callback_add
        (obj, "elm,action,decrement,stop", "*", _button_dec_stop_cb, obj);
   */

   if (strcmp("default",spinner_style))
     evas_object_data_set(obj, "ELM_BUTTON_FOR_DATETIME_SPINNER_INCREASE", (void *)spinner_style);
   priv->inc_button = elm_button_add(obj);
   if (!strcmp("default",spinner_style))
     elm_object_style_set(priv->inc_button, "spinner/increase/default");

   evas_object_smart_callback_add
      (priv->inc_button, "clicked", _inc_button_clicked_cb, obj);
   evas_object_smart_callback_add
      (priv->inc_button, "pressed", _inc_button_pressed_cb, obj);
   evas_object_smart_callback_add
      (priv->inc_button, "unpressed", _inc_button_unpressed_cb, obj);
   //TIZEN_ONLY(20150922): Delete longpress_timer in scroll enabled case.
   evas_object_event_callback_add
      (priv->inc_button, EVAS_CALLBACK_MOUSE_MOVE, _inc_dec_button_mouse_move_cb, obj);
   //

   elm_layout_content_set(obj, "elm.swallow.inc_button", priv->inc_button);
   elm_widget_sub_object_add(obj, priv->inc_button);

   if (strcmp("default",spinner_style))
     {
        evas_object_data_del(obj, "ELM_BUTTON_FOR_DATETIME_SPINNER_INCREASE");
        evas_object_data_set(obj, "ELM_BUTTON_FOR_DATETIME_SPINNER_TEXT", (void *)spinner_style);
     }
   priv->text_button = elm_button_add(obj);
   if (!strcmp("default",spinner_style))
     elm_object_style_set(priv->text_button, "spinner/default");

   evas_object_smart_callback_add
      (priv->text_button, "clicked", _text_button_clicked_cb, obj);

   elm_layout_content_set(obj, "elm.swallow.text_button", priv->text_button);
   elm_widget_sub_object_add(obj, priv->text_button);

   if (strcmp("default",spinner_style))
     {
        evas_object_data_del(obj, "ELM_BUTTON_FOR_DATETIME_SPINNER_TEXT");
        evas_object_data_set(obj, "ELM_BUTTON_FOR_DATETIME_SPINNER_DECREASE", (void *)spinner_style);
     }
   priv->dec_button = elm_button_add(obj);

   if (!strcmp("default",spinner_style))
     elm_object_style_set(priv->dec_button, "spinner/decrease/default");

   evas_object_smart_callback_add
      (priv->dec_button, "clicked", _dec_button_clicked_cb, obj);
   evas_object_smart_callback_add
      (priv->dec_button, "pressed", _dec_button_pressed_cb, obj);
   evas_object_smart_callback_add
      (priv->dec_button, "unpressed", _dec_button_unpressed_cb, obj);
   //TIZEN_ONLY(20150922): Delete longpress_timer in scroll enabled case.
   evas_object_event_callback_add
      (priv->dec_button, EVAS_CALLBACK_MOUSE_MOVE, _inc_dec_button_mouse_move_cb, obj);
   //

   elm_layout_content_set(obj, "elm.swallow.dec_button", priv->dec_button);
   elm_widget_sub_object_add(obj, priv->dec_button);
   //////////////////////////////////////////////////////////
   edje_object_part_drag_value_set
      (wd->resize_obj, "elm.dragable.slider", 0.0, 0.0);

   elm_layout_signal_callback_add
   //TIZEN_ONLY(20150121): spin implementation merge into 2.4
   //   (obj, "elm,action,entry,toggle", "*", _entry_toggle_cb, NULL);
      (obj, "elm,action,entry,toggle", "*", _entry_toggle_cb, obj);
   //////////////////////////////////////////////////////////

   _label_write(obj);
   elm_widget_can_focus_set(obj, EINA_TRUE);

   elm_layout_sizing_eval(obj);

   // TIZEN_ONLY(20150729): To defer the entry creation for performance
   if (priv->ent_create_job)
     {
        ecore_job_del(priv->ent_create_job);
        priv->ent_create_job = NULL;
     }
   priv->ent_create_job = ecore_job_add(_entry_create_cb, obj);
   //
   /* access */
   if (_elm_config->access_mode)
     _access_spinner_register(obj, EINA_TRUE);
}

EOLIAN static void
_elm_spinner_evas_object_smart_del(Eo *obj, Elm_Spinner_Data *sd)
{
   Elm_Spinner_Special_Value *sv;

   eina_stringshare_del(sd->label);
   ecore_timer_del(sd->delay_change_timer);
   ecore_timer_del(sd->spin_timer);
   ecore_timer_del(sd->longpress_timer);

   if (sd->special_values)
     {
        EINA_LIST_FREE(sd->special_values, sv)
          {
             eina_stringshare_del(sv->label);
             free(sv);
          }
     }

   // TIZEN_ONLY(20150729): To defer the entry creation for performance
   if (sd->ent_create_job)
     {
        ecore_job_del(sd->ent_create_job);
        sd->ent_create_job = NULL;
     }
   //
   eo_do_super(obj, MY_CLASS, evas_obj_smart_del());
}

EOLIAN static Eina_Bool
_elm_spinner_elm_widget_theme_apply(Eo *obj, Elm_Spinner_Data *sd)
{
   Eina_Bool int_ret = elm_layout_theme_set(obj, "spinner", "base",
                              elm_widget_style_get(obj));

   if (!int_ret) ERR("Failed to set layout!");

   if (sd->ent)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "spinner/%s", elm_widget_style_get(obj));
        elm_widget_style_set(sd->ent, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }

   if (sd->inc_button)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "spinner/increase/%s", elm_widget_style_get(obj));
        elm_widget_style_set(sd->inc_button, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }

   if (sd->text_button)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "spinner/%s", elm_widget_style_get(obj));
        elm_widget_style_set(sd->text_button, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }

   if (sd->dec_button)
     {
        Eina_Strbuf *buf = eina_strbuf_new();
        eina_strbuf_append_printf(buf, "spinner/decrease/%s", elm_widget_style_get(obj));
        elm_widget_style_set(sd->dec_button, eina_strbuf_string_get(buf));
        eina_strbuf_free(buf);
     }

   if (_elm_config->access_mode)
     _access_spinner_register(obj, EINA_TRUE);

   elm_layout_sizing_eval(obj);
   return int_ret;
}

static Eina_Bool _elm_spinner_smart_focus_next_enable = EINA_FALSE;

EOLIAN static Eina_Bool
_elm_spinner_elm_widget_focus_next_manager_is(Eo *obj EINA_UNUSED, Elm_Spinner_Data *_pd EINA_UNUSED)
{
   return _elm_spinner_smart_focus_next_enable;
}

EOLIAN static Eina_Bool
_elm_spinner_elm_widget_focus_direction_manager_is(Eo *obj EINA_UNUSED, Elm_Spinner_Data *_pd EINA_UNUSED)
{
   return EINA_FALSE;
}

static Evas_Object *
_access_object_get(const Evas_Object *obj, const char* part)
{
   Evas_Object *eo, *po, *ao;

   eo = elm_layout_edje_get(obj);

   po = (Evas_Object *)edje_object_part_object_get(eo, part);
   ao = evas_object_data_get(po, "_part_access_obj");

   return ao;
}

EOLIAN static Eina_Bool
_elm_spinner_elm_widget_focus_next(Eo *obj, Elm_Spinner_Data *_pd EINA_UNUSED, Elm_Focus_Direction dir, Evas_Object **next)
{
   Evas_Object *ao;

   Eina_List *items = NULL;
   const char* increment_part;
   const char* decrement_part;

   if (!strncmp(elm_widget_style_get(obj), "vertical", 8))
     {
        increment_part = "up_bt";
        decrement_part = "down_bt";
     }
   else
     {
        increment_part = "right_bt";
        decrement_part = "left_bt";
     }

   ao = _access_object_get(obj, "access.text");
   items = eina_list_append(items, ao);

   ao = _access_object_get(obj, decrement_part);
   items = eina_list_append(items, ao);

   ao = _access_object_get(obj, increment_part);
   items = eina_list_append(items, ao);

   return elm_widget_focus_list_next_get
            (obj, items, eina_list_data_get, dir, next);
}

EOLIAN static void
_elm_spinner_elm_widget_access(Eo *obj, Elm_Spinner_Data *_pd EINA_UNUSED, Eina_Bool acs)
{
   _elm_spinner_smart_focus_next_enable = acs;
   _access_spinner_register(obj, _elm_spinner_smart_focus_next_enable);
}

EAPI Evas_Object *
elm_spinner_add(Evas_Object *parent)
{
   EINA_SAFETY_ON_NULL_RETURN_VAL(parent, NULL);
   Evas_Object *obj = eo_add(MY_CLASS, parent);
   return obj;
}

EOLIAN static void
_elm_spinner_eo_base_constructor(Eo *obj, Elm_Spinner_Data *_pd EINA_UNUSED)
{
   eo_do_super(obj, MY_CLASS, eo_constructor());
   eo_do(obj,
         evas_obj_type_set(MY_CLASS_NAME_LEGACY),
         evas_obj_smart_callbacks_descriptions_set(_smart_callbacks),
         elm_interface_atspi_accessible_role_set(ELM_ATSPI_ROLE_SPIN_BUTTON));
}

EOLIAN static void
_elm_spinner_label_format_set(Eo *obj, Elm_Spinner_Data *sd, const char *fmt)
{
   eina_stringshare_replace(&sd->label, fmt);
   _label_write(obj);
   elm_layout_sizing_eval(obj);
   //TIZEN_ONLY(20150305): Added entry filter feature.
   _entry_filter_add(obj);
   //
}

EOLIAN static const char*
_elm_spinner_label_format_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd)
{
   return sd->label;
}

EOLIAN static void
_elm_spinner_min_max_set(Eo *obj, Elm_Spinner_Data *sd, double min, double max)
{
   if ((sd->val_min == min) && (sd->val_max == max)) return;
   sd->val_min = min;
   sd->val_max = max;
   if (sd->val < sd->val_min) sd->val = sd->val_min;
   if (sd->val > sd->val_max) sd->val = sd->val_max;
   _val_set(obj);
   _label_write(obj);
   //TIZEN_ONLY(20150305): Added entry filter feature.
   _entry_filter_add(obj);
   //
}

EOLIAN static void
_elm_spinner_min_max_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd, double *min, double *max)
{
   if (min) *min = sd->val_min;
   if (max) *max = sd->val_max;
}

EOLIAN static void
_elm_spinner_step_set(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd, double step)
{
   sd->step = step;
}

EOLIAN static double
_elm_spinner_step_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd)
{
   return sd->step;
}

EOLIAN static void
_elm_spinner_value_set(Eo *obj, Elm_Spinner_Data *sd, double val)
{
   if (sd->val == val) return;
   sd->val = val;
   sd->val_updated = EINA_FALSE;
   if (sd->val < sd->val_min)
     {
        sd->val = sd->val_min;
        sd->val_updated = EINA_TRUE;
     }
   if (sd->val > sd->val_max)
     {
        sd->val = sd->val_max;
        sd->val_updated = EINA_TRUE;
     }
   _val_set(obj);
   _label_write(obj);
   //TIZEN_ONLY(20150522): Changed smart callback call for value update.
   evas_object_smart_callback_call(obj, SIG_CHANGED, NULL);
   //
}

EOLIAN static double
_elm_spinner_value_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd)
{
   return sd->val;
}

EOLIAN static void
_elm_spinner_wrap_set(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd, Eina_Bool wrap)
{
   sd->wrap = wrap;
}

EOLIAN static Eina_Bool
_elm_spinner_wrap_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd)
{
   return sd->wrap;
}

EOLIAN static void
_elm_spinner_special_value_add(Eo *obj, Elm_Spinner_Data *sd, double value, const char *label)
{
   Elm_Spinner_Special_Value *sv;
   Eina_List *l;

   EINA_LIST_FOREACH(sd->special_values, l, sv)
     {
        if (sv->value != value)
          continue;

        eina_stringshare_replace(&sv->label, label);
        _label_write(obj);
        return;
     }

   sv = calloc(1, sizeof(*sv));
   if (!sv) return;
   sv->value = value;
   sv->label = eina_stringshare_add(label);

   sd->special_values = eina_list_append(sd->special_values, sv);
   _label_write(obj);
}

EAPI void
elm_spinner_special_value_del(Evas_Object *obj,
                              double value)
{
   Elm_Spinner_Special_Value *sv;
   Eina_List *l;

   ELM_SPINNER_CHECK(obj);
   ELM_SPINNER_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->special_values, l, sv)
     {
        if (sv->value != value)
          continue;

        sd->special_values = eina_list_remove_list(sd->special_values, l);
        eina_stringshare_del(sv->label);
        free(sv);
        _label_write(obj);
        return;
     }
}

EAPI const char *
elm_spinner_special_value_get(Evas_Object *obj,
                              double value)
{
   Elm_Spinner_Special_Value *sv;
   Eina_List *l;

   ELM_SPINNER_CHECK(obj) NULL;
   ELM_SPINNER_DATA_GET(obj, sd);

   EINA_LIST_FOREACH(sd->special_values, l, sv)
     {
        if (sv->value == value)
          return sv->label;
     }

   return NULL;
}

EOLIAN static void
_elm_spinner_editable_set(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd, Eina_Bool editable)
{
   sd->editable = editable;
   //TIZEN_ONLY(20150918): Emit signal to disable touch sound on editable = EINA_FALSE.
   if (!editable) elm_object_signal_emit(sd->text_button, "elm,state,non_editable", "elm");
   else elm_object_signal_emit(sd->text_button, "elm,state,editable", "elm");
   //
}

EOLIAN static Eina_Bool
_elm_spinner_editable_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd)
{
   return sd->editable;
}

EOLIAN static void
_elm_spinner_interval_set(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd, double interval)
{
   sd->first_interval = interval;
}

EOLIAN static double
_elm_spinner_interval_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd)
{
   return sd->first_interval;
}

EOLIAN static void
_elm_spinner_base_set(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd, double base)
{
   sd->val_base = base;
}

EOLIAN static double
_elm_spinner_base_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd)
{
   return sd->val_base;
}

EOLIAN static void
_elm_spinner_round_set(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd, int rnd)
{
   sd->round = rnd;
}

EOLIAN static int
_elm_spinner_round_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd)
{
   return sd->round;
}

EOLIAN static void
_elm_spinner_class_constructor(Eo_Class *klass)
{
   evas_smart_legacy_type_register(MY_CLASS_NAME_LEGACY, klass);

   if (_elm_config->access_mode)
      _elm_spinner_smart_focus_next_enable = EINA_TRUE;
}

// A11Y Accessibility

EOLIAN static void
_elm_spinner_elm_interface_atspi_value_value_and_text_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd, double *value, const char **text)
{
   if (value) *value = sd->val;
   if (text) *text = NULL;
}

EOLIAN static Eina_Bool
_elm_spinner_elm_interface_atspi_value_value_and_text_set(Eo *obj, Elm_Spinner_Data *sd, double value, const char *text EINA_UNUSED)
{
   if (sd->val_min > value) return EINA_FALSE;
   if (sd->val_max < value) return EINA_FALSE;

   sd->val = value;
   _val_set(obj);

   return EINA_TRUE;
}

EOLIAN static void
_elm_spinner_elm_interface_atspi_value_range_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd, double *lower, double *upper, const char **descr)
{
   if (lower) *lower = sd->val_min;
   if (upper) *upper = sd->val_max;
   if (descr) *descr = NULL;
}

EOLIAN static double
_elm_spinner_elm_interface_atspi_value_increment_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd)
{
   return sd->step;
}

EOLIAN static char*
_elm_spinner_elm_interface_atspi_accessible_name_get(Eo *obj, Elm_Spinner_Data *sd EINA_UNUSED)
{
   const char *ret = elm_layout_text_get(obj, "elm.text");
   return ret ? strdup(ret) : NULL;
}

EOLIAN static const Elm_Atspi_Action*
_elm_spinner_elm_interface_atspi_widget_action_elm_actions_get(Eo *obj EINA_UNUSED, Elm_Spinner_Data *sd EINA_UNUSED)
{
   static Elm_Atspi_Action atspi_actions[] = {
          { "spin,left", "spin", "left", _key_action_spin},
          { "spin,right", "spin", "right", _key_action_spin},
          { "spin,up", "spin", "up", _key_action_spin},
          { "spin,down", "spin", "down", _key_action_spin},
          { "toggle", "toggle", NULL, _key_action_toggle},
          { NULL, NULL, NULL, NULL }
   };
   return &atspi_actions[0];
}
// A11Y Accessibility - END

#include "elm_spinner.eo.c"
