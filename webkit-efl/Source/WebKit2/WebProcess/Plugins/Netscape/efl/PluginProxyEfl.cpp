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
#include "PluginProxy.h"

#if ENABLE(PLUGIN_PROCESS)
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING_PLUGIN_LAYER_EFL)
#include "PluginControllerProxyMessages.h"
#include "PluginProcessConnection.h"
#include "WebCoreArgumentCoders.h"
#endif

namespace WebKit {

bool PluginProxy::needsBackingStore() const
{
#if ENABLE(TIZEN_ACCELERATED_COMPOSITING_PLUGIN_LAYER_EFL)
    return false;
#else
    return true;
#endif
}

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING_PLUGIN_LAYER_EFL)
PlatformLayer* PluginProxy::pluginLayer()
{
    return static_cast<PlatformLayer*>(this);
}

void PluginProxy::updateContentBuffers(const WebCore::IntRect& rect)
{
    m_connection->connection()->send(Messages::PluginControllerProxy::UpdateContentBuffers(rect), m_pluginInstanceID);
}
#endif

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
