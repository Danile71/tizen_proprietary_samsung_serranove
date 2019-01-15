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

#ifndef ScreenReader_h
#define ScreenReader_h

#if ENABLE(TIZEN_SCREEN_READER)

#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {
    class IntPoint;
    class Node;
    class RenderObject;
}

namespace WebKit {

class WebPage;

class ScreenReader {
public:
    static PassOwnPtr<ScreenReader> create(WebPage* page)
    {
        return adoptPtr(new ScreenReader(page));
    }
    ~ScreenReader();

    bool moveFocus(bool);
    bool moveFocus(const WebCore::IntPoint&, bool);

    WebCore::Node* focusedNode();

    bool rendererWillBeDestroyed(WebCore::RenderObject*);
    void clearFocus();

private:
    ScreenReader(WebPage*);

    WebCore::RenderObject* traverse(WebCore::RenderObject*);
    WebCore::RenderObject* traverseSibling(WebCore::RenderObject*);
    WebCore::RenderObject* ownerElementSibling(WebCore::RenderObject*);

    WebCore::RenderObject* findFocusable(WebCore::RenderObject*);

    bool setFocus(WebCore::RenderObject*);

    WebPage* m_page;
    WebCore::RenderObject* m_focusedObject;
    bool m_hasFocus;
    bool m_isForward;

    static WebCore::IntSize s_hitTestPadding;
};

} // namespace WebKit

#endif // ENABLE(TIZEN_SCREEN_READER)

#endif // ScreenReader_h
