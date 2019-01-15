/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#if ENABLE(INDEXED_DATABASE)
#include "IDBBindingUtilities.h"

#include "DOMRequestState.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "IDBTracing.h"
#include "JSIDBKey.h"

#include <runtime/DateInstance.h>
#if ENABLE(TIZEN_INDEXED_DATABASE)
#include "JSDOMBinding.h"
#include <APICast.h>
#include <DateInstance.h>
#include <JSContextRef.h>

using namespace JSC;
#endif // TIZEN_INDEXED_DATABASE


namespace WebCore {

#if ENABLE(TIZEN_INDEXED_DATABASE)
static bool get(ExecState* exec, JSValue object, const String&keyPathElement, JSValue& result)
{
    if (object.isString() && keyPathElement == "length") {
        result = jsNumber(object.toString(exec)->length());
        return true;
    }
    if (!object.isObject())
        return false;

    Identifier identifier(&exec->globalData(), keyPathElement.utf8().data());
    if (!asObject(object)->hasProperty(exec, identifier))
        return false;
    result = asObject(object)->get(exec, identifier);
    return true;
}

static bool canSet(JSValue object)
{
    return object.isObject();
}

static const size_t maximumDepth = 2000;

PassRefPtr<IDBKey> createIDBKeyFromValue(ExecState* exec, JSValue value, Vector<JSArray*>&stack)
{
    if (value.isNumber() && !isnan(value.toNumber(exec)))
        return IDBKey::createNumber(value.toNumber(exec));
    if (value.isString())
        return IDBKey::createString(ustringToString(value.toString(exec)->tryGetValue()));
    if (value.inherits(&DateInstance::s_info) && !isnan(valueToDate(exec, value)))
        return IDBKey::createDate(valueToDate(exec, value));
    if (value.isObject()) {
        JSObject* object = asObject(value);
        if (isJSArray(object) || object->inherits(&JSArray::s_info)) {
            JSArray* array = asArray(object);
            size_t length = array->length();

            if (stack.contains(array))
                return 0;
            if (stack.size() >= maximumDepth)
                return 0;

            stack.append(array);

            IDBKey::KeyArray subkeys;
            for (size_t i = 0; i < length; i++) {
                JSValue item = array->getIndex(i);
                RefPtr<IDBKey> subkey = createIDBKeyFromValue(exec, item, stack);
                if (!subkey)
                    subkeys.append(IDBKey::createInvalid());
                else
                    subkeys.append(subkey);
            }

        stack.removeLast();
        return IDBKey::createArray(subkeys);
        }
    }
    return 0;
}

PassRefPtr<IDBKey> createIDBKeyFromValue(ExecState* exec, JSValue value)
{
    Vector<JSArray*> stack;
    RefPtr<IDBKey> key = createIDBKeyFromValue(exec, value, stack);
    if (key)
        return key;
    return IDBKey::createInvalid();
}

JSValue getNthValueOnKeyPath(ExecState* exec, JSValue rootValue, const Vector<String>& keyPathElements, size_t index)
{
    JSValue currentValue;
    JSObject* object = rootValue.toObject(exec);
    PropertySlot slot(object);
    ASSERT(index <= keyPathElements.size());
    for (size_t i = 0 ; i < index; ++i) {
        Identifier propertyName = Identifier(exec, keyPathElements[i].impl());
        if (!object->getOwnPropertySlot(object, exec, propertyName, slot))
            return jsUndefined();

        currentValue = slot.getValue(exec, propertyName);
        object = currentValue.toObject(exec);
    }
    return currentValue;
}

PassRefPtr<IDBKey> createIDBKeyFromScriptValueAndKeyPath(ExecState* exec, const ScriptValue& value, const String& keyPath)
{
    Vector<String> keyPathElements;
    IDBKeyPathParseError error;
    IDBParseKeyPath(keyPath, keyPathElements, error);

    JSValue jsValue = value.jsValue();
    if (jsValue) {
        jsValue = getNthValueOnKeyPath(exec, jsValue, keyPathElements, keyPathElements.size());
        if (jsValue.isUndefined())
            return 0;
    } else
        return 0;

    return createIDBKeyFromValue(exec, jsValue);
}

static JSValue ensureNthValueOnKeyPath(ExecState* exec, JSValue rootValue, const Vector<String>& keyPathElements, size_t index)
{
    JSValue currentValue(rootValue);

    ASSERT(index <= keyPathElements.size());
    for (size_t i = 0; i < index; i++) {
        JSValue parentValue(currentValue);
        const String& keyPathElement = keyPathElements[i];
        if (!get(exec, currentValue, keyPathElement, currentValue)) {
            JSObject* object = constructEmptyObject(exec);
            if (!canSet(parentValue))
                return jsUndefined();
            JSObject* parentObject = parentValue.toObject(exec);
            parentObject->putDirect(exec->globalData(), Identifier(exec, keyPathElement.impl()), JSValue(object));
            currentValue = JSValue(object);
        }
    }
    return currentValue;
}

static bool canInjectNthValueOnKeyPath(ExecState* exec, JSValue rootValue, const Vector<String>& keyPathElements, size_t index)
{
    if (!rootValue.isObject())
        return false;

    JSValue currentValue(rootValue);

    ASSERT(index <= keyPathElements.size());
    for (size_t i = 0; i < index; ++i) {
        JSValue parentValue(currentValue);
        const String& keyPathElement = keyPathElements[i];
        if (!get(exec, parentValue, keyPathElement, currentValue))
            return canSet(parentValue);
    }
    return true;
}

bool injectIDBKeyIntoScriptValue(DOMRequestState* requestState, PassRefPtr<IDBKey> key, ScriptValue& value, const IDBKeyPath& keyPath)
{
    ASSERT(keyPath.type() == IDBKeyPath::StringType);

    Vector<String> keyPathElements;
    IDBKeyPathParseError error;
    IDBParseKeyPath(keyPath.string(), keyPathElements, error);

    if (keyPathElements.isEmpty())
        return false;

    ExecState* exec = requestState->exec();

    JSValue parent = ensureNthValueOnKeyPath(exec, value.jsValue(), keyPathElements, keyPathElements.size() - 1);
    if (parent.isUndefined())
        return false;
    if (!canSet(parent))
        return false;

    JSObject* object = parent.toObject(exec);
    object->putDirect(exec->globalData(), Identifier(exec, keyPathElements.last().impl()), toJS(exec, jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), key.get()));

    return true;
}

PassRefPtr<IDBKey> createIDBKeyFromScriptValueAndKeyPath(DOMRequestState* requestState, const ScriptValue& value, const IDBKeyPath& keyPath)
{
    ASSERT(!keyPath.isNull());

    ExecState* exec = requestState->exec();

    if (keyPath.type() == IDBKeyPath::ArrayType) {
        IDBKey::KeyArray result;
        const Vector<String>& array = keyPath.array();
        for (size_t i = 0; i < array.size(); ++i) {
            RefPtr<IDBKey> key = createIDBKeyFromScriptValueAndKeyPath(exec, value, array[i]);
            if (!key)
                return 0;
            result.append(key);
        }
        return IDBKey::createArray(result);
    }

    ASSERT(keyPath.type() == IDBKeyPath::StringType);
    return createIDBKeyFromScriptValueAndKeyPath(exec, value, keyPath.string());
}

bool canInjectIDBKeyIntoScriptValue(DOMRequestState* requestState, const ScriptValue& scriptValue, const IDBKeyPath& keyPath)
{
    IDB_TRACE("canInjectIDBKeyIntoScriptValue");

    ASSERT(keyPath.type() == IDBKeyPath::StringType);
    Vector<String> keyPathElements;
    IDBKeyPathParseError error;
    IDBParseKeyPath(keyPath.string(), keyPathElements, error);
    ASSERT(error == IDBKeyPathParseErrorNone);

    if (!keyPathElements.size())
        return false;

    ExecState* exec = requestState->exec();
    return canInjectNthValueOnKeyPath(exec, scriptValue.jsValue(), keyPathElements, keyPathElements.size() - 1);
}

ScriptValue deserializeIDBValue(DOMRequestState* requestState, PassRefPtr<SerializedScriptValue> prpValue)
{
    ExecState* exec = requestState->exec();
    RefPtr<SerializedScriptValue> serializedValue = prpValue;
    if (serializedValue)
        return ScriptValue::deserialize(exec, serializedValue.get(), NonThrowing);
    return ScriptValue(exec->globalData(), jsNull());
}

ScriptValue idbKeyToScriptValue(DOMRequestState* requestState, PassRefPtr<IDBKey> key)
{
    ExecState* exec = requestState->exec();
    return ScriptValue(exec->globalData(), toJS(exec, jsCast<JSDOMGlobalObject*>(exec->lexicalGlobalObject()), key.get()));
}
#else
PassRefPtr<IDBKey> createIDBKeyFromValue(JSC::ExecState* exec, JSC::JSValue value)
{
    if (value.isNull())
        return IDBKey::createInvalid();
    if (value.isInt32())
        return IDBKey::createNumber(value.toNumber(exec));
    if (value.isString())
        return IDBKey::createString(ustringToString(value.toString(exec)->value(exec)));
    // FIXME: Implement dates.
    return 0;
}
#endif // TIZEN_INDEXED_DATABASE

} // namespace WebCore

#endif // ENABLE(INDEXED_DATABASE)
