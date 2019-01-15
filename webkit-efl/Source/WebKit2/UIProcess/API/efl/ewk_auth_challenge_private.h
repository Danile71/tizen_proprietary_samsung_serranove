/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef ewk_auth_challenge_private_h
#define ewk_auth_challenge_private_h

#include "WKAuthenticationChallenge.h"
#include "ewk_auth_challenge.h"

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
Ewk_Auth_Challenge* ewkAuthChallengeCreate(WKPageRef page, WKAuthenticationChallengeRef authenticationChallenge);
#else
Ewk_Auth_Challenge* ewkAuthChallengeCreate(WKAuthenticationChallengeRef authenticationChallenge);
#endif
void ewkAuthChallengeDelete(Ewk_Auth_Challenge* authChallenge);
bool ewkAuthChallengeDecided(Ewk_Auth_Challenge* authChallenge);
bool ewkAuthChallengeSuspended(Ewk_Auth_Challenge* authChallenge);

#endif // ewk_auth_challenge_private_h
