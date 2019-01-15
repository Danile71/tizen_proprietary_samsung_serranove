/*
 * Copyright (C) 2013 Samsung Electronics
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
 * @file    ewk_autofill_profile_product.h
 * @brief   Describes the Ewk autofill profile APIs.
 */

#ifndef ewk_autofill_profile_product_h
#define ewk_autofill_profile_product_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

#if 0
/**
 * @brief Enumeration for autofill profile data type.
 * @since_tizen 2.3
 */
enum Ewk_Autofill_Profile_Data_Type {
    EWK_PROFILE_ID, /**< User  id for autofile profile */
    EWK_PROFILE_NAME,   /**< User  name for autofile profile */
    EWK_PROFILE_COMPANY,    /**< User  company for autofile profile */
    EWK_PROFILE_ADDRESS1,   /**< User  address1 for autofile profile */
    EWK_PROFILE_ADDRESS2,   /**< User  address2 for autofile profile */
    EWK_PROFILE_CITY_TOWN,  /**< User city town for autofile profile */
    EWK_PROFILE_STATE_PROVINCE_REGION,  /**< User state province region for autofile profile */
    EWK_PROFILE_ZIPCODE,    /**< User zipcode for autofile profile */
    EWK_PROFILE_COUNTRY,    /**< User country for autofile profile */
    EWK_PROFILE_PHONE,  /**< User phone for autofile profile */
    EWK_PROFILE_EMAIL,  /**< User email for autofile profile */
    EWK_MAX_AUTOFILL    /**< End of autofile profile data type */
};
#endif

/**
 * @brief Creates a type name for #Ewk_Autofill_Profile_Data_Type
 * @since_tizen 2.3
 */
typedef enum Ewk_Autofill_Profile_Data_Type Ewk_Autofill_Profile_Data_Type;

/**
 * @brief Creates a type name for #Ewk_Autofill_Profile
 * @since_tizen 2.3
 */
typedef struct _Ewk_Autofill_Profile Ewk_Autofill_Profile;

/**
 * @brief Creates a new profile
 *
 * @since_tizen 2.3
 *
 * @remarks The created profile must be deleted by ewk_autofill_profile_delete
 *
 * @return @c Ewk_Autofill_Profile if new profile is successfully created, @c NULL otherwise
 * @see ewk_autofill_profile_data_set
 * @see ewk_autofill_profile_delete
 */
EXPORT_API Ewk_Autofill_Profile* ewk_autofill_profile_new(void);

/**
 * @brief Deletes a given profile
 *
 * @since_tizen 2.3
 *
 * @remarks The API will delete the a particular profile only from the memory.\n
 * To remove the profile permenantly use ewk_context_form_autofill_profile_remove
 *
 * @param[in] profile name
 *
 * @see ewk_autofill_profile_new
 * @see ewk_context_form_autofill_profile_get
 * @see ewk_context_form_autofill_profile_remove
 */
EXPORT_API void ewk_autofill_profile_delete(Ewk_Autofill_Profile* profile);

/**
 * @brief Sets the data in the profile created by ewk_autofill_profile_new
 *
 * @since_tizen 2.3
 *
 * @remarks The data set by this function is set locally. To save it to database use ewk_context_form_autofill_profile_add
 *
 * @param[in] profile contains the profile data
 * @param[in] name type of attribute to be set
 * @param[in] value value of the attribute
 *
 * @see ewk_autofill_profile_data_get
 * @see Ewk_Autofill_Profile_Data_Type
 * @see ewk_context_form_autofill_profile_add
 */
EXPORT_API void ewk_autofill_profile_data_set(Ewk_Autofill_Profile* profile, Ewk_Autofill_Profile_Data_Type name, const char* value);

/**
 * @brief Gets the id attribute value from a given profile
 *
 * @since_tizen 2.3
 *
 * @remarks The profile obtained from ewk_context_form_autofill_profile_get will be used to get the profileid
 *
 * @param[in] profile name of profile
 *
 * @return @c Value of attribute (unsigned), @c 0 otherwise
 * @see ewk_autofill_profile_new
 * @see ewk_context_form_autofill_profile_get
 * @see ewk_context_form_autofill_profile_get_all
 */
EXPORT_API unsigned ewk_autofill_profile_id_get(Ewk_Autofill_Profile* profile);

/**
 * @brief Gets the attribute value from a given profile
 *
 * @since_tizen 2.3
 *
 * @remarks The profile obtained from ewk_context_form_autofill_profile_get will be used to get the data
 *
 * @param[in] profile name of profile
 * @param[in] name name of attribute
 *
 * @return @c Value of attribute (char*), @c NULL otherwise
 * @see ewk_autofill_profile_new
 * @see ewk_context_form_autofill_profile_get
 * @see ewk_context_form_autofill_profile_get_all
 */
EXPORT_API const char* ewk_autofill_profile_data_get(Ewk_Autofill_Profile* profile, Ewk_Autofill_Profile_Data_Type name);

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_autofill_profile_product_h

