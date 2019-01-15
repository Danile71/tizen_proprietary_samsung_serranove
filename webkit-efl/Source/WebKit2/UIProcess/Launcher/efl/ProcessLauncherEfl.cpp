/*
    Copyright (C) 2012 Samsung Electronics

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
#include "ProcessLauncher.h"

#include "Connection.h"
#include "ProcessExecutablePath.h"
#include <WebCore/FileSystem.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/RunLoop.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if ENABLE(TIZEN_WEBPROCESS_MEM_TRACK)
#include <aul.h>
#include <unistd.h>
#endif

#if ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)
#include "WebProcessMainEfl.h"

extern int _ewkArgc;
extern char** _ewkArgv;
#endif

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)

enum {
    PROCESS_TYPE_NONE,
    PARENT_PROCESS,
    CHILD_PROCESS,
};

class StatusForWRTLaunchingPerformance {
public:
    StatusForWRTLaunchingPerformance();
    void setSkipExec(bool skipExec) { m_skipExec = skipExec; }
    bool isInitialFork() { return m_isInitialFork; }
    void setInitialForkAsFalse() { m_isInitialFork = false; }
    bool skipExec() { return m_skipExec; }
    bool isParentProcess() { return m_processType == PARENT_PROCESS; }
    bool isChildProcess() { return m_processType == CHILD_PROCESS; }
    pid_t pid() { return m_pid; }
    void forkProcess();
    void callWebProcessMain();
    int socket(int index) { return m_sockets[index]; }

private:
    bool m_isInitialFork;
    bool m_skipExec;
    int m_processType;
    char* m_argv[3];
    pid_t m_pid;
    int m_sockets[2];
};

StatusForWRTLaunchingPerformance::StatusForWRTLaunchingPerformance()
    : m_isInitialFork(true)
    , m_skipExec(false)
    , m_processType(PROCESS_TYPE_NONE)
    , m_pid(0)
{
    m_argv[0] = m_argv[1] = m_argv[2] = 0;
    m_sockets[0] = m_sockets[1] = -1;
}

void StatusForWRTLaunchingPerformance::forkProcess()
{
    if (!m_isInitialFork || !m_skipExec) {
        m_processType = PARENT_PROCESS;
        return;
    }

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_sockets) < 0) {
        ASSERT_NOT_REACHED();
        return;
    }

    m_pid = fork();
    if (!m_pid) { // child process
        close(m_sockets[1]);
        m_processType = CHILD_PROCESS;
    }
    else if (m_pid > 0) {
        close(m_sockets[0]);
        m_processType = PARENT_PROCESS;
    }
    else {
        ASSERT_NOT_REACHED();
        return;
    }
}

void StatusForWRTLaunchingPerformance::callWebProcessMain()
{
    if (m_processType == CHILD_PROCESS) {
        String socket = String::format("%d", m_sockets[0]);
        String executablePath = "/usr/bin/WebProcess";
        m_argv[0] = strdup(executablePath.utf8().data());
        if (_ewkArgc > 0 && _ewkArgv && _ewkArgv[0]) {
            int maxLen = strlen(_ewkArgv[0]);
            if (maxLen > 0) {
                strncpy(_ewkArgv[0], m_argv[0], maxLen);
                _ewkArgv[0][maxLen] = 0;
            }
        }
        m_argv[1] = strdup(socket.utf8().data());

        char* wrtLaunchingPerformanceExec = getenv("WRT_LAUNCHING_PERFORMANCE_EXEC");
        if (wrtLaunchingPerformanceExec && !strcmp(wrtLaunchingPerformanceExec, "1")) {
            TIZEN_LOGI("exec WebProcess");
            execl(executablePath.utf8().data(), executablePath.utf8().data(), socket.utf8().data(), static_cast<char*>(0));
        } else {
            TIZEN_LOGI("call WebProcessMainEfl");
            WebProcessMainEfl(2, m_argv);
        }
    }
}

static StatusForWRTLaunchingPerformance& getStatus()
{
    static StatusForWRTLaunchingPerformance status;
    return status;
}

void ProcessLauncher::setSkipExec(bool skipExec)
{
    getStatus().setSkipExec(skipExec);
}

bool ProcessLauncher::isInitialFork()
{
    return getStatus().isInitialFork();
}

bool ProcessLauncher::isParentProcess()
{
    return getStatus().isParentProcess();
}

bool ProcessLauncher::isChildProcess()
{
    return getStatus().isChildProcess();
}

void ProcessLauncher::forkProcess()
{
    getStatus().forkProcess();
}

void ProcessLauncher::callWebProcessMain()
{
    getStatus().callWebProcessMain();
}
#endif

void ProcessLauncher::launchProcess()
{
#if ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)
    if (getStatus().isInitialFork() && getStatus().skipExec()) {
        getStatus().setInitialForkAsFalse();
        if (getStatus().isParentProcess()) { // parent process;
            m_processIdentifier = getStatus().pid();
            // We've finished launching the process, message back to the main run loop.
            RunLoop::main()->dispatch(WTF::bind(&ProcessLauncher::didFinishLaunchingProcess, this, getStatus().pid() , getStatus().socket(1)));
        }
        return;
    }
#endif

    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        ASSERT_NOT_REACHED();
        return;
    }

    pid_t pid = fork();
#if ENABLE(TIZEN_WRT_LAUNCHING_PERFORMANCE)
    getStatus().setInitialForkAsFalse();
#endif
    if (!pid) { // child process
        close(sockets[1]);
        String socket = String::format("%d", sockets[0]);
        String executablePath;
        switch (m_launchOptions.processType) {
        case WebProcess:
#if ENABLE(TIZEN_PROCESS_PERMISSION_CONTROL)
            if (!m_launchOptions.customExecutablePath.isEmpty()) {
                executablePath = m_launchOptions.customExecutablePath;
                TIZEN_LOGI("web process executable path: %s", executablePath.utf8().data());
            } else
#endif
                executablePath = executablePathOfWebProcess();
            break;
        case PluginProcess:
#if ENABLE(TIZEN_PROCESS_PERMISSION_CONTROL)
            {
                // this env is used for tizen wrt process pool, and has highest priority for use.
                const char* pathForProcessPool = getenv("PLUGIN_PROCESS_EXECUTABLE_PATH_FOR_PROCESS_POOL");
                if (pathForProcessPool) {
                    executablePath = String::fromUTF8(pathForProcessPool);
                    TIZEN_LOGI("plugin process executable path: %s", executablePath.utf8().data());
                    break;
                }
            }

            if (!m_launchOptions.customExecutablePath.isEmpty()) {
                executablePath = m_launchOptions.customExecutablePath;
                TIZEN_LOGI("plugin process executable path: %s", executablePath.utf8().data());
            } else
#endif
                executablePath = executablePathOfPluginProcess();
            break;
        default:
            ASSERT_NOT_REACHED();
            return;
        }

#ifndef NDEBUG
        if (m_launchOptions.processCmdPrefix.isEmpty())
#endif
            execl(executablePath.utf8().data(), executablePath.utf8().data(), socket.utf8().data(), static_cast<char*>(0));
#ifndef NDEBUG
        else {
            String cmd = makeString(m_launchOptions.processCmdPrefix, ' ', executablePath, ' ', socket);
            if (system(cmd.utf8().data()) == -1) {
                ASSERT_NOT_REACHED();
                return;
            }
        }
#endif
    } else if (pid > 0) { // parent process;
        close(sockets[0]);
        m_processIdentifier = pid;
        // We've finished launching the process, message back to the main run loop.
#if ENABLE(TIZEN_WEBPROCESS_MEM_TRACK)
        char* ignoreWebProcessMemTrack = getenv("IGNORE_WEBPROCESS_MEM_TRACKING");
        if (!ignoreWebProcessMemTrack || strcmp(ignoreWebProcessMemTrack, "1"))
            aul_set_process_group(getpid(), pid);
#endif
        RunLoop::main()->dispatch(WTF::bind(&ProcessLauncher::didFinishLaunchingProcess, this, pid, sockets[1]));
    } else {
        ASSERT_NOT_REACHED();
        return;
    }
}

void ProcessLauncher::terminateProcess()
{
    if (!m_processIdentifier)
        return;
    kill(m_processIdentifier, SIGKILL);
}

void ProcessLauncher::platformInvalidate()
{
}

} // namespace WebKit
