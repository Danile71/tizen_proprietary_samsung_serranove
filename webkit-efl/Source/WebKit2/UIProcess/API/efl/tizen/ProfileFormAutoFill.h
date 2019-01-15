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

#ifndef ProfileFormAutoFill_h
#define ProfileFormAutoFill_h

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
#include "FormDatabase.h"

namespace WebKit {

class ProfileFormAutoFill {
public:
    static PassOwnPtr<ProfileFormAutoFill> create(RefPtr<WebKit::FormDatabase> formDataBase)
    {
        return adoptPtr(new ProfileFormAutoFill(formDataBase));
    }
    ~ProfileFormAutoFill();
    void getProfileFormCandidateData(const String& name, const String& value, Vector<std::pair<int, std::pair<String, String> > > & autoFillTextData);
    void setProfileFormCanditateToWebForm(const int& profileID, String& fillScript);
private:
    ProfileFormAutoFill(RefPtr<WebKit::FormDatabase> formDataBase);
    void updateProfileFormData();

    // Hash map for name attribute
    HashMap<String, HashSet<std::pair<String, String> > > m_nameRecordMap;

    Vector<String> m_profileID;
    RefPtr<WebKit::FormDatabase> m_formDatabase;
};

} // namespace WebKit

#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif // FormAutoFillText_h

