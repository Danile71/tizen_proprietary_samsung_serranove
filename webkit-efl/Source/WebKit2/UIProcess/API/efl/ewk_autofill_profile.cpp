/*
 * Copyright (C) 2013 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ewk_autofill_profile.h"
#include "ewk_autofill_profile_private.h"
#include <wtf/dtoa.h>
#include <Eina.h>

using namespace WebKit;

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
Ewk_Autofill_Profile* ewk_autofill_profile_new()
{
    Ewk_Autofill_Profile* localProfile = new Ewk_Autofill_Profile;
    if (!localProfile)
        return NULL;

    localProfile->profileID = 0;
    return localProfile;
}

void ewk_autofill_profile_delete(Ewk_Autofill_Profile* profile)
{
    EINA_SAFETY_ON_NULL_RETURN(profile);
    delete profile;
}

void ewk_autofill_profile_data_set(Ewk_Autofill_Profile* profile, Ewk_Autofill_Profile_Data_Type name, const char* value)
{
    EINA_SAFETY_ON_NULL_RETURN(profile);

    if (EWK_PROFILE_ID == name)
        return;

    WKEinaSharedString attributeValue(value);
    size_t size = profile->nameValuePair.size();
    for (size_t i = 0; i < size; i++) {
        if (name == profile->nameValuePair[i].first) {
            profile->nameValuePair.remove(i);
            break;
        }
    }
    profile->nameValuePair.append(std::pair<Ewk_Autofill_Profile_Data_Type, WKEinaSharedString>(name, attributeValue));
}

unsigned ewk_autofill_profile_id_get(Ewk_Autofill_Profile* profile)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(profile, 0);
    return profile->profileID;
}

const char* ewk_autofill_profile_data_get(Ewk_Autofill_Profile* profile, Ewk_Autofill_Profile_Data_Type name)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(profile, NULL);

    size_t size = profile->nameValuePair.size();
    for (size_t i = 0; i < size; i++) {
        if (name == profile->nameValuePair[i].first)
            return profile->nameValuePair[i].second;
    }
    return NULL;
}

#endif //TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM

