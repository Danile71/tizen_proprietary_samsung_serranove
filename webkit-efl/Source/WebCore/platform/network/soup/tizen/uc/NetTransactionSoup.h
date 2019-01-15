/*
 * Copyright (C) 2014 Samsung Electronics
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
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef __UCProxy__NetTransactionSoup__H__
#define __UCProxy__NetTransactionSoup__H__

#include <UCProxySDK/HTTPURLRequest.h>
#include <UCProxySDK/HTTPURLResponse.h>
#include <libsoup/soup.h>

namespace proxy {

class NetTransactionClient;

class NetTransactionSoup
{
public:
    static NetTransactionSoup* create(const HTTPURLRequest&, NetTransactionClient*);

    ~NetTransactionSoup();

    void ref();
    void deref();

    NetTransactionClient* client() const;
    void setClient(NetTransactionClient*);

    bool start(const HTTPURLRequest& request);
    void cancel();

    void gotHeaders(SoupMessage* soupMessage);
    void gotChunk(SoupMessage* soupMessage, SoupBuffer* chunk);
    void gotFinished(SoupMessage* soupMessage);

private:
    NetTransactionSoup(const HTTPURLRequest&, NetTransactionClient*);
    void destroySoupMessage();

	int m_refCount;

    HTTPURLRequest m_request;
    HTTPURLResponse m_response;

    SoupMessage *m_soupMessage;

    NetTransactionClient* m_client;
};

} // namespace WebCore

#endif // __UCProxy__NetTransactionSoup__H__