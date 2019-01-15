/*
 * Copyright (C) 2013 Samsung Electronic.
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

#ifndef LinkMagnifierProxy_h
#define LinkMagnifierProxy_h

#if ENABLE(TIZEN_LINK_MAGNIFIER)

#include <Evas.h>
#include <WebCore/IntPoint.h>

class EwkViewImpl;

namespace WebKit {

class LinkMagnifierProxy {
public:
    static LinkMagnifierProxy& linkMagnifier();
    ~LinkMagnifierProxy();

    void show(WebPageProxy*, const WebCore::IntPoint&, const WebCore::IntRect&);
    void requestUpdate(const WebCore::IntRect&);
    void didChangeBackForwardList(WebPageProxy*);
    void hide();
    void setClickedExactly(bool exactly) { m_isClickedExactly = exactly; }
    bool isClickedExactly(){ return m_isClickedExactly; }
private:
    LinkMagnifierProxy();

    void adjustRectByContentSize();
    void setImageSize(int&, int&, int, int, const WebCore::IntPoint&);
    void updateImage();
    void clear();
    void startMagnifierAnimation();
    void endMagnifierAnimation();
    bool isVisible();

    static void mouseUp(void*, Evas*, Evas_Object*, void*);
    static void bgMouseUp(void*, Evas*, Evas_Object*, void*);
    static void imageSizeChanged(void*, Evas*, Evas_Object*, void*);
#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    static void linkMagnifierHwMoreBackKeyCallback(void*, Evas_Object*, void*);
#endif
    static void transitionFinishedCallback(void*, Evas_Object*, const char*, const char*);

    void updateTimerFired(WebCore::Timer<LinkMagnifierProxy>*);

    RefPtr<WebPageProxy> m_page;
    Evas_Object* m_image;
    float m_scaleFactor;
    WebCore::IntRect m_pageRect;
    WebCore::IntPoint m_selectedPosition;
    WebCore::Timer<LinkMagnifierProxy> m_updateTimer;
    Evas_Object* m_magnifier;
    Evas_Object* m_magnifierbg;
    bool m_isClickedExactly;
};

} // namespace WebKit

#endif

#endif // LinkMagnifierProxy_h
