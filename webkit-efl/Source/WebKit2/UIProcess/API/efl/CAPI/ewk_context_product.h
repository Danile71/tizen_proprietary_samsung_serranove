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
 */

/**
 * @file    ewk_context_product.h
 * @brief   Describes the context API.
 *
 * @note ewk_context encapsulates all pages related to specific use of WebKit.
 *
 * Applications have the option of creating a context different than the default one
 * and use it for a group of pages. All pages in the same context share the same
 * preferences, visited link set, local storage, etc.
 *
 * A process model can be specified per context. The default one is the shared model
 * where the web-engine process is shared among the pages in the context. The second
 * model allows each page to use a separate web-engine process. This latter model is
 * currently not supported by WebKit2/EFL.
 *
 */

#ifndef ewk_context_product_h
#define ewk_context_product_h

#include "ewk_autofill_profile_product.h"
#include <Evas.h>

#include "ewk_security_origin_product.h"
#include <tizen.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * @brief Enumeration for password popup option.
 * @since_tizen 2.3
 */
enum Ewk_Context_Password_Popup_Option {
    EWK_CONTEXT_PASSWORD_POPUP_SAVE,    /**< The option of response */
    EWK_CONTEXT_PASSWORD_POPUP_NOT_NOW, /**< The option of response */
    EWK_CONTEXT_PASSWORD_POPUP_NEVER    /**< The option of response */
};
/**
 * @brief Creates a type name for @a Ewk_Context_Password_Popup_Option.
 * @since_tizen 2.3
 */
typedef enum Ewk_Context_Password_Popup_Option Ewk_Context_Password_Popup_Option;

#ifndef ewk_context_type
#define ewk_context_type
/**
 * @brief Creates a type name for @a Ewk_Context.
 * @since_tizen 2.3
 */
typedef struct Ewk_Context Ewk_Context;
#endif

/**
 * @brief Gets default Ewk_Context instance.
 *
 * @details The returned Ewk_Context object @b should not be unref'ed if application\n
 * does not call ewk_context_ref() for that.
 *
 * @since_tizen 2.3
 *
 * @return Ewk_Context object
 */
EXPORT_API Ewk_Context* ewk_context_default_get(void);

/**
 * @brief Notify low memory to free unused memory.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object to notify low memory
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_notify_low_memory(Ewk_Context* context);

/**
 * @brief Sets the given proxy URI to network backend of specific context.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object to set proxy URI
 * @param[in] proxy URI to set
 */
EINA_DEPRECATED EXPORT_API void ewk_context_proxy_uri_set(Ewk_Context* context, const char* proxy);

/**
 * @brief Gets the proxy URI from the network backend of specific context.
 *
 * @details It returns an internal string and should not\n
 * be modified. The string is guaranteed to be stringshared.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object to get proxy URI
 *
 * @return current proxy URI or @c 0 if it's not set
 */
EINA_DEPRECATED EXPORT_API const char* ewk_context_proxy_uri_get(Ewk_Context* context);

/**
 * @brief Callback for ewk_context_local_file_system_origins_get
 *
 * @since_tizen 2.3
 *
 * @param[in] origins local file system origins
 * @param[in] user_data user_data will be passsed when ewk_context_local_file_system_origins_get is called
 */
typedef void (*Ewk_Local_File_System_Origins_Get_Callback)(Eina_List* origins, void* user_data);

/**
 * @brief Callback for ewk_context_application_cache_origins_get
 *
 * @since_tizen 2.3
 *
 * @param[in] origins web application cache origins
 * @param[in] user_data user_data will be passsed when ewk_context_application_cache_origins_get is called
 */
typedef void (*Ewk_Web_Application_Cache_Origins_Get_Callback)(Eina_List* origins, void* user_data);

/**
 * @brief Callback for ewk_context_application_cache_usage_for_origin_get.
 *
 * @since_tizen 2.3
 *
 * @param[in] usage web application cache usage for origin
 * @param[in] user_data user_data will be passsed when ewk_context_application_cache_usage_for_origin_get is called
 */
typedef void (*Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback)(int64_t usage, void* user_data);

/**
 * @brief Callback for ewk_context_web_database_origins_get.
 *
 * @since_tizen 2.3
 *
 * @param[in] origins web database origins
 * @param[in] user_data user_data will be passsed when ewk_context_web_database_origins_get is called
 */
typedef void (*Ewk_Web_Database_Origins_Get_Callback)(Eina_List* origins, void* user_data);

/**
 * @brief Callback for ewk_context_web_database_quota_for_origin_get.
 *
 * @since_tizen 2.3
 *
 * @param[in] quota web database quota
 * @param[in] user_data user_data will be passsed when ewk_context_web_database_quota_for_origin_get is called
 */
typedef void (*Ewk_Web_Database_Quota_Get_Callback)(uint64_t quota, void* user_data);

/**
 * @brief Callback for ewk_context_web_database_usage_for_origin_get.
 *
 * @since_tizen 2.3
 *
 * @param[in] usage web database usage
 * @param[in] user_data user_data will be passsed when ewk_context_web_database_usage_for_origin_get is called
 */
typedef void (*Ewk_Web_Database_Usage_Get_Callback)(uint64_t usage, void* user_data);

/**
 * @brief Callback for ewk_context_web_storage_origins_get.
 *
 * @since_tizen 2.3
 *
 * @param[in] origins web storage origins
 * @param[in] user_data user_data will be passsed when ewk_context_web_storage_origins_get is called
 */
typedef void (*Ewk_Web_Storage_Origins_Get_Callback)(Eina_List* origins, void* user_data);

/**
 * @brief Callback for ewk_context_web_storage_usage_for_origin_get.
 *
 * @since_tizen 2.3
 *
 * @param[in] usage usage of web storage
 * @param[in] user_data user_data will be passsed when ewk_context_web_storage_usage_for_origin_get is called
 */
typedef void (*Ewk_Web_Storage_Usage_Get_Callback)(uint64_t usage, void* user_data);

/**
 * @brief Callback for didStartDownload
 *
 * @since_tizen 2.3
 *
 * @param[in] download_url url to download
 * @param[in] user_data user_data will be passsed when download is started
 */
typedef void (*Ewk_Context_Did_Start_Download_Callback)(const char* download_url, void* user_data);

/**
 * @brief Callback for passworSaveConfirmPopupCallbackCall
 *
 * @since_tizen 2.3
 *
 * @param[in] view current view
 * @param[in] user_data user_data will be passsed when password save confirm popup show
 */
typedef void (*Ewk_Context_Password_Confirm_Popup_Callback)(Evas_Object* view, void* user_data);

/**
 * @brief Requests for freeing origins.
 *
 * @since_tizen 2.3
 *
 * @param[in] origins list of origins
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_origins_free(Eina_List* origins);

/**
 * @brief Requests for deleting all web application caches.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA FALSE on failure
 */
EAPI Eina_Bool ewk_context_application_cache_delete_all(Ewk_Context* context);

/*
 * @brief Requests for deleting web application cache for origin.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] origin application cache origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_delete(Ewk_Context* context, Ewk_Security_Origin* origin);

/**
 * @brief Requests for getting web application cache origins.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] callback callback to get web application cache origins
 * @param[in] user_data user_data will be passsed when callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 *
 * @see ewk_context_origins_free
 */
EXPORT_API Eina_Bool ewk_context_application_cache_origins_get(Ewk_Context* context, Ewk_Web_Application_Cache_Origins_Get_Callback callback, void* user_data);

/**
 * @brief Requests for setting application cache quota for origin.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] origin serucity origin
 * @param[in] quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_quota_for_origin_set(Ewk_Context* context, const Ewk_Security_Origin* origin, int64_t quota);

/**
 * @brief Requests for getting application cache usage for origin.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] origin security origin
 * @param[in] callback callback to get web database usage
 * @param[in] user_data user_data will be passed when callback is called\n
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_usage_for_origin_get(Ewk_Context* context, const Ewk_Security_Origin* origin, Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback callback, void* user_data);

/**
 * @brief Requests setting of the favicon database path.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] path database path
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_icon_database_path_set(Ewk_Context* context, const char* path);

/**
 * @brief Requests to get image representing the given URL.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] uri which url to query icon, must not be @c 0
 * @param[in] canvas evas instance where to add resulting object, must not be @c 0
 *
 * @return newly allocated Evas_Object instance or @c 0 on\n
 *         errors. Delete the object with evas_object_del()
 */
EAPI Evas_Object* ewk_context_icon_database_icon_object_add(Ewk_Context* context, const char* uri, Evas* canvas);

/*
 * @brief Deletes all known icons from database.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 */
EXPORT_API void ewk_context_icon_database_delete_all(Ewk_Context* context);

/**
 * @brief Requests for deleting all local file systems.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_local_file_system_all_delete(Ewk_Context* context);

/**
 * @brief Requests for deleting local file system for origin.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] origin local file system origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_local_file_system_delete(Ewk_Context* context, Ewk_Security_Origin* origin);

 /**
 * @brief Requests for getting local file system origins.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] callback callback to get local file system origins
 * @param[in] user_data user_data will be passed when callback is called\n
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 *
 * @see ewk_context_origins_free
 */
EXPORT_API Eina_Bool ewk_context_local_file_system_origins_get(const Ewk_Context* context, Ewk_Local_File_System_Origins_Get_Callback callback, void* user_data);

/**
 * @brief Requests for deleting all web databases.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_delete_all(Ewk_Context* context);

/**
 * @brief Requests for deleting web databases for origin.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] origin database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_delete(Ewk_Context* context, Ewk_Security_Origin* origin);

/**
 * @brief Requests for getting web database origins.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] callback callback to get web database origins
 * @param[in] user_data user_data will be passed when callback is called\n
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 *
 * @see ewk_context_origins_free
 */
EXPORT_API Eina_Bool ewk_context_web_database_origins_get(Ewk_Context* context, Ewk_Web_Database_Origins_Get_Callback callback, void* user_data);

/**
 * @brief Requests for getting web database quota for origin.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] callback callback to get web database quota
 * @param[in] user_data user_data will be passed when callback is called\n
 *    -I.e., user data will be kept until callback is called
 * @param[in] origin database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_quota_for_origin_get(Ewk_Context* context, Ewk_Web_Database_Quota_Get_Callback callback, void* user_data, Ewk_Security_Origin* origin);

/**
 * @brief Requests for setting web database quota for origin.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] origin database origin
 * @param[in] quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_quota_for_origin_set(Ewk_Context* context, Ewk_Security_Origin* origin, uint64_t quota);

/**
 * @brief Requests for getting web database usage for origin.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] callback callback to get web database usage
 * @param[in] user_data user_data will be passed when callback is called\n
 *    -I.e., user data will be kept until callback is called
 * @param[in] origin database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_usage_for_origin_get(Ewk_Context* context, Ewk_Web_Database_Usage_Get_Callback callback, void* user_data, Ewk_Security_Origin* origin);

/**
 * @brief Requests for deleting all web indexed databases.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_indexed_database_delete_all(Ewk_Context* context);

/**
 * @brief Deletes all web storage.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_storage_delete_all(Ewk_Context* context);

/**
 * @brief Deletes origin that is stored in web storage db.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] origin origin of db
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_storage_origin_delete(Ewk_Context* context, Ewk_Security_Origin* origin);

/**
 * @brief Gets list of origins that is stored in web storage db.
 *
 * @details This function allocates memory for context structure made from callback and user_data.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] callback callback to get web storage origins
 * @param[in] user_data user_data will be passed when callback is called\n
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 *
 * @see ewk_context_origins_free()
 */
EXPORT_API Eina_Bool ewk_context_web_storage_origins_get(Ewk_Context* context, Ewk_Web_Storage_Origins_Get_Callback callback, void* user_data);

/**
 * @brief Gets usage of web storage for certain origin.
 *
 * @details This function allocates memory for context structure made from callback and user_data.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] origin security origin
 * @param[in] callback callback to get web storage usage
 * @param[in] user_data user_data will be passed when callback is called\n
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_storage_usage_for_origin_get(Ewk_Context* context, Ewk_Security_Origin* origin, Ewk_Web_Storage_Usage_Get_Callback callback, void* user_data);

/**
 * @brief Requests for setting soup data path(soup data include cookie and cache).
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] path soup data path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_soup_data_directory_set(Ewk_Context* context, const char* path);

/**
 * @brief Toggles the cache enable and disable
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] cacheDisabled or disable cache
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_cache_disabled_set(Ewk_Context* context, Eina_Bool cacheDisabled);

/**
 * @brief Queries if the cache is enabled
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c EINA_TRUE is cache is enabled or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_cache_disabled_get(const Ewk_Context* context);

/**
 * @brief Request to set certifcate file
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] certificate_file is path where certificate file is stored
 *
 * @return @c EINA_TRUE is cache is enabled or @c EINA_FALSE otherwise
 */
EINA_DEPRECATED EXPORT_API Eina_Bool ewk_context_certificate_file_set(Ewk_Context* context, const char* certificate_file);

/**
 * @brief Request to get certifcate file
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c certificate_file is path which is set during ewk_context_certificate_file_set or @c NULL otherwise
 */
EINA_DEPRECATED EXPORT_API const char* ewk_context_certificate_file_get(const Ewk_Context* context);

/**
 * @brief Requests to clear cache
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_cache_clear(Ewk_Context* context);

/**
 * @brief Sets callback for started download.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] callback callback for started download
 * @param[in] user_data user data
 */
EXPORT_API void ewk_context_did_start_download_callback_set(Ewk_Context* context, Ewk_Context_Did_Start_Download_Callback callback, void* user_data);

/**
 * @brief Sets callback for show password confirm popup.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] callback callback for show password confirm popup
 * @param[in] user_data user data
 */
EXPORT_API void ewk_context_password_confirm_popup_callback_set(Ewk_Context* context, Ewk_Context_Password_Confirm_Popup_Callback callback, void* user_data);

/**
 * @brief start memory sampler.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] timer_interval time gap to fire the timer
*/
EXPORT_API void ewk_context_memory_sampler_start(Ewk_Context* context, double timer_interval);

/**
* @brief stop memory sampler.
*
* @param[in] context context object
*/
EXPORT_API void ewk_context_memory_sampler_stop(Ewk_Context* context);

/**
 * @brief Callback for ewk_context_vibration_client_callbacks_set
 *
 * @since_tizen 2.3
 *
 * @details Type definition for a function that will be called back when vibrate request receiveed from the vibration controller.
 *
 * @param[in] vibration_time the number of vibration times
 * @param[in] user_data user_data will be passsed when ewk_context_vibration_client_callbacks_set is called
 */
typedef void (*Ewk_Vibration_Client_Vibrate_Cb)(uint64_t vibration_time, void* user_data);

/**
 * @brief Callback for ewk_context_vibration_client_callbacks_set
 *
 * @since_tizen 2.3
 *
 * @details Type definition for a function that will be called back when cancel vibration request receiveed from the vibration controller.
 *
 * @param[in] user_data user_data will be passsed when ewk_context_vibration_client_callbacks_set is called
 */
typedef void (*Ewk_Vibration_Client_Vibration_Cancel_Cb)(void* user_data);

/**
 * @brief Sets vibration client callbacks to handle the tactile feedback in the form of\n
 * vibration in the client application when the content asks for vibration.
 *
 * @details To stop listening for vibration events, you may call this function with @c NULL for the callbacks.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object to set vibration client callbacks
 * @param[in] vibrate The function to call when the vibrate request received from the controller (may be @c NULL)
 * @param[in] cancel The function to call when the cancel vibration request received from the controller (may be @c NULL)
 * @param[in] data User data (may be @c NULL)
 */
EXPORT_API void ewk_context_vibration_client_callbacks_set(Ewk_Context* context, Ewk_Vibration_Client_Vibrate_Cb vibrate, Ewk_Vibration_Client_Vibration_Cancel_Cb cancel, void* data);

/**
 * @brief Sets a additional plugin directory.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] path the directory to be set
 *
 * @return @c EINA_TRUE if the directory was set, @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_additional_plugin_path_set(Ewk_Context* context, const char* path);

/**
 * @brief Sets memory saving mode.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] mode or disable memory saving mode
 *
 */
EXPORT_API void ewk_context_memory_saving_mode_set(Ewk_Context* context, Eina_Bool mode);

/**
 * @brief Struct for password data.
 * @since_tizen 2.3
 */
struct Ewk_Password_Data {
    char* url;
    Eina_Bool useFingerprint;
};
/**
 * @brief Deletes whole password data from DB
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 */
EAPI void ewk_context_form_password_data_delete_all(Ewk_Context* context);

/**
 * @brief Updates use fingerprint value from DB for given URL
 *
 * @details The API will update use fingerprint value on DB for given URL.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] url url saved form password
 * @param[in] useFingerprint fingerprint for given URL will be used or not
 *
 * @see ewk_context_form_password_data_list_free
 * @see ewk_context_form_password_data_delete
 * @see ewk_context_form_password_data_list_get
 */
EXPORT_API void ewk_context_form_password_data_update(Ewk_Context* context, const char* url, Eina_Bool useFingerprint);

/**
 * @brief Deletes password data from DB for given URL
 *
 * @details The API will delete the a password data from DB.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] url url saved form password
 *
 * @see ewk_context_form_password_data_update
 * @see ewk_context_form_password_data_list_free
 * @see ewk_context_form_password_data_list_get
 */
EXPORT_API void ewk_context_form_password_data_delete(Ewk_Context* context, const char* url);

/**
 * @brief Get all password url list
 *
 * @details The API will delete the a password data list only from the memory.\n
 * To remove the password data for URL permenantly, use ewk_context_form_password_data_delete
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c Eina_List of Ewk_Password_Data @c NULL otherwise
 * @see ewk_context_form_password_data_update
 * @see ewk_context_form_password_data_delete
 * @see ewk_context_form_password_data_list_free
 */
EXPORT_API Eina_List* ewk_context_form_password_data_list_get(Ewk_Context* context);

/**
 * @brief Deletes a given password data list
 *
 * @details The API will delete the a password data list only from the memory.\n
 * To remove the password data for URL permenantly, use ewk_context_form_password_data_delete
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] list Eina_List with Ewk_Password_Data
 *
 * @see ewk_context_form_password_data_update
 * @see ewk_context_form_password_data_delete
 * @see ewk_context_form_password_data_list_get
 */
EXPORT_API void ewk_context_form_password_data_list_free(Ewk_Context* context, Eina_List* list);

/**
 * @brief Deletes all candidate form data from DB
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 */
EXPORT_API void ewk_context_form_candidate_data_delete_all(Ewk_Context* context);

/**
 * @brief Gets a list of all existing profiles
 *
 * @details The obtained profile must be deleted by ewk_autofill_profile_delete.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c Eina_List of Ewk_Autofill_Profile @c NULL otherwise
 * @see ewk_autofill_profile_delete
 */
EXPORT_API Eina_List* ewk_context_form_autofill_profile_get_all(Ewk_Context* context);

/**
 * @brief Gets the existing profile for given index
 *
 * @details The obtained profile must be deleted by ewk_autofill_profile_delete.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] id profile id
 *
 * @return @c Ewk_Autofill_Profile if profile exists, @c NULL otherwise
 * @see ewk_autofill_profile_delete
 */
EXPORT_API Ewk_Autofill_Profile* ewk_context_form_autofill_profile_get(Ewk_Context* context, unsigned id);

/**
 * @brief Sets the given profile for the given id
 *
 * @details Data can be added to the created profile by ewk_autofill_profile_data_set.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] id profile id
 * @param[in] profile Ewk_Autofill_Profile
 *
 * @return @c EINA_TRUE if the profile data is set successfully, @c EINA_FALSE otherwise
 * @see ewk_autofill_profile_new
 * @see ewk_context_form_autofill_profile_add
 */
EXPORT_API Eina_Bool ewk_context_form_autofill_profile_set(Ewk_Context* context, unsigned id, Ewk_Autofill_Profile* profile);

/**
 * @brief Saves the created profile into permenant storage
 *
 * @details The profile used to save must be created by ewk_autofill_profile_new.\n
 * Data can be added to the created profile by ewk_autofill_profile_data_set.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] profile Ewk_Autofill_Profile
 *
 * @return @c EINA_TRUE if the profile data is saved successfully, @c EINA_FALSE otherwise
 * @see ewk_autofill_profile_new
 */
EXPORT_API Eina_Bool ewk_context_form_autofill_profile_add(Ewk_Context* context, Ewk_Autofill_Profile* profile);

/**
 * @brief Removes Autofill Form profile completely
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] id profile id
 *
 * @return @c EINA_TRUE if the profile data is removed successfully, @c EINA_FALSE otherwise
 * @see ewk_context_form_autofill_profile_get_all
 */
EXPORT_API Eina_Bool ewk_context_form_autofill_profile_remove(Ewk_Context* context, unsigned id);

 /**
 * @brief Toggles tizen extensible api enable and disable
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] extensible_api extensible API name of every kind
 * @param[in] enable enable or disable tizen extensible api
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_tizen_extensible_api_string_set(Ewk_Context* context,  const char* extensible_api, Eina_Bool enable);

 /**
 * @brief Get tizen extensible api enable state
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] extensible_api extensible API name of every kind
 *
 * @return @c EINA_TRUE if the extensibleAPI set as true or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_tizen_extensible_api_string_get(Ewk_Context* context,  const char* extensible_api);

/**
 * @brief Start the inspector server
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] port number
 *
 * @return @c return the port number
 */
EXPORT_API unsigned int ewk_context_inspector_server_start(Ewk_Context* context, unsigned int port);

/**
 * @brief Stop the inspector server
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c EINA_TRUE if the inspector server stop set successfully, @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_inspector_server_stop(Ewk_Context* context);

/**
 * @brief Password confirm popup reply
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] result The option of response
 */
EXPORT_API void ewk_context_password_confirm_popup_reply(Ewk_Context* context, Ewk_Context_Password_Popup_Option result);

/**
 * @internal
 * @brief Enumeration for compression proxy image quality.
 * @since_tizen 2.3
 */
enum Ewk_Compression_Proxy_Image_Quality {
    EWK_COMPRESSION_PROXY_IMAGE_QUALITY_LOW = 0,    /**< @internal Low */
    EWK_COMPRESSION_PROXY_IMAGE_QUALITY_MEDIUM,     /**< @internal Medium */
    EWK_COMPRESSION_PROXY_IMAGE_QUALITY_HIGH        /**< @internal High */
};
/**
 * @internal
 * @brief Creates a type name for @a #Ewk_Compression_Proxy_Image_Quality.
 * @since_tizen 2.3
 */
typedef enum Ewk_Compression_Proxy_Image_Quality Ewk_Compression_Proxy_Image_Quality;

 /**
 * @internal
 * @brief Enable or disable proxy
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] enabled enable or disable proxy
 *
 */
EXPORT_API void ewk_context_compression_proxy_enabled_set(Ewk_Context* context, Eina_Bool enabled);

 /**
 * @internal
 * @brief Returns currently proxy is enabled or not
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c EINA_TRUE on enabled or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_compression_proxy_enabled_get(Ewk_Context* context);

 /**
 * @internal
 * @brief Set image quality of proxy compression
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] quality image quality
 *
 */
EXPORT_API void ewk_context_compression_proxy_image_quality_set(Ewk_Context* context, Ewk_Compression_Proxy_Image_Quality quality);

 /**
 * @internal
 * @brief Returns current image quality
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 * @return @c Ewk_Compression_Proxy_Image_Quality
 */
EAPI Ewk_Compression_Proxy_Image_Quality ewk_context_compression_proxy_image_quality_get(Ewk_Context* context);

 /**
 * @internal
 * @brief Returns original and compressed data size
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[out] original_size uncompressed data size
 * @param[out] compressed_size compressed size
 *
 */
EXPORT_API void ewk_context_compression_proxy_data_size_get(Ewk_Context* context, unsigned* original_size, unsigned* compressed_size);

 /**
 * @internal
 * @brief Reset original and compressed data size
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 *
 */
EXPORT_API void ewk_context_compression_proxy_data_size_reset(Ewk_Context* context);


/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_context_product_h
