/*
 * Copyright (C) 2011 Samsung Electronics
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
#include "NativeWebKeyboardEvent.h"

#include "WebCoreArgumentCoders.h"
#include "WebEventFactory.h"

namespace WebKit {

EcoreIMFEvent::EcoreIMFEvent(const Evas_Event_Key_Down* event)
    : m_type(ECORE_IMF_EVENT_KEY_DOWN)
{
    Ecore_IMF_Event_Key_Down* keyDownEvent = &m_event.key_down;

    ecore_imf_evas_event_key_down_wrap(const_cast<Evas_Event_Key_Down*>(event), keyDownEvent);

    if (keyDownEvent->keyname)
        keyDownEvent->keyname = strdup(keyDownEvent->keyname);
    if (keyDownEvent->key)
        keyDownEvent->key = strdup(keyDownEvent->key);
    if (keyDownEvent->string)
        keyDownEvent->string = strdup(keyDownEvent->string);
    if (keyDownEvent->compose)
        keyDownEvent->compose = strdup(keyDownEvent->compose);
}

EcoreIMFEvent::EcoreIMFEvent(const Evas_Event_Key_Up* event)
    : m_type(ECORE_IMF_EVENT_KEY_UP)
{
    Ecore_IMF_Event_Key_Up* keyUpEvent = &m_event.key_up;

    ecore_imf_evas_event_key_up_wrap(const_cast<Evas_Event_Key_Up*>(event), keyUpEvent);

    if (keyUpEvent->keyname)
        keyUpEvent->keyname = strdup(keyUpEvent->keyname);
    if (keyUpEvent->key)
        keyUpEvent->key = strdup(keyUpEvent->key);
    if (keyUpEvent->string)
        keyUpEvent->string = strdup(keyUpEvent->string);
    if (keyUpEvent->compose)
        keyUpEvent->compose = strdup(keyUpEvent->compose);
}

EcoreIMFEvent::~EcoreIMFEvent()
{
    if (m_type == ECORE_IMF_EVENT_KEY_DOWN) {
        Ecore_IMF_Event_Key_Down* keyDownEvent = &m_event.key_down;
        if (keyDownEvent->keyname)
            free(const_cast<char*>(keyDownEvent->keyname));
        if (keyDownEvent->key)
            free(const_cast<char*>(keyDownEvent->key));
        if (keyDownEvent->string)
            free(const_cast<char*>(keyDownEvent->string));
        if (keyDownEvent->compose)
            free(const_cast<char*>(keyDownEvent->compose));
    } else if (m_type == ECORE_IMF_EVENT_KEY_UP) {
        Ecore_IMF_Event_Key_Up* keyUpEvent = &m_event.key_up;
        if (keyUpEvent->keyname)
            free(const_cast<char*>(keyUpEvent->keyname));
        if (keyUpEvent->key)
            free(const_cast<char*>(keyUpEvent->key));
        if (keyUpEvent->string)
            free(const_cast<char*>(keyUpEvent->string));
        if (keyUpEvent->compose)
            free(const_cast<char*>(keyUpEvent->compose));
    }
}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(const Evas_Event_Key_Down* event, bool isFiltered)
    : WebKeyboardEvent(WebEventFactory::createWebKeyboardEvent(event))
    , m_nativeEvent(adoptRef(new EcoreIMFEvent(event)))
    , m_isFiltered(isFiltered)
    , m_inputMethodContextID(0)
{
}

NativeWebKeyboardEvent::NativeWebKeyboardEvent(const Evas_Event_Key_Up* event)
    : WebKeyboardEvent(WebEventFactory::createWebKeyboardEvent(event))
    , m_nativeEvent(adoptRef(new EcoreIMFEvent(event)))
    , m_isFiltered(false)
    , m_inputMethodContextID(0)
{
}

NativeWebKeyboardEvent::NativeWebKeyboardEvent()
    : m_isFiltered(false)
    , m_inputMethodContextID(0)
{
}

void NativeWebKeyboardEvent::encode(CoreIPC::ArgumentEncoder* encoder) const
{
    WebKeyboardEvent::encode(encoder);
    encoder->encode(m_isFiltered);
    encoder->encode(m_inputMethodContextID);
}

bool NativeWebKeyboardEvent::decode(CoreIPC::ArgumentDecoder* decoder, NativeWebKeyboardEvent& result)
{
    if (!WebKeyboardEvent::decode(decoder, result))
        return false;

    if (!decoder->decode(result.m_isFiltered))
        return false;

    if (!decoder->decode(result.m_inputMethodContextID))
        return false;

    return true;
}

} // namespace WebKit
