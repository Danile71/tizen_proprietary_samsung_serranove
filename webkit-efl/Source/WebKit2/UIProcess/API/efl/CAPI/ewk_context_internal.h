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
 * @file    ewk_context_internal.h
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

#ifndef ewk_context_internal_h
#define ewk_context_internal_h

#include <tizen.h>
#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef ewk_context_type
#define ewk_context_type
/** Creates a type name for @a Ewk_Context. */
typedef struct Ewk_Context Ewk_Context;
#endif

/**
 * Gets default Ewk_Context instance.
 *
 * The returned Ewk_Context object @b should not be unref'ed if application
 * does not call ewk_context_ref() for that.
 *
 * @return Ewk_Context object.
 */
EXPORT_API Ewk_Context* ewk_context_default_get(void);

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
EXPORT_API Ewk_Context* ewk_context_new(void);

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
EXPORT_API Ewk_Context* ewk_context_new_with_injected_bundle_path(const char* path);

/**
 * Deletes Ewk_Context.
 *
 * @param context Ewk_Context to delete
 */
EXPORT_API void ewk_context_delete(Ewk_Context* context);

/**
 * Notify low memory to free unused memory.
 *
 * @param context context object to notify low memory.
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise.
 */
EXPORT_API Eina_Bool ewk_context_notify_low_memory(Ewk_Context* context);

/**
 * Requests for setting soup data path(soup data include cookie and cache).
 *
 * @param context context object
 * @param path soup data path to set
 *
 * @return @c EINA_TRUE on successful request or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_context_soup_data_directory_set(Ewk_Context* context, const char* path);

/**
* Request to set certifcate file
*
* @param context context object
* @param certificate_file is path where certificate file is stored
*
* @return @c EINA_TRUE is cache is enabled or @c EINA_FALSE otherwise
*/
EINA_DEPRECATED EXPORT_API Eina_Bool ewk_context_certificate_file_set(Ewk_Context* context, const char* certificate_file);

/**
* Request to get certifcate file
*
* @param context context object
*
* @return @c certificate_file is path which is set during ewk_context_certificate_file_set or @c NULL otherwise
*/
EINA_DEPRECATED EXPORT_API const char* ewk_context_certificate_file_get(const Ewk_Context* context);

/**
 * Requests to clear cache
 *
 * @param context context object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_cache_clear(Ewk_Context* context);

/**
 * Posts message to injected bundle.
 *
 * @param context context object
 * @param name message name
 * @param body message body
 */
EXPORT_API void ewk_context_message_post_to_injected_bundle(Ewk_Context* context, const char* name, const char* body);

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
 * Sets callback for received injected bundle message.
 *
 * @param context context object
 * @param callback callback for received injected bundle message
 * @param user_data user data
 */
EXPORT_API void ewk_context_message_from_injected_bundle_callback_set(Ewk_Context* context, Ewk_Context_Message_From_Injected_Bundle_Callback callback, void* user_data);

/**
 * Callback for didStartDownload
 *
 * @param download_url url to download
 * @param user_data user_data will be passsed when download is started
 */
typedef void (*Ewk_Context_Did_Start_Download_Callback)(const char* download_url, void* user_data);

/**
 * Sets callback for started download.
 *
 * @param context context object
 * @param callback callback for started download
 * @param user_data user data
 */
EXPORT_API void ewk_context_did_start_download_callback_set(Ewk_Context* context, Ewk_Context_Did_Start_Download_Callback callback, void* user_data);

/**
 * @typedef Ewk_Vibration_Client_Vibrate_Cb Ewk_Vibration_Client_Vibrate_Cb
 * @brief Type definition for a function that will be called back when vibrate
 * request receiveed from the vibration controller.
 */
typedef void (*Ewk_Vibration_Client_Vibrate_Cb)(uint64_t vibration_time, void* user_data);

/**
 * @typedef Ewk_Vibration_Client_Vibration_Cancel_Cb Ewk_Vibration_Client_Vibration_Cancel_Cb
 * @brief Type definition for a function that will be called back when cancel
 * vibration request receiveed from the vibration controller.
 */
typedef void (*Ewk_Vibration_Client_Vibration_Cancel_Cb)(void* user_data);

/**
 * Sets a additional plugin directory.
 *
 * @param context context object
 * @param path the directory to be set
 *
 * @return @c EINA_TRUE if the directory was set, @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_additional_plugin_path_set(Ewk_Context* context, const char* path);

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
EXPORT_API void ewk_context_vibration_client_callbacks_set(Ewk_Context* context, Ewk_Vibration_Client_Vibrate_Cb vibrate, Ewk_Vibration_Client_Vibration_Cancel_Cb cancel, void* data);

 /**
 * Toggles tizen extensible api enable and disable
 *
 * @param context context object
 * @param extensible_api extensible API name of every kind
 * @param enable enable or disable tizen extensible api
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_tizen_extensible_api_string_set(Ewk_Context* context,  const char* extensible_api, Eina_Bool enable);

 /**
 * Get tizen extensible api enable state
 *
 * @param context context object
 * @param extensible_api extensible API name of every kind
 *  *
 * @return @c EINA_TRUE if the extensibleAPI set as true or @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_tizen_extensible_api_string_get(Ewk_Context* context,  const char* extensible_api);

/**
 * Reset storage path such as web storage, web database, application cache and so on
 *
 * @param context context object
 *
 */
EXPORT_API void ewk_context_storage_path_reset(Ewk_Context* context);

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

/**
 * Start the inspector server
 *
 * @param context context object
 * @param port number
 *
 * @return @c return the port number
 */
EXPORT_API unsigned int ewk_context_inspector_server_start(Ewk_Context* context, unsigned int port);

/**
 * Stop the inspector server
 *
 * @param context context object
  *
 * @return @c EINA_TRUE if the inspector server stop set successfully, @c EINA_FALSE otherwise
 */
EXPORT_API Eina_Bool ewk_context_inspector_server_stop(Ewk_Context* context);

#ifdef __cplusplus
}
#endif

#endif // ewk_context_internal_h
