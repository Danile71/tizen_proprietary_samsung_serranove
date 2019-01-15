/*
    Copyright (C) 2012 Samsung Electronics.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "URIUtils.h"

#if ENABLE(TIZEN_MEDIA_STREAM)
#include "MediaStream.h"
#include "MediaStreamRegistry.h"
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(TIZEN_FILE_SYSTEM)
#include "AsyncFileSystemTizen.h"
#include "Blob.h"
#include "BlobRegistryImpl.h"
#endif // ENABLE(TIZEN_FILE_SYSTEM)

namespace WebCore {

static void crackBlobURI(const String& url)
{
#if ENABLE(TIZEN_MEDIA_STREAM)
    MediaStreamDescriptor* descriptor = MediaStreamRegistry::registry().lookupMediaStreamDescriptor(url);
    if (descriptor) {
        for (unsigned int i = 0; i < descriptor->numberOfVideoComponents(); i++) {
            MediaStreamSource* source = descriptor->videoComponent(i)->source();
            if (source) {
                int cameraId = (source->name() == "Self camera") ? 1 : 0;
                String& mutableUrl = const_cast<String&>(url);
                String cameraUrl = String::format("camera://%d", cameraId);
                mutableUrl.swap(cameraUrl);
                return;
            }
        }
    }
#endif // ENABLE(TIZEN_MEDIA_STREAM)

#if ENABLE(TIZEN_FILE_SYSTEM)
    const KURL blobURL(ParsedURLString, url.utf8().data());
    RefPtr<BlobStorageData> blobStorage = static_cast<BlobRegistryImpl&>(blobRegistry()).getBlobDataFromURL(blobURL);
    if (blobStorage) {
        String filePath;
        for (unsigned int i = 0; i < blobStorage->items().size(); i++) {
            const BlobDataItem& blobItem = blobStorage->items()[i];
            if (blobItem.type == BlobDataItem::File) {
                String& mutableUrl = const_cast<String&>(url);
                String fileUrl = "file://" + blobItem.path;
                mutableUrl.swap(fileUrl);
            }
        }
    }
#endif // ENABLE(TIZEN_FILE_SYSTEM)
}

static void crackFileSystemURI(const String& url)
{
#if ENABLE(TIZEN_FILE_SYSTEM)
    const KURL fileSystemURL(ParsedURLString, url.utf8().data());
    String fileUrl = "file://" + AsyncFileSystemTizen::virtualPathToFileSystemPath(fileSystemURL);
    String& mutableUrl = const_cast<String&>(url);
    mutableUrl.swap(fileUrl);
#endif // ENABLE(TIZEN_FILE_SYSTEM)
}

void crackURI(const String& url)
{
    if (url.contains("blob:"))
        crackBlobURI(url);
    else if (url.contains("filesystem:"))
        crackFileSystemURI(url);
}

} // namespace WebCore
