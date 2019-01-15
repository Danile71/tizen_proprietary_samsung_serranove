/*
 * Copyright (C) 2011 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebContextMenuProxyTizen_h
#define WebContextMenuProxyTizen_h

#include "PageClientImpl.h"
#include "WebContextMenuProxy.h"
#include <WebCore/IntPoint.h>

namespace WebKit {

class WebContextMenuItemData;
class WebPageProxy;

class WebContextMenuProxyTizen : public WebContextMenuProxy {
public:
    static PassRefPtr<WebContextMenuProxyTizen> create(Evas_Object* webView, WebPageProxy* page, PageClientImpl* pageClientImpl)
    {
        return adoptRef(new WebContextMenuProxyTizen(webView, page, pageClientImpl));
    }
    ~WebContextMenuProxyTizen();

    virtual void showContextMenu(const WebCore::IntPoint&, const Vector<WebContextMenuItemData>&);
    virtual void hideContextMenu();
#if ENABLE(TIZEN_FOCUS_UI) && ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    virtual void setCenterPopupFocusable();
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
    virtual WebCore::IntPoint& positionForSelection();
#endif
    Evas_Object* getTopWidget() { return m_topWidget; }

private:
    WebContextMenuProxyTizen(Evas_Object*, WebPageProxy*, PageClientImpl*);
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    static void contextMenuHwMoreBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo);
#endif
    static void contextMenuListTrackResize(void *data, Evas *e, Evas_Object *obj, void *event_info);
    static void contextMenuListRealized(void* data, Evas_Object* obj, void* event_info);
    static char* contextMenuGenlistTextSet(void* data, Evas_Object* obj, const char* part);
    static void contextMenuPopupBoxResize(Evas_Object* topWidget, void* data);
    static void contextMenuPopupFocusInCallback(void* data, Evas_Object* obj, void* event_info);
    static void contextMenuPopupRotationCallback(void* data, Evas_Object* obj, void* event_info);
    static void contextMenuPopupResize(void *data, Evas *e, Evas_Object *obj, void *event_info);
    static void contextMenuItemSelectedCallback(void* data, Evas_Object* obj, void* eventInfo);
    static void contextMenuPopupDismissedCallback(void* data, Evas_Object* obj, void* eventInfo);
    static void languageChangedCallback(void*, Evas_Object*, void*);
    static void blockClickedCallback(void*, Evas_Object*, void*);
    void deletePopup();

    void createEflMenu(const Vector<WebContextMenuItemData>&);
#else
    void createEflMenu();
#endif
    WebPageProxy* m_page;
    PageClientImpl* m_pageClientImpl;
    WebCore::IntPoint m_popupPosition;

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    Evas_Object* m_widgetWin;
    Evas_Object* m_topWidget;
    Evas_Object* m_popup;
    Evas_Object* m_webView;
    Vector<WebContextMenuItemData> m_items;
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
    WebCore::IntPoint m_positionForSelection;
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    bool m_isContextMenuForTextSelection;
#endif
};


} // namespace WebKit

#endif // WebContextMenuProxyTizen_h
