/*
 * Copyright (C) 2011 Samsung Electronics. All rights reserved.
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
#include "WebEditorClient.h"

#include "Frame.h"
#include "NativeWebKeyboardEvent.h"
#include "PlatformKeyboardEvent.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#if ENABLE(TIZEN_ISF_PORT)
#include "WindowsKeyboardCodes.h"
#endif
#include <WebCore/FocusController.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
#include <Evas.h>
#include <WebCore/DataObjectTizen.h>
#endif

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_ISF_PORT)
static bool handleKeyPressCommands(WebPage* page, KeyboardEvent* event)
{
    const NativeWebKeyboardEvent* currentEvent = static_cast<const NativeWebKeyboardEvent*>(WebPage::currentEvent());
    bool isFiltered = (currentEvent && currentEvent->isFiltered());

    Frame* targetFrame = page->corePage()->focusController()->focusedOrMainFrame();
    if (!targetFrame)
        return isFiltered;

    Editor* editor = targetFrame->editor();
    if (!editor->canEdit())
        return isFiltered;

    if (event->type() != eventNames().keypressEvent) {
        const char* keydownCommandName = (event->type() == eventNames().keydownEvent ? getKeyDownCommandName(event) : 0);
        if (!keydownCommandName || editor->command(keydownCommandName).isTextInsertion())
            return isFiltered;
    }

    Vector<OwnPtr<KeyPressCommand> > commands;
    bool recalcFilterEvent = page->recalcFilterEvent();
    const EditorState& editorState = page->currentEditorState();

    if (recalcFilterEvent || (currentEvent && currentEvent->inputMethodContextID() != editorState.inputMethodContextID)) {
        if (!recalcFilterEvent)
            page->startRecalcFilterEvent();
        page->sendSync(Messages::WebPageProxy::RecalcFilterEvent(editorState, !recalcFilterEvent), Messages::WebPageProxy::RecalcFilterEvent::Reply(isFiltered));
        return isFiltered;
    }

    page->swapKeyPressCommands(commands);

    size_t size = commands.size();
    if (!size)
        return isFiltered;

    bool isContentRichlyEditable = targetFrame->selection()->isContentRichlyEditable();

    for (size_t i = 0; i < size; ++i) {
        switch (commands[i]->type) {
        case KeyPressCommandSetComposition: {
            if (i + 1 < size) {
                int nextType = commands[i + 1]->type;
                if (nextType == KeyPressCommandSetComposition || nextType == KeyPressCommandConfirmComposition)
                    break;
            }

            SetCompositionKeyPressCommand* command = reinterpret_cast<SetCompositionKeyPressCommand*>(commands[i].get());
            if (isContentRichlyEditable)
                command->compositionString.replace(space, noBreakSpace);
            page->setComposition(command->compositionString, command->underlines, command->cursorPosition);
            break;
        }
        case KeyPressCommandConfirmComposition: {
            ConfirmCompositionKeyPressCommand* command = reinterpret_cast<ConfirmCompositionKeyPressCommand*>(commands[i].get());
            if (isContentRichlyEditable)
                command->compositionString.replace(space, noBreakSpace);
            page->confirmComposition(command->compositionString);
            break;
        }
        case KeyPressCommandDeleteText: {
            bool selectOnly = false;
            if (!editor->hasComposition() && i + 1 < size) {
                int nextType = commands[i + 1]->type;
                if (nextType == KeyPressCommandSetComposition || nextType == KeyPressCommandConfirmComposition)
                    selectOnly = true;
            }

            DeleteTextKeyPressCommand* command = reinterpret_cast<DeleteTextKeyPressCommand*>(commands[i].get());
            if (selectOnly)
                page->selectSurroundingText(command->offset, command->count);
            else
                page->deleteSurroundingText(command->offset, command->count);
            break;
        }
        default:
            break;
        }
    }

    event->setDefaultHandled();

    return isFiltered;
}
#endif

void WebEditorClient::handleKeyboardEvent(KeyboardEvent* event)
{
#if ENABLE(TIZEN_ISF_PORT)
    if (handleKeyPressCommands(m_page, event))
        return;
#endif

    if (m_page->handleEditingKeyboardEvent(event))
        event->setDefaultHandled();
}

void WebEditorClient::handleInputMethodKeydown(KeyboardEvent* event)
{
#if ENABLE(TIZEN_ISF_PORT)
    return;
#endif

    Frame* frame = m_page->corePage()->focusController()->focusedOrMainFrame();
    if (!frame || !frame->editor()->canEdit())
        return;

    // FIXME: sending sync message might make input lagging.
    bool handled = false;
    m_page->sendSync(Messages::WebPageProxy::HandleInputMethodKeydown(), Messages::WebPageProxy::HandleInputMethodKeydown::Reply(handled));

    if (handled)
        event->setDefaultHandled();
}

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
void WebEditorClient::setClipboardData(const String& data, const String& type)
{
    m_page->send(Messages::WebPageProxy::SetClipboardData(data, type));
}

void WebEditorClient::setClipboardDataForPaste(const String& data, const String& type)
{
    DataObjectTizen* dataObject = DataObjectTizen::sharedDataObject();

    if (type == "Markup") {
        dataObject->setMarkup(data);
        char* plainText = evas_textblock_text_markup_to_utf8(0, data.utf8().data());
        if (plainText) {
            dataObject->setText(data);
            free(plainText);
        }
    } else if (type == "PlainText") {
        dataObject->setText(data);
    } else if (type == "Image") {
        dataObject->setImage(data);
        dataObject->setText(data);
    }
}

void WebEditorClient::clearClipboardData()
{
    m_page->send(Messages::WebPageProxy::ClearClipboardData());
}
#endif

#if ENABLE(TIZEN_ISF_PORT)
void WebEditorClient::didCancelComposition(Node* valueChangedNode)
{
    m_page->didCancelComposition(valueChangedNode);
}
#endif

}
