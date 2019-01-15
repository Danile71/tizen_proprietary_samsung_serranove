/*
 * Copyright (C) 2012 Samsung Electronics
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
 *
 */

#include "config.h"
#include "WebInspectorServerEfl.h"

#if ENABLE(TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR)

#include "MD5.h"
#include "WebPage.h"
#include "WebFrame.h"
#include "WebInspectorClient.h"
#include <WebCore/InspectorController.h>
#include <WebCore/Page.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <string.h>
#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/SHA1.h>
#include <wtf/text/Base64.h>

using namespace WebCore;
using std::numeric_limits;

#define WIDGET_DEBUG_PATH "/WidgetDebug"

namespace WebKit {

const char* websocketHybi10UpgradeValue= "websocket";
const char* websocketHybi00UpgradeValue= "WebSocket";
const size_t socketBufferSize = 8192;
const size_t maxPayloadLengthWithoutExtendedLengthField = 125;
const size_t payloadLengthWithTwoByteExtendedLengthField = 126;
const size_t payloadLengthWithEightByteExtendedLengthField = 127;
const size_t maskingKeyWidthInBytes = 4;

static String generateWebSocketAccept(const String& secWebSocketKey)
{
    static const char* const webSocketKeyGUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    static const size_t sha1HashSize = 20;
    SHA1 sha1;
    CString keyData = secWebSocketKey.ascii();
    sha1.addBytes(reinterpret_cast<const uint8_t*>(keyData.data()), keyData.length());
    sha1.addBytes(reinterpret_cast<const uint8_t*>(webSocketKeyGUID), strlen(webSocketKeyGUID));
    Vector<uint8_t, sha1HashSize> hash;
    sha1.computeHash(hash);
    return base64Encode(reinterpret_cast<const char*>(hash.data()), sha1HashSize);
}

static void setChallengeNumber(uint32_t number, unsigned char* buf)
{
    unsigned char* p = buf + 3;
    for (int i = 0; i < 4; i++) {
        *p = number & 0xFF;
        --p;
        number >>= 8;
    }
}

static void generateWebSocketChallengeResponse(uint32_t number1, uint32_t number2, const unsigned char key3[8], unsigned char response[16])
{
    uint8_t challenge[16];
    setChallengeNumber(number1, &challenge[0]);
    setChallengeNumber(number2, &challenge[4]);
    memcpy(&challenge[8], key3, 8);
    MD5 md5;
    md5.addBytes(challenge, sizeof(challenge));
    Vector<uint8_t, 16> digest;
    md5.checksum(digest);
    memcpy(response, digest.data(), 16);
}

static unsigned int parseWebSocketChallengeNumber(String field)
{
    String nString;
    int numSpaces = 0;

    for (unsigned int i = 0; i < field.length(); i++) {
        UChar c = field[i];
        if (c == ' ')
            numSpaces++;
        else if ((c >= '0') && (c <= '9'))
            nString.append(c);
    }

    unsigned int num = nString.toUIntStrict();
    unsigned int result = (numSpaces ? (num / numSpaces) : num);
    return result;
}

static WebInspectorServerEfl* webInspectorServerEfl;
WebInspectorServerEfl* WebInspectorServerEfl::server()
{
    if (!webInspectorServerEfl)
        webInspectorServerEfl = new WebInspectorServerEfl();
    return webInspectorServerEfl;
}

WebInspectorServerEfl::WebInspectorServerEfl()
    : m_pageNumber(1)
    , m_inspectorClient(0)
    , m_buffer(0)
    , m_bufferSize(0)
    , m_useHixie76Protocol(0)
#if ENABLE(GLIB_SUPPORT)
    , m_serverSocket(0)
    , m_serverChannel(0)
    , m_serverSource(0)
    , m_clientSocket(0)
    , m_clientChannel(0)
    , m_clientSource(0)
#endif
{
}

WebInspectorServerEfl::~WebInspectorServerEfl()
{
}

bool WebInspectorServerEfl::startServer(unsigned int port)
{
#if ENABLE(GLIB_SUPPORT)
    if (m_serverSocket || m_serverChannel || m_serverSource)
        return false;

    struct sockaddr_in sockAddrIn;
    memset(&sockAddrIn, 0, sizeof(sockAddrIn));
    sockAddrIn.sin_family = AF_INET;
    sockAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
    sockAddrIn.sin_port = htons(port);

    GSocketAddress *addr =
        g_socket_address_new_from_native((gpointer)&sockAddrIn,
                                          sizeof(sockAddrIn));
    if (!addr) {
        LOG_ERROR("Error : g_socket_address_new_from_native()\n");
        return false;
    }

    GError *error = 0;
    m_serverSocket = g_socket_new(G_SOCKET_FAMILY_IPV4,
                                  G_SOCKET_TYPE_STREAM,
                                  G_SOCKET_PROTOCOL_TCP,
                                  &error);
    if (!m_serverSocket) {
        g_object_unref(addr);
        LOG_ERROR("Error : g_socket_new()\n");
        if (error) {
            LOG_ERROR("Error : %s(%d)", error->message, error->code);
            g_error_free(error);
            error = 0;
        }
        return false;
    }

    g_socket_set_blocking(m_serverSocket, FALSE);
    g_socket_set_listen_backlog(m_serverSocket, 5);

    if (!g_socket_bind(m_serverSocket, addr, TRUE, &error)) {
        LOG_ERROR("Error : g_socket_bind()\n");
        g_object_unref(addr);
        if (error) {
            LOG_ERROR("Error : %s(%d)", error->message, error->code);
            g_error_free(error);
            error = 0;
        }
        g_socket_close(m_serverSocket, 0);
        m_serverSocket = 0;
        return false;
    }
    g_object_unref(addr);

    if (!g_socket_listen(m_serverSocket, &error)) {
        LOG_ERROR("Error : g_socket_listen()\n");
        if (error) {
            LOG_ERROR("Error : %s(%d)", error->message, error->code);
            g_error_free(error);
            error = 0;
        }
        g_socket_close(m_serverSocket, 0);
        m_serverSocket = 0;
        return false;
    }

    int fd = g_socket_get_fd(m_serverSocket);
    m_serverChannel = g_io_channel_unix_new(fd);
    if (!m_serverChannel) {
        LOG_ERROR("Error : g_io_channel_unix_new()\n");
        g_socket_close(m_serverSocket, 0);
        m_serverSocket = 0;
        return false;
    }
    g_io_channel_set_buffer_size(m_serverChannel, socketBufferSize);

    m_serverSource = g_io_add_watch(m_serverChannel,
                                    G_IO_IN,
                                    WebInspectorServerEfl::onHttpRequest,
                                    (gpointer)this);
    if (!m_serverSource) {
        LOG_ERROR("Error : g_io_add_watch()\n");
        g_io_channel_unref(m_serverChannel);
        m_serverChannel = 0;
        g_socket_close(m_serverSocket, 0);
        m_serverSocket = 0;
        return false;
    }
    return true;
#else
    return false;
#endif
}

bool WebInspectorServerEfl::stopServer()
{
#if ENABLE(GLIB_SUPPORT)
    if (!m_serverSocket || !m_serverChannel || !m_serverSource)
        return false;

    if (m_serverSource) {
        g_source_remove(m_serverSource);
        m_serverSource = 0;
    }

    if (m_serverChannel) {
        GError *error = 0;
        g_io_channel_shutdown(m_serverChannel, TRUE, &error);
        if (error) {
            LOG_ERROR("Error : %s(%d)\n", error->message, error->code);
            g_error_free(error);
            error = 0;
        }
        g_io_channel_unref(m_serverChannel);
        m_serverChannel = 0;
    }

    if (m_serverSocket) {
        g_socket_close(m_serverSocket, 0);
        m_serverSocket = 0;
    }

    webSocketDisconnected();

    return true;
#else
    return false;
#endif
}

unsigned int WebInspectorServerEfl::getServerPort()
{
#if ENABLE(GLIB_SUPPORT)
    if (!m_serverSocket)
        return 0;

    GSocketAddress* addr = g_socket_get_local_address(m_serverSocket, 0);
    if (!addr) {
        LOG_ERROR("Error : g_socket_get_local_address()\n");
        return 0;
    }

    struct sockaddr_in sockAddrIn;
    g_socket_address_to_native(addr, (gpointer)&sockAddrIn, sizeof(sockAddrIn), 0);
    g_object_unref(addr);

    // Get an allocated port number
    unsigned int port;
    port = ntohs(sockAddrIn.sin_port);

    return port;
#else
    return 0;
#endif
}

void WebInspectorServerEfl::registerClient(WebInspectorClient* client)
{
    HashMap<unsigned, WebInspectorClient*>::iterator it;
    for (it = m_clientMap.begin(); it != m_clientMap.end(); ++it) {
        if (it->second == client)
            return;
    }

    m_clientMap.add(m_pageNumber++, client);
}

void WebInspectorServerEfl::unregisterClient(WebInspectorClient* client)
{
    int pageNum = -1;
    HashMap<unsigned, WebInspectorClient*>::iterator it;
    for (it = m_clientMap.begin(); it != m_clientMap.end(); ++it) {
        if (it->second == client) {
            pageNum = it->first;
            break;
        }
    }

    if (pageNum >= 0)
        m_clientMap.remove(pageNum);

    if (!m_clientMap.size()) {
        WebInspectorServerEfl::server()->stopServer();

        if (webInspectorServerEfl) {
            delete webInspectorServerEfl;
            webInspectorServerEfl = 0;
        }
    }
}

WebInspectorClient* WebInspectorServerEfl::inspectorClientForPage(unsigned pageNum)
{
    WebInspectorClient* client = m_clientMap.get(pageNum);
    return client;
}

void WebInspectorServerEfl::webSocketConnected()
{
#if ENABLE(GLIB_SUPPORT)
     if (!m_clientSocket || !m_inspectorClient)
         return;

     g_socket_set_blocking(m_clientSocket, TRUE);
     int fd = g_socket_get_fd(m_clientSocket);
     m_clientChannel = g_io_channel_unix_new(fd);
     if (!m_clientChannel) {
         LOG_ERROR("Error : g_io_channel_unix_new()\n");
         GError *error = 0;
         g_socket_close(m_clientSocket, &error);
         if (error) {
             LOG_ERROR("Error : %s(%d)", error->message, error->code);
             g_error_free(error);
             error = 0;
         }
         m_clientSocket = 0;
         return;
     }
     g_io_channel_set_buffer_size(m_clientChannel, socketBufferSize);

     m_clientSource = g_io_add_watch(m_clientChannel,
                                    G_IO_IN,
                                    WebInspectorServerEfl::onWebSocketRequest,
                                    this);

    if (m_inspectorClient)
        m_inspectorClient->attachRemoteFrontend();

#endif
}

void WebInspectorServerEfl::webSocketDisconnected()
{
#if ENABLE(GLIB_SUPPORT)
    if (!m_clientSocket || !m_inspectorClient)
        return;

    if (m_buffer) {
       free(m_buffer);
       m_buffer = 0;
       m_bufferSize = 0;
    }

    if (m_inspectorClient) {
        m_inspectorClient->detachRemoteFrontend();
        m_inspectorClient = 0;
    }

    if (m_clientSource) {
        g_source_remove(m_clientSource);
        m_clientSource = 0;
    }

    GError *error = 0;
    if (m_clientChannel) {
        g_io_channel_shutdown(m_clientChannel, TRUE, &error);
        if (error) {
            LOG_ERROR("Error : %s(%d)\n", error->message, error->code);
            g_error_free(error);
            error = 0;
        }
        g_io_channel_unref(m_clientChannel);
        m_clientChannel = 0;
    }

    if (m_clientSocket) {
        g_socket_close(m_clientSocket, &error);
        if (error) {
            LOG_ERROR("Error : %s(%d)\n", error->message, error->code);
            g_error_free(error);
            error = 0;
        }
        m_clientSocket = 0;
    }

#endif
}

bool WebInspectorServerEfl::webSocketSend(const String& message)
{
#if ENABLE(GLIB_SUPPORT)
    if (!m_clientSocket || !message.length())
        return false;

    GError *error = 0;

    if (!m_useHixie76Protocol) {
        size_t dataLength = message.length();
        Vector<char> frame;

        frame.resize(2);
        frame.at(0) = 0x80 | 0x1; // Filnal Bit | OpCode(Text)
        frame.at(1) = 0x0; // maskBit

        if (dataLength <= maxPayloadLengthWithoutExtendedLengthField)
            frame.at(1) |= dataLength;
        else if (dataLength <= 0xFFFF) {
            frame.at(1) |= payloadLengthWithTwoByteExtendedLengthField; // Payload length
            frame.append((dataLength & 0xFF00) >> 8);
            frame.append(dataLength & 0xFF);
        } else {
            frame.at(1) |= payloadLengthWithEightByteExtendedLengthField; // Payload length
            char extendedPayloadLength[8];
            size_t remaining = dataLength;
            // Fill the length into extendedPayloadLength in the network byte order.
            for (int i = 0; i < 8; ++i) {
                extendedPayloadLength[7 - i] = remaining & 0xFF;
                remaining >>= 8;
            }
            frame.append(extendedPayloadLength, 8);
        }

        frame.append(message.utf8().data(), dataLength);

        unsigned int sent = 0;
        int ret;
        while (sent < frame.size()) {
            ret = g_socket_send(m_clientSocket,
                                frame.data() + sent,
                                (gsize)frame.size() - sent,
                                0, &error);
            if (ret == 0) {
                webSocketDisconnected();
                break;
            } else if (ret < 0) {
                if (error) {
                    LOG_ERROR("Error : %s(%d) %d", error->message, error->code, ret);
                    g_error_free(error);
                    error = 0;
                }
                break;
            } else {
                sent += ret;
                continue;
            }
        }
    } else {
        CString cstr = message.utf8();

        Vector<char> frame;
        frame.append('\0');
        frame.append(cstr.data(), cstr.length());
        frame.append('\xff');

        unsigned int sent = 0;
        int ret;
        while (sent < frame.size()) {
            ret = g_socket_send(m_clientSocket,
                                frame.data() + sent,
                                (gsize)frame.size() - sent,
                                0, &error);
            if (ret == 0) {
                webSocketDisconnected();
                break;
            } else if (ret < 0) {
                if (error) {
                    LOG_ERROR("Error : %s(%d) %d", error->message, error->code, ret);
                    g_error_free(error);
                    error = 0;
                }
                break;
            } else {
                sent += ret;
                continue;
            }
        }
    }
    return true;
#else
    return false;
#endif
}

#if ENABLE(GLIB_SUPPORT)
gboolean WebInspectorServerEfl::onHttpRequest(GIOChannel *source, GIOCondition condition, gpointer data)
{
    WebInspectorServerEfl *server = (WebInspectorServerEfl*)data;
    if (!server)
        return FALSE;

    GError *error = 0;
    gchar buffer[socketBufferSize] = {0};

    GSocket *clientSocket = g_socket_accept(server->m_serverSocket, 0, &error);
    if (error) {
        LOG_ERROR("Error : %s(%d)", error->message, error->code);
        g_error_free(error);
        error = 0;
    }

    gssize len = g_socket_receive(clientSocket, buffer, socketBufferSize, 0, &error);
    if (error) {
        LOG_ERROR("Error : %s(%d)", error->message, error->code);
        g_error_free(error);
        error = 0;
    }

#if USE(SOUP)
    // Parsing a header
    SoupMessageHeaders* requestHeader = 0;
    requestHeader = soup_message_headers_new(SOUP_MESSAGE_HEADERS_REQUEST);

    if (!soup_headers_parse(buffer, len, requestHeader) // HTTP Request or WebSocket Handshake(hybi-10)
        && !soup_headers_parse(buffer, len - 8, requestHeader)) {// WebSocket Handshake(hybi-00)
        LOG_ERROR("Error : soup_headers_parse() %s\n", buffer);
        if (requestHeader) {
            soup_message_headers_free(requestHeader);
            requestHeader = 0;
        }
        return FALSE;
    }

    // Request-Line
    char* idxStr = strstr(buffer, "\r");
    *idxStr = '\0';
    String requestLine(buffer);
    Vector<String> parsedRequestLine;
    requestLine.split(" ", parsedRequestLine);

    Vector<String> uri;
    parsedRequestLine[1].split('?', uri);
    String path = uri[0]; // only path
    const char* value = 0;

    value = soup_message_headers_get_one(requestHeader, "Upgrade");
    if (value && !strncmp(value, websocketHybi10UpgradeValue, strlen(websocketHybi10UpgradeValue))) {

        if (server->m_clientSocket || server->m_inspectorClient)
            server->webSocketDisconnected();

        // Header Response
        String headerResponse = "HTTP/1.1 101 Switching Protocols\r\n";
        headerResponse.append("Upgrade: websocket\r\n");
        headerResponse.append("Connection: Upgrade\r\n");
        String accept= generateWebSocketAccept(String::fromUTF8(soup_message_headers_get_one(requestHeader, "Sec-WebSocket-Key")));
        headerResponse += "Sec-WebSocket-Accept: " + accept + "\r\n";
        headerResponse += "\r\n";

        g_socket_send(clientSocket, headerResponse.utf8().data(), headerResponse.length(), 0, &error);
        if (error) {
            LOG_ERROR("Error : %s(%d)", error->message, error->code);
            g_error_free(error);
            error = 0;
        }

        Vector<String> words;
        path.split('/', words); // devtools/page/X
        if ((words.size() == 3)
             && (words.at(0) == "devtools")
             && (words.at(1) == "page")) {

            int pageNum = words.at(2).toInt();
            server->m_useHixie76Protocol = false;
            server->m_clientSocket = clientSocket;
            server->m_inspectorClient = server->inspectorClientForPage(pageNum);
            server->webSocketConnected();
        }
        if (requestHeader) {
            soup_message_headers_free(requestHeader);
            requestHeader = 0;
        }
        return TRUE;
    }
    else if (value && !strncmp(value, websocketHybi00UpgradeValue, strlen(websocketHybi00UpgradeValue))) {

        if (server->m_clientSocket || server->m_inspectorClient)
            server->webSocketDisconnected();

        // Get the Key3
        unsigned char key3[8];
        memcpy(key3, buffer+(len-8), 8);

        // Computes the WebSocket handshake response given the two challenge numbers and key3
        unsigned char responseKey[16];
        unsigned int number1 = parseWebSocketChallengeNumber(String::fromUTF8(soup_message_headers_get_one(requestHeader, "Sec-WebSocket-Key1")));
        unsigned int number2 = parseWebSocketChallengeNumber(String::fromUTF8(soup_message_headers_get_one(requestHeader, "Sec-WebSocket-Key2")));
        generateWebSocketChallengeResponse(number1, number2, key3, responseKey);

        // Header Response
        String headerResponse = "HTTP/1.1 101 WebSocket Protocol Handshake\r\n";
        headerResponse.append("Upgrade: WebSocket\r\n");
        headerResponse.append("Connection: Upgrade\r\n");
        headerResponse += "Sec-WebSocket-Origin: " + String::fromUTF8(soup_message_headers_get_one(requestHeader, "Origin")) + "\r\n";
        headerResponse += "Sec-WebSocket-Location: ws://" + String::fromUTF8(soup_message_headers_get_one(requestHeader, "Host")) + path + "\r\n";
        headerResponse += "\r\n";

        // Send Response
        g_socket_send(clientSocket, headerResponse.utf8().data(), headerResponse.length(), 0, &error);
        if (error) {
            LOG_ERROR("Error : %s(%d)", error->message, error->code);
            g_error_free(error);
            error = 0;
        }
        // Send Handshake-Response key
        g_socket_send(clientSocket, (gchar*)responseKey, 16, 0, &error);
        if (error) {
            LOG_ERROR("Error : %s(%d)", error->message, error->code);
            g_error_free(error);
            error = 0;
        }

        Vector<String> words;
        path.split('/', words); // devtools/page/X
        if ((words.size() == 3)
             && (words.at(0) == "devtools")
             && (words.at(1) == "page")) {

            int pageNum = words.at(2).toInt();
            server->m_useHixie76Protocol = true;
            server->m_clientSocket = clientSocket;
            server->m_inspectorClient = server->inspectorClientForPage(pageNum);
            server->webSocketConnected();
        }
        if (requestHeader) {
            soup_message_headers_free(requestHeader);
            requestHeader = 0;
        }
        return TRUE;
    }

    if (requestHeader) {
        soup_message_headers_free(requestHeader);
        requestHeader = 0;
    }
#endif // USE(SOUP)

    GString* response = g_string_new (0);
    if ((path == "") || (path == "/")) { // For Browser
        GString* indexHtml = g_string_new (0);

        g_string_append(indexHtml, "<html><head><title>Remote Web Inspector</title></head><body><ul>\n");
        for (HashMap<unsigned, WebInspectorClient*>::iterator it = server->m_clientMap.begin();
             it != server->m_clientMap.end();
             ++it) {
            g_string_append_printf(indexHtml, "<li><a href=\"inspector.html?page=%d\">%s</li>\n"
                                           ,it->first
                                           ,it->second->m_page->mainWebFrame()->url().utf8().data());
        }
        g_string_append(indexHtml, "</ul></body></html>");
        g_string_append_printf(response, "HTTP/1.0 200 OK\r\nContent-Type: text/HTML\r\nContent-Length: %d\r\n\r\n%s", indexHtml->len, indexHtml->str);

        g_socket_send(clientSocket, response->str, response->len, 0, &error);
        if (error) {
            LOG_ERROR("Error : %s(%d)", error->message, error->code);
            g_error_free(error);
            error = 0;
        }
        g_string_free (indexHtml, TRUE);
    }else if (path == WIDGET_DEBUG_PATH) { // For WAC Widget
        GString* indexHtml = g_string_new (0);

        g_string_append(indexHtml, "{\"inspector_url\":\"inspector.html?page=1\"}");
        g_string_append_printf(response, "HTTP/1.0 200 OK\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s", indexHtml->len, indexHtml->str);

        g_socket_send(clientSocket, response->str, response->len, 0, &error);
        if (error) {
            LOG_ERROR("Error : %s(%d)", error->message, error->code);
            g_error_free(error);
            error = 0;
        }
        g_string_free (indexHtml, TRUE);
    } else {
        using namespace std;
        int length;
        String contentType;
        String InspectorPath = WK2_WEB_INSPECTOR_INSTALL_DIR;

        InspectorPath.append(path);

        std::ifstream fin;
        fin.open(InspectorPath.utf8().data(), ifstream::in|ifstream::binary);

        if (fin.good()) {
            char* fileBuffer = 0;
            // get length of file:
            fin.seekg(0, ios::end);
            length = fin.tellg();
            fin.seekg(0, ios::beg);

            fileBuffer = static_cast<char*>(malloc(length));
            // read data as a block
            fin.read(fileBuffer, length);
            fin.close();

            if (InspectorPath.endsWith(".html"))
                contentType = "text/html";
            else if (InspectorPath.endsWith(".js"))
                contentType = "application/javascript";
            else if (InspectorPath.endsWith(".css"))
                contentType = "text/css";
            else if (InspectorPath.endsWith(".png"))
                contentType = "image/png";
            else if (InspectorPath.endsWith(".gif"))
                contentType = "image/gif";
            else
                contentType = "text/plain";

            g_string_append_printf(response, "HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", contentType.ascii().data(), length);
            g_socket_send(clientSocket, response->str, response->len, 0, &error);
            if (error) {
                LOG_ERROR("Error : %s(%d)", error->message, error->code);
                g_error_free(error);
                error = 0;
            }

            int sent = 0;
            int ret;
            while (sent < length) {
                if (0 < (ret = g_socket_send(clientSocket,
                                  fileBuffer + sent,
                                  (gsize)length - sent,
                                  0, &error))) {
                    if (error) {
                        LOG_ERROR("Error : %s(%d)", error->message, error->code);
                        g_error_free(error);
                        error = 0;
                    }
                    sent += ret;
                    continue;
                }
                break;
            }
            free(fileBuffer);
        } else {
            g_string_append(response, "HTTP/1.0 404 Not Found\r\n");
            g_socket_send(clientSocket, response->str, response->len, 0, &error);
            if (error) {
                LOG_ERROR("Error : %s(%d)", error->message, error->code);
                g_error_free(error);
                error = 0;
            }
        }
        fin.close();
    }
    g_string_free (response, TRUE);
    g_socket_close(clientSocket, 0);

    return TRUE;
}

gboolean WebInspectorServerEfl::onWebSocketRequest(GIOChannel *source, GIOCondition condition, gpointer data)
{
    WebInspectorServerEfl *server = (WebInspectorServerEfl*)data;
    if (!server)
        return FALSE;

    char socketBuffer[socketBufferSize] = {0};
    char* startFrame = 0;
    char* bufferEnd = 0;
    GError *error = 0;

    gssize len = g_socket_receive(server->m_clientSocket, socketBuffer, socketBufferSize, 0, &error);
    if (error) {
        LOG_ERROR("Error : %s(%d)", error->message, error->code);
        g_error_free(error);
        error = 0;
    }

    // Closing the connection
    if (!len || len == -1) {
        server->webSocketDisconnected();
        return TRUE;
    }

    if (!server->m_useHixie76Protocol) {
        // Apppend to buffer
        char* newBuffer = 0;
        int newBufferSize = server->m_bufferSize + len;

        newBuffer = static_cast<char*>(malloc(newBufferSize));
        if (server->m_buffer)
            memcpy(newBuffer, server->m_buffer, server->m_bufferSize);
        memcpy(newBuffer + server->m_bufferSize, socketBuffer, len);
        free(server->m_buffer);
        server->m_buffer = newBuffer;
        server->m_bufferSize = newBufferSize;

        while (server->m_bufferSize > 0) {
            startFrame = server->m_buffer;
            bufferEnd = server->m_buffer + server->m_bufferSize;

            // Checking for firstByte and secondByte
            if (server->m_bufferSize < 2)
                return FALSE;

            startFrame++;

            unsigned char secondByte = *startFrame++;
            bool masked = secondByte & 0x80; //Mask Bit
            uint64_t payloadLength64 = secondByte & 0x7F; // Payload length
            if (payloadLength64 > maxPayloadLengthWithoutExtendedLengthField) {
                int extendedPayloadLengthSize = 0;
                if (payloadLength64 == payloadLengthWithTwoByteExtendedLengthField)
                    extendedPayloadLengthSize = 2;
                else {
                    ASSERT(payloadLength64 == payloadLengthWithEightByteExtendedLengthField);
                    extendedPayloadLengthSize = 8;
                }

                if (bufferEnd - startFrame < extendedPayloadLengthSize)
                    return FALSE;

                payloadLength64 = 0;
                for (int i = 0; i < extendedPayloadLengthSize; ++i) {
                    payloadLength64 <<= 8;
                    payloadLength64 |= static_cast<unsigned char>(*startFrame++);
                }
            }

            size_t maskingKeyLength = masked ? maskingKeyWidthInBytes : 0;
            if (payloadLength64 + maskingKeyLength > numeric_limits<size_t>::max())
                return FALSE;

            size_t payloadLength = static_cast<size_t>(payloadLength64);
            if (static_cast<size_t>(bufferEnd - startFrame) < maskingKeyLength + payloadLength)
                return FALSE;

            if (masked) {
                const char* maskingKey = startFrame;
                char* payload = const_cast<char*>(startFrame + maskingKeyWidthInBytes);
                for (size_t i = 0; i < payloadLength; ++i)
                    payload[i] ^= maskingKey[i % maskingKeyWidthInBytes]; // Unmask the payload.
            }

            String message = String::fromUTF8(startFrame + maskingKeyLength, payloadLength);
            WebCore::Page* page = server->m_inspectorClient->m_page->corePage();
            if (page) {
                WebCore::InspectorController* inspectorController = page->inspectorController();
                inspectorController->dispatchMessageFromFrontend(message);
            }

            int count = (startFrame + maskingKeyLength + payloadLength) - server->m_buffer;
            server->m_bufferSize = server->m_bufferSize - count;

            // Skip the buffer
            if (!server->m_bufferSize) {
                free(server->m_buffer);
                server->m_buffer = 0;
            } else
                memmove(server->m_buffer, server->m_buffer + count, server->m_bufferSize);
        }
    } else {
        // FIXME : Add to append buffer
        char* endFrame;
        int count = 0;
        startFrame = socketBuffer;

        while (len > 0) {
            // Start of WebSocket frame is indicated by 0x00
            if (startFrame[0])
             return FALSE;

            // End of WebSocket frame indicated by 0xFF
            endFrame = (char*)memchr(startFrame, 0xFF, len);
            if (!endFrame)
             return FALSE;
            count = endFrame -startFrame + 1;

            String message = String::fromUTF8(startFrame + 1, count - 2);
            WebCore::Page* page = server->m_inspectorClient->m_page->corePage();
            if (page) {
                WebCore::InspectorController* inspectorController = page->inspectorController();
                inspectorController->dispatchMessageFromFrontend(message);
            }

            len = len - count;
            if (len)
             startFrame = endFrame + 1;
        }
    }

    return TRUE;
}
#endif // GLIB_SUPPORT

}

#endif
