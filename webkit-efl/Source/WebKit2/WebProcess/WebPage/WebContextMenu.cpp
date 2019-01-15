/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#if ENABLE(CONTEXT_MENUS)

#include "WebContextMenu.h"

#include "InjectedBundleHitTestResult.h"
#include "InjectedBundleUserMessageCoders.h"
#include "WebCoreArgumentCoders.h"
#include "WebHitTestResult.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/ContextMenu.h>
#include <WebCore/ContextMenuController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/Page.h>

using namespace WebCore;

namespace WebKit {

WebContextMenu::WebContextMenu(WebPage* page)
    : m_page(page)
{
}

WebContextMenu::~WebContextMenu()
{
}

void WebContextMenu::show()
{
#if ENABLE(CONTEXT_MENUS)
    ContextMenuController* controller = m_page->corePage()->contextMenuController();
    if (!controller)
        return;
    ContextMenu* menu = controller->contextMenu();
    if (!menu)
        return;
    Node* node = controller->hitTestResult().innerNonSharedNode();
    if (!node)
        return;
    Frame* frame = node->document()->frame();
    if (!frame)
        return;
    FrameView* view = frame->view();
    if (!view)
        return;

    // Give the bundle client a chance to process the menu.
#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
    const Vector<ContextMenuItem>& coreItems = menu->items();
#else
    Vector<ContextMenuItem> coreItems = contextMenuItemVector(menu->platformDescription());
#endif
    Vector<WebContextMenuItemData> proposedMenu = kitItems(coreItems, menu);
    Vector<WebContextMenuItemData> newMenu;
    RefPtr<APIObject> userData;
    RefPtr<InjectedBundleHitTestResult> hitTestResult = InjectedBundleHitTestResult::create(controller->hitTestResult());
    if (m_page->injectedBundleContextMenuClient().getCustomMenuFromDefaultItems(m_page, hitTestResult.get(), proposedMenu, newMenu, userData))
        proposedMenu = newMenu;

    WebHitTestResult::Data webHitTestResultData(controller->hitTestResult());

    // Mark the WebPage has having a shown context menu then notify the UIProcess.
    m_page->contextMenuShowing();

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    Vector<WebContextMenuItemData> newProposedMenu;
    size_t size = proposedMenu.size();
    for (size_t i = 0; i < size; i++) {
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_ICON_TYPE_SUPPORT)
        if (proposedMenu.at(i).title().isEmpty() && proposedMenu.at(i).iconFile().isEmpty())
#else
        if (proposedMenu.at(i).title().isEmpty())
#endif
            continue;
        newProposedMenu.append(proposedMenu.at(i));
    }

    //make context menu sequentially regarding TIZEN UX guidelines.
    Vector<WebContextMenuItemData> sortedProposedMenu;
    if (!sortProposedMenu(newProposedMenu, sortedProposedMenu)) {
        sortedProposedMenu.clear();
        sortedProposedMenu = newProposedMenu;
    }

    proposedMenu = sortedProposedMenu;
#endif
    m_page->send(Messages::WebPageProxy::ShowContextMenu(view->contentsToWindow(controller->hitTestResult().roundedPoint()), webHitTestResultData, proposedMenu, InjectedBundleUserMessageEncoder(userData.get())));
#endif
}

void WebContextMenu::itemSelected(const WebContextMenuItemData& item)
{
#if !PLATFORM(EFL) || ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    ContextMenuItem coreItem(ActionType, static_cast<ContextMenuAction>(item.action()), item.title());
    m_page->corePage()->contextMenuController()->contextMenuItemSelected(&coreItem);
#endif
}

#if ENABLE(TIZEN_CONTEXT_CLICK)
Vector<WebContextMenuItemData> WebContextMenu::items() const
{
    Vector<WebContextMenuItemData> proposedMenu;
#if ENABLE(CONTEXT_MENUS)
    ContextMenuController* controller = m_page->corePage()->contextMenuController();
    if (!controller)
        return proposedMenu;
    ContextMenu* menu = controller->contextMenu();
    if (!menu)
        return proposedMenu;

    // Give the bundle client a chance to process the menu.
#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
    const Vector<ContextMenuItem>& coreItems = menu->items();
#else
    Vector<ContextMenuItem> coreItems = contextMenuItemVector(menu->platformDescription());
#endif
    proposedMenu = kitItems(coreItems, menu);
    Vector<WebContextMenuItemData> newMenu;
    RefPtr<APIObject> userData;
    RefPtr<InjectedBundleHitTestResult> hitTestResult = InjectedBundleHitTestResult::create(controller->hitTestResult());
    if (m_page->injectedBundleContextMenuClient().getCustomMenuFromDefaultItems(m_page, hitTestResult.get(), proposedMenu, newMenu, userData))
        proposedMenu = newMenu;
#endif
    return proposedMenu;
}
#endif

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
void WebContextMenu::initContextMenuItemSequence(Vector<int>& contextMenuItemSequence)
{
    contextMenuItemSequence.append((int)ContextMenuItemTagSelectWord);
    contextMenuItemSequence.append((int)ContextMenuItemTagSelectAll);
    contextMenuItemSequence.append((int)ContextMenuItemTagCopy);
    contextMenuItemSequence.append((int)ContextMenuItemTagCut);
    contextMenuItemSequence.append((int)ContextMenuItemTagPaste);
    contextMenuItemSequence.append((int)ContextMenuItemTagClipboard);
    contextMenuItemSequence.append((int)ContextMenuItemTagSearchWeb);
}

bool WebContextMenu::sortProposedMenu(Vector<WebContextMenuItemData>& proposedMenu, Vector<WebContextMenuItemData>& sortedProposedMenu)
{
    Vector<int> contextMenuItemSequence;
    initContextMenuItemSequence(contextMenuItemSequence);

    Vector<WebContextMenuItemData> tempMenu;
    tempMenu = proposedMenu;

    size_t count = 0;
    for (size_t i = 0; i < contextMenuItemSequence.size(); i++) {
        if (count == proposedMenu.size())
            break;
        for (size_t j = 0; j < tempMenu.size(); j++) {
            if (contextMenuItemSequence.at(i) == tempMenu.at(j).action()) {
                sortedProposedMenu.append(tempMenu.at(j));
                tempMenu.remove(j);
                count++;
                break;
            }
        }
    }

    if (tempMenu.size()) {
        for (size_t i = 0; i < tempMenu.size(); i++)
            sortedProposedMenu.append(tempMenu.at(i));
    }

    if (sortedProposedMenu.size() != proposedMenu.size())
        return false;
    return true;
}
#endif

} // namespace WebKit

#endif // ENABLE(CONTEXT_MENUS)
