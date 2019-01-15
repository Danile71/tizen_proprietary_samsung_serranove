/*
 * Copyright (C) 2013 Samsung Electronic.
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
#include "LinkMagnifierProxy.h"

#if ENABLE(TIZEN_LINK_MAGNIFIER)

#include "ewk_util.h"

#include <cairo.h>

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
#include <dlfcn.h>
#include <efl_assist.h>
extern void* EflAssistHandle;
#endif

using namespace WebCore;

namespace WebKit {

LinkMagnifierProxy& LinkMagnifierProxy::linkMagnifier()
{
    DEFINE_STATIC_LOCAL(LinkMagnifierProxy, linkMagnifier, ());
    return linkMagnifier;
}

LinkMagnifierProxy::LinkMagnifierProxy()
    : m_page(0)
    , m_image(0)
    , m_scaleFactor(1)
    , m_updateTimer(this, &LinkMagnifierProxy::updateTimerFired)
    , m_magnifier(0)
    , m_magnifierbg(0)
    , m_isClickedExactly(false)
{
}

LinkMagnifierProxy::~LinkMagnifierProxy()
{
    clear();
}

void LinkMagnifierProxy::transitionFinishedCallback(void* data, Evas_Object* obj, const char* emission, const char* source)
{
    LinkMagnifierProxy& linkMagnifier(LinkMagnifierProxy::linkMagnifier());

    linkMagnifier.clear();
}

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
void LinkMagnifierProxy::linkMagnifierHwMoreBackKeyCallback(void* data, Evas_Object* obj, void* eventInfo)
{
    LinkMagnifierProxy& linkMagnifier(LinkMagnifierProxy::linkMagnifier());

    linkMagnifier.endMagnifierAnimation();
}
#endif

void LinkMagnifierProxy::show(WebPageProxy* page, const IntPoint& position, const IntRect& rect)
{
    if (!page->isValid())
        return;

    Evas_Object* viewWidget = page->viewWidget();

    if (!evas_object_focus_get(viewWidget) || !page->isViewVisible())
        return;

    Evas_Object* topWidget = elm_object_top_widget_get(elm_object_parent_widget_get(viewWidget));
    if (!topWidget)
        return;

    if (isVisible())
        return;

    m_magnifierbg = elm_bg_add(topWidget);
    elm_layout_file_set(m_magnifierbg, EDJE_DIR"/LinkMagnifier.edj", "link_magnifier_bg");

    m_magnifier = elm_layout_add(topWidget);
    elm_layout_file_set(m_magnifier, EDJE_DIR"/LinkMagnifier.edj", "link_magnifier");

    elm_object_tree_focus_allow_set(m_magnifier, false);
    elm_object_tree_focus_allow_set(m_magnifierbg, false);

    if (m_image)
        evas_object_del(m_image);

    m_page = page;

    EwkViewImpl* viewImpl = ewkViewGetPageClient(viewWidget)->viewImpl();

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    AffineTransform transform(viewImpl->transformToView());
#else
    AffineTransform transform;
#endif
    IntPoint tapPosition(transform.mapPoint(position));

    m_pageRect = transform.mapRect(rect);

    const float linkMagnifierMinWidth = WebCore::getDefaultScreenResolution().width() / 12;
    const float linkMagnifierMinHeight = WebCore::getDefaultScreenResolution().height() / 40;

    if (m_pageRect.width() < linkMagnifierMinWidth) {
        m_pageRect.setX(m_pageRect.x() - (linkMagnifierMinWidth - m_pageRect.width())/2);
        m_pageRect.setWidth(linkMagnifierMinWidth);
    }

    if (m_pageRect.height() < linkMagnifierMinHeight) {
        m_pageRect.setY(m_pageRect.y() - (linkMagnifierMinHeight - m_pageRect.height())/2);
        m_pageRect.setHeight(linkMagnifierMinHeight);
    }

    const float marginScaleFactor = .1;
    m_pageRect.inflateX(m_pageRect.width() * marginScaleFactor);
    m_pageRect.inflateY(m_pageRect.height() * marginScaleFactor);

    adjustRectByContentSize();

    const float targetScaleFactor = 2;
    m_scaleFactor = targetScaleFactor / viewImpl->pageClient->scaleFactor();

    m_image = evas_object_image_add(evas_object_evas_get(viewWidget));
    if (!m_image)
        return;

    evas_object_image_colorspace_set(m_image, EVAS_COLORSPACE_ARGB8888);
    evas_object_image_alpha_set(m_image, true);
    evas_object_image_filled_set(m_image, false);

    int imageWidth = static_cast<int>(m_pageRect.width() * m_scaleFactor);
    int imageHeight = static_cast<int>(m_pageRect.height() * m_scaleFactor);

    int parentX, parentY, parentWidth, parentHeight;
    evas_object_geometry_get(topWidget, &parentX, &parentY, &parentWidth, &parentHeight);

    setImageSize(imageWidth, imageHeight, parentWidth * 0.9, parentHeight * 0.45, tapPosition);

    int viewX, viewY;
    evas_object_geometry_get(viewImpl->view(), &viewX, &viewY, 0, 0);

    Eina_List* list = evas_object_smart_members_get(m_magnifier);
    Eina_List* iter;
    void* data;

    EINA_LIST_FOREACH(list, iter, data) {
        int widgetWidth = 0, widgetHeight = 0;
        const char* group;

        edje_object_file_get(static_cast<Evas_Object*>(data), 0, &group);
        if (group && !strncmp(group, "link_magnifier", 14))
            evas_object_geometry_get(static_cast<Evas_Object*>(data), 0, 0, &widgetWidth, &widgetHeight);

        if (widgetWidth > 0 && widgetHeight > 0 && (widgetWidth < imageWidth || widgetHeight < imageHeight)) {
            setImageSize(imageWidth, imageHeight, widgetWidth, widgetHeight, tapPosition);
            break;
        }
    }
    eina_list_free(list);

    updateImage();
    evas_object_image_fill_set(m_image, 0, 0, imageWidth, imageHeight);

    elm_object_part_content_set(m_magnifier, "elm.swallow.content", m_image);
    Evas_Object* clipRect = evas_object_rectangle_add(evas_object_evas_get(m_magnifier));
    evas_object_size_hint_min_set(clipRect, imageWidth, imageHeight);
    evas_object_size_hint_max_set(clipRect, imageWidth, imageHeight);
    evas_object_resize(clipRect, imageWidth, imageHeight);
    evas_object_clip_set(m_image, clipRect);
    evas_object_del(clipRect);

    evas_object_layer_set(m_magnifier, EVAS_LAYER_MAX);
    evas_object_layer_set(m_image, EVAS_LAYER_MAX);

    const int padding = 20;
    int x, y, w, h;
    evas_object_geometry_get(m_image, &x, &y, &w, &h);

    IntRect imageRect(x, y, w, h);
    IntRect parentRect(parentX, parentY, parentWidth, parentHeight);
    int movePointX = tapPosition.x() + viewX - (imageWidth / 2);
    int movePointY = tapPosition.y() + viewY - (imageHeight / 2);

    //for the left
    if (movePointX < parentRect.x())
        movePointX = parentRect.x() + padding;
    //for the top
    if (movePointY < parentRect.y())
        movePointY = parentRect.y() + padding;
    // for the right
    if (movePointX + imageRect.width() > parentRect.maxX())
        movePointX = parentRect.maxX() - imageRect.width() - padding;
    // for the bottom
    if (movePointY + imageRect.height() > parentRect.maxY())
        movePointY = parentRect.maxY() - imageRect.height() - padding;

    evas_object_move(m_magnifier, movePointX, movePointY);
    evas_object_resize(m_magnifier, imageRect.width(), imageRect.height());
    evas_object_move(m_magnifierbg, 0, 0);
    evas_object_resize(m_magnifierbg, parentWidth, parentHeight);

    // Make background color transparent
    evas_object_color_set(m_magnifierbg, 255, 255, 255, 0);
    evas_object_show(m_magnifierbg);
    evas_object_show(m_image);
    evas_object_show(m_magnifier);

    elm_layout_signal_emit(m_magnifierbg, "elm,state,show", "elm");

    startMagnifierAnimation();

#if ENABLE(TIZEN_HW_MORE_BACK_KEY)
    if (EflAssistHandle) {
        void (*webkit_ea_object_event_callback_add)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *);
        webkit_ea_object_event_callback_add = (void (*)(Evas_Object *, Ea_Callback_Type , Ea_Event_Cb func, void *))dlsym(EflAssistHandle, "ea_object_event_callback_add");
        (*webkit_ea_object_event_callback_add)(m_magnifier, EA_CALLBACK_BACK, linkMagnifierHwMoreBackKeyCallback, 0);
        (*webkit_ea_object_event_callback_add)(m_magnifier, EA_CALLBACK_MORE, linkMagnifierHwMoreBackKeyCallback, 0);
    }
#endif

    evas_object_propagate_events_set(m_magnifier, false);
    evas_object_event_callback_add(m_image, EVAS_CALLBACK_RESIZE, imageSizeChanged, 0);
    evas_object_event_callback_add(m_image, EVAS_CALLBACK_MOUSE_UP, mouseUp, 0);
    elm_object_signal_callback_add(m_magnifier, "animation,end", "elm", transitionFinishedCallback, 0);

    evas_object_propagate_events_set(m_magnifierbg, false);
    evas_object_event_callback_add(m_magnifierbg, EVAS_CALLBACK_MOUSE_UP, bgMouseUp, this);

    m_selectedPosition = IntPoint(-1, -1);
}

void LinkMagnifierProxy::hide()
{
    if (!isVisible())
        return;

    endMagnifierAnimation();
    m_pageRect = IntRect();
    m_updateTimer.stop();
}

void LinkMagnifierProxy::adjustRectByContentSize()
{
    if (m_pageRect.x() < 0)
        m_pageRect.shiftXEdgeTo(0);
    if (m_pageRect.y() < 0)
        m_pageRect.shiftYEdgeTo(0);

    IntSize contentsSize = m_page->contentsSize();
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    contentsSize.scale(ewkViewGetPageClient(m_page->viewWidget())->scaleFactor());
#endif

    if (m_pageRect.maxX() > contentsSize.width())
        m_pageRect.shiftMaxXEdgeTo(contentsSize.width());
    if (m_pageRect.maxY() > contentsSize.height())
        m_pageRect.shiftMaxYEdgeTo(contentsSize.height());
}

void LinkMagnifierProxy::setImageSize(int& width, int& height, int limitWidth, int limitHeight, const IntPoint& tapPosition)
{
    if (limitWidth < width) {
        int halfWidth = static_cast<int>((limitWidth / m_scaleFactor) / 2);
        bool overflowLeft = (tapPosition.x() - m_pageRect.x()) > halfWidth;
        bool overflowRight = (m_pageRect.maxX() - tapPosition.x()) > halfWidth;

        if (overflowLeft) {
            if (overflowRight)
                m_pageRect.setX(tapPosition.x() - halfWidth);
            else
                m_pageRect.setX(m_pageRect.maxX() - (halfWidth * 2));
        }
        m_pageRect.setWidth(halfWidth * 2);
        width = static_cast<int>(m_pageRect.width() * m_scaleFactor);
    }

    if (limitHeight < height) {
        int halfHeight = static_cast<int>((limitHeight / m_scaleFactor) / 2);
        bool overflowUp = (tapPosition.y() - m_pageRect.y()) > halfHeight;
        bool overflowDown = (m_pageRect.maxY() - tapPosition.y()) > halfHeight;

        if (overflowUp) {
            if (overflowDown)
                m_pageRect.setY(tapPosition.y() - halfHeight);
            else
                m_pageRect.setY(m_pageRect.maxY() - (halfHeight * 2));
        }
        m_pageRect.setHeight(halfHeight * 2);
        height = static_cast<int>(m_pageRect.height() * m_scaleFactor);
    }

    evas_object_image_size_set(m_image, width, height);
    evas_object_resize(m_image, width, height);
    evas_object_size_hint_min_set(m_image, width, height);

    evas_object_size_hint_min_set(m_magnifier, width, height);
}

void LinkMagnifierProxy::updateImage()
{
    if (!m_image)
        return;

    unsigned char* imageData = static_cast<unsigned char*>(evas_object_image_data_get(m_image, true));
    if (!imageData)
        return;

    RefPtr<WebImage> webImage = m_page->createSnapshot(m_pageRect, m_scaleFactor);
    if (!webImage || !webImage->bitmap())
        return;

    RefPtr<cairo_surface_t> pageSurface = webImage->bitmap()->createCairoSurface();
    if (!pageSurface)
        return;

    int imageWidth, imageHeight;
    evas_object_image_size_get(m_image, &imageWidth, &imageHeight);

    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, imageWidth, imageHeight));
    RefPtr<cairo_t> context = adoptRef(cairo_create(surface.get()));

    const double radius = 10.0;
    double degrees = M_PI / 180.0;

    Path path;
    path.addRoundedRect(IntRect(0, 0, imageWidth, imageHeight), FloatSize(radius, radius));
    appendWebCorePathToCairoContext(context.get(), path);

    cairo_clip(context.get());
    cairo_set_source_surface(context.get(), pageSurface.get(), 0, 0);
    cairo_paint(context.get());

    cairo_new_sub_path(context.get());
    cairo_arc(context.get(), imageWidth - radius, radius, radius, -90 * degrees, 0 * degrees);
    cairo_arc(context.get(), imageWidth - radius, imageHeight - radius, radius, 0 * degrees, 90 * degrees);
    cairo_arc(context.get(), radius, imageHeight - radius, radius, 90 * degrees, 180 * degrees);
    cairo_arc(context.get(), radius, radius, radius, 180 * degrees, 270 * degrees);
    cairo_close_path(context.get());

    float red, green, blue, alpha;

    if (EflAssistHandle) {
                Eina_Bool (*webkit_ea_theme_color_get)(const char *code, int *r, int *g, int *b, int *a, int *r2, int *g2, int *b2, int *a2, int *r3, int *g3, int *b3, int *a3);
                webkit_ea_theme_color_get = (Eina_Bool (*)(const char *code, int *r, int *g, int *b, int *a, int *r2, int *g2, int *b2, int *a2, int *r3, int *g3, int *b3, int *a3))dlsym(EflAssistHandle, "ea_theme_color_get");

        int r, g, b, a;
        (*webkit_ea_theme_color_get)("W161", &r, &g, &b, &a, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        Color linkMagnifierBorderColor(r, g, b, a);
        linkMagnifierBorderColor.getRGBA(red, green, blue, alpha);
    }

    cairo_set_source_rgba(context.get(), red, green, blue, alpha);
    cairo_set_line_width(context.get(), 10.0);
    cairo_stroke(context.get());

    RefPtr<cairo_surface_t> imageSurface = adoptRef(cairo_image_surface_create_for_data(imageData, CAIRO_FORMAT_ARGB32, imageWidth, imageHeight, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, imageWidth)));
    RefPtr<cairo_t> imageContext = adoptRef(cairo_create(imageSurface.get()));

    cairo_set_source_surface(imageContext.get(), surface.get(), 0, 0);
    cairo_rectangle(imageContext.get(), 0, 0, imageWidth, imageHeight);
    cairo_fill(imageContext.get());

    evas_object_image_data_set(m_image, imageData);
}

void LinkMagnifierProxy::clear()
{
    if (!m_page)
        return;

    if (m_page->isValid() && m_selectedPosition.x() >= 0 && m_selectedPosition.y() >= 0)
        m_page->openLink(m_selectedPosition);

    m_updateTimer.stop();
    m_page = 0;

    elm_layout_signal_emit(m_magnifierbg, "elm,state,hide", "elm");

    evas_object_event_callback_del(m_image, EVAS_CALLBACK_RESIZE, imageSizeChanged);
    evas_object_event_callback_del(m_image, EVAS_CALLBACK_MOUSE_UP, mouseUp);
    evas_object_event_callback_del(m_magnifierbg, EVAS_CALLBACK_MOUSE_UP, bgMouseUp);
    elm_object_signal_callback_del(m_magnifier, "animation,end", "elm", transitionFinishedCallback);

    elm_object_content_unset(m_magnifier);
    evas_object_hide(m_magnifier);
    evas_object_hide(m_magnifierbg);
    evas_object_del(m_magnifier);
    evas_object_del(m_magnifierbg);
    evas_object_del(m_image);

    m_magnifier = 0;
    m_magnifierbg = 0;
    m_image = 0;
}

void LinkMagnifierProxy::bgMouseUp(void*, Evas*, Evas_Object* image, void* eventInfo)
{
    LinkMagnifierProxy& linkMagnifier(LinkMagnifierProxy::linkMagnifier());
    linkMagnifier.hide();
}

void LinkMagnifierProxy::mouseUp(void*, Evas*, Evas_Object* image, void* eventInfo)
{
    Evas_Event_Mouse_Up* upEvent = static_cast<Evas_Event_Mouse_Up*>(eventInfo);
    LinkMagnifierProxy& linkMagnifier(LinkMagnifierProxy::linkMagnifier());
    if (!linkMagnifier.m_page || !linkMagnifier.m_page->isValid()) {
        linkMagnifier.hide();
        return;
    }

    Evas_Coord_Point imagePosition;
    evas_object_geometry_get(image, &imagePosition.x, &imagePosition.y, 0, 0);

    IntPoint position(linkMagnifier.m_pageRect.x() + (upEvent->canvas.x - imagePosition.x) / linkMagnifier.m_scaleFactor,
                      linkMagnifier.m_pageRect.y() + (upEvent->canvas.y - imagePosition.y) / linkMagnifier.m_scaleFactor);
#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    position = ewkViewGetPageClient(linkMagnifier.m_page->viewWidget())->viewImpl()->transformFromView().mapPoint(position);
#endif

    linkMagnifier.m_selectedPosition = position;

    linkMagnifier.clear();
}

void LinkMagnifierProxy::imageSizeChanged(void*, Evas*, Evas_Object*, void*)
{
    LinkMagnifierProxy& linkMagnifier(LinkMagnifierProxy::linkMagnifier());

    int width, height;
    evas_object_geometry_get(linkMagnifier.m_image, 0, 0, &width, &height);

    int imageWidth, imageHeight;
    evas_object_image_size_get(linkMagnifier.m_image, &imageWidth, &imageHeight);

    if (width != imageWidth) {
        linkMagnifier.m_pageRect.move((imageWidth - width) / linkMagnifier.m_scaleFactor / 2, 0);
        linkMagnifier.m_pageRect.setWidth(width / linkMagnifier.m_scaleFactor);
    }
    if (height != imageHeight) {
        linkMagnifier.m_pageRect.move(0, (imageHeight - height) / linkMagnifier.m_scaleFactor / 2);
        linkMagnifier.m_pageRect.setHeight(height / linkMagnifier.m_scaleFactor);
    }

    linkMagnifier.adjustRectByContentSize();

    evas_object_image_size_set(linkMagnifier.m_image, width, height);
    linkMagnifier.updateImage();

    evas_object_image_fill_set(linkMagnifier.m_image, 0, 0, width, height);
}

void LinkMagnifierProxy::updateTimerFired(Timer<LinkMagnifierProxy>*)
{
    if (!m_page || !m_page->isValid())
        return;

    updateImage();
}

void LinkMagnifierProxy::requestUpdate(const IntRect& dirtyRect)
{
    if (!m_page || !m_page->isValid() || m_updateTimer.isActive() || m_pageRect.isEmpty())
        return;

#if ENABLE(TIZEN_WEBKIT2_TILED_BACKING_STORE)
    if (!m_pageRect.intersects(ewkViewGetPageClient(m_page->viewWidget())->viewImpl()->transformToView().mapRect(dirtyRect)))
        return;
#else
    if (!m_pageRect.intersects(dirtyRect))
        return;
#endif

    m_updateTimer.startOneShot(0);
}

void LinkMagnifierProxy::didChangeBackForwardList(WebPageProxy* page)
{
    if (page != m_page)
        return;

    hide();
}

void LinkMagnifierProxy::startMagnifierAnimation()
{
    elm_layout_signal_emit(m_magnifierbg, "elm,action,_fade_in", "elm");
    elm_layout_signal_emit(m_magnifier, "elm,action,_zoom_in", "elm");
    elm_layout_signal_emit(m_magnifier, "elm,action,_fade_in", "elm");
}

void LinkMagnifierProxy::endMagnifierAnimation()
{
    elm_layout_signal_emit(m_magnifierbg, "elm,action,_fade_out", "elm");
    elm_layout_signal_emit(m_magnifier, "elm,action,_zoom_out", "elm");
    elm_layout_signal_emit(m_magnifier, "elm,action,_fade_out", "elm");
}

bool LinkMagnifierProxy::isVisible()
{
    return evas_object_visible_get(m_magnifier) || evas_object_visible_get(m_magnifierbg);
}

} // namespace WebKit

#endif
