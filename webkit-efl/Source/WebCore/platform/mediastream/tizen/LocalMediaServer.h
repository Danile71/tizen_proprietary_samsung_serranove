/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LocalMediaServer_h
#define LocalMediaServer_h

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "WebKitCameraSourceGStreamerTizen.h"
#include <gst/gst.h>
#include <wtf/HashMap.h>

namespace WebCore {

class LocalMediaServer {

public:
    enum activateState {Initiated, Preroll, Activate, Inactive};

    ~LocalMediaServer();

    static LocalMediaServer& instance();

    virtual void startStream();
    virtual void stopStream();

    virtual void add(WebKitCameraSrc*);
    virtual void remove(WebKitCameraSrc*);
    virtual void stateChanged(WebKitCameraSrc*, GstStateChange);
    gboolean handleMessage(GstMessage*);
    void releaseLocalMediaServer();

private:
    LocalMediaServer();

    void createPipeline();
    void suspendIfNecessary();

    GstElement* m_camPipeline;
    bool m_stoped;
    HashMap<WebKitCameraSrc*, activateState> m_clients;
};

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)

#endif // LocalMediaServer_h
