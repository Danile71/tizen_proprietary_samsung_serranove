/*
 * Copyright (C) 2010 University of Szeged
 * Copyright (C) 2010 Renata Hodovan (hodovan@inf.u-szeged.hu)
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include "config.h"

#include "StringFindCache.h"
#include "StringObject.h"
#include "StrongInlines.h"

namespace JSC {

JSString* StringFindCache::lookup(const UString& patternString, JSString* searchTerm)
{
    StringFindKey key(searchTerm, patternString);

    if (JSString* result = m_weakCache.get(key))
        return result;

    return 0;
}

JSString* StringFindCache::lookup(const StringFindKey& key)
{
    if (JSString* result = m_weakCache.get(key))
        return result;

    return 0;
}

void StringFindCache::create(const UString& patternString, JSString* searchTerm, JSString* result)
{

    if (cachedElementCount == maxCacheableEntries)
        invalidateCode();

    StringFindKey key = StringFindKey(searchTerm, patternString);

    weakAdd(m_weakCache, key, PassWeak<JSString>(result, this));

    ++cachedElementCount;
}

void StringFindCache::create(const StringFindKey& key, JSString* result)
{

    if (cachedElementCount == maxCacheableEntries)
        invalidateCode();

    weakAdd(m_weakCache, key, PassWeak<JSString>(result, this));

    ++cachedElementCount;
}

StringFindCache::StringFindCache()
{
    cachedElementCount = 0;
}

void StringFindCache::finalize(Handle<Unknown> handle, void*)
{
    m_weakCache.clear();
}

void StringFindCache::invalidateCode()
{
    m_weakCache.clear();
    cachedElementCount = 0;
}

}
