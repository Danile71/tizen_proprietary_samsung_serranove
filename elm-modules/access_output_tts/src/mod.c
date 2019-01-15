/*
 * Copyright (c) 2013 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Licensed under the Flora License, Version 1.1 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://floralicense.org/license/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <Elementary.h>
#include <tts.h>
#include <dlog.h>
#include <mm_sound_private.h>
#include <vconf.h>

#undef LOG_TAG
#define LOG_TAG "access-output-tts"

#define TEXT_CUT_SIZE 300

/* to enable this module
export ELM_MODULES="access_output_tts>access/api"
export ELM_ACCESS_MODE=1
 */

static void (*cb_func) (void *data);
static void *cb_data;
static tts_h tts;
static Eina_Strbuf *buf = NULL;

const char* FOCUS_SOUND_FILE = ACCESS_OUTPUT_TTS_SOUND_DIR "Focus.ogg";
const char* END_OF_LIST_SOUND_FILE = ACCESS_OUTPUT_TTS_SOUND_DIR "End_of_List.ogg";
const char* LIST_SCROLL_SOUND_FILE = ACCESS_OUTPUT_TTS_SOUND_DIR "List_scroll.ogg";

static void _text_keep(const char *txt)
{
   if (!buf) return;
   if (eina_strbuf_length_get(buf) > 0) eina_strbuf_append(buf, ", ");
   eina_strbuf_append(buf, txt);
}

static Eina_Bool _text_add(const char *txt)
{
   int ret = 0;
   int u_id = 0;
   int lcd_val = 0;

   vconf_get_int(VCONFKEY_PM_STATE, &lcd_val);
   if (lcd_val == VCONFKEY_PM_STATE_LCDOFF)
   {
      fprintf(stderr, "lcd state off (%d)\n", lcd_val);
      return EINA_TRUE;
   }

   if(txt == NULL) return EINA_FALSE;

   if (strlen(txt) < TEXT_CUT_SIZE)
     {
        ret = tts_add_text(tts, txt, NULL, TTS_VOICE_TYPE_AUTO,
                           TTS_SPEED_AUTO, &u_id);
     }
   else
     {
        char tmp[TEXT_CUT_SIZE + 1];
        int i, p = 0;

        while (strlen(&txt[p]) > TEXT_CUT_SIZE)
          {
             strncpy(tmp, &txt[p], TEXT_CUT_SIZE);

             for (i = 1; i < TEXT_CUT_SIZE - 1 ; i++)
               {
                  if (tmp[i] == ' ' &&
                      (tmp[i - 1] == '?' ||
                       tmp[i - 1] == '.' ||
                       tmp[i - 1] == ','))
                    break;
               }

             tmp[i + 1] = '\0';
             p += i + 1;

             ret = tts_add_text(tts, tmp, NULL, TTS_VOICE_TYPE_AUTO,
                                TTS_SPEED_AUTO, &u_id);

             if (TTS_ERROR_NONE != ret)
               {
                  fprintf(stderr, "Fail to add text : %s ret(%d)\n", txt, ret);
                  return EINA_FALSE;
               }
          }
        ret = tts_add_text(tts, &txt[p], NULL, TTS_VOICE_TYPE_AUTO,
                           TTS_SPEED_AUTO, &u_id);
     }
   if (TTS_ERROR_NONE != ret)
     {
        fprintf(stderr, "Fail to add text : %s ret(%d)\n", txt, ret);
        return EINA_FALSE;
     }

   return EINA_TRUE;
}

void _tts_state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* data)
{
   int ret = 0;

   if (TTS_STATE_CREATED == previous && TTS_STATE_READY == current)
     {
        char *txt;

        if (!buf) return;
        if (!eina_strbuf_length_get(buf)) return;

        txt = eina_strbuf_string_steal(buf);
        eina_strbuf_free(buf);

        if (txt && strlen(txt) == 0) return;

        if (_text_add(txt) == EINA_FALSE) return;

        ret = tts_play(tts);
        if (TTS_ERROR_NONE != ret)
          {
             fprintf(stderr, "Fail to play TTS : ret(%d)\n", ret);
          }
     }
   else if (TTS_STATE_PLAYING == previous && TTS_STATE_READY == current)
     {
        if (cb_func)
          {
             cb_func(cb_data);
             cb_func = NULL;
             cb_data = NULL;
          }
     }
}

void utterance_completed_callback(tts_h tts, int utt_id, void* user_data)
{
   if (cb_func)
     {
        cb_func(cb_data);
        cb_func = NULL;
        cb_data = NULL;
     }
}

EAPI int
elm_modapi_init(void *m )
{
   int ret = 0;

   if (tts)
     {
        tts_state_e state;
        tts_get_state(tts, &state);

        if (state == TTS_STATE_CREATED) goto prepare;

        if (state == TTS_STATE_READY ||
            state == TTS_STATE_PLAYING ||
            state == TTS_STATE_PAUSED)
          return 1;
     }

   ret = tts_create(&tts);
   if (TTS_ERROR_NONE != ret)
     {
        fprintf(stderr, "Fail to get handle : result(%d)", ret);
        return ret;
     }
   else
     {
        SLOG(LOG_DEBUG, LOG_TAG, "tts_create(); done");
     }

   ret = tts_set_state_changed_cb(tts, _tts_state_changed_cb, NULL);
   if (TTS_ERROR_NONE != ret)
     {
        fprintf(stderr, "Fail to set callback : result(%d)", ret);
        return ret;
     }

   ret = tts_set_mode(tts, TTS_MODE_SCREEN_READER);
   if (TTS_ERROR_NONE != ret)
     {
        fprintf(stderr, "Fail to set mode : result(%d)", ret);
        return ret;
     }

 prepare:
   ret = tts_set_utterance_completed_cb(tts, utterance_completed_callback, NULL);
   if (TTS_ERROR_NONE != ret)
     {
        fprintf(stderr, "Fail to register utterance completed callback function : result(%d)", ret);
        return ret;
     }

   ret = tts_prepare(tts);
   if (TTS_ERROR_NONE != ret)
     {
        fprintf(stderr, "Fail to prepare handle : result(%d)", ret);
        return ret;
     }

   buf = eina_strbuf_new();

   return 1;
}

EAPI int
elm_modapi_shutdown(void *m )
{
   int ret = 1;
   if (tts)
     {
        ret = tts_destroy(tts);
        tts = NULL;
     }
   return ret;
}

EAPI void
out_read(const char *txt)
{
   int ret = 0;

   tts_state_e state;
   tts_get_state(tts, &state);
   if (state != TTS_STATE_PLAYING &&
       state != TTS_STATE_PAUSED &&
       state != TTS_STATE_READY)
     {
        if (txt) _text_keep(txt);
        return;
     }

   if (_text_add(txt) == EINA_FALSE) return;

   /* check current state */
   tts_get_state(tts, &state);
   if (state == TTS_STATE_PLAYING) return;

   ret = tts_play(tts);
   if (TTS_ERROR_NONE != ret)
     {
        fprintf(stderr, "Fail to play TTS : ret(%d)\n", ret);
     }
   return;
}


EAPI void
out_cancel(void)
{
   int ret = 0;
   if (tts)
     {
        /* check current state */
        tts_state_e state;

        tts_get_state(tts, &state);
        if (state != TTS_STATE_PLAYING && state != TTS_STATE_PAUSED)
          {
             if (buf && (eina_strbuf_length_get(buf) > 0))
               eina_strbuf_string_free(buf);
             return;
          }

         ret = tts_stop(tts);
         if (TTS_ERROR_NONE != ret)
           {
             fprintf(stderr, "Fail to stop TTS : result(%d)", ret);
           }
     }
   return;
}

EAPI void
out_done_callback_set(void (*func) (void *data), const void *data)
{
   /* data and cb_data could be NULL */
   if (!func && (data != cb_data)) return;
   cb_func = func;
   cb_data = (void *)data;
}

EAPI void
sound_play(const Elm_Access_Sound_Type type)
{
   const char *file_name;

   file_name = NULL;
   switch (type)
     {
      case ELM_ACCESS_SOUND_HIGHLIGHT:
        file_name = FOCUS_SOUND_FILE;
        break;

      case ELM_ACCESS_SOUND_SCROLL:
        file_name = LIST_SCROLL_SOUND_FILE;
        break;

      case ELM_ACCESS_SOUND_END:
        file_name = END_OF_LIST_SOUND_FILE;
        break;

      default:
        break;
     }

   if (file_name)
     mm_sound_play_keysound(file_name, VOLUME_TYPE_SYSTEM & VOLUME_GAIN_TOUCH);
}
/*
 * unused api
 */

#if 0
EAPI void
out_read_done(void)
{
}
#endif
