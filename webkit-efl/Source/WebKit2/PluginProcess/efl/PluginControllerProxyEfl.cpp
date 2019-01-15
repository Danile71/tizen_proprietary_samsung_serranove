/*
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2011 Apple Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PluginControllerProxy.h"

#if ENABLE(PLUGIN_PROCESS)
#include "NetscapePlugin.h"

#include <WebCore/NotImplemented.h>

using namespace WebCore;

namespace WebKit {

void PluginControllerProxy::platformInitialize(const PluginCreationParameters&)
{
    notImplemented();
}

void PluginControllerProxy::platformDestroy()
{
    notImplemented();
}

void PluginControllerProxy::platformGeometryDidChange()
{
    notImplemented();
}

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING_PLUGIN_LAYER_EFL)
void PluginControllerProxy::updateContentBuffers(const IntRect& updateRect)
{
    if (m_plugin)
        m_plugin->updateContentBuffers(updateRect);
}
#endif

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
