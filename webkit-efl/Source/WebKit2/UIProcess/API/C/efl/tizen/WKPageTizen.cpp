/*
 * Copyright (C) 2011 Samsung Electronics
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
#include "WKPageTizen.h"

#include "WebImage.h"
#include "WebPageProxy.h"

using namespace WebKit;

void WKPageSetPageTizenClient(WKPageRef pageRef, const WKPageTizenClient* wkClient)
{
    toImpl(pageRef)->initializeTizenClient(wkClient);
}

WKImageRef WKPageCreateSnapshot(WKPageRef pageRef, WKRect viewArea, float scaleFactor)
{
    RefPtr<WebImage> webImage = toImpl(pageRef)->createSnapshot(toIntRect(viewArea), scaleFactor);
    return toAPI(webImage.release().leakRef());
}

#if ENABLE(TIZEN_CACHE_IMAGE_GET)
WKImageRef WKPageCreateCacheImage(WKPageRef pageRef, WKStringRef imageUrl)
{
    RefPtr<WebImage> webImage = toImpl(pageRef)->cacheImageGet(toImpl(imageUrl)->string());
    return toAPI(webImage.release().leakRef());
}
#endif

void WKPageGetSnapshotPdfFile(WKPageRef page, WKSize surfaceSize, WKSize contentsSize, WKStringRef fileName)
{
#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
    toImpl(page)->createPagesToPDF(toIntSize(surfaceSize), toIntSize(contentsSize), toImpl(fileName)->string());
#endif
}

void WKPageRecordingSurfaceSetEnable(WKPageRef page, bool enable)
{
#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
    toImpl(page)->recordingSurfaceSetEnableSet(enable);
#endif
}

WKStringRef WKPageContextMenuCopyAbsoluteLinkURLString(WKPageRef page)
{
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    return toCopiedAPI(toImpl(page)->contextMenuAbsoluteLinkURLString());
#else
    return toCopiedAPI(String());
#endif
}

WKStringRef WKPageContextMenuCopyAbsoluteImageURLString(WKPageRef page)
{
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    return toCopiedAPI(toImpl(page)->contextMenuAbsoluteImageURLString());
#else
    return toCopiedAPI(String());
#endif
}

bool WKPageScrollBy(WKPageRef page, WKSize scrollOffset)
{
    return toImpl(page)->scrollMainFrameBy(toIntSize(scrollOffset));
}

void WKPageGetWebStorageQuota(WKPageRef page, void* context, WKPageGetWebStorageQuotaFunction callback)
{
#if ENABLE(TIZEN_WEB_STORAGE)
#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
    toImpl(page)->getWebStorageQuotaBytes(WebStorageQuotaCallback::create(context, callback));
#endif
#endif
}

void WKPageSetWebStorageQuota(WKPageRef page, uint32_t quota)
{
#if ENABLE(TIZEN_WEB_STORAGE)
    toImpl(page)->setWebStorageQuotaBytes(quota);
#endif
}

void WKPageGetWebAppCapable(WKPageRef page, void* context, WKPageGetWebAppCapableFunction callback)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    toImpl(page)->getWebAppCapable(BooleanCallback::create(context, callback));
#endif
}

void WKPageGetWebAppIconURL(WKPageRef page, void* context, WKPageGetWebAppIconURLFunction callback)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    toImpl(page)->getWebAppIconURL(StringCallback::create(context, callback));
#endif
}

void WKPageGetWebAppIconURLs(WKPageRef page, void* context, WKPageGetWebAppIconURLsFunction callback)
{
#if ENABLE(TIZEN_SUPPORT_WEBAPP_META_TAG)
    toImpl(page)->getWebAppIconURLs(DictionaryCallback::create(context, callback));
#endif
}

void WKPageExecuteCommandWithArgument(WKPageRef page, WKStringRef command, WKStringRef argument)
{
    toImpl(page)->executeEditCommandWithArgument(toImpl(command)->string(), toImpl(argument)->string());
}

void WKPageReplyApplicationCachePermission(WKPageRef page, bool allow)
{
#if ENABLE(TIZEN_APPLICATION_CACHE)
    toImpl(page)->replyApplicationCachePermission(allow);
#endif
}

void WKPageReplyExceededIndexedDatabaseQuota(WKPageRef page, bool allow)
{
#if ENABLE(TIZEN_INDEXED_DATABASE)
    toImpl(page)->replyExceededIndexedDatabaseQuota(allow);
#endif
}

void WKPageReplyExceededDatabaseQuota(WKPageRef page, bool allow)
{
#if ENABLE(TIZEN_SQL_DATABASE)
    toImpl(page)->replyExceededDatabaseQuota(allow);
#endif
}

void WKPageReplyExceededLocalFileSystemQuota(WKPageRef page, bool allow)
{
#if ENABLE(TIZEN_FILE_SYSTEM)
    toImpl(page)->replyExceededLocalFileSystemQuota(allow);
#endif
}
