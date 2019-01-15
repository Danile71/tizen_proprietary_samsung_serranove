/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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

#ifndef MediaResourceControllerGStreamerTizen_h
#define MediaResourceControllerGStreamerTizen_h

#if ENABLE(TIZEN_GSTREAMER_VIDEO)

#include "AudioSessionManagerGStreamerTizen.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/ThreadingPrimitives.h>
#include <wtf/Vector.h>

namespace WebCore {

class HTMLMediaElement;

class MediaResourceControllerGStreamerTizen {
    WTF_MAKE_NONCOPYABLE(MediaResourceControllerGStreamerTizen);
public:
    static MediaResourceControllerGStreamerTizen& mediaResourceController();
    typedef Vector<HTMLMediaElement*> MediaResourceList;

    MediaResourceList* audioList() { return &m_audioList; }
    MediaResourceList* videoList() { return &m_videoList; }

    void activate(HTMLMediaElement*);
    void deactivate(HTMLMediaElement*, bool callOnDestructor);

    bool isActivated(HTMLMediaElement*);
    // Check it is on call whether can play media source.
    // Only this API can be called on play and autoplay logic.
    bool isOnCall();
    bool isRecordingAudio();

    size_t audioCount() { return m_audioList.size(); }
    size_t videoCount() { return m_videoList.size(); }

    void clearAllLists();
    void pauseOthers(HTMLMediaElement*);
    bool reservedMediaKey() { return m_reservedMediaKey; }
    void mediaKeyReserve(HTMLMediaElement* element);
    void mediaKeyRelease(HTMLMediaElement* element);
    void handleMediaKey(int key);

#if ENABLE(TIZEN_MEDIA_STREAM)
    void setUseCameraResource(bool);
#endif

    void updateAudioSessionState(HTMLMediaElement* element, ASM_sound_states_t newstate);
    void updatePowerControlState();
    void updateVisibilityState(bool);
    void updateOrientationState();
    void updateWFDMode();
    AudioSessionManagerGStreamerTizen* audioSessionManager() { return m_audioSessionManager.get(); }

private:
    MediaResourceControllerGStreamerTizen();
    ~MediaResourceControllerGStreamerTizen();

    bool addToList(HTMLMediaElement*, MediaResourceList*);
    bool removeFromList(HTMLMediaElement*, MediaResourceList*);

    RefPtr<AudioSessionManagerGStreamerTizen> m_audioSessionManager;
    MediaResourceList m_audioList;
    MediaResourceList m_videoList;
    Mutex m_locker;
    bool m_reservedMediaKey;
    bool m_powerLockOn;
#if ENABLE(TIZEN_MEDIA_STREAM)
    bool m_useCameraResource;
#endif

};

} // namespace
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
#endif
