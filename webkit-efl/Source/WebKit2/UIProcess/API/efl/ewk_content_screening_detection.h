/*
   Copyright (C) 2013 Samsung Electronics

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

#ifndef ewk_content_screening_detection_h
#define ewk_content_screening_detection_h

#include <Eina.h>
#include "ewk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Ewk_Content_Screening_Detection Ewk_Content_Screening_Detection;

/**
 * Set the variable to allow the release confirm about malware error.
 *
 * @param content_screening_detection malware information data
 *
 * @param confirmed decided permission value from user
 */
EAPI void ewk_content_screening_detection_confirmed_set(Ewk_Content_Screening_Detection* content_screening_detection, Eina_Bool confirmed);

/**
 * Suspend the operation for content screening detection.
 *
 * This suspends the operation for content screening detection when the signal is emitted.
 * This is very usefull to decide the policy from the additional UI operation like the popup.
 *
 * @param content_screening_detection malware information data
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EAPI void ewk_content_screening_detection_suspend(Ewk_Content_Screening_Detection* content_screening_detection);

/**
 * @brief Get the level to check the error cause about malware error.
 *
 * @since_tizen 2.3
 *
 * @param[in] content_screening_detection malware information data
 *
 * @return level
 */
EAPI int ewk_content_screening_detection_level_get(Ewk_Content_Screening_Detection* content_screening_detection);

/**
 * @brief Get the name to check the error cause about malware error.
 *
 * @since_tizen 2.3
 *
 * @param[in] content_screening_detection malware information data
 *
 * @return name
 */
EAPI const char* ewk_content_screening_detection_name_get(Ewk_Content_Screening_Detection* content_screening_detection);

/**
 * @brief Get the url to check the error cause about malware error.
 *
 * @since_tizen 2.3
 *
 * @param[in] content_screening_detection malware information data
 *
 * @return url
 */
EAPI const char* ewk_content_screening_detection_url_get(Ewk_Content_Screening_Detection* content_screening_detection);

#ifdef __cplusplus
}
#endif
#endif
