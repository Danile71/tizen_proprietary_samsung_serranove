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

/**
 * @file    ewk_auth_challenge_internal.h
 * @brief   Describes the authentication challenge API.
 */

#ifndef ewk_auth_challenge_internal_h
#define ewk_auth_challenge_internal_h

#include <tizen.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for _Ewk_Auth_Challenge */
typedef struct _Ewk_Auth_Challenge Ewk_Auth_Challenge;

/**
 * Gets the realm string of authentication challenge received from "authentication,challenge" evas object smart callback.
 *
 * @param auth_challenge authentication challenge instance received from "authentication,challenge" evas object smart callback.
 * @return the realm of authentication challenge on success, @c 0 otherwise
 *
 */
EXPORT_API const char* ewk_auth_challenge_realm_get(Ewk_Auth_Challenge* auth_challenge);

/**
 * Gets the url string of authentication challenge received from "authentication,challenge" evas object smart callback.
 *
 * @param auth_challenge authentication challenge request instance received from "authentication,challenge" evas object smart callback.
 * @return the url of authentication challenge on success, @c 0 otherwise
 *
 */
EXPORT_API const char* ewk_auth_challenge_url_get(Ewk_Auth_Challenge* auth_challenge);

/**
 * Suspend the operation for authentication challenge.
 *
 * @param auth_challenge authentication challenge instance received from "authentication,challenge" evas object smart callback.
 *
 */
EXPORT_API void ewk_auth_challenge_suspend(Ewk_Auth_Challenge* auth_challenge);

/**
 *  If user select ok, send credential for authentication challenge from user input.
 *
 * @param auth_challenge authentication challenge instance received from "authentication,challenge" evas object smart callback.
 * @param user user id from user input.
 * @param password user password from user input.
 *
 */
EXPORT_API void ewk_auth_challenge_credential_use(Ewk_Auth_Challenge* auth_challenge, char* user, char* password);

/**
 *  If user select cancel, send cancellation notification for authentication challenge.
 *
 * @param auth_challenge authentication challenge instance received from "authentication,challenge" evas object smart callback.
 *
 */
EXPORT_API void ewk_auth_challenge_credential_cancel(Ewk_Auth_Challenge* auth_challenge);

#ifdef __cplusplus
}
#endif

#endif // ewk_auth_challenge_internal_h