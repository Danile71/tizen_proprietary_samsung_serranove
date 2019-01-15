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

#ifndef PasswordSaveConfirmPopup_h
#define PasswordSaveConfirmPopup_h

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)

#include <Evas.h>
#include <wtf/PassOwnPtr.h>

class Ewk_Context;

namespace WebKit {

class PasswordSaveConfirmPopup : public RefCounted<PasswordSaveConfirmPopup> {
    WTF_MAKE_FAST_ALLOCATED;

public:
    static PassRefPtr<PasswordSaveConfirmPopup> create(Ewk_Context* context)
    {
        return adoptRef(new PasswordSaveConfirmPopup(context));
    }
    ~PasswordSaveConfirmPopup();

    void show(Evas_Object* ewkView);
    void hide();
    void setPasswordData(const String& url, WKFormDataRef formData);
    void savePasswordData();
    void clearPasswordData();
    void neverSavePasswordData();

    Ewk_Context* m_context;
    Evas_Object* m_view;
    Evas_Object* m_widgetWin;

private:
    explicit PasswordSaveConfirmPopup(Ewk_Context* context);

    Evas_Object* m_popup;
    RefPtr<WebFormData> m_formData;
    String m_url;
    bool m_isShow;
};

} // namespace WebKit

#endif // TIZEN_WEBKIT2_FORM_DATABASE
#endif // PasswordSaveConfirmPopup_h
