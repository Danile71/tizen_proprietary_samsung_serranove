/*
 * Copyright (C) 2012 Samsung Electronics
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

#if ENABLE(TIZEN_SEARCH_PROVIDER)

#include "External.h"

#include "Chrome.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "Page.h"
#include "Settings.h"

namespace WebCore {

External::External(Frame* frame)
    : DOMWindowProperty(frame)
{
}

External::~External()
{
}

void External::addSearchProvider(const String& engineURL)
{
    if(!m_frame || !m_frame->document())
        return;

    String baseURL = m_frame->document()->baseURL().baseAsString();

    Page* page = m_frame->page();
    if(!page)
        return;

    page->chrome()->addSearchProvider(baseURL, engineURL);
}

unsigned long External::isSearchProviderInstalled(const String& engineURL)
{
    if(!m_frame || !m_frame->document())
        return 0;

    String baseURL = m_frame->document()->baseURL().baseAsString();

    Page* page = m_frame->page();
    if (!page)
        return 0;

    return page->chrome()->isSearchProviderInstalled(baseURL, engineURL);
}

} // namespace WebCore

#endif // ENABLE(TIZEN_SEARCH_PROVIDER)
