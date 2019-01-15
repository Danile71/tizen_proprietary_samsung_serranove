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
#include "WebPageProxy.h"

#include "EwkViewImpl.h"
#include "NativeWebKeyboardEvent.h"
#include "NotImplemented.h"
#include "PageClientImpl.h"
#include "WebPageMessages.h"
#include "WebProcessProxy.h"

#include <sys/utsname.h>

#if PLATFORM(TIZEN)
#include "DrawingAreaMessages.h"
#include "WebKitVersion.h"
#include <system_info.h>

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
#include "LayerTreeCoordinatorProxy.h"
#endif
#include "NativeWebKeyboardEvent.h"
#include "WebImage.h"
#include "WebPageMessages.h"
#include "WebProcessProxy.h"
#include <WebCore/IntSize.h>
#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
#include "WebContextMenuProxy.h"
#include "ewk_view_private.h"
#endif

#define MESSAGE_CHECK(assertion) MESSAGE_CHECK_BASE(assertion, m_process->connection())

#if ENABLE(TIZEN_LINK_MAGNIFIER)
#include "LinkMagnifierProxy.h"
#endif

#if ENABLE(TIZEN_SCREEN_READER)
#include "ScreenReaderProxy.h"
#include "ewk_view_private.h"
#endif

#if ENABLE(TIZEN_CSP)
#include <WebCore/ContentSecurityPolicy.h>
#endif

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
#include "WebData.h"
#endif

using namespace WebCore;
#endif

namespace WebKit {

Evas_Object* WebPageProxy::viewWidget()
{
    return static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->view();
}

String WebPageProxy::standardUserAgent(const String& applicationNameForUserAgent)
{
    WTF::String platform;
    WTF::String version;
    WTF::String osVersion;

#if ENABLE(TIZEN_USER_AGENT)
    char* tizenVersion = 0;
    system_info_get_platform_string("http://tizen.org/feature/platform.version", &tizenVersion);
    osVersion = tizenVersion;
    free(tizenVersion);
    osVersion = osVersion.substring(0, 3);

    version = String::number(WEBKIT_MAJOR_VERSION) + '.' + String::number(WEBKIT_MINOR_VERSION);
#if ENABLE(TIZEN_EMULATOR)
    WTF::String userAgent = "Mozilla/5.0 (Linux; Tizen " + osVersion + "; sdk) AppleWebKit/" + version + " (KHTML, like Gecko)";
#else
    char* tizenManufacturer;
    char* tizenModelName;

    system_info_get_platform_string("http://tizen.org/system/manufacturer", &tizenManufacturer);
    WTF::String manufacturer(tizenManufacturer);
    manufacturer.makeUpper();
    free(tizenManufacturer);

    system_info_get_platform_string("http://tizen.org/system/model_name", &tizenModelName);

    WTF::String userAgent = "Mozilla/5.0 (Linux; Tizen " + osVersion;
    if (!manufacturer.isEmpty() && tizenModelName) {
        userAgent.append("; ");
        userAgent.append(manufacturer);
        userAgent.append(" ");
        userAgent.append(tizenModelName);
    }
    else if (!manufacturer.isEmpty()) {
        userAgent.append("; ");
        userAgent.append(manufacturer);
    }
    else if (tizenModelName) {
        userAgent.append("; ");
        userAgent.append(tizenModelName);
    }
    free(tizenModelName);

    userAgent.append(") AppleWebKit/");
    userAgent.append(version);
    userAgent.append(" (KHTML, like Gecko)");
#endif

    if (applicationNameForUserAgent.isEmpty()) {
        userAgent.append(" Version/");
        userAgent.append(osVersion);
    } else
        userAgent.append(" "+applicationNameForUserAgent);

    userAgent.append(" Mobile Safari/");
    userAgent.append(version);

    return userAgent;
#else // ENABLE(TIZEN_USER_AGENT)
#if PLATFORM(X11)
    platform = "X11";
#else
    platform = "Unknown";
#endif
    version = String::number(WEBKIT_MAJOR_VERSION) + '.' + String::number(WEBKIT_MINOR_VERSION) + '+';
    struct utsname name;
    if (uname(&name) != -1)
        osVersion = WTF::String(name.sysname) + " " + WTF::String(name.machine);
    else
        osVersion = "Unknown";

    return "Mozilla/5.0 (" + platform + "; " + osVersion + ") AppleWebKit/" + version
        + " (KHTML, like Gecko) Version/5.0 Safari/" + version;
#endif // ENABLE(TIZEN_USER_AGENT)
}

void WebPageProxy::getEditorCommandsForKeyEvent(Vector<WTF::String>& commandsList)
{
    notImplemented();
}

void WebPageProxy::saveRecentSearches(const String&, const Vector<String>&)
{
    notImplemented();
}

void WebPageProxy::loadRecentSearches(const String&, Vector<String>&)
{
    notImplemented();
}

void WebPageProxy::setThemePath(const String& themePath)
{
    process()->send(Messages::WebPage::SetThemePath(themePath), m_pageID, 0);
}

#if ENABLE(TIZEN_CUSTOM_HEADERS)
void WebPageProxy::addCustomHeader(const String& name, const String& value)
{
    if (name.isEmpty())
        return;

    if (value.isEmpty())
        return;

    if (!isValid())
        return;

    process()->send(Messages::WebPage::AddCustomHeader(name, value), m_pageID);
}

void WebPageProxy::removeCustomHeader(const String& name)
{
    if (name.isEmpty())
        return;

    if (!isValid())
        return;

    process()->send(Messages::WebPage::RemoveCustomHeader(name), m_pageID);
}

void WebPageProxy::clearCustomHeaders()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ClearCustomHeaders(), m_pageID);
}
#endif

#if PLATFORM(TIZEN)
bool WebPageProxy::scrollMainFrameBy(const IntSize& scrollOffset)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    return static_cast<PageClientImpl*>(m_pageClient)->scrollBy(scrollOffset);
#else
    if (!isValid())
        return false;

    process()->send(Messages::WebPage::ScrollMainFrameBy(scrollOffset), m_pageID);
    return true;
#endif
}

void WebPageProxy::scrollMainFrameTo(const IntPoint& scrollPosition)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    static_cast<PageClientImpl*>(m_pageClient)->scrollTo(scrollPosition);
#else
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ScrollMainFrameTo(scrollPosition), m_pageID);
#endif
}

void WebPageProxy::didChangeScrollPositionForMainFrame(const IntPoint& scrollPosition)
{
#if !ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    m_scrollPosition = scrollPosition;
#endif
}

void WebPageProxy::didChangeContentsSize(const IntSize& size)
{
    if (m_contentsSize == size)
        return;

    m_contentsSize = size;
    m_pageClient->didChangeContentsSize(size);
}

PassRefPtr<WebImage> WebPageProxy::createSnapshot(const IntRect& rect, float scaleFactor)
{
    if (!isValid())
        return 0;

    ShareableBitmap::Handle snapshotHandle;
    // Do not wait for more than a second (arbitrary) for the WebProcess to get the snapshot so
    // that the UI Process is not permanently stuck waiting on a potentially crashing Web Process.
    static const double createSnapshotSyncMessageTimeout = 1.0;
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    float baseScaleFactor = static_cast<PageClientImpl*>(m_pageClient)->scaleFactor();
    scaleFactor = scaleFactor * baseScaleFactor;

    IntRect visibleContentRect = static_cast<PageClientImpl*>(m_pageClient)->visibleContentRect();
    IntRect scaledRect = rect;
    scaledRect.move(visibleContentRect.x(), visibleContentRect.y());
    scaledRect.scale(1/baseScaleFactor);
    process()->sendSync(Messages::WebPage::CreateSnapshot(scaledRect, scaleFactor), Messages::WebPage::CreateSnapshot::Reply(snapshotHandle), m_pageID, createSnapshotSyncMessageTimeout);
#else
    process()->sendSync(Messages::WebPage::CreateSnapshot(rect, scaleFactor), Messages::WebPage::CreateSnapshot::Reply(snapshotHandle), m_pageID, createSnapshotSyncMessageTimeout);
#endif
    if (snapshotHandle.isNull())
        return 0;
    return WebImage::create(ShareableBitmap::create(snapshotHandle));
}

#if ENABLE(TIZEN_CACHE_IMAGE_GET)
PassRefPtr<WebImage> WebPageProxy::cacheImageGet(const String& imageUrl)
{
    if (!isValid() || imageUrl.isNull())
        return 0;

    ShareableBitmap::Handle cacheImageHandle;
    // Do not wait for more than a second (arbitrary) for the WebProcess to get the cache image so
    // that the UI Process is not permanently stuck waiting on a potentially crashing Web Process.
    static const double cacheImageGetSyncMessageTimeout = 1.0;
    process()->sendSync(Messages::WebPage::CacheImageGet(imageUrl), Messages::WebPage::CacheImageGet::Reply(cacheImageHandle), m_pageID, cacheImageGetSyncMessageTimeout);
    if (cacheImageHandle.isNull())
        return 0;
    return WebImage::create(ShareableBitmap::create(cacheImageHandle));
}
#endif

#if ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
#if ENABLE(TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM)
void WebPageProxy::textChangeInTextField(const String& name, const String& value, bool isInputInForm, bool isForcePopupShow, bool isHandleEvent)
{
    static_cast<PageClientImpl*>(m_pageClient)->textChangeInTextField(name, value, isInputInForm, isForcePopupShow, isHandleEvent);
}
#else
void WebPageProxy::textChangeInTextField(const String& name, const String& value)
{
    static_cast<PageClientImpl*>(m_pageClient)->textChangeInTextField(name, value);
}
#endif // TIZEN_WEBKIT2_AUTOFILL_PROFILE_FORM
#endif

#if ENABLE(TIZEN_ISF_PORT)
void WebPageProxy::prepareKeyDownEvent()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::PrepareKeyDownEvent(), m_pageID);
}

void WebPageProxy::deleteSurroundingText(int offset, int count)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::DeleteSurroundingText(offset, count), m_pageID);
}

void WebPageProxy::didCancelComposition()
{
    m_didCancelCompositionFromWebProcess = true;
    InputMethodContextEfl* inputMethodContext = static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->inputMethodContext();
    if (inputMethodContext)
        inputMethodContext->resetIMFContext();
    m_didCancelCompositionFromWebProcess = false;
}

void WebPageProxy::didRequestUpdatingEditorState()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->textSelection()->didRequestUpdatingEditorState()){
        static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->textSelection()->updateTextInputState();
    }
#endif
    InputMethodContextEfl* inputMethodContext = static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->inputMethodContext();
    if (inputMethodContext)
        inputMethodContext->didRequestUpdatingEditorState();
}

void WebPageProxy::recalcFilterEvent(const EditorState& editorState, bool isStart, bool& isFiltered)
{
    if (isStart)
        process()->send(Messages::WebPage::EndRecalcFilterEvent(), m_pageID);

    editorStateChanged(editorState);
    isFiltered = false;

    InputMethodContextEfl* inputMethodContext = static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->inputMethodContext();
    if (!inputMethodContext || m_keyEventQueue.isEmpty())
        return;

#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    NativeWebKeyboardEvent event = m_keyEventQueue.first().forwardedEvent;
#else
    NativeWebKeyboardEvent event = m_keyEventQueue.first();
#endif
    if (!event.nativeEvent() || event.nativeEvent()->type() != ECORE_IMF_EVENT_KEY_DOWN)
        return;

    isFiltered = inputMethodContext->recalcFilterEvent(event.nativeEvent()->event());
}
#endif // #if ENABLE(TIZEN_ISF_PORT)
#if ENABLE(TIZEN_WEBKIT2_FORM_NAVIGATION)
void WebPageProxy::requestUpdateFormNavigation()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::RequestUpdateFormNavigation(), m_pageID);
}

void WebPageProxy::moveFocus(int newIndex)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::MoveFocus(newIndex), m_pageID);
}

void WebPageProxy::updateFormNavigation(int length, int offset, bool prevState, bool nextState)
{
    static_cast<PageClientImpl*>(m_pageClient)->updateFormNavigation(length, offset, prevState, nextState);
}

void WebPageProxy::updateTextInputStateByUserAction(bool focus)
{
   InputMethodContextEfl* inputMethodContext = static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->inputMethodContext();
   inputMethodContext->updateTextInputStateByUserAction(focus);
}
#endif
#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
Eina_Bool WebPageProxy::updateOnlyVisibleAreaStateTimerFired(void* data)
{
    WebPageProxy* page = static_cast<WebPageProxy*>(data);
    if (!page)
        return ECORE_CALLBACK_CANCEL;

    uint64_t pageID = page->pageID();
    page->process()->send(Messages::LayerTreeCoordinator::UpdateOnlyVisibleAreaState(false), pageID);
    page->m_updateOnlyVisibleAreaState = false;
    page->m_updateOnlyVisibleAreaStateTimer = 0;

    return ECORE_CALLBACK_CANCEL;
}
#endif

#if ENABLE(TIZEN_TEXT_CARET_HANDLING_WK2)
void WebPageProxy::setCaretPosition(const WebCore::IntPoint& pos)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SetCaretPosition(pos), m_pageID);
}
#endif

#if ENABLE(TIZEN_PLUGIN_CUSTOM_REQUEST)
void WebPageProxy::processPluginCustomRequest(const String& request, const String& msg)
{
    if (String("requestKeyboard,plugin") == request) {
        bool active = false;
        if (String("show") == msg)
            active = true;
#if ENABLE(TIZEN_ISF_PORT)
        m_editorState = EditorState();
        m_editorState.isContentEditable = active;
        m_pageClient->updateTextInputState();
#endif
    }
#if ENABLE(TIZEN_JSBRIDGE_PLUGIN)
    else if (String("requestToNative,json") == request)
        m_tizenClient.processJSBridgePlugin(this, request, msg);
#endif
}
#endif

#if ENABLE(TIZEN_INPUT_TAG_EXTENSION) || ENABLE(TIZEN_WEBKIT2_FORM_DATABASE)
void WebPageProxy::setFocusedInputElementValue(const String& inputValue, bool clear)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SetFocusedInputElementValue(inputValue, clear), m_pageID);
}

String WebPageProxy::getFocusedInputElementValue()
{
    if (!isValid())
        return String();

    String inputValue;
    process()->sendSync(Messages::WebPage::GetFocusedInputElementValue(), Messages::WebPage::GetFocusedInputElementValue::Reply(inputValue), m_pageID);
    return inputValue;
}
#endif

#if ENABLE(TIZEN_DATALIST_ELEMENT)
Vector<String> WebPageProxy::getFocusedInputElementDataList()
{
    if (!isValid())
        return Vector<String>();

    Vector<String> optionList;
    process()->sendSync(Messages::WebPage::GetFocusedInputElementDataList(), Messages::WebPage::GetFocusedInputElementDataList::Reply(optionList), m_pageID);
    return optionList;
}
#endif

void WebPageProxy::initializeTizenClient(const WKPageTizenClient* client)
{
    m_tizenClient.initialize(client);
}

#if ENABLE(TIZEN_WEBKIT2_HIT_TEST)
#if ENABLE(TOUCH_ADJUSTMENT)
WebHitTestResult::Data WebPageProxy::hitTestResultAtPoint(const IntPoint& point, int hitTestMode, const IntSize& area)
#else
WebHitTestResult::Data WebPageProxy::hitTestResultAtPoint(const IntPoint& point, int hitTestMode)
#endif
{
    WebHitTestResult::Data hitTestResultData;
    if (!isValid())
        return hitTestResultData;

#if ENABLE(TOUCH_ADJUSTMENT)
    process()->sendSync(Messages::WebPage::HitTestResultAtPoint(point, hitTestMode, area),
                        Messages::WebPage::HitTestResultAtPoint::Reply(hitTestResultData), m_pageID);
#else
    process()->sendSync(Messages::WebPage::HitTestResultAtPoint(point, hitTestMode),
                        Messages::WebPage::HitTestResultAtPoint::Reply(hitTestResultData), m_pageID);
#endif

    return hitTestResultData;
}
#endif

#if ENABLE(TIZEN_RECORDING_SURFACE_SET)
void WebPageProxy::recordingSurfaceSetEnableSet(bool enable)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::RecordingSurfaceSetEnableSet(enable), m_pageID, 0);
}
#endif

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
void WebPageProxy::hideContextMenu()
{
    if (m_activeContextMenu)
        m_activeContextMenu->hideContextMenu();
}

String WebPageProxy::contextMenuAbsoluteLinkURLString()
{
    if (!m_activeContextMenu)
        return String();

    return m_activeContextMenuHitTestResultData.absoluteLinkURL;
}

String WebPageProxy::contextMenuAbsoluteImageURLString()
{
    if (!m_activeContextMenu)
        return String();
#if ENABLE(TIZEN_CHILD_NODE_IMAGE_URL)
    return m_activeContextMenuHitTestResultData.absoluteImageURL.isEmpty() ? m_activeContextMenuHitTestResultData.absoluteChildImageURL : m_activeContextMenuHitTestResultData.absoluteImageURL;
#else
    return m_activeContextMenuHitTestResultData.absoluteImageURL;
#endif
}

#if ENABLE(TIZEN_FOCUS_UI)
void WebPageProxy::setContextMenuFocusable()
{
    if (m_activeContextMenu)
        m_activeContextMenu->setCenterPopupFocusable();
}
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_CLIPBOARD_HELPER)
void WebPageProxy::pasteContextMenuSelected()
{
#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
    if (static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->textSelection()->isLargeHandleMode())
        static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->textSelection()->setIsTextSelectionMode(TextSelection::ModeNone);
#endif

    static_cast<PageClientImpl*>(m_pageClient)->pasteContextMenuSelected();
}

void WebPageProxy::didSelectPasteMenuFromContextMenu(const String& data, const String& type)
{
    if (!isValid())
        return;

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
    process()->send(Messages::WebPage::SetClipboardDataForPaste(data, type), m_pageID);
#endif
    WebContextMenuItemData item(ActionType, ContextMenuItemTagPaste, String("Paste"), true, false);
    process()->send(Messages::WebPage::DidSelectItemFromActiveContextMenu(item), m_pageID);
}
#endif

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
void WebPageProxy::setClipboardData(const String& data, const String& type)
{
    static_cast<PageClientImpl*>(m_pageClient)->setClipboardData(data, type);
}

void WebPageProxy::clearClipboardData()
{
    static_cast<PageClientImpl*>(m_pageClient)->clearClipboardData();
}
#endif

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_CLIPBOARD)
void WebPageProxy::executePasteFromClipboardItem(const String& data, const String& type)
{
    if (!isValid())
        return;

#if ENABLE(TIZEN_CLIPBOARD) || ENABLE(TIZEN_PASTEBOARD)
    process()->send(Messages::WebPage::SetClipboardDataForPaste(data, type), m_pageID);
#endif
    process()->send(Messages::WebPage::ExecuteEditCommandWithArgument("Paste", data), m_pageID);
}
#endif

#if ENABLE(TIZEN_MOBILE_WEB_PRINT)
void WebPageProxy::createPagesToPDF(const IntSize& surfaceSize, const IntSize& contentsSize, const String& fileName)
{
    process()->send(Messages::WebPage::CreatePagesToPDF(surfaceSize, contentsSize, fileName), m_pageID);
}

void WebPageProxy::didCreatePagesToPDF()
{
    static_cast<PageClientImpl*>(m_pageClient)->didCreatePagesToPDF();
}
#endif

#if ENABLE(TIZEN_WEB_STORAGE)
#if ENABLE(TIZEN_WEBKIT2_NUMBER_TYPE_SUPPORT)
void WebPageProxy::getWebStorageQuotaBytes(PassRefPtr<WebStorageQuotaCallback> prpCallback)
{
    RefPtr<WebStorageQuotaCallback> callback = prpCallback;
    if (!isValid()) {
        callback->invalidate();
        return;
    }

    uint64_t callbackID = callback->callbackID();
    m_quotaCallbacks.set(callbackID, callback.get());
    process()->send(Messages::WebPage::GetStorageQuotaBytes(callbackID), m_pageID);
}

void WebPageProxy::didGetWebStorageQuotaBytes(const uint32_t quota, uint64_t callbackID)
{
    RefPtr<WebStorageQuotaCallback> callback = m_quotaCallbacks.take(callbackID);
    if (!callback) {
        // FIXME: Log error or assert.
        // this can validly happen if a load invalidated the callback, though
        return;
    }

    m_quotaCallbacks.remove(callbackID);

    RefPtr<WebUInt32> uint32Object = WebUInt32::create(quota);
    callback->performCallbackWithReturnValue(uint32Object.release().leakRef());
}
#endif

void WebPageProxy::setWebStorageQuotaBytes(uint32_t quota)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SetStorageQuotaBytes(quota), m_pageID, 0);
}
#endif

void WebPageProxy::scale(double scaleFactor, const IntPoint& origin)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    static_cast<PageClientImpl*>(m_pageClient)->scaleContents(scaleFactor, origin);
#else
    scalePage(scaleFactor, origin);
#endif
}

void WebPageProxy::scaleImage(double scaleFactor, const IntPoint& origin)
{
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    static_cast<PageClientImpl*>(m_pageClient)->scaleImage(scaleFactor, origin);
#else
    scalePage(scaleFactor, origin);
#endif
}

double WebPageProxy::scaleFactor()
{
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    return static_cast<PageClientImpl*>(m_pageClient)->scaleFactor();
#else
    return pageScaleFactor();
#endif
}

#if ENABLE(TIZEN_CHANGE_TO_MEDIUM_SCALE_IMAGE_AFTER_ZOOM_START)
void WebPageProxy::changeToMediumScaleImage(double scaleFactor, const IntPoint& origin)
{
    static_cast<PageClientImpl*>(m_pageClient)->changeToMediumScaleImage(scaleFactor, origin);
}
#endif

#if ENABLE(TIZEN_ORIENTATION_EVENTS)
void WebPageProxy::sendOrientationChangeEvent(int orientation)
{
    process()->send(Messages::WebPage::SendOrientationChangeEvent(orientation), m_pageID, 0);
}
#endif

#if ENABLE(TIZEN_LOW_SCALE_LAYER)
void WebPageProxy::setPaintOnlyLowScaleLayer(bool b)
{
    static_cast<PageClientImpl*>(m_pageClient)->setPaintOnlyLowScaleLayer(b);
}
#endif

void WebPageProxy::suspendPainting()
{
    if (!isValid())
        return;

    process()->send(Messages::DrawingArea::SuspendPainting(), m_pageID);
}

void WebPageProxy::resumePainting()
{
    if (!isValid())
        return;

    process()->send(Messages::DrawingArea::ResumePainting(), m_pageID);
}

void WebPageProxy::suspendJavaScriptAndResource()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SuspendJavaScriptAndResources(), m_pageID);
}

void WebPageProxy::resumeJavaScriptAndResource()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ResumeJavaScriptAndResources(), m_pageID);
}

void WebPageProxy::suspendAnimations()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SuspendAnimations(), m_pageID);
}

void WebPageProxy::resumeAnimations()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ResumeAnimations(), m_pageID);
}

#if ENABLE(TIZEN_PLUGIN_SUSPEND_RESUME)
void WebPageProxy::suspendPlugin()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SuspendPlugin(), m_pageID);
}

void WebPageProxy::resumePlugin()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::ResumePlugin(), m_pageID);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
void WebPageProxy::purgeBackingStoresOfInactiveView()
{
    if (!isValid() || isViewVisible())
        return;

    process()->send(Messages::LayerTreeCoordinator::PurgeBackingStores(), m_pageID);
}
#endif

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
bool WebPageProxy::scrollOverflow(const FloatPoint& offset)
{
    if (!isValid())
        return false;

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    if (static_cast<PageClientImpl*>(m_pageClient)->isScrollableLayerFocused())
        return drawingArea()->layerTreeCoordinatorProxy()->setOffsetForFocusedScrollingContentsLayer(offset);
#endif
    if (static_cast<PageClientImpl*>(m_pageClient)->isScrollableNodeFocused()) {
        bool scrolled = false;
        //Setting a timeout of 0.05 would drop only those messages in a scenario when the WebProcess is busy
        const double timeout = 0.050;
        process()->sendSync(Messages::WebPage::ScrollOverflow(offset), Messages::WebPage::ScrollOverflow::Reply(scrolled), m_pageID,timeout);
        return scrolled;
    }

    return false;
}

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SPLIT)
FloatPoint WebPageProxy::splitScrollOverflow(const FloatPoint& offset)
{
    if (!isValid())
        return offset;

    FloatPoint remainingOffset = FloatPoint(0, 0);
#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION_ON_UI_SIDE)
    if (static_cast<PageClientImpl*>(m_pageClient)->isScrollableLayerFocused()) {
        drawingArea()->layerTreeCoordinatorProxy()->setOffsetForFocusedScrollingContentsLayer(offset);
        return remainingOffset;
    }
#endif
    if (static_cast<PageClientImpl*>(m_pageClient)->isScrollableNodeFocused())
        process()->sendSync(Messages::WebPage::SplitScrollOverflow(offset), Messages::WebPage::SplitScrollOverflow::Reply(remainingOffset), m_pageID);

    return remainingOffset;
}
#endif

bool WebPageProxy::setPressedNodeAtPoint(const IntPoint& point, bool checkOverflowLayer, WebLayerID& webLayerID)
{
    if (!isValid())
        return false;

    bool pressed = false;

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_SPLIT)
    if (splitScrollOverflowEnabled()) {
        process()->sendSync(Messages::WebPage::SetPressedNodeAtPoint(point, checkOverflowLayer), Messages::WebPage::SetPressedNodeAtPoint::Reply(pressed, webLayerID), m_pageID);
        return pressed;
    }
#endif

    const double timeout = 0.010;
    process()->sendSync(Messages::WebPage::SetPressedNodeAtPoint(point, checkOverflowLayer), Messages::WebPage::SetPressedNodeAtPoint::Reply(pressed, webLayerID), m_pageID, timeout);

    return pressed;
}

void WebPageProxy::setOverflowResult(bool pressed, uint32_t webLayerID)
{
    static_cast<PageClientImpl*>(m_pageClient)->setOverflowResult(pressed, webLayerID);
}
#endif

void WebPageProxy::executeEditCommandWithArgument(const String& commandName, const String& argument)
{
    if (!isValid())
        return;

    DEFINE_STATIC_LOCAL(String, ignoreSpellingCommandName, ("ignoreSpelling"));
    if (commandName == ignoreSpellingCommandName)
        ++m_pendingLearnOrIgnoreWordMessageCount;

    process()->send(Messages::WebPage::ExecuteEditCommandWithArgument(commandName, argument), m_pageID);
}

void WebPageProxy::replyJavaScriptAlert()
{
    if (!m_alertReply)
        return;

    m_alertReply->send();
    m_alertReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
    m_uiClient.notifyPopupReplyWaitingState(this, false);
#endif
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(false);
#endif
}

void WebPageProxy::replyJavaScriptConfirm(bool result)
{
#if ENABLE(TIZEN_JAVASCRIPT_POPUP_REPLY_HANDLING)
    if (m_confirmReply.isEmpty())
        return;

    RefPtr<Messages::WebPageProxy::RunJavaScriptConfirm::DelayedReply> confirmReply = m_confirmReply.takeFirst();
    confirmReply->send(result);
    confirmReply = nullptr;
#else
    if (!m_confirmReply)
        return;

    m_confirmReply->send(result);
    m_confirmReply = nullptr;
#endif

#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
    m_uiClient.notifyPopupReplyWaitingState(this, false);
#endif
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(false);
#endif
}

void WebPageProxy::replyJavaScriptPrompt(const String& result)
{
    if (!m_promptReply)
        return;

    m_promptReply->send(result);
    m_promptReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS)
    m_uiClient.notifyPopupReplyWaitingState(this, false);
#endif
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(false);
#endif
}

#if ENABLE(TIZEN_SUPPORT_BEFORE_UNLOAD_CONFIRM_PANEL)
void WebPageProxy::replyBeforeUnloadConfirmPanel(bool result)
{
    if (!m_beforeUnloadConfirmPanelReply)
        return;

    m_beforeUnloadConfirmPanelReply->send(result);
    m_beforeUnloadConfirmPanelReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(false);
#endif
}
#endif

#if ENABLE(TIZEN_ON_AUTHENTICATION_REQUESTED)
void WebPageProxy::replyReceiveAuthenticationChallengeInFrame(bool result)
{
    if (!m_AuthReply)
        return;

    m_AuthReply->send(result);
    m_AuthReply = nullptr;
}
#endif

#if ENABLE(TIZEN_CERTIFICATE_HANDLING)
void WebPageProxy::replyPolicyForCertificateError(bool result)
{
    if (!m_allowedReply)
        return;

    m_allowedReply->send(result);
    m_allowedReply = nullptr;
}
#endif

#if ENABLE(TIZEN_MALICIOUS_CONTENTS_SCAN)
void WebPageProxy::replyBlockingToLoadForMalwareScan(bool result)
{
    if (!m_releaseBlockReply)
        return;

    TIZEN_LOGE("WebPageProxy::replyBlockingToLoadForMalwareScan()");
    m_releaseBlockReply->send(result);
    m_releaseBlockReply = nullptr;
}
#endif

#if PLUGIN_ARCHITECTURE(X11)
void WebPageProxy::createPluginContainer(uint64_t& windowID)
{
    notImplemented();
}

void WebPageProxy::windowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID)
{
    notImplemented();
}
#endif

void WebPageProxy::didRenderFrame()
{
    static_cast<PageClientImpl*>(m_pageClient)->didRenderFrame();
}

#if ENABLE(TIZEN_WEBKIT2_TILED_AC)
bool WebPageProxy::makeContextCurrent()
{
    return static_cast<PageClientImpl*>(m_pageClient)->makeContextCurrent();
}
#endif

#if ENABLE(TIZEN_ICON_DATABASE)
void WebPageProxy::didReceiveIcon()
{
    static_cast<PageClientImpl*>(m_pageClient)->didReceiveIcon();
}
#endif

#if ENABLE(TIZEN_MULTIPLE_SELECT)
void WebPageProxy::valueChangedForPopupMenuMultiple(WebPopupMenuProxy*, Vector<int32_t> newSelectedIndex)
{
    process()->send(Messages::WebPage::DidChangeSelectedIndexForActivePopupMenuMultiple(newSelectedIndex), m_pageID);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_HISTORICAL_RESTORE_VISIBLE_CONTENT_RECT)
void WebPageProxy::pageDidRequestRestoreVisibleContentRect(const IntPoint& point, float scale)
{
    m_pageClient->pageDidRequestRestoreVisibleContentRect(point, scale);
}
#endif

#if ENABLE(TIZEN_OFFLINE_PAGE_SAVE)
void WebPageProxy::saveSerializedHTMLDataForMainPage(const String& serializedData, const String& fileName)
{
    static_cast<PageClientImpl*>(m_pageClient)->saveSerializedHTMLDataForMainPage(serializedData, fileName);
}

void WebPageProxy::saveSubresourcesData(Vector<WebSubresourceTizen> subresourceData)
{
    static_cast<PageClientImpl*>(m_pageClient)->saveSubresourcesData(subresourceData);
}

void WebPageProxy::startOfflinePageSave(String subresourceFolderName)
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::StartOfflinePageSave(subresourceFolderName), m_pageID);
}
#endif

#if ENABLE(TIZEN_WEBKIT2_TEXT_SELECTION)
bool WebPageProxy::selectClosestWord(const IntPoint& point, bool isAutoWordSelection, bool& isCaretSelection)
{
    if (!isValid())
        return false;

    bool result = false;
    process()->sendSync(Messages::WebPage::SelectClosestWord(point, isAutoWordSelection), Messages::WebPage::SelectClosestWord::Reply(result, isCaretSelection), m_pageID);
    return result;
}

int WebPageProxy::setLeftSelection(const IntPoint& point, const int direction)
{
    if (!isValid())
        return 0;

    int result = 0;
    process()->sendSync(Messages::WebPage::SetLeftSelection(point, direction), Messages::WebPage::SetLeftSelection::Reply(result), m_pageID);
    return result;
}

int WebPageProxy::setRightSelection(const IntPoint& point, const int direction)
{
    if (!isValid())
        return 0;

    int result = 0;
    process()->sendSync(Messages::WebPage::SetRightSelection(point, direction), Messages::WebPage::SetRightSelection::Reply(result), m_pageID);
    return result;
}

bool WebPageProxy::getSelectionHandlers(IntRect& leftRect, IntRect& rightRect)
{
    if (!isValid())
        return false;

    process()->sendSync(Messages::WebPage::GetSelectionHandlers(), Messages::WebPage::GetSelectionHandlers::Reply(leftRect, rightRect), m_pageID);
    return (!leftRect.isEmpty() || !rightRect.isEmpty());
}

String WebPageProxy::getSelectionText()
{
    String ret;
    if (!isValid())
        return ret;

    process()->sendSync(Messages::WebPage::GetSelectionText(), Messages::WebPage::GetSelectionText::Reply(ret), m_pageID);
    return ret;
}

void WebPageProxy::selectionRangeClear()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::SelectionRangeClear(), m_pageID);
}

bool WebPageProxy::scrollContentByCharacter(const IntPoint& point, SelectionDirection direction, bool extend)
{
    if (!isValid())
        return false;

    bool result = false;
    process()->sendSync(Messages::WebPage::ScrollContentByCharacter(point, direction, extend), Messages::WebPage::ScrollContentByCharacter::Reply(result), m_pageID);
    return result;
}

bool WebPageProxy::scrollContentByLine(const IntPoint& point, SelectionDirection direction)
{
    if (!isValid())
        return false;

    bool result = false;
    process()->sendSync(Messages::WebPage::ScrollContentByLine(point, direction), Messages::WebPage::ScrollContentByLine::Reply(result), m_pageID);
    return result;
}
#endif

#if ENABLE(TIZEN_LINK_MAGNIFIER)
void WebPageProxy::getLinkMagnifierRect(const WebCore::IntPoint& position)
{
    process()->send(Messages::WebPage::GetLinkMagnifierRect(position), m_pageID);
}

void WebPageProxy::didGetLinkMagnifierRect(const IntPoint& position, const IntRect& rect)
{
    if (!rect.isEmpty())
        LinkMagnifierProxy::linkMagnifier().show(this, position, rect);
    else
        openLink(position);
}

void WebPageProxy::openLink(const IntPoint& position)
{
#if ENABLE(GESTURE_EVENTS)
#if ENABLE(TIZEN_WEBKIT2_CONTROL_EFL_FOCUS)
    static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->setFocusManually();
#endif
    IntPoint globalPosition(static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->transformToScreen().mapPoint(position));
    WebGestureEvent gesture(WebEvent::GestureSingleTap, position, globalPosition, WebEvent::Modifiers(0), ecore_time_get());
    handleGestureEvent(gesture);
#endif
}
#endif

#if ENABLE(TIZEN_SCREEN_READER)
bool WebPageProxy::moveScreenReaderFocus(bool forward)
{
    bool result;
    process()->sendSync(Messages::WebPage::MoveScreenReaderFocus(forward), Messages::WebPage::MoveScreenReaderFocus::Reply(result), m_pageID);
    return result;
}

void WebPageProxy::moveScreenReaderFocusByPoint(const IntPoint& point, bool reload)
{
    process()->send(Messages::WebPage::MoveScreenReaderFocusByPoint(point, reload), m_pageID);
}

void WebPageProxy::clearScreenReaderFocus()
{
    process()->send(Messages::WebPage::ClearScreenReaderFocus(), m_pageID);
}

void WebPageProxy::raiseTapEvent(const IntPoint& position)
{
    EwkViewImpl* viewImpl = static_cast<PageClientImpl*>(m_pageClient)->viewImpl();
    IntPoint globalPosition = viewImpl->transformToScreen().mapPoint(position);
    process()->send(Messages::WebPage::RaiseTapEvent(position, globalPosition), m_pageID);
}

void WebPageProxy::adjustScreenReaderFocusedObjectValue(bool up)
{
    process()->send(Messages::WebPage::AdjustScreenReaderFocusedObjectValue(up), m_pageID);
}

void WebPageProxy::clearScreenReader()
{
    process()->send(Messages::WebPage::ClearScreenReader(), m_pageID);
}

bool WebPageProxy::extendParagraphOnScreenReader(int direction)
{
    bool result;
    process()->sendSync(Messages::WebPage::ExtendParagraphOnScreenReader(direction), Messages::WebPage::ExtendParagraphOnScreenReader::Reply(result), m_pageID);
    return result;
}


void WebPageProxy::didScreenReaderTextChanged(const String& text)
{
    ScreenReaderProxy::screenReader().setText(text);
}

void WebPageProxy::didScreenReaderRectsChanged(const Vector<IntRect>& rects)
{
    EwkViewImpl* viewImpl = static_cast<PageClientImpl*>(m_pageClient)->viewImpl();
    if (!ScreenReaderProxy::screenReader().isActive(viewImpl))
        return;

    FocusRing* focusRing = ScreenReaderProxy::screenReader().focusRing();
    if (!focusRing)
        return;

    focusRing->show(rects);
}

void WebPageProxy::readOutPasswords(bool& enabled)
{
    enabled = elm_config_access_password_read_enabled_get();
}

void WebPageProxy::didRaiseTapEvent(bool result, EditorState editorState)
{
#if ENABLE(TIZEN_ISF_PORT)
    EwkViewImpl* viewImpl = static_cast<PageClientImpl*>(m_pageClient)->viewImpl();
    if (result) {
        InputMethodContextEfl* inputMethodContext = viewImpl->inputMethodContext();
        if (inputMethodContext) {
            editorStateChanged(editorState);
            inputMethodContext->updateTextInputStateByUserAction(true);
        }
    }
#endif
}

#endif

#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
void WebPageProxy::didFocusedRectsChanged(const Vector<IntRect>& rects)
{
    EwkViewImpl* viewImpl = static_cast<PageClientImpl*>(m_pageClient)->viewImpl();
    FocusRing* focusRing = viewImpl->focusRing();
    if (!focusRing)
        return;

    bool useFocusUI = false;
#if ENABLE(TIZEN_FOCUS_UI)
    useFocusUI = m_focusUIEnabled;
#endif

    if (focusRing->rectsChanged(rects)) {
        EwkViewImpl* viewImpl = static_cast<PageClientImpl*>(m_pageClient)->viewImpl();
        viewImpl->hideCandidatePopup();
    }

    if (rects.isEmpty() || (!useFocusUI && focusRing->rectsChanged(rects))) {
        if (focusRing->canUpdate())
            focusRing->hide(false);
    }
    else if (focusRing->canUpdate() || useFocusUI)
        focusRing->show(rects);
}

void WebPageProxy::recalcFocusedRects()
{
    process()->send(Messages::WebPage::RecalcFocusedRects(), m_pageID);
}

void WebPageProxy::clearFocusedNode()
{
    process()->send(Messages::WebPage::ClearFocusedNode(), m_pageID);
}
#endif

#if ENABLE(TIZEN_CSP)
void WebPageProxy::setContentSecurityPolicy(const String& policy, WebCore::ContentSecurityPolicy::HeaderType type)
{
    process()->send(Messages::WebPage::SetContentSecurityPolicy(policy, type), m_pageID);
}
#endif

#if ENABLE(TIZEN_APPLICATION_CACHE)
void WebPageProxy::requestApplicationCachePermission(uint64_t frameID, const String& originIdentifier, PassRefPtr<Messages::WebPageProxy::RequestApplicationCachePermission::DelayedReply> allow)
{
    WebFrameProxy* frame = process()->webFrame(frameID);
    MESSAGE_CHECK(frame);

    // Since requestApplicationCachePermission() can spin a nested run loop we need to turn off the responsiveness timer.
    process()->responsivenessTimer()->stop();

    m_applicationCacheReply = allow;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(true);
#endif
    RefPtr<WebSecurityOrigin> origin = WebSecurityOrigin::createFromDatabaseIdentifier(originIdentifier);

    if (!m_tizenClient.decidePolicyForApplicationCachePermissionRequest(this, origin.get(), frame)) {
        replyApplicationCachePermission(true);
    }
}

void WebPageProxy::replyApplicationCachePermission(bool allow)
{
    if (!m_applicationCacheReply)
        return;

    m_applicationCacheReply->send(allow);
    m_applicationCacheReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(false);
#endif
}
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
void WebPageProxy::exceededIndexedDatabaseQuota(uint64_t frameID, const String& originIdentifier, int64_t currentUsage, PassRefPtr<Messages::WebPageProxy::ExceededIndexedDatabaseQuota::DelayedReply> reply)
{
    WebFrameProxy* frame = process()->webFrame(frameID);
    MESSAGE_CHECK(frame);

    // Since exceededIndexedDatabaseQuota() can spin a nested run loop we need to turn off the responsiveness timer.
    process()->responsivenessTimer()->stop();

    m_exceededIndexedDatabaseQuotaReply = reply;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(true);
#endif

    RefPtr<WebSecurityOrigin> origin = WebSecurityOrigin::createFromDatabaseIdentifier(originIdentifier);

    if (!m_tizenClient.exceededIndexedDatabaseQuota(this, origin.get(), currentUsage, frame))
        replyExceededIndexedDatabaseQuota(false);
}

void WebPageProxy::replyExceededIndexedDatabaseQuota(bool allow)
{
    if (!m_exceededIndexedDatabaseQuotaReply)
        return;

    m_exceededIndexedDatabaseQuotaReply->send(allow);
    m_exceededIndexedDatabaseQuotaReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(false);
#endif
}
#endif

#if ENABLE(TIZEN_SQL_DATABASE)
void WebPageProxy::replyExceededDatabaseQuota(bool allow)
{
    if (!m_exceededDatabaseQuotaReply) {
        TIZEN_LOGE("m_exceededDatabaseQuotaReply does not exist");
        return;
    }

    m_exceededDatabaseQuotaReply->send(allow);
    m_exceededDatabaseQuotaReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(false);
#endif
}
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
void WebPageProxy::exceededLocalFileSystemQuota(uint64_t frameID, const String& originIdentifier, int64_t currentUsage, PassRefPtr<Messages::WebPageProxy::ExceededLocalFileSystemQuota::DelayedReply> reply)
{
    WebFrameProxy* frame = process()->webFrame(frameID);
    MESSAGE_CHECK(frame);

    // Since exceededLocalFileSystemQuota() can spin a nested run loop we need to turn off the responsiveness timer.
    process()->responsivenessTimer()->stop();
    m_exceededLocalFileSystemQuotaReply = reply;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(true);
#endif

    RefPtr<WebSecurityOrigin> origin = WebSecurityOrigin::createFromDatabaseIdentifier(originIdentifier);

    if (!m_tizenClient.exceededLocalFileSystemQuota(this, origin.get(), currentUsage, frame))
        replyExceededLocalFileSystemQuota(false);
}

void WebPageProxy::replyExceededLocalFileSystemQuota(bool allow)
{
    if (!m_exceededLocalFileSystemQuotaReply)
        return;

    m_exceededLocalFileSystemQuotaReply->send(allow);
    m_exceededLocalFileSystemQuotaReply = nullptr;
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    process()->connection()->setForcelySetAllAsyncMessagesToDispatchEvenWhenWaitingForSyncReply(false);
#endif
}
#endif

#endif // #if PLATFORM(TIZEN)

void WebPageProxy::handleInputMethodKeydown(bool& handled)
{
#if ENABLE(TIZEN_WEBKIT2_ROTATION_WHILE_JAVASCRIPT_POPUP)
    handled = m_keyEventQueue.first().forwardedEvent.isFiltered();
#else
    handled = m_keyEventQueue.first().isFiltered();
#endif
}

void WebPageProxy::confirmComposition(const String& compositionString)
{
    if (!isValid())
        return;

#if ENABLE(TIZEN_ISF_PORT)
    if (m_didCancelCompositionFromWebProcess)
        return;
#endif

    process()->send(Messages::WebPage::ConfirmComposition(compositionString), m_pageID, 0);
}

void WebPageProxy::setComposition(const String& compositionString, Vector<WebCore::CompositionUnderline>& underlines, int cursorPosition)
{
    if (!isValid())
        return;

#if ENABLE(TIZEN_UPDATE_ONLY_VISIBLE_AREA_STATE)
    if (m_updateOnlyVisibleAreaStateTimer)
        ecore_timer_del(m_updateOnlyVisibleAreaStateTimer);
    m_updateOnlyVisibleAreaStateTimer = ecore_timer_add(1, updateOnlyVisibleAreaStateTimerFired, this);

    if (!m_updateOnlyVisibleAreaState) {
        process()->send(Messages::LayerTreeCoordinator::UpdateOnlyVisibleAreaState(true), m_pageID);
        m_updateOnlyVisibleAreaState = true;
    }
#endif

    process()->send(Messages::WebPage::SetComposition(compositionString, underlines, cursorPosition), m_pageID, 0);
}

void WebPageProxy::cancelComposition()
{
    if (!isValid())
        return;

    process()->send(Messages::WebPage::CancelComposition(), m_pageID, 0);
}

#if ENABLE(TIZEN_CSS_OVERFLOW_SCROLL_ACCELERATION)
bool WebPageProxy::askOverflow(const IntPoint& point)
{
    // We don't have to ask overflow if we send touch event to the web process,
    // because we will check overflow in the web process when touch event is processed.
#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
    return !m_needTouchEvents || !isPointInTouchEventTargetArea(point);
#else
    UNUSED_PARAM(point);
    return !m_needTouchEvents;
#endif
}
#endif

#if ENABLE(TIZEN_TOUCH_EVENT_TRACKING)
void WebPageProxy::setTouchEventTargetRects(const Vector<IntRect>& touchEventTargetRects)
{
    m_touchEventTargetRects = touchEventTargetRects;
}

bool WebPageProxy::isPointInTouchEventTargetArea(const IntPoint& point)
{
    for (int i = m_touchEventTargetRects.size() - 1; i >= 0; --i)
        if (m_touchEventTargetRects[i].contains(point))
            return true;
    return false;
}
#endif

#if ENABLE(TIZEN_USE_SETTINGS_FONT)
void WebPageProxy::useSettingsFont()
{
    process()->send(Messages::WebPage::UseSettingsFont(), m_pageID, 0);
}
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void WebPageProxy::mediaControlsRequestRotate(const String& status)
{
    static_cast<PageClientImpl*>(m_pageClient)->mediaControlsRequestRotate(status);
}

#if ENABLE(APP_LOGGING_FOR_MEDIA)
void WebPageProxy::videoStreamingCount()
{
    static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->videoStreamingCount();
}
#endif
#endif

#if ENABLE(TIZEN_WEBKIT2_NOTIFY_SUSPEND_BY_REMOTE_WEB_INSPECTOR)
void WebPageProxy::setContentSuspendedByInspector(bool isSuspended)
{
    m_contentSuspendedByInspector = isSuspended;
}
#endif

#if ENABLE(TIZEN_GET_COOKIES_FOR_URL)
void WebPageProxy::getCookiesForURL(const String& url, String& cookiesForURL)
{
    if (!isValid())
        return;

    process()->sendSync(Messages::WebPage::GetCookiesForURL(url), Messages::WebPage::GetCookiesForURL::Reply(cookiesForURL), m_pageID);
}
#endif

#if ENABLE(TIZEN_DISPLAY_MESSAGE_TO_CONSOLE)
void WebPageProxy::addMessageToConsole(uint32_t level, const String& message, uint32_t lineNumber, const String& source)
{
    static_cast<PageClientImpl*>(m_pageClient)->addMessageToConsole(level, message, lineNumber, source);
}
#endif

#if ENABLE(TIZEN_PROD_DETECT_CONTENTS)
void WebPageProxy::didDetectContents(const IntPoint& tapPosition, const String& detectedContents, const IntRect& detectedContentsRect, const Color& highlightColor)
{
#if ENABLE(TIZEN_WEBKIT2_FOCUS_RING)
    EwkViewImpl* viewImpl = static_cast<PageClientImpl*>(m_pageClient)->viewImpl();
    FocusRing* focusRing = viewImpl->focusRing();
    if (focusRing) {
        Vector<IntRect> rects;
        rects.append(detectedContentsRect);

        focusRing->setColor(highlightColor);
        focusRing->show(rects);
        focusRing->requestToHide();
    }
#endif

    m_tapPositionForDetectedContents = tapPosition;
    m_detectedContents = detectedContents;

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
    if (!m_showContextMenuForDetectedContentsTimer.isActive())
        m_showContextMenuForDetectedContentsTimer.startOneShot(0);
#endif
}

#if ENABLE(TIZEN_CONTEXT_MENU_WEBKIT_2)
void WebPageProxy::showContextMenuForDetectedContentsTimerFired(Timer<WebPageProxy>*)
{
    showContextMenuForDetectedContents(m_tapPositionForDetectedContents, m_detectedContents);
}

void WebPageProxy::showContextMenuForDetectedContents(const IntPoint& tapPosition, const String& detectedContents)
{
    TIZEN_LOGI("contents,detected : detectedContents -> [%s]", detectedContents.utf8().data());

    WebHitTestResult::Data hitTestResultData;
    hitTestResultData.absoluteLinkURL = detectedContents;

    m_activeContextMenuHitTestResultData = hitTestResultData;

    if (m_activeContextMenu) {
        m_activeContextMenu->hideContextMenu();
        m_activeContextMenu = 0;
    }

    m_activeContextMenu = m_pageClient->createContextMenuProxy(this);

    Vector<WebContextMenuItemData> proposedItems;
#if ENABLE(TIZEN_WEBKIT2_CONTEXT_MENU_TEXT_SELECTION_MODE)
    WebContextMenuItemData selectionModeItem(ActionType, ContextMenuItemTagTextSelectionMode, contextMenuItemTagTextSelectionMode(), true, false);
    proposedItems.append(selectionModeItem);
#endif

    // Give the PageContextMenuClient one last swipe at changing the menu.
    Vector<WebContextMenuItemData> items;
    if (!m_contextMenuClient.getContextMenuFromProposedMenu(this, proposedItems, items, hitTestResultData, 0))
        m_activeContextMenu->showContextMenu(tapPosition, proposedItems);
    else
        m_activeContextMenu->showContextMenu(tapPosition, items);

    m_contextMenuClient.contextMenuDismissed(this);
}
#endif
#endif

#if ENABLE(TIZEN_FOCUS_UI)
void WebPageProxy::setFocusUIEnabled(bool enabled)
{
    if (m_focusUIEnabled == enabled)
        return;

    m_focusUIEnabled = enabled;

    if (enabled)
        static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->focusRing()->setImage(FOCUS_UI_FOCUS_RING_IMAGE_PATH, 3, 2);
    else
        static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->focusRing()->setImage(String(), 0, 0);

    process()->send(Messages::WebPage::SetFocusUIEnabled(enabled), m_pageID);
}

void WebPageProxy::suspendFocusUI()
{
    if (!m_focusUIEnabled)
        return;

    process()->send(Messages::WebPage::SetFocusUIEnabled(false), m_pageID);
}

void WebPageProxy::resumeFocusUI()
{
#if ENABLE(TIZEN_FULLSCREEN_API)
    if (m_fullScreenManager->isFullScreen())
        return;
#endif

    if (!m_focusUIEnabled) {
        const Evas_Object* win = elm_object_top_widget_get(elm_object_parent_widget_get(viewWidget()));
        const char* type = evas_object_type_get(win);
        if (type && !strcmp(type, "elm_win") && elm_win_focus_highlight_enabled_get(win))
            setFocusUIEnabled(true);

        return;
    }

    process()->send(Messages::WebPage::SetFocusUIEnabled(true), m_pageID);
}
#endif

#if ENABLE(TIZEN_BROWSER_FONT)
void WebPageProxy::setBrowserFont()
{
    process()->send(Messages::WebPage::SetBrowserFont(), m_pageID, 0);
}
#endif

#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)
PassRefPtr<WebData> WebPageProxy::sessionStateData(WebPageProxySessionStateFilterCallback, void*) const
{
    return m_backForwardList->sessionStateData();
}

void WebPageProxy::restoreFromSessionStateData(WebData* webData)
{
    m_backForwardList->restoreFromSavedSession(reinterpret_cast<const char*>(webData->bytes()), webData->size());
}

void WebPageProxy::saveHistoryItem(void)
{
    if (!isValid()) {
#if ENABLE(TIZEN_DLOG_SUPPORT)
        TIZEN_LOGI("[BackForward] !isValid()");
#endif
        return;
    }

#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[BackForward] saveHistoryItem");
#endif
    process()->send(Messages::WebPage::saveHistoryItem(), m_pageID);
}

void WebPageProxy::backForwardItemChanged(void)
{
#if ENABLE(TIZEN_DLOG_SUPPORT)
    TIZEN_LOGI("[BackForward] call saveSessionData()");
#endif
    static_cast<PageClientImpl*>(m_pageClient)->viewImpl()->saveSessionData();
}
#endif //#if ENABLE(TIZEN_BACK_FORWARD_LIST_STORE_RESTORE)

#if ENABLE(TIZEN_PRERENDERING_FOR_ROTATION)
void WebPageProxy::resizeEventDone()
{
    static_cast<PageClientImpl*>(m_pageClient)->resizeEventDone();
}
#endif

#if HAVE(ACCESSIBILITY)
void WebPageProxy::bindAccessibilityTree(const String& plugID)
{
    m_accessibilityPlugID = plugID.utf8().data();
    evas_object_data_set(viewWidget(), "__PlugID", m_accessibilityPlugID);
}
#endif

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void WebPageProxy::setGrabMediaKey(bool grab)
{
    EwkViewImpl* viewImpl = static_cast<PageClientImpl*>(m_pageClient)->viewImpl();
    InputMethodContextEfl* inputMethodContext = viewImpl->inputMethodContext();
    if (grab)
        inputMethodContext->grabMediaKey();
    else
        inputMethodContext->ungrabMediaKey();
}
#endif

} // namespace WebKit
