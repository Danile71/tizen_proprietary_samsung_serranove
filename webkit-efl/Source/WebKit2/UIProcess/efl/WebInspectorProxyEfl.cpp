/*
 * Copyright (C) 2012 Samsung Electronics
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
#include "WebInspectorProxy.h"

#if ENABLE(INSPECTOR)

#include "EwkViewImpl.h"
#include "WebProcessProxy.h"
#include "ewk_view_private.h"
#include <WebCore/NotImplemented.h>
#include <unistd.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

WebPageProxy* WebInspectorProxy::platformCreateInspectorPage()
{
    ASSERT(m_page);

    m_inspectorWindow = ecore_evas_buffer_new(initialWindowWidth, initialWindowHeight);
    if (!m_inspectorWindow)
        return 0;

    m_inspectorView = ewk_view_base_add(ecore_evas_get(m_inspectorWindow), toAPI(page()->process()->context()), toAPI(inspectorPageGroup()));
    EwkViewImpl* inspectorViewImpl = EwkViewImpl::fromEvasObject(m_inspectorView);
    inspectorViewImpl->setThemePath(TEST_THEME_DIR "/default.edj");

#if ENABLE(WAIT_UPVERSION)
    Ewk_Settings* settings = inspectorViewImpl->settings();
    ewk_settings_file_access_from_file_urls_allowed_set(settings, true);
#endif

    return inspectorViewImpl->page();
}

void WebInspectorProxy::platformOpen()
{
    notImplemented();
}

void WebInspectorProxy::platformDidClose()
{
    if (m_inspectorView) {
        evas_object_del(m_inspectorView);
        m_inspectorView = 0;
    }

    if (m_inspectorWindow) {
        ecore_evas_free(m_inspectorWindow);
        m_inspectorWindow = 0;
    }
}

void WebInspectorProxy::platformBringToFront()
{
    notImplemented();
}

bool WebInspectorProxy::platformIsFront()
{
    notImplemented();
    return false;
}

void WebInspectorProxy::platformInspectedURLChanged(const String&)
{
    notImplemented();
}

String WebInspectorProxy::inspectorPageURL() const
{
    return makeString(inspectorBaseURL(), "/inspector.html");
}

String WebInspectorProxy::inspectorBaseURL() const
{
    String inspectorFilesPath = WK2_WEB_INSPECTOR_INSTALL_DIR;
    if (access(inspectorFilesPath.utf8().data(), R_OK))
        inspectorFilesPath = WK2_WEB_INSPECTOR_DIR;

    return "file://" + inspectorFilesPath;
}

unsigned WebInspectorProxy::platformInspectedWindowHeight()
{
    notImplemented();
    return 0;
}

void WebInspectorProxy::platformAttach()
{
    notImplemented();
}

void WebInspectorProxy::platformDetach()
{
    notImplemented();
}

void WebInspectorProxy::platformSetAttachedWindowHeight(unsigned)
{
    notImplemented();
}

} // namespace WebKit

#endif // ENABLE(INSPECTOR)
