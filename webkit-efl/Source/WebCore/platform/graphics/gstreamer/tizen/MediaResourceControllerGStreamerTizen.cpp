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

#include "config.h"
#include "MediaResourceControllerGStreamerTizen.h"

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#if ENABLE(TIZEN_MEDIA_STREAM)
#include "LocalMediaServer.h"
#endif
#include "Chrome.h"
#include "ChromeClient.h"
#include "Logging.h"
#include "HTMLMediaElement.h"
#include "Page.h"
#include "RunLoop.h"
#include <media_key.h>
#include <device/power.h>
#include <vconf.h>
#include <recorder_product.h>

#if ENABLE(TIZEN_EXTENSIBLE_API)
#include "TizenExtensibleAPI.h"
#endif

namespace WebCore {

#define MAX_ACTIVE_AUDIO_HANDLE 30
#define MAX_ACTIVE_VIDEO_HANDLE 1

#if !LOG_DISABLED
static const char* boolString(bool val)
{
    return val ? "true" : "false";
}
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
static void exitFullscreenByASM(MediaResourceControllerGStreamerTizen* controller)
{
    size_t videoCount = controller->videoCount();
    for (size_t i = 0; i < videoCount; i++) {
        HTMLMediaElement* element = controller->videoList()->at(i);
        if (element->isFullscreen() && TizenExtensibleAPI::extensibleAPI().hwVideoOverlayInFullscreen())
            element->exitFullscreen();
    }
}
#endif

static void eventSourcePlay(ASM_event_sources_t eventSource, void* callbackData)
{
    MediaResourceControllerGStreamerTizen* pController = static_cast<MediaResourceControllerGStreamerTizen*>(callbackData);
    size_t audioCount = pController->audioCount();
    size_t videoCount = pController->videoCount();

    for (size_t i = 0; i < audioCount + videoCount; i++) {
        HTMLMediaElement* element = i < audioCount ? pController->audioList()->at(i) : pController->videoList()->at(i - audioCount);

        if (!element->isPausedByASM())
            continue;

        switch (eventSource) {
        case ASM_EVENT_SOURCE_ALARM_END:
        case ASM_EVENT_SOURCE_CALL_END:
        case ASM_EVENT_SOURCE_EMERGENCY_END:
        case ASM_EVENT_SOURCE_RESUMABLE_MEDIA:
            if (!element->ended() && !element->isVideo()) {
                element->setIsPausedByASM(false);
                element->playByOthers();
                TIZEN_LOGI("Media Element has been played by ASM %d", eventSource);
            }
            break;
        default:
            break;
        }
    }

    return;
}

static void eventSourcePause(ASM_event_sources_t eventSource, void* callbackData)
{
    MediaResourceControllerGStreamerTizen* pController = static_cast<MediaResourceControllerGStreamerTizen*>(callbackData);
    size_t audioCount = pController->audioCount();
    size_t videoCount = pController->videoCount();

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (eventSource == ASM_EVENT_SOURCE_RESOURCE_CONFLICT)
        exitFullscreenByASM(pController);
#endif

    for (size_t i = 0; i < audioCount + videoCount; i++) {
        HTMLMediaElement* element = i < audioCount ? pController->audioList()->at(i) : pController->videoList()->at(i - audioCount);
        MediaPlayer* player = element->isPlaying() ? element->player() : 0;

        if (!player || element->isPausedByASM())
            continue;

        switch (eventSource) {
        case ASM_EVENT_SOURCE_RESOURCE_CONFLICT:
            element->setIsPausedByASM(true);
            element->setIsReleasedByMediaResourceController(true);
            element->releaseMediaPlayer();
            if (pController->reservedMediaKey() && element->isVideo())
                pController->mediaKeyRelease(element);
            TIZEN_LOGI("Media Element has been released by ASM %d", eventSource);
            break;
        case ASM_EVENT_SOURCE_EARJACK_UNPLUG:
            if ((element->isVideo() && !player->url().string().contains("camera://")) || element->controls()) {
                element->setIsPausedByASM(true);
                element->pauseByOthers();
                TIZEN_LOGI("Media Element has been paused by ASM %d", eventSource);
            } else {
                pController->audioSessionManager()->setSoundState(ASM_STATE_PLAYING, ASM_RESOURCE_NONE);
            }
            break;
        case ASM_EVENT_SOURCE_CALL_START:
        case ASM_EVENT_SOURCE_ALARM_START:
        case ASM_EVENT_SOURCE_EMERGENCY_START:
        case ASM_EVENT_SOURCE_MEDIA:
        case ASM_EVENT_SOURCE_OTHER_PLAYER_APP:
        case ASM_EVENT_SOURCE_RESUMABLE_MEDIA:
            element->setIsPausedByASM(true);
            element->pauseByOthers();
            TIZEN_LOGI("Media Element has been paused by ASM %d", eventSource);
            break;
        default:
            break;
        }
    }

    return;
}

static ASM_cb_result_t handleASMEventSync(ASM_sound_commands_t command, ASM_event_sources_t eventSource, void* callbackData)
{
    MediaResourceControllerGStreamerTizen* pController = static_cast<MediaResourceControllerGStreamerTizen*>(callbackData);
    size_t audioCount = pController->audioCount();
    size_t videoCount = pController->videoCount();

    if (command == ASM_COMMAND_STOP || command == ASM_COMMAND_PAUSE) {
        for (size_t i = 0; i < audioCount + videoCount; i++) {
            HTMLMediaElement* element = i < audioCount ? pController->audioList()->at(i) : pController->videoList()->at(i - audioCount);
            MediaPlayer* player = element->isPlaying() ? element->player() : 0;

            if (!player || element->isPausedByASM())
                continue;

            switch (eventSource) {
            case ASM_EVENT_SOURCE_RESOURCE_CONFLICT:
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
            if (element->isVideo())
                element->videoResourceConflicted();
#endif
#if ENABLE(TIZEN_MEDIA_STREAM)
            if (player->url().string().contains("camera://")) {
                element->releaseMediaPlayer();
                TIZEN_LOGI("LocalMediaServer has been released for othe camera app.");
            }
#endif
                break;
            case ASM_EVENT_SOURCE_EARJACK_UNPLUG:
                if ((element->isVideo() && !player->url().string().contains("camera://")) || element->controls())
                    continue;
                return ASM_CB_RES_PLAYING;
            case ASM_EVENT_SOURCE_NOTIFY_START:
                pController->audioSessionManager()->setHandlingASM(true);
                element->setIsPausedByASM(true);
                element->pauseByNotification(true);
                TIZEN_LOGI("Media Element has been paused by ASM %d", eventSource);
                return ASM_CB_RES_PAUSE;
            default:
                break;
            }
        }
        return ASM_CB_RES_PAUSE;
    } else if (command == ASM_COMMAND_PLAY || command == ASM_COMMAND_RESUME) {
        for (size_t i = 0; i < audioCount + videoCount; i++) {
            HTMLMediaElement* element = i < audioCount ? pController->audioList()->at(i) : pController->videoList()->at(i - audioCount);

            if (!element->isPausedByASM())
                continue;

            switch (eventSource) {
            case ASM_EVENT_SOURCE_NOTIFY_END:
                if (!element->ended()) {
                    pController->audioSessionManager()->setHandlingASM(false);
                    element->setIsPausedByASM(false);
                    element->pauseByNotification(false);
                    TIZEN_LOGI("Media Element has been played by ASM %d", eventSource);
                }
            case ASM_EVENT_SOURCE_ALARM_END:
            case ASM_EVENT_SOURCE_CALL_END:
            case ASM_EVENT_SOURCE_EMERGENCY_END:
            case ASM_EVENT_SOURCE_RESUMABLE_MEDIA:
                if (!element->ended() && (!element->isVideo() || eventSource == ASM_EVENT_SOURCE_NOTIFY_END))
                    return ASM_CB_RES_PLAYING;
                break;
            default:
                break;
            }
        }
        return ASM_CB_RES_PAUSE;
    }

    return ASM_CB_RES_NONE;
}

static void handleASMEventAsync(ASM_sound_commands_t command, ASM_event_sources_t eventSource, void* callbackData)
{
    // Do something which should be handled in webkit main thread.
    if (command == ASM_COMMAND_STOP || command == ASM_COMMAND_PAUSE)
        return eventSourcePause(eventSource, callbackData);
    else if (command == ASM_COMMAND_PLAY || command == ASM_COMMAND_RESUME)
        return eventSourcePlay(eventSource, callbackData);

    return;
}

static ASM_cb_result_t audioSessionCallback(int, ASM_event_sources_t eventSource, ASM_sound_commands_t command, unsigned int, void* callbackData)
{
    RunLoop::main()->dispatch(WTF::bind(&handleASMEventAsync, command, eventSource, callbackData));
    return handleASMEventSync(command, eventSource, callbackData);
}

static void mediaKeyReserveCallback(media_key_e key, media_key_event_e event, void *userData)
{
    if (event != MEDIA_KEY_STATUS_PRESSED)
        return;

    MediaResourceControllerGStreamerTizen* controller = static_cast<MediaResourceControllerGStreamerTizen*>(userData);
    controller->handleMediaKey(key);
}

void powersavingModeChangeCallback(keynode_t *keynode, void* data)
{
    int powersavingMode = SETTING_PSMODE_NORMAL;
    if (vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &powersavingMode) < 0) {
        TIZEN_LOGI("Failed to get the power saving mode from vconf");
        return;
    }

    // Release media resources only when device ultra power saving mode is enabled
    if (powersavingMode != SETTING_PSMODE_EMERGENCY)
        return;

    MediaResourceControllerGStreamerTizen* pController = static_cast<MediaResourceControllerGStreamerTizen*>(data);
    size_t audioCount = pController->audioCount();

    for (size_t i = 0; i < audioCount; i++) {
        HTMLMediaElement* element = pController->audioList()->at(i);
        MediaPlayer* player = element->isPlaying() ? element->player() : 0;
        if (!player)
            continue;

        TIZEN_LOGI("MediaPlayer released for Emergency Power Saving Mode");
        element->releaseMediaPlayer();
    }
}

MediaResourceControllerGStreamerTizen& MediaResourceControllerGStreamerTizen::mediaResourceController()
{
    static MediaResourceControllerGStreamerTizen* controller = 0;

    if (!controller)
        controller = new MediaResourceControllerGStreamerTizen();

    return *controller;
}

MediaResourceControllerGStreamerTizen::MediaResourceControllerGStreamerTizen()
    : m_audioSessionManager(AudioSessionManagerGStreamerTizen::create())
    , m_audioList(0)
    , m_videoList(0)
    , m_reservedMediaKey(false)
    , m_powerLockOn(false)
#if ENABLE(TIZEN_MEDIA_STREAM)
    , m_useCameraResource(false)
#endif
{
    m_audioSessionManager->registerCallback(audioSessionCallback, this);
    vconf_notify_key_changed(VCONFKEY_SETAPPL_PSMODE, powersavingModeChangeCallback, static_cast<void*>(this));
}

MediaResourceControllerGStreamerTizen::~MediaResourceControllerGStreamerTizen()
{
    int ret = 0;
    m_audioSessionManager->clearVolumeSessionFromMediaType();
    if (m_powerLockOn) {
        ret = device_power_release_lock(POWER_LOCK_DISPLAY);
        if (ret == DEVICE_ERROR_NONE)
            m_powerLockOn = false;
    }
    clearAllLists();
    if (m_reservedMediaKey)
        mediaKeyRelease(0);
    vconf_ignore_key_changed(VCONFKEY_SETAPPL_PSMODE, powersavingModeChangeCallback);
}

void MediaResourceControllerGStreamerTizen::activate(HTMLMediaElement* resource)
{
    if (!resource)
        return;

    bool isVideo = resource->isVideo();
    LOG(Media, "activate [%p], isVideo = %s", resource, boolString(isVideo));

    MediaResourceList* list = isVideo ? videoList() : audioList();
    size_t max = isVideo ? MAX_ACTIVE_VIDEO_HANDLE : MAX_ACTIVE_AUDIO_HANDLE;

    if (resource->isReleasedByMediaResourceController())
        resource->setReadyState(MediaPlayer::HaveMetadata);

    removeFromList(resource, list);
    addToList(resource, list);

    if (list->size() > max) {
        list->first()->setIsReleasedByMediaResourceController(true);
        deactivate(list->first(), false);
    }

    resource->setIsPausedByASM(false);
    resource->setIsReleased(false);
    resource->setIsReleasedByMediaResourceController(false);

    if (isVideo && !m_reservedMediaKey)
        mediaKeyReserve(resource);

    return;
}

void MediaResourceControllerGStreamerTizen::deactivate(HTMLMediaElement* resource, bool callOnDestructor)
{
    if (!resource)
        return;

    if (removeFromList(resource, audioList()) || removeFromList(resource, videoList())) {
        if (!callOnDestructor)
            resource->releaseMediaPlayer();

        LOG(Media, "deactivate [%p]", resource);

        if (m_audioList.isEmpty() && m_videoList.isEmpty()) {
            updatePowerControlState();
            updateWFDMode();
            m_audioSessionManager->clearVolumeSessionFromMediaType();
        }
    }

    if (m_reservedMediaKey && m_videoList.isEmpty())
        mediaKeyRelease(resource);

    return;
}

bool MediaResourceControllerGStreamerTizen::isActivated(HTMLMediaElement* resource)
{
    if (!resource)
        return false;

    bool isVideo = resource->isVideo();
    MediaResourceList* list = isVideo ? videoList() : audioList();

    size_t index = list->find(resource);
    if(index == notFound)
        return false;

    LOG(Media, "isActivated [%p], isVideo = %s", resource, boolString(isVideo));

    return true;
}

bool MediaResourceControllerGStreamerTizen::isOnCall()
{
    if (!TizenExtensibleAPI::extensibleAPI().blockMultimediaOnCall())
        return false;

    return !m_audioSessionManager->setSoundState(ASM_STATE_PLAYING, ASM_RESOURCE_NONE);
}

bool MediaResourceControllerGStreamerTizen::isRecordingAudio()
{
    bool isRecording = false;
    return (recorder_is_in_recording(RECORDER_AUDIO_SOURCE, &isRecording) == RECORDER_ERROR_NONE && isRecording);
}

bool MediaResourceControllerGStreamerTizen::addToList(HTMLMediaElement* resource, MediaResourceList* list)
{
    if (!resource || !list)
        return false;

    MutexLocker lockMediaResource(m_locker);
    list->append(resource);
    LOG(Media, "addToList [%p], isVideo = %s", resource, boolString(resource->isVideo()));

    return true;
}

bool MediaResourceControllerGStreamerTizen::removeFromList(HTMLMediaElement* resource, MediaResourceList* list)
{
    if (!resource || !list)
        return false;

    MutexLocker lockMediaResource(m_locker);

    size_t index = list->reverseFind(resource);
    if(index == notFound)
        return false;

    list->remove(index, 1);
    LOG(Media, "removeFromList [%p]", resource);

    return true;
}

void MediaResourceControllerGStreamerTizen::clearAllLists()
{
    LOG(Media, "Clear all mediaresource lists.");

    m_audioList.clear();
    m_videoList.clear();
}

void MediaResourceControllerGStreamerTizen::pauseOthers(HTMLMediaElement* resource)
{
    if (!resource)
        return;

    bool isVideo = resource->isVideo();
    LOG(Media, "pauseOthers [%p], isVideo = %s", resource, boolString(isVideo));

    for (size_t i = 0; i < audioCount() + videoCount(); i++) {
        HTMLMediaElement* element = i < audioCount() ? audioList()->at(i) : videoList()->at(i - audioCount());
        if (element != resource && element->isPlaying())
            element->pauseByOthers();
    }

    return;
}

void MediaResourceControllerGStreamerTizen::updateAudioSessionState(HTMLMediaElement* element, ASM_sound_states_t newstate)
{
    ASM_resource_t type = ASM_RESOURCE_NONE;
    ASM_sound_states_t state = ASM_STATE_STOP;
    MediaPlayer* player = 0;

    if (element)
        element->setSoundState(newstate);

    if (m_useCameraResource) {
        type = ASM_RESOURCE_CAMERA;
        state = ASM_STATE_PLAYING;
    }

    if (videoCount()) {
        for (size_t i = 0; i < videoCount(); i++) {
            if (videoList()->at(i)->soundState() == ASM_STATE_PLAYING && !videoList()->at(i)->isPausedByASM()) {
                player = videoList()->at(i)->player();
                if (player && player->url().string().contains("camera://"))
                    type = (ASM_resource_t)(type | ASM_RESOURCE_CAMERA);
                type = (ASM_resource_t)(type | ASM_RESOURCE_VIDEO_OVERLAY);
                state = ASM_STATE_PLAYING;
                break;
            }
        }
    }

    if (audioCount()) {
        for (size_t i = 0; i < audioCount(); i++) {
            if (audioList()->at(i)->soundState() == ASM_STATE_PLAYING && !audioList()->at(i)->isPausedByASM()) {
                state = ASM_STATE_PLAYING;
                break;
            }
        }
    }

    m_audioSessionManager->setSoundState(state, type);
}

void MediaResourceControllerGStreamerTizen::updatePowerControlState()
{
    int ret = 0;
    for (size_t i = 0; i < videoCount(); i++)
        if (videoList()->at(i)->isPlaying() || videoList()->at(i)->isBuffering()) {
            ret = device_power_request_lock(POWER_LOCK_DISPLAY, 0);
            if (ret == DEVICE_ERROR_NONE)
                m_powerLockOn = true;
            return;
        }
    device_power_wakeup(false);
    if (m_powerLockOn) {
        ret = device_power_release_lock(POWER_LOCK_DISPLAY);
        if (ret == DEVICE_ERROR_NONE)
            m_powerLockOn = false;
    }

    return;
}

void MediaResourceControllerGStreamerTizen::updateVisibilityState(bool visibility)
{
    MutexLocker lockMediaResource(m_locker);

    for (size_t i = 0; i < videoCount(); i++)
        videoList()->at(i)->visibilityChanged(visibility);

    visibility ? m_audioSessionManager->setVolumeSessionToMediaType() : m_audioSessionManager->clearVolumeSessionFromMediaType();

    return;
}

#if ENABLE(TIZEN_MEDIA_STREAM)
void MediaResourceControllerGStreamerTizen::setUseCameraResource(bool use)
{
    m_useCameraResource = use;
    updateAudioSessionState(0, use ? ASM_STATE_PLAYING : ASM_STATE_PAUSE);
}
#endif

void MediaResourceControllerGStreamerTizen::updateOrientationState()
{
    for (size_t i = 0; i < videoCount(); i++) {
        MediaPlayer* player = videoList()->at(i)->player();
        if (player->url().string().contains("camera://"))
            player->orientationStateChanged();
    }
}

void MediaResourceControllerGStreamerTizen::updateWFDMode()
{
    // If it need to update WiFi-Direct mode, please implement here for product.
}

void MediaResourceControllerGStreamerTizen::mediaKeyReserve(HTMLMediaElement* element)
{
    if (m_reservedMediaKey)
        return;

    // Reserve media key for BT media key
    int ret = media_key_reserve(mediaKeyReserveCallback, this);
    if (ret != MEDIA_KEY_ERROR_NONE) {
        TIZEN_LOGE("media_key_reserve is fail. [0x%x]", ret);
        return;
    }

    // Grab media key for earjack media key.
    // It should be handled in UI process by ecore key event policy.
    if (element && element->document() && element->document()->page())
        element->document()->page()->chrome()->client()->setGrabMediaKey(true);

    m_reservedMediaKey = true;
}

void MediaResourceControllerGStreamerTizen::mediaKeyRelease(HTMLMediaElement* element)
{
    if (!m_reservedMediaKey)
        return;

    media_key_release();
    if (element && element->document() && element->document()->page())
        element->document()->page()->chrome()->client()->setGrabMediaKey(false);

    m_reservedMediaKey = false;
}

void MediaResourceControllerGStreamerTizen::handleMediaKey(int key)
{
    if (videoList()->isEmpty())
        return;

    LOG(Media, "key : %d", key);
    HTMLMediaElement* element = videoList()->last();
    switch(key)
    {
    case MEDIA_KEY_PLAY:
    case MEDIA_KEY_PAUSE:
    case MEDIA_KEY_STOP:
    case MEDIA_KEY_PLAYPAUSE:
        element->isPlaying() ? element->pause() : element->play();
        break;
    case MEDIA_KEY_PREVIOUS:
    case MEDIA_KEY_REWIND:
        element->rewind(5.0f);
        break;
    case MEDIA_KEY_NEXT:
    case MEDIA_KEY_FASTFORWARD:
        element->rewind(-5.0f);
        break;
    default:
        LOG(Media, "Undefined key : %d", key);
        break;
    }
}

} // namespace
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
