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
 */

#ifndef WebInspectorServerEfl_h
#define WebInspectorServerEfl_h

#if ENABLE(TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR)

#if ENABLE(GLIB_SUPPORT)
#include <glib.h>
#include <gio/gio.h>
#endif
#if USE(SOUP)
#include <libsoup/soup.h>
#endif
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebKit {
class WebInspectorClient;

class WebInspectorServerEfl {
public:
    static WebInspectorServerEfl* server();
    bool startServer(unsigned int);
    bool stopServer();
    unsigned int getServerPort();

    void registerClient(WebInspectorClient*);
    void unregisterClient(WebInspectorClient*);
    WebInspectorClient* inspectorClientForPage(unsigned);

    void webSocketConnected();
    void webSocketDisconnected();
    bool webSocketSend(const String&);

#if ENABLE(GLIB_SUPPORT)
    static gboolean onHttpRequest(GIOChannel *source, GIOCondition condition, gpointer data);
    static gboolean onWebSocketRequest(GIOChannel *source, GIOCondition condition, gpointer data);
#endif

private:
    WebInspectorServerEfl();
    ~WebInspectorServerEfl();

    HashMap<unsigned, WebInspectorClient*> m_clientMap;
    unsigned m_pageNumber;
    WebInspectorClient* m_inspectorClient;
    char* m_buffer;
    int m_bufferSize;
    bool m_useHixie76Protocol;
#if ENABLE(GLIB_SUPPORT)
    GSocket *m_serverSocket;
    GIOChannel *m_serverChannel;
    guint m_serverSource;
    // FIXME : Not support multi-connection
    GSocket *m_clientSocket;
    GIOChannel *m_clientChannel;
    guint m_clientSource;
 #endif
};

}

#endif // TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR
#endif // WebInspectorServerEfl_h
