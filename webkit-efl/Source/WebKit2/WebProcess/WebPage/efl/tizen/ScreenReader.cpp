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

#include "config.h"
#include "ScreenReader.h"

#if ENABLE(TIZEN_SCREEN_READER)

#include "LocalizedStrings.h"
#include "WebPage.h"

#include <WebCore/AccessibilityObject.h>
#include <WebCore/Document.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/HTMLButtonElement.h>
#include <WebCore/HTMLFormControlElement.h>
#include <WebCore/HTMLFrameOwnerElement.h>
#include <WebCore/HTMLImageElement.h>
#include <WebCore/HTMLInputElement.h>
#include <WebCore/HTMLOptionElement.h>
#include <WebCore/HTMLOutputElement.h>
#include <WebCore/HTMLSelectElement.h>
#include <WebCore/HTMLTextAreaElement.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformScreen.h>
#include <WebCore/RenderObject.h>
#include <WebCore/Text.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace WebKit {

IntSize ScreenReader::s_hitTestPadding = IntSize();

ScreenReader::ScreenReader(WebPage* page)
    : m_page(page)
    , m_focusedObject(0)
    , m_hasFocus(false)
    , m_isForward(true)
{
    static bool initialized = false;
    if (!initialized) {
        AXObjectCache::enableAccessibility();
        int unit = static_cast<int>(screenRect(0).width() / 10);
        s_hitTestPadding.setWidth(unit);
        s_hitTestPadding.setHeight(unit);
        initialized = true;
    }
}

ScreenReader::~ScreenReader()
{
}

RenderObject* ScreenReader::traverse(RenderObject* object)
{
    if (m_isForward) {
        if (object->firstChild())
            return object->firstChild();
    } else {
        if (object->lastChild())
            return object->lastChild();
    }

    return traverseSibling(object);
}

RenderObject* ScreenReader::traverseSibling(RenderObject* object)
{
    if (m_isForward) {
        do {
            if (object->nextSibling())
                return object->nextSibling();
        } while ((object = object->parent()));
    } else {
        do {
            if (object->previousSibling())
                return object->previousSibling();
        } while ((object = object->parent()));
    }

    return 0;
}

RenderObject* ScreenReader::ownerElementSibling(RenderObject* object)
{
    Node* node;
    while ((node = object->document()->ownerElement())) {
        object = node->renderer();
        RenderObject* next = traverseSibling(object);
        if (next)
            return next;
    }

    return 0;
}

static RenderObject* contentDocumentBody(RenderObject* object)
{
    HTMLFrameOwnerElement* ownerElement = toFrameOwnerElement(object->node());
    if (!ownerElement || !ownerElement->contentDocument())
        return 0;

    return ownerElement->contentDocument()->body()->renderer();
}

static RenderObject* lastLeafChild(RenderObject* object)
{
    while (object->lastChild())
        object = object->lastChild();
    return object;
}

static AccessibilityObject* visibleAXObject(RenderObject* object)
{
    if (!object || !object->node() || object->style()->visibility() != VISIBLE)
        return 0;
    AccessibilityObject* axObject = object->document()->axObjectCache()->getOrCreate(object);
    if (!axObject)
        return 0;
    IntRect boundingRect = axObject->pixelSnappedBoundingBoxRect();
    return (boundingRect.maxX() > 0 && boundingRect.maxY() > 0) ? axObject : 0;
}

static bool isSpace(UChar character)
{
    return isASCIISpace(character) || character == noBreakSpace;
}

static bool containsOnlyWhitespace(const String& string)
{
    StringImpl* impl = string.impl();
    if (!impl)
        return true;

    unsigned length = impl->length();
    if (impl->is8Bit()) {
        const LChar* data = impl->characters8();
        for (unsigned i = 0; i < length; i++) {
            if (!isSpace(data[i]))
                return false;
        }
    } else {
        const UChar* data = impl->characters16();
        for (unsigned i = 0; i < length; i++) {
            if (!isSpace(data[i]))
                return false;
        }
    }

    return true;
}

static bool isAriaFocusable(Node* node)
{
    AccessibilityObject* axObject = node->document()->axObjectCache()->getOrCreate(node->renderer());
    if (!axObject)
        return false;

    switch (axObject->roleValue()) {
    case ButtonRole:
    case CheckBoxRole:
    case ComboBoxRole:
    case MenuRole:
    case MenuBarRole:
    case MenuItemRole:
    case LinkRole:
    case ListBoxOptionRole:
    case ProgressIndicatorRole:
    case RadioButtonRole:
    case ScrollBarRole:
    case SliderRole:
    case TreeRole:
    case TreeGridRole:
    case TreeItemRole:
    case WebCoreLinkRole:
        return true;
    default:
        break;
    }

    if (!containsOnlyWhitespace(axObject->ariaLabeledByAttribute())
        || !containsOnlyWhitespace(axObject->getAttribute(aria_labelAttr)))
        return true;

    return false;
}

static bool hasText(Node* node)
{
    return !containsOnlyWhitespace(node->nodeValue());
}

static bool isFocusable(Node* node)
{
    if (node->isFocusable() || isAriaFocusable(node))
        return true;
    return false;
}

static bool isAriaReadable(Node* node)
{
    AccessibilityObject* axObject = node->document()->axObjectCache()->getOrCreate(node->renderer());
    if (!axObject)
        return false;

    switch (axObject->roleValue()) {
    case ImageRole:
        return true;
    default:
        break;
    }
    return false;
}

static bool isReadable(Node* node)
{
    if (!(node->isTextNode() && hasText(node)) && !isHTMLImageElement(node) && !isAriaReadable(node))
        return false;

    for (Node* parent = node->parentOrHostNode(); parent; parent = parent->parentOrHostNode()) {
        if (isFocusable(parent))
            return false;
    }

    return true;
}

static bool isAriaHidden(AccessibilityObject* axObject)
{
    if (equalIgnoringCase(axObject->getAttribute(aria_hiddenAttr), "true"))
        return true;

    while ((axObject = axObject->parentObject())) {
        if (equalIgnoringCase(axObject->getAttribute(aria_hiddenAttr), "true"))
            return true;
    }

    return false;
}

RenderObject* ScreenReader::findFocusable(RenderObject* object)
{
    do {
        AccessibilityObject* axObject = visibleAXObject(object);
        if (!axObject)
            continue;

        Node* node = object->node();
        if (node->isFrameOwnerElement()) {
            RenderObject* body = contentDocumentBody(object);
            if (body) {
                RenderObject* result = findFocusable(m_isForward ? body : lastLeafChild(body));
                if (result)
                    return result;
            }
        } else if ((!m_focusedObject || node != m_focusedObject->node()) && (isFocusable(node) || isReadable(node)) && !isAriaHidden(axObject))
            return object;
    } while ((object = traverse(object)));

    return 0;
}

bool ScreenReader::moveFocus(bool forward)
{
    m_isForward = forward;
    m_page->mainFrame()->document()->updateLayoutIgnorePendingStylesheets();

    RenderObject* object;
    if (!m_focusedObject) {
        object = m_page->mainFrame()->document()->body()->renderer();
        if (!forward)
            object = lastLeafChild(object);
    } else {
        object = traverse(m_focusedObject);
        if (!object)
            object = ownerElementSibling(m_focusedObject);
    }

    if (!object) {
        clearFocus();
        return false;
    }

    RenderObject* candidate;
    do {
        candidate = findFocusable(object);
        if (candidate)
            break;
    } while ((object = ownerElementSibling(object)));

    if (!candidate) {
        clearFocus();
        return false;
    }

    Node* node = candidate->node();
    while (node) {
        node->renderer()->scrollRectToVisible(node->getRect(), ScrollAlignment::alignCenterIfNeeded, ScrollAlignment::alignCenterIfNeeded);
        node = node->document()->ownerElement();
    }

    if (!setFocus(candidate))
        return moveFocus(forward);

    return true;
}

static Node* findShortestDistanceNode(const IntPoint& point, const HitTestResult::NodeSet& nodeSet)
{
    HashSet<Node*> candidates;
    HitTestResult::NodeSet::const_iterator it = nodeSet.begin();
    const HitTestResult::NodeSet::const_iterator end = nodeSet.end();

    for (; it != end; ++it) {
        for (Node* node = it->get(); node; node = node->parentOrHostNode()) {
            if (isFocusable(node) || isReadable(node)) {
                AccessibilityObject* object = node->document()->axObjectCache()->getOrCreate(node->renderer());
                if (object && !isAriaHidden(object))
                    candidates.add(node);
            }
        }
    }

    Node* targetNode = 0;
    int targetSize = std::numeric_limits<int>::max();
    int targetDistance = std::numeric_limits<int>::max();

    while (!candidates.isEmpty()) {
        Node* node = *(candidates.begin());
        candidates.remove(candidates.begin());

        Vector<FloatQuad> quads;
        node->renderer()->absoluteQuads(quads);

        Vector<FloatQuad>::const_iterator quadIt = quads.begin();
        const Vector<FloatQuad>::const_iterator quadEnd = quads.end();

        for (; quadIt != quadEnd; ++quadIt) {
            IntRect box = quadIt->enclosingBoundingBox();
            int size = box.size().area();
            int distance = box.distanceSquaredToPoint(point);
            if (distance < targetDistance || (distance == targetDistance && size < targetSize)) {
                targetNode = node;
                targetSize = size;
                targetDistance = distance;
            }
        }
    }

    return targetNode;
}

bool ScreenReader::moveFocus(const IntPoint& point, bool reload)
{
    Frame* mainFrame = m_page->mainFrame();
    mainFrame->document()->updateLayoutIgnorePendingStylesheets();

    IntPoint hitTestPoint = mainFrame->view()->windowToContents(point);
    HitTestResult result = mainFrame->eventHandler()->hitTestResultAtPoint(hitTestPoint, false, false, DontHitTestScrollbars, HitTestRequest::ReadOnly | HitTestRequest::Active, s_hitTestPadding);

    Node* candidate = findShortestDistanceNode(hitTestPoint, result.rectBasedTestResult());
    if (!candidate)
        return false;

    if (candidate->renderer() != m_focusedObject || reload)
        setFocus(candidate->renderer());

    return true;
}

static String ariaRoleText(AccessibilityRole roleValue)
{
    switch (roleValue) {
    case ButtonRole:
    case PopUpButtonRole:
        return "button";
    case CheckBoxRole:
        return "check box";
    case ComboBoxRole:
        return "combo box";
    case ImageRole:
        return "image";
    case LinkRole:
    case WebCoreLinkRole:
        return "link";
    case ListBoxOptionRole:
        return "list box option";
    case ProgressIndicatorRole:
        return "";
    case RadioButtonRole:
        return "radio button";
    case SliderRole:
        return "slider";
    case TabRole:
        return "tab";
    default:
        break;
    }
    return emptyString();
}

static bool addNodeText(Node* node, WebPage* page, bool hasSubtreeText, Vector<String>& textList, bool isTop)
{
    AccessibilityObject* axObject = visibleAXObject(node->renderer());
    if (!axObject || isAriaHidden(axObject))
        return hasSubtreeText;

    if (node->isElementNode()) {
        Element* element = toElement(node);
        String type, text, state;

        if (element->isLink())
            type = String::fromUTF8(screenReaderLink().utf8().data());
        else if (element->isFormControlElement()) {
            if (element->hasTagName(HTMLNames::buttonTag)) {
                type = String::fromUTF8(screenReaderButton().utf8().data());
                text = static_cast<HTMLButtonElement*>(element)->value();
                if (!text.isEmpty() && textList.size() > 0 && text == textList[0])
                    text = "";
            } else if (element->hasTagName(HTMLNames::fieldsetTag))
                type = static_cast<HTMLFormControlElement*>(element)->type();
            else if (element->hasTagName(HTMLNames::inputTag)) {
                if (static_cast<HTMLInputElement*>(element)->isSubmitButton()) {
                    type = String::fromUTF8(screenReaderButton().utf8().data());
                    text = static_cast<HTMLInputElement*>(element)->value();
                } else {
                    type = String::fromUTF8(screenReaderEditField().utf8().data());
                    if (static_cast<HTMLInputElement*>(element)->isPasswordField()) {
                        bool enabled = false;
                        page->sendSync(Messages::WebPageProxy::ReadOutPasswords(), Messages::WebPageProxy::ReadOutPasswords::Reply(enabled));
                        if (enabled) {
                            text = static_cast<HTMLInputElement*>(element)->value();
                            if (text.isEmpty())
                                text = static_cast<HTMLInputElement*>(element)->placeholder();
                        } else {
                            if (!static_cast<HTMLInputElement*>(element)->value().isEmpty())
                                text = String::format(screenReaderPDCharacters().utf8().data(), static_cast<HTMLInputElement*>(element)->value().length());
                            else
                                text = static_cast<HTMLInputElement*>(element)->placeholder();
                        }
                        if (text.isEmpty())
                            text = static_cast<HTMLInputElement*>(element)->alt();
                    } else {
                        text = static_cast<HTMLInputElement*>(element)->value();
                        if (text.isEmpty())
                            text = static_cast<HTMLInputElement*>(element)->placeholder();
                        if (text.isEmpty())
                            text = static_cast<HTMLInputElement*>(element)->alt();
                    }
                }
            } else if (element->hasTagName(HTMLNames::keygenTag))
                type = static_cast<HTMLFormControlElement*>(element)->type();
            else if (element->hasTagName(HTMLNames::outputTag)) {
                type = static_cast<HTMLFormControlElement*>(element)->type();
                text = static_cast<HTMLOutputElement*>(element)->value();
            } else if (element->hasTagName(HTMLNames::selectTag)) {
                type = static_cast<HTMLFormControlElement*>(element)->type();
                text = static_cast<HTMLSelectElement*>(element)->value();
                if (static_cast<HTMLSelectElement*>(element)->size() == 1)
                    state = "1 item";
                else
                    state = String::format(screenReaderEditField().utf8().data(), static_cast<HTMLSelectElement*>(element)->size());
            } else if (element->hasTagName(HTMLNames::textareaTag)) {
                type = String::fromUTF8(screenReaderButton().utf8().data());
                text = static_cast<HTMLTextAreaElement*>(element)->value();
            }
        } else if (element->hasTagName(HTMLNames::imgTag)) {
            if (!isTop)
                return hasSubtreeText;
            type = String::fromUTF8(screenReaderImage().utf8().data());
            text = static_cast<HTMLImageElement*>(element)->altText();
        } else if (element->hasTagName(HTMLNames::optionTag)) {
            text = static_cast<HTMLOptionElement*>(element)->label();
            if (static_cast<HTMLOptionElement*>(element)->selected())
                state = String::fromUTF8(screenReaderSelected().utf8().data());
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        } else if (element->hasTagName(HTMLNames::videoTag)) {
            type = String::fromUTF8(screenReaderVideo().utf8().data());
        } else if (element->hasTagName(HTMLNames::audioTag)) {
             type = String::fromUTF8(screenReaderAudio().utf8().data());
#endif
        }

        if (text.isEmpty() && node->isHTMLElement()) {
            const AtomicString& title = element->fastGetAttribute(HTMLNames::titleAttr);
            if (!containsOnlyWhitespace(title))
                text = title;
        }

        String ariaText;
        if (!containsOnlyWhitespace((ariaText = axObject->ariaLabeledByAttribute())))
            text = ariaText;
        else if (!containsOnlyWhitespace((ariaText = element->fastGetAttribute(aria_labelAttr))))
            text = ariaText;

        if (element->fastHasAttribute(HTMLNames::roleAttr))
            type = element->fastGetAttribute(HTMLNames::roleAttr).isEmpty()? emptyString() : ariaRoleText(axObject->roleValue());

        String more;
        if (!containsOnlyWhitespace((ariaText = axObject->ariaDescribedByAttribute())))
            more = ariaText;

        if (text.isEmpty() && type.isEmpty())
            return hasSubtreeText;

        if (axObject->isSelected())
            state = String::fromUTF8(screenReaderSelected().utf8().data());
        else if (axObject->isChecked())
            state = "checked";

        if (!text.isEmpty())
            textList.append(text);
        if (!type.isEmpty())
            textList.append(type);
        if (!state.isEmpty())
            textList.append(state);
        if (!more.isEmpty())
            textList.append(more);
    } else if (node->isTextNode() && hasText(node))
        textList.append(node->nodeValue());
    else
        return hasSubtreeText;

    if (!axObject->isEnabled())
        textList.append("disabled");

    return true;
}

static bool addSubtreeText(Node* node, WebPage* page, Vector<String>& textList, bool isTop)
{
    bool hasSubtreeText = false;
    for (Node* child = node->firstChild(); child; child = child->nextSibling()) {
        if (!isFocusable(child))
            hasSubtreeText |= addSubtreeText(child, page, textList, false);
    }
    return addNodeText(node, page, hasSubtreeText, textList, isTop);
}

static String subtreeText(Node* top, WebPage* page)
{
    Vector<String> textList;
    addSubtreeText(top, page, textList, true);

    StringBuilder text;
    String separator(" ");
    for (Vector<String>::iterator iter = textList.begin(); iter != textList.end(); ++iter) {
        if (iter->isEmpty())
            continue;

        text.append(*iter);
        text.append(separator);
    }

    return text.toString().simplifyWhiteSpace();
}

bool ScreenReader::setFocus(RenderObject* object)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    // Fix for issue WEB-8519. Check for fullscreen to avoid the focusing of elements in the background.
    if (object->document()->webkitIsFullScreen() && !object->isMedia())
        return true;
#endif

    m_focusedObject = object;
    m_hasFocus = true;

    String text = subtreeText(object->node(), m_page);
    if (text.isEmpty())
        return false;

    m_page->send(Messages::WebPageProxy::DidScreenReaderTextChanged(text));

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    Vector<IntRect> rects;
    m_page->calcFocusedRects(m_focusedObject->node(), rects);
    m_page->send(Messages::WebPageProxy::DidScreenReaderRectsChanged(rects));
#endif

    return true;
}

Node* ScreenReader::focusedNode()
{
    if (!m_hasFocus)
        return 0;

    return m_focusedObject->node();
}

bool ScreenReader::rendererWillBeDestroyed(RenderObject* object)
{
    if (m_focusedObject != object)
        return false;

    clearFocus();
    m_isForward = false;
    m_focusedObject = traverse(object);

    return true;
}

void ScreenReader::clearFocus()
{
    m_focusedObject = 0;
    m_hasFocus = false;
    m_page->send(Messages::WebPageProxy::DidScreenReaderTextChanged(emptyString()));
}

}

#endif // ENABLE(TIZEN_SCREEN_READER)
