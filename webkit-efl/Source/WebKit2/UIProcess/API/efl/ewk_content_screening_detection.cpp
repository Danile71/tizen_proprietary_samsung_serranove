/*
   Copyright (C) 2013 Samsung Electronics

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
#include "ewk_view_private.h"
#include "ewk_content_screening_detection.h"

#include "WKAPICast.h"
#include "WKAuthenticationChallenge.h"
#include "WKAuthenticationDecisionListener.h"
#include "WKCredential.h"
#include "WKEinaSharedString.h"
#include "WKError.h"
#include "WKProtectionSpace.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include "ewk_auth_challenge_private.h"
#include <wtf/text/CString.h>

using namespace WebKit;

#include "ewk_context.h"
#if USE(SOUP)
#include "ResourceHandle.h"
#include <libsoup/soup.h>
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)

struct _Ewk_Content_Screening_Detection {
    WKPageRef page;
    int level;
    WKEinaSharedString name;
    WKEinaSharedString url;
    bool isDecided;
    bool isSuspended;
};

Ewk_Content_Screening_Detection* ewkContentScreeningDetectionCreate(WKPageRef page, WKErrorRef error)
{
    Ewk_Content_Screening_Detection* contentScreeningDetection = new Ewk_Content_Screening_Detection;

    contentScreeningDetection->page = page;
    contentScreeningDetection->level = WKErrorGetErrorCode(error);
    contentScreeningDetection->name = WKEinaSharedString(AdoptWK, WKErrorCopyLocalizedDescription(error));
    contentScreeningDetection->url = WKEinaSharedString(AdoptWK, WKErrorCopyFailingURL(error));
    contentScreeningDetection->isDecided = false;
    contentScreeningDetection->isSuspended = false;

    return contentScreeningDetection;
}

void ewkContentScreeningDetectionDelete(Ewk_Content_Screening_Detection* contentScreeningDetection)
{
    EINA_SAFETY_ON_NULL_RETURN(contentScreeningDetection);

    delete contentScreeningDetection;
}

bool ewkContentScreeningDetectionSuspended(Ewk_Content_Screening_Detection* contentScreeningDetection)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(contentScreeningDetection, false);

    return contentScreeningDetection->isSuspended;
}

bool ewkContentScreeningDetectionDecided(Ewk_Content_Screening_Detection* contentScreeningDetection)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(contentScreeningDetection, false);

    return contentScreeningDetection->isDecided;
}

void ewk_content_screening_detection_confirmed_set(Ewk_Content_Screening_Detection* content_screening_detection, Eina_Bool confirmed)
{
    EINA_SAFETY_ON_NULL_RETURN(content_screening_detection);

    TIZEN_LOGE("void ewk_content_screening_detection_confirmed_set()");
    content_screening_detection->isDecided = true;
    toImpl(content_screening_detection->page)->replyBlockingToLoadForMalwareScan(confirmed);
}

void ewk_content_screening_detection_suspend(Ewk_Content_Screening_Detection* content_screening_detection)
{
    EINA_SAFETY_ON_NULL_RETURN(content_screening_detection);

    TIZEN_LOGE("void ewk_content_screening_detection_suspend()");
    content_screening_detection->isSuspended = true;
}

int ewk_content_screening_detection_level_get(Ewk_Content_Screening_Detection* content_screening_detection)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(content_screening_detection, 0);

    return content_screening_detection->level;
}

const char* ewk_content_screening_detection_name_get(Ewk_Content_Screening_Detection* content_screening_detection)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(content_screening_detection, NULL);

    return content_screening_detection->name;
}

const char* ewk_content_screening_detection_url_get(Ewk_Content_Screening_Detection* content_screening_detection)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(content_screening_detection, NULL);

    return content_screening_detection->url;
}
#endif
