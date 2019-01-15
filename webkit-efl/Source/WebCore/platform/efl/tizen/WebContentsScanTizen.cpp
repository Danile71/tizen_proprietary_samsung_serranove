/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
#include "WebContentsScanTizen.h"

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)

#include <wtf/StdLibExtras.h>
#include <string.h>
#include "SharedBuffer.h"

namespace WebCore {

struct ScanContext
{
    char* data;
    long long size;
};

static long long getSizeCallback(void* priv)
{
    ScanContext* tscTx;
    tscTx = static_cast<ScanContext*>(priv);
    return tscTx->size;
}

static int setSizeCallback(void* priv, long long size)
{
    // FIXME: Implementation is needed when TCSScanParam.iAction is TCS_SA_SCANREPAIR
    return 0;
}

static unsigned int readCallback(void* priv, long long offset, void* buffer, unsigned int count)
{
    long long readByte = 0;
    ScanContext* ctx = static_cast<ScanContext*>(priv);

    if (offset < ctx->size) {
        if ((readByte = ctx->size - offset) > static_cast<long long>(count))
            readByte = static_cast<long long>(count);
    }
    if (readByte)
        memcpy(buffer, ctx->data + offset, static_cast<size_t>(readByte));

    return static_cast<unsigned int>(readByte);
}

static unsigned int writeCallback(void* priv, long long offset, void const* buffer, unsigned int count)
{
    // FIXME: Implementation is needed when TCSScanParam.iAction is TCS_SA_SCANREPAIR
    return 0;
}

static int notifierCallback(void* priv, int reason, void* param)
{
    // This is used to notify caller for specific events via this callback function
    return 0;
}

WebContentsScanTizen::WebContentsScanTizen()
{
    m_handle = TCSLibraryOpen();
}

WebContentsScanTizen::~WebContentsScanTizen()
{
    close();
}

WebContentsScanTizen& WebContentsScanTizen::shared()
{
    DEFINE_STATIC_LOCAL(WebContentsScanTizen, webContentsScanTizen, ());
    return webContentsScanTizen;
}

int WebContentsScanTizen::detectMalware(int mimeType, PassRefPtr<SharedBuffer> resourceData, String& malwareName)
{
    int errorLevel = -1;

    if (m_handle == INVALID_TCSLIB_HANDLE)
        return errorLevel;

    ScanContext tscTx;
    TCSScanResult result;
    TCSScanParam param;

    tscTx.data = const_cast<char*>(resourceData->data());
    tscTx.size = resourceData->size();

    param.iAction = TCS_SA_SCANONLY;
    param.iCompressFlag = 1;
    param.iDataType = mimeType;
    param.pPrivate = static_cast<void*>(&tscTx);
    param.pfGetSize = getSizeCallback;
    param.pfSetSize = setSizeCallback;
    param.pfRead = readCallback;
    param.pfWrite = writeCallback;
    param.pfCallBack = notifierCallback;

    int scanSuccess = TCSScanData(m_handle, &param, &result);
    if(!scanSuccess && result.iNumDetected) {
        malwareName = String(result.pDList->pszName);
        errorLevel = ((result.pDList->uAction >> 8) & 255);
        result.pfFreeResult(&result);
        return errorLevel;
    }

    return errorLevel;
}

void WebContentsScanTizen::close()
{
    TCSLibraryClose(m_handle);
    m_handle = INVALID_TCSLIB_HANDLE;
}

} // namespace WebCore
#endif // ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)

