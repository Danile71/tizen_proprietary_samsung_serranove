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
 * @brief   This file describes the Ewk Context API.
 *
 * @remarks ewk_context encapsulates all pages related to the specific use of WebKit.
 *
 * Applications have the option of creating a context different from the default one
 * and using it for a group of pages. All pages in the same context share the same
 * preferences, visited link set, local storage, and so on.
 *
 * A process model can be specified per context. The default one is the shared model
 * where the web-engine process is shared among the pages in the context. The second
 * model allows each page to use a separate web-engine process. This latter model is
 * currently not supported by WebKit2/EFL.
 *
 */

#ifndef ewk_context_h
#define ewk_context_h

#include "ewk_autofill_profile.h"
#include "ewk_cookie_manager.h"
#include <tizen.h>
#include <Eina.h>
#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

#ifndef ewk_context_type
#define ewk_context_type
/**
 * @brief The structure type that creates a type name for #Ewk_Context.
 * @since_tizen 2.3
 */
typedef struct Ewk_Context Ewk_Context;
#endif

/**
 * @brief Gets the cookie manager instance for this @a context.
 *
 * @since_tizen 2.3
 *
 * @param[in] context The context object to query
 *
 * @return The Ewk_Cookie_Manager object instance,\n
 *         otherwise @c NULL in case of failure
 */
EXPORT_API Ewk_Cookie_Manager* ewk_context_cookie_manager_get(const Ewk_Context* context);

/**
 * \enum    Ewk_Cache_Model
 *
 * @brief   Enumeration that contains option for the cache model.
 * @since_tizen 2.3
 */
enum Ewk_Cache_Model {
    EWK_CACHE_MODEL_DOCUMENT_VIEWER,    /**< Use the smallest cache capacity */
    EWK_CACHE_MODEL_DOCUMENT_BROWSER,   /**< Use bigger cache capacity than EWK_CACHE_MODEL_DOCUMENT_VIEWER */
    EWK_CACHE_MODEL_PRIMARY_WEBBROWSER  /**< Use the biggest cache capacity */
};

/**
 * @brief Enumeration that creates a type name for the #Ewk_Cache_Model.
 * @since_tizen 2.3
 */
typedef enum Ewk_Cache_Model Ewk_Cache_Model;

/**
 * @brief Requests for deleting all web application caches.
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 *
 * @return @c EINA_TRUE on success, otherwise @c EINA FALSE
 */
EXPORT_API Eina_Bool ewk_context_application_cache_delete_all(Ewk_Context* context);

/**
 * @brief Requests to set the cache model.
 *
 * @since_tizen 2.3
 *
 * @param[in] context The context object
 * @param[in] model The cache model
 *
 * @return @c EINA_TRUE on success,\n
 *         otherwise @c EINA_FALSE
 */
EXPORT_API Eina_Bool ewk_context_cache_model_set(Ewk_Context* context, Ewk_Cache_Model model);

/**
 * @brief Returns the cache model type.
 *
 * @since_tizen 2.3
 *
 * @param[in] context The context object
 *
 * @return #Ewk_Cache_Model
 */
EXPORT_API Ewk_Cache_Model ewk_context_cache_model_get(const Ewk_Context* context);

/**
 * @brief Saves the created profile into permenant storage
 *
 * The profile used to save must be created by ewk_autofill_profile_new.
 * Data can be added to the created profile by ewk_autofill_profile_data_set.
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 * @param[in] profile Ewk_Autofill_Profile
 *
 * @return @c EINA_TRUE if the profile data is saved successfully, otherwise @c EINA_FALSE
 *
 * @see ewk_autofill_profile_new
 */
EXPORT_API Eina_Bool ewk_context_form_autofill_profile_add(Ewk_Context* context, Ewk_Autofill_Profile* profile);

/**
 * @brief Gets the existing profile for given index
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 * @param[in] id Profile
 *
 * @return @c Ewk_Autofill_Profile if profile exists, otherwise @c NULL
 *
 * @see ewk_autofill_profile_delete
 */
EXPORT_API Ewk_Autofill_Profile* ewk_context_form_autofill_profile_get(Ewk_Context* context, unsigned id);


/**
 * @brief Gets a list of all existing profiles
 *
 * The obtained profile must be deleted by ewk_autofill_profile_delete.
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 *
 * @return @c Eina_List of Ewk_Autofill_Profile, otherwise @c NULL
 *
 * @see ewk_autofill_profile_delete
 */
EXPORT_API Eina_List* ewk_context_form_autofill_profile_get_all(Ewk_Context* context);

/**
 * @brief Sets the given profile for the given id
 *
 * Data can be added to the created profile by ewk_autofill_profile_data_set.
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 * @param[in] id Profile
 * @param[in] profile Ewk_Autofill_Profile
 *
 * @return @c EINA_TRUE if the profile data is set successfully, otherwise @c EINA_FALSE
 *
 * @see ewk_autofill_profile_new
 * @see ewk_context_form_autofill_profile_add
 */
EXPORT_API Eina_Bool ewk_context_form_autofill_profile_set(Ewk_Context* context, unsigned id, Ewk_Autofill_Profile* profile);

/**
 * @brief Removes Autofill Form profile completely
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 * @param[in] id Profile
 *
 * @return @c EINA_TRUE if the profile data is removed successfully, otherwise @c EINA_FALSE
 *
 * @see ewk_context_form_autofill_profile_get_all
 */
EXPORT_API Eina_Bool ewk_context_form_autofill_profile_remove(Ewk_Context* context, unsigned id);

/**
 * @brief Deletes all candidate form data from DB
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 */
EXPORT_API void ewk_context_form_candidate_data_delete_all(Ewk_Context* context);

/**
 * @brief Deletes whole password data from DB
 *
 * @since_tizen 2.4
 *
 * @param context Context object
 *
 */
EXPORT_API void ewk_context_form_password_data_delete_all(Ewk_Context* context);

/**
 * @brief Clears HTTP caches in the local storage and all resources cached in memory\n
 *        such as images, CSS, JavaScript, XSL, and fonts for @a context.
 *
 * @since_tizen 2.3
 *
 * @param[in] context The context object to clear all resource caches
 */
EXPORT_API void ewk_context_resource_cache_clear(Ewk_Context* context);

/**
 * @brief Requests to get image representing the given URL.
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 * @param[in] url Which url to query icon, must not be @c 0
 * @param[in] canvas Evas instance where to add resulting object, must not be @c 0
 *
 * @return @c Evas_Object instance, otherwise @c 0
 */
EXPORT_API Evas_Object* ewk_context_icon_database_icon_object_add(Ewk_Context* context, const char* uri, Evas* canvas);

/**
 * @deprecated Deprecated since 2.4
 *
 * @brief Sets the list of preferred languages.
 *
 * @details This function sets the list of preferred langages.\n
 *          This list will be used to build the "Accept-Language" header that will be included in the network requests.\n
 *          The client can pass @c NULL for languages to clear what is set before.
 *
 * @since_tizen 2.3
 *
 * @remarks All contexts will be affected.
 *
 * @param[in] languages The list of preferred languages (char* as data),\n
 *                      otherwise @c NULL
 */
EXPORT_API void ewk_context_preferred_languages_set(Eina_List* languages);

/**
 * @brief Requests for deleting all web indexed databases.
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 *
 * @return @c EINA_TRUE on success, otherwise @c EINA_FALSE
 */
EXPORT_API Eina_Bool ewk_context_web_indexed_database_delete_all(Ewk_Context* context);

/**
 * @brief Deletes all web storage.
 *
 * @since_tizen 2.4
 *
 * @param[in] context Context object
 *
 * @return @c EINA_TRUE on success, otherwise @c EINA_FALSE
 */
EXPORT_API Eina_Bool ewk_context_web_storage_delete_all(Ewk_Context* context);

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_context_h
