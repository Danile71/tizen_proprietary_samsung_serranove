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

#ifndef WebFormData_h
#define WebFormData_h

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)

#include <WebCore/HTMLFormElement.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

class WebFormData : public APIObject  {
public:
    static const Type APIType = TypeFormData;

    struct Data {

        String m_id;
        String m_sourceFrameURL;
        bool m_containsPassword;
        bool m_shouldAutocomplete;

        struct ValueData {
            String m_type;
            String m_name;
            String m_value;
            String m_id;
            bool m_shouldAutocomplete;

            ValueData()
            {
            }

            ValueData(const String& type, const String& name, const String& value, const String& id, bool shouldAutocomplete)
                : m_type(type)
                , m_name(name)
                , m_value(value)
                , m_id(id)
                , m_shouldAutocomplete(shouldAutocomplete)
            {
            }

            void encode(CoreIPC::ArgumentEncoder*) const;
            static bool decode(CoreIPC::ArgumentDecoder*, WebFormData::Data::ValueData&);
        };

        Vector<ValueData> m_values;

        Data()
        {
        }

        Data(WebCore::HTMLFormElement*);

        void encode(CoreIPC::ArgumentEncoder*) const;
        static bool decode(CoreIPC::ArgumentDecoder*, WebFormData::Data&);
    };

    static PassRefPtr<WebFormData> create(const WebFormData::Data& formData)
    {
        return adoptRef(new WebFormData(formData));
    }

    const String& id() const { return m_data.m_id; }
    const String& sourceFrameURL() const { return m_data.m_sourceFrameURL; }
    bool containsPassword() const { return m_data.m_containsPassword; }
    bool shouldAutocomplete() const { return m_data.m_shouldAutocomplete; }
    const Vector<Data::ValueData>& values() const { return m_data.m_values; }

    const Data& data() const { return m_data; }

private:
    explicit WebFormData(const WebFormData::Data& formData)
        : m_data(formData)
    {
    }

    virtual Type type() const { return APIType; }

    Data m_data;
};

} // namespace WebKit

#endif // ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#endif // WebFormData_h
