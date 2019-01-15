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

#include <app_control_internal.h>
#include <Ecore.h>
#include <eina_list.h>
#include <fcntl.h>
#include <notification_internal.h>
#include <string>

#include "NotificationManagerEfl.h"

#define BR_STRING_NO_NAME "No name"
#define BR_STRING_INTERNET "Internet"
#define BR_STRING_FAILED_TO_GET_WEB_NOTI_ICON "Failed to get notification icon"
#define ICON_FILE_NAME_MAX_LENGTH 10
#define ICON_DOWNLOAD_TIME_LIMIT 5
// FIXME: need to change correcting path
#define PATH_WEB_NOTI_ICON_PNG "/opt/usr/apps/org.tizen.browser/"
#define http_scheme "http://"

namespace WebKit {

NotificationManagerEfl::NotificationManagerEfl(Evas_Object* webView, EwkViewImpl* ewkView)
    : m_webView(webView)
    , m_ewkView(ewkView)
{}

Eina_Bool NotificationManagerEfl::showNotification(Ewk_Notification* ewkNoti)
{
    NotificationItem* item = new NotificationItem(this, ewkNoti);
    Eina_Bool isDuplicated = EINA_FALSE;
    for (unsigned int i = 0 ; i < m_notiItems.size() ; i++) {
        if (m_notiItems[i] == item) {
            LOG_ERROR("origin[%s], title[%s], body[%s] is already exist. Not making on noti bar", item->origin(), item->title(), item->body());
            isDuplicated = EINA_TRUE;
            break;
        }
    }
    if (isDuplicated == EINA_TRUE) {
        item->deactivateItem();
        delete item;
    } else {
        item->makeNotification();
        m_notiItems.push_back(item);
    }

    return EINA_TRUE;
}

Eina_Bool NotificationManagerEfl::cancelNotification(int ewkNotiId)
{
    if (!ewkNotiId) {
        LOG_ERROR("ewkNotiId is unrecognizable as 0");
        return EINA_FALSE;
    }

    for (unsigned int i = 0; i < m_notiItems.size(); i++) {
        if (m_notiItems[i]->ewkNotiId() == ewkNotiId) {
            m_notiItems[i]->deactivateItem();
            delete m_notiItems[i];
            m_notiItems.erase(m_notiItems.begin() + i);
            break;
        }
    }

    return EINA_TRUE;
}

Eina_Bool NotificationManagerEfl::deleteAllNotifications()
{
    if (!m_notiItems.size())
        return EINA_FALSE;

    for (unsigned int i = 0; i < m_notiItems.size(); i++) {
        if (m_notiItems[i])
            delete m_notiItems[i];
    }
    m_notiItems.clear();

    return EINA_TRUE;
}

NotificationItem::NotificationItem(NotificationManagerEfl* notificationManagerEfl, Ewk_Notification* ewkNoti)
    : m_ewkNoti(ewkNoti)
    , m_parent(notificationManagerEfl)
    , m_origin(NULL)
    , m_title(NULL)
    , m_body(NULL)
    , m_iconUri(NULL)
    , m_iconPath(NULL)
    , m_ewkNotiId(-1)
    , m_notiId(-1)
    , m_timeStamp(-1.0)
    , m_notiHandle(NULL)
    , m_iconDownloadTimer(NULL)
{
    init();
}

NotificationItem::~NotificationItem()
{
    if (m_iconPath)
        unlink(m_iconPath);

    eina_stringshare_del(m_origin);
    eina_stringshare_del(m_title);
    eina_stringshare_del(m_body);
    eina_stringshare_del(m_iconUri);
    eina_stringshare_del(m_iconPath);

    if (m_notiHandle) {
        notification_delete(m_notiHandle);
        notification_free(m_notiHandle);
    }
    removeIconDownloadTimer();
}

void NotificationItem::init()
{
    const char* origin = NULL;
    origin = ewk_security_origin_host_get(ewk_notification_security_origin_get(m_ewkNoti));
    if (origin && strlen(origin))
        m_origin = eina_stringshare_add(origin);
    else
        m_origin = eina_stringshare_add(BR_STRING_NO_NAME);

    const char* title = NULL;
    title = ewk_notification_title_get(m_ewkNoti);
    if (title && strlen(title))
        m_title = eina_stringshare_add(title);
    else
        m_title = eina_stringshare_add(BR_STRING_NO_NAME);

    const char* body = NULL;
    body = ewk_notification_body_get(m_ewkNoti);
    if (body && strlen(body))
        m_body = eina_stringshare_add(body);
    else
        m_body = eina_stringshare_add(BR_STRING_NO_NAME);

    const char* icon_url = NULL;
    icon_url = ewk_notification_icon_url_get(m_ewkNoti);
    if (icon_url && strlen(icon_url))
        m_iconUri = eina_stringshare_add(icon_url);

    m_ewkNotiId = (int)ewk_notification_id_get(m_ewkNoti);
    m_timeStamp = ecore_loop_time_get();
}

Eina_Bool NotificationItem::makeNotification()
{
    Eina_Bool result = EINA_FALSE;

    /* check duplicated */
    if (m_iconUri && strlen(m_iconUri)) {
        /* download icon */
        result = downloadNotificationIconStart();
        if (result == EINA_FALSE)
            result = createNotification(EINA_FALSE);
    } else
        result = createNotification(EINA_FALSE);

    ewk_notification_showed(m_parent->ewkview()->ewkContext(), m_ewkNotiId);

    return result;
}

Eina_Bool NotificationItem::createNotification(Eina_Bool hasIcon)
{
    int privId = 0;
    char ewkNotiId[10] = {0, };
    bundle* selected_data = NULL;
    app_control_h appControl = NULL;
    int err = NOTIFICATION_ERROR_NONE;
    notification_h handle = notification_create(NOTIFICATION_TYPE_NOTI);

    if (!handle) {
        LOG_ERROR("Failed to create notification");
        return EINA_FALSE;
    }

    err = notification_set_layout(handle, NOTIFICATION_LY_NOTI_EVENT_SINGLE);
    if (err != NOTIFICATION_ERROR_NONE) {
        LOG_ERROR("notification_set_layout NOTIFICATION_LY_NOTI_EVENT_MULTIPLE failed with err[%d]", err);
        goto ERROR;
    }

    err = notification_set_text(handle, NOTIFICATION_TEXT_TYPE_INFO_1, BR_STRING_INTERNET, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    if (err != NOTIFICATION_ERROR_NONE) {
        LOG_ERROR("Failed to set title in notification with err[%d]", err);
        goto ERROR;
    }

    if (m_origin && strlen(m_origin))
        err = notification_set_text(handle, NOTIFICATION_TEXT_TYPE_INFO_2, m_origin, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    else
        err = notification_set_text(handle, NOTIFICATION_TEXT_TYPE_INFO_2, BR_STRING_NO_NAME, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);

    if (err != NOTIFICATION_ERROR_NONE) {
        LOG_ERROR("notification_set_text m_origin failed with err[%d]", err);
        goto ERROR;
    }

    if (m_title && strlen(m_title))
        err = notification_set_text(handle, NOTIFICATION_TEXT_TYPE_TITLE, m_title, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    else
        err = notification_set_text(handle, NOTIFICATION_TEXT_TYPE_TITLE, BR_STRING_NO_NAME, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);

    if (err != NOTIFICATION_ERROR_NONE) {
        LOG_ERROR("notification_set_text m_title failed with err[%d]", err);
        goto ERROR;
    }

    if (m_body && strlen(m_body))
        err = notification_set_text(handle, NOTIFICATION_TEXT_TYPE_CONTENT, m_body, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    else
        err = notification_set_text(handle, NOTIFICATION_TEXT_TYPE_CONTENT, BR_STRING_NO_NAME, NULL, NOTIFICATION_VARIABLE_TYPE_NONE);
    if (err != NOTIFICATION_ERROR_NONE) {
        LOG_ERROR("notification_set_text m_title failed with err[%d]", err);
        goto ERROR;
    }

    if (hasIcon == EINA_TRUE && m_iconPath && strlen(m_iconPath)) {
        err = notification_set_image(handle, NOTIFICATION_IMAGE_TYPE_ICON, m_iconPath);
        if (err != NOTIFICATION_ERROR_NONE)
            LOG_ERROR("notification_set_image failed with err[%d]", err);
    }

    if (app_control_create(&appControl) != APP_CONTROL_ERROR_NONE) {
        LOG_ERROR("Fail to create appControl handle");
        goto ERROR;
    }

    if (app_control_set_operation(appControl, APP_CONTROL_OPERATION_VIEW) != APP_CONTROL_ERROR_NONE) {
        LOG_ERROR("Fail to set appControl operation");
        goto ERROR;
    }

    snprintf(ewkNotiId, 10, "%d", m_ewkNotiId);

    if (app_control_add_extra_data(appControl, "web_noti_launch", ewkNotiId) != APP_CONTROL_ERROR_NONE) {
        LOG_ERROR("app_control_add_extra_data is failed.");
        goto ERROR;
    }

    if (app_control_to_bundle(appControl, &selected_data) != APP_CONTROL_ERROR_NONE) {
        LOG_ERROR("Fail to app_control_to_bundle");
        goto ERROR;
    }

    if (selected_data) {
        err = notification_set_execute_option(handle, NOTIFICATION_EXECUTE_TYPE_SINGLE_LAUNCH, NULL, NULL, selected_data);
        if (err != NOTIFICATION_ERROR_NONE)
            LOG_ERROR("notification_set_execute_option is failed with err[%d]", err);
    }

    err = notification_set_display_applist(handle, NOTIFICATION_DISPLAY_APP_TICKER | NOTIFICATION_DISPLAY_APP_NOTIFICATION_TRAY);
    if (err != NOTIFICATION_ERROR_NONE)
       LOG_ERROR("notification_set_display_applist is failed with err[%d]", err);

    err = notification_insert(handle, &privId);
    if (err != NOTIFICATION_ERROR_NONE) {
        LOG_ERROR("notification_insert is failed with err[%d]", err);
        goto ERROR;
    }

    app_control_destroy(appControl);

    m_notiId = privId;
    m_notiHandle = handle;

    return EINA_TRUE;

ERROR:
    err = notification_free(handle);
    if (err != NOTIFICATION_ERROR_NONE)
        LOG_ERROR("notification_free is failed with err[%d]", err);

    if (appControl != NULL)
        app_control_destroy(appControl);

    return EINA_FALSE;
}

Eina_Bool NotificationItem::downloadNotificationIconStart()
{
    if (!m_iconUri)
        return EINA_FALSE;

    SoupSession* soupSession = NULL;
    SoupMessage* soupMsg = NULL;

    soupSession = soup_session_async_new();
    if (!soupSession) {
        LOG_ERROR("Failed to soup_session_async_new");
        return EINA_FALSE;
    }
    g_object_set(soupSession, SOUP_SESSION_TIMEOUT, 15, (char *)NULL);

    const char* defaultProxyUri = ewk_context_proxy_uri_get(ewk_context_default_get());
    if (defaultProxyUri) {
        std::string proxyUri = std::string(http_scheme) + std::string(defaultProxyUri);
        SoupURI* soupUri = soup_uri_new(proxyUri.c_str());
        g_object_set(soupSession, SOUP_SESSION_PROXY_URI, soupUri, (char *)NULL);
        if (soupUri)
            soup_uri_free(soupUri);
    }
    soupMsg = soup_message_new("GET", m_iconUri);
    soup_session_queue_message(soupSession, soupMsg, downloadNotificationIconFinishedCb, (void *)this);

    removeIconDownloadTimer();
    m_iconDownloadTimer = ecore_timer_add(ICON_DOWNLOAD_TIME_LIMIT, iconDownloadTimerExpiredCb, this);

    return EINA_TRUE;
}

void NotificationItem::removeIconDownloadTimer()
{
    if (m_iconDownloadTimer)
        ecore_timer_del(m_iconDownloadTimer);
    m_iconDownloadTimer = NULL;
}

void NotificationItem::downloadNotificationIconFinishedCb(SoupSession* session, SoupMessage* msg, gpointer data)
{
    if (!msg)
        return;

    if (!data)
        return;

    SoupBuffer* body = soup_message_body_flatten(msg->response_body);

    NotificationItem* wni = (NotificationItem *)data;
    if (wni->m_iconDownloadTimer) {
        wni->removeIconDownloadTimer();
        wni->createNotification(wni->saveNotificationIcon(body));
    } else
        LOG_ERROR("Icon data is received too late, and it's already been made by default icon");
}

Eina_Bool NotificationItem::iconDownloadTimerExpiredCb(void* data)
{
    if (!data)
        return ECORE_CALLBACK_CANCEL;

    NotificationItem* wni = (NotificationItem*)data;
    wni->removeIconDownloadTimer();
    wni->createNotification(EINA_FALSE);

    wni->showNotiPopup(BR_STRING_FAILED_TO_GET_WEB_NOTI_ICON);

    return ECORE_CALLBACK_CANCEL;
}

void NotificationItem::showNotiPopup(const char* msg)
{
    if (!msg)
        return;

    notification_status_message_post(msg);
    elm_access_say(msg);
}

Eina_Bool NotificationItem::saveNotificationIcon(SoupBuffer* body)
{
    if (!body)
        return EINA_FALSE;

    if (!body->data || body->length <= 0) {
        LOG_ERROR("body has no valid data");
        soup_buffer_free(body);
        return EINA_FALSE;
    }

    int fd = 0;
    char time_stamp[ICON_FILE_NAME_MAX_LENGTH + 1] = {0, };
    snprintf(time_stamp, ICON_FILE_NAME_MAX_LENGTH, "%lf", m_timeStamp);

    std::string file_name = std::string(PATH_WEB_NOTI_ICON_PNG) + std::string(time_stamp) + std::string(".png");

    if ((fd = open(file_name.c_str(), O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) {
        soup_buffer_free(body);
        return EINA_FALSE;
    }
    int wrtieLen = write(fd, body->data, body->length);
    close(fd);

    soup_buffer_free(body);

    if (wrtieLen != (int)body->length)
        return EINA_FALSE;

    m_iconPath = eina_stringshare_add(file_name.c_str());

    return EINA_TRUE;
}

Eina_Bool NotificationItem::deactivateItem()
{
    if (m_parent->webView()) {
        Eina_List* list = NULL;
        list = eina_list_append(list, m_ewkNoti);
        eina_list_free(list);
    } else
        LOG_ERROR("The webview, which this notification item had, is not valid any more");

    return EINA_TRUE;
}

} // namespace WebKit
