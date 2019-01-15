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

#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
#include "ClipboardHelper.h"

#include "PageClientImpl.h"
#include <Evas.h>
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <Elementary.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

namespace WebKit {

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
enum ClipType
{
    CLIP_TYPE_PRIMARY,
    CLIP_TYPE_SECONDARY,
    CLIP_TYPE_CLIPBOARD,
    CLIP_TYPE_XDND,
    CLIP_TYPE_MAX,
};

struct ClipData
{
    Ecore_X_Selection selection;
    void (* request)(Ecore_X_Window window, const char* pTarget);

    Elm_Sel_Format format;

    Elm_Sel_Format requestedFormat;
    int bufferLength;

    ClipboardHelper* clipboardHelper;
};

typedef int (*ClipNotifyHandler)(ClipData* clipData, Ecore_X_Event_Selection_Notify* notifyData);

enum
{
    ATOM_TARGETS = 0,
    ATOM_ATOM,
    ATOM_LISTING_ATOMS = ATOM_ATOM,
    ATOM_TEXT_URI,
    ATOM_TEXT_URILIST,
    ATOM_TEXT_X_VCARD,
    ATOM_IMAGE_PNG,
    ATOM_IMAGE_JPEG,
    ATOM_IMAGE_BMP,
    ATOM_IMAGE_GIF,
    ATOM_IMAGE_TIFF,
    ATOM_IMAGE_SVG,
    ATOM_IMAGE_XPM,
    ATOM_IMAGE_TGA,
    ATOM_IMAGE_PPM,
    ATOM_XELM,
    ATOM_TEXT_HTML_UTF8,
    ATOM_TEXT_HTML,
    ATOM_STRING_UTF8,
    ATOM_STRING,
    ATOM_TEXT,
    ATOM_TEXT_PLAIN_UTF8,
    ATOM_TEXT_PLAIN,
    ATOM_MAX,
};
struct AtomData
{
    const char* pName;
    Elm_Sel_Format formats;
    ClipNotifyHandler notify;
    Ecore_X_Atom atom;
};

AtomData atomList[ATOM_MAX];
ClipData clipList[CLIP_TYPE_MAX];

static Eina_Bool cbhmPropertyChangeCallback(void* data, int type, void* event);
static Eina_Bool clientMessageCallback(void* data, int type, void* event);
#endif

ClipboardHelper::ClipboardHelper(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    , m_selectionClearHandler(0)
    , m_selectionNotifyHandler(0)
    , m_clientMessageHandler(0)
    , m_cbhmPropertyChangeHandler(0)
    , m_clipboardWindowOpened(false)
#endif
    , m_allowNextCbAction(false)
{
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    m_clientMessageHandler = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, clientMessageCallback, this);
#endif
}

ClipboardHelper::~ClipboardHelper()
{
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    ecore_event_handler_del(m_clientMessageHandler);
    m_clientMessageHandler = 0;
    clearClipboardSelectionHandler();
#endif
}

// This function references from elementary's cbhm_helper.c
int ClipboardHelper::numberOfItems()
{
    // 1. Get CBHM Ecore_X_Window.
    Ecore_X_Atom xAtom;
    xAtom = ecore_x_atom_get("CBHM_XWIN");
    if (!xAtom)
        return 0;

    unsigned char* data = 0;
    int numberOfWindow = 0;
    int result = ecore_x_window_prop_property_get(0, xAtom, XA_WINDOW, 0, &data, &numberOfWindow);

    Ecore_X_Window xWindow = 0;
    if (result && numberOfWindow)
        memcpy(&xWindow, data, sizeof(Ecore_X_Window));

    if (data)
        free(data);

    if (!xWindow)
        return 0;

    ecore_x_sync();

    // 2. Get number of clipboard items.
    xAtom = ecore_x_atom_get("CBHM_cCOUNT");
    if (!xAtom)
        return 0;

    Display* display = static_cast<Display*>(ecore_x_display_get());

    Ecore_X_Atom type;
    int format;
    long unsigned numberOfItems = 0;
    long unsigned bytes = 0;
    unsigned char* dataInFormat = 0;
    result = XGetWindowProperty(display, xWindow, xAtom, 0, LONG_MAX, False, ecore_x_window_prop_any_type(),
                                reinterpret_cast<Atom*>(&type), &format, &numberOfItems, &bytes, &dataInFormat);
    if (result != Success)
        return 0;

    if (!numberOfItems) {
        XFree(dataInFormat);
        return 0;
    }

    if (!(data = static_cast<unsigned char*>(malloc(numberOfItems * format / 8)))) {
        XFree(dataInFormat);
        return 0;
    }

    switch (format) {
    case 8:
        for (long unsigned i = 0; i < numberOfItems; i++)
            (data)[i] = dataInFormat[i];
        break;
    case 16:
        for (long unsigned i = 0; i < numberOfItems; i++)
            (reinterpret_cast<unsigned short*>(data))[i] = (reinterpret_cast<unsigned short*>(dataInFormat))[i];
        break;
    case 32:
        for (long unsigned i = 0; i < numberOfItems; i++)
            (reinterpret_cast<unsigned int*>(data))[i] = (reinterpret_cast<unsigned long*>(dataInFormat))[i];
        break;
    }

    XFree(dataInFormat);

    if (data) {
        char count = atoi(reinterpret_cast<const char*>(data));
        free(data);
        return count;
    }

    return 0;
}

void ClipboardHelper::setData(const String& data, const String& type)
{
    Ecore_X_Atom dataType = 0;

    if (type == "PlainText" || type == "URIList" || type == "URL")
        dataType = ecore_x_atom_get("UTF8_STRING");
    else if (type == "Markup")
        dataType = ecore_x_atom_get("text/html;charset=utf-8");
    else if (type == "Image")
        dataType = ecore_x_atom_get("text/uri-list");

    setClipboardItem(dataType, data);
}

void ClipboardHelper::clear()
{
    elm_object_cnp_selection_clear(elm_object_parent_widget_get(m_viewImpl->view()), ELM_SEL_TYPE_CLIPBOARD);
}

void ClipboardHelper::processResult(const String& data, const String& type)
{
    MutexLocker locker(m_callbackQueueLock);
    while(!m_callbackQueue.isEmpty()) {
        RefPtr<ClipboardCallback> callback = m_callbackQueue.takeFirst();
        callback->performCallback(data, type);
    }
}

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
Eina_Bool cbhmPropertyChangeCallback(void* data, int type, void* event)
{
    ClipboardHelper *clipboardHelper = static_cast<ClipboardHelper*>(data);
    Ecore_X_Event_Window_Property *ev = (Ecore_X_Event_Window_Property*) event;

    if (ev->atom != ECORE_X_ATOM_E_ILLUME_CLIPBOARD_STATE)
        return ECORE_CALLBACK_PASS_ON;

    clipboardHelper->updateClipboardWindowState(ev);

    return ECORE_CALLBACK_PASS_ON;
}

Eina_Bool clientMessageCallback(void* data, int type, void* event)
{
    //This callback function is support for open CBHM window from IME clipboard button
    ClipboardHelper *clipboardHelper = static_cast<ClipboardHelper*>(data);
    Ecore_X_Event_Client_Message *ev = (Ecore_X_Event_Client_Message*) event;

    if (ev->message_type != ecore_x_atom_get("CBHM_MSG"))
        return ECORE_CALLBACK_PASS_ON;

    if (!strcmp("SET_OWNER", ev->data.b)) {
        clipboardHelper->openClipboardWindow(clipboardHelper->page()->editorState().isContentRichlyEditable);
    }

    return ECORE_CALLBACK_PASS_ON;
}

static void pasteSelectedClipboardItem(const String& data, const String& type, ClipboardHelper* clipboardHelper)
{
    clipboardHelper->page()->executePasteFromClipboardItem(data, type);

    clipboardHelper->connectClipboardWindow();
}

static Eina_Bool clearClip(void* data, int type, void* event)
{
    ClipboardHelper* clipboardHelper = static_cast<ClipboardHelper*>(data);
    if (!evas_object_focus_get(clipboardHelper->ewkView()))
        return ECORE_CALLBACK_PASS_ON;

    Ecore_X_Event_Selection_Clear* clearEvent = (Ecore_X_Event_Selection_Clear*) event;
    Ecore_X_Window window = clearEvent->win;

    ClipType clipType = CLIP_TYPE_SECONDARY;
    clipList[clipType].requestedFormat = static_cast<Elm_Sel_Format>(ELM_SEL_FORMAT_TEXT | ELM_SEL_FORMAT_IMAGE);

    ecore_x_selection_secondary_request(window, ECORE_X_SELECTION_TARGET_TARGETS);

    return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool notifyClip(void* data , int type, void* event)
{
    ClipboardHelper* clipboardHelper = static_cast<ClipboardHelper*>(data);
    if (!evas_object_focus_get(clipboardHelper->ewkView()))
        return ECORE_CALLBACK_PASS_ON;

    Ecore_X_Event_Selection_Notify* notifytEvent = (Ecore_X_Event_Selection_Notify*) event;

    int i = 0;
    for (i = 0; i < CLIP_TYPE_MAX; i++) {
        if (clipList[i].selection == notifytEvent->selection) {
            break;
        }
    }

    ClipData* clipData = clipList + i;

    clipData->clipboardHelper= clipboardHelper;

    for (i = 0; i < ATOM_MAX; i++) {
        if (!strcmp(notifytEvent->target, atomList[i].pName)) {
            if (atomList[i].notify) {
                atomList[i].notify(clipData, notifytEvent);
            }
        }
    }

    return ECORE_CALLBACK_PASS_ON;
}

static int notifyTarget(ClipData* clipData, Ecore_X_Event_Selection_Notify* notifyData)
{
    Ecore_X_Atom dataType = 0;

    if (clipData->clipboardHelper->getSelectedCbhmItem(&dataType)) {
        const char* pHtmlAtomName = "text/html;charset=utf-8";
        Ecore_X_Atom htmlType = ecore_x_atom_get(pHtmlAtomName);

        if (dataType == htmlType) {
            clipData->request(notifyData->win, pHtmlAtomName);
            return ECORE_CALLBACK_PASS_ON;
        }
    }

    Ecore_X_Selection_Data_Targets* pTargets = (Ecore_X_Selection_Data_Targets*) (notifyData->data);
    Ecore_X_Atom* pAtomList = (Ecore_X_Atom*) (pTargets->data.data);

    int i, j = 0;
    for (j = (ATOM_LISTING_ATOMS+1); j < ATOM_MAX; j++) {

        if (!(atomList[j].formats & clipData->requestedFormat))
            continue;

        for (i = 0; i < pTargets->data.length; i++) {
            if ((atomList[j].atom == pAtomList[i]) && (atomList[j].notify)) {
                if ((j == ATOM_XELM) && (!(clipData->requestedFormat & ELM_SEL_FORMAT_MARKUP)))
                    continue;

                goto FOUND;
            }
        }
    }

    return ECORE_CALLBACK_PASS_ON;

    FOUND:
    clipData->request(notifyData->win, atomList[j].pName);

    clipData->clipboardHelper->m_allowNextCbAction = true;

    return ECORE_CALLBACK_PASS_ON;
}

static int notifyText(ClipData* clipData, Ecore_X_Event_Selection_Notify* notifyData)
{
    if (!clipData->clipboardHelper->m_allowNextCbAction)
        return 0;

    Ecore_X_Selection_Data* pData = (Ecore_X_Selection_Data*) notifyData->data;
    pasteSelectedClipboardItem(String::fromUTF8(pData->data), String("PlainText"), clipData->clipboardHelper);
    clipData->clipboardHelper->m_allowNextCbAction = false;
    return 0;
}

static int notifyImage(ClipData* clipData, Ecore_X_Event_Selection_Notify* notifyData)
{
    Ecore_X_Selection_Data* pData = (Ecore_X_Selection_Data*) notifyData->data;
    pasteSelectedClipboardItem(String::fromUTF8(pData->data, (int)pData->length), String("Image"), clipData->clipboardHelper);

    return 0;
}

static int notifyUri(ClipData* clipData, Ecore_X_Event_Selection_Notify* notifyData)
{
    Ecore_X_Selection_Data* pData = (Ecore_X_Selection_Data*) notifyData->data;
    pasteSelectedClipboardItem(String::fromUTF8(pData->data, (int)pData->length), String("Image"), clipData->clipboardHelper);

    return 0;
}

static int notifyEdje(ClipData* clipData, Ecore_X_Event_Selection_Notify* notifyData)
{
    Ecore_X_Selection_Data* pData = (Ecore_X_Selection_Data*) notifyData->data;
    pasteSelectedClipboardItem(String::fromUTF8(pData->data, (int)pData->length), String("PlainText"), clipData->clipboardHelper);

    return 0;
}

static int notifyHtml(ClipData* clipData, Ecore_X_Event_Selection_Notify* notifyData)
{
    Ecore_X_Selection_Data* pData = (Ecore_X_Selection_Data*) notifyData->data;
    pasteSelectedClipboardItem(String::fromUTF8(pData->data, (int)pData->length), String("Markup"), clipData->clipboardHelper);

    return 0;
}

void ClipboardHelper::initializeAtomList()
{
    atomList[ATOM_TARGETS].pName = "TARGETS";
    atomList[ATOM_TARGETS].formats = ELM_SEL_FORMAT_TARGETS;
    atomList[ATOM_TARGETS].notify = notifyTarget;
    atomList[ATOM_TARGETS].atom = 0;

    atomList[ATOM_ATOM].pName = "ATOM";
    atomList[ATOM_ATOM].formats = ELM_SEL_FORMAT_TARGETS;
    atomList[ATOM_ATOM].notify = notifyTarget;
    atomList[ATOM_ATOM].atom = 0;

    atomList[ATOM_XELM].pName = "application/x-elementary-markup";
    atomList[ATOM_XELM].formats = ELM_SEL_FORMAT_MARKUP;
    atomList[ATOM_XELM].notify = notifyEdje;
    atomList[ATOM_XELM].atom = 0;

    atomList[ATOM_TEXT_URI].pName = "text/uri";
    atomList[ATOM_TEXT_URI].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_TEXT_URI].notify = notifyUri;
    atomList[ATOM_TEXT_URI].atom = 0;

    atomList[ATOM_TEXT_URILIST].pName = "text/uri-list";
    atomList[ATOM_TEXT_URILIST].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_TEXT_URILIST].notify = notifyUri;
    atomList[ATOM_TEXT_URILIST].atom = 0;

    atomList[ATOM_TEXT_X_VCARD].pName = "text/x-vcard";
    atomList[ATOM_TEXT_X_VCARD].formats = ELM_SEL_FORMAT_VCARD;
    atomList[ATOM_TEXT_X_VCARD].notify = 0;
    atomList[ATOM_TEXT_X_VCARD].atom = 0;

    atomList[ATOM_IMAGE_PNG].pName = "image/png";
    atomList[ATOM_IMAGE_PNG].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_IMAGE_PNG].notify = notifyImage;
    atomList[ATOM_IMAGE_PNG].atom = 0;

    atomList[ATOM_IMAGE_JPEG].pName = "image/jpeg";
    atomList[ATOM_IMAGE_JPEG].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_IMAGE_JPEG].notify = notifyImage;
    atomList[ATOM_IMAGE_JPEG].atom = 0;

    atomList[ATOM_IMAGE_BMP].pName = "image/x-ms-bmp";
    atomList[ATOM_IMAGE_BMP].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_IMAGE_BMP].notify = notifyImage;
    atomList[ATOM_IMAGE_BMP].atom = 0;

    atomList[ATOM_IMAGE_GIF].pName = "image/gif";
    atomList[ATOM_IMAGE_GIF].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_IMAGE_GIF].notify = notifyImage;
    atomList[ATOM_IMAGE_GIF].atom = 0;

    atomList[ATOM_IMAGE_TIFF].pName = "image/tiff";
    atomList[ATOM_IMAGE_TIFF].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_IMAGE_TIFF].notify = notifyImage;
    atomList[ATOM_IMAGE_TIFF].atom = 0;

    atomList[ATOM_IMAGE_SVG].pName = "image/svg+xml";
    atomList[ATOM_IMAGE_SVG].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_IMAGE_SVG].notify = notifyImage;
    atomList[ATOM_IMAGE_SVG].atom = 0;

    atomList[ATOM_IMAGE_XPM].pName = "image/x-xpixmap";
    atomList[ATOM_IMAGE_XPM].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_IMAGE_XPM].notify = notifyImage;
    atomList[ATOM_IMAGE_XPM].atom = 0;

    atomList[ATOM_IMAGE_TGA].pName = "image/x-tga";
    atomList[ATOM_IMAGE_TGA].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_IMAGE_TGA].notify = notifyImage;
    atomList[ATOM_IMAGE_TGA].atom = 0;

    atomList[ATOM_IMAGE_PPM].pName = "image/x-portable-pixmap";
    atomList[ATOM_IMAGE_PPM].formats = ELM_SEL_FORMAT_IMAGE;
    atomList[ATOM_IMAGE_PPM].notify = notifyImage;
    atomList[ATOM_IMAGE_PPM].atom = 0;

    atomList[ATOM_TEXT_HTML_UTF8].pName = "text/html;charset=utf-8";
    atomList[ATOM_TEXT_HTML_UTF8].formats = ELM_SEL_FORMAT_HTML;
    atomList[ATOM_TEXT_HTML_UTF8].notify = notifyHtml;
    atomList[ATOM_TEXT_HTML_UTF8].atom = 0;

    atomList[ATOM_TEXT_HTML].pName = "text/html";
    atomList[ATOM_TEXT_HTML].formats = ELM_SEL_FORMAT_HTML;
    atomList[ATOM_TEXT_HTML].notify = notifyHtml;
    atomList[ATOM_TEXT_HTML].atom = 0;

    atomList[ATOM_STRING_UTF8].pName = "UTF8_STRING";
    atomList[ATOM_STRING_UTF8].formats = static_cast<Elm_Sel_Format> (ELM_SEL_FORMAT_TEXT| ELM_SEL_FORMAT_MARKUP| ELM_SEL_FORMAT_HTML);
    atomList[ATOM_STRING_UTF8].notify = notifyText;
    atomList[ATOM_STRING_UTF8].atom = 0;

    atomList[ATOM_STRING].pName = "STRING";
    atomList[ATOM_STRING].formats = static_cast<Elm_Sel_Format> (ELM_SEL_FORMAT_TEXT| ELM_SEL_FORMAT_MARKUP| ELM_SEL_FORMAT_HTML);
    atomList[ATOM_STRING].notify = notifyText;
    atomList[ATOM_STRING].atom = 0;

    atomList[ATOM_TEXT].pName = "TEXT";
    atomList[ATOM_TEXT].formats = static_cast<Elm_Sel_Format> (ELM_SEL_FORMAT_TEXT| ELM_SEL_FORMAT_MARKUP| ELM_SEL_FORMAT_HTML);
    atomList[ATOM_TEXT].notify = 0;
    atomList[ATOM_TEXT].atom = 0;

    atomList[ATOM_TEXT_PLAIN_UTF8].pName = "text/plain;charset=utf-8";
    atomList[ATOM_TEXT_PLAIN_UTF8].formats = static_cast<Elm_Sel_Format> (ELM_SEL_FORMAT_TEXT| ELM_SEL_FORMAT_MARKUP| ELM_SEL_FORMAT_HTML);
    atomList[ATOM_TEXT_PLAIN_UTF8].notify = 0;
    atomList[ATOM_TEXT_PLAIN_UTF8].atom = 0;

    atomList[ATOM_TEXT_PLAIN].pName = "text/plain";
    atomList[ATOM_TEXT_PLAIN].formats = static_cast<Elm_Sel_Format> (ELM_SEL_FORMAT_TEXT| ELM_SEL_FORMAT_MARKUP| ELM_SEL_FORMAT_HTML);
    atomList[ATOM_TEXT_PLAIN].notify = 0;
    atomList[ATOM_TEXT_PLAIN].atom = 0;

    clipList[CLIP_TYPE_PRIMARY].selection = ECORE_X_SELECTION_PRIMARY;
    clipList[CLIP_TYPE_PRIMARY].request = ecore_x_selection_primary_request;
    clipList[CLIP_TYPE_PRIMARY].bufferLength = 0;

    clipList[CLIP_TYPE_SECONDARY].selection = ECORE_X_SELECTION_SECONDARY;
    clipList[CLIP_TYPE_SECONDARY].request = ecore_x_selection_secondary_request;
    clipList[CLIP_TYPE_SECONDARY].bufferLength = 0;

    clipList[CLIP_TYPE_CLIPBOARD].selection = ECORE_X_SELECTION_CLIPBOARD;
    clipList[CLIP_TYPE_CLIPBOARD].request = ecore_x_selection_clipboard_request;
    clipList[CLIP_TYPE_CLIPBOARD].bufferLength = 0;

    clipList[CLIP_TYPE_XDND].selection = ECORE_X_SELECTION_XDND;
    clipList[CLIP_TYPE_XDND].request = ecore_x_selection_xdnd_request;
    clipList[CLIP_TYPE_XDND].bufferLength = 0;

    for (int i = 0; i < ATOM_MAX; i++)
        atomList[i].atom = ecore_x_atom_get(atomList[i].pName);

    m_cbhmPropertyChangeHandler = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, cbhmPropertyChangeCallback, this);
    m_selectionClearHandler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_CLEAR, clearClip, this);
    m_selectionNotifyHandler = ecore_event_handler_add(ECORE_X_EVENT_SELECTION_NOTIFY, notifyClip, this);

    TIZEN_LOGI("m_cbhmPropertyChangeHandler->[%p]\nm_selectionClearHandler->[%p]\nm_selectionNotifyHandler->[%p]", m_cbhmPropertyChangeHandler, m_selectionClearHandler, m_selectionNotifyHandler);
}

bool ClipboardHelper::sendCbhmMessage(String message)
{
    Ecore_X_Window xCbhmWin = getCbhmWindow();
    Ecore_X_Atom xAtomCbhmMsg = ecore_x_atom_get("CBHM_MSG");

    if (!xCbhmWin || !xAtomCbhmMsg)
        return false;

    if (!m_viewImpl->view())
        return false;

    Evas_Object* parent = elm_object_parent_widget_get(m_viewImpl->view());

    if (!parent)
        return false;

    Ecore_X_Window xWin = 0;
    const char* engine = ecore_evas_engine_name_get(ecore_evas_ecore_evas_get(evas_object_evas_get(parent)));
    if (engine) {
        if (!strcmp(engine, "opengl_x11"))
            xWin = ecore_evas_gl_x11_window_get(ecore_evas_ecore_evas_get(evas_object_evas_get(parent)));
        else if (!strcmp(engine, "software_x11"))
            xWin = ecore_evas_software_x11_window_get(ecore_evas_ecore_evas_get(evas_object_evas_get(parent)));
    }

    if (!xWin)
        return false;

    XClientMessageEvent messageEvent;
    memset(&messageEvent, 0, sizeof(messageEvent));
    messageEvent.type = ClientMessage;
    messageEvent.display = static_cast<Display *> (ecore_x_display_get());
    messageEvent.window = xWin;
    messageEvent.message_type = xAtomCbhmMsg;
    messageEvent.format = 8;
    snprintf(messageEvent.data.b, 20, "%s", message.utf8().data());

    XSendEvent(static_cast<Display *> (ecore_x_display_get()), xCbhmWin, false, NoEventMask, (XEvent*)&messageEvent);

    ecore_x_sync();

    return true;
}

bool ClipboardHelper::setClipboardItem(Ecore_X_Atom dataType,const String& data)
{
    Ecore_X_Window cbhmWin = getCbhmWindow();
    Ecore_X_Atom atomCbhmItem = ecore_x_atom_get("CBHM_ITEM");
    String clipboardData = data;

    if (clipboardData.startsWith("file://"))
        clipboardData = clipboardData.substring(7, clipboardData.length()-7);

    CString utfData = clipboardData.utf8();

    ecore_x_sync();
    ecore_x_window_prop_property_set(cbhmWin, atomCbhmItem, dataType, 8, const_cast<char*>(utfData.data()), utfData.length() + 1);
    ecore_x_sync();

    if (sendCbhmMessage(String::fromUTF8(static_cast<const char*>("SET_ITEM"))))
        return true;

    return false;
}

void ClipboardHelper::openClipboardWindow(bool isContentRichlyEditable)
{
    clearClipboardSelectionHandler();
    initializeAtomList();

    if (isContentRichlyEditable)
        sendCbhmMessage(String::fromUTF8(static_cast<const char*>("show1")));
    else
        sendCbhmMessage(String::fromUTF8(static_cast<const char*>("show0")));

    connectClipboardWindow();

    evas_object_smart_callback_call(m_viewImpl->view(), "clipboard,opened", 0);
}

void ClipboardHelper::closeClipboardWindow()
{
    sendCbhmMessage(String::fromUTF8(static_cast<const char*>("cbhm_hide")));
}

bool ClipboardHelper::isClipboardWindowOpened()
{
    return m_clipboardWindowOpened;
}

void ClipboardHelper::connectClipboardWindow()
{
    ecore_x_selection_secondary_set(ecore_x_window_focus_get(), "",1);
}

void ClipboardHelper::clearClipboardSelectionHandler()
{
    TIZEN_LOGI("m_cbhmPropertyChangeHandler->[%p]\nm_selectionClearHandler->[%p]\nm_selectionNotifyHandler->[%p]", m_cbhmPropertyChangeHandler, m_selectionClearHandler, m_selectionNotifyHandler);

    if (m_selectionClearHandler) {
        ecore_event_handler_del(m_selectionClearHandler);
        m_selectionClearHandler = 0;
    }
    if (m_selectionNotifyHandler) {
        ecore_event_handler_del(m_selectionNotifyHandler);
        m_selectionNotifyHandler = 0;
    }
    if (m_cbhmPropertyChangeHandler) {
        ecore_event_handler_del(m_cbhmPropertyChangeHandler);
        m_cbhmPropertyChangeHandler = 0;
    }
    m_clipboardWindowOpened = false;
}

bool ClipboardHelper::getSelectedCbhmItem(Ecore_X_Atom* pDataType)
{
    Ecore_X_Window cbhmWin = getCbhmWindow();
    Ecore_X_Atom atomCbhmItem = ecore_x_atom_get("CBHM_SELECTED_ITEM");
    Ecore_X_Atom atomItemType = 0;

    String result = getCbhmReply(cbhmWin, atomCbhmItem, &atomItemType);

    if(result.isNull())
        return false;

    if (atomItemType == ecore_x_atom_get("CBHM_ERROR"))
        return false;

    if (pDataType)
        *pDataType = atomItemType;

    return true;
}

void ClipboardHelper::pasteClipboardLastItem(bool isContentEditable)
{
    // Find the item with dataTypes.
    int format = ELM_SEL_FORMAT_NONE;
    String clipboardData;

    bool ret = retrieveClipboardItem(0, &format, &clipboardData);

    if (!ret)
        return;

    switch(format) {
    case ELM_SEL_FORMAT_TEXT:
    case ELM_SEL_FORMAT_MARKUP:
        page()->didSelectPasteMenuFromContextMenu(clipboardData, String("PlainText"));
        break;
    case ELM_SEL_FORMAT_HTML:
        page()->didSelectPasteMenuFromContextMenu(clipboardData, String("Markup"));
        break;
    case ELM_SEL_FORMAT_IMAGE:
        if (isContentEditable)
            page()->didSelectPasteMenuFromContextMenu(clipboardData, String("Image"));
        break;
    default:
        page()->didSelectPasteMenuFromContextMenu(String(""), String("PlainText"));
        break;
    }
}

void ClipboardHelper::updateClipboardWindowState(Ecore_X_Event_Window_Property* ev)
{
    Ecore_X_Illume_Clipboard_State state = ecore_x_e_illume_clipboard_state_get(ev->win);

    if (state == ECORE_X_ILLUME_CLIPBOARD_STATE_UNKNOWN)
        state = ecore_x_e_illume_clipboard_state_get(ev->win);

    if (state == ECORE_X_ILLUME_CLIPBOARD_STATE_OFF)
        clearClipboardSelectionHandler();
    else if (state == ECORE_X_ILLUME_CLIPBOARD_STATE_ON)
        m_clipboardWindowOpened = true;
}
#endif // TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD

Ecore_X_Window getCbhmWindow()
{
    Ecore_X_Atom xAtomCbhm = ecore_x_atom_get("CBHM_XWIN");
    Ecore_X_Window xCbhmWin = 0;

    unsigned char* buf = 0;
    int num = 0;
    int ret = ecore_x_window_prop_property_get(0, xAtomCbhm, XA_WINDOW, 0, &buf, &num);

    if (ret && num)
        memcpy(&xCbhmWin, buf, sizeof(Ecore_X_Window));

    if (buf)
        free(buf);

    return xCbhmWin;
}

String getCbhmReply(Ecore_X_Window xwin, Ecore_X_Atom property, Ecore_X_Atom* pDataType)
{
    if (!property)
        return String();

    ecore_x_sync();

    Ecore_X_Atom type;
    int dataUnitSize = 0;
    long unsigned int dataLength = 0;
    long unsigned int bytes = 0;
    unsigned char* pData = 0;

    int result = XGetWindowProperty((Display*)ecore_x_display_get(), xwin, property, 0, LONG_MAX, False,
                                 ecore_x_window_prop_any_type(), (Atom*)&type, &dataUnitSize, &dataLength, &bytes, &pData);
    if (result != Success)
        return String();

    if (!dataLength) {
        XFree(pData);
        return String();
    }

    String cbhmData;

    switch (dataUnitSize) {
    case 8:
        cbhmData = String::fromUTF8(pData);
        break;
    case 16:
        cbhmData = String(reinterpret_cast<UChar*>(pData), dataLength);
        LOG_ERROR("case 16");
        break;
    case 32:
        LOG_ERROR("case 32");
        break;
    default:
        LOG_ERROR("case %d", dataUnitSize);
    }

    XFree(pData);

    if (pDataType)
        *pDataType = type;

    return cbhmData;
}

bool retrieveClipboardItem(int index, int* format, String* pData)
{
    if (!pData)
        return false;

    Ecore_X_Window cbhmWin = getCbhmWindow();

    Ecore_X_Atom atomCbhmItem = ecore_x_atom_get(String::format("CBHM_ITEM%d", index).utf8().data());
    Ecore_X_Atom atomItemType = 0;

    String result = getCbhmReply(cbhmWin, atomCbhmItem, &atomItemType);
    if (result.isNull())
        return false;

    *pData = result;

    Ecore_X_Atom dataType = atomItemType;

    if (atomItemType == ecore_x_atom_get("CBHM_ERROR"))
        return false;

    if ((dataType == ecore_x_atom_get("UTF8_STRING")) || (dataType == ecore_x_atom_get("application/x-elementary-markup")))
        *format = ELM_SEL_FORMAT_TEXT;
    else if (dataType == ecore_x_atom_get("text/uri"))
        *format = ELM_SEL_FORMAT_IMAGE;
    else if (dataType == ecore_x_atom_get("text/html;charset=utf-8"))
        *format = ELM_SEL_FORMAT_HTML;
    else
        return false;

    if (dataType == ecore_x_atom_get("application/x-elementary-markup"))
        *pData = String::fromUTF8(evas_textblock_text_markup_to_utf8(NULL, pData->utf8().data()));

    return true;
}

bool ClipboardHelper::isPastedItemOnlyImage(bool isContentRichlyEditable)
{
    int format = ELM_SEL_FORMAT_NONE;
    String clipboardData;

    int ret = retrieveClipboardItem(0, &format, &clipboardData);
    if (!ret || clipboardData.isEmpty())
        return true;

    if (!isContentRichlyEditable && format == ELM_SEL_FORMAT_IMAGE)
        return true;

    return false;
}

}// namespace WebKit

#endif // TIZEN_WEBKIT2_CLIPBOARD_HELPER
