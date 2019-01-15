/*
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
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

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
#include "TextSelectionHandle.h"

#include <Edje.h>

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION_TEXT_CURSOR)
#include <efl_assist.h>

#define TEXT_CURSOR_WIDTH   3
#endif

using namespace WebCore;

namespace WebKit {

Ecore_Job* TextSelectionHandle::s_job = 0;

TextSelectionHandle::TextSelectionHandle(Evas_Object* object, const char* themePath, const char* part, int type, TextSelection* textSelection)
    : m_widget(object)
    , m_type(type)
    , m_textSelection(textSelection)
    , m_position(IntPoint(0, 0))
    , m_isMouseDowned(false)
    , m_isTop(false)
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION_TEXT_CURSOR)
    , m_textCursor(0)
#endif
{
    Evas* evas = evas_object_evas_get(object);
    m_icon = edje_object_add(evas);

    if (!m_icon)
        return;

    if (!edje_object_file_set(m_icon, themePath, part))
        return;

    edje_object_signal_emit(m_icon, "edje,focus,in", "edje");
    edje_object_signal_emit(m_icon, "elm,state,bottom", "elm");
    evas_object_smart_member_add(m_icon, object);

    evas_object_propagate_events_set(m_icon, false);
    evas_object_event_callback_add(m_icon, EVAS_CALLBACK_MOUSE_DOWN, onMouseDown, this);
    evas_object_event_callback_add(m_icon, EVAS_CALLBACK_MOUSE_MOVE, onMouseMove, this);
    evas_object_event_callback_add(m_icon, EVAS_CALLBACK_MOUSE_UP, onMouseUp, this);
}

TextSelectionHandle::~TextSelectionHandle()
{
    evas_object_event_callback_del(m_icon, EVAS_CALLBACK_MOUSE_DOWN, onMouseDown);
    evas_object_event_callback_del(m_icon, EVAS_CALLBACK_MOUSE_MOVE, onMouseMove);
    evas_object_event_callback_del(m_icon, EVAS_CALLBACK_MOUSE_UP, onMouseUp);
    evas_object_del(m_icon);

    if (s_job) {
        ecore_job_del(s_job);
        s_job = 0;
    }
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION_TEXT_CURSOR)
    if (m_textCursor)
        evas_object_del(m_textCursor);
#endif
}

void TextSelectionHandle::move(const IntPoint& point, const IntRect& selectionRect, bool selectionDirection)
{
    IntPoint movePoint = point;
    const int reverseMargin = 32;
    int x, y, deviceWidth, deviceHeight;
    int handleHeight;

    edje_object_part_geometry_get(m_icon, "handle", 0, 0, 0, &handleHeight);
    evas_object_geometry_get(m_widget, &x, &y, &deviceWidth, &deviceHeight);
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION_TEXT_CURSOR)
    if (isLeft() || isRight()) {
        IntRect textCursorRect;
        if (isLeft())
            textCursorRect = IntRect(selectionRect.x() - 1, selectionRect.y(), TEXT_CURSOR_WIDTH, selectionRect.height());
        else // isRight
            textCursorRect = IntRect(selectionRect.maxX() - TEXT_CURSOR_WIDTH + 2, selectionRect.y(), TEXT_CURSOR_WIDTH, selectionRect.height());

        if (m_textCursor)
            evas_object_del(m_textCursor);
        m_textCursor = evas_object_rectangle_add(evas_object_evas_get(m_widget));

        int r, g, b, a;
        ea_theme_color_get("F052", &r, &g, &b, &a, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        Color textCursorColor(r, g, b, a);

        evas_object_color_set(m_textCursor, r, g, b, a);
        evas_object_move(m_textCursor, textCursorRect.x(), textCursorRect.y());
        evas_object_resize(m_textCursor, textCursorRect.width(), textCursorRect.height());
    }
#endif

    if ((isLeft() && (movePoint.x() <= reverseMargin)) || (isRight() && (movePoint.x() >= deviceWidth - reverseMargin))) {
        if ((movePoint.y() + handleHeight) > (y + deviceHeight)) {
            movePoint.setY(movePoint.y() - selectionRect.height());
            if (!m_isMouseDowned) {
                changeObjectDirection(DirectionTopReverse);
                m_isTop = true;
            }
        } else {
            if (!m_isMouseDowned) {
                changeObjectDirection(DirectionBottomReverse);
                m_isTop = false;
            }
        }
    }
    else {
        // Check first whethter handler is going beyond the Visible Content Rect.
        if ((movePoint.y() + handleHeight) > (y + deviceHeight)) {
            // Check If handler is not overLapping Device Boundary.
            if ((movePoint.y() - selectionRect.height() - handleHeight) >= y) {
                movePoint.setY(movePoint.y() - selectionRect.height());
                if (!m_isMouseDowned) {
                    if (isLarge())
                        changeObjectDirection(DirectionTopNormal);
                    else {
                        if (selectionDirection)
                            changeObjectDirection(DirectionTopNormal);
                        else
                            changeObjectDirection(DirectionTopReverse);
                    }
                    m_isTop = true;
                }
                moveObject(movePoint);
                m_position = movePoint;
                return;
            }
        }

        if (!m_isMouseDowned) {
            if (isLarge())
                changeObjectDirection(DirectionBottomNormal);
            else {
                if (selectionDirection)
                    changeObjectDirection(DirectionBottomNormal);
                else
                    changeObjectDirection(DirectionBottomReverse);
            }
            m_isTop = false;
        }
    }

    moveObject(movePoint);
    m_position = movePoint;
}

void TextSelectionHandle::show()
{
    int x, y, deviceWidth, deviceHeight;
    evas_object_geometry_get(m_widget, &x, &y, &deviceWidth, &deviceHeight);
    IntRect viewPort(x, y, deviceWidth, deviceHeight);
    // Checking Boundary conditions also if m_position is on boundary.
    if (m_position.x() >= viewPort.x() && m_position.x() <= viewPort.maxX()
        && m_position.y() >= viewPort.y() && m_position.y() <= viewPort.maxY())
        showObject();
    else
        hide();   // Hide the handle if we are not showing it.
}

void TextSelectionHandle::hide()
{
    hideObject();
}

const IntRect TextSelectionHandle::getHandleRect()
{
    if (!evas_object_visible_get(m_icon))
        return IntRect();

    int x, y;
    evas_object_geometry_get(m_icon, &x, &y, 0, 0);

    int partX, partY, partWidth, partHeight;
    edje_object_part_geometry_get(m_icon, "handle", &partX, &partY, &partWidth, &partHeight);

    return IntRect(x + partX, y + partY, partWidth, partHeight);
}

// callbacks
void TextSelectionHandle::mouseDown(const IntPoint& point)
{
    setIsMouseDowned(true);
    setFirstDownMousePosition(point);
    setMousePosition(point);
    m_textSelection->handleMouseDown(this, m_mousePosition);
}

void TextSelectionHandle::mouseMove(const IntPoint& point)
{
    setMousePosition(point);

    if (!s_job)
        s_job = ecore_job_add(update, this);
}

void TextSelectionHandle::mouseUp()
{
    setIsMouseDowned(false);
    m_textSelection->handleMouseUp(this, m_mousePosition);
}

void TextSelectionHandle::onMouseDown(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Down* event = static_cast<Evas_Event_Mouse_Down*>(eventInfo);
    TextSelectionHandle* handle = static_cast<TextSelectionHandle*>(data);

    handle->mouseDown(IntPoint(event->canvas.x, event->canvas.y));
}

void TextSelectionHandle::onMouseMove(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    Evas_Event_Mouse_Move* event = static_cast<Evas_Event_Mouse_Move*>(eventInfo);
    TextSelectionHandle* handle = static_cast<TextSelectionHandle*>(data);

    handle->mouseMove(IntPoint(event->cur.canvas.x, event->cur.canvas.y));
}

void TextSelectionHandle::onMouseUp(void* data, Evas*, Evas_Object*, void* eventInfo)
{
    TextSelectionHandle* handle = static_cast<TextSelectionHandle*>(data);

    handle->mouseUp();
}

// job
void TextSelectionHandle::update(void* data)
{
    TextSelectionHandle* handle = static_cast<TextSelectionHandle*>(data);

    int deltaX = handle->m_mousePosition.x() - handle->m_firstDownMousePosition.x();
    int deltaY = handle->m_mousePosition.y() - handle->m_firstDownMousePosition.y();
    if (deltaX || deltaY) {
        IntPoint movePosition;

        movePosition.setX(handle->m_basePositionForMove.x() + deltaX);
        movePosition.setY(handle->m_basePositionForMove.y() + deltaY);

        handle->m_textSelection->handleMouseMove(handle, movePosition);
    }

    s_job = 0;
}

void TextSelectionHandle::showObject()
{
    if (ewk_settings_selection_handle_enabled_get(ewk_view_settings_get(m_widget))) {
        edje_object_message_signal_process(m_icon);
        evas_object_show(m_icon);
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION_TEXT_CURSOR)
        evas_object_show(m_textCursor);
#endif
    } else
        evas_object_smart_callback_call(m_widget, "selection,handle,show", &m_type);
}

void TextSelectionHandle::hideObject()
{
    if (ewk_settings_selection_handle_enabled_get(ewk_view_settings_get(m_widget))) {
        evas_object_hide(m_icon);
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION_TEXT_CURSOR)
        evas_object_hide(m_textCursor);
#endif
    }
    else
        evas_object_smart_callback_call(m_widget, "selection,handle,hide", &m_type);
}

void TextSelectionHandle::moveObject(IntPoint position)
{
    if (ewk_settings_selection_handle_enabled_get(ewk_view_settings_get(m_widget)))
        evas_object_move(m_icon, position.x(), position.y());
    else {
        Evas_Point point;
        point.x = position.x();
        point.y = position.y();

        switch (m_type) {
        case LeftHandle :
            evas_object_smart_callback_call(m_widget, "selection,handle,left,move", &point);
            break;
        case RightHandle :
            evas_object_smart_callback_call(m_widget, "selection,handle,right,move", &point);
            break;
        case LargeHandle :
            evas_object_smart_callback_call(m_widget, "selection,handle,large,move", &point);
            break;
       }
    }
}

void TextSelectionHandle::changeObjectDirection(int direction)
{
    if (ewk_settings_selection_handle_enabled_get(ewk_view_settings_get(m_widget))) {
        switch (direction) {
        case DirectionBottomNormal :
            if (isLarge())
                edje_object_signal_emit(m_icon, "edje,cursor,handle,show", "edje");
            else
                edje_object_signal_emit(m_icon, "elm,state,bottom", "elm");
            break;
        case DirectionBottomReverse :
            if (isLarge())
                edje_object_signal_emit(m_icon, "edje,cursor,handle,show", "edje");
            else
                edje_object_signal_emit(m_icon, "elm,state,bottom,reversed", "elm");
            break;
        case DirectionTopNormal :
            if (isLarge())
                edje_object_signal_emit(m_icon, "edje,cursor,handle,top", "edje");
            else
                edje_object_signal_emit(m_icon, "elm,state,top", "elm");
            break;
        case DirectionTopReverse :
            if (isLarge())
                edje_object_signal_emit(m_icon, "edje,cursor,handle,top", "edje");
            else
                edje_object_signal_emit(m_icon, "elm,state,top,reversed", "elm");
            break;
        }

        return;
    }

    switch (m_type) {
    case LeftHandle :
        evas_object_smart_callback_call(m_widget, "selection,handle,left,direction", &direction);
        break;
    case RightHandle :
        evas_object_smart_callback_call(m_widget, "selection,handle,right,direction", &direction);
        break;
    case LargeHandle :
        evas_object_smart_callback_call(m_widget, "selection,handle,large,direction", &direction);
        break;
    }
}

} // namespace WebKit

#endif // TIZEN_WEBKIT2_TEXT_SELECTION
