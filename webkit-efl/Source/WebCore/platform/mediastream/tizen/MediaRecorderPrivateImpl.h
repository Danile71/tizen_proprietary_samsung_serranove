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

#ifndef MediaRecorderPrivateImpl_h
#define MediaRecorderPrivateImpl_h

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaRecorderPrivate.h"
#include <gst/gst.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class MediaRecorder;
class MediaStreamDescriptor;

enum {
    RECORD_MODE_NONE = 0x0000,
    RECORD_MODE_AUDIO_ONLY = 0x0001,
    RECORD_MODE_VIDEO_ONLY = 0x0010,
    RECORD_MODE_AUDIO_VIDEO = 0x0011,
    RECORD_MODE_INVALID = 0x0100
};

class MediaRecorderPrivate : public MediaRecorderPrivateInterface {
public:
    static PassOwnPtr<MediaRecorderPrivateInterface> create(MediaRecorder* recorder);
    virtual ~MediaRecorderPrivate();

    virtual void record(MediaStreamDescriptor*, const String& prefix);
    virtual void save(String& filename);
    virtual void cancel();

private:
    MediaRecorderPrivate(MediaRecorder*);

    void start();
    void stop();
    void createPipeline();
    void deletePipeline();
    void consumeEvents();

    MediaRecorder* m_recorder;

    GstElement *m_pipeline;
    GstElement *m_vsrc, *m_venc, *m_vqueue;
    GstElement *m_asrc, *m_aenc, *m_aqueue;
    GstElement *m_mux, *m_fsink;
    GstBus *m_bus;

    int m_cameraId;
    int m_recordMode;

    String m_recordedFileName;
    String m_prefix;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // MediaRecorderPrivateImpl_h
