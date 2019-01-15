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

#ifndef FormDatabase_h
#define FormDatabase_h

#include <WebCore/SQLiteDatabase.h>
#include <WebCore/SQLiteStatement.h>
#include <WebFormData.h>

namespace WebKit {

class FormDatabase : public RefCounted<FormDatabase> {
    WTF_MAKE_FAST_ALLOCATED;

public:
    struct PasswordFormData {
        String m_url;
        bool m_useFingerprint;
    };

public:
    static PassRefPtr<FormDatabase> create();
    virtual ~FormDatabase();
    static String defaultDatabaseDirectoryPath();
    static String defaultDatabaseFilename();
    virtual String databasePath() const;

    virtual bool open(const String& directory, const String& filename);
    virtual void close();
    virtual void clearCandidateFormData();
    virtual void clearPasswordFormData();
    void clearPasswordFormDataForURL(const String& url);

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    virtual bool clearProfileFormData(String& profileID);
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
    virtual bool isOpen() const;

    void addCandidateFormData(WKFormDataRef formData);
    void addPasswordFormData(const String& url, WKFormDataRef formData, bool useFingerprint);

    bool addURLForPasswordNever(const String& url);
    bool getPasswordFormValuesForURL(const String& url, String& formID, String& sourceFrameURL, bool& useFingerprint, String& buttonID, Vector<WebFormData::Data::ValueData>& passwordFormValues);
    bool getPasswordNeverURLs(Vector<String>& urls);
    bool getCandidateFormDataForName(const String& name, Vector<String>& candidateFormData);

    void getAllPasswordFormData(Vector<FormDatabase::PasswordFormData>& vector);

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    bool addProfileFormData(const String& profile, Vector<std::pair<String, String> >& textFormData);
    bool getProfileFormDataForProfile(const String& profile, Vector<std::pair<String, String> >& textFormData);
    void getProfileFormDataMap(HashMap<String, HashSet<std::pair<String, String> > > &getFormData);
    void getAllProfileIDForProfileFormData(Vector<String>& list);
    bool isProfileFormDataRecordUpdated() { return m_profileFormDataRecordMapUpdated; }
    void setProfileFormDataRecordUpdated(bool status) { m_profileFormDataRecordMapUpdated = status; };
    int getNextIdForProfileFormData();
    void getAuxStringForDisplay(const String& profileID, const String& mainString, std::pair<String, String>& auxStringPair);
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM

private:
    explicit FormDatabase();

    bool getPasswordFormDataForURL(const String& url, int& urlIndex, String& formID, String& sourceFrameURL, bool& useFingerprint, String& buttonID);

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    bool addProfileFormDataToMap(const String& profile, const String& name, const String& value);
    void initProfileFormData();
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM

    String m_databaseDirectoryPath;
    String m_databaseFilePath;

    String m_lastPasswordFormURL;
    Vector<WebFormData::Data::ValueData> m_lastPasswordFormValues;

    String m_lastCandidateFormName;
    Vector<String> m_lastCandidateFormData;

    WebCore::SQLiteDatabase m_syncFormDatabase;

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    // Hash map for password form data pairs (profile, (names, values))
    HashMap<String, HashSet<std::pair<String, String> > > m_profileFormDataRecordMap;
    bool m_profileFormDataRecordMapUpdated;
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM

    OwnPtr<WebCore::SQLiteStatement> m_insertPasswordFormDataStatement;
    OwnPtr<WebCore::SQLiteStatement> m_insertPasswordFormValuesStatement;
    OwnPtr<WebCore::SQLiteStatement> m_insertPasswordNeverDataStatement;
    OwnPtr<WebCore::SQLiteStatement> m_insertCandidateFormDataStatement;
    OwnPtr<WebCore::SQLiteStatement> m_selectPasswordFormDataForURLStatement;
    OwnPtr<WebCore::SQLiteStatement> m_selectPasswordFormValuesForURLStatement;
    OwnPtr<WebCore::SQLiteStatement> m_selectPasswordNeverURLStatement;
    OwnPtr<WebCore::SQLiteStatement> m_selectCandidateFormDataForNameStatement;

    OwnPtr<WebCore::SQLiteStatement> m_selectAllPasswordFormDataStatement;
    OwnPtr<WebCore::SQLiteStatement> m_deletePasswordFormDataForURLStatement;
    OwnPtr<WebCore::SQLiteStatement> m_deletePasswordFormValuesForURLStatement;

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    OwnPtr<WebCore::SQLiteStatement> m_selectProfileFormDataForProfileStatement;
    OwnPtr<WebCore::SQLiteStatement> m_insertProfileFormDataStatement;
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
};

}

#endif // FormDatabase_h
