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

#ifndef PermissionPopupManager_h
#define PermissionPopupManager_h

namespace WebKit {

class PermissionPopup;

class PermissionPopupManager {
public:
    PermissionPopupManager(Evas_Object* );
    ~PermissionPopupManager();

    bool isDecided() { return m_isDecided; }
    Evas_Object* ewkView();
    Evas_Object* getTopWidget() { return m_topWidget; }
    PermissionPopup* getPermissionPopup();
    void addPermissionRequest(PermissionPopup*);
    void closePopup();
    void deletePermissionRequest(PermissionPopup*);
    void deleteAllPermissionRequest();
    void hidePopup();
    void setIsDecided(bool decided) { m_isDecided = decided; }

private:
    bool setLabelText(String message);
    Evas_Object* createPopup();
    String getTitle(PermissionPopup* popup);
    void showPermissionPopup(PermissionPopup*);

    bool m_isDecided;
    Eina_List* m_permissionPopups;
    Evas_Object* m_ewkView;
    Evas_Object* m_popup;
    Evas_Object* m_widgetWin;
    Evas_Object* m_topWidget;
};

} // namespace WebKit

#endif // PermissionPopupManager_h
