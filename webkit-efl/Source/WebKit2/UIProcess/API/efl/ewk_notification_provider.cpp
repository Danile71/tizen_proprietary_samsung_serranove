/*
 * Copyright (C) 2012, 2013 Samsung Electronics
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

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "EwkViewImpl.h"
#include "WKNotification.h"
#include "WKNotificationManager.h"
#include "WKNotificationProvider.h"
#include "ewk_notification.h"
#include "ewk_notification_provider_private.h"
#include "ewk_view_private.h"

Ewk_Notification_Provider::Ewk_Notification_Provider(WKContextRef contextRef)
    : m_wkContext(contextRef)
{
    ASSERT(m_wkContext.get());

    WKNotificationManagerRef notificationManagerRef = WKContextGetNotificationManager(m_wkContext.get());
    ASSERT(notificationManagerRef);

    WKNotificationProvider provider = {
        kWKNotificationProviderCurrentVersion, //version
        this, //clientInfo
        showNotification, //show
        cancelNotification, //cancel
        0, //didDestroyNotification
        0, //addNotificationManger
        0, //removeNotificationManager
        0, //notificationPermissions
        0 //clearNotifications
    };

    WKNotificationManagerSetProvider(notificationManagerRef, &provider);
}

Ewk_Notification_Provider::~Ewk_Notification_Provider()
{
    WKNotificationManagerRef notificationManagerRef = WKContextGetNotificationManager(m_wkContext.get());
    ASSERT(notificationManagerRef);

    WKNotificationManagerSetProvider(notificationManagerRef, 0);
}

void Ewk_Notification_Provider::cancelNotification(WKNotificationRef notification, const void* clientInfo)
{
    Ewk_Notification_Provider* notificationProvider = static_cast<Ewk_Notification_Provider*>(const_cast<void*>(clientInfo));
    uint64_t notificationID = WKNotificationGetID(notification);
    WKPageRef wkPage = notificationProvider->m_notificationPageMap.take(notificationID);
    Evas_Object* ewkView = const_cast<Evas_Object*>(EwkViewImpl::viewFromPageViewMap(wkPage));
    if (!ewkView)
        return;

    ewkViewCancelNotification(ewkView, notificationID);
}

void Ewk_Notification_Provider::showNotification(WKPageRef page, WKNotificationRef notification, const void* clientInfo)
{
    Evas_Object* ewkView = const_cast<Evas_Object*>(EwkViewImpl::viewFromPageViewMap(page));
    if (!ewkView)
        return;

    Ewk_Notification_Provider* notificationProvider = static_cast<Ewk_Notification_Provider*>(const_cast<void*>(clientInfo));
    uint64_t notificationID = WKNotificationGetID(notification);
    notificationProvider->m_notificationPageMap.add(notificationID, page);
    Ewk_Notification* ewkNotification = ewkNotificationCreateNotification(notification);

    ewkViewShowNotification(ewkView, ewkNotification);
}
#endif // #if ENABLE(TIZEN_NOTIFICATIONS)