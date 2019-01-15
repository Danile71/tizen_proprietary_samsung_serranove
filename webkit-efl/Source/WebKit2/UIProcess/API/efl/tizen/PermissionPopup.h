/*
   Copyright (C) 2014 Samsung Electronics

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

#ifndef PermissionPopup_h
#define PermissionPopup_h

namespace WebKit {

class PermissionPopup {
public:
    PermissionPopup(const Ewk_Security_Origin* origin, const String& message)
    {
        m_origin = origin;
        m_popupMessage = message;
    }
    virtual ~PermissionPopup() { }

    String getMessage() { return m_popupMessage; }
    String getOriginHost() { return String::fromUTF8(ewk_security_origin_host_get(m_origin)); }
    virtual void sendDecidedPermission(Evas_Object*, bool isDecided) = 0;

private:
    const Ewk_Security_Origin* m_origin;
    String m_popupMessage;
};

} // namespace WebKit

#endif // PermissionPopup_h
