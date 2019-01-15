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

#include <eina_module.h>
#include <Edje.h>
#include <feedback.h>
//include <feedback-internal.h>

static int _edje_feedback_log_dom = -1;
static int _init_count = 0;
static Eina_Bool _init_feedback = EINA_FALSE;

#define CRITICAL(...) EINA_LOG_DOM_CRIT(_edje_feedback_log_dom, __VA_ARGS__)
#define ERR(...)      EINA_LOG_DOM_ERR(_edje_feedback_log_dom, __VA_ARGS__)
#define WRN(...)      EINA_LOG_DOM_WARN(_edje_feedback_log_dom, __VA_ARGS__)
#define INF(...)      EINA_LOG_DOM_INFO(_edje_feedback_log_dom, __VA_ARGS__)
#define DBG(...)      EINA_LOG_DOM_DBG(_edje_feedback_log_dom, __VA_ARGS__)

#define ERR_GOTO(label, fmt, args...)\
   do { \
        ERR(fmt"\n", ##args); \
        goto label; \
   } while (0)

#define ERR_RET_VAL(ret, fmt, args...)\
   do { \
        ERR(fmt"\n", ##args); \
        return ret; \
   } while (0)

#define ERR_RET(fmt, args...)\
   do { \
        ERR(fmt"\n", ##args); \
        return; \
   } while (0)

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef EFL_EINA_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_EINA_BUILD */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
/**
 * @def EAPI
 * @brief Used to export functions(by changing visibility).
 */
#  define EAPI
# endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

EAPI Eina_Bool
edje_plugin_run(Evas_Object *edje,
                const char *name,
                const char *param)
{
   if (!edje) return EINA_FALSE;
   if (!name) return EINA_FALSE;

   if (!_init_feedback)
     {
        feedback_initialize();
        _init_feedback = EINA_TRUE;
     }

   unsigned int cnt;
   char **tok;
   tok = eina_str_split_full(param, " ", 0, &cnt);
   if (tok && (cnt == 2) && tok[0] && tok[1])
     {
        feedback_error_e err;
        err = feedback_play_type_by_name(tok[0], tok[1]);

        if (tok[0]) free(tok[0]);
        free(tok);

        if (err != FEEDBACK_ERROR_NONE)
          {
             WRN("feedback_play_type() failed: %d", err);
             return EINA_FALSE;
          }
        return EINA_TRUE;
     }
   free(tok);
   return EINA_FALSE;
}

Eina_Bool
_module_init(void)
{
   _init_count++;
   if (_init_count == 1)
     {
        _edje_feedback_log_dom = eina_log_domain_register("elm_feedback",
                                                          EINA_COLOR_LIGHTBLUE);
        if (!_edje_feedback_log_dom)
          {
             EINA_LOG_ERR("could not register elm_feedback log domain.");
             _edje_feedback_log_dom = EINA_LOG_DOMAIN_GLOBAL;
          }
     }
   return EINA_TRUE;
}

void
_module_shutdown(void)
{
   _init_count--;
   if (_init_count == 0)
     {
        eina_log_domain_unregister(_edje_feedback_log_dom);
        _edje_feedback_log_dom = -1;
        if (_init_feedback)
          {
             feedback_deinitialize();
             _init_feedback = EINA_FALSE;
          }
     }
}

EINA_MODULE_INIT(_module_init);
EINA_MODULE_SHUTDOWN(_module_shutdown);
#ifdef __cplusplus
}
#endif
