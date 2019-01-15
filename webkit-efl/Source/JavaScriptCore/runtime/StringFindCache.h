/*
 * Copyright (C) 2010 University of Szeged
 * Copyright (C) 2010 Renata Hodovan (hodovan@inf.u-szeged.hu)
 * Copyright (C) 2013 Samsung Electronics
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "StringPrototype.h"
#include "StringFindKey.h"
#include "Strong.h"
#include "UString.h"
#include "Weak.h"
#include <wtf/FixedArray.h>
#include <wtf/HashMap.h>

#ifndef StringFindCache_h
#define StringFindCache_h

namespace JSC {

class StringFindCache : private WeakHandleOwner {

typedef HashMap<StringFindKey, Weak<JSString> > StringFindCacheMap;

public:
	StringFindCache();
    void invalidateCode();
    JSString* lookup(const UString& patternString, JSString* searchTerm);
    JSString* lookup(const StringFindKey& key);
    void create(const UString& patternString, JSString* searchTerm, JSString* result);
    void create(const StringFindKey& key, JSString* result);

private:
    virtual void finalize(Handle<Unknown>, void* context) OVERRIDE;

    StringFindCacheMap m_weakCache;
    static const int maxCacheableEntries = 125;
    int cachedElementCount;
};

} // namespace JSC

#endif // StringFindCache_h
