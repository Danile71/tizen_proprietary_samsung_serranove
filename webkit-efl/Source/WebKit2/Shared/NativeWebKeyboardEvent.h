/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011 Igalia S.L
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

#ifndef NativeWebKeyboardEvent_h
#define NativeWebKeyboardEvent_h

#include "WebEvent.h"

#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
OBJC_CLASS NSView;
#elif PLATFORM(QT)
#include <QKeyEvent>
#elif PLATFORM(GTK)
#include <GOwnPtrGtk.h>
typedef union _GdkEvent GdkEvent;
#elif PLATFORM(EFL)
#include <Evas.h>
#include <Ecore_IMF.h>
#endif

namespace WebKit {

#if ENABLE(TIZEN_ISF_PORT)
class EcoreIMFEvent : public RefCounted<EcoreIMFEvent> {
public:
    EcoreIMFEvent(const Evas_Event_Key_Down*);
    EcoreIMFEvent(const Evas_Event_Key_Up*);
    ~EcoreIMFEvent();

    Ecore_IMF_Event_Type type() const { return m_type; }
    const Ecore_IMF_Event* event() const { return &m_event; }

private:
    Ecore_IMF_Event_Type m_type;
    Ecore_IMF_Event m_event;
};
#endif

class NativeWebKeyboardEvent : public WebKeyboardEvent {
public:
#if USE(APPKIT)
    NativeWebKeyboardEvent(NSEvent *, NSView *);
#elif PLATFORM(WIN)
    NativeWebKeyboardEvent(HWND, UINT message, WPARAM, LPARAM);
#elif PLATFORM(QT)
    explicit NativeWebKeyboardEvent(QKeyEvent*);
#elif PLATFORM(GTK)
    NativeWebKeyboardEvent(const NativeWebKeyboardEvent&);
    NativeWebKeyboardEvent(GdkEvent*);
#elif PLATFORM(EFL)
    NativeWebKeyboardEvent(const Evas_Event_Key_Down*, bool);
    NativeWebKeyboardEvent(const Evas_Event_Key_Up*);
#if ENABLE(TIZEN_ISF_PORT)
    NativeWebKeyboardEvent();
#endif
#endif

#if USE(APPKIT)
    NSEvent *nativeEvent() const { return m_nativeEvent.get(); }
#elif PLATFORM(WIN)
    const MSG* nativeEvent() const { return &m_nativeEvent; }
#elif PLATFORM(QT)
    const QKeyEvent* nativeEvent() const { return &m_nativeEvent; }
#elif PLATFORM(GTK)
    GdkEvent* nativeEvent() const { return m_nativeEvent.get(); }
#elif PLATFORM(EFL)
#if ENABLE(TIZEN_ISF_PORT)
    const EcoreIMFEvent* nativeEvent() const { return m_nativeEvent.get(); }

    void setIsFiltered(bool isFiltered) { m_isFiltered = isFiltered; }
    bool isFiltered() const { return m_isFiltered; }

    void setInputMethodContextID(uintptr_t id) { m_inputMethodContextID = id; }
    uintptr_t inputMethodContextID() const { return m_inputMethodContextID; }

    void encode(CoreIPC::ArgumentEncoder*) const;
    static bool decode(CoreIPC::ArgumentDecoder*, NativeWebKeyboardEvent&);
#else
    const void* nativeEvent() const { return m_nativeEvent; }
    bool isFiltered() const { return m_isFiltered; }
#endif
#endif

private:
#if USE(APPKIT)
    RetainPtr<NSEvent> m_nativeEvent;
#elif PLATFORM(WIN)
    MSG m_nativeEvent;
#elif PLATFORM(QT)
    QKeyEvent m_nativeEvent;
#elif PLATFORM(GTK)
    GOwnPtr<GdkEvent> m_nativeEvent;
#elif PLATFORM(EFL)
#if ENABLE(TIZEN_ISF_PORT)
    RefPtr<EcoreIMFEvent> m_nativeEvent;
    bool m_isFiltered;
    uintptr_t m_inputMethodContextID;
#else
    const void* m_nativeEvent;
    bool m_isFiltered;
#endif
#endif
};

} // namespace WebKit

#endif // NativeWebKeyboardEvent_h
