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

#ifndef ClipboardHelper_h
#define ClipboardHelper_h

#include "EwkViewImpl.h"

#include <wtf/Deque.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
#include <Ecore.h>
#include <Ecore_X.h>
#endif

namespace WebKit {

typedef void (*ClipboardDataResultFunction)(const String&, const String&, void*);

// Utility functions to get clipboard data from CbhmWindow
Ecore_X_Window getCbhmWindow();
String getCbhmReply(Ecore_X_Window xwin, Ecore_X_Atom property, Ecore_X_Atom* pDataType);
bool retrieveClipboardItem(int index, int* format, String* pData);

class ClipboardCallback : public RefCounted<ClipboardCallback> {
public:

    static PassRefPtr<ClipboardCallback> create(void* context, ClipboardDataResultFunction callback)
    {
        return adoptRef(new ClipboardCallback(context, callback));
    }

    ~ClipboardCallback()
    {
        ASSERT(!m_callback);
    }

    void performCallback(const String& data, const String& type)
    {
        ASSERT(m_callback);
        m_callback(data, type, m_context);
        m_callback = 0;
    }

private:
    ClipboardCallback(void* context, ClipboardDataResultFunction callback)
        : m_context(context)
        , m_callback(callback)
    {
    }

    void* m_context;
    ClipboardDataResultFunction m_callback;

};

class ClipboardHelper {
public:
    static PassOwnPtr<ClipboardHelper> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new ClipboardHelper(viewImpl));
    }
    ~ClipboardHelper();

    enum SelectionType {
        SelectionTypePrimary,
        SelectionTypeSecondary,
        SelectionTypeXDND,
        SelectionTypeClipboard
    };

    static int numberOfItems();

    void setData(const String& data, const String& type);

    void clear();

    void processResult(const String& data, const String& type);
    void pasteClipboardLastItem(bool isContentEditable);

    PageClientImpl* pageClient() { return m_viewImpl->pageClient.get(); }

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    void openClipboardWindow(bool isContentRichlyEditable);
    void closeClipboardWindow();
    bool isClipboardWindowOpened();
    void connectClipboardWindow();
    void clearClipboardSelectionHandler();
    Evas_Object* ewkView() { return m_viewImpl->view(); }
    WebPageProxy* page() { return m_viewImpl->page(); }
    bool getSelectedCbhmItem(Ecore_X_Atom* pDataType);
    void updateClipboardWindowState(Ecore_X_Event_Window_Property* ev);
    bool isPastedItemOnlyImage(bool isContentRichlyEditable);
#endif

private:
    ClipboardHelper(EwkViewImpl*);

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    void initializeAtomList();
    bool sendCbhmMessage(String message);
    bool setClipboardItem(Ecore_X_Atom dataType, const String& data);
#endif

    EwkViewImpl* m_viewImpl;

    Deque<RefPtr<ClipboardCallback> > m_callbackQueue;
    Mutex m_callbackQueueLock;

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
    Ecore_Event_Handler* m_selectionClearHandler;
    Ecore_Event_Handler* m_selectionNotifyHandler;
    Ecore_Event_Handler* m_clientMessageHandler;
    Ecore_Event_Handler* m_cbhmPropertyChangeHandler;

    bool m_clipboardWindowOpened;
#endif
public:
    bool m_allowNextCbAction;
};

} // namespace WebKit

#endif // ClipboardHelper_h
