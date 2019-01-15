/*
 * Copyright (C) 2011 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "AsyncFileSystemTizen.h"

#if ENABLE(TIZEN_FILE_SYSTEM)

#include "AsyncFileSystemCallbacksTizen.h"
#include "AsyncFileWriterTizen.h"
#include "Blob.h"
#include "BlobRegistryImpl.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "CrossThreadTask.h"
#include "FileError.h"
#include "FileMetadata.h"
#include "FileSystem.h"
#include "GOwnPtr.h"
#include "GroupSettings.h"
#include "LocalFileSystem.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PageGroup.h"
#include "SecurityOrigin.h"
#include "WorkerContext.h"
#include "WorkerRunLoop.h"
#include "WorkerThread.h"
#include "dirent.h"
#include "sys/stat.h"
#include "sys/types.h"
#include "time.h"
#include "unistd.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

static void openFileSystem(ScriptExecutionContext* context, const String& basePath, const String& identifier, FileSystemType type, bool create, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
{
    String typeString = (type == FileSystemTypePersistent) ? "Persistent" : "Temporary";
    StringBuilder storageIdentifier;
    storageIdentifier.append(identifier);
    storageIdentifier.append('/');
    storageIdentifier.append(typeString);

    StringBuilder rootURL;
    rootURL.append("filesystem:");
    rootURL.append(storageIdentifier);
    rootURL.append('/');

    StringBuilder fileSystemPath;
    fileSystemPath.append(basePath);
    if (!fileSystemPath.toString().endsWith("/"))
        fileSystemPath.append('/');
    RefPtr<SecurityOrigin> origin = SecurityOrigin::createFromString(identifier);
    fileSystemPath.append(origin->databaseIdentifier());
    fileSystemPath.append('/');
    fileSystemPath.append(typeString);
    fileSystemPath.append('/');

    if (create)
        makeAllDirectories(fileSystemPath.toString());

    AsyncFileSystem::openFileSystem(rootURL.toString(), storageIdentifier.toString(), create, AsyncFileSystemCallbacksTizen::create(context, callbacks, synchronousType));
}

void AsyncFileSystem::deleteFileSystem(const String&, const String&, FileSystemType, PassOwnPtr<AsyncFileSystemCallbacks>)
{
    notImplemented();
}

void LocalFileSystem::readFileSystem(ScriptExecutionContext* context, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
{
    openFileSystem(context, fileSystemBasePath(), context->securityOrigin()->toString(), type, false, callbacks, synchronousType);
}

void LocalFileSystem::requestFileSystem(ScriptExecutionContext* context, FileSystemType type, long long, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
{
    openFileSystem(context, fileSystemBasePath(), context->securityOrigin()->toString(), type, true, callbacks, synchronousType);
}

bool AsyncFileSystem::isAvailable()
{
    return true;
}

PassOwnPtr<AsyncFileSystem> AsyncFileSystem::create()
{
    return adoptPtr(new AsyncFileSystemTizen());
}

void AsyncFileSystem::openFileSystem(const String& basePath, const String& storageIdentifier, bool create, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    if (basePath.isEmpty()) {
        callbacks->didFail(FileError::SECURITY_ERR);
        return;
    }

    callbacks->didOpenFileSystem(storageIdentifier, KURL(ParsedURLString, basePath), AsyncFileSystem::create());
}

AsyncFileSystemTizen::AsyncFileSystemTizen()
    : AsyncFileSystem()
    , m_localFileSystemBasePath(LocalFileSystem::localFileSystem().fileSystemBasePath())
{
}

AsyncFileSystemTizen::~AsyncFileSystemTizen()
{
}


void AsyncFileSystemTizen::stop()
{
}

bool AsyncFileSystemTizen::hasPendingActivity()
{
    return false;
}

bool AsyncFileSystemTizen::waitForOperationToComplete()
{
    return m_taskController->waitForTaskToComplete();
}

static void moveAsync(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacksTizen> helperCallbacks, const String& sourcePath, const String& destinationPath)
{
    if (!fileExists(sourcePath)) {
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
        return;
    }

    if (destinationPath.isEmpty()) {
        helperCallbacks->didFail(FileError::SECURITY_ERR);
        return;
    }

    if (renameFile(sourcePath, destinationPath))
        helperCallbacks->didSucceed();
    else
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
}

void AsyncFileSystemTizen::move(const KURL& srcPath, const KURL& destPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String sourceFileSystemPath = virtualPathToFileSystemPath(srcPath);
    String destinationFileSystemPath = virtualPathToFileSystemPath(destPath);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&moveAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), sourceFileSystemPath, destinationFileSystemPath));
}

static void copyAsync(ScriptExecutionContext* context, PassOwnPtr<AsyncFileSystemCallbacksTizen> helperCallbacks, const String& sourcePath, const String& destinationPath)
{
    if (!fileExists(sourcePath)) {
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
        return;
    }

    if (destinationPath.isEmpty()) {
        helperCallbacks->didFail(FileError::SECURITY_ERR);
        return;
    }

    int errorCode = 0;
    if (!AsyncFileSystemTizen::checkQuota(context, sourcePath, errorCode)) {
        helperCallbacks->didFail(errorCode);
        return;
    }

    if (copyFile(sourcePath, destinationPath))
        helperCallbacks->didSucceed();
    else
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
}

void AsyncFileSystemTizen::copy(const KURL& srcPath, const KURL& destPath, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String sourceFileSystemPath = virtualPathToFileSystemPath(srcPath);
    String destinationFileSystemPath = virtualPathToFileSystemPath(destPath);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&copyAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), sourceFileSystemPath, destinationFileSystemPath));
}

static void removeAsync(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacksTizen> helperCallbacks, const String& path)
{
    if (deleteFile(path))
        helperCallbacks->didSucceed();
    else
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
}

void AsyncFileSystemTizen::remove(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&removeAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), fileSystemPath));
}

static void removeRecursivelyAsync(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacksTizen> helperCallbacks, const String& path)
{
    if (removeDirectory(path))
        helperCallbacks->didSucceed();
    else
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
}

void AsyncFileSystemTizen::removeRecursively(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&removeRecursivelyAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), fileSystemPath));
}

static void readMetadataAsync(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacksTizen> helperCallbacks, const String& path)
{
    if (!fileExists(path)) {
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
        return;
    }

    FileMetadata metadata;
    if (!getFileMetadata(path, metadata)) {
        helperCallbacks->didFail(FileError::NOT_READABLE_ERR);
        return;
    }

    helperCallbacks->didReadMetadata(metadata);
}

void AsyncFileSystemTizen::readMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&readMetadataAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), fileSystemPath));
}

static void createFileAsync(ScriptExecutionContext* context, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacksTizen> helperCallbacks, const String& path)
{
    if (exclusive && fileExists(path)) {
        helperCallbacks->didFail(FileError::PATH_EXISTS_ERR);
        return;
    }

    int errorCode = 0;
    if (!AsyncFileSystemTizen::checkQuota(context, path, errorCode)) {
        helperCallbacks->didFail(errorCode);
        return;
    }

    PlatformFileHandle handle;
    handle = openFile(path, OpenForWrite);

    if (handle != invalidPlatformFileHandle) {
        closeFile(handle);
        helperCallbacks->didSucceed();
    } else
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
}

void AsyncFileSystemTizen::createFile(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&createFileAsync, exclusive, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), fileSystemPath));
}

static void createDirectoryAsync(ScriptExecutionContext* context, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacksTizen> helperCallbacks, const String& path)
{
    if (exclusive && fileExists(path)) {
        helperCallbacks->didFail(FileError::PATH_EXISTS_ERR);
        return;
    }

    int errorCode = 0;
    if (!AsyncFileSystemTizen::checkQuota(context, path, errorCode)) {
        helperCallbacks->didFail(errorCode);
        return;
    }

    if (makeAllDirectories(path))
        helperCallbacks->didSucceed();
    else
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
}

void AsyncFileSystemTizen::createDirectory(const KURL& path, bool exclusive, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&createDirectoryAsync, exclusive, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), fileSystemPath));
}

static void objectExistsAsync(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacksTizen> helperCallbacks, const String& path, FileMetadata::Type type)
{
    if (!fileExists(path)) {
        helperCallbacks->didFail(FileError::NOT_FOUND_ERR);
        return;
    }

    FileMetadata metadata;
    if (!getFileMetadata(path, metadata)) {
        helperCallbacks->didFail(FileError::NOT_READABLE_ERR);
        return;
    }

    if (type != metadata.type)
        helperCallbacks->didFail(FileError::TYPE_MISMATCH_ERR);
    else
        helperCallbacks->didSucceed();
}

void AsyncFileSystemTizen::fileExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&objectExistsAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), fileSystemPath, FileMetadata::TypeFile));
}


void AsyncFileSystemTizen::directoryExists(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&objectExistsAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), fileSystemPath, FileMetadata::TypeDirectory));
}

static void readDirectoryAsync(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacksTizen> callbacks, const String& path)
{
    if (!fileExists(path)) {
        callbacks->didFail(FileError::NOT_FOUND_ERR);
        return;
    }

    DEFINE_STATIC_LOCAL(const String, fileMatchPattern, ("*"));
    Vector<String> paths = listDirectory(path, fileMatchPattern);
    Vector<String>::const_iterator end = paths.end();
    for (Vector<String>::const_iterator it = paths.begin(); it != end; ++it) {
        String fileName = pathGetFileName(*it);
        if (fileName == "." || fileName == "..")
            continue;

        FileMetadata metadata;
        if (!getFileMetadata(*it, metadata)) {
            callbacks->didFail(FileError::NOT_READABLE_ERR);
            return;
        }

        bool isEntryDirectory;
        if (metadata.type == FileMetadata::TypeDirectory)
            isEntryDirectory = true;
        else
            isEntryDirectory = false;

        callbacks->didReadDirectoryEntry(fileName, isEntryDirectory);
    }

    callbacks->didReadDirectoryEntries(false);
}

void AsyncFileSystemTizen::readDirectory(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->postTaskToMainThread(createCallbackTask(&readDirectoryAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), fileSystemPath));
}

static void createWriterAsync(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacksTizen> helperCallbacks, AsyncFileWriterClient* client, const String& path, AsyncFileSystemTaskControllerTizen* taskController)
{
    long long fileSize = 0;
    getFileSize(path, fileSize);

    helperCallbacks->didCreateFileWriter(AsyncFileWriterTizen::create(client, path, taskController), fileSize);
}

void AsyncFileSystemTizen::createWriter(AsyncFileWriterClient* client, const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->setCurrentTaskMode(mode);
    m_taskController->postTaskToMainThread(createCallbackTask(&createWriterAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), AllowCrossThreadAccess(client), fileSystemPath, AllowCrossThreadAccess(m_taskController.get())));
}

void AsyncFileSystemTizen::createSnapshotFileAndReadMetadata(const KURL& path, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    // Tizen supports only local filesystem. So simply return the metadata of the file itself (as well as readMetadata does)
    String fileSystemPath = virtualPathToFileSystemPath(path);
    String mode = m_taskController->uniqueMode();
    m_taskController->setCurrentTaskMode(mode);
    m_taskController->postTaskToMainThread(createCallbackTask(&readMetadataAsync, AsyncFileSystemCallbacksTizen::create(m_taskController.get(), callbacks, mode), fileSystemPath));
}

void AsyncFileSystemTizen::setTaskController(PassRefPtr<AsyncFileSystemTaskControllerTizen> taskController)
{
    m_taskController = taskController;
}

String AsyncFileSystemTizen::virtualPathToFileSystemPath(const KURL& virtualPath)
{
    if (virtualPath.path().isEmpty())
        return String();

    KURL innerURL(ParsedURLString, virtualPath.path());
    String typeString = innerURL.path().substring(1);
    if (typeString != DOMFileSystemBase::temporaryPathPrefix && !typeString.startsWith(DOMFileSystemBase::temporaryPathPrefix, false)
        && typeString != DOMFileSystemBase::persistentPathPrefix && !typeString.startsWith(DOMFileSystemBase::persistentPathPrefix, false))
        return String();

    StringBuilder fileSystemPath;
    fileSystemPath.append(LocalFileSystem::localFileSystem().fileSystemBasePath());
    RefPtr<SecurityOrigin> origin = SecurityOrigin::create(innerURL);
    if (!LocalFileSystem::localFileSystem().fileSystemBasePath().endsWith("/") && !origin->databaseIdentifier().startsWith("/"))
        fileSystemPath.append('/');
    fileSystemPath.append(origin->databaseIdentifier());
    fileSystemPath.append(innerURL.path());

    return fileSystemPath.toString();
}

bool AsyncFileSystemTizen::checkQuota(ScriptExecutionContext* context, const String& sourcePath, int& errorCode, Blob* data)
{
    if (!sourcePath.startsWith(LocalFileSystem::localFileSystem().fileSystemBasePath())) {
        errorCode = FileError::NOT_FOUND_ERR;
        return false;
    }

    String subPath = sourcePath.substring(LocalFileSystem::localFileSystem().fileSystemBasePath().length());
    Vector<String> components;
    subPath.split('/', components);
    size_t componentsSize = components.size();
    if (!componentsSize) {
        errorCode = FileError::NOT_FOUND_ERR;
        return false;
    }

    StringBuilder fileSystemPath;
    fileSystemPath.append(LocalFileSystem::localFileSystem().fileSystemBasePath());
    if (subPath.startsWith('/'))
        fileSystemPath.append('/');
    fileSystemPath.append(components[0]);
    String originRootPath = String(fileSystemRepresentation(fileSystemPath.toString()).data());

    int64_t usage = 0;
    if (!getDirectorySize(originRootPath, usage)) {
        errorCode = FileError::NOT_READABLE_ERR;
        return false;
    }

    if (data) {
        RefPtr<BlobStorageData> blobStorage = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobDataFromURL(data->url());
        if (blobStorage) {
            for (size_t i = 0; i < blobStorage->items().size(); ++i)
                usage += blobStorage->items()[i].data->length();
        }
    }

    if (context->isDocument()) {
        Document* document = static_cast<Document*>(context);
        int64_t defaultQuota = document->page()->group().groupSettings()->localFileSystemQuotaBytes();
        int64_t maxQuota = 0x80000000; // 2GB
        if (usage > maxQuota) {
            errorCode = FileError::QUOTA_EXCEEDED_ERR;
            return false;
        }

        if (usage >= defaultQuota && defaultQuota != maxQuota) {
            if (document->page()->chrome()->client()->exceededLocalFileSystemQuota(document->frame(), usage))
                document->page()->group().groupSettings()->setLocalFileSystemQuotaBytes(maxQuota);
            else {
                errorCode = FileError::QUOTA_EXCEEDED_ERR;
                return false;
            }
        }
    } else {
        WorkerContext* workerContext = static_cast<WorkerContext*>(context);
        if (usage >= workerContext->groupSettings()->localFileSystemQuotaBytes()) {
            errorCode = FileError::QUOTA_EXCEEDED_ERR;
            return false;
        }
    }

    return true;
}

} // namespace WebCore

#endif// ENABLE(FILE_SYSTEM)
