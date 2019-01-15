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

#ifndef __UCProxy__CookieStorageAccessorImpl__
#define __UCProxy__CookieStorageAccessorImpl__

#include <UCProxySDK/CookieStorageAccessor.h>
#include <libsoup/soup-cookie-jar-sqlite.h>
#include <string>

namespace proxy {

class CookieStorageAccessorImpl : public CookieStorageAccessor
{
public:
    CookieStorageAccessorImpl();
    ~CookieStorageAccessorImpl();

    /*************************************************************************************************
     * @Function name:  setCookies
     *
     * Detailed description:  Set cookie for url.
     * @param[in] url     : url of cookie
     * @param[in] cookies : cookies
     * @return bool       : if operation succeeds
     *************************************************************************************************/
    bool setCookies(const std::string& url, const std::string& cookies);
    
    /*************************************************************************************************
     * @Function name:  getCookies
     *
     * Detailed description: get cookies for specfied domain and sub domain
     * @param[in] url
     * @param[in] cookies
     * @param[in] fillExtraInfo : if extra info required to fill, default is false
     * @return bool
     *************************************************************************************************/
    bool getCookies(const std::string& url, std::string& cookies, bool fillExtraInfo = false) const;
    
private:
    std::string formatString(const char* fmt, ...) const;
    SoupCookieJar* m_cookieJar;

};
    
}// namespace Proxy

#endif /* defined(__UCProxy__CookieStorageAccessorImpl__) */
