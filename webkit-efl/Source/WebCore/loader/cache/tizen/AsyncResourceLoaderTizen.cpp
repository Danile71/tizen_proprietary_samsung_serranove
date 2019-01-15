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

#include "config.h"
#include "AsyncResourceLoaderTizen.h"

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
#include "CachedImage.h"
#include "MemoryCache.h"
#include "CachedResource.h"
#include "SharedBuffer.h"
#include "GCController.h"
#include <stdio.h>

namespace WebCore {
AsyncResourceLoader::AsyncResourceLoader(MemoryCache* cache)
    : m_cache(cache)
{
}

AsyncResourceLoader::~AsyncResourceLoader()
{
    m_queue.kill();

    // Stop thread.
    waitForThreadCompletion(m_threadID);
    m_threadID = 0;
}

void AsyncResourceLoader::startRestoreThread()
{
    // Start worker thread.
    MutexLocker lock(m_threadCreationMutex);
    m_threadID = createThread(AsyncResourceLoader::threadEntry, this, "Resource Loader");
}

// Restore asynchronously in this thread.
void AsyncResourceLoader::threadEntry(void* threadData)
{
    ASSERT(threadData);
    AsyncResourceLoader* resourceLoader = reinterpret_cast<AsyncResourceLoader*>(threadData);
    resourceLoader->restoreResource();
}

void AsyncResourceLoader::restoreResource()
{
    ASSERT(!isMainThread());

    MutexLocker lock(m_threadCreationMutex);

    m_cache->restoreFinished(false);

    Vector<MemoryCache::LRUList, 32> resources = m_cache->resources();
    int size = resources.size();

    // Restore images in viewport
    for (int i = size - 1; i >= 0; i--) {
        CachedResource* current = resources[i].m_tail;
        while (current) {
            CachedResource* prev = current->prevInAllResourcesList();
            if (current->isLocatedInViewport()) {
                restoreFromDisk(current);
            }
            current = prev;
        }
    }

    OwnPtr<DecodingTask> finishTask = adoptPtr(new DecodingTask());
    m_queue.append(finishTask.release());

    // Restore the rest
    for (int i = size - 1; i >= 0; i--) {
        CachedResource* current = resources[i].m_tail;
        while (current) {
            CachedResource* prev = current->prevInAllResourcesList();
            if (!(current->isLocatedInViewport())) {
                restoreFromDisk(current);
            }
            current = prev;
        }
    }
    m_cache->setRestoredSize(0);
    m_cache->restoreFinished(true);

    detachThread(m_threadID);
}

void AsyncResourceLoader::restoreFromDisk(CachedResource* resource)
{
    if (resource->data()) {
        if (resource->data()->restoreFromCacheIfAvailable()) {
            unsigned size = m_cache->restoredSize();
            size += resource->encodedSize();
            m_cache->setRestoredSize(size);
        }
    }
}

PassOwnPtr<AsyncResourceLoader::DecodingTask> AsyncResourceLoader::DecodingTask::create(CachedResource* cachedResource)
{
    return adoptPtr(new DecodingTask(cachedResource));
}

AsyncResourceLoader::DecodingTask::DecodingTask(CachedResource* cachedResource)
    : m_finishDecode(false)
{
    m_resource = cachedResource;
}

AsyncResourceLoader::DecodingTask::DecodingTask()
    : m_finishDecode(true)
{
}


} // namespace WebCore

#endif
