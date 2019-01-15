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

#if ENABLE(TIZEN_JPEGIMAGE_DECODING_THREAD)
#include "AsyncImageDecoder.h"
#include "CachedImage.h"
#include "MemoryCache.h"

namespace WebCore {

AsyncImageDecoder& AsyncImageDecoder::sharedAsyncImageDecoder()
{
    DEFINE_STATIC_LOCAL(AsyncImageDecoder, asyncImageDecoderInstance, ());
    return asyncImageDecoderInstance;
}

AsyncImageDecoder::AsyncImageDecoder()
{
    // Start worker thread.
    MutexLocker lock(m_threadCreationMutex);
    m_threadID = createThread(AsyncImageDecoder::threadEntry, this, "Image Decoder");
}

AsyncImageDecoder::~AsyncImageDecoder()
{
    m_queue.kill();

    // Stop thread.
    waitForThreadCompletion(m_threadID);
    m_threadID = 0;
}

void AsyncImageDecoder::decodeAsync(PassRefPtr<Image> image)
{
    ASSERT(isMainThread());

    OwnPtr<ImageDecodingTask> decodingTask = ImageDecodingTask::create(image);
    m_queue.append(decodingTask.release()); // note that ownership of the task is effectively taken by the queue.
}

// Asynchronously decode in this thread.
void AsyncImageDecoder::threadEntry(void* threadData)
{
    ASSERT(threadData);
    AsyncImageDecoder* imageDecoder = reinterpret_cast<AsyncImageDecoder*>(threadData);
    imageDecoder->runLoop();
}

void AsyncImageDecoder::runLoop()
{
    ASSERT(!isMainThread());

    {
        // Wait for until we have m_threadID established before starting the run loop.
        MutexLocker lock(m_threadCreationMutex);
    }

    // Keep running decoding tasks until we're killed.
    while (OwnPtr<ImageDecodingTask> decodingTask = m_queue.waitForMessage()) {
        if (decodingTask.get()->isMemoryCacheAvailable())
            decodingTask.leakPtr()->decode();
    }
}

PassOwnPtr<AsyncImageDecoder::ImageDecodingTask> AsyncImageDecoder::ImageDecodingTask::create(PassRefPtr<Image> image)
{
    return adoptPtr(new ImageDecodingTask(image));
}

AsyncImageDecoder::ImageDecodingTask::ImageDecodingTask(PassRefPtr<Image> image)
{
    m_asyncDecoderImage = image;
}

void AsyncImageDecoder::ImageDecodingTask::decode()
{
    //Because all jpeg image has only one frame, we use fixed frame index.
    m_asyncDecoderImage->cacheFrame(0);
}

bool AsyncImageDecoder::ImageDecodingTask::isMemoryCacheAvailable()
{
    return memoryCache()->isSizeAvailable();
}

} // namespace WebCore

#endif