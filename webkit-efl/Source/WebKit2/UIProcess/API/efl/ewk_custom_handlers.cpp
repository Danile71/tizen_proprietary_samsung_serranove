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
#include "ewk_custom_handlers.h"

#if (ENABLE_TIZEN_REGISTER_PROTOCOL_HANDLER) || (ENABLE_TIZEN_REGISTER_CONTENT_HANDLER) || (ENABLE_TIZEN_CUSTOM_SCHEME_HANDLER)
#include "ewk_view_private.h"

struct _Ewk_Custom_Handlers_Data {
    const char* target;
    const char* base_url;
    const char* url;
    const char* title;
    Ewk_Custom_Handlers_State result;
};

const char* ewk_custom_handlers_data_target_get(Ewk_Custom_Handlers_Data* customHandlersData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(customHandlersData, 0);

    return customHandlersData->target;
}

const char* ewk_custom_handlers_data_base_url_get(Ewk_Custom_Handlers_Data* customHandlersData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(customHandlersData, 0);

    return customHandlersData->base_url;
}

const char* ewk_custom_handlers_data_url_get(Ewk_Custom_Handlers_Data* customHandlersData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(customHandlersData, 0);

    return customHandlersData->url;
}

const char* ewk_custom_handlers_data_title_get(Ewk_Custom_Handlers_Data* customHandlersData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(customHandlersData, 0);

    return customHandlersData->title;
}

void ewk_custom_handlers_data_result_set(Ewk_Custom_Handlers_Data* customHandlersData, Ewk_Custom_Handlers_State result)
{
    EINA_SAFETY_ON_NULL_RETURN(customHandlersData);

    customHandlersData->result = result;
}

Ewk_Custom_Handlers_State ewkGetCustomHandlersDataResult(Ewk_Custom_Handlers_Data* customHandlersData)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(customHandlersData, EWK_CUSTOM_HANDLERS_DECLINED);

    return customHandlersData->result;
}

Ewk_Custom_Handlers_Data* ewkCustomHandlersCreateData(const char* target, const char* baseUrl, const char* url, const char* title)
{
    Ewk_Custom_Handlers_Data* customHandlersData = new Ewk_Custom_Handlers_Data;
    customHandlersData->target = eina_stringshare_add(target);
    customHandlersData->base_url = eina_stringshare_add(baseUrl);
    customHandlersData->url = eina_stringshare_add(url);
    customHandlersData->title = eina_stringshare_add(title);

    return customHandlersData;
}

Ewk_Custom_Handlers_Data* ewkCustomHandlersCreateData(const char* target, const char* baseUrl, const char* url)
{
    Ewk_Custom_Handlers_Data* customHandlersData = new Ewk_Custom_Handlers_Data;
    customHandlersData->target = eina_stringshare_add(target);
    customHandlersData->base_url = eina_stringshare_add(baseUrl);
    customHandlersData->url = eina_stringshare_add(url);
    customHandlersData->title = 0;

    return customHandlersData;
}

void ewkCustomHandlersDeleteData(Ewk_Custom_Handlers_Data* customHandlersData)
{
    eina_stringshare_del(customHandlersData->target);
    eina_stringshare_del(customHandlersData->base_url);
    eina_stringshare_del(customHandlersData->url);
    if(customHandlersData->title)
        eina_stringshare_del(customHandlersData->title);
    delete customHandlersData;
}
#endif // #if (ENABLE_TIZEN_REGISTER_PROTOCOL_HANDLER) || (ENABLE_TIZEN_REGISTER_CONTENT_HANDLER) || (ENABLE_TIZEN_CUSTOM_SCHEME_HANDLER)
