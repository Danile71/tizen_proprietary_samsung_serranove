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

/**
 * @file    ewk_autofill_profile_private.h
 * @brief   Describes the Ewk autofill profile APIs.
 */

#ifndef ewk_autofill_profile_private_h
#define ewk_autofill_profile_private_h

using namespace WebKit;

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
struct _Ewk_Autofill_Profile {
    unsigned profileID;
    Vector <std::pair<Ewk_Autofill_Profile_Data_Type, WKEinaSharedString> > nameValuePair;
};

const char* autofillProfileNameAttributes[] = {"id", "name", "company", "address1", "address2", "city", "state", "zipcode", "country", "phone", "email"};

#endif//TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM

#endif // ewk_autofill_profile_private_h

