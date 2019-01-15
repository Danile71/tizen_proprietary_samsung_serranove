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

#include "config.h"
#include "WebContextMenuProxyTizen.h"

#include "WebContextMenuItemData.h"
#include "WebPageProxy.h"
#include "ewk_view_private.h"

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
#include <Elementary.h>
#include <WebCore/EflScreenUtilities.h>
#include "ewk_util.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
#include "ClipboardHelper.h"
#endif

#include <WebCore/ContextMenu.h>
#include <WebCore/NotImplemented.h>
#include <wtf/text/CString.h>

#include <stdio.h>

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
#include <dlfcn.h>
#include <efl_assist.h>
extern void* EflAssistHandle;
#endif

using namespace WebCore;

namespace WebKit {
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
ListHashSet<WebContextMenuProxyTizen *> contextMenuProxyList;

static int s_appendedItemSize = 0;
static int s_popupItemHeight = 96;
static Vector<WebContextMenuItemData> s_contextMenulistData;
static bool s_contextMenuResized = false;
static const int s_popupMaxHeightForVertical = 680;
static const int s_popupMaxHeightForHorizontal = 360;
#endif

WebContextMenuProxyTizen::WebContextMenuProxyTizen(Evas_Object* webView, WebPageProxy* page, PageClientImpl* pageClientImpl)
    : m_page(page)
    , m_pageClientImpl(pageClientImpl)
    , m_popupPosition()
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    , m_widgetWin(0)
    , m_topWidget(0)
    , m_popup(0)
    , m_webView(webView)
    , m_items()
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
    , m_positionForSelection()
#endif
{
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    contextMenuProxyList.add(this);
#endif
}

WebContextMenuProxyTizen::~WebContextMenuProxyTizen()
{
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    TIZEN_LOGI("WebContextMenuProxyTizen [%p]", this);
    deletePopup();
    contextMenuProxyList.remove(this);
#endif

}

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
void WebContextMenuProxyTizen::contextMenuListTrackResize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    int height;
    evas_object_geometry_get(obj, 0, 0, 0, &height);

    if (s_popupItemHeight == height)
        return;

    s_popupItemHeight = height;
    contextMenuPopupBoxResize(elm_object_top_widget_get(static_cast<Evas_Object*>(data)), data);
}

void WebContextMenuProxyTizen::contextMenuListRealized(void* data, Evas_Object* obj, void* event_info)
{
    if (!event_info)
        return;

    Elm_Object_Item* item = static_cast<Elm_Object_Item*>(event_info);
    int index = (int)elm_object_item_data_get(item);

    Evas_Object *track = elm_object_item_track(item);
    evas_object_event_callback_add(track, EVAS_CALLBACK_RESIZE, contextMenuListTrackResize, obj);

    if (index == s_appendedItemSize - 1)
        elm_object_item_signal_emit(item, "elm,state,bottomline,hide", "");

}

char* WebContextMenuProxyTizen::contextMenuGenlistTextSet(void* data, Evas_Object* obj, const char* part)
{
    if (!strcmp(part, "elm.text.main.left")) {
        int index = (int)data;
        if (index >= s_appendedItemSize) {
            TIZEN_LOGI("Invaild index!!! - index : %d, s_appendedItemSize : [%d], s_contextMenulistData.size() : [%d]", index, s_appendedItemSize, s_contextMenulistData.size());
            return 0;
        }

        if (s_contextMenulistData.at(index).title().isEmpty()) {
            TIZEN_LOGI("Title is empty!!! - index : %d, action : %d", index, s_contextMenulistData.at(index).action());
            return 0;
        }

        char* label = new char[s_contextMenulistData.at(index).title().utf8().length() + 1];
        strcpy(label, s_contextMenulistData.at(index).title().utf8().data());
        return label;
    } else
        return 0;
}

void WebContextMenuProxyTizen::contextMenuPopupBoxResize(Evas_Object* topWidget, void* data)
{
    s_contextMenuResized = true;

    Evas_Object* box = elm_object_parent_widget_get(static_cast<Evas_Object*>(data));

    int rotation= elm_win_rotation_get(topWidget);
    if ((rotation == 90 || rotation == 270) && s_appendedItemSize > 3)
        evas_object_size_hint_min_set(box, 0, s_popupMaxHeightForHorizontal);
    else if ((rotation == 0 || rotation == 180) && s_appendedItemSize > 7)
        evas_object_size_hint_min_set(box, 0, s_popupMaxHeightForVertical);
    else
        evas_object_size_hint_min_set(box, 0, s_appendedItemSize * s_popupItemHeight);

    evas_object_show(box);
}

void WebContextMenuProxyTizen::contextMenuPopupFocusInCallback(void* data, Evas_Object* obj, void* event_info)
{
    TIZEN_LOGI("[CALLBACK_CALLED] widget_window = %p, contextMenuPopupFocusInCallback = %p,data = %p",
                obj, contextMenuPopupFocusInCallback, data);
    WebContextMenuItemData itemData = *(static_cast<WebContextMenuItemData*>(data));
    WebContextMenuProxyTizen* menuProxy = static_cast<WebContextMenuProxyTizen*>(evas_object_data_get(obj, "WebContextMenuProxyTizen"));

    if (!contextMenuProxyList.contains(menuProxy)) {
        TIZEN_LOGE("contextMenuProxyList doesn't have this instance");
        return;
    }

    evas_object_smart_callback_del(obj, "focus,in", contextMenuPopupFocusInCallback);
    TIZEN_LOGI("[CALLBACK_DELETED] widget_window = %p, contextMenuPopupFocusInCallback = %p, data = %p",
                obj, contextMenuPopupFocusInCallback, data);

    if (!menuProxy) {
        TIZEN_LOGI("menuProxy is null!!!");
        return;
    }

    if (!menuProxy->m_page || !menuProxy->m_page->isValid()) {
        TIZEN_LOGI("page is invalid!!!");
        return;
    }

    menuProxy->m_page->contextMenuItemSelected(itemData);
}

void WebContextMenuProxyTizen::contextMenuPopupRotationCallback(void* data, Evas_Object* obj, void* event_info)
{
    contextMenuPopupBoxResize(obj, data);
}

void WebContextMenuProxyTizen::contextMenuPopupResize(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    if (!s_contextMenuResized)
        contextMenuPopupBoxResize(elm_object_top_widget_get(obj), data);
}

void WebContextMenuProxyTizen::contextMenuItemSelectedCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    WebContextMenuItemData itemData = *(static_cast<WebContextMenuItemData*>(data));
    WebContextMenuProxyTizen* menuProxy = static_cast<WebContextMenuProxyTizen*>(evas_object_data_get(obj, "WebContextMenuProxyTizen"));

    if (!contextMenuProxyList.contains(menuProxy)) {
        TIZEN_LOGE("contextMenuProxyList doesn't have this instance");
        return;
    }

    if (!menuProxy) {
        TIZEN_LOGI("menuProxy is null!!!");
        return;
    }

    if (!menuProxy->m_page || !menuProxy->m_page->isValid()) {
        TIZEN_LOGI("page is invalid!!!");
        return;
    }

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    menuProxy->m_pageClientImpl->viewImpl()->focusRing()->hide();
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    menuProxy->m_pageClientImpl->viewImpl()->textSelection()->initHandlesMouseDownedStatus();

    if (menuProxy->m_isContextMenuForTextSelection) {
        bool isHandleIncluded = false;
        if ((itemData.action() == ContextMenuItemTagSelectAll) || (itemData.action() == ContextMenuItemTagSelectWord))
            isHandleIncluded = true;
        menuProxy->m_pageClientImpl->viewImpl()->textSelection()->updateVisible(false, isHandleIncluded);

        int selectionMode = menuProxy->m_pageClientImpl->viewImpl()->textSelection()->isTextSelectionMode();
        selectionMode = (selectionMode & TextSelection::ModeContextMenu) ? (selectionMode ^ TextSelection::ModeContextMenu) : selectionMode;
        menuProxy->m_pageClientImpl->viewImpl()->textSelection()->setIsTextSelectionMode(selectionMode);
    }
#endif
    if (itemData.action() == ContextMenuItemTagTextSelectionMode) {
        evas_object_smart_callback_add(menuProxy->getTopWidget(), "focus,in", contextMenuPopupFocusInCallback, data);
        TIZEN_LOGI("[CALLBACK_ADDED] widget_window = %p, contextMenuPopupFocusInCallback = %p, data = %p",
                    menuProxy->getTopWidget(), contextMenuPopupFocusInCallback, data);
    } else
        menuProxy->m_page->contextMenuItemSelected(itemData);

    menuProxy->hideContextMenu();
}

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
void WebContextMenuProxyTizen::contextMenuHwMoreBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    WebContextMenuProxyTizen* menuProxy = static_cast<WebContextMenuProxyTizen*>(data);
    if (!menuProxy)
        return;

    if (menuProxy->m_isContextMenuForTextSelection) {
        elm_ctxpopup_dismiss(obj);
        return;
    }

    menuProxy->hideContextMenu();
}
#endif

void WebContextMenuProxyTizen::languageChangedCallback(void* data, Evas_Object*, void*)
{
    WebContextMenuProxyTizen* menuProxy = static_cast<WebContextMenuProxyTizen*>(data);

    if (!menuProxy)
        return;

    menuProxy->hideContextMenu();
}

void WebContextMenuProxyTizen::blockClickedCallback(void* data, Evas_Object*, void*)
{
    WebContextMenuProxyTizen* menuProxy = static_cast<WebContextMenuProxyTizen*>(data);

    if (!menuProxy)
        return;

    menuProxy->hideContextMenu();
}

void WebContextMenuProxyTizen::createEflMenu(const Vector<WebContextMenuItemData>& items)
{
    deletePopup();

    m_items = items;
    size_t size = m_items.size();
#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
    int clipboardItemSize = ClipboardHelper::numberOfItems();
    TIZEN_LOGI("clipboardItemSize : %d", clipboardItemSize);
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    m_isContextMenuForTextSelection = false;
#endif

    for (size_t i = 0; i < size; i++) {
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (m_items.at(i).action() == ContextMenuItemTagCopy || m_items.at(i).action() == ContextMenuItemTagSelectAll
            || m_items.at(i).action() == ContextMenuItemTagSelectWord || m_items.at(i).action() == ContextMenuItemTagPaste) {
            m_isContextMenuForTextSelection = true;
            break;
        }
#endif
    }

    m_topWidget = elm_object_parent_widget_get(m_webView);
    if (!m_isContextMenuForTextSelection)
        m_topWidget = elm_object_top_widget_get(elm_object_parent_widget_get(m_webView));

    if (!m_topWidget)
        m_topWidget = m_webView;

    if (m_isContextMenuForTextSelection) {
        m_popup = elm_ctxpopup_add(m_topWidget);
        elm_ctxpopup_horizontal_set(m_popup, EINA_TRUE);
        elm_object_tree_focus_allow_set(m_popup, false);
    } else {
        evas_object_data_set(m_topWidget, "WebContextMenuProxyTizen", this);
        m_widgetWin = elm_win_add(m_topWidget, "WebKit Center Popup", ELM_WIN_DIALOG_BASIC);
        if (!m_widgetWin)
            return;

#if ENABLE(TIZEN_FOCUS_UI)
        if (m_page->focusUIEnabled())
            elm_win_prop_focus_skip_set(m_widgetWin, EINA_TRUE);
#endif

        elm_win_alpha_set(m_widgetWin, EINA_TRUE);
        ecore_x_icccm_name_class_set(elm_win_xwindow_get(m_widgetWin), "APP_POPUP", "APP_POPUP");

        if (elm_win_wm_rotation_supported_get(m_topWidget)) {
            int preferredRotation = elm_win_wm_rotation_preferred_rotation_get(m_topWidget);
            if (preferredRotation == -1) {
                int rots[4] = {0, 90, 180, 270};
                elm_win_wm_rotation_available_rotations_set(m_widgetWin, rots, 4);
            } else
                elm_win_wm_rotation_available_rotations_set(m_widgetWin, &preferredRotation, 1);
        }

        m_popup = elm_popup_add(m_widgetWin);
    }

    if (!m_popup)
        return;

    if (!m_isContextMenuForTextSelection)
        elm_popup_align_set(m_popup, ELM_NOTIFY_ALIGN_FILL, 1.0);

    evas_object_data_set(m_popup, "WebContextMenuProxyTizen", this);

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    bool isOnlyImageItemInClipboard = m_pageClientImpl->isPastedItemOnlyImage();
#endif

    s_appendedItemSize = 0;
    if (m_isContextMenuForTextSelection) {
        for (size_t i = 0; i < size; i++) {
            Elm_Object_Item* appendedItem = 0;
#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
            if ((m_items.at(i).action() == ContextMenuItemTagPaste || m_items.at(i).action() == ContextMenuItemTagClipboard) && !clipboardItemSize)
                continue;
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
            if (m_items.at(i).action() == ContextMenuItemTagPaste && isOnlyImageItemInClipboard)
                continue;
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_ICON_TYPE_SUPPORT)
            if (!m_items.at(i).title().isEmpty() || !m_items.at(i).iconFile().isEmpty()) {
                Evas_Object* icon = 0;
                if (m_items.at(i).iconFile().isEmpty()) {
                    //For webkit default supported context menu
                    String theme = String::fromUTF8((elm_theme_get(NULL)));
                    theme = theme.left(theme.find(":"));
                    char* themePath = elm_theme_list_item_path_get(theme.utf8().data(), NULL);
                    String iconPath = String::fromUTF8(themePath);
                    if (themePath)
                        free(themePath);

                    if (!iconPath.isEmpty()) {
                        icon = elm_image_add(m_popup);
                        bool isSucceed = false;

                        switch (m_items.at(i).action()) {
                        case ContextMenuItemTagSelectAll:
                            isSucceed = elm_image_file_set(icon, iconPath.utf8().data(), "elm/copypaste/select_all/default");
                            break;
                        case ContextMenuItemTagSelectWord:
                            isSucceed = elm_image_file_set(icon, iconPath.utf8().data(), "elm/copypaste/select/default");
                            break;
                        case ContextMenuItemTagPaste:
                            isSucceed = elm_image_file_set(icon, iconPath.utf8().data(), "elm/copypaste/paste/default");
                            break;
                        case ContextMenuItemTagCopy:
                            isSucceed = elm_image_file_set(icon, iconPath.utf8().data(), "elm/copypaste/copy/default");
                            break;
                        case ContextMenuItemTagCut:
                            isSucceed = elm_image_file_set(icon, iconPath.utf8().data(), "elm/copypaste/cut/default");
                            break;
                        case ContextMenuItemTagClipboard:
                            isSucceed = elm_image_file_set(icon, iconPath.utf8().data(), "elm/copypaste/clipboard/default");
                            break;
                        case ContextMenuItemTagSearchWeb:
                            isSucceed = elm_image_file_set(icon, iconPath.utf8().data(), "elm/copypaste/search/default");
                            break;
                        default:
                            break;
                        }

                        if (!isSucceed) {
                            TIZEN_LOGE("elm_image_file_set is failed");
                            evas_object_del(icon);
                            icon = 0;
                        }
                    }
                } else {
                   //For custom context menu
                    String iconPath = m_items.at(i).iconFile();
                    icon = elm_image_add(m_popup);

                    if (!elm_image_file_set(icon, iconPath.utf8().data(), 0)) {
                        TIZEN_LOGE("elm_image_file_set is failed");
                        evas_object_del(icon);
                        icon = 0;
                    }
                }

                if (!m_items.at(i).title().isEmpty())
                    appendedItem = elm_ctxpopup_item_append(m_popup, m_items.at(i).title().utf8().data(), icon, contextMenuItemSelectedCallback, &(m_items.at(i)));
                else
                    appendedItem = elm_ctxpopup_item_append(m_popup, 0, icon, contextMenuItemSelectedCallback, &(m_items.at(i)));
            }
#else
            if (!m_items.at(i).title().isEmpty())
                appendedItem = elm_ctxpopup_item_append(m_popup, m_items.at(i).title().utf8().data(), 0, contextMenuItemSelectedCallback, &(m_items.at(i)));
#endif

            if (appendedItem)
                s_appendedItemSize++;
        }
        elm_object_style_set(m_popup,"copypaste");
    } else {
        String selectedLink = m_page->contextMenuAbsoluteLinkURLString();
        Vector<String> linkSplit;
        if (!selectedLink.isEmpty()) {
            if (selectedLink.startsWith(String::fromUTF8("mailto:"), false) || selectedLink.startsWith(String::fromUTF8("tel:"), false)) {
                selectedLink.split(':', linkSplit);
                if (!linkSplit[1].isNull()) {
                    Vector<String> realLinkSplit;
                    linkSplit[1].split('?', realLinkSplit);
                    elm_object_part_text_set(m_popup, "title,text", realLinkSplit[0].utf8().data());
                }
            } else
                elm_object_part_text_set(m_popup, "title,text", selectedLink.utf8().data());
        } else {
            String selectedImage = m_page->contextMenuAbsoluteImageURLString();
            if (selectedImage.startsWith(String::fromUTF8("file:///"), false)) {
                selectedImage.split('/', linkSplit);
                if (linkSplit.size() && !linkSplit[linkSplit.size()-1].isNull())
                    selectedImage = linkSplit[linkSplit.size()-1];
            }
            if (!selectedImage.isEmpty())
                elm_object_part_text_set(m_popup, "title,text", selectedImage.utf8().data());
        }

        evas_object_size_hint_weight_set(m_popup, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
        evas_object_resize(m_widgetWin, WebCore::getDefaultScreenResolution().width(), WebCore::getDefaultScreenResolution().height());

        static Elm_Genlist_Item_Class m_contextMenulistItem;

        m_contextMenulistItem.item_style = "1line";
        m_contextMenulistItem.func.text_get = contextMenuGenlistTextSet;
        m_contextMenulistItem.func.content_get = 0;
        m_contextMenulistItem.func.state_get = 0;
        m_contextMenulistItem.func.del = 0;

        Evas_Object* genlist = elm_genlist_add(m_popup);
        elm_object_style_set(m_popup, "theme_bg");
        evas_object_data_set(genlist, "WebContextMenuProxyTizen", this);
        evas_object_size_hint_align_set(genlist, EVAS_HINT_FILL, EVAS_HINT_FILL);
        evas_object_smart_callback_add(genlist, "realized", contextMenuListRealized, NULL);
        elm_scroller_content_min_limit(genlist, EINA_FALSE, EINA_TRUE);

        s_contextMenulistData.clear();
        for (size_t i = 0; i < m_items.size(); i++) {
#if ENABLE(TIZEN_FOCUS_UI)
            if (m_items.at(i).action() == ContextMenuItemTagTextSelectionMode && m_page->focusUIEnabled())
                continue;
#endif
            if (!m_items.at(i).title().isEmpty()) {
                Elm_Object_Item* appendedItem = 0;
                appendedItem = elm_genlist_item_append(genlist, &m_contextMenulistItem, (void *) s_appendedItemSize, NULL, ELM_GENLIST_ITEM_NONE, contextMenuItemSelectedCallback, &(m_items.at(i)));

                if (appendedItem) {
                    s_contextMenulistData.append(m_items.at(i));
                    s_appendedItemSize++;
                }
            }
        }

        evas_object_show(genlist);
        elm_object_content_set(m_popup, genlist);
        evas_object_event_callback_add(m_popup, EVAS_CALLBACK_RESIZE, contextMenuPopupResize, genlist);
        evas_object_smart_callback_add(m_widgetWin, "wm,rotation,changed", contextMenuPopupRotationCallback, genlist);
    }

    TIZEN_LOGI("s_appendedItemSize : %d", s_appendedItemSize);
    if (!s_appendedItemSize) {
        deletePopup();
        m_popup = 0;
        return;
    }
}

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
void WebContextMenuProxyTizen::contextMenuPopupDismissedCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    WebContextMenuProxyTizen* menuProxy = static_cast<WebContextMenuProxyTizen*>(data);
    if (menuProxy) {
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        menuProxy->m_pageClientImpl->viewImpl()->focusRing()->hide();
#endif
        menuProxy->m_pageClientImpl->setIsContextMenuVisible(false);
    }
}
#endif

#else
void WebContextMenuProxyTizen::createEflMenu()
{
    notImplemented();
}
#endif

void WebContextMenuProxyTizen::showContextMenu(const WebCore::IntPoint& position, const Vector<WebContextMenuItemData>& items)
{
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
    m_positionForSelection = position;
#endif
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    if (items.isEmpty())
        return;

    createEflMenu(items);

    if (!m_popup)
        return;

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (m_isContextMenuForTextSelection && !evas_object_focus_get(m_webView)) {
        deletePopup();
        return;
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (m_isContextMenuForTextSelection) {
        int webViewX, webViewY;
        evas_object_geometry_get(m_webView, &webViewX, &webViewY, 0, 0);
        IntPoint popupPosition = position;
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
        if (m_pageClientImpl) {
            popupPosition.scale(m_pageClientImpl->scaleFactor(), m_pageClientImpl->scaleFactor());
            IntPoint scrollPosition = m_pageClientImpl->scrollPosition();
            popupPosition.move(-scrollPosition.x(), -scrollPosition.y());
        }
#endif
        popupPosition.setX(popupPosition.x() + webViewX);
        popupPosition.setY(popupPosition.y() + webViewY);

        // Notify the operation of ContextMenu to Email
        Evas_Point point;
        point.x = popupPosition.x();
        point.y = popupPosition.y();

        evas_object_smart_callback_call(m_webView, "contextmenu,willshow", &point);

        bool allowed = true;
        evas_object_smart_callback_call(m_webView, "contextmenu,allowed", &allowed);
        if (!allowed) {
            deletePopup();
            return;
        }

        IntPoint webViewPoint(webViewX,webViewY);
        int drawDirection;
        m_pageClientImpl->viewImpl()->textSelection()->changeContextMenuPosition(popupPosition, webViewPoint, drawDirection);
        switch(drawDirection) {
        case 0:
            elm_ctxpopup_direction_priority_set(m_popup, ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_DOWN);
            break;
        case 1:
            elm_ctxpopup_direction_priority_set(m_popup, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_UP);
            break;
        case 2:
            elm_ctxpopup_direction_priority_set(m_popup, ELM_CTXPOPUP_DIRECTION_LEFT, ELM_CTXPOPUP_DIRECTION_LEFT, ELM_CTXPOPUP_DIRECTION_LEFT, ELM_CTXPOPUP_DIRECTION_LEFT);
            break;
        case 3:
            elm_ctxpopup_direction_priority_set(m_popup, ELM_CTXPOPUP_DIRECTION_RIGHT, ELM_CTXPOPUP_DIRECTION_RIGHT, ELM_CTXPOPUP_DIRECTION_RIGHT, ELM_CTXPOPUP_DIRECTION_RIGHT);
            break;
        default:
            elm_ctxpopup_direction_priority_set(m_popup, ELM_CTXPOPUP_DIRECTION_UP, ELM_CTXPOPUP_DIRECTION_DOWN, ELM_CTXPOPUP_DIRECTION_LEFT, ELM_CTXPOPUP_DIRECTION_RIGHT);
        };

        evas_object_move(m_popup, popupPosition.x(), popupPosition.y());
        evas_object_smart_callback_add(m_popup, "dismissed", contextMenuPopupDismissedCallback, this);
        elm_ctxpopup_auto_hide_disabled_set(m_popup, EINA_TRUE);
    } else {
#else
    {
#endif // closure of #if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
        m_pageClientImpl->viewImpl()->focusRing()->hide(); // No Need to show Focus ring for the normal context Menu
#endif

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
        if (EflAssistHandle) {
            void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
            webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
            (*webkit_ea_object_event_callback_add)(m_popup, EA_CALLBACK_BACK, contextMenuHwMoreBackKeyCallback, this);
            (*webkit_ea_object_event_callback_add)(m_popup, EA_CALLBACK_MORE, contextMenuHwMoreBackKeyCallback, this);
        }
#endif
        evas_object_show(m_widgetWin);
        evas_object_smart_callback_add(m_popup, "language,changed", languageChangedCallback, this);
        evas_object_smart_callback_add(m_popup, "block,clicked", blockClickedCallback, this);
    }
    evas_object_show(m_popup);
    m_pageClientImpl->setIsContextMenuVisible(true);

#if ENABLE(TOUCH_EVENTS) && ENABLE(TIZEN_GESTURE)
    // Cancel touch event when ContextMenu is shown.
    if (!m_isContextMenuForTextSelection)
        EwkViewImpl::fromEvasObject(m_webView)->feedTouchEventsByType(EWK_TOUCH_CANCEL);
#endif
#else
    createEflMenu();
    notImplemented();
#endif
}

void WebContextMenuProxyTizen::hideContextMenu()
{
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    if (m_popup)
        evas_object_hide(m_popup);

    if (m_widgetWin) {
        evas_object_hide(m_widgetWin);
        evas_object_smart_callback_del(m_widgetWin, "wm,rotation,changed", contextMenuPopupRotationCallback);
    }
    s_contextMenuResized = false;
    m_pageClientImpl->setIsContextMenuVisible(false);
#else
    notImplemented();
#endif
}

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
WebCore::IntPoint& WebContextMenuProxyTizen::positionForSelection()
{
    return m_positionForSelection;
}
#endif
void WebContextMenuProxyTizen::deletePopup()
{
    evas_object_data_set(m_topWidget, "WebContextMenuProxyTizen", 0);
    evas_object_data_set(m_popup, "WebContextMenuProxyTizen", 0);

    if (m_widgetWin) {
        if (!m_isContextMenuForTextSelection)
            evas_object_smart_callback_del(m_widgetWin, "wm,rotation,changed", contextMenuPopupRotationCallback);

        evas_object_smart_callback_del(m_widgetWin, "focus,in", contextMenuPopupFocusInCallback);
        TIZEN_LOGI("[CALLBACK_DELETED] widget_window = %p, contextMenuPopupFocusInCallback = %p",
                    m_widgetWin, contextMenuPopupFocusInCallback);
        evas_object_del(m_widgetWin);
        m_widgetWin = 0;
    }

    if (m_popup) {
        evas_object_del(m_popup);
        m_popup = 0;
    }
}

#if ENABLE(TIZEN_FOCUS_UI) && ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
void WebContextMenuProxyTizen::setCenterPopupFocusable()
{
    if (!m_widgetWin)
        return;

    elm_win_prop_focus_skip_set(m_widgetWin, EINA_FALSE);
    elm_win_activate(m_widgetWin);
}
#endif
} // namespace WebKit
