/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    ewk_settings.h
 * @brief   This file describes the Ewk Settings API.
 *
 * @remarks The ewk_settings is used for setting the preference of a specific ewk_view.
 * We can get the ewk_settings from ewk_view using the ewk_view_settings_get() API.
 */

#ifndef ewk_settings_h
#define ewk_settings_h

#include <tizen.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

#ifndef ewk_settings_type
#define ewk_settings_type
/**
 * @brief The structure type that creates a type name for #Ewk_Settings.
 * @since_tizen 2.3
 */
typedef struct Ewk_Settings Ewk_Settings;
#endif

/**
 * @brief Requests enable/disable password form autofill
 *
 * @since_tizen 2.4
 *
 * @param[in] setting Setting object to set password form autofill
 * @param[in] enable @c EINA_TRUE to enable password form autofill
 *                   @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success, otherwise @c EINA_FALSE
 */
EXPORT_API Eina_Bool ewk_settings_autofill_password_form_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * @brief Requests enable/disable form candidate data for autofill
 *
 * @since_tizen 2.4
 *
 * @param[in] setting Setting object to set form candidate data for autofill
 * @param[in] enable @c EINA_TRUE to enable form candidate data for autofill
 *                   @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success, otherwise  @c EINA_FALSE
 */
EXPORT_API Eina_Bool ewk_settings_form_candidate_data_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * @brief Enables/disables form autofill profile feature.
 *
 * @since_tizen 2.4
 *
 * @param[in] settings Settings object to set the form autofill profile
 * @param[in] enable @c EINA_TRUE to enable the text autosizing
 *                   @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success, otherwise @c EINA_FALSE
 *
 */
EXPORT_API Eina_Bool ewk_settings_form_profile_data_enabled_set(Ewk_Settings *settings, Eina_Bool enable);

/**
 * @brief Requests setting of auto fit.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to fit to width
 * @param[in] enable If @c true the settings object is fit to width,\n
 *                   otherwise @c false
 *
 * @return @c EINA_TRUE on success,\n
 *         otherwise @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_auto_fitting_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * @brief Returns the auto fit status.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to fit to width
 *
 * @return @c EINA_TRUE if auto fit is enabled,\n
 *         otherwise @c EINA_FALSE
 */
EXPORT_API Eina_Bool ewk_settings_auto_fitting_get(const Ewk_Settings* settings);

/**
 * @brief Enables/disables JavaScript executing.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to set JavaScript executing
 * @param[in] enable If @c EINA_TRUE JavaScript executing is enabled,\n
 *                   otherwise @c EINA_FALSE to disable it
 *
 * @return @c EINA_TRUE on success,\n
 *         otherwise @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_javascript_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * @brief Returns whether JavaScript can be executable.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to query if JavaScript can be executed
 *
 * @return @c EINA_TRUE if JavaScript can be executed,\n
 *         otherwise @c EINA_FALSE if not or on failure
 */
EXPORT_API Eina_Bool ewk_settings_javascript_enabled_get(const Ewk_Settings* settings);

/**
 * @brief Enables/disables auto loading of images.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to set auto loading of images
 * @param[in] automatic If @c EINA_TRUE auto loading of images is enabled,\n
 *                      otherwise @c EINA_FALSE to disable it
 *
 * @return @c EINA_TRUE on success,\n
 *         otherwise @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_loads_images_automatically_set(Ewk_Settings* settings, Eina_Bool automatic);

/**
 * @brief Returns whether images can be loaded automatically.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to get auto loading of images
 *
 * @return @c EINA_TRUE if images are loaded automatically,\n
 *         otherwise @c EINA_FALSE if not or on failure
 */
EXPORT_API Eina_Bool ewk_settings_loads_images_automatically_get(const Ewk_Settings* settings);

/**
 * @brief Sets the default text encoding name.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to set the default text encoding name
 * @param[in] encoding The default text encoding name
 *
 * @return @c EINA_TRUE on success,\n
 *         otherwise @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_default_text_encoding_name_set(Ewk_Settings* settings, const char* encoding);

/**
 * @brief Gets the default text encoding name.
 *
 * @details The returned string is guaranteed to be stringshared.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to query the default text encoding name
 *
 * @return The default text encoding name
 */
EXPORT_API const char* ewk_settings_default_text_encoding_name_get(const Ewk_Settings* settings);

/**
 * @brief Sets the default font size.
 *
 * @details By default, the default font size is @c 16.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to set the default font size
 * @param[in] size A new default font size to set
 *
 * @return @c EINA_TRUE on success,\n
 *         otherwise @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_default_font_size_set(Ewk_Settings* settings, int size);

/**
 * @brief Returns the default font size.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to get the default font size
 *
 * @return The default font size,\n
 *         otherwise @c 0 on failure
 */
EXPORT_API int ewk_settings_default_font_size_get(const Ewk_Settings* settings);

/**
 * @deprecated Deprecated since 2.4
 *
 * @brief Requests to enable/disable private browsing.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to set private browsing
 * @param[in] enable If @c EINA_TRUE private browsing is enabled\n
 *                   otherwise @c EINA_FALSE to disable it
 *
 * @return @c EINA_TRUE on success,\n
 *         otherwise @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_private_browsing_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * @deprecated Deprecated since 2.4
 *
 * @brief Enables/disables private browsing.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to query if private browsing is enabled
 *
 * @return @c EINA_TRUE if private browsing is enabled,\n
 *         otherwise @c EINA_FALSE if not or on failure
 */
EXPORT_API Eina_Bool ewk_settings_private_browsing_enabled_get(const Ewk_Settings* settings);

/**
 * @brief Enables/disables if the scripts can open new windows.
 *
 * @details By default, the scripts can open new windows.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to set if the scripts can open new windows
 * @param[in] enable If @c EINA_TRUE the scripts can open new windows\n
 *                   otherwise @c EINA_FALSE if not
 *
 * @return @c EINA_TRUE on success,\n
 *         otherwise @c EINA_FALSE on failure (scripts are disabled)
 */
EXPORT_API Eina_Bool ewk_settings_scripts_can_open_windows_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * @brief Returns whether the scripts can open new windows.
 *
 * @since_tizen 2.3
 *
 * @param[in] settings The settings object to query whether the scripts can open new windows
 *
 * @return @c EINA_TRUE if the scripts can open new windows\n
 *         otherwise @c EINA_FALSE if not or on failure (scripts are disabled)
 */
EXPORT_API Eina_Bool ewk_settings_scripts_can_open_windows_get(const Ewk_Settings* settings);

/**
* @}
*/

#ifdef __cplusplus
}
#endif
#endif // ewk_settings_h
