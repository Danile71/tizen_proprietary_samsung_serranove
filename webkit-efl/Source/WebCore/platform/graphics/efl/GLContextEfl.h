/*
    Copyright (C) 2011 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef GLContextEfl_h
#define GLContextEfl_h

#if ENABLE(TIZEN_ACCELERATED_COMPOSITING)

#include "IntSize.h"

#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

typedef struct _Eo_Opaque Eo;
typedef Eo Evas_Object;

class GLContextEfl : public RefCounted<GLContextEfl> {
public:
    GLContextEfl() {}
    virtual ~GLContextEfl() {}

    virtual void Destroy() = 0;

    virtual bool MakeCurrent() = 0;

    virtual bool IsCurrent() = 0;

    virtual void* GetContext() = 0;

    virtual void* GetDisplay() = 0;

    virtual void* GetSurface() = 0;

    virtual void SwapBuffers() = 0;

    virtual void waitClient() = 0;

    virtual bool modifySurface(Evas_Object*) = 0;

    static bool InitializeOneOff();

    static void HandleEGLError(const char* name);

    static PassOwnPtr<GLContextEfl> create(GLContextEfl* sharedContext = 0);
    static PassOwnPtr<GLContextEfl> CreateGLContext(Evas_Object*, GLContextEfl* sharedContext, bool bRenderDirectlyToEvas);
    static PassRefPtr<GLContextEfl> GetEvasContext();
};

#endif

#endif // GLContextEfl_h
