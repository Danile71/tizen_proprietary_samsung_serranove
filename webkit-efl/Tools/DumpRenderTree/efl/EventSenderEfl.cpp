#include "DumpRenderTreeView.h"
#include "EventSenderEfl.h"
#include "JSRetainPtr.h"
#include <iostream>
#include <Evas.h>
#include <EWebKit.h>

#define MSG_COUNT 1024

bool EventSenderEfl::dragMode = true;
bool EventSenderEfl::down = false;
bool EventSenderEfl::replayingSavedEvents = false;
int EventSenderEfl::clickCount = 0;
int EventSenderEfl::lastClickPositionX = 0;
int EventSenderEfl::lastClickPositionY = 0;
int EventSenderEfl::lastMousePositionX = 0;
int EventSenderEfl::lastMousePositionY = 0;
unsigned EventSenderEfl::endOfQueue = 0;
unsigned EventSenderEfl::startOfQueue = 0;

DelayedMessage msgQueue[MSG_COUNT] = {0};

Evas_Event_Mouse_Move* EventSenderEfl::createMouseMoveEvent(Evas* evas)
{
    Evas_Modifier* modifiers = (Evas_Modifier*)evas_key_modifier_get(evas);
    Evas_Lock* locks = (Evas_Lock*)evas_key_lock_get(evas);

    Evas_Event_Mouse_Move* mouseMove = new Evas_Event_Mouse_Move;
    mouseMove->buttons = 1;
    mouseMove->cur.output.x = lastMousePositionX;
    mouseMove->cur.output.y = lastMousePositionY;
    mouseMove->cur.canvas.x = lastMousePositionX;
    mouseMove->cur.canvas.y = lastMousePositionY;
    mouseMove->data = 0;
    mouseMove->modifiers = modifiers;
    mouseMove->locks = locks;
    mouseMove->timestamp = ecore_loop_time_get();
    mouseMove->event_flags = EVAS_EVENT_FLAG_NONE;
    mouseMove->dev = 0;

    return mouseMove;
}

Evas_Event_Mouse_Down* EventSenderEfl::createMouseDownEvent(Evas* evas)
{
    Evas_Modifier* modifiers = (Evas_Modifier*)evas_key_modifier_get(evas);
    Evas_Lock* locks = (Evas_Lock*)evas_key_lock_get(evas);

    Evas_Event_Mouse_Down* mouseDown = new Evas_Event_Mouse_Down;
    mouseDown->button = 1;
    mouseDown->output.x = lastMousePositionX;
    mouseDown->output.y = lastMousePositionY;
    mouseDown->canvas.x = lastMousePositionX;
    mouseDown->canvas.y = lastMousePositionY;
    mouseDown->data = 0;
    mouseDown->modifiers = modifiers;
    mouseDown->locks = locks;
    mouseDown->flags = EVAS_BUTTON_NONE;
    mouseDown->timestamp = ecore_loop_time_get();
    mouseDown->event_flags = EVAS_EVENT_FLAG_NONE;
    mouseDown->dev = 0;

    return mouseDown;
}

Evas_Event_Mouse_Up* EventSenderEfl::createMouseUpEvent(Evas* evas)
{
    Evas_Modifier* modifiers = (Evas_Modifier*)evas_key_modifier_get(evas);
    Evas_Lock* locks = (Evas_Lock*)evas_key_lock_get(evas);

    Evas_Event_Mouse_Up* mouseUp = new Evas_Event_Mouse_Up;
    mouseUp->button = 1;
    mouseUp->output.x = lastMousePositionX;
    mouseUp->output.y = lastMousePositionY;
    mouseUp->canvas.x = lastMousePositionX;
    mouseUp->canvas.y = lastMousePositionY;
    mouseUp->data = 0;
    mouseUp->modifiers = modifiers;
    mouseUp->locks = locks;
    mouseUp->flags = EVAS_BUTTON_NONE;
    mouseUp->timestamp = ecore_loop_time_get();
    mouseUp->event_flags = EVAS_EVENT_FLAG_NONE;
    mouseUp->dev = 0;

    return mouseUp;
}

Evas_Event_Mouse_Wheel* EventSenderEfl::createMouseWheelEvent(Evas* evas)
{
    Evas_Modifier* modifiers = (Evas_Modifier*)evas_key_modifier_get(evas);
    Evas_Lock* locks = (Evas_Lock*)evas_key_lock_get(evas);

    Evas_Event_Mouse_Wheel* mouseWheel = new Evas_Event_Mouse_Wheel;
    mouseWheel->output.x = lastMousePositionX;
    mouseWheel->output.y = lastMousePositionY;
    mouseWheel->canvas.x = lastMousePositionX;
    mouseWheel->canvas.y = lastMousePositionY;
    mouseWheel->data = 0;
    mouseWheel->modifiers = modifiers;
    mouseWheel->locks = locks;
    mouseWheel->timestamp = ecore_loop_time_get();
    mouseWheel->event_flags = EVAS_EVENT_FLAG_NONE;
    mouseWheel->dev = 0;

    return mouseWheel;
}

void EventSenderEfl::makeEventSender(JSContextRef context, JSObjectRef windowObject, JSValueRef* exception)
{
    dragMode = true;
    down = false;
    lastMousePositionX = lastMousePositionY = 0;
    lastClickPositionX = lastClickPositionY = 0;
    if (!replayingSavedEvents) {
        endOfQueue = 0;
        startOfQueue = 0;
    }
    
    JSRetainPtr<JSStringRef> eventSenderStr(Adopt, JSStringCreateWithUTF8CString("eventSender"));
    JSValueRef eventSenderObject = JSObjectMake(context, getClass(context), 0);
    JSObjectSetProperty(context, windowObject, eventSenderStr.get(), eventSenderObject, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, exception);
}

JSClassRef EventSenderEfl::getClass(JSContextRef context)
{
    static JSStaticFunction staticFunctions[] = {
        {"mouseWheelTo", mouseWheelToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
         {"contextClick", contextClickCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"mouseDown", mouseDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"mouseUp", mouseUpCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"mouseMoveTo", mouseMoveToCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"beginDragWithFiles", beginDragWithFilesCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"leapForward", leapForwardCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"keyDown", keyDownCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"textZoomIn", textZoomInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"textZoomOut", textZoomOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"zoomPageIn", zoomPageInCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {"zoomPageOut", zoomPageOutCallback, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete},
        {0, 0, 0}};

    static JSStaticValue staticValues[] = {
        {"dragMode", getDragModeCallback, setDragModeCallback, kJSPropertyAttributeNone},
        {0, 0, 0, 0}
    };

    static JSClassRef eventSenderClass = 0;

    if (!eventSenderClass) {
        JSClassDefinition jsClassDef = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        jsClassDef.staticFunctions = staticFunctions;
        jsClassDef.staticValues = staticValues;

        eventSenderClass = JSClassCreate(&jsClassDef);
    }

    return eventSenderClass;
}

JSValueRef EventSenderEfl::contextClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* mainFrame = EWebKitViewItem::instance()->getMainFrame();
    Evas_Object* webView = EWebKitViewItem::instance()->getWebView();
    Evas* evas = EWebKitViewItem::instance()->getEvas();

    if (!mainFrame || !webView || !evas)
        return JSValueMakeUndefined(context);

    ewk_frame_web_layout(mainFrame);

    Evas_Event_Mouse_Down* mouseDown = createMouseDownEvent(evas);
    
    down = true;
    evas_object_smart_callback_call(webView, "mouse,down,event", mouseDown);
    
    down = false;
    evas_object_smart_callback_call(webView, "mouse,up,event", mouseDown);

    delete mouseDown;
    
    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::mouseDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* mainFrame = EWebKitViewItem::instance()->getMainFrame();
    Evas_Object* webView = EWebKitViewItem::instance()->getWebView();
    Evas* evas = EWebKitViewItem::instance()->getEvas();

    if (!mainFrame || !webView || !evas)
        return JSValueMakeUndefined(context);

    down = true;

    Evas_Event_Mouse_Down* mouseDown = createMouseDownEvent(evas);

    if (!msgQueue[endOfQueue].delay) {
        ewk_frame_web_layout(mainFrame);
        evas_object_smart_callback_call(webView, "mouse,down,event", mouseDown);
        delete mouseDown;
    } else {
        msgQueue[endOfQueue].eventType = EVAS_CALLBACK_MOUSE_DOWN;
        msgQueue[endOfQueue++].eventInfo = mouseDown;
        replaySavedEvents();
    }
    
    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::mouseUpCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* mainFrame = EWebKitViewItem::instance()->getMainFrame();
    Evas_Object* webView = EWebKitViewItem::instance()->getWebView();
    Evas* evas = EWebKitViewItem::instance()->getEvas();

    if (!webView || !mainFrame || !evas)
        return JSValueMakeUndefined(context);

    Evas_Event_Mouse_Up* mouseUp = createMouseUpEvent(evas);

    down = false;

    if ((dragMode && !replayingSavedEvents) || msgQueue[endOfQueue].delay) {
        msgQueue[endOfQueue].eventType = EVAS_CALLBACK_MOUSE_UP;
        msgQueue[endOfQueue++].eventInfo = mouseUp;
        replaySavedEvents();
    } else {
        ewk_frame_web_layout(mainFrame);
        evas_object_smart_callback_call(webView, "mouse,up,event", mouseUp);
        delete mouseUp;
    }

    lastClickPositionX = lastMousePositionX;
    lastClickPositionY = lastMousePositionY;

    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::mouseMoveToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* mainFrame = EWebKitViewItem::instance()->getMainFrame();
    Evas_Object* webView = EWebKitViewItem::instance()->getWebView();
    Evas* evas = EWebKitViewItem::instance()->getEvas();

    if (!webView || !mainFrame || !evas)
        return JSValueMakeUndefined(context);

    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    lastMousePositionX = (int)JSValueToNumber(context, arguments[0], exception);
    lastMousePositionY = (int)JSValueToNumber(context, arguments[1], exception);
    
    Evas_Event_Mouse_Move* mouseMove = createMouseMoveEvent(evas);
    
    if (dragMode && down && !replayingSavedEvents) {
        msgQueue[endOfQueue].eventType = EVAS_CALLBACK_MOUSE_MOVE;
        msgQueue[endOfQueue].eventInfo = mouseMove;
    } else {
        DRT_DEBUG("Mouse Move Fire\n");
        ewk_frame_web_layout(mainFrame);
        evas_object_smart_callback_call(webView, "mouse,move,event", mouseMove);
        delete mouseMove;
    }
    
    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::beginDragWithFilesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::leapForwardCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::keyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::textZoomInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* mainFrame = EWebKitViewItem::instance()->getMainFrame();
    if (!mainFrame)
        return JSValueMakeUndefined(context);

    float zoom = ewk_frame_zoom_get(mainFrame);
    ewk_frame_zoom_set(mainFrame, zoom * zoomRatio);

    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::textZoomOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* mainFrame = EWebKitViewItem::instance()->getMainFrame();
    if (!mainFrame)
        return JSValueMakeUndefined(context);

    float zoom = ewk_frame_zoom_get(mainFrame);
    ewk_frame_zoom_set(mainFrame, zoom / zoomRatio);

    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::mouseWheelToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* mainFrame = EWebKitViewItem::instance()->getMainFrame();
    Evas* evas = EWebKitViewItem::instance()->getEvas();

    if (!mainFrame || !evas)
        return JSValueMakeUndefined(context);
    
    if (argumentCount < 2)
        return JSValueMakeUndefined(context);

    int horizontal = (int)JSValueToNumber(context, arguments[0], exception);
    int vertical = (int)JSValueToNumber(context, arguments[1], exception);
    
    Evas_Event_Mouse_Wheel* mouseWheel = createMouseWheelEvent(evas);

    if (vertical < 0)
        mouseWheel->direction = 0;
    else if (vertical > 0)
        mouseWheel->direction = 1;

    if (dragMode && down && !replayingSavedEvents) {
        msgQueue[endOfQueue].eventType = EVAS_CALLBACK_MOUSE_WHEEL;
        msgQueue[endOfQueue++].eventInfo = mouseWheel;
    } else {
        ewk_frame_web_layout(mainFrame);
        // ToDo what should be called to process this event
    }

    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::zoomPageOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* mainFrame = EWebKitViewItem::instance()->getMainFrame();
    if (!mainFrame)
        return JSValueMakeUndefined(context);

    const float zoomMultiplierRatio = 0.5f;
    float currentZoom = ewk_frame_zoom_get(mainFrame);

    ewk_frame_zoom_set(mainFrame, currentZoom - zoomMultiplierRatio);

    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::zoomPageInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception)
{
    Evas_Object* mainFrame = EWebKitViewItem::instance()->getMainFrame();
    if (!mainFrame)
        return JSValueMakeUndefined(context);

    const float zoomMultiplierRatio = 0.5f;
    float currentZoom = ewk_frame_zoom_get(mainFrame);

    ewk_frame_zoom_set(mainFrame, currentZoom + zoomMultiplierRatio);

    return JSValueMakeUndefined(context);
}

JSValueRef EventSenderEfl::getDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception)
{
    return JSValueMakeBoolean(context, dragMode);
}

bool EventSenderEfl::setDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception)
{
    dragMode = JSValueToBoolean(context, value);
}

void EventSenderEfl::replaySavedEvents()
{
    Evas_Object* webView = EWebKitViewItem::instance()->getWebView();
    if (!webView)
        return;

    replayingSavedEvents = true;

    for (unsigned pos = 0; pos < endOfQueue; pos++) {
        if (msgQueue[pos].eventType == EVAS_CALLBACK_MOUSE_UP) {
            evas_object_smart_callback_call(webView, "mouse,up,event", msgQueue[pos].eventInfo);
            delete static_cast<Evas_Event_Mouse_Up*>(msgQueue[pos].eventInfo);
        } else if (msgQueue[pos].eventType == EVAS_CALLBACK_MOUSE_DOWN) {
            evas_object_smart_callback_call(webView, "mouse,down,event", msgQueue[pos].eventInfo);
            delete static_cast<Evas_Event_Mouse_Down*>(msgQueue[pos].eventInfo);
        } else if (msgQueue[pos].eventType == EVAS_CALLBACK_MOUSE_MOVE) {
            evas_object_smart_callback_call(webView, "mouse,move,event", msgQueue[pos].eventInfo);
            delete static_cast<Evas_Event_Mouse_Move*>(msgQueue[pos].eventInfo);
        }
    }

    startOfQueue = 0;
    endOfQueue = 0;
    replayingSavedEvents = false;
}
