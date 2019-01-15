/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 Christian Dywan <christian@imendio.com>
 * Copyright (C) 2008 Nuanti Ltd.
 * Copyright (C) 2008 INdT Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2012 Samsung Electronics
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
#include "LocalizedStrings.h"

#include "NotImplemented.h"
#include "PlatformString.h"
#include <stdio.h>
#include <wtf/text/CString.h>

#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
#include <libintl.h>
#endif

namespace WebCore {

String submitButtonDefaultLabel()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_SUBMIT"));
}

String inputElementAltText()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_SUBMIT"));
}

String resetButtonDefaultLabel()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_RESET"));
}

String defaultDetailsSummaryText()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_DETAILS"));
}

String searchableIndexIntroduction()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_YOU_CAN_SEARCH_THIS_INDEX_ENTER_KEYWORDS_C"));
}

String fileButtonChooseFileLabel()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_UPLOAD_FILE"));
}

String fileButtonChooseMultipleFilesLabel()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_UPLOAD_MULTIPLE_FILES"));
}

String fileButtonNoFileSelectedLabel()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_NO_FILES_HAVE_BEEN_SELECTED"));
}

#if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)
String contextMenuItemTagOpenLinkInNewWindow()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_OPEN_LINK_IN_NEW_TAB_ABB"));
}

String contextMenuItemTagDownloadLinkToDisk()
{
    return String::fromUTF8(dgettext("WebKit","IDS_BR_BODY_SAVE_LINK"));
}

String contextMenuItemTagCopyLinkToClipboard()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY_LINK_URL_ABB"));
}

String contextMenuItemTagOpenImageInNewWindow()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_OPEN_IMAGE_IN_NEW_TAB_ABB"));
}

#if ENABLE(TIZEN_OPEN_IMAGE_IN_CURRENT_TAB)
String contextMenuItemTagOpenImageInCurrentWindow()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_OPEN_IMAGE_IN_CURRENT_TAB_ABB"));
}
#endif

String contextMenuItemTagDownloadImageToDisk()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SAVE_IMAGE_ABB"));
}

String contextMenuItemTagCopyImageToClipboard()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY_TO_CLIPBOARD"));
}

String contextMenuItemTagCopyImageUrlToClipboard()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY_IMAGE"));
}

String contextMenuItemTagOpenFrameInNewWindow()
{
    return String::fromUTF8("Open frame in new window");
}

String contextMenuItemTagCopy()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY"));
}

String contextMenuItemTagGoBack()
{
    return String::fromUTF8("Go Back");
}

String contextMenuItemTagGoForward()
{
    return String::fromUTF8("Go Forward");
}

String contextMenuItemTagStop()
{
    return String::fromUTF8("Stop");
}

String contextMenuItemTagReload()
{
    return String::fromUTF8("Reload");
}

String contextMenuItemTagCut()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_CUT_ABB"));
}

String contextMenuItemTagPaste()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_PASTE"));
}

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
String contextMenuItemTagClipboard()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_CLIPBOARD"));
}
#endif

String contextMenuItemTagSelectAll()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SELECT_ALL_ABB"));
}

String contextMenuItemTagSelectWord()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SELECT_ABB"));
}

String contextMenuItemTagNoGuessesFound()
{
    return String::fromUTF8("No guesses found");
}

String contextMenuItemTagIgnoreSpelling()
{
    return String::fromUTF8("Ignore spelling");
}

String contextMenuItemTagLearnSpelling()
{
    return String::fromUTF8("Learn spelling");
}

String contextMenuItemTagSearchWeb()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_WEB_SEARCH"));
}

String contextMenuItemTagLookUpInDictionary()
{
    return String::fromUTF8("Look up in dictionary");
}

String contextMenuItemTagOpenLink()
{
    return String::fromUTF8("Open link");
}

String contextMenuItemTagIgnoreGrammar()
{
    return String::fromUTF8("Ignore grammar");
}

String contextMenuItemTagSpellingMenu()
{
    return String::fromUTF8("Spelling and grammar");
}

String contextMenuItemTagShowSpellingPanel(bool show)
{
    return String::fromUTF8(show ? "Show spelling and grammar" : "Hide spelling and grammar");
}

String contextMenuItemTagCheckSpelling()
{
    return String::fromUTF8("Check document now");
}

String contextMenuItemTagCheckSpellingWhileTyping()
{
    return String::fromUTF8("Check spelling while _Typing");
}

String contextMenuItemTagCheckGrammarWithSpelling()
{
    return String::fromUTF8("Check grammar with spelling");
}

String contextMenuItemTagFontMenu()
{
    return String::fromUTF8("Font");
}

String contextMenuItemTagBold()
{
    return String::fromUTF8("Bold");
}

String contextMenuItemTagItalic()
{
    return String::fromUTF8("Italic");
}

String contextMenuItemTagUnderline()
{
    return String::fromUTF8("Underline");
}

String contextMenuItemTagOutline()
{
    return String::fromUTF8("Outline");
}

String contextMenuItemTagWritingDirectionMenu()
{
    return String();
}

String contextMenuItemTagTextDirectionMenu()
{
    return String();
}

String contextMenuItemTagDefaultDirection()
{
    return String();
}

String contextMenuItemTagLeftToRight()
{
    return String();
}

String contextMenuItemTagRightToLeft()
{
    return String();
}

String contextMenuItemTagOpenVideoInNewWindow()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_OPEN_VIDEO_IN_NEW_WINDOW_ABB"));
}

String contextMenuItemTagOpenAudioInNewWindow()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_PLAY_AUDIO_IN_NEW_TAB_ABB"));
}

String contextMenuItemTagCopyVideoLinkToClipboard()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY_VIDEO_URL_ABB"));
}

String contextMenuItemTagCopyAudioLinkToClipboard()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_COPY_AUDIO_URL_ABB"));
}

String contextMenuItemTagToggleMediaControls()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SHOW_HIDE_MEDIA_CONTROLS_ABB"));
}

String contextMenuItemTagToggleMediaLoop()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_ENABLE_DISABLE_REPEAT_MEDIA_ABB"));
}

String contextMenuItemTagEnterVideoFullscreen()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_FULL_VIEW_ABB"));
}

String contextMenuItemTagMediaPlay()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_PLAY"));
}

String contextMenuItemTagMediaPause()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_PAUSE"));
}

String contextMenuItemTagMediaMute()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_MUTE"));
}

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
String contextMenuItemTagMediaUnMute()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_UNMUTE"));
}
#endif

String contextMenuItemTagInspectElement()
{
    return String::fromUTF8("Inspect element");
}

String javaScriptPopupTitle()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_HEADER_MESSAGE_FROM_PS_M_WEBSITE"));
}

String fileButtonNoFilesSelectedLabel()
{
    return String();
}

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
String contextMenuItemTagTextSelectionMode()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_SELECT_TEXT_ABB"));
}
#endif
#if ENABLE(TIZEN_DRAG_SUPPORT)
String contextMenuItemTagDrag()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_OPT_DRAG_AND_DROP_ABB"));
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
String passwordSavePopupItemLater()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_LATER_ABB"));
}

String passwordSavePopupMessage()
{
    return String::fromUTF8(dgettext("WebKit","IDS_BR_POP_SAVE_YOUR_USERNAMES_AND_PASSWORDS_FOR_WEBSITES_Q"));
}
#endif

#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
String beforeUnloadConfirmPopupMessage()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_POP_LEAVE_THIS_PAGE_Q"));
}
#endif
#if ENABLE(TIZEN_SCREEN_READER)
String screenReaderLink()
{
    return String::fromUTF8(dgettext("WebKit","IDS_BR_OPT_LINK"));
}

String screenReaderButton()
{
    return String::fromUTF8(dgettext("WebKit","IDS_COM_BODY_BUTTON_T_TTS"));
}

String screenReaderTextField()
{
    return String::fromUTF8(dgettext("WebKit","IDS_COM_BODY_TEXT_FIELD_T_TTS"));
}

String screenReaderImage()
{
    return String::fromUTF8(dgettext("WebKit","IDS_COM_OPT_IMAGE"));
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
String screenReaderVideo()
{
    return String::fromUTF8("Video");
}

String screenReaderAudio()
{
    return String::fromUTF8("Audio");
}
#endif

String screenReaderDoubleTabToEdit()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_DOUBLE_TAB_TO_EDIT_TTS"));
}

String screenReaderEditing()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_EDITING_TTS"));
}

String screenReaderEditField()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_EDIT_FIELD__M_NOUN_TTS"));
}

String screenReaderNotSelected()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_NOT_SELECTED_TTS"));
}

String screenReaderSelected()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_SELECTED_TTS"));
}

String screenReaderPDCharacters()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_PD_CHARACTERS_TTS"));
}

String screenReaderPDItems()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_SHOWING_PD_ITEMS_TTS"));
}
#endif
#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
String popupPickerDone()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BUTTON_DONE"));
}
#endif
#endif // #if ENABLE(TIZEN_WEBKIT2_TEXT_TRANSLATION)

String searchMenuNoRecentSearchesText()
{
    return String::fromUTF8("No recent searches");
}

String searchMenuRecentSearchesText()
{
    return String::fromUTF8("Recent searches");
}

String searchMenuClearRecentSearchesText()
{
    return String::fromUTF8("Clear recent searches");
}

String AXDefinitionListTermText()
{
    return String::fromUTF8("term");
}

String AXDefinitionListDefinitionText()
{
    return String::fromUTF8("definition");
}

String AXFooterRoleDescriptionText()
{
    return String::fromUTF8("footer");
}

String AXButtonActionVerb()
{
    return String::fromUTF8("press");
}

String AXRadioButtonActionVerb()
{
    return String::fromUTF8("select");
}

String AXTextFieldActionVerb()
{
    return String::fromUTF8("activate");
}

String AXCheckedCheckBoxActionVerb()
{
    return String::fromUTF8("uncheck");
}

String AXUncheckedCheckBoxActionVerb()
{
    return String::fromUTF8("check");
}

String AXLinkActionVerb()
{
    return String::fromUTF8("jump");
}

String unknownFileSizeText()
{
    return String::fromUTF8("Unknown");
}

String imageTitle(const String& filename, const IntSize& size)
{
    notImplemented();
    return String();
}

#if ENABLE(VIDEO)
String localizedMediaControlElementString(const String& name)
{
    notImplemented();
    return String();
}

String localizedMediaControlElementHelpText(const String& name)
{
    notImplemented();
    return String();
}

String localizedMediaTimeDescription(float time)
{
    notImplemented();
    return String();
}
#endif

String mediaElementLoadingStateText()
{
    return String::fromUTF8("Loading...");
}

String mediaElementLiveBroadcastStateText()
{
    return String::fromUTF8("Live Broadcast");
}

#if ENABLE(TIZEN_APPLICATION_CACHE)
String applicationCachePermissionText(const String& value)
{
    String message = String::fromUTF8("Web Page (%s) is attempting to store a large amount of data on your device for offline use.");
    message = message.replace("%s", value.utf8().data());
    return message;
}
#endif

#if ENABLE(TIZEN_SQL_DATABASE)
String exceededDatabaseQuotaPermissionText(const String& value)
{
    String message = String::fromUTF8("Web Page (%s) is requesting permission to store data on your device for offline use.");
    message = message.replace("%s", value.utf8().data());
    return message;
}
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
String exceededIndexedDatabaseQuotaPermissionText(const String& value)
{
    String message = String::fromUTF8("Web Page (%s) is requesting permission to store data on your device for offline use.");
    message = message.replace("%s", value.utf8().data());
    return message;
}
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
String exceededLocalFileSystemQuotaPermissionText(const String& value)
{
    String message = String::fromUTF8("Web Page (%s) is requesting permission to store data on your device for offline use.");
    message = message.replace("%s", value.utf8().data());
    return message;
}
#endif

#if ENABLE(TIZEN_GEOLOCATION)
String geolocationPermissionText(const String& value)
{
    String message = String::fromUTF8("Web Page (%s) is requesting permission to access your location.");
    message = message.replace("%s", value.utf8().data());
    return message;
}
#endif

#if ENABLE(TIZEN_MEDIA_STREAM)
String userMediaPermissionText(const String& value)
{
    String message = String::fromUTF8("Web Page (%s) is requesting permission to use your camera.");
    message = message.replace("%s", value.utf8().data());
    return message;
}
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
String mediaElementVideoErrorText()
{
    return String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_POP_A_TEMPORARY_ERROR_HAS_OCCURRED"));
}

String mediaElementCallSessionErrorText()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_TPOP_UNABLE_TO_PLAY_VIDEOS_DURING_CALLS"));
}

String mediaElementAudioRecordingSessionErrorText()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_UNABLE_TO_PLAY_MEDIA_WHILE_RECORDING_AUDIO"));
}
#endif

#if ENABLE(TIZEN_NOTIFICATIONS)
String notificationPermissionText(const String& value)
{
    String message = String::fromUTF8("Web Page (%s) is requesting permission to show notifications.");
    message = message.replace("%s", value.utf8().data());
    return message;
}
#endif

String permissionPopupTitle()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_HEADER_MESSAGE_FROM_PS_M_WEBSITE"));
}

String validationMessagePatternMismatchText()
{
    return String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_INVALID_FORMAT_ENTERED"));
}

String validationMessageRangeOverflowText(const String& value)
{
    String message = String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_VALUE_MUST_BE_NO_HIGHER_THAN_PD"));
    message = message.replace("%d", value.latin1().data());
    return message;
}

String validationMessageRangeUnderflowText(const String& value)
{
    String message = String::fromUTF8(dgettext("WebKit", "IDS_WEBVIEW_BODY_VALUE_MUST_BE_AT_LEAST_PD"));
    message = message.replace("%d", value.latin1().data());
    return message;
}

String validationMessageStepMismatchText(const String&, const String&)
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_INVALID_VALUE_ENTERED"));
}

String validationMessageTooLongText(int, int)
{
    return String::fromUTF8("too long");
}

String validationMessageTypeMismatchText()
{
    return String::fromUTF8("type mismatch");
}

String validationMessageTypeMismatchForEmailText()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_ENTER_AN_EMAIL_ADDRESS"));
}

String validationMessageTypeMismatchForMultipleEmailText()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_ENTER_A_LIST_OF_EMAIL_ADDRESSES_SEPARATED_BY_COMMAS"));
}

String validationMessageTypeMismatchForURLText()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_ENTER_A_URL"));
}

String validationMessageValueMissingText()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_FIELD_CANNOT_BE_BLANK"));
}

String validationMessageValueMissingForCheckboxText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForMultipleFileText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForRadioText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String validationMessageValueMissingForSelectText()
{
    notImplemented();
    return validationMessageValueMissingText();
}

String missingPluginText()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_PLUG_IN_MISSING"));
}

String AXMenuListPopupActionVerb()
{
    return String();
}

String AXMenuListActionVerb()
{
    return String();
}

String multipleFileUploadText(unsigned numberOfFiles)
{
    return String::number(numberOfFiles) + String::fromUTF8(" files");
}

String crashedPluginText()
{
    return String::fromUTF8("plugin crashed");
}

String blockedPluginByContentSecurityPolicyText()
{
    notImplemented();
    return String();
}

String insecurePluginVersionText()
{
    notImplemented();
    return String();
}

String localizedString(const char* key)
{
    return String::fromUTF8(key, strlen(key));
}

String errorPageTitle()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_HEADER_THIS_WEBPAGE_IS_NOT_AVAILABLE"));
}

String errorMessageTitle()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_HEADER_THIS_WEBPAGE_IS_NOT_AVAILABLE"));
}

String errorMessageDNSError()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_THE_SERVER_AT_PS_CANT_BE_FOUND_BECAUSE_THE_DNS_LOOK_UP_FAILED_MSG"));
}

String errorMessageServerNotResponding()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_UNABLE_TO_LOAD_THE_PAGE_PS_TOOK_TOO_LONG_TO_RESPOND_THE_WEBSITE_MAY_BE_DOWN_OR_THERE_MAY_HAVE_BEEN_A_NETWORK_CONNECTION_ERROR"));
}

String errorMessageSSLFailedError()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_UNABLE_TO_LOAD_THE_PAGE_A_SECURE_CONNECTION_COULD_NOT_BE_MADE_TO_PS_THE_MOST_LIKELY_CAUSE_IS_THE_DEVICES_CLOCK_CHECK_THAT_THE_TIME_ON_YOUR_DEVICE_IS_CORRECT_AND_REFRESH_THE_PAGE"));
}

String errorMessageDefaultError()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_BODY_UNABLE_TO_LOAD_THE_PAGE_PS_MAY_BE_TEMPORARILY_DOWN_OR_HAVE_MOVED_TO_A_NEW_URL"));
}

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
String popupTitleSetDateAndTime()
{
    return String::fromUTF8(dgettext("WebKit","IDS_WEBVIEW_HEADER_SET_DATE_AND_TIME"));
}
#endif

#if ENABLE(TIZEN_OPEN_PANEL)
String openPanelErrorText()
{
    // FIXME
    return String::fromUTF8("No apps perform this action");
}
#endif
}
