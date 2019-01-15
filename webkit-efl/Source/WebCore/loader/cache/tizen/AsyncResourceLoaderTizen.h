/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)

#ifndef AsyncResourceLoader_h
#define AsyncResourceLoader_h

#include "Image.h"
#include <wtf/MessageQueue.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/Threading.h>
#include <wtf/threads/BinarySemaphore.h>

namespace WebCore {

class MemoryCache;
class CachedResource;

class AsyncResourceLoader {
    WTF_MAKE_NONCOPYABLE(AsyncResourceLoader);

public:
    explicit AsyncResourceLoader(MemoryCache* cache);
    ~AsyncResourceLoader();
    void startRestoreThread();

    class DecodingTask {
        WTF_MAKE_NONCOPYABLE(DecodingTask);
    public:
        DecodingTask();
        static PassOwnPtr<DecodingTask> create(CachedResource* cachedResource);
        CachedResource* cachedResource() { return m_resource; }
        bool isfinished() { return m_finishDecode; }

    private:
        DecodingTask(CachedResource* cachedResource);

        CachedResource* m_resource;
        bool m_finishDecode;

    };

    MessageQueue<DecodingTask> m_queue;

private:
    static void threadEntry(void* threadData);
    void restoreResource();
    void restoreFromDisk(CachedResource* resource);

    WTF::ThreadIdentifier m_threadID;
    Mutex m_threadCreationMutex;

    MemoryCache* m_cache;
};

} // namespace WebCore

#endif // AsyncImageDecoder_h

#endif

