/*
 * Copyright (C) 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(VIDEO)

#include "MediaControlElements.h"

#include "CSSStyleDeclaration.h"
#include "CSSValueKeywords.h"
#include "DOMTokenList.h"
#include "EventNames.h"
#include "FloatConversion.h"
#include "FloatPoint.h"
#include "Frame.h"
#include "HTMLDivElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLVideoElement.h"
#include "LayoutRepainter.h"
#include "LocalizedStrings.h"
#include "MediaControls.h"
#include "MouseEvent.h"
#include "Page.h"
#include "RenderDeprecatedFlexibleBox.h"
#include "RenderInline.h"
#include "RenderMedia.h"
#include "RenderSlider.h"
#include "RenderText.h"
#include "RenderTheme.h"
#include "RenderVideo.h"
#include "RenderView.h"
#include "ScriptController.h"
#include "Settings.h"
#include "StyleResolver.h"
#include "Text.h"

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#include "Chrome.h"
#include "ChromeClient.h"
#include "Frame.h"
#include "FrameView.h"
#include "PlatformScreen.h"
#include "Settings.h"
#include <runtime_info.h>
#endif

#if ENABLE(TIZEN_EXTENSIBLE_API)
#include "TizenExtensibleAPI.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
#include "EflScreenUtilities.h"
#endif

namespace WebCore {

using namespace HTMLNames;
using namespace std;

// FIXME: These constants may need to be tweaked to better match the seeking in the QuickTime plug-in.

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
static const float cSeekForwardTime = 5.0f;
static const float cSeekBackTime = (-5.0f);
#endif
static const float cSkipRepeatDelay = 0.25f;
static const float cSkipTime = 2.0f;

static const float cScanRepeatDelay = 1.5f;
static const float cScanMaximumRate = 8;

HTMLMediaElement* toParentMediaElement(Node* node)
{
    if (!node)
        return 0;
    Node* mediaNode = node->shadowHost();
    if (!mediaNode)
        mediaNode = node;
    if (!mediaNode || !mediaNode->isElementNode() || !static_cast<Element*>(mediaNode)->isMediaElement())
        return 0;

    return static_cast<HTMLMediaElement*>(mediaNode);
}

MediaControlElementType mediaControlElementType(Node* node)
{
    ASSERT(node->isMediaControlElement());
    HTMLElement* element = toHTMLElement(node);
    if (element->hasTagName(inputTag))
        return static_cast<MediaControlInputElement*>(element)->displayType();
    return static_cast<MediaControlElement*>(element)->displayType();
}

// ----------------------------

MediaControlElement::MediaControlElement(Document* document)
    : HTMLDivElement(divTag, document)
    , m_mediaController(0)
{
}

void MediaControlElement::show()
{
    removeInlineStyleProperty(CSSPropertyDisplay);
}

void MediaControlElement::hide()
{
    setInlineStyleProperty(CSSPropertyDisplay, CSSValueNone);
}

// ----------------------------

inline MediaControlPanelElement::MediaControlPanelElement(Document* document)
    : MediaControlElement(document)
    , m_canBeDragged(false)
    , m_isBeingDragged(false)
    , m_isDisplayed(false)
    , m_opaque(true)
    , m_transitionTimer(this, &MediaControlPanelElement::transitionTimerFired)
{
}

PassRefPtr<MediaControlPanelElement> MediaControlPanelElement::create(Document* document)
{
    return adoptRef(new MediaControlPanelElement(document));
}

MediaControlElementType MediaControlPanelElement::displayType() const
{
    return MediaControlsPanel;
}

const AtomicString& MediaControlPanelElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-panel"));
    return id;
}

void MediaControlPanelElement::startDrag(const LayoutPoint& eventLocation)
{
    if (!m_canBeDragged)
        return;

    if (m_isBeingDragged)
        return;

    RenderObject* renderer = this->renderer();
    if (!renderer || !renderer->isBox())
        return;

    Frame* frame = document()->frame();
    if (!frame)
        return;

    m_dragStartEventLocation = eventLocation;

    frame->eventHandler()->setCapturingMouseEventsNode(this);

    m_isBeingDragged = true;
}

void MediaControlPanelElement::continueDrag(const LayoutPoint& eventLocation)
{
    if (!m_isBeingDragged)
        return;

    LayoutSize distanceDragged = eventLocation - m_dragStartEventLocation;
    setPosition(LayoutPoint(distanceDragged.width(), distanceDragged.height()));
}

void MediaControlPanelElement::endDrag()
{
    if (!m_isBeingDragged)
        return;

    m_isBeingDragged = false;

    Frame* frame = document()->frame();
    if (!frame)
        return;

    frame->eventHandler()->setCapturingMouseEventsNode(0);
}

void MediaControlPanelElement::startTimer()
{
    stopTimer();

    // The timer is required to set the property display:'none' on the panel,
    // such that captions are correctly displayed at the bottom of the video
    // at the end of the fadeout transition.
    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeOutDuration() : 0;
    m_transitionTimer.startOneShot(duration);
}

void MediaControlPanelElement::stopTimer()
{
    if (m_transitionTimer.isActive())
        m_transitionTimer.stop();
}


void MediaControlPanelElement::transitionTimerFired(Timer<MediaControlPanelElement>*)
{
    if (!m_opaque)
        hide();

    stopTimer();
}

void MediaControlPanelElement::setPosition(const LayoutPoint& position)
{
    double left = position.x();
    double top = position.y();

    // Set the left and top to control the panel's position; this depends on it being absolute positioned.
    // Set the margin to zero since the position passed in will already include the effect of the margin.
    setInlineStyleProperty(CSSPropertyLeft, left, CSSPrimitiveValue::CSS_PX);
    setInlineStyleProperty(CSSPropertyTop, top, CSSPrimitiveValue::CSS_PX);
    setInlineStyleProperty(CSSPropertyMarginLeft, 0.0, CSSPrimitiveValue::CSS_PX);
    setInlineStyleProperty(CSSPropertyMarginTop, 0.0, CSSPrimitiveValue::CSS_PX);

    ExceptionCode ignored;
    classList()->add("dragged", ignored);
}

void MediaControlPanelElement::resetPosition()
{
    removeInlineStyleProperty(CSSPropertyLeft);
    removeInlineStyleProperty(CSSPropertyTop);
    removeInlineStyleProperty(CSSPropertyMarginLeft);
    removeInlineStyleProperty(CSSPropertyMarginTop);

    ExceptionCode ignored;
    classList()->remove("dragged", ignored);
}

void MediaControlPanelElement::makeOpaque()
{
    if (m_opaque)
        return;

    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeInDuration() : 0;

    setInlineStyleProperty(CSSPropertyWebkitTransitionProperty, CSSPropertyOpacity);
    setInlineStyleProperty(CSSPropertyWebkitTransitionDuration, duration, CSSPrimitiveValue::CSS_S);
    setInlineStyleProperty(CSSPropertyOpacity, 1.0, CSSPrimitiveValue::CSS_NUMBER);

    m_opaque = true;

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (m_isDisplayed)
#endif
        show();
}

void MediaControlPanelElement::makeTransparent()
{
    if (!m_opaque)
        return;

    setInlineStyleProperty(CSSPropertyWebkitTransitionProperty, CSSPropertyOpacity);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeInDuration() : 0;
    setInlineStyleProperty(CSSPropertyWebkitTransitionDuration, duration, CSSPrimitiveValue::CSS_S);
#else
    setInlineStyleProperty(CSSPropertyWebkitTransitionDuration, document()->page()->theme()->mediaControlsFadeOutDuration(), CSSPrimitiveValue::CSS_S);
#endif
    setInlineStyleProperty(CSSPropertyOpacity, 0.0, CSSPrimitiveValue::CSS_NUMBER);

    m_opaque = false;
    startTimer();
}

void MediaControlPanelElement::defaultEventHandler(Event* event)
{
    MediaControlElement::defaultEventHandler(event);

    if (event->isMouseEvent()) {
        LayoutPoint location = static_cast<MouseEvent*>(event)->absoluteLocation();
        if (event->type() == eventNames().mousedownEvent && event->target() == this) {
            startDrag(location);
            event->setDefaultHandled();
        } else if (event->type() == eventNames().mousemoveEvent && m_isBeingDragged)
            continueDrag(location);
        else if (event->type() == eventNames().mouseupEvent && m_isBeingDragged) {
            continueDrag(location);
            endDrag();
            event->setDefaultHandled();
        }
    }
}

void MediaControlPanelElement::setCanBeDragged(bool canBeDragged)
{
    if (m_canBeDragged == canBeDragged)
        return;

    m_canBeDragged = canBeDragged;

    if (!canBeDragged)
        endDrag();
}

void MediaControlPanelElement::setIsDisplayed(bool isDisplayed)
{
    m_isDisplayed = isDisplayed;
}

// ----------------------------

inline MediaControlTimelineContainerElement::MediaControlTimelineContainerElement(Document* document)
    : MediaControlElement(document)
{
}

PassRefPtr<MediaControlTimelineContainerElement> MediaControlTimelineContainerElement::create(Document* document)
{
    RefPtr<MediaControlTimelineContainerElement> element = adoptRef(new MediaControlTimelineContainerElement(document));
    element->hide();
    return element.release();
}

MediaControlElementType MediaControlTimelineContainerElement::displayType() const
{
    return MediaTimelineContainer;
}

const AtomicString& MediaControlTimelineContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-timeline-container"));
    return id;
}

// ----------------------------

class RenderMediaVolumeSliderContainer : public RenderBlock {
public:
    RenderMediaVolumeSliderContainer(Node*);

private:
    virtual void layout();
};

RenderMediaVolumeSliderContainer::RenderMediaVolumeSliderContainer(Node* node)
    : RenderBlock(node)
{
}

void RenderMediaVolumeSliderContainer::layout()
{
    RenderBlock::layout();

    if (style()->display() == NONE || !nextSibling() || !nextSibling()->isBox())
        return;

    RenderBox* buttonBox = toRenderBox(nextSibling());
    int absoluteOffsetTop = buttonBox->localToAbsolute(FloatPoint(0, -size().height())).y();

    LayoutStateDisabler layoutStateDisabler(view());

    // If the slider would be rendered outside the page, it should be moved below the controls.
    if (UNLIKELY(absoluteOffsetTop < 0))
        setY(buttonBox->offsetTop() + theme()->volumeSliderOffsetFromMuteButton(buttonBox, pixelSnappedSize()).y());
}

inline MediaControlVolumeSliderContainerElement::MediaControlVolumeSliderContainerElement(Document* document)
    : MediaControlElement(document)
{
}

PassRefPtr<MediaControlVolumeSliderContainerElement> MediaControlVolumeSliderContainerElement::create(Document* document)
{
    RefPtr<MediaControlVolumeSliderContainerElement> element = adoptRef(new MediaControlVolumeSliderContainerElement(document));
    element->hide();
    return element.release();
}

RenderObject* MediaControlVolumeSliderContainerElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderMediaVolumeSliderContainer(this);
}

void MediaControlVolumeSliderContainerElement::defaultEventHandler(Event* event)
{
    if (!event->isMouseEvent() || event->type() != eventNames().mouseoutEvent)
        return;

    // Poor man's mouseleave event detection.
    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    if (!mouseEvent->relatedTarget() || !mouseEvent->relatedTarget()->toNode())
        return;

    if (this->containsIncludingShadowDOM(mouseEvent->relatedTarget()->toNode()))
        return;

    hide();
}

MediaControlElementType MediaControlVolumeSliderContainerElement::displayType() const
{
    return MediaVolumeSliderContainer;
}

const AtomicString& MediaControlVolumeSliderContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-volume-slider-container"));
    return id;
}

// ----------------------------

inline MediaControlStatusDisplayElement::MediaControlStatusDisplayElement(Document* document)
    : MediaControlElement(document)
    , m_stateBeingDisplayed(Nothing)
{
}

PassRefPtr<MediaControlStatusDisplayElement> MediaControlStatusDisplayElement::create(Document* document)
{
    RefPtr<MediaControlStatusDisplayElement> element = adoptRef(new MediaControlStatusDisplayElement(document));
    element->hide();
    return element.release();
}

void MediaControlStatusDisplayElement::update()
{
    // Get the new state that we'll have to display.
    StateBeingDisplayed newStateToDisplay = Nothing;

    if (mediaController()->readyState() <= MediaControllerInterface::HAVE_METADATA && mediaController()->hasCurrentSrc())
        newStateToDisplay = Loading;
    else if (mediaController()->isLiveStream())
        newStateToDisplay = LiveBroadcast;

    if (newStateToDisplay == m_stateBeingDisplayed)
        return;

    ExceptionCode e;

    if (m_stateBeingDisplayed == Nothing)
        show();
    else if (newStateToDisplay == Nothing)
        hide();

    m_stateBeingDisplayed = newStateToDisplay;

    switch (m_stateBeingDisplayed) {
    case Nothing:
        setInnerText("", e);
        break;
    case Loading:
        setInnerText(mediaElementLoadingStateText(), e);
        break;
    case LiveBroadcast:
        setInnerText(mediaElementLiveBroadcastStateText(), e);
        break;
    }
}

MediaControlElementType MediaControlStatusDisplayElement::displayType() const
{
    return MediaStatusDisplay;
}

const AtomicString& MediaControlStatusDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-status-display"));
    return id;
}

// ----------------------------

MediaControlInputElement::MediaControlInputElement(Document* document, MediaControlElementType displayType)
    : HTMLInputElement(inputTag, document, 0, false)
    , m_mediaController(0)
    , m_displayType(displayType)
{
}

void MediaControlInputElement::show()
{
    removeInlineStyleProperty(CSSPropertyDisplay);
}

void MediaControlInputElement::hide()
{
    setInlineStyleProperty(CSSPropertyDisplay, CSSValueNone);
}


void MediaControlInputElement::setDisplayType(MediaControlElementType displayType)
{
    if (displayType == m_displayType)
        return;

    m_displayType = displayType;
    if (RenderObject* object = renderer())
        object->repaint();
}

// ----------------------------

inline MediaControlMuteButtonElement::MediaControlMuteButtonElement(Document* document, MediaControlElementType displayType)
    : MediaControlInputElement(document, displayType)
{
}

void MediaControlMuteButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaController()->setMuted(!mediaController()->muted());
        event->setDefaultHandled();
    }

    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlMuteButtonElement::changedMute()
{
    updateDisplayType();
}

void MediaControlMuteButtonElement::updateDisplayType()
{
    setDisplayType(mediaController()->muted() ? MediaUnMuteButton : MediaMuteButton);
}

// ----------------------------

inline MediaControlPanelMuteButtonElement::MediaControlPanelMuteButtonElement(Document* document, MediaControls* controls)
    : MediaControlMuteButtonElement(document, MediaMuteButton)
    , m_controls(controls)
{
}

PassRefPtr<MediaControlPanelMuteButtonElement> MediaControlPanelMuteButtonElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlPanelMuteButtonElement> button = adoptRef(new MediaControlPanelMuteButtonElement(document, controls));
    button->createShadowSubtree();
    button->setType("button");
    return button.release();
}

void MediaControlPanelMuteButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().mouseoverEvent)
        m_controls->showVolumeSlider();

    MediaControlMuteButtonElement::defaultEventHandler(event);
}

const AtomicString& MediaControlPanelMuteButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-mute-button"));
    return id;
}

// ----------------------------

inline MediaControlVolumeSliderMuteButtonElement::MediaControlVolumeSliderMuteButtonElement(Document* document)
    : MediaControlMuteButtonElement(document, MediaMuteButton)
{
}

PassRefPtr<MediaControlVolumeSliderMuteButtonElement> MediaControlVolumeSliderMuteButtonElement::create(Document* document)
{
    RefPtr<MediaControlVolumeSliderMuteButtonElement> button = adoptRef(new MediaControlVolumeSliderMuteButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    return button.release();
}

const AtomicString& MediaControlVolumeSliderMuteButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-volume-slider-mute-button"));
    return id;
}

// ----------------------------

inline MediaControlPlayButtonElement::MediaControlPlayButtonElement(Document* document)
    : MediaControlInputElement(document, MediaPlayButton)
{
}

PassRefPtr<MediaControlPlayButtonElement> MediaControlPlayButtonElement::create(Document* document)
{
    RefPtr<MediaControlPlayButtonElement> button = adoptRef(new MediaControlPlayButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    return button.release();
}

void MediaControlPlayButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("clickEvent");
#endif
        if (mediaController()->canPlay())
            mediaController()->play();
        else
            mediaController()->pause();
        updateDisplayType();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlPlayButtonElement::updateDisplayType()
{
    setDisplayType(mediaController()->canPlay() ? MediaPlayButton : MediaPauseButton);
}

const AtomicString& MediaControlPlayButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-play-button"));
    return id;
}

// ----------------------------

inline MediaControlOverlayPlayButtonElement::MediaControlOverlayPlayButtonElement(Document* document)
    : MediaControlInputElement(document, MediaOverlayPlayButton)
{
}

PassRefPtr<MediaControlOverlayPlayButtonElement> MediaControlOverlayPlayButtonElement::create(Document* document)
{
    RefPtr<MediaControlOverlayPlayButtonElement> button = adoptRef(new MediaControlOverlayPlayButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    return button.release();
}

void MediaControlOverlayPlayButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent && mediaController()->canPlay()) {
        mediaController()->play();
        updateDisplayType();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlOverlayPlayButtonElement::updateDisplayType()
{
    if (mediaController()->canPlay()) {
        show();
        setDisplayType(MediaOverlayPlayButton);
    } else
        hide();
}

const AtomicString& MediaControlOverlayPlayButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-overlay-play-button"));
    return id;
}

// ----------------------------
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
inline MediaControlSeekButtonElement::MediaControlSeekButtonElement(Document* document, MediaControlElementType displayType)
    : MediaControlInputElement(document, displayType)
    , m_seekTimer(this, &MediaControlSeekButtonElement::seekTimerFired)
{
}

void MediaControlSeekButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        ExceptionCode ignoredCode;
        float seekTime = isForwardButton() ? cSeekForwardTime : cSeekBackTime;
        mediaController()->setCurrentTime(mediaController()->currentTime() + seekTime, ignoredCode);
        event->setDefaultHandled();
    }
}

void MediaControlSeekButtonElement::setActive(bool flag, bool pause)
{
    if (flag && !m_seekTimer.isActive())
        m_seekTimer.start(1, cSkipRepeatDelay);
    else if (m_seekTimer.isActive())
        m_seekTimer.stop();

    MediaControlInputElement::setActive(flag, pause);
}

bool MediaControlSeekButtonElement::stopSeeking()
{
    if (m_seekTimer.isActive()) {
        m_seekTimer.stop();
        return true;
    }
    return false;
}

void MediaControlSeekButtonElement::seekTimerFired(Timer<MediaControlSeekButtonElement>*)
{
    ExceptionCode ec;
    float skipTime = isForwardButton() ? cSkipTime : -cSkipTime;
    mediaController()->setCurrentTime(mediaController()->currentTime() + skipTime, ec);
}
#else
inline MediaControlSeekButtonElement::MediaControlSeekButtonElement(Document* document, MediaControlElementType displayType)
    : MediaControlInputElement(document, displayType)
    , m_actionOnStop(Nothing)
    , m_seekType(Skip)
    , m_seekTimer(this, &MediaControlSeekButtonElement::seekTimerFired)
{
}

void MediaControlSeekButtonElement::defaultEventHandler(Event* event)
{
    // Set the mousedown and mouseup events as defaultHandled so they
    // do not trigger drag start or end actions in MediaControlPanelElement.
    if (event->type() == eventNames().mousedownEvent || event->type() == eventNames().mouseupEvent)
        event->setDefaultHandled();
}

void MediaControlSeekButtonElement::setActive(bool flag, bool pause)
{
    if (flag == active())
        return;

    if (flag)
        startTimer();
    else
        stopTimer();

    MediaControlInputElement::setActive(flag, pause);
}

void MediaControlSeekButtonElement::startTimer()
{
    m_seekType = mediaController()->supportsScanning() ? Scan : Skip;

    if (m_seekType == Skip) {
        // Seeking by skipping requires the video to be paused during seeking.
        m_actionOnStop = mediaController()->paused() ? Nothing : Play;
        mediaController()->pause();
    } else {
        // Seeking by scanning requires the video to be playing during seeking.
        m_actionOnStop = mediaController()->paused() ? Pause : Nothing;
        mediaController()->play();
        mediaController()->setPlaybackRate(nextRate());
    }

    m_seekTimer.start(0, m_seekType == Skip ? cSkipRepeatDelay : cScanRepeatDelay);
}

void MediaControlSeekButtonElement::stopTimer()
{
    if (m_seekType == Scan)
        mediaController()->setPlaybackRate(mediaController()->defaultPlaybackRate());

    if (m_actionOnStop == Play)
        mediaController()->play();
    else if (m_actionOnStop == Pause)
        mediaController()->pause();

    if (m_seekTimer.isActive())
        m_seekTimer.stop();
}

float MediaControlSeekButtonElement::nextRate() const
{
    float rate = std::min(cScanMaximumRate, fabsf(mediaController()->playbackRate() * 2));
    if (!isForwardButton())
        rate *= -1;
    return rate;
}

void MediaControlSeekButtonElement::seekTimerFired(Timer<MediaControlSeekButtonElement>*)
{
    if (m_seekType == Skip) {
        ExceptionCode ec;
        float skipTime = isForwardButton() ? cSkipTime : -cSkipTime;
        mediaController()->setCurrentTime(mediaController()->currentTime() + skipTime, ec);
    } else
        mediaController()->setPlaybackRate(nextRate());
}
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

// ----------------------------

inline MediaControlSeekForwardButtonElement::MediaControlSeekForwardButtonElement(Document* document)
    : MediaControlSeekButtonElement(document, MediaSeekForwardButton)
{
}

PassRefPtr<MediaControlSeekForwardButtonElement> MediaControlSeekForwardButtonElement::create(Document* document)
{
    RefPtr<MediaControlSeekForwardButtonElement> button = adoptRef(new MediaControlSeekForwardButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    return button.release();
}

const AtomicString& MediaControlSeekForwardButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-seek-forward-button"));
    return id;
}

// ----------------------------

inline MediaControlSeekBackButtonElement::MediaControlSeekBackButtonElement(Document* document)
    : MediaControlSeekButtonElement(document, MediaSeekBackButton)
{
}

PassRefPtr<MediaControlSeekBackButtonElement> MediaControlSeekBackButtonElement::create(Document* document)
{
    RefPtr<MediaControlSeekBackButtonElement> button = adoptRef(new MediaControlSeekBackButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    return button.release();
}

const AtomicString& MediaControlSeekBackButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-seek-back-button"));
    return id;
}

// ----------------------------

inline MediaControlRewindButtonElement::MediaControlRewindButtonElement(Document* document)
    : MediaControlInputElement(document, MediaRewindButton)
{
}

PassRefPtr<MediaControlRewindButtonElement> MediaControlRewindButtonElement::create(Document* document)
{
    RefPtr<MediaControlRewindButtonElement> button = adoptRef(new MediaControlRewindButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    return button.release();
}

void MediaControlRewindButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        ExceptionCode ignoredCode;
        mediaController()->setCurrentTime(max(0.0f, mediaController()->currentTime() - 30), ignoredCode);
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlRewindButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-rewind-button"));
    return id;
}

// ----------------------------

inline MediaControlReturnToRealtimeButtonElement::MediaControlReturnToRealtimeButtonElement(Document* document)
    : MediaControlInputElement(document, MediaReturnToRealtimeButton)
{
}

PassRefPtr<MediaControlReturnToRealtimeButtonElement> MediaControlReturnToRealtimeButtonElement::create(Document* document)
{
    RefPtr<MediaControlReturnToRealtimeButtonElement> button = adoptRef(new MediaControlReturnToRealtimeButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlReturnToRealtimeButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaController()->returnToRealtime();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlReturnToRealtimeButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-return-to-realtime-button"));
    return id;
}

// ----------------------------

inline MediaControlToggleClosedCaptionsButtonElement::MediaControlToggleClosedCaptionsButtonElement(Document* document)
    : MediaControlInputElement(document, MediaShowClosedCaptionsButton)
{
}

PassRefPtr<MediaControlToggleClosedCaptionsButtonElement> MediaControlToggleClosedCaptionsButtonElement::create(Document* document)
{
    RefPtr<MediaControlToggleClosedCaptionsButtonElement> button = adoptRef(new MediaControlToggleClosedCaptionsButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    button->hide();
    return button.release();
}

void MediaControlToggleClosedCaptionsButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        mediaController()->setClosedCaptionsVisible(!mediaController()->closedCaptionsVisible());
        setChecked(mediaController()->closedCaptionsVisible());
        updateDisplayType();
        event->setDefaultHandled();
    }

    HTMLInputElement::defaultEventHandler(event);
}

void MediaControlToggleClosedCaptionsButtonElement::updateDisplayType()
{
    setDisplayType(mediaController()->closedCaptionsVisible() ? MediaHideClosedCaptionsButton : MediaShowClosedCaptionsButton);
}

const AtomicString& MediaControlToggleClosedCaptionsButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-toggle-closed-captions-button"));
    return id;
}

// ----------------------------

MediaControlTimelineElement::MediaControlTimelineElement(Document* document, MediaControls* controls)
    : MediaControlInputElement(document, MediaSlider)
    , m_controls(controls)
 #if ENABLE(TIZEN_GSTREAMER_VIDEO)
    , m_isScrubbing(false)
#endif
{
}

PassRefPtr<MediaControlTimelineElement> MediaControlTimelineElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlTimelineElement> timeline = adoptRef(new MediaControlTimelineElement(document, controls));
    timeline->createShadowSubtree();
    timeline->setType("range");
    timeline->setAttribute(precisionAttr, "float");
    return timeline.release();
}

void MediaControlTimelineElement::defaultEventHandler(Event* event)
{
    // Left button is 0. Rejects mouse events not from left button.
    if (event->isMouseEvent() && static_cast<MouseEvent*>(event)->button())
        return;

    if (!attached())
        return;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (event->type() == eventNames().mousedownEvent || event->type() == eventNames().touchstartEvent) {
        m_isScrubbing = true;
        mediaController()->beginScrubbing();
    } else if (event->type() == eventNames().mouseupEvent || event->type() == eventNames().touchendEvent) {
        m_isScrubbing = false;
        mediaController()->endScrubbing();
    }

    MediaControlInputElement::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent
        || event->type() == eventNames().mouseoutEvent
        || event->type() == eventNames().mousemoveEvent
        || event->type() == eventNames().touchmoveEvent)
        return;

    float time = narrowPrecisionToFloat(value().toDouble());

    // Time value on touch end event could be same with previous seeked time.
    // Seeking as same value with previous one would be rejected.
    if (event->type() == eventNames().touchendEvent || event->type() == eventNames().mouseupEvent)
        time = (time >= mediaController()->duration()) ? mediaController()->duration() - 0.1f : time + 0.001f;

    // Need to call setCurrentTime() on touchendEvent.
    // Because we ignore seeking of remote source by scrubbing.
    if ((event->type() == eventNames().inputEvent
        || event->type() == eventNames().touchendEvent
        || event->type() == eventNames().mouseupEvent
        || event->type() == eventNames().clickEvent) && time != mediaController()->currentTime()) {
        ExceptionCode ec;
        mediaController()->setCurrentTime(time, ec);
    }

    m_controls->updateTimeDisplay();
#else
    if (event->type() == eventNames().mousedownEvent)
        mediaController()->beginScrubbing();

    if (event->type() == eventNames().mouseupEvent)
        mediaController()->endScrubbing();

    MediaControlInputElement::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent || event->type() == eventNames().mouseoutEvent || event->type() == eventNames().mousemoveEvent)
        return;

    float time = narrowPrecisionToFloat(value().toDouble());

    if (event->type() == eventNames().inputEvent && time != mediaController()->currentTime()) {
        ExceptionCode ec;
        mediaController()->setCurrentTime(time, ec);
    }

    RenderSlider* slider = toRenderSlider(renderer());
    if (slider && slider->inDragMode())
        m_controls->updateTimeDisplay();
#endif
}

bool MediaControlTimelineElement::willRespondToMouseClickEvents()
{
    if (!attached())
        return false;

    return true;
}

void MediaControlTimelineElement::setPosition(float currentTime)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    // Display the slider thumb at 0 if currentTime is invalid
    if (isinf(currentTime) || isnan(currentTime))
        currentTime = 0;
#endif
    setValue(String::number(currentTime));
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (RenderObject* object = renderer())
        object->repaint();
#endif
}

void MediaControlTimelineElement::setDuration(float duration)
{
    setAttribute(maxAttr, String::number(isfinite(duration) ? duration : 0));
}

const AtomicString& MediaControlTimelineElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-timeline"));
    return id;
}

// ----------------------------

inline MediaControlVolumeSliderElement::MediaControlVolumeSliderElement(Document* document)
    : MediaControlInputElement(document, MediaVolumeSlider)
    , m_clearMutedOnUserInteraction(false)
{
}

PassRefPtr<MediaControlVolumeSliderElement> MediaControlVolumeSliderElement::create(Document* document)
{
    RefPtr<MediaControlVolumeSliderElement> slider = adoptRef(new MediaControlVolumeSliderElement(document));
    slider->createShadowSubtree();
    slider->setType("range");
    slider->setAttribute(precisionAttr, "float");
    slider->setAttribute(maxAttr, "1");
    return slider.release();
}

void MediaControlVolumeSliderElement::defaultEventHandler(Event* event)
{
    // Left button is 0. Rejects mouse events not from left button.
    if (event->isMouseEvent() && static_cast<MouseEvent*>(event)->button())
        return;

    if (!attached())
        return;

    MediaControlInputElement::defaultEventHandler(event);

    if (event->type() == eventNames().mouseoverEvent || event->type() == eventNames().mouseoutEvent || event->type() == eventNames().mousemoveEvent)
        return;

    float volume = narrowPrecisionToFloat(value().toDouble());
    if (volume != mediaController()->volume()) {
        ExceptionCode ec = 0;
        mediaController()->setVolume(volume, ec);
        ASSERT(!ec);
    }
    if (m_clearMutedOnUserInteraction)
        mediaController()->setMuted(false);
}

bool MediaControlVolumeSliderElement::willRespondToMouseMoveEvents()
{
    if (!attached())
        return false;

    return MediaControlInputElement::willRespondToMouseMoveEvents();
}

bool MediaControlVolumeSliderElement::willRespondToMouseClickEvents()
{
    if (!attached())
        return false;

    return MediaControlInputElement::willRespondToMouseClickEvents();
}

void MediaControlVolumeSliderElement::setVolume(float volume)
{
    if (value().toFloat() != volume)
        setValue(String::number(volume));
}

void MediaControlVolumeSliderElement::setClearMutedOnUserInteraction(bool clearMute)
{
    m_clearMutedOnUserInteraction = clearMute;
}

const AtomicString& MediaControlVolumeSliderElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-volume-slider"));
    return id;
}

// ----------------------------

inline MediaControlFullscreenVolumeSliderElement::MediaControlFullscreenVolumeSliderElement(Document* document)
    : MediaControlVolumeSliderElement(document)
{
}

PassRefPtr<MediaControlFullscreenVolumeSliderElement> MediaControlFullscreenVolumeSliderElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenVolumeSliderElement> slider = adoptRef(new MediaControlFullscreenVolumeSliderElement(document));
    slider->createShadowSubtree();
    slider->setType("range");
    slider->setAttribute(precisionAttr, "float");
    slider->setAttribute(maxAttr, "1");
    return slider.release();
}

const AtomicString& MediaControlFullscreenVolumeSliderElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-volume-slider"));
    return id;
}

// ----------------------------

inline MediaControlFullscreenButtonElement::MediaControlFullscreenButtonElement(Document* document, MediaControls*)
    : MediaControlInputElement(document, MediaEnterFullscreenButton)
{
}

PassRefPtr<MediaControlFullscreenButtonElement> MediaControlFullscreenButtonElement::create(Document* document, MediaControls* controls)
{
    ASSERT(controls);

    RefPtr<MediaControlFullscreenButtonElement> button = adoptRef(new MediaControlFullscreenButtonElement(document, controls));
    button->createShadowSubtree();
    button->setType("button");
#if ENABLE(TIZEN_FULLSCREEN_API)
    button->setDisplayType(MediaFullscreenDisabledButton);
#else
    button->hide();
#endif
    return button.release();
}

void MediaControlFullscreenButtonElement::defaultEventHandler(Event* event)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (event->type() == eventNames().clickEvent && displayType() != MediaFullscreenDisabledButton) {
#else
    if (event->type() == eventNames().clickEvent) {
#endif
#if ENABLE(FULLSCREEN_API)
        // Only use the new full screen API if the fullScreenEnabled setting has
        // been explicitly enabled. Otherwise, use the old fullscreen API. This
        // allows apps which embed a WebView to retain the existing full screen
        // video implementation without requiring them to implement their own full
        // screen behavior.
        if (document()->settings() && document()->settings()->fullScreenEnabled()) {
            if (document()->webkitIsFullScreen() && document()->webkitCurrentFullScreenElement() == toParentMediaElement(this))
                document()->webkitCancelFullScreen();
            else {
#if ENABLE(TIZEN_FULLSCREEN_API)
                document()->requestFullScreenForElement(toParentMediaElement(this), Element::HARDWARE_BACKKEY_EXIT, Document::ExemptIFrameAllowFullScreenRequirement);
#else
                document()->requestFullScreenForElement(toParentMediaElement(this), 0, Document::ExemptIFrameAllowFullScreenRequirement);
#endif
            }
        } else
#endif
            mediaController()->enterFullscreen();
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-button"));
    return id;
}

void MediaControlFullscreenButtonElement::setIsFullscreen(bool isFullscreen)
{
    setDisplayType(isFullscreen ? MediaExitFullscreenButton : MediaEnterFullscreenButton);
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaControlFullscreenButtonElement::show()
{
    if (displayType() == MediaFullscreenDisabledButton)
        setDisplayType(MediaEnterFullscreenButton);

    removeInlineStyleProperty(CSSPropertyDisplay);
}
#endif

// ----------------------------

inline MediaControlFullscreenVolumeMinButtonElement::MediaControlFullscreenVolumeMinButtonElement(Document* document)
    : MediaControlInputElement(document, MediaUnMuteButton)
{
}

PassRefPtr<MediaControlFullscreenVolumeMinButtonElement> MediaControlFullscreenVolumeMinButtonElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenVolumeMinButtonElement> button = adoptRef(new MediaControlFullscreenVolumeMinButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    return button.release();
}

void MediaControlFullscreenVolumeMinButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        ExceptionCode code = 0;
        mediaController()->setVolume(0, code);
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenVolumeMinButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-volume-min-button"));
    return id;
}

// ----------------------------

inline MediaControlFullscreenVolumeMaxButtonElement::MediaControlFullscreenVolumeMaxButtonElement(Document* document)
: MediaControlInputElement(document, MediaMuteButton)
{
}

PassRefPtr<MediaControlFullscreenVolumeMaxButtonElement> MediaControlFullscreenVolumeMaxButtonElement::create(Document* document)
{
    RefPtr<MediaControlFullscreenVolumeMaxButtonElement> button = adoptRef(new MediaControlFullscreenVolumeMaxButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    return button.release();
}

void MediaControlFullscreenVolumeMaxButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        ExceptionCode code = 0;
        mediaController()->setVolume(1, code);
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlFullscreenVolumeMaxButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-fullscreen-volume-max-button"));
    return id;
}

// ----------------------------

class RenderMediaControlTimeDisplay : public RenderDeprecatedFlexibleBox {
public:
    RenderMediaControlTimeDisplay(Element*);

private:
    virtual void layout();
};

RenderMediaControlTimeDisplay::RenderMediaControlTimeDisplay(Element* element)
    : RenderDeprecatedFlexibleBox(element)
{
}

// We want the timeline slider to be at least 100 pixels wide.
// FIXME: Eliminate hard-coded widths altogether.
static const int minWidthToDisplayTimeDisplays = 45 + 100 + 45;

void RenderMediaControlTimeDisplay::layout()
{
    RenderDeprecatedFlexibleBox::layout();
    RenderBox* timelineContainerBox = parentBox();
    while (timelineContainerBox && timelineContainerBox->isAnonymous())
        timelineContainerBox = timelineContainerBox->parentBox();

    if (timelineContainerBox && timelineContainerBox->width() < minWidthToDisplayTimeDisplays)
        setWidth(0);
}

inline MediaControlTimeDisplayElement::MediaControlTimeDisplayElement(Document* document)
    : MediaControlElement(document)
    , m_currentValue(0)
{
}

void MediaControlTimeDisplayElement::setCurrentValue(float time)
{
    m_currentValue = time;
}

RenderObject* MediaControlTimeDisplayElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderMediaControlTimeDisplay(this);
}

// ----------------------------

PassRefPtr<MediaControlTimeRemainingDisplayElement> MediaControlTimeRemainingDisplayElement::create(Document* document)
{
    return adoptRef(new MediaControlTimeRemainingDisplayElement(document));
}

MediaControlTimeRemainingDisplayElement::MediaControlTimeRemainingDisplayElement(Document* document)
    : MediaControlTimeDisplayElement(document)
{
}

MediaControlElementType MediaControlTimeRemainingDisplayElement::displayType() const
{
    return MediaTimeRemainingDisplay;
}

const AtomicString& MediaControlTimeRemainingDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-time-remaining-display"));
    return id;
}

// ----------------------------

PassRefPtr<MediaControlCurrentTimeDisplayElement> MediaControlCurrentTimeDisplayElement::create(Document* document)
{
    return adoptRef(new MediaControlCurrentTimeDisplayElement(document));
}

MediaControlCurrentTimeDisplayElement::MediaControlCurrentTimeDisplayElement(Document* document)
    : MediaControlTimeDisplayElement(document)
{
}

MediaControlElementType MediaControlCurrentTimeDisplayElement::displayType() const
{
    return MediaCurrentTimeDisplay;
}

const AtomicString& MediaControlCurrentTimeDisplayElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-current-time-display"));
    return id;
}

// ----------------------------

#if ENABLE(VIDEO_TRACK)

class RenderTextTrackContainerElement : public RenderBlock {
public:
    RenderTextTrackContainerElement(Node*);

private:
    virtual void layout();
};

RenderTextTrackContainerElement::RenderTextTrackContainerElement(Node* node)
    : RenderBlock(node)
{
}

void RenderTextTrackContainerElement::layout()
{
    RenderBlock::layout();
    if (style()->display() == NONE)
        return;

    ASSERT(mediaControlElementType(node()) == MediaTextTrackDisplayContainer);

    LayoutStateDisabler layoutStateDisabler(view());
    static_cast<MediaControlTextTrackContainerElement*>(node())->updateSizes();
}

inline MediaControlTextTrackContainerElement::MediaControlTextTrackContainerElement(Document* document)
    : MediaControlElement(document)
    , m_fontSize(0)
{
}

PassRefPtr<MediaControlTextTrackContainerElement> MediaControlTextTrackContainerElement::create(Document* document)
{
    RefPtr<MediaControlTextTrackContainerElement> element = adoptRef(new MediaControlTextTrackContainerElement(document));
    element->hide();
    return element.release();
}

RenderObject* MediaControlTextTrackContainerElement::createRenderer(RenderArena* arena, RenderStyle*)
{
    return new (arena) RenderTextTrackContainerElement(this);
}

const AtomicString& MediaControlTextTrackContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-text-track-container"));
    return id;
}

void MediaControlTextTrackContainerElement::updateDisplay()
{
    HTMLMediaElement* mediaElement = toParentMediaElement(this);

    // 1. If the media element is an audio element, or is another playback
    // mechanism with no rendering area, abort these steps. There is nothing to
    // render.
    if (!mediaElement->isVideo())
        return;

    // 2. Let video be the media element or other playback mechanism.
    HTMLVideoElement* video = static_cast<HTMLVideoElement*>(mediaElement);

    // 3. Let output be an empty list of absolutely positioned CSS block boxes.
    Vector<RefPtr<HTMLDivElement> > output;

    // 4. If the user agent is exposing a user interface for video, add to
    // output one or more completely transparent positioned CSS block boxes that
    // cover the same region as the user interface.

    // 5. If the last time these rules were run, the user agent was not exposing
    // a user interface for video, but now it is, let reset be true. Otherwise,
    // let reset be false.

    // There is nothing to be done explicitly for 4th and 5th steps, as
    // everything is handled through CSS. The caption box is on top of the
    // controls box, in a container set with the -webkit-box display property.

    // 6. Let tracks be the subset of video's list of text tracks that have as
    // their rules for updating the text track rendering these rules for
    // updating the display of WebVTT text tracks, and whose text track mode is
    // showing or showing by default.
    // 7. Let cues be an empty list of text track cues.
    // 8. For each track track in tracks, append to cues all the cues from
    // track's list of cues that have their text track cue active flag set.
    CueList activeCues = video->currentlyActiveCues();

    // 9. If reset is false, then, for each text track cue cue in cues: if cue's
    // text track cue display state has a set of CSS boxes, then add those boxes
    // to output, and remove cue from cues.

    // There is nothing explicitly to be done here, as all the caching occurs
    // within the TextTrackCue instance itself. If parameters of the cue change,
    // the display tree is cleared.

    // 10. For each text track cue cue in cues that has not yet had
    // corresponding CSS boxes added to output, in text track cue order, run the
    // following substeps:
    for (size_t i = 0; i < activeCues.size(); ++i) {
        TextTrackCue* cue = activeCues[i].data();

        ASSERT(cue->isActive());
        if (!cue->track() || !cue->track()->isRendered())
            continue;

        RefPtr<TextTrackCueBox> displayBox = cue->getDisplayTree();

        if (displayBox->hasChildNodes() && !contains(static_cast<Node*>(displayBox.get())))
            // Note: the display tree of a cue is removed when the active flag of the cue is unset.
            appendChild(displayBox, ASSERT_NO_EXCEPTION, false);
    }

    // 11. Return output.
    hasChildNodes() ? show() : hide();
}

static const float mimimumFontSize = 16;
static const float videoHeightFontSizePercentage = .05;
static const float trackBottomMultiplier = .9;

void MediaControlTextTrackContainerElement::updateSizes()
{
    HTMLMediaElement* mediaElement = toParentMediaElement(this);
    if (!mediaElement || !mediaElement->renderer() || !mediaElement->renderer()->isVideo())
        return;

    IntRect videoBox = toRenderVideo(mediaElement->renderer())->videoBox();
    if (m_videoDisplaySize == videoBox)
        return;
    m_videoDisplaySize = videoBox;

    float fontSize = m_videoDisplaySize.size().height() * videoHeightFontSizePercentage;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (document()->settings() && mediaElement->isFullscreen())
        fontSize = (document()->settings()->deviceWidth() < document()->settings()->deviceHeight()) ? mimimumFontSize * 2 : mimimumFontSize * 2.5;
#endif

    if (fontSize != m_fontSize) {
        m_fontSize = fontSize;
        setInlineStyleProperty(CSSPropertyFontSize, String::number(fontSize) + "px");

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        float shadowSize = (fontSize * videoHeightFontSizePercentage < 1) ? 1 : fontSize * videoHeightFontSizePercentage;
        setInlineStyleProperty(CSSPropertyTextShadow, String::number(shadowSize) + "px " + String::number(shadowSize) + "px 0px black");
#endif
    }
}

#endif // ENABLE(VIDEO_TRACK)

// ----------------------------

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
inline MediaControlOverlayContainerElement::MediaControlOverlayContainerElement(Document* document)
    : MediaControlElement(document)
{
}

PassRefPtr<MediaControlOverlayContainerElement> MediaControlOverlayContainerElement::create(Document* document)
{
    RefPtr<MediaControlOverlayContainerElement> element = adoptRef(new MediaControlOverlayContainerElement(document));
    element->hide();
    return element.release();
}

const AtomicString& MediaControlOverlayContainerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-overlay-container"));
    return id;
}

inline MediaControlDefaultImageElement::MediaControlDefaultImageElement(Document* document)
    : MediaControlElement(document)
    , m_isDisplayed(false)
{
}

PassRefPtr<MediaControlDefaultImageElement> MediaControlDefaultImageElement::create(Document* document)
{
    RefPtr<MediaControlDefaultImageElement> element = adoptRef(new MediaControlDefaultImageElement(document));
    element->hide();
    return element.release();
}

const AtomicString& MediaControlDefaultImageElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-default-image"));
    return id;
}

void MediaControlDefaultImageElement::setIsDisplayed(bool isDisplayed)
{
    if (m_isDisplayed == isDisplayed)
        return;

    m_isDisplayed = isDisplayed;
}

// ----------------------------

inline MediaControlOverlayLoadingSpinnerElement::MediaControlOverlayLoadingSpinnerElement(Document* document)
    : MediaControlElement(document)
    , m_isDisplayed(false)
{
}

PassRefPtr<MediaControlOverlayLoadingSpinnerElement> MediaControlOverlayLoadingSpinnerElement::create(Document* document)
{
    RefPtr<MediaControlOverlayLoadingSpinnerElement> element = adoptRef(new MediaControlOverlayLoadingSpinnerElement(document));
    element->hide();
    return element.release();
}

void MediaControlOverlayLoadingSpinnerElement::setIsDisplayed(bool isDisplayed)
{
    if (m_isDisplayed == isDisplayed)
        return;

    m_isDisplayed = isDisplayed;
}

const AtomicString& MediaControlOverlayLoadingSpinnerElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-overlay-loading-spinner"));
    return id;
}

// ----------------------------

inline MediaControlRotateButtonElement::MediaControlRotateButtonElement(Document* document)
    : MediaControlInputElement(document, MediaRotateButton)
    , m_isDisplayed(false)
    , m_opaque(true)
    , m_transitionTimer(this, &MediaControlRotateButtonElement::transitionTimerFired)
    , m_isLocked(false)
{
}

MediaControlRotateButtonElement::~MediaControlRotateButtonElement()
{
    runtime_info_unset_changed_cb(RUNTIME_INFO_KEY_AUTO_ROTATION_ENABLED);
}

PassRefPtr<MediaControlRotateButtonElement> MediaControlRotateButtonElement::create(Document* document)
{
    RefPtr<MediaControlRotateButtonElement> button = adoptRef(new MediaControlRotateButtonElement(document));
    button->createShadowSubtree();
    button->setType("button");
    runtime_info_set_changed_cb(RUNTIME_INFO_KEY_AUTO_ROTATION_ENABLED, autoRotationInfoCallback, static_cast<void*>(button.get()));
    return button.release();
}

void MediaControlRotateButtonElement::defaultEventHandler(Event* event)
{
    if (event->type() == eventNames().clickEvent) {
        Page* page = document()->page();
        if (page && page->mainFrame() && page->mainFrame()->view()) {
            m_isLocked = true;
            FloatRect screenRect = WebCore::screenRect(page->mainFrame()->view());
            page->chrome()->client()->mediaControlsRequestRotate(screenRect.width() > screenRect.height() ? "vertical" : "horizontal");
        }
        event->setDefaultHandled();
    }
    HTMLInputElement::defaultEventHandler(event);
}

const AtomicString& MediaControlRotateButtonElement::shadowPseudoId() const
{
    DEFINE_STATIC_LOCAL(AtomicString, id, ("-webkit-media-controls-rotate-button"));
    return id;
}

void MediaControlRotateButtonElement::makeTransparent()
{
    if (!m_opaque)
        return;

    if (m_isDisplayed) {
        setInlineStyleProperty(CSSPropertyWebkitTransitionProperty, CSSPropertyOpacity);
        double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeInDuration() : 0;
        setInlineStyleProperty(CSSPropertyWebkitTransitionDuration, duration, CSSPrimitiveValue::CSS_S);
        setInlineStyleProperty(CSSPropertyOpacity, 0.0, CSSPrimitiveValue::CSS_NUMBER);

        m_opaque = false;

        startTimer();
    }
}

void MediaControlRotateButtonElement::makeOpaque()
{
    if (m_opaque)
        return;

    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeInDuration() : 0;

    setInlineStyleProperty(CSSPropertyWebkitTransitionProperty, CSSPropertyOpacity);
    setInlineStyleProperty(CSSPropertyWebkitTransitionDuration, duration, CSSPrimitiveValue::CSS_S);
    setInlineStyleProperty(CSSPropertyOpacity, 1.0, CSSPrimitiveValue::CSS_NUMBER);

    m_opaque = true;

    if (m_isDisplayed)
        show();
}

void MediaControlRotateButtonElement::startTimer()
{
    stopTimer();

    // The timer is required to set the property display:'none' on the rotation button,
    // such that captions are correctly displayed at the bottom of the video
    // at the end of the fadeout transition.
    double duration = document()->page() ? document()->page()->theme()->mediaControlsFadeOutDuration() : 0;
    m_transitionTimer.startOneShot(duration);
}

void MediaControlRotateButtonElement::stopTimer()
{
    if (m_transitionTimer.isActive())
        m_transitionTimer.stop();
}


void MediaControlRotateButtonElement::transitionTimerFired(Timer<MediaControlRotateButtonElement>*)
{
    stopTimer();
}

void MediaControlRotateButtonElement::setIsDisplayed(bool isDisplayed)
{
    bool auto_rotate = false;

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    if (isMiniWindowMode() || isMultiWindowMode())
        m_isDisplayed = false;
    else
#endif

    if (!isDisplayed || runtime_info_get_value_bool(RUNTIME_INFO_KEY_AUTO_ROTATION_ENABLED, &auto_rotate) != RUNTIME_INFO_ERROR_NONE)
        m_isDisplayed = false;
#if ENABLE(TIZEN_EXTENSIBLE_API)
    else if (TizenExtensibleAPI::extensibleAPI().rotationLock())
        m_isDisplayed = true;
#endif
    else if (!auto_rotate)
        m_isDisplayed = true;

#if ENABLE(TIZEN_EXTENSIBLE_API)
    if (!TizenExtensibleAPI::extensibleAPI().enableManualVideoRotation())
        m_isDisplayed = false;
#endif

    m_isDisplayed ? show() : hide();
}

void MediaControlRotateButtonElement::releaseRotationLock()
{
    if (document()->page()) {
        m_isLocked = false;
        document()->page()->chrome()->client()->mediaControlsRequestRotate("exit");
    }
}

// static
void MediaControlRotateButtonElement::autoRotationInfoCallback(runtime_info_key_e key, void *user_data)
{
    if (key == RUNTIME_INFO_KEY_AUTO_ROTATION_ENABLED) {
        MediaControlRotateButtonElement* element = static_cast<MediaControlRotateButtonElement*>(user_data);
        bool autoRotateScreen = false;
        runtime_info_get_value_bool(RUNTIME_INFO_KEY_AUTO_ROTATION_ENABLED, &autoRotateScreen);
        if (element && element->isLocked() && autoRotateScreen)
            element->releaseRotationLock();
    }
}

#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
} // namespace WebCore

#endif // ENABLE(VIDEO)
