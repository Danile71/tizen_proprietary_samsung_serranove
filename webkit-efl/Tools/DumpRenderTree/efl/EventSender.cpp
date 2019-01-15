/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Zan Dobersek <zandobersek@gmail.com>
 * Copyright (C) 2009 Holger Hans Peter Freyther
 * Copyright (C) 2010 Igalia S.L.
 * Copyright (C) 2011 ProFUSION Embedded Systems
 * Copyright (C) 2011, 2012 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "EventSender.h"

#if ENABLE(TIZEN_WEBKIT_EFL_DRT)
#include "DumpRenderTree.h"
#include "DumpRenderTreeChrome.h"
#include "IntPoint.h"
#include "JSStringUtils.h"
#include "NotImplemented.h"
#include "PlatformEvent.h"
#include "WebCoreSupport/DumpRenderTreeSupportEfl.h"
#include "ewk_private.h"
#include <EWebKit.h>
#include <Ecore_Input.h>
#include <JavaScriptCore/JSObjectRef.h>
#include <JavaScriptCore/JSRetainPtr.h>
#include <JavaScriptCore/JSStringRef.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <wtf/ASCIICType.h>
#include <wtf/Platform.h>
#include <wtf/text/CString.h>

static bool gDragMode;
static int gTimeOffset = 0;

static int gLastMousePositionX;
static int gLastMousePositionY;
static int gLastClickPositionX;
static int gLastClickPositionY;
static int gLastClickTimeOffset;
static int gLastClickButton;
static int gButtonCurrentlyDown;
static int gClickCount;

static const float zoomMultiplierRatio = 1.2f;

// Key event location code defined in DOM Level 3.
enum KeyLocationCode {
    DomKeyLocationStandard,
    DomKeyLocationLeft,
    DomKeyLocationRight,
    DomKeyLocationNumpad
};

enum EvasKeyModifier {
    EvasKeyModifierNone    = 0,
    EvasKeyModifierControl = 1 << 0,
    EvasKeyModifierShift   = 1 << 1,
    EvasKeyModifierAlt     = 1 << 2,
    EvasKeyModifierMeta    = 1 << 3
};

enum EvasMouseButton {
    EvasMouseButtonNone,
    EvasMouseButtonLeft,
    EvasMouseButtonMiddle,
    EvasMouseButtonRight
};

enum EvasMouseEvent {
    EvasMouseEventNone        = 0,
    EvasMouseEventDown        = 1 << 0,
    EvasMouseEventUp          = 1 << 1,
    EvasMouseEventMove        = 1 << 2,
    EvasMouseEventScrollUp    = 1 << 3,
    EvasMouseEventScrollDown  = 1 << 4,
    EvasMouseEventScrollLeft  = 1 << 5,
    EvasMouseEventScrollRight = 1 << 6,
    EvasMouseEventClick       = EvasMouseEventMove | EvasMouseEventDown | EvasMouseEventUp,
};

enum ZoomEvent {
    ZoomIn,
    ZoomOut
};

enum EventQueueStrategy {
    FeedQueuedEvents,
    DoNotFeedQueuedEvents
};

struct KeyEventInfo {
    KeyEventInfo(const CString& keyName, unsigned modifiers, const CString& keyString = CString())
        : keyName(keyName)
        , keyString(keyString)
        , modifiers(modifiers)
    {
    }

    const CString keyName;
    const CString keyString;
    unsigned modifiers;
};

struct MouseEventInfo {
    MouseEventInfo(EvasMouseEvent event, unsigned modifiers = EvasKeyModifierNone, EvasMouseButton button = EvasMouseButtonNone, int horizontalDelta = 0, int verticalDelta = 0)
        : event(event)
        , modifiers(modifiers)
        , button(button)
        , horizontalDelta(horizontalDelta)
        , verticalDelta(verticalDelta)
    {
    }

    EvasMouseEvent event;
    unsigned modifiers;
    EvasMouseButton button;
    int horizontalDelta;
    int verticalDelta;
};

struct DelayedEvent {
    DelayedEvent(MouseEventInfo* eventInfo, unsigned long delay = 0)
        : eventInfo(eventInfo)
        , delay(delay)
    {
    }

    MouseEventInfo* eventInfo;
    unsigned long delay;
};

struct TouchEventInfo {
    TouchEventInfo(unsigned id, Ewk_Touch_Point_Type state, const WebCore::IntPoint& point)
        : state(state)
        , point(point)
        , id(id)
    {
    }

    unsigned id;
    Ewk_Touch_Point_Type state;
    WebCore::IntPoint point;
};

static unsigned touchModifiers;

WTF::Vector<TouchEventInfo>& touchPointList()
{
    DEFINE_STATIC_LOCAL(WTF::Vector<TouchEventInfo>, staticTouchPointList, ());
    return staticTouchPointList;
}

WTF::Vector<DelayedEvent>& delayedEventQueue()
{
    DEFINE_STATIC_LOCAL(WTF::Vector<DelayedEvent>, staticDelayedEventQueue, ());
    return staticDelayedEventQueue;
}


static void feedOrQueueMouseEvent(MouseEventInfo*, EventQueueStrategy);
static void feedMouseEvent(MouseEventInfo*);
static void feedQueuedMouseEvents();

static void setEvasModifiers(Evas* evas, unsigned modifiers)
{
    static const char* modifierNames[] = { "Control", "Shift", "Alt", "Super" };
    for (unsigned modifier = 0; modifier < 4; ++modifier) {
        if (modifiers & (1 << modifier))
            evas_key_modifier_on(evas, modifierNames[modifier]);
        else
            evas_key_modifier_off(evas, modifierNames[modifier]);
    }
}

static EvasMouseButton translateMouseButtonNumber(int eventSenderButtonNumber)
{
    static const EvasMouseButton translationTable[] = {
        EvasMouseButtonLeft,
        EvasMouseButtonMiddle,
        EvasMouseButtonRight,
        EvasMouseButtonMiddle // fast/events/mouse-click-events expects the 4th button to be treated as the middle button
    };
    static const unsigned translationTableSize = sizeof(translationTable) / sizeof(translationTable[0]);

    if (eventSenderButtonNumber < translationTableSize)
        return translationTable[eventSenderButtonNumber];

    return EvasMouseButtonLeft;
}

static Eina_Bool sendClick(void*)
{
    MouseEventInfo* eventInfo = new MouseEventInfo(EvasMouseEventClick, EvasKeyModifierNone, EvasMouseButtonLeft);
    feedOrQueueMouseEvent(eventInfo, FeedQueuedEvents);
    return ECORE_CALLBACK_CANCEL;
}

static JSValueRef scheduleAsynchronousClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    ecore_idler_add(sendClick, 0);
    return JSValueMakeUndefined(context);
}

static void updateClickCount(int button)
{
    if (gLastClickPositionX != gLastMousePositionX
        || gLastClickPositionY != gLastMousePositionY
        || gLastClickButton != button
        || gTimeOffset - gLastClickTimeOffset >= 1)
        gClickCount = 1;
    else
        gClickCount++;
}

static EvasKeyModifier modifierFromJSValue(JSContextRef context, const JSValueRef value)
{
    JSRetainPtr<JSStringRef> jsKeyValue(Adopt, JSValueToStringCopy(context, value, 0));

    if (equals(jsKeyValue, "ctrlKey") || equals(jsKeyValue, "addSelectionKey"))
        return EvasKeyModifierControl;
    if (equals(jsKeyValue, "shiftKey") || equals(jsKeyValue, "rangeSelectionKey"))
        return EvasKeyModifierShift;
    if (equals(jsKeyValue, "altKey"))
        return EvasKeyModifierAlt;
    if (equals(jsKeyValue, "metaKey"))
        return EvasKeyModifierMeta;

    return EvasKeyModifierNone;
}

static unsigned modifiersFromJSValue(JSContextRef context, const JSValueRef modifiers)
{
    // The value may either be a string with a single modifier or an array of modifiers.
    if (JSValueIsString(context, modifiers))
        return modifierFromJSValue(context, modifiers);

    JSObjectRef modifiersArray = JSValueToObject(context, modifiers, 0);
    if (!modifiersArray)
        return EvasKeyModifierNone;

    unsigned modifier = EvasKeyModifierNone;
    JSRetainPtr<JSStringRef> lengthProperty(Adopt, JSStringCreateWithUTF8CString("length"));
    int modifiersCount = JSValueToNumber(context, JSObjectGetProperty(context, modifiersArray, lengthProperty.get(), 0), 0);
    for (int i = 0; i < modifiersCount; ++i)
        modifier |= modifierFromJSValue(context, JSObjectGetPropertyAtIndex(context, modifiersArray, i, 0));
    return modifier;
}

static JSValueRef mouseDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int button = 0;
    if (argumentCount == 1) {
        button = static_cast<int>(JSValueToNumber(context, arguments[0], exception));

        if (exception && *exception)
            return JSValueMakeUndefined(context);
    }

    button = translateMouseButtonNumber(button);
    // If the same mouse button is already in the down position don't send another event as it may confuse Xvfb.
    if (gButtonCurrentlyDown == button)
        return JSValueMakeUndefined(context);

    updateClickCount(button);

    unsigned modifiers = argumentCount >= 2 ? modifiersFromJSValue(context, arguments[1]) : EvasKeyModifierNone;
    MouseEventInfo* eventInfo = new MouseEventInfo(EvasMouseEventDown, modifiers, static_cast<EvasMouseButton>(button));
    feedOrQueueMouseEvent(eventInfo, FeedQueuedEvents);
    gButtonCurrentlyDown = button;
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseUpCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    int button = 0;
    if (argumentCount == 1) {
        button = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
        if (exception && *exception)
            return JSValueMakeUndefined(context);
    }

    gLastClickPositionX = gLastMousePositionX;
    gLastClickPositionY = gLastMousePositionY;
    gLastClickButton = gButtonCurrentlyDown;
    gLastClickTimeOffset = gTimeOffset;
    gButtonCurrentlyDown = 0;

    unsigned modifiers = argumentCount >= 2 ? modifiersFromJSValue(context, arguments[1]) : EvasKeyModifierNone;
    MouseEventInfo* eventInfo = new MouseEventInfo(EvasMouseEventUp, modifiers, translateMouseButtonNumber(button));
    feedOrQueueMouseEvent(eventInfo, FeedQueuedEvents);
    return JSValueMakeUndefined(context);
}

static JSValueRef mouseMoveToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    gLastMousePositionX = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    if (exception && *exception)
        return JSValueMakeUndefined(context);
    gLastMousePositionY = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    if (exception && *exception)
        return JSValueMakeUndefined(context);

    MouseEventInfo* eventInfo = new MouseEventInfo(EvasMouseEventMove);
    feedOrQueueMouseEvent(eventInfo, DoNotFeedQueuedEvents);
    return JSValueMakeUndefined(context);
}

static JSValueRef leapForwardCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount > 0) {
        const unsigned long leapForwardDelay = JSValueToNumber(context, arguments[0], exception);
        if (delayedEventQueue().isEmpty())
            delayedEventQueue().append(DelayedEvent(0, leapForwardDelay));
        else
            delayedEventQueue().last().delay = leapForwardDelay;
        gTimeOffset += leapForwardDelay;
    }

    return JSValueMakeUndefined(context);
}

static EvasMouseEvent evasMouseEventFromHorizontalAndVerticalOffsets(int horizontalOffset, int verticalOffset)
{
    unsigned mouseEvent = 0;

    if (verticalOffset > 0)
        mouseEvent |= EvasMouseEventScrollUp;
    else if (verticalOffset < 0)
        mouseEvent |= EvasMouseEventScrollDown;

    if (horizontalOffset > 0)
        mouseEvent |= EvasMouseEventScrollRight;
    else if (horizontalOffset < 0)
        mouseEvent |= EvasMouseEventScrollLeft;

    return static_cast<EvasMouseEvent>(mouseEvent);
}

static JSValueRef mouseScrollByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    // We need to invert scrolling values since in EFL negative z value means that
    // canvas is scrolling down
    const int horizontal = -(static_cast<int>(JSValueToNumber(context, arguments[0], exception)));
    if (exception && *exception)
        return JSValueMakeUndefined(context);
    const int vertical = -(static_cast<int>(JSValueToNumber(context, arguments[1], exception)));
    if (exception && *exception)
        return JSValueMakeUndefined(context);

    MouseEventInfo* eventInfo = new MouseEventInfo(evasMouseEventFromHorizontalAndVerticalOffsets(horizontal, vertical), EvasKeyModifierNone, EvasMouseButtonNone, horizontal, vertical);
    feedOrQueueMouseEvent(eventInfo, FeedQueuedEvents);
    return JSValueMakeUndefined(context);
}

static JSValueRef continuousMouseScrollByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeUndefined(context);
}

static KeyEventInfo* keyPadNameFromJSValue(JSStringRef character, unsigned modifiers)
{
    if (equals(character, "leftArrow"))
        return new KeyEventInfo("KP_Left", modifiers);
    if (equals(character, "rightArrow"))
        return new KeyEventInfo("KP_Right", modifiers);
    if (equals(character, "upArrow"))
        return new KeyEventInfo("KP_Up", modifiers);
    if (equals(character, "downArrow"))
        return new KeyEventInfo("KP_Down", modifiers);
    if (equals(character, "pageUp"))
        return new KeyEventInfo("KP_Prior", modifiers);
    if (equals(character, "pageDown"))
        return new KeyEventInfo("KP_Next", modifiers);
    if (equals(character, "home"))
        return new KeyEventInfo("KP_Home", modifiers);
    if (equals(character, "end"))
        return new KeyEventInfo("KP_End", modifiers);
    if (equals(character, "insert"))
        return new KeyEventInfo("KP_Insert", modifiers);
    if (equals(character, "delete"))
        return new KeyEventInfo("KP_Delete", modifiers);

    return new KeyEventInfo(character->ustring().utf8(), modifiers, character->ustring().utf8());
}

static KeyEventInfo* keyNameFromJSValue(JSStringRef character, unsigned modifiers)
{
    if (equals(character, "leftArrow"))
        return new KeyEventInfo("Left", modifiers);
    if (equals(character, "rightArrow"))
        return new KeyEventInfo("Right", modifiers);
    if (equals(character, "upArrow"))
        return new KeyEventInfo("Up", modifiers);
    if (equals(character, "downArrow"))
        return new KeyEventInfo("Down", modifiers);
    if (equals(character, "pageUp"))
        return new KeyEventInfo("Prior", modifiers);
    if (equals(character, "pageDown"))
        return new KeyEventInfo("Next", modifiers);
    if (equals(character, "home"))
        return new KeyEventInfo("Home", modifiers);
    if (equals(character, "end"))
        return new KeyEventInfo("End", modifiers);
    if (equals(character, "insert"))
        return new KeyEventInfo("Insert", modifiers);
    if (equals(character, "delete"))
        return new KeyEventInfo("Delete", modifiers);
    if (equals(character, "printScreen"))
        return new KeyEventInfo("Print", modifiers);
    if (equals(character, "menu"))
        return new KeyEventInfo("Menu", modifiers);
    if (equals(character, "leftControl"))
        return new KeyEventInfo("Control_L", modifiers);
    if (equals(character, "rightControl"))
        return new KeyEventInfo("Control_R", modifiers);
    if (equals(character, "leftShift"))
        return new KeyEventInfo("Shift_L", modifiers);
    if (equals(character, "rightShift"))
        return new KeyEventInfo("Shift_R", modifiers);
    if (equals(character, "leftAlt"))
        return new KeyEventInfo("Alt_L", modifiers);
    if (equals(character, "rightAlt"))
        return new KeyEventInfo("Alt_R", modifiers);
    if (equals(character, "F1"))
        return new KeyEventInfo("F1", modifiers);
    if (equals(character, "F2"))
        return new KeyEventInfo("F2", modifiers);
    if (equals(character, "F3"))
        return new KeyEventInfo("F3", modifiers);
    if (equals(character, "F4"))
        return new KeyEventInfo("F4", modifiers);
    if (equals(character, "F5"))
        return new KeyEventInfo("F5", modifiers);
    if (equals(character, "F6"))
        return new KeyEventInfo("F6", modifiers);
    if (equals(character, "F7"))
        return new KeyEventInfo("F7", modifiers);
    if (equals(character, "F8"))
        return new KeyEventInfo("F8", modifiers);
    if (equals(character, "F9"))
        return new KeyEventInfo("F9", modifiers);
    if (equals(character, "F10"))
        return new KeyEventInfo("F10", modifiers);
    if (equals(character, "F11"))
        return new KeyEventInfo("F11", modifiers);
    if (equals(character, "F12"))
        return new KeyEventInfo("F12", modifiers);

    int charCode = JSStringGetCharactersPtr(character)[0];
    if (charCode == '\n' || charCode == '\r')
        return new KeyEventInfo("Return", modifiers, "\r");
    if (charCode == '\t')
        return new KeyEventInfo("Tab", modifiers, "\t");
    if (charCode == '\x8')
        return new KeyEventInfo("BackSpace", modifiers, "\x8");
    if (charCode == ' ')
        return new KeyEventInfo("space", modifiers, " ");
    if (charCode == '\x1B')
        return new KeyEventInfo("Escape", modifiers, "\x1B");

    if ((character->length() == 1) && (charCode >= 'A' && charCode <= 'Z'))
        modifiers |= EvasKeyModifierShift;

    return new KeyEventInfo(character->ustring().utf8(), modifiers, character->ustring().utf8());
}

static KeyEventInfo* createKeyEventInfo(JSContextRef context, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (!ewk_frame_view_get(browser->mainFrame()))
        return 0;

    if (argumentCount < 1)
        return 0;

    // handle location argument.
    int location = DomKeyLocationStandard;
    if (argumentCount > 2)
        location = static_cast<int>(JSValueToNumber(context, arguments[2], exception));

    JSRetainPtr<JSStringRef> character(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    if (exception && *exception)
        return 0;

    unsigned modifiers = EvasKeyModifierNone;
    if (argumentCount >= 2)
        modifiers = modifiersFromJSValue(context, arguments[1]);

    return (location == DomKeyLocationNumpad) ? keyPadNameFromJSValue(character.get(), modifiers) : keyNameFromJSValue(character.get(), modifiers);
}

static void sendKeyDown(Evas* evas, KeyEventInfo* keyEventInfo)
{
    if (!keyEventInfo)
        return;

    const char* keyName = keyEventInfo->keyName.data();
    const char* keyString = keyEventInfo->keyString.data();
    unsigned modifiers = keyEventInfo->modifiers;

    DumpRenderTreeSupportEfl::layoutFrame(browser->mainFrame());

    ASSERT(evas);
    setEvasModifiers(evas, modifiers);
    evas_event_feed_key_down(evas, keyName, keyName, keyString, 0, 0, 0);
    evas_event_feed_key_up(evas, keyName, keyName, keyString, 0, 1, 0);
    setEvasModifiers(evas, EvasKeyModifierNone);

    DumpRenderTreeSupportEfl::deliverAllMutationsIfNecessary();
}

static JSValueRef keyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    OwnPtr<KeyEventInfo> keyEventInfo = adoptPtr(createKeyEventInfo(context, argumentCount, arguments, exception));
    sendKeyDown(evas_object_evas_get(browser->mainFrame()), keyEventInfo.get());

    return JSValueMakeUndefined(context);
}

static JSValueRef scalePageByCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount < 3)
        return JSValueMakeUndefined(context);

    Evas_Object* view = ewk_frame_view_get(browser->mainFrame());
    if (!view)
        return JSValueMakeUndefined(context);

    float scaleFactor = JSValueToNumber(context, arguments[0], exception);
    float x = JSValueToNumber(context, arguments[1], exception);
    float y = JSValueToNumber(context, arguments[2], exception);
    ewk_view_scale_set(view, scaleFactor, x, y);

    return JSValueMakeUndefined(context);
}

static void textZoom(ZoomEvent zoomEvent)
{
    Evas_Object* view = ewk_frame_view_get(browser->mainFrame());
    if (!view)
        return;

    float zoomFactor = ewk_view_text_zoom_get(view);
    if (zoomEvent == ZoomIn)
        zoomFactor *= zoomMultiplierRatio;
    else
        zoomFactor /= zoomMultiplierRatio;

    ewk_view_text_zoom_set(view, zoomFactor);
}

static void pageZoom(ZoomEvent zoomEvent)
{
    Evas_Object* view = ewk_frame_view_get(browser->mainFrame());
    if (!view)
        return;

    float zoomFactor = ewk_view_page_zoom_get(view);
    if (zoomEvent == ZoomIn)
        zoomFactor *= zoomMultiplierRatio;
    else
        zoomFactor /= zoomMultiplierRatio;

    ewk_view_page_zoom_set(view, zoomFactor);
}

static JSValueRef textZoomInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    textZoom(ZoomIn);
    return JSValueMakeUndefined(context);
}

static JSValueRef textZoomOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    textZoom(ZoomOut);
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    pageZoom(ZoomIn);
    return JSValueMakeUndefined(context);
}

static JSValueRef zoomPageOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    pageZoom(ZoomOut);
    return JSValueMakeUndefined(context);
}

static Eina_Bool sendAsynchronousKeyDown(void* userData)
{
    OwnPtr<KeyEventInfo> keyEventInfo = adoptPtr(static_cast<KeyEventInfo*>(userData));
    sendKeyDown(evas_object_evas_get(browser->mainFrame()), keyEventInfo.get());
    return ECORE_CALLBACK_CANCEL;
}

static JSValueRef scheduleAsynchronousKeyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    KeyEventInfo* keyEventInfo = createKeyEventInfo(context, argumentCount, arguments, exception);
    ecore_idler_add(sendAsynchronousKeyDown, static_cast<void*>(keyEventInfo));
    return JSValueMakeUndefined(context);
}

static void sendTouchEvent(Ewk_Touch_Event_Type type)
{
    Eina_List* eventList = 0;

    for (unsigned i = 0; i < touchPointList().size(); ++i) {
        Ewk_Touch_Point* event = new Ewk_Touch_Point;
        WebCore::IntPoint point = touchPointList().at(i).point;
        event->id = touchPointList().at(i).id;
        event->x = point.x();
        event->y = point.y();
        event->state = touchPointList().at(i).state;
        eventList = eina_list_append(eventList, event);
    }

    ewk_frame_feed_touch_event(browser->mainFrame(), type, eventList, touchModifiers);

    void* listData;
    EINA_LIST_FREE(eventList, listData) {
        Ewk_Touch_Point* event = static_cast<Ewk_Touch_Point*>(listData);
        delete event;
    }

    for (unsigned i = 0; i < touchPointList().size(); ) {
        if (touchPointList().at(i).state == EWK_TOUCH_POINT_RELEASED)
            touchPointList().remove(i);
        else {
            touchPointList().at(i).state = EWK_TOUCH_POINT_STATIONARY;
            ++i;
        }
    }
}

static JSValueRef addTouchPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 2)
        return JSValueMakeUndefined(context);

    int x = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    int y = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);

    const WebCore::IntPoint point(x, y);
    const unsigned id = touchPointList().isEmpty() ? 0 : touchPointList().last().id + 1;
    TouchEventInfo eventInfo(id, EWK_TOUCH_POINT_PRESSED, point);
    touchPointList().append(eventInfo);

    return JSValueMakeUndefined(context);
}

static JSValueRef touchStartCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    sendTouchEvent(EWK_TOUCH_START);
    return JSValueMakeUndefined(context);
}

static JSValueRef updateTouchPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 3)
        return JSValueMakeUndefined(context);

    int index = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    int x = static_cast<int>(JSValueToNumber(context, arguments[1], exception));
    ASSERT(!exception || !*exception);
    int y = static_cast<int>(JSValueToNumber(context, arguments[2], exception));
    ASSERT(!exception || !*exception);

    if (index < 0 || index >= touchPointList().size())
        return JSValueMakeUndefined(context);

    WebCore::IntPoint& point = touchPointList().at(index).point;
    point.setX(x);
    point.setY(y);
    touchPointList().at(index).state = EWK_TOUCH_POINT_MOVED;

    return JSValueMakeUndefined(context);
}

static JSValueRef touchMoveCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    sendTouchEvent(EWK_TOUCH_MOVE);
    return JSValueMakeUndefined(context);
}

static JSValueRef cancelTouchPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return JSValueMakeUndefined(context);

    int index = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    if (index < 0 || index >= touchPointList().size())
        return JSValueMakeUndefined(context);

    touchPointList().at(index).state = EWK_TOUCH_POINT_CANCELLED;
    return JSValueMakeUndefined(context);
}

static JSValueRef touchCancelCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    sendTouchEvent(EWK_TOUCH_CANCEL);
    return JSValueMakeUndefined(context);
}

static JSValueRef releaseTouchPointCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 1)
        return JSValueMakeUndefined(context);

    int index = static_cast<int>(JSValueToNumber(context, arguments[0], exception));
    ASSERT(!exception || !*exception);
    if (index < 0 || index >= touchPointList().size())
        return JSValueMakeUndefined(context);

    touchPointList().at(index).state = EWK_TOUCH_POINT_RELEASED;
    return JSValueMakeUndefined(context);
}

static JSValueRef touchEndCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    sendTouchEvent(EWK_TOUCH_END);
    touchModifiers = 0;
    return JSValueMakeUndefined(context);
}

static JSValueRef clearTouchPointsCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    touchPointList().clear();
    return JSValueMakeUndefined(context);
}

static JSValueRef setTouchModifierCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    if (argumentCount != 2)
        return JSValueMakeUndefined(context);

    JSRetainPtr<JSStringRef> jsModifier(Adopt, JSValueToStringCopy(context, arguments[0], exception));
    unsigned mask = 0;

    if (equals(jsModifier, "alt"))
        mask |= ECORE_EVENT_MODIFIER_ALT;
    else if (equals(jsModifier, "ctrl"))
        mask |= ECORE_EVENT_MODIFIER_CTRL;
    else if (equals(jsModifier, "meta"))
        mask |= ECORE_EVENT_MODIFIER_WIN;
    else if (equals(jsModifier, "shift"))
        mask |= ECORE_EVENT_MODIFIER_SHIFT;

    if (JSValueToBoolean(context, arguments[1]))
        touchModifiers |= mask;
    else
        touchModifiers &= ~mask;

    return JSValueMakeUndefined(context);
}

static JSStaticFunction staticFunctions[] = {
    { "mouseScrollBy", mouseScrollByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "continuousMouseScrollBy", continuousMouseScrollByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseDown", mouseDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseUp", mouseUpCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "mouseMoveTo", mouseMoveToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "leapForward", leapForwardCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "keyDown", keyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scheduleAsynchronousClick", scheduleAsynchronousClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scheduleAsynchronousKeyDown", scheduleAsynchronousKeyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "scalePageBy", scalePageByCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomIn", textZoomInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "textZoomOut", textZoomOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageIn", zoomPageInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "zoomPageOut", zoomPageOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "addTouchPoint", addTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchStart", touchStartCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "updateTouchPoint", updateTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchMove", touchMoveCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "releaseTouchPoint", releaseTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchEnd", touchEndCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "cancelTouchPoint", cancelTouchPointCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "touchCancel", touchCancelCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "clearTouchPoints", clearTouchPointsCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { "setTouchModifier", setTouchModifierCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete },
    { 0, 0, 0 }
};

static JSStaticValue staticValues[] = {
    { 0, 0, 0, 0 }
};

static JSClassRef getClass(JSContextRef context)
{
    static JSClassRef eventSenderClass = 0;

    if (!eventSenderClass) {
        JSClassDefinition classDefinition = {
                0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        classDefinition.staticFunctions = staticFunctions;
        classDefinition.staticValues = staticValues;

        eventSenderClass = JSClassCreate(&classDefinition);
    }

    return eventSenderClass;
}

JSObjectRef makeEventSender(JSContextRef context, bool isTopFrame)
{
    if (isTopFrame) {
        gDragMode = true;

        // Fly forward in time one second when the main frame loads. This will
        // ensure that when a test begins clicking in the same location as
        // a previous test, those clicks won't be interpreted as continuations
        // of the previous test's click sequences.
        gTimeOffset += 1000;

        gLastMousePositionX = gLastMousePositionY = 0;
        gLastClickPositionX = gLastClickPositionY = 0;
        gLastClickTimeOffset = 0;
        gLastClickButton = 0;
        gButtonCurrentlyDown = 0;
        gClickCount = 0;
    }

    return JSObjectMake(context, getClass(context), 0);
}

#else // ENABLE(TIZEN_WEBKIT_EFL_DRT)

#include "TestShell.h"
#include "WebContextMenuData.h"
#include "WebDragData.h"
#include "WebDragOperation.h"
#include "WebPoint.h"
#include "WebString.h"
#include "WebTouchPoint.h"
#include "WebView.h"
#include "webkit/support/webkit_support.h"
#include <wtf/Deque.h>
#include <wtf/StringExtras.h>

#if OS(WINDOWS)
#include "win/WebInputEventFactory.h"
#endif

// FIXME: layout before each event?

using namespace std;
using namespace WebKit;

WebPoint EventSender::lastMousePos;
WebMouseEvent::Button EventSender::pressedButton = WebMouseEvent::ButtonNone;
WebMouseEvent::Button EventSender::lastButtonType = WebMouseEvent::ButtonNone;

struct SavedEvent {
    enum SavedEventType {
        Unspecified,
        MouseUp,
        MouseMove,
        LeapForward
    };

    SavedEventType type;
    WebMouseEvent::Button buttonType; // For MouseUp.
    WebPoint pos; // For MouseMove.
    int milliseconds; // For LeapForward.

    SavedEvent()
        : type(Unspecified)
        , buttonType(WebMouseEvent::ButtonNone)
        , milliseconds(0) { }
};

static WebDragData currentDragData;
static WebDragOperation currentDragEffect;
static WebDragOperationsMask currentDragEffectsAllowed;
static bool replayingSavedEvents = false;
static Deque<SavedEvent> mouseEventQueue;
static int touchModifiers;
static Vector<WebTouchPoint> touchPoints;

// Time and place of the last mouse up event.
static double lastClickTimeSec = 0;
static WebPoint lastClickPos;
static int clickCount = 0;

// maximum distance (in space and time) for a mouse click
// to register as a double or triple click
static const double multipleClickTimeSec = 1;
static const int multipleClickRadiusPixels = 5;

// How much we should scroll per event - the value here is chosen to
// match the WebKit impl and layout test results.
static const float scrollbarPixelsPerTick = 40.0f;

inline bool outsideMultiClickRadius(const WebPoint& a, const WebPoint& b)
{
    return ((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y)) >
        multipleClickRadiusPixels * multipleClickRadiusPixels;
}

// Used to offset the time the event hander things an event happened. This is
// done so tests can run without a delay, but bypass checks that are time
// dependent (e.g., dragging has a timeout vs selection).
static uint32 timeOffsetMs = 0;

static double getCurrentEventTimeSec()
{
    return (webkit_support::GetCurrentTimeInMillisecond() + timeOffsetMs) / 1000.0;
}

static void advanceEventTime(int32_t deltaMs)
{
    timeOffsetMs += deltaMs;
}

static void initMouseEvent(WebInputEvent::Type t, WebMouseEvent::Button b,
                           const WebPoint& pos, WebMouseEvent* e)
{
    e->type = t;
    e->button = b;
    e->modifiers = 0;
    e->x = pos.x;
    e->y = pos.y;
    e->globalX = pos.x;
    e->globalY = pos.y;
    e->timeStampSeconds = getCurrentEventTimeSec();
    e->clickCount = clickCount;
}

// Returns true if the specified key is the system key.
static bool applyKeyModifier(const string& modifierName, WebInputEvent* event)
{
    bool isSystemKey = false;
    const char* characters = modifierName.c_str();
    if (!strcmp(characters, "ctrlKey")
#if !OS(MAC_OS_X)
        || !strcmp(characters, "addSelectionKey")
#endif
        ) {
        event->modifiers |= WebInputEvent::ControlKey;
    } else if (!strcmp(characters, "shiftKey") || !strcmp(characters, "rangeSelectionKey"))
        event->modifiers |= WebInputEvent::ShiftKey;
    else if (!strcmp(characters, "altKey")) {
        event->modifiers |= WebInputEvent::AltKey;
#if !OS(MAC_OS_X)
        // On Windows all keys with Alt modifier will be marked as system key.
        // We keep the same behavior on Linux and everywhere non-Mac, see:
        // WebKit/chromium/src/gtk/WebInputEventFactory.cpp
        // If we want to change this behavior on Linux, this piece of code must be
        // kept in sync with the related code in above file.
        isSystemKey = true;
#endif
#if OS(MAC_OS_X)
    } else if (!strcmp(characters, "metaKey") || !strcmp(characters, "addSelectionKey")) {
        event->modifiers |= WebInputEvent::MetaKey;
        // On Mac only command key presses are marked as system key.
        // See the related code in: WebKit/chromium/src/mac/WebInputEventFactory.cpp
        // It must be kept in sync with the related code in above file.
        isSystemKey = true;
#else
    } else if (!strcmp(characters, "metaKey")) {
        event->modifiers |= WebInputEvent::MetaKey;
#endif
    }
    return isSystemKey;
}

static bool applyKeyModifiers(const CppVariant* argument, WebInputEvent* event)
{
    bool isSystemKey = false;
    if (argument->isObject()) {
        Vector<string> modifiers = argument->toStringVector();
        for (Vector<string>::const_iterator i = modifiers.begin(); i != modifiers.end(); ++i)
            isSystemKey |= applyKeyModifier(*i, event);
    } else if (argument->isString())
        isSystemKey = applyKeyModifier(argument->toString(), event);
    return isSystemKey;
}

// Get the edit command corresponding to a keyboard event.
// Returns true if the specified event corresponds to an edit command, the name
// of the edit command will be stored in |*name|.
bool getEditCommand(const WebKeyboardEvent& event, string* name)
{
#if OS(MAC_OS_X)
    // We only cares about Left,Right,Up,Down keys with Command or Command+Shift
    // modifiers. These key events correspond to some special movement and
    // selection editor commands, and was supposed to be handled in
    // WebKit/chromium/src/EditorClientImpl.cpp. But these keys will be marked
    // as system key, which prevents them from being handled. Thus they must be
    // handled specially.
    if ((event.modifiers & ~WebKeyboardEvent::ShiftKey) != WebKeyboardEvent::MetaKey)
        return false;

    switch (event.windowsKeyCode) {
    case webkit_support::VKEY_LEFT:
        *name = "MoveToBeginningOfLine";
        break;
    case webkit_support::VKEY_RIGHT:
        *name = "MoveToEndOfLine";
        break;
    case webkit_support::VKEY_UP:
        *name = "MoveToBeginningOfDocument";
        break;
    case webkit_support::VKEY_DOWN:
        *name = "MoveToEndOfDocument";
        break;
    default:
        return false;
    }

    if (event.modifiers & WebKeyboardEvent::ShiftKey)
        name->append("AndModifySelection");

    return true;
#else
    return false;
#endif
}

// Key event location code introduced in DOM Level 3.
// See also: http://www.w3.org/TR/DOM-Level-3-Events/#events-keyboardevents
enum KeyLocationCode {
    DOMKeyLocationStandard      = 0x00,
    DOMKeyLocationLeft          = 0x01,
    DOMKeyLocationRight         = 0x02,
    DOMKeyLocationNumpad        = 0x03
};

EventSender::EventSender(TestShell* shell)
    : m_shell(shell)
{
    // Initialize the map that associates methods of this class with the names
    // they will use when called by JavaScript. The actual binding of those
    // names to their methods will be done by calling bindToJavaScript() (defined
    // by CppBoundClass, the parent to EventSender).
    bindMethod("addTouchPoint", &EventSender::addTouchPoint);
    bindMethod("beginDragWithFiles", &EventSender::beginDragWithFiles);
    bindMethod("cancelTouchPoint", &EventSender::cancelTouchPoint);
    bindMethod("clearKillRing", &EventSender::clearKillRing);
    bindMethod("clearTouchPoints", &EventSender::clearTouchPoints);
    bindMethod("contextClick", &EventSender::contextClick);
    bindMethod("continuousMouseScrollBy", &EventSender::continuousMouseScrollBy);
    bindMethod("dispatchMessage", &EventSender::dispatchMessage);
    bindMethod("dumpFilenameBeingDragged", &EventSender::dumpFilenameBeingDragged);
    bindMethod("enableDOMUIEventLogging", &EventSender::enableDOMUIEventLogging);
    bindMethod("fireKeyboardEventsToElement", &EventSender::fireKeyboardEventsToElement);
    bindMethod("keyDown", &EventSender::keyDown);
    bindMethod("leapForward", &EventSender::leapForward);
    bindMethod("mouseDown", &EventSender::mouseDown);
    bindMethod("mouseMoveTo", &EventSender::mouseMoveTo);
    bindMethod("mouseScrollBy", &EventSender::mouseScrollBy);
    bindMethod("mouseUp", &EventSender::mouseUp);
    bindMethod("releaseTouchPoint", &EventSender::releaseTouchPoint);
    bindMethod("scheduleAsynchronousClick", &EventSender::scheduleAsynchronousClick);
    bindMethod("setTouchModifier", &EventSender::setTouchModifier);
    bindMethod("textZoomIn", &EventSender::textZoomIn);
    bindMethod("textZoomOut", &EventSender::textZoomOut);
    bindMethod("touchCancel", &EventSender::touchCancel);
    bindMethod("touchEnd", &EventSender::touchEnd);
    bindMethod("touchMove", &EventSender::touchMove);
    bindMethod("touchStart", &EventSender::touchStart);
    bindMethod("updateTouchPoint", &EventSender::updateTouchPoint);
    bindMethod("gestureScrollBegin", &EventSender::gestureScrollBegin);
    bindMethod("gestureScrollEnd", &EventSender::gestureScrollEnd);
    bindMethod("gestureTap", &EventSender::gestureTap);
    bindMethod("zoomPageIn", &EventSender::zoomPageIn);
    bindMethod("zoomPageOut", &EventSender::zoomPageOut);
    bindMethod("scalePageBy", &EventSender::scalePageBy);

    // When set to true (the default value), we batch mouse move and mouse up
    // events so we can simulate drag & drop.
    bindProperty("dragMode", &dragMode);
#if OS(WINDOWS)
    bindProperty("WM_KEYDOWN", &wmKeyDown);
    bindProperty("WM_KEYUP", &wmKeyUp);
    bindProperty("WM_CHAR", &wmChar);
    bindProperty("WM_DEADCHAR", &wmDeadChar);
    bindProperty("WM_SYSKEYDOWN", &wmSysKeyDown);
    bindProperty("WM_SYSKEYUP", &wmSysKeyUp);
    bindProperty("WM_SYSCHAR", &wmSysChar);
    bindProperty("WM_SYSDEADCHAR", &wmSysDeadChar);
#endif
}

void EventSender::reset()
{
    // The test should have finished a drag and the mouse button state.
    ASSERT(currentDragData.isNull());
    currentDragData.reset();
    currentDragEffect = WebKit::WebDragOperationNone;
    currentDragEffectsAllowed = WebKit::WebDragOperationNone;
    pressedButton = WebMouseEvent::ButtonNone;
    dragMode.set(true);
#if OS(WINDOWS)
    wmKeyDown.set(WM_KEYDOWN);
    wmKeyUp.set(WM_KEYUP);
    wmChar.set(WM_CHAR);
    wmDeadChar.set(WM_DEADCHAR);
    wmSysKeyDown.set(WM_SYSKEYDOWN);
    wmSysKeyUp.set(WM_SYSKEYUP);
    wmSysChar.set(WM_SYSCHAR);
    wmSysDeadChar.set(WM_SYSDEADCHAR);
#endif
    lastMousePos = WebPoint(0, 0);
    lastClickTimeSec = 0;
    lastClickPos = WebPoint(0, 0);
    clickCount = 0;
    lastButtonType = WebMouseEvent::ButtonNone;
    timeOffsetMs = 0;
    touchModifiers = 0;
    touchPoints.clear();
    m_taskList.revokeAll();
}

WebView* EventSender::webview()
{
    return m_shell->webView();
}

void EventSender::doDragDrop(const WebDragData& dragData, WebDragOperationsMask mask)
{
    WebMouseEvent event;
    initMouseEvent(WebInputEvent::MouseDown, pressedButton, lastMousePos, &event);
    WebPoint clientPoint(event.x, event.y);
    WebPoint screenPoint(event.globalX, event.globalY);
    currentDragData = dragData;
    currentDragEffectsAllowed = mask;
    currentDragEffect = webview()->dragTargetDragEnter(dragData, clientPoint, screenPoint, currentDragEffectsAllowed);

    // Finish processing events.
    replaySavedEvents();
}

void EventSender::dumpFilenameBeingDragged(const CppArgumentList&, CppVariant*)
{
    printf("Filename being dragged: %s\n", currentDragData.fileContentFilename().utf8().data());
}

WebMouseEvent::Button EventSender::getButtonTypeFromButtonNumber(int buttonCode)
{
    if (!buttonCode)
        return WebMouseEvent::ButtonLeft;
    if (buttonCode == 2)
        return WebMouseEvent::ButtonRight;
    return WebMouseEvent::ButtonMiddle;
}

int EventSender::getButtonNumberFromSingleArg(const CppArgumentList& arguments)
{
    int buttonCode = 0;
    if (arguments.size() > 0 && arguments[0].isNumber())
        buttonCode = arguments[0].toInt32();
    return buttonCode;
}

void EventSender::updateClickCountForButton(WebMouseEvent::Button buttonType)
{
    if ((getCurrentEventTimeSec() - lastClickTimeSec < multipleClickTimeSec)
        && (!outsideMultiClickRadius(lastMousePos, lastClickPos))
        && (buttonType == lastButtonType))
        ++clickCount;
    else {
        clickCount = 1;
        lastButtonType = buttonType;
    }
}

//
// Implemented javascript methods.
//

void EventSender::mouseDown(const CppArgumentList& arguments, CppVariant* result)
{
    if (result) // Could be 0 if invoked asynchronously.
        result->setNull();

    webview()->layout();

    int buttonNumber = getButtonNumberFromSingleArg(arguments);
    ASSERT(buttonNumber != -1);

    WebMouseEvent::Button buttonType = getButtonTypeFromButtonNumber(buttonNumber);

    updateClickCountForButton(buttonType);

    WebMouseEvent event;
    pressedButton = buttonType;
    initMouseEvent(WebInputEvent::MouseDown, buttonType, lastMousePos, &event);
    if (arguments.size() >= 2 && (arguments[1].isObject() || arguments[1].isString()))
        applyKeyModifiers(&(arguments[1]), &event);
    webview()->handleInputEvent(event);
}

void EventSender::mouseUp(const CppArgumentList& arguments, CppVariant* result)
{
    if (result) // Could be 0 if invoked asynchronously.
        result->setNull();

    webview()->layout();

    int buttonNumber = getButtonNumberFromSingleArg(arguments);
    ASSERT(buttonNumber != -1);

    WebMouseEvent::Button buttonType = getButtonTypeFromButtonNumber(buttonNumber);

    if (isDragMode() && !replayingSavedEvents) {
        SavedEvent savedEvent;
        savedEvent.type = SavedEvent::MouseUp;
        savedEvent.buttonType = buttonType;
        mouseEventQueue.append(savedEvent);
        replaySavedEvents();
    } else {
        WebMouseEvent event;
        initMouseEvent(WebInputEvent::MouseUp, buttonType, lastMousePos, &event);
        if (arguments.size() >= 2 && (arguments[1].isObject() || arguments[1].isString()))
            applyKeyModifiers(&(arguments[1]), &event);
        doMouseUp(event);
    }
}

void EventSender::doMouseUp(const WebMouseEvent& e)
{
    webview()->handleInputEvent(e);

    pressedButton = WebMouseEvent::ButtonNone;
    lastClickTimeSec = e.timeStampSeconds;
    lastClickPos = lastMousePos;

    // If we're in a drag operation, complete it.
    if (currentDragData.isNull())
        return;
    WebPoint clientPoint(e.x, e.y);
    WebPoint screenPoint(e.globalX, e.globalY);

    currentDragEffect = webview()->dragTargetDragOver(clientPoint, screenPoint, currentDragEffectsAllowed);
    if (currentDragEffect)
        webview()->dragTargetDrop(clientPoint, screenPoint);
    else
        webview()->dragTargetDragLeave();
    webview()->dragSourceEndedAt(clientPoint, screenPoint, currentDragEffect);
    webview()->dragSourceSystemDragEnded();

    currentDragData.reset();
}

void EventSender::mouseMoveTo(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    if (arguments.size() < 2 || !arguments[0].isNumber() || !arguments[1].isNumber())
        return;
    webview()->layout();

    WebPoint mousePos(arguments[0].toInt32(), arguments[1].toInt32());

    if (isDragMode() && pressedButton == WebMouseEvent::ButtonLeft && !replayingSavedEvents) {
        SavedEvent savedEvent;
        savedEvent.type = SavedEvent::MouseMove;
        savedEvent.pos = mousePos;
        mouseEventQueue.append(savedEvent);
    } else {
        WebMouseEvent event;
        initMouseEvent(WebInputEvent::MouseMove, pressedButton, mousePos, &event);
        doMouseMove(event);
    }
}

void EventSender::doMouseMove(const WebMouseEvent& e)
{
    lastMousePos = WebPoint(e.x, e.y);

    webview()->handleInputEvent(e);

    if (pressedButton == WebMouseEvent::ButtonNone || currentDragData.isNull())
        return;
    WebPoint clientPoint(e.x, e.y);
    WebPoint screenPoint(e.globalX, e.globalY);
    currentDragEffect = webview()->dragTargetDragOver(clientPoint, screenPoint, currentDragEffectsAllowed);
}

void EventSender::keyDown(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    if (arguments.size() < 1 || !arguments[0].isString())
        return;
    bool generateChar = false;

    // FIXME: I'm not exactly sure how we should convert the string to a key
    // event. This seems to work in the cases I tested.
    // FIXME: Should we also generate a KEY_UP?
    string codeStr = arguments[0].toString();

    // Convert \n -> VK_RETURN. Some layout tests use \n to mean "Enter", when
    // Windows uses \r for "Enter".
    int code = 0;
    int text = 0;
    bool needsShiftKeyModifier = false;
    if ("\n" == codeStr) {
        generateChar = true;
        text = code = webkit_support::VKEY_RETURN;
    } else if ("rightArrow" == codeStr)
        code = webkit_support::VKEY_RIGHT;
    else if ("downArrow" == codeStr)
        code = webkit_support::VKEY_DOWN;
    else if ("leftArrow" == codeStr)
        code = webkit_support::VKEY_LEFT;
    else if ("upArrow" == codeStr)
        code = webkit_support::VKEY_UP;
    else if ("insert" == codeStr)
        code = webkit_support::VKEY_INSERT;
    else if ("delete" == codeStr)
        code = webkit_support::VKEY_DELETE;
    else if ("pageUp" == codeStr)
        code = webkit_support::VKEY_PRIOR;
    else if ("pageDown" == codeStr)
        code = webkit_support::VKEY_NEXT;
    else if ("home" == codeStr)
        code = webkit_support::VKEY_HOME;
    else if ("end" == codeStr)
        code = webkit_support::VKEY_END;
    else if ("printScreen" == codeStr)
        code = webkit_support::VKEY_SNAPSHOT;
    else if ("menu" == codeStr)
        // FIXME: Change this to webkit_support::VKEY_APPS.
        code = 0x5D;
    else {
        // Compare the input string with the function-key names defined by the
        // DOM spec (i.e. "F1",...,"F24"). If the input string is a function-key
        // name, set its key code.
        for (int i = 1; i <= 24; ++i) {
            char functionChars[10];
            snprintf(functionChars, 10, "F%d", i);
            string functionKeyName(functionChars);
            if (functionKeyName == codeStr) {
                code = webkit_support::VKEY_F1 + (i - 1);
                break;
            }
        }
        if (!code) {
            WebString webCodeStr = WebString::fromUTF8(codeStr.data(), codeStr.size());
            ASSERT(webCodeStr.length() == 1);
            text = code = webCodeStr.data()[0];
            needsShiftKeyModifier = needsShiftModifier(code);
            if ((code & 0xFF) >= 'a' && (code & 0xFF) <= 'z')
                code -= 'a' - 'A';
            generateChar = true;
        }
    }

    // For one generated keyboard event, we need to generate a keyDown/keyUp
    // pair; refer to EventSender.cpp in Tools/DumpRenderTree/win.
    // On Windows, we might also need to generate a char event to mimic the
    // Windows event flow; on other platforms we create a merged event and test
    // the event flow that that platform provides.
    WebKeyboardEvent eventDown, eventChar, eventUp;
    eventDown.type = WebInputEvent::RawKeyDown;
    eventDown.modifiers = 0;
    eventDown.windowsKeyCode = code;
    if (generateChar) {
        eventDown.text[0] = text;
        eventDown.unmodifiedText[0] = text;
    }
    eventDown.setKeyIdentifierFromWindowsKeyCode();

    if (arguments.size() >= 2 && (arguments[1].isObject() || arguments[1].isString()))
        eventDown.isSystemKey = applyKeyModifiers(&(arguments[1]), &eventDown);

    if (needsShiftKeyModifier)
        eventDown.modifiers |= WebInputEvent::ShiftKey;

    // See if KeyLocation argument is given.
    if (arguments.size() >= 3 && arguments[2].isNumber()) {
        int location = arguments[2].toInt32();
        if (location == DOMKeyLocationNumpad)
            eventDown.modifiers |= WebInputEvent::IsKeyPad;
    }

    eventChar = eventUp = eventDown;
    eventUp.type = WebInputEvent::KeyUp;
    // EventSender.m forces a layout here, with at least one
    // test (fast/forms/focus-control-to-page.html) relying on this.
    webview()->layout();

    // In the browser, if a keyboard event corresponds to an editor command,
    // the command will be dispatched to the renderer just before dispatching
    // the keyboard event, and then it will be executed in the
    // RenderView::handleCurrentKeyboardEvent() method, which is called from
    // third_party/WebKit/Source/WebKit/chromium/src/EditorClientImpl.cpp.
    // We just simulate the same behavior here.
    string editCommand;
    if (getEditCommand(eventDown, &editCommand))
        m_shell->webViewHost()->setEditCommand(editCommand, "");

    webview()->handleInputEvent(eventDown);

    m_shell->webViewHost()->clearEditCommand();

    if (generateChar) {
        eventChar.type = WebInputEvent::Char;
        eventChar.keyIdentifier[0] = '\0';
        webview()->handleInputEvent(eventChar);
    }

    webview()->handleInputEvent(eventUp);
}

void EventSender::dispatchMessage(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

#if OS(WINDOWS)
    if (arguments.size() == 3) {
        // Grab the message id to see if we need to dispatch it.
        int msg = arguments[0].toInt32();

        // WebKit's version of this function stuffs a MSG struct and uses
        // TranslateMessage and DispatchMessage. We use a WebKeyboardEvent, which
        // doesn't need to receive the DeadChar and SysDeadChar messages.
        if (msg == WM_DEADCHAR || msg == WM_SYSDEADCHAR)
            return;

        webview()->layout();

        unsigned long lparam = static_cast<unsigned long>(arguments[2].toDouble());
        webview()->handleInputEvent(WebInputEventFactory::keyboardEvent(0, msg, arguments[1].toInt32(), lparam));
    } else
        ASSERT_NOT_REACHED();
#endif
}

bool EventSender::needsShiftModifier(int keyCode)
{
    // If code is an uppercase letter, assign a SHIFT key to
    // eventDown.modifier, this logic comes from
    // Tools/DumpRenderTree/win/EventSender.cpp
    return (keyCode & 0xFF) >= 'A' && (keyCode & 0xFF) <= 'Z';
}

void EventSender::leapForward(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    if (arguments.size() < 1 || !arguments[0].isNumber())
        return;

    int milliseconds = arguments[0].toInt32();
    if (isDragMode() && pressedButton == WebMouseEvent::ButtonLeft && !replayingSavedEvents) {
        SavedEvent savedEvent;
        savedEvent.type = SavedEvent::LeapForward;
        savedEvent.milliseconds = milliseconds;
        mouseEventQueue.append(savedEvent);
    } else
        doLeapForward(milliseconds);
}

void EventSender::doLeapForward(int milliseconds)
{
    advanceEventTime(milliseconds);
}

// Apple's port of WebKit zooms by a factor of 1.2 (see
// WebKit/WebView/WebView.mm)
void EventSender::textZoomIn(const CppArgumentList&, CppVariant* result)
{
    webview()->setZoomLevel(true, webview()->zoomLevel() + 1);
    result->setNull();
}

void EventSender::textZoomOut(const CppArgumentList&, CppVariant* result)
{
    webview()->setZoomLevel(true, webview()->zoomLevel() - 1);
    result->setNull();
}

void EventSender::zoomPageIn(const CppArgumentList&, CppVariant* result)
{
    webview()->setZoomLevel(false, webview()->zoomLevel() + 1);
    result->setNull();
}

void EventSender::zoomPageOut(const CppArgumentList&, CppVariant* result)
{
    webview()->setZoomLevel(false, webview()->zoomLevel() - 1);
    result->setNull();
}

void EventSender::scalePageBy(const CppArgumentList& arguments, CppVariant* result)
{
    if (arguments.size() < 3 || !arguments[0].isNumber() || !arguments[1].isNumber() || !arguments[2].isNumber())
        return;

    float scaleFactor = static_cast<float>(arguments[0].toDouble());
    int x = arguments[1].toInt32();
    int y = arguments[2].toInt32();
    webview()->scalePage(scaleFactor, WebPoint(x, y));
    result->setNull();
}

void EventSender::mouseScrollBy(const CppArgumentList& arguments, CppVariant* result)
{
    handleMouseWheel(arguments, result, false);
}

void EventSender::continuousMouseScrollBy(const CppArgumentList& arguments, CppVariant* result)
{
    handleMouseWheel(arguments, result, true);
}

void EventSender::replaySavedEvents()
{
    replayingSavedEvents = true;
    while (!mouseEventQueue.isEmpty()) {
        SavedEvent e = mouseEventQueue.takeFirst();

        switch (e.type) {
        case SavedEvent::MouseMove: {
            WebMouseEvent event;
            initMouseEvent(WebInputEvent::MouseMove, pressedButton, e.pos, &event);
            doMouseMove(event);
            break;
        }
        case SavedEvent::LeapForward:
            doLeapForward(e.milliseconds);
            break;
        case SavedEvent::MouseUp: {
            WebMouseEvent event;
            initMouseEvent(WebInputEvent::MouseUp, e.buttonType, lastMousePos, &event);
            doMouseUp(event);
            break;
        }
        default:
            ASSERT_NOT_REACHED();
        }
    }

    replayingSavedEvents = false;
}

// Because actual context menu is implemented by the browser side, 
// this function does only what LayoutTests are expecting:
// - Many test checks the count of items. So returning non-zero value makes sense.
// - Some test compares the count before and after some action. So changing the count based on flags
//   also makes sense. This function is doing such for some flags.
// - Some test even checks actual string content. So providing it would be also helpful.
//
static Vector<WebString> makeMenuItemStringsFor(WebContextMenuData* contextMenu, MockSpellCheck* spellcheck)
{
    // These constants are based on Safari's context menu because tests are made for it.
    static const char* nonEditableMenuStrings[] = { "Back", "Reload Page", "Open in Dashbaord", "<separator>", "View Source", "Save Page As", "Print Page", "Inspect Element", 0 };
    static const char* editableMenuStrings[] = { "Cut", "Copy", "<separator>", "Paste", "Spelling and Grammar", "Substitutions, Transformations", "Font", "Speech", "Paragraph Direction", "<separator>", 0 };

    // This is possible because mouse events are cancelleable.
    if (!contextMenu)
        return Vector<WebString>();

    Vector<WebString> strings;

    if (contextMenu->isEditable) {
        for (const char** item = editableMenuStrings; *item; ++item) 
            strings.append(WebString::fromUTF8(*item));
        Vector<WebString> suggestions;
        spellcheck->fillSuggestionList(contextMenu->misspelledWord, &suggestions);
        for (size_t i = 0; i < suggestions.size(); ++i) 
            strings.append(suggestions[i]);
    } else {
        for (const char** item = nonEditableMenuStrings; *item; ++item) 
            strings.append(WebString::fromUTF8(*item));
    }

    return strings;
}


void EventSender::contextClick(const CppArgumentList& arguments, CppVariant* result)
{
    webview()->layout();

    updateClickCountForButton(WebMouseEvent::ButtonRight);

    // Clears last context menu data because we need to know if the context menu be requested 
    // after following mouse events.
    m_shell->webViewHost()->clearContextMenuData();

    // Generate right mouse down and up.
    WebMouseEvent event;
    pressedButton = WebMouseEvent::ButtonRight;
    initMouseEvent(WebInputEvent::MouseDown, WebMouseEvent::ButtonRight, lastMousePos, &event);
    webview()->handleInputEvent(event);

    initMouseEvent(WebInputEvent::MouseUp, WebMouseEvent::ButtonRight, lastMousePos, &event);
    webview()->handleInputEvent(event);

    pressedButton = WebMouseEvent::ButtonNone;

    WebContextMenuData* lastContextMenu = m_shell->webViewHost()->lastContextMenuData();
    result->set(WebBindings::makeStringArray(makeMenuItemStringsFor(lastContextMenu, m_shell->webViewHost()->mockSpellCheck())));
}

class MouseDownTask: public MethodTask<EventSender> {
public:
    MouseDownTask(EventSender* obj, const CppArgumentList& arg)
        : MethodTask<EventSender>(obj), m_arguments(arg) { }
    virtual void runIfValid() { m_object->mouseDown(m_arguments, 0); }

private:
    CppArgumentList m_arguments;
};

class MouseUpTask: public MethodTask<EventSender> {
public:
    MouseUpTask(EventSender* obj, const CppArgumentList& arg)
        : MethodTask<EventSender>(obj), m_arguments(arg) { }
    virtual void runIfValid() { m_object->mouseUp(m_arguments, 0); }

private:
    CppArgumentList m_arguments;
};

void EventSender::scheduleAsynchronousClick(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    postTask(new MouseDownTask(this, arguments));
    postTask(new MouseUpTask(this, arguments));
}

void EventSender::beginDragWithFiles(const CppArgumentList& arguments, CppVariant* result)
{
    currentDragData.initialize();
    Vector<string> files = arguments[0].toStringVector();
    for (size_t i = 0; i < files.size(); ++i)
        currentDragData.appendToFilenames(webkit_support::GetAbsoluteWebStringFromUTF8Path(files[i]));
    currentDragEffectsAllowed = WebKit::WebDragOperationCopy;

    // Provide a drag source.
    webview()->dragTargetDragEnter(currentDragData, lastMousePos, lastMousePos, currentDragEffectsAllowed);

    // dragMode saves events and then replays them later. We don't need/want that.
    dragMode.set(false);

    // Make the rest of eventSender think a drag is in progress.
    pressedButton = WebMouseEvent::ButtonLeft;

    result->setNull();
}

void EventSender::addTouchPoint(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    WebTouchPoint touchPoint;
    touchPoint.state = WebTouchPoint::StatePressed;
    touchPoint.position = WebPoint(arguments[0].toInt32(), arguments[1].toInt32());
    touchPoint.screenPosition = touchPoint.position;

    int lowestId = 0;
    for (size_t i = 0; i < touchPoints.size(); i++) {
        if (touchPoints[i].id == lowestId)
            lowestId++;
    }
    touchPoint.id = lowestId;
    touchPoints.append(touchPoint);
}

void EventSender::clearTouchPoints(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    touchPoints.clear();
}

void EventSender::releaseTouchPoint(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    const unsigned index = arguments[0].toInt32();
    ASSERT(index < touchPoints.size());

    WebTouchPoint* touchPoint = &touchPoints[index];
    touchPoint->state = WebTouchPoint::StateReleased;
}

void EventSender::setTouchModifier(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    int mask = 0;
    const string keyName = arguments[0].toString();
    if (keyName == "shift")
        mask = WebInputEvent::ShiftKey;
    else if (keyName == "alt")
        mask = WebInputEvent::AltKey;
    else if (keyName == "ctrl")
        mask = WebInputEvent::ControlKey;
    else if (keyName == "meta")
        mask = WebInputEvent::MetaKey;

    if (arguments[1].toBoolean())
        touchModifiers |= mask;
    else
        touchModifiers &= ~mask;
}

void EventSender::updateTouchPoint(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    const unsigned index = arguments[0].toInt32();
    ASSERT(index < touchPoints.size());

    WebPoint position(arguments[1].toInt32(), arguments[2].toInt32());
    WebTouchPoint* touchPoint = &touchPoints[index];
    touchPoint->state = WebTouchPoint::StateMoved;
    touchPoint->position = position;
    touchPoint->screenPosition = position;
}

void EventSender::cancelTouchPoint(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();

    const unsigned index = arguments[0].toInt32();
    ASSERT(index < touchPoints.size());

    WebTouchPoint* touchPoint = &touchPoints[index];
    touchPoint->state = WebTouchPoint::StateCancelled;
}

void EventSender::sendCurrentTouchEvent(const WebInputEvent::Type type)
{
    ASSERT(static_cast<unsigned>(WebTouchEvent::touchPointsLengthCap) > touchPoints.size());
    webview()->layout();

    WebTouchEvent touchEvent;
    touchEvent.type = type;
    touchEvent.modifiers = touchModifiers;
    touchEvent.timeStampSeconds = getCurrentEventTimeSec();
    touchEvent.touchPointsLength = touchPoints.size();
    for (unsigned i = 0; i < touchPoints.size(); ++i)
        touchEvent.touchPoints[i] = touchPoints[i];
    webview()->handleInputEvent(touchEvent);

    for (unsigned i = 0; i < touchPoints.size(); ++i) {
        WebTouchPoint* touchPoint = &touchPoints[i];
        if (touchPoint->state == WebTouchPoint::StateReleased) {
            touchPoints.remove(i);
            --i;
        } else
            touchPoint->state = WebTouchPoint::StateStationary;
    }
}

void EventSender::handleMouseWheel(const CppArgumentList& arguments, CppVariant* result, bool continuous)
{
    result->setNull();

    if (arguments.size() < 2 || !arguments[0].isNumber() || !arguments[1].isNumber())
        return;

    // Force a layout here just to make sure every position has been
    // determined before we send events (as well as all the other methods
    // that send an event do).
    webview()->layout();

    int horizontal = arguments[0].toInt32();
    int vertical = arguments[1].toInt32();

    WebMouseWheelEvent event;
    initMouseEvent(WebInputEvent::MouseWheel, pressedButton, lastMousePos, &event);
    event.wheelTicksX = static_cast<float>(horizontal);
    event.wheelTicksY = static_cast<float>(vertical);
    event.deltaX = event.wheelTicksX;
    event.deltaY = event.wheelTicksY;
    if (continuous) {
        event.wheelTicksX /= scrollbarPixelsPerTick;
        event.wheelTicksY /= scrollbarPixelsPerTick;
    } else {
        event.deltaX *= scrollbarPixelsPerTick;
        event.deltaY *= scrollbarPixelsPerTick;
    }
    webview()->handleInputEvent(event);
}

void EventSender::touchEnd(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    sendCurrentTouchEvent(WebInputEvent::TouchEnd);
}

void EventSender::touchMove(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    sendCurrentTouchEvent(WebInputEvent::TouchMove);
}

void EventSender::touchStart(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    sendCurrentTouchEvent(WebInputEvent::TouchStart);
}

void EventSender::touchCancel(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
    sendCurrentTouchEvent(WebInputEvent::TouchCancel);
}

void EventSender::gestureScrollBegin(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureScrollBegin, arguments);
}

void EventSender::gestureScrollEnd(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureScrollEnd, arguments);
}

void EventSender::gestureTap(const CppArgumentList& arguments, CppVariant* result)
{
    result->setNull();
    gestureEvent(WebInputEvent::GestureTap, arguments);
}

void EventSender::gestureEvent(WebInputEvent::Type type, const CppArgumentList& arguments)
{
    if (arguments.size() < 2 || !arguments[0].isNumber() || !arguments[1].isNumber())
        return;

    WebPoint point(arguments[0].toInt32(), arguments[1].toInt32());

    WebGestureEvent event;
    event.type = type;
    event.x = point.x;
    event.y = point.y;
    event.globalX = point.x;
    event.globalY = point.y;
    event.timeStampSeconds = getCurrentEventTimeSec();
    webview()->handleInputEvent(event);
}

//
// Unimplemented stubs
//

void EventSender::enableDOMUIEventLogging(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
}

void EventSender::fireKeyboardEventsToElement(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
}

void EventSender::clearKillRing(const CppArgumentList&, CppVariant* result)
{
    result->setNull();
}
#endif

static void feedOrQueueMouseEvent(MouseEventInfo* eventInfo, EventQueueStrategy strategy)
{
    if (!delayedEventQueue().isEmpty()) {
        if (delayedEventQueue().last().eventInfo)
            delayedEventQueue().append(DelayedEvent(eventInfo));
        else
            delayedEventQueue().last().eventInfo = eventInfo;

        if (strategy == FeedQueuedEvents)
            feedQueuedMouseEvents();
    } else
        feedMouseEvent(eventInfo);
}

static void feedMouseEvent(MouseEventInfo* eventInfo)
{
    if (!eventInfo)
        return;

    unsigned timeStamp = 0;
    Evas_Object* mainFrame = browser->mainFrame();
    Evas* evas = evas_object_evas_get(mainFrame);
    DumpRenderTreeSupportEfl::layoutFrame(mainFrame);
    EvasMouseEvent event = eventInfo->event;

    setEvasModifiers(evas, eventInfo->modifiers);

    Evas_Button_Flags flags = EVAS_BUTTON_NONE;

    // FIXME: We need to pass additional information with our events, so that
    // we could construct correct PlatformWheelEvent. At the moment, max number
    // of clicks is 3
    if (gClickCount == 3)
        flags = EVAS_BUTTON_TRIPLE_CLICK;
    else if (gClickCount == 2)
        flags = EVAS_BUTTON_DOUBLE_CLICK;

    if (event & EvasMouseEventMove)
        evas_event_feed_mouse_move(evas, gLastMousePositionX, gLastMousePositionY, timeStamp++, 0);
    if (event & EvasMouseEventDown)
        evas_event_feed_mouse_down(evas, eventInfo->button, flags, timeStamp++, 0);
    if (event & EvasMouseEventUp)
        evas_event_feed_mouse_up(evas, eventInfo->button, flags, timeStamp++, 0);
    if (event & EvasMouseEventScrollLeft | event & EvasMouseEventScrollRight)
        evas_event_feed_mouse_wheel(evas, 1, eventInfo->horizontalDelta, timeStamp, 0);
    if (event & EvasMouseEventScrollUp | event & EvasMouseEventScrollDown)
        evas_event_feed_mouse_wheel(evas, 0, eventInfo->verticalDelta, timeStamp, 0);

    setEvasModifiers(evas, EvasKeyModifierNone);

    delete eventInfo;
}

static void feedQueuedMouseEvents()
{
    WTF::Vector<DelayedEvent>::const_iterator it = delayedEventQueue().begin();
    for (; it != delayedEventQueue().end(); it++) {
        DelayedEvent delayedEvent = *it;
        if (delayedEvent.delay)
            usleep(delayedEvent.delay * 1000);
        feedMouseEvent(delayedEvent.eventInfo);
    }
    delayedEventQueue().clear();
}
