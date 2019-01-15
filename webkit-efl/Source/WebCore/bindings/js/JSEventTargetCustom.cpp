/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSEventTarget.h"

#include "EventTargetHeaders.h"
#include "EventTargetInterfaces.h"
#include "JSDOMWindowShell.h"
#include "JSEventListener.h"
#include "JSMessagePort.h"
#include "JSNode.h"
#if ENABLE(SHARED_WORKERS)

#include "JSSharedWorker.h"
#include "JSSharedWorkerContext.h"
#endif

#include "JSXMLHttpRequest.h"
#include "JSXMLHttpRequestUpload.h"
#include "MessagePort.h"

#if ENABLE(SHARED_WORKERS)
#include "SharedWorker.h"
#include "SharedWorkerContext.h"
#endif

#include "XMLHttpRequest.h"
#include "XMLHttpRequestUpload.h"

#include "EventSource.h"
#include "JSEventSource.h"

#include "DOMApplicationCache.h"
#include "JSDOMApplicationCache.h"

#if ENABLE(SVG)
#include "SVGElementInstance.h"
#include "JSSVGElementInstance.h"
#endif

#if ENABLE(WORKERS)
#include "DedicatedWorkerContext.h"
#include "JSDedicatedWorkerContext.h"
#include "JSWorker.h"
#include "Worker.h"
#endif

#if ENABLE(NOTIFICATIONS)
#include "JSNotification.h"
#include "Notification.h"
#endif

#if ENABLE(INDEXED_DATABASE)
#include "IDBRequest.h"
#include "JSIDBRequest.h"
#endif

#if ENABLE(TIZEN_INDEXED_DATABASE)
#include "IDBDatabase.h"
#include "IDBTransaction.h"
#include "JSIDBDatabase.h"
#include "JSIDBTransaction.h"
#endif

#if ENABLE(WEB_AUDIO)
#include "AudioContext.h"
#include "JSAudioContext.h"
#include "JSScriptProcessorNode.h"
#include "ScriptProcessorNode.h"
#endif

#if ENABLE(WEB_SOCKETS)
#include "JSWebSocket.h"
#include "WebSocket.h"
#endif

#if ENABLE(BLOB)
#include "JSFileReader.h"
#include "FileReader.h"
#endif

#if ENABLE(MEDIA_STREAM)
#include "LocalMediaStream.h"
#include "MediaStream.h"
#endif

#if ENABLE(TIZEN_FILE_SYSTEM)
#include "FileWriter.h"
#include "JSFileWriter.h"
#endif

using namespace JSC;

namespace WebCore {

#define TRY_TO_WRAP_WITH_INTERFACE(interfaceName) \
    if (eventNames().interfaceFor##interfaceName == desiredInterface) \
        return toJS(exec, globalObject, static_cast<interfaceName*>(target));

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, EventTarget* target)
{
    if (!target)
        return jsNull();

    AtomicString desiredInterface = target->interfaceName();

    // FIXME: Why can't we use toJS for these cases?
#if ENABLE(WORKERS)
    if (eventNames().interfaceForDedicatedWorkerContext == desiredInterface)
        return toJSDOMGlobalObject(static_cast<DedicatedWorkerContext*>(target), exec);
#endif
#if ENABLE(SHARED_WORKERS)
    if (eventNames().interfaceForSharedWorkerContext == desiredInterface)
        return toJSDOMGlobalObject(static_cast<SharedWorkerContext*>(target), exec);
#endif

    DOM_EVENT_TARGET_INTERFACES_FOR_EACH(TRY_TO_WRAP_WITH_INTERFACE)

#if ENABLE(SLP_FILE_SYSTEM)
#if ENABLE(SLP_DAILY_UPVERSIONING)
    if (target)
        return toJS(exec, globalObject, target);
#else
    if (FileWriter* fileWriter = target->toFileWriter())
        return toJS(exec, globalObject, fileWriter);
#endif
#endif
    ASSERT_NOT_REACHED();
    return jsNull();
}

#undef TRY_TO_WRAP_WITH_INTERFACE

#define TRY_TO_UNWRAP_WITH_INTERFACE(interfaceName) \
    if (value.inherits(&JS##interfaceName::s_info)) \
        return static_cast<interfaceName*>(jsCast<JS##interfaceName*>(asObject(value))->impl());

EventTarget* toEventTarget(JSC::JSValue value)
{
    if (value.inherits(&JSDOMWindowShell::s_info))
        return jsCast<JSDOMWindowShell*>(asObject(value))->impl();

    DOM_EVENT_TARGET_INTERFACES_FOR_EACH(TRY_TO_UNWRAP_WITH_INTERFACE)
    return 0;
}

#undef TRY_TO_UNWRAP_WITH_INTERFACE

} // namespace WebCore
