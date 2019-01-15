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

#ifndef ewk_intercept_request_product_h
#define ewk_intercept_request_product_h


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @internal
 * @addtogroup WEBVIEW
 * @{
 */

/**
 * @brief Creates a type name for #Ewk_Intercept_Request
 * @since_tizen 2.3
 */
typedef struct _Ewk_Intercept_Request Ewk_Intercept_Request;

/**
 * @brief When application doesn't have a response for intercept request url, it\n
 * calls this api which notifies with FALSE value to the resource handle\n
 * indicates to continue with normal load procedure for this resource.
 *
 * @since_tizen 2.3
 *
 * @param[in] intercept_request intercept request instance received from "intercept,request" evas object smart callback
 *
 * @return @c EINA_TRUE on success or @c EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_intercept_request_ignore(Ewk_Intercept_Request* intercept_request);

/**
 * @brief Returns url from Intercept Request object.
 *
 * @since_tizen 2.3
 *
 * @param[in] intercept_request intercept request instance received from "intercept,request" evas object smart callback
 *
 * @return @c url string on success or empty string on failure
 */
EXPORT_API const char* ewk_intercept_request_url_get (Ewk_Intercept_Request* intercept_request);

/**
 * @brief Returns method for the url from Intercept Request object.
 *
 * @since_tizen 2.3
 *
 * @param[in] intercept_request intercept request instance received from "intercept,request" evas object smart callback
 *
 * @return @c method string on success or empty string on failure
 */
EXPORT_API const char* ewk_intercept_request_http_method_get (Ewk_Intercept_Request* intercept_request);

/**
 * @brief Returns request body for the url from Intercept Request object.
 *
 * @since_tizen 2.3
 *
 * @param[in] intercept_request intercept request instance received from "intercept,request" evas object smart callback
 *
 * @return @c body string on success or empty string on failure
 */
EXPORT_API const char* ewk_intercept_request_body_get (Ewk_Intercept_Request* intercept_request);

/**
 * @brief Returns request headers for the intercept Request.
 *
 * @since_tizen 2.3
 *
 * @param[in] intercept_request intercept request instance received from "intercept, request" evas object smart callback
 *
 * @return @c header string on success or empty string on failure
*/
EXPORT_API const Eina_Hash* ewk_intercept_request_headers_get (Ewk_Intercept_Request* intercept_request);

/**
 * @brief When application has the response headers, body for the intercept request url, it call this api to set those.
 *
 * @since_tizen 2.3
 *
 * @param[in] intercept_request intercept request instance received from "intercept,request" evas object smart callback
 * @param[in] headers response headers for the intercept request
 * @param[in] body response body for the intercept request
 * @param[in] length of response body
 *
 * @return @c EINA_TRUE on success or EINA_FALSE on failure
 */
EXPORT_API Eina_Bool ewk_intercept_request_response_set (Ewk_Intercept_Request* intercept_request, const char* headers, const char* body, int length);

/**
* @}
*/

#ifdef __cplusplus
}
#endif
#endif // ewk_intercept_request_product_h
