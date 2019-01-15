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

#include "config.h"
#include "ewk_error.h"

#include "ErrorsEfl.h"
#include "WKString.h"
#include "WKURL.h"
#include "ewk_error_private.h"
#include <WKAPICast.h>
#include <wtf/text/CString.h>

#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
#include <libintl.h>
#endif

using namespace WebCore;
using namespace WebKit;

Ewk_Error::Ewk_Error(WKErrorRef errorRef)
    : wkError(errorRef)
    , url(AdoptWK, WKErrorCopyFailingURL(errorRef))
#if PLATFORM(TIZEN)
    , domain(AdoptWK, WKErrorCopyDomain(errorRef))
#endif
{
    setDescriptionFromErrorCode();
}

void Ewk_Error::setDescriptionFromErrorCode()
{
    int errorCode = ewk_error_code_get(this);
    String descriptionString;

    switch (errorCode) {
    case EWK_ERROR_CODE_CANCELED:
        descriptionString = String::fromUTF8("User canceled");
        break;
    case EWK_ERROR_CODE_CANT_SUPPORT_MIMETYPE:
        descriptionString = String::fromUTF8("Can't show page for this MIME Type");
        break;
    case EWK_ERROR_CODE_FAILED_FILE_IO:
        descriptionString = String::fromUTF8("Error regarding to file io");
        break;
    case EWK_ERROR_CODE_CANT_CONNECT:
        descriptionString = String::fromUTF8("Cannot connect to Network");
        break;
    case EWK_ERROR_CODE_CANT_LOOKUP_HOST:
        descriptionString = String::fromUTF8("Fail to look up host from DNS");
        break;
    case EWK_ERROR_CODE_FAILED_TLS_HANDSHAKE:
        descriptionString = String::fromUTF8("Fail to SSL/TLS handshake");
        break;
    case EWK_ERROR_CODE_INVALID_CERTIFICATE:
        descriptionString = String::fromUTF8("Received certificate is invalid");
        break;
    case EWK_ERROR_CODE_REQUEST_TIMEOUT:
        descriptionString = String::fromUTF8("Connection timeout");
        break;
    case EWK_ERROR_CODE_TOO_MANY_REDIRECTS:
        descriptionString = String::fromUTF8("Too many redirects");
        break;
    case EWK_ERROR_CODE_TOO_MANY_REQUESTS:
        descriptionString = String::fromUTF8("Too many requests during this load");
        break;
    case EWK_ERROR_CODE_BAD_URL:
        descriptionString = String::fromUTF8("Malformed url");
        break;
    case EWK_ERROR_CODE_UNSUPPORTED_SCHEME:
        descriptionString = String::fromUTF8("Unsupported scheme");
        break;
    case EWK_ERROR_CODE_AUTHENTICATION:
        descriptionString = String::fromUTF8("User authentication failed on server");
        break;
    case EWK_ERROR_CODE_INTERNAL_SERVER:
        descriptionString = String::fromUTF8("Web server has internal server error");
        break;
    case EWK_ERROR_CODE_UNKNOWN:
    default:
        descriptionString = String::fromUTF8("Unknown");
        break;
    }

    description = descriptionString.utf8().data();
}

#define EWK_ERROR_WK_GET_OR_RETURN(error, wkError_, ...)    \
    if (!(error)) {                                           \
        EINA_LOG_ERR("error is NULL.");                      \
        return __VA_ARGS__;                                    \
    }                                                          \
    if (!(error)->wkError) {                                 \
        EINA_LOG_ERR("error->wkError is NULL.");            \
        return __VA_ARGS__;                                    \
    }                                                          \
    WKErrorRef wkError_ = (error)->wkError.get()

const char* ewk_error_url_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);

    return error->url;
}

static int convertErrorCode(const Ewk_Error* error)
{
    EWK_ERROR_WK_GET_OR_RETURN(error, wkError, EWK_ERROR_CODE_UNKNOWN);

    int errorCode = WKErrorGetErrorCode(wkError);

    if (!strcmp(error->domain, WebError::webKitErrorDomain().utf8().data())) {
        switch (errorCode) {
        /* EWK_ERROR_CODE_CANCELED */
        case kWKErrorCodePlugInCancelledConnection:
            return EWK_ERROR_CODE_CANCELED;

        /* EWK_ERROR_CODE_CANT_SUPPORT_MIMETYPE */
        case kWKErrorCodeCannotShowMIMEType:
            return EWK_ERROR_CODE_CANT_SUPPORT_MIMETYPE;

        /* EWK_ERROR_CODE_UNKNOWN */
        case kWKErrorCodeCannotShowURL:
        case kWKErrorCodeFrameLoadInterruptedByPolicyChange:
        case kWKErrorCodeCannotUseRestrictedPort:
        case kWKErrorCodeCannotFindPlugIn:
        case kWKErrorCodeCannotLoadPlugIn:
        case kWKErrorCodeJavaUnavailable:
        case kWKErrorCodePlugInWillHandleLoad:
        case kWKErrorCodeInsecurePlugInVersion:
        default:
            return EWK_ERROR_CODE_UNKNOWN;
        }
    }

    if (!strcmp(error->domain, g_quark_to_string(SOUP_HTTP_ERROR))) {
        switch (errorCode) {
        /* EWK_ERROR_CODE_CANCELED */
        case SOUP_STATUS_CANCELLED:
            return EWK_ERROR_CODE_CANCELED;

        /* EWK_ERROR_CODE_CANT_SUPPORT_MIMETYPE */
        case SOUP_STATUS_UNSUPPORTED_MEDIA_TYPE:
            return EWK_ERROR_CODE_CANT_SUPPORT_MIMETYPE;

        /* EWK_ERROR_CODE_FAILED_FILE_IO */
        case SOUP_STATUS_IO_ERROR:
            return EWK_ERROR_CODE_FAILED_FILE_IO;

        /* EWK_ERROR_CODE_CANT_CONNECT */
        case SOUP_STATUS_CANT_CONNECT:
        case SOUP_STATUS_CANT_CONNECT_PROXY:
            return EWK_ERROR_CODE_CANT_CONNECT;

        /* EWK_ERROR_CODE_CANT_LOOKUP_HOST */
        case SOUP_STATUS_CANT_RESOLVE:
        case SOUP_STATUS_CANT_RESOLVE_PROXY:
            return EWK_ERROR_CODE_CANT_LOOKUP_HOST;

        /* EWK_ERROR_CODE_FAILED_TLS_HANDSHAKE */
        case SOUP_STATUS_SSL_FAILED:
        case SOUP_STATUS_TLS_FAILED:
            return EWK_ERROR_CODE_FAILED_TLS_HANDSHAKE;

        /* EWK_ERROR_CODE_REQUEST_TIMEOUT */
        case SOUP_STATUS_REQUEST_TIMEOUT:
        case SOUP_STATUS_GATEWAY_TIMEOUT:
            return EWK_ERROR_CODE_REQUEST_TIMEOUT;

        /* EWK_ERROR_CODE_TOO_MANY_REDIRECTS */
        case SOUP_STATUS_TOO_MANY_REDIRECTS:
            return EWK_ERROR_CODE_TOO_MANY_REDIRECTS;

        /* EWK_ERROR_CODE_BAD_URL */
        case SOUP_STATUS_MALFORMED:
        case SOUP_STATUS_BAD_REQUEST:
        case SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE:
        //case SOUP_STATUS_INVALID_RANGE:   /* SOUP_STATUS_INVALID_RANGE = SOUP_STATUS_REQUESTED_RANGE_NOT_SATISFIABLE */
        case SOUP_STATUS_EXPECTATION_FAILED:
        case SOUP_STATUS_UNPROCESSABLE_ENTITY:
            return EWK_ERROR_CODE_BAD_URL;

        /* EWK_ERROR_CODE_AUTHENTICATION */
        case SOUP_STATUS_UNAUTHORIZED:
        case SOUP_STATUS_PROXY_AUTHENTICATION_REQUIRED:
        //case SOUP_STATUS_PROXY_UNAUTHORIZED:  /* SOUP_STATUS_PROXY_UNAUTHORIZED = SOUP_STATUS_PROXY_AUTHENTICATION_REQUIRED */
            return EWK_ERROR_CODE_AUTHENTICATION;

        /* EWK_ERROR_CODE_INTERNAL_SERVER */
        case SOUP_STATUS_FORBIDDEN:
        case SOUP_STATUS_NOT_FOUND:
        case SOUP_STATUS_METHOD_NOT_ALLOWED:
        case SOUP_STATUS_NOT_ACCEPTABLE:
        case SOUP_STATUS_CONFLICT:
        case SOUP_STATUS_GONE:
        case SOUP_STATUS_PRECONDITION_FAILED:
        case SOUP_STATUS_REQUEST_ENTITY_TOO_LARGE:
        case SOUP_STATUS_REQUEST_URI_TOO_LONG:
        case SOUP_STATUS_LOCKED:
        case SOUP_STATUS_FAILED_DEPENDENCY:
        case SOUP_STATUS_INTERNAL_SERVER_ERROR:
        case SOUP_STATUS_NOT_IMPLEMENTED:
        case SOUP_STATUS_BAD_GATEWAY:
        case SOUP_STATUS_SERVICE_UNAVAILABLE:
        case SOUP_STATUS_HTTP_VERSION_NOT_SUPPORTED:
        case SOUP_STATUS_INSUFFICIENT_STORAGE:
        case SOUP_STATUS_NOT_EXTENDED:
            return EWK_ERROR_CODE_INTERNAL_SERVER;

        /* EWK_ERROR_CODE_UNKNOWN */
        case SOUP_STATUS_NONE:
        case SOUP_STATUS_TRY_AGAIN:
        case SOUP_STATUS_PAYMENT_REQUIRED:
        case SOUP_STATUS_LENGTH_REQUIRED:
        default:
            return EWK_ERROR_CODE_UNKNOWN;
        }
    }

    if (!strcmp(error->domain, g_quark_to_string(G_IO_ERROR)))
        return EWK_ERROR_CODE_FAILED_FILE_IO;

    return EWK_ERROR_CODE_UNKNOWN;
}

int ewk_error_code_get(const Ewk_Error* error)
{
    return convertErrorCode(error);
}

int ewk_error_extra_code_get(const Ewk_Error *error)
{
    EWK_ERROR_WK_GET_OR_RETURN(error, wkError, 0);

    return WKErrorGetErrorCode(wkError);
}

const char* ewk_error_description_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);

    return error->description;
}

Eina_Bool ewk_error_cancellation_get(const Ewk_Error* error)
{
    EWK_ERROR_WK_GET_OR_RETURN(error, wkError, false);

    return toImpl(wkError)->platformError().isCancellation();
}

const char* ewk_error_domain_get(const Ewk_Error* error)
{
#if PLATFORM(TIZEN)
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);

    return error->domain;
#else
    UNUSED_PARAM(error);
    return 0;
#endif
}

#if PLATFORM(TIZEN)
void ewk_error_load_error_page(Ewk_Error* error, WKPageRef page)
{
    EWK_ERROR_WK_GET_OR_RETURN(error, wkError);

    // if error domain is null, it is not a error.
    EINA_SAFETY_ON_NULL_RETURN(error->domain);

    int errorCode = WKErrorGetErrorCode(wkError);

#if ENABLE(TIZEN_DLOG_SUPPORT)
    WKRetainPtr<WKStringRef> errorDomain(AdoptWK, WKErrorCopyDomain(wkError));
    TIZEN_SECURE_LOGI("[Loader] error->domain[%s], errorCode[%d]", toImpl(errorDomain.get())->string().utf8().data(), errorCode);
#endif

    // error_code == 0  is ok, error_code == -999 is request cancelled
    if (!strcmp(error->domain, g_quark_to_string(SOUP_HTTP_ERROR))
        && (!errorCode || errorCode == SOUP_STATUS_CANCELLED))
            return;

    // webDEV error code but not a error
    if (!strcmp(error->domain, WebError::webKitErrorDomain().utf8().data())) {
        if (errorCode == kWKErrorCodeCannotShowMIMEType || errorCode == kWKErrorCodeFrameLoadInterruptedByPolicyChange || errorCode == kWKErrorCodePlugInWillHandleLoad)
            return;
    }

    // make Error Page Source
    String errorPageFile = WEBKIT_HTML_DIR"/errorPage.html";
    long long fileSize = 0;
    if (!getFileSize(errorPageFile, fileSize) || fileSize <= 0)
        return;

    PlatformFileHandle errorHtmlFile = openFile(errorPageFile, OpenForRead);

    OwnArrayPtr<char> arrayHtmlSource = adoptArrayPtr(new char[fileSize]);
    if (readFromFile(errorHtmlFile, arrayHtmlSource.get(), fileSize) != fileSize) {
        closeFile(errorHtmlFile);
        return;
    }
    closeFile(errorHtmlFile);

    String htmlSource = String::fromUTF8(arrayHtmlSource.get(), fileSize);

    gboolean isWebKitCannotShowUrlError = !strcmp(error->domain, WebError::webKitErrorDomain().utf8().data()) && (errorCode == kWKErrorCodeCannotShowURL);

    String urlLink = String::format("<a style=word-break:break-all href=%s>%s</a>", static_cast<const char*>(error->url), static_cast<const char*>(error->url));

    String errorPageTitleString = errorPageTitle();
    String errorMessageTitleString = errorMessageTitle();
    String errorMessage;

    switch (errorCode) {
        case SOUP_STATUS_CANT_RESOLVE:
        case SOUP_STATUS_CANT_RESOLVE_PROXY:
            errorMessage = errorMessageDNSError();
            break;
        case SOUP_STATUS_CANT_CONNECT:
        case SOUP_STATUS_CANT_CONNECT_PROXY:
            errorMessage = errorMessageServerNotResponding();
            break;
        case SOUP_STATUS_SSL_FAILED:
            errorMessage = errorMessageSSLFailedError();
            break;
        default:
            errorMessage = errorMessageDefaultError();
    }

    if (isWebKitCannotShowUrlError)
        errorMessage.replace("%s", "host");
    else
       errorMessage = errorMessage.replace("%s", urlLink);

    htmlSource = htmlSource.replace("%errorPageTitle%", errorPageTitleString);
    htmlSource = htmlSource.replace("%errorMessageTitle%", errorMessageTitleString);
    htmlSource = htmlSource.replace("%errorMessage%", errorMessage);

    WKRetainPtr<WKStringRef> htmlString(AdoptWK, WKStringCreateWithUTF8CString(htmlSource.utf8().data()));
    WKRetainPtr<WKURLRef> baseURL(AdoptWK, WKURLCreateWithUTF8CString(error->url));
    WKRetainPtr<WKURLRef> unreachableURL(AdoptWK, WKURLCreateWithUTF8CString(error->url));
    WKPageLoadAlternateHTMLString(page, htmlString.get(), baseURL.get(), unreachableURL.get());
}

#endif
