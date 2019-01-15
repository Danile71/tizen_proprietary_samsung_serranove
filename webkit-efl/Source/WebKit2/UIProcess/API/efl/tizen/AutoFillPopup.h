/*
 * Copyright (C) 2013 Samsung Electronics All rights reserved.
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

#ifndef AutoFillPopup_h
#define AutoFillPopup_h

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#include <Evas.h>
#include <WebCore/IntRect.h>
#include <wtf/PassOwnPtr.h>

class EwkViewImpl;

namespace WebKit {
enum AutoFillItemType {
    candidateAutoFill,
    profileAutoFill,
    dataListAutoFill
};

struct AutoFillPopupItem {
    String maintext;
    String subtext;
    int id;
    AutoFillItemType itemtype;

    AutoFillPopupItem(String m, AutoFillItemType t) : maintext(m), itemtype(t) { }
    AutoFillPopupItem(String m, String s, int i, AutoFillItemType t) : maintext(m), subtext(s), id(i), itemtype(t) { }
};

class AutoFillPopup {
public:
    static PassOwnPtr<AutoFillPopup> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new AutoFillPopup(viewImpl));
    }
    ~AutoFillPopup();

    bool isShowing() { return m_isShowing; }
    void move(const WebCore::IntRect& inputFieldRect);
    void show(const WebCore::IntRect&);
    void hide();
    void updateFormData(const Vector<AutoFillPopupItem>&);
    void updateAutoFillPopup(int inputFieldWidth);

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    void setValueForProfile(int&);
#endif
    void setValueForInputElement(String&);
    void updateCandidateValue(const String& value) { m_candidateValue = value; }
    String getCandidateValue() { return m_candidateValue; }

    int m_listWidth;
private:
    explicit AutoFillPopup(EwkViewImpl*);

    EwkViewImpl* m_viewImpl;
    Evas_Object* m_icon;
    Evas_Object* m_list;
    int m_borderWidth;
    bool m_isShowing;
    String m_candidateValue;
};

} // namespace WebKit

#endif // TIZEN_WEBKIT2_FORM_DATABASE
#endif // FormDataCandidate_h
