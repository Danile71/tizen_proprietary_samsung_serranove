/*
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2011 Apple Inc.
 * Copyright (C) 2011 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PluginProcessMainEfl.h"

#include "PluginProcess.h"
#include <Ecore.h>
#include <Ecore_X.h>
#include <WebCore/RunLoop.h>
#include <runtime/InitializeThreading.h>
#include <wtf/MainThread.h>

#if defined(XP_UNIX)
#include <X11/Xlib.h>
#endif

#if ENABLE(TIZEN_SCAN_PLUGIN)
#include "NetscapePlugin.h"
#endif // ENABLE(TIZEN_SCAN_PLUGIN)

#if ENABLE(TIZEN_PROCESS_PERMISSION_CONTROL)
#include "ProcessSmackLabel.h"
#endif

#if ENABLE(TIZEN_PLUGIN_SILENT_CRASH)
#include <signal.h>
#include <sys/prctl.h>
#endif

using namespace WebCore;

namespace WebKit {

#if defined(XP_UNIX)
static int webkiteflXError(Display* xdisplay, XErrorEvent* error)
{
    char errorMessage[64];
    XGetErrorText(xdisplay, error->error_code, errorMessage, 63);
    printf("The program received an X Window System error.\n"
              "This probably reflects a bug in a browser plugin.\n"
              "The error was '%s'.\n"
              "  (Details: serial %ld error_code %d request_code %d minor_code %d)\n",
              errorMessage,
              error->serial, error->error_code,
              error->request_code, error->minor_code);
    return 0;
}
#endif

#if ENABLE(TIZEN_PLUGIN_SILENT_CRASH)
static void signalHandler(int signal)
{
    TIZEN_LOGE("Exit due to signal: %d", signal);
    exit(0);
}
#endif

WK_EXPORT int PluginProcessMainEfl(int argc, char* argv[])
{
#if ENABLE(TIZEN_SCAN_PLUGIN)
    ASSERT(argc == 2 || argc == 3);
    bool scanPlugin = !strcmp(argv[1], "-scanPlugin");
    ASSERT(argc == 2 || (argc == 3 && scanPlugin));
#else
    ASSERT(argc == 2);
#endif // ENABLE(TIZEN_SCAN_PLUGIN)

#if ENABLE(TIZEN_PROCESS_PERMISSION_CONTROL) && !ENABLE(TIZEN_WEBKIT2_EFL_WTR)
    // check process smack label
    if (!changeProcessSmackLabel("/usr/bin/PluginProcess", argv[0])) {
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

    if (!ecore_x_init(0))
        return 1;

    JSC::initializeThreading();
    WTF::initializeMainThread();
#if !LOG_DISABLED && ENABLE(TIZEN_LOG)
    WebCore::initializeLoggingChannelsIfNecessary();
#endif

#if ENABLE(TIZEN_SCAN_PLUGIN)
    if (scanPlugin) {
#if ENABLE(TIZEN_LOG)
        TIZEN_LOGI("[Plugins] scan plugin");
#endif
        String pluginPath(argv[2]);
        if (!NetscapePluginModule::scanPlugin(pluginPath))
            return 1;
        return 0;
    }
#endif // ENABLE(TIZEN_SCAN_PLUGIN)

    RunLoop::initializeMainRunLoop();

#if defined(XP_UNIX)
    XSetErrorHandler(webkiteflXError);
#endif

#if ENABLE(TIZEN_PLUGIN_SILENT_CRASH)
    struct sigaction sa;
    sa.sa_handler = signalHandler;
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);

    prctl(PR_SET_DUMPABLE, 0);
#endif

    int socket = atoi(argv[1]);
    WebKit::PluginProcess::shared().initialize(socket, RunLoop::main());
    RunLoop::run();

    return 0;
}

} // namespace WebKit
