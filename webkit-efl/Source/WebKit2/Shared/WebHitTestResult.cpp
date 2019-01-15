/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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
#include "WebHitTestResult.h"

#include "WebCoreArgumentCoders.h"

#include <WebCore/KURL.h>
#include <wtf/text/WTFString.h>

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#include <wtf/text/StringHash.h>
#endif

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebHitTestResult> WebHitTestResult::create(const WebHitTestResult::Data& hitTestResultData)
{
    return adoptRef(new WebHitTestResult(hitTestResultData));
}

void WebHitTestResult::Data::encode(CoreIPC::ArgumentEncoder* encoder) const
{
    encoder->encode(absoluteImageURL);
    encoder->encode(absolutePDFURL);
    encoder->encode(absoluteLinkURL);
    encoder->encode(absoluteMediaURL);
    encoder->encode(linkLabel);
    encoder->encode(linkTitle);
    encoder->encode(isContentEditable);

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    encoder->encode(context);
    encoder->encode(hitTestMode);
#if ENABLE(TIZEN_DRAG_SUPPORT)
    encoder->encode(isDragSupport);
#endif
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    encoder->encode(focusedRects);
    encoder->encode(focusedColor);
    encoder->encode(nodeData.tagName);
#endif
#if ENABLE(TIZEN_CHILD_NODE_IMAGE_URL)
    encoder->encode(absoluteChildImageURL);
#endif

    if (hitTestMode & HitTestModeNodeData) {
        encoder->encode(nodeData.tagName);
        encoder->encode(nodeData.nodeValue);
        encoder->encode(nodeData.attributeMap);
    }

    if ((hitTestMode & HitTestModeImageData) && (context & HitTestResultContextImage)) {
        encoder->encode(CoreIPC::DataReference(imageData.data));
        encoder->encode(imageData.fileNameExtension);
    }
#endif
}

bool WebHitTestResult::Data::decode(CoreIPC::ArgumentDecoder* decoder, WebHitTestResult::Data& hitTestResultData)
{
    if (!decoder->decode(hitTestResultData.absoluteImageURL)
        || !decoder->decode(hitTestResultData.absolutePDFURL)
        || !decoder->decode(hitTestResultData.absoluteLinkURL)
        || !decoder->decode(hitTestResultData.absoluteMediaURL)
        || !decoder->decode(hitTestResultData.linkLabel)
        || !decoder->decode(hitTestResultData.linkTitle)
        || !decoder->decode(hitTestResultData.isContentEditable))
        return false;

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
    if (!decoder->decode(hitTestResultData.context))
        return false;

    if (!decoder->decode(hitTestResultData.hitTestMode))
        return false;

#if ENABLE(TIZEN_DRAG_SUPPORT)
    if (!decoder->decode(hitTestResultData.isDragSupport))
        return false;
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    if (!decoder->decode(hitTestResultData.focusedRects))
        return false;

    if (!decoder->decode(hitTestResultData.focusedColor))
        return false;

    if (!decoder->decode(hitTestResultData.nodeData.tagName))
        return false;
#endif

#if ENABLE(TIZEN_CHILD_NODE_IMAGE_URL)
    if (!decoder->decode(hitTestResultData.absoluteChildImageURL))
        return false;
#endif

    if (hitTestResultData.hitTestMode & HitTestModeNodeData) {
        if (!decoder->decode(hitTestResultData.nodeData.tagName))
            return false;

        if (!decoder->decode(hitTestResultData.nodeData.nodeValue))
            return false;

        if (!decoder->decode(hitTestResultData.nodeData.attributeMap))
            return false;
    }

    if ((hitTestResultData.hitTestMode & HitTestModeImageData) && (hitTestResultData.context & HitTestResultContextImage)) {
        CoreIPC::DataReference data;
        if (!decoder->decode(data))
            return false;
        hitTestResultData.imageData.data.append(data.data(), data.size());

        if (!decoder->decode(hitTestResultData.imageData.fileNameExtension))
            return false;
    }
#endif

    return true;
}

} // WebKit
