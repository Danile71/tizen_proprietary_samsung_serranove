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

#ifndef JavaScriptPopup_h
#define JavaScriptPopup_h

#if PLATFORM(TIZEN)

namespace WebKit {

class JavaScriptPopup {
public:
    JavaScriptPopup(Evas_Object* ewkView);
    ~JavaScriptPopup();

    bool alert(const char* message);
    bool confirm(const char* message);
    bool prompt(const char* message, const char* defaultValue);
#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
    bool beforeUnloadConfirmPanel(const char* message);
#endif
    Evas_Object* ewkView();
    Evas_Object* entry();
    void close();
    Evas_Object* getTopWidget() { return m_topWidget; }
    void hidePopup();
    Evas_Object* m_popup;
    String m_popupMessage;
    bool isConfirm() { return m_isConfirm; }
    void setIsConfirm(bool confirm) { m_isConfirm = confirm; }
    void setPromptReply(char* result) { m_promptReply = result;}
    char* getPromptReply() { return m_promptReply;}

private:
    Evas_Object* popupAdd();
    String getTitle();
    bool setLabelText(const char* message);

    Evas_Object* m_entry;
    Evas_Object* m_ewkView;
    Evas_Object* m_widgetWin;
    Evas_Object* m_topWidget;
    bool m_isConfirm;
    char* m_promptReply;
};

} // namespace WebKit
#endif // PLATFORM(TIZEN)

#endif // JavaScriptPopup_h
