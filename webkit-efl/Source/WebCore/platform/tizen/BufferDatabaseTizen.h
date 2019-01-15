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

#ifndef BufferDatabase_h
#define BufferDatabase_h

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
#include "SQLiteDatabase.h"

#include <wtf/Vector.h>

namespace WebCore {

class SharedBuffer;

class BufferDatabase {
public:
    ~BufferDatabase();

    bool isOpen() const;
    void open();
    void close();
    void ensureOpen();
    String defaultDatabaseFilename();

    int64_t cacheBuffer(const SharedBuffer& buffer);
    void getBufferData(int64_t id, Vector<char>& data);
    void removeBufferData(int64_t id);

    void setResourceCacheDirectory(const String&);
    const String& resourceCacheDirectory() const;

private:
    BufferDatabase();
    friend BufferDatabase& bufferDatabase();

    void deleteAllPreparedStatements();

    String m_databasePath;
    String m_databaseDirectoryPath;

    mutable Mutex m_DBLock;
    SQLiteDatabase m_DB;

    OwnPtr<SQLiteStatement> m_addBufferDataStatement;
    OwnPtr<SQLiteStatement> m_getBufferDataStatement;
    OwnPtr<SQLiteStatement> m_removeBufferDataStatement;

};


BufferDatabase& bufferDatabase();


};

#endif // ENABLE(TIZEN_BACKGROUND_DISK_CACHE)

#endif // Database_h

