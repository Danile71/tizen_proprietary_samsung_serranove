/*
   Copyright (C) 2012 Samsung Electronics

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

#ifndef ewk_policy_decision_product_h
#define ewk_policy_decision_product_h

#include "ewk_frame_product.h"
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

#ifndef ewk_policy_decision_type
#define ewk_policy_decision_type
/**
 * @brief The structure type that creates a type name for #Ewk_Policy_Decision.
 * @since_tizen 2.3
 */
typedef struct _Ewk_Policy_Decision Ewk_Policy_Decision;
#endif

/**
 * @internal
 * @brief Returns user id for Authorization from Policy Decision object.
 *
 * @since_tizen 2.3
 *
 * @param[in] policy_decision policy decision object
 *
 * @return @c user id string on success or empty string on failure
 */
EXPORT_API const char* ewk_policy_decision_userid_get(Ewk_Policy_Decision* policy_decision);

/**
 * @internal
 * @brief Returns password for Authorization from Policy Decision object.
 *
 * @since_tizen 2.3
 *
 * @param[in] policy_decision policy decision object
 *
 * @return @c password string on success or empty string on failure
 */
EXPORT_API const char* ewk_policy_decision_password_get(Ewk_Policy_Decision* policy_decision);

/**
 * @internal
 * @brief Suspend the operation for policy decision.
 *
 * @since_tizen 2.3
 *
 * @remarks This suspends the operation for policy decision when the signal for policy is emitted.\n
 * This is very usefull to decide the policy from the additional UI operation like the popup.
 *
 * @param[in] policy_decision policy decsision object
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_policy_decision_suspend(Ewk_Policy_Decision* policy_decision);

/**
 * @brief Gets the frame reference from Policy Decision object.
 *
 * @since_tizen 2.3
 *
 * @param[in] policy_decision policy decsision object
 *
 * @return frame reference on success, or NULL on failure
 */
EXPORT_API Ewk_Frame_Ref ewk_policy_decision_frame_get(Ewk_Policy_Decision* policy_decision);

/**
 * @brief Returns http body from Policy Decision object.
 *
 * @since_tizen 2.4
 *
 * @param[in] policy_decision policy decsision object
 *
 * @return @c http body string on success or empty string on failure
 */
EXPORT_API const char* ewk_policy_decision_http_body_get(Ewk_Policy_Decision* policy_decision);

/**
* @}
*/

#ifdef __cplusplus
}
#endif
#endif // ewk_policy_decision_product_h
