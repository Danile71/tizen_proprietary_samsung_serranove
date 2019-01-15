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

#ifndef AsyncFileSystemTaskControllerTizen_h
#define AsyncFileSystemTaskControllerTizen_h

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "FileSystemType.h"
#include "ScriptExecutionContext.h"
#include "WorkerContext.h"
#include "WorkerLoaderProxy.h"
#include <wtf/PassRefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Threading.h>

namespace WebCore {

class AsyncFileSystemTaskControllerTizen : public ThreadSafeRefCounted<AsyncFileSystemTaskControllerTizen> {
public:
    static PassRefPtr<AsyncFileSystemTaskControllerTizen> create(ScriptExecutionContext* context, FileSystemSynchronousType synchronousType)
    {
        return adoptRef(new AsyncFileSystemTaskControllerTizen(context, synchronousType));
    }

    ~AsyncFileSystemTaskControllerTizen();

    String uniqueMode();

    void postTaskToMainThread(PassOwnPtr<ScriptExecutionContext::Task>);
    void setCurrentTaskMode(const String&);
    void postCallbackTask(PassOwnPtr<ScriptExecutionContext::Task>, const String&);
    bool waitForTaskToComplete();

    FileSystemSynchronousType synchronousType() { return m_synchronousType; }

    void threadStopping();

private:
    AsyncFileSystemTaskControllerTizen(ScriptExecutionContext*, FileSystemSynchronousType);

    ScriptExecutionContext* m_context;
    FileSystemSynchronousType m_synchronousType;
    String m_currentTaskMode;
    bool m_isThreadStopping;

    Mutex m_mutex;

#if ENABLE(WORKERS)
    class Observer : public WorkerContext::Observer {
    public:
        static PassOwnPtr<Observer> create(WorkerContext* context, AsyncFileSystemTaskControllerTizen* taskController)
        {
            return adoptPtr(new Observer(context, taskController));
        }

        virtual void notifyStop();

    private:
        Observer(WorkerContext*, AsyncFileSystemTaskControllerTizen*);

        RefPtr<AsyncFileSystemTaskControllerTizen> m_taskController;
    };

    WorkerLoaderProxy* m_workerLoaderProxy;
    OwnPtr<Observer> m_workerObserver;
#endif
};

} // namespace

#endif // ENABLE(FILE_SYSTEM)

#endif // AsyncFileSystemCallbacksTizen_h

