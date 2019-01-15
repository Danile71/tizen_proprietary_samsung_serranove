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
#include "AsyncFileSystemTaskControllerTizen.h"

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "WorkerContext.h"
#include "WorkerLoaderProxy.h"
#include "WorkerRunLoop.h"
#include "WorkerScriptController.h"
#include "WorkerThread.h"

namespace WebCore {

AsyncFileSystemTaskControllerTizen::AsyncFileSystemTaskControllerTizen(ScriptExecutionContext* context, FileSystemSynchronousType synchronousType)
    : m_context(context)
    , m_synchronousType(synchronousType)
    , m_currentTaskMode(String())
    , m_isThreadStopping(false)
{
    if (m_context->isDocument())
        m_workerLoaderProxy = 0;
    else {
#if ENABLE(WORKERS)
        ASSERT(m_context->isWorkerContext());
        WorkerContext* workerContext = static_cast<WorkerContext*>(m_context);

        m_workerLoaderProxy = &workerContext->thread()->workerLoaderProxy();
        m_workerObserver = Observer::create(workerContext, this);
#else
        ASSERT_NOT_REACHED()
#endif
    }
}

AsyncFileSystemTaskControllerTizen::~AsyncFileSystemTaskControllerTizen()
{
}

String AsyncFileSystemTaskControllerTizen::uniqueMode()
{
    String uniqueMode;
    if (m_context->isDocument())
        uniqueMode = String();
    else {
#if ENABLE(WORKERS)
        ASSERT(m_context->isWorkerContext());
        WorkerContext* workerContext = static_cast<WorkerContext*>(m_context);

        uniqueMode = "fileSystemOperationMode";
        uniqueMode.append(String::number(workerContext->thread()->runLoop().createUniqueId()));
#else
        ASSERT_NOT_REACHED();
#endif
    }

    return uniqueMode;
}

void AsyncFileSystemTaskControllerTizen::postTaskToMainThread(PassOwnPtr<ScriptExecutionContext::Task> task)
{
    if (m_context->isDocument())
        m_context->postTask(task);
    else {
#if ENABLE(WORKERS)
        ASSERT(m_context->isWorkerContext());

        MutexLocker locker(m_mutex);
        m_workerLoaderProxy->postTaskToLoader(task);
#else
        ASSERT_NOT_REACHED();
#endif
    }
}

void AsyncFileSystemTaskControllerTizen::setCurrentTaskMode(const String& taskMode)
{
    m_currentTaskMode = taskMode;
}

void AsyncFileSystemTaskControllerTizen::postCallbackTask(PassOwnPtr<ScriptExecutionContext::Task> task, const String& mode)
{
    if (m_context->isDocument())
        m_context->postTask(task);
    else {
#if ENABLE(WORKERS)
        ASSERT(m_context->isWorkerContext());

        MutexLocker locker(m_mutex);
        m_workerLoaderProxy->postTaskForModeToWorkerContext(task, mode);
#else
        ASSERT_NOT_REACHED();
#endif
    }
}

bool AsyncFileSystemTaskControllerTizen::waitForTaskToComplete()
{
    if (m_context->isDocument()) {
        ASSERT(m_synchronousType == AsynchronousFileSystem);
        return false;
    }

#if ENABLE(WORKERS)
    ASSERT(m_context->isWorkerContext());

    if (m_synchronousType == AsynchronousFileSystem)
        return false;

    WorkerContext* workerContext = static_cast<WorkerContext*>(m_context);
    WorkerRunLoop& runLoop = workerContext->thread()->runLoop();
    if (runLoop.runInMode(workerContext, m_currentTaskMode) == MessageQueueTerminated) {
        m_currentTaskMode = String();
        return false;
    }

    m_currentTaskMode = String();
    return true;
#else
    ASSERT_NOT_REACHED();
    return false;
#endif
}

void AsyncFileSystemTaskControllerTizen::threadStopping()
{
}

AsyncFileSystemTaskControllerTizen::Observer::Observer(WorkerContext* context, AsyncFileSystemTaskControllerTizen* taskController)
    : WorkerContext::Observer(context)
    , m_taskController(taskController)
{
}

void AsyncFileSystemTaskControllerTizen::Observer::notifyStop()
{
}


} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)

