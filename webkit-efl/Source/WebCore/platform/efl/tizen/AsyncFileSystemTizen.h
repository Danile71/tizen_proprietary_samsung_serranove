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

#ifndef AsyncFileSystemTizen_h
#define AsyncFileSystemTizen_h

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "AsyncFileSystem.h"

#include "AsyncFileSystemTaskControllerTizen.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

class AsyncFileSystemTizen : public AsyncFileSystem {
public:
    AsyncFileSystemTizen();
    ~AsyncFileSystemTizen();

    void stop();
    bool hasPendingActivity();
    bool waitForOperationToComplete();

    void move(const KURL& srcPath, const KURL& destPath, PassOwnPtr<AsyncFileSystemCallbacks>);
    void copy(const KURL& srcPath, const KURL& destPath, PassOwnPtr<AsyncFileSystemCallbacks>);
    void remove(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    void removeRecursively(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    void readMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    void createFile(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks>);
    void createDirectory(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks>);
    void fileExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    void directoryExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    void readDirectory(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);
    void createWriter(AsyncFileWriterClient*, const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);

    void createSnapshotFileAndReadMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks>);

    void setTaskController(PassRefPtr<AsyncFileSystemTaskControllerTizen>);

    static String virtualPathToFileSystemPath(const KURL&);
    static bool checkQuota(ScriptExecutionContext*, const String& path, int& errorCode, Blob* data = NULL);

private:
    String m_localFileSystemBasePath;

    RefPtr<AsyncFileSystemTaskControllerTizen> m_taskController;
};

}

#endif // ENABLE(FILE_SYSTEM)

#endif // AsyncFileSystemTizen_h

