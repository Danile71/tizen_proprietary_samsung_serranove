/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file    ewk_error.h
 * @brief   This file describes the Ewk Web Error API.
 */

#ifndef ewk_error_h
#define ewk_error_h

#include <tizen.h>
#include <Eina.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup WEBVIEW
 * @{
 */

#ifndef ewk_error_type
#define ewk_error_type
/**
 * @brief The structure type that creates a type name for #Ewk_Error.
 * @since_tizen 2.3
 */
typedef struct Ewk_Error Ewk_Error;
#endif

/**
 * \enum   Ewk_Error_Code
 * @brief  Enumeration that provides an option to error codes.
 * @since_tizen 2.3
 */
typedef enum {
    EWK_ERROR_CODE_UNKNOWN,    /**< Unknown */
    EWK_ERROR_CODE_CANCELED,   /**< User canceled */
    EWK_ERROR_CODE_CANT_SUPPORT_MIMETYPE,  /**< Can't show page for this MIME Type */
    EWK_ERROR_CODE_FAILED_FILE_IO, /**< Error regarding to file io */
    EWK_ERROR_CODE_CANT_CONNECT,   /**< Cannot connect to Network */
    EWK_ERROR_CODE_CANT_LOOKUP_HOST,   /**< Fail to look up host from DNS */
    EWK_ERROR_CODE_FAILED_TLS_HANDSHAKE,   /**< Fail to SSL/TLS handshake */
    EWK_ERROR_CODE_INVALID_CERTIFICATE,    /**< Received certificate is invalid */
    EWK_ERROR_CODE_REQUEST_TIMEOUT,    /**< Connection timeout */
    EWK_ERROR_CODE_TOO_MANY_REDIRECTS, /**< Too many redirects */
    EWK_ERROR_CODE_TOO_MANY_REQUESTS,  /**< Too many requests during this load */
    EWK_ERROR_CODE_BAD_URL,    /**< Malformed url */
    EWK_ERROR_CODE_UNSUPPORTED_SCHEME, /**< Unsupported scheme */
    EWK_ERROR_CODE_AUTHENTICATION, /**< User authentication failed on server */
    EWK_ERROR_CODE_INTERNAL_SERVER /**< Web server has internal server error */
} Ewk_Error_Code;

/**
 * @brief Query failing URL for this error.
 *
 * @details URL that failed loading.
 *
 * @since_tizen 2.3
 *
 * @param[in] error The error object to query
 *
 * @return The URL pointer, that may be @c NULL. This pointer is\n
 *         guaranteed to be eina_stringshare, so whenever possible\n
 *         save yourself some cpu cycles and use\n
 *         eina_stringshare_ref() instead of eina_stringshare_add() or\n
 *         strdup()
 */
EXPORT_API const char* ewk_error_url_get(const Ewk_Error* error);

/**
 * @brief Query the error code.
 *
 * @since_tizen 2.3
 *
 * @param[in] error The error object to query
 *
 * @return The error code #Ewk_Error_Code
 */
EXPORT_API int ewk_error_code_get(const Ewk_Error* error);

/**
 * @brief Query description for this error.
 *
 * @since_tizen 2.3
 *
 * @param[in] error The error object to query
 *
 * @return The description pointer, that may be @c NULL. This pointer is\n
 *         guaranteed to be eina_stringshare, so whenever possible\n
 *         save yourself some cpu cycles and use\n
 *         eina_stringshare_ref() instead of eina_stringshare_add() or\n
 *         strdup()
 */
EXPORT_API const char* ewk_error_description_get(const Ewk_Error* error);

/**
 * @brief Query if error should be treated as a cancellation.
 *
 * @since_tizen 2.3
 *
 * @param[in] error The error object to query
 *
 * @return @c EINA_TRUE if this error should be treated as a cancellation\n
 *         otherwise @c EINA_FALSE
 */
EXPORT_API Eina_Bool ewk_error_cancellation_get(const Ewk_Error* error);

/**
* @}
*/

#ifdef __cplusplus
}
#endif

#endif // ewk_error_h
