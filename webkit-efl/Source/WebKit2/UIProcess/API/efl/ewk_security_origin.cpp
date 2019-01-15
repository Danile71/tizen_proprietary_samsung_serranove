/*
   Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

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
#include "ewk_security_origin.h"

#if PLATFORM(TIZEN)
#include "WKAPICast.h"
#include "WebSecurityOrigin.h"
#include "ewk_view_private.h"
#include <wtf/text/CString.h>

struct _Ewk_Security_Origin {
    const char* host;
    const char* protocol;
    uint16_t port;
};

using namespace WebKit;

Ewk_Security_Origin* createSecurityOrigin(WKSecurityOriginRef securityOrigin)
{
    Ewk_Security_Origin* origin = new Ewk_Security_Origin();

    origin->host = eina_stringshare_add(toImpl(securityOrigin)->host().utf8().data());
    origin->protocol = eina_stringshare_add(toImpl(securityOrigin)->protocol().utf8().data());
    origin->port = toImpl(securityOrigin)->port();

    return origin;
}

void deleteSecurityOrigin(Ewk_Security_Origin* origin)
{
    eina_stringshare_del(origin->host);
    eina_stringshare_del(origin->protocol);
    delete origin;
}

const char* ewk_security_origin_host_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);

    return origin->host;
}

const char* ewk_security_origin_protocol_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);
    TIZEN_LOGI("protocol (%s)", origin->protocol);

    return origin->protocol;
}

uint32_t ewk_security_origin_port_get(const Ewk_Security_Origin* origin)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(origin, 0);
    TIZEN_LOGI("port (%d)", origin->port);

    return origin->port;
}
#endif
