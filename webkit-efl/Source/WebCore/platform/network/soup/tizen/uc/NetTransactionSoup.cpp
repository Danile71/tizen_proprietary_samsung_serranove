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

#include "config.h"
#include "NetTransactionSoup.h"

#include "Logging.h"
#include "NetTransactionClient.h"
#include "ResourceHandle.h"

#include <UCProxySDK/HTTPHeaderMap.h>

namespace proxy {

static void finishedCallback(SoupSession* session, SoupMessage* msg, gpointer user_data);
static void gotHeadersCallback(SoupMessage* msg, gpointer data);
static void gotChunkCallback(SoupMessage* msg, SoupBuffer* chunk, gpointer data);

NetTransactionSoup::NetTransactionSoup(const HTTPURLRequest& request, NetTransactionClient* client)
   : m_refCount(1)
   , m_request(request)
   , m_soupMessage(NULL)
   , m_client(client)
{
}

NetTransactionSoup::~NetTransactionSoup()
{
	destroySoupMessage();
}

NetTransactionSoup* NetTransactionSoup::create(const HTTPURLRequest& request, NetTransactionClient* client)
{
	NetTransactionSoup* newHandle = new NetTransactionSoup(request, client);
	return newHandle;
}

void NetTransactionSoup::ref()
{
	m_refCount++;
}

void NetTransactionSoup::deref()
{
	m_refCount--;
	if (m_refCount <= 0)
		delete this;
}

NetTransactionClient* NetTransactionSoup::client() const
{
    return m_client;
}

void NetTransactionSoup::setClient(NetTransactionClient* client)
{
    m_client = client;
}

bool NetTransactionSoup::start(const HTTPURLRequest& request)
{
    if (m_soupMessage) {
        TIZEN_LOGI ("[HTTPWrapper] existing soupMessage ERROR m_soupMessage[%p]", m_soupMessage);
        return false;
    }

	std::string url = request.getUrl();
	std::string method = request.httpMethod();

    m_request = request;

    m_soupMessage = soup_message_new (method.c_str(), url.c_str());
    if (!m_soupMessage) {
        TIZEN_SECURE_LOGI ("[HTTPWrapper] fail to create soupMessage for url[%s]", url.c_str());
    	return false;
    }

    soup_message_set_flags(m_soupMessage, SOUP_MESSAGE_NO_REDIRECT);

    HTTPHeaderMap headerMap = request.getAllHttpHeaderFields();
    for (HTTPHeaderMap::iterator it=headerMap.begin(); it!=headerMap.end(); ++it) {
        const proxy::UCString name = it->first;
        const proxy::UCString value = it->second;
        soup_message_headers_append(m_soupMessage->request_headers, name.c_str(), value.c_str());
    }

    std::string body = request.getHttpBody();
    if (!body.empty()) {
        std::string type = request.httpHeaderField("Content-Type");
        soup_message_set_request(m_soupMessage,
                         type.c_str(),
                         SOUP_MEMORY_COPY,
                         body.c_str(),
                         body.length());
    }

	g_signal_connect(m_soupMessage, "got-headers", G_CALLBACK(gotHeadersCallback), this);
	g_signal_connect(m_soupMessage, "got-chunk", G_CALLBACK(gotChunkCallback), this);

    // according to https://developer.gnome.org/libsoup/stable/libsoup-client-howto.html,
    // "soup_session_queue_message is slightly unusual in that it steals a reference to the message object, 
    // and unrefs it after the last callback is invoked on it. So when using this API, you should not unref the message yourself."
    // so, call to g_object_ref(m_soupMessage) before soup_session_queue_message.
    g_object_ref(m_soupMessage);
    
    // ref() so that NetTransactionSoup object will not be destroyed until got finishedCallback.
    ref();

	soup_session_queue_message(WebCore::ResourceHandle::defaultSession(), m_soupMessage, finishedCallback, this);

	return true;
}

void NetTransactionSoup::cancel()
{
    if (!m_soupMessage) {
         TIZEN_LOGI ("[HTTPWrapper] can't cancel m_soupMessage[%p]", m_soupMessage);
         return;
    }

    soup_session_cancel_message(WebCore::ResourceHandle::defaultSession(), m_soupMessage, SOUP_STATUS_CANCELLED);
}

void NetTransactionSoup::gotHeaders(SoupMessage* soupMessage)
{
    if (soupMessage != m_soupMessage) {
        TIZEN_LOGI ("[HTTPWrapper] not matched m_soupMessage[%p] soupMessage[%p]", m_soupMessage, soupMessage);
        return;
    }

	SoupMessageHeaders* response_headers;
	response_headers = m_soupMessage->response_headers;

	m_response.setHTTPStatusCode(m_soupMessage->status_code);

    SoupMessageHeadersIter iter;
    const char* name;
    const char* value;
    soup_message_headers_iter_init(&iter, m_soupMessage->response_headers);
    while (soup_message_headers_iter_next(&iter, &name, &value))
        m_response.setHTTPHeaderField(name, value);

    if (m_client)
		m_client->onResponseReceived(m_response);
}

void NetTransactionSoup::gotChunk(SoupMessage* soupMessage, SoupBuffer* chunk)
{
    if (soupMessage != m_soupMessage) {
        TIZEN_LOGI ("[HTTPWrapper] not matched m_soupMessage[%p] soupMessage[%p]", m_soupMessage, soupMessage);
        return;
    }

	if (m_client)
		m_client->onDataReceived(chunk->data, chunk->length);
}

void NetTransactionSoup::gotFinished(SoupMessage* soupMessage)
{
    if (soupMessage != m_soupMessage) {
        TIZEN_LOGI ("[HTTPWrapper] not matched m_soupMessage[%p] soupMessage[%p]", m_soupMessage, soupMessage);
        return;
    }

	if (m_soupMessage->status_code < 200) {
		std::string domain = g_quark_to_string(SOUP_HTTP_ERROR);
		int errorCode = m_soupMessage->status_code;
		std::string failingURL = m_request.getUrl();
		std::string errorMessage = m_soupMessage->reason_phrase;

        destroySoupMessage();

		if (m_client)
			m_client->onError(domain, errorCode, failingURL, errorMessage);

	} else {
        destroySoupMessage();

        if (m_client)
			m_client->onFinishLoading();
    }

    // NetTransactionSoup will be destroyed now.
    deref();
}

void NetTransactionSoup::destroySoupMessage()
{
    if (m_soupMessage) {
        g_object_unref(m_soupMessage);
        m_soupMessage = NULL;
    }
}

/////////////////////////////////////////////////////////////////////////
// callbacks for libsoup
/////////////////////////////////////////////////////////////////////////
void gotHeadersCallback(SoupMessage* soupMessage, gpointer user_data)
{
	NetTransactionSoup* trans = static_cast<NetTransactionSoup*>(user_data);
	trans->gotHeaders(soupMessage);
}

void gotChunkCallback(SoupMessage *soupMessage, SoupBuffer *chunk, gpointer user_data)
{
	NetTransactionSoup* trans = static_cast<NetTransactionSoup*>(user_data);
	trans->gotChunk(soupMessage, chunk);
}

void finishedCallback(SoupSession *session, SoupMessage *soupMessage, gpointer user_data)
{
	NetTransactionSoup* trans = static_cast<NetTransactionSoup*>(user_data);
	trans->gotFinished(soupMessage);
}

} // namespace WebCore
