/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "ewk_context.h"

#include "BatteryProvider.h"
#include "ContextHistoryClientEfl.h"
#include "NetworkInfoProvider.h"
#include "RequestManagerClientEfl.h"
#include "VibrationProvider.h"
#include "WKAPICast.h"
#include "WKContextSoup.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "WebContext.h"
#include "WebSoupRequestManagerProxy.h"
#include "ewk_context_private.h"
#include "ewk_cookie_manager_private.h"
#include "ewk_favicon_database_private.h"
#include "ewk_url_scheme_request_private.h"
#include <WebCore/FileSystem.h>
#include <WebCore/IconDatabase.h>
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(TIZEN)
#include "WKApplicationCacheManager.h"
#include "WKArray.h"
#include "WKContext.h"
#include "WKContextPrivate.h"
#include "WKContextTizen.h"
#include "WKDatabaseManager.h"
#include "WKIconDatabase.h"
#include "WKIconDatabaseTizen.h"
#if ENABLE(TIZEN_INDEXED_DATABASE)
#include "WKIndexedDatabaseManager.h"
#endif
#include "WKKeyValueStorageManager.h"
#if ENABLE(TIZEN_FILE_SYSTEM)
#include "WKLocalFileSystemManager.h"
#endif
#include "WKNumber.h"
#include "WKSecurityOrigin.h"
#include "WKURL.h"
#include "ewk_context_injected_bundle_client.h"
#include "ewk_security_origin.h"
#include "ewk_private.h"
#include <Eina.h>

#if ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)
#include "ProcessLauncher.h"
#include <stdlib.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#include "FormDatabase.h"
#include "WKDictionary.h"
#endif

#if ENABLE(TIZEN_COMPRESSION_PROXY)
#include <UCProxySDK/ImageQualityLevel.h>
#endif

#endif // #if PLATFORM(TIZEN)
#if ENABLE(SPELLCHECK)
#include "ewk_settings.h"
#include "ewk_text_checker_private.h"
#endif

#if ENABLE(TIZEN_HW_MORE_BACK_KEY) || ENABLE(TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_FOCUS_RING)
#include <dlfcn.h>
void* EflAssistHandle = 0;
#endif

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
#include "ewk_autofill_profile_private.h"
#include <wtf/dtoa.h>
#endif

#if ENABLE(TIZEN_GEOLOCATION)
#include "ewk_geolocation_provider_private.h"
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "ewk_notification_provider_private.h"
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
#define certificate_crt_path "/opt/share/cert-svc/ca-certificate.crt"
#endif

using namespace WebCore;
using namespace WebKit;

typedef HashMap<WKContextRef, Ewk_Context*> ContextMap;

static inline ContextMap& contextMap()
{
    DEFINE_STATIC_LOCAL(ContextMap, map, ());
    return map;
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
static void _ewk_context_default_password_confirm_popup(Evas_Object* ewkView, void* userData)
{
    if (!ewkView)
        return;
    Ewk_Settings *settings = ewk_view_settings_get(ewkView);
    if (settings && ewk_settings_form_candidate_data_enabled_get(settings)) {
        ewk_view_context_get(ewkView)->showPasswordSaveConfirmPopup(ewkView);
    }
}
#endif

Ewk_Context::Ewk_Context(WKContextRef context)
    : m_context(context)
    , m_historyClient(ContextHistoryClientEfl::create(context))
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    , m_isCachedToDisk(false)
    , m_isForeground(true)
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    , m_xWindow(0)
#endif
#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    , m_pixmap(0)
#endif
#if ENABLE(TIZEN_WEBKIT2_PROXY)
    , m_connectionHandle(0)
    , m_proxySetting(false)
#endif
{
    ContextMap::AddResult result = contextMap().add(context, this);
    ASSERT_UNUSED(result, result.isNewEntry);

#if ENABLE(BATTERY_STATUS)
    m_batteryProvider = BatteryProvider::create(context);
#endif

#if ENABLE(NETWORK_INFO)
    m_networkInfoProvider = NetworkInfoProvider::create(context);
#endif

#if ENABLE(VIBRATION)
    m_vibrationProvider = VibrationProvider::create(context);
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
    m_notificationProvider = Ewk_Notification_Provider::create(context);
#endif

#if ENABLE(MEMORY_SAMPLER)
    static bool initializeMemorySampler = false;
    static const char environmentVariable[] = "SAMPLE_MEMORY";

    if (!initializeMemorySampler && getenv(environmentVariable)) {
        WKRetainPtr<WKDoubleRef> interval(AdoptWK, WKDoubleCreate(0.0));
        WKContextStartMemorySampler(context, interval.get());
        initializeMemorySampler = true;
    }
#endif

#if ENABLE(SPELLCHECK)
    Ewk_Text_Checker::initialize();
    if (ewk_settings_continuous_spell_checking_enabled_get()) {
        // Load the default language.
        ewk_settings_spell_checking_languages_set(0);
    }
#endif

    // Initialize WKContext clients.
    m_downloadManager = DownloadManagerEfl::create(this);
    m_requestManagerClient = RequestManagerClientEfl::create(this);

#if PLATFORM(TIZEN)
#if ENABLE(TIZEN_SQL_DATABASE)
    m_defaultDatabaseQuota = 5 * 1024 * 1024;
#endif
    m_messageFromInjectedBundle.callback = 0;
    m_messageFromInjectedBundle.userData = 0;
    m_didStartDownload.callback = 0;
    m_didStartDownload.userData = 0;
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    m_passwordConfirmPopup.callback = 0;
    m_passwordConfirmPopup.userData = 0;

    m_formDatabase = FormDatabase::create();
    m_passwordSaveConfirmPopup = PasswordSaveConfirmPopup::create(this);
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
    m_profileFormAutoFill = ProfileFormAutoFill::create(m_formDatabase);
#endif // ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
#endif
#endif

#if ENABLE(TIZEN_HW_MORE_BACK_KEY) || ENABLE(TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_FOCUS_RING)
    if (!EflAssistHandle)
        EflAssistHandle = dlopen("/usr/lib/libefl-assist.so.0", RTLD_LAZY);
#endif
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    ewk_context_password_confirm_popup_callback_set(this, _ewk_context_default_password_confirm_popup, 0);
#endif
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    setCertificate(certificate_crt_path);
#endif
}

Ewk_Context::~Ewk_Context()
{
    ASSERT(contextMap().get(m_context.get()) == this);

#if PLATFORM(TIZEN)
    m_messageFromInjectedBundle.callback = 0;
    m_messageFromInjectedBundle.userData = 0;
#endif

#if ENABLE(TIZEN_INJECTED_BUNDLE_CRASH_WORKAROUND)
    TIZEN_LOGI("EwkContext will be removed");
    ewkContextInjectedBundleClientDetachClient(this);
#endif

#if ENABLE(TIZEN_WEB_STORAGE)
    syncLocalStorage();
#endif

    contextMap().remove(m_context.get());
#if ENABLE(TIZEN_CACHE_DUMP_SYNC)
    dumpCache();
#endif

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
    m_pixmap = 0;
#endif

#if ENABLE(TIZEN_WEBKIT2_PROXY)
    if (m_connectionHandle)
        connection_destroy(m_connectionHandle);
#endif
}

PassRefPtr<Ewk_Context> Ewk_Context::create(WKContextRef context)
{
    if (contextMap().contains(context))
        return contextMap().get(context); // Will be ref-ed automatically.

    return adoptRef(new Ewk_Context(context));
}

PassRefPtr<Ewk_Context> Ewk_Context::create()
{
    return create(adoptWK(WKContextCreate()).get());
}

PassRefPtr<Ewk_Context> Ewk_Context::create(const String& injectedBundlePath)
{
    if (!fileExists(injectedBundlePath))
        return 0;

    WKRetainPtr<WKStringRef> injectedBundlePathWK = adoptWK(toCopiedAPI(injectedBundlePath));
    WKRetainPtr<WKContextRef> contextWK = adoptWK(WKContextCreateWithInjectedBundlePath(injectedBundlePathWK.get()));

    return create(contextWK.get());
}

PassRefPtr<Ewk_Context> Ewk_Context::defaultContext()
{
    static RefPtr<Ewk_Context> defaultInstance = create(adoptWK(WKContextCreate()).get());

    return defaultInstance;
}

Ewk_Cookie_Manager* Ewk_Context::cookieManager()
{
    if (!m_cookieManager)
        m_cookieManager = Ewk_Cookie_Manager::create(WKContextGetCookieManager(m_context.get()));

    return m_cookieManager.get();
}

Ewk_Favicon_Database* Ewk_Context::faviconDatabase()
{
#if ENABLE(TIZEN_ICON_DATABASE) && !ENABLE(TIZEN_FAVICON)
    return 0;
#endif
    if (!m_faviconDatabase) {
        WKRetainPtr<WKIconDatabaseRef> iconDatabase = WKContextGetIconDatabase(m_context.get());
        // Set the database path if it is not open yet.
        if (!toImpl(iconDatabase.get())->isOpen()) {
            WebContext* webContext = toImpl(m_context.get());
            String databasePath = webContext->iconDatabasePath() + "/" + WebCore::IconDatabase::defaultDatabaseFilename();
            webContext->setIconDatabasePath(databasePath);
        }
        m_faviconDatabase = Ewk_Favicon_Database::create(iconDatabase.get());
    }

    return m_faviconDatabase.get();
}

RequestManagerClientEfl* Ewk_Context::requestManager()
{
    return m_requestManagerClient.get();
}

#if ENABLE(VIBRATION)
PassRefPtr<VibrationProvider> Ewk_Context::vibrationProvider()
{
    return m_vibrationProvider;
}
#endif

#if ENABLE(TIZEN_GEOLOCATION)
Ewk_Geolocation_Provider* Ewk_Context::geolocationProvider()
{
    if (!m_geolocationProvider)
        m_geolocationProvider = Ewk_Geolocation_Provider::create(m_context.get());

    return m_geolocationProvider.get();
}
#endif

void Ewk_Context::addVisitedLink(const String& visitedURL)
{
    toImpl(m_context.get())->addVisitedLink(visitedURL);
}

void Ewk_Context::setCacheModel(Ewk_Cache_Model cacheModel)
{
    WKContextSetCacheModel(m_context.get(), static_cast<Ewk_Cache_Model>(cacheModel));
}

Ewk_Cache_Model Ewk_Context::cacheModel() const
{
    return static_cast<Ewk_Cache_Model>(WKContextGetCacheModel(m_context.get()));
}

Ewk_Context* ewk_context_ref(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);

    ewkContext->ref();

    return ewkContext;
}

void ewk_context_unref(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    ewkContext->deref();
}

Ewk_Cookie_Manager* ewk_context_cookie_manager_get(const Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);

    return const_cast<Ewk_Context*>(ewkContext)->cookieManager();
}

Ewk_Favicon_Database* ewk_context_favicon_database_get(const Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);

    return const_cast<Ewk_Context*>(ewkContext)->faviconDatabase();
}

WKContextRef Ewk_Context::wkContext()
{
#if ENABLE(TIZEN_EWK_CONTEXT_CRASH_PATCH)
    if (m_context)
#endif
        return m_context.get();
    return 0;
}

DownloadManagerEfl* Ewk_Context::downloadManager() const
{
    return m_downloadManager.get();
}

ContextHistoryClientEfl* Ewk_Context::historyClient()
{
    return m_historyClient.get();
}

#if PLATFORM(TIZEN)
bool Ewk_Context::setProxyAddress(const char* proxyAddress)
{
    if (m_proxyAddress == proxyAddress)
        return false;

    TIZEN_SECURE_LOGI("proxyAddress [%s]", proxyAddress);

    m_proxyAddress = proxyAddress;
    return true;
}

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
bool Ewk_Context::setCertificateFile(const char* certificateFile)
{
    if (m_certificateFile == certificateFile)
        return false;

    m_certificateFile = certificateFile;
    return true;
}
#endif

void Ewk_Context::setDefaultDatabaseQuota(uint64_t defaultDatabaseQuota)
{
    m_defaultDatabaseQuota = defaultDatabaseQuota;
}

void Ewk_Context::setMessageFromInjectedBundleCallback(Ewk_Context_Message_From_Injected_Bundle_Callback callback, void* userData)
{
    TIZEN_LOGI("callback:%p, userData:%p", callback, userData);
    m_messageFromInjectedBundle.callback = callback;
    m_messageFromInjectedBundle.userData = userData;
}

void Ewk_Context::didReceiveMessageFromInjectedBundle(WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData)
{
    if (!m_messageFromInjectedBundle.callback)
        return;

    CString name = toImpl(messageName)->string().utf8();
    CString body;
    if (messageBody && WKStringGetTypeID() == WKGetTypeID(messageBody))
        body = toImpl(static_cast<WKStringRef>(messageBody))->string().utf8();

    if (returnData) {
        char* returnString = 0;
        m_messageFromInjectedBundle.callback(name.data(), body.data(), &returnString,
                                                       m_messageFromInjectedBundle.userData);
        if (returnString) {
            *returnData = WKStringCreateWithUTF8CString(returnString);
            free(returnString);
        } else
            *returnData = WKStringCreateWithUTF8CString("");
    } else
        m_messageFromInjectedBundle.callback(name.data(), body.data(), 0, m_messageFromInjectedBundle.userData);
}

void Ewk_Context::setDidStartDownloadCallback(Ewk_Context_Did_Start_Download_Callback callback, void* userData)
{
    m_didStartDownload.callback = callback;
    m_didStartDownload.userData = userData;
}

void Ewk_Context::didStartDownload(WKStringRef downloadURL)
{
    EINA_SAFETY_ON_NULL_RETURN(m_didStartDownload.callback);

    m_didStartDownload.callback(toImpl(downloadURL)->string().utf8().data(), m_didStartDownload.userData);
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void Ewk_Context::setPassworSaveConfirmPopupCallback(Ewk_Context_Password_Confirm_Popup_Callback callback, void* userData)
{
    m_passwordConfirmPopup.callback = callback;
    m_passwordConfirmPopup.userData = userData;
}

void Ewk_Context::passworSaveConfirmPopupCallbackCall(Evas_Object* ewkView)
{
    EINA_SAFETY_ON_NULL_RETURN(m_passwordConfirmPopup.callback);

    m_passwordConfirmPopup.callback(ewkView, m_passwordConfirmPopup.userData);
}
#endif

#if ENABLE(TIZEN_LITE_MEMORYCACHE_OPTIMIZATION)
void Ewk_Context::clearAllInMemoryResources()
{
    WKResourceCacheManagerRef cacheManager = WKContextGetResourceCacheManager(wkContext());
    WKResourceCacheManagerClearCacheForAllOrigins(cacheManager, WKResourceCachesToClearInMemoryOnly);
}
#endif

#if ENABLE(TIZEN_CACHE_MEMORY_OPTIMIZATION)
void Ewk_Context::clearAllLiveDecodedResources()
{
    WKResourceCacheManagerRef cacheManager = WKContextGetResourceCacheManager(wkContext());
    WKResourceCacheManagerClearCacheForAllOrigins(cacheManager, WKResourceCachesToClearLiveDecodedResourcesOnly);
}

void Ewk_Context::clearAllLiveEncodedResources()
{
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    if (m_isCachedToDisk || m_isForeground)
        return;
#endif
    WKResourceCacheManagerRef cacheManager = WKContextGetResourceCacheManager(wkContext());
    WKResourceCacheManagerClearCacheForAllOrigins(cacheManager, WKResourceCachesToClearLiveEncodedResourcesOnly);
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    m_isCachedToDisk = true;
#endif
}

void Ewk_Context::clearAllDeadResources()
{
    WKResourceCacheManagerRef cacheManager = WKContextGetResourceCacheManager(wkContext());
    WKResourceCacheManagerClearCacheForAllOrigins(cacheManager, WKResourceCachesToClearDeadResourcesOnly);
}
#endif

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
void Ewk_Context::restoreAllEncodedResources()
{
    if (!m_isCachedToDisk)
        return;

    WKResourceCacheManagerRef cacheManager = WKContextGetResourceCacheManager(wkContext());
    WKResourceCacheManagerRestoreCacheForAllOrigins(cacheManager);
    m_isCachedToDisk = false;
}

void Ewk_Context::markVisibleContents(WebCore::IntRect& visibleRect)
{
    WKResourceCacheManagerRef cacheManager = WKContextGetResourceCacheManager(wkContext());
    WKResourceCacheManagerMarkVisibleContents(cacheManager, visibleRect);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
Ecore_X_Window Ewk_Context::xWindow()
{
    return m_xWindow;
}

void Ewk_Context::setXWindow(Ecore_X_Window xWindow)
{
    m_xWindow = xWindow;
    toImpl(wkContext())->setXWindow(xWindow);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void Ewk_Context::hidePasswordSaveConfirmPopup()
{
    m_passwordSaveConfirmPopup->hide();
}

void Ewk_Context::showPasswordSaveConfirmPopup(Evas_Object* ewkView)
{
    m_passwordSaveConfirmPopup->show(ewkView);
}

void Ewk_Context::setPasswordData(const String& url, WKFormDataRef formData)
{
    m_passwordSaveConfirmPopup->setPasswordData(url, formData);
}

void Ewk_Context::allowSavePassword()
{
    m_passwordSaveConfirmPopup->savePasswordData();
}

void Ewk_Context::cancelSavePassword()
{
    m_passwordSaveConfirmPopup->clearPasswordData();
}

void Ewk_Context::neverSavePassword()
{
    m_passwordSaveConfirmPopup->neverSavePasswordData();
}

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
bool Ewk_Context::clearProfileFormData(unsigned id)
{
    NumberToStringBuffer buffer;
    String profileID = numberToString(id, buffer);
    return m_formDatabase->clearProfileFormData(profileID);
}

bool Ewk_Context::addProfileFormData(Ewk_Autofill_Profile* profile, Vector<std::pair<String, String> >& textFormData)
{
    if (profile && m_formDatabase) {
        NumberToStringBuffer buffer;
        String profileID = numberToString(profile->profileID, buffer);
        if (profileID.isEmpty())
            return false;
        return m_formDatabase->addProfileFormData(profileID, textFormData);
    }
    return false;
}

Ewk_Autofill_Profile* Ewk_Context::getProfileFormDataByProfileId(unsigned id)
{
    NumberToStringBuffer buffer;
    String profileIDString = numberToString(id, buffer);
    if(profileIDString.isEmpty())
        return NULL;

    Vector<std::pair<String, String> > textFormData;
    m_formDatabase->getProfileFormDataForProfile(profileIDString, textFormData);
    Ewk_Autofill_Profile* profileData = ewk_autofill_profile_new();

    if (!profileData)
        return NULL;

    profileData->profileID = id;
    Vector <std::pair<Ewk_Autofill_Profile_Data_Type, WKEinaSharedString> > nameValuePair;
    Vector<std::pair<String, String> >::iterator end = textFormData.end();

    for (Vector<std::pair<String, String> >::iterator it = textFormData.begin(); it != end; ++it) {
        int enumTypeForProfileName = getProfileFormEnumFromName(it->first);
        if (enumTypeForProfileName == -1) {
            TIZEN_LOGE("Ewk_Context::getProfileFormDataByProfileId ::return value is -1 : Id is not proper");
            return NULL;
        }

        Ewk_Autofill_Profile_Data_Type name = static_cast <Ewk_Autofill_Profile_Data_Type>(enumTypeForProfileName);
        WKEinaSharedString value(it->second.utf8().data());
        nameValuePair.append(std::pair<Ewk_Autofill_Profile_Data_Type, WKEinaSharedString>(name, value));
    }
    profileData->nameValuePair = nameValuePair;
    return profileData;
}

String Ewk_Context::getProfileFormNameFromEnum(Ewk_Autofill_Profile_Data_Type name)
{
    return autofillProfileNameAttributes[name];
}

int Ewk_Context::getProfileFormEnumFromName(String name)
{
    for (int i = 0; i < static_cast<int>(EWK_MAX_AUTOFILL); ++i) {
        if (name == autofillProfileNameAttributes[i])
            return i;
    }
    return -1;
}

void Ewk_Context::getAllProfileIDForProfileFormData(Vector<String>& list)
{
    m_formDatabase->getAllProfileIDForProfileFormData(list);
}


void Ewk_Context::candidateProfileFormData(const String& name, const String& value, Vector<std::pair<int, std::pair<String, String> > > & autoFillTextData)
{
    m_profileFormAutoFill->getProfileFormCandidateData(name, value, autoFillTextData);
}

void Ewk_Context::setProfileFormCanditateToForm(const unsigned & profileID, String & fillScript)
{
    m_profileFormAutoFill->setProfileFormCanditateToWebForm(profileID, fillScript);
}

Eina_List* Ewk_Context::getAllProfilesForProfileForm()
{
    Vector<String> profileList;
    getAllProfileIDForProfileFormData(profileList);
    if (profileList.isEmpty())
        return NULL;

    Eina_List* profile = NULL;
    for (size_t i=0; i<profileList.size(); i++) {
        Vector<std::pair<String, String> > textFormData;
        m_formDatabase->getProfileFormDataForProfile(profileList[i], textFormData);
        Ewk_Autofill_Profile* profileData = ewk_autofill_profile_new();
        if (!profileData)
            return NULL;
        profileData->profileID = atoi(profileList[i].utf8().data());

        Vector <std::pair<Ewk_Autofill_Profile_Data_Type, WKEinaSharedString> > nameValuePair;
        Vector <std::pair<String, String> >::iterator end = textFormData.end();
        for (Vector<std::pair<String, String> >::iterator it = textFormData.begin(); it != end; ++it) {
            int enumTypeForProfileName = getProfileFormEnumFromName(it->first);
            if (enumTypeForProfileName == -1) {
                TIZEN_LOGE("Ewk_Context::getAllProfilesForProfileForm ::return value is -1 : Name attribute is not proper.");
                return NULL;
            }
            Ewk_Autofill_Profile_Data_Type name = static_cast <Ewk_Autofill_Profile_Data_Type>(enumTypeForProfileName);
            WKEinaSharedString value(it->second.utf8().data());
            nameValuePair.append(std::pair<Ewk_Autofill_Profile_Data_Type, WKEinaSharedString>(name, value));
        }
        profileData->nameValuePair = nameValuePair;
        profile = eina_list_append(profile, profileData);
    }
    return profile;
}

#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif

#if ENABLE(TIZEN_CACHE_DUMP_SYNC)
/**
 * @internal
 * Request WebProcess to dump cache.
 *
 * This sends sync message to WebProcess to dump memory cache, that is, soup cache.
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 *
 * @note This can effect UIProcess's performance because it calls to sync IPC message eventually.
 */
void Ewk_Context::dumpCache()
{
    WKResourceCacheManagerRef cacheManager = WKContextGetResourceCacheManager(wkContext());
    toImpl(cacheManager)->dumpCache();
}
#endif

#if ENABLE(TIZEN_WEB_STORAGE)
/**
 * @internal
 * Request WebProcess to sync local storage.
 *
 * This sends message to WebProcess to sync local storage.
 */
void Ewk_Context::syncLocalStorage()
{
    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(wkContext());
    WKKeyValueStorageManagerSyncKeyValueStorage(storageManager);
}
#endif

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
void Ewk_Context::setPixmap(int pixmap)
{
    m_pixmap = pixmap;
}

int Ewk_Context::pixmap()
{
    return m_pixmap;
}
#endif

#if ENABLE(TIZEN_EXTENSIBLE_API)
int Ewk_Context::getExtensibleAPIEnumFromName(const char* extensibleName)
{
    String extensibleString = String::fromUTF8(extensibleName);

    for (int i = 0; i < static_cast<int>(EWK_MAX_EXTENSIBLE_API); ++i) {
        if (equalIgnoringCase(extensibleString, tizenExtensibleAPINames[i]))
            return i;
    }
    return -1;
}
#endif
#endif

Ewk_Context* ewk_context_default_get()
{
    return Ewk_Context::defaultContext().get();
}

#if PLATFORM(TIZEN)
typedef struct Ewk_Context_Callback_Context
{
    union {
#if ENABLE(TIZEN_FILE_SYSTEM)
        Ewk_Local_File_System_Origins_Get_Callback localFileSystemOriginsCallback;
#endif
        Ewk_Web_Application_Cache_Origins_Get_Callback webApplicationCacheOriginsCallback;
        Ewk_Web_Application_Cache_Quota_Get_Callback webApplicationCacheQuotaCallback;
        Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback webApplicationCacheUsageForOriginCallback;
        Ewk_Web_Application_Cache_Path_Get_Callback webApplicationCachePathCallback;
        Ewk_Web_Database_Origins_Get_Callback webDatabaseOriginsCallback;
        Ewk_Web_Database_Quota_Get_Callback webDatabaseQuotaCallback;
        Ewk_Web_Database_Usage_Get_Callback webDatabaseUsageCallback;
        Ewk_Web_Database_Path_Get_Callback webDatabasePathCallback;
#if ENABLE(TIZEN_INDEXED_DATABASE)
        Ewk_Web_Indexed_Database_Origins_Get_Callback webIndexedDatabaseOriginsCallback;
#endif
        Ewk_Web_Storage_Origins_Get_Callback webStorageOriginsCallback;
        Ewk_Web_Storage_Usage_Get_Callback webStorageUsageCallback;
        Ewk_Web_Storage_Path_Get_Callback webStoragePathCallback;
    };
    void* userData;
} Ewk_Context_Callback_Context;

/* public */
Ewk_Context* ewk_context_new()
{
#if PLATFORM(TIZEN)
    Ewk_Context* ewkContext = Ewk_Context::create().leakRef();
    ewkContextInjectedBundleClientAttachClient(ewkContext);
#if ENABLE(TIZEN_PROCESS_PERMISSION_CONTROL)
    const char* webProcessExecutablePath = getenv("WEB_PROCESS_EXECUTABLE_PATH");
    if (webProcessExecutablePath) {
        WKContextRef contextRef = ewkContext->wkContext();
        toImpl(contextRef)->setWebProcessExecutablePath(String::fromUTF8(webProcessExecutablePath));
    }

    const char* pluginProcessExecutablePath = getenv("PLUGIN_PROCESS_EXECUTABLE_PATH");
    if (pluginProcessExecutablePath) {
        WKContextRef contextRef = ewkContext->wkContext();
        toImpl(contextRef)->pluginInfoStore().setExecutablePath(String::fromUTF8(pluginProcessExecutablePath));
    }
#endif
    return ewkContext;
#else
    return Ewk_Context::create().leakRef();
#endif
}

Ewk_Context* ewk_context_new_with_injected_bundle_path(const char* path)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, 0);

#if ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)
    char* wrtLaunchingPerformance = getenv("WRT_LAUNCHING_PERFORMANCE");
    if (wrtLaunchingPerformance && !strcmp(wrtLaunchingPerformance, "1")) {
        static bool firstTime = true;
        if (firstTime) {
            firstTime = false;

            if (ProcessLauncher::isInitialFork())
                ProcessLauncher::setSkipExec(true);
            else
                ProcessLauncher::setSkipExec(false);

            ProcessLauncher::forkProcess();

            if (ProcessLauncher::isParentProcess()) {
                Ewk_Context* ewkContext = ewk_context_new_with_injected_bundle_path(path);
                WKContextRef contextRef = ewkContext->wkContext();
                toImpl(contextRef)->ensureWebProcess();
                return ewkContext;
            }
            else if (ProcessLauncher::isChildProcess()) {
                ProcessLauncher::callWebProcessMain();
                exit(0);
            }

            ASSERT_NOT_REACHED();
            return 0;
        }
    }
#endif

#if PLATFORM(TIZEN)
    Ewk_Context* ewkContext = Ewk_Context::create(String::fromUTF8(path)).leakRef();
    ewkContextInjectedBundleClientAttachClient(ewkContext);
#if ENABLE(TIZEN_PROCESS_PERMISSION_CONTROL)
    const char* webProcessExecutablePath = getenv("WEB_PROCESS_EXECUTABLE_PATH");
    if (webProcessExecutablePath) {
        WKContextRef contextRef = ewkContext->wkContext();
        toImpl(contextRef)->setWebProcessExecutablePath(String::fromUTF8(webProcessExecutablePath));
    }

    const char* pluginProcessExecutablePath = getenv("PLUGIN_PROCESS_EXECUTABLE_PATH");
    if (pluginProcessExecutablePath) {
        WKContextRef contextRef = ewkContext->wkContext();
        toImpl(contextRef)->pluginInfoStore().setExecutablePath(String::fromUTF8(pluginProcessExecutablePath));
    }
#endif
    return ewkContext;
#else
    return Ewk_Context::create(String::fromUTF8(path)).leakRef();
#endif
}

void ewk_context_delete(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    if (ewkContext == ewk_context_default_get() && ewkContext->hasOneRef())
        return;

    delete ewkContext;
}

// This function will be removed soon because the proxy setting feature is internalized.
void ewk_context_proxy_uri_set(Ewk_Context* ewkContext, const char* proxy)
{
    return;
}

const char* ewk_context_proxy_uri_get(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_WEBKIT2_PATCH_FOR_TC)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);
#endif
    return ewkContext->proxyAddress();
}

void ewk_context_network_session_requests_cancel(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    TIZEN_LOGI("ewk_context_network_session_requests_cancel() is called.");
    WKContextRef contextRef = ewkContext->wkContext();
    toImpl(contextRef)->abortSession();
}

Eina_Bool ewk_context_notify_low_memory(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

#if ENABLE(TIZEN_LITE_MEMORYCACHE_OPTIMIZATION)
    ewkContext->clearAllInMemoryResources();
#endif
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    ewkContext->clearAllLiveEncodedResources();
#endif
    WKContextRef contextRef = ewkContext->wkContext();
    toImpl(contextRef)->notifyLowMemory();
    return true;
}

#if ENABLE(TIZEN_FILE_SYSTEM)
static void didGetLocalFileSystemOrigins(WKArrayRef origins, WKErrorRef error, void* context)
{
    Eina_List* originList = 0;
    for (size_t i = 0; i < WKArrayGetSize(origins); ++i) {
        WKSecurityOriginRef securityOriginRef = static_cast<WKSecurityOriginRef>(WKArrayGetItemAtIndex(origins, i));
        Ewk_Security_Origin* origin = createSecurityOrigin(securityOriginRef);
        originList = eina_list_append(originList, origin);
    }

    Ewk_Context_Callback_Context* locaFileSystemContext = static_cast<Ewk_Context_Callback_Context*>(context);
    locaFileSystemContext->localFileSystemOriginsCallback(originList, locaFileSystemContext->userData);
    delete locaFileSystemContext;
}
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
static void didGetWebIndexedDatabaseOrigins(WKArrayRef origins, WKErrorRef error, void* context)
{
    TIZEN_LOGI("origin size(%d)", WKArrayGetSize(origins));
    Eina_List* originList = 0;
    for (size_t i = 0; i < WKArrayGetSize(origins); ++i) {
        WKSecurityOriginRef securityOriginRef = static_cast<WKSecurityOriginRef>(WKArrayGetItemAtIndex(origins, i));
        Ewk_Security_Origin* origin = createSecurityOrigin(securityOriginRef);
        originList = eina_list_append(originList, origin);
    }

    Ewk_Context_Callback_Context* indexedDatabaseContext = static_cast<Ewk_Context_Callback_Context*>(context);
    indexedDatabaseContext->webIndexedDatabaseOriginsCallback(originList, indexedDatabaseContext->userData);
    delete indexedDatabaseContext;
}
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
static void didGetWebApplicationOrigins(WKArrayRef origins, WKErrorRef error, void* context)
{
    TIZEN_LOGI("origin size(%d)", WKArrayGetSize(origins));
    Eina_List* originList = 0;

    for(size_t i = 0; i < WKArrayGetSize(origins); i++) {
        WKSecurityOriginRef securityOriginRef = static_cast<WKSecurityOriginRef>(WKArrayGetItemAtIndex(origins, i));
        Ewk_Security_Origin* origin = createSecurityOrigin(securityOriginRef);
        originList = eina_list_append(originList, origin);
    }

    Ewk_Context_Callback_Context* applicationCacheContext = static_cast<Ewk_Context_Callback_Context*>(context);
    applicationCacheContext->webApplicationCacheOriginsCallback(originList, applicationCacheContext->userData);
    delete applicationCacheContext;
}

static void didGetWebApplicationPath(WKStringRef path, WKErrorRef error, void* context)
{
    Ewk_Context_Callback_Context* applicationCacheContext = static_cast<Ewk_Context_Callback_Context*>(context);

    int length = WKStringGetMaximumUTF8CStringSize(path);
    OwnArrayPtr<char> applicationCachePath = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(path, applicationCachePath.get(), length);

    TIZEN_LOGI("path (%s)", applicationCachePath.get());
    applicationCacheContext->webApplicationCachePathCallback(eina_stringshare_add(applicationCachePath.get()), applicationCacheContext->userData);
    delete applicationCacheContext;
}

#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
static void didGetWebApplicationQuota(WKInt64Ref quota, WKErrorRef error, void* context)
{
    Ewk_Context_Callback_Context* applicationCacheContext = static_cast<Ewk_Context_Callback_Context*>(context);
    TIZEN_LOGI("quota (%d)", toImpl(quota)->value());
    applicationCacheContext->webApplicationCacheQuotaCallback(toImpl(quota)->value(), applicationCacheContext->userData);
    delete applicationCacheContext;
}

static void didGetWebApplicationUsageForOrigin(WKInt64Ref usage, WKErrorRef error, void* context)
{
    Ewk_Context_Callback_Context* applicationCacheContext = static_cast<Ewk_Context_Callback_Context*>(context);
    TIZEN_LOGI("usage (%d)", toImpl(usage)->value());
    applicationCacheContext->webApplicationCacheUsageForOriginCallback(toImpl(usage)->value(), applicationCacheContext->userData);
    delete applicationCacheContext;
}
#endif
#endif

#if ENABLE(TIZEN_SQL_DATABASE)
static void didGetWebDatabaseOrigins(WKArrayRef origins, WKErrorRef error, void* context)
{
    TIZEN_LOGI("origin size(%d)", WKArrayGetSize(origins));
    Eina_List* originList = 0;

    for(size_t i = 0; i < WKArrayGetSize(origins); i++) {
        WKSecurityOriginRef securityOriginRef = static_cast<WKSecurityOriginRef>(WKArrayGetItemAtIndex(origins, i));
        Ewk_Security_Origin* origin = createSecurityOrigin(securityOriginRef);
        originList = eina_list_append(originList, origin);
    }

    Ewk_Context_Callback_Context* webDatabaseContext = static_cast<Ewk_Context_Callback_Context*>(context);
    webDatabaseContext->webDatabaseOriginsCallback(originList, webDatabaseContext->userData);
    delete webDatabaseContext;
}

static void didGetWebDatabaseQuota(WKUInt64Ref quota, WKErrorRef error, void* context)
{
    Ewk_Context_Callback_Context* webDatabaseContext = static_cast<Ewk_Context_Callback_Context*>(context);
    TIZEN_LOGI("quota (%d)", toImpl(quota)->value());
    webDatabaseContext->webDatabaseQuotaCallback(toImpl(quota)->value(), webDatabaseContext->userData);
    delete webDatabaseContext;
}

static void didGetWebDatabaseUsage(WKUInt64Ref usage, WKErrorRef error, void* context)
{
    Ewk_Context_Callback_Context* webDatabaseContext = static_cast<Ewk_Context_Callback_Context*>(context);
    TIZEN_LOGI("usage (%d)", toImpl(usage)->value());
    webDatabaseContext->webDatabaseUsageCallback(toImpl(usage)->value(), webDatabaseContext->userData);
    delete webDatabaseContext;
}

static void didGetWebDatabasePath(WKStringRef path, WKErrorRef error, void * context)
{
    Ewk_Context_Callback_Context* webDatabaseContext = static_cast<Ewk_Context_Callback_Context*>(context);

    int length = WKStringGetMaximumUTF8CStringSize(path);
    OwnArrayPtr<char> databasePath = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(path, databasePath.get(), length);

    TIZEN_LOGI("path (%s)", databasePath.get());
    webDatabaseContext->webDatabasePathCallback(eina_stringshare_add(databasePath.get()), webDatabaseContext->userData);
    delete webDatabaseContext;
}
#endif

#if ENABLE(TIZEN_WEB_STORAGE)
static void didGetWebStorageOrigins(WKArrayRef origins, WKErrorRef error, void* context)
{
    TIZEN_LOGI("origin size(%d)", WKArrayGetSize(origins));
    Eina_List* originList = 0;

    for(size_t i = 0; i < WKArrayGetSize(origins); i++) {
        WKSecurityOriginRef securityOriginRef = static_cast<WKSecurityOriginRef>(WKArrayGetItemAtIndex(origins, i));
        Ewk_Security_Origin* origin = createSecurityOrigin(securityOriginRef);
        originList = eina_list_append(originList, origin);
    }

    Ewk_Context_Callback_Context* webStorageContext = static_cast<Ewk_Context_Callback_Context*>(context);
    webStorageContext->webStorageOriginsCallback(originList, webStorageContext->userData);
    delete webStorageContext;
}

static void didGetWebStoragePath(WKStringRef path, WKErrorRef error, void * context)
{
    Ewk_Context_Callback_Context* webStorageContext = static_cast<Ewk_Context_Callback_Context*>(context);

    int length = WKStringGetMaximumUTF8CStringSize(path);
    OwnArrayPtr<char> storagePath = adoptArrayPtr(new char[length]);
    WKStringGetUTF8CString(path, storagePath.get(), length);

    TIZEN_LOGI("path (%s)", storagePath.get());
    webStorageContext->webStoragePathCallback(eina_stringshare_add(storagePath.get()), webStorageContext->userData);
    delete webStorageContext;
}

#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
static void didGetWebStorageUsage(WKInt64Ref usage, WKErrorRef error, void* context)
{
    Ewk_Context_Callback_Context* webStorageContext = static_cast<Ewk_Context_Callback_Context*>(context);

    TIZEN_LOGI("usage (%d)", toImpl(usage)->value());
    webStorageContext->webStorageUsageCallback(toImpl(usage)->value(), webStorageContext->userData);
    delete webStorageContext;
}
#endif

#endif

Eina_Bool ewk_context_origins_free(Eina_List* originList)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(originList, false);

    void* currentOrigin;
    EINA_LIST_FREE(originList, currentOrigin) {
        Ewk_Security_Origin* origin = static_cast<Ewk_Security_Origin*>(currentOrigin);
        deleteSecurityOrigin(origin);
    }

    return true;
}

Eina_Bool ewk_context_application_cache_delete_all(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    TIZEN_LOGI("ewkContext (%p)", ewkContext);
    WKApplicationCacheManagerRef applicationCache = WKContextGetApplicationCacheManager(ewkContext->wkContext());
    WKApplicationCacheManagerDeleteAllEntries(applicationCache);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_delete(Ewk_Context* ewkContext, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> securityOriginRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKApplicationCacheManagerRef applicationCacheRef = WKContextGetApplicationCacheManager(ewkContext->wkContext());
    WKApplicationCacheManagerDeleteEntriesForOrigin(applicationCacheRef, securityOriginRef.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_origins_get(Ewk_Context* ewkContext, Ewk_Web_Application_Cache_Origins_Get_Callback callback, void *userData)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("ewkContext (%p)", ewkContext);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webApplicationCacheOriginsCallback = callback;
    context->userData = userData;

    WKApplicationCacheManagerRef applicationCacheRef = WKContextGetApplicationCacheManager(ewkContext->wkContext());
    WKApplicationCacheManagerGetApplicationCacheOrigins(applicationCacheRef, context, didGetWebApplicationOrigins);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    if (!path) {
        TIZEN_LOGE("Path value is invalid");
        return false;
    }

    TIZEN_LOGI("path (%s)", path);
    WKRetainPtr<WKStringRef> applicationCachePathRef(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetApplicationCacheDirectory(ewkContext->wkContext(), applicationCachePathRef.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_path_get(Ewk_Context* ewkContext, Ewk_Web_Application_Cache_Path_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("callback (%p)", callback);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webApplicationCachePathCallback= callback;
    context->userData = userData;

    WKApplicationCacheManagerRef applicationCacheRef = WKContextGetApplicationCacheManager(ewkContext->wkContext());
    WKApplicationCacheManagerGetApplicationCachePath(applicationCacheRef, context, didGetWebApplicationPath);
    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_quota_get(Ewk_Context* ewkContext, Ewk_Web_Application_Cache_Quota_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_APPLICATION_CACHE) && ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("callback (%p)", callback);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webApplicationCacheQuotaCallback = callback;
    context->userData = userData;

    WKApplicationCacheManagerRef applicationCacheRef = WKContextGetApplicationCacheManager(ewkContext->wkContext());
    WKApplicationCacheManagerGetApplicationCacheQuota(applicationCacheRef, context, didGetWebApplicationQuota);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_usage_for_origin_get(Ewk_Context* ewkContext, const Ewk_Security_Origin* origin, Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_APPLICATION_CACHE) && ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webApplicationCacheUsageForOriginCallback = callback;
    context->userData = userData;
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));

    WKApplicationCacheManagerRef applicationCache = WKContextGetApplicationCacheManager(ewkContext->wkContext());
    WKApplicationCacheManagerGetApplicationCacheUsageForOrigin(applicationCache, context, originRef.get(), didGetWebApplicationUsageForOrigin);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_quota_set(Ewk_Context* ewkContext, int64_t quota)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    if (!quota) {
        TIZEN_LOGE("Quota value is invalid");
        return false;
    }

    TIZEN_LOGI("quota (%d)", quota);
    WKApplicationCacheManagerRef applicationCacheRef = WKContextGetApplicationCacheManager(ewkContext->wkContext());
    WKApplicationCacheManagerSetApplicationCacheQuota(applicationCacheRef, quota);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_application_cache_quota_for_origin_set(Ewk_Context* ewkContext, const Ewk_Security_Origin* origin, int64_t quota)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);
    if (!quota) {
        TIZEN_LOGE("Quota value is invalid");
        return false;
    }

    TIZEN_LOGI("quota (%d)", quota);
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));

    WKApplicationCacheManagerRef applicationCache = WKContextGetApplicationCacheManager(ewkContext->wkContext());
    WKApplicationCacheManagerSetApplicationCacheQuotaForOrigin(applicationCache, originRef.get(), quota);

    return true;
#else
    return false;
#endif
}


Eina_Bool ewk_context_icon_database_path_set(Ewk_Context* ewkContext, const char* path)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    WKRetainPtr<WKStringRef> databasePath(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetIconDatabasePath(ewkContext->wkContext(), databasePath.get());

    return true;
}

Evas_Object* ewk_context_icon_database_icon_object_add(Ewk_Context* ewkContext, const char* uri, Evas* canvas)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(uri, 0);
    EINA_SAFETY_ON_NULL_RETURN_VAL(canvas, 0);

#if ENABLE(TIZEN_FAVICON)
    return ewk_favicon_database_icon_get(ewk_context_favicon_database_get(ewkContext), uri, canvas);
#else
    WKIconDatabaseRef iconDatabase = WKContextGetIconDatabase(ewkContext->wkContext());
    WKRetainPtr<WKURLRef> urlString(AdoptWK, WKURLCreateWithUTF8CString(uri));

    return WKIconDatabaseTryGetImageForURL(iconDatabase, canvas, urlString.get());
#endif
}

void ewk_context_icon_database_delete_all(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    WKIconDatabaseRef iconDatabase = WKContextGetIconDatabase(ewkContext->wkContext());
    WKIconDatabaseRemoveAllIcons(iconDatabase);
}

Eina_Bool ewk_context_local_file_system_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(TIZEN_FILE_SYSTEM)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    TIZEN_LOGI("path (%s)", path);
    WKRetainPtr<WKStringRef> localFileSystemPathRef(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetLocalFileSystemDirectory(ewkContext->wkContext(), localFileSystemPathRef.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_local_file_system_all_delete(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_FILE_SYSTEM)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    TIZEN_LOGI("ewkContex (%p)", ewkContext);
    WKLocalFileSystemManagerRef localFileSystemManager = WKContextGetLocalFileSystemManager(ewkContext->wkContext());
    WKLocalFileSystemManagerDeleteAllLocalFileSystem(localFileSystemManager);

    return true;
#else
    UNUSED_PARAM(ewkContext);

    return false;
#endif
}

Eina_Bool ewk_context_local_file_system_delete(Ewk_Context* ewkContext, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_FILE_SYSTEM)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKLocalFileSystemManagerRef localFileSystemManager = WKContextGetLocalFileSystemManager(ewkContext->wkContext());

    WKLocalFileSystemManagerDeleteLocalFileSystem(localFileSystemManager, originRef.get());

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(origin);

    return false;
#endif
}

Eina_Bool ewk_context_local_file_system_origins_get(const Ewk_Context* ewkContext, Ewk_Local_File_System_Origins_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_FILE_SYSTEM)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("callback (%p)", callback);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->localFileSystemOriginsCallback= callback;
    context->userData = userData;
    WKLocalFileSystemManagerRef localFileSystemManager = WKContextGetLocalFileSystemManager(const_cast<Ewk_Context*>(ewkContext)->wkContext());

    WKLocalFileSystemManagerGetLocalFileSystemOrigins(localFileSystemManager, context, didGetLocalFileSystemOrigins);

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);

    return false;
#endif
}

Eina_Bool ewk_context_web_database_delete_all(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    TIZEN_LOGI("ewkContext (%p)", ewkContext);
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerDeleteAllDatabases(databaseManager);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_database_delete(Ewk_Context* ewkContext, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerDeleteDatabasesForOrigin(databaseManager, originRef.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_database_origins_get(Ewk_Context* ewkContext, Ewk_Web_Database_Origins_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("callback (%p)", callback);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webDatabaseOriginsCallback = callback;
    context->userData = userData;

    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerGetDatabaseOrigins(databaseManager, context, didGetWebDatabaseOrigins);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_database_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    TIZEN_LOGI("path (%s)", path);
    WKRetainPtr<WKStringRef> databasePath(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetDatabaseDirectory(ewkContext->wkContext(), databasePath.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_database_path_get(Ewk_Context* ewkContext, Ewk_Web_Database_Path_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("callback (%p)", callback);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webDatabasePathCallback= callback;
    context->userData = userData;

    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerGetDatabasePath(databaseManager, context, didGetWebDatabasePath);
    return true;
#else
    return false;
#endif
}
Eina_Bool ewk_context_web_database_quota_for_origin_get(Ewk_Context* ewkContext, Ewk_Web_Database_Quota_Get_Callback callback, void* userData, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webDatabaseQuotaCallback = callback;
    context->userData = userData;

    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerGetQuotaForOrigin(databaseManager, context, didGetWebDatabaseQuota, originRef.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_database_default_quota_set(Ewk_Context* ewkContext, uint64_t quota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    TIZEN_LOGI("quota (%d)", quota);
    ewkContext->setDefaultDatabaseQuota(quota);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_database_quota_for_origin_set(Ewk_Context* ewkContext, Ewk_Security_Origin* origin, uint64_t quota)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerSetQuotaForOrigin(databaseManager, originRef.get(), quota);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_database_usage_for_origin_get(Ewk_Context* ewkContext, Ewk_Web_Database_Quota_Get_Callback callback, void* userData, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webDatabaseQuotaCallback = callback;
    context->userData = userData;

    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKDatabaseManagerRef databaseManager = WKContextGetDatabaseManager(ewkContext->wkContext());
    WKDatabaseManagerGetUsageForOrigin(databaseManager, context, didGetWebDatabaseUsage, originRef.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_indexed_database_delete_all(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    TIZEN_LOGI("ewkContex (%p)", ewkContext);
    WKIndexedDatabaseManagerRef indexedDatabaseManager = WKContextGetIndexedDatabaseManager(ewkContext->wkContext());
    WKIndexedDatabaseManagerDeleteAllIndexedDatabase(indexedDatabaseManager);

    return true;
#else
    UNUSED_PARAM(ewkContext);

    return false;
#endif
}

Eina_Bool ewk_context_web_indexed_database_delete(Ewk_Context* ewkContext, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKIndexedDatabaseManagerRef indexedDatabaseManager = WKContextGetIndexedDatabaseManager(ewkContext->wkContext());

    WKIndexedDatabaseManagerDeleteIndexedDatabase(indexedDatabaseManager, originRef.get());

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(origin);

    return false;
#endif
}

Eina_Bool ewk_context_web_indexed_database_origins_get(const Ewk_Context* ewkContext, Ewk_Web_Indexed_Database_Origins_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("callback (%p)", callback);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webIndexedDatabaseOriginsCallback= callback;
    context->userData = userData;
    WKIndexedDatabaseManagerRef indexedDatabaseManager = WKContextGetIndexedDatabaseManager(const_cast<Ewk_Context*>(ewkContext)->wkContext());

    WKIndexedDatabaseManagerGetIndexedDatabaseOrigins(indexedDatabaseManager, context, didGetWebIndexedDatabaseOrigins);

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(callback);
    UNUSED_PARAM(userData);

    return false;
#endif
}

Eina_Bool ewk_context_web_indexed_database_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    TIZEN_LOGI("path (%s)", path);
    WKContextGetIndexedDatabaseManager(ewkContext->wkContext());

    WKRetainPtr<WKStringRef> indexedDatabasePath(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetIndexedDatabaseDirectory(ewkContext->wkContext(), indexedDatabasePath.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_storage_delete_all(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    TIZEN_LOGI("ewkContext (%p)", ewkContext);
    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(ewkContext->wkContext());
    WKKeyValueStorageManagerDeleteAllEntries(storageManager);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_storage_origin_delete(Ewk_Context* ewkContext, Ewk_Security_Origin* origin)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> securityOriginRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));
    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(ewkContext->wkContext());
    WKKeyValueStorageManagerDeleteEntriesForOrigin(storageManager, securityOriginRef.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_storage_origins_get(Ewk_Context* ewkContext, Ewk_Web_Storage_Origins_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("callback (%p)", callback);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webStorageOriginsCallback = callback;
    context->userData = userData;

    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(ewkContext->wkContext());
    WKKeyValueStorageManagerGetKeyValueStorageOrigins(storageManager, context, didGetWebStorageOrigins);

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_storage_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    TIZEN_LOGI("path (%s)", path);
    WKRetainPtr<WKStringRef> webStoragePath(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetLocalStorageDirectory(ewkContext->wkContext(), webStoragePath.get());

    return true;
#else
    return false;
#endif
}

Eina_Bool ewk_context_web_storage_path_get(Ewk_Context* ewkContext, Ewk_Web_Storage_Path_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    TIZEN_LOGI("callback (%p)", callback);
    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webStoragePathCallback= callback;
    context->userData = userData;

    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(ewkContext->wkContext());
    WKKeyValueStorageManagerGetKeyValueStoragePath(storageManager, context, didGetWebStoragePath);
    return true;
#else
    return false;
#endif
}
Eina_Bool ewk_context_web_storage_usage_for_origin_get(Ewk_Context* ewkContext, Ewk_Security_Origin* origin, Ewk_Web_Storage_Usage_Get_Callback callback, void* userData)
{
#if ENABLE(TIZEN_WEB_STORAGE) && ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, false);

    Ewk_Context_Callback_Context* context = new Ewk_Context_Callback_Context;
    context->webStorageUsageCallback = callback;
    context->userData = userData;
    WKRetainPtr<WKStringRef> protocolRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_protocol_get(origin)));
    WKRetainPtr<WKStringRef> hostRef(AdoptWK, WKStringCreateWithUTF8CString(ewk_security_origin_host_get(origin)));
    WKRetainPtr<WKSecurityOriginRef> originRef(AdoptWK, WKSecurityOriginCreate(protocolRef.get(), hostRef.get(), ewk_security_origin_port_get(origin)));

    WKKeyValueStorageManagerRef storageManager = WKContextGetKeyValueStorageManager(ewkContext->wkContext());
    WKKeyValueStorageManagerGetKeyValueStorageUsageForOrigin(storageManager, context, didGetWebStorageUsage, originRef.get());

    return true;
#else
    return false;
#endif
}

//#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
Eina_Bool ewk_context_soup_data_directory_set(Ewk_Context* ewkContext, const char* path)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    TIZEN_SECURE_LOGI("ewkContext = %p, path = %s", ewkContext, path);

    WKRetainPtr<WKStringRef> soupDataDirectory(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetSoupDataDirectory(ewkContext->wkContext(), soupDataDirectory.get());
    return true;
}
//#endif

COMPILE_ASSERT_MATCHING_ENUM(EWK_CACHE_MODEL_DOCUMENT_VIEWER, kWKCacheModelDocumentViewer);
COMPILE_ASSERT_MATCHING_ENUM(EWK_CACHE_MODEL_DOCUMENT_BROWSER, kWKCacheModelDocumentBrowser);
COMPILE_ASSERT_MATCHING_ENUM(EWK_CACHE_MODEL_PRIMARY_WEBBROWSER, kWKCacheModelPrimaryWebBrowser);

Eina_Bool ewk_context_cache_model_set(Ewk_Context* ewkContext, Ewk_Cache_Model cacheModel)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    ewkContext->setCacheModel(cacheModel);

    return true;
}

Ewk_Cache_Model ewk_context_cache_model_get(const Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, EWK_CACHE_MODEL_DOCUMENT_VIEWER);

    return ewkContext->cacheModel();
}

Eina_Bool ewk_context_cache_disabled_set(Ewk_Context* ewkContext, Eina_Bool cacheDisabled)
{
#if ENABLE(TIZEN_CACHE_CONTROL)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    WKContextSetCacheDisabled(ewkContext->wkContext(), cacheDisabled);
#endif

    return true;
}

Eina_Bool ewk_context_cache_disabled_get(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_CACHE_CONTROL)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    return WKContextGetCacheDisabled(ewkContext->wkContext());
#else
    return false;
#endif
}

EINA_DEPRECATED Eina_Bool ewk_context_certificate_file_set(Ewk_Context* context, const char* certificateFile)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(context, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificateFile, false);
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    if (!context->setCertificateFile(certificateFile))
        return true;

    if (fileExists(WTF::String::fromUTF8(certificateFile))) {
        long long fileSize = -1;
        getFileSize(WTF::String::fromUTF8(certificateFile), fileSize);
        TIZEN_LOGI("[Network] ewk_context_certificate_file_set certificateFile fileSize [%lld]\n", fileSize);
    } else
        TIZEN_LOGE("[Network] ewk_context_certificate_file_set certificateFile does not exist!\n");

    WKRetainPtr<WKStringRef> certificateFileRef(AdoptWK, WKStringCreateWithUTF8CString(certificateFile));
    WKContextSetCertificateFile(context->wkContext(), certificateFileRef.get());
    return true;
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(certificateFile);
    return false;
#endif
}

EINA_DEPRECATED const char* ewk_context_certificate_file_get(const Ewk_Context* context)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(context, 0);
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    return context->certificateFile();
#else
    UNUSED_PARAM(context);
    return 0;
#endif
}

Eina_Bool ewk_context_cache_clear(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    WKResourceCacheManagerRef cacheManager = WKContextGetResourceCacheManager(ewkContext->wkContext());
#if ENABLE(TIZEN_EWK_CONTEXT_CACHE_MANAGER_NULL_CHECK_WORKAROUND)
    if (!cacheManager)
        return false;
#endif
    WKResourceCacheManagerClearCacheForAllOrigins(cacheManager, WKResourceCachesToClearAll);

    return true;
}

void ewk_context_resource_cache_clear(Ewk_Context* ewkContext)
{
    ewk_context_cache_clear(ewkContext);
}

void ewk_context_message_post_to_injected_bundle(Ewk_Context* ewkContext, const char* name, const char* body)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(name);
    EINA_SAFETY_ON_NULL_RETURN(body);

    WKRetainPtr<WKStringRef> messageName(AdoptWK, WKStringCreateWithUTF8CString(name));
    WKRetainPtr<WKStringRef> messageBody(AdoptWK, WKStringCreateWithUTF8CString(body));
    WKContextPostMessageToInjectedBundle(ewkContext->wkContext(), messageName.get(), messageBody.get());
}

void ewk_context_message_from_injected_bundle_callback_set(Ewk_Context* ewkContext, Ewk_Context_Message_From_Injected_Bundle_Callback callback, void* userData)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    ewkContext->setMessageFromInjectedBundleCallback(callback, userData);
}

void ewk_context_did_start_download_callback_set(Ewk_Context* ewkContext, Ewk_Context_Did_Start_Download_Callback callback, void* userData)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    ewkContext->setDidStartDownloadCallback(callback, userData);
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void ewk_context_password_confirm_popup_callback_set(Ewk_Context* ewkContext, Ewk_Context_Password_Confirm_Popup_Callback callback, void* userData)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    ewkContext->setPassworSaveConfirmPopupCallback(callback, userData);
}
#endif

#if ENABLE(MEMORY_SAMPLER)
void ewk_context_memory_sampler_start(Ewk_Context* ewkContext, double timerInterval)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    WKRetainPtr<WKDoubleRef> interval(AdoptWK, WKDoubleCreate(timerInterval));
    WKContextStartMemorySampler(ewkContext->wkContext(), interval.get());
}

void ewk_context_memory_sampler_stop(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    WKContextStopMemorySampler(ewkContext->wkContext());
}
#endif

Eina_Bool ewk_context_additional_plugin_path_set(Ewk_Context* ewkContext, const char* path)
{
#if ENABLE(TIZEN_SUPPORT_PLUGINS)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(path, false);

    WKRetainPtr<WKStringRef> pluginPath(AdoptWK, WKStringCreateWithUTF8CString(path));
    WKContextSetAdditionalPluginsDirectory(ewkContext->wkContext(), pluginPath.get());

    return true;
#else
    return false;
#endif
}

#if ENABLE(TIZEN_WEBKIT2_MEMORY_SAVING_MODE)
void ewk_context_memory_saving_mode_set(Ewk_Context* ewkContext, Eina_Bool mode)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    WKContextRef contextRef = ewkContext->wkContext();
    toImpl(contextRef)->setMemorySavingMode(mode);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void ewk_context_form_password_data_clear(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    ewkContext->formDatabase()->clearPasswordFormData();
}

void ewk_context_form_password_data_delete_all(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    ewkContext->formDatabase()->clearPasswordFormData();
}

void ewk_context_form_password_data_update(Ewk_Context* ewkContext, const char* url, Eina_Bool useFingerprint)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(url);
}

void ewk_context_form_password_data_delete(Ewk_Context* ewkContext, const char* url)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(url);

    ewkContext->formDatabase()->clearPasswordFormDataForURL(url);
}

Eina_List* ewk_context_form_password_data_list_get(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);

    Vector<FormDatabase::PasswordFormData> vector;
    ewkContext->formDatabase()->getAllPasswordFormData(vector);

    Eina_List* list = 0;
    for (size_t i = 0; i < vector.size(); i++) {
        Ewk_Password_Data* data = new Ewk_Password_Data;

        data->url = new char[vector[i].m_url.length() + 1];
        strcpy(data->url, vector[i].m_url.utf8().data());
        data->useFingerprint = vector[i].m_useFingerprint;

        list = eina_list_append(list, data);
    }

    return list;
}

void ewk_context_form_password_data_list_free(Ewk_Context* ewkContext, Eina_List* list)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(list);

    void* item;
    EINA_LIST_FREE(list, item) {
        delete [] (static_cast<Ewk_Password_Data*>(item))->url;
        delete static_cast<Ewk_Password_Data*>(item);
    }
}

void ewk_context_form_candidate_data_clear(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    ewkContext->formDatabase()->clearCandidateFormData();
}

void ewk_context_form_candidate_data_delete_all(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    ewkContext->formDatabase()->clearCandidateFormData();
}

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
Eina_List* ewk_context_form_autofill_profile_get_all(Ewk_Context* context)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);
    return context->getAllProfilesForProfileForm();
}

Ewk_Autofill_Profile* ewk_context_form_autofill_profile_get(Ewk_Context* context, unsigned id)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(context, NULL);

    return context->getProfileFormDataByProfileId(id);
}

bool Ewk_Context::isProfileDataDuplicate(unsigned id)
{
    NumberToStringBuffer buffer;
    String profileID = numberToString(id, buffer);

    Vector<String> profileList;
    getAllProfileIDForProfileFormData(profileList);
    for (size_t i = 0;i < profileList.size(); i++) {
        if (profileList[i] == profileID)
            return true;
    }
    return false;
}

unsigned Ewk_Context::getProfileHashValue(Vector<std::pair<String, String> >& textFormData)
{
    String hashStr;

    size_t size = textFormData.size();
    for (size_t i = 0; i < size; i++) {
        if (!textFormData[i].second.length())
            continue;

        hashStr.append(textFormData[i].first);
        hashStr.append(textFormData[i].second);
    }
    return hashStr.impl()->hash();
}

void Ewk_Context::getProfileDataVector(Ewk_Autofill_Profile* profile, Vector<std::pair<String, String> >& textFormData)
{
    if (!profile)
        return;

    size_t size = profile->nameValuePair.size();
    for (size_t i = 0; i < size; i++) {
        String name = getProfileFormNameFromEnum(profile->nameValuePair[i].first);
        String value(String::fromUTF8(profile->nameValuePair[i].second));
        textFormData.append(std::pair<String, String>(name, value));
    }

}

#if ENABLE(TIZEN_WEBKIT2_PROXY)
void Ewk_Context::setProxyUri(const char* proxy)
{
    if (setProxyAddress(proxy)) {
        WKRetainPtr<WKURLRef> proxyAddress(AdoptWK, WKURLCreateWithUTF8CString(proxy));

        TIZEN_SECURE_LOGI("proxy [%s]", proxy);

        WKContextRef contextRef = wkContext();
        toImpl(contextRef)->setProxy(toWTFString(proxyAddress.get()));
    }
}

void Ewk_Context::setProxy()
{
    if (m_proxySetting)
            return;

    if (!m_connectionHandle) {
        connection_create(&m_connectionHandle);

        if (!m_connectionHandle)
            return;

        connection_set_ip_address_changed_cb(m_connectionHandle, connectionChangedCallback, this);
        connection_set_proxy_address_changed_cb(m_connectionHandle, connectionChangedCallback, this);
    }
    m_proxySetting = true;

    char* proxy = 0;
    connection_address_family_e family = CONNECTION_ADDRESS_FAMILY_IPV4;
    connection_get_proxy(m_connectionHandle, family, &proxy);

    if (proxy) {
        size_t proxy_len = strlen(proxy);
        if (!proxy_len) {
            setProxyUri(0);
        } else if (proxy_len > strlen("0.0.0.0")) {
            if (!strncmp(proxy, "0.0.0.0", proxy_len))
                setProxyUri(0);
            else
                setProxyUri(proxy);
        } else {
            if (!strncmp("0.0.0.0", proxy, strlen("0.0.0.0")))
                setProxyUri(0);
            else
                setProxyUri(proxy);
        }
    } else
        setProxyUri(0);
}

void Ewk_Context::connectionChangedCallback(const char* ipv4, const char* ipv6, void *data)
{
    if (!data)
        return;

    Ewk_Context* context = (Ewk_Context*) data;
    context->setProxySetting(false);
    context->setProxy();
}
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
Eina_Bool Ewk_Context::setCertificate(const char* certificateFile)
{
    if (fileExists(WTF::String::fromUTF8(certificateFile))) {
        long long fileSize = -1;
        getFileSize(WTF::String::fromUTF8(certificateFile), fileSize);
        TIZEN_LOGI("[Network] ewk_context_certificate_file_set certificateFile fileSize [%lld]\n", fileSize);
    } else {
        TIZEN_LOGE("[Network] ewk_context_certificate_file_set certificateFile does not exist!\n");
        return false;
    }

    WKRetainPtr<WKStringRef> certificateFileRef(AdoptWK, WKStringCreateWithUTF8CString(certificateFile));
    WKContextSetCertificateFile(wkContext(), certificateFileRef.get());
    return true;
}
#endif

Eina_Bool ewk_context_form_autofill_profile_set(Ewk_Context* ewkContext, unsigned id, Ewk_Autofill_Profile* profile)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(profile, false);

    Ewk_Autofill_Profile *existingProfile = ewkContext->getProfileFormDataByProfileId(id);
    if (!existingProfile)
        return false;

    ewk_autofill_profile_delete(existingProfile);

    Vector<std::pair<String, String> > textFormData;
    ewkContext->getProfileDataVector(profile, textFormData);
    unsigned profileID = ewkContext->getProfileHashValue(textFormData);
    if (!profileID)
        return false;

    if (ewkContext->isProfileDataDuplicate(profileID))
        return false;

    ewkContext->clearProfileFormData(id);

    profile->profileID = profileID;

    return ewkContext->addProfileFormData(profile, textFormData);
}

Eina_Bool ewk_context_form_autofill_profile_add(Ewk_Context* ewkContext, Ewk_Autofill_Profile* profile)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(profile, false);

    Vector<std::pair<String, String> > textFormData;
    ewkContext->getProfileDataVector(profile,textFormData);
    unsigned profileID = ewkContext->getProfileHashValue(textFormData);
    if (!profileID)
        return false;

    if (ewkContext->isProfileDataDuplicate(profileID))
        return false;

    profile->profileID = profileID;

    return ewkContext->addProfileFormData(profile, textFormData);
}

Eina_Bool ewk_context_form_autofill_profile_remove(Ewk_Context* ewkContext, unsigned id)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    return ewkContext->clearProfileFormData(id);
}
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif
#endif // #if PLATFORM(TIZEN)

Eina_Bool ewk_context_url_scheme_register(Ewk_Context* ewkContext, const char* scheme, Ewk_Url_Scheme_Request_Cb callback, void* userData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(scheme, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    ewkContext->requestManager()->registerURLSchemeHandler(String::fromUTF8(scheme), callback, userData);

    return true;
}

void ewk_context_vibration_client_callbacks_set(Ewk_Context* ewkContext, Ewk_Vibration_Client_Vibrate_Cb vibrate, Ewk_Vibration_Client_Vibration_Cancel_Cb cancel, void* data)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    TIZEN_LOGI("vibrate (%p)", vibrate);
#if ENABLE(VIBRATION)
    if (ewkContext->vibrationProvider().get())
        ewkContext->vibrationProvider()->setVibrationClientCallbacks(vibrate, cancel, data);
#endif
}

Eina_Bool ewk_context_tizen_extensible_api_string_set(Ewk_Context* ewkContext,  const char* extensibleAPI, Eina_Bool enable)
{
#if ENABLE(TIZEN_EXTENSIBLE_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(extensibleAPI, false);

    TIZEN_LOGI("extensibleAPI (%s) enable (%d)", extensibleAPI, enable);
    int extensibleId = ewkContext->getExtensibleAPIEnumFromName(extensibleAPI);
    if (extensibleId < 0)
        return false;
    WKContextSetTizenExtensibleAPI(ewkContext->wkContext(), static_cast<WKTizenExtensibleAPI>(extensibleId), enable);

    return true;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(extensibleAPI);
    UNUSED_PARAM(enable);

    return false;
#endif
}

Eina_Bool ewk_context_tizen_extensible_api_string_get(Ewk_Context* ewkContext,  const char* extensibleAPI)
{
#if ENABLE(TIZEN_EXTENSIBLE_API)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);

    int extensibleId = ewkContext->getExtensibleAPIEnumFromName(extensibleAPI);
    if (extensibleId < 0)
        return false;
    bool result = WKContextGetTizenExtensibleAPI(ewkContext->wkContext(), static_cast<WKTizenExtensibleAPI>(extensibleId));
    TIZEN_LOGI("extensibleAPI (%s) result (%d)", extensibleAPI, result);

    return result;
#else
    UNUSED_PARAM(ewkContext);
    UNUSED_PARAM(extensibleAPI);

    return false;
#endif
}

void ewk_context_storage_path_reset(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_RESET_PATH)
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    TIZEN_LOGI("ewkContext (%p)", ewkContext);
    WKContextResetStoragePath(ewkContext->wkContext());
#else
    UNUSED_PARAM(ewkContext);
#endif
}

void ewk_context_history_callbacks_set(Ewk_Context* ewkContext, Ewk_History_Navigation_Cb navigate, Ewk_History_Client_Redirection_Cb clientRedirect, Ewk_History_Server_Redirection_Cb serverRedirect, Ewk_History_Title_Update_Cb titleUpdate, Ewk_History_Populate_Visited_Links_Cb populateVisitedLinks, void* data)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    ewkContext->historyClient()->setCallbacks(navigate, clientRedirect, serverRedirect, titleUpdate, populateVisitedLinks, data);
}

void ewk_context_visited_link_add(Ewk_Context* ewkContext, const char* visitedURL)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(visitedURL);

    ewkContext->addVisitedLink(visitedURL);
}

#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
void ewk_context_pixmap_set(Ewk_Context* ewkContext, int pixmap)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    TIZEN_LOGI("ewkContext(%p), pixmap(%d)", ewkContext, pixmap);
    ewkContext->setPixmap(pixmap);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR)
unsigned int ewk_context_inspector_server_start(Ewk_Context* ewkContext, unsigned int port)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, 0);

    WKContextRef contextRef = ewkContext->wkContext();
    return toImpl(contextRef)->inspectorServerStart(port);
}

Eina_Bool ewk_context_inspector_server_stop(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, EINA_FALSE);

    WKContextRef contextRef = ewkContext->wkContext();
    return toImpl(contextRef)->inspectorServerStop();
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void ewk_context_password_confirm_popup_reply(Ewk_Context* ewkContext, Ewk_Context_Password_Popup_Option result)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    switch (result) {
    case EWK_CONTEXT_PASSWORD_POPUP_SAVE:
        ewkContext->allowSavePassword();
        break;
    case EWK_CONTEXT_PASSWORD_POPUP_NOT_NOW:
        ewkContext->cancelSavePassword();
        break;
    case EWK_CONTEXT_PASSWORD_POPUP_NEVER:
        ewkContext->neverSavePassword();
        break;
    default:
        break;
    }
}
#endif

void ewk_context_preferred_languages_set(Eina_List* languages)
{
    Vector<String> preferredLanguages;
    if (languages) {
        Eina_List* l;
        void* data;
        EINA_LIST_FOREACH(languages, l, data)
            preferredLanguages.append(String::fromUTF8(static_cast<char*>(data)).lower().replace("_", "-"));
    }

    WebCore::overrideUserPreferredLanguages(preferredLanguages);
    WebCore::languageDidChange();
}

//For UCProxy
void ewk_context_compression_proxy_enabled_set(Ewk_Context* ewkContext, Eina_Bool enabled)
{
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    WKContextSetCompressionProxyEnabled(ewkContext->wkContext(), enabled);
#endif
}

Eina_Bool ewk_context_compression_proxy_enabled_get(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, false);
    return WKContextGetCompressionProxyEnabled(ewkContext->wkContext());
#else
    return EINA_FALSE;
#endif
}

void ewk_context_compression_proxy_image_quality_set(Ewk_Context* ewkContext, Ewk_Compression_Proxy_Image_Quality quality)
{
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    TIZEN_SECURE_LOGI("[UCPROXY] COMPRESSION PROXY Image Quality [%d]", quality);

    ImageQualityLevel level = ImageQualityLowColor;
    switch (quality) {
    case EWK_COMPRESSION_PROXY_IMAGE_QUALITY_LOW:
        level = ImageQualityLowColor;
        break;
    case EWK_COMPRESSION_PROXY_IMAGE_QUALITY_MEDIUM:
        level = ImageQualityStandard;
        break;
    case EWK_COMPRESSION_PROXY_IMAGE_QUALITY_HIGH:
        level = ImageQualityFullColor;
        break;
    default:
        level = ImageQualityLowColor;
    }
    WKContextCompressionProxyImageQualitySet(ewkContext->wkContext(), level);
#endif
}

Ewk_Compression_Proxy_Image_Quality ewk_context_compression_proxy_image_quality_get(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    EINA_SAFETY_ON_NULL_RETURN_VAL(ewkContext, EWK_COMPRESSION_PROXY_IMAGE_QUALITY_MEDIUM);
    return EWK_COMPRESSION_PROXY_IMAGE_QUALITY_MEDIUM;
#else
    return EWK_COMPRESSION_PROXY_IMAGE_QUALITY_MEDIUM;
#endif
}

void ewk_context_compression_proxy_data_size_get(Ewk_Context* ewkContext, unsigned* original_size, unsigned* compressed_size)
{
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    EINA_SAFETY_ON_NULL_RETURN(original_size);
    EINA_SAFETY_ON_NULL_RETURN(compressed_size);
    WKContextCompressionProxyDataSizeGet(ewkContext->wkContext(), original_size, compressed_size);
    TIZEN_SECURE_LOGI("[UCPROXY] original_size: [%u], compressed_size: [%u]", *original_size, *compressed_size);
#endif
}

void ewk_context_compression_proxy_data_size_reset(Ewk_Context* ewkContext)
{
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);
    TIZEN_LOGI("[UCPROXY]");
    WKContextCompressionProxyDataSizeSet(ewkContext->wkContext(), 0, 0);
#endif
}
//For UCProxy
