/*
   Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/


#include "config.h"

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)

#include "WebFormData.h"

#include "ArgumentCoders.h"
#include "Arguments.h"
#include <WebCore/HTMLFormElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLSelectElement.h>
#include <WebCore/HTMLNames.h>
#include <WebCore/NodeList.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

WebFormData::Data::Data(HTMLFormElement* form)
{
    if (!form)
        return;

    m_id = form->fastGetAttribute(idAttr);
    m_sourceFrameURL = form->document()->url().host();
    m_containsPassword = false;
    m_shouldAutocomplete = form->shouldAutocomplete();

    Node* focusedNode = form->document()->focusedNode();
    if (focusedNode && focusedNode->isHTMLElement()) {
        HTMLFormElement* formForFocusedNode = toHTMLElement(focusedNode)->form();
        if (form != formForFocusedNode)
            return;
    }

    for (unsigned i = 0; i < form->associatedElements().size(); ++i) {
        FormAssociatedElement* control = form->associatedElements()[i];
        HTMLElement* element = toHTMLElement(control);
        if (!element->hasLocalName(inputTag))
            continue;

        HTMLInputElement* input = static_cast<HTMLInputElement*>(control);
        if (!input->isTextField() || input->value().isEmpty() || input->name().isEmpty())
            continue;

        if (input->isPasswordField())
            m_containsPassword = true;

        if (!input->shouldAutocomplete())
            m_shouldAutocomplete = input->shouldAutocomplete();

        m_values.append(Data::ValueData(input->type().string(), input->name().string(), input->value(), input->fastGetAttribute(idAttr).string(), input->shouldAutocomplete()));
    }

    RefPtr<NodeList> list = form->getElementsByTagName("select");
    unsigned len = list->length();
    for (unsigned i = 0; i < len; i++) {
        HTMLSelectElement* select = static_cast<HTMLSelectElement*>(list->item(i));
        m_values.append(Data::ValueData(select->type().string(), select->fastGetAttribute(nameAttr).string(), select->value(), select->fastGetAttribute(idAttr).string(), true));
    }
}

void WebFormData::Data::ValueData::encode(CoreIPC::ArgumentEncoder* encoder) const
{
    encoder->encode(m_type);
    encoder->encode(m_name);
    encoder->encode(m_value);
    encoder->encode(m_id);
    encoder->encode(m_shouldAutocomplete);
}

bool WebFormData::Data::ValueData::decode(CoreIPC::ArgumentDecoder* decoder, WebFormData::Data::ValueData& data)
{
    if (!decoder->decode(data.m_type))
        return false;

    if (!decoder->decode(data.m_name))
        return false;

    if (!decoder->decode(data.m_value))
        return false;

    if (!decoder->decode(data.m_id))
        return false;

    if (!decoder->decode(data.m_shouldAutocomplete))
        return false;

    return true;
}


void WebFormData::Data::encode(CoreIPC::ArgumentEncoder* encoder) const
{
    encoder->encode(m_id);
    encoder->encode(m_sourceFrameURL);
    encoder->encode(m_containsPassword);
    encoder->encode(m_shouldAutocomplete);
    encoder->encode(m_values);
}

bool WebFormData::Data::decode(CoreIPC::ArgumentDecoder* decoder, WebFormData::Data& data)
{
    if (!decoder->decode(data.m_id))
        return false;

    if (!decoder->decode(data.m_sourceFrameURL))
        return false;

    if (!decoder->decode(data.m_containsPassword))
        return false;

    if (!decoder->decode(data.m_shouldAutocomplete))
        return false;

    if (!decoder->decode(data.m_values))
        return false;

    return true;
}


} // namespace WebKit
#endif // ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
