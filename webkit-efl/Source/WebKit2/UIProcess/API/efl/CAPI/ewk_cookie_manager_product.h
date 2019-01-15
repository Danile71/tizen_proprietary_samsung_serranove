/*
 * Copyright (C) 2015 Samsung Electronics
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

/**
 * @file    ewk_cookie_manager_product.h
 * @brief   This file describes the Ewk Cookie Manager API.
 */

#ifndef ewk_cookie_manager_product_h
#define ewk_cookie_manager_product_h

#include <tizen.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

#ifndef ewk_cookie_manager_type
#define ewk_cookie_manager_type
/**
 * @brief The structure type that creates a type name for #Ewk_Cookie_Manager.
 * @since_tizen 2.4
 */
typedef struct Ewk_Cookie_Manager Ewk_Cookie_Manager;
#endif

/**
 * @brief Remove all cookies of @a manager for the given @a hostname.
 *
 * @since_tizen 2.4
 *
 * @param[in] manager The cookie manager to update.
 * @param[in] hostname A host name.
 */
EXPORT_API void ewk_cookie_manager_hostname_cookies_clear(Ewk_Cookie_Manager *manager, const char *hostname);

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_cookie_manager_product_h
