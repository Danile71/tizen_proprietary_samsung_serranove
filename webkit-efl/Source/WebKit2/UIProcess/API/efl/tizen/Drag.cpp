/*
 * Copyright (C) 2012 Samsung Electronics
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(TIZEN_DRAG_SUPPORT)
#include "Drag.h"

#include "EwkViewImpl.h"
#include "ewk_view.h"
#include <WebCore/DragActions.h>
#include <WebCore/DragData.h>
#include <WebCore/DragSession.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

Drag::Drag(EwkViewImpl* viewImpl)
      : m_viewImpl(viewImpl)
      , m_isDragMode(false)
      , m_dragStorageName("Drag")
      , m_dragData(0)
{
    ASSERT(viewImpl);

    m_Handle = new DragHandle(viewImpl->view(), EDJE_DIR"/Drag.edj", "drag_support", this);
}

Drag::~Drag()
{
    delete m_Handle;
}

WebFrameProxy* Drag::focusedOrMainFrame()
{
    WebFrameProxy* frame = m_viewImpl->page()->focusedFrame();
    if(!frame)
        frame = m_viewImpl->page()->mainFrame();

    return frame;
}


void Drag::updateDragPosition()
{
    m_Handle->updateDragPosition();
}

void Drag::setDragMode(bool isDragMode)
{
    if (!isDragMode)
        hide();

    m_isDragMode = isDragMode;
}

void Drag::setDragData(WebCore::DragData* dragdata, PassRefPtr<ShareableBitmap> dragImage)
{
    m_dragData = dragdata;
    m_dragImage = dragImage;
}

void Drag::hide()
{
    IntPoint viewPoint = m_viewImpl->transformFromScene().mapPoint(m_Handle->position());

    DragData* dragData = new DragData(m_dragData->platformData(), viewPoint,
        viewPoint, m_dragData->draggingSourceOperationMask(), m_dragData->flags());

    m_viewImpl->page()->dragExited(dragData ,m_dragStorageName);
    m_Handle->hide();
    m_viewImpl->page()->dragEnded(viewPoint, viewPoint, m_dragData->draggingSourceOperationMask());
}

void Drag::Show()
{
    m_Handle->show();
    m_Handle->move(m_savePoint);
}

// handle callbacks
void Drag::handleMouseDown(DragHandle* handle)
{
    m_Handle->move(handle->position());
}

void Drag::handleMouseMove(DragHandle* handle)
{
    IntPoint viewPoint = m_viewImpl->transformFromScene().mapPoint(handle->position());

    DragData* dragData = new DragData(m_dragData->platformData(), viewPoint,
        viewPoint, m_dragData->draggingSourceOperationMask(), m_dragData->flags());

    m_viewImpl->page()->dragUpdated(dragData ,m_dragStorageName);

    m_Handle->move(handle->position());
    delete(dragData);
}

void Drag::handleMouseUp(DragHandle* handle)
{
    IntPoint viewPoint = m_viewImpl->transformFromScene().mapPoint(handle->position());

    DragData* dragData = new DragData(m_dragData->platformData(), viewPoint,
        viewPoint, m_dragData->draggingSourceOperationMask(), m_dragData->flags());

    m_viewImpl->page()->dragUpdated(dragData ,m_dragStorageName);

    DragSession dragSession = m_viewImpl->page()->dragSession();
    if (dragSession.operation != DragOperationNone) {
        m_viewImpl->page()->performDrag(dragData, m_dragStorageName);
        setDragMode(false);
        m_viewImpl->page()->dragEnded(viewPoint, viewPoint, m_dragData->draggingSourceOperationMask());
    }
    delete(dragData);
}
} // namespace WebKit

#endif // TIZEN_DRAG_SUPPORT
