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

#ifndef ewk_context_menu_private_h
#define ewk_context_menu_private_h

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
#include "WKContextMenuItem.h"
#include "ewk_context_menu.h"

Ewk_Context_Menu* ewkContextMenuCreateFromWKArray(WKArrayRef menu, WKStringRef linkURLString = 0, WKStringRef imageURLString = 0);
void ewkContextMenuDelete(Ewk_Context_Menu* menu);
WKArrayRef ewkContextMenuConvertToWKArray(Ewk_Context_Menu* menu);
Ewk_Context_Menu_Item* ewkContextMenuItemCreateFromWKContextMenuItem(WKContextMenuItemRef wkContextMenuItem, WKStringRef linkURLString = 0, WKStringRef imageURLString = 0);
void ewkContextMenuItemDelete(Ewk_Context_Menu_Item* item);
#endif

#endif // ewk_context_menu_private_h
