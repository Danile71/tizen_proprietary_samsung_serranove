/*
 * ISF(Input Service Framework)
 *
 * ISF is based on SCIM 1.4.7 and extended for supporting more mobile fitable.
 * Copyright (c) 2012-2014 Samsung Electronics Co., Ltd.
 *
 * Contact: Haifeng Deng <haifeng.deng@samsung.com>, Hengliang Luo <hl.luo@samsung.com>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the
 * Free Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef __ISF_CONTROL_H
#define __ISF_CONTROL_H

#include <scim_visibility.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/////////////////////////////////////////////////////////////////////////////
// Declaration of global data types.
/////////////////////////////////////////////////////////////////////////////
typedef enum
{
    HARDWARE_KEYBOARD_ISE = 0,  /* Hardware keyboard ISE */
    SOFTWARE_KEYBOARD_ISE       /* Software keyboard ISE */
} ISE_TYPE_T;

/**
 * @brief The structure type to contain IME's information.
 *
 * @since_tizen 2.4
 *
 * @see isf_control_get_all_ime_info
 */
typedef struct {
    char appid[128];
    char label[256];
    bool is_enabled;
    bool is_preinstalled;
    int has_option; // 0: no keyboard option, 1: keyboard option is available, -1: unknown yet
} ime_info_s;

/////////////////////////////////////////////////////////////////////////////
// Declaration of global functions.
/////////////////////////////////////////////////////////////////////////////
/**
 * @deprecated Deprecated since tizen 2.4. [Use isf_control_set_active_ime() instead]
 *
 * @brief Set active ISE by UUID.
 *
 * @param uuid The active ISE's UUID.
 *
 * @return 0 if successfully, otherwise return -1;
 */
EXAPI int isf_control_set_active_ise_by_uuid (const char *uuid);

/**
 * @deprecated Deprecated since tizen 2.4. [Use isf_control_get_active_ime() instead]
 *
 * @brief Get active ISE's UUID.
 *
 * @param uuid The parameter is used to store active ISE's UUID.
 *             Application needs free *uuid if it is not used.
 *
 * @return the length of UUID if successfully, otherwise return -1
 */
EXAPI int isf_control_get_active_ise (char **uuid);

/**
 * @brief Get the list of all ISEs' UUID.
 *
 * @param uuid_list The list is used to store all ISEs' UUID.
 *                  Application needs free **uuid_list if it is not used.
 *
 * @return the count of UUID list if successfully, otherwise return -1;
 */
EXAPI int isf_control_get_ise_list (char ***uuid_list);

/**
 * @brief Get ISE's information according to ISE's UUID.
 *
 * @param uuid The ISE's UUID.
 * @param name     The parameter is used to store ISE's name. Application needs free *name if it is not used.
 * @param language The parameter is used to store ISE's language. Application needs free *language if it is not used.
 * @param type     The parameter is used to store ISE's type.
 * @param option   The parameter is used to store ISE's option.
 *
 * @return 0 if successfully, otherwise return -1
 */
EXAPI int isf_control_get_ise_info (const char *uuid, char **name, char **language, ISE_TYPE_T *type, int *option);

/**
 * @brief Get ISE's information according to ISE's UUID.
 *
 * @param uuid The ISE's UUID.
 * @param name     The parameter is used to store ISE's name. Application needs free *name if it is not used.
 * @param language The parameter is used to store ISE's language. Application needs free *language if it is not used.
 * @param type     The parameter is used to store ISE's type.
 * @param option   The parameter is used to store ISE's option.
 * @param module_name The parameter is used to store ISE's module file name.
 *
 * @return 0 if successfully, otherwise return -1
 */
EXAPI int isf_control_get_ise_info_and_module_name (const char *uuid, char **name, char **language, ISE_TYPE_T *type, int *option, char **module_name);

/**
 * @brief Set active ISE to default ISE.
 *
 * @return 0 if successfully, otherwise return -1
 */
EXAPI int isf_control_set_active_ise_to_default (void);

/**
 * @brief Reset all ISEs' options.
 *
 * @return 0 if successfully, otherwise return -1;
 */
EXAPI int isf_control_reset_ise_option (void);

/**
 * @brief Set initial ISE by UUID.
 *
 * @param uuid The initial ISE's UUID.
 *
 * @return 0 if successfully, otherwise return -1
 */
EXAPI int isf_control_set_initial_ise_by_uuid (const char *uuid);

/**
 * @brief Get initial ISE UUID.
 *
 * @param uuid The parameter is used to store initial ISE's UUID.
 *             Application needs free *uuid if it is not used.
 *
 * @return the length of UUID if successfully, otherwise return -1;
 */
EXAPI int isf_control_get_initial_ise (char **uuid);

/**
 * @deprecated Deprecated since tizen 2.4. [Use isf_control_show_ime_selector() instead]
 *
 * @brief Show ISE selector.
 *
 * @return 0 if successfully, otherwise return -1;
 */
EXAPI int isf_control_show_ise_selector (void);

/**
 * @brief Get the number of S/W or H/W keyboard ISEs
 *
 * @param type     ISE's type.
 *
 * @return the count of ISEs if successfully, otherwise return -1;
 */
EXAPI int isf_control_get_ise_count (ISE_TYPE_T type);

/**
 * @deprecated Deprecated since tizen 2.4. [Use isf_control_open_ime_option_window() instead]
 *
 * @brief Show ISE's option window.
 *
 * @return 0 if successfully, otherwise return -1
 */
EXAPI int isf_control_show_ise_option_window (void);

/**
 * @brief Gets the information of all IME (on-screen keyboard).
 *
 * @remarks This API should not be used by IME process.
 *
 * @since_tizen 2.4
 *
 * @param[out] info The parameter is used to store all IME information. Application needs to free @a *info variable
 *
 * @return The count of IME on success, otherwise -1
 *
 * @post The current active IME's has_option variable will have 0 or 1 value. Other IME's has_option variable might be -1 (unknown).
 *
 * @code
     ime_info_s *ime_info = NULL;
     int i, cnt = isf_control_get_all_ime_info(&ime_info);
     if (ime_info) {
         for (i = 0; i < cnt; i++) {
             LOGD("%s %s %d %d %d", ime_info[i].appid, ime_info[i].label, ime_info[i].is_enabled, ime_info[i].is_preinstalled, ime_info[i].has_option);
         }
         free(ime_info);
     }
 * @endcode
 */
EXAPI int isf_control_get_all_ime_info (ime_info_s **info);

/**
 * @brief Requests to open the current IME's option window.
 *
 * @remarks Each IME might have its option (setting) or not. This function should be called only if the current IME provides its option.
 *
 * @since_tizen 2.4
 *
 * @return 0 on success, otherwise return -1
 *
 * @pre The availibility of IME option can be found using isf_control_get_all_ime_info() and isf_control_get_active_ime() functions.
 */
EXAPI int isf_control_open_ime_option_window (void);

/**
 * @brief Gets active IME's Application ID.
 *
 * @since_tizen 2.4
 *
 * @param[out] appid This is used to store active IME's Application ID. The allocated @a appid needs to be released using free()
 *
 * @return The length of @a appid on success, otherwise -1
 */
EXAPI int isf_control_get_active_ime (char **appid);

/**
 * @brief Sets active IME by Application ID.
 *
 * @since_tizen 2.4
 *
 * @param[in] appid Application ID of IME to set as active one
 *
 * @return 0 on success, otherwise return -1
 */
EXAPI int isf_control_set_active_ime (const char *appid);

/**
 * @brief Sets On/Off of installed IME by Application ID.
 *
 * @since_tizen 2.4
 *
 * @param[in] appid Application ID of IME to enable or disable
 * @param[in] is_enabled @c true to enable the IME, otherwise @c false
 *
 * @return 0 on success, otherwise return -1
 */
EXAPI int isf_control_set_enable_ime (const char *appid, bool is_enabled);

/**
 * @brief Requests to open the installed IME list application.
 *
 * @remarks This API should not be used by inputmethod-setting application.
 *
 * @since_tizen 2.4
 *
 * @return 0 on success, otherwise return -1
 */
EXAPI int isf_control_show_ime_list (void);

/**
 * @brief Requests to open the IME selector application.
 *
 * @remarks This API should not be used by inputmethod-setting application.
 *
 * @since_tizen 2.4
 *
 * @return 0 on success, otherwise return -1
 */
EXAPI int isf_control_show_ime_selector (void);

/**
 * @brief Checks if the specific IME is enabled or disabled in the system keyboard setting.
 *
 * @since_tizen 2.4
 *
 * @param[in] appid The application ID of the IME
 * @param[out] enabled The On (enabled) and Off (disabled) state of the IME
 *
 * @return 0 on success, otherwise return -1
 */
EXAPI int isf_control_is_ime_enabled (const char *appid, bool *enabled);

/**
 * @brief Get the recent geometry of S/W keyboard
 *
 * @remarks If the keyboard has never been shown, this function will return -1 since the framework can't know its size.
 * The caller application needs to assume the default height.
 *
 * @since_tizen 2.4
 *
 * @param[out] x Pointer to an integer in which to store the X coordinate of the IME that appeared recently.
 * @param[out] y Pointer to an integer in which to store the Y coordinate of the IME that appeared recently.
 * @param[out] w Pointer to an integer in which to store the width of the IME that appeared recently.
 * @param[out] h Pointer to an integer in which to store the height of the IME that appeared recently.
 *
 * @return 0 if successfully, otherwise return -1;
 */
EXAPI int isf_control_get_recent_ime_geometry (int *x, int *y, int *w, int *h);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __ISF_CONTROL_H */

/*
vi:ts=4:nowrap:ai:expandtab
*/
