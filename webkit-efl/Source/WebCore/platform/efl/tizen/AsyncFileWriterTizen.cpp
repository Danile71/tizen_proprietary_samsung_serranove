/*
 * Copyright (C) 2011 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "AsyncFileWriterTizen.h"

#include "NotImplemented.h"

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "AsyncFileSystemTizen.h"
#include "AsyncFileWriterClientTizen.h"
#include "Blob.h"
#include "BlobRegistryImpl.h"
#include "CrossThreadTask.h"
#include "FileSystem.h"
#include "GOwnPtr.h"
#include <wtf/text/CString.h>

namespace WebCore {

AsyncFileWriterTizen::AsyncFileWriterTizen(AsyncFileWriterClient* client, const String& path, AsyncFileSystemTaskControllerTizen* taskController)
    : m_client(client)
    , m_path(path)
    , m_taskController(taskController)
{
}

static void writeAsync(ScriptExecutionContext* context, PassOwnPtr<AsyncFileWriterClientTizen> helperClient, const String& path, long long position, Blob* data)
{
    int bytesWritten;
    PlatformFileHandle handle;
    handle = openFile(path, OpenForWriteOnly);

    if (!isHandleValid(handle)) {
        helperClient->didFail(FileError::NOT_FOUND_ERR);
        return;
    }

    int errorCode = 0;
    if (!AsyncFileSystemTizen::checkQuota(context, path, errorCode, data)) {
        closeFile(handle);
        helperClient->didFail(static_cast<FileError::ErrorCode>(errorCode));
        return;
    }

    seekFile(handle, position, SeekFromBeginning);
    RefPtr<BlobStorageData> blobStorage = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobDataFromURL(data->url());
    if (blobStorage) {
        for (size_t i = 0; i < blobStorage->items().size(); i++) {
            const BlobDataItem& blobItem = blobStorage->items()[i];
            if (blobItem.type == BlobDataItem::Data) {
                bytesWritten = 0;
                bytesWritten = writeToFile(handle, blobItem.data->data(), blobItem.data->length());

                if (bytesWritten != static_cast<int>(blobItem.data->length()))
                    helperClient->didFail(FileError::SECURITY_ERR);
                else
                    helperClient->didWrite(bytesWritten, (i + 1 == blobStorage->items().size()) ? true : false);
            } else if (blobItem.type == BlobDataItem::File) {
                closeFile(handle);

                if (copyFile(blobItem.path, path))
                    helperClient->didWrite(blobItem.length, (i + 1 == blobStorage->items().size()) ? true : false);
                else
                    helperClient->didFail(FileError::SECURITY_ERR);
            } else if (blobItem.type == BlobDataItem::Blob) {
                LOG_ERROR("[writeAsync] data item is blob");
                notImplemented();
            }
        }
    }

    closeFile(handle);
}

void AsyncFileWriterTizen::write(long long position, Blob* data)
{
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&writeAsync, AsyncFileWriterClientTizen::create(m_taskController.get(), m_client, mode), m_path, position, AllowCrossThreadAccess(data)));
}

static void truncateAsync(ScriptExecutionContext*, PassOwnPtr<AsyncFileWriterClientTizen> helperClient, const String& path, long long length)
{
    if (!fileExists(path)) {
        helperClient->didFail(FileError::NOT_FOUND_ERR);
        return;
    }

    PlatformFileHandle handle;
    handle = openFile(path, OpenForWrite);

    if (!isHandleValid(handle))
        return;

    if (truncateFile(handle, length))
        helperClient->didTruncate();
    else
        helperClient->didFail(FileError::SECURITY_ERR);

    closeFile(handle);
}

void AsyncFileWriterTizen::truncate(long long length)
{
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&truncateAsync, AsyncFileWriterClientTizen::create(m_taskController.get(), m_client, mode), m_path, length));
}

void AsyncFileWriterTizen::abort()
{
}

bool AsyncFileWriterTizen::waitForOperationToComplete()
{
    if (m_taskController->synchronousType() == SynchronousFileSystem)
        return m_taskController->waitForTaskToComplete();

    return false;
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

