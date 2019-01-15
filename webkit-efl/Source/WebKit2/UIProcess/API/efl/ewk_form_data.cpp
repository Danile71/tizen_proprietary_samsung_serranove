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
#include "ewk_form_data.h"

#if PLATFORM(TIZEN)
#include "WKAPICast.h"
#include "WKArray.h"
#include "WKDictionary.h"
#include "WKFrame.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "WKURL.h"
#include <wtf/OwnArrayPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

struct _Ewk_Form_Data {
    Eina_Hash* values;
    const char* url;

    _Ewk_Form_Data()
        : values(NULL)
        , url(NULL)
    {

    }
};

static void freeFormValues(void* data)
{
    EINA_SAFETY_ON_NULL_RETURN(data);
    eina_stringshare_del(static_cast<char*>(data));
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
Ewk_Form_Data* ewkFormDataCreate(WKFrameRef frame, WKFormDataRef formDataRef)
{
    WebFormData* formData = toImpl(formDataRef);

    Eina_Hash* formValues = eina_hash_string_small_new(freeFormValues);
    int size = formData->values().size();
    for (int i = 0; i < size; i++)
        eina_hash_add(formValues, formData->values()[i].m_name.utf8().data(), eina_stringshare_add(formData->values()[i].m_value.utf8().data()));

    Ewk_Form_Data* ewkFormData = new Ewk_Form_Data;
    ewkFormData->values = formValues;

    WKRetainPtr<WKURLRef> wkUrl(AdoptWK, WKFrameCopyURL(frame));
    if (wkUrl)
        ewkFormData->url = eina_stringshare_add(toImpl(wkUrl.get())->string().utf8().data());

    return ewkFormData;
}
#else
Ewk_Form_Data* ewkFormDataCreate(WKFrameRef frame, WKDictionaryRef values)
{
    WKRetainPtr<WKArrayRef> keys(AdoptWK, WKDictionaryCopyKeys(values));
    int size = WKArrayGetSize(keys.get());
    Eina_Hash* formValues = eina_hash_string_small_new(freeFormValues);
    for (int i = 0; i < size; i++) {
        WKStringRef key = static_cast<WKStringRef>(WKArrayGetItemAtIndex(keys.get(), i));
        if (WKGetTypeID(key) == WKStringGetTypeID()) {
            WKStringRef value = static_cast<WKStringRef>(WKDictionaryGetItemForKey(values, key));
            if (WKGetTypeID(value) == WKStringGetTypeID()) {
                int length = WKStringGetMaximumUTF8CStringSize(key);
                if (length <= 0)
                    continue;
                OwnArrayPtr<char> keyBuffer = adoptArrayPtr(new char[length]);
                WKStringGetUTF8CString(key, keyBuffer.get(), length);

                length = WKStringGetMaximumUTF8CStringSize(value);
                if (length <= 0)
                    continue;
                OwnArrayPtr<char> valueBuffer = adoptArrayPtr(new char[length]);
                WKStringGetUTF8CString(value, valueBuffer.get(), length);
                eina_hash_add(formValues, keyBuffer.get(), eina_stringshare_add(valueBuffer.get()));
            }
        }
    }


    Ewk_Form_Data* formData = new Ewk_Form_Data;
    formData->values = formValues;
    WKRetainPtr<WKURLRef> wkUrl(AdoptWK, WKFrameCopyURL(frame));
    if (wkUrl)
        formData->url = eina_stringshare_add(toImpl(wkUrl.get())->string().utf8().data());

    return formData;
}
#endif

void ewkFormDataDelete(Ewk_Form_Data* formData)
{
    EINA_SAFETY_ON_NULL_RETURN(formData);

    eina_stringshare_del(formData->url);
    eina_hash_free(formData->values);

    delete formData;
}

const char* ewk_form_data_url_get(Ewk_Form_Data* formData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(formData, 0);
    return formData->url;
}

Eina_Hash* ewk_form_data_values_get(Ewk_Form_Data* formData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(formData, 0);
    return formData->values;
}

#endif // #if PLATFORM(TIZEN)
