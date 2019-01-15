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
#include "ewk_context_menu.h"

#if PLATFORM(TIZEN)
#include "WKAPICast.h"
#include "WKArray.h"
#include "WKContextMenuItem.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "ewk_context_menu_private.h"
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * \struct  _Ewk_Context_Menu
 * @brief   Contains the context menu data.
 */
struct _Ewk_Context_Menu {
    Eina_List* items; /**< the list of Ewk_Context_Menu_Item */
};

/**
 * \struct  _Ewk_Context_Menu_Item
 * @brief   Represents one item of the context menu object.
 */
struct _Ewk_Context_Menu_Item {
    WKRetainPtr<WKContextMenuItemRef> itemRef;
    CString linkURL;
    CString imageURL;
};

static Eina_Bool appendItemToContextMenu(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item* item)
{
    if (!menu || !item)
        return false;

    menu->items = eina_list_append(menu->items, item);
    return true;
}

static Ewk_Context_Menu* createContextMenu()
{
    Ewk_Context_Menu* menu = new Ewk_Context_Menu;
    menu->items = 0;
    return menu;
}

Ewk_Context_Menu* ewkContextMenuCreateFromWKArray(WKArrayRef menu, WKStringRef linkURLString, WKStringRef imageURLString)
{
    if (!menu)
        return 0;

    Ewk_Context_Menu* ewkContextMenu = createContextMenu();
    int size = WKArrayGetSize(menu);
    WKContextMenuItemRef item;
    for (int i = 0; i < size; i++) {
        item = static_cast<WKContextMenuItemRef>(WKArrayGetItemAtIndex(menu, i));
        if (WKGetTypeID(item) == WKContextMenuItemGetTypeID())
            appendItemToContextMenu(ewkContextMenu, ewkContextMenuItemCreateFromWKContextMenuItem(item, linkURLString, imageURLString));
    }

    return ewkContextMenu;
}

void ewkContextMenuDelete(Ewk_Context_Menu* menu)
{
    if (!menu)
        return;

    void* data = 0;
    EINA_LIST_FREE(menu->items, data)
        ewkContextMenuItemDelete(static_cast<Ewk_Context_Menu_Item*>(data));
    delete menu;
}

WKArrayRef ewkContextMenuConvertToWKArray(Ewk_Context_Menu* menu)
{
    if (!menu)
        return 0;

    unsigned count = eina_list_count(menu->items);
    if (!count)
        return 0;

    Ewk_Context_Menu_Item* item = 0;
    OwnArrayPtr<WKTypeRef> items = adoptArrayPtr(new WKTypeRef[count]);
    for (unsigned i = 0; i < count; i++) {
        item = static_cast<Ewk_Context_Menu_Item*>(eina_list_nth(menu->items, i));
        items[i] = item->itemRef.get();
    }

    return WKArrayCreate(items.get(), count);
}

Ewk_Context_Menu_Item* ewkContextMenuItemCreateFromWKContextMenuItem(WKContextMenuItemRef wkContextMenuItem, WKStringRef linkURLString, WKStringRef imageURLString)
{
    if (!wkContextMenuItem)
        return 0;

    Ewk_Context_Menu_Item* item = new Ewk_Context_Menu_Item;
    item->itemRef = wkContextMenuItem;
    if (linkURLString)
        item->linkURL = toImpl(linkURLString)->string().utf8();
    if (imageURLString)
        item->imageURL = toImpl(imageURLString)->string().utf8();

    return item;
}

void ewkContextMenuItemDelete(Ewk_Context_Menu_Item* item)
{
    if (!item)
        return;

    delete item;
}

unsigned ewk_context_menu_item_count(Ewk_Context_Menu* menu)
{
    if (!menu)
        return 0;

    return eina_list_count(menu->items);
}

Ewk_Context_Menu_Item* ewk_context_menu_nth_item_get(Ewk_Context_Menu* menu, unsigned int n)
{
    if (!menu)
        return 0;

    return static_cast<Ewk_Context_Menu_Item*>(eina_list_nth(menu->items, n));
}

Eina_Bool ewk_context_menu_item_remove(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item* item)
{
    if (!menu || !item)
        return false;

    menu->items = eina_list_remove(menu->items, static_cast<const void*>(item));
    ewkContextMenuItemDelete(item);

    return true;
}

Eina_Bool ewk_context_menu_item_append_as_action(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item_Tag tag, const char* title, Eina_Bool enabled)
{
    if (!menu)
        return false;

    WKRetainPtr<WKStringRef> titleString(AdoptWK, WKStringCreateWithUTF8CString(title));
    WKContextMenuItemRef item = WKContextMenuItemCreateAsAction(static_cast<WKContextMenuItemTag>(tag), titleString.get(), enabled);
    return appendItemToContextMenu(menu, ewkContextMenuItemCreateFromWKContextMenuItem(item));
}

Eina_Bool ewk_context_menu_item_append(Ewk_Context_Menu* menu, Ewk_Context_Menu_Item_Tag tag, const char* title, const char* iconFile, Eina_Bool enabled)
{
    if (!menu)
        return false;

    WKRetainPtr<WKStringRef> titleString(AdoptWK, WKStringCreateWithUTF8CString(title));
    WKRetainPtr<WKStringRef> iconString(AdoptWK, WKStringCreateWithUTF8CString(iconFile));
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_ICON_TYPE_SUPPORT)
    WKContextMenuItemRef item = WKContextMenuItemCreateWithTitleAndIconAsAction(static_cast<WKContextMenuItemTag>(tag), titleString.get(), iconString.get(), enabled);
#else
    WKContextMenuItemRef item = WKContextMenuItemCreateAsAction(static_cast<WKContextMenuItemTag>(tag), titleString.get(), enabled);
#endif
    return appendItemToContextMenu(menu, ewkContextMenuItemCreateFromWKContextMenuItem(item));
}

Ewk_Context_Menu_Item_Tag ewk_context_menu_item_tag_get(Ewk_Context_Menu_Item* item)
{
    if (!item)
        return EWK_CONTEXT_MENU_ITEM_TAG_NO_ACTION;

    return static_cast<Ewk_Context_Menu_Item_Tag>(WKContextMenuItemGetTag(item->itemRef.get()));
}

Ewk_Context_Menu_Item_Type ewk_context_menu_item_type_get(Ewk_Context_Menu_Item* item)
{
    if (!item)
        return EWK_CONTEXT_MENU_ITEM_TYPE_ACTION;

    return static_cast<Ewk_Context_Menu_Item_Type>(WKContextMenuItemGetType(item->itemRef.get()));
}

Eina_Bool ewk_context_menu_item_enabled_get(const Ewk_Context_Menu_Item* item)
{
    if (!item)
        return false;

    return WKContextMenuItemGetEnabled(item->itemRef.get());
}

const char* ewk_context_menu_item_link_url_get(Ewk_Context_Menu_Item* item)
{
    if (!item)
        return 0;

    return item->linkURL.data();
}

const char* ewk_context_menu_item_image_url_get(Ewk_Context_Menu_Item* item)
{
    if (!item)
        return 0;

    return item->imageURL.data();
}
#endif // #if PLATFORM(TIZEN)
