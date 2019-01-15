/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Brent Fulgham <bfulgham@gmail.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef GraphicsContextPlatformPrivateCairo_h
#define GraphicsContextPlatformPrivateCairo_h

#include "GraphicsContext.h"

#include "PlatformContextCairo.h"
#include "RefPtrCairo.h"
#include <cairo.h>
#include <math.h>
#include <stdio.h>
#include <wtf/MathExtras.h>

#if PLATFORM(GTK)
#include <pango/pango.h>
typedef struct _GdkExposeEvent GdkExposeEvent;
#elif PLATFORM(WIN)
#include <cairo-win32.h>
#endif

namespace WebCore {

class GraphicsContextPlatformPrivate {
public:
    GraphicsContextPlatformPrivate(PlatformContextCairo* newPlatformContext)
#if ENABLE(TIZEN_GRAPHICSCONTEXT_COLLECT_REGION)
        : m_isCollectRegion(false)
        , m_collectedRegion(NULL)
        , platformContext(newPlatformContext)
#else
        : platformContext(newPlatformContext)
#endif
#if PLATFORM(GTK)
        , expose(0)
#endif
#if PLATFORM(WIN) || (PLATFORM(GTK) && OS(WINDOWS))
        // NOTE:  These may note be needed: review and remove once Cairo implementation is complete
        , m_hdc(0)
        , m_shouldIncludeChildWindows(false)
#endif
    {
    }

    virtual ~GraphicsContextPlatformPrivate()
    {
    }

#if PLATFORM(WIN)
    // On Windows, we need to update the HDC for form controls to draw in the right place.
    void save();
    void restore();
    void flush();
    void clip(const FloatRect&);
    void clip(const Path&);
    void scale(const FloatSize&);
    void rotate(float);
    void translate(float, float);
    void concatCTM(const AffineTransform&);
    void setCTM(const AffineTransform&);
    void syncContext(cairo_t* cr);
#else
    // On everything else, we do nothing.
    void save() {}
    void restore() {}
    void flush() {}
    void clip(const FloatRect& rect) {
#if ENABLE(TIZEN_GRAPHICSCONTEXT_COLLECT_REGION)
       if(m_isCollectRegion)
           collectRegion(rect);
#endif
    }
    void clip(const Path&) {}
    void scale(const FloatSize&) {}
    void rotate(float) {}
    void translate(float, float) {}
    void concatCTM(const AffineTransform&) {}
    void setCTM(const AffineTransform&) {}
    void beginTransparencyLayer() {
#if ENABLE(TIZEN_GRAPHICSCONTEXT_COLLECT_REGION)
       beginCollectRegion();
#endif
    }
    void endTransparencyLayer() {
#if ENABLE(TIZEN_GRAPHICSCONTEXT_COLLECT_REGION)
       endCollectRegion();
#endif
    }
    void syncContext(cairo_t* cr) {}
#endif
#if ENABLE(TIZEN_GRAPHICSCONTEXT_COLLECT_REGION)
    void beginCollectRegion(){
        if(m_collectedRegion!=NULL)
            cairo_region_destroy(m_collectedRegion);
        m_collectedRegion = cairo_region_create();
        m_isCollectRegion=true;
    }
    void endCollectRegion(){
        if(m_collectedRegion!=NULL)
            cairo_region_destroy(m_collectedRegion);
	m_collectedRegion=NULL;
	m_isCollectRegion=false;
    }
    void clipWithCollectedRegion(cairo_t * cairo){
        if(m_isCollectRegion==false || m_collectedRegion==NULL)
	    return;
	cairo_rectangle_int_t extents;
	cairo_region_get_extents (m_collectedRegion,&extents);
	if( extents.width==0 || extents.height==0)
	    return;
        cairo_rectangle(cairo,extents.x, extents.y, extents.width, extents.height);
        cairo_fill_rule_t savedFillRule = cairo_get_fill_rule(cairo);
        cairo_set_fill_rule(cairo, CAIRO_FILL_RULE_WINDING);
        cairo_clip(cairo);
        cairo_set_fill_rule(cairo, savedFillRule);
    }
    void collectRegion(const FloatRect& rect){
        if(m_isCollectRegion==false||m_collectedRegion==NULL)
	    return;
        cairo_rectangle_int_t rectangle = {(int)rect.x(),(int) rect.y(),(int) rect.width(),(int) rect.height()};
        cairo_region_union_rectangle (m_collectedRegion,&rectangle);
    }
    void setCollectRegion(bool val)
    {
        m_isCollectRegion=val;
    }
    bool m_isCollectRegion;
    cairo_region_t *m_collectedRegion;
#endif
    cairo_t* cr;
    PlatformContextCairo* platformContext;
    Vector<float> layers;

#if PLATFORM(GTK)
    GdkEventExpose* expose;
#endif
#if PLATFORM(WIN) || (PLATFORM(GTK) && OS(WINDOWS))
    HDC m_hdc;
    bool m_shouldIncludeChildWindows;
#endif
};

// This is a specialized private section for the Cairo GraphicsContext, which knows how
// to clean up the heap allocated PlatformContextCairo that we must use for the top-level
// GraphicsContext.
class GraphicsContextPlatformPrivateToplevel : public GraphicsContextPlatformPrivate {
public:
    GraphicsContextPlatformPrivateToplevel(PlatformContextCairo* platformContext)
        : GraphicsContextPlatformPrivate(platformContext)
    {
    }

    virtual ~GraphicsContextPlatformPrivateToplevel()
    {
        delete platformContext;
    }
};


} // namespace WebCore

#endif // GraphicsContextPlatformPrivateCairo_h
