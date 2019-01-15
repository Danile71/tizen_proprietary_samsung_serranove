/*
   Copyright (C) 2012 Samsung Electronics

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
#include "Pasteboard.h"

#include "DataObjectTizen.h"
#include "DocumentFragment.h"
#include "EditorClient.h"
#include "FileSystem.h"
#include "Frame.h"
#include "HTMLImageElement.h"
#include "HTMLParserIdioms.h"
#include "Image.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "PlatformString.h"
#include "RenderImage.h"
#include "markup.h"
#include <Evas.h>
#include <cairo.h>
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>

#if ENABLE(SVG)
#include "SVGNames.h"
#include "XLinkNames.h"
#endif

#if ENABLE(TIZEN_PASTE_IMAGE_URI)
#include "Settings.h"
#endif

// Utility function to get clipboard data from CbhmWindow (defined in ClipboardHelper.cpp)
namespace WebKit {
bool retrieveClipboardItem(int index, int* format, String* pData);
}

namespace WebCore {

Pasteboard* Pasteboard::generalPasteboard()
{
    static Pasteboard* pasteboard = new Pasteboard();
    return pasteboard;
}

Pasteboard::Pasteboard()
    : m_page(0)
    , m_imageURIData("")
    , m_dataType("PlainText")
    , m_needsUpdate(true)
{
    makeImageDirectory();
}

void Pasteboard::setDataWithType(const String& data, const String& dataType)
{
    DataObjectTizen* dataObject = DataObjectTizen::sharedDataObject();
    m_dataType = dataType;

#if ENABLE(TIZEN_CLIPBOARD)
    dataObject->clearAll();
#endif

    // We have to save Markup and Image type data as a text data,
    // because they can be requested as PlainText.
    if (dataType == "Markup") {
        dataObject->setMarkup(data);
        char* plainText = evas_textblock_text_markup_to_utf8(0, data.utf8().data());
        if (plainText) {
            dataObject->setText(String::fromUTF8(plainText));
            free(plainText);
        }
    } else if (dataType == "PlainText") {
        dataObject->setText(data);
    } else if (dataType == "Image") {
        m_imageURIData = data;
        dataObject->setText(data);
#if ENABLE(TIZEN_CLIPBOARD)
        dataObject->setImage(data);
#endif
    }
    // FIXME: It's needed for now because paste routine is diverged. It should be unified ASAP.
    m_needsUpdate = false;
}

void Pasteboard::setPage(Page* page)
{
    m_page = page;
}

void Pasteboard::makeImageDirectory()
{
    m_imageDirectory = pathByAppendingComponent(homeDirectoryPath(), ".webkit/copiedImages/");
    makeAllDirectories(m_imageDirectory);

    // Delete all png files in the image directory because they are temporary files.
    Vector<String> files = listDirectory(m_imageDirectory, String("*.png"));
    for (size_t i = 0, size = files.size(); i < size; ++i)
        deleteFile(files[i]);
}

void Pasteboard::writePlainText(const String& text)
{
    m_needsUpdate = true;
    DataObjectTizen* dataObject = DataObjectTizen::sharedDataObject();
    dataObject->clearAll();
    dataObject->setText(text);

    if (m_page && m_page->mainFrame())
        m_page->mainFrame()->editor()->client()->setClipboardData(text, String("PlainText"));
}

void Pasteboard::writeSelection(Range* selectedRange, bool canSmartCopyOrDelete, Frame* frame)
{
    m_needsUpdate = true;
    String data = createMarkup(selectedRange, 0, AnnotateForInterchange, false, ResolveAllURLs);

    DataObjectTizen* dataObject = DataObjectTizen::sharedDataObject();
    dataObject->clearAll();
    dataObject->setText(frame->editor()->selectedText());
    dataObject->setMarkup(data);

    if (m_page && m_page->mainFrame())
        m_page->mainFrame()->editor()->client()->setClipboardData(data, String("Markup"));
}

void Pasteboard::writeURL(const KURL& url, const String& label, Frame* frame)
{
    m_needsUpdate = true;
    if (url.isEmpty())
        return;

    DataObjectTizen* dataObject = DataObjectTizen::sharedDataObject();
    dataObject->clearAll();
    dataObject->setURL(url, label);

    if (m_page && m_page->mainFrame())
        m_page->mainFrame()->editor()->client()->setClipboardData(url.string(), String("PlainText"));
}

static KURL getURLForImageNode(Node* node)
{
    // FIXME: Later this code should be shared with Chromium somehow. Chances are all platforms want it.
    String urlString;
    if (node->hasTagName(HTMLNames::imgTag) || node->hasTagName(HTMLNames::inputTag)) {
        urlString = static_cast<Element*>(node)->getAttribute(HTMLNames::srcAttr);
        urlString.replace("?", "_");
    }
#if ENABLE(SVG)
    else if (node->hasTagName(SVGNames::imageTag))
        urlString = static_cast<Element*>(node)->getAttribute(XLinkNames::hrefAttr);
#endif
    else if (node->hasTagName(HTMLNames::embedTag) || node->hasTagName(HTMLNames::objectTag)) {
        Element* element = static_cast<Element*>(node);
        urlString = element->getAttribute(element->imageSourceAttributeName());
    }
    return urlString.isEmpty() ? KURL() : node->document()->completeURL(stripLeadingAndTrailingHTMLSpaces(urlString));
}

void Pasteboard::writeImage(Node* node, const KURL&, const String& title)
{
    m_needsUpdate = true;
    DataObjectTizen* dataObject = DataObjectTizen::sharedDataObject();
    dataObject->clearAll();

    KURL url = getURLForImageNode(node);
    if (!url.isEmpty()) {
        dataObject->setURL(url, title);
        dataObject->setMarkup(createMarkup(static_cast<Element*>(node), IncludeNode, 0, ResolveAllURLs));
    }

    ASSERT(node);

    if (!m_page || !m_page->mainFrame())
        return;

    if (!(node->renderer() && node->renderer()->isImage()))
        return;

    RenderImage* renderer = toRenderImage(node->renderer());
    CachedImage* cachedImage = renderer->cachedImage();
    if (!cachedImage || cachedImage->errorOccurred())
        return;
    Image* image = cachedImage->imageForRenderer(renderer);
    ASSERT(image);

    String imagePath = m_imageDirectory;
    String imageName;
    if (url.protocolIsData())
        imageName = url.string();
    else
        imageName = url.path() + url.query();

    if (imageName.length() > 250)
        imageName = imageName.right(250);

    imageName = decodeURLEscapeSequences(imageName);
    imageName.replace("/", "_");
    size_t index = imageName.reverseFind(".");
    if (index != WTF::notFound)
        imageName = imageName.left(index);

    imagePath.append(imageName);
    imagePath.append(".png");

    NativeImageCairo* nativeImage = image->getCairoSurface();
    if (!nativeImage)
        return;

    cairo_surface_write_to_png(nativeImage->surface(), imagePath.utf8().data());

    imagePath.insert("file://", 0);
    m_page->mainFrame()->editor()->client()->setClipboardData(imagePath, String("Image"));
}

void Pasteboard::writeClipboard(Clipboard*)
{
    notImplemented();
}

void Pasteboard::clear()
{
    notImplemented();
}

bool Pasteboard::canSmartReplace()
{
    notImplemented();
    return false;
}

static PassRefPtr<DocumentFragment> documentFragmentWithImageURI(Frame* frame, String imageURI)
{
    RefPtr<HTMLImageElement> imageElement = HTMLImageElement::create(frame->document());
    if (!imageElement)
        return 0;

#if ENABLE(TIZEN_PASTE_IMAGE_URI)
    if (frame->page()->settings()->pasteImageUriEnabled())
        imageElement->setSrc(imageURI);
    else
#endif
    {
        String filePath(imageURI);
        filePath.remove(filePath.find("file://"), 7);

        // Support only png and jpg image.
        String fileExtension;
        if (filePath.endsWith(".jpg", false) || filePath.endsWith(".jpeg", false))
            fileExtension = "jpg";
        else if (filePath.endsWith(".png", false))
            fileExtension = "png";
        else
            return 0;

        long long size = 0;
        if (!getFileSize(filePath, size) || size <= 0)
            return 0;

        PlatformFileHandle file = openFile(filePath, OpenForRead);
        if (!file)
            return 0;

        OwnArrayPtr<char> buffer = adoptArrayPtr(new char[size]);
        if (readFromFile(file, buffer.get(), size) != size) {
            closeFile(file);
            return 0;
        }
        closeFile(file);

        String encodedString = String("data:image/");
        encodedString.append(fileExtension);
        encodedString.append(";base64,");
        encodedString.append(base64Encode(buffer.get(), size));
        imageElement->setSrc(encodedString);
    }

    RefPtr<DocumentFragment> fragment = frame->document()->createDocumentFragment();
    if (fragment) {
        ExceptionCode ec;
        fragment->appendChild(imageElement, ec);
        return fragment.release();
    }
    return 0;
}

PassRefPtr<DocumentFragment> Pasteboard::documentFragment(Frame* frame, PassRefPtr<Range> context,
                                                          bool allowPlainText, bool& chosePlainText)
{
    m_needsUpdate = true;
    DataObjectTizen* dataObject = DataObjectTizen::sharedDataObject();
    chosePlainText = false;

    if (m_dataType == "Image") {
        RefPtr<DocumentFragment> fragment;
        fragment = documentFragmentWithImageURI(frame, m_imageURIData);
        if (fragment)
            return fragment.release();
    }

    if (m_dataType == "Markup") {
        RefPtr<DocumentFragment> fragment = createFragmentFromMarkup(frame->document(), dataObject->markup(), "", DisallowScriptingContent);
        if (fragment)
            return fragment.release();
    }

    if (allowPlainText && m_dataType == "PlainText") {
        chosePlainText = true;
        RefPtr<DocumentFragment> fragment = createFragmentFromText(context.get(), dataObject->text());
        if (fragment)
            return fragment.release();
    }
    return 0;
}

String Pasteboard::plainText(Frame*)
{
    return DataObjectTizen::sharedDataObject()->text();
}

bool Pasteboard::updateDataFromClipboard(bool isContentEditable)
{
    if (!m_needsUpdate)
        return false;

    int format = 0; // ELM_SEL_FORMAT_NONE
    String clipboardData, dataType;
    bool ret = WebKit::retrieveClipboardItem(0, &format, &clipboardData);
    if (!ret)
        return false;
    switch(format) {
    case 0x01: // ELM_SEL_FORMAT_TEXT
    case 0x02: // ELM_SEL_FORMAT_MARKUP
        dataType = String("PlainText");
        break;
    case 0x10: // ELM_SEL_FORMAT_HTML
        dataType = String("Markup");
        break;
    case 0x04: // ELM_SEL_FORMAT_IMAGE
        if (isContentEditable)
            dataType = String("Image");
        break;
    default:
        dataType = String("PlainText");
    }
    setDataWithType(clipboardData, dataType);
    return true;
}
}
