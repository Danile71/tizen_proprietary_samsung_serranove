/*
 * Copyright (C) 2013 Samsung Electronics
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

#ifndef ewk_notification_provider_private_h
#define ewk_notification_provider_private_h

#if ENABLE(TIZEN_NOTIFICATIONS)
#include "WKRetainPtr.h"
#include <WebKit2/WKBase.h>
#include <wtf/HashMap.h>

typedef HashMap<uint64_t, WKPageRef> NotificationMap;

class Ewk_Notification_Provider {
public:
    static PassOwnPtr<Ewk_Notification_Provider> create(WKContextRef contextRef)
    {
        return adoptPtr(new Ewk_Notification_Provider(contextRef));
    }
    ~Ewk_Notification_Provider();

private:
    explicit Ewk_Notification_Provider(WKContextRef contextrRef);

    static void showNotification(WKPageRef, WKNotificationRef, const void* clientInfo);
    static void cancelNotification(WKNotificationRef, const void* clientInfo);

    WKRetainPtr<WKContextRef> m_wkContext;
    NotificationMap m_notificationPageMap;
};
#endif // #if ENABLE(TIZEN_NOTIFICATIONS)

#endif // ewk_notification_provider_private_h
