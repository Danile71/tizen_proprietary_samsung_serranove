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

#ifndef StringFindKey_h
#define StringFindKey_h

#include <wtf/text/StringHash.h>
#include "UString.h"

namespace JSC {

struct StringFindKey {
    JSString* searchTerm;
    RefPtr<StringImpl> pattern;

    StringFindKey()
    {
    }

    StringFindKey(JSString* searchTerm, const UString& pattern)
        : searchTerm(searchTerm)
        , pattern(pattern.impl())
    {
    }

    friend inline bool operator==(const StringFindKey& a, const StringFindKey& b);

    struct Hash {
        static unsigned hash(const StringFindKey& key) { return key.pattern->hash(); }
        static bool equal(const StringFindKey& a, const StringFindKey& b) { return a == b; }
        static const bool safeToCompareToEmptyOrDeleted = false;
    };
};

inline bool operator==(const StringFindKey& a, const StringFindKey& b)
{
    return (a.searchTerm == b.searchTerm) && equal(a.pattern.get(), b.pattern.get());
}

} // namespace JSC

namespace WTF {
template<typename T> struct DefaultHash;

template<> struct DefaultHash<JSC::StringFindKey> {
    typedef JSC::StringFindKey::Hash Hash;
};

template<> struct HashTraits<JSC::StringFindKey> : GenericHashTraits<JSC::StringFindKey> {
    static const bool emptyValueIsZero = true;
    static void constructDeletedValue(JSC::StringFindKey& slot) {  }
    static bool isDeletedValue(const JSC::StringFindKey& value) { return false; }
};
} // namespace WTF

#endif // StringFindKey_h
