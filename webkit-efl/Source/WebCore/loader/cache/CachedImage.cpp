/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

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
#include "CachedImage.h"

#include "BitmapImage.h"
#include "MemoryCache.h"
#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceLoader.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderTypes.h"
#include "FrameView.h"
#include "Page.h"
#include "RenderObject.h"
#include "Settings.h"
#include "SharedBuffer.h"
#include "SubresourceLoader.h"
#include <wtf/CurrentTime.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

#if USE(CG)
#include "PDFDocumentImage.h"
#endif

#if ENABLE(SVG)
#include "SVGImage.h"
#endif

#if ENABLE(TIZEN_JPEGIMAGE_DECODING_THREAD)
#include "ImageDecoder.h"
#endif

using std::max;

namespace WebCore {

CachedImage::CachedImage(const ResourceRequest& resourceRequest)
    : CachedResource(resourceRequest, ImageResource)
    , m_image(0)
    , m_shouldPaintBrokenImage(true)
#if ENABLE(TIZEN_PREVENT_INCREMENTAL_IMAGE_RENDERING_WORKAROUND)
    , m_sizeUpdated(false)
#endif
{
    setStatus(Unknown);
}

CachedImage::CachedImage(Image* image)
    : CachedResource(ResourceRequest(), ImageResource)
    , m_image(image)
    , m_shouldPaintBrokenImage(true)
#if ENABLE(TIZEN_PREVENT_INCREMENTAL_IMAGE_RENDERING_WORKAROUND)
    , m_sizeUpdated(false)
#endif
{
    setStatus(Cached);
    setLoading(false);
}

CachedImage::~CachedImage()
{
    //TIZEN_LOGI("~:[%p]", this);
    clearImage();
}

void CachedImage::load(CachedResourceLoader* cachedResourceLoader, const ResourceLoaderOptions& options)
{
#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
    if (!cachedResourceLoader || (cachedResourceLoader->autoLoadImages() && cachedResourceLoader->loadRemoteImages(m_resourceRequest.url())))
#else
    if (!cachedResourceLoader || cachedResourceLoader->autoLoadImages())
#endif
        CachedResource::load(cachedResourceLoader, options);
    else
        setLoading(false);
}

void CachedImage::didAddClient(CachedResourceClient* c)
{
    if (m_data && !m_image && !errorOccurred()) {
        createImage();
        m_image->setData(m_data, true);
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
        if (m_image->data())
            m_image->data()->enableCache(true);
#endif
    }
    
    ASSERT(c->resourceClientType() == CachedImageClient::expectedType());
    if (m_image && !m_image->isNull())
        static_cast<CachedImageClient*>(c)->imageChanged(this);

    CachedResource::didAddClient(c);
}

void CachedImage::didRemoveClient(CachedResourceClient* c)
{
    ASSERT(c);
    ASSERT(c->resourceClientType() == CachedImageClient::expectedType());

    m_pendingContainerSizeRequests.remove(static_cast<CachedImageClient*>(c));
#if ENABLE(SVG)
    if (m_svgImageCache)
        m_svgImageCache->removeClientFromCache(static_cast<CachedImageClient*>(c));
#endif

    CachedResource::didRemoveClient(c);
}

void CachedImage::allClientsRemoved()
{
    m_pendingContainerSizeRequests.clear();
    if (m_image && !errorOccurred())
        m_image->resetAnimation();
}

pair<Image*, float> CachedImage::brokenImage(float deviceScaleFactor) const
{
    if (deviceScaleFactor >= 2) {
        DEFINE_STATIC_LOCAL(Image*, brokenImageHiRes, (Image::loadPlatformResource("missingImage@2x").leakRef()));
        return std::make_pair(brokenImageHiRes, 2);
    }

    DEFINE_STATIC_LOCAL(Image*, brokenImageLoRes, (Image::loadPlatformResource("missingImage").leakRef()));
    return std::make_pair(brokenImageLoRes, 1);
}

bool CachedImage::willPaintBrokenImage() const
{
    return errorOccurred() && m_shouldPaintBrokenImage;
}

#if ENABLE(SVG)
inline Image* CachedImage::lookupOrCreateImageForRenderer(const RenderObject* renderer)
{
    if (!m_image)
        return 0;
    if (!m_image->isSVGImage())
        return m_image.get();
    Image* useImage = m_svgImageCache->imageForRenderer(renderer);
    if (useImage == Image::nullImage())
        return m_image.get();
    return useImage;
}
#else
inline Image* CachedImage::lookupOrCreateImageForRenderer(const RenderObject*)
{
    return m_image.get();
}
#endif

Image* CachedImage::image()
{
    ASSERT(!isPurgeable());

    if (errorOccurred() && m_shouldPaintBrokenImage) {
        // Returning the 1x broken image is non-ideal, but we cannot reliably access the appropriate
        // deviceScaleFactor from here. It is critical that callers use CachedImage::brokenImage() 
        // when they need the real, deviceScaleFactor-appropriate broken image icon. 
        return brokenImage(1).first;
    }

    if (m_image)
        return m_image.get();

    return Image::nullImage();
}

#if ENABLE(TIZEN_DISPLAY_MISSING_IMAGES)
Image* CachedImage::imageForRenderer(const RenderObject* renderer, bool autoLoadImages)
#else
Image* CachedImage::imageForRenderer(const RenderObject* renderer)
#endif // ENABLE(TIZEN_DISPLAY_MISSING_IMAGES)
{
    ASSERT(!isPurgeable());

#if ENABLE(TIZEN_DISPLAY_MISSING_IMAGES)
    if ((!autoLoadImages || errorOccurred()) && m_shouldPaintBrokenImage) {
#else
    if (errorOccurred() && m_shouldPaintBrokenImage) {
#endif // ENABLE(TIZEN_DISPLAY_MISSING_IMAGES)
        // Returning the 1x broken image is non-ideal, but we cannot reliably access the appropriate
        // deviceScaleFactor from here. It is critical that callers use CachedImage::brokenImage() 
        // when they need the real, deviceScaleFactor-appropriate broken image icon. 
        return brokenImage(1).first;
    }

    if (m_image)
        return lookupOrCreateImageForRenderer(renderer);

    return Image::nullImage();
}

void CachedImage::setContainerSizeForRenderer(const CachedImageClient* renderer, const LayoutSize& containerSize, float containerZoom)
{
    if (containerSize.isEmpty())
        return;
    ASSERT(renderer);
    ASSERT(containerZoom);
    if (!m_image) {
        m_pendingContainerSizeRequests.set(renderer, SizeAndZoom(containerSize, containerZoom));
        return;
    }
#if ENABLE(SVG)
    if (!m_image->isSVGImage()) {
        m_image->setContainerSize(containerSize);
        return;
    }

    m_svgImageCache->setContainerSizeForRenderer(renderer, containerSize, containerZoom);
#else
    UNUSED_PARAM(containerZoom);
    m_image->setContainerSize(containerSize);
#endif
}

bool CachedImage::usesImageContainerSize() const
{
    if (m_image)
        return m_image->usesContainerSize();

    return false;
}

bool CachedImage::imageHasRelativeWidth() const
{
    if (m_image)
        return m_image->hasRelativeWidth();

    return false;
}

bool CachedImage::imageHasRelativeHeight() const
{
    if (m_image)
        return m_image->hasRelativeHeight();

    return false;
}

LayoutSize CachedImage::imageSizeForRenderer(const RenderObject* renderer, float multiplier)
{
    ASSERT(!isPurgeable());

    if (!m_image)
        return LayoutSize();

    LayoutSize imageSize;

    if (m_image->isBitmapImage() && (renderer && renderer->shouldRespectImageOrientation() == RespectImageOrientation))
        imageSize = static_cast<BitmapImage*>(m_image.get())->sizeRespectingOrientation();
#if ENABLE(SVG)
    else if (m_image->isSVGImage()) {
        imageSize = LayoutSize(m_svgImageCache->imageSizeForRenderer(renderer));
    }
#endif
    else
        imageSize = LayoutSize(m_image->size());

    if (multiplier == 1.0f)
        return imageSize;
        
    // Don't let images that have a width/height >= 1 shrink below 1 when zoomed.
    float widthScale = m_image->hasRelativeWidth() ? 1.0f : multiplier;
    float heightScale = m_image->hasRelativeHeight() ? 1.0f : multiplier;
    LayoutSize minimumSize(imageSize.width() > 0 ? 1 : 0, imageSize.height() > 0 ? 1 : 0);
    imageSize.scale(widthScale, heightScale);
    imageSize.clampToMinimumSize(minimumSize);
    return imageSize;
}

void CachedImage::computeIntrinsicDimensions(Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    if (m_image)
        m_image->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
}

void CachedImage::notifyObservers(const IntRect* changeRect)
{
    CachedResourceClientWalker<CachedImageClient> w(m_clients);
    while (CachedImageClient* c = w.next())
        c->imageChanged(this, changeRect);
}

void CachedImage::checkShouldPaintBrokenImage()
{
    if (!m_loader || m_loader->reachedTerminalState())
        return;

    m_shouldPaintBrokenImage = m_loader->frameLoader()->client()->shouldPaintBrokenImage(m_resourceRequest.url());
}

void CachedImage::clear()
{
    destroyDecodedData();
    clearImage();
    m_pendingContainerSizeRequests.clear();
    setEncodedSize(0);
}

inline void CachedImage::createImage()
{
    // Create the image if it doesn't yet exist.
    if (m_image)
        return;
#if USE(CG) && !USE(WEBKIT_IMAGE_DECODERS)
    else if (m_response.mimeType() == "application/pdf")
        m_image = PDFDocumentImage::create();
#endif
#if ENABLE(SVG)
    else if (m_response.mimeType() == "image/svg+xml") {
        RefPtr<SVGImage> svgImage = SVGImage::create(this);
        m_svgImageCache = SVGImageCache::create(svgImage.get());
        m_image = svgImage.release();
    }
#endif
    else
        m_image = BitmapImage::create(this);

    // Send queued container size requests.
    if (m_image && m_image->usesContainerSize()) {
        for (ContainerSizeRequests::iterator it = m_pendingContainerSizeRequests.begin(); it != m_pendingContainerSizeRequests.end(); ++it)
            setContainerSizeForRenderer(it->first, it->second.first, it->second.second);
        m_pendingContainerSizeRequests.clear();
    }
}

inline void CachedImage::clearImage()
{
    // If our Image has an observer, it's always us so we need to clear the back pointer
    // before dropping our reference.
    if (m_image)
        m_image->setImageObserver(0);
    m_image.clear();
}

size_t CachedImage::maximumDecodedImageSize()
{
    if (!m_loader || m_loader->reachedTerminalState())
        return 0;
    Settings* settings = m_loader->frameLoader()->frame()->settings();
    return settings ? settings->maximumDecodedImageSize() : 0;
}

void CachedImage::data(PassRefPtr<SharedBuffer> data, bool allDataReceived)
{
    m_data = data;

    createImage();

    bool sizeAvailable = false;

    // Have the image update its data from its internal buffer.
    // It will not do anything now, but will delay decoding until 
    // queried for info (like size or specific image frames).
    sizeAvailable = m_image->setData(m_data, allDataReceived);
#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
    if (m_image->data())
        m_image->data()->enableCache(true);
#endif

    // Go ahead and tell our observers to try to draw if we have either
    // received all the data or the size is known.  Each chunk from the
    // network causes observers to repaint, which will force that chunk
    // to decode.
#if ENABLE(TIZEN_PREVENT_INCREMENTAL_IMAGE_RENDERING_WORKAROUND)
    if ((sizeAvailable && !m_sizeUpdated) || allDataReceived) {
#else
    if (sizeAvailable || allDataReceived) {
#endif
        size_t maxDecodedImageSize = maximumDecodedImageSize();
        FloatSize s = m_image->size();
        size_t estimatedDecodedImageSize = s.width() * s.height() * 4; // no overflow check
        if (m_image->isNull() || (maxDecodedImageSize > 0 && estimatedDecodedImageSize > maxDecodedImageSize)) {
            error(errorOccurred() ? status() : DecodeError);
            if (inCache())
                memoryCache()->remove(this);
            return;
        }
        
#if ENABLE(TIZEN_JPEGIMAGE_DECODING_THREAD)
        if (allDataReceived && m_image->getBitmapimageSource() && m_image->getBitmapimageSource()->decoder()->jpegImage())
            AsyncImageDecoder::sharedAsyncImageDecoder().decodeAsync(m_image);
#endif

        // It would be nice to only redraw the decoded band of the image, but with the current design
        // (decoding delayed until painting) that seems hard.
        notifyObservers();

        if (m_image)
            setEncodedSize(m_image->data() ? m_image->data()->size() : 0);

#if ENABLE(TIZEN_PREVENT_INCREMENTAL_IMAGE_RENDERING_WORKAROUND)
        m_sizeUpdated = true;
#endif
    }
    
    if (allDataReceived) {
        setLoading(false);
        checkNotify();
    }
}

void CachedImage::error(CachedResource::Status status)
{
    checkShouldPaintBrokenImage();
    clear();
    setStatus(status);
    ASSERT(errorOccurred());
    m_data.clear();
    notifyObservers();
    setLoading(false);
    checkNotify();
}

void CachedImage::setResponse(const ResourceResponse& response)
{
    if (!m_response.isNull())
        clear();
    CachedResource::setResponse(response);
}

void CachedImage::destroyDecodedData()
{
    bool canDeleteImage = !m_image || (m_image->hasOneRef() && m_image->isBitmapImage());
    if (isSafeToMakePurgeable() && canDeleteImage && !isLoading()) {
        // Image refs the data buffer so we should not make it purgeable while the image is alive. 
        // Invoking addClient() will reconstruct the image object.
        m_image = 0;
        setDecodedSize(0);
        if (!MemoryCache::shouldMakeResourcePurgeableOnEviction())
            makePurgeable(true);
    } else if (m_image && !errorOccurred())
        m_image->destroyDecodedData();
}

void CachedImage::decodedSizeChanged(const Image* image, int delta)
{
    if (!image || image != m_image)
        return;

    setDecodedSize(decodedSize() + delta);
}

void CachedImage::didDraw(const Image* image)
{
    if (!image || image != m_image)
        return;
    
    double timeStamp = FrameView::currentPaintTimeStamp();
    if (!timeStamp) // If didDraw is called outside of a Frame paint.
        timeStamp = currentTime();
    
    CachedResource::didAccessDecodedData(timeStamp);
}

bool CachedImage::shouldPauseAnimation(const Image* image)
{
    if (!image || image != m_image)
        return false;
    
    CachedResourceClientWalker<CachedImageClient> w(m_clients);
    while (CachedImageClient* c = w.next()) {
        if (c->willRenderImage(this))
            return false;
    }

    return true;
}

void CachedImage::animationAdvanced(const Image* image)
{
    if (!image || image != m_image)
        return;
    notifyObservers();
}

void CachedImage::changedInRect(const Image* image, const IntRect& rect)
{
    if (!image || image != m_image)
        return;
    notifyObservers(&rect);
}

bool CachedImage::currentFrameKnownToBeOpaque(const RenderObject* renderer)
{
    Image* image = imageForRenderer(renderer);
    if (image->isBitmapImage())
        image->nativeImageForCurrentFrame(); // force decode
    return image->currentFrameKnownToBeOpaque();
}

} // namespace WebCore
