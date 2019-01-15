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

#ifndef ewk_notification_product_h
#define ewk_notification_product_h

#include "ewk_context_product.h"
#include "ewk_security_origin_product.h"
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @internal
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * @brief Creates a type name for #Ewk_Notification
 * @since_tizen 2.3
 */
typedef struct _Ewk_Notification Ewk_Notification;

/**
 * @brief Creates a type name for #Ewk_Notification_Permission_Request
 * @since_tizen 2.3
 */
typedef struct _Ewk_Notification_Permission_Request Ewk_Notification_Permission_Request;

/**
 * @brief Requests for getting body of notification.
 *
 * @since_tizen 2.3
 *
 * @param[in] ewk_notification pointer of notificaion data
 *
 * @return body of notification
 */
EXPORT_API const char* ewk_notification_body_get(const Ewk_Notification* ewk_notification);

/**
 * @brief Notify that notification is clicked.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] notification_id identifier of notification
 */
EXPORT_API void ewk_notification_clicked(Ewk_Context* context, uint64_t notification_id);

/**
 * @brief Requests for getting icon url of notification.
 *
 * @since_tizen 2.3
 *
 * @param[in] ewk_notification pointer of notification data
 *
 * @return icon url of notification
 */
EXPORT_API const char* ewk_notification_icon_url_get(const Ewk_Notification* ewk_notification);

/**
 * @brief Requests for getting id of notification.
 *
 * @since_tizen 2.3
 *
 * @param[in] ewk_notification pointer of notification data
 *
 * @return id of notification
 */
EXPORT_API uint64_t ewk_notification_id_get(const Ewk_Notification* ewk_notification);

/**
 * @brief Requests for getting origin of notification permission request.
 *
 * @since_tizen 2.3
 *
 * @param[in] request #Ewk_Notification_Permission_Request object to get origin for notification permission request
 *
 * @return security origin of notification permission request
 */
EXPORT_API const Ewk_Security_Origin* ewk_notification_permission_request_origin_get(const Ewk_Notification_Permission_Request* request);

/**
 * @brief Sets permission of notification.
 *
 * @since_tizen 2.3
 *
 * @param[in] request #Ewk_Notification_Permission_Request object to allow/deny notification permission\n
 *        request is freed in this function.
 * @param[in] allowed @c EINA_TRUE if permission is allowed, @c EINA_FALSE if permission is denied
 */
EINA_DEPRECATED EXPORT_API void ewk_notification_permission_request_set(Ewk_Notification_Permission_Request* request, Eina_Bool allowed);

/**
 * @brief Suspend the operation for permission request.
 *
 * @since_tizen 2.3
 *
 * @remarks This suspends the operation for permission request.\n
 * This is very useful to decide the policy from the additional UI operation like the popup.
 *
 * @param[in] request #Ewk_Notification_Permission_Request object to suspend notification permission request
 */
EINA_DEPRECATED EXPORT_API void ewk_notification_permission_request_suspend(Ewk_Notification_Permission_Request* request);

/**
 * @brief Reply the result about notification permission.
 *
 * @since_tizen 2.3
 *
 * @param[in] request #Ewk_Notification_Permission_Request object to get the infomation about notification permission request
 * @param[in] allow result about notification permission
 */
EXPORT_API void ewk_notification_permission_reply(Ewk_Notification_Permission_Request* request, Eina_Bool allow);

/**
 * @brief Notify that notification policies are removed.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param[in] origins list of security origins(made by UAs)
 */
EXPORT_API void ewk_notification_policies_removed(Ewk_Context* context, Eina_List* origins);

/**
 * @brief Requests for getting security origin of notification.
 *
 * @since_tizen 2.3
 *
 * @param[in] ewk_notification pointer of notification data
 *
 * @return security origin of notification
 */
EXPORT_API const Ewk_Security_Origin* ewk_notification_security_origin_get(const Ewk_Notification* ewk_notification);

/**
 * @brief Notify that notification is showed.
 *
 * @since_tizen 2.3
 *
 * @param[in] context context object
 * @param notification_id identifier of notification
 */
EXPORT_API void ewk_notification_showed(Ewk_Context* context, uint64_t notification_id);

/**
 * @brief Requests for getting title of notification.
 *
 * @since_tizen 2.3
 *
 * @param[in] ewk_notification pointer of notification data
 *
 * @return title of notification
 */
EXPORT_API const char* ewk_notification_title_get(const Ewk_Notification* ewk_notification);

/**
* @}
*/

#ifdef __cplusplus
}
#endif
#endif // ewk_notification_product_h
