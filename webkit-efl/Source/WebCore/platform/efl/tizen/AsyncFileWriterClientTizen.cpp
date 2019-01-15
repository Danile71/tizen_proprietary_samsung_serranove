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
#include "AsyncFileWriterClientTizen.h"

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "CrossThreadTask.h"
#include <stdio.h>

namespace WebCore {

static void performDidWriteCallback(ScriptExecutionContext*, AsyncFileWriterClient* client, long long bytes, bool complete)
{
    client->didWrite(bytes, complete);
}

static void performDidTruncateCallback(ScriptExecutionContext*, AsyncFileWriterClient* client)
{
    client->didTruncate();
}

static void performDidFailCallback(ScriptExecutionContext*, AsyncFileWriterClient* client, FileError::ErrorCode code)
{
    client->didFail(code);
}

AsyncFileWriterClientTizen::AsyncFileWriterClientTizen(AsyncFileSystemTaskControllerTizen* taskController, AsyncFileWriterClient* client, const String& mode)
    : m_taskController(taskController)
    , m_client(client)
    , m_taskMode(mode)
{
    m_taskController->setCurrentTaskMode(m_taskMode);
}

void AsyncFileWriterClientTizen::didWrite(long long bytes, bool complete)
{
    m_taskController->postCallbackTask(createCallbackTask(&performDidWriteCallback, AllowCrossThreadAccess(m_client), bytes, complete), m_taskMode);
}

void AsyncFileWriterClientTizen::didTruncate()
{
    m_taskController->postCallbackTask(createCallbackTask(&performDidTruncateCallback, AllowCrossThreadAccess(m_client)), m_taskMode);
}

void AsyncFileWriterClientTizen::didFail(FileError::ErrorCode code)
{
    m_taskController->postCallbackTask(createCallbackTask(&performDidFailCallback, AllowCrossThreadAccess(m_client), code), m_taskMode);
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
