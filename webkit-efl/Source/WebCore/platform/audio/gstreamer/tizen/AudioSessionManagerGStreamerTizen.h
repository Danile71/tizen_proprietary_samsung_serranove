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

#ifndef AudioSessionManagerGStreamerTizen_h
#define AudioSessionManagerGStreamerTizen_h

#if ENABLE(TIZEN_GSTREAMER_VIDEO) || ENABLE(TIZEN_WEB_AUDIO) || ENABLE(TIZEN_MEDIA_STREAM)

#include <audio-session-manager.h>
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class AudioSessionManagerGStreamerTizen : public RefCounted<AudioSessionManagerGStreamerTizen> {
   WTF_MAKE_NONCOPYABLE(AudioSessionManagerGStreamerTizen);
public:
    static PassRefPtr<AudioSessionManagerGStreamerTizen> create()
    {
        return adoptRef(new AudioSessionManagerGStreamerTizen());
    }
    ~AudioSessionManagerGStreamerTizen();

    bool registerCallback(ASM_sound_cb_t, void*);
    void unregisterCallback();

    void clearVolumeSessionFromMediaType();
    void setVolumeSessionToMediaType();

    ASM_sound_states_t getSoundState();
    bool setSoundState(ASM_sound_states_t, ASM_resource_t);

    int totalManagerCount();
    void setHandlingASM(bool);
    bool handlingASM();

private:
    AudioSessionManagerGStreamerTizen();

    int m_handle;
    ASM_sound_states_t m_stateType;
    ASM_resource_t m_resourceType;
};

} // namespace
#endif
#endif
