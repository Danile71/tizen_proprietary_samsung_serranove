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
#include "ewk_auth_challenge.h"

#if PLATFORM(TIZEN)
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

/**
 * @brief  Structure used to send credential for authentication challenge.
 *
 * Details If Authentication challenge requirement is received, AuthenticationChallenge is created,
 * and realm, host url are received from server.
 * These information are sent to notify by evas_object_smart_callback_call.
 */
struct _Ewk_Auth_Challenge {
    WKAuthenticationChallengeRef authenticationChallenge;
#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    WKPageRef page;
#endif

    CString realm;
    CString url;

    bool isDecided;
    bool isSuspended;
};

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
Ewk_Auth_Challenge* ewkAuthChallengeCreate(WKPageRef page, WKAuthenticationChallengeRef authenticationChallenge)
#else
Ewk_Auth_Challenge* ewkAuthChallengeCreate(WKAuthenticationChallengeRef authenticationChallenge)
#endif
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(authenticationChallenge, 0);

    WKProtectionSpaceRef protectionSpace = WKAuthenticationChallengeGetProtectionSpace(authenticationChallenge);
    EINA_SAFETY_ON_NULL_RETURN_VAL(protectionSpace, 0);

    WKRetainPtr<WKStringRef> hostString(AdoptWK, WKProtectionSpaceCopyHost(protectionSpace));
    WKRetainPtr<WKStringRef> realmString(AdoptWK, WKProtectionSpaceCopyRealm(protectionSpace));

    Ewk_Auth_Challenge* authChallenge = new Ewk_Auth_Challenge;

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
    authChallenge->page = page;
#endif
    authChallenge->authenticationChallenge = authenticationChallenge;
    authChallenge->realm = toImpl(realmString.get())->string().utf8();
    authChallenge->url = toImpl(hostString.get())->string().utf8();

    authChallenge->isDecided = false;
    authChallenge->isSuspended = false;

    return authChallenge;
}

void ewkAuthChallengeDelete(Ewk_Auth_Challenge* authChallenge)
{
    EINA_SAFETY_ON_NULL_RETURN(authChallenge);

    delete authChallenge;
}

bool ewkAuthChallengeDecided(Ewk_Auth_Challenge* authChallenge)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(authChallenge, false);

    return authChallenge->isDecided;
}

bool ewkAuthChallengeSuspended(Ewk_Auth_Challenge* authChallenge)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(authChallenge, false);

    return authChallenge->isSuspended;
}

const char* ewk_auth_challenge_realm_get(Ewk_Auth_Challenge* authChallenge)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(authChallenge, 0);

    return authChallenge->realm.data();
}

const char* ewk_auth_challenge_url_get(Ewk_Auth_Challenge* authChallenge)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(authChallenge, 0);

    return authChallenge->url.data();
}

void ewk_auth_challenge_suspend(Ewk_Auth_Challenge* authChallenge)
{
    EINA_SAFETY_ON_NULL_RETURN(authChallenge);

    authChallenge->isSuspended = true;
}

void ewk_auth_challenge_credential_use(Ewk_Auth_Challenge* authChallenge, char* user, char* password)
{
    EINA_SAFETY_ON_NULL_RETURN(authChallenge);
    EINA_SAFETY_ON_NULL_RETURN(user);
    EINA_SAFETY_ON_NULL_RETURN(password);

    authChallenge->isDecided = true;
    toImpl(authChallenge->page)->replyReceiveAuthenticationChallengeInFrame(true);

    WKAuthenticationChallengeRef authenticationChallenge = authChallenge->authenticationChallenge;
    WKAuthenticationDecisionListenerRef authenticationDecisionListener = WKAuthenticationChallengeGetDecisionListener(authenticationChallenge);

    WKRetainPtr<WKStringRef> userString(AdoptWK, WKStringCreateWithUTF8CString(user));
    WKRetainPtr<WKStringRef> passwordString(AdoptWK, WKStringCreateWithUTF8CString(password));

    WKRetainPtr<WKCredentialRef> credential(AdoptWK, WKCredentialCreate(userString.get(), passwordString.get(), kWKCredentialPersistenceNone));

    WKAuthenticationDecisionListenerUseCredential(authenticationDecisionListener, credential.get());
}

void ewk_auth_challenge_credential_cancel(Ewk_Auth_Challenge* authChallenge)
{
    EINA_SAFETY_ON_NULL_RETURN(authChallenge);

    authChallenge->isDecided = true;
    toImpl(authChallenge->page)->replyReceiveAuthenticationChallengeInFrame(false);

    WKAuthenticationChallengeRef authenticationChallenge = authChallenge->authenticationChallenge;
    WKAuthenticationDecisionListenerRef authenticationDecisionListener = WKAuthenticationChallengeGetDecisionListener(authenticationChallenge);

    WKAuthenticationDecisionListenerCancel(authenticationDecisionListener);
}

#endif // #if PLATFORM(TIZEN)
