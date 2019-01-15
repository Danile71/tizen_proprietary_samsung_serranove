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

#ifndef ewk_settings_private_h
#define ewk_settings_private_h

#include <wtf/PassOwnPtr.h>

#if PLATFORM(TIZEN)
#include "WKEinaSharedString.h"
#endif

namespace WebKit {
class WebPreferences;
}
class EwkViewImpl;

/**
 * \struct  Ewk_Settings
 * @brief   Contains the settings data.
 */
class Ewk_Settings {
public:
    static PassOwnPtr<Ewk_Settings> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new Ewk_Settings(viewImpl));
    }

    const WebKit::WebPreferences* preferences() const;
    WebKit::WebPreferences* preferences();

#if PLATFORM(TIZEN)
    const char* defaultTextEncoding() const { return m_defaultTextEncoding; }
    void setDefaultTextEncoding(const char*);
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    void openFormDataBase();
    bool autofillPasswordForm() const { return m_autofillPasswordForm; }
    void setAutofillPasswordForm(bool);
    bool formCandidateData() const { return m_formCandidateData; }
    void setFormCandidateData(bool);
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    bool autofillProfileForm() const { return m_autofillProfileForm; }
    void setAutofillProfileForm(bool);
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    bool textSelectionEnabled() const { return m_textSelectionEnabled; }
    void setTextSelectionEnabled(bool enable) { m_textSelectionEnabled = enable; }
    bool autoClearTextSelection() const { return m_autoClearTextSelection; }
    void setAutoClearTextSelection(bool enable) { m_autoClearTextSelection = enable; }
    bool autoSelectWord() const { return m_autoSelectWord; }
    void setAutoSelectWord(bool enable) { m_autoSelectWord = enable; }
    bool selectionHandleEnabled() const { return m_selectionHandleEnabled; }
    void setSelectionHandleEnabled(bool enable) { m_selectionHandleEnabled = enable; }
#endif
#if ENABLE(TIZEN_EDGE_SUPPORT)
    bool edgeEffectEnabled() const { return m_edgeEffectEnabled; }
    void setEdgeEffectEnabled(bool enable) { m_edgeEffectEnabled = enable; }
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_LONG_PRESS)
    bool longPressEnabled() const { return m_longPressEnabled; }
    void setLongPressEnabled(bool enable) { m_longPressEnabled = enable; }
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_DOUBLE_TAP)
    bool doubleTapEnabled() const { return m_doubleTapEnabled; }
    void setDoubleTapEnabled(bool enable) { m_doubleTapEnabled = enable; }
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_ZOOM)
    bool zoomEnabled() const { return m_zoomEnabled; }
    void setZoomEnabled(bool enable) { m_zoomEnabled = enable; }
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_TEXT_SELECTION_MAGNIFIER)
    bool selectionMagnifierEnabled() const { return m_selectionMagnifier; }
    void setSelectionMagnifierEnabled(bool enable) { m_selectionMagnifier = enable; }
#endif
#if ENABLE(TIZEN_OPEN_PANEL)
    bool openPanelEnabled() const { return m_openPanelEnabled; }
    void setOpenPanelEnabled(bool enable) { m_openPanelEnabled = enable;}
#endif
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    bool hideUrlBarEnabled() const { return m_hideUrlBar; }
    void setHideUrlBarEnabled(bool enable) { m_hideUrlBar = enable; }
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
    bool touchFocusEnabled() const { return m_touchFocus; }
    void setTouchFocusEnabled(bool enable) { m_touchFocus = enable; }
#endif
#endif

#if ENABLE(TIZEN_USE_SETTINGS_FONT)
    bool isUseSystemFont() const { return m_useSystemFont; }
    void setUseSystemFont(bool use);
#endif
private:
    explicit Ewk_Settings(EwkViewImpl* viewImpl)
        : m_viewImpl(viewImpl)
#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
        , m_autofillPasswordForm(false)
        , m_formCandidateData(false)
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
        , m_autofillProfileForm(false)
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        , m_textSelectionEnabled(true)
        , m_autoClearTextSelection(true)
        , m_autoSelectWord(false)
        , m_selectionHandleEnabled(true)
#endif
#if ENABLE(TIZEN_EDGE_SUPPORT)
        , m_edgeEffectEnabled(true)
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_LONG_PRESS)
        , m_longPressEnabled(true)
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_DOUBLE_TAP)
        , m_doubleTapEnabled(true)
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_ZOOM)
        , m_zoomEnabled(true)
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_TEXT_SELECTION_MAGNIFIER)
    , m_selectionMagnifier(true)
#endif
#if ENABLE(TIZEN_OPEN_PANEL)
        , m_openPanelEnabled(true)
#endif
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
        , m_hideUrlBar(false)
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
        , m_touchFocus(true)
#endif
#endif
    {
        ASSERT(m_viewImpl);
    }

    EwkViewImpl* m_viewImpl;

#if PLATFORM(TIZEN)
    WKEinaSharedString m_defaultTextEncoding;
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    bool m_autofillPasswordForm;
    bool m_formCandidateData;
    bool m_autofillProfileForm;
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    bool m_textSelectionEnabled;
    bool m_autoClearTextSelection;
    bool m_autoSelectWord;
    bool m_selectionHandleEnabled;
#endif
#if ENABLE(TIZEN_EDGE_SUPPORT)
    bool m_edgeEffectEnabled;
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_LONG_PRESS)
    bool m_longPressEnabled;
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_DOUBLE_TAP)
    bool m_doubleTapEnabled;
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_ZOOM)
    bool m_zoomEnabled;
#endif
#if ENABLE(TIZEN_WEBKIT2_PREVENT_TEXT_SELECTION_MAGNIFIER)
    bool m_selectionMagnifier;
#endif
#if ENABLE(TIZEN_OPEN_PANEL)
    bool m_openPanelEnabled;
#endif
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    bool m_hideUrlBar;
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
    bool m_touchFocus;
#endif
#endif
#if ENABLE(TIZEN_USE_SETTINGS_FONT)
    bool m_useSystemFont;
#endif
};

#endif // ewk_settings_private_h
