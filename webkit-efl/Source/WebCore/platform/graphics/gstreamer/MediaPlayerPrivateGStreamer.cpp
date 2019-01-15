/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Gustavo Noronha Silva <gns@gnome.org>
 * Copyright (C) 2009, 2010 Igalia S.L
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * aint with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "MediaPlayerPrivateGStreamer.h"

#if ENABLE(VIDEO) && USE(GSTREAMER)

#include "ColorSpace.h"
#include "Document.h"
#include "Frame.h"
#include "FrameView.h"
#include "GStreamerGWorld.h"
#include "GStreamerUtilities.h"
#include "GStreamerVersioning.h"
#include "GraphicsContext.h"
#include "GraphicsTypes.h"
#include "ImageGStreamer.h"
#include "ImageOrientation.h"
#include "IntRect.h"
#include "KURL.h"
#include "MIMETypeRegistry.h"
#include "MediaPlayer.h"
#include "NotImplemented.h"
#include "SecurityOrigin.h"
#include "TimeRanges.h"
#include "VideoSinkGStreamer.h"
#include "WebKitWebSourceGStreamer.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <limits>
#include <math.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/CString.h>

#include <gst/audio/gstaudiobasesink.h>
#ifdef GST_API_VERSION_1
#include <gst/audio/streamvolume.h>
#else
#include <gst/interfaces/streamvolume.h>
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#include "Cookie.h"
#include "CookieJarSoup.h"
#include "GOwnPtrSoup.h"
#include "FileSystem.h"
#include "MediaResourceControllerGStreamerTizen.h"
#include "RunLoop.h"
#include <wtf/CurrentTime.h>
#endif

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)
#include "Page.h"
#include "Settings.h"
#include "VideoLayerTizen.h"
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "URIUtils.h"
#include "WebKitCameraSourceGStreamerTizen.h"
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
#include "EflScreenUtilities.h"
#ifdef GST_API_VERSION_1
#include <gst/video/videooverlay.h>
#else
#include <gst/interfaces/xoverlay.h>
#endif
#endif

#if ENABLE(TIZEN_EXTENSIBLE_API)
#include "TizenExtensibleAPI.h"
#endif

#define MEGA_TO_BYTE(size) (size * 1024 * 1024)
#define KILO_TO_BYTE(size) (size * 1024)

// GstPlayFlags flags from playbin2. It is the policy of GStreamer to
// not publicly expose element-specific enums. That's why this
// GstPlayFlags enum has been copied here.
typedef enum {
    GST_PLAY_FLAG_VIDEO         = 0x00000001,
    GST_PLAY_FLAG_AUDIO         = 0x00000002,
    GST_PLAY_FLAG_TEXT          = 0x00000004,
    GST_PLAY_FLAG_VIS           = 0x00000008,
    GST_PLAY_FLAG_SOFT_VOLUME   = 0x00000010,
    GST_PLAY_FLAG_NATIVE_AUDIO  = 0x00000020,
    GST_PLAY_FLAG_NATIVE_VIDEO  = 0x00000040,
    GST_PLAY_FLAG_DOWNLOAD      = 0x00000080,
    GST_PLAY_FLAG_BUFFERING     = 0x00000100
} GstPlayFlags;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
typedef enum {
    DEGREE_0, // No rotate
    DEGREE_90, // Rotate 90 degree count clockwise
    DEGREE_180, // Rotate 180 degree count clockwise
    DEGREE_270 // Rotate 270 degree count clockwise
} RotateAngle;

enum {
    VIDEO_BUFFER_SIZE = 0,
    VIDEO_HIGH_PERCENT,
    VIDEO_LOW_PERCENT,

    VIDEO_BUFFER_SIZE_HD,
    VIDEO_HIGH_PERCENT_HD,
    VIDEO_LOW_PERCENT_HD,

    AUDIO_BUFFER_SIZE,
    AUDIO_HIGH_PERCENT,
    AUDIO_LOW_PERCENT
};

static gint DOWNLOAD_BUFFER_INFO[] =
{
    // VIDEO
    MEGA_TO_BYTE(15),
    99,  // High Percent
    10,  // Low Percent

    // VIDEO_HD
    MEGA_TO_BYTE(30),
    99,  // High Percent
    10,  // Low Percent

    // AUDIO
    MEGA_TO_BYTE(2),
    99, // High Percent
    10  // Low Percent
};

#define TEMP_FILE_LOCATION "/opt/usr/media"
#define TEMP_FILE_TEMPLATE "/opt/usr/media/XXXXXX" // should contain 'XXXXXX'.

// 'decodebin2' reset size property of 'multiqueue',
// when 'multiqueue' emit 'overrun' signal.
// And 'decodebin2' doesn't respect 'max-size-byte' with value '0'.
// Because of that we need to set 'max-size-byte' even it is useless for us.
// For 'max-size-time' only to be consider in buffering percent and actual buffer size,
// set 'max-size-byte' as large value compare to 'max-size-time'.
// Then actually it will be ignored.

#if ENABLE(TIZEN_LITE)
#define MULTI_QUEUE_MAX_SIZE_BYTE (10 * 1024 * 1024)
#else
#define MULTI_QUEUE_MAX_SIZE_BYTE (20 * 1024 * 1024)
#endif

#define MULTI_QUEUE_LOW_PERCENT (10)
#define MULTI_QUEUE_HIGH_PERCENT (30)
#define MULTI_QUEUE_HD_HIGH_PERCENT (50)
#define MULTI_QUEUE_MAX_SIZE_TIME (2 * GST_SECOND)

#define NECESSARY_SIZE_FOR_DOWNLOAD_MODE (30 * 1024 * 1024)

#define BLOCK_SIZE (64 * 1024) // 64KB
static const int  gHdVideoHeight = 720;
#endif

#ifdef GST_API_VERSION_1
static const char* gPlaybinName = "playbin";
static const char* gDecodebinName = "decodebin";
#else
static const char* gPlaybinName = "playbin2";
static const char* gDecodebinName = "decodebin2";
#endif

GST_DEBUG_CATEGORY_STATIC(webkit_media_player_debug);
#define GST_CAT_DEFAULT webkit_media_player_debug

using namespace std;

namespace WebCore {

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
static Ecore_X_Window xWindow = 0;
#endif
Vector<MediaPlayerPrivateInterface*> MediaPlayerPrivateGStreamer::m_playerList;
#endif

static int greatestCommonDivisor(int a, int b)
{
    while (b) {
        int temp = a;
        a = b;
        b = temp % b;
    }

    return ABS(a);
}

static gboolean mediaPlayerPrivateMessageCallback(GstBus*, GstMessage* message, MediaPlayerPrivateGStreamer* player)
{
    return player->handleMessage(message);
}

static void mediaPlayerPrivateSourceChangedCallback(GObject*, GParamSpec*, MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return;
#endif
    player->sourceChanged();
}

static void mediaPlayerPrivateVolumeChangedCallback(GObject*, GParamSpec*, MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return;
#endif

    // This is called when playbin receives the notify::volume signal.
    player->volumeChanged();
}

static gboolean mediaPlayerPrivateVolumeChangeTimeoutCallback(MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return FALSE;
#endif

    // This is the callback of the timeout source created in ::volumeChanged.
    player->notifyPlayerOfVolumeChange();
    return FALSE;
}

static void mediaPlayerPrivateMuteChangedCallback(GObject*, GParamSpec*, MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return;
#endif

    // This is called when playbin receives the notify::mute signal.
    player->muteChanged();
}

static gboolean mediaPlayerPrivateMuteChangeTimeoutCallback(MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return FALSE;
#endif

    // This is the callback of the timeout source created in ::muteChanged.
    player->notifyPlayerOfMute();
    return FALSE;
}

static void mediaPlayerPrivateVideoSinkCapsChangedCallback(GObject*, GParamSpec*, MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return;
#endif
    player->videoChanged();
}

static void mediaPlayerPrivateVideoChangedCallback(GObject*, MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return;
#endif
    player->videoChanged();
}

static void mediaPlayerPrivateAudioChangedCallback(GObject*, MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return;
#endif
    player->audioChanged();
}

static gboolean mediaPlayerPrivateAudioChangeTimeoutCallback(MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return FALSE;
#endif

    // This is the callback of the timeout source created in ::audioChanged.
    player->notifyPlayerOfAudio();
    return FALSE;
}

static gboolean mediaPlayerPrivateVideoChangeTimeoutCallback(MediaPlayerPrivateGStreamer* player)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (player->isDestroyed(player))
        return FALSE;
#endif

    // This is the callback of the timeout source created in ::videoChanged.
    player->notifyPlayerOfVideo();
    return FALSE;
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN) && defined(GST_API_VERSION_1)
static int getXWindowIDCallback(void* userData, guint videoWidth, guint videoHight) {
    return xWindow;
}
#endif
static void mediaPlayerPrivateSourceSetupCallback(GObject*, GstElement* source, MediaPlayerPrivateGStreamer* player)
{
    if (player->isDestroyed(player))
        return;

    // We need below logic only for 'souphttpsrc'.
    if (!player->url().protocol().startsWith("http", false))
        return;

    player->setCookie(source);
#if ENABLE(TIZEN_WEBKIT2_PROXY)
    player->setProxy(source);
#endif
    g_object_set(source, "timeout", 20, NULL);

    Document* document = player->element()->document();
    if (!document)
        document =  player->element()->ownerDocument();

    Frame* frame = document ? document->frame() : 0;
    FrameLoader* frameLoader = frame ? frame->loader() : 0;
    if (frameLoader)
        g_object_set(source, "user-agent", frameLoader->userAgent(player->url()).utf8().data(), NULL);

    g_object_set(source, "blocksize", BLOCK_SIZE, NULL);
}

static void restoreToReleasedTime(MediaPlayerPrivateGStreamer* player)
{
    if (player->isDestroyed(player))
        return;

    LOG_MEDIA_MESSAGE("restoreToReleasedTime : %f", player->releasedTime());
#if ENABLE(TIZEN_MEDIA_STREAM)
    if (player->isLocalMediaStream())
        player->play();
    else
#endif
        player->seek(player->releasedTime());

    player->setReleasedTime(numeric_limits<float>::infinity());
}

static void mediaPlayerPrivateHaveTypeCallback(GstElement* typefind, guint probability, GstCaps* caps, MediaPlayerPrivateGStreamer* player)
{
    if (player->isDestroyed(player) || !player->element())
        return;

    // For the case that url of hls stream doesn't contain '.m3u8' file extension.
    if (player->downloadBufferingEnabled() && String(gst_caps_to_string(caps)).contains("hls", false)) {
        GstElement* pipeline = player->playBin();
        if (pipeline) {
            GstPlayFlags flags;
            g_object_get(pipeline, "flags", &flags, NULL);
            g_object_set(pipeline, "flags", flags & ~GST_PLAY_FLAG_DOWNLOAD, NULL);
        }

        GstElement* queue2 = player->queue2();
        if (queue2)
            g_object_set(queue2, "temp-template", NULL, NULL);

        GstElement* downloadBuffer = player->downloadBuffer();
        if (downloadBuffer)
            g_object_set(downloadBuffer , "temp-template", NULL, NULL);
    }

    g_signal_handlers_disconnect_by_func(typefind, reinterpret_cast<gpointer>(mediaPlayerPrivateHaveTypeCallback), player);
}

static void mediaPlayerPrivateElementAddedCallback(GstBin* bin, GstElement* element, MediaPlayerPrivateGStreamer* player)
{
    if (player->isDestroyed(player) || !player->element())
        return;

    // The signal callback is called in gstreamer thread.
    if (String(GST_ELEMENT_NAME (element)).startsWith("downloadbuffer", false)) {
        player->setDownloadBuffer(element);
        player->initializeDownloadBuffer();
        g_signal_handlers_disconnect_by_func(GST_ELEMENT_PARENT(element), reinterpret_cast<gpointer>(mediaPlayerPrivateElementAddedCallback), player);
    } else if (String(GST_ELEMENT_NAME (element)).startsWith(gDecodebinName, false)) {
        player->setDecodebin(element);
        player->initializeDecodebin();
        g_signal_connect(GST_BIN(element), "element-added", G_CALLBACK(mediaPlayerPrivateElementAddedCallback), player);
    } else if (String(GST_ELEMENT_NAME (element)).startsWith("multiqueue", false)) {
        player->setMultiQueue(element);
        player->initializeMultiQueue();
        g_signal_handlers_disconnect_by_func(GST_ELEMENT_PARENT(element), reinterpret_cast<gpointer>(mediaPlayerPrivateElementAddedCallback), player);
    } else if (String(GST_ELEMENT_NAME (element)).startsWith("typefindelement", false)) {
        g_signal_connect(GST_BIN(element), "have-type", G_CALLBACK(mediaPlayerPrivateHaveTypeCallback), player);
    } else if (String(GST_ELEMENT_NAME (element)).startsWith("queue2", false)) {
        // For maintenance and readability compute |queue2| directly without check GST_PLAY_FLAG_DOWNLOAD.
        player->setQueue2(element);
        player->initializeQueue2();
        g_signal_handlers_disconnect_by_func(GST_ELEMENT_PARENT(element), reinterpret_cast<gpointer>(mediaPlayerPrivateElementAddedCallback), player);
    }
}

static void mediaPlayerPrivateElementRemovedCallback(GstBin* bin, GstElement* element, MediaPlayerPrivateGStreamer* player)
{
    if (player->isDestroyed(player) || !player->element())
        return;

    // The signal callack is called in gstreamer thread.
    if (String(GST_ELEMENT_NAME (element)).startsWith("downloadbuffer", false)) {
        player->setDownloadBuffer(0);
        player->setDownloadBufferConfigFinished(false);
        g_signal_handlers_disconnect_by_func(GST_ELEMENT_PARENT(element), reinterpret_cast<gpointer>(mediaPlayerPrivateElementRemovedCallback), player);
    } else if (String(GST_ELEMENT_NAME (element)).startsWith(gDecodebinName, false)) {
        player->setDecodebin(0);
        player->setMultiQueue(0);
        player->setMultiQueueConfigFinished(false);
    } else if (String(GST_ELEMENT_NAME (element)).startsWith("queue2", false)) {
        // For maintenance and readability compute |queue2| directly without check GST_PLAY_FLAG_DOWNLOAD.
        player->setQueue2(0);
        player->setQueue2ConfigFinished(false);
        g_signal_handlers_disconnect_by_func(GST_ELEMENT_PARENT(element), reinterpret_cast<gpointer>(mediaPlayerPrivateElementRemovedCallback), player);
    }
}

static GstBusSyncReply mediaPlayerPrivateSyncHandler(GstBus* bus, GstMessage* message, MediaPlayerPrivateGStreamer* player)
{
    if (player->isDestroyed(player))
        return GST_BUS_PASS;

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#ifdef GST_API_VERSION_1
    const GstStructure* structure = gst_message_get_structure(message);
#else
    const GstStructure* structure = message->structure;
#endif

    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ELEMENT
        && gst_structure_has_name(structure, "prepare-xid")) {
        player->xWindowIdPrepared(message);
        gst_message_unref(message);
        return GST_BUS_DROP;
    }
#endif

    if (player->isReleased()
        && GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED
        && GST_MESSAGE_SRC(message) == reinterpret_cast<GstObject*>(player->playBin())) {
            GstState oldState, newState;
            gst_message_parse_state_changed(message, &oldState, &newState, 0);
            if (oldState == GST_STATE_READY && newState == GST_STATE_PAUSED)
                RunLoop::main()->dispatch(WTF::bind(&restoreToReleasedTime, player));
            else if (oldState == GST_STATE_NULL && newState == GST_STATE_READY)
                player->setDrawable();
    }

    if (String(GST_MESSAGE_SRC_NAME(message)).startsWith("uridecodebin", false)
        && GST_MESSAGE_TYPE(message) == GST_MESSAGE_STATE_CHANGED) {
        GstState oldState, newState;
        gst_message_parse_state_changed(message, &oldState, &newState, 0);
        if (oldState == GST_STATE_NULL && newState == GST_STATE_READY) {
            g_signal_connect(GST_BIN(GST_MESSAGE_SRC(message)), "element-added", G_CALLBACK(mediaPlayerPrivateElementAddedCallback), player);
            g_signal_connect(GST_BIN(GST_MESSAGE_SRC(message)), "element-removed", G_CALLBACK(mediaPlayerPrivateElementRemovedCallback), player);
        } else if (oldState == GST_STATE_PAUSED && newState == GST_STATE_READY) {
            g_signal_handlers_disconnect_by_func(GST_BIN(GST_MESSAGE_SRC(message)), reinterpret_cast<gpointer>(mediaPlayerPrivateElementAddedCallback), player);
            g_signal_handlers_disconnect_by_func(GST_BIN(GST_MESSAGE_SRC(message)), reinterpret_cast<gpointer>(mediaPlayerPrivateElementRemovedCallback), player);
        }
    }

    return GST_BUS_PASS;
}
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivateGStreamer::create(MediaPlayer* player)
{
    return adoptPtr(new MediaPlayerPrivateGStreamer(player));
}

void MediaPlayerPrivateGStreamer::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar(create, getSupportedTypes, supportsType, 0, 0, 0);
}

bool initializeGStreamerAndRegisterWebKitElements()
{
    if (!initializeGStreamer())
        return false;

#if ENABLE(TIZEN_MEDIA_STREAM)
    GRefPtr<GstElementFactory> srcFactory = gst_element_factory_find("webkitcamerasrc");
    if (!srcFactory)
        return gst_element_register(0, "webkitcamerasrc", GST_RANK_PRIMARY + 200, WEBKIT_TYPE_CAMERA_SRC);
#endif

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    GRefPtr<GstElementFactory> srcFactory = gst_element_factory_find("webkitwebsrc");
    if (!srcFactory) {
        GST_DEBUG_CATEGORY_INIT(webkit_media_player_debug, "webkitmediaplayer", 0, "WebKit media player");
        return gst_element_register(0, "webkitwebsrc", GST_RANK_PRIMARY + 100, WEBKIT_TYPE_WEB_SRC);
    }
#endif

    return true;
}

bool MediaPlayerPrivateGStreamer::isAvailable()
{
    if (!initializeGStreamerAndRegisterWebKitElements())
        return false;

    GRefPtr<GstElementFactory> factory = gst_element_factory_find(gPlaybinName);
    return factory;
}

MediaPlayerPrivateGStreamer::MediaPlayerPrivateGStreamer(MediaPlayer* player)
    : m_player(player)
    , m_webkitVideoSink(0)
    , m_videoSinkBin(0)
    , m_fpsSink(0)
    , m_source(0)
    , m_seekTime(0)
    , m_changingRate(false)
    , m_endTime(numeric_limits<float>::infinity())
    , m_isEndReached(false)
    , m_networkState(MediaPlayer::Empty)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_isStreaming(false)
    , m_size(IntSize())
    , m_buffer(0)
    , m_mediaLocations(0)
    , m_mediaLocationCurrentIndex(0)
    , m_resetPipeline(false)
    , m_paused(true)
    , m_seeking(false)
    , m_buffering(false)
#if ENABLE(TIZEN_MEDIA_STREAM)
    , m_rotation(DEGREE_0)
    , m_cameraId(0)
#endif
    , m_playbackRate(1)
    , m_errorOccured(false)
    , m_mediaDuration(0)
    , m_startedBuffering(false)
    , m_fillTimer(this, &MediaPlayerPrivateGStreamer::fillTimerFired)
    , m_maxTimeLoaded(0)
    , m_bufferingPercentage(0)
    , m_preload(player->preload())
    , m_delayingLoad(false)
    , m_maxTimeLoadedAtLastDidLoadingProgress(0)
    , m_volumeTimerHandler(0)
    , m_muteTimerHandler(0)
    , m_hasVideo(false)
    , m_hasAudio(false)
    , m_audioTimerHandler(0)
    , m_videoTimerHandler(0)
    , m_webkitAudioSink(0)
    , m_totalBytes(-1)
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)
    , m_videoLayer(VideoLayerTizen::create(element()))
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    , m_mediaDurationKnown(false)
    , m_preservesPitch(player->preservesPitch())
    , m_fallbackCurrentTime(0)
    , m_releasedTime(numeric_limits<float>::infinity())
    , m_shouldDelayErrorEvent(false)
    , m_pendingSeek(false)
    , m_pendingSeekTime(0.0f)
    , m_havePlayed(false)
    , m_queue2ConfigFinished(false)
    , m_downloadBufferConfigFinished(false)
    , m_multiQueueConfigFinished(false)
    , m_queue2(0)
    , m_downloadBuffer(0)
    , m_multiQueue(0)
    , m_decodebin(0)
#ifdef GST_API_VERSION_1
    , m_queue2_probe_id(0)
    , m_downloadBuffer_probe_id(0)
    , m_decodebin_probe_id(0)
#endif
#else
    , m_mediaDurationKnown(true)
    , m_preservesPitch(false)
#endif
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    , m_overlayType(MediaPlayer::Pixmap)
#endif
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    , m_updatePixmapSize(true)
#endif
{
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (element()->isFullscreen())
        m_overlayType = MediaPlayer::HwOverlay;
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_playerList.append(this);
#endif
}

MediaPlayerPrivateGStreamer::~MediaPlayerPrivateGStreamer()
{
    if (m_fillTimer.isActive())
        m_fillTimer.stop();

    if (m_buffer)
        gst_buffer_unref(m_buffer);
    m_buffer = 0;

    if (m_mediaLocations) {
        gst_structure_free(m_mediaLocations);
        m_mediaLocations = 0;
    }

#ifndef GST_API_VERSION_1
    if (m_videoSinkBin) {
        gst_object_unref(m_videoSinkBin);
        m_videoSinkBin = 0;
    }
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (fileExists(m_tagImagePath))
        deleteFile(m_tagImagePath);

    size_t index = m_playerList.find(this);
    if (index != notFound)
        m_playerList.remove(index, 1);

    if (m_decodebin)
        g_signal_handlers_disconnect_by_func(m_decodebin, reinterpret_cast<gpointer>(mediaPlayerPrivateElementAddedCallback), this);

    setQueue2Blocked(false);
    setDownloadBufferBlocked(false);
    setDecodebinBlocked(false);
#endif

    if (m_webkitVideoSink) {
        GRefPtr<GstPad> videoSinkPad = adoptGRef(gst_element_get_static_pad(m_webkitVideoSink, "sink"));
        g_signal_handlers_disconnect_by_func(videoSinkPad.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateVideoSinkCapsChangedCallback), this);
    }
    if (m_playBin) {
        GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_playBin.get()));
        ASSERT(bus);
        gst_bus_set_sync_handler(bus.get(), NULL, NULL, NULL);
        g_signal_handlers_disconnect_by_func(bus.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateMessageCallback), this);
        gst_bus_remove_signal_watch(bus.get());

        gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateSourceChangedCallback), this);
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateVideoChangedCallback), this);
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateAudioChangedCallback), this);
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateVolumeChangedCallback), this);
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateMuteChangedCallback), this);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        g_signal_handlers_disconnect_by_func(m_playBin.get(), reinterpret_cast<gpointer>(mediaPlayerPrivateSourceSetupCallback), this);
#endif
        m_playBin = 0;
    }

    m_player = 0;

    if (m_muteTimerHandler)
        g_source_remove(m_muteTimerHandler);

    if (m_volumeTimerHandler)
        g_source_remove(m_volumeTimerHandler);

    if (m_videoTimerHandler)
        g_source_remove(m_videoTimerHandler);

    if (m_audioTimerHandler)
        g_source_remove(m_audioTimerHandler);

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (m_videoLayer)
        m_videoLayer->clearSurface();
#endif
}

void MediaPlayerPrivateGStreamer::load(const String& url)
{
    if (!initializeGStreamerAndRegisterWebKitElements())
        return;

#if ENABLE(TIZEN_MEDIA_STREAM)
    crackURI(url);
#endif
    KURL kurl(KURL(), url);
    String cleanUrl(url);

    // Clean out everything after file:// url path.
    if (kurl.isLocalFile())
        cleanUrl = cleanUrl.substring(0, kurl.pathEnd());

    if (!m_playBin) {
        createGSTPlayBin();
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        if (url.startsWith("http", false) && !url.contains(".m3u8", false))
#endif
        setDownloadBuffering();
    }

    ASSERT(m_playBin);

    m_url = KURL(KURL(), cleanUrl);
    g_object_set(m_playBin.get(), "uri", cleanUrl.utf8().data(), NULL);

#if ENABLE(TIZEN_MEDIA_STREAM)
    if (isLocalMediaStream()) {
        m_isStreaming = true;
        sscanf(cleanUrl.utf8().data(), "camera://%d", &m_cameraId);
#if CPU(X86) || CPU(X86_64)
        m_videoSize = IntSize(640, 480);
#endif
    }
#endif

    LOG_MEDIA_MESSAGE("Load %s", cleanUrl.utf8().data());
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_volumeAndMuteInitialized = false;

    if (m_preload != MediaPlayer::None)
        changePipelineState(GST_STATE_PAUSED);

#if ENABLE(TIZEN_MEDIA_STREAM)
    if (isLocalMediaStream())
        changePipelineState(GST_STATE_PLAYING);
#endif

#else
    if (m_preload == MediaPlayer::None) {
        LOG_MEDIA_MESSAGE("Delaying load.");
        m_delayingLoad = true;
    }

    // Reset network and ready states. Those will be set properly once
    // the pipeline pre-rolled.
    m_networkState = MediaPlayer::Loading;
    m_player->networkStateChanged();
    m_readyState = MediaPlayer::HaveNothing;
    m_player->readyStateChanged();
    m_volumeAndMuteInitialized = false;

    // GStreamer needs to have the pipeline set to a paused state to
    // start providing anything useful.
    gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);

    if (!m_delayingLoad)
        commitLoad();
#endif
}

void MediaPlayerPrivateGStreamer::commitLoad()
{
    ASSERT(!m_delayingLoad);
    LOG_MEDIA_MESSAGE("Committing load.");
    updateStates();
}

float MediaPlayerPrivateGStreamer::playbackPosition() const
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    float ret = numeric_limits<float>::infinity();
#else
    if (m_isEndReached) {
        // Position queries on a null pipeline return 0. If we're at
        // the end of the stream the pipeline is null but we want to
        // report either the seek time or the duration because this is
        // what the Media element spec expects us to do.
        if (m_seeking)
            return m_seekTime;
        if (m_mediaDuration)
            return m_mediaDuration;
    }
    float ret = 0.0f;
#endif

    GstQuery* query = gst_query_new_position(GST_FORMAT_TIME);
    if (!gst_element_query(m_playBin.get(), query)) {
        LOG_MEDIA_MESSAGE("Position query failed...");
        gst_query_unref(query);
        return ret;
    }

    gint64 position;
    gst_query_parse_position(query, 0, &position);

    // Position is available only if the pipeline is not in GST_STATE_NULL or
    // GST_STATE_READY state.
    if (position != static_cast<gint64>(GST_CLOCK_TIME_NONE)) {
        ret = static_cast<double>(position) / GST_SECOND;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        m_fallbackCurrentTime = ret;
#endif
    }

    LOG_MEDIA_MESSAGE("Position %" GST_TIME_FORMAT, GST_TIME_ARGS(position));

    gst_query_unref(query);

    return ret;
}

bool MediaPlayerPrivateGStreamer::changePipelineState(GstState newState)
{
    ASSERT(newState == GST_STATE_PLAYING || newState == GST_STATE_PAUSED);

    GstState currentState;
    GstState pending;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (!m_webkitAudioSink)
        createAudioSink();
#endif
    gst_element_get_state(m_playBin.get(), &currentState, &pending, 0);
    LOG_MEDIA_MESSAGE("Current state: %s, pending: %s", gst_element_state_get_name(currentState), gst_element_state_get_name(pending));
    if (currentState == newState || pending == newState)
        return true;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    TIZEN_LOGI("Element : %p, Current state: %s, New state: %s", element(), gst_element_state_get_name(currentState), gst_element_state_get_name(newState));

    // Resolves Audio not playing issues when number of audio resources are more than 30.
    if (currentState == GST_STATE_NULL && isReleased())
        m_readyState = MediaPlayer::HaveMetadata;

    if (newState == GST_STATE_PLAYING) {
        willStartPlay();
        if (element()->isVideo())
            g_object_set(m_webkitAudioSink.get(), "provide-clock", 0, NULL);
    }

    if (newState == GST_STATE_PLAYING)
        MediaResourceControllerGStreamerTizen::mediaResourceController().updateAudioSessionState(element(), ASM_STATE_PLAYING);
#endif
    GstStateChangeReturn setStateResult = gst_element_set_state(m_playBin.get(), newState);

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (currentState > newState)
        MediaResourceControllerGStreamerTizen::mediaResourceController().updateAudioSessionState(element(), element()->isVideo() ? ASM_STATE_PLAYING : ASM_STATE_PAUSE);
#endif
    GstState pausedOrPlaying = newState == GST_STATE_PLAYING ? GST_STATE_PAUSED : GST_STATE_PLAYING;
    if (currentState != pausedOrPlaying && setStateResult == GST_STATE_CHANGE_FAILURE) {
        loadingFailed(MediaPlayer::Empty);
        return false;
    }
    return true;
}

void MediaPlayerPrivateGStreamer::prepareToPlay()
{
    m_isEndReached = false;
    m_seeking = false;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_errorOccured = false;
#endif
    if (m_delayingLoad) {
        m_delayingLoad = false;
        commitLoad();
    }
}

void MediaPlayerPrivateGStreamer::play()
{
    TIZEN_LOGI("MediaPlayerPrivateGStreamer::play");
    if (changePipelineState(GST_STATE_PLAYING)) {
        m_isEndReached = false;
        LOG_MEDIA_MESSAGE("Play");
    }
}

void MediaPlayerPrivateGStreamer::pause()
{
#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (m_isEndReached)
        return;
#endif
    TIZEN_LOGI("MediaPlayerPrivateGStreamer::pause");
    if (changePipelineState(GST_STATE_PAUSED))
        LOG_MEDIA_MESSAGE("Pause");
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaPlayerPrivateGStreamer::release()
{
    LOG_MEDIA_MESSAGE("release");
    TIZEN_LOGI("MediaPlayerPrivateGStreamer::release");
    if (!m_playBin)
        return;
    // Save when mediaplayer is released.
    m_fallbackCurrentTime = m_releasedTime = currentTime();

    setQueue2Blocked(false);
    setDownloadBufferBlocked(false);
    setDecodebinBlocked(false);

    GstState state;
    gst_element_get_state(m_playBin.get(), &state, 0, 0);
    if (state > GST_STATE_NULL) {
        m_buffering = false;
        gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
        gst_element_get_state(m_playBin.get(), &state, 0, 1 * GST_SECOND);
        MediaResourceControllerGStreamerTizen::mediaResourceController().updateAudioSessionState(element(), ASM_STATE_STOP);
        updateStates();
        g_object_set(m_playBin.get(), "audio-sink", 0, NULL);
#ifndef GST_API_VERSION_1
        // When audio-sink of playbin is unrefered for release filedescriptor which is using by audiosink,
        // audiosink of playsink should be set 0 to unref. Playbin2 doesn't unref audio-sink when it replacsed by get_property.
        // It seems to be buggy in playbin2.
        GstIterator* iterator = gst_bin_iterate_sinks(GST_BIN(m_playBin.get()));
        if (iterator) {
            GstElement* playsink = 0;
            if (gst_iterator_next(iterator, (gpointer*)(&playsink)) == GST_ITERATOR_OK) {
                if (String(GST_ELEMENT_NAME(playsink)).startsWith("playsink", false))
                    g_object_set(playsink, "audio-sink", 0, NULL);
            }
            if (playsink)
                gst_object_unref(playsink);
            gst_iterator_free(iterator);
        }
#endif
        m_webkitAudioSink = 0;
    }
}
#endif

float MediaPlayerPrivateGStreamer::duration() const
{
    if (!m_playBin)
        return 0.0f;

    if (m_errorOccured)
        return 0.0f;

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    // Media duration query failed already, don't attempt new useless queries.
    if (!m_mediaDurationKnown)
        return numeric_limits<float>::infinity();
#endif

    if (m_mediaDuration)
        return m_mediaDuration;

    GstFormat timeFormat = GST_FORMAT_TIME;
    gint64 timeLength = 0;

#ifdef GST_API_VERSION_1
    bool failure = !gst_element_query_duration(m_playBin.get(), timeFormat, &timeLength) || static_cast<guint64>(timeLength) == GST_CLOCK_TIME_NONE;
#else
    bool failure = !gst_element_query_duration(m_playBin.get(), &timeFormat, &timeLength) || timeFormat != GST_FORMAT_TIME || static_cast<guint64>(timeLength) == GST_CLOCK_TIME_NONE;
#endif
    if (failure) {
        LOG_MEDIA_MESSAGE("Time duration query failed for %s", m_url.string().utf8().data());
        return numeric_limits<float>::infinity();
    }

    LOG_MEDIA_MESSAGE("Duration: %" GST_TIME_FORMAT, GST_TIME_ARGS(timeLength));

    return static_cast<double>(timeLength) / GST_SECOND;
    // FIXME: handle 3.14.9.5 properly
}

float MediaPlayerPrivateGStreamer::currentTime() const
{
    if (!m_playBin)
        return 0.0f;

    if (m_errorOccured)
        return 0.0f;

    if (m_seeking)
        return m_seekTime;

    // Workaround for
    // https://bugzilla.gnome.org/show_bug.cgi?id=639941 In GStreamer
    // 0.10.35 basesink reports wrong duration in case of EOS and
    // negative playback rate. There's no upstream accepted patch for
    // this bug yet, hence this temporary workaround.
    if (m_isEndReached && m_playbackRate < 0)
        return 0.0f;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (m_isEndReached && m_mediaDurationKnown)
        return m_mediaDuration;

    float curruntTime = playbackPosition();
    if (isinf(curruntTime))
        return m_fallbackCurrentTime;

    return curruntTime;
#else
    return playbackPosition();
#endif
}

void MediaPlayerPrivateGStreamer::seek(float time)
{
    if (!m_playBin)
        return;

    if (m_errorOccured)
        return;

    LOG_MEDIA_MESSAGE("Seek attempt to %f secs", time);
    TIZEN_LOGI("MediaPlayerPrivateGStreamer::seek to %f secs", time);
#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    // Avoid useless seeking.
    if (time == currentTime())
        return;

#else
    setQueue2Blocked(false);
    setDownloadBufferBlocked(false);
    setDecodebinBlocked(false);

    GstState state;
    GstState pending;
    gst_element_get_state(m_playBin.get(), &state, &pending, 0);
    if (state < GST_STATE_PAUSED) {
        changePipelineState(GST_STATE_PAUSED);
        if (isReleased())
            m_releasedTime = time;
        LOG_MEDIA_MESSAGE("Pipeline is not ready to seek, delaying seek when state changes to pause");
        return;
    }

    if (state == GST_STATE_PLAYING || (state == GST_STATE_PAUSED && pending == GST_STATE_PAUSED)) {
        if (state == GST_STATE_PLAYING) {
            LOG_MEDIA_MESSAGE("need to pause a pipeline to seek");
            changePipelineState(GST_STATE_PAUSED);
        }
        m_pendingSeekTime = time;
        m_pendingSeek = true;
        return;
    }
#endif
    // Extract the integer part of the time (seconds) and the
    // fractional part (microseconds). Attempt to round the
    // microseconds so no floating point precision is lost and we can
    // perform an accurate seek.
    float seconds;
    float microSeconds = modf(time, &seconds) * 1000000;
    GTimeVal timeValue;
    timeValue.tv_sec = static_cast<glong>(seconds);
    timeValue.tv_usec = static_cast<glong>(roundf(microSeconds / 10000) * 10000);

    GstClockTime clockTime = GST_TIMEVAL_TO_TIME(timeValue);
    LOG_MEDIA_MESSAGE("Seek: %" GST_TIME_FORMAT, GST_TIME_ARGS(clockTime));

    if (!gst_element_seek(m_playBin.get(), m_player->rate(),
            GST_FORMAT_TIME,
            (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
            GST_SEEK_TYPE_SET, clockTime,
            GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
        LOG_MEDIA_MESSAGE("Seek to %f failed", time);
    } else {
        m_seeking = true;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        if (time != m_mediaDuration)
            m_isEndReached = false;
        m_fallbackCurrentTime = m_seekTime = time;
        m_bufferingPercentage = 0;
#endif
    }
}

void MediaPlayerPrivateGStreamer::pendingSeek()
{
    if (!m_pendingSeek)
        return;

    m_pendingSeek = false;
    seek(m_pendingSeekTime);

    // pending can occur again in seek()
    if (!m_pendingSeek)
        m_pendingSeekTime = 0.0f;

    LOG_MEDIA_MESSAGE("Pending Seek.");
}

bool MediaPlayerPrivateGStreamer::paused() const
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    GstState state, pending;
    gst_element_get_state(m_playBin.get(), &state, &pending, 0);
    return state == GST_STATE_PAUSED || pending == GST_STATE_PAUSED;
#else
    if (m_isEndReached) {
        LOG_MEDIA_MESSAGE("Ignoring pause at EOS");
        return true;
    }

    GstState state;
    gst_element_get_state(m_playBin.get(), &state, 0, 0);
    return state == GST_STATE_PAUSED;
#endif
}

bool MediaPlayerPrivateGStreamer::seeking() const
{
    return m_seeking;
}

// Returns the size of the video
IntSize MediaPlayerPrivateGStreamer::naturalSize() const
{
    if (!hasVideo())
        return IntSize();

    if (!m_videoSize.isEmpty())
        return m_videoSize;

    GRefPtr<GstCaps> caps = webkitGstGetPadCaps(m_videoSinkPad.get());
    if (!caps)
        return IntSize();


    // TODO: handle possible clean aperture data. See
    // https://bugzilla.gnome.org/show_bug.cgi?id=596571
    // TODO: handle possible transformation matrix. See
    // https://bugzilla.gnome.org/show_bug.cgi?id=596326

    // Get the video PAR and original size, if this fails the
    // video-sink has likely not yet negotiated its caps.
    int pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride;
    IntSize originalSize;
    GstVideoFormat format;
    if (!getVideoSizeAndFormatFromCaps(caps.get(), originalSize, format, pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride))
        return IntSize();

    LOG_MEDIA_MESSAGE("Original video size: %dx%d", originalSize.width(), originalSize.height());
    LOG_MEDIA_MESSAGE("Pixel aspect ratio: %d/%d", pixelAspectRatioNumerator, pixelAspectRatioDenominator);

    // Calculate DAR based on PAR and video size.
    int displayWidth = originalSize.width() * pixelAspectRatioNumerator;
    int displayHeight = originalSize.height() * pixelAspectRatioDenominator;

    // Divide display width and height by their GCD to avoid possible overflows.
    int displayAspectRatioGCD = greatestCommonDivisor(displayWidth, displayHeight);
    displayWidth /= displayAspectRatioGCD;
    displayHeight /= displayAspectRatioGCD;

    // Apply DAR to original video size. This is the same behavior as in xvimagesink's setcaps function.
    guint64 width = 0, height = 0;
    if (!(originalSize.height() % displayHeight)) {
        LOG_MEDIA_MESSAGE("Keeping video original height");
        width = gst_util_uint64_scale_int(originalSize.height(), displayWidth, displayHeight);
        height = static_cast<guint64>(originalSize.height());
    } else if (!(originalSize.width() % displayWidth)) {
        LOG_MEDIA_MESSAGE("Keeping video original width");
        height = gst_util_uint64_scale_int(originalSize.width(), displayHeight, displayWidth);
        width = static_cast<guint64>(originalSize.width());
    } else {
        LOG_MEDIA_MESSAGE("Approximating while keeping original video height");
        width = gst_util_uint64_scale_int(originalSize.height(), displayWidth, displayHeight);
        height = static_cast<guint64>(originalSize.height());
    }

    LOG_MEDIA_MESSAGE("Natural size: %" G_GUINT64_FORMAT "x%" G_GUINT64_FORMAT, width, height);
    m_videoSize = IntSize(static_cast<int>(width), static_cast<int>(height));
    return m_videoSize;
}

void MediaPlayerPrivateGStreamer::videoChanged()
{
    if (m_videoTimerHandler)
        g_source_remove(m_videoTimerHandler);
    m_videoTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivateVideoChangeTimeoutCallback), this);
}

void MediaPlayerPrivateGStreamer::notifyPlayerOfVideo()
{
    m_videoTimerHandler = 0;

    gint videoTracks = 0;
    if (m_playBin)
        g_object_get(m_playBin.get(), "n-video", &videoTracks, NULL);

    m_hasVideo = videoTracks > 0;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    LOG_MEDIA_MESSAGE("notifyPlayerOfVideo = %d", m_hasVideo);
    int width, height;
    bool ret = false;

#ifdef GST_API_VERSION_1
    GstCaps* caps = gst_pad_get_current_caps(m_videoSinkPad.get());
    if (caps) {
        GstVideoInfo info;
        gst_video_info_init(&info);

        if (ret = gst_video_info_from_caps(&info, caps)){
            width = info.width;
            height = info.height;
        }
    }
#else
    ret = gst_video_get_size(m_videoSinkPad.get(), &width, &height))
#endif

    if(ret) {
        IntSize size(width, height);
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
        if (m_videoSize != size || m_updatePixmapSize) {
#else
        if (m_videoSize != size) {
#endif
            m_videoSize = IntSize();
            naturalSize();
#if ENABLE(TIZEN_MEDIA_STREAM)
            int orientation = 0;
            if (isLocalMediaStream() && getFrameOrientation(&orientation)
                && (orientation == 0 || orientation == 180)) {
                m_videoSize = IntSize(static_cast<int>(height), static_cast<int>(width));
                m_rotation = getCameraRotateDirection();
                if(m_cameraId == 1)
                    setRotateProperty(m_rotation);
                else
                    setRotateProperty(DEGREE_270);
            }

#endif

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
            m_updatePixmapSize = true;
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
            if (m_overlayType == MediaPlayer::HwOverlay)
                setXWindowHandle();
            else if (m_overlayType == MediaPlayer::Pixmap)
#endif
            setPixmap();
#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
        }
    }

    m_player->mediaPlayerClient()->mediaPlayerEngineUpdated(m_player);
#else // ENABLE(TIZEN_GSTREAMER_VIDEO)
    m_videoSize = IntSize();

    m_player->mediaPlayerClient()->mediaPlayerEngineUpdated(m_player);
#endif
}

void MediaPlayerPrivateGStreamer::audioChanged()
{
    if (m_audioTimerHandler)
        g_source_remove(m_audioTimerHandler);
    m_audioTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivateAudioChangeTimeoutCallback), this);
}

void MediaPlayerPrivateGStreamer::notifyPlayerOfAudio()
{
    m_audioTimerHandler = 0;

    gint audioTracks = 0;
    if (m_playBin)
        g_object_get(m_playBin.get(), "n-audio", &audioTracks, NULL);
    m_hasAudio = audioTracks > 0;
    m_player->mediaPlayerClient()->mediaPlayerEngineUpdated(m_player);
}

void MediaPlayerPrivateGStreamer::setVolume(float volume)
{
    if (!m_playBin)
        return;

    gst_stream_volume_set_volume(GST_STREAM_VOLUME(m_playBin.get()), GST_STREAM_VOLUME_FORMAT_CUBIC,
                                 static_cast<double>(volume));
}

void MediaPlayerPrivateGStreamer::notifyPlayerOfVolumeChange()
{
    m_volumeTimerHandler = 0;

    if (!m_player || !m_playBin)
        return;

    double volume;
    volume = gst_stream_volume_get_volume(GST_STREAM_VOLUME(m_playBin.get()), GST_STREAM_VOLUME_FORMAT_CUBIC);
    // get_volume() can return values superior to 1.0 if the user
    // applies software user gain via third party application (GNOME
    // volume control for instance).
    volume = CLAMP(volume, 0.0, 1.0);
    m_player->volumeChanged(static_cast<float>(volume));
}

void MediaPlayerPrivateGStreamer::volumeChanged()
{
    if (m_volumeTimerHandler)
        g_source_remove(m_volumeTimerHandler);
    m_volumeTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivateVolumeChangeTimeoutCallback), this);
}

void MediaPlayerPrivateGStreamer::setRate(float rate)
{
    // Avoid useless playback rate update.
    if (m_playbackRate == rate)
        return;

    GstState state;
    GstState pending;

    gst_element_get_state(m_playBin.get(), &state, &pending, 0);
    if ((state != GST_STATE_PLAYING && state != GST_STATE_PAUSED)
        || (pending == GST_STATE_PAUSED))
        return;

    if (isLiveStream())
        return;

    m_playbackRate = rate;
    m_changingRate = true;

    if (!rate) {
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        changePipelineState(GST_STATE_PAUSED);
#else
        gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
#endif
        return;
    }
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    float currentPosition = static_cast<float>(currentTime() * GST_SECOND);
#else
    float currentPosition = static_cast<float>(playbackPosition() * GST_SECOND);
#endif
    GstSeekFlags flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH);
    gint64 start, end;
    bool mute = false;

    LOG_MEDIA_MESSAGE("Set Rate to %f", rate);
    if (rate > 0) {
        // Mute the sound if the playback rate is too extreme and
        // audio pitch is not adjusted.
        mute = (!m_preservesPitch && (rate < 0.8 || rate > 2));
        start = currentPosition;
        end = GST_CLOCK_TIME_NONE;
    } else {
        start = 0;
        mute = true;

        // If we are at beginning of media, start from the end to
        // avoid immediate EOS.
        if (currentPosition <= 0)
            end = static_cast<gint64>(duration() * GST_SECOND);
        else
            end = currentPosition;
    }

    LOG_MEDIA_MESSAGE("Need to mute audio: %d", (int) mute);

    if (!gst_element_seek(m_playBin.get(), rate, GST_FORMAT_TIME, flags,
                          GST_SEEK_TYPE_SET, start,
                          GST_SEEK_TYPE_SET, end))
        LOG_MEDIA_MESSAGE("Set rate to %f failed", rate);
    else
        g_object_set(m_playBin.get(), "mute", mute, NULL);
}

void MediaPlayerPrivateGStreamer::setPreservesPitch(bool preservesPitch)
{
    m_preservesPitch = preservesPitch;
}

MediaPlayer::NetworkState MediaPlayerPrivateGStreamer::networkState() const
{
    return m_networkState;
}

MediaPlayer::ReadyState MediaPlayerPrivateGStreamer::readyState() const
{
    return m_readyState;
}

PassRefPtr<TimeRanges> MediaPlayerPrivateGStreamer::buffered() const
{
    RefPtr<TimeRanges> timeRanges = TimeRanges::create();
    if (m_errorOccured || isLiveStream())
        return timeRanges.release();

#if GST_CHECK_VERSION(0, 10, 31)
    float mediaDuration(duration());
    if (!mediaDuration || isinf(mediaDuration))
        return timeRanges.release();

    GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);

    if (!gst_element_query(m_playBin.get(), query)) {
        gst_query_unref(query);
        return timeRanges.release();
    }

    gint64 rangeStart = 0, rangeStop = 0;
    for (guint index = 0; index < gst_query_get_n_buffering_ranges(query); index++) {
        if (gst_query_parse_nth_buffering_range(query, index, &rangeStart, &rangeStop))
            timeRanges->add(static_cast<float>((rangeStart * mediaDuration) / GST_FORMAT_PERCENT_MAX ),
                            static_cast<float>((rangeStop * mediaDuration) / GST_FORMAT_PERCENT_MAX ));
    }

    // Fallback to the more general maxTimeLoaded() if no range has
    // been found.
    if (!timeRanges->length())
        if (float loaded = maxTimeLoaded())
            timeRanges->add(0, loaded);

    gst_query_unref(query);
#else
    float loaded = maxTimeLoaded();
    if (!m_errorOccured && !isLiveStream() && loaded > 0)
        timeRanges->add(0, loaded);
#endif
    return timeRanges.release();
}

gboolean MediaPlayerPrivateGStreamer::handleMessage(GstMessage* message)
{
    GOwnPtr<GError> err;
    GOwnPtr<gchar> debug;
    MediaPlayer::NetworkState error;
    bool issueError = true;
    bool attemptNextLocation = false;
    const GstStructure* structure = gst_message_get_structure(message);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    const gchar* messageTypeName = structure ? gst_structure_get_name(structure) : 0;
#else
    if (structure) {
        const gchar* messageTypeName = gst_structure_get_name(structure);

        // Redirect messages are sent from elements, like qtdemux, to
        // notify of the new location(s) of the media.
        if (!g_strcmp0(messageTypeName, "redirect")) {
            mediaLocationChanged(message);
            return TRUE;
       }
    }
#endif

    LOG_MEDIA_MESSAGE("Message received from element %s", GST_MESSAGE_SRC_NAME(message));
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_ERROR:
        if (m_resetPipeline)
            break;
        gst_message_parse_error(message, &err.outPtr(), &debug.outPtr());
        LOG_MEDIA_MESSAGE("Error %d: %s (url=%s)", err->code, err->message, m_url.string().utf8().data());
        TIZEN_LOGI("Error %d: %s (url=%s)", err->code, err->message,  m_url.string().utf8().data());
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        TIZEN_SECURE_LOGE("GST_MESSAGE_ERROR domain %d code %d: %s (url=%s)", err->domain, err->code, err->message, m_url.string().utf8().data());
#endif
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(m_playBin.get()), GST_DEBUG_GRAPH_SHOW_ALL, "webkit-video.error");

        error = MediaPlayer::Empty;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        if (err->domain == GST_CORE_ERROR) {
            if (err->code == GST_CORE_ERROR_MISSING_PLUGIN)
                error = MediaPlayer::FormatError;
        } else if (err->domain == GST_STREAM_ERROR) {
            if (err->code == GST_STREAM_ERROR_WRONG_TYPE
                || err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND
                || err->code == GST_STREAM_ERROR_TYPE_NOT_FOUND
                || err->code == GST_STREAM_ERROR_FORMAT)
                error = MediaPlayer::FormatError;
            else if (err->code == GST_STREAM_ERROR_FAILED)
                error = MediaPlayer::NetworkError;
            else {
                error = MediaPlayer::DecodeError;
                attemptNextLocation = true;
            }
        } else if (err->domain == GST_RESOURCE_ERROR)
            error = MediaPlayer::NetworkError;

        if (error == MediaPlayer::NetworkError && m_readyState >= MediaPlayer::HaveMetadata && !isLiveStream()) {
            m_shouldDelayErrorEvent = true;
            return TRUE;
        }
#else
        if (err->code == GST_STREAM_ERROR_CODEC_NOT_FOUND
            || err->code == GST_STREAM_ERROR_WRONG_TYPE
            || err->code == GST_STREAM_ERROR_FAILED
            || err->code == GST_CORE_ERROR_MISSING_PLUGIN
            || err->code == GST_RESOURCE_ERROR_NOT_FOUND)
            error = MediaPlayer::FormatError;
        else if (err->domain == GST_STREAM_ERROR) {
            // Let the mediaPlayerClient handle the stream error, in
            // this case the HTMLMediaElement will emit a stalled
            // event.
            if (err->code == GST_STREAM_ERROR_TYPE_NOT_FOUND) {
                LOG_MEDIA_MESSAGE("Decode error, let the Media element emit a stalled event.");
                break;
            }
            error = MediaPlayer::DecodeError;
            attemptNextLocation = true;
        } else if (err->domain == GST_RESOURCE_ERROR)
            error = MediaPlayer::NetworkError;
#endif
        if (attemptNextLocation)
            issueError = !loadNextLocation();
        if (issueError)
            loadingFailed(error);
        break;
    case GST_MESSAGE_EOS:
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        TIZEN_LOGI("End of Stream");
#else
        LOG_MEDIA_MESSAGE("End of Stream");
#endif
        didEnd();
        break;
    case GST_MESSAGE_STATE_CHANGED:
        // Ignore state changes if load is delayed (preload=none). The
        // player state will be updated once commitLoad() is called.
        if (m_delayingLoad) {
            LOG_MEDIA_MESSAGE("Media load has been delayed. Ignoring state changes for now");
            break;
        }

        // Ignore state changes from internal elements. They are
        // forwarded to playbin2 anyway.
        if (GST_MESSAGE_SRC(message) == reinterpret_cast<GstObject*>(m_playBin.get())) {
            updateStates();

            // Construct a filename for the graphviz dot file output.
            GstState oldState, newState;
            gst_message_parse_state_changed(message, &oldState, &newState, 0);

            CString dotFileName = String::format("webkit-video.%s_%s",
                                                 gst_element_state_get_name(oldState),
                                                 gst_element_state_get_name(newState)).utf8();

            GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(m_playBin.get()), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.data());
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
            if (oldState == GST_STATE_READY && newState == GST_STATE_PAUSED) {
                if (!m_volumeAndMuteInitialized) {
                    notifyPlayerOfVolumeChange();
                    notifyPlayerOfMute();
                    m_volumeAndMuteInitialized = true;
                }
                updateStates();
            }

            if (m_pendingSeek && oldState == GST_STATE_PLAYING && newState == GST_STATE_PAUSED)
                pendingSeek();

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)
            if (newState == GST_STATE_PLAYING)
                m_player->mediaPlayerClient()->mediaPlayerRenderingModeChanged(m_player);
#endif
#endif
        }
        break;
    case GST_MESSAGE_BUFFERING:
        processBufferingStats(message);
        break;
    case GST_MESSAGE_DURATION:
        LOG_MEDIA_MESSAGE("Duration changed");
        durationChanged();
        break;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    case GST_MESSAGE_TAG:
        LOG_MEDIA_MESSAGE("Tag found");
        if (element()->isVideo() && !hasVideo())
            if (extractTagImage(message))
                element()->displayTagImage(m_tagImagePath);
        break;
    case GST_MESSAGE_ELEMENT:
        if (!g_strcmp0(messageTypeName, "redirect")) {
            // Redirect messages are sent from elements, like qtdemux, to
            // notify of the new location(s) of the media.
            LOG_MEDIA_MESSAGE("Redirect");
            mediaLocationChanged(message);
        } else if (!g_strcmp0(messageTypeName, "cookies")) {
            LOG_MEDIA_MESSAGE("Updated cookie");
            const GValue* newCookie = gst_structure_get_value(structure, "updated-cookie");
            const String& cookie = g_value_get_string(newCookie);
            TIZEN_SECURE_LOGI("new cookie : %s", cookie.utf8().data());

            const GValue* requestedUrl = gst_structure_get_value(structure, "updated-url");
            const String& url = g_value_get_string(requestedUrl);
            TIZEN_SECURE_LOGI("requested url : %s", url.utf8().data());

            if (!cookie.isEmpty() && !url.isEmpty())
                setCookies(element()->document() ? element()->document() : element()->ownerDocument(), KURL(KURL(), url), cookie);
        }
        break;
#endif
    default:
        LOG_MEDIA_MESSAGE("Unhandled GStreamer message type: %s",
                    GST_MESSAGE_TYPE_NAME(message));
        break;
    }
    return TRUE;
}

void MediaPlayerPrivateGStreamer::processBufferingStats(GstMessage* message)
{
    // This is the immediate buffering that needs to happen so we have
    // enough to play right now.
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#ifdef GST_API_VERSION_1
    // Use multiqueue for listening to buffer message.
    if (GST_MESSAGE_SRC(message) == reinterpret_cast<GstObject*>(m_downloadBuffer))
        return;
#else
    bool bufferingStarted = !m_buffering;
#endif

    m_buffering = true;
    String elementName = String::fromUTF8(GST_MESSAGE_SRC_NAME(message));
    const GstStructure *structure = gst_message_get_structure(message);
    gst_structure_get_int(structure, "buffer-percent", &m_bufferingPercentage);
    LOG_MEDIA_MESSAGE("%s is Buffering: %d%%", elementName.utf8().data(), m_bufferingPercentage);

    TIZEN_LOGI("%s is Buffering: %d%%", elementName.utf8().data(), m_bufferingPercentage);

    // Buffer messages can be ignored while using DOWNLOAD mode. But, spinner animation
    // needs to be shown. Hence, the messages are handled.
    //
    // To avoid frequent and long stopage.
#ifdef GST_API_VERSION_1
    if (m_bufferingPercentage <= 10 || m_bufferingPercentage >= 30) {
#else
    if (bufferingStarted || m_bufferingPercentage == 100) {
#endif
        updateStates();
        return;
    }
#else
    m_buffering = true;
    const GstStructure *structure = gst_message_get_structure(message);
    gst_structure_get_int(structure, "buffer-percent", &m_bufferingPercentage);
    LOG_MEDIA_MESSAGE("[Buffering] Buffering: %d%%.", m_bufferingPercentage);

    GstBufferingMode mode;
    gst_message_parse_buffering_stats(message, &mode, 0, 0, 0);
    if (mode != GST_BUFFERING_DOWNLOAD) {
        updateStates();
        return;
    }

    // This is on-disk buffering, that allows us to download much more
    // than needed for right now.
    if (!m_startedBuffering) {
        LOG_MEDIA_MESSAGE("[Buffering] Starting on-disk buffering.");

        m_startedBuffering = true;

        if (m_fillTimer.isActive())
            m_fillTimer.stop();

        m_fillTimer.startRepeating(0.2);
    }
#endif
}

void MediaPlayerPrivateGStreamer::fillTimerFired(Timer<MediaPlayerPrivateGStreamer>*)
{
    GstQuery* query = gst_query_new_buffering(GST_FORMAT_PERCENT);

    if (!gst_element_query(m_playBin.get(), query)) {
        gst_query_unref(query);
        return;
    }

    gint64 start, stop;
    gdouble fillStatus = 100.0;

    gst_query_parse_buffering_range(query, 0, &start, &stop, 0);
    gst_query_unref(query);

    if (stop != -1)
        fillStatus = 100.0 * stop / GST_FORMAT_PERCENT_MAX;

    LOG_MEDIA_MESSAGE("[Buffering] Download buffer filled up to %f%%", fillStatus);

    if (!m_mediaDuration)
        durationChanged();

    // Update maxTimeLoaded only if the media duration is
    // available. Otherwise we can't compute it.
    if (m_mediaDuration) {
        if (fillStatus == 100.0)
            m_maxTimeLoaded = m_mediaDuration;
        else
            m_maxTimeLoaded = static_cast<float>((fillStatus * m_mediaDuration) / 100.0);
        LOG_MEDIA_MESSAGE("[Buffering] Updated maxTimeLoaded: %f", m_maxTimeLoaded);
    }

    if (fillStatus != 100.0) {
        updateStates();
        return;
    }

    // Media is now fully loaded. It will play even if network
    // connection is cut. Buffering is done, remove the fill source
    // from the main loop.
    m_fillTimer.stop();
    m_startedBuffering = false;
    updateStates();
}

float MediaPlayerPrivateGStreamer::maxTimeSeekable() const
{
    if (m_errorOccured)
        return 0.0f;

    LOG_MEDIA_MESSAGE("maxTimeSeekable");
    // infinite duration means live stream
    if (isinf(duration()))
        return 0.0f;

    return duration();
}

float MediaPlayerPrivateGStreamer::maxTimeLoaded() const
{
    if (m_errorOccured)
        return 0.0f;

    float loaded = m_maxTimeLoaded;
    if (!loaded && !m_fillTimer.isActive())
        loaded = duration();
    LOG_MEDIA_MESSAGE("maxTimeLoaded: %f", loaded);
    return loaded;
}

bool MediaPlayerPrivateGStreamer::didLoadingProgress() const
{
    if (!m_playBin || !m_mediaDuration || !totalBytes())
        return false;
    float currentMaxTimeLoaded = maxTimeLoaded();
    bool didLoadingProgress = currentMaxTimeLoaded != m_maxTimeLoadedAtLastDidLoadingProgress;
    m_maxTimeLoadedAtLastDidLoadingProgress = currentMaxTimeLoaded;
    LOG_MEDIA_MESSAGE("didLoadingProgress: %d", didLoadingProgress);
    return didLoadingProgress;
}

unsigned MediaPlayerPrivateGStreamer::totalBytes() const
{
    if (m_errorOccured)
        return 0;

    if (m_totalBytes != -1)
        return m_totalBytes;

    if (!m_source)
        return 0;

    GstFormat fmt = GST_FORMAT_BYTES;
    gint64 length = 0;
#ifdef GST_API_VERSION_1
    if (gst_element_query_duration(m_source.get(), fmt, &length)) {
#else
    if (gst_element_query_duration(m_source.get(), &fmt, &length)) {
#endif
        LOG_MEDIA_MESSAGE("totalBytes %" G_GINT64_FORMAT, length);
        m_totalBytes = static_cast<unsigned>(length);
        m_isStreaming = !length;
        return m_totalBytes;
    }

    // Fall back to querying the source pads manually.
    // See also https://bugzilla.gnome.org/show_bug.cgi?id=638749
    GstIterator* iter = gst_element_iterate_src_pads(m_source.get());
    bool done = false;
    while (!done) {
#ifdef GST_API_VERSION_1
        GValue item = G_VALUE_INIT;
        switch (gst_iterator_next(iter, &item)) {
        case GST_ITERATOR_OK: {
            GstPad* pad = static_cast<GstPad*>(g_value_get_object(&item));
            gint64 padLength = 0;
            if (gst_pad_query_duration(pad, fmt, &padLength) && padLength > length)
                length = padLength;
            break;
        }
#else
        gpointer data;

        switch (gst_iterator_next(iter, &data)) {
        case GST_ITERATOR_OK: {
            GRefPtr<GstPad> pad = adoptGRef(GST_PAD_CAST(data));
            gint64 padLength = 0;
            if (gst_pad_query_duration(pad.get(), &fmt, &padLength) && padLength > length)
                length = padLength;
            break;
        }
#endif
        case GST_ITERATOR_RESYNC:
            gst_iterator_resync(iter);
            break;
        case GST_ITERATOR_ERROR:
            // Fall through.
        case GST_ITERATOR_DONE:
            done = true;
            break;
        }

#ifdef GST_API_VERSION_1
        g_value_unset(&item);
#endif
    }

    gst_iterator_free(iter);

    LOG_MEDIA_MESSAGE("totalBytes %" G_GINT64_FORMAT, length);
    m_totalBytes = static_cast<unsigned>(length);
    m_isStreaming = !length;
    return m_totalBytes;
}

unsigned MediaPlayerPrivateGStreamer::decodedFrameCount() const
{
    guint64 decodedFrames = 0;
    if (m_fpsSink)
        g_object_get(m_fpsSink, "frames-rendered", &decodedFrames, NULL);
    return static_cast<unsigned>(decodedFrames);
}

unsigned MediaPlayerPrivateGStreamer::droppedFrameCount() const
{
    guint64 framesDropped = 0;
    if (m_fpsSink)
        g_object_get(m_fpsSink, "frames-dropped", &framesDropped, NULL);
    return static_cast<unsigned>(framesDropped);
}

unsigned MediaPlayerPrivateGStreamer::audioDecodedByteCount() const
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    GstState state;
    gst_element_get_state(m_playBin.get(), &state, 0, 0);
    if (state < GST_STATE_PAUSED)
        return 0;
#endif
    GstQuery* query = gst_query_new_position(GST_FORMAT_BYTES);
    gint64 position = 0;

    if (m_webkitAudioSink && gst_element_query(m_webkitAudioSink.get(), query))
        gst_query_parse_position(query, 0, &position);

    gst_query_unref(query);
    return static_cast<unsigned>(position);
}

unsigned MediaPlayerPrivateGStreamer::videoDecodedByteCount() const
{
    GstQuery* query = gst_query_new_position(GST_FORMAT_BYTES);
    gint64 position = 0;

    if (gst_element_query(m_webkitVideoSink, query))
        gst_query_parse_position(query, 0, &position);

    gst_query_unref(query);
    return static_cast<unsigned>(position);
}

void MediaPlayerPrivateGStreamer::updateAudioSink()
{
    if (!m_playBin)
        return;

    GstElement* sinkPtr = 0;

    g_object_get(m_playBin.get(), "audio-sink", &sinkPtr, NULL);
    m_webkitAudioSink = adoptGRef(sinkPtr);

}


void MediaPlayerPrivateGStreamer::sourceChanged()
{
    GstElement* srcPtr = 0;

    g_object_get(m_playBin.get(), "source", &srcPtr, NULL);
    m_source = adoptGRef(srcPtr);

    if (WEBKIT_IS_WEB_SRC(m_source.get()))
        webKitWebSrcSetMediaPlayer(WEBKIT_WEB_SRC(m_source.get()), m_player);
}

void MediaPlayerPrivateGStreamer::cancelLoad()
{
    if (m_networkState < MediaPlayer::Loading || m_networkState == MediaPlayer::Loaded)
        return;

    if (m_playBin)
        gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaPlayerPrivateGStreamer::updateStates()
{
    if (!m_playBin)
        return;

    if (m_errorOccured)
        return;

    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;
    GstState state, pending;

    GstStateChangeReturn ret = gst_element_get_state(m_playBin.get(), &state, &pending, 250 * GST_NSECOND);
    LOG_MEDIA_MESSAGE("Ret: %s, State: %s, pending: %s", gst_element_state_change_return_get_name(ret), gst_element_state_get_name(state), gst_element_state_get_name(pending));

    switch (ret) {
    case GST_STATE_CHANGE_SUCCESS:
    case GST_STATE_CHANGE_ASYNC:
    case GST_STATE_CHANGE_NO_PREROLL:
        if (!isLiveStream())
            cacheDuration();

        // Check readyState
        if (m_mediaDurationKnown || state >= GST_STATE_PAUSED || oldReadyState >= MediaPlayer::HaveMetadata)
            m_readyState = MediaPlayer::HaveMetadata;

        if (oldReadyState >= MediaPlayer::HaveMetadata
            && state >= GST_STATE_PAUSED
            && (pending == GST_STATE_VOID_PENDING || pending == GST_STATE_PAUSED)) {
            if (!isinf(playbackPosition()))
                m_readyState = MediaPlayer::HaveCurrentData;

            if (m_readyState == MediaPlayer::HaveCurrentData) {
#ifdef GST_API_VERSION_1
                if (m_bufferingPercentage >= 30) {
#else
                if (m_bufferingPercentage == 100) {
#endif
                    // Block pipeline to prevent to load unnecessary data before play() is called explicitly.
                    if (!m_havePlayed && !element()->autoplay()
                        && (m_hasAudio || m_hasVideo) && url().protocol().startsWith("http", false)) {
                        setQueue2Blocked(true);
                        setDownloadBufferBlocked(true);
                        setDecodebinBlocked(true);
                    }

                    m_readyState = MediaPlayer::HaveEnoughData;
                    m_buffering = false;
                    if (!m_mediaDurationKnown)
                        m_isStreaming = true;
                } else if (state == GST_STATE_PLAYING && url().protocol().startsWith("http", false)) {
                    LOG_MEDIA_MESSAGE("Buffering, need to pause pipeline until finish buffering.");
                    gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
                }
                if (!url().protocol().startsWith("http", false))
                    m_readyState = MediaPlayer::HaveEnoughData;
            }
        }

        m_buffering || m_networkState == MediaPlayer::Empty ? element()->showLoadingSpinner() : element()->hideLoadingSpinner();

        if (element()->isReleasedByMediaResourceController())
            m_readyState = MediaPlayer::HaveEnoughData;

        // Check networkState
        if (state >= GST_STATE_READY)
            m_networkState = MediaPlayer::Loading;

        if (m_isEndReached || isReleased() || !url().protocol().startsWith("http", false) || maxTimeLoaded() == duration() || (isLiveStream() && state >= GST_STATE_PAUSED))
            m_networkState = MediaPlayer::Loaded;

        break;
    case GST_STATE_CHANGE_FAILURE:
        TIZEN_LOGE("GST_STATE_CHANGE_FAILURE.");
        break;
    default:
        LOG_MEDIA_MESSAGE("Else : %d", ret);
        break;
    }

    if (m_changingRate) {
        m_player->rateChanged();
        m_changingRate = false;
    }

    if (seeking() && m_readyState >= MediaPlayer::HaveCurrentData) {
        m_seeking = false;
        timeChanged();
    }

    if (m_shouldDelayErrorEvent && (m_isEndReached || m_buffering)) {
        LOG_MEDIA_MESSAGE("Delayed Error event fired that was for spending buffered duration.");
        loadingFailed(MediaPlayer::NetworkError);
        m_shouldDelayErrorEvent = false;
    }

    if (m_networkState != oldNetworkState) {
        LOG_MEDIA_MESSAGE("Network State Changed from %u to %u", oldNetworkState, m_networkState);
        m_player->networkStateChanged();
    }
    if (m_readyState != oldReadyState) {
        LOG_MEDIA_MESSAGE("Ready State Changed from %u to %u", oldReadyState, m_readyState);
        m_player->readyStateChanged();
    }
}
#else // updateStates() not used
void MediaPlayerPrivateGStreamer::updateStates()
{
    if (!m_playBin)
        return;

    if (m_errorOccured)
        return;

    MediaPlayer::NetworkState oldNetworkState = m_networkState;
    MediaPlayer::ReadyState oldReadyState = m_readyState;
    GstState state;
    GstState pending;

    GstStateChangeReturn ret = gst_element_get_state(m_playBin.get(),
        &state, &pending, 250 * GST_NSECOND);

    bool shouldUpdateAfterSeek = false;
    switch (ret) {
    case GST_STATE_CHANGE_SUCCESS:
        LOG_MEDIA_MESSAGE("State: %s, pending: %s",
            gst_element_state_get_name(state),
            gst_element_state_get_name(pending));

        m_resetPipeline = state <= GST_STATE_READY;

        // Try to figure out ready and network states.
        if (state == GST_STATE_READY) {
            m_readyState = MediaPlayer::HaveMetadata;
            m_networkState = MediaPlayer::Empty;
            // Cache the duration without emiting the durationchange
            // event because it's taken care of by the media element
            // in this precise case.
            if (!m_isEndReached)
                cacheDuration();
        } else if ((state == GST_STATE_NULL) || (maxTimeLoaded() == duration())) {
            m_networkState = MediaPlayer::Loaded;
            m_readyState = MediaPlayer::HaveEnoughData;
        } else {
            m_readyState = currentTime() < maxTimeLoaded() ? MediaPlayer::HaveFutureData : MediaPlayer::HaveCurrentData;
            m_networkState = MediaPlayer::Loading;
        }

        if (m_buffering && state != GST_STATE_READY) {
            m_readyState = MediaPlayer::HaveCurrentData;
            m_networkState = MediaPlayer::Loading;
        }

        // Now let's try to get the states in more detail using
        // information from GStreamer, while we sync states where
        // needed.
        if (state == GST_STATE_PAUSED) {
            if (!m_webkitAudioSink)
                updateAudioSink();

            if (!m_volumeAndMuteInitialized) {
                notifyPlayerOfVolumeChange();
                notifyPlayerOfMute();
                m_volumeAndMuteInitialized = true;
            }

            if (m_buffering && m_bufferingPercentage == 100) {
                m_buffering = false;
                m_bufferingPercentage = 0;
                m_readyState = MediaPlayer::HaveEnoughData;

                LOG_MEDIA_MESSAGE("[Buffering] Complete.");

                if (!m_paused) {
                    LOG_MEDIA_MESSAGE("[Buffering] Restarting playback.");
                    gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
                }
            } else if (!m_buffering && (currentTime() < duration())) {
                m_paused = true;
            }
        } else if (state == GST_STATE_PLAYING) {
            m_readyState = MediaPlayer::HaveEnoughData;
            m_paused = false;

            if (m_buffering && !isLiveStream()) {
                m_readyState = MediaPlayer::HaveCurrentData;
                m_networkState = MediaPlayer::Loading;

                LOG_MEDIA_MESSAGE("[Buffering] Pausing stream for buffering.");

                gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
            }
        } else
            m_paused = true;

        // Is on-disk buffering in progress?
        if (m_fillTimer.isActive())
            m_networkState = MediaPlayer::Loading;

        if (m_changingRate) {
            m_player->rateChanged();
            m_changingRate = false;
        }

        if (m_seeking) {
            shouldUpdateAfterSeek = true;
            m_seeking = false;
        }

        break;
    case GST_STATE_CHANGE_ASYNC:
        LOG_MEDIA_MESSAGE("Async: State: %s, pending: %s",
            gst_element_state_get_name(state),
            gst_element_state_get_name(pending));
        // Change in progress

        // On-disk buffering was attempted but the media is live. This
        // can't work so disable on-disk buffering and reset the
        // pipeline.
        if (state == GST_STATE_READY && isLiveStream() && m_preload == MediaPlayer::Auto) {
            setPreload(MediaPlayer::None);
            gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
            gst_element_set_state(m_playBin.get(), GST_STATE_PAUSED);
        }

        // A live stream was paused, reset the pipeline.
        if (state == GST_STATE_PAUSED && pending == GST_STATE_PLAYING && isLiveStream()) {
            gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
            gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
        }

        if (!isLiveStream() && !m_buffering)
            return;

        if (m_seeking) {
            shouldUpdateAfterSeek = true;
            m_seeking = false;
        }
        break;
    case GST_STATE_CHANGE_FAILURE:
        LOG_MEDIA_MESSAGE("Failure: State: %s, pending: %s",
            gst_element_state_get_name(state),
            gst_element_state_get_name(pending));
        // Change failed
        return;
    case GST_STATE_CHANGE_NO_PREROLL:
        LOG_MEDIA_MESSAGE("No preroll: State: %s, pending: %s",
            gst_element_state_get_name(state),
            gst_element_state_get_name(pending));

        if (state == GST_STATE_READY)
            m_readyState = MediaPlayer::HaveNothing;
        else if (state == GST_STATE_PAUSED) {
            m_readyState = MediaPlayer::HaveEnoughData;
            m_paused = true;
            // Live pipelines go in PAUSED without prerolling.
            m_isStreaming = true;
        } else if (state == GST_STATE_PLAYING)
            m_paused = false;

        if (m_seeking) {
            shouldUpdateAfterSeek = true;
            m_seeking = false;
            if (!m_paused)
                gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
        } else if (!m_paused)
            gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);

        m_networkState = MediaPlayer::Loading;
        break;
    default:
        LOG_MEDIA_MESSAGE("Else : %d", ret);
        break;
    }

    if (seeking())
        m_readyState = MediaPlayer::HaveNothing;

    if (shouldUpdateAfterSeek)
        timeChanged();

    if (m_networkState != oldNetworkState) {
        LOG_MEDIA_MESSAGE("Network State Changed from %u to %u",
            oldNetworkState, m_networkState);
        m_player->networkStateChanged();
    }
    if (m_readyState != oldReadyState) {
        LOG_MEDIA_MESSAGE("Ready State Changed from %u to %u",
            oldReadyState, m_readyState);
        m_player->readyStateChanged();
    }
}
#endif

void MediaPlayerPrivateGStreamer::mediaLocationChanged(GstMessage* message)
{
    if (m_mediaLocations)
        gst_structure_free(m_mediaLocations);

    const GstStructure* structure = gst_message_get_structure(message);
    if (structure) {
        // This structure can contain:
        // - both a new-location string and embedded locations structure
        // - or only a new-location string.
        m_mediaLocations = gst_structure_copy(structure);
        const GValue* locations = gst_structure_get_value(m_mediaLocations, "locations");

        if (locations)
            m_mediaLocationCurrentIndex = static_cast<int>(gst_value_list_get_size(locations)) -1;

        loadNextLocation();
    }
}

bool MediaPlayerPrivateGStreamer::loadNextLocation()
{
    if (!m_mediaLocations)
        return false;

    const GValue* locations = gst_structure_get_value(m_mediaLocations, "locations");
    const gchar* newLocation = 0;

    if (!locations) {
        // Fallback on new-location string.
        newLocation = gst_structure_get_string(m_mediaLocations, "new-location");
        if (!newLocation)
            return false;
    }

    if (!newLocation) {
        if (m_mediaLocationCurrentIndex < 0) {
            m_mediaLocations = 0;
            return false;
        }

        const GValue* location = gst_value_list_get_value(locations,
                                                          m_mediaLocationCurrentIndex);
        const GstStructure* structure = gst_value_get_structure(location);

        if (!structure) {
            m_mediaLocationCurrentIndex--;
            return false;
        }

        newLocation = gst_structure_get_string(structure, "new-location");
    }

    if (newLocation) {
        // Found a candidate. new-location is not always an absolute url
        // though. We need to take the base of the current url and
        // append the value of new-location to it.

        gchar* currentLocation = 0;
        g_object_get(m_playBin.get(), "uri", &currentLocation, NULL);

        KURL currentUrl(KURL(), currentLocation);
        g_free(currentLocation);

        KURL newUrl;

        if (gst_uri_is_valid(newLocation))
            newUrl = KURL(KURL(), newLocation);
        else
            newUrl = KURL(KURL(), currentUrl.baseAsString() + newLocation);

        RefPtr<SecurityOrigin> securityOrigin = SecurityOrigin::create(currentUrl);
        if (securityOrigin->canRequest(newUrl)) {
            LOG_MEDIA_MESSAGE("New media url: %s", newUrl.string().utf8().data());

            // Reset player states.
            m_networkState = MediaPlayer::Loading;
            m_player->networkStateChanged();
            m_readyState = MediaPlayer::HaveNothing;
            m_player->readyStateChanged();

            // Reset pipeline state
            m_resetPipeline = true;
            gst_element_set_state(m_playBin.get(), GST_STATE_READY);

            GstState state;
            gst_element_get_state(m_playBin.get(), &state, 0, 0);
            if (state <= GST_STATE_READY) {
                // Set the new uri and start playing.
                g_object_set(m_playBin.get(), "uri", newUrl.string().utf8().data(), NULL);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
                changePipelineState(GST_STATE_PLAYING);
#else
                gst_element_set_state(m_playBin.get(), GST_STATE_PLAYING);
#endif
                return true;
            }
        }
    }
    m_mediaLocationCurrentIndex--;
    return false;

}

void MediaPlayerPrivateGStreamer::loadStateChanged()
{
    updateStates();
}

void MediaPlayerPrivateGStreamer::sizeChanged()
{
    notImplemented();
}

void MediaPlayerPrivateGStreamer::timeChanged()
{
#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    updateStates();
#endif
    m_player->timeChanged();
}

void MediaPlayerPrivateGStreamer::didEnd()
{
    // Synchronize position and duration values to not confuse the
    // HTMLMediaElement. In some cases like reverse playback the
    // position is not always reported as 0 for instance.
    float now = currentTime();
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (now > 0 && m_mediaDuration != now) {
#else
    if (now > 0 && now <= duration() && m_mediaDuration != now) {
#endif
        m_mediaDurationKnown = true;
        m_mediaDuration = now;
        m_player->durationChanged();
    }

    m_isEndReached = true;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    updateStates();
#endif
    timeChanged();

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (!m_player->mediaPlayerClient()->mediaPlayerIsLooping()) {
        m_paused = true;
        gst_element_set_state(m_playBin.get(), GST_STATE_NULL);
    }
#endif
}

void MediaPlayerPrivateGStreamer::cacheDuration()
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    // And re-cache it if possible.
    GstState state;
    gst_element_get_state(m_playBin.get(), &state, 0, 0);
    if (state < GST_STATE_PAUSED)
        return;

    float previousDuration = m_mediaDuration;

    // Reset cached media duration
    m_mediaDuration = 0;

    float newDuration = duration();
    m_mediaDurationKnown = !isinf(newDuration);

    if (m_mediaDurationKnown) {
        m_mediaDuration = newDuration;
        if (newDuration != previousDuration && m_readyState >= MediaPlayer::HaveMetadata)
            m_player->durationChanged();
    } else
        m_mediaDuration = previousDuration;
#else
    // Reset cached media duration
    m_mediaDuration = 0;

    // And re-cache it if possible.
    GstState state;
    gst_element_get_state(m_playBin.get(), &state, 0, 0);
    float newDuration = duration();

    if (state <= GST_STATE_READY) {
        // Don't set m_mediaDurationKnown yet if the pipeline is not
        // paused. This allows duration() query to fail at least once
        // before playback starts and duration becomes known.
        if (!isinf(newDuration))
            m_mediaDuration = newDuration;
    } else {
        m_mediaDurationKnown = !isinf(newDuration);
        if (m_mediaDurationKnown)
            m_mediaDuration = newDuration;
    }

    if (!isinf(newDuration))
        m_mediaDuration = newDuration;
#endif
}

void MediaPlayerPrivateGStreamer::durationChanged()
{
#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    float previousDuration = m_mediaDuration;
#endif

    cacheDuration();

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    // Avoid emiting durationchanged in the case where the previous
    // duration was 0 because that case is already handled by the
    // HTMLMediaElement.
    if (previousDuration && m_mediaDuration != previousDuration)
        m_player->durationChanged();
#endif
}

bool MediaPlayerPrivateGStreamer::supportsMuting() const
{
    return true;
}

void MediaPlayerPrivateGStreamer::setMuted(bool muted)
{
    if (!m_playBin)
        return;

    g_object_set(m_playBin.get(), "mute", muted, NULL);
}

void MediaPlayerPrivateGStreamer::notifyPlayerOfMute()
{
    m_muteTimerHandler = 0;

    if (!m_player || !m_playBin)
        return;

    gboolean muted;
    g_object_get(m_playBin.get(), "mute", &muted, NULL);
    m_player->muteChanged(static_cast<bool>(muted));
}

void MediaPlayerPrivateGStreamer::muteChanged()
{
    if (m_muteTimerHandler)
        g_source_remove(m_muteTimerHandler);
    m_muteTimerHandler = g_timeout_add(0, reinterpret_cast<GSourceFunc>(mediaPlayerPrivateMuteChangeTimeoutCallback), this);
}

void MediaPlayerPrivateGStreamer::loadingFailed(MediaPlayer::NetworkState error)
{
    m_errorOccured = true;
    if (m_networkState != error) {
        m_networkState = error;
        m_player->networkStateChanged();
    }
    if (m_readyState != MediaPlayer::HaveNothing) {
        m_readyState = MediaPlayer::HaveNothing;
        m_player->readyStateChanged();
    }
}

void MediaPlayerPrivateGStreamer::setSize(const IntSize& size)
{
    m_size = size;
}

void MediaPlayerPrivateGStreamer::setVisible(bool visible)
{
}

void MediaPlayerPrivateGStreamer::triggerRepaint(GstBuffer* buffer)
{
    g_return_if_fail(GST_IS_BUFFER(buffer));
    gst_buffer_replace(&m_buffer, buffer);
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)
#if !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    if (m_videoSize.isEmpty())
        m_videoSize =  naturalSize();
    m_videoLayer->paintVideoLayer(m_videoSize);
    return;
#endif
#endif
    m_player->repaint();
}

void MediaPlayerPrivateGStreamer::paint(GraphicsContext* context, const IntRect& rect)
{
    if (context->paintingDisabled())
        return;

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (!m_player->visible())
        return;
#endif

    if (!m_buffer)
        return;

    GRefPtr<GstCaps> caps = webkitGstGetPadCaps(m_videoSinkPad.get());
    if (!caps)
        return;

    RefPtr<ImageGStreamer> gstImage = ImageGStreamer::createImage(m_buffer, caps.get());
    if (!gstImage)
        return;

    context->drawImage(reinterpret_cast<Image*>(gstImage->image().get()), ColorSpaceSRGB,
                       rect, gstImage->rect(), CompositeCopy, DoNotRespectImageOrientation, false);
}

static HashSet<String> mimeTypeCache()
{
    initializeGStreamerAndRegisterWebKitElements();

    DEFINE_STATIC_LOCAL(HashSet<String>, cache, ());
    static bool typeListInitialized = false;

    if (typeListInitialized)
        return cache;
    const char* mimeTypes[] = {
        "application/ogg",
        "application/vnd.apple.mpegurl",
        "application/vnd.rn-realmedia",
        "application/x-3gp",
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        "application/x-mpegurl",
        "application/x-mpegURL",
#endif
        "application/x-pn-realaudio",
        "audio/3gpp",
        "audio/aac",
        "audio/flac",
        "audio/iLBC-sh",
        "audio/midi",
        "audio/mobile-xmf",
        "audio/mp1",
        "audio/mp2",
        "audio/mp3",
        "audio/mp4",
        "audio/mpeg",
        "audio/ogg",
        "audio/opus",
        "audio/qcelp",
        "audio/riff-midi",
        "audio/wav",
        "audio/webm",
        "audio/x-ac3",
        "audio/x-aiff",
        "audio/x-amr-nb-sh",
        "audio/x-amr-wb-sh",
        "audio/x-au",
        "audio/x-ay",
        "audio/x-celt",
        "audio/x-dts",
        "audio/x-flac",
        "audio/x-gbs",
        "audio/x-gsm",
        "audio/x-gym",
        "audio/x-imelody",
        "audio/x-ircam",
        "audio/x-kss",
        "audio/x-m4a",
        "audio/x-mod",
        "audio/x-mp3",
        "audio/x-mpeg",
        "audio/x-musepack",
        "audio/x-nist",
        "audio/x-nsf",
        "audio/x-paris",
        "audio/x-sap",
        "audio/x-sbc",
        "audio/x-sds",
        "audio/x-shorten",
        "audio/x-sid",
        "audio/x-spc",
        "audio/x-speex",
        "audio/x-svx",
        "audio/x-ttafile",
        "audio/x-vgm",
        "audio/x-voc",
        "audio/x-vorbis+ogg",
        "audio/x-w64",
        "audio/x-wav",
        "audio/x-wavpack",
        "audio/x-wavpack-correction",
        "video/3gpp",
        "video/mj2",
        "video/mp4",
        "video/mpeg",
        "video/mpegts",
        "video/ogg",
        "video/quicktime",
        "video/vivo",
#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
        "video/webm",
#endif
        "video/x-cdxa",
        "video/x-dirac",
        "video/x-dv",
        "video/x-fli",
#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
        "video/x-flv",
#endif
        "video/x-h263",
        "video/x-ivf",
        "video/x-m4v",
        "video/x-matroska",
        "video/x-mng",
        "video/x-ms-asf",
        "video/x-msvideo",
        "video/x-mve",
        "video/x-nuv",
        "video/x-vcd"
    };

    for (unsigned i = 0; i < (sizeof(mimeTypes) / sizeof(*mimeTypes)); ++i)
        cache.add(String(mimeTypes[i]));

    typeListInitialized = true;
    return cache;
}

void MediaPlayerPrivateGStreamer::getSupportedTypes(HashSet<String>& types)
{
    types = mimeTypeCache();
}

MediaPlayer::SupportsType MediaPlayerPrivateGStreamer::supportsType(const String& type, const String& codecs, const KURL&)
{
    if (type.isNull() || type.isEmpty())
        return MediaPlayer::IsNotSupported;

    // spec says we should not return "probably" if the codecs string is empty
    if (mimeTypeCache().contains(type))
        return codecs.isEmpty() ? MediaPlayer::MayBeSupported : MediaPlayer::IsSupported;
    return MediaPlayer::IsNotSupported;
}

bool MediaPlayerPrivateGStreamer::hasSingleSecurityOrigin() const
{
    return true;
}

bool MediaPlayerPrivateGStreamer::supportsFullscreen() const
{
#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED == 1050
    // See <rdar://problem/7389945>
    return false;
#else
    return true;
#endif
}

PlatformMedia MediaPlayerPrivateGStreamer::platformMedia() const
{
    PlatformMedia p;
#ifndef GST_API_VERSION_1
    p.type = PlatformMedia::GStreamerGWorldType;
    p.media.gstreamerGWorld = m_gstGWorld.get();
#endif
    return p;
}

MediaPlayer::MovieLoadType MediaPlayerPrivateGStreamer::movieLoadType() const
{
    if (m_readyState == MediaPlayer::HaveNothing)
        return MediaPlayer::Unknown;

    if (isLiveStream())
        return MediaPlayer::LiveStream;

    return MediaPlayer::Download;
}

void MediaPlayerPrivateGStreamer::setDownloadBuffering()
{
    if (!m_playBin)
        return;

    GstPlayFlags flags;

    g_object_get(m_playBin.get(), "flags", &flags, NULL);

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (element()->isVideo() && haveEnoughSpace(String(TEMP_FILE_LOCATION), NECESSARY_SIZE_FOR_DOWNLOAD_MODE)) {
        LOG_MEDIA_MESSAGE("Enabling on-disk buffering");
        g_object_set(m_playBin.get(), "flags", flags | GST_PLAY_FLAG_DOWNLOAD, NULL);
    }
#else
    if (m_preload == MediaPlayer::Auto) {
        LOG_MEDIA_MESSAGE("Enabling on-disk buffering");
        g_object_set(m_playBin.get(), "flags", flags | GST_PLAY_FLAG_DOWNLOAD, NULL);
    } else {
        LOG_MEDIA_MESSAGE("Disabling on-disk buffering");
        g_object_set(m_playBin.get(), "flags", flags & ~GST_PLAY_FLAG_DOWNLOAD, NULL);
    }
#endif
}

void MediaPlayerPrivateGStreamer::setPreload(MediaPlayer::Preload preload)
{
#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (preload == MediaPlayer::Auto && isLiveStream())
        return;
#endif
    m_preload = preload;

#if !ENABLE(TIZEN_GSTREAMER_VIDEO)
    setDownloadBuffering();
#endif

    if (m_delayingLoad && m_preload != MediaPlayer::None) {
        m_delayingLoad = false;
        commitLoad();
    }
}

void MediaPlayerPrivateGStreamer::createAudioSink()
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (!m_playBin)
        return;

    GstElement* soundAlive = gst_element_factory_make("soundalive", 0);
    GstElement* sink = gst_element_factory_make("pulsesink", 0);
    g_object_set(sink, "close-handle-on-prepare", 1, NULL);
    g_object_set(sink, "slave-method", GST_AUDIO_BASE_SINK_SLAVE_NONE, NULL);
    gint latencyMode = 2; // Set latency to high instead of default low
    g_object_set(sink, "latency", latencyMode, NULL);

    m_webkitAudioSink = gst_bin_new("audio-sink");
    gst_bin_add_many(GST_BIN(m_webkitAudioSink.get()), soundAlive, sink, NULL);

    if (!gst_element_link_many(soundAlive, sink, NULL)) {
        TIZEN_LOGI("Failed to link audio elements");
        m_webkitAudioSink = 0;
        return;
    }

    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(soundAlive, "sink"));
    gst_element_add_pad(m_webkitAudioSink.get(), gst_ghost_pad_new("sink", pad.get()));

    g_object_set(m_playBin.get(), "audio-sink", m_webkitAudioSink.get(), NULL);
#else
    // Construct audio sink if pitch preserving is enabled.
    if (!m_preservesPitch)
        return;

    if (!m_playBin)
        return;

    GstElement* scale = gst_element_factory_make("scaletempo", 0);
    if (!scale) {
        GST_WARNING("Failed to create scaletempo");
        return;
    }

    GstElement* convert = gst_element_factory_make("audioconvert", 0);
    GstElement* resample = gst_element_factory_make("audioresample", 0);
    GstElement* sink = gst_element_factory_make("autoaudiosink", 0);

    GstElement* audioSink = gst_bin_new("audio-sink");
    gst_bin_add_many(GST_BIN(audioSink), scale, convert, resample, sink, NULL);

    if (!gst_element_link_many(scale, convert, resample, sink, NULL)) {
        GST_WARNING("Failed to link audio sink elements");
        gst_object_unref(audioSink);
        return;
    }

    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(scale, "sink"));
    gst_element_add_pad(audioSink, gst_ghost_pad_new("sink", pad.get()));

    g_object_set(m_playBin.get(), "audio-sink", audioSink, NULL);
#endif
}

#if !ENABLE(TIZEN_GSTREAMER_VIDEO) || !ENABLE(TIZEN_ACCELERATED_COMPOSITING) || !USE(TIZEN_TEXTURE_MAPPER) || !ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
//Callback used only in createVideoSink
static void mediaPlayerPrivateRepaintCallback(WebKitVideoSink*, GstBuffer *buffer, MediaPlayerPrivateGStreamer* playerPrivate)
{
	playerPrivate->triggerRepaint(buffer);
}
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
GstElement* MediaPlayerPrivateGStreamer::createVideoSink()
{
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    m_webkitVideoSink = m_videoLayer->createVideoSink();
    m_videoSinkPad = adoptGRef(gst_element_get_static_pad(m_webkitVideoSink, "sink"));

    // Don't fill empty area with borders
    g_object_set(m_webkitVideoSink, "draw-borders", false, NULL);

#else // ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#ifndef GST_API_VERSION_1
    m_webkitVideoSink = webkitVideoSinkNew(m_gstGWorld.get());
#else
    m_webkitVideoSink = webkitVideoSinkNew();
#endif
    m_videoSinkPad = adoptGRef(gst_element_get_static_pad(m_webkitVideoSink, "sink"));
    g_signal_connect(m_webkitVideoSink, "repaint-requested", G_CALLBACK(mediaPlayerPrivateRepaintCallback), this);
#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)

    return m_webkitVideoSink;
}

#else // ENABLE(TIZEN_GSTREAMER_VIDEO)
GstElement* MediaPlayerPrivateGStreamer::createVideoSink()
{
#ifndef GST_API_VERSION_1
    m_webkitVideoSink = webkitVideoSinkNew(m_gstGWorld.get());
#else
    m_webkitVideoSink = webkitVideoSinkNew();
#endif
    m_videoSinkPad = adoptGRef(gst_element_get_static_pad(m_webkitVideoSink, "sink"));
    g_signal_connect(m_webkitVideoSink, "repaint-requested", G_CALLBACK(mediaPlayerPrivateRepaintCallback), this);

#ifndef GST_API_VERSION_1
    m_videoSinkBin = gst_bin_new("video-sink");

    GstElement* videoTee = gst_element_factory_make("tee", "videoTee");
    GstElement* queue = gst_element_factory_make("queue", 0);

    // Take ownership.
    gst_object_ref_sink(m_videoSinkBin);

    // Build a new video sink consisting of a bin containing a tee
    // (meant to distribute data to multiple video sinks) and our
    // internal video sink. For fullscreen we create an autovideosink
    // and initially block the data flow towards it and configure it
    gst_bin_add_many(GST_BIN(m_videoSinkBin), videoTee, queue, NULL);
#endif
    GstElement* actualVideoSink = 0;
    m_fpsSink = gst_element_factory_make("fpsdisplaysink", "sink");
    if (m_fpsSink) {
        // The verbose property has been added in -bad 0.10.22. Making
        // this whole code depend on it because we don't want
        // fpsdiplaysink to spit data on stdout.
        GstElementFactory* factory = GST_ELEMENT_FACTORY(GST_ELEMENT_GET_CLASS(m_fpsSink)->elementfactory);
        if (gst_plugin_feature_check_version(GST_PLUGIN_FEATURE(factory), 0, 10, 22)) {
            g_object_set(m_fpsSink, "silent", TRUE , NULL);

            // Turn off text overlay unless logging is enabled.
#if LOG_DISABLED
            g_object_set(m_fpsSink, "text-overlay", FALSE , NULL);
#else
            WTFLogChannel* channel = getChannelFromName("Media");
            if (channel->state != WTFLogChannelOn)
                g_object_set(m_fpsSink, "text-overlay", FALSE , NULL);
#endif // LOG_DISABLED

            if (g_object_class_find_property(G_OBJECT_GET_CLASS(m_fpsSink), "video-sink")) {
                g_object_set(m_fpsSink, "video-sink", m_webkitVideoSink, NULL);
#ifndef GST_API_VERSION_1
                gst_bin_add(GST_BIN(m_videoSinkBin), m_fpsSink);
#endif
                actualVideoSink = m_fpsSink;
            } else
                m_fpsSink = 0;
        } else
            m_fpsSink = 0;
    }

    if (!m_fpsSink) {
#ifndef GST_API_VERSION_1
        gst_bin_add(GST_BIN(m_videoSinkBin), m_webkitVideoSink);
#endif
        actualVideoSink = m_webkitVideoSink;
    }

    ASSERT(actualVideoSink);

#ifndef GST_API_VERSION_1
    // Faster elements linking.
    gst_element_link_pads_full(queue, "src", actualVideoSink, "sink", GST_PAD_LINK_CHECK_NOTHING);

    // Add a ghostpad to the bin so it can proxy to tee.
    GRefPtr<GstPad> pad = adoptGRef(gst_element_get_static_pad(videoTee, "sink"));
    gst_element_add_pad(m_videoSinkBin, gst_ghost_pad_new("sink", pad.get()));

    // Set the bin as video sink of playbin.
    return m_videoSinkBin;
#else
    return actualVideoSink;
#endif
}
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

void MediaPlayerPrivateGStreamer::createGSTPlayBin()
{
    ASSERT(!m_playBin);

    // gst_element_factory_make() returns a floating reference so
    // we should not adopt.
    m_playBin = gst_element_factory_make(gPlaybinName, "play");

#ifndef GST_API_VERSION_1
    m_gstGWorld = GStreamerGWorld::createGWorld(m_playBin.get());
#endif

    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_playBin.get()));
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(mediaPlayerPrivateMessageCallback), this);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#ifdef GST_API_VERSION_1
    gst_bus_set_sync_handler(bus.get(), GstBusSyncHandler(mediaPlayerPrivateSyncHandler), this, NULL);
#else
    gst_bus_set_sync_handler(bus.get(), GstBusSyncHandler(mediaPlayerPrivateSyncHandler), this);
#endif
#endif

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    // Omits configuration of ffmpegcolorspace and videoscale
    unsigned int flags = GST_PLAY_FLAG_SOFT_VOLUME | GST_PLAY_FLAG_NATIVE_VIDEO | GST_PLAY_FLAG_AUDIO;
    g_object_set(m_playBin.get(), "flags", flags, NULL);
#endif

    g_object_set(m_playBin.get(), "mute", m_player->muted(), NULL);

    g_signal_connect(m_playBin.get(), "notify::volume", G_CALLBACK(mediaPlayerPrivateVolumeChangedCallback), this);
    g_signal_connect(m_playBin.get(), "notify::source", G_CALLBACK(mediaPlayerPrivateSourceChangedCallback), this);
    g_signal_connect(m_playBin.get(), "notify::mute", G_CALLBACK(mediaPlayerPrivateMuteChangedCallback), this);
    g_signal_connect(m_playBin.get(), "video-changed", G_CALLBACK(mediaPlayerPrivateVideoChangedCallback), this);
    g_signal_connect(m_playBin.get(), "audio-changed", G_CALLBACK(mediaPlayerPrivateAudioChangedCallback), this);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    g_signal_connect(m_playBin.get(), "source-setup", G_CALLBACK(mediaPlayerPrivateSourceSetupCallback), this);

    if (element()->isVideo()) {
        GstElement* videoSink = createVideoSink();
        g_object_set(m_playBin.get(), "video-sink", videoSink, NULL);

        GRefPtr<GstPad> videoSinkPad = adoptGRef(gst_element_get_static_pad(m_webkitVideoSink, "sink"));
        if (videoSinkPad)
            g_signal_connect(videoSinkPad.get(), "notify::caps", G_CALLBACK(mediaPlayerPrivateVideoSinkCapsChangedCallback), this);
    } else {
        GstElement* fakeSink = gst_element_factory_make("fakesink", 0);
        g_object_set(fakeSink, "sync", true, NULL);
        g_object_set(m_playBin.get(), "video-sink", fakeSink, NULL);
    }
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

    createAudioSink();
}

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)
bool MediaPlayerPrivateGStreamer::supportsAcceleratedRendering() const
{
    bool isEmbedVideo = false;
    if (m_player->mediaPlayerClient()) {
        Document* document = m_player->mediaPlayerClient()->mediaPlayerOwningDocument();
        if (document && document->settings())
            isEmbedVideo = document->settings()->acceleratedCompositingForVideoEnabled() && document->settings()->acceleratedCompositingEnabled();
    }

    return isEmbedVideo;
}

PlatformLayer* MediaPlayerPrivateGStreamer::platformLayer() const
{
    return m_videoLayer->platformLayer();
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
void MediaPlayerPrivateGStreamer::paintCurrentFrameInContext(GraphicsContext* context, const IntRect& rect)
{
    m_videoLayer->paintCurrentFrameInContext(context, rect);
}

void MediaPlayerPrivateGStreamer::xWindowIdPrepared(GstMessage* message)
{
    // It is called in streaming thread.
    // So it should be used only to set window handle to video sink.
    // And it is called just once after video src is set.
    // So video resolution change should be handled in notifyPlayerOfVideo().

    int width, height;

#ifdef GST_API_VERSION_1
    const GstStructure* structure = gst_message_get_structure(message);

    gst_structure_get_int(structure, "video-width", &width);
    gst_structure_get_int(structure, "video-height", &height);
#else
    gst_structure_get_int(message->structure, "video-width", &width);
    gst_structure_get_int(message->structure, "video-height", &height);
#endif

    TIZEN_LOGI("Width: %d, Height: %d", width, height);

#if ENABLE(TIZEN_MEDIA_STREAM)
    int rotate = DEGREE_0;
    if (isLocalMediaStream()) {
        rotate  = getCameraRotateDirection();
        m_videoSize = (rotate == DEGREE_90 || rotate == DEGREE_270) ? IntSize(height, width) : IntSize(width, height);
    } else
#endif
    m_videoSize = IntSize(width, height);

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (m_overlayType == MediaPlayer::HwOverlay) {
        m_updatePixmapSize = true;
        setXWindowHandle();
    } else
#endif
    setPixmap();
#if ENABLE(TIZEN_MEDIA_STREAM)
    if (isLocalMediaStream())
        setRotateProperty(rotate);
#endif
}

void MediaPlayerPrivateGStreamer::setPixmap()
{
    if (m_updatePixmapSize || !m_videoLayer->graphicsSurfaceToken()) {
        m_updatePixmapSize = false;
        m_videoLayer->setOverlay(m_videoSize);
    } else {
        m_videoLayer->setOverlay();
    }

#if ENABLE(TIZEN_MEDIA_STREAM)
    if (!isLocalMediaStream())
#endif
        setRotateProperty(DEGREE_0);
}
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)

#if ENABLE(TIZEN_WEBKIT2_PROXY)
void MediaPlayerPrivateGStreamer::setProxy(GstElement* source)
{
    if (!url().protocol().startsWith("http", false))
        return;

    SoupSession* session = WebCore::ResourceHandle::defaultSession();
    if (!session)
        return;

    SoupURI* proxyUri = 0;
    g_object_get(session, SOUP_SESSION_PROXY_URI, &proxyUri, NULL);
    if (!proxyUri)
        return;

    char* proxy = soup_uri_to_string(proxyUri, false);
    g_object_set(source, "proxy", proxy, NULL);

    soup_uri_free(proxyUri);
    g_free(proxy);
}
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void MediaPlayerPrivateGStreamer::setCookie(GstElement* source)
{
    if (!url().protocol().startsWith("http", false))
        return;

    SoupCookieJar* jar = soupCookieJar();
    if (!jar)
        return;

    GSList* allCookies = soup_cookie_jar_all_cookies(jar);
    if (!allCookies)
        return;

    GSList* cookies = 0;
    GOwnPtr<SoupURI> uri(soup_uri_new(url().string().utf8().data()));
    for (GSList* iter = allCookies; iter; iter = g_slist_next(iter)) {
        SoupCookie* cookie = static_cast<SoupCookie*>(iter->data);
        if (!soup_cookie_applies_to_uri(cookie, uri.get()) || !soup_cookie_domain_matches(cookie, uri.get()->host))
            continue;

        cookies = g_slist_prepend(cookies, soup_cookie_copy(cookie));
    }
    g_object_set(source, "soupcookies", cookies, NULL);

    if (allCookies)
        soup_cookies_free(allCookies);

    if (cookies)
        soup_cookies_free(cookies);

    return;
}

bool MediaPlayerPrivateGStreamer::isFullScreen()
{
    if (element() && element()->isFullscreen())
        return true;

    return false;
}

#ifdef GST_API_VERSION_1
bool MediaPlayerPrivateGStreamer::extractTagImage(GstMessage* message)
{
    GstTagList* tagList = 0;
    GstBuffer* buffer = 0;
    GstSample* sample = 0;
    gchar* codecType = 0;
    gst_message_parse_tag(message, &tagList);
    if (!tagList)
        return false;

    gst_tag_list_get_sample(tagList, GST_TAG_IMAGE, &sample);

    if (!sample) {
        return false;
    }

    buffer = gst_sample_get_buffer(sample);

    GstMapInfo map;
    if(!buffer || !gst_buffer_map(buffer, &map, GST_MAP_READ) ||
       !map.data || map.size <= 0) {
        freeTagImageResource(tagList, sample, buffer, codecType, &map);
        return false;
    }

    gst_tag_list_get_string(tagList, GST_TAG_CODEC, &codecType);
    if (!codecType || !String(codecType).startsWith("image/", false)) {
        freeTagImageResource(tagList, sample, buffer, codecType, &map);
        return false;
    }

    String mimeType = codecType;
    String path = homeDirectoryPath() + String::format("/%f.", WTF::currentTime()) + mimeType.substring(mimeType.find('/') + 1);
    PlatformFileHandle fileHandle = openFile(path, OpenForWrite);
    if (fileHandle == invalidPlatformFileHandle) {
        freeTagImageResource(tagList, sample, buffer, codecType, &map);
        return false;
    }
    writeToFile(fileHandle, (const char*)map.data, map.size);
    closeFile(fileHandle);

    if (fileExists(m_tagImagePath))
        deleteFile(m_tagImagePath);
    m_tagImagePath = path;


    freeTagImageResource(tagList, sample, buffer, codecType, &map);
    return true;
}

void MediaPlayerPrivateGStreamer::freeTagImageResource(GstTagList*& tagList,
                                                       GstSample*& sample,
                                                       GstBuffer*& buffer,
                                                       gchar*& codecType,
                                                       GstMapInfo *map)
{
    if (tagList) {
        gst_tag_list_free(tagList);
        tagList = 0;
    }

    if (map) {
        gst_buffer_unmap(buffer, map);
    }

    if (buffer) {
        gst_buffer_unref(buffer);
        buffer = 0;
    }

    if (sample) {
        gst_sample_unref(sample);
        sample = 0;
    }

    if (codecType) {
        g_free(codecType);
        codecType = 0;
    }

    return;
}


#else

bool MediaPlayerPrivateGStreamer::extractTagImage(GstMessage* message)
{
    GstTagList* tagList = 0;
    GstBuffer* buffer = 0;
    gchar* codecType = 0;
    gst_message_parse_tag(message, &tagList);
    if (!tagList)
        return false;

    gst_tag_list_get_buffer(tagList, GST_TAG_IMAGE, &buffer);

    if (!buffer || !buffer->data || buffer->size <= 0) {
        freeTagImageResource(tagList, buffer, codecType);
        return false;
    }

    gst_tag_list_get_string(tagList, GST_TAG_CODEC, &codecType);
    if (!codecType || !String(codecType).startsWith("image/", false)) {
        freeTagImageResource(tagList, buffer, codecType);
        return false;
    }

    String mimeType = codecType;
    String path = homeDirectoryPath() + String::format("/%f.", WTF::currentTime()) + mimeType.substring(mimeType.find('/') + 1);
    PlatformFileHandle fileHandle = openFile(path, OpenForWrite);
    if (fileHandle == invalidPlatformFileHandle) {
        freeTagImageResource(tagList, buffer, codecType);
        return false;
    }

    writeToFile(fileHandle, (const char*)buffer->data, buffer->size);
    closeFile(fileHandle);

    if (fileExists(m_tagImagePath))
        deleteFile(m_tagImagePath);
    m_tagImagePath = path;

    freeTagImageResource(tagList, buffer, codecType);
    return true;
}

void MediaPlayerPrivateGStreamer::freeTagImageResource(GstTagList*& tagList, GstBuffer*& buffer, gchar*& codecType)
{
    if (tagList) {
        gst_tag_list_free(tagList);
        tagList = 0;
    }

    if (buffer) {
        gst_buffer_unref(buffer);
        buffer = 0;
    }

    if (codecType) {
        g_free(codecType);
        codecType = 0;
    }

    return;
}

#endif



bool MediaPlayerPrivateGStreamer::isReleased()
{
    return m_releasedTime != numeric_limits<float>::infinity();
}

bool MediaPlayerPrivateGStreamer::isDestroyed(MediaPlayerPrivateGStreamer* player)
{
    if (!m_playerList.contains(player))
        return true;

    return false;
}

bool MediaPlayerPrivateGStreamer::downloadBufferingEnabled()
{
    GstPlayFlags flags;
    g_object_get(m_playBin.get(), "flags", &flags, NULL);

    return flags & GST_PLAY_FLAG_DOWNLOAD ? true : false;
}

void MediaPlayerPrivateGStreamer::initializeQueue2()
{
    if (!m_queue2)
        return;

    g_object_set(m_queue2, "use-rate-estimate", false, NULL);
    g_object_set(m_queue2, "max-size-bytes", bufferMaxSizeBytes(), NULL);

    if (downloadBufferingEnabled()) {
        g_object_set(m_queue2, "temp-template", TEMP_FILE_TEMPLATE, NULL);
        g_object_set(m_queue2, "file-buffer-max-size", (guint64)bufferMaxSizeBytes(), NULL);
    }

    // Listen buffering message from 'queue2' in case of audio element.
    if (!element()->isVideo()) {
        g_object_set(m_queue2, "low-percent", bufferLowPercent(), NULL);
        g_object_set(m_queue2, "high-percent", bufferHighPercent(), NULL);
        g_object_set(m_queue2, "use-buffering", true, NULL);
    }
#ifndef GST_API_VERSION_1
    else {
        g_object_set(m_queue2, "use-buffering", false, NULL);
    }
#endif
}

void MediaPlayerPrivateGStreamer::finishQueue2Config()
{
    // Delay config if any media stream is not found.
    if (!m_queue2 || (!m_hasVideo && !m_hasAudio) || m_queue2ConfigFinished)
        return;

    m_queue2ConfigFinished = true;

    // Reconfigure after getting video resolution
    // FIX ME : Changing queue size after initialization can cause read error.
    g_object_set(m_queue2, "max-size-bytes", bufferMaxSizeBytes(), NULL);
    if (downloadBufferingEnabled())
        g_object_set(m_queue2, "file-buffer-max-size", (guint64)bufferMaxSizeBytes(), NULL);

    if (!element()->isVideo()) {
        g_object_set(m_queue2, "low-percent", bufferLowPercent(), NULL);
        g_object_set(m_queue2, "high-percent", bufferHighPercent(), NULL);
    }
}

void MediaPlayerPrivateGStreamer::initializeDownloadBuffer()
{
    if (!m_downloadBuffer)
        return;


    if (downloadBufferingEnabled()) {
        g_object_set(m_downloadBuffer, "temp-template", TEMP_FILE_TEMPLATE, NULL);
    }

// Do not control property of child element in playbin directly.
// It may cause of infinite buffering issue by broken sync of each element.
#ifndef GST_API_VERSION_1
    g_object_set(m_downloadBuffer, "max-size-bytes", bufferMaxSizeBytes(), NULL);
    g_object_set(m_downloadBuffer, "low-percent", bufferLowPercent(), NULL);
    g_object_set(m_downloadBuffer, "high-percent", bufferHighPercent(), NULL);
#endif
}

void MediaPlayerPrivateGStreamer::finishDownloadBufferConfig()
{
    // Delay config if any media stream is not found.
    if (!m_downloadBuffer || (!m_hasVideo && !m_hasAudio) || m_downloadBufferConfigFinished)
        return;

    m_downloadBufferConfigFinished = true;
    g_object_set(m_downloadBuffer, "max-size-bytes", bufferMaxSizeBytes(), NULL);
}

void MediaPlayerPrivateGStreamer::initializeDecodebin()
{
    if (!m_decodebin)
        return;

    g_object_set(m_decodebin, "max-size-bytes", (guint)MULTI_QUEUE_MAX_SIZE_BYTE, NULL);
    g_object_set(m_decodebin, "max-size-time", (guint64)MULTI_QUEUE_MAX_SIZE_TIME, NULL);

    // we don't want to 'decodebin' manage 'multiqueue' regarding buffering.
    g_object_set(m_decodebin, "use-buffering", false, NULL);
}

void MediaPlayerPrivateGStreamer::initializeMultiQueue()
{
    if (!m_multiQueue)
        return;

#ifndef GST_API_VERSION_1
    g_object_set(m_multiQueue, "low-percent", MULTI_QUEUE_LOW_PERCENT, NULL);
    g_object_set(m_multiQueue, "high-percent", MULTI_QUEUE_HIGH_PERCENT, NULL);

    // In case of download enabled, download buffer will manage the buffering state.
    if (downloadBufferingEnabled() || !element()->isVideo())
        g_object_set(m_multiQueue, "use-buffering", false, NULL);
    else
#endif
    g_object_set(m_multiQueue, "use-buffering", true, NULL);
}

void MediaPlayerPrivateGStreamer::finishMultiQueueConfig()
{
    // Delay config if video stream is not found.
    if (!m_multiQueue || !m_hasVideo || m_multiQueueConfigFinished)
        return;

    m_multiQueueConfigFinished = true;

    if (m_videoSize.height() >= gHdVideoHeight)
        g_object_set(m_multiQueue, "high-percent", MULTI_QUEUE_HD_HIGH_PERCENT, NULL);
}

guint MediaPlayerPrivateGStreamer::bufferMaxSizeBytes()
{
    if (!downloadBufferingEnabled()) {
#if ENABLE(TIZEN_LITE)
        return MEGA_TO_BYTE(3);
#else
        return MEGA_TO_BYTE(5);
#endif
    }

    return DOWNLOAD_BUFFER_INFO[
            element()->isVideo()
            ? m_videoSize.height() >= gHdVideoHeight ? VIDEO_BUFFER_SIZE_HD : VIDEO_BUFFER_SIZE
            : AUDIO_BUFFER_SIZE];
}

gint MediaPlayerPrivateGStreamer::bufferLowPercent()
{
    return DOWNLOAD_BUFFER_INFO[
            element()->isVideo()
            ? m_videoSize.height() >= gHdVideoHeight ? VIDEO_LOW_PERCENT_HD : VIDEO_LOW_PERCENT
            : AUDIO_LOW_PERCENT];
}

gint MediaPlayerPrivateGStreamer::bufferHighPercent()
{
    return DOWNLOAD_BUFFER_INFO[
            element()->isVideo()
            ? m_videoSize.height() >= gHdVideoHeight ? VIDEO_HIGH_PERCENT_HD : VIDEO_HIGH_PERCENT
            : AUDIO_HIGH_PERCENT];
}

void MediaPlayerPrivateGStreamer::setDrawable()
{
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (isFullScreen()) {
        setXWindowHandle();
        return;
    }
#endif
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    setPixmap();
#endif
}

#ifndef GST_API_VERSION_1
static void mediaPlayerPrivatePadBlockCallback(GstPad *pad, gboolean blocked, gpointer user_data)
{
    // Registered callback to block decodebin2 asynchronously.
    // It will called when a pad is blocked or unblocked.
    // Nothing is needed here.

    return;
}
#endif

void MediaPlayerPrivateGStreamer::setQueue2Blocked(bool blocked)
{
#ifndef GST_API_VERSION_1
    // FIXME :
    // GStreamer 1.x has deadlock issue during query operation.

    if (!m_queue2)
        return;

    // To block 'queue2', we need to block peer of sink pad of 'queue2'.
    GRefPtr<GstPad> sinkPad = adoptGRef(gst_element_get_static_pad(m_queue2, "sink"));
    GRefPtr<GstPad> srcPad = gst_pad_get_peer(sinkPad.get());
    if (srcPad && gst_pad_is_linked(srcPad.get())) {
#ifdef GST_API_VERSION_1
        if (m_queue2_probe_id)
            gst_pad_remove_probe(srcPad.get(), m_queue2_probe_id);

        m_queue2_probe_id = blocked ? gst_pad_add_probe(srcPad.get(), GST_PAD_PROBE_TYPE_BLOCK, NULL, NULL, NULL) : 0;
#else
        gst_pad_set_blocked_async(srcPad.get(), blocked, mediaPlayerPrivatePadBlockCallback, this);
#endif
    }
#endif
}

void MediaPlayerPrivateGStreamer::setDownloadBufferBlocked(bool blocked)
{
#ifndef GST_API_VERSION_1
    // FIXME :
    // GStreamer 1.x has deadlock issue during query operation.

    if (!m_downloadBuffer)
        return;

    // To block 'm_downloadBuffer', we need to block peer of sink pad of 'm_downloadBuffer'.
    GRefPtr<GstPad> sinkPad = adoptGRef(gst_element_get_static_pad(m_downloadBuffer, "sink"));
    GRefPtr<GstPad> srcPad = gst_pad_get_peer(sinkPad.get());
    if (srcPad && gst_pad_is_linked(srcPad.get())) {
#ifdef GST_API_VERSION_1
        if (m_downloadBuffer_probe_id)
            gst_pad_remove_probe(srcPad.get(), m_downloadBuffer_probe_id);

        m_downloadBuffer_probe_id = blocked ? gst_pad_add_probe(srcPad.get(), GST_PAD_PROBE_TYPE_BLOCK, NULL, NULL, NULL) : 0;
#else
        gst_pad_set_blocked_async(srcPad.get(), blocked, mediaPlayerPrivatePadBlockCallback, this);
#endif

    }
#endif
}

void MediaPlayerPrivateGStreamer::setDecodebinBlocked(bool blocked)
{
#ifndef GST_API_VERSION_1
    // FIXME :
    // GStreamer 1.x has deadlock issue during query operation.

    if (!m_decodebin)
        return;

    GRefPtr<GstPad> sinkPad = adoptGRef(gst_element_get_static_pad(m_decodebin, "sink"));
    if (sinkPad && gst_pad_is_linked(sinkPad.get())) {
#ifdef GST_API_VERSION_1
        if (m_decodebin_probe_id)
            gst_pad_remove_probe(sinkPad.get(), m_decodebin_probe_id);

        m_decodebin_probe_id = blocked ? gst_pad_add_probe(sinkPad.get(), GST_PAD_PROBE_TYPE_BLOCK, NULL, NULL, NULL) : 0;
#else
        gst_pad_set_blocked_async(sinkPad.get(), blocked, mediaPlayerPrivatePadBlockCallback, this);
#endif
    }
#endif
}

void MediaPlayerPrivateGStreamer::willStartPlay()
{
    if (!m_havePlayed)
        m_havePlayed = true;

    finishQueue2Config();
    finishDownloadBufferConfig();
    finishMultiQueueConfig();

    setQueue2Blocked(false);
    setDownloadBufferBlocked(false);
    setDecodebinBlocked(false);
}
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
void MediaPlayerPrivateGStreamer::changeOverlayType(MediaPlayer::OverlayType type)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("Overlay Type: %d", type);
#endif

    if (m_overlayType == type)
        return;
    m_overlayType = type;

    if (type == MediaPlayer::HwOverlay)
        setXWindowHandle();
    else {
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
        setPixmap();
#endif
    }
}

void MediaPlayerPrivateGStreamer::rotateHwOverlayVideo()
{
    setRotateForHwOverlayVideo();
}

void MediaPlayerPrivateGStreamer::setXWindowHandle()
{
    if (!m_webkitVideoSink || m_overlayType != MediaPlayer::HwOverlay)
        return;

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
    xWindow = getXWindow();
#endif

    if (!xWindow) {
        TIZEN_LOGE("Fail to get XWindow handle");
        return;
    }

#ifdef GST_API_VERSION_1
    g_object_set(GST_VIDEO_OVERLAY(m_webkitVideoSink), "pixmap-id-callback", &getXWindowIDCallback, NULL);
#else
    gst_x_overlay_set_window_handle(GST_X_OVERLAY(m_webkitVideoSink), xWindow);
#endif

    setRotateForHwOverlayVideo();
}

void MediaPlayerPrivateGStreamer::setRotateForHwOverlayVideo()
{
    if (!m_webkitVideoSink || m_overlayType != MediaPlayer::HwOverlay)
        return;

    int orientation = 0;
    if (!getFrameOrientation(&orientation))
      return;

    int rotate = 0;
    switch (orientation) {
    case 0:
        rotate = DEGREE_0;
        break;
    case 90:
        rotate = DEGREE_270;
        break;
    case 180:
        rotate = DEGREE_180;
        break;
    case -90:
        rotate = DEGREE_90;
        break;
    default:
        TIZEN_LOGE("Unsupported orientation value : [%d]", orientation);
        break;
    }

    setRotateProperty(rotate);
}
#endif // ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)

#if ENABLE(TIZEN_MEDIA_STREAM)
void MediaPlayerPrivateGStreamer::orientationStateChanged()
{
    int rotate = getCameraRotateDirection();
    if (rotate == m_rotation)
        return;

    if (!((rotate == DEGREE_180 && m_rotation == DEGREE_0) || (rotate == DEGREE_0 && m_rotation == DEGREE_180)))
        m_videoSize = IntSize(m_videoSize.height(), m_videoSize.width());

    m_rotation = rotate;

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
    m_updatePixmapSize = true;
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
    if (m_overlayType == MediaPlayer::Pixmap)
#endif
    {
        m_videoLayer->clearSurface();
        setPixmap();
        // for rear camera applying 270 degree rotate to avoid inverted image in case of "camerasrc" gstreamer plugin.
        if (m_cameraId == 0 && m_rotation == 1)
            setRotateProperty(DEGREE_270);
        else if (m_cameraId == 1 && m_rotation == 2)
            setRotateProperty(DEGREE_0);
         else
             setRotateProperty(m_rotation);
    }

    m_player->sizeChanged();
#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
}

int MediaPlayerPrivateGStreamer::getCameraRotateDirection()
{
    int orientation = 0;
    int rotate = DEGREE_0;

    if (!getFrameOrientation(&orientation))
      return rotate;

    switch (orientation) {
    case 0:
        rotate = DEGREE_90;
        break;
    case 90:
        rotate = DEGREE_0;
        break;
    case 180:
        rotate = DEGREE_270;
        break;
    case -90:
        rotate = DEGREE_180;
        break;
    default:
        TIZEN_LOGE("Unsupported orientation value : [%d]", orientation);
        break;
    }

    return rotate;
}
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(TIZEN_MEDIA_STREAM) || ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
bool MediaPlayerPrivateGStreamer::getFrameOrientation(int* orientation)
{
#if ENABLE(ORIENTATION_EVENTS) && ENABLE(TIZEN_ORIENTATION_EVENTS)
    Document* document = element()->document();
    if (!document)
        document = element()->ownerDocument();

    Frame* frame = document ? document->frame() : 0;
    if (!frame)
        return false;

    *orientation = frame->orientation();
#else
    *orientation = 0;
#endif

     TIZEN_LOGI("Orientation : [%d]", *orientation);
     return true;
}

void MediaPlayerPrivateGStreamer::setRotateProperty(int rotation)
{
    TIZEN_LOGI("Rotation : [%d]", rotation);

    g_object_set(m_webkitVideoSink, "rotate", rotation, NULL);
}
#endif // ENABLE(TIZEN_MEDIA_STREAM) || ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)

}

#endif // USE(GSTREAMER)
