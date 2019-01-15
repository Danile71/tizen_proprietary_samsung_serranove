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
#include "CookieStorageAccessorImpl.h"

#include "Logging.h"
#include "ResourceHandle.h"

#include <libsoup/soup-cookie-jar-sqlite.h>

#include <stdarg.h>
#include <stdio.h>
#include <string>

namespace proxy {

CookieStorageAccessorImpl::CookieStorageAccessorImpl()
{
    SoupSession* session = WebCore::ResourceHandle::defaultSession();

    m_cookieJar = reinterpret_cast<SoupCookieJar*>(soup_session_get_feature(session, SOUP_TYPE_COOKIE_JAR));

    g_object_ref(m_cookieJar);
}

CookieStorageAccessorImpl::~CookieStorageAccessorImpl()
{
    g_object_unref(m_cookieJar);
    m_cookieJar = NULL;
}

bool CookieStorageAccessorImpl::setCookies(const std::string& url, const std::string& cookies)
{
    SoupURI* origin = soup_uri_new(url.c_str());
    SoupURI* firstParty = soup_uri_new(url.c_str());

    soup_cookie_jar_set_cookie_with_first_party(m_cookieJar, origin, firstParty, cookies.c_str());

    TIZEN_SECURE_LOGI("[Network] setCookies:: %s for url %s\n", cookies.c_str(), url.c_str());

    soup_uri_free(origin);
    soup_uri_free(firstParty);

    return true;
}

bool CookieStorageAccessorImpl::getCookies(const std::string& url, std::string& cookies, bool fillExtraInfo) const
{
    bool ret = false;

    if (!m_cookieJar) {
        TIZEN_LOGI("[Network] m_cookieJar is NULL");
        return false;
    }

    GSList* soup_cookies = soup_cookie_jar_all_cookies(m_cookieJar);
    if (!soup_cookies) {
        TIZEN_LOGI("[Network] fail to get jar cookies");
        return false;
    }

    SoupURI* soupURI = soup_uri_new(url.c_str());
    if (!soupURI) {
        TIZEN_LOGI("[Network] fail to create soupURI");
        return false;
    }

    for (GSList* iter = soup_cookies; iter; iter = g_slist_next(iter)) {
        SoupCookie* cookie = static_cast<SoupCookie*>(iter->data);

        if (!soup_cookie_domain_matches(cookie, soup_uri_get_host(soupURI)))
            continue;

        // just need to check secure & expire , not path part
        if (cookie->secure && soupURI->scheme != SOUP_URI_SCHEME_HTTPS)
            continue;

        if (cookie->expires && soup_date_is_past (cookie->expires))
            continue;

        ret = true;
        if (fillExtraInfo) {
           if (cookie->path && strlen(cookie->path) > 1) {
                cookies += formatString("%s=%s;domain=%s;path=%s; ", cookie->name, cookie->value, cookie->domain, cookie->path);
           } else {
                cookies += formatString("%s=%s;domain=%s; ", cookie->name, cookie->value, cookie->domain);
           }
        } else {
            cookies += formatString("%s=%s; ", cookie->name, cookie->value);
        }
    }

    TIZEN_SECURE_LOGI("[Network] getCookies:: %s for url %s\n", cookies.c_str(), url.c_str());

    soup_uri_free(soupURI);
    g_slist_free(soup_cookies);

    return ret;
}

std::string CookieStorageAccessorImpl::formatString(const char* fmt, ...) const
{
    int size = 512;
    char buffer[size];
    
    va_list vl;
    va_start(vl, fmt);
    int nsize = vsnprintf(buffer, size, fmt, vl);
    if (size<=nsize) {
        //fail on default buffer then try again
        char *bufferHeap = new char[nsize+1]; //+1 for /0
        va_end(vl);
        va_start(vl, fmt);
        nsize = vsnprintf(bufferHeap, nsize+1, fmt, vl);

        std::string ret(bufferHeap);
        va_end(vl);
        delete[] bufferHeap;
        
        return ret;
    }
    va_end(vl);
    
    std::string ret(buffer);
    
    return ret;
}

} // namespace proxy
