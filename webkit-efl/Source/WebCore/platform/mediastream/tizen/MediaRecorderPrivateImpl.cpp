/*
    Copyright (C) 2012 Samsung Electronics.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "MediaRecorderPrivateImpl.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "FileSystem.h"
#include "MediaRecorder.h"
#include "MediaStreamComponent.h"
#include "MediaStreamDescriptor.h"
#include "MediaStreamSource.h"
#include "WebKitCameraSourceGStreamerTizen.h"
#include <wtf/text/WTFString.h>

#define ACAPS "audio/x-raw-int, endianness=(int)1234, signed=(boolean)true, width=(int)16, depth=(int)16, rate=(int)44100, channels=(int)2"

namespace WebCore {

PassOwnPtr<MediaRecorderPrivateInterface> MediaRecorderPrivate::create(MediaRecorder* recorder)
{
    return adoptPtr(new MediaRecorderPrivate(recorder));
}

MediaRecorderPrivate::MediaRecorderPrivate(MediaRecorder* recorder)
    : m_recorder(recorder)
    , m_pipeline(0)
    , m_recordMode(RECORD_MODE_NONE)
{
    gst_init(0, 0);
    gst_element_register(0, "webkitcamerasrc", GST_RANK_PRIMARY + 200, WEBKIT_TYPE_CAMERA_SRC);
}

MediaRecorderPrivate::~MediaRecorderPrivate()
{
    deletePipeline();
}

void MediaRecorderPrivate::createPipeline()
{
    if (m_pipeline || m_recordMode == RECORD_MODE_NONE || m_recordMode == RECORD_MODE_INVALID)
        return;

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("");
#endif

    m_pipeline = gst_pipeline_new(0);
    m_bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));

    if (m_recordMode & RECORD_MODE_AUDIO_ONLY) {
        m_asrc = gst_element_factory_make("pulsesrc", 0);
        m_aenc = gst_element_factory_make("savsenc_amrnb", 0);
        m_aqueue = gst_element_factory_make("queue", 0);
    }

    if (m_recordMode & RECORD_MODE_VIDEO_ONLY) {
        m_vsrc = gst_element_factory_make("webkitcamerasrc", 0);
        m_venc = gst_element_factory_make("ffenc_h263p", 0);
        m_vqueue = gst_element_factory_make("queue", 0);
    }

    if (m_recordMode & RECORD_MODE_AUDIO_ONLY)
        m_mux = gst_element_factory_make("avmux_amr", 0);
    else
        m_mux = gst_element_factory_make("ffmux_3gp", 0);

    m_fsink = gst_element_factory_make("filesink", 0);

    if (m_recordMode & RECORD_MODE_VIDEO_ONLY) {
        String cameraUrl = String::format("camera://%d", m_cameraId);
        g_object_set(G_OBJECT(m_vsrc), "location", cameraUrl.utf8().data(), NULL);
        g_object_set(G_OBJECT(m_vsrc), "is-recording", TRUE, NULL);
    }

    m_recordedFileName = "/opt/usr/media/";
    if (m_recordMode == RECORD_MODE_AUDIO_ONLY)
        m_recordedFileName.append("Sounds/");
    else
        m_recordedFileName.append("Vidoes/");

    if (!m_prefix.isEmpty()) {
        m_recordedFileName.append(m_prefix);
        m_recordedFileName.append("-");
    }

    GTimeZone* timeZone = g_time_zone_new_local();
    GDateTime* dateTime = g_date_time_new_now(timeZone);
    gchar* defaultName;

    if (m_recordMode & RECORD_MODE_AUDIO_ONLY)
        defaultName = g_date_time_format(dateTime, "%s.amr");
    else
        defaultName = g_date_time_format(dateTime, "%s.3gp");

    m_recordedFileName.append(defaultName);

    g_object_set(G_OBJECT(m_fsink), "location", m_recordedFileName.utf8().data(), NULL);

    g_time_zone_unref(timeZone);
    g_date_time_unref(dateTime);
    g_free(defaultName);

    if (m_recordMode == RECORD_MODE_AUDIO_ONLY) {
        gst_bin_add_many(GST_BIN(m_pipeline), m_asrc, m_aenc, m_aqueue, m_mux, m_fsink, NULL);
        gst_element_link_many(m_asrc, m_aenc, m_aqueue, m_mux, m_fsink, NULL);
    } else {
        gst_bin_add_many(GST_BIN(m_pipeline), m_asrc, m_vsrc, m_aenc, m_venc, m_aqueue, m_vqueue, m_mux, m_fsink, NULL);
        gst_element_link_many(m_asrc, m_aenc, m_aqueue, m_mux, NULL);
        gst_element_link_many(m_vsrc, m_venc, m_vqueue, m_mux, NULL);
        gst_element_link(m_mux, m_fsink);
    }

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_READY))
        g_warning("can't set pipeline to ready");
}


void MediaRecorderPrivate::deletePipeline()
{
    if (!m_pipeline)
        return;

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_NULL))
        g_warning("can't set pipeline to stop");

    if (GST_STATE_CHANGE_FAILURE == gst_element_get_state(m_pipeline, 0, 0, GST_CLOCK_TIME_NONE))
        g_warning("can't get pipeline status");

    gst_object_unref(m_pipeline);
    m_pipeline = 0;
}

void MediaRecorderPrivate::start()
{
    createPipeline();

    if (GST_STATE_CHANGE_FAILURE == gst_element_set_state(m_pipeline, GST_STATE_PLAYING)) {
        g_warning("can't set pipeline to playing");
        return;
    }
}

void MediaRecorderPrivate::stop()
{
    deletePipeline();
}

void MediaRecorderPrivate::cancel()
{
    stop();

    if (!m_recordedFileName.isEmpty())
        deleteFile(m_recordedFileName);
}

void MediaRecorderPrivate::record(MediaStreamDescriptor* descriptor, const String& prefix)
{
    if (m_pipeline) {
        g_warning("recording already started");
        return;
    }

    if (!prefix.isEmpty())
        m_prefix = prefix;

    m_recordMode = RECORD_MODE_NONE;

    for (unsigned int i = 0; i < descriptor->numberOfAudioComponents(); i++)
        m_recordMode |= RECORD_MODE_AUDIO_ONLY;

    for (unsigned int i = 0; i < descriptor->numberOfVideoComponents(); i++) {
        MediaStreamSource* source = descriptor->videoComponent(i)->source();
        m_cameraId = (source->name() == "Self camera") ? 1 : 0;
        m_recordMode |= RECORD_MODE_VIDEO_ONLY;
    }

    if (m_recordMode != RECORD_MODE_NONE)
        start();
}

void MediaRecorderPrivate::save(String& filename)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("Filename: %s", filename.utf8().data());
#endif

    gst_element_send_event(m_pipeline, gst_event_new_eos());
    GstBus* bus = gst_element_get_bus(GST_ELEMENT(m_pipeline));
    GstMessage* message = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, (GstMessageType)(GST_MESSAGE_EOS | GST_MESSAGE_ERROR));

    if (!message)
        g_warning("Message is NULL");
    else {
        switch (message->type) {
        case GST_MESSAGE_EOS:
            gst_message_unref(message);
            break;
        default:
            g_warning("Got message but not GST_MESSAGE_EOS");
            gst_message_unref(message);
            break;
        }
    }

    deletePipeline();
    filename.swap(m_recordedFileName);
}

void MediaRecorderPrivate::consumeEvents()
{
    GstBus* bus = gst_element_get_bus(GST_ELEMENT(m_pipeline));

    GstMessage* message = 0;
    while (1) {
        message = gst_bus_poll(bus, GST_MESSAGE_ANY, GST_SECOND * 0.1);
        if (!message) {
            g_warning("Message is NULL");
            break;
        }

        switch (message->type) {
        case GST_MESSAGE_EOS:
            gst_message_unref(message);
            break;
        default:
            gst_message_unref(message);
            break;
        }
    }
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
