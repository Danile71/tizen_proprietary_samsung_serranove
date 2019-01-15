/*
   Copyright (C) 2011 Samsung Electronics
   Copyright (C) 2012 Intel Corporation. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "InputMethodContextEfl.h"

#include "EwkViewImpl.h"
#include "WebPageProxy.h"
#include <Ecore_Evas.h>
#include <Ecore_IMF_Evas.h>

#if ENABLE(TIZEN_BROWSER_DASH_MODE)
#include <TizenSystemUtilities.h>
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#include <media_key.h>
#include <utilX.h>
#endif

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_ISF_PORT)
const unsigned InputMethodContextEfl::maxContextSize = 10;
Ecore_X_Atom InputMethodContextEfl::s_externalKeyboardProperty = 0;
bool InputMethodContextEfl::s_shouldUseExternalKeyboard = false;
bool InputMethodContextEfl::s_isSystemKeypadShow = false;

void InputMethodContextEfl::initializeExternalKeyboard()
{
    s_externalKeyboardProperty = ecore_x_atom_get("HW Keyboard Input Started");
    if (!s_externalKeyboardProperty)
        return;

    Ecore_X_Window rootWin = ecore_x_window_root_first_get();
    ecore_x_event_mask_set(rootWin, ECORE_X_EVENT_MASK_WINDOW_PROPERTY);

    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, windowPropertyChanged, 0);

    unsigned int value;
    if (ecore_x_window_prop_card32_get(rootWin, s_externalKeyboardProperty, &value, 1) <= 0)
        return;

    s_shouldUseExternalKeyboard = value;

    if (ecore_x_e_virtual_keyboard_state_get(rootWin) == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON)
        s_isSystemKeypadShow = true;
}

Eina_Bool InputMethodContextEfl::windowPropertyChanged(void*, int, void* event)
{
    Ecore_X_Event_Window_Property* propertyEvent = static_cast<Ecore_X_Event_Window_Property*>(event);

    if (propertyEvent->atom == s_externalKeyboardProperty) {
        unsigned int value;
        if (ecore_x_window_prop_card32_get(propertyEvent->win, s_externalKeyboardProperty, &value, 1) <= 0 || s_shouldUseExternalKeyboard == value)
            return ECORE_CALLBACK_PASS_ON;

        s_shouldUseExternalKeyboard = value;

#if ENABLE(TIZEN_FOCUS_UI)
        if (!s_shouldUseExternalKeyboard) {
            Vector<RefPtr<WebPageProxy> > pages;
            EwkViewImpl::pages(pages);

            for (size_t i = 0; i < pages.size(); ++i)
                pages[i]->setFocusUIEnabled(false);
        }
#endif
    } else if (propertyEvent->atom == ECORE_X_ATOM_E_VIRTUAL_KEYBOARD_STATE) {
        if (ecore_x_e_virtual_keyboard_state_get(propertyEvent->win) == ECORE_X_VIRTUAL_KEYBOARD_STATE_ON)
            s_isSystemKeypadShow = true;
        else
            s_isSystemKeypadShow = false;
    }

    return ECORE_CALLBACK_PASS_ON;
}
#endif

InputMethodContextEfl::InputMethodContextEfl(EwkViewImpl* viewImpl, PassOwnPtr<Ecore_IMF_Context> context)
    : m_viewImpl(viewImpl)
    , m_context(context)
    , m_focused(false)
#if ENABLE(TIZEN_ISF_PORT)
    , m_contextID(0)
    , m_state(ECORE_IMF_INPUT_PANEL_STATE_HIDE)
    , m_inputPickerType(-1)
    , m_doNotHandleFakeKeyEvent(false)
    , m_doNothingOnIMFReset(false)
    , m_fakeKeyEventTimer(this, &InputMethodContextEfl::fakeKeyEventTimerFired)
    , m_updateTextInputStateByUserActionTimer(this, &InputMethodContextEfl::updateTextInputStateByUserActionTimerFired)
    , m_approximateCursorPosition(0)
    , m_isLastKeyEventFiltered(false)
    , m_preeditOccurred(false)
#endif
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    , m_countInputMethod(0)
    , m_countDashMode(0)
    , m_isDashMode(0)
    , m_inputMethodDashModeTimer(this, &InputMethodContextEfl::inputMethodDashModeFired)
    , m_updateInputMethodTimer(this, &InputMethodContextEfl::updateInputMethodFired)
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    , m_mdiaKeyGrabbed(false)
    , m_mediaKeyUpHandler(0)
#endif
{
#if ENABLE(TIZEN_ISF_PORT)
    if (!s_externalKeyboardProperty)
        initializeExternalKeyboard();
#else
    ASSERT(context);
    ecore_imf_context_event_callback_add(m_context.get(), ECORE_IMF_CALLBACK_PREEDIT_CHANGED, onIMFPreeditSequenceChanged, this);
    ecore_imf_context_event_callback_add(m_context.get(), ECORE_IMF_CALLBACK_COMMIT, onIMFInputSequenceComplete, this);
#endif
}

InputMethodContextEfl::~InputMethodContextEfl()
{
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    if (m_inputMethodDashModeTimer.isActive())
        m_inputMethodDashModeTimer.stop();
    if (m_updateInputMethodTimer.isActive())
        m_updateInputMethodTimer.stop();
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    ungrabMediaKey();
#endif

}

#if ENABLE(TIZEN_ISF_PORT)
void InputMethodContextEfl::onIMFInputPanelStateChanged(void* data, Ecore_IMF_Context*, int state)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);

    inputMethodContext->setState(state);

    if (state == ECORE_IMF_INPUT_PANEL_STATE_HIDE) {
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
        if (inputMethodContext->m_viewImpl->pageClient->isClipboardWindowOpened())
            inputMethodContext->m_viewImpl->pageClient->closeClipboardWindow();
#endif

        inputMethodContext->resetIMFContext();

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    Ewk_View_Smart_Data* smartData = static_cast<Ewk_View_Smart_Data*>(evas_object_smart_data_get(inputMethodContext->m_viewImpl->view()));
    if(smartData->api->formdata_candidate_is_showing(smartData))
        smartData->api->formdata_candidate_hide(smartData);
#endif

        evas_object_smart_callback_call(inputMethodContext->m_viewImpl->view(), "editorclient,ime,closed", 0);
    } else if (state == ECORE_IMF_INPUT_PANEL_STATE_SHOW)
        evas_object_smart_callback_call(inputMethodContext->m_viewImpl->view(), "editorclient,ime,opened", 0);
}

void InputMethodContextEfl::onIMFInputPanelGeometryChanged(void* data, Ecore_IMF_Context*, int)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!inputMethodContext->m_context)
        return;

    Eina_Rectangle rect;
    ecore_imf_context_input_panel_geometry_get(inputMethodContext->m_context.get(), &rect.x, &rect.y, &rect.w, &rect.h);
    evas_object_smart_callback_call(inputMethodContext->m_viewImpl->view(), "inputmethod,changed", &rect);

    inputMethodContext->setIMERect(IntRect(rect.x, rect.y, rect.w, rect.h));
}

void InputMethodContextEfl::onIMFCandidatePanelStateChanged(void* data, Ecore_IMF_Context*, int state)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);

    if (state == ECORE_IMF_CANDIDATE_PANEL_SHOW)
        evas_object_smart_callback_call(inputMethodContext->m_viewImpl->view(), "editorclient,candidate,opened", 0);
    else
        evas_object_smart_callback_call(inputMethodContext->m_viewImpl->view(), "editorclient,candidate,closed", 0);
}

void InputMethodContextEfl::onIMFCandidatePanelGeometryChanged(void* data, Ecore_IMF_Context*, int)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!inputMethodContext->m_context)
        return;

    Eina_Rectangle rect;
    ecore_imf_context_candidate_panel_geometry_get(inputMethodContext->m_context.get(), &rect.x, &rect.y, &rect.w, &rect.h);
    evas_object_smart_callback_call(inputMethodContext->m_viewImpl->view(), "editorclient,candidate,changed", &rect);
}

Eina_Bool InputMethodContextEfl::onIMFRetrieveSurrounding(void* data, Ecore_IMF_Context*, char** text, int* offset)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!inputMethodContext->m_viewImpl->page()->focusedFrame() || !inputMethodContext->m_focused || (!text && !offset))
        return false;

    const EditorState& editor = inputMethodContext->m_viewImpl->page()->editorState();

    if (editor.isInPasswordField)
        return false;

    if (text) {
        CString utf8Text;
        if (!inputMethodContext->m_approximateSurroundingText.isNull())
            utf8Text = inputMethodContext->m_approximateSurroundingText.utf8();
        else
            utf8Text = editor.surroundingText.utf8();
        size_t length = utf8Text.length();

        *text = static_cast<char*>(malloc((length + 1) * sizeof(char)));
        if (!(*text))
            return false;

        if (length)
            strncpy(*text, utf8Text.data(), length);
        (*text)[length] = 0;
    }

    if (offset) {
        if (!inputMethodContext->m_approximateSurroundingText.isNull())
            *offset = inputMethodContext->m_approximateCursorPosition;
        else
            *offset = editor.cursorPosition;
    }

    return true;
}

void InputMethodContextEfl::onIMFDeleteSurrounding(void* data, Ecore_IMF_Context*, void* eventInfo)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!eventInfo || !inputMethodContext->m_viewImpl->page()->focusedFrame() || !inputMethodContext->m_focused)
        return;

    inputMethodContext->requestFakeKeyEvent();

    Ecore_IMF_Event_Delete_Surrounding* event = static_cast<Ecore_IMF_Event_Delete_Surrounding*>(eventInfo);
    inputMethodContext->m_viewImpl->page()->deleteSurroundingText(event->offset, event->n_chars);

    inputMethodContext->updateApproximateText(String(), event->offset, event->n_chars);
}

void InputMethodContextEfl::onIMFInputSequenceComplete(void* data, Ecore_IMF_Context*, void* eventInfo)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!eventInfo || !inputMethodContext->m_focused)
        return;


#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    inputMethodContext->checkFrequencyOfInputMethod();
#endif

    if (inputMethodContext->m_doNothingOnIMFReset)
        return;

    inputMethodContext->requestFakeKeyEvent();

#if ENABLE(TIZEN_KEYPRESS_KEYCODE_FIX)
    inputMethodContext->preeditString = "";
#endif

    String completeString = String::fromUTF8(static_cast<char*>(eventInfo));
    inputMethodContext->m_viewImpl->page()->confirmComposition(completeString);

    inputMethodContext->updateApproximateText(String(), 0, 0);
}

void InputMethodContextEfl::onIMFPreeditSequenceChanged(void* data, Ecore_IMF_Context* context, void*)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);

    if (!inputMethodContext->m_viewImpl->page()->focusedFrame() || !inputMethodContext->m_focused)
        return;

    WebPageProxy* page = inputMethodContext->m_viewImpl->page();
    if (!page->focusedFrame())
        return;

#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    inputMethodContext->checkFrequencyOfInputMethod();
#endif

    inputMethodContext->requestFakeKeyEvent();

    const EditorState& editor = inputMethodContext->m_viewImpl->page()->editorState();
    PageClientImpl* pageClient = inputMethodContext->m_viewImpl->pageClient.get();
    IntRect caretRect;
    if (!editor.selectionIsNone && !editor.selectionIsRange) {
        caretRect = editor.selectionRect;
        caretRect.scale(pageClient->scaleFactor());
    }

    int viewX, viewY;
    evas_object_geometry_get(inputMethodContext->m_viewImpl->view(), &viewX, &viewY, 0, 0);

    int x = caretRect.x() - pageClient->scrollPosition().x() + viewX;
    int y = caretRect.y() - pageClient->scrollPosition().y() + viewY;
    int w = caretRect.width();
    int h = caretRect.height();
    ecore_imf_context_cursor_location_set(context, x, y, w, h);

    char* buffer = 0;
    Eina_List* preeditAttrs = 0;
    int cursorPosition = 0;

    ecore_imf_context_preedit_string_with_attributes_get(context, &buffer, &preeditAttrs, &cursorPosition);

#if ENABLE(TIZEN_KEYPRESS_KEYCODE_FIX)
    String & preeditString = inputMethodContext->preeditString;
#else
    String preeditString;
#endif
    preeditString = String::fromUTF8(buffer);
    Vector<CompositionUnderline> underlines;

    if (preeditAttrs) {
        void* item = 0;
        EINA_LIST_FREE(preeditAttrs, item)
            free(item);
    }

    if (underlines.isEmpty())
        underlines.append(CompositionUnderline(0, preeditString.length(), Color(0, 0, 0), false));

    page->setComposition(preeditString, underlines, cursorPosition);

    if (buffer)
        free(buffer);

    inputMethodContext->updateApproximateText(preeditString, 0, 0);
    inputMethodContext->m_preeditOccurred = true;
}

#if ENABLE(TIZEN_BROWSER_DASH_MODE)
static double timeDiffAPI(struct timeval *present, struct timeval *prev)
{
   return (((present->tv_sec + present->tv_usec / 1000000.0) -(prev->tv_sec + prev->tv_usec / 1000000.0)) * 1000);
}
void InputMethodContextEfl::checkFrequencyOfInputMethod()
{
    ++m_countInputMethod;

#if ENABLE(TIZEN_LITE_ENHANCED_KEYEVENT)
    Ewk_Settings* settings = ewk_view_settings_get(m_viewImpl->view());
    if (settings && settings->enhancedKeyEventEnabled() && !m_isDashMode) {
        struct timeval present;
        static struct timeval previous;
        double presentTimeDiff = 0;
        static double prevTimeDiff = 0;
        gettimeofday(&present, 0);
        presentTimeDiff = timeDiffAPI(&present, &previous);
        previous = present;

        if (presentTimeDiff >10 && presentTimeDiff < 150 && prevTimeDiff > 10 && prevTimeDiff < 150 ) {
            m_viewImpl->page()->process()->send(Messages::WebPage::SuspendJavaScript(), m_viewImpl->page()->pageID());
            m_isDashMode = true;
        }
        prevTimeDiff = presentTimeDiff;

    } else {
#endif
        if (!m_updateInputMethodTimer.isActive() && !m_isDashMode) 
            m_updateInputMethodTimer.startOneShot(0.5);
#if ENABLE(TIZEN_LITE_ENHANCED_KEYEVENT)
    }
#endif
    if (m_isDashMode && !m_inputMethodDashModeTimer.isActive()) {
        WTF::setWebkitDashMode(WTF::DashModeBrowserJavaScript, 3000);
        WTF::setWebkitDashMode(WTF::DashModeGpuBoost, 3000);
        m_inputMethodDashModeTimer.startOneShot(2.9);
        ++m_countDashMode;
    }
}

void InputMethodContextEfl::updateInputMethodFired(Timer<InputMethodContextEfl>*)
{
    const int kInputMethodRepeatThreshold = 2;
    if (!m_isDashMode) {
        if (m_countInputMethod >= kInputMethodRepeatThreshold) {
            m_isDashMode = true;
        }
        m_countInputMethod = 0;
    }
}

void InputMethodContextEfl::inputMethodDashModeFired(WebCore::Timer<InputMethodContextEfl>*)
{
    const int kDashModeRepeatThreshold = 4;
    const int kFirstDashModeInputThreshold = 20;

    if (m_countDashMode < kDashModeRepeatThreshold && m_countInputMethod > kFirstDashModeInputThreshold) {
        WTF::setWebkitDashMode(WTF::DashModeBrowserJavaScript, 3000);
        WTF::setWebkitDashMode(WTF::DashModeGpuBoost, 3000);
        m_inputMethodDashModeTimer.startOneShot(2.9);
        ++m_countDashMode;
    } else {
        m_isDashMode = false;
        m_countDashMode = 0;
        m_countInputMethod = 0;
#if ENABLE(TIZEN_LITE_ENHANCED_KEYEVENT)
        Ewk_Settings* settings = ewk_view_settings_get(m_viewImpl->view());
        if (settings && settings->enhancedKeyEventEnabled())
            m_viewImpl->page()->process()->send(Messages::WebPage::ResumeJavaScript(), m_viewImpl->page()->pageID());
#endif
    }
}
#endif

#else
void InputMethodContextEfl::onIMFInputSequenceComplete(void* data, Ecore_IMF_Context*, void* eventInfo)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);
    if (!eventInfo || !inputMethodContext->m_focused)
        return;

    inputMethodContext->m_viewImpl->page()->confirmComposition(String::fromUTF8(static_cast<char*>(eventInfo)));
}

void InputMethodContextEfl::onIMFPreeditSequenceChanged(void* data, Ecore_IMF_Context* context, void*)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(data);

    if (!inputMethodContext->m_viewImpl->page()->focusedFrame() || !inputMethodContext->m_focused)
        return;

    char* buffer = 0;
    ecore_imf_context_preedit_string_get(context, &buffer, 0);
    if (!buffer)
        return;

    String preeditString = String::fromUTF8(buffer);
    free(buffer);
    Vector<CompositionUnderline> underlines;
    underlines.append(CompositionUnderline(0, preeditString.length(), Color(0, 0, 0), false));
    inputMethodContext->m_viewImpl->page()->setComposition(preeditString, underlines, 0);
}
#endif

PassOwnPtr<Ecore_IMF_Context> InputMethodContextEfl::createIMFContext(Evas* canvas)
{
    const char* defaultContextID = ecore_imf_context_default_id_get();
    if (!defaultContextID)
        return nullptr;

    OwnPtr<Ecore_IMF_Context> imfContext = adoptPtr(ecore_imf_context_add(defaultContextID));
    if (!imfContext)
        return nullptr;

    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(canvas);
    ecore_imf_context_client_window_set(imfContext.get(), reinterpret_cast<void*>(ecore_evas_window_get(ecoreEvas)));
    ecore_imf_context_client_canvas_set(imfContext.get(), canvas);

    return imfContext.release();
}

void InputMethodContextEfl::handleMouseUpEvent(const Evas_Event_Mouse_Up*)
{
#if ENABLE(TIZEN_ISF_PORT)
    if (!m_context)
        return;

    resetIMFContext();
#else
    ecore_imf_context_reset(m_context.get());
#endif
}

void InputMethodContextEfl::handleKeyDownEvent(const Evas_Event_Key_Down* downEvent, bool* isFiltered)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (m_viewImpl->textSelection()->isLargeHandleMode())
        m_viewImpl->textSelection()->setIsTextSelectionMode(TextSelection::ModeNone);
#endif

#if ENABLE(TIZEN_ISF_PORT)
    if (!m_context)
        return;

    const EditorState& state = m_viewImpl->page()->editorState();

    if (!m_isLastKeyEventFiltered && !m_approximateSurroundingText.isNull())
        ecore_imf_context_cursor_position_set(m_context.get(), m_approximateCursorPosition);

    if (m_fakeKeyEventTimer.isActive())
        m_fakeKeyEventTimer.stop();
    else
        m_viewImpl->page()->prepareKeyDownEvent();

    m_doNotHandleFakeKeyEvent = true;
#endif

    Ecore_IMF_Event inputMethodEvent;
    ecore_imf_evas_event_key_down_wrap(const_cast<Evas_Event_Key_Down*>(downEvent), &inputMethodEvent.key_down);

    *isFiltered = ecore_imf_context_filter_event(m_context.get(), ECORE_IMF_EVENT_KEY_DOWN, &inputMethodEvent);

#if ENABLE(TIZEN_ISF_PORT)
    m_doNotHandleFakeKeyEvent = false;
    m_isLastKeyEventFiltered = *isFiltered;

    if (!*isFiltered) {
        if (!strcmp(downEvent->key, "BackSpace")) {
            if (state.cursorPosition > 0)
                updateApproximateText(String(), state.cursorPosition - 1, 1);
        } else if (!strcmp(downEvent->key, "Delete")) {
            if (state.cursorPosition < state.surroundingText.length())
                updateApproximateText(String(), state.cursorPosition, 1);
        } else
            updateApproximateText(String::fromUTF8(downEvent->compose), 0, 0);
    }
#endif
}

#if ENABLE(TIZEN_ISF_PORT)
void InputMethodContextEfl::handleKeyUpEvent(const Evas_Event_Key_Up* upEvent)
{
    if (!m_context)
        return;

    Ecore_IMF_Event inputMethodEvent;
    ecore_imf_evas_event_key_up_wrap(const_cast<Evas_Event_Key_Up*>(upEvent), &inputMethodEvent.key_up);
    ecore_imf_context_filter_event(m_context.get(), ECORE_IMF_EVENT_KEY_UP, &inputMethodEvent);
}

void InputMethodContextEfl::updateTextInputState()
{
    const EditorState& editor = m_viewImpl->page()->editorState();
    if (editor.shouldIgnoreCompositionSelectionChange || editor.updateEditorRectOnly)
        return;

    if (editor.isContentEditable)
        showIMFContext(editor);
    else
        hideIMFContext();

    m_approximateSurroundingText = String();

    if (m_context)
        ecore_imf_context_cursor_position_set(m_context.get(), editor.cursorPosition);
}

void InputMethodContextEfl::updateTextInputStateByUserAction(bool setFocus)
{
    const EditorState& editor = m_viewImpl->page()->editorState();

    if (editor.isContentEditable) {
#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
        if (setFocus)
            m_viewImpl->setFocusManually();
#endif
        showIMFContext(editor, true);
    } else
        hideIMFContext();

    if (m_context)
        ecore_imf_context_cursor_position_set(m_context.get(), editor.cursorPosition);
}
#else
void InputMethodContextEfl::updateTextInputState()
{
    if (!m_context)
        return;

    const EditorState& editor = m_viewImpl->page()->editorState();

    if (editor.isContentEditable) {
        if (m_focused)
            return;

        ecore_imf_context_reset(m_context.get());
        ecore_imf_context_focus_in(m_context.get());
        m_focused = true;
    } else {
        if (!m_focused)
            return;

        if (editor.hasComposition)
            m_viewImpl->page()->cancelComposition();

        m_focused = false;
        ecore_imf_context_reset(m_context.get());
        ecore_imf_context_focus_out(m_context.get());
    }
}
#endif

#if ENABLE(TIZEN_ISF_PORT)
void InputMethodContextEfl::initializeIMFContext(Ecore_IMF_Context* context, Ecore_IMF_Input_Panel_Layout layout, int layoutVariation, Ecore_IMF_Input_Panel_Return_Key_Type returnKeyType, bool useComposition, bool useAutocapital)
{
    ecore_imf_context_input_panel_enabled_set(context, false);
    ecore_imf_context_input_panel_event_callback_add(context, ECORE_IMF_INPUT_PANEL_STATE_EVENT, onIMFInputPanelStateChanged, this);
    ecore_imf_context_input_panel_event_callback_add(context, ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT, onIMFInputPanelGeometryChanged, this);
    ecore_imf_context_input_panel_event_callback_add(context, ECORE_IMF_CANDIDATE_PANEL_STATE_EVENT, onIMFCandidatePanelStateChanged, this);
    ecore_imf_context_input_panel_event_callback_add(context, ECORE_IMF_CANDIDATE_PANEL_GEOMETRY_EVENT, onIMFCandidatePanelGeometryChanged, this);
    ecore_imf_context_retrieve_surrounding_callback_set(context, onIMFRetrieveSurrounding, this);
    ecore_imf_context_event_callback_add(context, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, onIMFDeleteSurrounding, this);
    ecore_imf_context_event_callback_add(context, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, onIMFPreeditSequenceChanged, this);
    ecore_imf_context_event_callback_add(context, ECORE_IMF_CALLBACK_COMMIT, onIMFInputSequenceComplete, this);

    ecore_imf_context_input_panel_layout_set(context, layout);
    if (layoutVariation >= 0)
        ecore_imf_context_input_panel_layout_variation_set(context, layoutVariation);
    ecore_imf_context_input_panel_return_key_type_set(context, returnKeyType);
    ecore_imf_context_prediction_allow_set(context, useComposition);
    ecore_imf_context_autocapital_type_set(context, useAutocapital ? ECORE_IMF_AUTOCAPITAL_TYPE_SENTENCE : ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
}

PassOwnPtr<Ecore_IMF_Context> InputMethodContextEfl::takeContext(uintptr_t contextID)
{
    size_t i = m_contextList.size();
    while (i > 0) {
        --i;
        if (m_contextList[i].first == contextID) {
            PassOwnPtr<Ecore_IMF_Context> context = m_contextList[i].second.release();
            m_contextList.remove(i);
            return context;
        }
    }

    return PassOwnPtr<Ecore_IMF_Context>();
}

void InputMethodContextEfl::setIMFContext(const EditorState& editor)
{
    OwnPtr<Ecore_IMF_Context> context;
    if (m_contextID == editor.inputMethodContextID)
        context = m_context.release();
    else
        context = takeContext(editor.inputMethodContextID);

    revertIMFContext();

    if (!context) {
        context = createIMFContext(evas_object_evas_get(m_viewImpl->view()));
        if (!context)
            return;

        const String& type = editor.inputMethodHints;
        Ecore_IMF_Input_Panel_Layout layout;
        int layoutVariation = -1;
        Ecore_IMF_Input_Panel_Return_Key_Type returnKeyType = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
        bool useComposition = !(type == "password" || type == "plugin");
        bool isMultiLine = (type.isEmpty() || type == "textarea");

        if (!isMultiLine) {
            if (editor.hasForm)
                returnKeyType = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_GO;
            else
                returnKeyType = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_DONE;
        }

        if (type.startsWith("number")) {
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY;
            if (type.endsWith("SignedDecimal"))
                layoutVariation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED_AND_DECIMAL;
            else if (type.endsWith("Signed"))
                layoutVariation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_SIGNED;
            else if (type.endsWith("Decimal"))
                layoutVariation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_DECIMAL;
            else
                layoutVariation = ECORE_IMF_INPUT_PANEL_LAYOUT_NUMBERONLY_VARIATION_NORMAL;
        } else if (type == "email")
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_EMAIL;
        else if (type == "url")
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_URL;
        else if (type == "tel")
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PHONENUMBER;
        else if (type == "password")
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_PASSWORD;
        else {
            layout = ECORE_IMF_INPUT_PANEL_LAYOUT_NORMAL;
            if (type == "search")
                returnKeyType = ECORE_IMF_INPUT_PANEL_RETURN_KEY_TYPE_SEARCH;
        }

        initializeIMFContext(context.get(), layout, layoutVariation, returnKeyType, useComposition, isMultiLine);
    }

    m_context = context.release();
    m_contextID = editor.inputMethodContextID;
}

bool InputMethodContextEfl::isShow()
{
    return (m_context && m_focused && ecore_imf_context_input_panel_state_get(m_context.get()) != ECORE_IMF_INPUT_PANEL_STATE_HIDE);
}

Ecore_IMF_Autocapital_Type InputMethodContextEfl::autoCapitalType()
{
    return (m_context ? ecore_imf_context_autocapital_type_get(m_context.get()) : ECORE_IMF_AUTOCAPITAL_TYPE_NONE);
}

void InputMethodContextEfl::onFocusIn()
{
    if (isSystemKeypadShow() && m_context) {
        ecore_imf_context_focus_in(m_context.get());
        ecore_imf_context_input_panel_show(m_context.get());
    } else
        m_viewImpl->page()->process()->send(Messages::WebPage::RequestUpdatingEditorState(), m_viewImpl->page()->pageID());
}

void InputMethodContextEfl::onFocusOut()
{
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    if (m_state != ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
        if (m_viewImpl->pageClient->isClipboardWindowOpened())
            m_viewImpl->pageClient->closeClipboardWindow();
    }
#endif

    if (!m_context || !m_focused)
        return;

    resetIMFContext(false);
    ecore_imf_context_input_panel_hide(m_context.get());
    ecore_imf_context_focus_out(m_context.get());
}

void InputMethodContextEfl::didRequestUpdatingEditorState()
{
    if (m_inputPickerType >= 0) {
        showInputPicker(m_viewImpl->page()->editorState());
        return;
    }

    bool hasFocus = evas_object_focus_get(m_viewImpl->view());

    if (!m_context || !m_focused || !hasFocus)
        return;

    ecore_imf_context_focus_in(m_context.get());
    ecore_imf_context_input_panel_show(m_context.get());
    if (!isSystemKeypadShow())
        setState(ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW);
}

void InputMethodContextEfl::revertIMFContext()
{
    if (!m_context)
        return;

    if (m_contextList.size() >= maxContextSize)
        m_contextList.remove(0);

    PassOwnPtr<Ecore_IMF_Context> imfContext = m_context.release();
    m_contextList.append(std::make_pair(m_contextID, imfContext));
    m_contextID = 0;
}

void InputMethodContextEfl::resetIMFContext(bool doNotConfirmComposition)
{
    if (!m_context || !m_preeditOccurred)
        return;

    // TODO: while this looks ugly, it serves 2 different purposes.
    // m_doNothingOnIMFReset is for clearing not commited text from preedit
    // when switching to new InputField. (otherwise it appeared)
    // m_doNotHandleFakeKeyEvent has slightly different meaning, at some point
    // this should be refactored, but it's bigger task.
    m_doNotHandleFakeKeyEvent = true;
    m_doNothingOnIMFReset = doNotConfirmComposition;
    ecore_imf_context_reset(m_context.get());
    m_doNothingOnIMFReset = false;
    m_doNotHandleFakeKeyEvent = false;

    m_approximateSurroundingText = String();
    m_preeditOccurred = false;
}

void InputMethodContextEfl::showIMFContext(const EditorState& editor, bool isUserAction)
{
    Ewk_Settings* settings = ewk_view_settings_get(m_viewImpl->view());
    bool isContextTransition = (m_contextID && m_contextID != editor.inputMethodContextID);

    if (!isUserAction && !isContextTransition) {
        if (!ewk_settings_uses_keypad_without_user_action_get(settings) || (m_focused && m_contextID == editor.inputMethodContextID))
            return;
    }

    if (m_contextID != editor.inputMethodContextID)
        hideIMFContext();

    m_focused = true;

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
    if (editor.inputMethodHints == "date")
        m_inputPickerType = EWK_INPUT_TYPE_DATE;
    else if (editor.inputMethodHints == "datetime")
        m_inputPickerType = EWK_INPUT_TYPE_DATETIME;
    else if (editor.inputMethodHints == "datetime-local")
        m_inputPickerType = EWK_INPUT_TYPE_DATETIMELOCAL;
    else if (editor.inputMethodHints == "month")
        m_inputPickerType = EWK_INPUT_TYPE_MONTH;
    else if (editor.inputMethodHints == "time")
        m_inputPickerType = EWK_INPUT_TYPE_TIME;
    else if (editor.inputMethodHints == "week")
        m_inputPickerType = EWK_INPUT_TYPE_WEEK;
    else
        m_inputPickerType = -1;

    if (m_inputPickerType >= 0) {
        showInputPicker(editor);
        m_contextID = editor.inputMethodContextID;

        return;
    }
#endif // ENABLE(TIZEN_INPUT_TAG_EXTENSION)

    bool hasFocus = evas_object_focus_get(m_viewImpl->view());

    if (!ewk_settings_default_keypad_enabled_get(settings)) {
        if (hasFocus) {
            Eina_Rectangle dummyRectForCustomKeypadCallback;
            memset(&dummyRectForCustomKeypadCallback, 0, sizeof(Eina_Rectangle));
            evas_object_smart_callback_call(m_viewImpl->view(), "inputmethod,changed", &dummyRectForCustomKeypadCallback);
        }
        return;
    }

    setIMFContext(editor);
    if (!m_context)
        return;

    if (!hasFocus)
        return;

     // input field zoom for external keyboard
    if (s_shouldUseExternalKeyboard)
        ewk_view_focused_node_adjust(m_viewImpl->view(), EINA_TRUE);
    else {
        if (ecore_imf_context_input_panel_state_get(m_context.get()) == ECORE_IMF_INPUT_PANEL_STATE_SHOW)
            ewk_view_focused_node_adjust(m_viewImpl->view(), EINA_TRUE, EINA_FALSE);
    }

    resetIMFContext();

    ecore_imf_context_focus_in(m_context.get());
    ecore_imf_context_input_panel_show(m_context.get());
    setState(ECORE_IMF_INPUT_PANEL_STATE_WILL_SHOW);
}

void InputMethodContextEfl::hideIMFContext()
{
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    if (m_state != ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
        if (m_viewImpl->pageClient->isClipboardWindowOpened())
            m_viewImpl->pageClient->closeClipboardWindow();
    }
#endif

    if (m_context) {
        if (evas_object_focus_get(m_viewImpl->view())) {
            resetIMFContext();
            ecore_imf_context_input_panel_hide(m_context.get());
            ecore_imf_context_focus_out(m_context.get());
        }
        revertIMFContext();
    }

    m_inputPickerType = -1;
    m_focused = false;
}

void InputMethodContextEfl::destroyIMFContextList()
{
    m_contextList.clear();
}

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
void InputMethodContextEfl::showInputPicker(const EditorState& editorState)
{
    if (editorState.selectionIsRange || !evas_object_focus_get(m_viewImpl->view()))
        return;

    ewkViewInputPickerRequest(m_viewImpl->view(), static_cast<Ewk_Input_Type>(m_inputPickerType), editorState.surroundingText);
    m_inputPickerType = -1;
}
#endif

bool InputMethodContextEfl::isIMEPostion(int x, int y)
{
    if (m_state == ECORE_IMF_INPUT_PANEL_STATE_SHOW)
        return m_imeRect.contains(x, y);

    return false;
}

void InputMethodContextEfl::removeIMFContext(uintptr_t contextID)
{
    if (m_contextID == contextID)
        hideIMFContext();

    takeContext(contextID);
}

void InputMethodContextEfl::requestFakeKeyEvent()
{
    if (m_doNotHandleFakeKeyEvent || m_fakeKeyEventTimer.isActive())
        return;

    m_fakeKeyEventTimer.startOneShot(0);
    m_viewImpl->page()->prepareKeyDownEvent();
}

void InputMethodContextEfl::fakeKeyEventTimerFired(Timer<InputMethodContextEfl>*)
{
    UChar ch = 0;
    if (!m_approximateSurroundingText.isNull()) {
        if (!m_approximateCursorPosition)
            ch = m_approximateSurroundingText[0];
        else if (m_approximateCursorPosition > m_approximateSurroundingText.length())
            ch = m_approximateSurroundingText[m_approximateSurroundingText.length() - 1];
        else
            ch = m_approximateSurroundingText[m_approximateCursorPosition - 1];
    }

    String string;
    if (Unicode::isSeparatorSpace(ch))
        string = "space";
    else if (Unicode::isPrintableChar(ch))
        string.append(ch);
    else
        string = "Shift_L";

    CString stringForEvent = string.utf8();
    const char* data = stringForEvent.data();

    Evas_Event_Key_Down downEvent;
    memset(&downEvent, 0, sizeof(Evas_Event_Key_Down));
    downEvent.key = data;
    downEvent.string = data;
    NativeWebKeyboardEvent nativeEvent(&downEvent, true);
    nativeEvent.setInputMethodContextID(m_viewImpl->page()->editorState().inputMethodContextID);
    m_viewImpl->page()->handleKeyboardEvent(nativeEvent);

    Evas_Event_Key_Up upEvent;
    memset(&upEvent, 0, sizeof(Evas_Event_Key_Up));
    upEvent.key = data;
    upEvent.string = data;
    m_viewImpl->page()->handleKeyboardEvent(NativeWebKeyboardEvent(&upEvent));
}

void InputMethodContextEfl::requestUpdateTextInputStateByUserAction()
{
    if (!m_updateTextInputStateByUserActionTimer.isActive())
        m_updateTextInputStateByUserActionTimer.startOneShot(0);
}

void InputMethodContextEfl::updateTextInputStateByUserActionTimerFired( Timer < InputMethodContextEfl > *timer)
{
    const EditorState& editor = m_viewImpl->page()->editorState();
    if (!isShow() && editor.isContentEditable)
        updateTextInputStateByUserAction(true);
    else
        hideIMFContext();

}
void InputMethodContextEfl::updateApproximateText(const String& text, unsigned removePosition, unsigned removeLength)
{
    if (m_approximateSurroundingText.isNull()) {
        const EditorState& state = m_viewImpl->page()->editorState();

        m_approximateSurroundingText = state.surroundingText;
        if (m_approximateSurroundingText.isNull())
            m_approximateSurroundingText = emptyString();
        m_approximateCursorPosition = state.cursorPosition;
    }

    if (!text.isNull()) {
        m_approximateSurroundingText.insert(text, m_approximateCursorPosition);
        m_approximateCursorPosition += text.length();
    } else {
        m_approximateSurroundingText.remove(removePosition, removeLength);
        if (m_approximateCursorPosition > removePosition && m_approximateCursorPosition <= removePosition + removeLength)
            m_approximateCursorPosition = removePosition;
    }
}

bool InputMethodContextEfl::recalcFilterEvent(const Ecore_IMF_Event* event)
{
    if (!m_context)
        return false;

    m_approximateSurroundingText = String();

    bool isFakeKeyEvent = (!event->key_down.keyname || event->key_down.keyname[0] == 0);
    if (isFakeKeyEvent)
        return true;

    m_doNotHandleFakeKeyEvent = true;
    bool isFiltered = ecore_imf_context_filter_event(m_context.get(), ECORE_IMF_EVENT_KEY_DOWN, const_cast<Ecore_IMF_Event*>(event));
    m_doNotHandleFakeKeyEvent = false;

    return isFiltered;
}
#endif // #if ENABLE(TIZEN_ISF_PORT)

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
static Eina_Bool mediaKeyUpCallback(void* userData, int type, void* event)
{
    InputMethodContextEfl* inputMethodContext = static_cast<InputMethodContextEfl*>(userData);
    Ecore_Event_Key* keyEvent = static_cast<Ecore_Event_Key*>(event);
    LOG(Media, "keyname(%s)", keyEvent->keyname);

    if (!strcmp(keyEvent->keyname, KEY_MEDIA))
        inputMethodContext->sendMediaKey(static_cast<int>(MEDIA_KEY_PLAYPAUSE));

    return ECORE_CALLBACK_PASS_ON;
}

void InputMethodContextEfl::sendMediaKey(int key)
{
    m_viewImpl->page()->process()->send(Messages::WebPage::HandleMediaKey(key), m_viewImpl->page()->pageID());
}

void InputMethodContextEfl::grabMediaKey()
{
    if (m_mdiaKeyGrabbed)
        return;

    // Grab media key for earjack media key
    Ecore_X_Window xWindow = 0;
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    if (m_viewImpl && m_viewImpl->ewkContext())
        xWindow = m_viewImpl->ewkContext()->xWindow();
#endif
    Display* display = static_cast<Display*>(ecore_x_display_get());
    if (xWindow && display) {
        utilx_grab_key(display, xWindow, KEY_MEDIA, OR_EXCLUSIVE_GRAB);
        m_mediaKeyUpHandler = ecore_event_handler_add(ECORE_EVENT_KEY_UP, mediaKeyUpCallback, this);

        m_mdiaKeyGrabbed = true;
    }
}

void InputMethodContextEfl::ungrabMediaKey()
{
    if (!m_mdiaKeyGrabbed)
        return;

    Ecore_X_Window xWindow = 0;
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    if (m_viewImpl && m_viewImpl->ewkContext())
        xWindow = m_viewImpl->ewkContext()->xWindow();
#endif
    Display* display = static_cast<Display*>(ecore_x_display_get());
    if (xWindow && display)
        utilx_ungrab_key(display, xWindow, KEY_MEDIA);

    if (m_mediaKeyUpHandler) {
        ecore_event_handler_del(static_cast<Ecore_Event_Handler*>(m_mediaKeyUpHandler));
        m_mediaKeyUpHandler = 0;
    }

    m_mdiaKeyGrabbed = false;
}
#endif

}
