/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "config.h"

#if ENABLE(TIZEN_EXTENSIBLE_API)
#include "TizenExtensibleAPI.h"

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
#include "EflScreenUtilities.h"
#endif

namespace WebCore {

static TizenExtensibleAPI* tizenExtensibleAPI = 0;

void TizenExtensibleAPI::initializeTizenExtensibleAPI()
{
    ASSERT(!tizenExtensibleAPI);

    tizenExtensibleAPI = new TizenExtensibleAPI();
}

TizenExtensibleAPI& TizenExtensibleAPI::extensibleAPI()
{
    return *tizenExtensibleAPI;
}

TizenExtensibleAPI::TizenExtensibleAPI()
    : m_backgroundMusic(true)
    , m_backgroundVibration(false)
    , m_blockMultimediaOnCall(false)
    , m_csp(false)
    , m_enableManualVideoRotation(false)
    , m_encryptDatabase(false)
    , m_fullScreen(false)
    , m_mediaStreamRecord(false)
    , m_mediaVolumeControl(false)
    , m_prerenderingForRotation(false)
    , m_rotateCameraView(true)
    , m_rotationLock(false)
    , m_soundMode(false)
    , m_supportFullScreen(true)
    , m_visibilitySuspend(false)
    , m_xwindowForFullScreenVideo(false)
    , m_supportMultimedia(true)
{
}

void TizenExtensibleAPI::setTizenExtensibleAPI(ExtensibleAPI extensibleAPI, bool enable)
{
    switch (extensibleAPI) {
    case BackgroundMusic:
        m_backgroundMusic = enable;
        break;
    case BackgroundVibration:
        m_backgroundVibration = enable;
        break;
    case BlockMultimediaOnCall:
        m_blockMultimediaOnCall = enable;
        break;
    case CSP:
        m_csp = enable;
        break;
    case EnableManualVideoRotation:
        m_enableManualVideoRotation = enable;
        break;
    case EncryptionDatabase:
        m_encryptDatabase = enable;
        break;
    case FullScreen:
        m_fullScreen = enable;
        break;
    case MediaStreamRecord:
        m_mediaStreamRecord = enable;
        break;
    case MediaVolumeControl:
        // As per HQ request ignoring mediaVolumeControl set from WRT
        //m_mediaVolumeControl = enable;
        break;
    case PrerenderingForRotation:
        m_prerenderingForRotation = enable;
        break;
    case RotateCameraView:
        m_rotateCameraView = enable;
        break;
    case RotationLock:
        m_rotationLock = enable;
        break;
    case SoundMode:
        m_soundMode = enable;
        break;
    case SupportFullScreen:
        m_supportFullScreen = enable;
        break;
    case VisibilitySuspend:
        m_visibilitySuspend = enable;
        break;
    case XwindowForFullScreenVideo:
        m_xwindowForFullScreenVideo = enable;
        break;
    case SupportMultimedia:
        m_supportMultimedia = enable;
        break;
    default:
        ASSERT_NOT_REACHED();
        return;
    }
}

bool TizenExtensibleAPI::getTizenExtensibleAPI(ExtensibleAPI extensibleAPI)
{
    bool enabled = false;

    switch (extensibleAPI) {
    case BackgroundMusic:
        enabled = m_backgroundMusic;
        break;
    case BackgroundVibration:
        enabled = m_backgroundVibration;
        break;
    case BlockMultimediaOnCall:
        enabled = m_blockMultimediaOnCall;
        break;
    case CSP:
        enabled = m_csp;
        break;
    case EnableManualVideoRotation:
        enabled = m_enableManualVideoRotation;
        break;
    case EncryptionDatabase:
        enabled = m_encryptDatabase;
        break;
    case FullScreen:
        enabled = m_fullScreen;
        break;
    case MediaStreamRecord:
        enabled = m_mediaStreamRecord;
        break;
    case MediaVolumeControl:
        enabled = m_mediaVolumeControl;
        break;
    case PrerenderingForRotation:
        enabled = m_prerenderingForRotation;
        break;
    case RotateCameraView:
        enabled = m_rotateCameraView;
        break;
    case RotationLock:
        enabled = m_rotationLock;
        break;
    case SoundMode:
        enabled = m_soundMode;
        break;
    case SupportFullScreen:
        enabled = m_supportFullScreen;
        break;
    case VisibilitySuspend:
        enabled = m_visibilitySuspend;
        break;
    case XwindowForFullScreenVideo:
        enabled = m_xwindowForFullScreenVideo;
        break;
    case SupportMultimedia:
        enabled = m_supportMultimedia;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }

    return enabled;
}

bool TizenExtensibleAPI::hwVideoOverlayInFullscreen() const
{
    if (!m_xwindowForFullScreenVideo)
        return false;

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    if (!getXWindow())
        return false;

    if (isMiniWindowMode() || isMultiWindowMode())
        return false;
#endif

    return true;
}

} // namespace WebCore

#endif // ENABLE(TIZEN_EXTENSIBLE_API)
