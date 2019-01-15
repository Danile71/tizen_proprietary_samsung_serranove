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

#ifndef TextSelectionHandle_h
#define TextSelectionHandle_h

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)

#include "TextSelection.h"
#include <Ecore.h>
#include <Evas.h>
#include <WebCore/IntPoint.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

class TextSelection;

class TextSelectionHandle {
public:
    TextSelectionHandle(Evas_Object* object, const char* path, const char* part, int type, TextSelection* textSelection);
    ~TextSelectionHandle();

    enum HandleType {
        LeftHandle,
        RightHandle,
        LargeHandle,
    };

    enum HandleDirection {
        DirectionBottomNormal,
        DirectionBottomReverse,
        DirectionTopNormal,
        DirectionTopReverse,
    };

    void move(const WebCore::IntPoint& point, const WebCore::IntRect& selectionRect, bool selectionDirection);
    void show();
    void hide();
    bool isLeft() const { return (m_type ==  LeftHandle) ? true : false; }
    bool isRight() const { return (m_type ==  RightHandle) ? true : false; }
    bool isLarge() const { return (m_type ==  LargeHandle) ? true : false; }
    const WebCore::IntPoint position() const { return m_position; }
    void setBasePositionForMove(const WebCore::IntPoint& position) { m_basePositionForMove = position; }
    bool isMouseDowned() { return m_isMouseDowned; }
    bool setIsMouseDowned(bool isMouseDowned) { return m_isMouseDowned = isMouseDowned; }
    const WebCore::IntRect getHandleRect();
    bool isTop() const { return m_isTop; }
    bool isVisible() const { return evas_object_visible_get(m_icon); }

    void mouseDown(const WebCore::IntPoint& point);
    void mouseMove(const WebCore::IntPoint& point);
    void mouseUp();

private:
    // callbacks
    static void onMouseDown(void*, Evas*, Evas_Object*, void*);
    static void onMouseMove(void*, Evas*, Evas_Object*, void*);
    static void onMouseUp(void*, Evas*, Evas_Object*, void*);
    static void update(void*);

    void setFirstDownMousePosition(const WebCore::IntPoint& position) { m_firstDownMousePosition = position; }
    void setMousePosition(const WebCore::IntPoint& position) { m_mousePosition = position; }

    void showObject();
    void hideObject();
    void moveObject(WebCore::IntPoint position);
    void changeObjectDirection(int direction);

private:
    Evas_Object* m_icon;
    Evas_Object* m_widget;
    WebCore::IntPoint m_mousePosition;
    static Ecore_Job* s_job;
    int m_type;
    TextSelection* m_textSelection;
    WebCore::IntPoint m_position;
    WebCore::IntPoint m_firstDownMousePosition;
    WebCore::IntPoint m_basePositionForMove;
    bool m_isMouseDowned;
    bool m_isTop;
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION_TEXT_CURSOR)
    Evas_Object* m_textCursor;
#endif
};

} // namespace WebKit

#endif // TIZEN_WEBKIT2_TEXT_SELECTION
#endif // TextSelectionHandle_h
