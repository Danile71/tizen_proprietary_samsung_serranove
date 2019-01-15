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

#ifndef AsyncFileWriterClientTizen_h
#define AsyncFileWriterClientTizen_h

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "AsyncFileSystemTaskControllerTizen.h"
#include "AsyncFileWriterClient.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class AsyncFileWriterClientTizen : public AsyncFileWriterClient {
public:
    static PassOwnPtr<AsyncFileWriterClientTizen> create(AsyncFileSystemTaskControllerTizen* taskController, AsyncFileWriterClient* client, const String& mode)
    {
        return adoptPtr(new AsyncFileWriterClientTizen(taskController, client, mode));
    }

    void didWrite(long long bytes, bool complete);
    void didTruncate();
    void didFail(FileError::ErrorCode);

private:
    AsyncFileWriterClientTizen(AsyncFileSystemTaskControllerTizen*, AsyncFileWriterClient*, const String&);

    RefPtr<AsyncFileSystemTaskControllerTizen> m_taskController;
    AsyncFileWriterClient* m_client;
    String m_taskMode;
};

} // namespace

#endif // ENABLE(FILE_SYSTEM)

#endif // AsyncFileWriterClientTizen_h

