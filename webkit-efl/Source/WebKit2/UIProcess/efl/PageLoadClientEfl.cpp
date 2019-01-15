/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "PageLoadClientEfl.h"

#include "EwkViewImpl.h"
#include "WKAPICast.h"
#include "WKFrame.h"
#include "WKPage.h"
#include "ewk_back_forward_list_private.h"
#include "ewk_error_private.h"
#include "ewk_intent_private.h"
#include "ewk_intent_service_private.h"
#include "ewk_view.h"

#if PLATFORM(TIZEN)
#include "WKRetainPtr.h"
#include "ewk_auth_challenge.h"
#include "ewk_auth_challenge_private.h"

#if ENABLE(TIZEN_LINK_MAGNIFIER)
#include "LinkMagnifierProxy.h"
#endif
#endif

using namespace WebKit;

namespace WebKit {

static inline PageLoadClientEfl* toPageLoadClientEfl(const void* clientInfo)
{
    return static_cast<PageLoadClientEfl*>(const_cast<void*>(clientInfo));
}

void PageLoadClientEfl::didReceiveTitleForFrame(WKPageRef, WKStringRef title, WKFrameRef frame, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    viewImpl->informTitleChange(toImpl(title)->string());
}

#if ENABLE(WEB_INTENTS)
void PageLoadClientEfl::didReceiveIntentForFrame(WKPageRef, WKFrameRef, WKIntentDataRef intent, WKTypeRef, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    RefPtr<Ewk_Intent> ewkIntent = Ewk_Intent::create(intent);
    viewImpl->informIntentRequest(ewkIntent.get());
}
#endif

#if ENABLE(WEB_INTENTS_TAG)
void PageLoadClientEfl::registerIntentServiceForFrame(WKPageRef, WKFrameRef, WKIntentServiceInfoRef serviceInfo, WKTypeRef, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    RefPtr<Ewk_Intent_Service> ewkIntentService = Ewk_Intent_Service::create(serviceInfo);
    viewImpl->informIntentServiceRegistration(ewkIntentService.get());
}
#endif

void PageLoadClientEfl::didChangeProgress(WKPageRef page, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    viewImpl->informLoadProgress(WKPageGetEstimatedProgress(page));
}

void PageLoadClientEfl::didFinishLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef /*userData*/, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    viewImpl->informLoadFinished();
}

#if ENABLE(TIZEN_CSS_THEME_COLOR)
void PageLoadClientEfl::didChangeThemeColor(WKPageRef, WKFrameRef frame, WKStringRef theme_color, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    WebCore::Color color = WebCore::Color(WebKit::toWTFString(theme_color));
    viewImpl->informThemeColor(color);
}
#endif

void PageLoadClientEfl::didFailLoadWithErrorForFrame(WKPageRef, WKFrameRef frame, WKErrorRef error, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
    {
        ewkViewLoadError(viewImpl->view(), error);
        return;
    }
#endif
    OwnPtr<Ewk_Error> ewkError = Ewk_Error::create(error);
    viewImpl->informLoadError(ewkError.get());
    viewImpl->informLoadFinished();
}

void PageLoadClientEfl::didStartProvisionalLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef /*userData*/, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    viewImpl->informProvisionalLoadStarted();
}

void PageLoadClientEfl::didReceiveServerRedirectForProvisionalLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef /*userData*/, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    viewImpl->informProvisionalLoadRedirect();
}

void PageLoadClientEfl::didFailProvisionalLoadWithErrorForFrame(WKPageRef, WKFrameRef frame, WKErrorRef error, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
#if ENABLE(TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR)
    {
        ewkViewLoadError(viewImpl->view(), error);
        return;
    }
#endif
    OwnPtr<Ewk_Error> ewkError = Ewk_Error::create(error);
    viewImpl->informProvisionalLoadFailed(ewkError.get());
}

void PageLoadClientEfl::didChangeBackForwardList(WKPageRef, WKBackForwardListItemRef addedItem, WKArrayRef removedItems, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    ASSERT(viewImpl);

    Ewk_Back_Forward_List* list = ewk_view_back_forward_list_get(viewImpl->view());
    ASSERT(list);
    list->update(addedItem, removedItems);

    viewImpl->informBackForwardListChange();

#if ENABLE(TIZEN_LINK_MAGNIFIER)
    LinkMagnifierProxy::linkMagnifier().didChangeBackForwardList(viewImpl->page());
#endif
}

void PageLoadClientEfl::didSameDocumentNavigationForFrame(WKPageRef, WKFrameRef frame, WKSameDocumentNavigationType, WKTypeRef, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    viewImpl->informURLChange();
#if PLATFORM(TIZEN)
    viewImpl->informTitleChange(viewImpl->page()->pageTitle());
#endif
}

#if PLATFORM(TIZEN)
void PageLoadClientEfl::didCommitLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->viewImpl()->view();
    ewkViewLoadCommitted(ewkView);
}

void PageLoadClientEfl::didFirstVisuallyNonEmptyLayoutForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    if (!WKFrameIsMainFrame(frame))
        return;

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->viewImpl()->view();
    ewkViewDidFirstVisuallyNonEmptyLayout(ewkView);
}

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
bool PageLoadClientEfl::didReceiveAuthenticationChallengeInFrame(WKPageRef page, WKFrameRef frame, WKAuthenticationChallengeRef authenticationChallenge, const void* clientInfo)
{
#if !ENABLE(TIZEN_AUTHENTICATION_CHALLENGE_ENABLED_IN_ALL_FRAMES)
    if (!WKFrameIsMainFrame(frame))
        return false;
#endif

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->viewImpl()->view();
    Ewk_Auth_Challenge* authChallenge = ewkAuthChallengeCreate(page, authenticationChallenge);
    ewkViewDidReceiveAuthenticationChallenge(ewkView, authChallenge);

    if (!ewkAuthChallengeDecided(authChallenge) && !ewkAuthChallengeSuspended(authChallenge))
        ewk_auth_challenge_credential_cancel(authChallenge);

    return true;
}
#else
void PageLoadClientEfl::didReceiveAuthenticationChallengeInFrame(WKPageRef page, WKFrameRef frame, WKAuthenticationChallengeRef authenticationChallenge, const void* clientInfo)
{
#if !ENABLE(TIZEN_AUTHENTICATION_CHALLENGE_ENABLED_IN_ALL_FRAMES)
    if (!WKFrameIsMainFrame(frame))
        return;
#endif

    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->viewImpl()->view();
    Ewk_Auth_Challenge* authChallenge = ewkAuthChallengeCreate(authenticationChallenge);
    ewkViewDidReceiveAuthenticationChallenge(ewkView, authChallenge);

    if (!ewkAuthChallengeDecided(authChallenge) && !ewkAuthChallengeSuspended(authChallenge))
        ewk_auth_challenge_credential_cancel(authChallenge);
}
#endif

void PageLoadClientEfl::processDidCrash(WKPageRef page, const void* clientInfo)
{
    Evas_Object* ewkView = toPageLoadClientEfl(clientInfo)->viewImpl()->view();
    ewk_view_process_crashed(ewkView);
}

#if ENABLE(TIZEN_WEBKIT2_SEPERATE_LOAD_PROGRESS)
void PageLoadClientEfl::didStartProgress(WKPageRef page, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    viewImpl->informLoadProgressStarted();
}

void PageLoadClientEfl::didFinishProgress(WKPageRef page, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    viewImpl->informLoadProgressFinished();
}
#endif

#if ENABLE(TIZEN_ISF_PORT)
void PageLoadClientEfl::willGoToBackForwardListItem(WKPageRef page, WKBackForwardListItemRef, WKTypeRef, const void* clientInfo)
{
    EwkViewImpl* viewImpl = toPageLoadClientEfl(clientInfo)->viewImpl();
    InputMethodContextEfl* inputMethodContext = viewImpl->inputMethodContext();
    if (inputMethodContext)
        inputMethodContext->hideIMFContext();
}
#endif
#endif // #if PLATFORM(TIZEN)

PageLoadClientEfl::PageLoadClientEfl(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
{
    WKPageRef pageRef = m_viewImpl->wkPage();
    ASSERT(pageRef);

    WKPageLoaderClient loadClient;
    memset(&loadClient, 0, sizeof(WKPageLoaderClient));
    loadClient.version = kWKPageLoaderClientCurrentVersion;
    loadClient.clientInfo = this;
    loadClient.didReceiveTitleForFrame = didReceiveTitleForFrame;
#if ENABLE(WEB_INTENTS)
    loadClient.didReceiveIntentForFrame = didReceiveIntentForFrame;
#endif
#if ENABLE(WEB_INTENTS_TAG)
    loadClient.registerIntentServiceForFrame = registerIntentServiceForFrame;
#endif
#if ENABLE(TIZEN_WEBKIT2_SEPERATE_LOAD_PROGRESS)
    loadClient.didStartProgress = didStartProgress;
#else
    loadClient.didStartProgress = didChangeProgress;
#endif
    loadClient.didChangeProgress = didChangeProgress;
#if ENABLE(TIZEN_WEBKIT2_SEPERATE_LOAD_PROGRESS)
    loadClient.didFinishProgress = didFinishProgress;
#else
    loadClient.didFinishProgress = didChangeProgress;
#endif
    loadClient.didFinishLoadForFrame = didFinishLoadForFrame;
#if ENABLE(TIZEN_CSS_THEME_COLOR)
    loadClient.didChangeThemeColor = didChangeThemeColor;
#endif
    loadClient.didFailLoadWithErrorForFrame = didFailLoadWithErrorForFrame;
    loadClient.didStartProvisionalLoadForFrame = didStartProvisionalLoadForFrame;
    loadClient.didReceiveServerRedirectForProvisionalLoadForFrame = didReceiveServerRedirectForProvisionalLoadForFrame;
    loadClient.didFailProvisionalLoadWithErrorForFrame = didFailProvisionalLoadWithErrorForFrame;
    loadClient.didChangeBackForwardList = didChangeBackForwardList;
    loadClient.didSameDocumentNavigationForFrame = didSameDocumentNavigationForFrame;
#if PLATFORM(TIZEN)
    loadClient.didCommitLoadForFrame = didCommitLoadForFrame;
    loadClient.didFirstVisuallyNonEmptyLayoutForFrame = didFirstVisuallyNonEmptyLayoutForFrame;
    loadClient.didReceiveAuthenticationChallengeInFrame = didReceiveAuthenticationChallengeInFrame;
    loadClient.processDidCrash = processDidCrash;
#if ENABLE(TIZEN_ISF_PORT)
    loadClient.willGoToBackForwardListItem = willGoToBackForwardListItem;
#endif
#endif
    WKPageSetPageLoaderClient(pageRef, &loadClient);
}

} // namespace WebKit
