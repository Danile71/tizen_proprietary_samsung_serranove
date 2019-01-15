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

#include "config.h"
#include "ewk_view_context_menu_client.h"

#if PLATFORM(TIZEN)
#include "EwkViewImpl.h"
#include "WKPage.h"
#include "WKPageTizen.h"
#include "WKRetainPtr.h"
#include "ewk_context_menu.h"
#include "ewk_context_menu_private.h"
#include "ewk_view.h"

static void getContextMenuFromProposedMenu(WKPageRef page, WKArrayRef proposedMenu, WKArrayRef* newMenu, WKHitTestResultRef hitTestResult, WKTypeRef userData, const void* clientInfo)
{
    WKRetainPtr<WKStringRef> wkLinkURLString(AdoptWK, WKPageContextMenuCopyAbsoluteLinkURLString(page));
    WKRetainPtr<WKStringRef> wkImageURLString(AdoptWK, WKPageContextMenuCopyAbsoluteImageURLString(page));
    Ewk_Context_Menu* menu = ewkContextMenuCreateFromWKArray(proposedMenu, wkLinkURLString.get(), wkImageURLString.get());

    if (!menu)
        return;

    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));
    ewkViewCustomizeContextMenu(ewkView, menu);
    *newMenu = ewkContextMenuConvertToWKArray(menu);

    ewkContextMenuDelete(menu);
}

static void customContextMenuItemSelected(WKPageRef page, WKContextMenuItemRef menuItem, const void* clientInfo)
{
    WKRetainPtr<WKStringRef> wkLinkURLString(AdoptWK, WKPageContextMenuCopyAbsoluteLinkURLString(page));
    WKRetainPtr<WKStringRef> wkImageURLString(AdoptWK, WKPageContextMenuCopyAbsoluteImageURLString(page));
    Ewk_Context_Menu_Item* item = ewkContextMenuItemCreateFromWKContextMenuItem(menuItem, wkLinkURLString.get(), wkImageURLString.get());
    if (!item)
        return;

    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));
    ewkViewCustomContextMenuItemSelected(ewkView, item);

    ewkContextMenuItemDelete(item);
}

void ewkViewContextMenuClientAttachClient(Evas_Object* ewkView)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkView);

    WKPageContextMenuClient contextMenuClient = {
        kWKPageContextMenuClientCurrentVersion,
        ewkView,
        0, // getContextMenuFromProposedMenu_deprecatedForUseWithV0,
        customContextMenuItemSelected,
        0, // contextMenuDismissed
        getContextMenuFromProposedMenu
    };

    WKPageSetPageContextMenuClient(toAPI(EwkViewImpl::fromEvasObject(ewkView)->page()), &contextMenuClient);
}
#endif // #if PLATFORM(TIZEN)
