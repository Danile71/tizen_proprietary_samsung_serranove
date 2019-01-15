/*
 * Copyright (C) 2013 Samsung Electronics
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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
#include "FileSystem.h"

#if ENABLE(TIZEN_FILE_SYSTEM)
#include "FileMetadata.h"
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <wtf/text/CString.h>

namespace WebCore {

bool getDirectorySize(const String& path, long long& size)
{
    if(!fileExists(path))
        return false;

    DIR* directory = opendir(fileSystemRepresentation(path).data());
    if(!directory)
        return false;

    struct dirent* directoryEntry;
    struct stat buf;

    while (directoryEntry = readdir(directory)) {
        if (!strcmp(directoryEntry->d_name, ".") || !strcmp(directoryEntry->d_name, ".."))
            continue;

        String absolutePath = pathByAppendingComponent(path, String(directoryEntry->d_name));

        if (lstat(absolutePath.utf8().data(), &buf) == -1) {
            closedir(directory);
            return false;
        }

        if (S_ISDIR(buf.st_mode)) {
            if (!getDirectorySize(absolutePath, size)) {
                closedir(directory);
                return false;
            }
        } else
            size += buf.st_size;
    }

    closedir(directory);
    return true;
}

bool getFileMetadata(const String& path, FileMetadata& metadata)
{
    CString fsRep = fileSystemRepresentation(path);

    if (!fsRep.data() || fsRep.data()[0] == '\0')
        return false;

    struct stat fileInfo;
    if (stat(fsRep.data(), &fileInfo))
        return false;

    metadata.modificationTime = fileInfo.st_mtime;
    metadata.length = fileInfo.st_size;
    metadata.type = S_ISDIR(fileInfo.st_mode) ? FileMetadata::TypeDirectory : FileMetadata::TypeFile;
    metadata.platformPath = path;
    return true;
}

bool linkFile(const String& sourcePath, const String& destinationPath)
{
    CString fsRepSource = fileSystemRepresentation(sourcePath);
    CString fsRepDestination = fileSystemRepresentation(destinationPath);

    if (!fsRepSource.data() || fsRepSource.data()[0] == '\0'
        || !fsRepDestination.data() || fsRepDestination.data()[0] == '\0')
        return false;

    return !link(fsRepSource.data(), fsRepDestination.data());
}

PlatformFileHandle openFile(const String& path, FileOpenMode mode)
{
    CString fsRep = fileSystemRepresentation(path);

    if (fsRep.isNull())
        return invalidPlatformFileHandle;

    int platformFlag = 0;
    if (mode == OpenForRead)
        platformFlag |= O_RDONLY;
    else if (mode == OpenForWrite)
        platformFlag |= (O_WRONLY | O_CREAT | O_TRUNC);
    else if (mode == OpenForWriteOnly)
        platformFlag |= (O_WRONLY);
    return open(fsRep.data(), platformFlag, 0666);
}

bool renameFile(const String& sourcePath, const String& destinationPath)
{
    CString fsRepSource = fileSystemRepresentation(sourcePath);
    CString fsRepDestination = fileSystemRepresentation(destinationPath);

    if (!fsRepSource.data() || fsRepSource.data()[0] == '\0'
        || !fsRepDestination.data() || fsRepDestination.data()[0] == '\0')
        return false;

    return !rename(fsRepSource.data(), fsRepDestination.data());
}

bool haveEnoughSpace(const String& path, unsigned requestSize)
{
    struct statvfs buf;
    if (!statvfs(path.utf8().data(), &buf)) {
        unsigned long availableSize = buf.f_bavail * buf.f_bsize;
        if (availableSize < requestSize)
            return false;
    }
    return true;
}
}
#endif
