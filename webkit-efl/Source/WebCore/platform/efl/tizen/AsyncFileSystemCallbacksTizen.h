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

#ifndef AsyncFileSystemCallbacksTizen_h
#define AsyncFileSystemCallbacksTizen_h

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "AsyncFileSystemCallbacks.h"
#include "AsyncFileSystemTaskControllerTizen.h"
#include "FileSystemType.h"
#include "ScriptExecutionContext.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class AsyncFileSystemCallbacksTizen : public AsyncFileSystemCallbacks {
public:
    static PassOwnPtr<AsyncFileSystemCallbacksTizen> create(ScriptExecutionContext* context, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
    {
        return adoptPtr(new AsyncFileSystemCallbacksTizen(context, callbacks, synchronousType));
    }

    static PassOwnPtr<AsyncFileSystemCallbacksTizen> create(AsyncFileSystemTaskControllerTizen* taskController, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, const String& mode)
    {
        return adoptPtr(new AsyncFileSystemCallbacksTizen(taskController, callbacks, mode));
    }


    virtual void didSucceed();
    virtual void didOpenFileSystem(const String& name, const KURL& rootURL, PassOwnPtr<AsyncFileSystem>);
    virtual void didReadMetadata(const FileMetadata&);
    virtual void didReadDirectoryEntry(const String& name, bool isDirectory);
    virtual void didReadDirectoryEntries(bool hasMore);
    virtual void didCreateFileWriter(PassOwnPtr<AsyncFileWriter>, long long length);
    virtual void didFail(int code);

private:
    AsyncFileSystemCallbacksTizen(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacks>, FileSystemSynchronousType);
    AsyncFileSystemCallbacksTizen(AsyncFileSystemTaskControllerTizen*, PassOwnPtr<AsyncFileSystemCallbacks>, const String&);

    RefPtr<AsyncFileSystemTaskControllerTizen> m_taskController;
    OwnPtr<AsyncFileSystemCallbacks> m_callbacks;
    FileSystemSynchronousType m_synchronousType;
    String m_taskMode;

    struct DirectoryEntry {
        String name;
        bool isDirectory;

        DirectoryEntry() : isDirectory(false) { }
    };

    Vector<DirectoryEntry> m_directoryEntries;
};

} // namespace

#endif // ENABLE(FILE_SYSTEM)

#endif // AsyncFileSystemCallbacksTizen_h

