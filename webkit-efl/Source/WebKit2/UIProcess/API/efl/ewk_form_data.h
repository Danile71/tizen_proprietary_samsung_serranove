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

#ifndef ewk_form_data_h
#define ewk_form_data_h

#include <tizen.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _Ewk_Form_Data Ewk_Form_Data;

/**
 * Returns url from Form Data object.
 *
 * @param form_data Form Data object received from "form,submit" evas object smart callback
 *
 * @return @c url string on success or empty string on failure
 */
EXPORT_API const char* ewk_form_data_url_get(Ewk_Form_Data* form_data);

/**
 * Returns values from Form Data object.
 *
 * @param form_data Form Data object received from "form,submit" evas object smart callback
 *
 * @return @c host string on success or empty string on failure
 */
EXPORT_API Eina_Hash* ewk_form_data_values_get(Ewk_Form_Data* form_data);

#ifdef __cplusplus
}
#endif
#endif // ewk_form_data_h
