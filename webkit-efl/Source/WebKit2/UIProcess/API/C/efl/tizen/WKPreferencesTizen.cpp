/*
 * Copyright (C) 2011 Samsung Electronics
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
#include "WKPreferencesTizen.h"

#include "WKAPICast.h"
#include "WebPreferences.h"

using namespace WebKit;

void WKPreferencesSetDefaultViewLevel(WKPreferencesRef preferencesRef, double level)
{
#if ENABLE(TIZEN_PREFERENCE)
    toImpl(preferencesRef)->setDefaultViewLevel(level);
#endif
}

double WKPreferencesGetDefaultViewLevel(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_PREFERENCE)
    return toImpl(preferencesRef)->defaultViewLevel();
#else
    return 0;
#endif
}

void WKPreferencesSetUsesEncodingDetector(WKPreferencesRef preferencesRef, bool use)
{
#if ENABLE(TIZEN_PREFERENCE)
    toImpl(preferencesRef)->setUsesEncodingDetector(use);
#endif
}

bool WKPreferencesGetUsesEncodingDetector(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_PREFERENCE)
    return toImpl(preferencesRef)->usesEncodingDetector();
#else
    return 0;
#endif
}

void WKPreferencesSetLoadRemoteImages(WKPreferencesRef preferencesRef, bool loadRemoteImages)
{
#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
    toImpl(preferencesRef)->setLoadRemoteImages(loadRemoteImages);
#endif
}

bool WKPreferencesGetLoadRemoteImages(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
    return toImpl(preferencesRef)->loadRemoteImages();
#else
    return 0;
#endif
}

void WKPreferencesSetEnableDefaultKeypad(WKPreferencesRef preferencesRef, bool enable)
{
#if ENABLE(TIZEN_ISF_PORT)
    toImpl(preferencesRef)->setEnableDefaultKeypad(enable);
#endif
}

bool WKPreferencesGetDefaultKeypadEnabled(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_ISF_PORT)
    return toImpl(preferencesRef)->defaultKeypadEnabled();
#else
    return 0;
#endif
}

void WKPreferencesSetTextZoomEnabled(WKPreferencesRef preferencesRef, bool enable)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_ZOOM)
    toImpl(preferencesRef)->setTextZoomEnabled(enable);
#endif
}

bool WKPreferencesGetTextZoomEnabled(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_ZOOM)
    return toImpl(preferencesRef)->textZoomEnabled();
#else
    return false;
#endif
}

void WKPreferencesSetStyleScopedEnabled(WKPreferencesRef preferencesRef, bool enable)
{
#if ENABLE(TIZEN_STYLE_SCOPED)
    toImpl(preferencesRef)->setStyleScopedEnabled(enable);
#else
    UNUSED_PARAM(preferencesRef);
    UNUSED_PARAM(enable);
#endif
}

bool WKPreferencesGetStyleScopedEnabled(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_STYLE_SCOEPD)
    return toImpl(preferencesRef)->styleScopedEnabled();
#else
    UNUSED_PARAM(preferencesRef);
    return false;
#endif
}

void WKPreferencesSetLinkMagnifierEnabled(WKPreferencesRef preferencesRef, bool enable)
{
#if ENABLE(TIZEN_LINK_MAGNIFIER)
    toImpl(preferencesRef)->setLinkMagnifierEnabled(enable);
#else
    UNUSED_PARAM(preferencesRef);
    UNUSED_PARAM(enable);
#endif
}

bool WKPreferencesGetLinkMagnifierEnabled(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_LINK_MAGNIFIER)
    return toImpl(preferencesRef)->linkMagnifierEnabled();
#else
    UNUSED_PARAM(preferencesRef);
    return false;
#endif
}

void WKPreferencesSetPasteImageUriEnabled(WKPreferencesRef preferencesRef, bool pasteImageUriEnabled)
{
#if ENABLE(TIZEN_PASTE_IMAGE_URI)
    toImpl(preferencesRef)->setPasteImageUriEnabled(pasteImageUriEnabled);
#else
    UNUSED_PARAM(preferencesRef);
    UNUSED_PARAM(pasteImageUriEnabled);
#endif
}

bool WKPreferencesGetPasteImageUriEnabled(WKPreferencesRef preferencesRef)
{
#if ENABLE(TIZEN_PASTE_IMAGE_URI)
    return toImpl(preferencesRef)->pasteImageUriEnabled();
#else
    UNUSED_PARAM(preferencesRef);
    return false;
#endif
}
