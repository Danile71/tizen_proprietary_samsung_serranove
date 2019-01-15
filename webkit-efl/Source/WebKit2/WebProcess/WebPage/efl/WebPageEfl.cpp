/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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
#include "WebPage.h"

#include "EditorState.h"
#include "NamedNodeMap.h"
#include "NotImplemented.h"
#include "WebEvent.h"
#include "WindowsKeyboardCodes.h"
#include <WebCore/EflKeyboardUtilities.h>
#include <WebCore/FocusController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/KeyboardEvent.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformKeyboardEvent.h>
#include <WebCore/RenderThemeEfl.h>
#include <WebCore/Settings.h>

#if PLATFORM(TIZEN)
#include "Arguments.h"
#include "GraphicsContext.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebImage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/FrameView.h>
#if ENABLE(TIZEN_GET_COOKIES_FOR_URL)
#include <WebCore/CookieJar.h>
#endif
#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLSelectElement.h>
#include <WebCore/HTMLNames.h>
#endif

#if ENABLE(TIZEN_MULTIPLE_SELECT)
#include "WebPopupMenu.h"
#include <WebCore/PopupMenuClient.h>
#endif

#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
#include <WebCore/PlatformContextCairo.h>
#endif

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
#include <WebCore/ClipboardTizen.h>
#endif

#if ENABLE(TIZEN_PASTEBOARD)
#include <WebCore/Pasteboard.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_REMOTE_WEB_INSPECTOR)
#include "WebInspectorServerEfl.h"
#endif

#if ENABLE(TIZEN_WEB_STORAGE)
#include <WebCore/GroupSettings.h>
#include <WebCore/PageGroup.h>
#endif

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
#include "PluginView.h"
#endif

#if ENABLE(TIZEN_PREFERENCE)
#include "WebPreferencesStore.h"
#include <WebCore/Settings.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
#include <WebCore/HTMLFrameOwnerElement.h>
#include <WebCore/HTMLImageElement.h>
#endif

#if ENABLE(TIZEN_DEVICE_ORIENTATION)
#include "DeviceMotionClientTizen.h"
#include "DeviceOrientationClientTizen.h"
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
#include "RenderLayer.h"
#include "WebGraphicsLayer.h"
#include <WebCore/RenderView.h>
#endif

#if ENABLE(TIZEN_ISF_PORT)
#include <WebCore/Text.h>
#include <WebCore/EditorClient.h>
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION) || ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#include "EditorClient.h"
#endif

#if ENABLE(TIZEN_DATALIST_ELEMENT)
#include "HTMLCollection.h"
#include "HTMLDataListElement.h"
#include "HTMLOptionElement.h"
#endif

#if ENABLE(TIZEN_STYLE_SCOPED)
#include <WebCore/RuntimeEnabledFeatures.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
#include "RenderIFrame.h"
#include "RenderImage.h"
#include "RenderText.h"
#include "htmlediting.h"
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
#include "LinkMagnifier.h"
#endif

#if ENABLE(TIZEN_SCREEN_READER)
#include "WebEventConversion.h"
#endif

#if ENABLE(TIZEN_SCREEN_READER) || ENABLE(TIZEN_FOCUS_UI) || ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
#include <WebCore/HTMLAreaElement.h>
#endif

#if ENABLE(TIZEN_CSP)
#include <WebCore/ContentSecurityPolicy.h>
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
#include "WebPageSerializerTizen.h"
#endif

#if ENABLE(TIZEN_USE_SETTINGS_FONT) || ENABLE(TIZEN_BROWSER_FONT)
#include "fontconfig/fontconfig.h"
#include <WebCore/FontCache.h>
#include <WebCore/PageCache.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
#include "visible_units.h"
#endif

#if ENABLE(TIZEN_PROD_DETECT_CONTENTS)
#include "SurroundingText.h"
#include <wkext.h>
#endif
#if ENABLE(TIZEN_ISF_PORT) || ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
#include <WebCore/HTMLFormElement.h>
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
#include <WebCore/HTMLTextAreaElement.h>
#endif

#if ENABLE(TIZEN_PROD_DETECT_CONTENTS)
#include <WebCore/ContextMenuController.h>
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
#include "MediaResourceControllerGStreamerTizen.h"
#endif

#endif // #if PLATFORM(TIZEN)

using namespace WebCore;

namespace WebKit {

#if ENABLE(TIZEN_CACHE_IMAGE_GET)
static CachedImage* getCachedImage(Element* element)
{
    ASSERT(element);
    RenderObject* renderer = element->renderer();
    if (!renderer || !renderer->isImage())
        return 0;
    RenderImage* image = toRenderImage(renderer);
    return image->cachedImage();
}

static Image* getImage(Element* element)
{
    ASSERT(element);
    CachedImage* cachedImage = getCachedImage(element);
    return (cachedImage && !cachedImage->errorOccurred()) ?
        cachedImage->image() : 0;
}
#endif

#if HAVE(ACCESSIBILITY)
void WebPage::updateAccessibilityTree()
{
    if (!m_webPageAccessibilityObject)
        return;

    webPageAccessibilityObjectRefresh(m_webPageAccessibilityObject.get());
}
#endif

void WebPage::platformInitialize()
{
#if HAVE(ACCESSIBILITY)
    m_webPageAccessibilityObject = adoptGRef(webPageAccessibilityObjectNew(this));
    String plugID(atk_plug_get_id(ATK_PLUG(m_webPageAccessibilityObject.get())));
    send(Messages::WebPageProxy::BindAccessibilityTree(plugID));
#endif
#if ENABLE(TIZEN_DEVICE_ORIENTATION)
    WebCore::provideDeviceMotionTo(m_page.get(), new DeviceMotionClientTizen);
    WebCore::provideDeviceOrientationTo(m_page.get(), new DeviceOrientationClientTizen);
#endif
#if ENABLE(TIZEN_FILE_SYSTEM)
    m_page->group().groupSettings()->setLocalFileSystemQuotaBytes(0x6400000); //100M
#endif

#if ENABLE(TIZEN_WORKERS)
    m_page->group().groupSettings()->setAllowUniversalAccessFromFileURLs(m_page->settings()->allowUniversalAccessFromFileURLs());
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
    m_page->group().groupSettings()->setIndexedDBDatabasePath(WebProcess::shared().indexedDatabaseDirectory());
    m_page->group().groupSettings()->setIndexedDBQuotaBytes(0x6400000); //100M
#endif
}

#if ENABLE(TIZEN_PREFERENCE)
void WebPage::platformPreferencesDidChange(const WebPreferencesStore& store)
{
    Settings* settings = m_page->settings();
    settings->setInteractiveFormValidationEnabled(store.getBoolValueForKey(WebPreferencesKey::interactiveFormValidationEnabledKey()));
    settings->setUsesEncodingDetector(store.getBoolValueForKey(WebPreferencesKey::usesEncodingDetectorKey()));
#if ENABLE(TIZEN_LOAD_REMOTE_IMAGES)
    settings->setLoadRemoteImages(store.getBoolValueForKey(WebPreferencesKey::loadRemoteImagesKey()));
#endif
#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
    settings->setScanMalwareEnabled(store.getBoolValueForKey(WebPreferencesKey::scanMalwareEnabledKey()));
#endif
#if ENABLE(TIZEN_USER_AGENT_WHITELIST)
    settings->setUserAgentWhitelistPath(store.getStringValueForKey(WebPreferencesKey::userAgentWhitelistPathKey()));
#endif
#if ENABLE(TIZEN_SOUP_FEATURES)
    settings->setSpdyEnabled(store.getBoolValueForKey(WebPreferencesKey::spdyEnabledKey()));
    settings->setPerformanceFeaturesEnabled(store.getBoolValueForKey(WebPreferencesKey::performanceFeaturesEnabledKey()));
#endif
#if ENABLE(TIZEN_LINK_EFFECT)
    settings->setLinkEffectEnabled(store.getBoolValueForKey(WebPreferencesKey::linkEffectEnabledKey()));
#endif
#if ENABLE(TIZEN_ISF_PORT)
    settings->setEnableDefaultKeypad(store.getBoolValueForKey(WebPreferencesKey::defaultKeypadEnabledKey()));
#endif
#if ENABLE(TIZEN_STYLE_SCOPED)
    WebCore::RuntimeEnabledFeatures::setStyleScopedEnabled(store.getBoolValueForKey(WebPreferencesKey::styleScopedEnabledKey()));
#endif
#if ENABLE(TIZEN_WEB_AUDIO)
    settings->setWebAudioEnabled(true);
#endif
}
#else
void WebPage::platformPreferencesDidChange(const WebPreferencesStore&)
{
    notImplemented();
}
#endif // #if ENABLE(TIZEN_PREFERENCE)

static inline void scroll(Page* page, ScrollDirection direction, ScrollGranularity granularity)
{
    page->focusController()->focusedOrMainFrame()->eventHandler()->scrollRecursively(direction, granularity);
}

bool WebPage::performDefaultBehaviorForKeyEvent(const WebKeyboardEvent& keyboardEvent)
{
    notImplemented();
    return false;
}

bool WebPage::platformHasLocalDataForURL(const KURL&)
{
    notImplemented();
    return false;
}

String WebPage::cachedResponseMIMETypeForURL(const KURL&)
{
    notImplemented();
    return String();
}

bool WebPage::platformCanHandleRequest(const ResourceRequest&)
{
    notImplemented();
    return true;
}

String WebPage::cachedSuggestedFilenameForURL(const KURL&)
{
    notImplemented();
    return String();
}

PassRefPtr<SharedBuffer> WebPage::cachedResponseDataForURL(const KURL&)
{
    notImplemented();
    return 0;
}

const char* WebPage::interpretKeyEvent(const KeyboardEvent* event)
{
    ASSERT(event->type() == eventNames().keydownEvent || event->type() == eventNames().keypressEvent);

    if (event->type() == eventNames().keydownEvent)
        return getKeyDownCommandName(event);

    return getKeyPressCommandName(event);
}

void WebPage::setThemePath(const String& themePath)
{
    WebCore::RenderThemeEfl* theme = static_cast<WebCore::RenderThemeEfl*>(m_page->theme());
    theme->setThemePath(themePath);
}

static Frame* targetFrameForEditing(WebPage* page)
{
    Frame* frame = page->corePage()->focusController()->focusedOrMainFrame();
    if (!frame)
        return 0;

    Editor* editor = frame->editor();
    if (!editor->canEdit())
        return 0;

    if (editor->hasComposition()) {
        // We should verify the parent node of this IME composition node are
        // editable because JavaScript may delete a parent node of the composition
        // node. In this case, WebKit crashes while deleting texts from the parent
        // node, which doesn't exist any longer.
        if (PassRefPtr<Range> range = editor->compositionRange()) {
            Node* node = range->startContainer();
            if (!node || !node->isContentEditable())
                return 0;
        }
    }

    return frame;
}

void WebPage::confirmComposition(const String& compositionString)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame)
        return;

#if ENABLE(TIZEN_ISF_PORT)
    if (m_prepareKeyDownEvent) {
        m_keyPressCommands.append(adoptPtr(new ConfirmCompositionKeyPressCommand(compositionString)));
        return;
    }
#endif

    targetFrame->editor()->confirmComposition(compositionString);

#if ENABLE(TIZEN_ISF_PORT)
    m_page->editorClient()->respondToChangedSelection(targetFrame);
#endif
}

void WebPage::setComposition(const String& compositionString, const Vector<WebCore::CompositionUnderline>& underlines, uint64_t cursorPosition)
{
    Frame* targetFrame = targetFrameForEditing(this);
    if (!targetFrame)
        return;

#if ENABLE(TIZEN_ISF_PORT)
    if (!targetFrame->editor()->hasComposition() && compositionString.isEmpty())
        return;

    if (m_prepareKeyDownEvent) {
        m_keyPressCommands.append(adoptPtr(new SetCompositionKeyPressCommand(compositionString, underlines, cursorPosition)));
        return;
    }
#endif

    targetFrame->editor()->setComposition(compositionString, underlines, cursorPosition, 0);
}

void WebPage::cancelComposition()
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    frame->editor()->cancelComposition();
}

#if PLATFORM(TIZEN)

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
IntSize WebPage::contentsSize() const
{
    FrameView* frameView = m_page->mainFrame()->view();
    if (!frameView)
        return IntSize(0, 0);

    return frameView->contentsSize();
}
#endif

void WebPage::scrollMainFrameBy(const IntSize& scrollOffset)
{
    m_page->mainFrame()->view()->scrollBy(scrollOffset);
}

void WebPage::scrollMainFrameTo(const IntPoint& scrollPosition)
{
    m_page->mainFrame()->view()->setScrollPosition(scrollPosition);
}

void WebPage::createSnapshot(const IntRect rect, float scaleFactor, ShareableBitmap::Handle& snapshotHandle)
{
    FrameView* frameView = m_mainFrame->coreFrame()->view();
    if (!frameView)
        return;

    RefPtr<WebImage> snapshotImage = scaledSnapshotInViewCoordinates(rect, scaleFactor, ImageOptionsShareable);
    if (!snapshotImage || !snapshotImage->bitmap())
        return;

    snapshotImage->bitmap()->createHandle(snapshotHandle);
}

#if ENABLE(TIZEN_CACHE_IMAGE_GET)
void WebPage::cacheImageGet(const String& imageUrl, ShareableBitmap::Handle& cacheImageHandle)
{
    RefPtr<WebImage> cacheImage = NULL;
    NativeImagePtr bitmapPtr = NULL;

    FrameView* frameView = m_mainFrame->coreFrame()->view();
    if (!frameView || frameView->frameRect().isEmpty())
        return;

    Document* document = m_mainFrame->coreFrame()->document();
    if (!document)
        return;

    cairo_surface_t* surface = NULL;
    Image* cachedImagePtr = NULL;
    PassRefPtr<HTMLCollection> images = document->images();
    if (!images)
        return;

    unsigned sourceLength = images->length();

    for (unsigned i = 0; i < sourceLength; ++i) {
        Node* node = images->item(i);
        Element* element = toElement(node);

        if (element && (element->getAttribute(element->imageSourceAttributeName()).string().contains(imageUrl))) {
            cachedImagePtr = getImage(element);
            if (cachedImagePtr) {
                bitmapPtr = cachedImagePtr->nativeImageForCurrentFrame();
                if(bitmapPtr) {
#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
                    if (bitmapPtr->surfaceType() == NativeImageCairo::NATIVE_GL_SURFACE)
                        surface = bitmapPtr->glsurface();
                    else
                        surface = bitmapPtr->surface();
#else
                    surface = bitmapPtr->surface();
#endif
                    break;
                }
            }
        }
    }
    if(!cachedImagePtr || !bitmapPtr || !surface)
        return;
    IntSize imageSize = IntSize();
    imageSize = cachedImagePtr->getCacheImageSize(bitmapPtr);
    RefPtr<WebImage> cacheImageRef = WebImage::create(imageSize, ImageOptionsShareable);
    if (!cacheImageRef || !cacheImageRef->bitmap())
        return;
    OwnPtr<GraphicsContext> graphicsContext = cacheImageRef->bitmap()->createGraphicsContext();

    cairo_t* cr = graphicsContext->platformContext()->cr();
    cairo_set_source_surface(cr, surface, 0, 0);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(cr, 0, 0, imageSize.width(), imageSize.height());
    cairo_fill(cr);

    cacheImage = cacheImageRef.release();
    if (!cacheImage || !cacheImage->bitmap())
        return;

    cacheImage->bitmap()->createHandle(cacheImageHandle);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
bool WebPage::isReadOnlyDisabledNode(Node* node)
{
    bool result = FALSE;
    if (node && node->isElementNode()) {
        if (equalIgnoringCase(node->nodeName(), "SELECT")) {
            HTMLSelectElement* selectElem = toHTMLSelectElement(node);
            if (selectElem && (selectElem->readOnly() || selectElem->disabled()))
                result = TRUE;
        } else if (equalIgnoringCase(node->nodeName(), "INPUT")) {
            HTMLInputElement* inputElem = node->toInputElement();
            if (inputElem && (inputElem->readOnly() || inputElem->disabled()))
                result = TRUE;
        } else if (equalIgnoringCase(node->nodeName(), "TEXTAREA")) {
            HTMLTextAreaElement* textareaElem = static_cast<HTMLTextAreaElement*>(node);
            if (textareaElem && (textareaElem->readOnly() || textareaElem->disabled()))
                result = TRUE;
        }
    }
    return result;
}

void WebPage::requestUpdateFormNavigation()
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    Document* document = frame->document();
    if (!document)
        return;

    Node* focusedNode = document->focusedNode();

    Vector<RefPtr<Node> > focusableNodes;
    document->getFocusableNodes(focusableNodes);

    int formElementCount = 0;
    int currentNodeIndex = -1;
    bool prevState = false;
    bool nextState = false;
    bool selectState = false;
    const Vector<RefPtr<Node> >::iterator end = focusableNodes.end();
    for (Vector<RefPtr<Node> >::iterator it = focusableNodes.begin(); it != end; ++it) {
        if (isReadOnlyDisabledNode((*it).get()))
            continue;
        AtomicString nodeName = (*it).get()->nodeName();
        if (equalIgnoringCase(nodeName, "SELECT")) {
            if (formElementCount == (currentNodeIndex + 1) && selectState)
                nextState = true;
            if ((*it).get() == focusedNode) {
                currentNodeIndex = formElementCount;
                prevState = selectState;
            }
            formElementCount++;
            selectState = true;
        } else if (equalIgnoringCase(nodeName, "INPUT") || equalIgnoringCase(nodeName, "TEXTAREA")) {
            const AtomicString& type = equalIgnoringCase(nodeName, "INPUT") ? (*it).get()->toInputElement()->type() : nullAtom;
            if (equalIgnoringCase(nodeName, "TEXTAREA") || (!equalIgnoringCase(type, "CHECKBOX")
              && !equalIgnoringCase(type, "RADIO")
              && !equalIgnoringCase(type, "SUBMIT")
              && !equalIgnoringCase(type, "BUTTON")
              && !equalIgnoringCase(type, "COLOR")
              && !equalIgnoringCase(type, "DATETIME")
              && !equalIgnoringCase(type, "FILE")
              && !equalIgnoringCase(type, "HIDDEN")
              && !equalIgnoringCase(type, "IMAGE")
              && !equalIgnoringCase(type, "RESET")
              && !equalIgnoringCase(type, "MONTH")
              && !equalIgnoringCase(type, "WEEK")
              && !equalIgnoringCase(type, "TIME")
              && !equalIgnoringCase(type, "RANGE")
              && !equalIgnoringCase(type, "DATETIME-LOCAL")
              && !equalIgnoringCase(type, "DATE"))
              ) {
                if ((*it).get() == focusedNode)
                    currentNodeIndex = formElementCount;
                    formElementCount++;
                    selectState = false;
                }
          }
    }

    if (currentNodeIndex == -1)
        return;
    send(Messages::WebPageProxy::UpdateFormNavigation(formElementCount, currentNodeIndex, prevState, nextState));
}

void WebPage::moveFocus(int newIndex)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;
    Document* document = frame->document();
    if (!document)
        return;

    Vector<RefPtr<Node> > focusableNodes;
    document->getFocusableNodes(focusableNodes);

    int index = 0;
    const Vector<RefPtr<Node> >::iterator end = focusableNodes.end();
    for (Vector<RefPtr<Node> >::iterator it = focusableNodes.begin(); it != end; ++it) {
        if (isReadOnlyDisabledNode((*it).get()))
            continue;
        AtomicString nodeName = (*it).get()->nodeName();
        if (equalIgnoringCase(nodeName, "SELECT")) {
            if (index == newIndex) {
                (*it).get()->setFocus();
                LayoutPoint position = LayoutPoint(0, 0);
                PlatformMouseEvent event(flooredIntPoint(position), flooredIntPoint(position), LeftButton, PlatformEvent::MouseMoved, 1, false, false, false, false, 0);
                (*it).get()->dispatchMouseEvent(event, "mousedown", 0, 0);
                break;
            }
            index++;
        } else if (equalIgnoringCase(nodeName, "INPUT") || equalIgnoringCase(nodeName, "TEXTAREA")) {
            const AtomicString& type = equalIgnoringCase(nodeName, "INPUT") ? (*it).get()->toInputElement()->type() : nullAtom;
            if (equalIgnoringCase(nodeName, "TEXTAREA") || (!equalIgnoringCase(type, "CHECKBOX")
              && !equalIgnoringCase(type, "RADIO")
              && !equalIgnoringCase(type, "SUBMIT")
              && !equalIgnoringCase(type, "BUTTON")
              && !equalIgnoringCase(type, "COLOR")
              && !equalIgnoringCase(type, "DATETIME")
              && !equalIgnoringCase(type, "FILE")
              && !equalIgnoringCase(type, "HIDDEN")
              && !equalIgnoringCase(type, "IMAGE")
              && !equalIgnoringCase(type, "RESET")
              && !equalIgnoringCase(type, "MONTH")
              && !equalIgnoringCase(type, "WEEK")
              && !equalIgnoringCase(type, "TIME")
              && !equalIgnoringCase(type, "RANGE")
              && !equalIgnoringCase(type, "DATETIME-LOCAL")
              && !equalIgnoringCase(type, "DATE"))
              ) {
                if (index == newIndex) {
                    HTMLTextFormControlElement* elem = NULL;
                    if (equalIgnoringCase(nodeName, "INPUT")) {
                        elem = (*it).get()->toInputElement();
                        ((HTMLInputElement*)elem)->cacheSelectionInResponseToSetValue(elem->value().length());
                    } else if (equalIgnoringCase(nodeName, "TEXTAREA"))
                        elem = static_cast<HTMLTextAreaElement*>((*it).get());
                    elem->focus();
                    if (!elem->readOnly() && !elem->disabled()) {
                        m_editorState.isContentEditable = true;
                        send(Messages::WebPageProxy::EditorStateChanged(m_editorState));
                        send(Messages::WebPageProxy::UpdateTextInputStateByUserAction(true));
                    } else
                        send(Messages::WebPageProxy::UpdateTextInputStateByUserAction(false));
                    break;
                }
                index++;
            }
        }
    }
}
#endif
#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
#define INCH_TO_MM 25.4
#define INCH_TO_POINTS 72.0

void WebPage::createPagesToPDF(const IntSize& surfaceSize, const IntSize& contentsSize, const String& fileName)
{
    FrameView* frameView = m_mainFrame->coreFrame()->view();
    if (!frameView)
        return;

    RefPtr<WebImage> pageshotImage = WebImage::create(contentsSize, ImageOptionsShareable);
    if (!pageshotImage->bitmap())
        return;

    double pdfWidth = (double)surfaceSize.width() / INCH_TO_MM * INCH_TO_POINTS;
    double pdfHeight = (double)surfaceSize.height() / INCH_TO_MM * INCH_TO_POINTS;
    OwnPtr<WebCore::GraphicsContext> graphicsContext = pageshotImage->bitmap()->createGraphicsContextForPdfSurface(fileName, pdfWidth, pdfHeight);

    double scaleFactorPdf = 1.0;
    if (contentsSize.width() > pdfWidth)
        scaleFactorPdf = pdfWidth / (double)contentsSize.width();
    else
        graphicsContext->translate(ceil((pdfWidth - contentsSize.width()) / 2), 0);

    graphicsContext->scale(FloatSize(scaleFactorPdf, scaleFactorPdf));

    frameView->updateLayoutAndStyleIfNeededRecursive();

    int pageNumber = ((contentsSize.height() * scaleFactorPdf) / pdfHeight) + 1;
    float paintY = 0.0;

    PaintBehavior oldBehavior = frameView->paintBehavior();
    frameView->setPaintBehavior(oldBehavior | PaintBehaviorFlattenCompositingLayers);
    for (int i = 0; i < pageNumber; i++) {
        IntRect paintRect(0, (int)paintY, contentsSize.width(), (int)(pdfHeight / scaleFactorPdf));

        frameView->paint(graphicsContext.get(), paintRect);
        cairo_show_page(graphicsContext->platformContext()->cr());
        graphicsContext->translate(0, -ceil(pdfHeight / scaleFactorPdf));
        paintY += (pdfHeight / scaleFactorPdf);
    }
    frameView->setPaintBehavior(oldBehavior);

    pageshotImage.release();

    send(Messages::WebPageProxy::DidCreatePagesToPDF());
}
#endif

#if ENABLE(TIZEN_TEXT_CARET_HANDLING_WK2)
bool WebPage::setCaretPosition(const IntPoint& pos)
{
    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
    if (!focusedFrame)
        return false;

    FrameSelection* frameSelection = focusedFrame->selection();
    if (!frameSelection)
        return false;

    FrameView* frameView = focusedFrame->view();
    if (!frameView)
        return false;

    IntPoint point = m_page->mainFrame()->view()->windowToContents(pos);
    HitTestResult hitTestResult = m_page->mainFrame()->eventHandler()->hitTestResultAtPoint(point, /*allowShadowContent*/ true, /*ignoreClipping*/ true);
    if (hitTestResult.scrollbar())
        return false;

    Node* innerNode = hitTestResult.innerNode();
    if (!innerNode || !innerNode->renderer())
        return false;

    // for plain text input fields we can get only a caret bounding box
    if (!frameSelection->isCaret() || !frameSelection->caretRenderer())
        return false;

    const Node* node = frameSelection->start().deprecatedNode();
    if (!node || !node->renderer())
        return false;

    Element* currentRootEditableElement = node->rootEditableElement();
    Element* newRootEditableElement = innerNode->rootEditableElement();
    if (currentRootEditableElement != newRootEditableElement)
        return false;

    IntRect rect = frameSelection->caretRenderer()->absoluteBoundingBoxRect(true);

    // The below wirtten code is not correct way to implement. Presntly the is no
    // other working way. To be replaced by better logic
    // here we also cheat input field that we actually are just inside of if
    IntPoint focusedFramePoint = focusedFrame->view()->windowToContents(pos);
    IntPoint oldFocusedFramePoint = focusedFramePoint;

    const int boundariesWidth = 2;
    const int leftBoundaryWidth = 4;
    if (focusedFramePoint.x() < rect.x() + leftBoundaryWidth)
        focusedFramePoint.setX(rect.x());
    else if (focusedFramePoint.x() > rect.maxX())
        focusedFramePoint.setX(rect.maxX());

    if (focusedFramePoint.y() < rect.y() + boundariesWidth)
        focusedFramePoint.setY(rect.y() + boundariesWidth);
    else if (focusedFramePoint.y() >= rect.maxY() - boundariesWidth)
        focusedFramePoint.setY(rect.maxY() - boundariesWidth - 1);

    int diffX = focusedFramePoint.x() - oldFocusedFramePoint.x();
    int diffY = focusedFramePoint.y() - oldFocusedFramePoint.y();

    // point gets inner node local coordinates
    IntPoint localPoint = flooredIntPoint(hitTestResult.localPoint());
    localPoint.setX(localPoint.x() + diffX);
    localPoint.setY(localPoint.y() + diffY);

    // visible position created
    VisiblePosition visiblePos = innerNode->renderer()->positionForPoint(localPoint);
    if (visiblePos.isNull())
        return false;

    // create visible selection from visible position
    VisibleSelection newSelection = VisibleSelection(visiblePos);
    frameSelection->setSelection(newSelection, CharacterGranularity);
    // after setting selection caret blinking is suspended by default so we are unsuspedning it
    frameSelection->setCaretBlinkingSuspended(false);

    return true;
}
#endif

#if ENABLE(TIZEN_ISF_PORT)
void WebPage::didCancelComposition(Node* valueChangedNode)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || frame->editor()->ignoreCompositionSelectionChange())
        return;
    if (!valueChangedNode->containsIncludingShadowDOM(frame->editor()->compositionNode())
        )
        return;

    frame->editor()->cancelComposition();
    send(Messages::WebPageProxy::DidCancelComposition());
}

void WebPage::requestUpdatingEditorState()
{
    if (!m_page)
        return;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    if (frame->selection() && !frame->selection()->isRange()){
        if (m_updateEditorStateTimer.isActive()) {
            m_updateEditorStateTimer.stop();
            updateEditorStateTimerFired();
        }

        send(Messages::WebPageProxy::DidRequestUpdatingEditorState());
    } else {
        if (m_updateEditorStateTimer.isActive())
            m_updateEditorStateTimer.stop();
        updateEditorState(true);

    }
}

void WebPage::prepareKeyDownEvent()
{
    m_prepareKeyDownEvent = true;
}

void WebPage::swapKeyPressCommands(Vector<OwnPtr<KeyPressCommand> >& commands)
{
    m_keyPressCommands.swap(commands);
}

void WebPage::selectSurroundingText(int offset, int count)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    Position base(frame->selection()->base());
    Position extent(base);

    int start = offset + base.offsetInContainerNode();
    int end = start + count;
    int last = lastOffsetForEditing(base.anchorNode());

    start = std::min(max(start, 0), last);
    end = std::min(max(end, start), last);

    base.moveToOffset(start);
    extent.moveToOffset(end);

    VisibleSelection selection(base, extent);
    if (!selection.isRange())
        return;

    frame->selection()->setSelection(selection);
}

void WebPage::deleteSurroundingText(int offset, int count)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->editor()->canEdit())
        return;

    if (m_prepareKeyDownEvent) {
        m_keyPressCommands.append(adoptPtr(new DeleteTextKeyPressCommand(offset, count)));
        return;
    }

    selectSurroundingText(offset, count);
    frame->editor()->deleteWithDirection(DirectionBackward, CharacterGranularity, false, true);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
static void adjustRect(IntRect& rect)
{
    if (rect.x() < 0)
        rect.shiftXEdgeTo(0);
    if (rect.y() < 0)
        rect.shiftYEdgeTo(0);
}

static void addSelectionRects(RenderObject* renderer, int startPos, int endPos, Vector<IntRect>& rects)
{
    if (renderer->isText()) {
        RenderObject::SelectionState state = renderer->selectionState();
        int startOffset = (state == RenderObject::SelectionStart || state == RenderObject::SelectionBoth) ? startPos : 0;
        int endOffset = (state == RenderObject::SelectionEnd || state == RenderObject::SelectionBoth) ? endPos : std::numeric_limits<int>::max();

        toRenderText(renderer)->absoluteRectsForRange(rects, startOffset, endOffset, true, 0);
    } else
        rects.append(pixelSnappedIntRect(renderer->selectionRect()));
}

static IntRect findBoundarySelectionRect(RenderObject* renderer, Vector<IntRect>& rects, bool isLeft, bool restrictToParent)
{
    size_t size = rects.size();
    if (!size)
        return IntRect();

    IntRect rect;
    if (isLeft) {
        for (size_t i = 0; i < size; ++i) {
            adjustRect(rects[i]);
            if (rects[i].isEmpty())
                continue;

            if (rect.isEmpty())
                rect = rects[i];
            else if (rect.y() == rects[i].y() && rect.height() == rects[i].height() && rect.maxX() == rects[i].x())
                rect.setWidth(rect.width() + rects[i].width());
            else
                break;
        }
    } else {
        size_t i = size - 1;
        do {
            adjustRect(rects[i]);
            if (rects[i].isEmpty())
                continue;

            if (rect.isEmpty())
                rect = rects[i];
            else if (rect.y() == rects[i].y() && rect.height() == rects[i].height() && rect.x() == rects[i].maxX())
                rect.shiftXEdgeTo(rects[i].x());
            else
                break;
        } while (i--);
    }

    if (rect.isEmpty())
        return IntRect();

    RenderLayer* layer = renderer->enclosingLayer();
    while (layer && restrictToParent) {
        RenderObject* layerRenderer = layer->renderer();
        layer = layer->parent();

        if (layerRenderer->hasOverflowClip()) {
            FloatRect boxRect = toRenderBox(layerRenderer)->borderBoxRect();
            IntRect layerRect = enclosingIntRect(layerRenderer->localToAbsoluteQuad(boxRect).boundingBox());
            rect.intersect(layerRect);
        }
    }

    return rect;
}

static bool getBoundarySelectionRects(RenderView* view, IntRect& leftRect, IntRect& rightRect)
{
    RenderObject* start = view->selectionStart();
    RenderObject* stop = view->selectionEnd();
    int startPos, endPos;
    view->selectionStartEnd(startPos, endPos);

    if (!start || !stop)
        return false;

    stop = stop->childAt(endPos);
    if (!stop)
        stop = view->selectionEnd()->nextInPreOrderAfterChildren();

    RenderObject* leftRenderer = 0;
    Vector<IntRect> leftRects;
    for (RenderObject* renderer = start; renderer != stop; renderer = renderer->nextInPreOrder()) {
        if (renderer->firstChild())
            continue;

        addSelectionRects(renderer, startPos, endPos, leftRects);
        IntRect rect = findBoundarySelectionRect(renderer, leftRects, true, false);

        if (!rect.isEmpty()) {
            leftRenderer = renderer;
            leftRect = rect;
            break;
        }

        leftRects.clear();
    }

    if (!leftRenderer)
        return false;

    RenderObject* rightRenderer = 0;
    for (RenderObject* renderer = stop ? stop->previousInPreOrder() : view->selectionEnd(); ; renderer = renderer->previousInPreOrder()) {
        IntRect rect;

        if (!renderer->firstChild()) {
            if (renderer == leftRenderer)
                rect = findBoundarySelectionRect(renderer, leftRects, false, false);
            else {
                Vector<IntRect> rightRects;
                addSelectionRects(renderer, startPos, endPos, rightRects);
                rect = findBoundarySelectionRect(renderer, rightRects, false, false);
            }
        }

        if (!rect.isEmpty()) {
            rightRenderer = renderer;
            rightRect = rect;
            break;
        } else if (renderer == start)
            break;
    }

    if (!rightRenderer)
        return false;

    if (leftRenderer->enclosingBoxModelObject() == rightRenderer->enclosingBoxModelObject()
        && leftRect.y() == rightRect.y()) {
        if (leftRect.height() > rightRect.height())
            rightRect.setHeight(leftRect.height());
        else
            leftRect.setHeight(rightRect.height());
    }

    return true;
}

static LayoutPoint offsetOfFrame(const Frame* frame)
{
    Frame* mainFrame = frame->page()->mainFrame();
    return mainFrame->view()->windowToContents(frame->view()->contentsToWindow(IntPoint()));
}

static IntRect getUpperLayerRect(const Frame* frame, Range* range, const IntPoint& point)
{
    HitTestResult result = frame->page()->mainFrame()->eventHandler()->hitTestResultAtPoint(point, false);
    Node* hitNode = result.innerNonSharedNode();
    if (!hitNode || !hitNode->renderer())
        return IntRect();

    Frame* hitFrame = hitNode->document()->frame();
    RenderBoxModelObject* layerRenderer = hitNode->renderer()->enclosingLayer()->renderer();

    if (frame == hitFrame) {
        if (!range) {
            for (RenderObject* renderer = hitNode->renderer(); renderer; renderer = renderer->parent()) {
                if (renderer->selectionState() != RenderObject::SelectionNone)
                    return IntRect();
            }
        } else {
            Node* layerNode = 0;
            for (RenderObject* renderer = layerRenderer; renderer; renderer = renderer->parent()) {
                if (Node* node = renderer->node()) {
                    layerNode = node;
                    break;
                }
            }

            Position beforePos(layerNode, Position::PositionIsBeforeAnchor);
            Position afterPos(layerNode, Position::PositionIsAfterAnchor);
            if (comparePositions(beforePos, range->endPosition()) <= 0
                && comparePositions(afterPos, range->startPosition()) >= 0)
                return IntRect();
        }
    } else
        layerRenderer = hitFrame->contentRenderer();

    FloatRect boxRect = toRenderBox(layerRenderer)->borderBoxRect();
    IntRect enclosingRect = enclosingIntRect(layerRenderer->localToAbsoluteQuad(boxRect).boundingBox());
    enclosingRect.moveBy(roundedIntPoint(offsetOfFrame(layerRenderer->frame())));

    return enclosingRect;
}

static IntRect calcVisibleRect(const Frame* frame, Range* range, const IntRect& rect, bool isLeft)
{
    if (rect.isEmpty() || !frame)
        return IntRect();

    IntRect contentRect = frame->page()->mainFrame()->view()->windowToContents(rect);

    IntPoint top;
    if (isLeft)
        top = contentRect.minXMinYCorner();
    else
        top = contentRect.maxXMinYCorner();

    Region region(contentRect);

    IntRect upperLayerRect = getUpperLayerRect(frame, range, top);
    if (!upperLayerRect.isEmpty()) {
        region.subtract(Region(upperLayerRect));
        if (region.isEmpty() || (isLeft ? region.bounds().x() : region.bounds().maxX()) != top.x())
            return IntRect();
    }

    IntPoint bottom;
    if (isLeft)
        bottom = region.bounds().minXMaxYCorner();
    else
        bottom = region.bounds().maxXMaxYCorner();

    if (top.x() != bottom.x())
        return IntRect();

    upperLayerRect = getUpperLayerRect(frame, range, bottom);
    if (!upperLayerRect.isEmpty()) {
        region.subtract(Region(upperLayerRect));
        if (region.isEmpty() || (isLeft ? region.bounds().x() : region.bounds().maxX()) != top.x())
            return IntRect();
    }

    return region.bounds();
}

void WebPage::calcSelectionRects(const Frame* frame, const IntRect& editorRect, bool needVisibleRect, IntRect& leftRect, IntRect& rightRect, bool& isLeftToRightDirection) const
{
    if (!getBoundarySelectionRects(frame->contentRenderer(), leftRect,  rightRect))
        return;

    leftRect = frame->view()->contentsToWindow(leftRect);
    rightRect = frame->view()->contentsToWindow(rightRect);
    isLeftToRightDirection = WebCore::isLeftToRightDirection(frame->selection()->getSelectionTextDirection());

    if (needVisibleRect) {
        leftRect = calcVisibleRect(frame, 0, leftRect, isLeftToRightDirection);
        rightRect = calcVisibleRect(frame, 0, rightRect, !isLeftToRightDirection);
    }
}

bool WebPage::updateEditorStateRect(const Frame* frame, EditorState& state) const
{
    FrameSelection* frameSelection = frame->selection();
    Vector<IntRect> rects;
    calcFocusedRects(frameSelection->rootEditableElement(), rects);
    IntRect editorRect = unionRect(rects);
    bool hasSameRects = (state.editorRect == editorRect);

    IntRect selectionRect;
    IntRect leftSelectionRect, rightSelectionRect;
    bool isLeftToRightDirection;

    if (frameSelection->isCaret()) {
        if (state.editorRect != editorRect)
            frameSelection->setCaretRectNeedsUpdate();
        selectionRect = frame->view()->contentsToWindow(frameSelection->absoluteCaretBounds());
        RefPtr<Range> range = frameSelection->selection().firstRange();
        selectionRect = calcVisibleRect(frame, range.get(), selectionRect, true);

        hasSameRects &= (state.selectionRect == selectionRect);
    } else if (frameSelection->isRange()) {
        selectionRect = frame->view()->contentsToWindow(enclosingIntRect(frameSelection->bounds(false)));
        calcSelectionRects(frame, editorRect, true, leftSelectionRect, rightSelectionRect, isLeftToRightDirection);
        if (leftSelectionRect.isEmpty() && rightSelectionRect.isEmpty() && !selectionRect.isEmpty()) {
            leftSelectionRect = selectionRect;
            rightSelectionRect = selectionRect;
        }
        hasSameRects &= (state.selectionRect == selectionRect && state.leftSelectionRect == leftSelectionRect
                      && state.rightSelectionRect == rightSelectionRect && state.isLeftToRightDirection == isLeftToRightDirection);
        auto visibleContentRect = frame->view()->visibleContentRect();
        hasSameRects &= (state.isLeftHandleVisible == leftSelectionRect.intersects(visibleContentRect) &&
                        state.isRightHandleVisible == rightSelectionRect.intersects(visibleContentRect));
    }

#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    IntRect caretRect;
    if (frameSelection->isCaret())
        caretRect = frame->view()->contentsToWindow(frameSelection->absoluteCaretBounds());

    IntRect editableElementRect;
    if (frameSelection->rootEditableElement()) {
        Node* hostNode = frameSelection->rootEditableElement()->shadowHost();
        if (!hostNode)
            hostNode = frameSelection->rootEditableElement();

        editableElementRect = frame->view()->contentsToWindow(hostNode->getPixelSnappedRect());

        hasSameRects &= (state.focusedNodeRect == editableElementRect);
    }
#endif

    if (hasSameRects)
        return false;

    state.selectionRect = selectionRect;
    state.editorRect = editorRect;
    state.leftSelectionRect = leftSelectionRect;
    state.rightSelectionRect = rightSelectionRect;
    state.isLeftToRightDirection = isLeftToRightDirection;

    auto hitTest = [&](const IntPoint & hitpoint)
    {
        return frame->eventHandler()->hitTestResultAtPoint(
            frame->view()->windowToContents(hitpoint),
            false).isSelected();
    };

    // We need to test both upper, and lower corner for partial overlap if input field case
    state.isLeftHandleVisible = hitTest(leftSelectionRect.minXMinYCorner()) || hitTest(leftSelectionRect.minXMaxYCorner());
//Will add SUBPIXEL_LAYOUT flag here later
    // As SUBPIXEL_LAYOUT has been enabled, right selection handler is not visible sometimes.
    // It caused by calculations which are done earlier.
    //
    // This is regression from http://168.219.209.56/gerrit/#/c/73933/
    //
    // Workaround: try to treat |rightSelectionRect - 1| as proper selection block as well as |rightSelectionRect|.
    // FIXME: where should it be fixed?.
    IntPoint decreasedXCandidateFirst = rightSelectionRect.maxXMinYCorner();
    decreasedXCandidateFirst.move(-1, 0);
    IntPoint decreasedXCandidateSecond = rightSelectionRect.maxXMaxYCorner();
    decreasedXCandidateSecond.move(-1, 0);
//SUBPIXEL_LAYOUT endif
    state.isRightHandleVisible = hitTest(rightSelectionRect.maxXMinYCorner())
                                || hitTest(rightSelectionRect.maxXMaxYCorner())
//Will add SUBPIXEL_LAYOUT flag here later
                                || hitTest(decreasedXCandidateFirst)
                                || hitTest(decreasedXCandidateSecond);
//SUBPIXEL_LAYOUT endif
#if ENABLE(TIZEN_WEBKIT2_INPUT_FIELD_ZOOM)
    state.caretRect = caretRect;
    state.focusedNodeRect = editableElementRect;
#endif

    return true;
}
#endif

#if ENABLE(TIZEN_ISF_PORT) || ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
void WebPage::setBaseEditorState(const EditorState& editorState)
{
    m_editorState = editorState;

    if (!m_updateEditorStateTimer.isActive())
        m_updateEditorStateTimer.startOneShot(0);
}

static bool isUTF16SurrogatePair(UChar lead, UChar trail)
{
    return (lead >= 0xd800 && lead < 0xdc00) && (trail >= 0xdc00 && trail < 0xe000);
}

void WebPage::updateEditorState(bool sendMessage)
{
    if (!m_page)
        return;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    m_editorState = editorState();

#if ENABLE(TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION)
    if (!m_editorState.shouldIgnoreCompositionSelectionChange && m_page->settings()->textStyleStateEnabled()) {
        m_editorState.underlineState = frame->editor()->selectionHasStyle(CSSPropertyWebkitTextDecorationsInEffect, "underline");
        m_editorState.italicState = frame->editor()->selectionHasStyle(CSSPropertyFontStyle, "italic");
        m_editorState.boldState = frame->editor()->selectionHasStyle(CSSPropertyFontWeight, "bold");
        m_editorState.bgColor = frame->editor()->selectionStartCSSPropertyValue(CSSPropertyBackgroundColor);
        m_editorState.color = frame->editor()->selectionStartCSSPropertyValue(CSSPropertyColor);
        m_editorState.fontSize = frame->editor()->selectionStartCSSPropertyValue(CSSPropertyFontSize);
        m_editorState.orderedListState = frame->editor()->selectionOrderedListState();
        m_editorState.unorderedListState = frame->editor()->selectionUnorderedListState();
        m_editorState.textAlignCenterState = frame->editor()->selectionHasStyle(CSSPropertyTextAlign, "center");
        m_editorState.textAlignLeftState = frame->editor()->selectionHasStyle(CSSPropertyTextAlign, "left");
        m_editorState.textAlignRightState = frame->editor()->selectionHasStyle(CSSPropertyTextAlign, "right");
        m_editorState.textAlignFullState = frame->editor()->selectionHasStyle(CSSPropertyTextAlign, "justify");
    }
#endif

#if ENABLE(TIZEN_FONT_SIZE_CSS_STYLING)
    Node* anchorNode = frame->selection()->selection().base().anchorNode();
    if (anchorNode && anchorNode->parentElement() && anchorNode->parentElement()->isStyledElement()) {
        Element* parent = anchorNode->parentElement();
        RefPtr<CSSValue> fontSize = parent->style()->getPropertyCSSValue(getPropertyName(CSSPropertyFontSize));
        CSSPrimitiveValue* value = static_cast<CSSPrimitiveValue*>(fontSize.get());
        if (value)
            m_editorState.fontSize = value->customCssText();
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    m_editorState.isOnlyImageSelection = isSelectionOnlyImage();
    updateEditorStateRect(frame, m_editorState);
#endif

#if ENABLE(TIZEN_ISF_PORT)
    Element* rootEditableElement = frame->selection()->rootEditableElement();
    if (!rootEditableElement) {
        if (sendMessage) {
            send(Messages::WebPageProxy::EditorStateChanged(m_editorState));
#if ENABLE(TIZEN_WEBKIT2_SCROLL_POSITION_BUG_FIX)
            m_shouldSyncVisibleContentRect = false;
#endif
        }
        return;
    }

    if (m_editorState.isContentRichlyEditable && rootEditableElement != frame->document()->focusedNode())
        m_editorState.isContentEditable = false;

    if (!m_editorState.shouldIgnoreCompositionSelectionChange && m_editorState.isContentEditable) {
        Node* hostNode = rootEditableElement->shadowHost();
        if (!hostNode)
            hostNode = rootEditableElement;

        HTMLTextFormControlElement* formControl = toTextFormControl(hostNode);
        if (formControl) {
            const AtomicString& type = formControl->type();

            if (type == "number" && formControl->hasTagName(HTMLNames::inputTag)) {
                StepRange stepRange = static_cast<HTMLInputElement*>(formControl)->createStepRange(RejectAny);
                bool needsSigned = stepRange.minimum().isNegative();
                bool needsDecimal = (stepRange.step().floor() != stepRange.step());

                if (needsSigned && needsDecimal)
                    m_editorState.inputMethodHints = "numberSignedDecimal";
                else if (needsSigned)
                    m_editorState.inputMethodHints = "numberSigned";
                else if (needsDecimal)
                    m_editorState.inputMethodHints = "numberDecimal";
                else
                    m_editorState.inputMethodHints = "number";
            } else if (type == "text" && formControl->form() && equalIgnoringCase(formControl->form()->fastGetAttribute(HTMLNames::roleAttr), "search"))
                m_editorState.inputMethodHints = "search";
#if ENABLE(TIZEN_INPUT_PICKER_MAX_MIN)
            else if ((type == "date" || type == "month" || type == "week" || type == "time" || type == "datetime" || type == "datetime-local") && formControl->hasTagName(HTMLNames::inputTag)) {
                if (type == "date")
                    m_editorState.inputMethodHints = "date";
                else if (type == "month")
                    m_editorState.inputMethodHints = "month";
                else if (type == "week")
                    m_editorState.inputMethodHints = "week";
                else if (type == "time")
                    m_editorState.inputMethodHints = "time";
                else if (type == "datetime")
                    m_editorState.inputMethodHints = "datetime";
                else if (type == "datetime-local")
                    m_editorState.inputMethodHints = "datetime-local";

                HTMLInputElement* inputElement = frame->document()->focusedNode()->toInputElement();
                m_editorState.minDate = inputElement->fastGetAttribute(HTMLNames::minAttr);
                m_editorState.maxDate = inputElement->fastGetAttribute(HTMLNames::maxAttr);
            }
#endif
            else
                m_editorState.inputMethodHints = type;

            m_editorState.hasForm = formControl->form();
        }

        Position base = frame->selection()->base();
        Node* baseNode = base.containerNode();
        if (baseNode && baseNode->isTextNode()) {
            m_editorState.surroundingText = baseNode->nodeValue();
            int utf16Offset = base.offsetInContainerNode();
            for (int i = 0; i < utf16Offset; ++i) {
                if (isUTF16SurrogatePair(m_editorState.surroundingText[i], m_editorState.surroundingText[i + 1]))
                    ++i;
                ++m_editorState.cursorPosition;
            }
        }

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
        if (frame->selection()->isCaret()) {
            VisiblePosition currentPosition = frame->selection()->selection().visibleStart();
            VisiblePosition startPosition = startOfEditableContent(currentPosition);
            VisiblePosition endPosition = endOfEditableContent(currentPosition);
            if (comparePositions(startPosition, endPosition)) {
                m_editorState.isContentEmpty = false;
                if (m_editorState.isContentEditable && !m_editorState.isContentRichlyEditable) {
                    if (baseNode && (!baseNode->isTextNode() || !baseNode->nodeValue().length()))
                        m_editorState.isContentEmpty = true;
                }
            }
        }
#endif
    }
#endif

    if (sendMessage) {
        send(Messages::WebPageProxy::EditorStateChanged(m_editorState));
#if ENABLE(TIZEN_WEBKIT2_SCROLL_POSITION_BUG_FIX)
        m_shouldSyncVisibleContentRect = true;
#endif
    }
}

void WebPage::updateEditorStateTimerFired()
{
    updateEditorState(true);
}
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION) || ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void WebPage::setFocusedInputElementValue(const String& inputValue, bool clear)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->document() || !frame->document()->focusedNode())
        return;

    HTMLInputElement* inputElement = frame->document()->focusedNode()->toInputElement();
    if (!inputElement)
        return;

    inputElement->toNode()->dispatchFocusEvent(0);
    inputElement->setValue(inputValue, DispatchChangeEvent);

    if (clear)
        frame->selection()->clear();
}

void  WebPage::getFocusedInputElementValue(String& inputValue)
{
    inputValue = String();

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->document() || !frame->document()->focusedNode())
        return;

    HTMLInputElement* inputElement = frame->document()->focusedNode()->toInputElement();
    if (!inputElement)
        return;

    inputValue = inputElement->value();
}
#endif

#if ENABLE(TIZEN_DATALIST_ELEMENT)
void WebPage::getFocusedInputElementDataList(Vector<String>& optionList)
{
    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->document())
        return;

    Node* node = frame->document()->focusedNode();
    if (!node)
        return;

    HTMLInputElement* input = node->toInputElement();
    if (!input)
        return;

    HTMLDataListElement* dataList = static_cast<HTMLDataListElement*>(input->list());
    if (!dataList)
        return;

    RefPtr<HTMLCollection> options = static_cast<HTMLDataListElement*>(dataList)->options();
    for (unsigned i = 0; Node* node = options->item(i); i++) {
        HTMLOptionElement* optionElement = static_cast<HTMLOptionElement*>(node);
        String value = optionElement->value();
        optionList.append(value);
    }
}
#endif

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
static bool isClickableOrFocusable(Node* focusableNode)
{

   if (!focusableNode)
       return false;
   if (focusableNode->disabled())
        return false;
   if (!focusableNode->inDocument())
        return false;
   if (!focusableNode->renderer() || focusableNode->renderer()->style()->visibility() != VISIBLE)
        return false;
   if (focusableNode->isFocusable()) {
       if (focusableNode->isLink()
           || focusableNode->hasTagName(HTMLNames::inputTag)
           || focusableNode->hasTagName(HTMLNames::selectTag)
           || focusableNode->hasTagName(HTMLNames::buttonTag))
           return true;
   }
   if (focusableNode->supportsFocus()
       || focusableNode->hasEventListeners(eventNames().clickEvent)
       || focusableNode->hasEventListeners(eventNames().mousedownEvent)
       || focusableNode->hasEventListeners(eventNames().mouseupEvent)) {
       return true;
   }
   return false;
}

#if ENABLE(TIZEN_TAP_ON_LINK_INVOKES_HAND_CURSOR)
static bool invokesHandCursor(Node* node, bool shiftKey, Frame* frame)
{
    if (!node || !node->renderer())
        return false;
    ECursor cursor = node->renderer()->style()->cursor();
    return cursor == CURSOR_POINTER
            || (cursor == CURSOR_AUTO && frame->eventHandler()->useHandCursor(node, node->isLink(), shiftKey));
}
#endif

#if ENABLE(TOUCH_ADJUSTMENT)
void WebPage::getFocusedRect(HitTestResult hitTestResult, Page* page, bool setFocus, Vector<IntRect>& rects, const IntPoint& point, const IntSize& area)
#else
void WebPage::getFocusedRect(HitTestResult hitTestResult, Page* page, bool setFocus, Vector<IntRect>& rects)
#endif
{
    Node* node = hitTestResult.innerNode();
    Frame* mainFrame = page->mainFrame();
#if ENABLE(TIZEN_TAP_ON_LINK_INVOKES_HAND_CURSOR)
    // If the hittest inner node is not focusable and invokes hand cursor but events are handled by its parent,
    // avoid focusing entire parent.
    if (invokesHandCursor(node, false, mainFrame) && !isClickableOrFocusable(node)) {
        while (node->parentNode() && invokesHandCursor(node->parentNode(), false, mainFrame)) {
            // Find best enclosing node which sets hand cursor
            node = node->parentNode();
            if (isClickableOrFocusable(node)) {
                break;
            }
        }
    }
#else
#if ENABLE(TOUCH_ADJUSTMENT)
    IntPoint adustedPoint;
    Node* adjustedNode = 0;
    mainFrame->eventHandler()->bestClickableNodeForTouchPoint(point, IntSize(area.width() / 2, area.height() / 2), adustedPoint, adjustedNode);
    if (!isClickableOrFocusable(node))
        mainFrame->eventHandler()->bestClickableNodeForTouchPoint(point, IntSize(area.width() / 2, area.height() / 2), adustedPoint, adjustedNode);

    if (adjustedNode)
        node = adjustedNode;
#endif
#endif
    if (!node)
        return;

    bool isFocusRingDrawable = false;
    Node* focusableNode = node;
    while (focusableNode) {
        RenderObject* renderer = focusableNode->renderer();
        if (renderer && (renderer->isBody() || renderer->isRenderView() || renderer->isRoot()))
            break;

        //set the focused node if only image
        if (!hitTestResult.absoluteImageURL().isEmpty() && hitTestResult.absoluteLinkURL().isEmpty() && node->hasTagName(HTMLNames::imgTag))
            break;

#if ENABLE(TIZEN_TAP_ON_LINK_INVOKES_HAND_CURSOR)
        if (isClickableOrFocusable(focusableNode) || invokesHandCursor(node, false, mainFrame)) {
#else
        if (isClickableOrFocusable(focusableNode) {
#endif
#if ENABLE(TIZEN_GSTREAMER_VIDEO) && ENABLE(TIZEN_SCREEN_READER)
            // Do not Focus Media Element when no ScreenReader.
            if (renderer->isMedia() && !m_screenReader)
                break;
#endif
            isFocusRingDrawable = true;
            break;
        }

        focusableNode = focusableNode->parentNode();
    }

    // Don't draw focus ring if child is focusable or has trigger
    if (focusableNode && focusableNode->isContainerNode() && !focusableNode->isLink()) {
        WebCore::Node *child = static_cast<const ContainerNode*>(focusableNode)->firstChild();
        while(child) {
            if( child->supportsFocus()
                || child->hasEventListeners(eventNames().clickEvent)
                || child->hasEventListeners(eventNames().mousedownEvent)
                || child->hasEventListeners(eventNames().mouseupEvent)) {
                return;
            }
            child = child->traverseNextNode(focusableNode);
        }
    }

    if (!isFocusRingDrawable) {
        if (!node->hasTagName(HTMLNames::imgTag))
            return;
        focusableNode = node;
    }

    if (setFocus)
        setFocusedNode(focusableNode);

    calcFocusedRects(focusableNode, rects);
}
#endif

#if ENABLE(TOUCH_ADJUSTMENT)
void WebPage::hitTestResultAtPoint(const IntPoint& point, int hitTestMode, const IntSize& area, WebHitTestResult::Data& hitTestResultData)
#else
void WebPage::hitTestResultAtPoint(const IntPoint& point, int hitTestMode, WebHitTestResult::Data& hitTestResultData)
#endif
{
    Frame* frame = m_page->mainFrame();
    FrameView* frameView = frame->view();
    if (!frameView)
        return;

    HitTestResult hitTestResult = frame->eventHandler()->hitTestResultAtPoint(frameView->windowToContents(point), false);
    hitTestResultData.absoluteImageURL = hitTestResult.absoluteImageURL().string();
    hitTestResultData.absoluteLinkURL = hitTestResult.absoluteLinkURL().string();
    hitTestResultData.absoluteMediaURL = hitTestResult.absoluteMediaURL().string();
    hitTestResultData.linkLabel = hitTestResult.textContent();
    hitTestResultData.linkTitle = hitTestResult.titleDisplayString();
    hitTestResultData.isContentEditable = hitTestResult.isContentEditable();
#if ENABLE(TIZEN_DRAG_SUPPORT)
    hitTestResultData.isDragSupport = hitTestResult.isDragSupport();
#endif

    int context = WebHitTestResult::HitTestResultContextDocument;

    if (!hitTestResult.absoluteLinkURL().isEmpty())
        context |= WebHitTestResult::HitTestResultContextLink;
    if (!hitTestResult.absoluteImageURL().isEmpty())
        context |= WebHitTestResult::HitTestResultContextImage;
    if (!hitTestResult.absoluteMediaURL().isEmpty())
        context |= WebHitTestResult::HitTestResultContextMedia;
    if (hitTestResult.isSelected())
        context |= WebHitTestResult::HitTestResultContextSelection;
    if (hitTestResult.isContentEditable())
        context |= WebHitTestResult::HitTestResultContextEditable;
    if (hitTestResult.innerNonSharedNode() && hitTestResult.innerNonSharedNode()->isTextNode())
        context |= WebHitTestResult::HitTestResultContextText;

    hitTestResultData.context = context;
    hitTestResultData.hitTestMode = hitTestMode;

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    bool setFocus = hitTestResultData.hitTestMode & WebHitTestResult::HitTestModeSetFocus;
#if ENABLE(TOUCH_ADJUSTMENT)
    getFocusedRect(hitTestResult, m_page.get(), setFocus, hitTestResultData.focusedRects, point, area);
#else
    getFocusedRect(hitTestResult, m_page.get(), setFocus, hitTestResultData.focusedRects);
    if (!hitTestResult.absoluteImageURL().isEmpty() && hitTestResult.absoluteLinkURL().isEmpty())
        hitTestResultData.nodeData.tagName = static_cast<WebCore::Element*>(hitTestResult.innerNonSharedNode())->tagName();
#endif

    // Don't display FocusRect if the size is too big..
    IntRect framerect = frameView->visibleContentRect(true);
    for (size_t i = 0; i < hitTestResultData.focusedRects.size(); ++i) {
        if (hitTestResultData.focusedRects[i].width() > (0.8 * framerect.width())
            && hitTestResultData.focusedRects[i].height() > (0.8 * framerect.height()))
            hitTestResultData.focusedRects.clear();
    }

    if (hitTestResult.innerNode() && hitTestResult.innerNode()->renderer() && hitTestResult.innerNode()->renderer()->style())
        hitTestResultData.focusedColor = hitTestResult.innerNode()->renderer()->style()->tapHighlightColor();
#endif

    if (hitTestResultData.hitTestMode & WebHitTestResult::HitTestModeNodeData) {
        WebCore::Node* hitNode = hitTestResult.innerNonSharedNode();
        if (hitNode) {
            hitTestResultData.nodeData.nodeValue = hitNode->nodeValue();

            if ((hitTestResultData.context & WebHitTestResult::HitTestResultContextText) && hitNode->parentNode())
                hitNode = hitNode->parentNode(); // if hittest inner node is Text node, fill tagName with parent node's info and fill attributeMap with parent node's attributes.

            if (hitNode->isElementNode()) {
                WebCore::Element* hitElement = static_cast<WebCore::Element*>(hitNode);
                if (hitElement) {
                    hitTestResultData.nodeData.tagName = hitElement->tagName();
                }
            }

            WebCore::NamedNodeMap* namedNodeMap = hitNode->attributes();
            if (namedNodeMap) {
                for (size_t i = 0; i < namedNodeMap->length(); i++) {
                    const WebCore::Attribute* attribute = namedNodeMap->element()->attributeItem(i);
                    String key = attribute->name().toString();
                    String value = attribute->value();
                    hitTestResultData.nodeData.attributeMap.add(key, value);
                }
            }
        }
    }

    if ((hitTestResultData.hitTestMode & WebHitTestResult::HitTestModeImageData) && (hitTestResultData.context & WebHitTestResult::HitTestResultContextImage)) {
        WebCore::Image* hitImage = hitTestResult.image();
        if (hitImage && hitImage->data() && hitImage->data()->data()) {
            hitTestResultData.imageData.data.append(hitImage->data()->data(), hitImage->data()->size());
            hitTestResultData.imageData.fileNameExtension = hitImage->filenameExtension();
        }
    }
}
#endif

#if ENABLE(TIZEN_WEB_STORAGE)
void WebPage::getStorageQuotaBytes(uint64_t callbackID)
{
    uint32_t quota = m_page->group().groupSettings()->localStorageQuotaBytes();
    send(Messages::WebPageProxy::DidGetWebStorageQuotaBytes(quota, callbackID));
}

void WebPage::setStorageQuotaBytes(uint32_t quota)
{
    m_page->group().groupSettings()->setLocalStorageQuotaBytes(quota);
}
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
void WebPage::recordingSurfaceSetEnableSet(bool enable)
{
    m_recordingSurfaceSetSettings = enable;
}
#endif

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
void WebPage::setClipboardDataForPaste(const String& data, const String& type)
{
#if ENABLE(TIZEN_PASTEBOARD)
    // FIXME: Should move to EditorClient like Clipboard
    Pasteboard::generalPasteboard()->setDataWithType(data, type);
#else
    Frame* mainFrame = m_page->mainFrame();
    if (!mainFrame)
        return;

    mainFrame->editor()->client()->setClipboardDataForPaste(data, type);
#endif
}
#endif

void WebPage::suspendJavaScriptAndResources()
{
    Frame* mainFrame = m_page->mainFrame();
    if (!mainFrame)
        return;

    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext())
        frame->document()->suspendScheduledTasks(WebCore::ActiveDOMObject::PageWillBeSuspended);
#if ENABLE(TIZEN_PAUSE_NETWORK)
    mainFrame->loader()->suspendAllLoaders();
#endif
#if ENABLE(TIZEN_PAUSE_REFRESH)
    mainFrame->navigationScheduler()->pause();
#endif
}

void WebPage::resumeJavaScriptAndResources()
{
    Frame* mainFrame = m_page->mainFrame();
    if (!mainFrame)
        return;

    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext())
        frame->document()->resumeScheduledTasks();
#if ENABLE(TIZEN_PAUSE_NETWORK)
    mainFrame->loader()->resumeAllLoaders();
#endif
#if ENABLE(TIZEN_PAUSE_REFRESH)
    mainFrame->navigationScheduler()->startTimer();
#endif
}

void WebPage::suspendAnimations()
{
    Frame* mainFrame = m_page->mainFrame();
    if (!mainFrame)
        return;

    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext())
        frame->animation()->suspendAnimationsForDocument(frame->document());
}

void WebPage::resumeAnimations()
{
    Frame* mainFrame = m_page->mainFrame();
    if (!mainFrame)
        return;

    for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext())
        frame->animation()->resumeAnimationsForDocument(frame->document());
}

#if ENABLE(TIZEN_SYNC_REQUEST_ANIMATION_FRAME)
void WebPage::suspendAnimationController()
{
    if (!m_suspendedAnimationController) {
        Frame* mainFrame = m_page->mainFrame();
        if (!mainFrame)
            return;

        for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext())
            frame->document()->suspendScriptedAnimationControllerCallbacks();

        m_suspendedAnimationController = true;
    }
}

void WebPage::resumeAnimationController()
{
    if (m_suspendedAnimationController) {
        Frame* mainFrame = m_page->mainFrame();
        if (!mainFrame)
            return;

        for (Frame* frame = mainFrame; frame; frame = frame->tree()->traverseNext())
            frame->document()->resumeScriptedAnimationControllerCallbacks();

        m_suspendedAnimationController = false;
    }
}
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
void WebPage::scrollOverflowWithTrajectoryVector(const WebCore::FloatPoint& trajectoryVector)
{
    Frame* frame = m_page->focusController()->overflowScrollTargetFrame();
    if (!frame)
        return;
    frame->eventHandler()->scrollOverflow(trajectoryVector);
}
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
void WebPage::scrollOverflow(const WebCore::FloatPoint& delta, bool& scrolled)
{
    scrolled = m_page->focusController()->overflowScrollTargetFrame()->eventHandler()->scrollOverflow(delta);
}

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SPLIT)
void WebPage::splitScrollOverflow(const WebCore::FloatPoint& delta, WebCore::FloatPoint& remainingOffset)
{
    remainingOffset = m_page->focusController()->overflowScrollTargetFrame()->eventHandler()->splitScrollOverflow(delta);
}
#endif

void WebPage::setPressedNodeAtPoint(const IntPoint& point, bool checkOverflowLayer, bool& pressed, uint32_t& id)
{
    RenderObject* renderer = 0;
    id = 0;
    pressed = m_page->mainFrame()->eventHandler()->setMousePressNodeAtPoint(point, checkOverflowLayer, renderer);
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    if (pressed && renderer)
        id = toWebGraphicsLayer(renderer->enclosingLayer()->layerForScrollingContents())->id();
#endif
}
#endif

void WebPage::executeEditCommandWithArgument(const String& commandName, const String& argument)
{
    executeEditingCommand(commandName, argument);
}

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
void WebPage::suspendPlugin()
{
    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        FrameView* view = frame->view();
        if (!view)
            continue;

        const HashSet<RefPtr<Widget> >* children = view->children();
        ASSERT(children);

        HashSet<RefPtr<Widget> >::const_iterator end = children->end();
        for (HashSet<RefPtr<Widget> >::const_iterator it = children->begin(); it != end; ++it) {
            Widget* widget = (*it).get();
            if (widget->isPluginViewBase()) {
                PluginView* pluginView = static_cast<PluginView*>(widget);
                if (pluginView)
                    pluginView->suspendPlugin();
            }
        }
    }
}

void WebPage::resumePlugin()
{
    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        FrameView* view = frame->view();
        if (!view)
            continue;

        const HashSet<RefPtr<Widget> >* children = view->children();
        ASSERT(children);

        HashSet<RefPtr<Widget> >::const_iterator end = children->end();
        for (HashSet<RefPtr<Widget> >::const_iterator it = children->begin(); it != end; ++it) {
            Widget* widget = (*it).get();
            if (widget->isPluginViewBase()) {
                PluginView* pluginView = static_cast<PluginView*>(widget);
                if (pluginView)
                    pluginView->resumePlugin();
            }
        }
    }
}
#endif

#if ENABLE(TIZEN_MULTIPLE_SELECT)
void WebPage::didChangeSelectedIndexForActivePopupMenuMultiple(Vector<int32_t> newIndex)
{
    if (!m_activePopupMenu)
        return;

    m_activePopupMenu->client()->popupDidHide();

    size_t indexSize = newIndex.size();
    for (size_t i = 0; i < indexSize; i++)
        m_activePopupMenu->didChangeSelectedIndex(newIndex.at(i));

}
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
void WebPage::startOfflinePageSave(String subresourceFolderName)
{
    WebPageSerializerTizen::getSerializedPageContent(this, subresourceFolderName);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
void WebPage::selectClosestWord(const IntPoint& point, bool isAutoWordSelection, bool& result, bool& isCaretSelection)
{
    result = false;
    isCaretSelection = false;

    Frame* mainFrame = m_page->mainFrame();
    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();

    HitTestResult hitTestResult = mainFrame->eventHandler()->hitTestResultAtPoint(m_page->mainFrame()->view()->windowToContents(point), false);

    Node* node = hitTestResult.innerNonSharedNode();
    if (!node)
        return;

    if (node->renderer() && node->renderer()->style() && (node->renderer()->style()->userSelect() == SELECT_NONE))
        return;

     Frame* newFocusFrame = node->document()->frame();
    // Magnifier selection should not move b/w the different frames,avoiding the next frame word selection
    // if current frame has the selection.
    if (newFocusFrame && focusedFrame != newFocusFrame && focusedFrame->selection()->isRange())
        return;

     if (focusedFrame != newFocusFrame) {
         m_page->focusController()->setFocusedFrame(newFocusFrame);
         focusedFrame = newFocusFrame;
     }

    HTMLInputElement* inputElement = node->toInputElement();
#if ENABLE(TIZEN_INPUT_TAG_EXTENSION)
    if (inputElement && inputElement->shouldUsePicker())
        return;
#endif

    bool caretSelection = false;
    if (isAutoWordSelection) {
        if (hitTestResult.isContentEditable() && !node->isTextNode())
            caretSelection = true;
    } else {
        if (hitTestResult.isContentEditable())
            caretSelection = true;
    }

    if (caretSelection) {
        result = setCaretPosition(point);
        isCaretSelection = true;
        return;
    }

    if (!node->isTextNode())
        return;

    for (Node* node = hitTestResult.innerNonSharedNode(); node; node = node->parentNode()) {
        if (node->isFocusable()) {
            // Text selection shoud not be started when text of <button> tag is selected.
            if (node->hasTagName(HTMLNames::buttonTag))
                return;

            if (inputElement && (inputElement->isTextButton() || inputElement->isCheckbox()))
                return;

            break;
        }
    }

    // To check whether Select Start is allowed or not in case selection is disabled by Javascript.
    if (!node->dispatchEvent(Event::create(eventNames().selectstartEvent, true, true)))
        return;

    FrameSelection* frameSelection = focusedFrame->selection();

    VisiblePosition position = mainFrame->visiblePositionForPoint(point);
    VisibleSelection selection(position);

    // This changes just the 'start' and 'end' positions of the VisibleSelection
    selection.expandUsingGranularity(WordGranularity);

    FrameSelection::SetSelectionOptions options = FrameSelection::CloseTyping | FrameSelection::ClearTypingStyle | FrameSelection::DoNotSetFocus;
    frameSelection->setSelection(VisibleSelection(selection.start(), selection.end()), options);

#if ENABLE(TIZEN_ISF_PORT) || ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (m_updateEditorStateTimer.isActive()) {
        m_updateEditorStateTimer.stop();
        updateEditorState(false);

        m_editorState.selectionByUserAction = true;
        send(Messages::WebPageProxy::EditorStateChanged(m_editorState));
    }
#endif

    if (!frameSelection->isRange() && !m_editorState.isContentEditable)
        return;

    // This changes just the 'start' and 'end' positions of the VisibleSelection
    // Find handlers positions
    if (m_editorState.leftSelectionRect.isEmpty() && m_editorState.rightSelectionRect.isEmpty()) {
        // Sometimes there is no selected text, but isNone() returns TRUE
        // in this case ewk_frame_selection_handlers_get() returns FALSE and handlers are invalid
        // Workaround - clear the selection.
        // Better solution would be to modify the ewk_frame_select_closest_word()
        // to not select anything in the first place (for example - don't call setSelection()
        // if there is nothing under the cursor).
        selectionClearAllSelection(m_page->mainFrame());
        return;
    }

    result = true;
}

void WebPage::setLeftSelection(const IntPoint& point, const int direction, int& result)
{
    result = HandleMovingDirectionNone;

    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
    FrameSelection* frameSelection = focusedFrame->selection();
    if (!frameSelection->isRange())
        return;

    Node* selectionEndNode = frameSelection->end().deprecatedNode();
    if (!selectionEndNode || !selectionEndNode->renderer())
        return;

    FrameView* frameView = focusedFrame->view();
    if (!frameView)
        return;

    IntPoint pos = frameView->windowToContents(point);

    if (selectionEndNode->rendererIsEditable() && !selectionEndNode->rendererIsRichlyEditable()) {
        const int boundariesWidth = 2;

        IntRect rect = frameSelection->caretRenderer()->absoluteBoundingBoxRect(true);
        // here we cheat input field that we actually are just inside of if
        if (pos.y() < rect.y() + boundariesWidth)
            pos.setY(rect.y() + boundariesWidth);
        else if (pos.y() >= rect.maxY() - boundariesWidth)
            pos.setY(rect.maxY() - boundariesWidth - 1);
    }

    OwnPtr<VisiblePosition> position = adoptPtr(new VisiblePosition(focusedFrame->visiblePositionForPoint(pos)));
    Position base = frameSelection->base();
    Position extent = frameSelection->extent();

    Node* newSelectionStartNode = position->deepEquivalent().deprecatedNode();

    if (newSelectionStartNode && newSelectionStartNode->document()->frame() != selectionEndNode->document()->frame())
        return;
    // Avoiding the selection moving b/w the different root editable element
    if (selectionEndNode->rootEditableElement() != VisibleSelection(focusedFrame->visiblePositionForPoint(pos)).rootEditableElement())
        return;

    // both start and end nodes should be in the same area type: both should be editable or both should be not editable
    // Check if the new position is before the extent's position
    if (newSelectionStartNode
        && selectionEndNode->isContentEditable() == newSelectionStartNode->isContentEditable()) {
        // Change the 'base' and 'extent' positions to 'start' and 'end' positions.
        // We do it, because without this, the other modification of the selection
        // would destroy the 'start' and/or 'end' positions and set them to
        // the 'base'/'extent' positions accordingly
        VisibleSelection sel(frameSelection->start(), frameSelection->end());
        frameSelection->setSelection(sel);

        bool oldProhibitsScrolling = focusedFrame->view()->prohibitsScrolling();
        focusedFrame->view()->setProhibitsScrolling(true);

        if (direction == HandleMovingDirectionNormal) {
            if (comparePositions(position->deepEquivalent(), extent) < 0) {
                frameSelection->setBase(*position);
                result = HandleMovingDirectionNormal;
            } else if (comparePositions(position->deepEquivalent(), extent) > 0) {
                frameSelection->setExtent(*position);
                frameSelection->setBase(extent);
                result = HandleMovingDirectionReverse;
            }
        } else if (direction == HandleMovingDirectionReverse) {
            if (comparePositions(position->deepEquivalent(), base) > 0) {
                frameSelection->setExtent(*position);
                result = HandleMovingDirectionReverse;
            } else if (comparePositions(position->deepEquivalent(), base) < 0) {
                frameSelection->setBase(*position);
                frameSelection->setExtent(base);
                result = HandleMovingDirectionNormal;
            }
        }

        focusedFrame->view()->setProhibitsScrolling(oldProhibitsScrolling);
        // This forces webkit to show selection
        // m_coreFrame->invalidateSelection();
    }
}

void WebPage::setRightSelection(const IntPoint& point, const int direction, int& result)
{
    result = HandleMovingDirectionNone;

    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
    FrameSelection* frameSelection = focusedFrame->selection();

    if (!frameSelection->isRange())
        return;

    Node* selectionStartNode = frameSelection->start().deprecatedNode();
    if (!selectionStartNode || !selectionStartNode->renderer())
        return;

    FrameView* frameView = focusedFrame->view();
    if (!frameView)
        return;

    IntPoint pos = frameView->windowToContents(point);
    if (selectionStartNode->rendererIsEditable() && !selectionStartNode->rendererIsRichlyEditable()) {
        const int boundariesWidth = 2;

        IntRect rect = frameSelection->caretRenderer()->absoluteBoundingBoxRect(true);
        // here we cheat input field that we actually are just inside of if
        if (pos.y() < rect.y() + boundariesWidth)
            pos.setY(rect.y() + boundariesWidth);
        else if (pos.y() >= rect.maxY() - boundariesWidth)
            pos.setY(rect.maxY() - boundariesWidth - 1);
    }

    OwnPtr<VisiblePosition> position = adoptPtr(new VisiblePosition(focusedFrame->visiblePositionForPoint(pos)));
    Position base = frameSelection->base();
    Position extent = frameSelection->extent();

    Node* newSelectionEndNode = position->deepEquivalent().deprecatedNode();

    // both start and end nodes should be in the same area type: both should be editable or both should be not editable
    // Check if the new position is after the base's position

    if (newSelectionEndNode && selectionStartNode->document()->frame() != newSelectionEndNode->document()->frame())
        return;
    // Avoiding the selection moving b/w the different root editable element
    if (selectionStartNode->rootEditableElement() != VisibleSelection(focusedFrame->visiblePositionForPoint(pos)).rootEditableElement())
        return;

    if (newSelectionEndNode
        && selectionStartNode->isContentEditable() == newSelectionEndNode->isContentEditable()) {
        // Change the 'base' and 'extent' positions to 'start' and 'end' positions.
        // We do it, because without this, the other modifications of the selection
        // would destroy the 'start' and/or 'end' positions and set them to
        // the 'base'/'extent' positions accordingly

        VisibleSelection sel(frameSelection->start(), frameSelection->end());
        frameSelection->setSelection(sel);

        bool oldProhibitsScrolling = focusedFrame->view()->prohibitsScrolling();
        focusedFrame->view()->setProhibitsScrolling(true);

        if (direction == HandleMovingDirectionNormal) {
            if (comparePositions(position->deepEquivalent(), base) > 0) {
                frameSelection->setExtent(*position);
                result = HandleMovingDirectionNormal;
            } else if (comparePositions(position->deepEquivalent(), base) < 0) {
                frameSelection->setBase(*position);
                frameSelection->setExtent(base);
                result = HandleMovingDirectionReverse;
            }
        } else if (direction == HandleMovingDirectionReverse) {
            if (comparePositions(position->deepEquivalent(), extent) < 0) {
                frameSelection->setBase(*position);
                result = HandleMovingDirectionReverse;
            } else if (comparePositions(position->deepEquivalent(), extent) > 0) {
                frameSelection->setExtent(*position);
                frameSelection->setBase(extent);
                result = HandleMovingDirectionNormal;
            }
        }

        focusedFrame->view()->setProhibitsScrolling(oldProhibitsScrolling);
    }
}

bool WebPage::isSelectionOnlyImage() const
{
    bool isImage = false;
    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
    if (!focusedFrame->selection()->isRange())
        return  isImage;

    RefPtr<Range> selectedRange = focusedFrame->selection()->toNormalizedRange();
    Vector<IntRect> rects;
    selectedRange->boundingBoxEx(rects, true);
    unsigned size = rects.size();
    if(size == 1) {
        Node* startNode = selectedRange->startContainer();
        Node* endNode = selectedRange->endContainer();
        if(startNode == endNode) {
            if(endNode && endNode->isContainerNode() && !(endNode->hasTagName(HTMLNames::imgTag))) {
                WebCore::Node *child = static_cast<const ContainerNode*>(endNode)->firstChild();
                while(child) {
                    if(child->hasTagName(HTMLNames::imgTag)) {
                        isImage = true;
                        break;
                    }
                    child = child->traverseNextNode(endNode);
                }
            }
            else {
                if(endNode->hasTagName(HTMLNames::imgTag))
                    isImage = true;
            }
        }
    }
    return isImage;
}

void WebPage::getSelectionHandlers(IntRect& leftRect, IntRect& rightRect)
{
    leftRect = IntRect();
    rightRect = IntRect();

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame || !frame->selection()->isRange())
        return;

    bool isL2RDirection;
    calcSelectionRects(frame, m_editorState.editorRect, false, leftRect, rightRect, isL2RDirection);
}

void WebPage::getSelectionText(String& result)
{
    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
    result = focusedFrame->editor()->selectedText();
}

void WebPage::selectionRangeClear()
{
    selectionClearAllSelection(m_page->mainFrame());
}

void WebPage::selectionClearAllSelection(Frame* frame)
{
    if (!frame)
        return;

    FrameSelection* frameSelection = frame->selection();

    if (frameSelection) {
        if (frame == m_page->focusController()->focusedFrame()
            && frameSelection->isRange()
            && frameSelection->isContentEditable()) {
            VisiblePosition visiblePos(frameSelection->extent());
            if (visiblePos.isNull())
                return;

            frame->editor()->setIgnoreCompositionSelectionChange(true);
            frameSelection->setSelection(VisibleSelection(visiblePos), CharacterGranularity);
            frame->editor()->setIgnoreCompositionSelectionChange(false);

            frameSelection->setCaretBlinkingSuspended(false);
        } else if (!frameSelection->isCaret())
            frameSelection->clear();
    }

    if (!frame->tree())
        return;

    if (frame->tree()->childCount() > 0) {
        if (frame->tree()->firstChild())
            selectionClearAllSelection(frame->tree()->firstChild());
    } else if (frame->tree()->nextSibling())
        selectionClearAllSelection(frame->tree()->nextSibling());
}

void WebPage::scrollContentByCharacter(const IntPoint&, int direction, bool extend, bool& result)
{
    result = false;

    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
    if (!focusedFrame)
        return;

    FrameSelection* frameSelection = focusedFrame->selection();
    if (!frameSelection)
        return;

    FrameSelection::EAlteration alteration = (extend) ? FrameSelection::AlterationExtend : FrameSelection::AlterationMove;

    VisiblePosition currentPosition = frameSelection->selection().visibleStart();
    if (direction) {
        if (isStartOfLine(currentPosition))
            return;

        focusedFrame->selection()->modify(alteration, DirectionBackward, CharacterGranularity, UserTriggered);
    } else {
        if (isEndOfLine(currentPosition))
            return;

        focusedFrame->selection()->modify(alteration, DirectionForward, CharacterGranularity, UserTriggered);
    }
}

void WebPage::scrollContentByLine(const IntPoint&, int direction, bool& result)
{
    result = false;

    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
    if (!focusedFrame)
        return;

    FrameSelection* frameSelection = focusedFrame->selection();
    if (!frameSelection)
        return;

    VisiblePosition currentPosition = frameSelection->selection().visibleStart();
    if (direction) {
        if (inSameLine(currentPosition, previousLinePosition(currentPosition, 0)))
            return;

        focusedFrame->selection()->modify(FrameSelection::AlterationMove, DirectionBackward, LineGranularity, UserTriggered);
    } else {
        if (inSameLine(currentPosition, nextLinePosition(currentPosition, 0)))
            return;

        focusedFrame->selection()->modify(FrameSelection::AlterationMove, DirectionForward, LineGranularity, UserTriggered);
    }
}
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
void WebPage::getLinkMagnifierRect(const IntPoint& position)
{
    IntRect rect = LinkMagnifier::rect(this, position);
    if (rect.isEmpty())
        send(Messages::WebPageProxy::DidGetLinkMagnifierRect(rect.location(), IntRect()));
    else
        send(Messages::WebPageProxy::DidGetLinkMagnifierRect(position, rect));
}
#endif

#if ENABLE(TIZEN_SCREEN_READER)
void WebPage::moveScreenReaderFocus(bool forward, bool& result)
{
    if (!m_screenReader)
        m_screenReader = ScreenReader::create(this);

    result = m_screenReader->moveFocus(forward);
}

void WebPage::moveScreenReaderFocusByPoint(const IntPoint& point, bool reload)
{
    if (!m_screenReader)
        m_screenReader = ScreenReader::create(this);

    m_screenReader->moveFocus(point, reload);
}

void WebPage::clearScreenReaderFocus()
{
    if (!m_screenReader)
        return;

    m_screenReader->clearFocus();
}

void WebPage::raiseTapEvent(const IntPoint& position, const IntPoint& globalPosition)
{
    bool result = false;
    EditorState editorState;
#if ENABLE(GESTURE_EVENTS)
    Frame* frame = m_page->mainFrame();
    if (!frame->view())
        return;

    Vector<WebPlatformTouchPoint> touchPoints;
    touchPoints.append(WebPlatformTouchPoint(0, WebPlatformTouchPoint::TouchPressed, globalPosition, position));

    WebTouchEvent touchStartEvent(WebEvent::TouchStart, touchPoints, WebEvent::Modifiers(0), ecore_time_get());
    bool handled = frame->eventHandler()->handleTouchEvent(platform(touchStartEvent));

    touchPoints.at(0).setState(WebPlatformTouchPoint::TouchReleased);

    WebTouchEvent touchEndEvent(WebEvent::TouchEnd, touchPoints, WebEvent::Modifiers(0), ecore_time_get());
    handled |= frame->eventHandler()->handleTouchEvent(platform(touchEndEvent));

    if (!handled) {
        WebGestureEvent gestureEvent(WebEvent::GestureSingleTap, position, globalPosition, WebEvent::Modifiers(0), ecore_time_get());
        frame->eventHandler()->handleGestureEvent(platform(gestureEvent));
        result = true;
    }
#endif

#if ENABLE(TIZEN_ISF_PORT)
    if (m_updateEditorStateTimer.isActive()) {
        m_updateEditorStateTimer.stop();
        updateEditorStateTimerFired();
    }

    editorState = m_editorState;
#endif

    send(Messages::WebPageProxy::DidRaiseTapEvent(result,editorState));
}

void WebPage::adjustScreenReaderFocusedObjectValue(bool up)
{
    if (!m_screenReader || !m_focusedNode || !m_focusedNode->toInputElement())
        return;

    ExceptionCode ec;
    if (up)
        m_focusedNode->toInputElement()->stepUp(ec);
    else
        m_focusedNode->toInputElement()->stepDown(ec);
}

void WebPage::updateScreenReaderFocus(RenderObject* object)
{
    if (!m_screenReader)
        return;

    if (!object)
        m_screenReader->clearFocus();
    else if (!m_screenReader->rendererWillBeDestroyed(object))
        return;
}

void WebPage::clearScreenReader()
{
    m_screenReader.clear();
}

void WebPage::extendParagraphOnScreenReader(int direction, bool& result)
{
    result = true;

    Frame* focusedFrame = m_page->focusController()->focusedOrMainFrame();
    if (!focusedFrame) {
        result = false;
        return;
    }

    FrameSelection* frameSelection = focusedFrame->selection();
    if (!frameSelection) {
        result = false;
        return;
    }

    if (!frameSelection->isContentEditable() && !frameSelection->isContentRichlyEditable()) {
        result = false;
        return;
    }

    VisiblePosition currentPosition = frameSelection->selection().visibleStart();
    if (direction) {
        if (inSameLine(currentPosition, previousLinePosition(currentPosition, 0))) {
            result = false;
            return;
        }
    } else {
        if (inSameLine(currentPosition, nextLinePosition(currentPosition, 0))) {
            result = false;
            return;
        }
    }

    bool firstTime = true;
    do {
        currentPosition = frameSelection->selection().visibleStart();
        if (direction) {
            if (inSameLine(currentPosition, previousLinePosition(currentPosition, 0)))
                return;

            if (!firstTime || frameSelection->selection().isRange()) {
                focusedFrame->selection()->modify(FrameSelection::AlterationMove, DirectionBackward, ParagraphBoundary, UserTriggered);
                focusedFrame->selection()->modify(FrameSelection::AlterationMove, DirectionBackward, ParagraphGranularity, UserTriggered);
            }
            focusedFrame->selection()->modify(FrameSelection::AlterationMove, DirectionBackward, ParagraphBoundary, UserTriggered);
            focusedFrame->selection()->modify(FrameSelection::AlterationExtend, DirectionForward, ParagraphBoundary, UserTriggered);
        } else {
            if (inSameLine(currentPosition, nextLinePosition(currentPosition, 0)))
                return;

            if (!firstTime || frameSelection->selection().isRange()) {
                focusedFrame->selection()->modify(FrameSelection::AlterationMove, DirectionBackward, ParagraphBoundary, UserTriggered);
                focusedFrame->selection()->modify(FrameSelection::AlterationMove, DirectionForward, ParagraphGranularity, UserTriggered);
            }
            focusedFrame->selection()->modify(FrameSelection::AlterationMove, DirectionBackward, ParagraphBoundary, UserTriggered);
            focusedFrame->selection()->modify(FrameSelection::AlterationExtend, DirectionForward, ParagraphBoundary, UserTriggered);
        }
        firstTime = false;
    } while (!frameSelection->selection().isRange());

    send(Messages::WebPageProxy::DidScreenReaderTextChanged(focusedFrame->editor()->selectedText()));
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING) || ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
static bool isVisible(RenderObject* renderer)
{
    return (renderer->style() && renderer->style()->visibility() == VISIBLE);
}

static void addFocusedRects(Node* node, const LayoutPoint& frameOffset, bool force, Vector<IntRect>& rects)
{
    RenderObject* renderer;
    Vector<LayoutRect> candidateRects;

    if (node->hasTagName(HTMLNames::areaTag)) {
        HTMLAreaElement* area = static_cast<HTMLAreaElement*>(node);
        HTMLImageElement* image = area->imageElement();
        renderer = image ? image->renderer() : 0;
        if (!renderer || (!force && !isVisible(renderer)))
            return;

        candidateRects.append(area->computeRect(renderer));
    } else {
        renderer = node->renderer();
        if (!renderer || (!force && !isVisible(renderer)))
            return;

        Vector<FloatQuad> quads;
        renderer->absoluteQuads(quads);

        for (size_t i = 0; i < quads.size(); ++i)
            candidateRects.append(enclosingLayoutRect(quads[i].boundingBox()));
    }

    if (candidateRects.isEmpty())
        return;

    RenderLayer* layer = renderer->enclosingLayer();
    while (layer) {
        RenderObject* layerRenderer = layer->renderer();
        layer = layer->parent();

        if (layerRenderer->hasOverflowClip()) {
            FloatRect boxRect = toRenderBox(layerRenderer)->borderBoxRect();
            LayoutRect currentRect = enclosingLayoutRect(layerRenderer->localToAbsoluteQuad(boxRect).boundingBox());
            for (size_t i = 0; i < candidateRects.size(); ++i)
                candidateRects[i].intersect(currentRect);
        }
    }

    for (size_t i = 0; i < candidateRects.size(); ++i) {
        LayoutRect rect = candidateRects[i];
        if (rect.pixelSnappedX() < 0)
            rect.shiftXEdgeTo(0);
        if (rect.pixelSnappedY() < 0)
            rect.shiftYEdgeTo(0);

        if (!rect.isEmpty()) {
            rect.moveBy(frameOffset);
            rects.append(enclosingIntRect(rect));
        }
    }
}

void WebPage::calcFocusedRects(Node* root, Vector<IntRect>& rects) const
{
    if (!root || !root->renderer())
        return;

    LayoutPoint frameOffset = offsetOfFrame(root->document()->frame());
    Node* node = root;

    while (node) {
        RenderObject* childRenderer = node->renderer();
        if (childRenderer && !childRenderer->isRenderInline())
            addFocusedRects(node, frameOffset, (node == root), rects);

        // FIXME: Rect of text in button element which has line-height css property
        // can be overflowed rect of button. In this case, we should display focus ring
        // on the button element only. (TNEF-4709)
        if (root == node && toElement(node)->tagName() == "BUTTON")
            break;

        node = node->traverseNextNode(root);
    }
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
void WebPage::setFocusedNode(Node* node)
{
    m_focusedNode = node;
    didFocusedRectsChanged();
}

void WebPage::didFocusedRectsChanged()
{
    Vector<IntRect> rects;
    calcFocusedRects(m_focusedNode.get(), rects);
    m_focusedRects = rects;
    send(Messages::WebPageProxy::DidFocusedRectsChanged(m_focusedRects));
}

void WebPage::recalcFocusedRects()
{
    if (!m_focusedNode)
        return;

    didFocusedRectsChanged();
}

void WebPage::clearFocusedNode()
{
    if (m_focusedNode)
        setFocusedNode(0);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_POPUP_INTERNAL)
// FIXME: Currently with cached pages, hiding Popup list menu is not working correctly.
// This patch is a fix allowing any popup list menu to get close for any page navigation.
void WebPage::notifyTransitionToCommitted(bool forNewPage)
{
    if (m_activePopupMenu) {
        TIZEN_LOGI("");
        m_activePopupMenu->hide();
        m_activePopupMenu = 0;
    }
}
#endif

#if ENABLE(TIZEN_CSP)
void WebPage::setContentSecurityPolicy(const String& policy, uint32_t headerType)
{
    // Keep the CSP with the Page, so that it can be accessed by widgets
    // or webapps who do not get the CSP using the usual HTTP header route.
    m_page->setContentPolicy(policy, static_cast<WebCore::ContentSecurityPolicy::HeaderType>(headerType));

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
    if (!frame)
        return;

    Document* document = frame->document();
    if (!document)
        return;

    document->contentSecurityPolicy()->didReceiveHeader(m_page->contentPolicyString(), m_page->headerType());
}
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
void WebPage::setIndexedDatabaseDirectory(const String& path)
{
    m_page->group().groupSettings()->setIndexedDBDatabasePath(path);
}
#endif

#if ENABLE(TIZEN_WEB_STORAGE)
void WebPage::setLocalStorageDirectory(const String& path)
{
    m_page->settings()->setLocalStorageDatabasePath(path);
}
#endif

#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
void WebPage::setTouchEventTargetRects(const Vector<IntRect>& touchEventTargetRects)
{
    send(Messages::WebPageProxy::SetTouchEventTargetRects(touchEventTargetRects));
}
#endif

#if ENABLE(TIZEN_USE_SETTINGS_FONT)
void WebPage::useSettingsFont()
{
    if (!WebCore::fontCache()->isFontFamliyTizen())
        return;

    FcInitReinitialize();
    WebCore::fontCache()->invalidate();

    int pageCapacity = WebCore::pageCache()->capacity();
    // Setting size to 0, makes all pages be released.
    WebCore::pageCache()->setCapacity(0);
    WebCore::pageCache()->releaseAutoreleasedPagesNow();
    WebCore::pageCache()->setCapacity(pageCapacity);

    Frame* frame = m_mainFrame->coreFrame();
    if (!frame)
        return;

    if (frame->document())
        frame->document()->dispatchWindowEvent(Event::create(eventNames().resizeEvent, false, false));

    FrameView* frameView = frame->view();
    if (!frameView)
        return;

    frameView->forceLayout();
}
#endif

#if ENABLE(TIZEN_GET_COOKIES_FOR_URL)
void WebPage::getCookiesForURL(const String& url, String& cookiesForURL)
{
    cookiesForURL = cookieRequestHeaderFieldValue(m_mainFrame->coreFrame()->document(), KURL(KURL(), url));
}
#endif

void WebPage::updateRectsTimerFired()
{
    if (!m_page)
        return;

    Frame* frame = m_page->focusController()->focusedOrMainFrame();
     if (!frame || !frame->view())
        return;

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (updateEditorStateRect(frame, m_editorState)) {
        m_editorState.updateEditorRectOnly = true;
        send(Messages::WebPageProxy::EditorStateChanged(m_editorState));
    }
#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    didFocusedRectsChanged();

#if ENABLE(TIZEN_SCREEN_READER)
    Node* screenReaderNode = m_screenReader ? m_screenReader->focusedNode() : 0;
    if (screenReaderNode) {
        Vector<IntRect> screenReaderRects;
        if (screenReaderNode == m_focusedNode)
            screenReaderRects = m_focusedRects;
        else
            calcFocusedRects(screenReaderNode, screenReaderRects);
        send(Messages::WebPageProxy::DidScreenReaderRectsChanged(screenReaderRects));
    }
#endif
#endif
}

void WebPage::didChangeContents()
{
    if (!m_page || m_updateRectsTimer.isActive())
        return;

    m_updateRectsTimer.startOneShot(0);
}

#if ENABLE(TIZEN_PROD_DETECT_CONTENTS)
bool WebPage::detectContents(const IntPoint& position, String& contents, IntRect& detectedContentsRect)
{
    if (!m_page)
        return false;

    Frame* frame = m_page->mainFrame();
    if (!frame)
        return false;

    FrameView* frameView = frame->view();
    if (!frameView)
        return false;

    HitTestResult hitTestResult = frame->eventHandler()->hitTestResultAtPoint(frameView->windowToContents(position), false);

    m_page->contextMenuController()->setHitTestResult(hitTestResult);

    if (hitTestResult.isContentEditable())
        return false;

    Node* hitNode = hitTestResult.innerNode();
    if (!hitNode || !hitNode->isTextNode())
        return false;

    // Ignore when tapping on links or nodes listening to click events, unless the click event is on the
    // body element, in which case it's unlikely that the original node itself was intended to be clickable.
    for (Node* node = hitNode; node && !node->hasTagName(HTMLNames::bodyTag); node = node->parentNode()) {
        if (node->isLink() || node->willRespondToMouseClickEvents())
            return false;
    }

    const int maximumContentsLength = 50;
    SurroundingText surroundingText(hitNode->renderer()->positionForPoint(hitTestResult.localPoint()), maximumContentsLength * 2);
    String content = surroundingText.content();
    if (content.isEmpty())
        return false;
    int selectedOffset = surroundingText.positionOffsetInContent();
    int startOffsetInContent;
    int endOffsetInContent;

    char* detectedContents = 0;
    char* inputContents = static_cast<char*>(calloc(sizeof(char), (content.utf8().length() + 1)));
    if (inputContents)
        strcpy(inputContents, content.utf8().data());

    if (!wkext_contents_detect(inputContents, selectedOffset, &startOffsetInContent, &endOffsetInContent, &detectedContents))
        return false;

    RefPtr<Range> matchRange;
    matchRange = surroundingText.rangeFromContentOffsets(startOffsetInContent, endOffsetInContent);

    if (!matchRange || !detectedContents)
        return false;

    contents = String::fromUTF8(detectedContents);
    detectedContentsRect = frameView->contentsToWindow(matchRange->boundingBox());

    if (inputContents)
        free(inputContents);
    if (detectedContents)
        free(detectedContents);

    setFocusedNode(0);
    return true;
}
#endif

#if ENABLE(TIZEN_FOCUS_UI)
void WebPage::setFocusUIEnabled(bool enabled)
{
    m_page->settings()->setSpatialNavigationEnabled(enabled);

    if (m_focusedNode && m_focusedNode->renderer())
        m_focusedNode->renderer()->repaint();

    if (enabled) {
        Frame* frame = m_page->focusController()->focusedOrMainFrame();
        if (frame) {
            PlatformMouseEvent fakeMouseMove(IntPoint(-1, -1), IntPoint(-1, -1), NoButton, PlatformEvent::MouseMoved, 0, false, false, false, false, currentTime());
            frame->eventHandler()->mouseMoved(fakeMouseMove);
        }

        setFocusedNode(frame && frame->document() ? frame->document()->focusedNode() : 0);
        if (!m_focusedNode)
            m_page->focusController()->advanceFocus(FocusDirectionForward, 0);
    } else
        setFocusedNode(0);
}
#endif

#if ENABLE(TIZEN_BROWSER_FONT)
void WebPage::setBrowserFont()
{
    WebCore::fontCache()->setFontFamliyTizenBrowser();
}
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void WebPage::handleMediaKey(int key)
{
    MediaResourceControllerGStreamerTizen::mediaResourceController().handleMediaKey(key);
}
#endif

#endif // #if PLATFORM(TIZEN)

} // namespace WebKit
