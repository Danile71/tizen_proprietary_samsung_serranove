/*
 *  Copyright (C) 2011, 2012 Igalia S.L
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#if ENABLE(WEB_AUDIO)

#include "AudioDestinationGStreamer.h"

#include "AudioChannel.h"
#include "AudioSourceProvider.h"
#include <wtf/gobject/GOwnPtr.h>
#include "GRefPtrGStreamer.h"
#include "GStreamerVersioning.h"
#include "Logging.h"
#include "WebKitWebAudioSourceGStreamer.h"
#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>
#if ENABLE(TIZEN_WEB_AUDIO)
#include "RunLoop.h"
#include <wtf/text/CString.h>
#endif

namespace WebCore {

// Size of the AudioBus for playback. The webkitwebaudiosrc element
// needs to handle this number of frames per cycle as well.
const unsigned framesToPull = 128;

gboolean messageCallback(GstBus* bus, GstMessage* message, AudioDestinationGStreamer* destination)
{
    return destination->handleMessage(message);
}

PassOwnPtr<AudioDestination> AudioDestination::create(AudioIOCallback& callback, unsigned numberOfInputChannels, unsigned numberOfOutputChannels, float sampleRate)
{
    // FIXME: Add support for local/live audio input.
    if (numberOfInputChannels)
        LOG(Media, "AudioDestination::create(%u, %u, %f) - unhandled input channels", numberOfInputChannels, numberOfOutputChannels, sampleRate);

    // FIXME: Add support for multi-channel (> stereo) output.
    if (numberOfOutputChannels != 2)
        LOG(Media, "AudioDestination::create(%u, %u, %f) - unhandled output channels", numberOfInputChannels, numberOfOutputChannels, sampleRate);

    return adoptPtr(new AudioDestinationGStreamer(callback, sampleRate));
}

float AudioDestination::hardwareSampleRate()
{
    return 44100;
}

#ifndef GST_API_VERSION_1
static void onGStreamerWavparsePadAddedCallback(GstElement*, GstPad* pad, AudioDestinationGStreamer* destination)
{
    destination->finishBuildingPipelineAfterWavParserPadReady(pad);
}
#endif

#if ENABLE(TIZEN_WEB_AUDIO)
static void eventSourcePlay(ASM_event_sources_t eventSource, void* callbackData)
{
    AudioDestinationGStreamer* pDestination = static_cast<AudioDestinationGStreamer*>(callbackData);
    if (!pDestination || !pDestination->isPausedByASM())
        return;

    switch (eventSource) {
    case ASM_EVENT_SOURCE_ALARM_END:
    case ASM_EVENT_SOURCE_CALL_END:
    case ASM_EVENT_SOURCE_EMERGENCY_END:
    case ASM_EVENT_SOURCE_RESUMABLE_MEDIA:
        if (!pDestination->context()->isNeededPauseRendering()) {
            pDestination->setIsPausedByASM(false);
            pDestination->context()->startRendering();
            TIZEN_LOGI("AudioContext has been played by ASM %d", eventSource);
        }
        break;
    default:
        break;
    }

    return;
}

static void eventSourcePause(ASM_event_sources_t eventSource, void* callbackData)
{
    AudioDestinationGStreamer* pDestination = static_cast<AudioDestinationGStreamer*>(callbackData);
    if (!pDestination)
        return;

    switch (eventSource) {
    case ASM_EVENT_SOURCE_MEDIA:
    case ASM_EVENT_SOURCE_OTHER_PLAYER_APP:
    case ASM_EVENT_SOURCE_RESOURCE_CONFLICT:
        pDestination->context()->stopSchedulingSources();
        TIZEN_LOGI("AudioSourceNodes are stopped by ASM %d", eventSource);
        break;
    case ASM_EVENT_SOURCE_ALARM_START:
    case ASM_EVENT_SOURCE_CALL_START:
    case ASM_EVENT_SOURCE_EMERGENCY_START:
    case ASM_EVENT_SOURCE_RESUMABLE_MEDIA:
        pDestination->setIsPausedByASM(true);
        pDestination->context()->pauseRendering();
        TIZEN_LOGI("AudioContext has been paused by ASM %d", eventSource);
        break;
    default:
        break;
    }

    return;
}

static void handleASMEventAsync(ASM_sound_commands_t command, ASM_event_sources_t eventSource, void* callbackData)
{
    if (command == ASM_COMMAND_STOP || command == ASM_COMMAND_PAUSE)
        return eventSourcePause(eventSource, callbackData);
    else if (command == ASM_COMMAND_PLAY || command == ASM_COMMAND_RESUME)
        return eventSourcePlay(eventSource, callbackData);

    return;
}

static ASM_cb_result_t audioSessionCallback(int, ASM_event_sources_t eventSource, ASM_sound_commands_t command, unsigned int, void* callbackData)
{
    RunLoop::main()->dispatch(WTF::bind(&handleASMEventAsync, command, eventSource, callbackData));

    // Handle ASM event syncronously.
    AudioDestinationGStreamer* pDestination = static_cast<AudioDestinationGStreamer*>(callbackData);
    if (!pDestination)
        return ASM_CB_RES_NONE;

    if (command == ASM_COMMAND_STOP || command == ASM_COMMAND_PAUSE) {
        if (eventSource == ASM_EVENT_SOURCE_NOTIFY_START)
            pDestination->stopInternal();
        return ASM_CB_RES_PAUSE;
    } else if (command == ASM_COMMAND_PLAY || command == ASM_COMMAND_RESUME) {
        if (eventSource == ASM_EVENT_SOURCE_NOTIFY_END)
            pDestination->startInternal();
        return ASM_CB_RES_PLAYING;
    }

    return ASM_CB_RES_NONE;
}
#endif

AudioDestinationGStreamer::AudioDestinationGStreamer(AudioIOCallback& callback, float sampleRate)
    : m_callback(callback)
    , m_renderBus(2, framesToPull, false)
    , m_sampleRate(sampleRate)
    , m_isPlaying(false)
#if ENABLE(TIZEN_WEB_AUDIO)
    , m_wavParserAvailable(false)
    , m_audioSinkAvailable(false)
    , m_timer(this, &AudioDestinationGStreamer::destinationStoptimerFired)
    , m_audioSessionManager(AudioSessionManagerGStreamerTizen::create())
    , m_isPausedByASM(false)
#endif
{
    m_pipeline = gst_pipeline_new("play");
    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_pipeline));
    ASSERT(bus);
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(messageCallback), this);
#if ENABLE(TIZEN_WEB_AUDIO)
    m_webkitAudioSrc = reinterpret_cast<GstElement*>(g_object_new(WEBKIT_TYPE_WEB_AUDIO_SRC,
                                                                            "rate", sampleRate,
                                                                            "bus", &m_renderBus,
                                                                            "provider", &m_callback,
                                                                            "frames", framesToPull,
                                                                            "stop", 0,
                                                                            "start", 0, NULL));
    GstElement* webkitAudioSrc = m_webkitAudioSrc;
#else
    GstElement* webkitAudioSrc = reinterpret_cast<GstElement*>(g_object_new(WEBKIT_TYPE_WEB_AUDIO_SRC,
                                                                            "rate", sampleRate,
                                                                            "bus", &m_renderBus,
                                                                            "provider", &m_callback,
                                                                            "frames", framesToPull, NULL));
#endif

    GstElement* wavParser = gst_element_factory_make("wavparse", 0);

    m_wavParserAvailable = wavParser;
    ASSERT_WITH_MESSAGE(m_wavParserAvailable, "Failed to create GStreamer wavparse element");
    if (!m_wavParserAvailable)
        return;

#ifndef GST_API_VERSION_1
    g_signal_connect(wavParser, "pad-added", G_CALLBACK(onGStreamerWavparsePadAddedCallback), this);
#endif
    gst_bin_add_many(GST_BIN(m_pipeline), webkitAudioSrc, wavParser, NULL);
    gst_element_link_pads_full(webkitAudioSrc, "src", wavParser, "sink", GST_PAD_LINK_CHECK_NOTHING);

#ifdef GST_API_VERSION_1
    GRefPtr<GstPad> srcPad = adoptGRef(gst_element_get_static_pad(wavParser, "src"));
    finishBuildingPipelineAfterWavParserPadReady(srcPad.get());
#endif
#if ENABLE(TIZEN_WEB_AUDIO)
    m_audioSessionManager->registerCallback(audioSessionCallback, this);
#endif
}

AudioDestinationGStreamer::~AudioDestinationGStreamer()
{
    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_pipeline));
    ASSERT(bus);
    g_signal_handlers_disconnect_by_func(bus.get(), reinterpret_cast<gpointer>(messageCallback), this);
    gst_bus_remove_signal_watch(bus.get());

#if ENABLE(TIZEN_WEB_AUDIO)
    stopDestinationStopTimer();
    g_object_set(m_webkitAudioSrc, "stop", true, NULL);
    m_isPlaying = false;
#endif
    gst_element_set_state(m_pipeline, GST_STATE_NULL);
    gst_object_unref(m_pipeline);
}

void AudioDestinationGStreamer::finishBuildingPipelineAfterWavParserPadReady(GstPad* pad)
{
    ASSERT(m_wavParserAvailable);

    GRefPtr<GstElement> audioSink = gst_element_factory_make("autoaudiosink", 0);
    m_audioSinkAvailable = audioSink;

    if (!audioSink) {
        LOG_ERROR("Failed to create GStreamer autoaudiosink element");
        return;
    }

    // Autoaudiosink does the real sink detection in the GST_STATE_NULL->READY transition
    // so it's best to roll it to READY as soon as possible to ensure the underlying platform
    // audiosink was loaded correctly.
    GstStateChangeReturn stateChangeReturn = gst_element_set_state(audioSink.get(), GST_STATE_READY);
    if (stateChangeReturn == GST_STATE_CHANGE_FAILURE) {
        LOG_ERROR("Failed to change autoaudiosink element state");
        gst_element_set_state(audioSink.get(), GST_STATE_NULL);
        m_audioSinkAvailable = false;
        return;
    }
    GstElement* audioConvert = gst_element_factory_make("audioconvert", 0);
    gst_bin_add_many(GST_BIN(m_pipeline), audioConvert, audioSink.get(), NULL);

    // Link wavparse's src pad to audioconvert sink pad.
    GRefPtr<GstPad> sinkPad = adoptGRef(gst_element_get_static_pad(audioConvert, "sink"));
    gst_pad_link_full(pad, sinkPad.get(), GST_PAD_LINK_CHECK_NOTHING);

    // Link audioconvert to audiosink and roll states.
    gst_element_link_pads_full(audioConvert, "src", audioSink.get(), "sink", GST_PAD_LINK_CHECK_NOTHING);
    gst_element_sync_state_with_parent(audioConvert);
    gst_element_sync_state_with_parent(audioSink.leakRef());
}

gboolean AudioDestinationGStreamer::handleMessage(GstMessage* message)
{
    GOwnPtr<GError> error;
    GOwnPtr<gchar> debug;
#if ENABLE(TIZEN_WEB_AUDIO)
    CString dotFileName;
#endif

    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_WARNING:
        gst_message_parse_warning(message, &error.outPtr(), &debug.outPtr());
        g_warning("Warning: %d, %s. Debug output: %s", error->code,  error->message, debug.get());
        break;
    case GST_MESSAGE_ERROR:
        gst_message_parse_error(message, &error.outPtr(), &debug.outPtr());
        g_warning("Error: %d, %s. Debug output: %s", error->code,  error->message, debug.get());

        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        m_isPlaying = false;
#if ENABLE(TIZEN_WEB_AUDIO)
        audioSessionManager()->setSoundState(ASM_STATE_STOP, ASM_RESOURCE_NONE);
#endif
        break;
#if ENABLE(TIZEN_WEB_AUDIO)
    case GST_MESSAGE_STATE_CHANGED:
        GstState oldState, newState;
        gst_message_parse_state_changed(message, &oldState, &newState, 0);

        // Construct a filename for the graphviz dot file output.
        dotFileName = String::format("webkit-webaudio.%s_%s",
                                             gst_element_state_get_name(oldState),
                                             gst_element_state_get_name(newState)).utf8();

        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(m_pipeline), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.data());
        break;
#endif
    default:
        break;
    }
    return TRUE;
}

void AudioDestinationGStreamer::startInternal()
{
    audioSessionManager()->setSoundState(ASM_STATE_PLAYING, ASM_RESOURCE_NONE);
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    g_object_set(m_webkitAudioSrc, "start", true, NULL);
    m_isPlaying = true;
}

void AudioDestinationGStreamer::start()
{
    ASSERT(m_wavParserAvailable);
    if (!m_wavParserAvailable)
        return;
#if ENABLE(TIZEN_WEB_AUDIO)
    stopDestinationStopTimer();

    if (m_isPlaying)
        return;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    // Check if media element handling ASM.
    if (!audioSessionManager()->handlingASM())
#endif
        startInternal();
#else
    gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    g_object_set(m_webkitAudioSrc, "start", true, NULL);
    m_isPlaying = true;
#endif
}

void AudioDestinationGStreamer::stopInternal()
{
    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    g_object_set(m_webkitAudioSrc, "stop", true, NULL);
    m_isPlaying = false;
    audioSessionManager()->setSoundState(ASM_STATE_PAUSE, ASM_RESOURCE_NONE);
}

void AudioDestinationGStreamer::stop()
{
#if ENABLE(TIZEN_WEB_AUDIO)
    if (!m_wavParserAvailable)
        return;
    if (!m_isPlaying)
        return;
    startDestinationStopTimer(1.0);
#else
    ASSERT(m_wavParserAvailable && m_audioSinkAvailable);

    if (!m_wavParserAvailable || m_audioSinkAvailable)
        return;

    gst_element_set_state(m_pipeline, GST_STATE_PAUSED);
    m_isPlaying = false;
#endif
}

#if ENABLE(TIZEN_WEB_AUDIO)
void AudioDestinationGStreamer::startDestinationStopTimer(double expiredTime)
{
    if (!m_timer.isActive())
        m_timer.startOneShot(expiredTime);
}

void AudioDestinationGStreamer::stopDestinationStopTimer()
{
    if (m_timer.isActive()) {
        m_timer.stop();

        if (context())
            context()->setWatingPauseDispatchComplete(false);
    }
}

void AudioDestinationGStreamer::destinationStoptimerFired(Timer<AudioDestinationGStreamer>*)
{
    stopInternal();
    if (context())
        context()->setWatingPauseDispatchComplete(false);
}

#endif
} // namespace WebCore

#endif // ENABLE(WEB_AUDIO)
