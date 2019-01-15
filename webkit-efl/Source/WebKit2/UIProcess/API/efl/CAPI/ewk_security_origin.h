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
 * @file    ewk_security_origin.h
 * @brief   This file describes the Ewk Security API.
 */

#ifndef ewk_security_origin_h
#define ewk_security_origin_h

#include <stdint.h>
#include <tizen.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Ewk_Security_Origin Ewk_Security_Origin;

/**
 * @brief Requests for getting host of security origin.
 *
 * @since_tizen 2.4
 *
 * @param[in] origin Security origin
 *
 * @return host of security origin
 */
EXPORT_API const char* ewk_security_origin_host_get(const Ewk_Security_Origin* origin);

/**
 * @brief Requests for getting host of security origin.
 *
 * @since_tizen 2.4
 *
 * @param[in] origin Security origin
 *
 * @return host of security origin
 */
EXPORT_API const char* ewk_security_origin_protocol_get(const Ewk_Security_Origin* origin);

#ifdef __cplusplus
}
#endif
#endif // ewk_security_origin_h
