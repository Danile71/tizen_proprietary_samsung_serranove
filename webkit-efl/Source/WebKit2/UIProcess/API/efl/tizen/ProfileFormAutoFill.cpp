/*
 * Copyright (C) 2013 Samsung Electronics All rights reserved.
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
#include "ProfileFormAutoFill.h"
#include "ewk_view.h"

#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)

using namespace WebCore;

namespace WebKit {

ProfileFormAutoFill::ProfileFormAutoFill(RefPtr<WebKit::FormDatabase> formDataBase)
{
    m_formDatabase = formDataBase;
    updateProfileFormData();
}

ProfileFormAutoFill::~ProfileFormAutoFill()
{
}

void ProfileFormAutoFill::updateProfileFormData()
{
    // Update the Map only if new details are updated.
    if(!m_formDatabase->isProfileFormDataRecordUpdated())
       return;

    m_nameRecordMap.clear(); // clearing old data before updating with new
    m_profileID.clear(); // clearing old profile data before updating
    m_formDatabase->getAllProfileIDForProfileFormData (m_profileID);
    if (m_profileID.isEmpty())
        return;

    HashMap<String, HashSet<std::pair<String, String> > >  formAutoFillData;
    m_formDatabase->getProfileFormDataMap(formAutoFillData);

    if (formAutoFillData.isEmpty())
        return;

    for (size_t listItem = 0; listItem != m_profileID.size(); ++listItem) {
        HashSet<std::pair<String, String> > textFormData = formAutoFillData.get(m_profileID[listItem]);
        if (!textFormData.size())
            continue;

        HashSet<std::pair<String, String> >::iterator end = textFormData.end();
        for (HashSet<std::pair<String, String> >::iterator it = textFormData.begin(); it != end; ++it) {
            HashSet<std::pair<String, String> > data_map;
            data_map = m_nameRecordMap.get(it->first);
            data_map.add(std::pair<String, String>(m_profileID[listItem], it->second));
            m_nameRecordMap.set(it->first, data_map);
        }
    }
    m_formDatabase->setProfileFormDataRecordUpdated(false);
}

static bool getLastNameSupport(const String& attrname)
{
    const String lastName[] = {"lastname", "lname", "LastName", "lastName", "lName", "last_name"};
    const String partialLastName[] = {"Lname","lastName"};

    // check for lastname
    for (size_t i=0; i < sizeof(lastName)/sizeof(*lastName); ++i) {
        if (lastName[i] == attrname)
            return true;
    }
    //check for partial lastname
    for (size_t i=0; i < sizeof(partialLastName)/sizeof(*partialLastName); ++i) {
        if (attrname.contains(partialLastName[i], false))
            return true;
    }
    return false;
}

static String getNameFromSupportedNames(const String& attrname)
{
    const String firstName[] = {"name", "firstName", "FirstName", "firstname", "fName", "fname", "linkManName", "first_name", "rb_ps_name"};
    const String partialFirstName[] = {"Fname", "firstName"};
    const String company[] = {"company", "companyname"};
    const String city[] = {"city"};
    const String zipcode[] = {"zipcode", "zip", "pin", "pincode"};
    const String phone[] = {"phone", "cellno", "RecoveryPhoneNumber", "gsmNumber", "mobile", "tel", "Phone", "mobile_number", "mobile[connection]", "mobilenumber"};
    const String email[] = {"email", "emailid", "RecoveryEmailAddress", "reg_email__", "mailAddress", "session[username_or_email]", "usr_email", "emailAdd", "emailAddress", "user[email]", "username_or_email", "EmailID"};
    // phone exception list to avoid showing suggestion for invalid name attributes.
    const String phoneException[] = {"mobile[amount]"};

    // This below fields do not have multiple details so no check is added for these field.
    // Future if there are mutliple values for these fields then add the check same as below
    // String address1("address1");
    // String address2("address2");
    // String city("city");
    // String state("state");
    // String country("country");

    // check for name
    for (size_t i=0; i < sizeof(firstName)/sizeof(*firstName); ++i) {
        if (firstName[i] == attrname)
            return firstName[0];
    }
    //check for partial firstname
    for (size_t i=0; i < sizeof(partialFirstName)/sizeof(*partialFirstName); ++i) {
        if (attrname.contains(partialFirstName[i], false))
            return firstName[0];
    }
    // check for company
    for (size_t i=0; i < sizeof(company)/sizeof(*company); ++i) {
        if ((company[i] == attrname) || (company[i].contains(attrname, false)) || (attrname.contains(company[i], false)))
            return company[0];
    }
    // check for city
    for (size_t i=0; i < sizeof(city)/sizeof(*city); ++i) {
        if ((city[i] == attrname) || (city[i].contains(attrname, false)) || (attrname.contains(city[i], false)))
            return city[0];
    }
    // check for zipcode
    for (size_t i=0; i < sizeof(zipcode)/sizeof(*zipcode); ++i) {
        if ((zipcode[i] == attrname) || (zipcode[i].contains(attrname, false)) || (attrname.contains(zipcode[i], false)))
            return zipcode[0];
    }
    // check for phone number
    for (size_t i=0; i < sizeof(phone)/sizeof(*phone); ++i) {
        if ((phone[i] == attrname) || (phone[i].contains(attrname, false)) || (attrname.contains(phone[i], false))) {
            for (size_t i=0; i < sizeof(phoneException)/sizeof(*phoneException); ++i) {
                if ((phoneException[i] == attrname))
                    return "";
            }
            return phone[0];
        }
    }
    // check for email
    for (size_t i=0; i < sizeof(email)/sizeof(*email); ++i) {
        if ((email[i] == attrname) || (attrname.contains(email[i], true)))
            return email[0];
    }
    return ""; // do not emit anything what we didn't match earlier
}

void ProfileFormAutoFill::getProfileFormCandidateData(const String& name, const String& value, Vector<std::pair<int, std::pair<String, String> > > & autoFillTextData)
{
    updateProfileFormData();
    if (name.isEmpty())
        return;

    String queryName = getNameFromSupportedNames(name);
    bool isLastName = getLastNameSupport(name);

    if (isLastName)
        queryName = "name";

    HashSet<std::pair<String, String> > candidateSet = m_nameRecordMap.get(queryName);
    if (!candidateSet.size())
        return;

    String auxStringName;
    HashSet<std::pair<String, String> >::iterator end = candidateSet.end();
    for (HashSet<std::pair<String, String> >::iterator it = candidateSet.begin(); it != end; ++it) {
        if (isLastName) {
            String mainString = it->second;
            String auxStringValue;
            std::pair<String, String> auxStringPair(auxStringName,auxStringValue);
            m_formDatabase->getAuxStringForDisplay(it->first, mainString, auxStringPair);
            auxStringName = auxStringPair.first;
            auxStringValue = auxStringPair.second;
            Vector<String> splitnames;
            mainString.split(" ",splitnames);
            int splitsize = splitnames.size();
            if (splitsize == 1)
                continue;
            if (splitsize)
                mainString = splitnames[splitsize-1];
            if (mainString.startsWith(value, false)) {
                if (auxStringValue.isEmpty())
                    auxStringValue = it->second;
                std::pair<String, String> addPair(mainString,auxStringValue);
                autoFillTextData.append(std::pair<int, std::pair<String, String> >(atoi(it->first.utf8().data()), addPair));
                continue;
            }
        }
        if (value.isEmpty()) {
            String mainString = it->second;
            String auxStringValue;
            std::pair<String, String> auxStringPair(auxStringName,auxStringValue);
            m_formDatabase->getAuxStringForDisplay(it->first, mainString, auxStringPair);
            auxStringName = auxStringPair.first;
            auxStringValue = auxStringPair.second;
            if (auxStringValue.isEmpty())
                auxStringValue = mainString;
            std::pair<String, String> addPair(mainString,auxStringValue);
            autoFillTextData.append(std::pair<int, std::pair<String, String> >(atoi(it->first.utf8().data()), addPair));
            continue;
        }
        if (!isLastName && it->second.startsWith(value, false)) {
            String mainString = it->second;
            String auxStringValue;
            std::pair<String, String> auxStringPair(auxStringName,auxStringValue);
            m_formDatabase->getAuxStringForDisplay(it->first, mainString, auxStringPair);
            auxStringName = auxStringPair.first;
            auxStringValue = auxStringPair.second;
            if (auxStringValue.isEmpty())
                auxStringValue = mainString;
            std::pair<String, String> addPair(mainString,auxStringValue);
            autoFillTextData.append(std::pair<int, std::pair<String, String> >(atoi(it->first.utf8().data()), addPair));
            continue;
        }
    }
}

void ProfileFormAutoFill::setProfileFormCanditateToWebForm(const int& profileID, String& fillScript)
{
    NumberToStringBuffer buffer;
    String profileIDString = numberToString(profileID, buffer);
    if (profileIDString.isEmpty())
       return;

    HashMap<String, HashSet<std::pair<String, String> > >  formAutoFillData;
    m_formDatabase->getProfileFormDataMap(formAutoFillData);

    if (formAutoFillData.isEmpty())
        return;

    HashSet<std::pair<String, String> > textFormData = formAutoFillData.get(profileIDString);
    if (textFormData.isEmpty())
        return;

    HashMap<String,String> finalData;
    HashSet<std::pair<String, String> >::iterator end = textFormData.end();
    for (HashSet<std::pair<String, String> >::iterator it = textFormData.begin(); it != end; ++it) {
        finalData.set(it->first,it->second);
    }
    Vector<String> splitnames;
    String fullname = finalData.get("name");
    fullname.split(" ",splitnames);

    int splitsize = splitnames.size();
    if (splitsize > 1) {
        finalData.set("name",splitnames[0]);
        finalData.set("lastname",splitnames[splitsize-1]);
    }

    fillScript = String::format("function bindEvent(el, eventName, eventHandler) {"
        "if (el.addEventListener){"
            "el.addEventListener(eventName, eventHandler, false);"
        "} else if (el.attachEvent){"
            "el.attachEvent('on'+eventName, eventHandler);"
        "}"
    "}"
    "function isAncestor(child, ancestor)"
    "{"
        "if(!child)"
            "return false;"
        "var elem = child;"
        "var retVal = false;"
        "while(elem.tagName !='BODY'){"
            "if(ancestor==elem.parentNode ){"
                "retVal = true;"
                "break;"
            "}"
            "elem = elem.parentNode;"
        "}"
        "return retVal;"
    "}"
    "function populate()"
    "{"
        "var name, lastname, company, address1, address2, city, state, zipcode, country, phone, email;"
        "var nameFinal=null, lastnameFinal=null, companyFinal=null, address1Final=null, address2Final=null, cityFinal=null, stateFinal=null, zipcodeFinal=null, countryFinal=null, phoneFinal=null, emailFinal=null;"
        "var proposedName=[], proposedPartialName=[], proposedLastName=[], proposedPartialLastName=[], proposedCompany=[], proposedAddress1=[], proposedAddress2=[], proposedCity=[], proposedState=[], proposedZipcode=[], proposedCountry=[], proposedPhone=[], proposedEmail=[];"
        "proposedName = ['name', 'firstName', 'FirstName', 'firstname', 'fName', 'fname', 'linkManName', 'first_name', 'rb_ps_name'];"
        "proposedPartialName = ['Fname', 'firstName', 'firstname'];"
        "proposedLastName = ['lastname', 'lname', 'LastName', 'lastName', 'lName', 'last_name'];"
        "proposedPartialLastName = ['Lname','lastName', 'lastname'];"
        "proposedCompany = ['company'];"
        "proposedAddress1 = ['address1'];"
        "proposedAddress2 = ['address2'];"
        "proposedCity = ['city'];"
        "proposedState = ['state'];"
        "proposedZipcode = ['zipcode', 'zip', 'pincode', 'pin'];"
        "proposedCountry = ['country'];"
        "proposedPhone = ['phone', 'cellno', 'RecoveryPhoneNumber', 'gsmNumber', 'mobile', 'tel', 'Phone', 'mobile_number', 'mobile[connection]', 'mobilenumber', 'PhoneNo', 'rb_phone'];"
        "proposedEmail = ['email', 'emailId', 'RecoveryEmailAddress', 'reg_email__', 'mailAddress', 'session[username_or_email]', 'usr_email', 'emailAdd', 'emailAddress', 'user[email]', 'username_or_email', 'EmailID', 'rb_email'];"
        "var finalisedAttributes = "
        "{"
            "name : false,"
            "lastname : false,"
            "company : false,"
            "address1 : false,"
            "address2 : false,"
            "city : false,"
            "state : false,"
            "zipcode : false,"
            "country : false,"
            "phone : false,"
            "email : false"
        "};"
        "var hashMap ="
        "{");
       const String mapString[] = {"name", "lastname", "company", "address1", "address2", "city", "state", "zipcode", "country", "phone", "email"};

        fillScript += String::format("%s : ",mapString[0].utf8().data());
        fillScript += "\'"+finalData.get(mapString[0]) + "\'";

        for (size_t ittr = 1; ittr != sizeof(mapString)/sizeof(*mapString); ++ittr) {
            fillScript += String::format(",%s : ",mapString[ittr].utf8().data());
            fillScript += "\'"+finalData.get(mapString[ittr]) + "\'";
        }
        fillScript +=  String::format("};"
        "var focusedElement = document.activeElement;"
        "var mainForm = focusedElement;"
        "while(mainForm && mainForm.tagName!=='FORM'){"
            "mainForm = mainForm.parentNode;"
        "}"
        "if(mainForm == null)"
            "return;"
        "for(var count=0; count<proposedName.length; count++){"
            "name = document.getElementsByName(proposedName[count]);"
            "for(var t=0; t<name.length; t++){"
                "if(name[t].getAttribute('type') == 'text' && isAncestor(name[t], mainForm)){"
                   "nameFinal = name[t];"
                   "finalisedAttributes['name'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['name'])"
                "break;"
        "}"
        "for(var count=0; count<proposedLastName.length; count++){"
            "lastname = document.getElementsByName(proposedLastName[count]);"
            "for(var t=0; t<lastname.length; t++){"
                "if(lastname[t].getAttribute('type') == 'text' && isAncestor(lastname[t], mainForm)){"
                   "lastnameFinal = lastname[t];"
                   "finalisedAttributes['lastname'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['lastname'])"
                "break;"
        "}"
        "if(!finalisedAttributes['name']){"
            "name = document.getElementsByTagName('input');"
            "var t = 0, countFN, countLN;"
            "var nodeName = focusedElement.getAttribute('name');"
            "for(countFN=0; countFN<proposedPartialName.length; countFN++){"
                "if((nodeName != null && nodeName.indexOf(proposedPartialName[countFN]) != -1) && isAncestor(focusedElement, mainForm)){"
                    "finalisedAttributes['name'] = true;"
                    "break;"
                "}"
            "}"
            "for(var countLN=0; countLN<proposedPartialLastName.length; countLN++){"
                "if((nodeName != null && nodeName.indexOf(proposedPartialLastName[countLN]) != -1) && isAncestor(focusedElement, mainForm)){"
                    "finalisedAttributes['lastname'] = true;"
                    "break;"
                "}"
            "}"
            "if(finalisedAttributes['name']) {"
                "while(focusedElement != name[t]) t++;"
                "for(var i=t; i<name.length; i++){"
                    "var nodeName = name[i].getAttribute('name');"
                    "if((nodeName != null && nodeName.indexOf(proposedPartialLastName[countFN]) != -1) && isAncestor(name[i], mainForm)){"
                        "finalisedAttributes['lastname'] = true;"
                        "lastnameFinal = name[i];"
                        "nameFinal = focusedElement;"
                        "break;"
                    "}"
                "}"
            "}"
            "if(finalisedAttributes['lastname']) {"
                "while(focusedElement != name[t]) t++;"
                "for(var i=t-1; i>=0; i--){"
                    "var nodeName = name[i].getAttribute('name');"
                    "if((nodeName != null && nodeName.indexOf(proposedPartialName[countLN]) != -1) && isAncestor(name[i], mainForm)){"
                        "finalisedAttributes['name'] = true;"
                        "lastnameFinal = focusedElement;"
                        "nameFinal = name[i];"
                        "break;"
                    "}"
                "}"
            "}"
        "}"
        "for(count=0; count<proposedCompany.length; count++){"
            "company = document.getElementsByName(proposedCompany[count]);"
            "for(var t=0; t<company.length; t++){"
                "if(isAncestor(company[t], mainForm)){"
                   "companyFinal = company[t];"
                   "finalisedAttributes['company'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['company'])"
                "break;"
        "}"
        "for(count=0; count<proposedAddress1.length; count++){"
            "address1 = document.getElementsByName(proposedAddress1[count]);"
            "for(var t=0; t<address1.length; t++){"
                "if(isAncestor(address1[t], mainForm)){"
                   "address1Final = address1[t];"
                   "finalisedAttributes['address1'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['address1'])"
                "break;"
        "}"
        "for(count=0; count<proposedAddress2.length; count++){"
            "address2 = document.getElementsByName(proposedAddress2[count]);"
            "for(var t=0; t<address2.length; t++){"
                "if(isAncestor(address2[t], mainForm)){"
                   "address2Final = address2[t];"
                   "finalisedAttributes['address2'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['address2'])"
                "break;"
        "}"
        "for(count=0; count<proposedCity.length; count++){"
            "city = document.getElementsByName(proposedCity[count]);"
            "for(var t=0; t<city.length; t++){"
                "if(isAncestor(city[t], mainForm)){"
                   "cityFinal = city[t];"
                   "finalisedAttributes['city'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['city'])"
                "break;"
        "}"
        "for(count=0; count<proposedState.length; count++){"
            "state = document.getElementsByName(proposedState[count]);"
            "for(var t=0; t<state.length; t++){"
                "if(isAncestor(state[t], mainForm)){"
                   "stateFinal = state[t];"
                   "finalisedAttributes['state'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['state'])"
                "break;"
        "}"
        "for(count=0; count<proposedZipcode.length; count++){"
            "zipcode = document.getElementsByName(proposedZipcode[count]);"
            "for(var t=0; t<zipcode.length; t++){"
                "if(isAncestor(zipcode[t], mainForm)){"
                   "zipcodeFinal = zipcode[t];"
                   "finalisedAttributes['zipcode'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['zipcode'])"
                "break;"
        "}"
        "for(count=0; count<proposedCountry.length; count++){"
            "country = document.getElementsByName(proposedCountry[count]);"
            "for(var t=0; t<country.length; t++){"
                "if(isAncestor(country[t], mainForm)){"
                   "countryFinal = country[t];"
                   "finalisedAttributes['country'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['country'])"
                "break;"
        "}"
        "for(count=0; count<proposedPhone.length; count++){"
            "phone = document.getElementsByName(proposedPhone[count]);"
            "for(var t=0; t<phone.length; t++){"
                "if(isAncestor(phone[t], mainForm)){"
                   "phoneFinal = phone[t];"
                   "finalisedAttributes['phone'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['phone'])"
                "break;"
        "}"
        "for(count=0; count<proposedEmail.length; count++){"
            "email = document.getElementsByName(proposedEmail[count]);"
            "for(var t=0; t<email.length; t++){"
                "if(isAncestor(email[t], mainForm)){"
                   "emailFinal = email[t];"
                   "finalisedAttributes['email'] = true;"
                   "break;"
                "}"
            "}"
            "if(finalisedAttributes['email'])"
                "break;"
        "}"
        "var elemArray = [];"
        "elemArray = [nameFinal, lastnameFinal, companyFinal, address1Final, address2Final, cityFinal, stateFinal, zipcodeFinal, countryFinal, phoneFinal, emailFinal];"
        "var i=0;"
        "for(var key in hashMap){"
            "if(elemArray[i]!=null){"
                "elemArray[i].value = hashMap[key];"
                "if(!elemArray[i].isValueAdded){"
                    "elemArray[i].isValueAdded = elemArray[i].style.backgroundColor;"
                    "bindEvent(elemArray[i], 'input',"
                                    "function () {"
                                        "if(this.isValueAdded !== undefined){"
                                            "this.style.backgroundColor = this.isValueAdded;"
                                            "this.isValueAdded = null;"
                                            "this.removeEventListener('input',arguments.callee,false);"
                                        "}"
                                    "});"
                    "var inputEvent = new Event('input'); elemArray[i].dispatchEvent(inputEvent);"
                    "var changeEvent = new Event('change'); elemArray[i].dispatchEvent(changeEvent);"
                "}"
                "if(elemArray[i].value) {"
                     "elemArray[i].style.backgroundColor = '#faffbd';"
                     "elemArray[i].style.color = '#000000';"
                "}"
            "}"
            "i++;"
        "}"
        "i=0;"
    "}"
    "populate();");
}

} //namespace WebKit

#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
