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
#include "FormClientEfl.h"

#include "EwkViewImpl.h"
#include "WKPage.h"
#include "ewk_form_submission_request_private.h"

namespace WebKit {

static inline FormClientEfl* toFormClientEfl(const void* clientInfo)
{
    return static_cast<FormClientEfl*>(const_cast<void*>(clientInfo));
}

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void FormClientEfl::willSendSubmitEvent(WKPageRef, WKFrameRef, WKFrameRef sourceFrame, WKFormDataRef formDataRef, const void* clientInfo)
{
    FormClientEfl* formClient = toFormClientEfl(clientInfo);

    if (formClient->m_timer) {
        ecore_timer_del(formClient->m_timer);
        formClient->m_timer = 0;
    }

    formClient->m_formData.clear();
    formClient->m_formData = WebFormData::create(toImpl(formDataRef)->data());

    formClient->scheduleFormDataSubmitted();
}

void FormClientEfl::willSubmitForm(WKPageRef, WKFrameRef, WKFrameRef sourceFrame, WKFormDataRef formDataRef, WKTypeRef, WKFormSubmissionListenerRef listener, const void* clientInfo)
{
    FormClientEfl* formClient = toFormClientEfl(clientInfo);

    if (formClient->m_timer) {
        ecore_timer_del(formClient->m_timer);
        formClient->m_timer = 0;
    } else {
        formClient->m_formData.clear();
        formClient->m_formData = WebFormData::create(toImpl(formDataRef)->data());
    }

    formClient->formDataSubmitted(formClient);

    WKFormSubmissionListenerContinue(listener);
}

Eina_Bool FormClientEfl::formDataSubmitted(void *data)
{
    if (!data)
        return ECORE_CALLBACK_CANCEL;

    FormClientEfl* object = (FormClientEfl*)data;
    object->m_timer = 0;

    ewk_view_form_data_add(object->m_viewImpl->view(), toAPI(object->m_formData.get()));

    return ECORE_CALLBACK_CANCEL;
}

void FormClientEfl::scheduleFormDataSubmitted()
{
    if (m_timer) {
        ecore_timer_del(m_timer);
        m_timer = 0;
    }

    m_timer = ecore_timer_add(1.0, formDataSubmitted, this);
}

FormClientEfl::~FormClientEfl()
{
    if (m_timer) {
        ecore_timer_del(m_timer);
        m_timer = 0;
    }
}
#else
void FormClientEfl::willSubmitForm(WKPageRef, WKFrameRef /*frame*/, WKFrameRef /*sourceFrame*/, WKDictionaryRef values, WKTypeRef /*userData*/, WKFormSubmissionListenerRef listener, const void* clientInfo)
{
    FormClientEfl* formClient = toFormClientEfl(clientInfo);

    RefPtr<Ewk_Form_Submission_Request> request = Ewk_Form_Submission_Request::create(values, listener);
    formClient->m_viewImpl->informNewFormSubmissionRequest(request.get());
}
#endif

FormClientEfl::FormClientEfl(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    , m_timer(0)
#endif
{
    WKPageRef pageRef = m_viewImpl->wkPage();
    ASSERT(pageRef);

    WKPageFormClient formClient;
    memset(&formClient, 0, sizeof(WKPageFormClient));
    formClient.version = kWKPageFormClientCurrentVersion;
    formClient.clientInfo = this;
#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
    formClient.willSendSubmitEvent = willSendSubmitEvent;
#endif
    formClient.willSubmitForm = willSubmitForm;
    WKPageSetPageFormClient(pageRef, &formClient);
}

} // namespace WebKit
