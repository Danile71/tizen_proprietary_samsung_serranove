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

#include "config.h"
#include "ewk_context_injected_bundle_client.h"

#include "WKContext.h"
#include "ewk_context_private.h"
#include "ewk_view_private.h"

void didReceiveMessageFromInjectedBundle(WKContextRef page, WKStringRef messageName, WKTypeRef messageBody, const void* clientInfo)
{
    Ewk_Context* ewkContext = static_cast<Ewk_Context*>(const_cast<void*>(clientInfo));
    ewkContext->didReceiveMessageFromInjectedBundle(messageName, messageBody, 0);
}

void didReceiveSynchronousMessageFromInjectedBundle(WKContextRef page, WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData, const void* clientInfo)
{
    Ewk_Context* ewkContext = static_cast<Ewk_Context*>(const_cast<void*>(clientInfo));
    ewkContext->didReceiveMessageFromInjectedBundle(messageName, messageBody, returnData);
}

void ewkContextInjectedBundleClientAttachClient(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    TIZEN_LOGI("EwkContext(%p) attach InjectedBundleClient", ewkContext);

    WKContextInjectedBundleClient injectedBundleClient = {
        kWKContextInjectedBundleClientCurrentVersion,
        ewkContext,
        didReceiveMessageFromInjectedBundle,
        didReceiveSynchronousMessageFromInjectedBundle,
        0 /* WKContextGetInjectedBundleInitializationUserDataCallback */
    };

    WKContextSetInjectedBundleClient(ewkContext->wkContext(), &injectedBundleClient);
}

#if ENABLE(TIZEN_INJECTED_BUNDLE_CRASH_WORKAROUND)
void ewkContextInjectedBundleClientDetachClient(Ewk_Context* ewkContext)
{
    EINA_SAFETY_ON_NULL_RETURN(ewkContext);

    TIZEN_LOGI("EwkContext(%p) detach InjectedBundleClient", ewkContext);

    WKContextInjectedBundleClient injectedBundleClient = {
        kWKContextInjectedBundleClientCurrentVersion,
        0,
        0,
        0,
        0 /* WKContextGetInjectedBundleInitializationUserDataCallback */
    };

    WKContextSetInjectedBundleClient(ewkContext->wkContext(), &injectedBundleClient);
}
#endif
