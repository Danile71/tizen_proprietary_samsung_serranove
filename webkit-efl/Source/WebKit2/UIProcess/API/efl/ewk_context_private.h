/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef ewk_context_private_h
#define ewk_context_private_h

#include "DownloadManagerEfl.h"
#include "WKAPICast.h"
#include "WKRetainPtr.h"
#include "ewk_context.h"

#if PLATFORM(TIZEN)
#include "WKEinaSharedString.h"
#include "WKString.h"
#include "ewk_context.h"
#if ENABLE(TIZEN_SUPPORT)
#include "ewk_security_origin.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif
#endif
#if ENABLE(TIZEN_WEBKIT2_PROXY)
#include <net_connection.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#include "FormDatabase.h"
#include "PasswordSaveConfirmPopup.h"

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
#include "ProfileFormAutoFill.h"
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM

#endif
#endif // #if PLATFORM(TIZEN)

class Ewk_Url_Scheme_Request;
class Ewk_Cookie_Manager;
class Ewk_Favicon_Database;
#if ENABLE(BATTERY_STATUS)
class BatteryProvider;
#endif
#if ENABLE(NETWORK_INFO)
class NetworkInfoProvider;
#endif
#if ENABLE(VIBRATION)
class VibrationProvider;
#endif
#if ENABLE(TIZEN_GEOLOCATION)
class Ewk_Geolocation_Provider;
#endif
#if ENABLE(TIZEN_NOTIFICATIONS)
class Ewk_Notification_Provider;
#endif

namespace WebKit {
class ContextHistoryClientEfl;
class RequestManagerClientEfl;
}

class Ewk_Context : public RefCounted<Ewk_Context> {
public:
    static PassRefPtr<Ewk_Context> create(WKContextRef context);
    static PassRefPtr<Ewk_Context> create();
    static PassRefPtr<Ewk_Context> create(const String& injectedBundlePath);

    static PassRefPtr<Ewk_Context> defaultContext();

    ~Ewk_Context();

    Ewk_Cookie_Manager* cookieManager();

    Ewk_Favicon_Database* faviconDatabase();

    WebKit::RequestManagerClientEfl* requestManager();

#if ENABLE(VIBRATION)
    PassRefPtr<VibrationProvider> vibrationProvider();
#endif

#if ENABLE(TIZEN_GEOLOCATION)
    Ewk_Geolocation_Provider* geolocationProvider();
#endif

    void addVisitedLink(const String& visitedURL);

    void setCacheModel(Ewk_Cache_Model);

    Ewk_Cache_Model cacheModel() const;

    WKContextRef wkContext();

    void urlSchemeRequestReceived(Ewk_Url_Scheme_Request*);

    WebKit::DownloadManagerEfl* downloadManager() const;

    WebKit::ContextHistoryClientEfl* historyClient();

#if PLATFORM(TIZEN)
    const char* proxyAddress() const { return m_proxyAddress; }
    bool setProxyAddress(const char*);

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    const char* certificateFile() const { return m_certificateFile; }
    bool setCertificateFile(const char*);
#endif

#if ENABLE(TIZEN_SQL_DATABASE)
    uint64_t defaultDatabaseQuota() const { return m_defaultDatabaseQuota; }
    void setDefaultDatabaseQuota(uint64_t);
#endif

    void setMessageFromInjectedBundleCallback(Ewk_Context_Message_From_Injected_Bundle_Callback, void*);
    void didReceiveMessageFromInjectedBundle(WKStringRef, WKTypeRef, WKTypeRef*);

    void setDidStartDownloadCallback(Ewk_Context_Did_Start_Download_Callback, void*);
    void didStartDownload(WKStringRef);
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    void setPassworSaveConfirmPopupCallback(Ewk_Context_Password_Confirm_Popup_Callback, void*);
    void passworSaveConfirmPopupCallbackCall(Evas_Object* ewkView);
#endif
#if ENABLE(TIZEN_LITE_MEMORYCACHE_OPTIMIZATION)
    void clearAllInMemoryResources();
#endif
#if ENABLE(TIZEN_CACHE_MEMORY_OPTIMIZATION)
    void clearAllLiveDecodedResources();
    void clearAllLiveEncodedResources();
    void clearAllDeadResources();
#endif
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    void restoreAllEncodedResources();
    void markVisibleContents(WebCore::IntRect& visibleRect);
    void setIsForeground(bool isForeground) { m_isForeground = isForeground; }
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    Ecore_X_Window xWindow();
    void setXWindow(Ecore_X_Window);
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    WebKit::FormDatabase* formDatabase() { return m_formDatabase.get(); }
    void hidePasswordSaveConfirmPopup();
    void showPasswordSaveConfirmPopup(Evas_Object* ewkView);
    void setPasswordData(const String& url, WKFormDataRef formData);
    void allowSavePassword();
    void cancelSavePassword();
    void neverSavePassword();

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    bool clearProfileFormData(unsigned id);
    bool addProfileFormData(Ewk_Autofill_Profile* profile, Vector<std::pair<String, String> >& textFormData);
    void candidateProfileFormData(const String& name, const String& value, Vector<std::pair<int, std::pair<String, String> > > & autoFillTextData);
    void setProfileFormCanditateToForm(const unsigned & profileID, String & fillScript);
    Ewk_Autofill_Profile* getProfileFormDataByProfileId(unsigned id);
    String getProfileFormNameFromEnum(Ewk_Autofill_Profile_Data_Type name);
    int getProfileFormEnumFromName(String name);
    void getAllProfileIDForProfileFormData(Vector<String>& list);
    Eina_List* getAllProfilesForProfileForm();
    int getNewProfileID() { return m_formDatabase->getNextIdForProfileFormData(); }
    bool isProfileDataDuplicate(unsigned id);
    unsigned getProfileHashValue(Vector<std::pair<String, String> >& textFormData);
    void getProfileDataVector(Ewk_Autofill_Profile* profile, Vector<std::pair<String, String> >& textFormData);
#if ENABLE(TIZEN_WEBKIT2_PROXY)
    void setProxyUri(const char* proxy);
    void setProxy();
    void setProxySetting(bool setting) { m_proxySetting = setting; }
#endif
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    Eina_Bool setCertificate(const char* certificateFile);
#endif
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM

#if ENABLE(TIZEN_EXTENSIBLE_API)
    int getExtensibleAPIEnumFromName(const char* extensibleName);
#endif

#endif

#if ENABLE(TIZEN_CACHE_DUMP_SYNC)
    void dumpCache();
#endif
#if ENABLE(TIZEN_WEB_STORAGE)
    void syncLocalStorage();
#endif
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    void setPixmap(int pixmap);
    int pixmap();
#endif
#endif // #if PLATFORM(TIZEN)

private:
    explicit Ewk_Context(WKContextRef);

#if ENABLE(TIZEN_WEBKIT2_PROXY)
    static void connectionChangedCallback(const char* ipv4, const char* ipv6, void *data);
#endif

    WKRetainPtr<WKContextRef> m_context;

    OwnPtr<Ewk_Cookie_Manager> m_cookieManager;
    OwnPtr<Ewk_Favicon_Database> m_faviconDatabase;
#if ENABLE(BATTERY_STATUS)
    RefPtr<BatteryProvider> m_batteryProvider;
#endif
#if ENABLE(NETWORK_INFO)
    RefPtr<NetworkInfoProvider> m_networkInfoProvider;
#endif
#if ENABLE(VIBRATION)
    RefPtr<VibrationProvider> m_vibrationProvider;
#endif
#if ENABLE(TIZEN_GEOLOCATION)
    OwnPtr<Ewk_Geolocation_Provider> m_geolocationProvider;
#endif
#if ENABLE(TIZEN_NOTIFICATIONS)
    OwnPtr<Ewk_Notification_Provider> m_notificationProvider;
#endif
    OwnPtr<WebKit::DownloadManagerEfl> m_downloadManager;
    OwnPtr<WebKit::RequestManagerClientEfl> m_requestManagerClient;

    OwnPtr<WebKit::ContextHistoryClientEfl> m_historyClient;

#if PLATFORM(TIZEN)
    WKEinaSharedString m_proxyAddress;
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    WKEinaSharedString m_certificateFile;
#endif
#if ENABLE(TIZEN_SQL_DATABASE)
    uint64_t m_defaultDatabaseQuota;
#endif
    struct {
        Ewk_Context_Message_From_Injected_Bundle_Callback callback;
        void* userData;
    } m_messageFromInjectedBundle;
    struct {
        Ewk_Context_Did_Start_Download_Callback callback;
        void* userData;
    } m_didStartDownload;
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    struct {
        Ewk_Context_Password_Confirm_Popup_Callback callback;
        void* userData;
    } m_passwordConfirmPopup;
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    Ecore_X_Window m_xWindow;
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    RefPtr<WebKit::FormDatabase> m_formDatabase;
    RefPtr<WebKit::PasswordSaveConfirmPopup> m_passwordSaveConfirmPopup;
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    OwnPtr<WebKit::ProfileFormAutoFill> m_profileFormAutoFill;
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    bool m_isCachedToDisk;
    bool m_isForeground;
#endif
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    int m_pixmap;
#endif
#if ENABLE(TIZEN_WEBKIT2_PROXY)
    connection_h m_connectionHandle;
    bool m_proxySetting;
#endif
#endif
};

#if PLATFORM(TIZEN)
Ewk_Security_Origin* createSecurityOrigin(WKSecurityOriginRef securityOrigin);
void deleteSecurityOrigin(Ewk_Security_Origin* origin);

#if ENABLE(TIZEN_EXTENSIBLE_API)
enum Ewk_Extensible_API {
    EWK_EXTENSIBLE_API_BACKGROUND_MUSIC,
    EWK_EXTENSIBLE_API_CSP,
    EWK_EXTENSIBLE_API_ENCRYPTION_DATABASE,
    EWK_EXTENSIBLE_API_FULL_SCREEN,
    EWK_EXTENSIBLE_API_MEDIA_STREAM_RECORD,
    EWK_EXTENSIBLE_API_MEDIA_VOLUME_CONTROL,
    EWK_EXTENSIBLE_API_PRERENDERING_FOR_ROTATION,
    EWK_EXTENSIBLE_API_ROTATE_CAMERA_VIEW,
    EWK_EXTENSIBLE_API_ROTATION_LOCK,
    EWK_EXTENSIBLE_API_SOUND_MODE,
    EWK_EXTENSIBLE_API_SUPPORT_FULL_SCREEN,
    EWK_EXTENSIBLE_API_VISIBILITY_SUSPEND,
    EWK_EXTENSIBLE_API_XWINDOW_FOR_FULL_SCREEN_VIDEO,
    EWK_EXTENSIBLE_API_SUPPORT_MULTIMEDIA,
    EWK_EXTENSIBLE_API_BACKGROUND_VIBRATION,
    EWK_EXTENSIBLE_API_BLOCK_MULTIMEDIA_ON_CALL,
    EWK_EXTENSIBLE_API_ENABLE_MANUAL_VIDEO_ROTATION,
    EWK_MAX_EXTENSIBLE_API
};
typedef enum Ewk_Extensible_API Ewk_Extensible_API;

static const char* tizenExtensibleAPINames[] = {"background,music", "csp", "encrypted,database", "fullscreen", "mediastream,record",
                                                                    "media,volume,control", "prerendering,for,rotation", "rotate,camera,view", "rotation,lock", "sound,mode",
                                                                    "support,fullscreen", "visibility,suspend", "xwindow,for,fullscreen,video", "support,multimedia", "background,vibration", "block,multimedia,on,call", "enable,manual,video,rotation"};
#endif
#endif

#endif // ewk_context_private_h
