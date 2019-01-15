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
 * @file    ewk_settings_internal.h
 * @brief   Describes the settings API.
 *
 * @note The ewk_settings is for setting the preference of specific ewk_view.
 * We can get the ewk_settings from ewk_view using ewk_view_settings_get() API.
 */

#ifndef ewk_settings_internal_h
#define ewk_settings_internal_h

#include <tizen.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ewk_settings_type
#define ewk_settings_type
/** Creates a type name for Ewk_Settings */
typedef struct Ewk_Settings Ewk_Settings;
#endif

/**
 * Requests enables/disables the plug-ins.
 *
 * @param settings settings object to set the plug-ins
 * @param enable @c EINA_TRUE to enable the plug-ins
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_plugins_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returs enables/disables the plug-ins.
 *
 * @param settings settings object to set the plug-ins
*
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_plugins_enabled_get(const Ewk_Settings* settings);

/**
 * Checks whether WebKit supports the @a encoding.
 *
 * @param encoding the encoding string to check whether WebKit supports it
 *
 * @return @c EINA_TRUE if WebKit supports @a encoding or @c EINA_FALSE if not or
 *      on failure
 */
EXPORT_API Eina_Bool ewk_settings_is_encoding_valid(const char* encoding);

/**
 * Requests to set default text encoding name.
 *
 * @param settings settings object to set default text encoding name
 * @param encoding default text encoding name
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_default_encoding_set(Ewk_Settings* settings, const char* encoding);

/**
 * Returns default text encoding name.
 *
 * @param settings settings object to query default text encoding nae
 *
 * @return default text encoding name
 */
EXPORT_API const char* ewk_settings_default_encoding_get(const Ewk_Settings* settings);

/**
 * Requests to enable/disable link effect
 *
 * @param settings settings object to enable/disable link effect
 *
 * @param linkEffectEnabled @c EINA_TRUE to enable the link effect
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_link_effect_enabled_set(Ewk_Settings* settings, Eina_Bool linkEffectEnabled);

/**
 * Returns enable/disable link effect
 *
 * @param settings settings object to get whether link effect is enabled or disabled
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EXPORT_API Eina_Bool ewk_settings_link_effect_enabled_get(const Ewk_Settings* settings);

/**
 * Requests to set using default keypad (default value : true)
 *
 * @param settings settings object to use default keypad
 * @param enable @c EINA_TRUE to use default keypad  @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_default_keypad_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns enable/disable using default keypad
 *
 * @param settings settings object to use default keypad
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EXPORT_API Eina_Bool ewk_settings_default_keypad_enabled_get(const Ewk_Settings* settings);

/**
 * Requests to set using keypad without user action (default value : false)
 *
 * @param settings settings object using keypad without user action
 * @param enable @c EINA_TRUE to use without user action @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_uses_keypad_without_user_action_set(Ewk_Settings* settings, Eina_Bool use);

/**
 * Returns using keypad without user action
 *
 * @param settings settings object using keypad without user action
 * @param settings settings object to query using keypad without user action
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EXPORT_API Eina_Bool ewk_settings_uses_keypad_without_user_action_get(const Ewk_Settings* settings);

/**
 * Requests enable/disable password form autofill
 *
 * @param setting setting object to set password form autofill
 * @param enable @c EINA_TRUE to enable password form autofill
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_autofill_password_form_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns if password form autofill is enabled or disabled.
 *
 * @param setting setting object to get password form autofill
 *
 * @return @c EINA_TRUE if password form autofill is enabled
 *         @c EINA_FALSE if password form autofill is disabled
 */
EXPORT_API Eina_Bool ewk_settings_autofill_password_form_enabled_get(Ewk_Settings* settings);
/**
 * Requests enable/disable form candidate data for autofill
 *
 * @param setting setting object to set form candidate data for autofill
 * @param enable @c EINA_TRUE to enable form candidate data for autofill
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_form_candidate_data_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns if form candidate data for autofill is enabled or disabled.
 *
 * @param setting setting object to get form candidate data for autofill
 *
 * @return @c EINA_TRUE if form candidate data for autofill is enabled
 *         @c EINA_FALSE if form candidate data for autofill is disabled
 */
EXPORT_API Eina_Bool ewk_settings_form_candidate_data_enabled_get(Ewk_Settings* settings);

/**
 * Requests enable/disable text selection by default WebKit.
 *
 * @param settings setting object to set text selection by default WebKit
 * @param enable @c EINA_TRUE to enable text selection by default WebKit
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_text_selection_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns if text selection by default WebKit is enabled or disabled.
 *
 * @param settings setting object to get text selection by default WebKit
 *
 * @return @c EINA_TRUE if text selection by default WebKit is enabled
 *         @c EINA_FALSE if not or on failure
 */
EXPORT_API Eina_Bool ewk_settings_text_selection_enabled_get(const Ewk_Settings* settings);

/**
 * Requests to enable/disable edge effect
 *
 * @param settings settings object to enable/disable edge effect
 *
 * @param enable @c EINA_TRUE to enable the edge effect
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_edge_effect_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns enable/disable edge effect
 *
 * @param settings settings object to get whether edge effect is enabled or disabled
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EXPORT_API Eina_Bool ewk_settings_edge_effect_enabled_get(const Ewk_Settings* settings);

/**
 * Enables/disables text autosizing.
 *
 * By default, the text autosizing is disabled.
 *
 * @param settings settings object to set the text autosizing
 * @param enable @c EINA_TRUE to enable the text autosizing
 *               @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @see ewk_settings_text_autosizing_enabled_get()
 */
EXPORT_API Eina_Bool ewk_settings_text_autosizing_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns whether the text autosizing is enabled.
 *
 * The text autosizing is a feature which adjusts the font size of text in wide
 * columns, and makes text more legible.
 *
 * @param settings settings object to query whether text autosizing is enabled
 *
 * @return @c EINA_TRUE if the text autosizing is enabled
 *         @c EINA_FALSE if not or on failure
 */
EXPORT_API Eina_Bool ewk_settings_text_autosizing_enabled_get(const Ewk_Settings* settings);
/**
 * Sets the scale factor for text autosizing.
 *
 * Default value is 1.0.
 *
 * @param settings settings object to set the text autosizing
 * @param factor font scale factor for text autosizing
 */
EINA_DEPRECATED EXPORT_API Eina_Bool ewk_settings_text_autosizing_font_scale_factor_set(Ewk_Settings* settings, double factor);

/**
 * Sets the scale factor for text autosizing.
 *
 * Default value is 1.0.
 *
 * @param settings settings object to set the text autosizing
 * @param factor font scale factor for text autosizing
 */
EXPORT_API Eina_Bool ewk_settings_text_autosizing_scale_factor_set(Ewk_Settings* settings, double factor);

/**
 * Sets the scale factor for text autosizing.
 *
 * Default value is 1.0.
 *
 * @param settings settings object to set the text autosizing
 * @param factor font scale factor for text autosizing
 */
EXPORT_API Eina_Bool ewk_settings_text_autosizing_font_scale_factor_set(Ewk_Settings* settings, double factor);
/**
 * Gets the current scale factor for text autosizing.
 *
 * @param settings settings object to set scale factor for text autosizing
 *
 * @return the current font scale factor for text autosizing
 */
EINA_DEPRECATED EXPORT_API double ewk_settings_text_autosizing_font_scale_factor_get(const Ewk_Settings* settings);

/**
 * Gets the current scale factor for text autosizing.
 *
 * @param settings settings object to set scale factor for text autosizing
 *
 * @return the current font scale factor for text autosizing
 */
EXPORT_API double ewk_settings_text_autosizing_font_scale_factor_get(const Ewk_Settings* settings);
/**
 * Gets the current scale factor for text autosizing.
 *
 * @param settings settings object to set scale factor for text autosizing
 *
 * @return the current font scale factor for text autosizing
 */
EXPORT_API double ewk_settings_text_autosizing_scale_factor_get(const Ewk_Settings* settings);

/**
 * Requests setting use of text zoom.
 *
 * @param settings settings object to text zoom
 * @param enable to text zoom.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_text_zoom_enabled_set(Ewk_Settings* settings, Eina_Bool enable);

/**
 * Returns whether text zoom is enabled or not.
 *
 * @param settings settings object to text zoom
 *
 * @return @c EINA_TRUE if enable text zoom or @c EINA_FALSE.
 */
EXPORT_API Eina_Bool ewk_settings_text_zoom_enabled_get(const Ewk_Settings* settings);

/**
 * Requests enables/disables to the specific extra feature
 *
 * @param settings setting object to enable/disable the specific extra feature
 * @param feature feature name
 * @param enable @c EINA_TRUE to enable the specific extra feature
 *        @c EINA_FALSE to disable
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API void ewk_settings_extra_feature_set(Ewk_Settings* settings, const char* feature, Eina_Bool enable);

/**
 * Returns enable/disable to the specific extra feature
 *
 * @param settings settings object to get whether the specific extra feature is enabled or not.
 * @param feature feature name
 *
 * @return @c EINA_TRUE on enable or @c EINA_FALSE on disable
 */
EXPORT_API Eina_Bool ewk_settings_extra_feature_get(const Ewk_Settings* settings, const char* feature);

/**
 * Sets font-family as system font for font rendering
 *
 * @param settings settings object
 * @param use @c EINA_TRUE to use one of the system fonts which is selected by user in Settings
 *               @c EINA_FALSE to use a system default font
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_settings_use_system_font_set(Ewk_Settings* settings, Eina_Bool use);

/**
 * Returns whether we use the system font which is selected by user in Settings or use a system default font
 *
 * @param settings settings object
 *
 * @return @c EINA_TRUE if we use the sysem font which is selected by user in Settings
 *        @c EINA_FALSE if we use a system default font or on failure
 */
EXPORT_API Eina_Bool ewk_settings_use_system_font_get(Ewk_Settings* settings);
#ifdef __cplusplus
}
#endif
#endif // ewk_settings_internal_h
