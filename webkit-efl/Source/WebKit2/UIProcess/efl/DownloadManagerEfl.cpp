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
#include "DownloadManagerEfl.h"

#include "DownloadProxy.h"
#include "EwkViewImpl.h"
#include "WKContext.h"
#include "WKString.h"
#include "ewk_context_private.h"
#include "ewk_error_private.h"

#if PLATFORM(TIZEN) && ENABLE(TIZEN_USING_EXTERNAL_DOWNLOAD_CLIENT)
#include "WKDownload.h"
#include "WKRetainPtr.h"
#endif

namespace WebKit {

static inline DownloadManagerEfl* toDownloadManagerEfl(const void* clientInfo)
{
    return static_cast<DownloadManagerEfl*>(const_cast<void*>(clientInfo));
}

#if PLATFORM(TIZEN) && ENABLE(TIZEN_USING_EXTERNAL_DOWNLOAD_CLIENT)
// FIMXE : This macro should be unified or modified according to Tizen macro policy.
// WebKit opensource does not have any concept for external download module, but, Tizen needs it.
// So, localizing this patch is helpless situation unless WebKit supports the concept or we contribute it.
// If policy for Tizen macro and ewk local API are determined, this patch should be took care of according to them.
void DownloadManagerEfl::didStart(WKContextRef, WKDownloadRef wkDownload, const void* clientInfo)
{
    WebCore::ResourceRequest request = toImpl(wkDownload)->request();
    WKRetainPtr<WKStringRef> downloadUrlString = adoptWK(WKStringCreateWithUTF8CString(request.url().string().utf8().data()));

    toDownloadManagerEfl(clientInfo)->m_context->didStartDownload(downloadUrlString.get());
}
#else
WKStringRef DownloadManagerEfl::decideDestinationWithSuggestedFilename(WKContextRef, WKDownloadRef wkDownload, WKStringRef filename, bool* /*allowOverwrite*/, const void* clientInfo)
{
    Ewk_Download_Job* download = toDownloadManagerEfl(clientInfo)->downloadJob(toImpl(wkDownload)->downloadID());
    ASSERT(download);

    download->setSuggestedFileName(toImpl(filename)->string().utf8().data());

    // We send the new download signal on the Ewk_View only once we have received the response
    // and the suggested file name.
    download->viewImpl()->informDownloadJobRequested(download);

    // DownloadSoup expects the destination to be a URL.
    String destination = ASCIILiteral("file://") + String::fromUTF8(download->destination());

    return WKStringCreateWithUTF8CString(destination.utf8().data());
}

void DownloadManagerEfl::didReceiveResponse(WKContextRef, WKDownloadRef wkDownload, WKURLResponseRef wkResponse, const void* clientInfo)
{
    Ewk_Download_Job* download = toDownloadManagerEfl(clientInfo)->downloadJob(toImpl(wkDownload)->downloadID());
    ASSERT(download);
    download->setResponse(Ewk_Url_Response::create(wkResponse));
}

void DownloadManagerEfl::didCreateDestination(WKContextRef, WKDownloadRef wkDownload, WKStringRef /*path*/, const void* clientInfo)
{
    Ewk_Download_Job* download = toDownloadManagerEfl(clientInfo)->downloadJob(toImpl(wkDownload)->downloadID());
    ASSERT(download);

    download->setState(EWK_DOWNLOAD_JOB_STATE_DOWNLOADING);
}

void DownloadManagerEfl::didReceiveData(WKContextRef, WKDownloadRef wkDownload, uint64_t length, const void* clientInfo)
{
    Ewk_Download_Job* download = toDownloadManagerEfl(clientInfo)->downloadJob(toImpl(wkDownload)->downloadID());
    ASSERT(download);
    download->incrementReceivedData(length);
}

void DownloadManagerEfl::didFail(WKContextRef, WKDownloadRef wkDownload, WKErrorRef error, const void* clientInfo)
{
    DownloadManagerEfl* downloadManager = toDownloadManagerEfl(clientInfo);
    uint64_t downloadId = toImpl(wkDownload)->downloadID();
    Ewk_Download_Job* download = downloadManager->downloadJob(downloadId);
    ASSERT(download);

    OwnPtr<Ewk_Error> ewkError = Ewk_Error::create(error);
    download->setState(EWK_DOWNLOAD_JOB_STATE_FAILED);
    download->viewImpl()->informDownloadJobFailed(download, ewkError.get());
    downloadManager->unregisterDownloadJob(downloadId);
}

void DownloadManagerEfl::didCancel(WKContextRef, WKDownloadRef wkDownload, const void* clientInfo)
{
    DownloadManagerEfl* downloadManager = toDownloadManagerEfl(clientInfo);
    uint64_t downloadId = toImpl(wkDownload)->downloadID();
    Ewk_Download_Job* download = downloadManager->downloadJob(downloadId);
    ASSERT(download);

    download->setState(EWK_DOWNLOAD_JOB_STATE_CANCELLED);
    download->viewImpl()->informDownloadJobCancelled(download);
    downloadManager->unregisterDownloadJob(downloadId);
}

void DownloadManagerEfl::didFinish(WKContextRef, WKDownloadRef wkDownload, const void* clientInfo)
{
    DownloadManagerEfl* downloadManager = toDownloadManagerEfl(clientInfo);
    uint64_t downloadId = toImpl(wkDownload)->downloadID();
    Ewk_Download_Job* download = downloadManager->downloadJob(downloadId);
    ASSERT(download);

    download->setState(EWK_DOWNLOAD_JOB_STATE_FINISHED);
    download->viewImpl()->informDownloadJobFinished(download);
    downloadManager->unregisterDownloadJob(downloadId);
}
#endif // PLATFORM(TIZEN) && ENABLE(TIZEN_USING_EXTERNAL_DOWNLOAD_CLIENT)

DownloadManagerEfl::DownloadManagerEfl(Ewk_Context* context)
    : m_context(context)
{
    WKContextDownloadClient wkDownloadClient;
    memset(&wkDownloadClient, 0, sizeof(WKContextDownloadClient));

    wkDownloadClient.version = kWKContextDownloadClientCurrentVersion;
    wkDownloadClient.clientInfo = this;
#if PLATFORM(TIZEN) && ENABLE(TIZEN_USING_EXTERNAL_DOWNLOAD_CLIENT)
    wkDownloadClient.didStart = didStart;
#else
    wkDownloadClient.didCancel = didCancel;
    wkDownloadClient.decideDestinationWithSuggestedFilename = decideDestinationWithSuggestedFilename;
    wkDownloadClient.didCreateDestination = didCreateDestination;
    wkDownloadClient.didReceiveResponse = didReceiveResponse;
    wkDownloadClient.didReceiveData = didReceiveData;
    wkDownloadClient.didFail = didFail;
    wkDownloadClient.didFinish = didFinish;
#endif

    WKContextSetDownloadClient(context->wkContext(), &wkDownloadClient);
}

void DownloadManagerEfl::registerDownload(DownloadProxy* download, EwkViewImpl* viewImpl)
{
    uint64_t downloadId = download->downloadID();
    if (m_downloadJobs.contains(downloadId))
        return;

    RefPtr<Ewk_Download_Job> ewkDownload = Ewk_Download_Job::create(download, viewImpl);
    m_downloadJobs.add(downloadId, ewkDownload);
}

Ewk_Download_Job* DownloadManagerEfl::downloadJob(uint64_t id) const
{
    return m_downloadJobs.get(id).get();
}

void DownloadManagerEfl::unregisterDownloadJob(uint64_t id)
{
    m_downloadJobs.remove(id);
}

} // namespace WebKit
