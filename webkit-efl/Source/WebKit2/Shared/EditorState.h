/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef EditorState_h
#define EditorState_h

#include "ArgumentCoders.h"
#include <WebCore/IntRect.h>
#include <wtf/NotFound.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

struct EditorState {
    EditorState()
        : shouldIgnoreCompositionSelectionChange(false)
        , selectionIsNone(true)
        , selectionIsRange(false)
        , isContentEditable(false)
        , isContentRichlyEditable(false)
        , isInPasswordField(false)
        , hasComposition(false)
#if ENABLE(TIZEN_ISF_PORT)
        , inputMethodContextID(0)
        , hasForm(false)
        , cursorPosition(0)
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        , isLeftToRightDirection(true)
        , updateEditorRectOnly(false)
        , isOnlyImageSelection(false)
        , isContentEmpty(true)
        , selectionByUserAction(false)
        , selectionInMarque(false)
        , isLeftHandleVisible(false)
        , isRightHandleVisible(false)
#endif
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
        , underlineState(0)
        , italicState(0)
        , boldState(0)
#endif
#if PLATFORM(QT)
        , cursorPosition(0)
        , anchorPosition(0)
        , inputMethodHints(0)
#endif
    {
    }

    bool shouldIgnoreCompositionSelectionChange;

    bool selectionIsNone; // This will be false when there is a caret selection.
    bool selectionIsRange;
    bool isContentEditable;
    bool isContentRichlyEditable;
    bool isInPasswordField;
    bool hasComposition;
#if ENABLE(TIZEN_ISF_PORT)
    uintptr_t inputMethodContextID;
    WTF::String inputMethodHints;
    bool hasForm;
    WTF::String surroundingText;
    unsigned cursorPosition;
#if ENABLE(TIZEN_INPUT_PICKER_MAX_MIN)
    WTF::String minDate;
    WTF::String maxDate;
#endif
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    WebCore::IntRect selectionRect;
    WebCore::IntRect editorRect;
    WebCore::IntRect leftSelectionRect;
    WebCore::IntRect rightSelectionRect;
    bool isLeftToRightDirection;
    bool updateEditorRectOnly;
    bool isOnlyImageSelection;
    bool isContentEmpty;
    bool selectionByUserAction;
    bool selectionInMarque;
    bool isLeftHandleVisible;
    bool isRightHandleVisible;
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    int underlineState;
    int italicState;
    int boldState;
    WTF::String bgColor;
    WTF::String color;
    WTF::String fontSize;
    int orderedListState;
    int unorderedListState;
    int textAlignCenterState;
    int textAlignLeftState;
    int textAlignRightState;
    int textAlignFullState;
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    // Currently selection rect is generally same to caret's rect
    // But sometimes, especially caret is hidden by DIV or something else,
    // selectionRect is empty to avoid selection handle for unvisible selection
    // So, caretRect is added to pass the native caret's rect for input field zoom
    WebCore::IntRect caretRect;
    WebCore::IntRect focusedNodeRect;
#endif

#if PLATFORM(QT)
    // The anchor, cursor represent either the selection or composition, depending
    // whether a composition exists or not.
    unsigned cursorPosition;
    unsigned anchorPosition;

    WebCore::IntRect editorRect;
    WebCore::IntRect cursorRect;
    WebCore::IntRect compositionRect;

    uint64_t inputMethodHints;

    WTF::String selectedText;
    WTF::String surroundingText;
#endif

    void encode(CoreIPC::ArgumentEncoder*) const;
    static bool decode(CoreIPC::ArgumentDecoder*, EditorState&);
};

}

#endif // EditorState_h
