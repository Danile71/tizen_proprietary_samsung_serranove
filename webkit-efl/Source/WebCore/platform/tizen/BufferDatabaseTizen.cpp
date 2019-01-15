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

// TODO: SQL optimization

#include "config.h"
#include "BufferDatabaseTizen.h"

#if ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
#include "Logging.h"
#include "SharedBuffer.h"
#include "SQLiteStatement.h"
#include "FileSystem.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <sys/time.h>

namespace WebCore {

static void createDatabaseTables(SQLiteDatabase& db)
{
    if (!db.executeCommand("CREATE TABLE BufferData (bufferID INTEGER PRIMARY KEY AUTOINCREMENT, data BLOB);")) {
        LOG_ERROR("Could not create BufferData table in database (%i) -%s", db.lastError(), db.lastErrorMsg());
        db.close();
    }
}

// readySQLiteStatement() handles two things
// 1 - If the SQLDatabase& argument is different, the statement must be destroyed and remade.  This happens when the user
//     switches to and from private browsing
// 2 - Lazy construction of the Statement in the first place, in case we've never made this query before
inline void readySQLiteStatement(OwnPtr<SQLiteStatement>& statement, SQLiteDatabase& db, const String& str)
{
    if (statement && (statement->database() != &db || statement->isExpired())) {
        if (statement->isExpired()) {
            LOG(SQLDatabase, "SQLiteStatement associated with %s is expired", str.ascii().data());
            statement.clear();
        }
    }

    if (!statement) {
        statement = adoptPtr(new SQLiteStatement(db, str));
        if (statement->prepare() != SQLResultOk)
            LOG_ERROR("Preparing statement %s failed", str.ascii().data());
    }
}

static bool isValidDatabase(SQLiteDatabase& db)
{
    // These four tables should always exist in a valid db
    if (!db.tableExists("BufferData"))
        return false;

    return true;
}

BufferDatabase::BufferDatabase()
{
}

BufferDatabase::~BufferDatabase()
{
}

void BufferDatabase::open()
{
    m_databasePath = pathByAppendingComponent(m_databaseDirectoryPath, defaultDatabaseFilename());
    makeAllDirectories(m_databaseDirectoryPath);

    MutexLocker locker(m_DBLock);

    if (!m_DB.open(m_databasePath)) {
        LOG_ERROR("Unable to open cache database at path %s - %s",
                  m_databasePath.ascii().data(),
                  m_DB.lastErrorMsg());
        return;
    }

    if (!isValidDatabase(m_DB)) {
      LOG(SQLDatabase, "%s is missingn or in an invalid  state - reconstructing",
          m_databasePath.ascii().data());
        m_DB.clearAllTables();
        createDatabaseTables(m_DB);
    }

    LOG(DiskCache, "DB opened");
}

void BufferDatabase::ensureOpen()
{
    if (!isOpen())
        open();
}

void BufferDatabase::deleteAllPreparedStatements()
{
    m_addBufferDataStatement.clear();
    m_getBufferDataStatement.clear();
    m_removeBufferDataStatement.clear();
}

void BufferDatabase::close()
{
    MutexLocker locker(m_DBLock);

    deleteAllPreparedStatements();
    m_DB.close();
}

bool BufferDatabase::isOpen() const
{
    MutexLocker locker(m_DBLock);
    return m_DB.isOpen();
}

String BufferDatabase::defaultDatabaseFilename()
{
    DEFINE_STATIC_LOCAL(String, defaultDatabaseFilename, ("resource.db"));
    return defaultDatabaseFilename.isolatedCopy();
}

BufferDatabase& bufferDatabase()
{
    static BufferDatabase* defaultDatabase;

    if (!defaultDatabase)
        defaultDatabase = new BufferDatabase;

    return *defaultDatabase;
}

int64_t BufferDatabase::cacheBuffer(const SharedBuffer& buffer)
{
    readySQLiteStatement(m_addBufferDataStatement, m_DB,
                         "INSERT INTO BufferData (data) VALUES (?);");
    m_addBufferDataStatement->bindBlob(1, buffer.data(), buffer.size());

    if (m_addBufferDataStatement->step() != SQLResultDone) {
        LOG_ERROR("Failed to add buffer data");
        return 0;
    }

    int64_t bufferID = m_DB.lastInsertRowID();

    m_addBufferDataStatement->reset();

    return bufferID;
}

void BufferDatabase::getBufferData(int64_t id, Vector<char>& data)
{
    readySQLiteStatement(m_getBufferDataStatement, m_DB,
                        "SELECT BufferData.data FROM BufferData WHERE BufferData.bufferID = (?);");
    m_getBufferDataStatement->bindInt64(1, id);

    int result = m_getBufferDataStatement->step();
    if (result == SQLResultRow)
        m_getBufferDataStatement->getColumnBlobAsVector(0, data);
    else if (result != SQLResultDone)
        LOG_ERROR("SQL failed for get buffer data on ID: %ld", id);

    m_getBufferDataStatement->reset();
}

void BufferDatabase::removeBufferData(int64_t id)
{
    readySQLiteStatement(m_removeBufferDataStatement, m_DB,
                        "DELETE FROM BufferData WHERE BufferData.bufferID = (?);");
    m_removeBufferDataStatement->bindInt64(1, id);

    if (m_removeBufferDataStatement->step() != SQLResultDone)
        LOG_ERROR("SQL failed for remove buffer data on ID: %ld", id);

    m_removeBufferDataStatement->reset();
}

void BufferDatabase::setResourceCacheDirectory(const String& resourceCacheDirectory)
{
    m_databaseDirectoryPath = resourceCacheDirectory;
}

const String& BufferDatabase::resourceCacheDirectory() const
{
    return m_databaseDirectoryPath;
}

} // namespace WebCore
#endif // ENABLE(TIZEN_BACKGROUND_DISK_CACHE)
