/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "GCActivityCallback.h"

#include "APIShims.h"
#include "Heap.h"
#include "JSGlobalData.h"

#include <wtf/MainThread.h>

namespace JSC {

const double timeInterval = 0.3;
const size_t threshold = 2 * MB;

DefaultGCActivityCallback::DefaultGCActivityCallback(Heap* heap)
    : GCActivityCallback(heap->globalData(), WTF::isMainThread())
{
}

void DefaultGCActivityCallback::doWork()
{
    APIEntryShim shim(m_globalData);
#if ENABLE(TIZEN_JSC_LAZY_GARBAGE_COLLECTION)
    m_globalData->heap.collect(Heap::DoSweepByCallback);
#else
    m_globalData->heap.collect(Heap::DoNotSweep);
#endif
}

void DefaultGCActivityCallback::scheduleTimer(double newDelay)
{
    stop();
    ASSERT(!m_timer);
    m_timer = add(newDelay, this);
}

void DefaultGCActivityCallback::cancelTimer()
{
    stop();
}

void DefaultGCActivityCallback::didAllocate(size_t bytes)
{
    if (!isEnabled() || (bytes < threshold) || m_timer)
        return;

    ASSERT(WTF::isMainThread());
    scheduleTimer(timeInterval);
}

void DefaultGCActivityCallback::willCollect()
{
    cancelTimer();
}

void DefaultGCActivityCallback::cancel()
{
    cancelTimer();
}

} // namespace JSC
