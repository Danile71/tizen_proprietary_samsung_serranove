/*
 * Copyright (C) 2012 Samsung Electronics
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
#include "NetworkStateNotifier.h"

#include <vconf.h>

namespace WebCore {

void networkChanged(keynode_t* keynode, void* data)
{
    bool isOnLine = true;

    int state = 0;
    if (!vconf_get_int(VCONFKEY_NETWORK_STATUS, &state) && state == VCONFKEY_NETWORK_OFF)
        isOnLine = false;

    static_cast<NetworkStateNotifier*>(data)->setOnLine(isOnLine);
}

NetworkStateNotifier::NetworkStateNotifier()
    : m_isOnLine(true)
    , m_networkStateChangedFunction(0)
{
    int state = 0;
    if (!vconf_get_int(VCONFKEY_NETWORK_STATUS, &state) && state == VCONFKEY_NETWORK_OFF)
        m_isOnLine = false;

    vconf_notify_key_changed(VCONFKEY_NETWORK_CONFIGURATION_CHANGE_IND, networkChanged, this);
}

}
