/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebIndexedDatabaseManager_h
#define WebIndexedDatabaseManager_h

#if ENABLE(TIZEN_INDEXED_DATABASE)
#include <wtf/Noncopyable.h>
#include <wtf/text/WTFString.h>

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
class MessageID;
}

namespace WebKit {

class WebIndexedDatabaseManager {
WTF_MAKE_NONCOPYABLE(WebIndexedDatabaseManager);
public:
    static WebIndexedDatabaseManager& shared();

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

private:
    WebIndexedDatabaseManager();

    void deleteAllIndexedDatabase() const;
    void deleteIndexedDatabaseForOrigin(const String& originIdentifier) const;
    void getIndexedDatabaseOrigins(uint64_t callbackID) const;

    void didReceiveWebIndexedDatabaseManagerMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    Vector<uint64_t> m_originsRequestCallbackIDs;
    String m_indexedDatabaseDirectory;
};

} // namespace WebKit
#endif
#endif // WebIndexedDatabaseManager_h
