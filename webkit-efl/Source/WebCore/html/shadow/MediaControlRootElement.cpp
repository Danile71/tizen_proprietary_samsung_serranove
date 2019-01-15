/*
 * Copyright (C) 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO)
#include "MediaControlRootElement.h"

#include "Chrome.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "MediaControlElements.h"
#include "MouseEvent.h"
#include "Page.h"
#include "RenderTheme.h"
#include "Text.h"

#if ENABLE(VIDEO_TRACK)
#include "TextTrackCue.h"
#endif

#if ENABLE(TIZEN_FULLSCREEN_API)
#include "Frame.h"
#include "FrameView.h"
#include "PlatformScreen.h"
#endif

using namespace std;

namespace WebCore {

static const double timeWithoutMouseMovementBeforeHidingControls = 3;

#if ENABLE(TIZEN_FULLSCREEN_API)
const FloatSize compareResolution(720.0f, 1280.0f);
#endif

MediaControlRootElement::MediaControlRootElement(Document* document)
    : MediaControls(document)
    , m_mediaController(0)
    , m_rewindButton(0)
    , m_playButton(0)
    , m_returnToRealTimeButton(0)
    , m_statusDisplay(0)
    , m_currentTimeDisplay(0)
    , m_timeline(0)
    , m_timeRemainingDisplay(0)
    , m_timelineContainer(0)
    , m_seekBackButton(0)
    , m_seekForwardButton(0)
    , m_toggleClosedCaptionsButton(0)
    , m_panelMuteButton(0)
    , m_volumeSlider(0)
    , m_volumeSliderMuteButton(0)
    , m_volumeSliderContainer(0)
    , m_fullScreenButton(0)
    , m_fullScreenMinVolumeButton(0)
    , m_fullScreenVolumeSlider(0)
    , m_fullScreenMaxVolumeButton(0)
    , m_panel(0)
#if ENABLE(VIDEO_TRACK)
    , m_textDisplayContainer(0)
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    , m_overlayContainer(0)
    , m_defaultImage(0)
    , m_overlayLoadingSpinner(0)
    , m_hideControlsTimer(this, &MediaControlRootElement::hideControlsTimerFired)
    , m_rotateButton(0)
#else
    , m_hideFullscreenControlsTimer(this, &MediaControlRootElement::hideFullscreenControlsTimerFired)
#endif
    , m_isMouseOverControls(false)
    , m_isFullscreen(false)
{
}

PassRefPtr<MediaControls> MediaControls::create(Document* document)
{
    return MediaControlRootElement::create(document);
}

PassRefPtr<MediaControlRootElement> MediaControlRootElement::create(Document* document)
{
    if (!document->page())
        return 0;

    RefPtr<MediaControlRootElement> controls = adoptRef(new MediaControlRootElement(document));

    RefPtr<MediaControlPanelElement> panel = MediaControlPanelElement::create(document);

    ExceptionCode ec;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    RefPtr<MediaControlOverlayContainerElement> overlayContainer = MediaControlOverlayContainerElement::create(document);
    RefPtr<MediaControlDefaultImageElement> defaultImage = MediaControlDefaultImageElement::create(document);
    controls->m_defaultImage = defaultImage.get();
    overlayContainer->appendChild(defaultImage.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlOverlayLoadingSpinnerElement> overlayLoadingSpinner = MediaControlOverlayLoadingSpinnerElement::create(document);
    controls->m_overlayLoadingSpinner = overlayLoadingSpinner.get();
    overlayContainer->appendChild(overlayLoadingSpinner.release(), ec, true);
    if (ec)
        return 0;

    controls->m_overlayContainer = overlayContainer.get();
    controls->appendChild(overlayContainer.release(), ec, true);
    if (ec)
        return 0;
#endif

    RefPtr<MediaControlRewindButtonElement> rewindButton = MediaControlRewindButtonElement::create(document);
    controls->m_rewindButton = rewindButton.get();
    panel->appendChild(rewindButton.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlPlayButtonElement> playButton = MediaControlPlayButtonElement::create(document);
    controls->m_playButton = playButton.get();
    panel->appendChild(playButton.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlReturnToRealtimeButtonElement> returnToRealtimeButton = MediaControlReturnToRealtimeButtonElement::create(document);
    controls->m_returnToRealTimeButton = returnToRealtimeButton.get();
    panel->appendChild(returnToRealtimeButton.release(), ec, true);
    if (ec)
        return 0;

    if (document->page()->theme()->usesMediaControlStatusDisplay()) {
        RefPtr<MediaControlStatusDisplayElement> statusDisplay = MediaControlStatusDisplayElement::create(document);
        controls->m_statusDisplay = statusDisplay.get();
        panel->appendChild(statusDisplay.release(), ec, true);
        if (ec)
            return 0;
    }

    RefPtr<MediaControlTimelineContainerElement> timelineContainer = MediaControlTimelineContainerElement::create(document);

    RefPtr<MediaControlCurrentTimeDisplayElement> currentTimeDisplay = MediaControlCurrentTimeDisplayElement::create(document);
    controls->m_currentTimeDisplay = currentTimeDisplay.get();
    timelineContainer->appendChild(currentTimeDisplay.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlTimelineElement> timeline = MediaControlTimelineElement::create(document, controls.get());
    controls->m_timeline = timeline.get();
    timelineContainer->appendChild(timeline.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlTimeRemainingDisplayElement> timeRemainingDisplay = MediaControlTimeRemainingDisplayElement::create(document);
    controls->m_timeRemainingDisplay = timeRemainingDisplay.get();
    timelineContainer->appendChild(timeRemainingDisplay.release(), ec, true);
    if (ec)
        return 0;

    controls->m_timelineContainer = timelineContainer.get();
    panel->appendChild(timelineContainer.release(), ec, true);
    if (ec)
        return 0;

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlSeekBackButtonElement> seekBackButton = MediaControlSeekBackButtonElement::create(document);
    controls->m_seekBackButton = seekBackButton.get();
    panel->appendChild(seekBackButton.release(), ec, true);
    if (ec)
        return 0;

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlSeekForwardButtonElement> seekForwardButton = MediaControlSeekForwardButtonElement::create(document);
    controls->m_seekForwardButton = seekForwardButton.get();
    panel->appendChild(seekForwardButton.release(), ec, true);
    if (ec)
        return 0;

    if (document->page()->theme()->supportsClosedCaptioning()) {
        RefPtr<MediaControlToggleClosedCaptionsButtonElement> toggleClosedCaptionsButton = MediaControlToggleClosedCaptionsButtonElement::create(document);
        controls->m_toggleClosedCaptionsButton = toggleClosedCaptionsButton.get();
        panel->appendChild(toggleClosedCaptionsButton.release(), ec, true);
        if (ec)
            return 0;
    }

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlFullscreenButtonElement> fullScreenButton = MediaControlFullscreenButtonElement::create(document, controls.get());
    controls->m_fullScreenButton = fullScreenButton.get();
    panel->appendChild(fullScreenButton.release(), ec, true);

    // The mute button and the slider element should be in the same div.
    RefPtr<HTMLDivElement> panelVolumeControlContainer = HTMLDivElement::create(document);

    if (document->page()->theme()->usesMediaControlVolumeSlider()) {
        RefPtr<MediaControlVolumeSliderContainerElement> volumeSliderContainer = MediaControlVolumeSliderContainerElement::create(document);

        RefPtr<MediaControlVolumeSliderElement> slider = MediaControlVolumeSliderElement::create(document);
        controls->m_volumeSlider = slider.get();
        volumeSliderContainer->appendChild(slider.release(), ec, true);
        if (ec)
            return 0;

        // This is a duplicate mute button, which is visible in some ports at the bottom of the volume bar.
        // It's important only when the volume bar is displayed below the controls.
        RefPtr<MediaControlVolumeSliderMuteButtonElement> volumeSliderMuteButton = MediaControlVolumeSliderMuteButtonElement::create(document);
        controls->m_volumeSliderMuteButton = volumeSliderMuteButton.get();
        volumeSliderContainer->appendChild(volumeSliderMuteButton.release(), ec, true);

        if (ec)
            return 0;

        controls->m_volumeSliderContainer = volumeSliderContainer.get();
        panelVolumeControlContainer->appendChild(volumeSliderContainer.release(), ec, true);
        if (ec)
            return 0;
    }

    RefPtr<MediaControlPanelMuteButtonElement> panelMuteButton = MediaControlPanelMuteButtonElement::create(document, controls.get());
    controls->m_panelMuteButton = panelMuteButton.get();
    panelVolumeControlContainer->appendChild(panelMuteButton.release(), ec, true);
    if (ec)
        return 0;

    panel->appendChild(panelVolumeControlContainer, ec, true);
    if (ec)
        return 0;

    // FIXME: Only create when needed <http://webkit.org/b/57163>
    RefPtr<MediaControlFullscreenVolumeMinButtonElement> fullScreenMinVolumeButton = MediaControlFullscreenVolumeMinButtonElement::create(document);
    controls->m_fullScreenMinVolumeButton = fullScreenMinVolumeButton.get();
    panel->appendChild(fullScreenMinVolumeButton.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenVolumeSliderElement> fullScreenVolumeSlider = MediaControlFullscreenVolumeSliderElement::create(document);
    controls->m_fullScreenVolumeSlider = fullScreenVolumeSlider.get();
    panel->appendChild(fullScreenVolumeSlider.release(), ec, true);
    if (ec)
        return 0;

    RefPtr<MediaControlFullscreenVolumeMaxButtonElement> fullScreenMaxVolumeButton = MediaControlFullscreenVolumeMaxButtonElement::create(document);
    controls->m_fullScreenMaxVolumeButton = fullScreenMaxVolumeButton.get();
    panel->appendChild(fullScreenMaxVolumeButton.release(), ec, true);
    if (ec)
        return 0;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    RefPtr<MediaControlRotateButtonElement> rotateButton = MediaControlRotateButtonElement::create(document);
    controls->m_rotateButton = rotateButton.get();
    controls->appendChild(rotateButton.release(), ec, true);
    if (ec)
        return 0;
#endif

    controls->m_panel = panel.get();
    controls->appendChild(panel.release(), ec, true);
    if (ec)
        return 0;

    return controls.release();
}

void MediaControlRootElement::setMediaController(MediaControllerInterface* controller)
{
    if (m_mediaController == controller)
        return;
    m_mediaController = controller;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (m_overlayContainer)
        m_overlayContainer->setMediaController(controller);
    if (m_defaultImage)
        m_defaultImage->setMediaController(controller);
    if (m_overlayLoadingSpinner)
        m_overlayLoadingSpinner->setMediaController(controller);
#endif
    if (m_rewindButton)
        m_rewindButton->setMediaController(controller);
    if (m_playButton)
        m_playButton->setMediaController(controller);
    if (m_returnToRealTimeButton)
        m_returnToRealTimeButton->setMediaController(controller);
    if (m_statusDisplay)
        m_statusDisplay->setMediaController(controller);
    if (m_currentTimeDisplay)
        m_currentTimeDisplay->setMediaController(controller);
    if (m_timeline)
        m_timeline->setMediaController(controller);
    if (m_timeRemainingDisplay)
        m_timeRemainingDisplay->setMediaController(controller);
    if (m_timelineContainer)
        m_timelineContainer->setMediaController(controller);
    if (m_seekBackButton)
        m_seekBackButton->setMediaController(controller);
    if (m_seekForwardButton)
        m_seekForwardButton->setMediaController(controller);
    if (m_toggleClosedCaptionsButton)
        m_toggleClosedCaptionsButton->setMediaController(controller);
    if (m_panelMuteButton)
        m_panelMuteButton->setMediaController(controller);
    if (m_volumeSlider)
        m_volumeSlider->setMediaController(controller);
    if (m_volumeSliderMuteButton)
        m_volumeSliderMuteButton->setMediaController(controller);
    if (m_volumeSliderContainer)
        m_volumeSliderContainer->setMediaController(controller);
    if (m_fullScreenButton)
        m_fullScreenButton->setMediaController(controller);
    if (m_fullScreenMinVolumeButton)
        m_fullScreenMinVolumeButton->setMediaController(controller);
    if (m_fullScreenVolumeSlider)
        m_fullScreenVolumeSlider->setMediaController(controller);
    if (m_fullScreenMaxVolumeButton)
        m_fullScreenMaxVolumeButton->setMediaController(controller);
    if (m_panel)
        m_panel->setMediaController(controller);
#if ENABLE(VIDEO_TRACK)
    if (m_textDisplayContainer)
        m_textDisplayContainer->setMediaController(controller);
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (m_rotateButton)
        m_rotateButton->setMediaController(controller);
#endif
    reset();
}

void MediaControlRootElement::show()
{
    m_panel->setIsDisplayed(true);
    m_panel->show();
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_overlayContainer->show();

    if (m_isFullscreen)
        m_rotateButton->setIsDisplayed(true);

    if (!m_panel->isOpaque())
        makeOpaque();

    if (m_mediaController->isVideo())
        m_hideControlsTimer.startOneShot(timeWithoutMouseMovementBeforeHidingControls);
#endif
}

void MediaControlRootElement::hide()
{
    m_panel->setIsDisplayed(false);
    m_panel->hide();
    m_volumeSliderContainer->hide();
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_overlayContainer->hide();

    if (m_isFullscreen)
        m_rotateButton->setIsDisplayed(false);

    if (m_mediaController->isVideo() && m_panel->isOpaque())
        makeTransparent();

    m_mediaController->didHideControls();
#endif
}

void MediaControlRootElement::makeOpaque()
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (!m_panel->isOpaque()) {
        m_mediaController->willShowControls();
#endif
        m_panel->makeOpaque();
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    }
    if (m_isFullscreen) {
        m_rotateButton->setIsDisplayed(true);
        m_rotateButton->makeOpaque();
    }
#endif
}

void MediaControlRootElement::makeTransparent()
{
    m_panel->makeTransparent();
    m_volumeSliderContainer->hide();
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (m_isFullscreen) {
        m_rotateButton->setIsDisplayed(false);
        m_rotateButton->makeTransparent();
    }

    m_mediaController->didHideControls();
#endif
}

void MediaControlRootElement::reset()
{
    Page* page = document()->page();
    if (!page)
        return;

    updateStatusDisplay();

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (m_mediaController->supportsFullscreen() && !m_isFullscreen)
        m_fullScreenButton->show();
#else
    if (m_mediaController->supportsFullscreen())
        m_fullScreenButton->show();
    else
        m_fullScreenButton->hide();
#endif

    float duration = m_mediaController->duration();
    if (isfinite(duration) || page->theme()->hasOwnDisabledStateHandlingFor(MediaSliderPart)) {
        m_timeline->setDuration(duration);
        m_timelineContainer->show();
        m_timeline->setPosition(m_mediaController->currentTime());
        updateTimeDisplay();
    } else
        m_timelineContainer->hide();

    if (m_mediaController->hasAudio() || page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_panelMuteButton->show();
    else
        m_panelMuteButton->hide();

    if (m_volumeSlider)
        m_volumeSlider->setVolume(m_mediaController->volume());

    if (m_toggleClosedCaptionsButton) {
        if (m_mediaController->hasClosedCaptions())
            m_toggleClosedCaptionsButton->show();
        else
            m_toggleClosedCaptionsButton->hide();
    }

    m_playButton->updateDisplayType();

#if ENABLE(FULLSCREEN_API)
    if (m_fullScreenVolumeSlider)
        m_fullScreenVolumeSlider->setVolume(m_mediaController->volume());

    if (m_isFullscreen) {
        if (m_mediaController->isLiveStream()) {
            m_seekBackButton->hide();
            m_seekForwardButton->hide();
            m_rewindButton->show();
            m_returnToRealTimeButton->show();
        } else {
            m_seekBackButton->show();
            m_seekForwardButton->show();
            m_rewindButton->hide();
            m_returnToRealTimeButton->hide();
        }
    } else
#endif
    if (!m_mediaController->isLiveStream()) {
        m_returnToRealTimeButton->hide();
        m_rewindButton->show();
    } else {
        m_returnToRealTimeButton->show();
        m_rewindButton->hide();
    }

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_rotateButton->setIsDisplayed(m_isFullscreen);

    if (m_overlayContainer)
        m_overlayContainer->show();
    if (m_defaultImage)
        if (m_defaultImage->isDisplayed() && !m_overlayLoadingSpinner->isDisplayed())
            m_defaultImage->show();
#endif
    makeOpaque();
}

void MediaControlRootElement::playbackStarted()
{
    m_playButton->updateDisplayType();
    m_timeline->setPosition(m_mediaController->currentTime());
    updateTimeDisplay();

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_hideControlsTimer.startOneShot(timeWithoutMouseMovementBeforeHidingControls);
#else
    if (m_isFullscreen)
        startHideFullscreenControlsTimer();
#endif
}

void MediaControlRootElement::playbackProgressed()
{
    m_timeline->setPosition(m_mediaController->currentTime());
    updateTimeDisplay();

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (!m_isMouseOverControls && m_mediaController->hasVideo())
        makeTransparent();
#endif
}

void MediaControlRootElement::playbackStopped()
{
    m_playButton->updateDisplayType();
    m_timeline->setPosition(m_mediaController->currentTime());
    updateTimeDisplay();
    makeOpaque();

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_hideControlsTimer.stop();
#else
    stopHideFullscreenControlsTimer();
#endif
}

void MediaControlRootElement::updateTimeDisplay()
{
    float now = m_mediaController->currentTime();
    float duration = m_mediaController->duration();

    Page* page = document()->page();
    if (!page)
        return;

    // Allow the theme to format the time.
    ExceptionCode ec;
    m_currentTimeDisplay->setInnerText(page->theme()->formatMediaControlsCurrentTime(now, duration), ec);
    m_currentTimeDisplay->setCurrentValue(now);

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_timeRemainingDisplay->setInnerText(page->theme()->formatMediaControlsTime(duration), ec);
    m_timeRemainingDisplay->setCurrentValue(duration);
#else
    m_timeRemainingDisplay->setInnerText(page->theme()->formatMediaControlsRemainingTime(now, duration), ec);
    m_timeRemainingDisplay->setCurrentValue(now - duration);
#endif
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaControlRootElement::updateTimelineDisplay()
{
    m_timeline->setPosition(m_mediaController->currentTime());
    updateTimeDisplay();
}
#endif

void MediaControlRootElement::reportedError()
{
    Page* page = document()->page();
    if (!page)
        return;

    if (!page->theme()->hasOwnDisabledStateHandlingFor(MediaSliderPart))
        m_timelineContainer->hide();

    if (!page->theme()->hasOwnDisabledStateHandlingFor(MediaMuteButtonPart))
        m_panelMuteButton->hide();

#if ENABLE(TIZEN_FULLSCREEN_API)
    if (m_fullScreenButton && !page->theme()->hasOwnDisabledStateHandlingFor(MediaEnterFullscreenButtonPart))
#endif
     m_fullScreenButton->hide();

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->hide();
    if (m_toggleClosedCaptionsButton && !page->theme()->hasOwnDisabledStateHandlingFor(MediaToggleClosedCaptionsButtonPart))
        m_toggleClosedCaptionsButton->hide();
}

void MediaControlRootElement::updateStatusDisplay()
{
    if (m_statusDisplay)
        m_statusDisplay->update();
}

void MediaControlRootElement::loadedMetadata()
{
    if (m_statusDisplay && m_mediaController->isLiveStream())
        m_statusDisplay->hide();

    reset();
}

void MediaControlRootElement::changedClosedCaptionsVisibility()
{
    if (m_toggleClosedCaptionsButton)
        m_toggleClosedCaptionsButton->updateDisplayType();
}

void MediaControlRootElement::changedMute()
{
    m_panelMuteButton->changedMute();
    if (m_volumeSliderMuteButton)
        m_volumeSliderMuteButton->changedMute();
}

void MediaControlRootElement::changedVolume()
{
    if (m_volumeSlider)
        m_volumeSlider->setVolume(m_mediaController->volume());
}

#if ENABLE(TIZEN_FULLSCREEN_API)
void MediaControlRootElement::updateMediaControlScale()
{
    Page* page = document()->page();
    if (!page)
        return;

    float scaleFactor = 1 / page->chrome()->contentsScaleFactor();
    if (page->mainFrame() && page->mainFrame()->view()) {
        FloatRect windowRect = document()->page()->chrome()->windowRect();
        float compareWidth = windowRect.width() > windowRect.height() ? compareResolution.height() : compareResolution.width();
        scaleFactor *= (windowRect.width() / compareWidth);

        TIZEN_LOGI("rect.w : %f, rect.h : %f, scale : %f", windowRect.width(), windowRect.height(), scaleFactor);
    }

    setInlineStyleProperty(CSSPropertyWebkitTransform, "scale(" + String::number(scaleFactor) + ", " + String::number(scaleFactor) + ")", false);
    setInlineStyleProperty(CSSPropertyWebkitTransformOrigin, "left top", false);
}
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaControlRootElement::stopSeek()
{
    if (m_seekForwardButton->stopSeeking())
        return;
    m_seekBackButton->stopSeeking();
}
#endif

void MediaControlRootElement::enteredFullscreen()
{
    m_isFullscreen = true;

    if (m_mediaController->isLiveStream()) {
        m_seekBackButton->hide();
        m_seekForwardButton->hide();
        m_rewindButton->show();
        m_returnToRealTimeButton->show();
    } else {
        m_seekBackButton->show();
        m_seekForwardButton->show();
        m_rewindButton->hide();
        m_returnToRealTimeButton->hide();
    }

#if ENABLE(TIZEN_FULLSCREEN_API)
    m_fullScreenButton->hide();
#else
    m_fullScreenButton->setIsFullscreen(true);
    m_panel->setCanBeDragged(true);
#endif

    if (Page* page = document()->page()) {
        page->chrome()->setCursorHiddenUntilMouseMoves(true);
    }

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    startHideFullscreenControlsTimer();
#endif
}

void MediaControlRootElement::exitedFullscreen()
{
    m_isFullscreen = false;

    // "show" actually means removal of display:none style, so we are just clearing styles
    // when exiting fullscreen.
    // FIXME: Clarify naming of show/hide <http://webkit.org/b/58157>
    m_rewindButton->show();
    m_seekBackButton->show();
    m_seekForwardButton->show();
    m_returnToRealTimeButton->show();

#if ENABLE(TIZEN_FULLSCREEN_API)
    m_fullScreenButton->show();
#else
    m_fullScreenButton->setIsFullscreen(false);
    m_panel->setCanBeDragged(false);
#endif

    // We will keep using the panel, but we want it to go back to the standard position.
    // This will matter right away because we use the panel even when not fullscreen.
    // And if we reenter fullscreen we also want the panel in the standard position.
    m_panel->resetPosition();

#if ENABLE(TIZEN_FULLSCREEN_API)
    if (m_rotateButton) {
        m_rotateButton->releaseRotationLock();
        if (m_rotateButton->isDisplayed())
            m_rotateButton->setIsDisplayed(false);
    }

    removeInlineStyleProperty(CSSPropertyWebkitTransform);
    removeInlineStyleProperty(CSSPropertyWebkitTransformOrigin);
#endif

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    stopHideFullscreenControlsTimer();
#endif
}

void MediaControlRootElement::showVolumeSlider()
{
    if (!m_mediaController->hasAudio())
        return;

    if (m_volumeSliderContainer)
        m_volumeSliderContainer->show();
}

bool MediaControlRootElement::shouldHideControls()
{
    return !m_panel->hovered();
}

bool MediaControlRootElement::containsRelatedTarget(Event* event)
{
    if (!event->isMouseEvent())
        return false;
    EventTarget* relatedTarget = static_cast<MouseEvent*>(event)->relatedTarget();
    if (!relatedTarget)
        return false;
    return contains(relatedTarget->toNode());
}

void MediaControlRootElement::defaultEventHandler(Event* event)
{
    MediaControls::defaultEventHandler(event);

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (event->type() == eventNames().clickEvent && m_mediaController->isVideo()) {
        if (!containsRelatedTarget(event)) {
            if (m_panel->isOpaque())
                makeTransparent();
            else {
                makeOpaque();
                m_hideControlsTimer.startOneShot(timeWithoutMouseMovementBeforeHidingControls);
            }
        }
    }
#else
    if (event->type() == eventNames().mouseoverEvent) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = true;
            if (!m_mediaController->canPlay()) {
                makeOpaque();
                if (shouldHideControls())
                    startHideFullscreenControlsTimer();
            }
        }
    } else if (event->type() == eventNames().mouseoutEvent) {
        if (!containsRelatedTarget(event)) {
            m_isMouseOverControls = false;
            stopHideFullscreenControlsTimer();
        }
    } else if (event->type() == eventNames().mousemoveEvent) {
        if (m_isFullscreen) {
            // When we get a mouse move in fullscreen mode, show the media controls, and start a timer
            // that will hide the media controls after a 3 seconds without a mouse move.
            makeOpaque();
            if (shouldHideControls())
                startHideFullscreenControlsTimer();
        }
    }
#endif
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaControlRootElement::hideControlsTimerFired(Timer<MediaControlRootElement>*)
{
    if (m_mediaController->paused())
        return;

    if (!m_mediaController->isVideo())
        return;

    if (m_timeline->isScrubbing()) {
        m_hideControlsTimer.stop();
        m_hideControlsTimer.startOneShot(timeWithoutMouseMovementBeforeHidingControls);
        return;
    }

    if (Page* page = document()->page())
        page->chrome()->setCursorHiddenUntilMouseMoves(true);

    makeTransparent();
}
#else
void MediaControlRootElement::startHideFullscreenControlsTimer()
{
    if (!m_isFullscreen)
        return;

    m_hideFullscreenControlsTimer.startOneShot(timeWithoutMouseMovementBeforeHidingControls);
}

void MediaControlRootElement::hideFullscreenControlsTimerFired(Timer<MediaControlRootElement>*)
{
    if (m_mediaController->paused())
        return;

    if (!m_isFullscreen)
        return;

    if (!shouldHideControls())
        return;

    if (Page* page = document()->page())
        page->chrome()->setCursorHiddenUntilMouseMoves(true);

    makeTransparent();
}

void MediaControlRootElement::stopHideFullscreenControlsTimer()
{
    m_hideFullscreenControlsTimer.stop();
}
#endif

#if ENABLE(VIDEO_TRACK)
void MediaControlRootElement::createTextTrackDisplay()
{
    if (m_textDisplayContainer)
        return;

    RefPtr<MediaControlTextTrackContainerElement> textDisplayContainer = MediaControlTextTrackContainerElement::create(document());
    m_textDisplayContainer = textDisplayContainer.get();

    // Insert it before the first controller element so it always displays behind the controls.
    ExceptionCode ec;
    insertBefore(textDisplayContainer.release(), m_panel, ec, true);
}

void MediaControlRootElement::showTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();
    m_textDisplayContainer->show();
}

void MediaControlRootElement::hideTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();
    m_textDisplayContainer->hide();
}

void MediaControlRootElement::updateTextTrackDisplay()
{
    if (!m_textDisplayContainer)
        createTextTrackDisplay();

    m_textDisplayContainer->updateDisplay();

}
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaControlRootElement::showLoadingSpinner()
{
    if (m_seekForwardButton->isLongPressSeeking() || m_seekBackButton->isLongPressSeeking())
        return;

    if (m_defaultImage->isDisplayed())
        m_defaultImage->hide();

    if (m_overlayContainer)
        m_overlayContainer->show();

    m_overlayLoadingSpinner->setIsDisplayed(true);
    m_overlayLoadingSpinner->show();
}
void MediaControlRootElement::hideLoadingSpinner()
{
    m_overlayLoadingSpinner->setIsDisplayed(false);
    m_overlayLoadingSpinner->hide();
    if (m_defaultImage->isDisplayed())
        m_defaultImage->show();
}
#endif

const AtomicString& MediaControlRootElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls"));
    return id;
}

void MediaControlRootElement::bufferingProgressed()
{
    // We only need to update buffering progress when paused, during normal
    // playback playbackProgressed() will take care of it.
    if (m_mediaController->paused())
        m_timeline->setPosition(m_mediaController->currentTime());
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaControlRootElement::updateDefaultImage(bool display)
{
    if (m_defaultImage)
        m_defaultImage->setIsDisplayed(display);
    if (display) {
        if (!m_overlayLoadingSpinner->isDisplayed())
            m_defaultImage->show();
    } else
        m_defaultImage->hide();
}
#endif

}

#endif
