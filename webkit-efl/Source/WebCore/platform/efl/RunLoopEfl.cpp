/*
 * Copyright (C) 2012 ProFUSION embedded systems. All rights reserved.
 * Copyright (C) 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RunLoop.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_File.h>
#include <Edje.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#define ENABLE_TIZEN_TIMER_START_IN_CALLBACK_FIX 1 /* youngtaeck.song : fix bug that timer doesn't expired if it start in callback. This codes needs to be contributed */

static const int ecorePipeMessageSize = 1;
static const char wakupEcorePipeMessage[] = "W";

namespace WebCore {

RunLoop::RunLoop()
    : m_initEfl(false)
{
    if (!ecore_init()) {
        LOG_ERROR("could not init ecore.");
        return;
    }

    if (!ecore_evas_init()) {
        LOG_ERROR("could not init ecore_evas.");
        goto errorEcoreEvas;
    }

    if (!ecore_file_init()) {
        LOG_ERROR("could not init ecore_file.");
        goto errorEcoreFile;
    }

    if (!edje_init()) {
        LOG_ERROR("could not init edje.");
        goto errorEdje;
    }

    m_pipe = adoptPtr(ecore_pipe_add(wakeUpEvent, this));
    m_initEfl = true;

    return;

errorEdje:
    ecore_file_shutdown();
errorEcoreFile:
    ecore_evas_shutdown();
errorEcoreEvas:
    ecore_shutdown();
}

RunLoop::~RunLoop()
{
    if (m_initEfl) {
        edje_shutdown();
        ecore_file_shutdown();
        ecore_evas_shutdown();
        ecore_shutdown();
    }
}

void RunLoop::run()
{
    ecore_main_loop_begin();
}

void RunLoop::stop()
{
    ecore_main_loop_quit();
}

void RunLoop::wakeUpEvent(void* data, void*, unsigned int)
{
    static_cast<RunLoop*>(data)->performWork();
}

void RunLoop::wakeUp()
{
    MutexLocker locker(m_pipeLock);
#if ENABLE(TIZEN_RUNLOOP_WAKEUP_ERROR_WORKAROUND)
    Eina_Bool result = false;
    while(1) {
        result = ecore_pipe_write(m_pipe.get(), wakupEcorePipeMessage, ecorePipeMessageSize);
        if (result)
            return;

        LOG_ERROR("Failed to write a wakupEcorePipeMessage\n");
        m_pipe = adoptPtr(ecore_pipe_add(wakeUpEvent, this)); // due to OwnPtr, ecore_pipe_del is called automatically.
    }
#else
    ecore_pipe_write(m_pipe.get(), wakupEcorePipeMessage, ecorePipeMessageSize);
#endif
}

RunLoop::TimerBase::TimerBase(RunLoop*)
    : m_timer(0)
    , m_isRepeating(false)
{
}

RunLoop::TimerBase::~TimerBase()
{
    stop();
}

bool RunLoop::TimerBase::timerFired(void* data)
{
    RunLoop::TimerBase* timer = static_cast<RunLoop::TimerBase*>(data);

#ifndef ENABLE_TIZEN_TIMER_START_IN_CALLBACK_FIX
    timer->fired();
#endif

    if (!timer->m_isRepeating) {
        timer->m_timer = 0;
#ifdef ENABLE_TIZEN_TIMER_START_IN_CALLBACK_FIX
        timer->fired();
#endif
        return ECORE_CALLBACK_CANCEL;
    }

#ifdef ENABLE_TIZEN_TIMER_START_IN_CALLBACK_FIX
    timer->fired();
#endif

    return ECORE_CALLBACK_RENEW;
}

void RunLoop::TimerBase::start(double nextFireInterval, bool repeat)
{
    if (isActive())
        stop();

    m_isRepeating = repeat;
    ASSERT(!m_timer);
    m_timer = ecore_timer_add(nextFireInterval, reinterpret_cast<Ecore_Task_Cb>(timerFired), this);
}

void RunLoop::TimerBase::stop()
{
    if (m_timer) {
        ecore_timer_del(m_timer);
        m_timer = 0;
    }
}

bool RunLoop::TimerBase::isActive() const
{
    return (m_timer) ? true : false;
}

} // namespace WebCore
