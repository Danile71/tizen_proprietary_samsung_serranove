/*
 * Copyright (C) 2013 Samsung Electronics
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

#if ENABLE(TIZEN_OPEN_PANEL)
#include "OpenMediaPlayer.h"

#include "EwkViewImpl.h"
#include "ewk_context.h"
#include "ewk_view.h"
#include <Elementary.h>

namespace WebKit {
OpenMediaPlayer::OpenMediaPlayer(Evas_Object* ewkView)
    : m_ewkView(ewkView)
{
}

OpenMediaPlayer::~OpenMediaPlayer()
{
}

bool OpenMediaPlayer::open(Evas_Object* ewkView, const String& url, const String& cookies, const String& type)
{
    app_control_h service = NULL;
    if (app_control_create(&service) != APP_CONTROL_ERROR_NONE)
        return false;

    app_control_set_operation(service, APP_CONTROL_OPERATION_VIEW);
    app_control_set_uri(service, url.utf8().data());
    app_control_set_mime(service, type.utf8().data());

    if (!cookies.isEmpty())
        app_control_add_extra_data(service, "cookie", cookies.utf8().data());

    Ewk_Context* ewkContex = EwkViewImpl::fromEvasObject(m_ewkView)->ewkContext();
    const char* proxy = ewk_context_proxy_uri_get(ewkContex);
    if (proxy && strlen(proxy))
        app_control_add_extra_data(service, "proxy", proxy);

    if (app_control_send_launch_request(service, NULL, NULL) == APP_CONTROL_ERROR_APP_NOT_FOUND) {
        TIZEN_LOGE("APP_CONTROL_ERROR_APP_NOT_FOUND");
        app_control_destroy(service);
        return false;
    }

    app_control_destroy(service);

    return true;
}
}
#endif //ENABLE(TIZEN_OPEN_PANEL)
