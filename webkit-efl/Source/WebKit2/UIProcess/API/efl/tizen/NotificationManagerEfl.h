/*
*
* Copyright (c) 2015 Samsung Electronics Co., Ltd. All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#ifndef NOTIFICATION_MANAGER_EFL_H
#define NOTIFICATION_MANAGER_EFL_H

#include <vector>
#include <Ecore.h>
#include <notification.h>

#include "ewk_notification.h"

namespace WebKit {

class NotificationItem;
class NotificationManagerEfl {
public:
    NotificationManagerEfl(Evas_Object* webView, EwkViewImpl* ewkView);

    Evas_Object* webView() { return m_webView; }
    EwkViewImpl* ewkview() { return m_ewkView; }
    Eina_Bool showNotification(Ewk_Notification* ewkNoti);
    Eina_Bool cancelNotification(int);
    Eina_Bool deleteAllNotifications();

private:
    Evas_Object* m_webView;
    EwkViewImpl* m_ewkView;

    std::vector<NotificationItem *> m_notiItems;
};


class NotificationItem {
public:
    NotificationItem(NotificationManagerEfl* notificationManagerEfl, Ewk_Notification* ewkNoti);
    ~NotificationItem();

    friend bool operator==(NotificationItem item1, NotificationItem item2)
    {
        return ((item1.m_origin && item1.m_title && item1.m_body)
                && (item2.m_origin && item2.m_title && item2.m_body)
                && (!strcmp(item1.m_origin, item2.m_origin))
                && (!strcmp(item1.m_title, item2.m_title))
                && (!strcmp(item1.m_body, item2.m_body)));
    }

    Eina_Bool makeNotification();
    Eina_Bool deactivateItem();
    void showNotiPopup(const char*);
    const char* origin() { return m_origin; }
    const char* title() { return m_title; }
    const char* body() { return m_body; }
    int ewkNotiId() { return m_ewkNotiId; }
    int notiId() { return m_notiId; }

private:
    void init();
    Eina_Bool createNotification(Eina_Bool);
    Eina_Bool downloadNotificationIconStart();
    Eina_Bool saveNotificationIcon(SoupBuffer*);
    void removeIconDownloadTimer();

    static void downloadNotificationIconFinishedCb(SoupSession*, SoupMessage*, gpointer);
    static Eina_Bool iconDownloadTimerExpiredCb(void*);

    Ewk_Notification* m_ewkNoti;
    NotificationManagerEfl* m_parent;
    const char* m_origin;
    const char* m_title;
    const char* m_body;
    const char* m_iconUri;
    const char* m_iconPath;
    int m_ewkNotiId;
    int m_notiId;
    double m_timeStamp;
    notification_h m_notiHandle;
    Ecore_Timer* m_iconDownloadTimer;
};

} // namespace WebKit

#endif /* NOTIFICATION_MANAGER_EFL_H */
