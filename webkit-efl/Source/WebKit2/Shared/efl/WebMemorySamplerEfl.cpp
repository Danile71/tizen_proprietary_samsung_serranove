/*
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"
#include "WebMemorySampler.h"

#if ENABLE(MEMORY_SAMPLER)

#include <JavaScriptCore/MemoryStatistics.h>
#include <malloc.h>
#include <runtime/JSLock.h>
#include <WebCore/JSDOMWindow.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;
using namespace JSC;
using namespace WTF;

namespace WebKit {

typedef struct ApplicationMemoryStats {
    size_t totalProgramSize;
    size_t residentSetSize;
    size_t proportionalSetSize;
    size_t peak;
    size_t sharedSize;
    size_t textSize;
    size_t librarySize;
    size_t dataStackSize;
    size_t dirtyPageSize;
    size_t UMPSize;
    size_t graphics3DSize;
    size_t privateCleanSize;
    size_t privateDirtySize;
} ApplicationMemoryStats;

const int MAX_BUFFER = 128;
const int MAX_PROCESS_PATH = 35;

static String getToken(FILE* file)
{
    if (!file)
        return String();

    char buffer[MAX_BUFFER];
    memset(buffer, 0, MAX_BUFFER);

    if (fscanf(file, "%s", buffer) > 0)
        return String(buffer);

    return String();
}

void sampleSystemMemoryInfo(WebMemoryStatistics& statics)
{
    bool foundKeyName = false;
    FILE* fSystemMemoryStatus = fopen("/proc/meminfo", "r");
    if (fSystemMemoryStatus) {
        while (!feof(fSystemMemoryStatus)) {
            String strToken = getToken(fSystemMemoryStatus);
            if (strToken.find(':') != notFound) {
                String keyName = strToken.left(strToken.length() - 1);
                statics.keys.append(keyName);
                foundKeyName = true;
            } else if (foundKeyName) {
                statics.values.append(strToken.toInt());
                foundKeyName = false;
            }
        }
        fclose(fSystemMemoryStatus);
    }
}

static unsigned getPeakRSS(int pid)
{
    char processPath[MAX_PROCESS_PATH];
    sprintf(processPath, "/proc/%d/status", pid);

    FILE* fStatus = fopen(processPath, "r");
    if (!fStatus) {
        fprintf(stderr, "cannot open %s\n", processPath);
        return 0;
    }

    unsigned size = 0;
    while (!feof(fStatus)) {
        String strToken = getToken(fStatus);
        if (strToken.find("VmHWM") != notFound) {
            size = getToken(fStatus).toUInt();
            break;
        }
    }

    fclose(fStatus);
    return size;
}

void sampleDeviceMemoryMalloc(ApplicationMemoryStats& applicationStats, int pid)
{
    const char* strUMPZone = "/dev/ump";
    const char* strGraphicsZone = "/dev/mali";
    const char* strPrivateClean = "Private_Clean";
    const char* strPrivateDirty = "Private_Dirty";
    const char* strDeviceDriver = "/dev/dri/card0";
    const char* strProportionalSetSize = "Pss";

    char processPath[MAX_PROCESS_PATH];
    sprintf(processPath, "/proc/%d/smaps", pid);
    FILE* fMAPS = fopen(processPath, "r");
    if (fMAPS) {
        int UMPSize = 0;
        int graphics3DSize = 0;
        int privateCleanSize = 0;
        int privateDirtySize = 0;
        int deviceDriverSize = 0;
        int proportionalSetSize = 0;
        while (!feof(fMAPS)) {
            String strToken = getToken(fMAPS);
            if (strToken.find(strUMPZone) != notFound) {
                getToken(fMAPS);                // Skip "Size:"
                strToken = getToken(fMAPS);     // Actual size
                UMPSize = UMPSize + strToken.toInt();
            }
            if (strToken.find(strGraphicsZone) != notFound) {
                getToken(fMAPS);                // Skip "Size:"
                strToken = getToken(fMAPS);     // Actual size
                graphics3DSize = graphics3DSize + strToken.toInt();
            }
            if (strToken.find(strPrivateClean) != notFound) {
                strToken = getToken(fMAPS);     // Actual size
                privateCleanSize = privateCleanSize + strToken.toInt();
            }
            if (strToken.find(strPrivateDirty) != notFound) {
                strToken = getToken(fMAPS);     // Actual size
                privateDirtySize = privateDirtySize + strToken.toInt();
            }
            if (strToken.find(strDeviceDriver) != notFound) {
                getToken(fMAPS);                // Skip "Size:"
                strToken = getToken(fMAPS);     // Actual size
                deviceDriverSize = deviceDriverSize + strToken.toInt();
            }
            if (strToken.find(strProportionalSetSize) != notFound) {
                strToken = getToken(fMAPS);     // Actual size
                proportionalSetSize += strToken.toInt();
            }
        }
        applicationStats.UMPSize = UMPSize;
        applicationStats.graphics3DSize = graphics3DSize;
        applicationStats.privateCleanSize = privateCleanSize;
        applicationStats.privateDirtySize = privateDirtySize;
        applicationStats.residentSetSize -= deviceDriverSize;
        applicationStats.proportionalSetSize = proportionalSetSize - deviceDriverSize;
        applicationStats.peak = getPeakRSS(pid) - deviceDriverSize;

        fclose(fMAPS);
    }
}

ApplicationMemoryStats sampleApplicationMalloc(int pid)
{
    ApplicationMemoryStats applicationStats;

    char processPath[MAX_PROCESS_PATH];
    sprintf(processPath, "/proc/%d/statm", getpid());
    FILE* fMemoryStatus = fopen(processPath, "r");
    if (fMemoryStatus) {
        int ret = fscanf(fMemoryStatus, "%d %d %d %d %d %d %d",
                &applicationStats.totalProgramSize,
                &applicationStats.residentSetSize,
                &applicationStats.sharedSize,
                &applicationStats.textSize,
                &applicationStats.librarySize,
                &applicationStats.dataStackSize,
                &applicationStats.dirtyPageSize);
        UNUSED_PARAM(ret);

        fclose(fMemoryStatus);
    }

    // Get Additional Memory Info: UMP, 3D Graphics, Device Driver, Private Clean/Dirty, RSS, PSS Bytes
    sampleDeviceMemoryMalloc(applicationStats, pid);

    return applicationStats;
}

String WebMemorySampler::processName() const
{
    String processName;
    char processPath[MAX_PROCESS_PATH];
    sprintf(processPath, "/proc/%d/status", getpid());
    FILE* fProcessStatus = fopen(processPath, "r");
    if (fProcessStatus) {
        getToken(fProcessStatus);
        processName = getToken(fProcessStatus);
        fclose(fProcessStatus);
    }

    return processName;
}

WebMemoryStatistics WebMemorySampler::sampleWebKit() const
{
    size_t totalBytesInUse = 0, totalBytesCommitted = 0;

    ApplicationMemoryStats applicationStats = sampleApplicationMalloc(getpid());

    WebMemoryStatistics webKitMemoryStats;

    FastMallocStatistics fastMallocStatistics = WTF::fastMallocStatistics();
    size_t fastMallocBytesInUse = fastMallocStatistics.committedVMBytes - fastMallocStatistics.freeListBytes;
    size_t fastMallocBytesCommitted = fastMallocStatistics.committedVMBytes;
    totalBytesInUse += fastMallocBytesInUse;
    totalBytesCommitted += fastMallocBytesCommitted;

    JSLockHolder lock(JSDOMWindow::commonJSGlobalData());
    size_t jscHeapBytesInUse = JSDOMWindow::commonJSGlobalData()->heap.size();
    size_t jscHeapBytesCommitted = JSDOMWindow::commonJSGlobalData()->heap.capacity();
    totalBytesInUse += jscHeapBytesInUse;
    totalBytesCommitted += jscHeapBytesCommitted;

    GlobalMemoryStatistics globalMemoryStats = globalMemoryStatistics();
    totalBytesInUse += globalMemoryStats.stackBytes + globalMemoryStats.JITBytes;
    totalBytesCommitted += globalMemoryStats.stackBytes + globalMemoryStats.JITBytes;

    double now = currentTime();

    webKitMemoryStats.keys.append(String("Timestamp"));
    webKitMemoryStats.values.append(now);

    webKitMemoryStats.keys.append(String("Total Program Size"));
    webKitMemoryStats.values.append(applicationStats.totalProgramSize);
    webKitMemoryStats.keys.append(String("RSS"));
    webKitMemoryStats.values.append(applicationStats.residentSetSize);
    webKitMemoryStats.keys.append(String("Shared"));
    webKitMemoryStats.values.append(applicationStats.sharedSize);
    webKitMemoryStats.keys.append(String("Text"));
    webKitMemoryStats.values.append(applicationStats.textSize);
    webKitMemoryStats.keys.append(String("Library"));
    webKitMemoryStats.values.append(applicationStats.librarySize);
    webKitMemoryStats.keys.append(String("Data/Stack"));
    webKitMemoryStats.values.append(applicationStats.dataStackSize);
    webKitMemoryStats.keys.append(String("Dirty"));
    webKitMemoryStats.values.append(applicationStats.dirtyPageSize);
    webKitMemoryStats.keys.append(String("UMP"));
    webKitMemoryStats.values.append(applicationStats.UMPSize);
    webKitMemoryStats.keys.append(String("Graphics 3D"));
    webKitMemoryStats.values.append(applicationStats.graphics3DSize);

    webKitMemoryStats.keys.append(String("Total Memory In Use"));
    webKitMemoryStats.values.append(totalBytesInUse);
    webKitMemoryStats.keys.append(String("Fast Malloc Zone Bytes"));
    webKitMemoryStats.values.append(fastMallocBytesInUse);
    webKitMemoryStats.keys.append(String("JavaScript Heap Bytes"));
    webKitMemoryStats.values.append(jscHeapBytesInUse);

    webKitMemoryStats.keys.append(String("Total Committed Memory"));
    webKitMemoryStats.values.append(totalBytesCommitted);
    webKitMemoryStats.keys.append(String("Fast Malloc Zone Bytes"));
    webKitMemoryStats.values.append(fastMallocBytesCommitted);
    webKitMemoryStats.keys.append(String("JavaScript Heap Bytes"));
    webKitMemoryStats.values.append(jscHeapBytesCommitted);

    webKitMemoryStats.keys.append(String("JavaScript Stack Bytes"));
    webKitMemoryStats.values.append(globalMemoryStats.stackBytes);
    webKitMemoryStats.keys.append(String("JavaScript JIT Bytes"));
    webKitMemoryStats.values.append(globalMemoryStats.JITBytes);

    sampleSystemMemoryInfo(webKitMemoryStats);

    // Code For Watching Memory in RealTime
    if (processName().find("browser") != notFound)
        printf("\033[22;31m");
    else
        printf("\033[01;32m");
    printf("[Process Name %s | TimeStamp %lf]\n", processName().utf8().data(), now);
    printf("\033[22;37m");
    printf("AP Total KB %d | RSS %d | UMP %d | 3D %d \n", applicationStats.totalProgramSize, applicationStats.residentSetSize, applicationStats.UMPSize, applicationStats.graphics3DSize);
    printf("WK Total Bytes (In Use) %d | Fast Malloc %d | JavaScript Heap Bytes %d\n", totalBytesInUse, fastMallocBytesInUse, jscHeapBytesInUse);
    printf("WK Total Bytes (Committed) %d | Fast Malloc %d | JavaScript Heap Bytes %d\n", totalBytesCommitted, fastMallocBytesCommitted, jscHeapBytesCommitted);
    printf("WK JavaScript Stack Bytes %d | JavaScript JIT Bytes %d\n\n", globalMemoryStats.stackBytes, globalMemoryStats.JITBytes);

    return webKitMemoryStats;
}

}

#endif
