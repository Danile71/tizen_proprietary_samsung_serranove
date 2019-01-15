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

#include "config.h"
#include "WKContext.h"
#include "WKContextTizen.h"

#include "WebContext.h"
#include <wtf/text/WTFString.h>

using namespace WebKit;

void WKContextSetProxy(WKContextRef contextRef, WKURLRef proxyAddress)
{
    toImpl(contextRef)->setProxy(toWTFString(proxyAddress));
}

void WKContextSetApplicationCacheDirectory(WKContextRef contextRef, WKStringRef directory)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    toImpl(contextRef)->setApplicationCacheDirectory(toWTFString(directory));
#endif
}
void WKContextSetSoupDataDirectory(WKContextRef contextRef, WKStringRef soupDataDirectory)
{
#if ENABLE(TIZEN_SOUP_COOKIE_CACHE_FOR_WEBKIT2)
    toImpl(contextRef)->setSoupDataDirectory(toImpl(soupDataDirectory)->string());
#endif
}
void WKContextSetCertificateFile(WKContextRef contextRef, WKStringRef certificateFile)
{
#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
    toImpl(contextRef)->setCertificateFile(toWTFString(certificateFile));
#endif
}

void WKContextResetStoragePath(WKContextRef contextRef)
{
#if ENABLE(TIZEN_RESET_PATH)
    toImpl(contextRef)->resetStoragePath();
#endif
}

void WKContextSetTizenExtensibleAPI(WKContextRef contextRef, WKTizenExtensibleAPI extensibleAPI, bool enable)
{
#if ENABLE(TIZEN_EXTENSIBLE_API)
    toImpl(contextRef)->setTizenExtensibleAPI(static_cast<WebCore::ExtensibleAPI>(extensibleAPI), enable);
#endif
}

bool WKContextGetTizenExtensibleAPI(WKContextRef contextRef, WKTizenExtensibleAPI extensibleAPI)
{
#if ENABLE(TIZEN_EXTENSIBLE_API)
    return toImpl(contextRef)->getTizenExtensibleAPI(static_cast<WebCore::ExtensibleAPI>(extensibleAPI));
#endif
}
