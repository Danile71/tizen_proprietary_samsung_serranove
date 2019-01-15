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

#include "config.h"
#include "ewk_history.h"

#if PLATFORM(TIZEN)
#include "WKAPICast.h"
#include "WKArray.h"
#include "WKBackForwardList.h"
#include "WKBackForwardListItem.h"
#include "WKRetainPtr.h"
#include "ewk_history_private.h"
#include <wtf/text/CString.h>

using namespace WebKit;

struct _Ewk_History {
    WKBackForwardListRef wkBackForwardList;

    Ewk_History_Item* item;
};

struct _Ewk_History_Item {
    WKRetainPtr<WKBackForwardListItemRef> itemRef;

    CString uri;
    CString title;
};

Ewk_History* ewkHistoryCreate(WKBackForwardListRef wkBackForwardList)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(wkBackForwardList, 0);

    Ewk_History* history = new Ewk_History;

    history->wkBackForwardList = wkBackForwardList;
    history->item = 0;

    return history;
}

int ewk_history_back_list_length_get(Ewk_History* history)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(history, 0);

    return WKBackForwardListGetBackListCount(history->wkBackForwardList);
}

int ewk_history_forward_list_length_get(Ewk_History* history)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(history, 0);

    return WKBackForwardListGetForwardListCount(history->wkBackForwardList);
}

Ewk_History_Item* ewk_history_nth_item_get(Ewk_History* history, int index)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(history, 0);

    if (history->item)
        delete history->item;

    WKBackForwardListItemRef itemRef = WKBackForwardListGetItemAtIndex(history->wkBackForwardList, index);
    if (!itemRef)
        return 0;

    history->item = new Ewk_History_Item;
    history->item->itemRef = itemRef;

    return history->item;
}

const char* ewk_history_item_uri_get(Ewk_History_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, 0);

    WKRetainPtr<WKURLRef> wkURI(AdoptWK, WKBackForwardListItemCopyURL(item->itemRef.get()));

    if (toImpl(wkURI.get())->string().isEmpty())
        return 0;

    item->uri = toImpl(wkURI.get())->string().utf8();
    return item->uri.data();
}

const char* ewk_history_item_title_get(Ewk_History_Item* item)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(item, 0);

    WKRetainPtr<WKStringRef> wkTitle(AdoptWK, WKBackForwardListItemCopyTitle(item->itemRef.get()));

    if (toImpl(wkTitle.get())->string().isEmpty())
        return 0;

    item->title = toImpl(wkTitle.get())->string().utf8();
    return item->title.data();
}

void ewk_history_free(Ewk_History* history)
{
    EINA_SAFETY_ON_NULL_RETURN(history);

    if (history->item)
        delete history->item;

    delete history;
}
#endif // #if PLATFORM(TIZEN)
