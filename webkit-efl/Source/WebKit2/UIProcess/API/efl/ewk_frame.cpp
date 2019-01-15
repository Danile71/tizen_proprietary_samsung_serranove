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
#include "ewk_frame.h"

#include "WKAPICast.h"
#include "WKFrame.h"
#include "WKPage.h"
#include "WKRetainPtr.h"
#include "WKString.h"
#include <wtf/text/CString.h>

using namespace WebKit;
using namespace WebCore;

typedef struct _Ewk_Frame_Callback_Context
{
    union {
        Ewk_Frame_Source_Get_Callback frameSourceGetCallback;
    };
    Ewk_Frame_Ref frame;
    void* userData;
} Ewk_Frame_Callback_Context;

static void getSourceCallback(WKStringRef source, WKErrorRef error, void* context)
{
    EINA_SAFETY_ON_NULL_RETURN(context);
    Ewk_Frame_Callback_Context* callbackContext = static_cast<Ewk_Frame_Callback_Context*>(context);

    ASSERT(callbackContext->frameSourceGetCallback);

    if (source)
        callbackContext->frameSourceGetCallback(callbackContext->frame, toImpl(source)->string().utf8().data(), callbackContext->userData);
    else
        callbackContext->frameSourceGetCallback(callbackContext->frame, 0, callbackContext->userData);

    delete callbackContext;
}

Ewk_Frame_Ref ewk_frame_parent_get(Ewk_Frame_Ref frame)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(frame, 0);

    WKFrameRef wkFrame = static_cast<WKFrameRef>(frame);
    return static_cast<Ewk_Frame_Ref>(WKFrameGetParentFrame(wkFrame));
}

Eina_Bool ewk_frame_source_get(Ewk_Frame_Ref frame, Ewk_Frame_Source_Get_Callback callback, void* user_data)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(callback, false);

    Ewk_Frame_Callback_Context* context = new Ewk_Frame_Callback_Context;
    context->frameSourceGetCallback = callback;
    context->frame = frame;
    context->userData = user_data;
    WKFrameRef wkFrame = static_cast<WKFrameRef>(frame);
    WKPageGetSourceForFrame(WKFrameGetPage(wkFrame), wkFrame, context, getSourceCallback);

    return true;
}

Eina_Bool ewk_frame_can_show_mime_type(Ewk_Frame_Ref frame, char* mimeType)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(frame, false);
    EINA_SAFETY_ON_NULL_RETURN_VAL(mimeType, false);

    WKFrameRef wkFrame = static_cast<WKFrameRef>(frame);
    WKRetainPtr<WKStringRef> mimeTypeRef(AdoptWK, WKStringCreateWithUTF8CString(mimeType));
    return WKFrameCanShowMIMEType(wkFrame, mimeTypeRef.get());
}

Eina_Bool ewk_frame_is_main_frame(Ewk_Frame_Ref frame)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(frame, false);

    WKFrameRef wkFrame = static_cast<WKFrameRef>(frame);
    return WKFrameIsMainFrame(wkFrame);
}
