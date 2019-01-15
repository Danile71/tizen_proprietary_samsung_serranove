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

#if ENABLE(TIZEN_JPEGIMAGE_DECODING_THREAD)

#ifndef AsyncImageDecoder_h
#define AsyncImageDecoder_h

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

class AsyncImageDecoder {
    WTF_MAKE_NONCOPYABLE(AsyncImageDecoder);
public:
    AsyncImageDecoder();
    ~AsyncImageDecoder();

    static AsyncImageDecoder& sharedAsyncImageDecoder();

    // Must be called on the main thread.
    void decodeAsync(PassRefPtr<Image> image);

private:
    class ImageDecodingTask {
        WTF_MAKE_NONCOPYABLE(ImageDecodingTask);
        friend class MemoryCache;
    public:
        static PassOwnPtr<ImageDecodingTask> create(PassRefPtr<Image> image);
        void decode();
        bool isMemoryCacheAvailable();

    private:
        ImageDecodingTask(PassRefPtr<Image> image);

        static void notifyCompleteDispatch(void* userData);
        void notifyComplete();

        RefPtr<Image> m_asyncDecoderImage;
    };

    static void threadEntry(void* threadData);
    void runLoop();

    WTF::ThreadIdentifier m_threadID;
    Mutex m_threadCreationMutex;
    MessageQueue<ImageDecodingTask> m_queue;
};

} // namespace WebCore

#endif // AsyncImageDecoder_h

#endif
