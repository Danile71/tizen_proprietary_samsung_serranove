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
#include "LocalMediaServer.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "GStreamerVersioning.h"

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#include "MediaResourceControllerGStreamerTizen.h"
#endif

#define CAMERA_CAPS "video/x-raw-yuv, format=(fourcc)NV12, width = (int) 1280, height = (int) 720, framerate=(fraction)30/1"
#define HOST_ADDRESS "127.0.0.1"
#define HOST_PORT 8888

namespace WebCore {

static gboolean localMediaServerMessageCallback(GstBus*, GstMessage* message, LocalMediaServer* mediaServer)
{
    return mediaServer->handleMessage(message);
}

LocalMediaServer::LocalMediaServer()
    : m_camPipeline(0), m_stoped(true)
{
    if (!gst_is_initialized())
        gst_init_check(0, 0, 0);
}

LocalMediaServer::~LocalMediaServer()
{
    if (m_camPipeline) {
        gst_element_set_state(m_camPipeline, GST_STATE_NULL);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        MediaResourceControllerGStreamerTizen::mediaResourceController().setUseCameraResource(false);
#endif
        gst_object_unref(GST_OBJECT(m_camPipeline));
    }
}

void LocalMediaServer::createPipeline()
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("");
#endif

    m_camPipeline = gst_pipeline_new(0);
    GRefPtr<GstBus> bus = webkitGstPipelineGetBus(GST_PIPELINE(m_camPipeline));
    gst_bus_add_signal_watch(bus.get());
    g_signal_connect(bus.get(), "message", G_CALLBACK(localMediaServerMessageCallback), this);

    GstElement* src = 0;
    if (gst_element_factory_find("camerasrc"))
        src = gst_element_factory_make("camerasrc", 0);
    else if (gst_element_factory_find("qcamerasrc"))
        src = gst_element_factory_make("qcamerasrc", 0);

    if (!src) {
        TIZEN_LOGE("Fail to create src element for camera");
        return;
    }
    g_object_set(src, "camera-id", 1, NULL);
    g_object_set(src, "hflip", 1, NULL);

    // caps Filter to make caps data from camera.
    GstCaps* caps = gst_caps_from_string(CAMERA_CAPS);
    GstElement* capsFilter = gst_element_factory_make("capsfilter", 0);
    g_object_set(G_OBJECT(capsFilter), "caps", caps, NULL);
    gst_caps_unref(caps);

    GstElement* sink = gst_element_factory_make("tcpserversink", 0);
    g_object_set(sink, "host", HOST_ADDRESS, NULL);
    g_object_set(sink, "port", HOST_PORT, NULL);

    GstElement* mux = gst_element_factory_make("multipartmux", 0);

    gst_bin_add_many(GST_BIN(m_camPipeline), src, capsFilter, mux, sink, NULL);
    gst_element_link_many(src, capsFilter, mux, sink, NULL);
    gst_element_set_state(m_camPipeline, GST_STATE_PLAYING);
}

LocalMediaServer& LocalMediaServer::instance()
{
    DEFINE_STATIC_LOCAL(LocalMediaServer, mediaServer, ());
    return static_cast<LocalMediaServer&>(mediaServer);
}

void LocalMediaServer::startStream()
{
    m_stoped = false;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    MediaResourceControllerGStreamerTizen::mediaResourceController().setUseCameraResource(true);
#endif
    if (!m_camPipeline)
        createPipeline();
    else
        gst_element_set_state(m_camPipeline, GST_STATE_PLAYING);
}

void LocalMediaServer::stopStream()
{
    m_stoped = true;

    if (m_camPipeline) {
        gst_element_set_state(m_camPipeline, GST_STATE_NULL);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        MediaResourceControllerGStreamerTizen::mediaResourceController().setUseCameraResource(false);
#endif
    }
}

void LocalMediaServer::add(WebKitCameraSrc* client)
{
    m_clients.add(client, Initiated);
}

void LocalMediaServer::remove(WebKitCameraSrc* client)
{
    m_clients.remove(client);
    suspendIfNecessary();
}

void LocalMediaServer::stateChanged(WebKitCameraSrc* client, GstStateChange stateChange)
{
    switch (stateChange) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        m_clients.set(client, Preroll);
        if (!m_stoped) {
 #if ENABLE(TIZEN_GSTREAMER_VIDEO)
            MediaResourceControllerGStreamerTizen::mediaResourceController().setUseCameraResource(true);
#endif
            gst_element_set_state(m_camPipeline, GST_STATE_PLAYING);
        }
        break;
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        m_clients.set(client, Activate);
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        m_clients.set(client, Inactive);
        suspendIfNecessary();
        break;
    default:
        break;
    }
}

void LocalMediaServer::suspendIfNecessary()
{
    if (!m_camPipeline)
        return;

    HashMap<WebKitCameraSrc*, activateState>::iterator iter = m_clients.begin();
    for (; iter != m_clients.end(); ++iter) {
        if (iter->second != Inactive)
            return;
    }
    gst_element_set_state(m_camPipeline, GST_STATE_NULL);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    MediaResourceControllerGStreamerTizen::mediaResourceController().setUseCameraResource(false);
#endif
}

gboolean LocalMediaServer::handleMessage(GstMessage* message)
{
    switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_STATE_CHANGED:
        // Ignore state changes from internal elements.
        // They are forwarded to pipeline anyway.
        if (GST_MESSAGE_SRC(message) == GST_OBJECT(m_camPipeline)) {
            GstState newState, oldState;
            gst_message_parse_state_changed(message, &oldState, &newState, 0);

            CString dotFileName = String::format("webkit-localMediaServer.%s_%s",
                                                 gst_element_state_get_name(oldState),
                                                 gst_element_state_get_name(newState)).utf8();

            GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(m_camPipeline), GST_DEBUG_GRAPH_SHOW_ALL, dotFileName.data());
            if (newState == GST_STATE_PLAYING) {
                HashMap<WebKitCameraSrc*, activateState>::iterator iter = m_clients.begin();
                for (; iter != m_clients.end(); ++iter)
                    if (iter->second == Preroll)
                        gst_element_set_state(GST_ELEMENT(iter->first), GST_STATE_PLAYING);
            }
        }
        break;
    default:
        break;
    }

    return TRUE;
}

void LocalMediaServer::releaseLocalMediaServer()
{
    if (!m_camPipeline)
        return;

    gst_element_set_state(m_camPipeline, GST_STATE_NULL);
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    MediaResourceControllerGStreamerTizen::mediaResourceController().setUseCameraResource(false);
#endif
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
