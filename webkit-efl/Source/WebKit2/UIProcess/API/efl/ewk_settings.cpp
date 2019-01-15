/*
 * Copyright (C) 2012 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#include "config.h"
#include "ewk_settings.h"

#include "EwkViewImpl.h"
#include "ewk_settings_private.h"
#include <WebKit2/WebPageGroup.h>
#include <WebKit2/WebPageProxy.h>
#include <WebKit2/WebPreferences.h>

#if PLATFORM(TIZEN)
#include "WKAPICast.h"
#include "WKPreferences.h"
#include "WKPreferencesTizen.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include <Eina.h>
#include <wtf/OwnArrayPtr.h>
#endif

#if ENABLE(SPELLCHECK)
#include "WKTextChecker.h"
#include "ewk_text_checker_private.h"
#include <Ecore.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>
#endif

#if ENABLE(TIZEN_ENCODING_VERIFICATION)
#include "TextEncoding.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#include "FormDatabase.h"
#endif

using namespace WebKit;

const WebKit::WebPreferences* Ewk_Settings::preferences() const
{
    return m_viewImpl->pageProxy->pageGroup()->preferences();
}

WebKit::WebPreferences* Ewk_Settings::preferences()
{
    return m_viewImpl->pageProxy->pageGroup()->preferences();
}

#if PLATFORM(TIZEN)
void Ewk_Settings::setDefaultTextEncoding(const char* defaultTextEncoding)
{
    m_defaultTextEncoding = defaultTextEncoding;
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void Ewk_Settings::openFormDataBase()
{
    if (m_viewImpl->ewkContext())
        m_viewImpl->ewkContext()->formDatabase()->open(FormDatabase::defaultDatabaseDirectoryPath(), FormDatabase::defaultDatabaseFilename());
}

void Ewk_Settings::setAutofillPasswordForm(bool enable)
{
    if (enable)
        openFormDataBase();

    m_autofillPasswordForm = enable;
}

void Ewk_Settings::setFormCandidateData(bool enable)
{
    if (enable)
        openFormDataBase();

    m_formCandidateData = enable;
}

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
void Ewk_Settings::setAutofillProfileForm(bool enable)
{
    if (enable)
        openFormDataBase();

    m_autofillProfileForm = enable;
}
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM

#endif
#endif

#if ENABLE(SPELLCHECK)
static struct {
    bool isContinuousSpellCheckingEnabled : 1;
    Vector<String> spellCheckingLanguages;
    Ewk_Settings_Continuous_Spell_Checking_Change_Cb onContinuousSpellChecking;
} ewkTextCheckerSettings = { false, Vector<String>(), 0 };

static Eina_Bool onContinuousSpellCheckingIdler(void*)
{
    if (ewkTextCheckerSettings.onContinuousSpellChecking)
        ewkTextCheckerSettings.onContinuousSpellChecking(ewkTextCheckerSettings.isContinuousSpellCheckingEnabled);

    return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool spellCheckingLanguagesSetUpdate(void*)
{
    // FIXME: Consider to delegate calling of this method in WebProcess to do not delay/block UIProcess.
    Ewk_Text_Checker::updateSpellCheckingLanguages(ewkTextCheckerSettings.spellCheckingLanguages);
    return ECORE_CALLBACK_CANCEL;
}

static void spellCheckingLanguagesSet(const Vector<String>& newLanguages)
{
    ewkTextCheckerSettings.spellCheckingLanguages = newLanguages;
    ecore_idler_add(spellCheckingLanguagesSetUpdate, 0);
}
#endif // ENABLE(SPELLCHECK)

Eina_Bool ewk_settings_fullscreen_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(FULLSCREEN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setFullScreenEnabled(enable);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_settings_fullscreen_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(FULLSCREEN_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return settings->preferences()->fullScreenEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_javascript_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setJavaScriptEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_javascript_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->javaScriptEnabled();
}

Eina_Bool ewk_settings_loads_images_automatically_set(Ewk_Settings* settings, Eina_Bool automatic)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLoadsImagesAutomatically(automatic);

    return true;
}

Eina_Bool ewk_settings_loads_images_automatically_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->loadsImagesAutomatically();
}

#if PLATFORM(TIZEN)
Eina_Bool ewk_settings_plugins_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setPluginsEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_plugins_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->pluginsEnabled();
}

Eina_Bool ewk_settings_auto_fitting_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setAutoFittingEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_auto_fitting_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->autoFittingEnabled();
}

Eina_Bool ewk_settings_force_zoom_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setForceZoomEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_force_zoom_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->forceZoomEnabled();
}

Eina_Bool ewk_settings_font_default_size_set(Ewk_Settings* settings, int size)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setDefaultFontSize(size);

    return true;
}

Eina_Bool ewk_settings_default_font_size_set(Ewk_Settings* settings, int size)
{
    return ewk_settings_font_default_size_set(settings, size);
}

int ewk_settings_font_default_size_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, 0);

    return settings->preferences()->defaultFontSize();
}

int ewk_settings_default_font_size_get(const Ewk_Settings* settings)
{
    return ewk_settings_font_default_size_get(settings);
}

Eina_Bool ewk_settings_scripts_window_open_set(Ewk_Settings* settings, Eina_Bool allow)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setJavaScriptCanOpenWindowsAutomatically(allow);

    return true;
}

Eina_Bool ewk_settings_scripts_can_open_windows_set(Ewk_Settings* settings, Eina_Bool allow)
{
    return ewk_settings_scripts_window_open_set(settings, allow);
}

Eina_Bool ewk_settings_scripts_window_open_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->javaScriptCanOpenWindowsAutomatically();
}

Eina_Bool ewk_settings_scripts_can_open_windows_get(const Ewk_Settings* settings)
{
    return ewk_settings_scripts_window_open_get(settings);
}

Eina_Bool ewk_settings_compositing_borders_visible_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setCompositingBordersVisible(enable);

    return true;
}

Eina_Bool ewk_settings_default_encoding_set(Ewk_Settings* settings, const char* encoding)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setDefaultTextEncodingName(String::fromUTF8(encoding));

    return true;
}

Eina_Bool ewk_settings_default_text_encoding_name_set(Ewk_Settings* settings, const char* encoding)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(encoding, false);

    return ewk_settings_default_encoding_set(settings, encoding);
}

Eina_Bool ewk_settings_is_encoding_valid(const char* encoding)
{
#if ENABLE(TIZEN_ENCODING_VERIFICATION)
    return WebCore::TextEncoding(encoding).isValid();
#else
    return false;
#endif
}

const char* ewk_settings_default_encoding_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, 0);

    const String& encodingString(settings->preferences()->defaultTextEncodingName());
    const_cast<Ewk_Settings*>(settings)->setDefaultTextEncoding(encodingString.utf8().data());

    return settings->defaultTextEncoding();
}

const char* ewk_settings_default_text_encoding_name_get(const Ewk_Settings* settings)
{
    return ewk_settings_default_encoding_get(settings);
}

Eina_Bool ewk_settings_private_browsing_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setPrivateBrowsingEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_private_browsing_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->privateBrowsingEnabled();
}

Eina_Bool ewk_settings_editable_link_behavior_set(Ewk_Settings* settings, Ewk_Editable_Link_Behavior behavior)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setEditableLinkBehavior(static_cast<WKEditableLinkBehavior>(behavior));

    return true;
}

#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
Eina_Bool ewk_settings_load_remote_images_set(Ewk_Settings* settings, Eina_Bool loadRemoteImages)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLoadRemoteImages(loadRemoteImages);

    return true;
}

Eina_Bool ewk_settings_load_remote_images_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->loadRemoteImages();
}
#endif

Eina_Bool ewk_settings_scan_malware_enabled_set(Ewk_Settings* settings, Eina_Bool scanMalwareEnabled)
{
#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setScanMalwareEnabled(scanMalwareEnabled);
    TIZEN_LOGI("Eina_Bool ewk_settings_scan_malware_enabled_set() [%d]", static_cast<int>(scanMalwareEnabled));

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(scanMalwareEnabled);

    return false;
#endif
}

Eina_Bool ewk_settings_scan_malware_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->scanMalwareEnabled();
#else
    UNUSED_PARAM(settings);

    return false;
#endif
}

Eina_Bool ewk_settings_user_agent_whitelist_path_set(Ewk_Settings* settings, char* whitelistPath)
{
#if ENABLE(TIZEN_USER_AGENT_WHITELIST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    if (whitelistPath)
        settings->preferences()->setUserAgentWhitelistPath(String::fromUTF8(whitelistPath));

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(whitelistPath);

    return false;
#endif
}

const char* ewk_settings_user_agent_whitelist_path_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_USER_AGENT_WHITELIST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, NULL);

    return settings->preferences()->userAgentWhitelistPath().utf8().data();
#else
    UNUSED_PARAM(settings);

    return NULL;
#endif
}

Eina_Bool ewk_settings_spdy_enabled_set(Ewk_Settings* settings, Eina_Bool spdyEnabled)
{
#if ENABLE(TIZEN_SOUP_FEATURES)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setSpdyEnabled(spdyEnabled);
    TIZEN_LOGI("Eina_Bool ewk_settings_spdy_enabled_set() [%d]", static_cast<int>(spdyEnabled));

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(spdyEnabled);

    return false;
#endif
}

Eina_Bool ewk_settings_spdy_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_SOUP_FEATURES)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->spdyEnabled();
#else
    UNUSED_PARAM(settings);

    return false;
#endif
}

Eina_Bool ewk_settings_performance_features_enabled_set(Ewk_Settings* settings, Eina_Bool performanceFeaturesEnabled)
{
#if ENABLE(TIZEN_SOUP_FEATURES)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setPerformanceFeaturesEnabled(performanceFeaturesEnabled);
    TIZEN_LOGI("Eina_Bool ewk_settings_performance_features_enabled_set() [%d]", static_cast<int>(performanceFeaturesEnabled));

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(performanceFeaturesEnabled);

    return false;
#endif
}

Eina_Bool ewk_settings_performance_features_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_SOUP_FEATURES)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->performanceFeaturesEnabled();
#else
    UNUSED_PARAM(settings);

    return false;
#endif
}

Eina_Bool ewk_settings_link_effect_enabled_set(Ewk_Settings* settings, Eina_Bool linkEffectEnabled)
{
#if ENABLE(TIZEN_LINK_EFFECT)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setLinkEffectEnabled(linkEffectEnabled);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_settings_link_effect_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_LINK_EFFECT)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->linkEffectEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_uses_encoding_detector_set(Ewk_Settings* settings, Eina_Bool use)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setUsesEncodingDetector(use);

    return true;
}

Eina_Bool ewk_settings_uses_encoding_detector_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->usesEncodingDetector();
}

#if ENABLE(TIZEN_ISF_PORT)
Eina_Bool ewk_settings_default_keypad_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setEnableDefaultKeypad(enable);

    return true;
}

Eina_Bool ewk_settings_default_keypad_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->defaultKeypadEnabled();
}

Eina_Bool ewk_settings_uses_keypad_without_user_action_set(Ewk_Settings* settings, Eina_Bool use)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setUsesKeypadWithoutUserAction(use);

    return true;
}

Eina_Bool ewk_settings_uses_keypad_without_user_action_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, true);

    return settings->preferences()->usesKeypadWithoutUserAction();
}
#endif

Eina_Bool ewk_settings_frame_flattening_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setFrameFlatteningEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_frame_flattening_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->frameFlatteningEnabled();
}

#if ENABLE(TIZEN_WEBKIT2_TEXT_ZOOM)
Eina_Bool ewk_settings_text_zoom_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setTextZoomEnabled(enable);

    return true;
}

Eina_Bool ewk_settings_text_zoom_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->textZoomEnabled();
}
#endif

Eina_Bool ewk_settings_autofill_password_form_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->setAutofillPasswordForm(enable);
    return true;
#endif
    return false;
}

Eina_Bool ewk_settings_autofill_password_form_enabled_get(Ewk_Settings* settings)
{
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return settings->autofillPasswordForm();
#endif
    return false;
}

Eina_Bool ewk_settings_form_candidate_data_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->setFormCandidateData(enable);
    return true;
#endif
    return false;
}

Eina_Bool ewk_settings_form_candidate_data_enabled_get(Ewk_Settings* settings)
{
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return settings->formCandidateData();
#endif
    return false;
}

Eina_Bool ewk_settings_form_profile_data_enabled_set(Ewk_Settings *settings, Eina_Bool enable)
{
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->setAutofillProfileForm(enable);
    return true;
#endif
    return false;
}

Eina_Bool ewk_settings_form_profile_data_enabled_get(const Ewk_Settings *settings)
{
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return settings->autofillProfileForm();
#endif
    return false;
}

Eina_Bool ewk_settings_legacy_font_size_enabled_set(Ewk_Settings *settings, Eina_Bool enable)
{
#if ENABLE(TIZEN_FONT_SIZE_CSS_STYLING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setLegacyFontSizeEnabled(enable);
    return true;
#else
    return false;
#endif
}

EAPI Eina_Bool ewk_settings_legacy_font_size_enabled_get(const Ewk_Settings *settings)
{
#if ENABLE(TIZEN_FONT_SIZE_CSS_STYLING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return settings->preferences()->legacyFontSizeEnabled();
#else
    return true;
#endif
}

Eina_Bool ewk_settings_current_legacy_font_size_mode_set(Ewk_Settings *settings, Ewk_Legacy_Font_Size_Mode mode)
{
#if ENABLE(TIZEN_FONT_SIZE_CSS_STYLING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return ewk_settings_legacy_font_size_enabled_set(settings, mode == EWK_LEGACY_FONT_SIZE_MODE_NEVER ? false : true);
#else
    return false;
#endif
}

EAPI Ewk_Legacy_Font_Size_Mode ewk_settings_current_legacy_font_size_mode_get(const Ewk_Settings *settings)
{
#if ENABLE(TIZEN_FONT_SIZE_CSS_STYLING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EWK_LEGACY_FONT_SIZE_MODE_ONLY_IF_PIXEL_VALUES_MATCH);
    return settings->preferences()->legacyFontSizeEnabled() ? EWK_LEGACY_FONT_SIZE_MODE_ONLY_IF_PIXEL_VALUES_MATCH : EWK_LEGACY_FONT_SIZE_MODE_NEVER;
#else
    return EWK_LEGACY_FONT_SIZE_MODE_ONLY_IF_PIXEL_VALUES_MATCH;
#endif
}

Eina_Bool ewk_settings_paste_image_uri_mode_set(Ewk_Settings *settings, Eina_Bool enable)
{
#if ENABLE(TIZEN_PASTE_IMAGE_URI)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setPasteImageUriEnabled(enable);
    return true;
#endif
    return false;
}

Eina_Bool ewk_settings_paste_image_uri_mode_get(const Ewk_Settings *settings)
{
#if ENABLE(TIZEN_PASTE_IMAGE_URI)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    return settings->preferences()->pasteImageUriEnabled();
#endif
    return false;
}

#endif // if PLATFORM(TIZEN)

void ewk_settings_continuous_spell_checking_change_cb_set(Ewk_Settings_Continuous_Spell_Checking_Change_Cb callback)
{
#if ENABLE(SPELLCHECK)
    ewkTextCheckerSettings.onContinuousSpellChecking = callback;
#endif
}

Eina_Bool ewk_settings_continuous_spell_checking_enabled_get()
{
#if ENABLE(SPELLCHECK)
    return ewkTextCheckerSettings.isContinuousSpellCheckingEnabled;
#else
    return false;
#endif
}

void ewk_settings_continuous_spell_checking_enabled_set(Eina_Bool enable)
{
#if ENABLE(SPELLCHECK)
    enable = !!enable;
    if (ewkTextCheckerSettings.isContinuousSpellCheckingEnabled != enable) {
        ewkTextCheckerSettings.isContinuousSpellCheckingEnabled = enable;

        WKTextCheckerContinuousSpellCheckingEnabledStateChanged(enable);

        // Sets the default language if user didn't specify any.
        if (enable && !Ewk_Text_Checker::hasDictionary())
            spellCheckingLanguagesSet(Vector<String>());

        if (ewkTextCheckerSettings.onContinuousSpellChecking)
            ecore_idler_add(onContinuousSpellCheckingIdler, 0);
    }
#endif
}

Eina_List* ewk_settings_spell_checking_available_languages_get()
{
    Eina_List* listOflanguages = 0;
#if ENABLE(SPELLCHECK)
    const Vector<String>& languages = Ewk_Text_Checker::availableSpellCheckingLanguages();
    size_t numberOfLanuages = languages.size();

    for (size_t i = 0; i < numberOfLanuages; ++i)
        listOflanguages = eina_list_append(listOflanguages, eina_stringshare_add(languages[i].utf8().data()));
#endif
    return listOflanguages;
}

void ewk_settings_spell_checking_languages_set(const char* languages)
{
#if ENABLE(SPELLCHECK)
    Vector<String> newLanguages;
    String::fromUTF8(languages).split(',', newLanguages);

    spellCheckingLanguagesSet(newLanguages);
#endif
}

Eina_List* ewk_settings_spell_checking_languages_get()
{
    Eina_List* listOflanguages = 0;
#if ENABLE(SPELLCHECK)
    Vector<String> languages = Ewk_Text_Checker::loadedSpellCheckingLanguages();
    size_t numberOfLanuages = languages.size();

    for (size_t i = 0; i < numberOfLanuages; ++i)
        listOflanguages = eina_list_append(listOflanguages, eina_stringshare_add(languages[i].utf8().data()));

#endif
    return listOflanguages;
}

Eina_Bool ewk_settings_text_selection_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    settings->setTextSelectionEnabled(enable);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_settings_text_selection_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    return settings->textSelectionEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_clear_text_selection_automatically_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    settings->setAutoClearTextSelection(enable);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_settings_clear_text_selection_automatically_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    return settings->autoClearTextSelection();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_text_autosizing_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
#if ENABLE(TEXT_AUTOSIZING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setTextAutosizingEnabled(enable);
    
    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(enable);
    return false;
#endif
}

Eina_Bool ewk_settings_text_autosizing_enabled_get(const Ewk_Settings* settings)
{
#if ENABLE(TEXT_AUTOSIZING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->textAutosizingEnabled();
#else
    UNUSED_PARAM(settings);
    return false;
#endif
}

Eina_Bool ewk_settings_text_autosizing_font_scale_factor_set(Ewk_Settings* settings, double factor)
{
#if ENABLE(TIZEN_TEXT_AUTOSIZING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setTextAutosizingFontScaleFactor(factor);

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(factor);
    return false;
#endif
}

double ewk_settings_text_autosizing_font_scale_factor_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_TEXT_AUTOSIZING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    return settings->preferences()->textAutosizingFontScaleFactor();
#else
    UNUSED_PARAM(settings);
    return 1.0;
#endif
}

Eina_Bool ewk_settings_edge_effect_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_EDGE_SUPPORT)
    settings->setEdgeEffectEnabled(enable);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_settings_edge_effect_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_EDGE_SUPPORT)
    return settings->edgeEffectEnabled();
#else
    return false;
#endif
}

void ewk_settings_text_style_state_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    EINA_SAFETY_ON_NULL_RETURN(settings);
    settings->preferences()->setTextStyleStateEnabled(enabled);
#endif
}

Eina_Bool ewk_settings_text_style_state_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    return settings->preferences()->textStyleStateEnabled();
#else
    return false;
#endif
}

Eina_Bool ewk_settings_select_word_automatically_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    settings->setAutoSelectWord(enable);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_settings_select_word_automatically_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    return settings->autoSelectWord();
#else
    return false;
#endif
}

Ewk_List_Style_Position ewk_settings_initial_list_style_position_get(const Ewk_Settings* settings)
{
#if ENABLE(TIZEN_LIST_STYLE_POSITION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, EWK_LIST_STYLE_POSITION_OUTSIDE);
    return static_cast<Ewk_List_Style_Position>(settings->preferences()->listStylePosition());
#else
    return EWK_LIST_STYLE_POSITION_OUTSIDE;
#endif
}

Eina_Bool ewk_settings_initial_list_style_position_set(Ewk_Settings* settings, Ewk_List_Style_Position style)
{
#if ENABLE(TIZEN_LIST_STYLE_POSITION)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setListStylePosition(static_cast<WKListStylePosition>(style));
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_settings_webkit_text_size_adjust_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
#if ENABLE(TIZEN_DISABLE_WEBKIT_TEXT_SIZE_ADJUST)
    return settings->preferences()->webkitTextSizeAdjustEnabled();
#else
    return true;
#endif
}

Eina_Bool ewk_settings_webkit_text_size_adjust_enabled_set(Ewk_Settings* settings, Eina_Bool enabled)
{
#if ENABLE(TIZEN_DISABLE_WEBKIT_TEXT_SIZE_ADJUST)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    settings->preferences()->setWebkitTextSizeAdjustEnabled(enabled);

    return true;
#else
    return false;
#endif
}

void ewk_settings_selection_handle_enabled_set(Ewk_Settings* settings, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN(settings);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    settings->setSelectionHandleEnabled(enable);
#endif
}

Eina_Bool ewk_settings_selection_handle_enabled_get(const Ewk_Settings* settings)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    return settings->selectionHandleEnabled();
#else
    return false;
#endif
}

void ewk_settings_extra_feature_set(Ewk_Settings* settings, const char* feature, Eina_Bool enable)
{
    EINA_SAFETY_ON_NULL_RETURN(settings);
    EINA_SAFETY_ON_NULL_RETURN(feature);

#if PLATFORM(TIZEN)
    String featureName = String::fromUTF8(feature);

#if ENABLE(TIZEN_LINK_MAGNIFIER)
    if (equalIgnoringCase(featureName, "link,magnifier")) {
        settings->preferences()->setLinkMagnifierEnabled(enable);
        return;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_PREVENT_TEXT_SELECTION_MAGNIFIER)
    if (equalIgnoringCase(featureName, "selection,magnifier")) {
        settings->setSelectionMagnifierEnabled(enable);
        return;
    }
#endif

#if ENABLE(TIZEN_PROD_DETECT_CONTENTS)
    if (equalIgnoringCase(featureName, "detect,contents")) {
        settings->preferences()->setDetectContentsAutomatically(enable);
        return;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_PREVENT_LONG_PRESS)
    if (equalIgnoringCase(featureName, "longpress,enable")) {
        settings->setLongPressEnabled(enable);
        return;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_PREVENT_DOUBLE_TAP)
    if (equalIgnoringCase(featureName, "doubletap,enable")) {
        settings->setDoubleTapEnabled(enable);
        return;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_PREVENT_ZOOM)
    if (equalIgnoringCase(featureName, "zoom,enable")) {
        settings->setZoomEnabled(enable);
        return;
    }
#endif

#if ENABLE(TIZEN_OPEN_PANEL)
    if (equalIgnoringCase(featureName, "openpanel,enable")) {
        settings->setOpenPanelEnabled(enable);
        return;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    if (equalIgnoringCase(featureName, "urlbar,hide")) {
        settings->setHideUrlBarEnabled(enable);
        return;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
    if (equalIgnoringCase(featureName, "touch,focus")) {
        settings->setTouchFocusEnabled(enable);
        return;
    }
#endif

#if ENABLE(TIZEN_EDGE_SUPPORT)
    if (equalIgnoringCase(featureName, "edge,enable")) {
        settings->setEdgeEffectEnabled(enable);
        return;
    }
#endif

    if (equalIgnoringCase(featureName, "allow,universalaccesslocal")) {
        settings->preferences()->setAllowUniversalAccessFromFileURLs(enable);
        return;
    }

    if (equalIgnoringCase(featureName, "allow,fileaccesslocal")) {
        settings->preferences()->setAllowFileAccessFromFileURLs(enable);
        return;
    }
#endif
}

Eina_Bool ewk_settings_extra_feature_get(const Ewk_Settings* settings, const char* feature)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(feature, false);

#if PLATFORM(TIZEN)
    String featureName = String::fromUTF8(feature);

#if ENABLE(TIZEN_LINK_MAGNIFIER)
    if (equalIgnoringCase(featureName, "link,magnifier"))
        return settings->preferences()->linkMagnifierEnabled();
#endif

#if ENABLE(TIZEN_WEBKIT2_PREVENT_TEXT_SELECTION_MAGNIFIER)
    if (equalIgnoringCase(featureName, "selection,magnifier"))
        return settings->selectionMagnifierEnabled();
#endif

#if ENABLE(TIZEN_PROD_DETECT_CONTENTS)
    if (equalIgnoringCase(featureName, "detect,contents"))
        return settings->preferences()->detectContentsAutomatically();
#endif

#if ENABLE(TIZEN_WEBKIT2_PREVENT_LONG_PRESS)
    if (equalIgnoringCase(featureName, "longpress,enable"))
        return settings->longPressEnabled();
#endif

#if ENABLE(TIZEN_WEBKIT2_PREVENT_DOUBLE_TAP)
    if (equalIgnoringCase(featureName, "doubletap,enable"))
        return settings->doubleTapEnabled();
#endif

#if ENABLE(TIZEN_WEBKIT2_PREVENT_ZOOM)
    if (equalIgnoringCase(featureName, "zoom,enable"))
        return settings->zoomEnabled();
#endif

#if ENABLE(TIZEN_OPEN_PANEL)
    if (equalIgnoringCase(featureName, "openpanel,enable"))
        return settings->openPanelEnabled();
#endif

#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    if (equalIgnoringCase(featureName, "urlbar,hide"))
        return settings->hideUrlBarEnabled();
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
    if (equalIgnoringCase(featureName, "touch,focus"))
        return settings->touchFocusEnabled();
#endif

#if ENABLE(TIZEN_EDGE_SUPPORT)
    if (equalIgnoringCase(featureName, "edge,enable"))
        return settings->edgeEffectEnabled();
#endif

    if (equalIgnoringCase(featureName, "allow,universalaccesslocal")) {
        return settings->preferences()->allowUniversalAccessFromFileURLs();
    }

    if (equalIgnoringCase(featureName, "allow,fileaccesslocal")) {
        return settings->preferences()->allowFileAccessFromFileURLs();
    }
#endif

    return false;
}

Eina_Bool ewk_settings_use_system_font_set(Ewk_Settings* settings, Eina_Bool use)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

#if ENABLE(TIZEN_USE_SETTINGS_FONT)
    settings->setUseSystemFont(use);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_settings_text_autosizing_scale_factor_set(Ewk_Settings* settings, double factor)
{
#if ENABLE(TIZEN_TEXT_AUTOSIZING)
    EINA_SAFETY_ON_NULL_RETURN_VAL(settings, false);

    settings->preferences()->setTextAutosizingFontScaleFactor(factor);

    return true;
#else
    UNUSED_PARAM(settings);
    UNUSED_PARAM(factor);
    return false;
#endif
}
#if ENABLE(TIZEN_USE_SETTINGS_FONT)
void Ewk_Settings::setUseSystemFont(bool use)
{
    if (use)
        m_viewImpl->pageProxy->useSettingsFont();
    else
        m_viewImpl->pageProxy->setBrowserFont();

    m_useSystemFont = use;
}
#endif
