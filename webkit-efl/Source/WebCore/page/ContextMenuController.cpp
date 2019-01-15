/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Igalia S.L
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ContextMenuController.h"

#if ENABLE(CONTEXT_MENUS)

#include "BackForwardController.h"
#include "Chrome.h"
#include "ContextMenu.h"
#include "ContextMenuClient.h"
#include "ContextMenuItem.h"
#include "ContextMenuProvider.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentLoader.h"
#include "Editor.h"
#include "EditorClient.h"
#include "Event.h"
#include "EventHandler.h"
#include "EventNames.h"
#include "FormState.h"
#include "Frame.h"
#include "FrameLoadRequest.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameSelection.h"
#include "HTMLFormElement.h"
#include "HitTestRequest.h"
#include "HitTestResult.h"
#include "InspectorController.h"
#include "LocalizedStrings.h"
#include "MouseEvent.h"
#include "NavigationAction.h"
#include "Node.h"
#include "Page.h"
#include "PlatformEvent.h"
#include "RenderObject.h"
#include "ReplaceSelectionCommand.h"
#include "ResourceRequest.h"
#include "Settings.h"
#include "TextIterator.h"
#include "TypingCommand.h"
#include "UserTypingGestureIndicator.h"
#include "WindowFeatures.h"
#include "markup.h"
#include <wtf/unicode/Unicode.h>
#if ENABLE(TIZEN_DRAG_SUPPORT)
#include "DragController.h"
#endif

#if PLATFORM(GTK)
#include <wtf/gobject/GOwnPtr.h>
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
#include "HTMLInputElement.h"
#endif
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
#include "FocusController.h"
#endif

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
#include "visible_units.h"
#include "htmlediting.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
#include "HTMLAnchorElement.h"
#endif

using namespace WTF;
using namespace Unicode;

namespace WebCore {

ContextMenuController::ContextMenuController(Page* page, ContextMenuClient* client)
    : m_page(page)
    , m_client(client)
{
    ASSERT_ARG(page, page);
    ASSERT_ARG(client, client);
}

ContextMenuController::~ContextMenuController()
{
    m_client->contextMenuDestroyed();
}

PassOwnPtr<ContextMenuController> ContextMenuController::create(Page* page, ContextMenuClient* client)
{
    return adoptPtr(new ContextMenuController(page, client));
}

void ContextMenuController::clearContextMenu()
{
    m_contextMenu.clear();
    if (m_menuProvider)
        m_menuProvider->contextMenuCleared();
    m_menuProvider = 0;
}

void ContextMenuController::handleContextMenuEvent(Event* event)
{
    m_contextMenu = createContextMenu(event);
    if (!m_contextMenu)
        return;

    populate();

    showContextMenu(event);
}

static PassOwnPtr<ContextMenuItem> separatorItem()
{
    return adoptPtr(new ContextMenuItem(SeparatorType, ContextMenuItemTagNoAction, String()));
}

void ContextMenuController::showContextMenu(Event* event, PassRefPtr<ContextMenuProvider> menuProvider)
{
    m_menuProvider = menuProvider;

    m_contextMenu = createContextMenu(event);
    if (!m_contextMenu) {
        clearContextMenu();
        return;
    }

    m_menuProvider->populateContextMenu(m_contextMenu.get());
    if (m_hitTestResult.isSelected()) {
        appendItem(*separatorItem(), m_contextMenu.get());
        populate();
    }
    showContextMenu(event);
}

PassOwnPtr<ContextMenu> ContextMenuController::createContextMenu(Event* event)
{
    if (!event->isMouseEvent())
        return nullptr;

    MouseEvent* mouseEvent = static_cast<MouseEvent*>(event);
    HitTestResult result(mouseEvent->absoluteLocation());

    if (Frame* frame = event->target()->toNode()->document()->frame())
        result = frame->eventHandler()->hitTestResultAtPoint(mouseEvent->absoluteLocation(), false);

    if (!result.innerNonSharedNode())
        return nullptr;

    m_hitTestResult = result;

    return adoptPtr(new ContextMenu);
}

void ContextMenuController::showContextMenu(Event* event)
{
#if ENABLE(INSPECTOR) && !PLATFORM(TIZEN)
    if (m_page->inspectorController()->enabled())
        addInspectElementItem();
#endif

#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
    m_contextMenu = m_client->customizeMenu(m_contextMenu.release());
#else
    PlatformMenuDescription customMenu = m_client->getCustomMenuFromDefaultItems(m_contextMenu.get());
    m_contextMenu->setPlatformDescription(customMenu);
#endif
    event->setDefaultHandled();
}

static void openNewWindow(const KURL& urlToLoad, Frame* frame)
{
    if (Page* oldPage = frame->page()) {
        FrameLoadRequest request(frame->document()->securityOrigin(), ResourceRequest(urlToLoad, frame->loader()->outgoingReferrer()));
        if (Page* newPage = oldPage->chrome()->createWindow(frame, request, WindowFeatures(), NavigationAction(request.resourceRequest()))) {
            newPage->mainFrame()->loader()->loadFrameRequest(request, false, false, 0, 0, MaybeSendReferrer);
            newPage->chrome()->show();
        }
    }
}

#if ENABLE(TIZEN_OPEN_IMAGE_IN_CURRENT_TAB)
static void openInCurrentWindow(const KURL& urlToLoad, Frame* frame)
{
    FrameLoadRequest request(frame->document()->securityOrigin(), ResourceRequest(urlToLoad, frame->loader()->outgoingReferrer()));
    frame->loader()->loadFrameRequest(request, false, false, 0, 0, MaybeSendReferrer);
}
#endif

#if PLATFORM(GTK)
static void insertUnicodeCharacter(UChar character, Frame* frame)
{
    String text(&character, 1);
    if (!frame->editor()->shouldInsertText(text, frame->selection()->toNormalizedRange().get(), EditorInsertActionTyped))
        return;

    TypingCommand::insertText(frame->document(), text, 0, TypingCommand::TextCompositionNone);
}
#endif

void ContextMenuController::contextMenuItemSelected(ContextMenuItem* item)
{
    ASSERT(item->type() == ActionType || item->type() == CheckableActionType);

    if (item->action() >= ContextMenuItemBaseApplicationTag) {
        m_client->contextMenuItemSelected(item, m_contextMenu.get());
        return;
    }

    if (item->action() >= ContextMenuItemBaseCustomTag) {
        ASSERT(m_menuProvider);
        m_menuProvider->contextMenuItemSelected(item);
        return;
    }

    Frame* frame = m_hitTestResult.innerNonSharedNode()->document()->frame();
    if (!frame)
        return;

    switch (item->action()) {
    case ContextMenuItemTagOpenLinkInNewWindow:
        openNewWindow(m_hitTestResult.absoluteLinkURL(), frame);
        break;
    case ContextMenuItemTagDownloadLinkToDisk:
        // FIXME: Some day we should be able to do this from within WebCore.
        m_client->downloadURL(m_hitTestResult.absoluteLinkURL());
        break;
    case ContextMenuItemTagCopyLinkToClipboard:
        frame->editor()->copyURL(m_hitTestResult.absoluteLinkURL(), m_hitTestResult.textContent());
        break;
#if ENABLE(TIZEN_WEBKIT2_COPY_MENU_FOR_SPECIFIC_SCHEME_LINK)
    case ContextMenuItemTagCopyLinkData:
        frame->editor()->copyUrlWithoutScheme(m_hitTestResult.absoluteLinkURL());
        break;
#endif
    case ContextMenuItemTagOpenImageInNewWindow:
#if ENABLE(TIZEN_CHILD_NODE_IMAGE_URL)
        {
            KURL imageURL = m_hitTestResult.absoluteImageURL();
            if (imageURL.isEmpty())
                imageURL = m_hitTestResult.findChildAbsoluteImageURL();
#if ENABLE(TIZEN_LITE_VIEW_IMAGE_IN_SAME_WINDOW)
        Frame* mainFrame = m_page->mainFrame();
        mainFrame->loader()->loadFrameRequest(FrameLoadRequest(frame->document()->securityOrigin(),
            ResourceRequest(imageURL, frame->loader()->outgoingReferrer())), false, false, 0, 0,MaybeSendReferrer);
#else
            openNewWindow(imageURL, frame);
#endif
        }
#else
        openNewWindow(m_hitTestResult.absoluteImageURL(), frame);
#endif
        break;
#if ENABLE(TIZEN_OPEN_IMAGE_IN_CURRENT_TAB)
    case ContextMenuItemTagOpenImageInCurrentWindow:
#if ENABLE(TIZEN_CHILD_NODE_IMAGE_URL)
        {
            KURL imageURL = m_hitTestResult.absoluteImageURL();
            if (imageURL.isEmpty())
                imageURL = m_hitTestResult.findChildAbsoluteImageURL();
            openInCurrentWindow(imageURL, frame->page()->mainFrame());
        }
#else
        openInCurrentWindow(m_hitTestResult.absoluteImageURL(), frame->page()->mainFrame());
#endif
        break;
#endif /* ENABLE(TIZEN_OPEN_IMAGE_IN_CURRENT_TAB) */
    case ContextMenuItemTagDownloadImageToDisk:
         // FIXME: Some day we should be able to do this from within WebCore.
#if ENABLE(TIZEN_CHILD_NODE_IMAGE_URL)
        {
            KURL imageURL = m_hitTestResult.absoluteImageURL();
            if (imageURL.isEmpty())
                imageURL = m_hitTestResult.findChildAbsoluteImageURL();
            m_client->downloadURL(imageURL);
        }
#else
        m_client->downloadURL(m_hitTestResult.absoluteImageURL());
#endif
        break;
    case ContextMenuItemTagCopyImageToClipboard:
        // FIXME: The Pasteboard class is not written yet
        // For now, call into the client. This is temporary!
        frame->editor()->copyImage(m_hitTestResult);
        break;
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    case ContextMenuItemTagCopyImageUrlToClipboard:
#if ENABLE(TIZEN_CHILD_NODE_IMAGE_URL)
        {
            KURL imageURL = m_hitTestResult.absoluteImageURL();
            if (imageURL.isEmpty())
                imageURL = m_hitTestResult.findChildAbsoluteImageURL();
            frame->editor()->copyURL(imageURL, m_hitTestResult.textContent());
        }
#else
        frame->editor()->copyURL(m_hitTestResult.absoluteImageURL(), m_hitTestResult.textContent());
#endif
        break;
#endif
    case ContextMenuItemTagOpenMediaInNewWindow:
        openNewWindow(m_hitTestResult.absoluteMediaURL(), frame);
        break;
    case ContextMenuItemTagCopyMediaLinkToClipboard:
        frame->editor()->copyURL(m_hitTestResult.absoluteMediaURL(), m_hitTestResult.textContent());
        break;
    case ContextMenuItemTagToggleMediaControls:
        m_hitTestResult.toggleMediaControlsDisplay();
        break;
    case ContextMenuItemTagToggleMediaLoop:
        m_hitTestResult.toggleMediaLoopPlayback();
        break;
    case ContextMenuItemTagEnterVideoFullscreen:
        m_hitTestResult.enterFullscreenForVideo();
        break;
    case ContextMenuItemTagMediaPlayPause:
        m_hitTestResult.toggleMediaPlayState();
        break;
    case ContextMenuItemTagMediaMute:
        m_hitTestResult.toggleMediaMuteState();
        break;
    case ContextMenuItemTagOpenFrameInNewWindow: {
        DocumentLoader* loader = frame->loader()->documentLoader();
        if (!loader->unreachableURL().isEmpty())
            openNewWindow(loader->unreachableURL(), frame);
        else
            openNewWindow(loader->url(), frame);
        break;
    }
    case ContextMenuItemTagCopy: {
        frame->editor()->copy();
        break;
    }
    case ContextMenuItemTagGoBack:
        if (Page* page = frame->page())
            page->backForward()->goBackOrForward(-1);
        break;
    case ContextMenuItemTagGoForward:
        if (Page* page = frame->page())
            page->backForward()->goBackOrForward(1);
        break;
    case ContextMenuItemTagStop:
        frame->loader()->stop();
        break;
    case ContextMenuItemTagReload:
        frame->loader()->reload();
        break;
    case ContextMenuItemTagCut:
        frame->editor()->command("Cut").execute();
        break;
    case ContextMenuItemTagPaste:
        frame->editor()->command("Paste").execute();
        break;
#if PLATFORM(GTK)
    case ContextMenuItemTagDelete:
        frame->editor()->performDelete();
        break;
    case ContextMenuItemTagUnicodeInsertLRMMark:
        insertUnicodeCharacter(leftToRightMark, frame);
        break;
    case ContextMenuItemTagUnicodeInsertRLMMark:
        insertUnicodeCharacter(rightToLeftMark, frame);
        break;
    case ContextMenuItemTagUnicodeInsertLREMark:
        insertUnicodeCharacter(leftToRightEmbed, frame);
        break;
    case ContextMenuItemTagUnicodeInsertRLEMark:
        insertUnicodeCharacter(rightToLeftEmbed, frame);
        break;
    case ContextMenuItemTagUnicodeInsertLROMark:
        insertUnicodeCharacter(leftToRightOverride, frame);
        break;
    case ContextMenuItemTagUnicodeInsertRLOMark:
        insertUnicodeCharacter(rightToLeftOverride, frame);
        break;
    case ContextMenuItemTagUnicodeInsertPDFMark:
        insertUnicodeCharacter(popDirectionalFormatting, frame);
        break;
    case ContextMenuItemTagUnicodeInsertZWSMark:
        insertUnicodeCharacter(zeroWidthSpace, frame);
        break;
    case ContextMenuItemTagUnicodeInsertZWJMark:
        insertUnicodeCharacter(zeroWidthJoiner, frame);
        break;
    case ContextMenuItemTagUnicodeInsertZWNJMark:
        insertUnicodeCharacter(zeroWidthNonJoiner, frame);
        break;
#endif
#if PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL)
    case ContextMenuItemTagSelectAll:
        frame->editor()->command("SelectAll").execute();
        break;
#endif
#if ENABLE(TIZEN_CONTEXT_MENU_SELECT)
    case ContextMenuItemTagSelectWord:
        frame->editor()->command("SelectWord").execute();
        break;
#endif
    case ContextMenuItemTagSpellingGuess:
        ASSERT(frame->editor()->selectedText().length());
        if (frame->editor()->shouldInsertText(item->title(), frame->selection()->toNormalizedRange().get(), EditorInsertActionPasted)) {
            Document* document = frame->document();
            RefPtr<ReplaceSelectionCommand> command = ReplaceSelectionCommand::create(document, createFragmentFromMarkup(document, item->title(), ""), ReplaceSelectionCommand::SelectReplacement | ReplaceSelectionCommand::MatchStyle | ReplaceSelectionCommand::PreventNesting);
            applyCommand(command);
            frame->selection()->revealSelection(ScrollAlignment::alignToEdgeIfNeeded);
        }
        break;
    case ContextMenuItemTagIgnoreSpelling:
        frame->editor()->ignoreSpelling();
        break;
    case ContextMenuItemTagLearnSpelling:
        frame->editor()->learnSpelling();
        break;
    case ContextMenuItemTagSearchWeb:
        m_client->searchWithGoogle(frame);
        break;
    case ContextMenuItemTagLookUpInDictionary:
        // FIXME: Some day we may be able to do this from within WebCore.
        m_client->lookUpInDictionary(frame);
        break;
    case ContextMenuItemTagOpenLink:
        if (Frame* targetFrame = m_hitTestResult.targetFrame())
            targetFrame->loader()->loadFrameRequest(FrameLoadRequest(frame->document()->securityOrigin(), ResourceRequest(m_hitTestResult.absoluteLinkURL(), frame->loader()->outgoingReferrer())), false, false, 0, 0, MaybeSendReferrer);
        else
            openNewWindow(m_hitTestResult.absoluteLinkURL(), frame);
        break;
    case ContextMenuItemTagBold:
        frame->editor()->command("ToggleBold").execute();
        break;
    case ContextMenuItemTagItalic:
        frame->editor()->command("ToggleItalic").execute();
        break;
    case ContextMenuItemTagUnderline:
        frame->editor()->toggleUnderline();
        break;
    case ContextMenuItemTagOutline:
        // We actually never enable this because CSS does not have a way to specify an outline font,
        // which may make this difficult to implement. Maybe a special case of text-shadow?
        break;
    case ContextMenuItemTagStartSpeaking: {
        ExceptionCode ec;
        RefPtr<Range> selectedRange = frame->selection()->toNormalizedRange();
        if (!selectedRange || selectedRange->collapsed(ec)) {
            Document* document = m_hitTestResult.innerNonSharedNode()->document();
            selectedRange = document->createRange();
            selectedRange->selectNode(document->documentElement(), ec);
        }
        m_client->speak(plainText(selectedRange.get()));
        break;
    }
    case ContextMenuItemTagStopSpeaking:
        m_client->stopSpeaking();
        break;
    case ContextMenuItemTagDefaultDirection:
        frame->editor()->setBaseWritingDirection(NaturalWritingDirection);
        break;
    case ContextMenuItemTagLeftToRight:
        frame->editor()->setBaseWritingDirection(LeftToRightWritingDirection);
        break;
    case ContextMenuItemTagRightToLeft:
        frame->editor()->setBaseWritingDirection(RightToLeftWritingDirection);
        break;
    case ContextMenuItemTagTextDirectionDefault:
        frame->editor()->command("MakeTextWritingDirectionNatural").execute();
        break;
    case ContextMenuItemTagTextDirectionLeftToRight:
        frame->editor()->command("MakeTextWritingDirectionLeftToRight").execute();
        break;
    case ContextMenuItemTagTextDirectionRightToLeft:
        frame->editor()->command("MakeTextWritingDirectionRightToLeft").execute();
        break;
#if PLATFORM(MAC)
    case ContextMenuItemTagSearchInSpotlight:
        m_client->searchWithSpotlight();
        break;
#endif
    case ContextMenuItemTagShowSpellingPanel:
        frame->editor()->showSpellingGuessPanel();
        break;
    case ContextMenuItemTagCheckSpelling:
        frame->editor()->advanceToNextMisspelling();
        break;
    case ContextMenuItemTagCheckSpellingWhileTyping:
        frame->editor()->toggleContinuousSpellChecking();
        break;
    case ContextMenuItemTagCheckGrammarWithSpelling:
        frame->editor()->toggleGrammarChecking();
        break;
#if PLATFORM(MAC)
    case ContextMenuItemTagShowFonts:
        frame->editor()->showFontPanel();
        break;
    case ContextMenuItemTagStyles:
        frame->editor()->showStylesPanel();
        break;
    case ContextMenuItemTagShowColors:
        frame->editor()->showColorPanel();
        break;
#endif
#if USE(APPKIT)
    case ContextMenuItemTagMakeUpperCase:
        frame->editor()->uppercaseWord();
        break;
    case ContextMenuItemTagMakeLowerCase:
        frame->editor()->lowercaseWord();
        break;
    case ContextMenuItemTagCapitalize:
        frame->editor()->capitalizeWord();
        break;
#endif
#if PLATFORM(MAC)
    case ContextMenuItemTagChangeBack:
        frame->editor()->changeBackToReplacedString(m_hitTestResult.replacedString());
        break;
#endif
#if USE(AUTOMATIC_TEXT_REPLACEMENT)
    case ContextMenuItemTagShowSubstitutions:
        frame->editor()->showSubstitutionsPanel();
        break;
    case ContextMenuItemTagSmartCopyPaste:
        frame->editor()->toggleSmartInsertDelete();
        break;
    case ContextMenuItemTagSmartQuotes:
        frame->editor()->toggleAutomaticQuoteSubstitution();
        break;
    case ContextMenuItemTagSmartDashes:
        frame->editor()->toggleAutomaticDashSubstitution();
        break;
    case ContextMenuItemTagSmartLinks:
        frame->editor()->toggleAutomaticLinkDetection();
        break;
    case ContextMenuItemTagTextReplacement:
        frame->editor()->toggleAutomaticTextReplacement();
        break;
    case ContextMenuItemTagCorrectSpellingAutomatically:
        frame->editor()->toggleAutomaticSpellingCorrection();
        break;
#endif
#if ENABLE(INSPECTOR)
    case ContextMenuItemTagInspectElement:
        if (Page* page = frame->page())
            page->inspectorController()->inspect(m_hitTestResult.innerNonSharedNode());
        break;
#endif
    case ContextMenuItemTagDictationAlternative:
        frame->editor()->applyDictationAlternativelternative(item->title());
        break;
#if ENABLE(TIZEN_DRAG_SUPPORT)
    case ContextMenuItemTagDrag:
        TIZEN_LOGI("ContextMenuItemTagDrag");
        if (Page* page = frame->page()) {
            page->dragController()->setDragState(true);
            frame->eventHandler()->handleMousePressEvent(page->dragController()->dragPositionEvent());
            page->dragController()->setDragState(false);
        }
        break;
#endif
    default:
        break;
    }
}

void ContextMenuController::appendItem(ContextMenuItem& menuItem, ContextMenu* parentMenu)
{
    checkOrEnableIfNeeded(menuItem);
    if (parentMenu)
        parentMenu->appendItem(menuItem);
}

void ContextMenuController::createAndAppendFontSubMenu(ContextMenuItem& fontMenuItem)
{
    ContextMenu fontMenu;

#if PLATFORM(MAC)
    ContextMenuItem showFonts(ActionType, ContextMenuItemTagShowFonts, contextMenuItemTagShowFonts());
#endif
    ContextMenuItem bold(CheckableActionType, ContextMenuItemTagBold, contextMenuItemTagBold());
    ContextMenuItem italic(CheckableActionType, ContextMenuItemTagItalic, contextMenuItemTagItalic());
    ContextMenuItem underline(CheckableActionType, ContextMenuItemTagUnderline, contextMenuItemTagUnderline());
    ContextMenuItem outline(ActionType, ContextMenuItemTagOutline, contextMenuItemTagOutline());
#if PLATFORM(MAC)
    ContextMenuItem styles(ActionType, ContextMenuItemTagStyles, contextMenuItemTagStyles());
    ContextMenuItem showColors(ActionType, ContextMenuItemTagShowColors, contextMenuItemTagShowColors());
#endif

#if PLATFORM(MAC)
    appendItem(showFonts, &fontMenu);
#endif
    appendItem(bold, &fontMenu);
    appendItem(italic, &fontMenu);
    appendItem(underline, &fontMenu);
    appendItem(outline, &fontMenu);
#if PLATFORM(MAC)
    appendItem(styles, &fontMenu);
    appendItem(*separatorItem(), &fontMenu);
    appendItem(showColors, &fontMenu);
#endif

    fontMenuItem.setSubMenu(&fontMenu);
}


#if !PLATFORM(GTK)

void ContextMenuController::createAndAppendSpellingAndGrammarSubMenu(ContextMenuItem& spellingAndGrammarMenuItem)
{
    ContextMenu spellingAndGrammarMenu;

    ContextMenuItem showSpellingPanel(ActionType, ContextMenuItemTagShowSpellingPanel, 
        contextMenuItemTagShowSpellingPanel(true));
    ContextMenuItem checkSpelling(ActionType, ContextMenuItemTagCheckSpelling, 
        contextMenuItemTagCheckSpelling());
    ContextMenuItem checkAsYouType(CheckableActionType, ContextMenuItemTagCheckSpellingWhileTyping, 
        contextMenuItemTagCheckSpellingWhileTyping());
    ContextMenuItem grammarWithSpelling(CheckableActionType, ContextMenuItemTagCheckGrammarWithSpelling, 
        contextMenuItemTagCheckGrammarWithSpelling());
#if PLATFORM(MAC)
    ContextMenuItem correctSpelling(CheckableActionType, ContextMenuItemTagCorrectSpellingAutomatically, 
        contextMenuItemTagCorrectSpellingAutomatically());
#endif

    appendItem(showSpellingPanel, &spellingAndGrammarMenu);
    appendItem(checkSpelling, &spellingAndGrammarMenu);
#if PLATFORM(MAC)
    appendItem(*separatorItem(), &spellingAndGrammarMenu);
#endif
    appendItem(checkAsYouType, &spellingAndGrammarMenu);
    appendItem(grammarWithSpelling, &spellingAndGrammarMenu);
#if PLATFORM(MAC)
    appendItem(correctSpelling, &spellingAndGrammarMenu);
#endif

    spellingAndGrammarMenuItem.setSubMenu(&spellingAndGrammarMenu);
}

#endif // !PLATFORM(GTK)


#if PLATFORM(MAC)

void ContextMenuController::createAndAppendSpeechSubMenu(ContextMenuItem& speechMenuItem)
{
    ContextMenu speechMenu;

    ContextMenuItem start(ActionType, ContextMenuItemTagStartSpeaking, contextMenuItemTagStartSpeaking());
    ContextMenuItem stop(ActionType, ContextMenuItemTagStopSpeaking, contextMenuItemTagStopSpeaking());

    appendItem(start, &speechMenu);
    appendItem(stop, &speechMenu);

    speechMenuItem.setSubMenu(&speechMenu);
}

#endif
 
#if PLATFORM(GTK)

void ContextMenuController::createAndAppendUnicodeSubMenu(ContextMenuItem& unicodeMenuItem)
{
    ContextMenu unicodeMenu;

    ContextMenuItem leftToRightMarkMenuItem(ActionType, ContextMenuItemTagUnicodeInsertLRMMark, contextMenuItemTagUnicodeInsertLRMMark());
    ContextMenuItem rightToLeftMarkMenuItem(ActionType, ContextMenuItemTagUnicodeInsertRLMMark, contextMenuItemTagUnicodeInsertRLMMark());
    ContextMenuItem leftToRightEmbedMenuItem(ActionType, ContextMenuItemTagUnicodeInsertLREMark, contextMenuItemTagUnicodeInsertLREMark());
    ContextMenuItem rightToLeftEmbedMenuItem(ActionType, ContextMenuItemTagUnicodeInsertRLEMark, contextMenuItemTagUnicodeInsertRLEMark());
    ContextMenuItem leftToRightOverrideMenuItem(ActionType, ContextMenuItemTagUnicodeInsertLROMark, contextMenuItemTagUnicodeInsertLROMark());
    ContextMenuItem rightToLeftOverrideMenuItem(ActionType, ContextMenuItemTagUnicodeInsertRLOMark, contextMenuItemTagUnicodeInsertRLOMark());
    ContextMenuItem popDirectionalFormattingMenuItem(ActionType, ContextMenuItemTagUnicodeInsertPDFMark, contextMenuItemTagUnicodeInsertPDFMark());
    ContextMenuItem zeroWidthSpaceMenuItem(ActionType, ContextMenuItemTagUnicodeInsertZWSMark, contextMenuItemTagUnicodeInsertZWSMark());
    ContextMenuItem zeroWidthJoinerMenuItem(ActionType, ContextMenuItemTagUnicodeInsertZWJMark, contextMenuItemTagUnicodeInsertZWJMark());
    ContextMenuItem zeroWidthNonJoinerMenuItem(ActionType, ContextMenuItemTagUnicodeInsertZWNJMark, contextMenuItemTagUnicodeInsertZWNJMark());

    appendItem(leftToRightMarkMenuItem, &unicodeMenu);
    appendItem(rightToLeftMarkMenuItem, &unicodeMenu);
    appendItem(leftToRightEmbedMenuItem, &unicodeMenu);
    appendItem(rightToLeftEmbedMenuItem, &unicodeMenu);
    appendItem(leftToRightOverrideMenuItem, &unicodeMenu);
    appendItem(rightToLeftOverrideMenuItem, &unicodeMenu);
    appendItem(popDirectionalFormattingMenuItem, &unicodeMenu);
    appendItem(zeroWidthSpaceMenuItem, &unicodeMenu);
    appendItem(zeroWidthJoinerMenuItem, &unicodeMenu);
    appendItem(zeroWidthNonJoinerMenuItem, &unicodeMenu);

    unicodeMenuItem.setSubMenu(&unicodeMenu);
}

#else

void ContextMenuController::createAndAppendWritingDirectionSubMenu(ContextMenuItem& writingDirectionMenuItem)
{
    ContextMenu writingDirectionMenu;

    ContextMenuItem defaultItem(ActionType, ContextMenuItemTagDefaultDirection, 
        contextMenuItemTagDefaultDirection());
    ContextMenuItem ltr(CheckableActionType, ContextMenuItemTagLeftToRight, contextMenuItemTagLeftToRight());
    ContextMenuItem rtl(CheckableActionType, ContextMenuItemTagRightToLeft, contextMenuItemTagRightToLeft());

    appendItem(defaultItem, &writingDirectionMenu);
    appendItem(ltr, &writingDirectionMenu);
    appendItem(rtl, &writingDirectionMenu);

    writingDirectionMenuItem.setSubMenu(&writingDirectionMenu);
}

void ContextMenuController::createAndAppendTextDirectionSubMenu(ContextMenuItem& textDirectionMenuItem)
{
    ContextMenu textDirectionMenu;

    ContextMenuItem defaultItem(ActionType, ContextMenuItemTagTextDirectionDefault, contextMenuItemTagDefaultDirection());
    ContextMenuItem ltr(CheckableActionType, ContextMenuItemTagTextDirectionLeftToRight, contextMenuItemTagLeftToRight());
    ContextMenuItem rtl(CheckableActionType, ContextMenuItemTagTextDirectionRightToLeft, contextMenuItemTagRightToLeft());

    appendItem(defaultItem, &textDirectionMenu);
    appendItem(ltr, &textDirectionMenu);
    appendItem(rtl, &textDirectionMenu);

    textDirectionMenuItem.setSubMenu(&textDirectionMenu);
}

#endif

#if PLATFORM(MAC)

void ContextMenuController::createAndAppendSubstitutionsSubMenu(ContextMenuItem& substitutionsMenuItem)
{
    ContextMenu substitutionsMenu;

    ContextMenuItem showSubstitutions(ActionType, ContextMenuItemTagShowSubstitutions, contextMenuItemTagShowSubstitutions(true));
    ContextMenuItem smartCopyPaste(CheckableActionType, ContextMenuItemTagSmartCopyPaste, contextMenuItemTagSmartCopyPaste());
    ContextMenuItem smartQuotes(CheckableActionType, ContextMenuItemTagSmartQuotes, contextMenuItemTagSmartQuotes());
    ContextMenuItem smartDashes(CheckableActionType, ContextMenuItemTagSmartDashes, contextMenuItemTagSmartDashes());
    ContextMenuItem smartLinks(CheckableActionType, ContextMenuItemTagSmartLinks, contextMenuItemTagSmartLinks());
    ContextMenuItem textReplacement(CheckableActionType, ContextMenuItemTagTextReplacement, contextMenuItemTagTextReplacement());

    appendItem(showSubstitutions, &substitutionsMenu);
    appendItem(*separatorItem(), &substitutionsMenu);
    appendItem(smartCopyPaste, &substitutionsMenu);
    appendItem(smartQuotes, &substitutionsMenu);
    appendItem(smartDashes, &substitutionsMenu);
    appendItem(smartLinks, &substitutionsMenu);
    appendItem(textReplacement, &substitutionsMenu);

    substitutionsMenuItem.setSubMenu(&substitutionsMenu);
}

void ContextMenuController::createAndAppendTransformationsSubMenu(ContextMenuItem& transformationsMenuItem)
{
    ContextMenu transformationsMenu;

    ContextMenuItem makeUpperCase(ActionType, ContextMenuItemTagMakeUpperCase, contextMenuItemTagMakeUpperCase());
    ContextMenuItem makeLowerCase(ActionType, ContextMenuItemTagMakeLowerCase, contextMenuItemTagMakeLowerCase());
    ContextMenuItem capitalize(ActionType, ContextMenuItemTagCapitalize, contextMenuItemTagCapitalize());

    appendItem(makeUpperCase, &transformationsMenu);
    appendItem(makeLowerCase, &transformationsMenu);
    appendItem(capitalize, &transformationsMenu);

    transformationsMenuItem.setSubMenu(&transformationsMenu);
}

#endif

static bool selectionContainsPossibleWord(Frame* frame)
{
    // Current algorithm: look for a character that's not just a separator.
    for (TextIterator it(frame->selection()->toNormalizedRange().get()); !it.atEnd(); it.advance()) {
        int length = it.length();
        const UChar* characters = it.characters();
        for (int i = 0; i < length; ++i)
            if (!(category(characters[i]) & (Separator_Space | Separator_Line | Separator_Paragraph)))
                return true;
    }
    return false;
}

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
static bool canWordSelect(Frame* frame)
{
    // Keep this in same with the WordGranularity case of VisibleSelection::setStartAndEndFromBaseAndExtentRespectingGranularity() function
    // General case: Select the word the caret is positioned inside of, or at the start of (RightWordIfOnBoundary).
    // Edge case: If the caret is after the last word in a soft-wrapped line or the last word in
    // the document, select that last word (LeftWordIfOnBoundary).
    // Edge case: If the caret is after the last word in a paragraph, select from the the end of the
    // last word to the line break (also RightWordIfOnBoundary);
    Position startPositionOfWord, endPositionOfWord;
    VisiblePosition start = frame->selection()->selection().visibleStart();
    VisiblePosition originalEnd = frame->selection()->selection().visibleEnd();
    EWordSide side = RightWordIfOnBoundary;
    if (isEndOfEditableOrNonEditableContent(start) || (isEndOfLine(start) && !isStartOfLine(start) && !isEndOfParagraph(start)))
        side = LeftWordIfOnBoundary;
    startPositionOfWord = startOfWord(start, side).deepEquivalent();
    side = RightWordIfOnBoundary;
    if (isEndOfEditableOrNonEditableContent(originalEnd) || (isEndOfLine(originalEnd) && !isStartOfLine(originalEnd) && !isEndOfParagraph(originalEnd)))
        side = LeftWordIfOnBoundary;

    VisiblePosition wordEnd(endOfWord(originalEnd, side));
    VisiblePosition end(wordEnd);

    if (isEndOfParagraph(originalEnd) && !isEmptyTableCell(startPositionOfWord.deprecatedNode())) {
        // Select the paragraph break (the space from the end of a paragraph to the start of
        // the next one) to match TextEdit.
        end = wordEnd.next();

        if (Node* table = isFirstPositionAfterTable(end)) {
            // The paragraph break after the last paragraph in the last cell of a block table ends
            // at the start of the paragraph after the table.
            if (isBlock(table))
                end = end.next(CannotCrossEditingBoundary);
            else
                end = wordEnd;
        }

        if (end.isNull())
            end = wordEnd;
    }

    endPositionOfWord = end.deepEquivalent();

    if (comparePositions(startPositionOfWord, endPositionOfWord))
        return true;

    return false;
}
#endif

#if PLATFORM(MAC)
#if __MAC_OS_X_VERSION_MIN_REQUIRED == 1060
#define INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM 1
#else
#define INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM 0
#endif
#endif

void ContextMenuController::populate()
{
    ContextMenuItem OpenLinkItem(ActionType, ContextMenuItemTagOpenLink, contextMenuItemTagOpenLink());
    ContextMenuItem OpenLinkInNewWindowItem(ActionType, ContextMenuItemTagOpenLinkInNewWindow, 
        contextMenuItemTagOpenLinkInNewWindow());
    ContextMenuItem DownloadFileItem(ActionType, ContextMenuItemTagDownloadLinkToDisk, 
        contextMenuItemTagDownloadLinkToDisk());
    ContextMenuItem CopyLinkItem(ActionType, ContextMenuItemTagCopyLinkToClipboard, 
        contextMenuItemTagCopyLinkToClipboard());
    ContextMenuItem OpenImageInNewWindowItem(ActionType, ContextMenuItemTagOpenImageInNewWindow, 
        contextMenuItemTagOpenImageInNewWindow());
#if ENABLE(TIZEN_OPEN_IMAGE_IN_CURRENT_TAB)
    ContextMenuItem OpenImageInCurrentWindowItem(ActionType, ContextMenuItemTagOpenImageInCurrentWindow,
        contextMenuItemTagOpenImageInCurrentWindow());
#endif
    ContextMenuItem DownloadImageItem(ActionType, ContextMenuItemTagDownloadImageToDisk, 
        contextMenuItemTagDownloadImageToDisk());
    ContextMenuItem CopyImageItem(ActionType, ContextMenuItemTagCopyImageToClipboard, 
        contextMenuItemTagCopyImageToClipboard());
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    ContextMenuItem CopyImageUrlItem(ActionType, ContextMenuItemTagCopyImageUrlToClipboard, 
        contextMenuItemTagCopyImageUrlToClipboard());
#endif
    ContextMenuItem OpenMediaInNewWindowItem(ActionType, ContextMenuItemTagOpenMediaInNewWindow, String());
    ContextMenuItem CopyMediaLinkItem(ActionType, ContextMenuItemTagCopyMediaLinkToClipboard, 
        String());
    ContextMenuItem MediaPlayPause(ActionType, ContextMenuItemTagMediaPlayPause, 
        contextMenuItemTagMediaPlay());
    ContextMenuItem MediaMute(ActionType, ContextMenuItemTagMediaMute, 
        contextMenuItemTagMediaMute());
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    ContextMenuItem MediaUnMute(ActionType, ContextMenuItemTagMediaMute,
        contextMenuItemTagMediaUnMute());
#endif
    ContextMenuItem ToggleMediaControls(CheckableActionType, ContextMenuItemTagToggleMediaControls, 
        contextMenuItemTagToggleMediaControls());
    ContextMenuItem ToggleMediaLoop(CheckableActionType, ContextMenuItemTagToggleMediaLoop, 
        contextMenuItemTagToggleMediaLoop());
    ContextMenuItem EnterVideoFullscreen(ActionType, ContextMenuItemTagEnterVideoFullscreen, 
        contextMenuItemTagEnterVideoFullscreen());
#if PLATFORM(MAC)
    ContextMenuItem SearchSpotlightItem(ActionType, ContextMenuItemTagSearchInSpotlight, 
        contextMenuItemTagSearchInSpotlight());
#endif
#if !PLATFORM(GTK)
    ContextMenuItem SearchWebItem(ActionType, ContextMenuItemTagSearchWeb, contextMenuItemTagSearchWeb());
#endif
    ContextMenuItem CopyItem(ActionType, ContextMenuItemTagCopy, contextMenuItemTagCopy());
    ContextMenuItem BackItem(ActionType, ContextMenuItemTagGoBack, contextMenuItemTagGoBack());
    ContextMenuItem ForwardItem(ActionType, ContextMenuItemTagGoForward,  contextMenuItemTagGoForward());
    ContextMenuItem StopItem(ActionType, ContextMenuItemTagStop, contextMenuItemTagStop());
    ContextMenuItem ReloadItem(ActionType, ContextMenuItemTagReload, contextMenuItemTagReload());
    ContextMenuItem OpenFrameItem(ActionType, ContextMenuItemTagOpenFrameInNewWindow, 
        contextMenuItemTagOpenFrameInNewWindow());
    ContextMenuItem NoGuessesItem(ActionType, ContextMenuItemTagNoGuessesFound, 
        contextMenuItemTagNoGuessesFound());
    ContextMenuItem IgnoreSpellingItem(ActionType, ContextMenuItemTagIgnoreSpelling, 
        contextMenuItemTagIgnoreSpelling());
    ContextMenuItem LearnSpellingItem(ActionType, ContextMenuItemTagLearnSpelling, 
        contextMenuItemTagLearnSpelling());
    ContextMenuItem IgnoreGrammarItem(ActionType, ContextMenuItemTagIgnoreGrammar, 
        contextMenuItemTagIgnoreGrammar());
    ContextMenuItem CutItem(ActionType, ContextMenuItemTagCut, contextMenuItemTagCut());
    ContextMenuItem PasteItem(ActionType, ContextMenuItemTagPaste, contextMenuItemTagPaste());
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    ContextMenuItem ClipboardItem(ActionType, ContextMenuItemTagClipboard, contextMenuItemTagClipboard());
#endif
#if PLATFORM(GTK)
    ContextMenuItem DeleteItem(ActionType, ContextMenuItemTagDelete, contextMenuItemTagDelete());
#endif
#if PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL)
    ContextMenuItem SelectAllItem(ActionType, ContextMenuItemTagSelectAll, contextMenuItemTagSelectAll());
#endif
#if ENABLE(TIZEN_CONTEXT_MENU_SELECT)
    ContextMenuItem SelectWordItem(ActionType, ContextMenuItemTagSelectWord, contextMenuItemTagSelectWord());
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
    ContextMenuItem TextSelectionModeItem(ActionType, ContextMenuItemTagTextSelectionMode, contextMenuItemTagTextSelectionMode());
#endif
#if ENABLE(TIZEN_DRAG_SUPPORT)
    ContextMenuItem DragItem(ActionType, ContextMenuItemTagDrag, contextMenuItemTagDrag());
#endif
#if ENABLE(TIZEN_WEBKIT2_COPY_MENU_FOR_SPECIFIC_SCHEME_LINK)
    ContextMenuItem CopyLinkDataItem(ActionType, ContextMenuItemTagCopyLinkData, contextMenuItemTagCopy());
#endif

    Node* node = m_hitTestResult.innerNonSharedNode();
    if (!node)
        return;
#if PLATFORM(GTK)
    if (!m_hitTestResult.isContentEditable() && (node->isElementNode() && static_cast<Element*>(node)->isFormControlElement()))
        return;
#endif
    Frame* frame = node->document()->frame();
    if (!frame)
        return;

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    bool isTouchPointOnSelectionMultiNode = false;
    Frame* focusedFrame = frame->page()->focusController()->focusedFrame();
    if (!(focusedFrame && focusedFrame->selection()))
        focusedFrame = frame;
    if (focusedFrame->selection()->isRange()) {
        Node* startNode = focusedFrame->selection()->start().deprecatedNode();
        Node* endNode = focusedFrame->selection()->end().deprecatedNode();
        if (!m_hitTestResult.isContentEditable() && (startNode->rendererIsEditable() || endNode->rendererIsEditable()))
            isTouchPointOnSelectionMultiNode = true;
        if (m_hitTestResult.isContentEditable() && (!startNode->rendererIsEditable() || !endNode->rendererIsEditable()))
            isTouchPointOnSelectionMultiNode = true;
    }

    if (!m_hitTestResult.isContentEditable() || isTouchPointOnSelectionMultiNode) {
        if (m_hitTestResult.isSelected() || focusedFrame->selection()->isRange())
            appendItem(SelectAllItem, m_contextMenu.get());
        if (focusedFrame->selection()->isRange()) {
            if (selectionContainsPossibleWord(focusedFrame))
                appendItem(SearchWebItem, m_contextMenu.get());
            appendItem(CopyItem, m_contextMenu.get());
            return;
        }
#else
    if (!m_hitTestResult.isContentEditable()) {
#endif

        FrameLoader* loader = frame->loader();
        KURL linkURL = m_hitTestResult.absoluteLinkURL();
        if (!linkURL.isEmpty()) {
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
            if (m_hitTestResult.isSelected() || frame->selection()->isRange()) {
                if (selectionContainsPossibleWord(frame))
                    appendItem(SearchWebItem, m_contextMenu.get());
                appendItem(CopyItem, m_contextMenu.get());
                return;
            }
#endif
#if ENABLE(TIZEN_DOWNLOAD_LINK_FILTER)
            if (loader->client()->canHandleRequest(ResourceRequest(linkURL)) &&
               (linkURL.protocolIs("http") || linkURL.protocolIs("https") || linkURL.protocolIs("ftp") || linkURL.protocolIs("ftps"))) {
#else
            if (loader->client()->canHandleRequest(ResourceRequest(linkURL))) {
#endif
                appendItem(OpenLinkItem, m_contextMenu.get());
                appendItem(OpenLinkInNewWindowItem, m_contextMenu.get());
                appendItem(DownloadFileItem, m_contextMenu.get());
            }
#if PLATFORM(QT)
            if (m_hitTestResult.isSelected()) 
                appendItem(CopyItem, m_contextMenu.get());
#endif
#if ENABLE(TIZEN_WEBKIT2_COPY_MENU_FOR_SPECIFIC_SCHEME_LINK)
            if (linkURL.protocolIs("mailto") || linkURL.protocolIs("tel"))
                appendItem(CopyLinkDataItem, m_contextMenu.get());
            else if(!linkURL.isEmpty() && linkURL.string() != String("javascript:;"))
#endif
                appendItem(CopyLinkItem, m_contextMenu.get());
        }

        KURL imageURL = m_hitTestResult.absoluteImageURL();
#if ENABLE(TIZEN_CHILD_NODE_IMAGE_URL)
        if (imageURL.isEmpty())
            imageURL = m_hitTestResult.findChildAbsoluteImageURL();
#endif
        if (!imageURL.isEmpty()) {
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
            if (m_hitTestResult.isSelected() || frame->selection()->isRange()) {
                appendItem(CopyItem, m_contextMenu.get());
                return;
            }
#endif
            if (!linkURL.isEmpty())
                appendItem(*separatorItem(), m_contextMenu.get());

            appendItem(OpenImageInNewWindowItem, m_contextMenu.get());
            appendItem(DownloadImageItem, m_contextMenu.get());
#if ENABLE(TIZEN_CHILD_NODE_IMAGE_URL)
            if (imageURL.isLocalFile() || m_hitTestResult.image() || m_hitTestResult.childNodeImage())
#else
            if (imageURL.isLocalFile() || m_hitTestResult.image())
#endif
                appendItem(CopyImageItem, m_contextMenu.get());
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
            appendItem(CopyImageUrlItem, m_contextMenu.get());
#endif
        }

        KURL mediaURL = m_hitTestResult.absoluteMediaURL();
        if (!mediaURL.isEmpty()) {
            if (!linkURL.isEmpty() || !imageURL.isEmpty())
                appendItem(*separatorItem(), m_contextMenu.get());

            appendItem(MediaPlayPause, m_contextMenu.get());
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
            if (m_hitTestResult.mediaMuted())
                appendItem(MediaUnMute, m_contextMenu.get());
            else
                appendItem(MediaMute, m_contextMenu.get());
#else
            appendItem(MediaMute, m_contextMenu.get());
#endif
            appendItem(ToggleMediaControls, m_contextMenu.get());
            appendItem(ToggleMediaLoop, m_contextMenu.get());
            appendItem(EnterVideoFullscreen, m_contextMenu.get());

            appendItem(*separatorItem(), m_contextMenu.get());
            appendItem(CopyMediaLinkItem, m_contextMenu.get());
            appendItem(OpenMediaInNewWindowItem, m_contextMenu.get());
        }
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
        if (!linkURL.isEmpty() && imageURL.isEmpty() && mediaURL.isEmpty()) {
            if (node->renderer() && node->renderer()->style()
                && (node->renderer()->style()->userSelect() != SELECT_NONE)
                && !(static_cast<HTMLAnchorElement*>(node)->text().isEmpty()))
                appendItem(TextSelectionModeItem, m_contextMenu.get());
        }
#endif

#if ENABLE(TIZEN_DRAG_SUPPORT)
        if (imageURL.isEmpty() && linkURL.isEmpty() && mediaURL.isEmpty() && !m_hitTestResult.isDragSupport()) {
#else
        if (imageURL.isEmpty() && linkURL.isEmpty() && mediaURL.isEmpty()) {
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
            if (m_hitTestResult.isSelected() || frame->selection()->isRange()) {
#else
            if (m_hitTestResult.isSelected()) {
#endif
                if (selectionContainsPossibleWord(frame)) {
#if PLATFORM(MAC)
                    String selectedString = frame->displayStringModifiedByEncoding(frame->editor()->selectedText());
                    ContextMenuItem LookUpInDictionaryItem(ActionType, ContextMenuItemTagLookUpInDictionary, contextMenuItemTagLookUpInDictionary(selectedString));

#if INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM
                    appendItem(SearchSpotlightItem, m_contextMenu.get());
#else
                    appendItem(LookUpInDictionaryItem, m_contextMenu.get());
#endif
#endif

#if !PLATFORM(GTK)
                    appendItem(SearchWebItem, m_contextMenu.get());
                    appendItem(*separatorItem(), m_contextMenu.get());
#endif

#if PLATFORM(MAC) && INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM
                    appendItem(LookUpInDictionaryItem, m_contextMenu.get());
                    appendItem(*separatorItem(), m_contextMenu.get());
#endif
                }

                appendItem(CopyItem, m_contextMenu.get());
#if PLATFORM(MAC)
                appendItem(*separatorItem(), m_contextMenu.get());

                ContextMenuItem SpeechMenuItem(SubmenuType, ContextMenuItemTagSpeechMenu, contextMenuItemTagSpeechMenu());
                createAndAppendSpeechSubMenu(SpeechMenuItem);
                appendItem(SpeechMenuItem, m_contextMenu.get());
#endif                
            } else {
#if ENABLE(INSPECTOR)
                if (!(frame->page() && frame->page()->inspectorController()->hasInspectorFrontendClient())) {
#endif

                // In GTK+ unavailable items are not hidden but insensitive
#if PLATFORM(GTK)
                appendItem(BackItem, m_contextMenu.get());
                appendItem(ForwardItem, m_contextMenu.get());
                appendItem(StopItem, m_contextMenu.get());
                appendItem(ReloadItem, m_contextMenu.get());
#else
#if !ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
                if (frame->page() && frame->page()->backForward()->canGoBackOrForward(-1))
                    appendItem(BackItem, m_contextMenu.get());

                if (frame->page() && frame->page()->backForward()->canGoBackOrForward(1))
                    appendItem(ForwardItem, m_contextMenu.get());

                // use isLoadingInAPISense rather than isLoading because Stop/Reload are
                // intended to match WebKit's API, not WebCore's internal notion of loading status
                if (loader->documentLoader()->isLoadingInAPISense())
                    appendItem(StopItem, m_contextMenu.get());
                else
                    appendItem(ReloadItem, m_contextMenu.get());
#endif
#endif
#if ENABLE(INSPECTOR)
                }
#endif

                if (frame->page() && frame != frame->page()->mainFrame())
                    appendItem(OpenFrameItem, m_contextMenu.get());
            }
        }
#if ENABLE(TIZEN_DRAG_SUPPORT)
        if (frame->page() && m_hitTestResult.isDragSupport()) {
            TIZEN_LOGI("appendItem for drag");
            appendItem(DragItem, m_contextMenu.get());
        }
#endif
    } else { // Make an editing context menu
        FrameSelection* selection = frame->selection();
        bool inPasswordField = selection->isInPasswordField();
        if (!inPasswordField) {
            bool haveContextMenuItemsForMisspellingOrGrammer = false;
            bool spellCheckingEnabled = frame->editor()->isSpellCheckingEnabledFor(node);
            if (spellCheckingEnabled) {
                // Consider adding spelling-related or grammar-related context menu items (never both, since a single selected range
                // is never considered a misspelling and bad grammar at the same time)
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
                bool misspelling = false;
                bool badGrammar = false;
#else
                bool misspelling;
                bool badGrammar;
#endif
                Vector<String> guesses = frame->editor()->guessesForMisspelledOrUngrammaticalSelection(misspelling, badGrammar);
                if (misspelling || badGrammar) {
                    size_t size = guesses.size();
                    if (!size) {
                        // If there's bad grammar but no suggestions (e.g., repeated word), just leave off the suggestions
                        // list and trailing separator rather than adding a "No Guesses Found" item (matches AppKit)
                        if (misspelling) {
                            appendItem(NoGuessesItem, m_contextMenu.get());
                            appendItem(*separatorItem(), m_contextMenu.get());
                        }
                    } else {
                        for (unsigned i = 0; i < size; i++) {
                            const String &guess = guesses[i];
                            if (!guess.isEmpty()) {
                                ContextMenuItem item(ActionType, ContextMenuItemTagSpellingGuess, guess);
                                appendItem(item, m_contextMenu.get());
                            }
                        }
                        appendItem(*separatorItem(), m_contextMenu.get());
                    }
                    if (misspelling) {
                        appendItem(IgnoreSpellingItem, m_contextMenu.get());
                        appendItem(LearnSpellingItem, m_contextMenu.get());
                    } else
                        appendItem(IgnoreGrammarItem, m_contextMenu.get());
                    appendItem(*separatorItem(), m_contextMenu.get());
                    haveContextMenuItemsForMisspellingOrGrammer = true;
#if PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1060
                } else {
                    // If the string was autocorrected, generate a contextual menu item allowing it to be changed back.
                    String replacedString = m_hitTestResult.replacedString();
                    if (!replacedString.isEmpty()) {
                        ContextMenuItem item(ActionType, ContextMenuItemTagChangeBack, contextMenuItemTagChangeBack(replacedString));
                        appendItem(item, m_contextMenu.get());
                        appendItem(*separatorItem(), m_contextMenu.get());
                        haveContextMenuItemsForMisspellingOrGrammer = true;
                    }
#endif
                }
            }

            if (!haveContextMenuItemsForMisspellingOrGrammer) {
                // Spelling and grammar checking is mutually exclusive with dictation alternatives.
                Vector<String> dictationAlternatives = m_hitTestResult.dictationAlternatives();
                if (!dictationAlternatives.isEmpty()) {
                    for (size_t i = 0; i < dictationAlternatives.size(); ++i) {
                        ContextMenuItem item(ActionType, ContextMenuItemTagDictationAlternative, dictationAlternatives[i]);
                        appendItem(item, m_contextMenu.get());
                    }
                    appendItem(*separatorItem(), m_contextMenu.get());
                }
            }
        }

        FrameLoader* loader = frame->loader();
        KURL linkURL = m_hitTestResult.absoluteLinkURL();
        if (!linkURL.isEmpty()) {
            if (loader->client()->canHandleRequest(ResourceRequest(linkURL))) {
                appendItem(OpenLinkItem, m_contextMenu.get());
                appendItem(OpenLinkInNewWindowItem, m_contextMenu.get());
                appendItem(DownloadFileItem, m_contextMenu.get());
            }
#if ENABLE(TIZEN_WEBKIT2_COPY_MENU_FOR_SPECIFIC_SCHEME_LINK)
            if (linkURL.protocolIs("mailto") || linkURL.protocolIs("tel"))
                appendItem(CopyLinkDataItem, m_contextMenu.get());
            else if(linkURL.protocolIs("http") || linkURL.protocolIs("https") || linkURL.protocolIs("ftp") || linkURL.protocolIs("ftps"))
#endif
            {
                appendItem(CopyLinkItem, m_contextMenu.get());
                appendItem(*separatorItem(), m_contextMenu.get());
            }
        }

        if (m_hitTestResult.isSelected() && !inPasswordField && selectionContainsPossibleWord(frame)) {
#if PLATFORM(MAC)
            String selectedString = frame->displayStringModifiedByEncoding(frame->editor()->selectedText());
            ContextMenuItem LookUpInDictionaryItem(ActionType, ContextMenuItemTagLookUpInDictionary, contextMenuItemTagLookUpInDictionary(selectedString));

#if INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM
            appendItem(SearchSpotlightItem, m_contextMenu.get());
#else
            appendItem(LookUpInDictionaryItem, m_contextMenu.get());
#endif
#endif

#if !PLATFORM(GTK)
            appendItem(SearchWebItem, m_contextMenu.get());
            appendItem(*separatorItem(), m_contextMenu.get());
#endif

#if PLATFORM(MAC) && INCLUDE_SPOTLIGHT_CONTEXT_MENU_ITEM
            appendItem(LookUpInDictionaryItem, m_contextMenu.get());
            appendItem(*separatorItem(), m_contextMenu.get());
#endif
        }

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
        bool isInputPickerType = false;
#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
        HTMLInputElement* inputElement = node->toInputElement();
        if (inputElement &&
            (inputElement->isDateField() || inputElement->isDateTimeField() || inputElement->isDateTimeLocalField()
            || inputElement->isMonthField() || inputElement->isTimeField() || inputElement->isWeekField()))
            isInputPickerType = true;
#endif
        if (m_hitTestResult.isSelected() && !inPasswordField) {
            if (!isInputPickerType)
                appendItem(CutItem, m_contextMenu.get());
            appendItem(CopyItem, m_contextMenu.get());
        }
        if (!isInputPickerType) {
            appendItem(PasteItem, m_contextMenu.get());
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
            if (!inPasswordField)
                appendItem(ClipboardItem, m_contextMenu.get());
#endif
        }
#else
        appendItem(CutItem, m_contextMenu.get());
        appendItem(CopyItem, m_contextMenu.get());
        appendItem(PasteItem, m_contextMenu.get());
#endif
#if PLATFORM(GTK)
        appendItem(DeleteItem, m_contextMenu.get());
        appendItem(*separatorItem(), m_contextMenu.get());
#endif
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
        if (frame->selection() && !inPasswordField) {
            Node* baseNode = frame->selection()->base().containerNode();
            VisiblePosition currentPosition = frame->selection()->selection().visibleStart();
            VisiblePosition startPosition = startOfEditableContent(currentPosition);
            VisiblePosition endPosition = endOfEditableContent(currentPosition);
            if (comparePositions(startPosition, endPosition))
                appendItem(SelectAllItem, m_contextMenu.get());

            bool canSelectWordItemAppend = canWordSelect(frame);
            if (canSelectWordItemAppend) {
                if (m_hitTestResult.isContentEditable() && !node->isContentRichlyEditable()) {
                    if (baseNode && baseNode->textContent().isEmpty())
                        canSelectWordItemAppend = false;
                }

                if (canSelectWordItemAppend)
                    appendItem(SelectWordItem, m_contextMenu.get());
            }
        }
#else
#if PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL)
        appendItem(SelectAllItem, m_contextMenu.get());
#endif
#endif

        if (!inPasswordField) {
#if !PLATFORM(GTK)
#if !ENABLE(TIZEN_CONTEXT_MENU_TEMPORARY_FIX)
            appendItem(*separatorItem(), m_contextMenu.get());
            ContextMenuItem SpellingAndGrammarMenuItem(SubmenuType, ContextMenuItemTagSpellingMenu, 
                contextMenuItemTagSpellingMenu());
            createAndAppendSpellingAndGrammarSubMenu(SpellingAndGrammarMenuItem);
            appendItem(SpellingAndGrammarMenuItem, m_contextMenu.get());
#endif //TIZEN_CONTEXT_MENU_TEMPORARY_FIX
#endif
#if PLATFORM(MAC)
            ContextMenuItem substitutionsMenuItem(SubmenuType, ContextMenuItemTagSubstitutionsMenu, 
                contextMenuItemTagSubstitutionsMenu());
            createAndAppendSubstitutionsSubMenu(substitutionsMenuItem);
            appendItem(substitutionsMenuItem, m_contextMenu.get());
            ContextMenuItem transformationsMenuItem(SubmenuType, ContextMenuItemTagTransformationsMenu, 
                contextMenuItemTagTransformationsMenu());
            createAndAppendTransformationsSubMenu(transformationsMenuItem);
            appendItem(transformationsMenuItem, m_contextMenu.get());
#endif
#if PLATFORM(GTK)
            bool shouldShowFontMenu = frame->editor()->canEditRichly();
#else
#if ENABLE(TIZEN_CONTEXT_MENU_TEMPORARY_FIX)
            bool shouldShowFontMenu = false;
#else
            bool shouldShowFontMenu = true;
#endif //TIZEN_CONTEXT_MENU_TEMPORARY_FIX
#endif
            if (shouldShowFontMenu) {
                ContextMenuItem FontMenuItem(SubmenuType, ContextMenuItemTagFontMenu, 
                    contextMenuItemTagFontMenu());
                createAndAppendFontSubMenu(FontMenuItem);
                appendItem(FontMenuItem, m_contextMenu.get());
            }
#if PLATFORM(MAC)
            ContextMenuItem SpeechMenuItem(SubmenuType, ContextMenuItemTagSpeechMenu, contextMenuItemTagSpeechMenu());
            createAndAppendSpeechSubMenu(SpeechMenuItem);
            appendItem(SpeechMenuItem, m_contextMenu.get());
#endif
#if PLATFORM(GTK)
            EditorClient* client = frame->editor()->client();
            if (client && client->shouldShowUnicodeMenu()) {
                ContextMenuItem UnicodeMenuItem(SubmenuType, ContextMenuItemTagUnicode, contextMenuItemTagUnicode());
                createAndAppendUnicodeSubMenu(UnicodeMenuItem);
                appendItem(*separatorItem(), m_contextMenu.get());
                appendItem(UnicodeMenuItem, m_contextMenu.get());
            }
#else
            ContextMenuItem WritingDirectionMenuItem(SubmenuType, ContextMenuItemTagWritingDirectionMenu, 
                contextMenuItemTagWritingDirectionMenu());
#if !ENABLE(TIZEN_CONTEXT_MENU_TEMPORARY_FIX)
            createAndAppendWritingDirectionSubMenu(WritingDirectionMenuItem);
            appendItem(WritingDirectionMenuItem, m_contextMenu.get());
            if (Page* page = frame->page()) {
                if (Settings* settings = page->settings()) {
                    bool includeTextDirectionSubmenu = settings->textDirectionSubmenuInclusionBehavior() == TextDirectionSubmenuAlwaysIncluded
                        || (settings->textDirectionSubmenuInclusionBehavior() == TextDirectionSubmenuAutomaticallyIncluded && frame->editor()->hasBidiSelection());
                    if (includeTextDirectionSubmenu) {
                        ContextMenuItem TextDirectionMenuItem(SubmenuType, ContextMenuItemTagTextDirectionMenu, 
                            contextMenuItemTagTextDirectionMenu());
                        createAndAppendTextDirectionSubMenu(TextDirectionMenuItem);
                        appendItem(TextDirectionMenuItem, m_contextMenu.get());
                    }
                }
            }
#endif //TIZEN_CONTEXT_MENU_TEMPORARY_FIX
#endif
        }
    }
}

#if ENABLE(INSPECTOR)
void ContextMenuController::addInspectElementItem()
{
    Node* node = m_hitTestResult.innerNonSharedNode();
    if (!node)
        return;

    Frame* frame = node->document()->frame();
    if (!frame)
        return;

    Page* page = frame->page();
    if (!page)
        return;

    if (!page->inspectorController())
        return;

    ContextMenuItem InspectElementItem(ActionType, ContextMenuItemTagInspectElement, contextMenuItemTagInspectElement());
#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
    if (!m_contextMenu->items().isEmpty())
#else
    if (m_contextMenu->itemCount())
#endif
        appendItem(*separatorItem(), m_contextMenu.get());
    appendItem(InspectElementItem, m_contextMenu.get());
}
#endif // ENABLE(INSPECTOR)

void ContextMenuController::checkOrEnableIfNeeded(ContextMenuItem& item) const
{
    if (item.type() == SeparatorType)
        return;
    
    Frame* frame = m_hitTestResult.innerNonSharedNode()->document()->frame();
    if (!frame)
        return;

    // Custom items already have proper checked and enabled values.
    if (ContextMenuItemBaseCustomTag <= item.action() && item.action() <= ContextMenuItemLastCustomTag)
        return;

    bool shouldEnable = true;
    bool shouldCheck = false; 

    switch (item.action()) {
        case ContextMenuItemTagCheckSpelling:
            shouldEnable = frame->editor()->canEdit();
            break;
        case ContextMenuItemTagDefaultDirection:
            shouldCheck = false;
            shouldEnable = false;
            break;
        case ContextMenuItemTagLeftToRight:
        case ContextMenuItemTagRightToLeft: {
            String direction = item.action() == ContextMenuItemTagLeftToRight ? "ltr" : "rtl";
            shouldCheck = frame->editor()->selectionHasStyle(CSSPropertyDirection, direction) != FalseTriState;
            shouldEnable = true;
            break;
        }
        case ContextMenuItemTagTextDirectionDefault: {
            Editor::Command command = frame->editor()->command("MakeTextWritingDirectionNatural");
            shouldCheck = command.state() == TrueTriState;
            shouldEnable = command.isEnabled();
            break;
        }
        case ContextMenuItemTagTextDirectionLeftToRight: {
            Editor::Command command = frame->editor()->command("MakeTextWritingDirectionLeftToRight");
            shouldCheck = command.state() == TrueTriState;
            shouldEnable = command.isEnabled();
            break;
        }
        case ContextMenuItemTagTextDirectionRightToLeft: {
            Editor::Command command = frame->editor()->command("MakeTextWritingDirectionRightToLeft");
            shouldCheck = command.state() == TrueTriState;
            shouldEnable = command.isEnabled();
            break;
        }
        case ContextMenuItemTagCopy:
            shouldEnable = frame->editor()->canDHTMLCopy() || frame->editor()->canCopy();
            break;
        case ContextMenuItemTagCut:
            shouldEnable = frame->editor()->canDHTMLCut() || frame->editor()->canCut();
            break;
        case ContextMenuItemTagIgnoreSpelling:
        case ContextMenuItemTagLearnSpelling:
            shouldEnable = frame->selection()->isRange();
            break;
        case ContextMenuItemTagPaste:
            shouldEnable = frame->editor()->canDHTMLPaste() || frame->editor()->canPaste();
            break;
#if ENABLE(TIZEN_CONTEXT_MENU_SELECT)
        case ContextMenuItemTagSelectWord:
            shouldEnable = frame->editor()->canSelectRange();
            break;
#endif
#if PLATFORM(GTK)
        case ContextMenuItemTagDelete:
            shouldEnable = frame->editor()->canDelete();
            break;
        case ContextMenuItemTagInputMethods:
        case ContextMenuItemTagUnicode:
        case ContextMenuItemTagUnicodeInsertLRMMark:
        case ContextMenuItemTagUnicodeInsertRLMMark:
        case ContextMenuItemTagUnicodeInsertLREMark:
        case ContextMenuItemTagUnicodeInsertRLEMark:
        case ContextMenuItemTagUnicodeInsertLROMark:
        case ContextMenuItemTagUnicodeInsertRLOMark:
        case ContextMenuItemTagUnicodeInsertPDFMark:
        case ContextMenuItemTagUnicodeInsertZWSMark:
        case ContextMenuItemTagUnicodeInsertZWJMark:
        case ContextMenuItemTagUnicodeInsertZWNJMark:
            shouldEnable = true;
            break;
#endif
#if PLATFORM(GTK) || PLATFORM(EFL)
        case ContextMenuItemTagSelectAll:
            shouldEnable = true;
            break;
#endif
        case ContextMenuItemTagUnderline: {
            shouldCheck = frame->editor()->selectionHasStyle(CSSPropertyWebkitTextDecorationsInEffect, "underline") != FalseTriState;
            shouldEnable = frame->editor()->canEditRichly();
            break;
        }
        case ContextMenuItemTagLookUpInDictionary:
            shouldEnable = frame->selection()->isRange();
            break;
        case ContextMenuItemTagCheckGrammarWithSpelling:
            if (frame->editor()->isGrammarCheckingEnabled())
                shouldCheck = true;
            shouldEnable = true;
            break;
        case ContextMenuItemTagItalic: {
            shouldCheck = frame->editor()->selectionHasStyle(CSSPropertyFontStyle, "italic") != FalseTriState;
            shouldEnable = frame->editor()->canEditRichly();
            break;
        }
        case ContextMenuItemTagBold: {
            shouldCheck = frame->editor()->selectionHasStyle(CSSPropertyFontWeight, "bold") != FalseTriState;
            shouldEnable = frame->editor()->canEditRichly();
            break;
        }
        case ContextMenuItemTagOutline:
            shouldEnable = false;
            break;
        case ContextMenuItemTagShowSpellingPanel:
            if (frame->editor()->spellingPanelIsShowing())
                item.setTitle(contextMenuItemTagShowSpellingPanel(false));
            else
                item.setTitle(contextMenuItemTagShowSpellingPanel(true));
            shouldEnable = frame->editor()->canEdit();
            break;
        case ContextMenuItemTagNoGuessesFound:
            shouldEnable = false;
            break;
        case ContextMenuItemTagCheckSpellingWhileTyping:
            shouldCheck = frame->editor()->isContinuousSpellCheckingEnabled();
            break;
#if PLATFORM(MAC)
        case ContextMenuItemTagSubstitutionsMenu:
        case ContextMenuItemTagTransformationsMenu:
            break;
        case ContextMenuItemTagShowSubstitutions:
            if (frame->editor()->substitutionsPanelIsShowing())
                item.setTitle(contextMenuItemTagShowSubstitutions(false));
            else
                item.setTitle(contextMenuItemTagShowSubstitutions(true));
            shouldEnable = frame->editor()->canEdit();
            break;
        case ContextMenuItemTagMakeUpperCase:
        case ContextMenuItemTagMakeLowerCase:
        case ContextMenuItemTagCapitalize:
        case ContextMenuItemTagChangeBack:
            shouldEnable = frame->editor()->canEdit();
            break;
        case ContextMenuItemTagCorrectSpellingAutomatically:
            shouldCheck = frame->editor()->isAutomaticSpellingCorrectionEnabled();
            break;
        case ContextMenuItemTagSmartCopyPaste:
            shouldCheck = frame->editor()->smartInsertDeleteEnabled();
            break;
        case ContextMenuItemTagSmartQuotes:
            shouldCheck = frame->editor()->isAutomaticQuoteSubstitutionEnabled();
            break;
        case ContextMenuItemTagSmartDashes:
            shouldCheck = frame->editor()->isAutomaticDashSubstitutionEnabled();
            break;
        case ContextMenuItemTagSmartLinks:
            shouldCheck = frame->editor()->isAutomaticLinkDetectionEnabled();
            break;
        case ContextMenuItemTagTextReplacement:
            shouldCheck = frame->editor()->isAutomaticTextReplacementEnabled();
            break;
        case ContextMenuItemTagStopSpeaking:
            shouldEnable = client() && client()->isSpeaking();
            break;
#else // PLATFORM(MAC) ends here
        case ContextMenuItemTagStopSpeaking:
            break;
#endif
#if PLATFORM(GTK)
        case ContextMenuItemTagGoBack:
            shouldEnable = frame->page() && frame->page()->backForward()->canGoBackOrForward(-1);
            break;
        case ContextMenuItemTagGoForward:
            shouldEnable = frame->page() && frame->page()->backForward()->canGoBackOrForward(1);
            break;
        case ContextMenuItemTagStop:
            shouldEnable = frame->loader()->documentLoader()->isLoadingInAPISense();
            break;
        case ContextMenuItemTagReload:
            shouldEnable = !frame->loader()->documentLoader()->isLoadingInAPISense();
            break;
        case ContextMenuItemTagFontMenu:
            shouldEnable = frame->editor()->canEditRichly();
            break;
#else
        case ContextMenuItemTagGoBack:
        case ContextMenuItemTagGoForward:
        case ContextMenuItemTagStop:
        case ContextMenuItemTagReload:
        case ContextMenuItemTagFontMenu:
#endif
        case ContextMenuItemTagNoAction:
        case ContextMenuItemTagOpenLinkInNewWindow:
        case ContextMenuItemTagDownloadLinkToDisk:
        case ContextMenuItemTagCopyLinkToClipboard:
        case ContextMenuItemTagOpenImageInNewWindow:
#if ENABLE(TIZEN_OPEN_IMAGE_IN_CURRENT_TAB)
        case ContextMenuItemTagOpenImageInCurrentWindow:
#endif
        case ContextMenuItemTagDownloadImageToDisk:
        case ContextMenuItemTagCopyImageToClipboard:
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
        case ContextMenuItemTagCopyImageUrlToClipboard:
#endif
            break;
        case ContextMenuItemTagOpenMediaInNewWindow:
            if (m_hitTestResult.mediaIsVideo())
                item.setTitle(contextMenuItemTagOpenVideoInNewWindow());
            else
                item.setTitle(contextMenuItemTagOpenAudioInNewWindow());
            break;
        case ContextMenuItemTagCopyMediaLinkToClipboard:
            if (m_hitTestResult.mediaIsVideo())
                item.setTitle(contextMenuItemTagCopyVideoLinkToClipboard());
            else
                item.setTitle(contextMenuItemTagCopyAudioLinkToClipboard());
            break;
        case ContextMenuItemTagToggleMediaControls:
            shouldCheck = m_hitTestResult.mediaControlsEnabled();
            break;
        case ContextMenuItemTagToggleMediaLoop:
            shouldCheck = m_hitTestResult.mediaLoopEnabled();
            break;
        case ContextMenuItemTagEnterVideoFullscreen:
            shouldEnable = m_hitTestResult.mediaSupportsFullscreen();
            break;
        case ContextMenuItemTagOpenFrameInNewWindow:
        case ContextMenuItemTagSpellingGuess:
        case ContextMenuItemTagOther:
        case ContextMenuItemTagSearchInSpotlight:
        case ContextMenuItemTagSearchWeb:
        case ContextMenuItemTagOpenWithDefaultApplication:
        case ContextMenuItemPDFActualSize:
        case ContextMenuItemPDFZoomIn:
        case ContextMenuItemPDFZoomOut:
        case ContextMenuItemPDFAutoSize:
        case ContextMenuItemPDFSinglePage:
        case ContextMenuItemPDFFacingPages:
        case ContextMenuItemPDFContinuous:
        case ContextMenuItemPDFNextPage:
        case ContextMenuItemPDFPreviousPage:
        case ContextMenuItemTagOpenLink:
        case ContextMenuItemTagIgnoreGrammar:
        case ContextMenuItemTagSpellingMenu:
        case ContextMenuItemTagShowFonts:
        case ContextMenuItemTagStyles:
        case ContextMenuItemTagShowColors:
        case ContextMenuItemTagSpeechMenu:
        case ContextMenuItemTagStartSpeaking:
        case ContextMenuItemTagWritingDirectionMenu:
        case ContextMenuItemTagTextDirectionMenu:
        case ContextMenuItemTagPDFSinglePageScrolling:
        case ContextMenuItemTagPDFFacingPagesScrolling:
#if ENABLE(INSPECTOR)
        case ContextMenuItemTagInspectElement:
#endif
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
        case ContextMenuItemTagClipboard:
#endif
#if ENABLE(TIZEN_DRAG_SUPPORT)
        case ContextMenuItemTagDrag:
#endif
#if ENABLE(TIZEN_WEBKIT2_COPY_MENU_FOR_SPECIFIC_SCHEME_LINK)
        case ContextMenuItemTagCopyLinkData:
#endif
        case ContextMenuItemBaseCustomTag:
        case ContextMenuItemCustomTagNoAction:
        case ContextMenuItemLastCustomTag:
        case ContextMenuItemBaseApplicationTag:
        case ContextMenuItemTagDictationAlternative:
            break;
        case ContextMenuItemTagMediaPlayPause:
            if (m_hitTestResult.mediaPlaying())
                item.setTitle(contextMenuItemTagMediaPause());
            else
                item.setTitle(contextMenuItemTagMediaPlay());
            break;
        case ContextMenuItemTagMediaMute:
            shouldEnable = m_hitTestResult.mediaHasAudio();
            shouldCheck = shouldEnable &&  m_hitTestResult.mediaMuted();
            break;
    }

    item.setChecked(shouldCheck);
    item.setEnabled(shouldEnable);
}

#if USE(ACCESSIBILITY_CONTEXT_MENUS)
void ContextMenuController::showContextMenuAt(Frame* frame, const IntPoint& clickPoint)
{
    // Simulate a click in the middle of the accessibility object.
    PlatformMouseEvent mouseEvent(clickPoint, clickPoint, RightButton, PlatformEvent::MousePressed, 1, false, false, false, false, currentTime());
    bool handled = frame->eventHandler()->sendContextMenuEvent(mouseEvent);
    if (handled && client())
        client()->showContextMenu();
}
#endif

} // namespace WebCore

#endif // ENABLE(CONTEXT_MENUS)
