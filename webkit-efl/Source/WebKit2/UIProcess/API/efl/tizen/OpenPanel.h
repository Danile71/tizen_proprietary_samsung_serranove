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

#ifndef OpenPanel_h
#define OpenPanel_h

#if ENABLE(TIZEN_OPEN_PANEL)

namespace WebKit {

enum _OpenPanelType{
    OPEN_PANEL_IMAGE_TYPE,
    OPEN_PANEL_VIDEO_TYPE,
    OPEN_PANEL_AUDIO_TYPE,
    OPEN_PANEL_ALL_TYPE
};
typedef enum _OpenPanelType OpenPanelType;

enum _OpenPanelOperationType{
    OPEN_PANEL_PICK,
    OPEN_PANEL_CREATE_CONTENT
};
typedef enum _OpenPanelOperationType OpenPanelOperationType;

class OpenPanel {
public:
    OpenPanel(Evas_Object* ewkView);
    ~OpenPanel();
    bool openPanel(Evas_Object* ewkView, Eina_Bool allow_multiple_files, Eina_List *accepted_mime_types, const char* capture, void* userData);
    Evas_Object* ewkView();
    void close();

private:
    bool launchApplicationForType(OpenPanelOperationType operationType, OpenPanelType panelType, bool);
    bool launchFileSystem(OpenPanelType panelType, bool allow_multiple_files);

    Evas_Object* m_ewkView;
};

} // namespace WebKit

#endif // ENABLE(TIZEN_OPEN_PANEL)

#endif // OpenPanel_h
