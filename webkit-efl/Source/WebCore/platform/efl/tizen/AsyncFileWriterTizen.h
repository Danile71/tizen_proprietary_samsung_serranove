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

#ifndef AsyncFileWriterTizen_h
#define AsyncFileWriterTizen_h

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "AsyncFileSystemTaskControllerTizen.h"
#include "AsyncFileWriter.h"
#include "AsyncFileWriterClientTizen.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class AsyncFileWriterClient;

class AsyncFileWriterTizen : public AsyncFileWriter {
public:
    static PassOwnPtr<AsyncFileWriterTizen> create(AsyncFileWriterClient* client, const String& path, AsyncFileSystemTaskControllerTizen* taskController)
    {
        return adoptPtr(new AsyncFileWriterTizen(client, path, taskController));
    }

    void write(long long position, Blob* data);
    void truncate(long long length);
    void abort();

    bool waitForOperationToComplete();

private:
    AsyncFileWriterTizen(AsyncFileWriterClient*, const String& path, AsyncFileSystemTaskControllerTizen*);

    AsyncFileWriterClient* m_client;
    String m_path;
    RefPtr<AsyncFileSystemTaskControllerTizen> m_taskController;

};

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

#endif // AsyncFileWriterTizen_h

