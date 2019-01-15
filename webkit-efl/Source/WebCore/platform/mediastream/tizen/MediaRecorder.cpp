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
#include "MediaRecorder.h"

#if ENABLE(TIZEN_MEDIA_STREAM)

#include "MediaRecorderPrivate.h"
#include "MediaStreamDescriptor.h"

#if PLATFORM(EFL)
#if ENABLE(TIZEN_WEBKIT2)
#include "MediaRecorderPrivateImpl.h"
#else
#include "MediaRecorderPrivateDeprecated.h"
#endif
#endif

namespace WebCore {

static PassOwnPtr<MediaRecorderPrivateInterface> createMediaRecorderPrivate(MediaRecorder* recorder)
{
#if PLATFORM(EFL)
    return MediaRecorderPrivate::create(recorder);
#else
    return createNullMeidaRecorderPrivate(recorder);
#endif
}

MediaRecorder::MediaRecorder(MediaRecorderClient* client)
    : m_client(client)
    , m_private(createMediaRecorderPrivate(this))
{
}

MediaRecorder::~MediaRecorder()
{
    m_client = 0;
}

void MediaRecorder::record(MediaStreamDescriptor* descriptor, const String& prefix)
{
    m_private->record(descriptor, prefix);
}

void MediaRecorder::save()
{
    String filename;
    m_private->save(filename);

    onRecorded(filename);
}

void MediaRecorder::cancel()
{
    m_private->cancel();
}

void MediaRecorder::onRecorded(PassOwnPtr<Vector<char> > data)
{
    if (m_client)
        m_client->onRecorded(data);
}

void MediaRecorder::onRecorded(const String& path)
{
    if (m_client)
        m_client->onRecorded(path);
}

} // namespace WebCore

#endif // ENABLE(TIZEN_MEDIA_STREAM)
