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

#include "config.h"
#include "ewk_view_icondatabase_client.h"

#if ENABLE(TIZEN_ICON_DATABASE)
#include "WKIconDatabase.h"
#include "ewk_view_private.h"

static void didChangeIconForPageURLTizen(WKIconDatabaseRef iconDatabase, WKURLRef pageURL, const void* clientInfo)
{
    Evas_Object* ewkView = static_cast<Evas_Object*>(const_cast<void*>(clientInfo));
    ewkViewIconReceived(ewkView);
}

void ewk_view_icondatabase_client_attach(Evas_Object* ewkView, WKContextRef contextRef)
{
    WKIconDatabaseClient iconDataBaseClient;
    memset(&iconDataBaseClient, 0, sizeof(WKIconDatabaseClient));
    iconDataBaseClient.version = kWKIconDatabaseClientCurrentVersion;
    iconDataBaseClient.clientInfo = ewkView;

    iconDataBaseClient.didChangeIconForPageURL = didChangeIconForPageURLTizen;

    WKIconDatabaseRef iconDataBase = WKContextGetIconDatabase(contextRef);
    WKIconDatabaseSetIconDatabaseClient(iconDataBase, &iconDataBaseClient);
}

void ewk_view_icondatabase_client_detach(WKContextRef contextRef)
{
    WKIconDatabaseRef iconDataBase = WKContextGetIconDatabase(contextRef);
    WKIconDatabaseSetIconDatabaseClient(iconDataBase, 0);
}
#endif
