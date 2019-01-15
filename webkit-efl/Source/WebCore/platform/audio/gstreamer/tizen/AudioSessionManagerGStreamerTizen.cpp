/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
#include "AudioSessionManagerGStreamerTizen.h"

#if ENABLE(TIZEN_GSTREAMER_VIDEO) || ENABLE(TIZEN_WEB_AUDIO) || ENABLE(TIZEN_MEDIA_STREAM)

#include "Logging.h"
#include "GStreamerUtilities.h"
#include "GStreamerVersioning.h"

#include <mm_sound.h>
#include <unistd.h>

#if ENABLE(TIZEN_EXTENSIBLE_API)
#include "TizenExtensibleAPI.h"
#endif

namespace WebCore {

static int managerCount = 0;
static bool isHandlingASM = false;

AudioSessionManagerGStreamerTizen::AudioSessionManagerGStreamerTizen()
    : m_handle(-1)
    , m_stateType(ASM_STATE_NONE)
    , m_resourceType(ASM_RESOURCE_NONE)
{
    atomicIncrement(&managerCount);
    if (managerCount == 1)
        setVolumeSessionToMediaType();
}

AudioSessionManagerGStreamerTizen::~AudioSessionManagerGStreamerTizen()
{
    unregisterCallback();

    atomicDecrement(&managerCount);
    if (managerCount == 0)
        clearVolumeSessionFromMediaType();
}

bool AudioSessionManagerGStreamerTizen::registerCallback(ASM_sound_cb_t notifyCallback, void* callbackData)
{
    int error = 0;
    if (!ASM_register_sound(getpid(), &m_handle, ASM_EVENT_MEDIA_WEBKIT, m_stateType, notifyCallback, callbackData, m_resourceType, &error)) {
        LOG_MEDIA_MESSAGE("register is failed. errcode = 0x%X", error);
        return false;
    }

#if ENABLE(TIZEN_EXTENSIBLE_API)
    if (TizenExtensibleAPI::extensibleAPI().soundMode()) {
        bool ret = ASM_set_session_option(m_handle, ASM_SESSION_OPTION_PAUSE_OTHERS, &error);
        if (ret) {
            TIZEN_LOGI("AudioSession mode has been set to ASM_SESSION_OPTION_PAUSE_OTHERS.");
        } else {
            TIZEN_LOGE("AudioSession mode has been not set to ASM_SESSION_OPTION_PAUSE_OTHERS with error = 0x%X", error);
        }
    }
#endif

    setSoundState(ASM_STATE_PAUSE, ASM_RESOURCE_NONE);

    return true;
}

void AudioSessionManagerGStreamerTizen::unregisterCallback()
{
    int error = 0;

    setSoundState(ASM_STATE_STOP, ASM_RESOURCE_NONE);

    if (!ASM_unregister_sound(m_handle, ASM_EVENT_MEDIA_WEBKIT, &error))
        LOG_MEDIA_MESSAGE("unregister() is failed 0x%X\n", error);

    m_handle = -1;

    return;
}

ASM_sound_states_t AudioSessionManagerGStreamerTizen::getSoundState()
{
    int error = 0;
    ASM_sound_states_t currentState = ASM_STATE_NONE;

    if (!ASM_get_sound_state(m_handle, ASM_EVENT_MEDIA_WEBKIT, &currentState, &error))
        LOG_MEDIA_MESSAGE("getSoundState state is failed 0x%X", error);

    return currentState;
}

bool AudioSessionManagerGStreamerTizen::setSoundState(ASM_sound_states_t newState, ASM_resource_t newType)
{
    int error = 0;

    ASM_resource_t oldResourceType = m_resourceType;
    ASM_sound_states_t oldStateType = getSoundState();

    if (oldResourceType == newType && oldStateType == newState)
        return true;

    m_resourceType = newType;

    if (!ASM_set_sound_state(m_handle, ASM_EVENT_MEDIA_WEBKIT, newState, m_resourceType, &error)) {
        LOG_MEDIA_MESSAGE("setSoundState state = [%d] failed 0x%X", newState, error);
        return false;
    }

    TIZEN_LOGI("setSoundState state = [%d] type = [%d]", newState, newType);

    return true;
}

void AudioSessionManagerGStreamerTizen::setVolumeSessionToMediaType()
{
#if ENABLE(TIZEN_EXTENSIBLE_API)
    if (TizenExtensibleAPI::extensibleAPI().mediaVolumeControl())
        mm_sound_volume_primary_type_set(VOLUME_TYPE_MEDIA);
#endif
}

void AudioSessionManagerGStreamerTizen::clearVolumeSessionFromMediaType()
{
#if ENABLE(TIZEN_EXTENSIBLE_API)
    if (TizenExtensibleAPI::extensibleAPI().mediaVolumeControl())
        mm_sound_volume_primary_type_clear();
#endif
}

int AudioSessionManagerGStreamerTizen::totalManagerCount()
{
    return managerCount;
}

void AudioSessionManagerGStreamerTizen::setHandlingASM(bool state)
{
    isHandlingASM = state;
}

bool AudioSessionManagerGStreamerTizen::handlingASM()
{
    return isHandlingASM;
}
} // namespace
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO) || ENABLE(TIZEN_WEB_AUDIO) || ENABLE(TIZEN_MEDIA_STREAM)
