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
#include "ewk_view_private.h"
#include "ewk_certificate.h"

#include "WKAPICast.h"
#include "WKAuthenticationChallenge.h"
#include "WKAuthenticationDecisionListener.h"
#include "WKCredential.h"
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

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)

struct _Ewk_Certificate_Policy_Decision {
    WKPageRef page;
    const char* url;
    const char* certificatePem;
    int error;
    bool isDecided;
    bool isSuspended;
};

Ewk_Certificate_Policy_Decision* ewkCertificatePolicyDecisionCreate(WKPageRef page, WKStringRef url, WKStringRef certificate, int error)
{
    Ewk_Certificate_Policy_Decision* certificatePolicyDecision = new Ewk_Certificate_Policy_Decision;

    certificatePolicyDecision->page = page;
    certificatePolicyDecision->url = eina_stringshare_add(toImpl(url)->string().utf8().data());
    certificatePolicyDecision->certificatePem = eina_stringshare_add(toImpl(certificate)->string().utf8().data());
    certificatePolicyDecision->error = error;
    certificatePolicyDecision->isDecided = false;
    certificatePolicyDecision->isSuspended = false;

    return certificatePolicyDecision;
}

void ewkCertificatePolicyDecisionDelete(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN(certificatePolicyDecision);

    if (certificatePolicyDecision->url)
        eina_stringshare_del(certificatePolicyDecision->url);
    if (certificatePolicyDecision->certificatePem)
        eina_stringshare_del(certificatePolicyDecision->certificatePem);

    delete certificatePolicyDecision;
}

bool ewkCertificatePolicyDecisionSuspended(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, false);
    return certificatePolicyDecision->isSuspended;
}

bool ewkCertificatePolicyDecisionDecided(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, false);
    return certificatePolicyDecision->isDecided;
}

void ewk_certificate_policy_decision_allowed_set(Ewk_Certificate_Policy_Decision* certificatePolicyDecision, Eina_Bool allowed)
{
    EINA_SAFETY_ON_NULL_RETURN(certificatePolicyDecision);

    certificatePolicyDecision->isDecided = true;
    toImpl(certificatePolicyDecision->page)->replyPolicyForCertificateError(allowed);
}

void ewk_certificate_policy_decision_suspend(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN(certificatePolicyDecision);

    certificatePolicyDecision->isSuspended = true;
}

const char* ewk_certificate_policy_decision_url_get(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, 0);

    return certificatePolicyDecision->url;
}

const char* ewk_certificate_policy_decision_certificate_pem_get(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, 0);

    return certificatePolicyDecision->certificatePem;
}

int ewk_certificate_policy_decision_error_get(Ewk_Certificate_Policy_Decision* certificatePolicyDecision)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(certificatePolicyDecision, 0);

    return certificatePolicyDecision->error;
}

#endif
