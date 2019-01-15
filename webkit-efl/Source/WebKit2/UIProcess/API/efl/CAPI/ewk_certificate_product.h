/*
   Copyright (C) 2011 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef ewk_certificate_product_h
#define ewk_certificate_product_h

#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * @brief Creates a type name for #Ewk_Certificate_Policy_Decision
 * @since_tizen 2.3
 */
typedef struct _Ewk_Certificate_Policy_Decision Ewk_Certificate_Policy_Decision;

/**
 * @brief Set the variable to allow the site access about certificate error.
 *
 * @since_tizen 2.3
 *
 * @param[in] certificate_policy_decision certificate information data
 * @param[in] allowed decided permission value from user
 */
EXPORT_API void ewk_certificate_policy_decision_allowed_set(Ewk_Certificate_Policy_Decision* certificate_policy_decision, Eina_Bool allowed);

/**
 * @brief Suspend the operation for certificate error policy decision.
 *
 * @details This suspends the operation for certificate error policy decision when the signal for policy is emitted.\n
 * This is very usefull to decide the policy from the additional UI operation like the popup.
 *
 * @since_tizen 2.3
 *
 * @param[in] certificate_policy_decision certificate information data
 */
EXPORT_API void ewk_certificate_policy_decision_suspend(Ewk_Certificate_Policy_Decision* certificate_policy_decision);

/**
 * @brief Get the variable url to check the site's url data about certificate error.
 *
 * @since_tizen 2.3
 *
 * @param[in] certificate_policy_decision certificate information data
 *
 * @return @c url string on success or empty string on failure
 */
EXPORT_API const char* ewk_certificate_policy_decision_url_get(Ewk_Certificate_Policy_Decision* certificate_policy_decision);

/**
 * @brief Get the variable certificate pem data to check the information about certificate error.
 *
 * @since_tizen 2.3
 *
 * @param[in] certificate_policy_decision certificate information data
 *
 * @return @c certificate pem string on success or empty string on failure
 */
EXPORT_API const char* ewk_certificate_policy_decision_certificate_pem_get(Ewk_Certificate_Policy_Decision* certificate_policy_decision);

/**
* @}
*/

#ifdef __cplusplus
}
#endif
#endif
