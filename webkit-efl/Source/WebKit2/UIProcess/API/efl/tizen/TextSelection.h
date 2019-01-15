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

#ifndef TextSelection_h
#define TextSelection_h

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)

#include "PageClientImpl.h"
#include "TextSelectionHandle.h"
#include "TextSelectionMagnifier.h"
#include "WebPageProxy.h"
#include <Evas.h>
#include <WebCore/IntPoint.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

class EwkViewImpl;

namespace WebKit {

class TextSelectionHandle;
class TextSelectionMagnifier;
class PageClientImpl;

class TextSelection {
public:
    static PassOwnPtr<TextSelection> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new TextSelection(viewImpl));
    }
    ~TextSelection();

    enum HandleMovingDirection {
        HandleMovingDirectionNone = 0,
        HandleMovingDirectionNormal,
        HandleMovingDirectionReverse,
    };

    enum TextSelectionDirection {
        RToL = 0,
        LToR,
    };

    enum Mode {
        ModeNone = 0,
        ModeSelection = 1 << 1,
        ModeHandle = 1 << 2,
        ModeContextMenu = 1 << 3,
    };

    enum doShow {
        ShowNone = 0,
        ShowLeftHandle = 1 << 1,
        ShowRightHandle = 1 << 2,
        ShowContextMenu = 1 << 3,
    };

    enum ContextMenuDirection {
        DirectionDown = 0,
        DirectionUp,
        DirectionLeft,
        DirectionRight,
        DirectionNone,
    };

    void updateTextInputState();
    bool isTextSelectionDowned() { return m_isTextSelectionDowned; }
    void setIsTextSelectionDowned(bool isTextSelectionDowned) { m_isTextSelectionDowned = isTextSelectionDowned; }
    int isTextSelectionMode() { return m_isTextSelectionMode; }
    bool isLargeHandleMode() { return !(m_isTextSelectionMode & ModeSelection) && (m_isTextSelectionMode & ModeHandle); }
    void setIsTextSelectionMode(int isTextSelectionMode);
    bool isMagnifierVisible();
    void updateVisible(bool isVisible, bool isHandleIncluded = true);
    bool isEnabled();
    bool isAutomaticClearEnabled();
    bool textSelectionDown(const WebCore::IntPoint& point);
    void textSelectionMove(const WebCore::IntPoint& point);
    void textSelectionUp(const WebCore::IntPoint& point, bool isStaredTextSelectionFromOutside = false);

    // handle callback
    void handleMouseDown(TextSelectionHandle* handle, const WebCore::IntPoint& position);
    void handleMouseMove(TextSelectionHandle* handle, const WebCore::IntPoint& position);
    void handleMouseUp(TextSelectionHandle* handle, const WebCore::IntPoint& position);

    void handleDown(TextSelectionHandle::HandleType type, const WebCore::IntPoint& position);
    void handleMove(TextSelectionHandle::HandleType type, const WebCore::IntPoint& position);
    void handleUp(TextSelectionHandle::HandleType type, const WebCore::IntPoint& position);

    bool isTextSelectionHandleDowned();

    void requestToShow();
    void initHandlesMouseDownedStatus();

    void changeContextMenuPosition(WebCore::IntPoint& position, WebCore::IntPoint, int&);
    void selectWordAutomatically(const WebCore::IntPoint& point);

    void scheduleLargeHandle();
    void showScheduledLargeHandle();

    bool isUpdateRequired();
    void setRelayoutRequired(bool required);
    bool relayoutRequired() { return m_relayoutRequired; }

    void hideMagnifierOnRotation();

    void requestUpdatingEditorState();
    bool didRequestUpdatingEditorState() { return m_requestUpdatingEditorState; }

    friend class PageClientImpl; // to allow hideHandlers() call while zooming
private:
    TextSelection(EwkViewImpl*);
    void clear();
    void hide();
    void updateHandlers();
    void hideHandlers();
    void updateLargeHandler();
    void hideLargeHandler();
    void reloadMagnifier();
    void showMagnifier();
    void hideMagnifier();
    void updateMagnifier(const WebCore::IntPoint& position);
    void showContextMenu();
    void hideContextMenu();
    void setLeftSelectionToEvasPoint(const WebCore::IntPoint& evasPoint);
    void setRightSelectionToEvasPoint(const WebCore::IntPoint& evasPoint);

    void startMoveAnimator();
    void stopMoveAnimator();
    static void onMouseUp(void* data, Evas*, Evas_Object*, void* eventInfo);
    static Eina_Bool moveAnimatorCallback(void*);

    static Eina_Bool showTimerCallback(void* data);
    void showHandlesAndContextMenu();
    void scrollContentWithSelectionIfRequired(const WebCore::IntPoint& evasPoint);

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    void informTextStyleState();
#endif
    void setAutoWordSelectionScheduled(bool scheduled, const WebCore::IntPoint& position);
    void setAutoWordSelection(const WebCore::IntPoint& point);

    void requestToShowContextMenu();
    static Eina_Bool showContextMenuTimerCallback(void* data);

    void moveSelection(const WebCore::IntPoint& point, bool extend);
    void setShowLargeHandleScheduled(bool scheduled, uintptr_t inputMethodContextID);

    void initTextSelectionHandle();

#if ENABLE(TIZEN_WEBKIT2_BROWSER_TOOLBAR_OVERLAP_EXCEPTION)
    static void browserToolbarChanged(void *data, Evas_Object *obj, void *eventInfo);
    void setBrowserToolbarRect(const WebCore::IntRect& rect);
#endif

private:
    EwkViewImpl* m_viewImpl;
    TextSelectionHandle* m_leftHandle;
    TextSelectionHandle* m_rightHandle;
    TextSelectionHandle* m_largeHandle;
    TextSelectionMagnifier* m_magnifier;
    bool m_isTextSelectionDowned;
    int m_isTextSelectionMode;
    Ecore_Animator* m_moveAnimator;
    Ecore_Timer* m_showTimer;
    WebCore::IntRect m_lastLeftHandleRect;
    WebCore::IntRect m_lastRightHandleRect;
    WebCore::IntRect m_lastLargeHandleRect;
    int m_handleMovingDirection;
    bool m_isScrollingByHandle;
    bool m_isAutoWordSelectionScheduled;
    WebCore::IntPoint  m_scheduledAutoWordSelectionPosition;
    EditorState m_lastEditorState;
    Ecore_Timer* m_showContextMenuTimer;
    bool m_isLargeHandleScheduled;
    uintptr_t m_inputMethodContextID;
    bool m_relayoutRequired;
    bool m_requestUpdatingEditorState;
    int m_doShowHandlesAndContextMenu;
#if ENABLE(TIZEN_WEBKIT2_BROWSER_TOOLBAR_OVERLAP_EXCEPTION)
    WebCore::IntRect m_browserToolbarRect;
#endif
    Evas_Coord_Point m_lastAnimatorPoint;
};

} // namespace WebKit

#endif // TIZEN_WEBKIT2_TEXT_SELECTION
#endif // TextSelection_h
