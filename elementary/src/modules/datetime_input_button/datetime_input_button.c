#ifdef HAVE_CONFIG_H
#include "elementary_config.h"
#endif

#include <Elementary.h>
#include "elm_priv.h"
#include "elm_widget.h"
#include "elm_widget_datetime.h"
#include <unicode/udat.h>
#include <unicode/ustring.h>
#include <unicode/ucnv.h>

#define BUFF_SIZE                       256
#define ROTATE_TIME_NORMAL              0.2

#define STRUCT_TM_TIME_12HRS_MAX_VALUE  12
#define STRUCT_TM_YEAR_BASE_VALUE       1900

#define DATETIME_FIELD_COUNT            6

typedef struct _Input_Button_Module_Data Input_Button_Module_Data;

struct _Input_Button_Module_Data
{
   Elm_Datetime_Module_Data mod_data;
   Evas_Object *radio_group;
   Ecore_Timer *longpress_timer;
   Ecore_Timer *spin_timer;

   Evas_Object *top_button[DATETIME_FIELD_COUNT];
   Evas_Object *foot_button[DATETIME_FIELD_COUNT];
};

static void
_AM_PM_string_set(Evas_Object *field, Eina_Bool is_am)
{
   if (is_am)
     elm_object_domain_translatable_text_set(field, PACKAGE, E_("AM"));
   else
     elm_object_domain_translatable_text_set(field, PACKAGE, E_("PM"));
}

EAPI void
field_value_display(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Button_Module_Data *button_mod;
   Elm_Datetime_Field_Type field_type;
   struct tm tim;
   char buf[BUFF_SIZE] = {0};
   const char *fmt;
   char strbuf[BUFF_SIZE];
   char *locale_tmp;
   char *p = NULL;
   char locale[BUFF_SIZE] = {0,};

   UDateFormat *dt_formatter = NULL;
   UErrorCode status = U_ZERO_ERROR;
   UChar str[BUFF_SIZE];
   UChar Pattern[BUFF_SIZE] = {0, };
   UChar Ufield[BUFF_SIZE] = {0, };
   UDate date;
   UCalendar *calendar;
   int32_t pos;

   button_mod = (Input_Button_Module_Data *)module_data;
   if (!button_mod || !obj) return;

   elm_datetime_value_get(button_mod->mod_data.base, &tim);

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type");
   fmt = button_mod->mod_data.field_format_get(button_mod->mod_data.base, field_type);

   locale_tmp = setlocale(LC_ALL, NULL);
   strcpy(locale, locale_tmp);

   if (locale[0] != '\0')
     {
        p = strstr(locale, ".UTF-8");
        if (p) *p = 0;
     }

   switch (field_type)
     {
      case ELM_DATETIME_YEAR:
        if (!strcmp(fmt, "%y"))
          u_uastrncpy(Pattern, "yy", strlen("yy"));
        else if (!strcmp(fmt, "%Y"))
          u_uastrncpy(Pattern, "yyyy", strlen("yyyy"));
        else
          return;

        snprintf(strbuf, sizeof(strbuf), "%d", STRUCT_TM_YEAR_BASE_VALUE + tim.tm_year);
        u_uastrncpy(Ufield, strbuf, strlen(strbuf));
        break;

      case ELM_DATETIME_MONTH:
        if (!strcmp(fmt, "%m"))
          u_uastrncpy(Pattern, "MM", strlen("MM"));
        else if (!strcmp(fmt, "%B"))
          u_uastrncpy(Pattern, "MMMM", strlen("MMMM"));
        else if (!strcmp(fmt, "%b") || !strcmp(fmt, "%h"))
          u_uastrncpy(Pattern, "MMM", strlen("MMM"));
        else
          return;

        snprintf(strbuf, sizeof(strbuf), "%d", 1 + tim.tm_mon);
        u_uastrncpy(Ufield, strbuf, strlen(strbuf));
        break;

      case ELM_DATETIME_DATE:
        if (!strcmp(fmt, "%e"))
          u_uastrncpy(Pattern, "d", strlen("d"));
        else if (!strcmp(fmt, "%d"))
          u_uastrncpy(Pattern, "dd", strlen("dd"));
        else
          return;

        snprintf(strbuf, sizeof(strbuf), "%d", tim.tm_mday);
        u_uastrncpy(Ufield, strbuf, strlen(strbuf));
        break;

      case ELM_DATETIME_HOUR:
        if (!strcmp(fmt, "%l"))
          {
             if (!strcmp(locale,"ja_JP"))
               u_uastrncpy(Pattern, "K", strlen("K"));
             else
               u_uastrncpy(Pattern, "h", strlen("h"));
          }
        else if (!strcmp(fmt, "%I"))
          {
             if (!strcmp(locale,"ja_JP"))
               u_uastrncpy(Pattern, "KK", strlen("KK"));
             else
               u_uastrncpy(Pattern, "hh", strlen("hh"));
          }
        else if (!strcmp(fmt, "%k"))
          u_uastrncpy(Pattern, "H", strlen("H"));
        else if (!strcmp(fmt, "%H"))
          u_uastrncpy(Pattern, "HH", strlen("HH"));
        else
          return;

        snprintf(strbuf, sizeof(strbuf), "%d", tim.tm_hour);
        u_uastrncpy(Ufield, strbuf, strlen(strbuf));
        break;

      case ELM_DATETIME_MINUTE:
        if (!strcmp(fmt, "%M"))
          {
             u_uastrncpy(Pattern, "mm", strlen("mm"));
             snprintf(strbuf, sizeof(strbuf), "%d", tim.tm_min);
             u_uastrncpy(Ufield, strbuf, strlen(strbuf));
          }
        else
          return;
        break;

      case ELM_DATETIME_AMPM:
        if ((tim.tm_hour >= 0) && (tim.tm_hour < 12))
          _AM_PM_string_set(obj, EINA_TRUE);
        else
          _AM_PM_string_set(obj, EINA_FALSE);
        return;

      default:
        return;
     }

   dt_formatter = udat_open(UDAT_IGNORE, UDAT_IGNORE, locale, NULL, -1, Pattern, -1, &status);
   calendar = ucal_open(NULL, 0, locale, UCAL_GREGORIAN, &status);
   ucal_clear(calendar);
   pos = 0;
   udat_parseCalendar(dt_formatter, calendar, Ufield, sizeof(Ufield), &pos, &status);
   date = ucal_getMillis(calendar, &status);
   udat_format(dt_formatter, date, str, sizeof(str), NULL, &status);
   u_austrcpy(buf, str);
   udat_close(dt_formatter);

   elm_object_text_set(obj, buf);
}

static void
_datetime_radio_ampm_clicked_cb(void *data, Evas_Object *obj EINA_UNUSED, const char *emission EINA_UNUSED, const char *source EINA_UNUSED)
{
   struct tm tim;
   int min, max;

   Input_Button_Module_Data *button_mod = data;

   if (!button_mod) return;

   elm_datetime_value_get(button_mod->mod_data.base, &tim);

   button_mod->mod_data.field_limit_get(button_mod->mod_data.base, ELM_DATETIME_HOUR, &min, &max);

   if (tim.tm_hour >= min && tim.tm_hour <= max)
     {
        if (tim.tm_hour > STRUCT_TM_TIME_12HRS_MAX_VALUE)
           tim.tm_hour -= STRUCT_TM_TIME_12HRS_MAX_VALUE;
        else if (tim.tm_hour == STRUCT_TM_TIME_12HRS_MAX_VALUE)
           tim.tm_hour = 0;
        else
           tim.tm_hour += STRUCT_TM_TIME_12HRS_MAX_VALUE;

        elm_datetime_value_set(button_mod->mod_data.base, &tim);
     }
}

static Eina_Bool
_value_increment_cb(void *data)
{
   Input_Button_Module_Data *button_mod = (Input_Button_Module_Data *)data;
   struct tm set_time;
   int min, max;
   int field_type;

   if (!button_mod) return ECORE_CALLBACK_CANCEL;

   field_type = elm_radio_value_get(button_mod->radio_group);

   elm_datetime_value_get(button_mod->mod_data.base, &set_time);
   button_mod->mod_data.field_limit_get(button_mod->mod_data.base, field_type, &min, &max);

   switch (field_type)
     {
      case ELM_DATETIME_YEAR:
         set_time.tm_year++;
         if (set_time.tm_year > max)
           set_time.tm_year = min;
         break;
      case ELM_DATETIME_MONTH:
         set_time.tm_mon++;
         if (set_time.tm_mon > max)
           set_time.tm_mon = min;
         break;
      case ELM_DATETIME_DATE:
         set_time.tm_mday++;
         if (set_time.tm_mday > max)
           set_time.tm_mday = min;
         break;
      case ELM_DATETIME_HOUR:
         set_time.tm_hour++;
         if (set_time.tm_hour > max)
           set_time.tm_hour = min;
         break;
      case ELM_DATETIME_MINUTE:
         set_time.tm_min++;
         if (set_time.tm_min > max)
           set_time.tm_min = min;
         break;
      case ELM_DATETIME_AMPM:
      default:
         break;
     }
   elm_datetime_value_set(button_mod->mod_data.base, &set_time);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_value_decrement_cb(void *data)
{
   Input_Button_Module_Data *button_mod = (Input_Button_Module_Data *)data;
   struct tm set_time;
   int min, max;
   int field_type;

   if (!button_mod) return ECORE_CALLBACK_CANCEL;

   field_type = elm_radio_value_get(button_mod->radio_group);

   elm_datetime_value_get(button_mod->mod_data.base, &set_time);
   button_mod->mod_data.field_limit_get(button_mod->mod_data.base, field_type, &min, &max);

   switch (field_type)
     {
      case ELM_DATETIME_YEAR:
         set_time.tm_year--;
         if (set_time.tm_year < min)
           set_time.tm_year = max;
         break;
      case ELM_DATETIME_MONTH:
         set_time.tm_mon--;
         if (set_time.tm_mon < min)
           set_time.tm_mon = max;
         break;
      case ELM_DATETIME_DATE:
         set_time.tm_mday--;
         if (set_time.tm_mday < min)
           set_time.tm_mday = max;
         break;
      case ELM_DATETIME_HOUR:
         set_time.tm_hour--;
         if (set_time.tm_hour < min)
           set_time.tm_hour = max;
         break;
      case ELM_DATETIME_MINUTE:
         set_time.tm_min--;
         if (set_time.tm_min < min)
           set_time.tm_min = max;
         break;
      case ELM_DATETIME_AMPM:
      default:
         break;
     }
   elm_datetime_value_set(button_mod->mod_data.base, &set_time);
   return ECORE_CALLBACK_RENEW;
}

static Eina_Bool
_longpress_top_button_timer_cb(void *data)
{
   Input_Button_Module_Data *button_mod = (Input_Button_Module_Data *)data;

   if (!button_mod) return ECORE_CALLBACK_CANCEL;

   button_mod->longpress_timer = NULL;
   if (button_mod->spin_timer) ecore_timer_del(button_mod->spin_timer);

   button_mod->spin_timer = ecore_timer_add(ROTATE_TIME_NORMAL, _value_increment_cb, button_mod);
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_longpress_foot_button_timer_cb(void *data)
{
   Input_Button_Module_Data *button_mod = (Input_Button_Module_Data *)data;

   if (!button_mod) return ECORE_CALLBACK_CANCEL;

   button_mod->longpress_timer = NULL;
   if (button_mod->spin_timer) ecore_timer_del(button_mod->spin_timer);

   button_mod->spin_timer = ecore_timer_add(ROTATE_TIME_NORMAL, _value_decrement_cb, button_mod);
   return ECORE_CALLBACK_CANCEL;
}

static void
_button_top_pressed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Button_Module_Data *button_mod = (Input_Button_Module_Data *)data;
   Elm_Datetime_Field_Type field_type;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type_top_btn");
   elm_radio_value_set(button_mod->radio_group, field_type);

   if (button_mod->longpress_timer) ecore_timer_del(button_mod->longpress_timer);

   button_mod->longpress_timer = ecore_timer_add(_elm_config->longpress_timeout, _longpress_top_button_timer_cb, button_mod);
}

static void
_button_foot_pressed_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Input_Button_Module_Data *button_mod = (Input_Button_Module_Data *)data;
   Elm_Datetime_Field_Type field_type;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type_foot_btn");
   elm_radio_value_set(button_mod->radio_group, field_type);

   if (button_mod->longpress_timer) ecore_timer_del(button_mod->longpress_timer);

   button_mod->longpress_timer = ecore_timer_add(_elm_config->longpress_timeout, _longpress_foot_button_timer_cb, button_mod);
}

static void
_button_top_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Elm_Datetime_Field_Type field_type;
   Input_Button_Module_Data *button_mod;
   button_mod = (Input_Button_Module_Data *)data;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type_top_btn");

   elm_radio_value_set(button_mod->radio_group, field_type);

   _value_increment_cb(button_mod);
}

static void
_button_top_foot_unpressed_cb(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Input_Button_Module_Data *button_mod = (Input_Button_Module_Data *)data;
   if (button_mod->longpress_timer)
     {
        ecore_timer_del(button_mod->longpress_timer);
        button_mod->longpress_timer = NULL;
     }
   if (button_mod->spin_timer)
     {
        ecore_timer_del(button_mod->spin_timer);
        button_mod->spin_timer = NULL;
     }
}

static void
_button_foot_clicked_cb(void *data, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   Elm_Datetime_Field_Type field_type;
   Input_Button_Module_Data *button_mod;
   button_mod = (Input_Button_Module_Data *)data;

   field_type = (Elm_Datetime_Field_Type)evas_object_data_get(obj, "_field_type_foot_btn");
   elm_radio_value_set(button_mod->radio_group, field_type);

   _value_decrement_cb(button_mod);
}

static Evas_Object *
_top_button_field_create(Input_Button_Module_Data *button_mod, Elm_Datetime_Field_Type field_type)
{
   Evas_Object *top;
   char buf[BUFF_SIZE];
   int loc;

   if (!button_mod) return NULL;

   top = elm_button_add(button_mod->mod_data.base);
   elm_object_style_set(top, "datetime/top_btn");
   evas_object_size_hint_align_set(top, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(top, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(top, "clicked", _button_top_clicked_cb, button_mod);
   evas_object_smart_callback_add(top, "unpressed", _button_top_foot_unpressed_cb, button_mod);
   evas_object_smart_callback_add(top, "pressed", _button_top_pressed_cb, button_mod);

   button_mod->mod_data.field_location_get(button_mod->mod_data.base, field_type, &loc);
   snprintf(buf, sizeof(buf), "up_btn_field%d.padding", loc);

   elm_layout_content_set(button_mod->mod_data.base, buf, top);

   return top;
}

static Evas_Object *
_foot_button_field_create(Input_Button_Module_Data *button_mod, Elm_Datetime_Field_Type field_type)
{
   Evas_Object *foot;
   char buf[BUFF_SIZE];
   int loc;

   if (!button_mod) return NULL;

   foot = elm_button_add(button_mod->mod_data.base);
   elm_object_style_set(foot, "datetime/foot_btn");
   evas_object_size_hint_align_set(foot, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_size_hint_weight_set(foot, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_smart_callback_add(foot, "clicked", _button_foot_clicked_cb, button_mod);
   evas_object_smart_callback_add(foot, "unpressed", _button_top_foot_unpressed_cb, button_mod);
   evas_object_smart_callback_add(foot, "pressed", _button_foot_pressed_cb, button_mod);

   button_mod->mod_data.field_location_get(button_mod->mod_data.base, field_type, &loc);
   snprintf(buf, sizeof(buf), "down_btn_field%d.padding", loc);

   elm_layout_content_set(button_mod->mod_data.base, buf, foot);

   return foot;
}

static void
_first_field_select(Input_Button_Module_Data *button_mod)
{
   const char *style;
   int idx, loc;

   if (!button_mod) return;

   style = elm_object_style_get(button_mod->mod_data.base);

   for (idx = 0; idx <= ELM_DATETIME_AMPM; idx++)
     {
        button_mod->mod_data.field_location_get(button_mod->mod_data.base, idx, &loc);

        if (loc == 0 && (!strcmp(style, "datepicker_layout")))
           elm_radio_value_set(button_mod->radio_group, idx);
        else if (loc == 3 && (!strcmp(style, "timepicker_layout")))
           elm_radio_value_set(button_mod->radio_group, idx);
     }
}

static void
_adjust_layout(Input_Button_Module_Data *button_mod)
{
   Evas_Object *datetime;
   char buf[BUFF_SIZE];
   int idx, loc;
   Eina_Bool is_ampm_exist;

   if (!button_mod) return;
   datetime = button_mod->mod_data.base;

   for (idx = 0; idx <= ELM_DATETIME_AMPM; idx++)
     {
        button_mod->mod_data.field_location_get(datetime, idx, &loc);
        switch (idx)
          {
           case ELM_DATETIME_YEAR:
              snprintf(buf, sizeof(buf), "elm,state,field%d,year", loc);
              elm_object_signal_emit(datetime, buf, "elm");
              break;
           case ELM_DATETIME_MONTH:
              snprintf(buf, sizeof(buf), "elm,state,field%d,month", loc);
              elm_object_signal_emit(datetime, buf, "elm");
              break;
           case ELM_DATETIME_DATE:
              snprintf(buf, sizeof(buf), "elm,state,field%d,date", loc);
              elm_object_signal_emit(datetime, buf, "elm");
              break;
           case ELM_DATETIME_HOUR:
              snprintf(buf, sizeof(buf), "elm,state,field%d,hour", loc);
              elm_object_signal_emit(datetime, buf, "elm");
              break;
           case ELM_DATETIME_MINUTE:
              snprintf(buf, sizeof(buf), "elm,state,field%d,minute", loc);
              elm_object_signal_emit(datetime, buf, "elm");
              break;
           case ELM_DATETIME_AMPM:
              snprintf(buf, sizeof(buf), "elm,state,field%d,ampm", loc);
              elm_object_signal_emit(datetime, buf, "elm");
              break;
           default:
             break;
          }
     }

   is_ampm_exist = button_mod->mod_data.field_location_get(datetime, ELM_DATETIME_AMPM, &loc);
   if (!is_ampm_exist)
     {
        snprintf(buf, sizeof(buf), "elm,state,field%d,ampm,hide", loc);
        elm_layout_signal_emit(button_mod->mod_data.base, buf, "elm");
     }

   edje_object_message_signal_process(elm_layout_edje_get(datetime));
}

EAPI Evas_Object *
field_create(Elm_Datetime_Module_Data *module_data, Elm_Datetime_Field_Type field_type)
{
   Input_Button_Module_Data *button_mod;
   Evas_Object *field_obj;
   char buf[BUFF_SIZE];

   button_mod = (Input_Button_Module_Data *)module_data;
   if (!button_mod) return NULL;

   field_obj = elm_radio_add(button_mod->mod_data.base);

   switch ((int)field_type)
   {
      case ELM_DATETIME_YEAR:
         snprintf(buf, sizeof(buf), "datetime/style%d", 1);
         break;
      case ELM_DATETIME_MONTH:
         snprintf(buf, sizeof(buf), "datetime/style%d", 2);
         break;
      case ELM_DATETIME_DATE:
         snprintf(buf, sizeof(buf), "datetime/style%d", 1);
         break;
      case ELM_DATETIME_HOUR:
         snprintf(buf, sizeof(buf), "datetime/style%d", 1);
         break;
      case ELM_DATETIME_MINUTE:
         snprintf(buf, sizeof(buf), "datetime/style%d", 1);
         break;
      case ELM_DATETIME_AMPM:
         snprintf(buf, sizeof(buf), "datetime/style%d", 3);
         elm_layout_signal_callback_add(field_obj, "elm,action,radio,clicked", "", _datetime_radio_ampm_clicked_cb, button_mod);
         break;
   }

   elm_object_style_set(field_obj, buf);

   elm_radio_state_value_set(field_obj, field_type);
   evas_object_data_set(field_obj, "_field_type", (void *)field_type);

   if (!button_mod->radio_group)
     {
        button_mod->radio_group = field_obj;
        elm_radio_value_set(button_mod->radio_group, 1);
     }
   else
     elm_radio_group_add(field_obj, button_mod->radio_group);

   evas_object_data_set(field_obj, "_field_type", (void *)field_type);

   return field_obj;
}

static void
_field_button_del(Input_Button_Module_Data *button_mod, Elm_Datetime_Field_Type field_type)
{
   if (!button_mod) return;

   if (button_mod->top_button[field_type])
     evas_object_del(button_mod->top_button[field_type]);
   if (button_mod->foot_button[field_type])
     evas_object_del(button_mod->foot_button[field_type]);
}

// module fucns for the specific module type
EAPI void
field_format_changed(Elm_Datetime_Module_Data *module_data, Evas_Object *obj)
{
   Input_Button_Module_Data *button_mod;
   Elm_Datetime_Field_Type  field_type;
   Evas_Object *top_foot_button;

   button_mod = (Input_Button_Module_Data *)module_data;
   if (!button_mod || !obj) return;

   field_type = (Elm_Datetime_Field_Type )evas_object_data_get(obj, "_field_type");

   _adjust_layout(button_mod);

   _field_button_del(button_mod, field_type);

   if (field_type != ELM_DATETIME_AMPM)
     {
        top_foot_button = _top_button_field_create(button_mod, field_type);
        button_mod->top_button[field_type] = top_foot_button;
        evas_object_data_set(top_foot_button, "_field_type_top_btn", (void *)field_type);

        top_foot_button = _foot_button_field_create(button_mod, field_type);
        evas_object_data_set(top_foot_button, "_field_type_foot_btn", (void *)field_type);
        button_mod->foot_button[field_type] = top_foot_button;
     }

   _first_field_select(button_mod);
}

EAPI Elm_Datetime_Module_Data *
obj_hook(Evas_Object *obj EINA_UNUSED)
{
   Input_Button_Module_Data *button_mod;
   button_mod = ELM_NEW(Input_Button_Module_Data);
   if (!button_mod) return NULL;
   button_mod->radio_group = NULL;

   return ((Elm_Datetime_Module_Data*)button_mod);
}

EAPI void
obj_unhook(Elm_Datetime_Module_Data *module_data EINA_UNUSED)
{
   Input_Button_Module_Data *button_mod;

   button_mod = (Input_Button_Module_Data *)module_data;
   if (!button_mod) return;

   if (button_mod)
     {
        if (button_mod->longpress_timer)
           ecore_timer_del(button_mod->longpress_timer);
        if (button_mod->spin_timer)
           ecore_timer_del(button_mod->spin_timer);

        free(button_mod);
        button_mod = NULL;
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
