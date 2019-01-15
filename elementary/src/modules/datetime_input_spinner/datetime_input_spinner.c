#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget_datetime.h"
#include <unicode/udat.h>
#include <unicode/ustring.h>

#define DATETIME_FIELD_COUNT            6
#define FIELD_FORMAT_LEN                3
#define BUFF_SIZE                       100
#define STRUCT_TM_YEAR_BASE_VALUE       1900
#define STRUCT_TM_MONTH_BASE_VALUE      1
#define STRUCT_TM_TIME_12HRS_MAX_VALUE  12
#define DATETIME_LOCALE_LENGTH_MAX 32

typedef struct _Input_Spinner_Module_Data Input_Spinner_Module_Data;

struct _Input_Spinner_Module_Data
{
   Elm_Datetime_Module_Data mod_data;
   Evas_Object *field_obj[DATETIME_FIELD_COUNT];
   Evas_Object *ampm_box;
   Evas_Object *am_button;
   Evas_Object *pm_button;
   Eina_Bool field_status[DATETIME_FIELD_COUNT];
   Eina_Bool time_12hr_fmt;
   Eina_Bool is_pm;
   Eina_Bool is_timepicker;
   Eina_Bool is_init;
   Eina_Bool is_month_changed;
   int pre_decimal_month;
   const char *pre_str_value;
   char user_locale[DATETIME_LOCALE_LENGTH_MAX];
};

static void _ampm_clicked_cb(void *data, Evas_Object *obj,
                             void *event_info EINA_UNUSED);

static void
_field_value_set(struct tm *tim, Elm_Datetime_Field_Type  field_type, int val)
{
   if (field_type >= DATETIME_FIELD_COUNT - 1) return;

   int *timearr[]= { &tim->tm_year, &tim->tm_mon, &tim->tm_mday, &tim->tm_hour, &tim->tm_min };
   *timearr[field_type] = val;
}

static void
_fields_visible_set(Input_Spinner_Module_Data *layout_mod)
{
   Evas_Object *datetime;

   if (!layout_mod) return;
   datetime = layout_mod->mod_data.base;

   if (layout_mod->is_timepicker)
     {
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_YEAR, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MONTH, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_DATE, EINA_FALSE);

        elm_datetime_field_visible_set(datetime, ELM_DATETIME_HOUR, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MINUTE, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_AMPM, EINA_TRUE);
     }
   else
     {
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_YEAR, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MONTH, EINA_TRUE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_DATE, EINA_TRUE);

        elm_datetime_field_visible_set(datetime, ELM_DATETIME_HOUR, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_MINUTE, EINA_FALSE);
        elm_datetime_field_visible_set(datetime, ELM_DATETIME_AMPM, EINA_FALSE);
     }
}

static Eina_List *
_fields_sorted_get(Evas_Object *obj)
{
   Eina_List *items = NULL;
   Datetime_Field *field;
   unsigned int idx;
   Datetime_Field *sorted_fields[ELM_DATETIME_TYPE_COUNT];

   ELM_DATETIME_DATA_GET(obj, sd);

   for (idx = 0; idx < ELM_DATETIME_TYPE_COUNT; idx++)
     {
        field = sd->field_list + idx;
        sorted_fields[field->location] = field;
     }

   for (idx = 0; idx < ELM_DATETIME_TYPE_COUNT; idx++)
     {
        field = sorted_fields[idx];
        if (field->fmt_exist && field->visible)
          items = eina_list_append(items, field->item_obj);
     }

   return items;
}

static void
_field_toggle_to_next_location(Input_Spinner_Module_Data *layout_mod, Evas_Object *spinner, Elm_Input_Panel_Return_Key_Type key_type)
{
   Evas_Object *spinner_next = NULL;
   Eina_List *fields, *l;

   fields = _fields_sorted_get(layout_mod->mod_data.base);
   l = eina_list_data_find_list(fields, spinner);
   if (l)
     {
        if (eina_list_next(l))
          spinner_next = eina_list_data_get(eina_list_next(l));
        else
          spinner_next =  eina_list_data_get(fields);
     }

   if (spinner_next)
     {
        if (key_type != ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT && key_type != ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE)
          {
             elm_layout_signal_emit(spinner, "elm,action,entry,toggle", "elm");
             edje_object_message_signal_process(elm_layout_edje_get(spinner));
          }
        elm_layout_signal_emit(spinner_next, "elm,action,entry,toggle", "elm");
     }
}

char *
_text_insert(const char *text, const char *input, int pos)
{
   char *result;
   int text_len, input_len;

   text_len = strlen(text);
   input_len = strlen(input);
   result = (char *)calloc(text_len + input_len + 1,  sizeof(char));

   strncpy(result, text, pos);
   strcpy(result + pos, input);
   strcpy(result + pos + input_len, text + pos);

   return result;
}

static void
_year_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   int min, max, val;
   const char *curr_str;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   curr_str = elm_object_text_get(obj);
   if (!curr_str || strlen(curr_str) < 3) return;

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_YEAR, &min, &max);

   min += STRUCT_TM_YEAR_BASE_VALUE;
   max += STRUCT_TM_YEAR_BASE_VALUE;

   if (((val >= min) && (val <= max)) || val == 0)
     elm_entry_cursor_end_set(obj);
   else
     *insert = 0;

   free(new_str);
}

static void
_month_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   const char *fmt;
   double min, max;
   int val, len, loc;
   const char *curr_str;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH);

   if (strcmp(fmt, "%m"))
     {
        elm_entry_entry_set(obj, ""); //clear entry first in case of abbreviated month name
        curr_str = elm_object_text_get(obj);
        if (!curr_str) return;
     }
   else
     {
        curr_str = elm_object_text_get(obj);
        if (!curr_str) return;

         if (!strcmp(*text, "1") || !strcmp(*text, "0"))
           len = strlen(curr_str);
         else
           len = strlen(*text);

         if (len < 1) return;
     }

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   elm_spinner_min_max_get(layout_mod->field_obj[ELM_DATETIME_MONTH], &min, &max);

   if (((val >= min) && (val <= max)) || val == 0)
     {
        layout_mod->is_month_changed = EINA_TRUE;

        elm_entry_entry_set(obj, new_str);
        elm_entry_cursor_end_set(obj);

        layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH, &loc);
        if (loc != 2)
          _field_toggle_to_next_location(layout_mod, layout_mod->field_obj[ELM_DATETIME_MONTH], -1);
     }

   *insert = 0;

   free(new_str);
}

static void
_date_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   int val;
   const char *curr_str;
   double min, max;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   curr_str = elm_object_text_get(obj);
   if (!curr_str || strlen(curr_str) < 1) return;

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   elm_spinner_min_max_get(layout_mod->field_obj[ELM_DATETIME_DATE], &min, &max);

   if (((val >= min) && (val <= max)) || val == 0)
     elm_entry_cursor_end_set(obj);
   else
     *insert = 0;

   free(new_str);
}

static void
_hour_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   int val, len;
   const char *curr_str;
   double min, max;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   curr_str = elm_object_text_get(obj);
   if (!curr_str) return;

   if ((layout_mod->time_12hr_fmt && (!strcmp(*text, "1") || !strcmp(*text, "0"))) ||
       (!layout_mod->time_12hr_fmt && (!strcmp(*text, "1") || !strcmp(*text, "2") || !strcmp(*text, "0"))))
     len = strlen(curr_str);
   else
     len = strlen(*text);

   if (len < 1) return;

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   if (layout_mod->time_12hr_fmt)
     {
        if (val > STRUCT_TM_TIME_12HRS_MAX_VALUE)
          {
             *insert = 0;
             free(new_str);
             return;
          }
        else if (val == 0)
          sprintf(new_str, "%d", STRUCT_TM_TIME_12HRS_MAX_VALUE);
     }

   elm_spinner_min_max_get(layout_mod->field_obj[ELM_DATETIME_HOUR], &min, &max);

   if ((val >= min) && (val <= max))
     {
        elm_entry_entry_set(obj, new_str);
        elm_entry_cursor_end_set(obj);

        _field_toggle_to_next_location(layout_mod, layout_mod->field_obj[ELM_DATETIME_HOUR], -1);
     }

   *insert = 0;

    free(new_str);
}

static void
_minute_validity_checking_filter(void *data, Evas_Object *obj, char **text)
{
   Input_Spinner_Module_Data *layout_mod;
   char *new_str, *insert;
   int val;
   double min, max;
   const char *curr_str;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (!(*text) || !strcmp(*text, "")) return;

   curr_str = elm_object_text_get(obj);
   if (!curr_str || strlen(curr_str) < 1) return;

   insert = *text;
   new_str = _text_insert(curr_str, insert, elm_entry_cursor_pos_get(obj));
   if (!new_str) return;

   val = atoi(new_str);

   elm_spinner_min_max_get(layout_mod->field_obj[ELM_DATETIME_MINUTE], &min, &max);

   if ((val >= min) && (val <= max))
     elm_entry_cursor_end_set(obj);
   else
     *insert = 0;

   free(new_str);
}

static void
_widget_theme_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *widget;
   const char *style;
   int idx, loc;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   widget = layout_mod->mod_data.base;
   style = elm_object_style_get(widget);

   if (!strcmp(style, "time_layout") || !strcmp(style, "time_layout_24hr"))
     {
        layout_mod->is_timepicker = EINA_TRUE;

        for (idx = ELM_DATETIME_HOUR; idx < ELM_DATETIME_AMPM; idx++)
          elm_object_style_set(layout_mod->field_obj[idx], "vertical");

        if (!strcmp(style, "time_layout_24hr"))
          layout_mod->time_12hr_fmt = EINA_FALSE;
        else
          {
             layout_mod->time_12hr_fmt = EINA_TRUE;
             if (!layout_mod->am_button)
               {
                  layout_mod->am_button = elm_button_add(layout_mod->ampm_box);
                  elm_object_style_set(layout_mod->am_button, "datetime/ampm");
                  evas_object_smart_callback_add(layout_mod->am_button, "clicked", _ampm_clicked_cb, layout_mod);
                  evas_object_show(layout_mod->am_button);
                  elm_box_pack_end(layout_mod->ampm_box, layout_mod->am_button);
               }

             if (!layout_mod->pm_button)
               {
                  layout_mod->pm_button = elm_button_add(layout_mod->ampm_box);
                  elm_object_style_set(layout_mod->pm_button, "datetime/ampm");
                  evas_object_smart_callback_add(layout_mod->pm_button, "clicked", _ampm_clicked_cb, layout_mod);
                  evas_object_show(layout_mod->pm_button);
                  elm_box_pack_end(layout_mod->ampm_box, layout_mod->pm_button);
               }
          }

        layout_mod->mod_data.field_location_get(widget, ELM_DATETIME_AMPM, &loc);
        if (loc == 5)
          {
             elm_object_signal_emit(widget, "elm,state,colon,visible,field3", "elm");
             elm_object_signal_emit(widget, "elm,state,colon,invisible,field4", "elm");
          }
        else
          {
             elm_object_signal_emit(widget, "elm,state,colon,visible,field4", "elm");
             elm_object_signal_emit(widget, "elm,state,colon,invisible,field3", "elm");
          }

        edje_object_message_signal_process(elm_layout_edje_get(widget));
     }
   else
     {
       layout_mod->is_timepicker = EINA_FALSE;

       for (idx = ELM_DATETIME_YEAR; idx <= ELM_DATETIME_DATE; idx++)
          elm_object_style_set(layout_mod->field_obj[idx], "vertical_date_picker");
     }

   _fields_visible_set(layout_mod);
}

static void
_spinner_changed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *entry;
   struct tm tim;
   int value;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;
   if (!layout_mod->is_init) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &tim);
   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");
   value =  elm_spinner_value_get(obj);

   if (field_type == ELM_DATETIME_YEAR)
     value -= 1900;
   else if (field_type == ELM_DATETIME_MONTH)
     value -= 1;
   else if (field_type == ELM_DATETIME_HOUR && layout_mod->time_12hr_fmt)
     {
        entry = elm_object_part_content_get(obj, "elm.swallow.entry");
        if (elm_widget_focus_get(entry))
          {
             if (layout_mod->is_pm && value >= 0 && value < STRUCT_TM_TIME_12HRS_MAX_VALUE)
               value += STRUCT_TM_TIME_12HRS_MAX_VALUE;
             else if  (!layout_mod->is_pm && value == STRUCT_TM_TIME_12HRS_MAX_VALUE)
               value = 0;
          }
     }

   _field_value_set(&tim, field_type, value);

   elm_datetime_value_set(layout_mod->mod_data.base, &tim);
}

static void
_spinner_entry_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
   Input_Spinner_Module_Data *layout_mod;
   char *str;
   int len, max_len, loc;
   double min, max;
   Elm_Datetime_Field_Type  field_type;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   str = event_info;
   len = strlen(str);
   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   if (field_type == ELM_DATETIME_MONTH || field_type == ELM_DATETIME_HOUR) return;
   elm_spinner_min_max_get(obj, &min, &max);
   max_len = log10(abs(max)) + 1;

   if (max_len == len)
     {
        if (!layout_mod->is_timepicker)
          {
             layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, field_type, &loc);
             if (loc != 2)
               _field_toggle_to_next_location(layout_mod, obj, -1);
          }
     }
}

static void
_spinner_entry_apply_cb(void *data, Evas_Object *obj, void *event_info)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *entry;
   double *value;
   const char *curr_str, *fmt;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   value = event_info;
   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   entry = elm_object_part_content_get(obj, "elm.swallow.entry");
   curr_str = elm_object_text_get(entry);
   if (!curr_str || strlen(curr_str) < 1)
     {
        if (field_type == ELM_DATETIME_MONTH)
          *value = layout_mod->pre_decimal_month;
        else
          *value = atoi(layout_mod->pre_str_value);
     }
   else if (field_type == ELM_DATETIME_MONTH && !layout_mod->is_month_changed)
     {
        fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH);
        if (strcmp(fmt, "%m"))
          *value = layout_mod->pre_decimal_month;
     }
}

static void
_entry_focused_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *entry;
   int value;
   const char *str_value;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (layout_mod->is_timepicker)
     {
        entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_HOUR],
                                       "elm.swallow.entry");
        if (obj == entry)
          {
             value = (int)elm_spinner_value_get(layout_mod->field_obj[ELM_DATETIME_HOUR]);
             str_value = elm_spinner_special_value_get(layout_mod->field_obj[ELM_DATETIME_HOUR], value);
             if (str_value)
               elm_object_text_set(obj, str_value);
          }
     }
   else
     {
        entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_MONTH],
                                       "elm.swallow.entry");
        if (obj == entry)
          {
             layout_mod->is_month_changed = EINA_FALSE;

             value = (int)elm_spinner_value_get(layout_mod->field_obj[ELM_DATETIME_MONTH]);
             str_value = elm_spinner_special_value_get(layout_mod->field_obj[ELM_DATETIME_MONTH], value);
             if (str_value)
               elm_object_text_set(obj, str_value);

             layout_mod->pre_decimal_month = value;
          }
     }
   layout_mod->pre_str_value = elm_object_text_get(obj);
}

static void
_entry_unfocused_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *entry;
   int value;
   char buf[BUFF_SIZE];
   const char *fmt;
   const int hour12_min_value = 1;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   if (layout_mod->is_timepicker)
     {
        entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_HOUR],
                                         "elm.swallow.entry");
        // Below code is to prevent automatic toggling of am/pm.
        if (obj == entry && layout_mod->time_12hr_fmt)
          {
             value = atoi(elm_object_text_get(obj));
             if (value != hour12_min_value)
               {
                  value = (int)elm_spinner_value_get(layout_mod->field_obj[ELM_DATETIME_HOUR]);
                  snprintf(buf, sizeof(buf), "%d", value);
                  elm_object_text_set(obj, buf);
               }
             else
               {
                  // Special handling when the hour in entry is 1
                  if (layout_mod->is_pm)
                    {
                       value += STRUCT_TM_TIME_12HRS_MAX_VALUE;
                       snprintf(buf, sizeof(buf), "%d", value);
                       elm_object_text_set(obj, buf);
                    }
               }
          }
     }
   else
     {
        entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_MONTH],
                                       "elm.swallow.entry");
        if (obj == entry && layout_mod->is_month_changed)
          {
             fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH);
             if (strcmp(fmt, "%m"))
               {
                  value = (int)elm_spinner_value_get(layout_mod->field_obj[ELM_DATETIME_MONTH]);
                  snprintf(buf, sizeof(buf), "%d", value);
                  elm_object_text_set(obj, buf);
               }
          }
     }
}

static void
_entry_activated_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *entry;
   int idx;
   Elm_Input_Panel_Return_Key_Type key_type;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   key_type = elm_entry_input_panel_return_key_type_get(obj);
   if (layout_mod->is_timepicker)
     {
        entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_HOUR], "elm.swallow.entry");
        if (obj == entry)
          _field_toggle_to_next_location(layout_mod, layout_mod->field_obj[ELM_DATETIME_HOUR], key_type);
     }
   else
     {
        for (idx = 0; idx <= ELM_DATETIME_DATE; idx++)
         {
            entry = elm_object_part_content_get(layout_mod->field_obj[idx], "elm.swallow.entry");
            if (obj == entry)
              {
                 if (key_type == ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT)
                   _field_toggle_to_next_location(layout_mod, layout_mod->field_obj[idx], key_type);

                 return;
              }
         }
     }
}

static void
_spinner_entry_show_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *entry;
   static Elm_Entry_Filter_Accept_Set digits_filter_data;
   static Elm_Entry_Filter_Limit_Size limit_filter_data;
   int loc;
   const char *fmt;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   entry = elm_object_part_content_get(obj, "elm.swallow.entry");
   if (!entry) return;

   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   /*Please note that only date time format is supported not time date as per config.*/
   if (!layout_mod->field_status[field_type])
     {
        evas_object_smart_callback_add(entry, "activated", _entry_activated_cb, layout_mod);
        evas_object_smart_callback_add(entry, "focused", _entry_focused_cb, layout_mod);
        evas_object_smart_callback_add(entry, "unfocused", _entry_unfocused_cb, layout_mod);

        switch (field_type)
          {
             case ELM_DATETIME_YEAR:
               elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
               elm_entry_markup_filter_append(entry, _year_validity_checking_filter, layout_mod);
               break;

             case ELM_DATETIME_MONTH:
               digits_filter_data.accepted = "0123456789";
               digits_filter_data.rejected = NULL;
               limit_filter_data.max_char_count = 2; //Month's string(3) + (2)(max digits)
               limit_filter_data.max_byte_count = 0;
               elm_entry_markup_filter_prepend(entry, elm_entry_filter_accept_set, &digits_filter_data);
               elm_entry_markup_filter_prepend(entry, elm_entry_filter_limit_size, &limit_filter_data);
               elm_entry_markup_filter_prepend(entry, _month_validity_checking_filter, layout_mod);
               break;

             case ELM_DATETIME_DATE:
               elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
               elm_entry_markup_filter_append(entry, _date_validity_checking_filter, layout_mod);
               break;

             case ELM_DATETIME_HOUR:
               elm_entry_markup_filter_append(entry, _hour_validity_checking_filter, layout_mod);
               elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
               elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
               break;

             case ELM_DATETIME_MINUTE:
               elm_entry_markup_filter_append(entry, _minute_validity_checking_filter, layout_mod);
               elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
               elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
               break;

             default:
               printf("the default case is not allowed\n");
               break;
          }

        layout_mod->field_status[field_type] = EINA_TRUE;
     }

   if (layout_mod->is_timepicker) return;

   layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, field_type, &loc);

   switch (field_type)
     {
        case ELM_DATETIME_YEAR:
           if (loc == 2)
             elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
           else
             elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
           break;

         case ELM_DATETIME_MONTH:
           fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, field_type);
           if (strcmp(fmt, "%m"))
             elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_MONTH);
           else
             {
                elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);

                if (loc == 2)
                  elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
                else
                  elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
             }
           break;

         case ELM_DATETIME_DATE:
           if (loc == 2)
             elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
           else
             elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
           break;

         default:
           break;
      }
}

static void
_ampm_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Spinner_Module_Data *layout_mod;
   struct tm curr_time;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);

   if (curr_time.tm_hour >= 12 && obj == layout_mod->am_button)
     curr_time.tm_hour -= 12;
   else if (curr_time.tm_hour < 12 && obj == layout_mod->pm_button)
     curr_time.tm_hour += 12;
   else
     return;

   elm_datetime_value_set(layout_mod->mod_data.base, &curr_time);
}

static void
_access_set(Evas_Object *ob EINA_UNUSED, Elm_Datetime_Field_Type field_type EINA_UNUSED)
{
/*
   char *type = NULL;

   switch (field_type)
     {
        case ELM_DATETIME_YEAR:
          type = "datetime field, year";
          break;

        case ELM_DATETIME_MONTH:
          type = "datetime field, month";
          break;

        case ELM_DATETIME_DATE:
          type = "datetime field, date";
          break;

        case ELM_DATETIME_HOUR:
          type = "datetime field, hour";
          break;

        case ELM_DATETIME_MINUTE:
          type = "datetime field, minute";
          break;

        case ELM_DATETIME_AMPM:
          type = "datetime field, AM/PM";
          break;

        default:
          break;
     }

   _elm_access_text_set(_elm_access_info_get(obj), ELM_ACCESS_TYPE, type);
   _elm_access_callback_set(_elm_access_info_get(obj), ELM_ACCESS_STATE, NULL, NULL);

*/
}

static void
_special_value_add(Input_Spinner_Module_Data *layout_mod, Evas_Object *obj)
{
   Elm_Datetime_Field_Type field_type;
   struct tm curr_time;
   const char *fmt;
   char *locale_tmp, *p;
   char buf[BUFF_SIZE];
   char locale[BUFF_SIZE];
   int val;
   UDate date;
   UDateFormat *dt_formatter;
   UErrorCode status = U_ZERO_ERROR;
   UChar result[BUFF_SIZE];
   UChar pattern[BUFF_SIZE];
   UChar ufield[BUFF_SIZE];
   UCalendar *calendar;
   int32_t pos = 0;

   if (!layout_mod || !obj) return;

   if (strlen(layout_mod->user_locale) == 0)
     locale_tmp = setlocale(LC_TIME, NULL);
   else
     locale_tmp = layout_mod->user_locale;

   snprintf(locale, sizeof(locale), "%s", locale_tmp);
   if (locale[0] != '\0')
     {
        p = strchr(locale, '.');
        if (p) *p = 0;
     }

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);
   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");
   fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, field_type);

   switch (field_type)
     {
        case ELM_DATETIME_YEAR:
          if (!strcmp(fmt, "%y"))
            u_uastrcpy(pattern, "yy");
          else
            u_uastrcpy(pattern, "yyyy");

          val = STRUCT_TM_YEAR_BASE_VALUE + curr_time.tm_year;
          break;

        case ELM_DATETIME_MONTH:
          if (!strcmp(fmt, "%m"))
            u_uastrcpy(pattern, "MM");
          else if (!strcmp(fmt, "%B"))
            u_uastrcpy(pattern, "MMMM");
          else
            u_uastrcpy(pattern, "MMM");

          val = STRUCT_TM_MONTH_BASE_VALUE + curr_time.tm_mon;
          break;

        case ELM_DATETIME_DATE:
          if (!strcmp(fmt, "%e"))
            u_uastrcpy(pattern, "d");
          else
            u_uastrcpy(pattern, "dd");

          val = curr_time.tm_mday;
          break;

        case ELM_DATETIME_HOUR:
          if (!strcmp(fmt, "%l"))
            {
               if (!strcmp(locale,"ja_JP"))
                 u_uastrcpy(pattern, "K");
               else
                 u_uastrcpy(pattern, "h");
            }
          else if (!strcmp(fmt, "%I"))
            {
               if (!strcmp(locale,"ja_JP"))
                 u_uastrcpy(pattern, "KK");
               else
                 u_uastrcpy(pattern, "hh");
            }
          else if (!strcmp(fmt, "%k"))
            u_uastrcpy(pattern, "H");
          else
            u_uastrcpy(pattern, "HH");

          val = curr_time.tm_hour;
          break;

        case ELM_DATETIME_MINUTE:
          u_uastrcpy(pattern, "mm");

          val = curr_time.tm_min;
          break;

        default:
          printf("the default case is not allowed\n");
          return;
     }

   snprintf(buf, sizeof(buf), "%d", val);
   u_uastrcpy(ufield, buf);

   calendar = ucal_open(NULL, -1, locale, UCAL_GREGORIAN, &status);
   ucal_clear(calendar);

   dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, pattern, -1, &status);
   udat_parseCalendar(dt_formatter, calendar, ufield, sizeof(ufield), &pos, &status);
   date = ucal_getMillis(calendar, &status);
   ucal_close(calendar);

   udat_format(dt_formatter, date, result, sizeof(result), NULL, &status);
   udat_close(dt_formatter);
   u_austrcpy(buf, result);

   elm_spinner_special_value_add(layout_mod->field_obj[field_type], val, buf);
}

static void
_ampm_text_set(Input_Spinner_Module_Data *layout_mod)
{
   struct tm curr_time;
   const char *fmt;
   char locale[32] = {0,};
   char *locale_tmp, *p;
   char buf[BUFF_SIZE];
   int idx;
   UDate date;
   UDateFormat *dt_formatter;
   UErrorCode status = U_ZERO_ERROR;
   UChar result[BUFF_SIZE];
   UChar pattern[BUFF_SIZE];
   UCalendar *calendar;

   if (!layout_mod) return;

   if (strlen(layout_mod->user_locale) == 0)
     locale_tmp = setlocale(LC_TIME, NULL);
   else
     locale_tmp = layout_mod->user_locale;

   snprintf(locale, sizeof(locale), "%s", locale_tmp);
   if (locale[0] != '\0')
     {
        p = strchr(locale, '.');
        if (p) *p = 0;
     }

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);
   fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, ELM_DATETIME_AMPM);

   calendar = ucal_open(NULL, -1, locale, UCAL_GREGORIAN, &status);
   ucal_clear(calendar);
   ucal_setDateTime(calendar, STRUCT_TM_YEAR_BASE_VALUE + curr_time.tm_year, UCAL_JANUARY + curr_time.tm_mon,
      curr_time.tm_mday, 0, curr_time.tm_min, 0, &status);
   date = ucal_getMillis(calendar, &status);
   ucal_close(calendar);

   u_uastrcpy(pattern, "a");
   dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, pattern, -1, &status);
   udat_format(dt_formatter, date, result, sizeof(result), NULL, &status);
   u_austrcpy(buf, result);

   if (u_strlen(result) >= 4 && strcmp(buf, "a.m.")) strcpy(buf, "AM");
   if (!strcmp(fmt, "%P"))
     for (idx = 0; idx < strlen(buf); idx++)
       buf[idx] = tolower(buf[idx]);
   elm_object_text_set(layout_mod->am_button, buf);

   date += STRUCT_TM_TIME_12HRS_MAX_VALUE * 60 * 60 * 1000;
   udat_format(dt_formatter, date, result, sizeof(result), NULL, &status);
   u_austrcpy(buf, result);
   udat_close(dt_formatter);

   if (u_strlen(result) >= 4 && strcmp(buf, "p.m.")) strcpy(buf, "PM");
   if (!strcmp(fmt, "%P"))
     for (idx = 0; idx < strlen(buf); idx++)
       buf[idx] = tolower(buf[idx]);
   elm_object_text_set(layout_mod->pm_button, buf);
}

EAPI void
field_value_display(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type field_type;
   struct tm curr_time;
   int min, max;
   const char *fmt;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod || !obj) return;

   elm_datetime_value_get(layout_mod->mod_data.base, &curr_time);
   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   switch (field_type)
     {
        case ELM_DATETIME_YEAR:
          if (layout_mod->is_timepicker) return;

          layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_YEAR, &min, &max);
          elm_spinner_min_max_set(obj, STRUCT_TM_YEAR_BASE_VALUE + min, STRUCT_TM_YEAR_BASE_VALUE + max);
          _special_value_add(layout_mod, obj);
          elm_spinner_value_set(obj, STRUCT_TM_YEAR_BASE_VALUE + curr_time.tm_year);
          break;

        case ELM_DATETIME_MONTH:
          if (layout_mod->is_timepicker) return;

          layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_MONTH, &min, &max);
          elm_spinner_min_max_set(obj, STRUCT_TM_MONTH_BASE_VALUE + min, STRUCT_TM_MONTH_BASE_VALUE + max);
          _special_value_add(layout_mod, obj);
          elm_spinner_value_set(obj, STRUCT_TM_MONTH_BASE_VALUE + curr_time.tm_mon);
          break;

        case ELM_DATETIME_DATE:
          if (layout_mod->is_timepicker) return;

          layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_DATE, &min, &max);
          elm_spinner_min_max_set(obj, min, max);
          _special_value_add(layout_mod, obj);
          elm_spinner_value_set(obj, curr_time.tm_mday);
          break;

        case ELM_DATETIME_HOUR:
          if (!layout_mod->is_timepicker) return;

          layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_HOUR, &min, &max);
          elm_spinner_min_max_set(obj, min, max);
          _special_value_add(layout_mod, obj);
          elm_spinner_value_set(obj, curr_time.tm_hour);
          break;

        case ELM_DATETIME_MINUTE:
          if (!layout_mod->is_timepicker) return;

          layout_mod->mod_data.field_limit_get(layout_mod->mod_data.base, ELM_DATETIME_MINUTE, &min, &max);
          elm_spinner_min_max_set(obj, min, max);
          _special_value_add(layout_mod, obj);
          elm_spinner_value_set(obj, curr_time.tm_min);
          break;

        case ELM_DATETIME_AMPM:
          if (!layout_mod->is_timepicker) return;

          if ((curr_time.tm_hour >= 0) && (curr_time.tm_hour < 12))
            layout_mod->is_pm = EINA_FALSE;
          else
            layout_mod->is_pm = EINA_TRUE;

          if (layout_mod->is_pm)
            {
               elm_layout_signal_emit(layout_mod->am_button, "elm,action,button,unselected", "elm");
               elm_layout_signal_emit(layout_mod->pm_button, "elm,action,button,selected", "elm");
            }
          else
            {
               elm_layout_signal_emit(layout_mod->am_button, "elm,action,button,selected", "elm");
               elm_layout_signal_emit(layout_mod->pm_button, "elm,action,button,unselected", "elm");
            }

          _ampm_text_set(layout_mod);
          break;

        default:
          printf("the default case is not allowed\n");
          break;
     }
}

static void
_locale_changed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info)
{
   Input_Spinner_Module_Data *layout_mod;
   const char *locale;
   int idx;
   Eina_Bool ret;

   layout_mod = (Input_Spinner_Module_Data *)data;
   if (!layout_mod) return;

   locale = event_info;
   strcpy(layout_mod->user_locale, locale);

   for (idx = 0; idx < ELM_DATETIME_TYPE_COUNT; idx++)
     {
        ret = layout_mod->mod_data.field_location_get(layout_mod->mod_data.base, idx, NULL);
        if (ret)
          field_value_display((Elm_Datetime_Module_Data*)layout_mod, layout_mod->field_obj[idx]);
     }
}

EAPI Evas_Object *
field_create(Elm_Datetime_Module_Data *module_data, Elm_Datetime_Field_Type  field_type)
{
   Input_Spinner_Module_Data *layout_mod;
   Evas_Object *field_obj;
   Evas_Object *parent = NULL;
   char *parent_style = NULL;
   char *time_picker_style = "vertical_time_picker";
   char *date_picker_style = "vertical_date_picker";

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod) return NULL;

   if (field_type == ELM_DATETIME_AMPM)
     {
        field_obj = elm_box_add(layout_mod->mod_data.base);
        evas_object_size_hint_weight_set(field_obj, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_size_hint_align_set(field_obj, EVAS_HINT_FILL, EVAS_HINT_FILL);
        elm_box_padding_set(field_obj, 0, ELM_SCALE_SIZE(26));
        layout_mod->ampm_box = field_obj;
     }
   else
     {
       parent = elm_object_parent_widget_get(layout_mod->mod_data.base);
       parent_style = evas_object_data_get(parent, "UXT_DATE_TIME_STYLE_ADD");

       if (parent_style)
         {
            if (!strcmp(parent_style, "time_layout") || !strcmp(parent_style, "time_layout_24hr"))
              evas_object_data_set(layout_mod->mod_data.base, "ELM_SPINNER", (void *)time_picker_style);
            else if (!strcmp(parent_style, "date_layout"))
              evas_object_data_set(layout_mod->mod_data.base, "ELM_SPINNER", (void *)date_picker_style);
         }

       field_obj = elm_spinner_add(layout_mod->mod_data.base);
       elm_spinner_editable_set(field_obj, EINA_TRUE);

       if (!parent_style)
         {
            if (field_type < ELM_DATETIME_HOUR)
              elm_object_style_set(field_obj, "vertical_date_picker");
         }

       elm_spinner_step_set(field_obj, 1);
       if (field_type == ELM_DATETIME_YEAR)
         elm_spinner_wrap_set(field_obj, EINA_FALSE);
       else
         elm_spinner_wrap_set(field_obj, EINA_TRUE);
       if (field_type == ELM_DATETIME_HOUR || field_type == ELM_DATETIME_MONTH)
         elm_spinner_interval_set(field_obj, 0.2);
       else
         elm_spinner_interval_set(field_obj, 0.1);

       evas_object_smart_callback_add(field_obj, "changed", _spinner_changed_cb, layout_mod);
       evas_object_smart_callback_add(field_obj, "entry,changed", _spinner_entry_changed_cb, layout_mod);
       evas_object_smart_callback_add(field_obj, "entry,show", _spinner_entry_show_cb, layout_mod);
       evas_object_smart_callback_add(field_obj, "entry,apply", _spinner_entry_apply_cb, layout_mod);
     }

   evas_object_data_set(field_obj, "_field_type", (void *)field_type);
   layout_mod->field_obj[field_type] = field_obj;

   // ACCESS
   _access_set(field_obj, field_type);

   return field_obj;
}

EAPI void
field_format_changed(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Spinner_Module_Data *layout_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *datetime;
   Evas_Object *entry;
   int loc;
   char buf[BUFF_SIZE] = {0};
   const char *fmt;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod || !obj) return;

   if (!layout_mod->is_init) layout_mod->is_init = EINA_TRUE;

   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");
   datetime = layout_mod->mod_data.base;

   switch (field_type)
     {
        case ELM_DATETIME_MONTH:
          layout_mod->mod_data.field_location_get(datetime, ELM_DATETIME_AMPM, &loc);
          snprintf(buf, sizeof(buf), "field%d", loc);

          entry = elm_object_part_content_get(layout_mod->field_obj[ELM_DATETIME_MONTH], buf);
          if (entry)
            {
               fmt = layout_mod->mod_data.field_format_get(layout_mod->mod_data.base, field_type);
               if (strcmp(fmt, "%m"))
                 elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_MONTH);
               else
                 {
                    elm_entry_input_panel_layout_set(entry, ELM_INPUT_PANEL_LAYOUT_DATETIME);
                    if (loc == 2)
                      elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_DONE);
                    else
                      elm_entry_input_panel_return_key_type_set(entry, ELM_INPUT_PANEL_RETURN_KEY_TYPE_NEXT);
                 }
            }
          break;

        case ELM_DATETIME_AMPM:
          layout_mod->mod_data.field_location_get(datetime, ELM_DATETIME_AMPM, &loc);
          if (loc == 5)
            {
               elm_object_signal_emit(datetime, "elm,state,colon,visible,field3", "elm");
               elm_object_signal_emit(datetime, "elm,state,colon,invisible,field4", "elm");
            }
          else
            {
               elm_object_signal_emit(datetime, "elm,state,colon,visible,field4", "elm");
               elm_object_signal_emit(datetime, "elm,state,colon,invisible,field3", "elm");
            }

          edje_object_message_signal_process(elm_layout_edje_get(datetime));
          break;

        default:
          break;
     }
}

EAPI Elm_Datetime_Module_Data *
obj_hook(Evas_Object *obj)
{
   Input_Spinner_Module_Data *layout_mod;
   int idx;

   layout_mod = ELM_NEW(Input_Spinner_Module_Data);
   if (!layout_mod) return NULL;

   layout_mod->am_button = NULL;
   layout_mod->pm_button = NULL;
   layout_mod->ampm_box = NULL;

   for (idx = 0; idx < DATETIME_FIELD_COUNT; idx++)
     layout_mod->field_status[idx] = EINA_FALSE;

   layout_mod->time_12hr_fmt = EINA_FALSE;
   layout_mod->is_pm = EINA_FALSE;
   layout_mod->is_timepicker = EINA_FALSE;
   layout_mod->is_init = EINA_FALSE;
   layout_mod->is_month_changed = EINA_FALSE;
   layout_mod->pre_decimal_month = 0;
   layout_mod->pre_str_value = NULL;
   memset(layout_mod->user_locale, 0, DATETIME_LOCALE_LENGTH_MAX);

   evas_object_smart_callback_add(obj, "theme,changed", _widget_theme_changed_cb, layout_mod);
   evas_object_smart_callback_add(obj, "locale,changed", _locale_changed_cb, layout_mod);

   return (Elm_Datetime_Module_Data*)layout_mod;
}

EAPI void
obj_unhook(Elm_Datetime_Module_Data *module_data)
{
   Input_Spinner_Module_Data *layout_mod;

   layout_mod = (Input_Spinner_Module_Data *)module_data;
   if (!layout_mod) return;

   if (layout_mod)
     {
        free(layout_mod);
        layout_mod = NULL;
     }
}

EAPI void
obj_hide(Elm_Datetime_Module_Data *module_data EINA_UNUSED)
{
   return;
}

// module api funcs needed
EAPI int
elm_modapi_init(void *m EINA_UNUSED)
{
   return 1; // succeed always
}

EAPI int
elm_modapi_shutdown(void *m EINA_UNUSED)
{
   return 1; // succeed always
}
