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
#include "AsyncFileSystemCallbacksTizen.h"

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "AsyncFileSystemTizen.h"
#include "AsyncFileWriter.h"
#include "CrossThreadTask.h"
#include "FileMetadata.h"

#include <wtf/text/CString.h>

namespace WebCore {

template<> struct CrossThreadCopierBase<false, false, FileMetadata> {
    typedef FileMetadata Type;
    static FileMetadata copy(const FileMetadata&);
};

CrossThreadCopierBase<false, false, FileMetadata>::Type CrossThreadCopierBase<false, false, FileMetadata>::copy(const FileMetadata& original)
{
    FileMetadata metadata;
    metadata.modificationTime = original.modificationTime;
    metadata.length = original.length;
    metadata.type = original.type;
    metadata.platformPath = original.platformPath;
    return metadata;
}

static void performDidSucceedCallback(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    callbacks->didSucceed();
}

static void performDidOpenFileSystemCallback(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, const String& name, const KURL& rootURL, PassOwnPtr<AsyncFileSystem> fileSystem)
{
    callbacks->didOpenFileSystem(name, rootURL, fileSystem);
}

static void performDidReadMetadataCallback(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, const FileMetadata& metadata)
{
    callbacks->didReadMetadata(metadata);
}

static void performDidReadDirectoryEntriesAndFinishedCallback(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    callbacks->didReadDirectoryEntries(false);
}

static void performDidCreateFileWriterCallback(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, PassOwnPtr<AsyncFileWriter> writer, long long length)
{
    callbacks->didCreateFileWriter(writer, length);
}

static void performDidFailCallback(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, int code)
{
    callbacks->didFail(code);
}

AsyncFileSystemCallbacksTizen::AsyncFileSystemCallbacksTizen(ScriptExecutionContext* context, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
    : m_taskController(AsyncFileSystemTaskControllerTizen::create(context, synchronousType))
    , m_callbacks(callbacks)
    , m_synchronousType(synchronousType)
    , m_taskMode(String())
{
}

AsyncFileSystemCallbacksTizen::AsyncFileSystemCallbacksTizen(AsyncFileSystemTaskControllerTizen* taskController, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, const String& mode)
    : m_taskController(taskController)
    , m_callbacks(callbacks)
    , m_synchronousType(AsynchronousFileSystem)
    , m_taskMode(mode)
{
}

void AsyncFileSystemCallbacksTizen::didSucceed()
{
    m_taskController->postCallbackTask(createCallbackTask(&performDidSucceedCallback, m_callbacks.release()), m_taskMode);
}

void AsyncFileSystemCallbacksTizen::didOpenFileSystem(const String& name, const KURL& rootURL, PassOwnPtr<AsyncFileSystem> fileSystem)
{
    AsyncFileSystemTizen* fileSystemTizen = static_cast<AsyncFileSystemTizen*>(fileSystem.get());

    String mode = m_taskController->uniqueMode();
    m_taskController->setCurrentTaskMode(mode);
    m_taskController->postCallbackTask(createCallbackTask(&performDidOpenFileSystemCallback, m_callbacks.release(), name, rootURL, fileSystem), mode);
    fileSystemTizen->setTaskController(m_taskController.release());
    fileSystemTizen->waitForOperationToComplete();
}

void AsyncFileSystemCallbacksTizen::didReadMetadata(const FileMetadata& metadata)
{
    m_taskController->postCallbackTask(createCallbackTask(&performDidReadMetadataCallback, m_callbacks.release(), metadata), m_taskMode);
}

void AsyncFileSystemCallbacksTizen::didReadDirectoryEntry(const String& name, bool isDirectory)
{
    DirectoryEntry directoryEntry;
    directoryEntry.name = name;
    directoryEntry.isDirectory = isDirectory;
    m_directoryEntries.append(directoryEntry);
}

void AsyncFileSystemCallbacksTizen::didReadDirectoryEntries(bool hasMore)
{
    if (hasMore)
        ASSERT_NOT_REACHED();

    Vector<DirectoryEntry>::iterator end = m_directoryEntries.end();
    for (Vector<DirectoryEntry>::iterator it = m_directoryEntries.begin(); it != end; ++it)
        m_callbacks->didReadDirectoryEntry(it->name, it->isDirectory);

    m_directoryEntries.clear();

    m_taskController->postCallbackTask(createCallbackTask(&performDidReadDirectoryEntriesAndFinishedCallback, m_callbacks.release()), m_taskMode);
}

void AsyncFileSystemCallbacksTizen::didCreateFileWriter(PassOwnPtr<AsyncFileWriter> writer, long long length)
{
    m_taskController->postCallbackTask(createCallbackTask(&performDidCreateFileWriterCallback, m_callbacks.release(), writer, length), m_taskMode);
}

void AsyncFileSystemCallbacksTizen::didFail(int code)
{
    m_taskController->postCallbackTask(createCallbackTask(&performDidFailCallback, m_callbacks.release(), code), m_taskMode);
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)


