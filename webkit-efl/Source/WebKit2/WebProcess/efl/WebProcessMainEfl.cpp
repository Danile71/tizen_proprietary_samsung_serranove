/*
 * Copyright (C) 2011 Samsung Electronics. All rights reserved.
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
#include "WebProcessMainEfl.h"

#include "ProxyResolverSoup.h"
#include "WKBase.h"
#include <Ecore.h>
#include <Ecore_File.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/RunLoop.h>
#include <WebKit2/WebProcess.h>
#include <runtime/InitializeThreading.h>
#include <unistd.h>
#include <wtf/MainThread.h>

#ifdef HAVE_ECORE_X
#include <Ecore_X.h>
#endif

#if USE(ACCELERATED_COMPOSITING)
#include "WebGraphicsLayer.h"
#endif

#if ENABLE(TIZEN_NATIVE_MEMORY_SNAPSHOT)
#include "WebPage.h"
#endif

#if ENABLE(TIZEN_USE_STACKPOINTER_AT_WEBPROCESS_STARTUP)
#include "wtf/WTFThreadData.h"
#endif

#if ENABLE(TIZEN_PROCESS_PERMISSION_CONTROL)
#include "ProcessSmackLabel.h"
#endif

#if ENABLE(TIZEN_COMPRESSION_PROXY)
#include <set>
#include <string>
#endif

#if ENABLE(TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_TEXT_SELECTION)
#include <efl_assist.h>
#endif

#if HAVE(ACCESSIBILITY)
#include "WebBaseAccessibilityTizen.h"
#endif

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_NATIVE_MEMORY_SNAPSHOT)
void webkit_memory_dump_cb(void* data, Ecore_File_Monitor* monitor, Ecore_File_Event event, const char* path)
{
    if (event == ECORE_FILE_EVENT_MODIFIED)
        dumpMemorySnapshot();
}
#endif

WK_EXPORT int WebProcessMainEfl(int argc, char* argv[])
{
#if ENABLE(TIZEN_COMPRESSION_PROXY)
    std::set<std::string> temp;
    std::string tempStr = "CompressionProxy";
    temp.insert(tempStr);
#endif

    // WebProcess should be launched with an option.
    if (argc != 2)
        return 1;

#if ENABLE(TIZEN_PROCESS_PERMISSION_CONTROL) && !ENABLE(TIZEN_WEBKIT2_EFL_WTR)
    // change process smack label
    if (!changeProcessSmackLabel("/usr/bin/WebProcess", argv[0])) {
        TIZEN_LOGI("failed to change smack label");
        return 1;
    }
    // drop CAP_MAC_ADMIN capability
    if (!dropProcessCapability()) {
        TIZEN_LOGI("failed to drop CAP_MAC_ADMIN");
        return 1;
    }
#endif

    if (!eina_init())
        return 1;

    if (!ecore_init()) {
        // Could not init ecore.
        eina_shutdown();
        return 1;
    }

    if (!ecore_x_init(0)) {
        // Could not init ecore_x.
        // PlatformScreenEfl and systemBeep() functions
        // depend on ecore_x functionality.
        ecore_shutdown();
        eina_shutdown();
        return 1;
    }

    if (!ecore_file_init()) {
        ecore_x_shutdown();
        ecore_shutdown();
        eina_shutdown();
        return 1;
    }

#if ENABLE(TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_TEXT_SELECTION)
    // Elementary uses HOME path to get the elementary profile
    // WRT might launch WebProcess as root.
    // In this case, the logic in elm_init will access "/opt/home/root" instead of "/opt/home/app".
    // So, HOME should be set as "/opt/home/app" even though WebProcess is launched as root
    setenv("HOME", "/opt/home/app", 1);
    if (!elm_init(0 ,0)) {
        //edje_shutdown();
        ecore_file_shutdown();
        ecore_x_shutdown();
        ecore_shutdown();
        eina_shutdown();
        return 1;
    }

    ea_theme_changeable_ui_enabled_set(EINA_TRUE);
    ea_theme_style_set(EA_THEME_STYLE_LIGHT);

    setenv("HOME", "/opt/home/root", 1);
#endif

#if ENABLE(TIZEN_USE_STACKPOINTER_AT_WEBPROCESS_STARTUP)
    void* dummy;
    wtfThreadData().setJsStackOrigin(&dummy);
#endif

#if ENABLE(GLIB_SUPPORT)
    g_type_init();

    if (!ecore_main_loop_glib_integrate())
        return 1;
#endif

#if HAVE(ACCESSIBILITY)
    WebAccessibility::instance().init();
#endif

    JSC::initializeThreading();
    WTF::initializeMainThread();

    RunLoop::initializeMainRunLoop();

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    const char* httpProxy = getenv("http_proxy");
    if (httpProxy) {
        const char* noProxy = getenv("no_proxy");
        SoupProxyURIResolver* resolverEfl = soupProxyResolverWkNew(httpProxy, noProxy);
        soup_session_add_feature(session, SOUP_SESSION_FEATURE(resolverEfl));
        g_object_unref(resolverEfl);
    }

#if ENABLE(TIZEN_NATIVE_MEMORY_SNAPSHOT)
    const char* isMemorySnapshot = getenv("TIZEN_MEMORY_SNAPSHOT");
    if (isMemorySnapshot && isMemorySnapshot[0] != '0')
        ecore_file_monitor_add("/tmp/webkit_memory_dump", webkit_memory_dump_cb, NULL);
#endif

#if USE(ACCELERATED_COMPOSITING)
    WebCore::WebGraphicsLayer::initFactory();
#endif

    int socket = atoi(argv[1]);
    WebProcess::shared().initialize(socket, RunLoop::main());
    RunLoop::run();

#if ENABLE(TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_TEXT_SELECTION)
    elm_shutdown();
#endif

    ecore_file_shutdown();
    ecore_x_shutdown();
    ecore_shutdown();
    eina_shutdown();

#if HAVE(ACCESSIBILITY)
    WebAccessibility::instance().cleanup();
#endif

    return 0;

}

} // namespace WebKit
