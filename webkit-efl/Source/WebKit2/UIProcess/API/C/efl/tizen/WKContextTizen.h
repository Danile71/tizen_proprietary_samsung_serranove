/*
   Copyright (C) 2011 Samsung Electronics

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

#ifndef WKContextTizen_h
#define WKContextTizen_h

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT void WKContextSetProxy(WKContextRef context, WKURLRef proxyAddress);

// #if ENABLE(TIZEN_APPLICATION_CACHE)
WK_EXPORT void WKContextSetApplicationCacheDirectory(WKContextRef context, WKStringRef directory);
// #endif

// #if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
WK_EXPORT void WKContextSetSoupDataDirectory(WKContextRef contextRef, WKStringRef soupDataDirectory);
// #endif

// #if ENABLE(TIZEN_CERTIFICATE_HANDLING)
WK_EXPORT void WKContextSetCertificateFile(WKContextRef contextRef, WKStringRef certificateFile);
//#endif

// #if ENABLE(TIZEN_RESET_PATH)
WK_EXPORT void WKContextResetStoragePath(WKContextRef contextRef);
// #endif

//#if ENABLE(TIZEN_EXTENSIBLE_API)
WK_EXPORT void WKContextSetTizenExtensibleAPI(WKContextRef context, WKTizenExtensibleAPI extensibleAPI, bool enable);
WK_EXPORT bool WKContextGetTizenExtensibleAPI(WKContextRef contextRef, WKTizenExtensibleAPI extensibleAPI);
//#endif
#ifdef __cplusplus
}
#endif

#endif /* WKContextTizen_h */
