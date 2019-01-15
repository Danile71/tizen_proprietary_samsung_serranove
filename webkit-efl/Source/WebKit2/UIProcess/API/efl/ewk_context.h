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
 * @file    ewk_context.h
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

#ifndef ewk_context_h
#define ewk_context_h

#include "ewk_cookie_manager.h"
#include "ewk_favicon_database.h"
#include "ewk_navigation_data.h"
#include "ewk_url_scheme_request.h"
#include "ewk_autofill_profile.h"
#include <Evas.h>

#include "ewk_security_origin.h"
#include <tizen.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

enum Ewk_Context_Password_Popup_Option {
    EWK_CONTEXT_PASSWORD_POPUP_SAVE,
    EWK_CONTEXT_PASSWORD_POPUP_NOT_NOW,
    EWK_CONTEXT_PASSWORD_POPUP_NEVER
};
/// Creates a type name for @a Ewk_Context_Password_Popup_Option.
typedef enum Ewk_Context_Password_Popup_Option Ewk_Context_Password_Popup_Option;


enum Ewk_Compression_Proxy_Image_Quality {
    EWK_COMPRESSION_PROXY_IMAGE_QUALITY_LOW = 0,    // Low
    EWK_COMPRESSION_PROXY_IMAGE_QUALITY_MEDIUM,     // Medium
    EWK_COMPRESSION_PROXY_IMAGE_QUALITY_HIGH        // High
};
typedef enum Ewk_Compression_Proxy_Image_Quality Ewk_Compression_Proxy_Image_Quality;

/** Creates a type name for @a Ewk_Context. */
typedef struct Ewk_Context Ewk_Context;
typedef struct Ewk_Context_Exceeded_Quota Ewk_Context_Exceeded_Quota;

/**
 * Deletes Ewk_Context.
 *
 * @param context Ewk_Context to delete
 */
EXPORT_API void ewk_context_delete(Ewk_Context* context);

/**
 * Notify low memory to free unused memory.
 *
 * @param o context object to notify low memory.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise.
 */
EXPORT_API Eina_Bool ewk_context_notify_low_memory(Ewk_Context* ewkContext);

/**
 * Sets the given proxy URI to network backend of specific context.
 *
 * @param ewkContext context object to set proxy URI.
 * @param proxy URI to set
 */
EINA_DEPRECATED EXPORT_API void ewk_context_proxy_uri_set(Ewk_Context* ewkContext, const char* proxy);

/**
 * Gets the proxy URI from the network backend of specific context.
 *
 * It returns an internal string and should not
 * be modified. The string is guaranteed to be stringshared.
 *
 * @param ewkContext context object to get proxy URI.
 *
 * @return current proxy URI or @c 0 if it's not set
 */
EINA_DEPRECATED EXPORT_API const char* ewk_context_proxy_uri_get(Ewk_Context* ewkContext);

/**
 * Cancels network session requests.
 *
 * @param context context object to cancel network session requests
 */
EXPORT_API void ewk_context_network_session_requests_cancel(Ewk_Context* context);

/**
 * @typedef Ewk_Local_File_System_Origins_Get_Callback Ewk_Local_File_System_Origins_Get_Callback
 * @brief Type definition for use with ewk_context_local_file_system_origins_get()
 */
typedef void (*Ewk_Local_File_System_Origins_Get_Callback)(Eina_List *origins, void *user_data);

/**
 * Callback for ewk_context_application_cache_origins_get
 *
 * @param origins web application cache origins
 * @param user_data user_data will be passsed when ewk_context_application_cache_origins_get is called
 */
typedef void (*Ewk_Web_Application_Cache_Origins_Get_Callback)(Eina_List* origins, void* user_data);

/**
 * Callback for ewk_context_application_cache_quota_get.
 *
 * @param quota web application cache quota
 * @param user_data user_data will be passsed when ewk_context_application_cache_quota_get is called
 */
typedef void (*Ewk_Web_Application_Cache_Quota_Get_Callback)(int64_t quota, void* user_data);

/**
 * Callback for ewk_context_application_cache_usage_for_origin_get.
 *
 * @param usage web application cache usage for origin
 * @param user_data user_data will be passsed when ewk_context_application_cache_usage_for_origin_get is called
 */
typedef void (*Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback)(int64_t usage, void* user_data);

/**
 * Callback for ewk_context_application_cache_path_get.
 *
 * @param path web application cache directory
 * @param user_data user_data will be passsed when ewk_context_application_cache_path_get is called
 */
typedef void (*Ewk_Web_Application_Cache_Path_Get_Callback)(const char* path, void* user_data);

/**
 * Callback for ewk_context_web_database_origins_get.
 *
 * @param origins web database origins
 * @param user_data user_data will be passsed when ewk_context_web_database_origins_get is called
 */
typedef void (*Ewk_Web_Database_Origins_Get_Callback)(Eina_List* origins, void* user_data);

/**
 * Callback for ewk_context_web_database_quota_for_origin_get.
 *
 * @param quota web database quota
 * @param user_data user_data will be passsed when ewk_context_web_database_quota_for_origin_get is called
 */
typedef void (*Ewk_Web_Database_Quota_Get_Callback)(uint64_t quota, void* user_data);

/**
 * Callback for ewk_context_web_database_usage_for_origin_get.
 *
 * @param usage web database usage
 * @param user_data user_data will be passsed when ewk_context_web_database_usage_for_origin_get is called
 */
typedef void (*Ewk_Web_Database_Usage_Get_Callback)(uint64_t usage, void* user_data);

/**
 * Callback for ewk_context_web_database_path_get.
 *
 * @param path web database directory
 * @param user_data user_data will be passsed when ewk_context_web_database_path_get is called
 */
typedef void (*Ewk_Web_Database_Path_Get_Callback)(const char* path, void* user_data);

/**
 * Callback for ewk_context_web_storage_origins_get.
 *
 * @param origins web storage origins
 * @param user_data user_data will be passsed when ewk_context_web_storage_origins_get is called
 */
typedef void (*Ewk_Web_Storage_Origins_Get_Callback)(Eina_List* origins, void* user_data);

/**
 * Callback for ewk_context_web_storage_usage_for_origin_get.
 *
 * @param usage usage of web storage
 * @param user_data user_data will be passsed when ewk_context_web_storage_usage_for_origin_get is called
 */
typedef void (*Ewk_Web_Storage_Usage_Get_Callback)(uint64_t usage, void* user_data);

/**
 * Callback for ewk_context_web_storage_path_get.
 *
 * @param path web storage directory
 * @param user_data user_data will be passsed when ewk_context_web_storage_path_get is called
 */
typedef void (*Ewk_Web_Storage_Path_Get_Callback)(const char* path, void* user_data);

/**
 * @typedef Ewk_Web_Indexed_Database_Origins_Get_Callback Ewk_Web_Indexed_Database_Origins_Get_Callback
 * @brief Type definition for use with ewk_context_web_indexed_database_origins_get()
 */
typedef void (*Ewk_Web_Indexed_Database_Origins_Get_Callback)(Eina_List *origins, void *user_data);

/**
 * Callback for didReceiveMessageFromInjectedBundle and didReceiveSynchronousMessageFromInjectedBundle
 *
 * User should allocate new string for return_data before setting it.
 * The return_data string will be freed in the WebKit side.
 *
 * @param name name of message from injected bundle
 * @param body body of message from injected bundle
 * @param return_data return_data string from application
 * @param user_data user_data will be passsed when receiving message from injected bundle
 */
typedef void (*Ewk_Context_Message_From_Injected_Bundle_Callback)(const char* name, const char* body, char** return_data, void* user_data);

/**
 * Callback for didStartDownload
 *
 * @param download_url url to download
 * @param user_data user_data will be passsed when download is started
 */
typedef void (*Ewk_Context_Did_Start_Download_Callback)(const char* download_url, void* user_data);

//#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
/**
 * Callback for passworSaveConfirmPopupCallbackCall
 *
 * @param view current view
 * @param user_data user_data will be passsed when password save confirm popup show
 */
typedef void (*Ewk_Context_Password_Confirm_Popup_Callback)(Evas_Object* view, void* user_data);
//#endif

/**
 * Requests for freeing origins.
 *
 * @param origins list of origins
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_origins_free(Eina_List* origins);

/**
 * Requests for deleting all web application caches.
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_delete_all(Ewk_Context* context);

/**
 * Requests for deleting web application cache for origin.
 *
 * @param context context object
 * @param origin application cache origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_delete(Ewk_Context* context, Ewk_Security_Origin* origin);

/**
 * Requests for getting web application cache origins.
 *
 * @param context context object
 * @param result_callback callback to get web application cache origins
 * @param user_data user_data will be passsed when result_callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 *
 * @see ewk_context_origins_free
 */
EXPORT_API Eina_Bool ewk_context_application_cache_origins_get(Ewk_Context* context, Ewk_Web_Application_Cache_Origins_Get_Callback callback, void* user_data);

/**
 * Requests for setting application cache path.
 *
 * @param context context object
 * @param path application cache path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_path_set(Ewk_Context* context, const char* path);

/**
 * Requests for getting application cache path.
 *
 * @param context context object
 * @param callback callback to get web application cache directory
 * @param userData will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_path_get(Ewk_Context* ewkContext, Ewk_Web_Application_Cache_Path_Get_Callback callback, void* userData);

/**
 * Requests for getting application cache quota.
 *
 * @param context context object
 * @param result_callback callback to get web database quota
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_quota_get(Ewk_Context* context, Ewk_Web_Application_Cache_Quota_Get_Callback callback, void* user_data);

/**
 * Requests for setting application cache quota.
 *
 * @param context context object
 * @param quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_quota_set(Ewk_Context* context, int64_t quota);

/**
 * Requests for setting application cache quota for origin.
 *
 * @param context context object
 * @param origin serucity origin
 * @param quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_quota_for_origin_set(Ewk_Context* context, const Ewk_Security_Origin* origin, int64_t quota);

/**
 * Requests for getting application cache usage for origin.
 *
 * @param context context object
 * @param origin security origin
 * @param result_callback callback to get web database usage
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_application_cache_usage_for_origin_get(Ewk_Context* context, const Ewk_Security_Origin* origin, Ewk_Web_Application_Cache_Usage_For_Origin_Get_Callback callback, void* userData);

/**
 * Requests setting of the favicon database path.
 *
 * @param context context object
 * @param path database path.
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_icon_database_path_set(Ewk_Context* context, const char* path);

/**
 * Requests to get image representing the given URL.
 *
 * @param context context object
 * @param url which url to query icon, must not be @c 0
 * @param canvas evas instance where to add resulting object, must not be @c 0
 *
 * @return newly allocated Evas_Object instance or @c 0 on
 *         errors. Delete the object with evas_object_del().
 */
EXPORT_API Evas_Object* ewk_context_icon_database_icon_object_add(Ewk_Context* context, const char* uri, Evas* canvas);

/**
 * Deletes all known icons from database.
 *
 * @param context context object
 */
EXPORT_API void ewk_context_icon_database_delete_all(Ewk_Context* context);

/**
 * Requests for setting local file system path.
 *
 * @param context context object
 * @param path local file system path
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_local_file_system_path_set(Ewk_Context* context, const char* path);

/**
 * Requests for deleting all local file systems.
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_local_file_system_all_delete(Ewk_Context *context);

/**
 * Requests for deleting local file system for origin.
 *
 * @param context context object
 * @param origin local file system origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_local_file_system_delete(Ewk_Context *context, Ewk_Security_Origin *origin);

 /**
 * Requests for getting local file system origins.
 *
 * @param context context object
 * @param result_callback callback to get local file system origins
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 *
 * @see ewk_context_origins_free
 */
EXPORT_API Eina_Bool ewk_context_local_file_system_origins_get(const Ewk_Context *context, Ewk_Local_File_System_Origins_Get_Callback callback, void *user_data);

/**
 * Requests for deleting all web databases.
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_delete_all(Ewk_Context* context);

/**
 * Requests for deleting web databases for origin.
 *
 * @param context context object
 * @param origin database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_delete(Ewk_Context* context, Ewk_Security_Origin* origin);

/**
 * Requests for getting web database origins.
 *
 * @param context context object
 * @param result_callback callback to get web database origins
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 *
 * @see ewk_context_origins_free
 */
EXPORT_API Eina_Bool ewk_context_web_database_origins_get(Ewk_Context* context, Ewk_Web_Database_Origins_Get_Callback callback, void* user_data);

/**
 * Requests for setting web database path.
 *
 * @param context context object
 * @param path web database path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_path_set(Ewk_Context* context, const char* path);

/**
 * Requests for getting web database path.
 *
 * @param context context object
 * @param callback callback to get web database directory
 * @param userData will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_path_get(Ewk_Context* context, Ewk_Web_Database_Path_Get_Callback callback, void* user_data);

/**
 * Requests for getting web database quota for origin.
 *
 * @param context context object
 * @param result_callback callback to get web database quota
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 * @param origin database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_quota_for_origin_get(Ewk_Context* context, Ewk_Web_Database_Quota_Get_Callback callback, void* user_data, Ewk_Security_Origin* origin);

/**
 * Requests for setting web database default quota.
 *
 * @param context context object
 * @param quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_default_quota_set(Ewk_Context* context, uint64_t quota);

/**
 * Requests for setting web database quota for origin.
 *
 * @param context context object
 * @param origin database origin
 * @param quota size of quota
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_quota_for_origin_set(Ewk_Context* context, Ewk_Security_Origin* origin, uint64_t quota);

/**
 * Requests for getting web database usage for origin.
 *
 * @param context context object
 * @param result_callback callback to get web database usage
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 * @param origin database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_database_usage_for_origin_get(Ewk_Context* context, Ewk_Web_Database_Usage_Get_Callback callback, void* user_data, Ewk_Security_Origin* origin);

/**
 * Requests for deleting all web indexed databases.
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_indexed_database_delete_all(Ewk_Context* context);

/**
 * Requests for deleting web indexed database for origin.
 *
 * @param context context object
 * @param origin indexed database origin
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_indexed_database_delete(Ewk_Context* ewkContext, Ewk_Security_Origin* origin);

/**
 * Gets list of origins that is stored in web indexed database.
 *
 * This function allocates memory for context structure made from callback and user_data.
 *
 * @param context context object
 * @param result_callback callback to get web storage origins
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 *
 * @See ewk_context_origins_free()
 */
EXPORT_API Eina_Bool ewk_context_web_indexed_database_origins_get(const Ewk_Context* ewkContext, Ewk_Web_Indexed_Database_Origins_Get_Callback callback, void* userData);

/**
 * Requests for setting indexed database path.
 *
 * @param context context object
 * @param path indexed database path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_indexed_database_path_set (Ewk_Context* context, const char* path);

/**
 * Deletes all web storage.
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_storage_delete_all(Ewk_Context* context);

/**
 * Deletes origin that is stored in web storage db.
 *
 * @param context context object
 * @param origin origin of db
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_storage_origin_delete(Ewk_Context* context, Ewk_Security_Origin* origin);

/**
 * Gets list of origins that is stored in web storage db.
 *
 * This function allocates memory for context structure made from callback and user_data.
 *
 * @param context context object
 * @param result_callback callback to get web storage origins
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 *
 * @See ewk_context_origins_free()
 */
EXPORT_API Eina_Bool ewk_context_web_storage_origins_get(Ewk_Context* context, Ewk_Web_Storage_Origins_Get_Callback callback, void* user_data);

/**
 * Requests for setting web storage path.
 *
 * @param context context object
 * @param path web storage path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_storage_path_set(Ewk_Context* context, const char* path);

/**
 * Requests for getting web storage path.
 *
 * @param context context object
 * @param callback callback to get web storage directory
 * @param userData will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_storage_path_get(Ewk_Context* context, Ewk_Web_Storage_Path_Get_Callback callback, void* user_data);

/**
 * Gets usage of web storage for certain origin.
 *
 * This function allocates memory for context structure made from callback and user_data.
 *
 * @param context context object
 * @param origin security origin
 * @param callback callback to get web storage usage
 * @param user_data user_data will be passed when result_callback is called
 *    -I.e., user data will be kept until callback is called
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_web_storage_usage_for_origin_get(Ewk_Context* context, Ewk_Security_Origin* origin, Ewk_Web_Storage_Usage_Get_Callback callback, void* user_data);

//#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
/**
 * Requests for setting soup data path(soup data include cookie and cache).
 *
 * @param context context object
 * @param path soup data path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_soup_data_directory_set(Ewk_Context* context, const char* path);
//#endif

/**
 * \enum    Ewk_Cache_Model
 *
 * @brief   Contains option for cache model
 */
enum Ewk_Cache_Model {
    /// Use the smallest cache capacity.
    EWK_CACHE_MODEL_DOCUMENT_VIEWER,
    /// Use bigger cache capacity than EWK_CACHE_MODEL_DOCUMENT_VIEWER.
    EWK_CACHE_MODEL_DOCUMENT_BROWSER,
    /// Use the biggest cache capacity.
    EWK_CACHE_MODEL_PRIMARY_WEBBROWSER
};

/// Creates a type name for the Ewk_Cache_Model.
typedef enum Ewk_Cache_Model Ewk_Cache_Model;

/**
* Requests to set the cache model
*
* @param context context object
* @param model cache model
*
* @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
*/
EXPORT_API Eina_Bool ewk_context_cache_model_set(Ewk_Context* context, Ewk_Cache_Model model);

/**
* Returns the cache model type
*
* @param context context object
*
* @return Ewk_Cache_Model
*/
EXPORT_API Ewk_Cache_Model ewk_context_cache_model_get(const Ewk_Context* context);

 /**
* Toggles the cache enable and disable
*
* @param context context object
* @param enable or disable cache
*
* @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
*/
EXPORT_API Eina_Bool ewk_context_cache_disabled_set(Ewk_Context* ewkContext, Eina_Bool cacheDisabled);

/**
* Queries if the cache is enabled
*
* @param context context object
*
* @return @c EINA_TRUE is cache is enabled or @c EINA_FALSE otherwise
*/
EXPORT_API Eina_Bool ewk_context_cache_disabled_get(const Ewk_Context* ewkContext);

/**
* Request to set certifcate file
*
* @param context context object
* @param certificate_file is path where certificate file is stored
*
* @return @c EINA_TRUE is cache is enabled or @c EINA_FALSE otherwise
*/
EINA_DEPRECATED EXPORT_API Eina_Bool ewk_context_certificate_file_set(Ewk_Context *context, const char *certificate_file);

/**
* Request to get certifcate file
*
* @param context context object
*
* @return @c certificate_file is path which is set during ewk_context_certificate_file_set or @c NULL otherwise
*/
EINA_DEPRECATED EXPORT_API const char* ewk_context_certificate_file_get(const Ewk_Context *context);

/**
 * Requests to clear cache
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_cache_clear(Ewk_Context* context);

/**
 * @brief Clears HTTP caches in local storage and all resources cached in memory\n
 * such as images, CSS, JavaScript, XSL, and fonts for @a context.
 *
 * @param[in] context context object to clear all resource caches
 */
EXPORT_API void ewk_context_resource_cache_clear(Ewk_Context *context);

/**
 * Posts message to injected bundle.
 *
 * @param context context object
 * @param name message name
 * @param body message body
 */
EXPORT_API void ewk_context_message_post_to_injected_bundle(Ewk_Context* context, const char* name, const char* body);

/**
 * Sets callback for received injected bundle message.
 *
 * @param context context object
 * @param callback callback for received injected bundle message
 * @param user_data user data
 */
EXPORT_API void ewk_context_message_from_injected_bundle_callback_set(Ewk_Context* context, Ewk_Context_Message_From_Injected_Bundle_Callback callback, void* user_data);

/**
 * Sets callback for started download.
 *
 * @param context context object
 * @param callback callback for started download
 * @param user_data user data
 */
EXPORT_API void ewk_context_did_start_download_callback_set(Ewk_Context* context, Ewk_Context_Did_Start_Download_Callback callback, void* user_data);

//#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
/**
 * Sets callback for show password confirm popup.
 *
 * @param context context object
 * @param callback callback for show password confirm popup
 * @param user_data user data
 */
EXPORT_API void ewk_context_password_confirm_popup_callback_set(Ewk_Context* context, Ewk_Context_Password_Confirm_Popup_Callback callback, void* user_data);
//#endif

//#if ENABLE(MEMORY_SAMPLER)
/**
 * start memory sampler.
 *
 * @param context context object
 * @param interval time gap to fire the timer
*/
EXPORT_API void ewk_context_memory_sampler_start(Ewk_Context* context, double timer_interval);

/**
* stop memory sampler.
*
* @param context context object
*/
EXPORT_API void ewk_context_memory_sampler_stop(Ewk_Context* context);
//#endif

/**
 * @typedef Ewk_Url_Scheme_Request_Cb Ewk_Url_Scheme_Request_Cb
 * @brief Callback type for use with ewk_context_url_scheme_register().
 */
typedef void (*Ewk_Url_Scheme_Request_Cb) (Ewk_Url_Scheme_Request *request, void *user_data);

/**
 * @typedef Ewk_Vibration_Client_Vibrate_Cb Ewk_Vibration_Client_Vibrate_Cb
 * @brief Type definition for a function that will be called back when vibrate
 * request receiveed from the vibration controller.
 */
typedef void (*Ewk_Vibration_Client_Vibrate_Cb)(uint64_t vibration_time, void *user_data);

/**
 * @typedef Ewk_Vibration_Client_Vibration_Cancel_Cb Ewk_Vibration_Client_Vibration_Cancel_Cb
 * @brief Type definition for a function that will be called back when cancel
 * vibration request receiveed from the vibration controller.
 */
typedef void (*Ewk_Vibration_Client_Vibration_Cancel_Cb)(void *user_data);

/**
 * @typedef Ewk_History_Navigation_Cb Ewk_History_Navigation_Cb
 * @brief Type definition for a function that will be called back when @a view did navigation (loaded new URL).
 */
typedef void (*Ewk_History_Navigation_Cb)(const Evas_Object *view, Ewk_Navigation_Data *navigation_data, void *user_data);

/**
 * @typedef Ewk_History_Client_Redirection_Cb Ewk_History_Client_Redirection_Cb
 * @brief Type definition for a function that will be called back when @a view performed a client redirect.
 */
typedef void (*Ewk_History_Client_Redirection_Cb)(const Evas_Object *view, const char *source_url, const char *destination_url, void *user_data);

/**
 * @typedef Ewk_History_Server_Redirection_Cb Ewk_History_Server_Redirection_Cb
 * @brief Type definition for a function that will be called back when @a view performed a server redirect.
 */
typedef void (*Ewk_History_Server_Redirection_Cb)(const Evas_Object *view, const char *source_url, const char *destination_url, void *user_data);

/**
 * @typedef Ewk_History_Title_Update_Cb Ewk_History_Title_Update_Cb
 * @brief Type definition for a function that will be called back when history title is updated.
 */
typedef void (*Ewk_History_Title_Update_Cb)(const Evas_Object *view, const char *title, const char *url, void *user_data);

/**
 * @typedef Ewk_Context_History_Client_Visited_Links_Populate_Cb Ewk_Context_History_Client_Visited_Links_Populate_Cb
 * @brief Type definition for a function that will be called back when client is asked to provide visited links from a client-managed storage.
 *
 * @see ewk_context_visited_link_add
 */
typedef void (*Ewk_History_Populate_Visited_Links_Cb)(void *user_data);

/**
 * Increases the reference count of the given object.
 *
 * @param context context object to increase the reference count
 *
 * @return Ewk_Context object on success or @c NULL on failure
 */
EXPORT_API Ewk_Context *ewk_context_ref(Ewk_Context *context);

/**
 * Decreases the reference count of the given object, possibly freeing it.
 *
 * When the reference count it's reached 0, the Ewk_Context is freed.
 *
 * @param context context object to decrease the reference count
 */
EXPORT_API void ewk_context_unref(Ewk_Context *context);

/**
 * Gets default Ewk_Context instance.
 *
 * The returned Ewk_Context object @b should not be unref'ed if application
 * does not call ewk_context_ref() for that.
 *
 * @return Ewk_Context object.
 */
EXPORT_API Ewk_Context *ewk_context_default_get(void);

/**
 * Creates a new Ewk_Context.
 *
 * The returned Ewk_Context object @b should be unref'ed after use.
 *
 * @return Ewk_Context object on success or @c NULL on failure
 *
 * @see ewk_context_unref
 * @see ewk_context_new_with_injected_bundle_path
 */
EXPORT_API Ewk_Context *ewk_context_new(void);

/**
 * Creates a new Ewk_Context.
 *
 * The returned Ewk_Context object @b should be unref'ed after use.
 *
 * @param path path of injected bundle library
 *
 * @return Ewk_Context object on success or @c NULL on failure
 *
 * @see ewk_context_unref
 * @see ewk_context_new
 */
EXPORT_API Ewk_Context *ewk_context_new_with_injected_bundle_path(const char *path);

/**
 * Gets the cookie manager instance for this @a context.
 *
 * @param context context object to query.
 *
 * @return Ewk_Cookie_Manager object instance or @c NULL in case of failure.
 */
EXPORT_API Ewk_Cookie_Manager *ewk_context_cookie_manager_get(const Ewk_Context *context);

/**
 * Gets the favicon database instance for this @a context.
 *
 * @param context context object to query.
 *
 * @return Ewk_Favicon_Database object instance or @c NULL in case of failure.
 */
EXPORT_API Ewk_Favicon_Database *ewk_context_favicon_database_get(const Ewk_Context *context);

/**
 * Register @a scheme in @a context.
 *
 * When an URL request with @a scheme is made in the #Ewk_Context, the callback
 * function provided will be called with a #Ewk_Url_Scheme_Request.
 *
 * It is possible to handle URL scheme requests asynchronously, by calling ewk_url_scheme_ref() on the
 * #Ewk_Url_Scheme_Request and calling ewk_url_scheme_request_finish() later when the data of
 * the request is available.
 *
 * @param context a #Ewk_Context object.
 * @param scheme the network scheme to register
 * @param callback the function to be called when an URL request with @a scheme is made.
 * @param user_data data to pass to callback function
 *
 * @code
 * static void about_url_scheme_request_cb(Ewk_Url_Scheme_Request *request, void *user_data)
 * {
 *     const char *path;
 *     char *contents_data = NULL;
 *     unsigned int contents_length = 0;
 *
 *     path = ewk_url_scheme_request_path_get(request);
 *     if (!strcmp(path, "plugins")) {
 *         // Initialize contents_data with the contents of plugins about page, and set its length to contents_length
 *     } else if (!strcmp(path, "memory")) {
 *         // Initialize contents_data with the contents of memory about page, and set its length to contents_length
 *     } else if (!strcmp(path, "applications")) {
 *         // Initialize contents_data with the contents of application about page, and set its length to contents_length
 *     } else {
 *         Eina_Strbuf *buf = eina_strbuf_new();
 *         eina_strbuf_append_printf(buf, "&lt;html&gt;&lt;body&gt;&lt;p&gt;Invalid about:%s page&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;", path);
 *         contents_data = eina_strbuf_string_steal(buf);
 *         contents_length = strlen(contents);
 *         eina_strbuf_free(buf);
 *     }
 *     ewk_url_scheme_request_finish(request, contents_data, contents_length, "text/html");
 *     free(contents_data);
 * }
 * @endcode
 */
EXPORT_API Eina_Bool ewk_context_url_scheme_register(Ewk_Context *context, const char *scheme, Ewk_Url_Scheme_Request_Cb callback, void *user_data);

// #if ENABLE(TIZEN_SUPPORT_PLUGINS)
/**
 * Sets a additional plugin directory.
 *
 * @param context context object
 * @param path the directory to be set
 *
 * @return @c EINA_TRUE if the directory was set, @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_additional_plugin_path_set(Ewk_Context *context, const char *path);
// #endif

/**
 * Sets vibration client callbacks to handle the tactile feedback in the form of
 * vibration in the client application when the content asks for vibration.
 *
 * To stop listening for vibration events, you may call this function with @c
 * NULL for the callbacks.
 *
 * @param context context object to set vibration client callbacks.
 * @param vibrate The function to call when the vibrate request received from the
 *        controller (may be @c NULL).
 * @param cancel The function to call when the cancel vibration request received
 *        from the controller (may be @c NULL).
 * @param data User data (may be @c NULL).
 */
EXPORT_API void ewk_context_vibration_client_callbacks_set(Ewk_Context *context, Ewk_Vibration_Client_Vibrate_Cb vibrate, Ewk_Vibration_Client_Vibration_Cancel_Cb cancel, void *data);

/**
 * Sets history callbacks for the given @a context.
 *
 * To stop listening for history events, you may call this function with @c
 * NULL for the callbacks.
 *
 * @param context context object to set history callbacks
 * @param navigate_func The function to call when @c ewk_view did navigation (may be @c NULL).
 * @param client_redirect_func The function to call when @c ewk_view performed a client redirect (may be @c NULL).
 * @param server_redirect_func The function to call when @c ewk_view performed a server redirect (may be @c NULL).
 * @param title_update_func The function to call when history title is updated (may be @c NULL).
 * @param populate_visited_links_func The function is called when client is asked to provide visited links from a
 *        client-managed storage (may be @c NULL).
 * @param data User data (may be @c NULL).
 */
EXPORT_API void ewk_context_history_callbacks_set(Ewk_Context *context,
                                            Ewk_History_Navigation_Cb navigate_func,
                                            Ewk_History_Client_Redirection_Cb client_redirect_func,
                                            Ewk_History_Server_Redirection_Cb server_redirect_func,
                                            Ewk_History_Title_Update_Cb title_update_func,
                                            Ewk_History_Populate_Visited_Links_Cb populate_visited_links_func,
                                            void *data);

/**
 * Registers the given @a visited_url as visited link in @a context visited link cache.
 *
 * This function shall be invoked as a response to @c populateVisitedLinks callback of the history cient.
 *
 * @param context context object to add visited link data
 * @param visited_url visited url
 *
 * @see Ewk_Context_History_Client
 */
EXPORT_API void ewk_context_visited_link_add(Ewk_Context *context, const char *visited_url);

//#if ENABLE(TIZEN_WEBKIT2_MEMORY_SAVING_MODE)
/**
 * Sets memory saving mode.
 *
 * @param context context object
 * @param enable or disable memory saving mode
 *
 */
EXPORT_API void ewk_context_memory_saving_mode_set(Ewk_Context *context, Eina_Bool mode);
//#endif

//#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
struct Ewk_Password_Data {
    char* url;
    Eina_Bool useFingerprint;
};

/**
 * Clear password data
 *
 * @param context context object
 *
 */
EINA_DEPRECATED EXPORT_API void ewk_context_form_password_data_clear(Ewk_Context* context);

/**
 * Deletes whole password data from DB
 *
 * @param context context object
 *
 */
EXPORT_API void ewk_context_form_password_data_delete_all(Ewk_Context* context);

/**
 * Updates use fingerprint value from DB for given URL
 *
 * The API will update use fingerprint value on DB for given URL.
 *
 * @param const char* with url
 * @param Eina_Bool with useFingerprint
 *
 * @see ewk_context_form_password_data_list_free
 * @see ewk_context_form_password_data_delete
 * @see ewk_context_form_password_data_list_get
 */
EXPORT_API void ewk_context_form_password_data_update(Ewk_Context* context, const char* url, Eina_Bool useFingerprint);

/**
 * Deletes password data from DB for given URL
 *
 * The API will delete the a password data from DB.
 *
 * @param const char* with url
 *
 * @see ewk_context_form_password_data_update
 * @see ewk_context_form_password_data_list_free
 * @see ewk_context_form_password_data_list_get
 */
EXPORT_API void ewk_context_form_password_data_delete(Ewk_Context* context, const char* url);

/**
 * Get all password url list
 *
 * The API will delete the a password data list only from the memory.
 * To remove the password data for URL permenantly, use ewk_context_form_password_data_delete
 *
 * @param Eina_List with Ewk_Password_Data
 *
 * @see ewk_context_form_password_data_update
 * @see ewk_context_form_password_data_delete
 * @see ewk_context_form_password_data_list_free
 */
EXPORT_API Eina_List* ewk_context_form_password_data_list_get(Ewk_Context* context);

/**
 * Deletes a given password data list
 *
 * The API will delete the a password data list only from the memory.
 * To remove the password data for URL permenantly, use ewk_context_form_password_data_delete
 *
 * @param Eina_List with Ewk_Password_Data
 *
 * @see ewk_context_form_password_data_update
 * @see ewk_context_form_password_data_delete
 * @see ewk_context_form_password_data_list_get
 */
EXPORT_API void ewk_context_form_password_data_list_free(Ewk_Context* context, Eina_List* list);

/**
 * Clear candidate data
 *
 * @param context context object
 *
 */
EINA_DEPRECATED EXPORT_API void ewk_context_form_candidate_data_clear(Ewk_Context* context);

/**
 * Deletes all candidate form data from DB
 *
 * @param context context object
 *
 */
EXPORT_API void ewk_context_form_candidate_data_delete_all(Ewk_Context* context);

//#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
/**
 * Gets a list of all existing profiles
 *
 * The obtained profile must be deleted by ewk_autofill_profile_delete.
 * @param context context object
 *
 * @return @c Eina_List of Ewk_Autofill_Profile @c NULL otherwise
 * @see ewk_autofill_profile_delete
 */
EXPORT_API Eina_List* ewk_context_form_autofill_profile_get_all(Ewk_Context* context);

/**
 * Gets the existing profile for given index
 *
 * The obtained profile must be deleted by ewk_autofill_profile_delete.
 *
 * @param context context object
 * @param profile id
 *
 * @return @c Ewk_Autofill_Profile if profile exists, @c NULL otherwise
 * @see ewk_autofill_profile_delete
 */
EXPORT_API Ewk_Autofill_Profile* ewk_context_form_autofill_profile_get(Ewk_Context* context, unsigned id);

/**
 * Sets the given profile for the given id
 *
 * Data can be added to the created profile by ewk_autofill_profile_data_set.
 *
 * @param context context object
 * @param profile id
 * @param profile Ewk_Autofill_Profile
 *
 * @return @c EINA_TRUE if the profile data is set successfully, @c EINA_FALSE otherwise
 * @see ewk_autofill_profile_new
 * @see ewk_context_form_autofill_profile_add
 */
EXPORT_API Eina_Bool ewk_context_form_autofill_profile_set(Ewk_Context* context, unsigned id, Ewk_Autofill_Profile* profile);

/**
 * Saves the created profile into permenant storage
 *
 * The profile used to save must be created by ewk_autofill_profile_new.
 * Data can be added to the created profile by ewk_autofill_profile_data_set.
 *
 * @param context context object
 * @param profile Ewk_Autofill_Profile
 *
 * @return @c EINA_TRUE if the profile data is saved successfully, @c EINA_FALSE otherwise
 * @see ewk_autofill_profile_new
 */
EXPORT_API Eina_Bool ewk_context_form_autofill_profile_add(Ewk_Context* context, Ewk_Autofill_Profile* profile);

/**
 * Removes Autofill Form profile completely
 *
 * @param context context object
 * @param index profile id
 *
 * @return @c EINA_TRUE if the profile data is removed successfully, @c EINA_FALSE otherwise
 * @see ewk_context_form_autofill_profile_get_all
 */
EXPORT_API Eina_Bool ewk_context_form_autofill_profile_remove(Ewk_Context* context, unsigned id);
//#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
//#endif

//#if ENABLE(TIZEN_EXTENSIBLE_API)
 /**
 * Toggles tizen extensible api enable and disable
 *
 * @param context context object
 * @param extensible_api extensible API name of every kind
 * @param enable enable or disable tizen extensible api
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_tizen_extensible_api_string_set(Ewk_Context *context,  const char *extensible_api, Eina_Bool enable);
//#endif

//#if ENABLE(TIZEN_EXTENSIBLE_API)
 /**
 * Get tizen extensible api enable state
 *
 * @param context context object
 * @param extensible_api extensible API name of every kind
 *  *
 * @return @c EINA_TRUE if the extensibleAPI set as true or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_tizen_extensible_api_string_get(Ewk_Context *context,  const char *extensible_api);
//#endif

//#if ENABLE(TIZEN_RESET_PATH)
/**
 * Reset storage path such as web storage, web database, application cache and so on
 *
 * @param context context object
 *
 */
EXPORT_API void ewk_context_storage_path_reset(Ewk_Context* ewkContext);
//#endif

//#if ENABLE(TIZEN_SUPPORT_WEBPROVIDER_AC)
/**
 * Sets the given id for the pixmap
 *
 * Data can be added to the created profile by ewk_autofill_profile_data_set.
 *
 * @param context context object
 * @param pixmap id
 *
 * @return @c EINA_TRUE if the pixmap set successfully, @c EINA_FALSE otherwise
 */
EXPORT_API void ewk_context_pixmap_set(Ewk_Context* context, int pixmap);
// #endif

//#if ENABLE(TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR)
/**
 * Start the inspector server
 *
 * @param context context object
 * @param port number
 *
 * @return @c return the port number
 */
EXPORT_API unsigned int ewk_context_inspector_server_start(Ewk_Context* context, unsigned int port);
// #endif

//#if ENABLE(TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR)
/**
 * Stop the inspector server
 *
 * @param context context object
  *
 * @return @c EINA_TRUE if the inspector server stop set successfully, @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_inspector_server_stop(Ewk_Context* context);
// #endif

//#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
/**
 * Password confirm popup reply
 *
 * @param context context object
 * @param result The option of response
  *
 */
EXPORT_API void ewk_context_password_confirm_popup_reply(Ewk_Context* context, Ewk_Context_Password_Popup_Option result);
// #endif

/**
 * @deprecated Deprecated since 2.4
 *
 * Sets the list of preferred languages.
 *
 * Sets the list of preferred langages. This list will be used to build the "Accept-Language"
 * header that will be included in the network requests.
 * Client can pass @c NULL for languages to clear what is set before.
 *
 * @param languages the list of preferred languages (char* as data) or @c NULL
 *
 * @note all contexts will be affected.
 */
EXPORT_API void ewk_context_preferred_languages_set(Eina_List *languages);

 /**
 * Returns original and compressed data size
 *
 * @param context context object
 * @param original_size uncompressed data size
 * @param compressed_size compressed size
 *
 */
EXPORT_API void ewk_context_compression_proxy_data_size_get(Ewk_Context* context, unsigned* original_size, unsigned* compressed_size);

 /**
 * Enable or disable proxy
 *
 * @param context context object
 * @param enable enable or disable proxy
 *
 */
EXPORT_API void ewk_context_compression_proxy_enabled_set(Ewk_Context* context, Eina_Bool enabled);

 /**
 * Set image quality of proxy compression
 *
 * @param context context object
 * @param quality image quality
 *
 */
EXPORT_API void ewk_context_compression_proxy_image_quality_set(Ewk_Context* context, Ewk_Compression_Proxy_Image_Quality quality);

 /**
 * Reset original and compressed data size
 *
 * @param context context object
 *
 */
EXPORT_API void ewk_context_compression_proxy_data_size_reset(Ewk_Context* context);

#ifdef __cplusplus
}
#endif

#endif // ewk_context_h
