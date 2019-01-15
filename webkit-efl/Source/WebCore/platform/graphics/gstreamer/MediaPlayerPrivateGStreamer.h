/*
 * Copyright (C) 2007, 2009 Apple Inc.  All rights reserved.
 * Copyright (C) 2007 Collabora Ltd. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
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

#ifndef MediaPlayerPrivateGStreamer_h
#define MediaPlayerPrivateGStreamer_h
#if ENABLE(VIDEO) && USE(GSTREAMER)

#include "GRefPtrGStreamer.h"
#include "MediaPlayerPrivate.h"
#include "Timer.h"

#include <glib.h>
#include <gst/gst.h>
#include <wtf/Forward.h>

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#include "HTMLMediaElement.h"

#include <wtf/Vector.h>
#endif

typedef struct _WebKitVideoSink WebKitVideoSink;
typedef struct _GstBuffer GstBuffer;
typedef struct _GstMessage GstMessage;
typedef struct _GstElement GstElement;

namespace WebCore {

class GraphicsContext;
class IntSize;
class IntRect;
class GStreamerGWorld;
class MediaPlayerPrivateGStreamer;
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)
class VideoLayerTizen;
#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)

class MediaPlayerPrivateGStreamer : public MediaPlayerPrivateInterface
{

        public:
            ~MediaPlayerPrivateGStreamer();
            static void registerMediaEngine(MediaEngineRegistrar);
            gboolean handleMessage(GstMessage*);

            IntSize naturalSize() const;
            bool hasVideo() const { return m_hasVideo; }
            bool hasAudio() const { return m_hasAudio; }

            void load(const String &url);
            void commitLoad();
            void cancelLoad();

            void prepareToPlay();
            void play();
            void pause();

            bool paused() const;
            bool seeking() const;

            float duration() const;
            float currentTime() const;
            void seek(float);

            void setRate(float);
            void setPreservesPitch(bool);

            void setVolume(float);
            void volumeChanged();
            void notifyPlayerOfVolumeChange();

            bool supportsMuting() const;
            void setMuted(bool);
            void muteChanged();
            void notifyPlayerOfMute();

            void setPreload(MediaPlayer::Preload);
            void fillTimerFired(Timer<MediaPlayerPrivateGStreamer>*);

            MediaPlayer::NetworkState networkState() const;
            MediaPlayer::ReadyState readyState() const;

            PassRefPtr<TimeRanges> buffered() const;
            float maxTimeSeekable() const;
            bool didLoadingProgress() const;
            unsigned totalBytes() const;

            void setVisible(bool);
            void setSize(const IntSize&);

            void loadStateChanged();
            void sizeChanged();
            void timeChanged();
            void didEnd();
            void durationChanged();
            void loadingFailed(MediaPlayer::NetworkState);
            void triggerRepaint(GstBuffer*);
            void repaint();
            void paint(GraphicsContext*, const IntRect&);

            bool hasSingleSecurityOrigin() const;

            bool supportsFullscreen() const;
            PlatformMedia platformMedia() const;

            void videoChanged();
            void audioChanged();
            void notifyPlayerOfVideo();
            void notifyPlayerOfAudio();

            void sourceChanged();

            unsigned decodedFrameCount() const;
            unsigned droppedFrameCount() const;
            unsigned audioDecodedByteCount() const;
            unsigned videoDecodedByteCount() const;

            MediaPlayer::MovieLoadType movieLoadType() const;

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)
            // whether accelerated rendering is supported by the media engine for the current media.
            virtual bool supportsAcceleratedRendering() const;
            // called when the rendering system flips the into or out of accelerated rendering mode.
            virtual void acceleratedRenderingStateChanged() { }
            // Const-casting here is safe, since all of TextureMapperPlatformLayer's functions are const.g
            virtual PlatformLayer* platformLayer() const;

#if ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
            void xWindowIdPrepared(GstMessage*);
            virtual void paintCurrentFrameInContext(GraphicsContext*, const IntRect&);
#endif // ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
#endif // ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)

#if ENABLE(TIZEN_WEBKIT2_PROXY)
            void setProxy(GstElement* source);
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
            MediaPlayer* player() { return m_player; }
            HTMLMediaElement* element() { return static_cast<HTMLMediaElement*>(player()->mediaPlayerClient()); }
            GstElement* playBin() { return m_playBin.get(); }

            KURL url() { return m_url; }
            void setCookie(GstElement* source);

            bool isBuffering() { return m_buffering; }
            void release();
            bool isReleased();
            float releasedTime() { return m_releasedTime; }
            void setReleasedTime(float time) { m_releasedTime = time; }
            static bool isDestroyed(MediaPlayerPrivateGStreamer*);

            void setDrawable();
            GstElement* videoSink() { return m_webkitVideoSink; }

            bool downloadBufferingEnabled();
            void setQueue2ConfigFinished(bool finished) { m_queue2ConfigFinished = finished; }
            void setDownloadBufferConfigFinished(bool finished) { m_downloadBufferConfigFinished = finished; }
            void setMultiQueueConfigFinished(bool finished) { m_multiQueueConfigFinished = finished; }

            GstElement* queue2() { return m_queue2; }
            void setQueue2(GstElement* queue2) { m_queue2 = queue2; }
            void initializeQueue2();

            GstElement* downloadBuffer() { return m_downloadBuffer; }
            void setDownloadBuffer(GstElement* downloadBuffer) { m_downloadBuffer = downloadBuffer; }
            void initializeDownloadBuffer();

            GstElement* multiQueue() { return m_multiQueue; }
            void setMultiQueue(GstElement* multiQueue) { m_multiQueue = multiQueue; }
            void initializeMultiQueue();

            void setDecodebin(GstElement* decodebin) { m_decodebin = decodebin; }
            void initializeDecodebin();
            void willStartPlay();
            void pendingSeek();
#endif
#if ENABLE(TIZEN_MEDIA_STREAM)
            bool isLocalMediaStream() { return m_url.string().contains("camera://") ? true : false; }
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
            MediaPlayer::OverlayType overlayType() { return m_overlayType; }
            virtual void changeOverlayType(MediaPlayer::OverlayType);
            virtual void rotateHwOverlayVideo();
#endif
        private:
            MediaPlayerPrivateGStreamer(MediaPlayer*);

            static PassOwnPtr<MediaPlayerPrivateInterface> create(MediaPlayer*);

            static void getSupportedTypes(HashSet<String>&);
            static MediaPlayer::SupportsType supportsType(const String& type, const String& codecs, const KURL&);

            static bool isAvailable();

            void updateAudioSink();
            void createAudioSink();

            float playbackPosition() const;

            void cacheDuration();
            void updateStates();
            float maxTimeLoaded() const;

            void createGSTPlayBin();
            bool changePipelineState(GstState state);

            bool loadNextLocation();
            void mediaLocationChanged(GstMessage*);

            void setDownloadBuffering();
            void processBufferingStats(GstMessage*);

            virtual String engineDescription() const { return "GStreamer"; }
            bool isLiveStream() const { return m_isStreaming; }

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
            GstElement* createVideoSink();
            bool isFullScreen();

            bool extractTagImage(GstMessage*);
#ifdef GST_API_VERSION_1
            void freeTagImageResource(GstTagList*&,
                                      GstSample*&,
                                      GstBuffer*&,
                                      gchar*&,
                                      GstMapInfo*);
#else
            void freeTagImageResource(GstTagList*&, GstBuffer*&, gchar*&);
#endif

            guint bufferMaxSizeBytes();
            gint bufferLowPercent();
            gint bufferHighPercent();

            void finishQueue2Config();
            void finishDownloadBufferConfig();
            void finishMultiQueueConfig();

            void setDownloadBufferBlocked(bool blocked);
            void setQueue2Blocked(bool blocked);
            void setDecodebinBlocked(bool blocked);
#endif

#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
            void setXWindowHandle();
            void setRotateForHwOverlayVideo();
#endif
#if ENABLE(TIZEN_MEDIA_STREAM)
            int getCameraRotateDirection();
            void orientationStateChanged();
#endif
#if ENABLE(TIZEN_MEDIA_STREAM) || ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
            void setRotateProperty(int rotation);
            bool getFrameOrientation(int * orientation);
#endif
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
            void setPixmap();
#endif
        private:
            MediaPlayer* m_player;
            GRefPtr<GstElement> m_playBin;
            GstElement* m_webkitVideoSink;
            GstElement* m_videoSinkBin;
            GstElement* m_fpsSink;
            GRefPtr<GstElement> m_source;
            float m_seekTime;
            bool m_changingRate;
            float m_endTime;
            bool m_isEndReached;
            MediaPlayer::NetworkState m_networkState;
            MediaPlayer::ReadyState m_readyState;
            mutable bool m_isStreaming;
            IntSize m_size;
            GstBuffer* m_buffer;
            GstStructure* m_mediaLocations;
            int m_mediaLocationCurrentIndex;
            bool m_resetPipeline;
            bool m_paused;
            bool m_seeking;
            bool m_buffering;
#if ENABLE(TIZEN_MEDIA_STREAM)
            int m_rotation;
            int m_cameraId;
#endif
            float m_playbackRate;
            bool m_errorOccured;
            gfloat m_mediaDuration;
            bool m_startedBuffering;
            Timer<MediaPlayerPrivateGStreamer> m_fillTimer;
            float m_maxTimeLoaded;
            int m_bufferingPercentage;

            MediaPlayer::Preload m_preload;
            bool m_delayingLoad;
            mutable float m_maxTimeLoadedAtLastDidLoadingProgress;
#ifndef GST_API_VERSION_1
            RefPtr<GStreamerGWorld> m_gstGWorld;
#endif
            guint m_volumeTimerHandler;
            guint m_muteTimerHandler;
            bool m_volumeAndMuteInitialized;
            bool m_hasVideo;
            bool m_hasAudio;
            guint m_audioTimerHandler;
            guint m_videoTimerHandler;
            GRefPtr<GstElement> m_webkitAudioSink;
            mutable long m_totalBytes;
            GRefPtr<GstPad> m_videoSinkPad;
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER)
            OwnPtr<VideoLayerTizen> m_videoLayer;
#endif
            bool m_mediaDurationKnown;
            mutable IntSize m_videoSize;
            KURL m_url;
            bool m_preservesPitch;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
            mutable float m_fallbackCurrentTime;
            float m_releasedTime;
            bool m_shouldDelayErrorEvent;
            String m_tagImagePath;
            bool m_pendingSeek;
            float m_pendingSeekTime;

            // To check whether a player is destroyed when dispatch function of runloop is called.
            static Vector<MediaPlayerPrivateInterface*> m_playerList;

            bool m_havePlayed;
            bool m_queue2ConfigFinished;
            bool m_downloadBufferConfigFinished;
            bool m_multiQueueConfigFinished;

            // Elements needed to set buffering property
            GstElement* m_queue2;
            GstElement* m_downloadBuffer;
            GstElement* m_multiQueue;
            GstElement* m_decodebin;

#ifdef GST_API_VERSION_1
            gulong m_queue2_probe_id;
            gulong m_downloadBuffer_probe_id;
            gulong m_decodebin_probe_id;
#endif
#endif
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
            MediaPlayer::OverlayType m_overlayType;
#endif
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING) && USE(TIZEN_TEXTURE_MAPPER) && ENABLE(TIZEN_WEBKIT2_TILED_AC_SHARED_PLATFORM_SURFACE)
            bool m_updatePixmapSize;
#endif
    };
}

#endif // USE(GSTREAMER)
#endif
