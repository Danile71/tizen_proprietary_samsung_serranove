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
#include "PeerConnectionHandler.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "PeerConnectionHandlerClient.h"
#include "SecurityOrigin.h"

namespace WebCore {

PassOwnPtr<PeerConnectionHandler> PeerConnectionHandler::create(PeerConnectionHandlerClient* client, const String& serverConfiguration, const String& username)
{
    return adoptPtr(new PeerConnectionHandler(client, serverConfiguration, username));
}

// FIXME: remove when real implementations are available
// Empty implementations for ports that build with MEDIA_STREAM enabled by default.
PeerConnectionHandler::PeerConnectionHandler(PeerConnectionHandlerClient*, const String& serverConfiguration, const String& username)
{
}

PeerConnectionHandler::~PeerConnectionHandler()
{
}

void PeerConnectionHandler::produceInitialOffer(const MediaStreamDescriptorVector&)
{
}

void PeerConnectionHandler::handleInitialOffer(const String&)
{
}

void PeerConnectionHandler::processSDP(const String&)
{
}

void PeerConnectionHandler::processPendingStreams(const MediaStreamDescriptorVector&, const MediaStreamDescriptorVector&)
{
}

void PeerConnectionHandler::sendDataStreamMessage(const char*, size_t)
{
}

void PeerConnectionHandler::stop()
{
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
