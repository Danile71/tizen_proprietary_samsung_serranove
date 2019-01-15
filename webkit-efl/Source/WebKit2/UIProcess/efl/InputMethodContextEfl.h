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

#ifndef InputMethodContextEfl_h
#define InputMethodContextEfl_h

#include <Ecore_IMF.h>
#include <Evas.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#if ENABLE(TIZEN_ISF_PORT)
#include <Ecore_X.h>
#endif

class EwkViewImpl;

namespace WebKit {

class WebPageProxy;

class InputMethodContextEfl {
public:
    static PassOwnPtr<InputMethodContextEfl> create(EwkViewImpl* viewImpl, Evas* canvas)
    {
#if ENABLE(TIZEN_ISF_PORT)
        OwnPtr<Ecore_IMF_Context> context;
#else
        OwnPtr<Ecore_IMF_Context> context = createIMFContext(canvas);
        if (!context)
            return nullptr;
#endif

        return adoptPtr(new InputMethodContextEfl(viewImpl, context.release()));
    }
    ~InputMethodContextEfl();

    void handleMouseUpEvent(const Evas_Event_Mouse_Up* upEvent);
    void handleKeyDownEvent(const Evas_Event_Key_Down* downEvent, bool* isFiltered);
    void updateTextInputState();

#if ENABLE(TIZEN_ISF_PORT)
    static bool shouldUseExternalKeyboard() { return s_shouldUseExternalKeyboard; }
    static bool isSystemKeypadShow() { return s_isSystemKeypadShow; }

    void handleKeyUpEvent(const Evas_Event_Key_Up*);
    bool isShow();
    Ecore_IMF_Autocapital_Type autoCapitalType();
    void onFocusIn();
    void onFocusOut();
    void didRequestUpdatingEditorState();
    void resetIMFContext(bool doNotConfirmComposition = true);
    void updateTextInputStateByUserAction(bool);
    void hideIMFContext();
    bool isIMEPostion(int, int);
    void removeIMFContext(uintptr_t);
    int state() { return m_state; }
    bool recalcFilterEvent(const Ecore_IMF_Event*);
    WebCore::IntRect getIMERect() { return m_imeRect; }
    void requestUpdateTextInputStateByUserAction();
#if ENABLE(TIZEN_KEYPRESS_KEYCODE_FIX)
    String preeditString;
#endif
#endif

    void sendMediaKey(int key);
    void grabMediaKey();
    void ungrabMediaKey();

private:
    InputMethodContextEfl(EwkViewImpl*, PassOwnPtr<Ecore_IMF_Context>);

    static PassOwnPtr<Ecore_IMF_Context> createIMFContext(Evas* canvas);
    static void onIMFInputSequenceComplete(void* data, Ecore_IMF_Context*, void* eventInfo);
    static void onIMFPreeditSequenceChanged(void* data, Ecore_IMF_Context*, void* eventInfo);

#if ENABLE(TIZEN_ISF_PORT)
    static void initializeExternalKeyboard();
    static Eina_Bool windowPropertyChanged(void*, int, void*);

    static void onIMFInputPanelStateChanged(void*, Ecore_IMF_Context*, int);
    static void onIMFInputPanelGeometryChanged(void*, Ecore_IMF_Context*, int);
    static void onIMFCandidatePanelStateChanged(void*, Ecore_IMF_Context*, int);
    static void onIMFCandidatePanelGeometryChanged(void*, Ecore_IMF_Context*, int);
    static Eina_Bool onIMFRetrieveSurrounding(void*, Ecore_IMF_Context*, char**, int*);
    static void onIMFDeleteSurrounding(void*, Ecore_IMF_Context*, void*);

    void initializeIMFContext(Ecore_IMF_Context*, Ecore_IMF_Input_Panel_Layout, int, Ecore_IMF_Input_Panel_Return_Key_Type, bool, bool);

    PassOwnPtr<Ecore_IMF_Context> takeContext(uintptr_t);
    void setIMFContext(const EditorState&);
    void revertIMFContext();
    void showIMFContext(const EditorState&, bool = false);
    void destroyIMFContextList();
    void setState(int state) { m_state = state; }
    void setIMERect(const WebCore::IntRect& rect) { m_imeRect = rect; }

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
    void showInputPicker(const EditorState&);
#endif

    void requestFakeKeyEvent();
    void fakeKeyEventTimerFired(WebCore::Timer<InputMethodContextEfl>*);
    void updateTextInputStateByUserActionTimerFired(WebCore::Timer<InputMethodContextEfl>*);
    void updateApproximateText(const String&, unsigned, unsigned);
#endif

    EwkViewImpl* m_viewImpl;
    OwnPtr<Ecore_IMF_Context> m_context;
    bool m_focused;

#if ENABLE(TIZEN_ISF_PORT)
    static const unsigned maxContextSize;

    static Ecore_X_Atom s_externalKeyboardProperty;
    static bool s_shouldUseExternalKeyboard;
    static bool s_isSystemKeypadShow;

    Vector<std::pair<uintptr_t, OwnPtr<Ecore_IMF_Context> > > m_contextList;
    uintptr_t m_contextID;
    int m_state;
    WebCore::IntRect m_imeRect;
    int m_inputPickerType;
    bool m_doNotHandleFakeKeyEvent;
    bool m_doNothingOnIMFReset;
    WebCore::Timer<InputMethodContextEfl> m_fakeKeyEventTimer;
    WebCore::Timer<InputMethodContextEfl> m_updateTextInputStateByUserActionTimer;
    String m_approximateSurroundingText;
    unsigned m_approximateCursorPosition;
    bool m_isLastKeyEventFiltered;
    bool m_preeditOccurred;

#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    int m_countInputMethod;
    int m_countDashMode;
    bool m_isDashMode;

    void checkFrequencyOfInputMethod();
    void inputMethodDashModeFired(WebCore::Timer<InputMethodContextEfl>*);
    void updateInputMethodFired(WebCore::Timer<InputMethodContextEfl>*);
    WebCore::Timer<InputMethodContextEfl> m_inputMethodDashModeTimer;
    WebCore::Timer<InputMethodContextEfl> m_updateInputMethodTimer;
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    bool m_mdiaKeyGrabbed;
    void* m_mediaKeyUpHandler;
#endif
#endif
};

} // namespace WebKit

#endif // InputMethodContextEfl_h
