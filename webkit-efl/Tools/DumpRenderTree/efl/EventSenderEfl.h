#ifndef EventSenderEfl_h
#define EventSenderEfl_h

#include <JavaScriptCore/JavaScript.h>
#include "utility.h"

struct DelayedMessage {
    void* eventInfo;
    unsigned eventType;
    unsigned delay;
};

class EventSenderEfl {
    static bool down;
    static bool dragMode;
    static bool replayingSavedEvents;
    static int clickCount;
    static int lastClickPositionX;
    static int lastClickPositionY;
    static int lastMousePositionX;
    static int lastMousePositionY;
    static unsigned endOfQueue;
    static unsigned startOfQueue;
    static const float zoomRatio = 1.2f;
    
public:    
    EventSenderEfl() {}
    ~EventSenderEfl() {}

    void makeEventSender(JSContextRef context, JSObjectRef windowObject, JSValueRef* exception);
    JSClassRef getClass(JSContextRef context);

    static Evas_Event_Mouse_Move* createMouseMoveEvent(Evas* evas);
    static Evas_Event_Mouse_Down* createMouseDownEvent(Evas* evas);
    static Evas_Event_Mouse_Up* createMouseUpEvent(Evas* evas);
    static Evas_Event_Mouse_Wheel* createMouseWheelEvent(Evas* evas);

    static JSValueRef zoomPageOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef zoomPageInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef mouseWheelToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef contextClickCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef mouseDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef mouseUpCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef mouseMoveToCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef beginDragWithFilesCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef leapForwardCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef keyDownCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef textZoomInCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef textZoomOutCallback(JSContextRef context, JSObjectRef function, JSObjectRef thisObject, size_t argumentCount, const JSValueRef arguments[], JSValueRef* exception);
    static JSValueRef getDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef* exception);
    static bool setDragModeCallback(JSContextRef context, JSObjectRef object, JSStringRef propertyName, JSValueRef value, JSValueRef* exception);
    static void replaySavedEvents();
};

#endif
