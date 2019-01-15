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

#include "config.h"
#include "EditorState.h"

#include "Arguments.h"
#include "WebCoreArgumentCoders.h"

namespace WebKit {

void EditorState::encode(CoreIPC::ArgumentEncoder* encoder) const
{
    encoder->encode(shouldIgnoreCompositionSelectionChange);
    encoder->encode(selectionIsNone);
    encoder->encode(selectionIsRange);
    encoder->encode(isContentEditable);
    encoder->encode(isContentRichlyEditable);
    encoder->encode(isInPasswordField);
    encoder->encode(hasComposition);

#if ENABLE(TIZEN_ISF_PORT)
    encoder->encode(inputMethodContextID);
    encoder->encode(inputMethodHints);
    encoder->encode(hasForm);
    encoder->encode(surroundingText);
    encoder->encode(cursorPosition);
#if ENABLE(TIZEN_INPUT_PICKER_MAX_MIN)
    encoder->encode(minDate);
    encoder->encode(maxDate);
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    encoder->encode(selectionRect);
    encoder->encode(editorRect);
    encoder->encode(leftSelectionRect);
    encoder->encode(rightSelectionRect);
    encoder->encode(isLeftToRightDirection);
    encoder->encode(updateEditorRectOnly);
    encoder->encode(isOnlyImageSelection);
    encoder->encode(isContentEmpty);
    encoder->encode(selectionByUserAction);
    encoder->encode(selectionInMarque);
    encoder->encode(isLeftHandleVisible);
    encoder->encode(isRightHandleVisible);
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    encoder->encode(underlineState);
    encoder->encode(italicState);
    encoder->encode(boldState);
    encoder->encode(bgColor);
    encoder->encode(color);
    encoder->encode(fontSize);
    encoder->encode(orderedListState);
    encoder->encode(unorderedListState);
    encoder->encode(textAlignCenterState);
    encoder->encode(textAlignLeftState);
    encoder->encode(textAlignRightState);
    encoder->encode(textAlignFullState);
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    encoder->encode(caretRect);
    encoder->encode(focusedNodeRect);
#endif

#if PLATFORM(QT)
    encoder->encode(cursorPosition);
    encoder->encode(anchorPosition);
    encoder->encode(editorRect);
    encoder->encode(cursorRect);
    encoder->encode(compositionRect);
    encoder->encode(inputMethodHints);
    encoder->encode(selectedText);
    encoder->encode(surroundingText);
#endif
}

bool EditorState::decode(CoreIPC::ArgumentDecoder* decoder, EditorState& result)
{
    if (!decoder->decode(result.shouldIgnoreCompositionSelectionChange))
        return false;

    if (!decoder->decode(result.selectionIsNone))
        return false;

    if (!decoder->decode(result.selectionIsRange))
        return false;

    if (!decoder->decode(result.isContentEditable))
        return false;

    if (!decoder->decode(result.isContentRichlyEditable))
        return false;

    if (!decoder->decode(result.isInPasswordField))
        return false;

    if (!decoder->decode(result.hasComposition))
        return false;

#if ENABLE(TIZEN_ISF_PORT)
    if (!decoder->decode(result.inputMethodContextID))
        return false;

    if (!decoder->decode(result.inputMethodHints))
        return false;

    if (!decoder->decode(result.hasForm))
        return false;

    if (!decoder->decode(result.surroundingText))
        return false;

    if (!decoder->decode(result.cursorPosition))
        return false;

#if ENABLE(TIZEN_INPUT_PICKER_MAX_MIN)
    if (!decoder->decode(result.minDate))
        return false;

    if (!decoder->decode(result.maxDate))
        return false;
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (!decoder->decode(result.selectionRect))
        return false;

    if (!decoder->decode(result.editorRect))
        return false;

    if (!decoder->decode(result.leftSelectionRect))
        return false;

    if (!decoder->decode(result.rightSelectionRect))
        return false;

    if (!decoder->decode(result.isLeftToRightDirection))
        return false;

    if (!decoder->decode(result.updateEditorRectOnly))
        return false;

    if (!decoder->decode(result.isOnlyImageSelection))
        return false;

    if (!decoder->decode(result.isContentEmpty))
        return false;

    if (!decoder->decode(result.selectionByUserAction))
        return false;

    if (!decoder->decode(result.selectionInMarque))
        return false;

    if (!decoder->decode(result.isLeftHandleVisible))
        return false;

    if (!decoder->decode(result.isRightHandleVisible))
        return false;
#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    if (!decoder->decode(result.underlineState))
        return false;

    if (!decoder->decode(result.italicState))
        return false;

    if (!decoder->decode(result.boldState))
        return false;

    if (!decoder->decode(result.bgColor))
        return false;

    if (!decoder->decode(result.color))
        return false;

    if (!decoder->decode(result.fontSize))
        return false;

    if (!decoder->decode(result.orderedListState))
        return false;

    if (!decoder->decode(result.unorderedListState))
        return false;

    if (!decoder->decode(result.textAlignCenterState))
        return false;

    if (!decoder->decode(result.textAlignLeftState))
        return false;

    if (!decoder->decode(result.textAlignRightState))
        return false;

    if (!decoder->decode(result.textAlignFullState))
        return false;
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    if (!decoder->decode(result.caretRect))
        return false;
    if (!decoder->decode(result.focusedNodeRect))
        return false;
#endif

#if PLATFORM(QT)
    if (!decoder->decode(result.cursorPosition))
        return false;

    if (!decoder->decode(result.anchorPosition))
        return false;

    if (!decoder->decode(result.editorRect))
        return false;

    if (!decoder->decode(result.cursorRect))
        return false;

    if (!decoder->decode(result.compositionRect))
        return false;

    if (!decoder->decode(result.inputMethodHints))
        return false;

    if (!decoder->decode(result.selectedText))
        return false;

    if (!decoder->decode(result.surroundingText))
        return false;
#endif

    return true;
}

}
