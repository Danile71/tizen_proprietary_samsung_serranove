/*
 * Copyright (C) 2011 Samsung Electronics
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
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebFullScreenManagerProxy.h"

#if ENABLE(FULLSCREEN_API)
#include "WebContext.h"
#include "WebFullScreenManagerMessages.h"
#include "WebFullScreenManagerProxyMessages.h"
#include "WebProcess.h"

#include "EwkViewImpl.h"
#include <WebCore/NotImplemented.h>


using namespace WebCore;

namespace WebKit {

void WebFullScreenManagerProxy::invalidate()
{
    m_webView = 0;
}

void WebFullScreenManagerProxy::close()
{
    notImplemented();
}

bool WebFullScreenManagerProxy::isFullScreen()
{
    return m_isFullScreen;
}

void WebFullScreenManagerProxy::enterFullScreen()
{
    if (!m_webView)
        return;

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    m_compositingEnabled = true;
#endif

    willEnterFullScreen();
    EwkViewImpl::fromEvasObject(m_webView)->enterFullScreen();
    didEnterFullScreen();

    m_isFullScreen = true;
}

void WebFullScreenManagerProxy::exitFullScreen()
{
    if (!m_webView)
        return;

    willExitFullScreen();
    EwkViewImpl::fromEvasObject(m_webView)->exitFullScreen();
    didExitFullScreen();

    m_isFullScreen = false;
#if ENABLE(TIZEN_FULLSCREEN_API)
    setExitFullScreenByHwBackKey(false);
    // Start timer to avoid zoom out in fullscreen state.
    // 0.5 is appropriate value found by the tests.
    if (m_scaleFactorToRestore)
        m_restoreScaleFactorTimer.startOneShot(0.5);
#endif
}

#if ENABLE(TIZEN_FULLSCREEN_API)
void WebFullScreenManagerProxy::updateMediaControlsStyle()
{
    m_page->process()->send(Messages::WebFullScreenManager::updateMediaControlsStyle(), m_page->pageID());
}

void WebFullScreenManagerProxy::setExitFullScreenByHwBackKey(bool isNeeded)
{
    m_exitFullScreenByHwBackKey = isNeeded;
}

void WebFullScreenManagerProxy::restoreScaleFactorTimerFired(Timer < WebFullScreenManagerProxy > *)
{
    EwkViewImpl::fromEvasObject(m_webView)->restoreScaleFactorAfterExitingFullScreen(m_scaleFactorToRestore);
    m_scaleFactorToRestore = 0;
}

void WebFullScreenManagerProxy::willRotateScreen()
{
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (isUsingHwVideoOverlay())
        enableCompositing(false);
#endif

    m_page->process()->send(Messages::WebFullScreenManager::WillRotateScreen(), m_page->pageID());
}

void WebFullScreenManagerProxy::didRotateScreen()
{
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (isUsingHwVideoOverlay())
        enableCompositing(true);
#endif

    m_page->process()->send(Messages::WebFullScreenManager::DidRotateScreen(), m_page->pageID());
}
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
void WebFullScreenManagerProxy::enableCompositing(bool enable)
{
    m_compositingEnabled = enable;
}

void WebFullScreenManagerProxy::setUsingHwVideoOverlay(bool use)
{
    m_isUsingHwVideoOverlay = use;
    if (use)
        EwkViewImpl::fromEvasObject(m_webView)->hwVideoOverlayEnabled();
    else
        EwkViewImpl::fromEvasObject(m_webView)->hwVideoOverlayDisabled();
}
#endif

void WebFullScreenManagerProxy::beganEnterFullScreen(const IntRect& initialFrame, const IntRect& finalFrame)
{
    notImplemented();
}

void WebFullScreenManagerProxy::beganExitFullScreen(const IntRect& initialFrame, const IntRect& finalFrame)
{
    notImplemented();
}

} // namespace WebKit

#endif // ENABLE(FULLSCREEN_API)
