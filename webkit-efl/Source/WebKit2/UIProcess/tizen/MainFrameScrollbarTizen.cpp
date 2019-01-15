/*
   Copyright (C) 2011 Samsung Electronics

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
#include "MainFrameScrollbarTizen.h"

#include "ewk_view.h"
#include <Evas.h>
#include <Edje.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
#if ENABLE(TIZEN_WEBKIT2_TILED_SCROLLBAR)

using namespace WebCore;

static const double s_minimumThumbSize = 0.02;

namespace WebKit {

PassRefPtr<MainFrameScrollbarTizen> MainFrameScrollbarTizen::createNativeScrollbar(Evas_Object* ewkView, ScrollbarOrientation orientation)
{
    return adoptRef(new MainFrameScrollbarTizen(ewkView, orientation));
}

MainFrameScrollbarTizen::MainFrameScrollbarTizen(Evas_Object* view, ScrollbarOrientation orientation)
    : m_view(view)
    , m_position(0)
    , m_visibleSize(0)
    , m_isEnabled(false)
{
    ASSERT(m_view);

    m_theme = ewk_view_theme_get(m_view);
    m_object = edje_object_add(evas_object_evas_get(m_view));
    if (!m_object)
        return;

    const char* group = (orientation == HorizontalScrollbar)
        ? "scrollbar.horizontal" : "scrollbar.vertical";
    if (!edje_object_file_set(m_object, m_theme.utf8().data(), group)) {
        fprintf(stderr, "edje_object_file_set is error\n");
        return;
    }
    evas_object_smart_member_add(m_object, m_view);

    updateVisibility();
}

MainFrameScrollbarTizen::~MainFrameScrollbarTizen()
{
    if (m_object)
        evas_object_del(m_object);
}

void MainFrameScrollbarTizen::updateThumbPositionAndProportion()
{
    if (!ewk_view_main_frame_scrollbar_visible_get(m_view))
        return;

    OwnArrayPtr<char> buffer = adoptArrayPtr(new char[sizeof(Edje_Message_Float_Set) + sizeof(double)]);
    Edje_Message_Float_Set* message = new(buffer.get()) Edje_Message_Float_Set;
    message->count = 2;

    if (m_totalSize - m_visibleSize > 0)
        message->val[0] = m_position / static_cast<double>(m_totalSize - m_visibleSize);
    else
        message->val[0] = 0;

    if (m_totalSize > 0)
        message->val[1] = m_visibleSize / static_cast<double>(m_totalSize);
    else
        message->val[1] = 0;

    if (message->val[1] < s_minimumThumbSize)
        message->val[1] = s_minimumThumbSize;

    edje_object_message_send(m_object, EDJE_MESSAGE_FLOAT_SET, 0, message);

    // Process scrollbar's message queue in order to trigger scrollbar's rendering here
    // because we want to scroll contents and scrollbar at a time.
    // If we do not process message queue, scrollbar will be rendered in the next frame even though
    // scroll is already moved. We can not recognize that by seeing, but it can be recognized by
    // testing using video camera (240 FPS).
    edje_object_message_signal_process(m_object);
}

void MainFrameScrollbarTizen::setFrameRect(const WebCore::IntRect& rect)
{
    m_frameRect = rect;
    frameRectChanged();
}

void MainFrameScrollbarTizen::setPosition(int position)
{
    if (m_position == position)
        return;

    m_position = position;
    if (m_isEnabled && m_position > 1)
        updateThumbPositionAndProportion();
}

void MainFrameScrollbarTizen::setProportion(int visibleSize, int totalSize)
{
    if (m_visibleSize != visibleSize || m_totalSize != totalSize) {
        m_visibleSize = visibleSize;
        m_totalSize = totalSize;
        // Do not update scrollbar when size changed in order to show the scrollbar only when user interacts.
    }
}

void MainFrameScrollbarTizen::frameRectChanged()
{
    int x, y;
    evas_object_geometry_get(m_view, &x, &y, 0, 0);
    evas_object_move(m_object, x + m_frameRect.x(), y + m_frameRect.y());
    evas_object_resize(m_object, m_frameRect.width(), m_frameRect.height());
}

void MainFrameScrollbarTizen::updateVisibility()
{
    if (ewk_view_main_frame_scrollbar_visible_get(m_view))
        evas_object_show(m_object);
    else
        evas_object_hide(m_object);
}

} // namespace WebKit
#endif
#endif
