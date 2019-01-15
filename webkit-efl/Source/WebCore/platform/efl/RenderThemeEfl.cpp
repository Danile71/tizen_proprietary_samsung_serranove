/*
 * Copyright (C) 2007 Apple Inc.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Collabora Ltd.
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2011 Samsung Electronics
 * Copyright (c) 2012 Intel Corporation. All rights reserved.
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
#include "RenderThemeEfl.h"

#include "CSSValueKeywords.h"
#include "CairoUtilitiesEfl.h"
#include "GraphicsContext.h"
#include "HTMLInputElement.h"
#include "NotImplemented.h"
#include "Page.h"
#include "PaintInfo.h"
#include "PlatformContextCairo.h"
#include "RenderBox.h"
#include "RenderObject.h"
#include "RenderProgress.h"
#include "RenderSlider.h"
#include "UserAgentStyleSheets.h"

#include <Ecore_Evas.h>
#include <Edje.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if ENABLE(VIDEO)
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "TimeRanges.h"
#endif

#if ENABLE(TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_TEXT_SELECTION)
#include "efl_assist.h"
#endif

#if PLATFORM(TIZEN)
#include "StyleResolver.h"
#endif

namespace WebCore {
#if ENABLE(VIDEO)
using namespace HTMLNames;
#endif

// TODO: change from object count to ecore_evas size (bytes)
// TODO: as objects are webpage/user defined and they can be very large.
#define RENDER_THEME_EFL_PART_CACHE_MAX 32

// Initialize default font size.
#if ENABLE(TIZEN_SYSTEM_FONT)
float RenderThemeEfl::defaultFontSize = 13.0f;
#else
float RenderThemeEfl::defaultFontSize = 16.0f;
#endif

static const float minCancelButtonSize = 5;
static const float maxCancelButtonSize = 21;

// Constants for progress tag animation.
// These values have been copied from RenderThemeGtk.cpp
static const int progressAnimationFrames = 10;
static const double progressAnimationInterval = 0.125;

static const int sliderThumbWidth = 29;
static const int sliderThumbHeight = 11;

#if ENABLE(VIDEO)
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
static const int mediaSliderThumbWidth = 16;
static const int mediaSliderThumbHeight = 16;
static const int mediaSliderPressThumbWidth = 32;
static const int mediaSliderPressThumbHeight = 32;
static bool sliderScrubbing = false;
#else
static const int mediaSliderThumbWidth = 12;
static const int mediaSliderThumbHeight = 12;
static const int mediaSliderHeight = 14;
#endif
#endif

#if PLATFORM(TIZEN)
void RenderThemeEfl::adjustSizeConstraints(RenderStyle* style, FormType type, StyleResolver* styleResolver) const
#else
void RenderThemeEfl::adjustSizeConstraints(RenderStyle* style, FormType type) const
#endif
{
    const struct ThemePartDesc* desc = m_partDescs + (size_t)type;

    if (style->minWidth().isIntrinsic())
        style->setMinWidth(desc->min.width());
    if (style->minHeight().isIntrinsic())
        style->setMinHeight(desc->min.height());

    if (desc->max.width().value() > 0 && style->maxWidth().isIntrinsicOrAuto())
        style->setMaxWidth(desc->max.width());
    if (desc->max.height().value() > 0 && style->maxHeight().isIntrinsicOrAuto())
        style->setMaxHeight(desc->max.height());

#if PLATFORM(TIZEN)
    if (!styleResolver->currentMatchResultHasAuthorStylesForProperty(style, CSSPropertyPaddingTop))
#endif
        style->setPaddingTop(desc->padding.top());
#if PLATFORM(TIZEN)
    if (!styleResolver->currentMatchResultHasAuthorStylesForProperty(style, CSSPropertyPaddingBottom))
#endif
        style->setPaddingBottom(desc->padding.bottom());
#if PLATFORM(TIZEN)
    if (!styleResolver->currentMatchResultHasAuthorStylesForProperty(style, CSSPropertyPaddingLeft))
#endif
        style->setPaddingLeft(desc->padding.left());
#if PLATFORM(TIZEN)
    if (!styleResolver->currentMatchResultHasAuthorStylesForProperty(style, CSSPropertyPaddingRight))
#endif
        style->setPaddingRight(desc->padding.right());
}

bool RenderThemeEfl::themePartCacheEntryReset(struct ThemePartCacheEntry* entry, FormType type)
{
    const char *file, *group;

    ASSERT(entry);
    ASSERT(m_edje);

    edje_object_file_get(m_edje, &file, 0);
    group = edjeGroupFromFormType(type);
    ASSERT(file);
    ASSERT(group);

    if (!edje_object_file_set(entry->o, file, group)) {
        Edje_Load_Error err = edje_object_load_error_get(entry->o);
        const char *errmsg = edje_load_error_str(err);
        EINA_LOG_ERR("Could not load '%s' from theme %s: %s",
                     group, file, errmsg);
        return false;
    }
    return true;
}

bool RenderThemeEfl::themePartCacheEntrySurfaceCreate(struct ThemePartCacheEntry* entry)
{
    int w, h;
    cairo_status_t status;

    ASSERT(entry);
    ASSERT(entry->ee);

    ecore_evas_geometry_get(entry->ee, 0, 0, &w, &h);
    ASSERT(w > 0);
    ASSERT(h > 0);

    entry->surface = cairo_image_surface_create_for_data((unsigned char *)ecore_evas_buffer_pixels_get(entry->ee),
                                                      CAIRO_FORMAT_ARGB32, w, h, w * 4);
    status = cairo_surface_status(entry->surface);
    if (status != CAIRO_STATUS_SUCCESS) {
        EINA_LOG_ERR("Could not create cairo surface: %s",
                     cairo_status_to_string(status));
        return false;
    }

    return true;
}

bool RenderThemeEfl::isFormElementTooLargeToDisplay(const IntSize& elementSize)
{
    // This limit of 20000 pixels is hardcoded inside edje -- anything above this size
    // will be clipped. This value seems to be reasonable enough so that hardcoding it
    // here won't be a problem.
    static const int maxEdjeDimension = 20000;

    return elementSize.width() > maxEdjeDimension || elementSize.height() > maxEdjeDimension;
}

// allocate a new entry and fill it with edje group
struct RenderThemeEfl::ThemePartCacheEntry* RenderThemeEfl::cacheThemePartNew(FormType type, const IntSize& size)
{
    if (isFormElementTooLargeToDisplay(size)) {
        EINA_LOG_ERR("cannot render an element of size %dx%d", size.width(), size.height());
        return 0;
    }

    ThemePartCacheEntry* entry = new ThemePartCacheEntry;
    if (!entry) {
        EINA_LOG_ERR("could not allocate ThemePartCacheEntry.");
        return 0;
    }

    entry->ee = ecore_evas_buffer_new(size.width(), size.height());
    if (!entry->ee) {
        EINA_LOG_ERR("ecore_evas_buffer_new(%d, %d) failed.",
                     size.width(), size.height());
        delete entry;
        return 0;
    }

    // By default EFL creates buffers without alpha.
    ecore_evas_alpha_set(entry->ee, EINA_TRUE);

    entry->o = edje_object_add(ecore_evas_get(entry->ee));
    ASSERT(entry->o);
    if (!themePartCacheEntryReset(entry, type)) {
        evas_object_del(entry->o);
        ecore_evas_free(entry->ee);
        delete entry;
        return 0;
    }

    if (!themePartCacheEntrySurfaceCreate(entry)) {
        evas_object_del(entry->o);
        ecore_evas_free(entry->ee);
        delete entry;
        return 0;
    }

    evas_object_resize(entry->o, size.width(), size.height());
    evas_object_show(entry->o);

    entry->type = type;
    entry->size = size;

    m_partCache.prepend(entry);
    return entry;
}

// just change the edje group and return the same entry
struct RenderThemeEfl::ThemePartCacheEntry* RenderThemeEfl::cacheThemePartReset(FormType type, struct RenderThemeEfl::ThemePartCacheEntry* entry)
{
    if (!themePartCacheEntryReset(entry, type)) {
        entry->type = FormTypeLast; // invalidate
        m_partCache.append(entry);
        return 0;
    }
    entry->type = type;
    m_partCache.prepend(entry);
    return entry;
}

// resize entry and reset it
struct RenderThemeEfl::ThemePartCacheEntry* RenderThemeEfl::cacheThemePartResizeAndReset(FormType type, const IntSize& size, struct RenderThemeEfl::ThemePartCacheEntry* entry)
{
    cairo_surface_finish(entry->surface);

    entry->size = size;
    ecore_evas_resize(entry->ee, size.width(), size.height());
    evas_object_resize(entry->o, size.width(), size.height());

    if (!themePartCacheEntrySurfaceCreate(entry)) {
        evas_object_del(entry->o);
        ecore_evas_free(entry->ee);
        delete entry;
        return 0;
    }

    return cacheThemePartReset(type, entry);
}

// general purpose get (will create, reuse and all)
struct RenderThemeEfl::ThemePartCacheEntry* RenderThemeEfl::cacheThemePartGet(FormType type, const IntSize& size)
{
    Vector<struct ThemePartCacheEntry *>::iterator itr, end;
    struct ThemePartCacheEntry *ce_last_size = 0;
    int i, idxLastSize = -1;

    itr = m_partCache.begin();
    end = m_partCache.end();
    for (i = 0; itr != end; i++, itr++) {
        struct ThemePartCacheEntry *entry = *itr;
        if (entry->size == size) {
            if (entry->type == type)
                return entry;
            ce_last_size = entry;
            idxLastSize = i;
        }
    }

    if (m_partCache.size() < RENDER_THEME_EFL_PART_CACHE_MAX)
        return cacheThemePartNew(type, size);

    if (ce_last_size && ce_last_size != m_partCache.first()) {
        m_partCache.remove(idxLastSize);
        return cacheThemePartReset(type, ce_last_size);
    }

    ThemePartCacheEntry* entry = m_partCache.last();
    m_partCache.removeLast();
    return cacheThemePartResizeAndReset(type, size, entry);
}

void RenderThemeEfl::cacheThemePartFlush()
{
    Vector<struct ThemePartCacheEntry *>::iterator itr, end;

    itr = m_partCache.begin();
    end = m_partCache.end();
    for (; itr != end; itr++) {
        struct ThemePartCacheEntry *entry = *itr;
        cairo_surface_destroy(entry->surface);
        evas_object_del(entry->o);
        ecore_evas_free(entry->ee);
        delete entry;
    }
    m_partCache.clear();
}

void RenderThemeEfl::applyEdjeStateFromForm(Evas_Object* object, ControlStates states)
{
    const char *signals[] = { // keep in sync with WebCore/platform/ThemeTypes.h
        "hovered",
        "pressed",
        "focused",
        "enabled",
        "checked",
        "read-only",
        "default",
        "window-inactive",
        "indeterminate",
        "spinup"
    };

    edje_object_signal_emit(object, "reset", "");

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(signals); ++i) {
        if (states & (1 << i))
            edje_object_signal_emit(object, signals[i], "");
    }
}

bool RenderThemeEfl::paintThemePart(RenderObject* object, FormType type, const PaintInfo& info, const IntRect& rect)
{
    ThemePartCacheEntry* entry;
    Eina_List* updates;
    cairo_t* cairo;

    ASSERT(m_canvas);
    ASSERT(m_edje);

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    cairo_matrix_t matrix;
    cairo = info.context->platformContext()->cr();
    ASSERT(cairo);

    bool isMediaFullScreen = false;
    Node* node = object->node();
    if (node && node->isMediaControlElement())
        isMediaFullScreen = toParentMediaElement(object)->document()->webkitIsFullScreen();

    if (type <= RotateButton && !isMediaFullScreen) {
        cairo_get_matrix(cairo, &matrix);

        float themeScaleValue = m_partDescs[type].scale;
        FloatSize scaledEntrySize(rect.size());
        scaledEntrySize.scale(matrix.xx, matrix.yy);

        entry = cacheThemePartGet(type, IntSize(scaledEntrySize.width(), scaledEntrySize.height()));
        edje_object_scale_set(entry->o, matrix.xx / themeScaleValue);
    } else {
        entry = cacheThemePartGet(type, rect.size());
        edje_object_scale_set(entry->o, 1);
    }
#else
    entry = cacheThemePartGet(type, rect.size());
#endif

    if (!entry)
        return false;

    applyEdjeStateFromForm(entry->o, controlStatesForRenderer(object));
    if (object && object->hasBackground() && object->style()->visitedDependentColor(CSSPropertyBackgroundColor) != Color::white)
        edje_object_signal_emit(entry->o, "bg_styled", "");
#if !ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    cairo = info.context->platformContext()->cr();
    ASSERT(cairo);
#endif

    // Currently, only sliders needs this message; if other widget ever needs special
    // treatment, move them to special functions.
    if (type == SliderVertical || type == SliderHorizontal) {
        RenderSlider* renderSlider = toRenderSlider(object);
        HTMLInputElement* input = renderSlider->node()->toInputElement();
        if (!input)
            return false;
        double valueRange = input->maximum() - input->minimum();

        OwnArrayPtr<char> buffer = adoptArrayPtr(new char[sizeof(Edje_Message_Float_Set) + sizeof(double)]);
        Edje_Message_Float_Set* msg = new(buffer.get()) Edje_Message_Float_Set;
        msg->count = 2;

        // The first parameter of the message decides if the progress bar
        // grows from the end of the slider or from the beginning. On vertical
        // sliders, it should always be the same and will not be affected by
        // text direction settings.
        if (object->style()->direction() == RTL || type == SliderVertical)
            msg->val[0] = 1;
        else
            msg->val[0] = 0;

        msg->val[1] = (input->valueAsNumber() - input->minimum()) / valueRange;
        edje_object_message_send(entry->o, EDJE_MESSAGE_FLOAT_SET, 0, msg);
#if ENABLE(PROGRESS_ELEMENT)
    } else if (type == ProgressBar) {
        RenderProgress* renderProgress = toRenderProgress(object);
        int max;
        double value;

        max = rect.width();
        value = renderProgress->position();

        OwnArrayPtr<char> buffer = adoptArrayPtr(new char[sizeof(Edje_Message_Float_Set) + sizeof(double)]);
        Edje_Message_Float_Set* msg = new(buffer.get()) Edje_Message_Float_Set;
        msg->count = 2;
        if (object->style()->direction() == RTL)
            msg->val[0] = (1.0 - value) * max;
        else
            msg->val[0] = 0;
        msg->val[1] = value;
        edje_object_message_send(entry->o, EDJE_MESSAGE_FLOAT_SET, 0, msg);
#endif
    }

    edje_object_calc_force(entry->o);
    edje_object_message_signal_process(entry->o);
    updates = evas_render_updates(ecore_evas_get(entry->ee));
    evas_render_updates_free(updates);

    cairo_save(cairo);

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    float xScale = 1;
    float yScale = 1;

    if (type <= RotateButton && !isMediaFullScreen) {
        xScale = matrix.xx;
        yScale = matrix.yy;
        matrix.xx = 1;
        matrix.yy = 1;
        cairo_set_matrix(cairo, &matrix);
    }

    cairo_set_source_surface(cairo, entry->surface, static_cast<float>(rect.x()) * xScale, static_cast<float>(rect.y()) * yScale);
#else
    cairo_set_source_surface(cairo, entry->surface, rect.x(), rect.y());
#endif
    cairo_paint_with_alpha(cairo, 1.0);
    cairo_restore(cairo);

    return false;
}

PassRefPtr<RenderTheme> RenderThemeEfl::create(Page* page)
{
    return adoptRef(new RenderThemeEfl(page));
}

PassRefPtr<RenderTheme> RenderTheme::themeForPage(Page* page)
{
    if (page)
        return RenderThemeEfl::create(page);

    static RenderTheme* fallback = RenderThemeEfl::create(0).leakRef();
    return fallback;
}

static void renderThemeEflColorClassSelectionForeground(void* data, Evas_Object* object, const char* signal, const char* source)
{
    RenderThemeEfl* that = static_cast<RenderThemeEfl *>(data);
    int fr, fg, fb, fa, br, bg, bb, ba;

    if (!edje_object_color_class_get(object, source, &fr, &fg, &fb, &fa, &br, &bg, &bb, &ba, 0, 0, 0, 0))
        return;

    that->setSelectionForegroundColor(fr, fg, fb, fa, br, bg, bb, ba);
}

static void renderThemeEflColorClassSelectionBackground(void* data, Evas_Object* object, const char* signal, const char* source)
{
    RenderThemeEfl* that = static_cast<RenderThemeEfl *>(data);
    int fr, fg, fb, fa, br, bg, bb, ba;

    if (!edje_object_color_class_get(object, source, &fr, &fg, &fb, &fa, &br, &bg, &bb, &ba, 0, 0, 0, 0))
        return;

    that->setSelectionBackgroundColor(fr, fg, fb, fa, br, bg, bb, ba);
}

static void renderThemeEflColorClassFocusRing(void* data, Evas_Object* object, const char* signal, const char* source)
{
    RenderThemeEfl* that = static_cast<RenderThemeEfl *>(data);
    int fr, fg, fb, fa;

    if (!edje_object_color_class_get(object, source, &fr, &fg, &fb, &fa, 0, 0, 0, 0, 0, 0, 0, 0))
        return;

    that->setFocusRingColor(fr, fg, fb, fa);
}

void RenderThemeEfl::setThemePath(const String& path)
{
    if (path == m_themePath)
        return;

    m_themePath = path;
    themeChanged();
}

void RenderThemeEfl::createCanvas()
{
    ASSERT(!m_canvas);
    m_canvas = ecore_evas_buffer_new(1, 1);
    ASSERT(m_canvas);
}

void RenderThemeEfl::createEdje()
{
    ASSERT(!m_edje);
    if (m_themePath.isEmpty())
        EINA_LOG_ERR("No theme defined, unable to set RenderThemeEfl.");
    else {
        m_edje = edje_object_add(ecore_evas_get(m_canvas));
        if (!m_edje)
            EINA_LOG_ERR("Could not create base edje object.");
        else if (!edje_object_file_set(m_edje, m_themePath.utf8().data(), "webkit/base")) {
            Edje_Load_Error err = edje_object_load_error_get(m_edje);
            const char* errmsg = edje_load_error_str(err);
            EINA_LOG_ERR("Could not load 'webkit/base' from theme %s: %s",
                         m_themePath.utf8().data(), errmsg);
            evas_object_del(m_edje);
            m_edje = 0;
        } else {
#define CONNECT(cc, func)                                               \
            edje_object_signal_callback_add(m_edje, "color_class,set",  \
                                            "webkit/" cc, func, this)

            CONNECT("selection/foreground",
                    renderThemeEflColorClassSelectionForeground);
            CONNECT("selection/background",
                    renderThemeEflColorClassSelectionBackground);
            CONNECT("focus_ring", renderThemeEflColorClassFocusRing);
#undef CONNECT
        }
    }
}

void RenderThemeEfl::applyEdjeColors()
{
    int fr, fg, fb, fa, br, bg, bb, ba;
    ASSERT(m_edje);
#define COLOR_GET(cls)                                                  \
    edje_object_color_class_get(m_edje, "webkit/" cls,                   \
                                &fr, &fg, &fb, &fa, &br, &bg, &bb, &ba, \
                                0, 0, 0, 0)

    if (COLOR_GET("selection/foreground")) {
        m_supportsSelectionForegroundColors = true;
        m_activeSelectionForegroundColor = Color(fr, fg, fb, fa);
        m_inactiveSelectionForegroundColor = Color(br, bg, bb, ba);
    }
    if (COLOR_GET("selection/background")) {
        m_inactiveSelectionBackgroundColor = Color(fr, fg, fb, fa);
        m_activeSelectionBackgroundColor = Color(br, bg, bb, ba);
    }
    if (COLOR_GET("focus_ring")) {
        m_focusRingColor = Color(fr, fg, fb, fa);
        // webkit just use platformFocusRingColor() for default theme (without page)
        // this is ugly, but no other way to do it unless we change
        // it to use page themes as much as possible.
        RenderTheme::setCustomFocusRingColor(m_focusRingColor);
    }
#undef COLOR_GET
    platformColorsDidChange();
}

void RenderThemeEfl::applyPartDescriptionFallback(struct ThemePartDesc* desc)
{
    desc->min.setWidth(Length(0, Fixed));
    desc->min.setHeight(Length(0, Fixed));

    desc->max.setWidth(Length(0, Fixed));
    desc->max.setHeight(Length(0, Fixed));

    desc->padding = LengthBox(0, 0, 0, 0);
}

void RenderThemeEfl::applyPartDescription(Evas_Object* object, struct ThemePartDesc* desc)
{
    Evas_Coord minw, minh, maxw, maxh;

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    const char* scale = edje_object_data_get(object, "scale");

    if (scale)
        desc->scale = atof(scale);
    else
        desc->scale = 1;
#endif

    edje_object_size_min_get(object, &minw, &minh);
    if (!minw && !minh)
        edje_object_size_min_calc(object, &minw, &minh);

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    minw /= desc->scale;
    minh /= desc->scale;
#endif
    desc->min.setWidth(Length(minw, Fixed));
    desc->min.setHeight(Length(minh, Fixed));

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    maxw /= desc->scale;
    maxh /= desc->scale;
#endif
    edje_object_size_max_get(object, &maxw, &maxh);
    desc->max.setWidth(Length(maxw, Fixed));
    desc->max.setHeight(Length(maxh, Fixed));

    if (!edje_object_part_exists(object, "text_confinement"))
        desc->padding = LengthBox(0, 0, 0, 0);
    else {
        Evas_Coord px, py, pw, ph;
        Evas_Coord ox = 0, oy = 0, ow = 0, oh = 0;
        int t, r, b, l;

        if (minw > 0)
            ow = minw;
        else
            ow = 100;
        if (minh > 0)
            oh = minh;
        else
            oh = 100;
        if (maxw > 0 && ow > maxw)
            ow = maxw;
        if (maxh > 0 && oh > maxh)
            oh = maxh;

        evas_object_move(object, ox, oy);
        evas_object_resize(object, ow, oh);
        edje_object_calc_force(object);
        edje_object_message_signal_process(object);
        edje_object_part_geometry_get(object, "text_confinement", &px, &py, &pw, &ph);

        t = py - oy;
        b = (oh + oy) - (ph + py);

        l = px - ox;
        r = (ow + ox) - (pw + px);

        desc->padding = LengthBox(t, r, b, l);
    }
}

const char* RenderThemeEfl::edjeGroupFromFormType(FormType type) const
{
    static const char* groups[] = {
#define W(n) "webkit/widget/" n
        W("button"),
        W("radio"),
        W("entry"),
        W("checkbox"),
        W("combo"),
#if ENABLE(PROGRESS_ELEMENT)
        W("progressbar"),
#endif
        W("search/field"),
        W("search/decoration"),
        W("search/results_button"),
        W("search/results_decoration"),
        W("search/cancel_button"),
        W("slider/vertical"),
        W("slider/horizontal"),
        W("slider/thumb_vertical"),
        W("slider/thumb_horizontal"),
#if ENABLE(VIDEO)
        W("mediacontrol/playpause_button"),
        W("mediacontrol/mute_button"),
        W("mediacontrol/seekforward_button"),
        W("mediacontrol/seekbackward_button"),
        W("mediacontrol/fullscreen_button"),
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        W("mediacontrol/slider_thumb"),
        W("mediacontrol/default_image"),
        W("mediacontrol/loading_spinner"),
        W("mediacontrol/rotate_button"),
        W("mediacontrol/control_panel_background"),
#endif
#endif
        W("spinner"),
#undef W
        0
    };
    ASSERT(type >= 0);
    ASSERT((size_t)type < sizeof(groups) / sizeof(groups[0])); // out of sync?
    return groups[type];
}

void RenderThemeEfl::applyPartDescriptions()
{
    Evas_Object* object;
    unsigned int i;
    const char* file;

    ASSERT(m_canvas);
    ASSERT(m_edje);

    edje_object_file_get(m_edje, &file, 0);
    ASSERT(file);

    object = edje_object_add(ecore_evas_get(m_canvas));
    if (!object) {
        EINA_LOG_ERR("Could not create Edje object.");
        return;
    }

    for (i = 0; i < FormTypeLast; i++) {
        FormType type = static_cast<FormType>(i);
        const char* group = edjeGroupFromFormType(type);
        m_partDescs[i].type = type;
        if (!edje_object_file_set(object, file, group)) {
            Edje_Load_Error err = edje_object_load_error_get(object);
            const char* errmsg = edje_load_error_str(err);
            EINA_LOG_ERR("Could not set theme group '%s' of file '%s': %s",
                         group, file, errmsg);

            applyPartDescriptionFallback(m_partDescs + i);
        } else
            applyPartDescription(object, m_partDescs + i);
    }
    evas_object_del(object);
}

void RenderThemeEfl::themeChanged()
{
    cacheThemePartFlush();

    if (!m_canvas) {
        createCanvas();
        if (!m_canvas)
            return;
    }

    if (!m_edje) {
        createEdje();
        if (!m_edje)
            return;
    }

    applyEdjeColors();
    applyPartDescriptions();
}

RenderThemeEfl::RenderThemeEfl(Page* page)
    : RenderTheme()
    , m_page(page)
    , m_activeSelectionBackgroundColor(0, 0, 255)
    , m_activeSelectionForegroundColor(Color::white)
    , m_inactiveSelectionBackgroundColor(0, 0, 128)
    , m_inactiveSelectionForegroundColor(200, 200, 200)
    , m_focusRingColor(32, 32, 224, 224)
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    , m_sliderThumbColor(0xee, 0xee, 0xee)
#else
    , m_sliderThumbColor(Color::darkGray)
#endif
#if ENABLE(VIDEO)
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    , m_mediaPanelColor(0, 0, 0, 0)
    , m_mediaSliderColor(102, 102, 102, 153)
    , m_mediaBufferingColor(94, 127, 147, 255)
    , m_mediaPlayingColor(0, 157, 255, 255)
#else
    , m_mediaPanelColor(220, 220, 195) // light tannish color.
    , m_mediaSliderColor(Color::white)
#endif
#endif
    , m_canvas(0)
    , m_edje(0)
    , m_supportsSelectionForegroundColors(false)
{
}

RenderThemeEfl::~RenderThemeEfl()
{
    cacheThemePartFlush();

    if (m_canvas) {
        if (m_edje)
            evas_object_del(m_edje);
        ecore_evas_free(m_canvas);
    }
}

void RenderThemeEfl::setSelectionForegroundColor(int foreR, int foreG, int foreB, int foreA, int backR, int backG, int backB, int backA)
{
    m_activeSelectionForegroundColor = Color(foreR, foreG, foreB, foreA);
    m_inactiveSelectionForegroundColor = Color(backR, backG, backB, backA);
    m_supportsSelectionForegroundColors = true;
    platformColorsDidChange();
}

void RenderThemeEfl::setSelectionBackgroundColor(int foreR, int foreG, int foreB, int foreA, int backR, int backG, int backB, int backA)
{
    m_activeSelectionBackgroundColor = Color(foreR, foreG, foreB, foreA);
    m_inactiveSelectionBackgroundColor = Color(backR, backG, backB, backA);
    platformColorsDidChange();
}

void RenderThemeEfl::setFocusRingColor(int r, int g, int b, int a)
{
    m_focusRingColor = Color(r, g, b, a);
    // webkit just use platformFocusRingColor() for default theme (without page)
    // this is ugly, but no other way to do it unless we change
    // it to use page themes as much as possible.
    RenderTheme::setCustomFocusRingColor(m_focusRingColor);
    platformColorsDidChange();
}

static bool supportsFocus(ControlPart appearance)
{
    switch (appearance) {
    case PushButtonPart:
    case ButtonPart:
    case TextFieldPart:
    case TextAreaPart:
    case SearchFieldPart:
    case MenulistPart:
    case RadioPart:
    case CheckboxPart:
    case SliderVerticalPart:
    case SliderHorizontalPart:
        return true;
    default:
        return false;
    }
}

bool RenderThemeEfl::supportsFocusRing(const RenderStyle* style) const
{
    return supportsFocus(style->appearance());
}

bool RenderThemeEfl::controlSupportsTints(const RenderObject* object) const
{
    return isEnabled(object);
}

LayoutUnit RenderThemeEfl::baselinePosition(const RenderObject* object) const
{
    if (!object->isBox())
        return 0;

    if (object->style()->appearance() == CheckboxPart
    ||  object->style()->appearance() == RadioPart)
        return toRenderBox(object)->marginTop() + toRenderBox(object)->height() - 3;

    return RenderTheme::baselinePosition(object);
}

#if ENABLE(TIZEN_WEBKIT2_CHANGEABLE_UI_FOR_TEXT_SELECTION)
Color RenderThemeEfl::platformActiveSelectionBackgroundColor() const
{
    int r, g, b, a;
    ea_theme_color_get("F065", &r, &g, &b, &a, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    return Color(r, g, b, a);
}

Color RenderThemeEfl::platformInactiveSelectionBackgroundColor() const
{
    int r, g, b, a;
    ea_theme_color_get("F065", &r, &g, &b, &a, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    return Color(r, g, b, a);
}
#endif

bool RenderThemeEfl::paintSliderTrack(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    if (object->style()->appearance() == SliderHorizontalPart)
        return paintThemePart(object, SliderHorizontal, info, rect);
    return paintThemePart(object, SliderVertical, info, rect);
}

void RenderThemeEfl::adjustSliderTrackStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    style->setBoxShadow(nullptr);
}

void RenderThemeEfl::adjustSliderThumbStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    RenderTheme::adjustSliderThumbStyle(styleResolver, style, element);
    style->setBoxShadow(nullptr);
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
void RenderThemeEfl::adjustSliderThumbSize(RenderStyle* style, Element* element) const
#else
void RenderThemeEfl::adjustSliderThumbSize(RenderStyle* style, Element*) const
#endif
{
    ControlPart part = style->appearance();
    if (part == SliderThumbVerticalPart) {
        style->setWidth(Length(sliderThumbHeight, Fixed));
        style->setHeight(Length(sliderThumbWidth, Fixed));
    } else if (part == SliderThumbHorizontalPart) {
        style->setWidth(Length(sliderThumbWidth, Fixed));
        style->setHeight(Length(sliderThumbHeight, Fixed));
#if ENABLE(VIDEO)
    } else if (part == MediaSliderThumbPart) {
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
        int width = mediaSliderPressThumbWidth;
        int height = mediaSliderPressThumbWidth;


        if (element->document()->webkitIsFullScreen() || element->document()->isMediaDocument()) {
            width = mediaSliderPressThumbWidth;
            height = mediaSliderPressThumbHeight;
        } else {
            width = mediaSliderPressThumbWidth / 2;
            height = mediaSliderPressThumbHeight / 2;
        }

        style->setWidth(Length(width, Fixed));
        style->setHeight(Length(height, Fixed));
#else
        style->setWidth(Length(mediaSliderThumbWidth, Fixed));
        style->setHeight(Length(mediaSliderThumbHeight, Fixed));
#endif
#endif
    }
}

#if ENABLE(DATALIST_ELEMENT)
IntSize RenderThemeEfl::sliderTickSize() const
{
    // FIXME: We need to set this to the size of one tick mark.
    return IntSize(0, 0);
}

int RenderThemeEfl::sliderTickOffsetFromTrackCenter() const
{
    // FIXME: We need to set this to the position of the tick marks.
    return 0;
}
#endif

bool RenderThemeEfl::paintSliderThumb(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    if (object->style()->appearance() == SliderThumbHorizontalPart)
        paintThemePart(object, SliderThumbHorizontal, info, rect);
    else
        paintThemePart(object, SliderThumbVertical, info, rect);

    return false;
}

void RenderThemeEfl::adjustCheckboxStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustCheckboxStyle(styleResolver, style, element);
        return;
    }
#if PLATFORM(TIZEN)
    adjustSizeConstraints(style, CheckBox, styleResolver);
#else
    adjustSizeConstraints(style, CheckBox);
#endif
    style->resetBorder();

    const struct ThemePartDesc *desc = m_partDescs + (size_t)CheckBox;
    if (style->width().value() < desc->min.width().value())
        style->setWidth(desc->min.width());
    if (style->height().value() < desc->min.height().value())
        style->setHeight(desc->min.height());
}

bool RenderThemeEfl::paintCheckbox(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, CheckBox, info, rect);
}

void RenderThemeEfl::adjustRadioStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustRadioStyle(styleResolver, style, element);
        return;
    }
#if PLATFORM(TIZEN)
    adjustSizeConstraints(style, RadioButton, styleResolver);
#else
    adjustSizeConstraints(style, RadioButton);
#endif
    style->resetBorder();

    const struct ThemePartDesc *desc = m_partDescs + (size_t)RadioButton;
    if (style->width().value() < desc->min.width().value())
        style->setWidth(desc->min.width());
    if (style->height().value() < desc->min.height().value())
        style->setHeight(desc->min.height());
}

bool RenderThemeEfl::paintRadio(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, RadioButton, info, rect);
}

void RenderThemeEfl::adjustButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustButtonStyle(styleResolver, style, element);
        return;
    }

    // adjustSizeConstrains can make SquareButtonPart's size wrong (by adjusting paddings), so call it only for PushButtonPart and ButtonPart
    if (style->appearance() == PushButtonPart || style->appearance() == ButtonPart)
#if PLATFORM(TIZEN)
        adjustSizeConstraints(style, Button, styleResolver);
#else
        adjustSizeConstraints(style, Button);
#endif
}

bool RenderThemeEfl::paintButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, Button, info, rect);
}

void RenderThemeEfl::adjustMenuListStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustMenuListStyle(styleResolver, style, element);
        return;
    }

#if PLATFORM(TIZEN)
    // Height is locked to auto on all browsers.
    style->setLineHeight(RenderStyle::initialLineHeight());
    adjustSizeConstraints(style, ComboBox, styleResolver);
    const struct ThemePartDesc* desc = m_partDescs + ComboBox;
    // Add right padding to display combo box button
    if (styleResolver->currentMatchResultHasAuthorStylesForProperty(style, CSSPropertyPaddingRight))
        style->setPaddingRight(Length(desc->padding.right().value() + style->paddingRight().value(), Fixed));
#else
    adjustSizeConstraints(style, ComboBox);
#endif
    style->resetBorder();
    style->setWhiteSpace(PRE);
}

bool RenderThemeEfl::paintMenuList(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, ComboBox, info, rect);
}

void RenderThemeEfl::adjustMenuListButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
  // The <select> box must be at least 12px high for the button to render the text inside the box without clipping.
    const int dropDownBoxMinHeight = 12;

    // Calculate min-height of the <select> element.
    int minHeight = style->fontMetrics().height();
    minHeight = max(minHeight, dropDownBoxMinHeight);
    style->setMinHeight(Length(minHeight, Fixed));

    adjustMenuListStyle(styleResolver, style, element);
}

bool RenderThemeEfl::paintMenuListButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintMenuList(object, info, rect);
}

void RenderThemeEfl::adjustTextFieldStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustTextFieldStyle(styleResolver, style, element);
        return;
    }
#if PLATFORM(TIZEN)
    adjustSizeConstraints(style, TextField, styleResolver);
#else
    adjustSizeConstraints(style, TextField);
#endif
    style->resetBorder();
}

bool RenderThemeEfl::paintTextField(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, TextField, info, rect);
}

void RenderThemeEfl::adjustTextAreaStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    adjustTextFieldStyle(styleResolver, style, element);
}

bool RenderThemeEfl::paintTextArea(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintTextField(object, info, rect);
}

void RenderThemeEfl::adjustSearchFieldDecorationStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustSearchFieldDecorationStyle(styleResolver, style, element);
        return;
    }
#if PLATFORM(TIZEN)
    adjustSizeConstraints(style, SearchFieldDecoration, styleResolver);
#else
    adjustSizeConstraints(style, SearchFieldDecoration);
#endif
    style->resetBorder();
    style->setWhiteSpace(PRE);
}

bool RenderThemeEfl::paintSearchFieldDecoration(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, SearchFieldDecoration, info, rect);
}

void RenderThemeEfl::adjustSearchFieldResultsButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustSearchFieldResultsButtonStyle(styleResolver, style, element);
        return;
    }
#if PLATFORM(TIZEN)
    adjustSizeConstraints(style, SearchFieldResultsButton, styleResolver);
#else
    adjustSizeConstraints(style, SearchFieldResultsButton);
#endif
    style->resetBorder();
    style->setWhiteSpace(PRE);
}

bool RenderThemeEfl::paintSearchFieldResultsButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, SearchFieldResultsButton, info, rect);
}

void RenderThemeEfl::adjustSearchFieldResultsDecorationStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustSearchFieldResultsDecorationStyle(styleResolver, style, element);
        return;
    }
#if PLATFORM(TIZEN)
    adjustSizeConstraints(style, SearchFieldResultsDecoration, styleResolver);
#else
    adjustSizeConstraints(style, SearchFieldResultsDecoration);
#endif
    style->resetBorder();
    style->setWhiteSpace(PRE);
}

bool RenderThemeEfl::paintSearchFieldResultsDecoration(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, SearchFieldResultsDecoration, info, rect);
}

void RenderThemeEfl::adjustSearchFieldCancelButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustSearchFieldCancelButtonStyle(styleResolver, style, element);
        return;
    }
#if PLATFORM(TIZEN)
    adjustSizeConstraints(style, SearchFieldCancelButton, styleResolver);
#else
    adjustSizeConstraints(style, SearchFieldCancelButton);
#endif
    style->resetBorder();
    style->setWhiteSpace(PRE);

    // Logic taken from RenderThemeChromium.cpp.
    // Scale the button size based on the font size.
    float fontScale = style->fontSize() / defaultFontSize;
    int cancelButtonSize = lroundf(std::min(std::max(minCancelButtonSize, defaultFontSize * fontScale), maxCancelButtonSize));

    style->setWidth(Length(cancelButtonSize, Fixed));
    style->setHeight(Length(cancelButtonSize, Fixed));
}

bool RenderThemeEfl::paintSearchFieldCancelButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, SearchFieldCancelButton, info, rect);
}

void RenderThemeEfl::adjustSearchFieldStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustSearchFieldStyle(styleResolver, style, element);
        return;
    }
#if PLATFORM(TIZEN)
    adjustSizeConstraints(style, SearchField, styleResolver);
#else
    adjustSizeConstraints(style, SearchField);
#endif
    style->resetBorder();
    style->setWhiteSpace(PRE);
}

bool RenderThemeEfl::paintSearchField(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, SearchField, info, rect);
}
#if !ENABLE(TIZEN_INPUTTYPE_NUMBER)
void RenderThemeEfl::adjustInnerSpinButtonStyle(StyleResolver* styleResolver, RenderStyle* style, Element* element) const
{
    if (!m_page && element && element->document()->page()) {
        static_cast<RenderThemeEfl*>(element->document()->page()->theme())->adjustInnerSpinButtonStyle(styleResolver, style, element);
        return;
    }
#if PLATFORM(TIZEN)
    adjustSizeConstraints(style, Spinner, styleResolver);
#else
    adjustSizeConstraints(style, Spinner);
#endif
}

bool RenderThemeEfl::paintInnerSpinButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, Spinner, info, rect);
}
#endif
void RenderThemeEfl::setDefaultFontSize(int size)
{
    defaultFontSize = size;
}

void RenderThemeEfl::systemFont(int propId, FontDescription& fontDescription) const
{
    // It was called by RenderEmbeddedObject::paintReplaced to render alternative string.
    // To avoid cairo_error while rendering, fontDescription should be passed.
#if ENABLE(TIZEN_SYSTEM_FONT)
    DEFINE_STATIC_LOCAL(String, fontFace, ("Arial"));
#else
    DEFINE_STATIC_LOCAL(String, fontFace, ("Sans"));
#endif
    float fontSize = defaultFontSize;

    fontDescription.firstFamily().setFamily(fontFace);
    fontDescription.setSpecifiedSize(fontSize);
    fontDescription.setIsAbsoluteSize(true);
    fontDescription.setGenericFamily(FontDescription::NoFamily);
    fontDescription.setWeight(FontWeightNormal);
    fontDescription.setItalic(false);
}

#if ENABLE(PROGRESS_ELEMENT)
void RenderThemeEfl::adjustProgressBarStyle(StyleResolver*, RenderStyle* style, Element*) const
{
    style->setBoxShadow(nullptr);
}

double RenderThemeEfl::animationRepeatIntervalForProgressBar(RenderProgress*) const
{
    return progressAnimationInterval;
}

double RenderThemeEfl::animationDurationForProgressBar(RenderProgress*) const
{
    return progressAnimationInterval * progressAnimationFrames * 2; // "2" for back and forth;
}

bool RenderThemeEfl::paintProgressBar(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    return paintThemePart(object, ProgressBar, info, rect);
}
#endif

#if ENABLE(VIDEO)
bool RenderThemeEfl::emitMediaButtonSignal(FormType formType, MediaControlElementType mediaElementType, const IntRect& rect)
{
    ThemePartCacheEntry* entry;

    entry = cacheThemePartGet(formType, rect.size());
    ASSERT(entry);
    if (!entry)
        return false;

    if (mediaElementType == MediaPlayButton)
        edje_object_signal_emit(entry->o, "play", "");
    else if (mediaElementType == MediaPauseButton)
        edje_object_signal_emit(entry->o, "pause", "");
    else if (mediaElementType == MediaMuteButton)
        edje_object_signal_emit(entry->o, "mute", "");
    else if (mediaElementType == MediaUnMuteButton)
        edje_object_signal_emit(entry->o, "sound", "");
    else if (mediaElementType == MediaSeekForwardButton)
        edje_object_signal_emit(entry->o, "seekforward", "");
    else if (mediaElementType == MediaSeekBackButton)
        edje_object_signal_emit(entry->o, "seekbackward", "");
    else if (mediaElementType == MediaEnterFullscreenButton)
        edje_object_signal_emit(entry->o, "fullscreen", "");
    else if (mediaElementType == MediaExitFullscreenButton)
        edje_object_signal_emit(entry->o, "normalscreen", "");
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    else if (mediaElementType == MediaFullscreenDisabledButton)
        edje_object_signal_emit(entry->o, "fullscreendisabled", "");
    else if (mediaElementType == MediaDefaultImage)
        edje_object_signal_emit(entry->o, "defaultimage", "");
    else if (mediaElementType == MediaOverlayLoadingSpinner)
        edje_object_signal_emit(entry->o, "loading", "");
    else if (mediaElementType == MediaRotateButton)
        edje_object_signal_emit(entry->o, "rotate", "");
    else if (mediaElementType == MediaControlPanelBackGroundVertical)
        edje_object_signal_emit(entry->o, "ControlPanelBackGroundVertical", "");
    else if (mediaElementType == MediaControlPanelBackGroundHorizontal)
        edje_object_signal_emit(entry->o, "ControlPanelBackGroundHorizontal", "");
    else if (mediaElementType == MediaSliderThumb)
        edje_object_signal_emit(entry->o, "scrubbing", "");
#endif
    else
        return false;

    return true;
}

String RenderThemeEfl::extraMediaControlsStyleSheet()
{
#if ENABLE(TIZEN_MEDIA_CONTROL_USER_AGENT_SHEET)
    return String(mediaControlsTizenUserAgentStyleSheet, sizeof(mediaControlsTizenUserAgentStyleSheet));
#else
    return String(mediaControlsEflUserAgentStyleSheet, sizeof(mediaControlsEflUserAgentStyleSheet));
#endif
}

#if ENABLE(FULLSCREEN_API)
#if ENABLE(TIZEN_MEDIA_CONTROL_USER_AGENT_SHEET)
String RenderThemeEfl::extraFullScreenHorizontalStyleSheet()
{
    return String(mediaControlsTizenFullscreenHorizontalUserAgentStyleSheet, sizeof(mediaControlsTizenFullscreenHorizontalUserAgentStyleSheet));
}

String RenderThemeEfl::extraFullScreenVerticalStyleSheet()
{
    return String(mediaControlsTizenFullscreenVerticalUserAgentStyleSheet, sizeof(mediaControlsTizenFullscreenVerticalUserAgentStyleSheet));
}
#else
String RenderThemeEfl::extraFullScreenStyleSheet()
{
    return String(mediaControlsEflFullscreenUserAgentStyleSheet, sizeof(mediaControlsEflFullscreenUserAgentStyleSheet));
}
#endif
#endif

String RenderThemeEfl::formatMediaControlsCurrentTime(float currentTime, float duration) const
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    return formatMediaControlsTime(currentTime); // show current time only
#else
    return formatMediaControlsTime(currentTime) + " / " + formatMediaControlsTime(duration);
#endif
}

bool RenderThemeEfl::paintMediaFullscreenButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    Node* mediaNode = object->node() ? object->node()->shadowHost() : 0;
    if (!mediaNode)
        mediaNode = object->node();

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (!mediaNode)
#else
    if (!mediaNode || (!mediaNode->hasTagName(videoTag)))
#endif
        return false;

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    if (!emitMediaButtonSignal(FullScreenButton, mediaControlElementType(object->node()), convertToScaleRect(info, rect, FullScreenButton, object)))
#else
    if (!emitMediaButtonSignal(FullScreenButton, mediaControlElementType(object->node()), rect))
#endif
        return false;

    return paintThemePart(object, FullScreenButton, info, rect);
}

bool RenderThemeEfl::paintMediaMuteButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    Node* mediaNode = object->node() ? object->node()->shadowHost() : 0;
    if (!mediaNode)
        mediaNode = object->node();
#if ENABLE(VIDEO)
    if (!mediaNode || !mediaNode->isElementNode() || !static_cast<Element*>(mediaNode)->isMediaElement())
        return false;
#endif

    HTMLMediaElement* mediaElement = static_cast<HTMLMediaElement*>(mediaNode);

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    if (!emitMediaButtonSignal(MuteUnMuteButton, mediaElement->muted() ? MediaMuteButton : MediaUnMuteButton, convertToScaleRect(info, rect, MuteUnMuteButton, object)))
#else
    if (!emitMediaButtonSignal(MuteUnMuteButton, mediaElement->muted() ? MediaMuteButton : MediaUnMuteButton, rect))
#endif
        return false;

    return paintThemePart(object, MuteUnMuteButton, info, rect);
}

bool RenderThemeEfl::paintMediaPlayButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    Node* node = object->node();
    if (!node || !node->isMediaControlElement())
        return false;

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    // Notify the edc on entering fullscreen
    HTMLMediaElement* mediaElement = toParentMediaElement(object);
    if (mediaElement->document()->webkitIsFullScreen()) {
#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
        emitMediaButtonSignal(PlayPauseButton, MediaControlElementType::MediaEnterFullscreenButton, convertToScaleRect(info, rect, PlayPauseButton, object));
#else
        emitMediaButtonSignal(PlayPauseButton, MediaControlElementType::MediaEnterFullscreenButton, rect);
#endif
    }
#endif

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    if (!emitMediaButtonSignal(PlayPauseButton, mediaControlElementType(node), convertToScaleRect(info, rect, PlayPauseButton, object)))
#else
    if (!emitMediaButtonSignal(PlayPauseButton, mediaControlElementType(node), rect))
#endif
        return false;

    return paintThemePart(object, PlayPauseButton, info, rect);
}

bool RenderThemeEfl::paintMediaSeekBackButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    Node* node = object->node();
    if (!node || !node->isMediaControlElement())
        return 0;

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    if (!emitMediaButtonSignal(SeekBackwardButton, mediaControlElementType(node), convertToScaleRect(info, rect, SeekBackwardButton, object)))
#else
    if (!emitMediaButtonSignal(SeekBackwardButton, mediaControlElementType(node), rect))
#endif
        return false;

    return paintThemePart(object, SeekBackwardButton, info, rect);
}

bool RenderThemeEfl::paintMediaSeekForwardButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    Node* node = object->node();
    if (!node || !node->isMediaControlElement())
        return 0;

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    if (!emitMediaButtonSignal(SeekForwardButton, mediaControlElementType(node), convertToScaleRect(info, rect, SeekForwardButton, object)))
#else
    if (!emitMediaButtonSignal(SeekForwardButton, mediaControlElementType(node), rect))
#endif
        return false;

    return paintThemePart(object, SeekForwardButton, info, rect);
}

bool RenderThemeEfl::paintMediaSliderTrack(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    GraphicsContext* context = info.context;
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    HTMLMediaElement* mediaElement = toParentMediaElement(object);

    if (!mediaElement)
        return false;

    sliderScrubbing = mediaElement->isScrubbing();
    IntRect trackRect = rect;

    if (mediaElement->document()->webkitIsFullScreen() || mediaElement->document()->isMediaDocument()) {
        trackRect.setX(rect.x() + mediaSliderPressThumbWidth / 2 + 2);
        trackRect.setWidth(rect.width() - mediaSliderPressThumbWidth - 4);
    } else {
        trackRect.setX(rect.x() + mediaSliderPressThumbWidth / 4);
        trackRect.setWidth(rect.width() - mediaSliderPressThumbWidth / 2);
    }

    context->setLineCap(RoundCap);
    context->setStrokeStyle(SolidStroke);
    context->setStrokeThickness(trackRect.height());

    context->setStrokeColor(m_mediaSliderColor, ColorSpaceDeviceRGB);
    context->drawLine(IntPoint(trackRect.x(), trackRect.y() + trackRect.height() / 2), IntPoint(trackRect.x() + trackRect.width(), trackRect.y() + trackRect.height() / 2));

    float mediaDuration = mediaElement->duration();
    if (!isfinite(mediaDuration))
        return false;

    RefPtr<TimeRanges> timeRanges = mediaElement->buffered();

    float suspendedPoint = 0;

    for (unsigned index = 0; index < timeRanges->length(); ++index) {
        ExceptionCode ignoredException;
        float start = timeRanges->start(index, ignoredException);
        float end = timeRanges->end(index, ignoredException);
        IntRect rangeRect;

        if (suspendedPoint != start)
            start = suspendedPoint;

        int width = trackRect.width() * ((end - start) / mediaDuration);

        if (!index) {
            rangeRect = trackRect;
            rangeRect.setWidth(width);
        } else {
            rangeRect.setLocation(IntPoint(trackRect.x() + start / mediaDuration * trackRect.width(), trackRect.y()));
            rangeRect.setSize(IntSize(width, trackRect.height()));
        }

        suspendedPoint = timeRanges->end(index, ignoredException);

        // Don't bother drawing empty range.
        if (rangeRect.isEmpty())
            continue;

        context->setStrokeColor(m_mediaBufferingColor, ColorSpaceDeviceRGB);
        context->drawLine(IntPoint(rangeRect.x(), rangeRect.y() + rect.height() / 2), IntPoint(rangeRect.x() + rangeRect.width(), rangeRect.y() + rect.height() / 2));
    }

    float currentTime = mediaElement->currentTime();
    int width = trackRect.width() * (currentTime / mediaDuration);

    // Draw playing track
    context->setStrokeColor(m_mediaPlayingColor, ColorSpaceDeviceRGB);
    context->drawLine(IntPoint(trackRect.x(), trackRect.y() + trackRect.height() / 2), IntPoint(trackRect.x() + width, trackRect.y() + trackRect.height() / 2));
#else
    context->fillRect(FloatRect(rect), m_mediaPanelColor, ColorSpaceDeviceRGB);
    context->fillRect(FloatRect(IntRect(rect.x(), rect.y() + (rect.height() - mediaSliderHeight) / 2,
                                        rect.width(), mediaSliderHeight)), m_mediaSliderColor, ColorSpaceDeviceRGB);

    RenderStyle* style = object->style();
    HTMLMediaElement* mediaElement = toParentMediaElement(object);

    if (!mediaElement)
        return false;

    // Draw the buffered ranges. This code is highly inspired from
    // Chrome for the gradient code.
    float mediaDuration = mediaElement->duration();

    RefPtr<TimeRanges> timeRanges = mediaElement->buffered();
    IntRect trackRect = rect;
    int totalWidth = trackRect.width();

    trackRect.inflate(-style->borderLeftWidth());

    context->save();
    context->setStrokeStyle(NoStroke);

    for (unsigned index = 0; index < timeRanges->length(); ++index) {
        ExceptionCode ignoredException;
        float start = timeRanges->start(index, ignoredException);
        float end = timeRanges->end(index, ignoredException);
        int width = ((end - start) * totalWidth) / mediaDuration;
        IntRect rangeRect;

        if (!index) {
            rangeRect = trackRect;
            rangeRect.setWidth(width);
        } else {
            rangeRect.setLocation(IntPoint(trackRect.x() + start / mediaDuration* totalWidth, trackRect.y()));
            rangeRect.setSize(IntSize(width, trackRect.height()));
        }
        // Don't bother drawing empty range.
        if (rangeRect.isEmpty())
            continue;

        IntPoint sliderTopLeft = rangeRect.location();
        IntPoint sliderTopRight = sliderTopLeft;
        sliderTopRight.move(0, rangeRect.height());
        context->fillRect(FloatRect(rect), m_mediaPanelColor, ColorSpaceDeviceRGB);
    }
    context->restore();
#endif // ENABLE(TIZEN_GSTREAMER_VIDEO)
    return true;
}

bool RenderThemeEfl::paintMediaSliderThumb(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
#if ENABLE(TIZEN_GSTREAMER_VIDEO)
    if (object->style()->appearance() == MediaSliderThumbPart) {
        if (sliderScrubbing)
#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
            emitMediaButtonSignal(SliderThumb, MediaControlElementType::MediaSliderThumb, convertToScaleRect(info, rect, SliderThumb, object));
#else
            emitMediaButtonSignal(SliderThumb, MediaControlElementType::MediaSliderThumb, rect);
#endif
        return paintThemePart(object, SliderThumb, info, rect);
    }

    return true;
#else
    IntSize thumbRect(3, 3);
    info.context->fillRoundedRect(rect, thumbRect, thumbRect, thumbRect, thumbRect, m_sliderThumbColor, ColorSpaceDeviceRGB);
    return true;
#endif
}

bool RenderThemeEfl::paintMediaVolumeSliderContainer(RenderObject*, const PaintInfo& info, const IntRect& rect)
{
    notImplemented();
    return false;
}

bool RenderThemeEfl::paintMediaVolumeSliderTrack(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    notImplemented();
    return false;
}

bool RenderThemeEfl::paintMediaVolumeSliderThumb(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    notImplemented();
    return false;
}

bool RenderThemeEfl::paintMediaCurrentTime(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    info.context->fillRect(FloatRect(rect), m_mediaPanelColor, ColorSpaceDeviceRGB);
    return true;
}

#if ENABLE(TIZEN_GSTREAMER_VIDEO)
bool RenderThemeEfl::paintMediaTimeRemaining(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    info.context->fillRect(FloatRect(rect), m_mediaPanelColor, ColorSpaceDeviceRGB);
    return true;
}

bool RenderThemeEfl::paintMediaDefaultImage(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    Node* node = object->node();
    if (!node || !node->isMediaControlElement())
        return 0;

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    if (!emitMediaButtonSignal(DefaultImage, mediaControlElementType(node), convertToScaleRect(info, rect, DefaultImage, object)))
#else
    if (!emitMediaButtonSignal(DefaultImage, mediaControlElementType(node), rect))
#endif
        return false;

    return paintThemePart(object, DefaultImage, info, rect);
}

bool RenderThemeEfl::paintMediaOverlayLoadingSpinner(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    Node* node = object->node();
    if (!node || !node->isMediaControlElement())
        return false;

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    if (!emitMediaButtonSignal(OverlayLoadingSpinner, mediaControlElementType(object->node()), convertToScaleRect(info, rect, OverlayLoadingSpinner, object)))
#else
    if (!emitMediaButtonSignal(OverlayLoadingSpinner, mediaControlElementType(object->node()), rect))
#endif
        return false;

    return paintThemePart(object, OverlayLoadingSpinner, info, rect);
}

bool RenderThemeEfl::paintMediaRotateButton(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    Node* node = object->node();
    if (!node || !node->isMediaControlElement())
        return 0;

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
    if (!emitMediaButtonSignal(RotateButton, mediaControlElementType(node), convertToScaleRect(info, rect, RotateButton, object)))
#else
    if (!emitMediaButtonSignal(RotateButton, mediaControlElementType(node), rect))
#endif
        return false;

    return paintThemePart(object, RotateButton, info, rect);
}

bool RenderThemeEfl::paintMediaControlPanelBackGroundImage(RenderObject* object, const PaintInfo& info, const IntRect& rect)
{
    Node* node = object->node();
    if (!node || !node->isMediaControlElement())
        return 0;

    if (object->style()->appearance() == ControlPart::MediaControlPanelBackGroundImageVerticalPart) {
        if (!emitMediaButtonSignal(ControlPanelBackGround, MediaControlElementType::MediaControlPanelBackGroundVertical, rect))
            return false;
    } else {
        if (!emitMediaButtonSignal(ControlPanelBackGround, MediaControlElementType::MediaControlPanelBackGroundHorizontal, rect))
            return false;
    }

    return paintThemePart(object, ControlPanelBackGround, info, rect);
}
#endif

#if ENABLE(TIZEN_THEME_SCALE_SUPPORT)
IntRect RenderThemeEfl::convertToScaleRect(const PaintInfo& info, const IntRect& rect, FormType type, RenderObject* object) {
    Node* node = object->node();
    if (node && node->isMediaControlElement() && toParentMediaElement(object)->document()->webkitIsFullScreen())
        return rect;

    cairo_t* cairo;
    cairo_matrix_t matrix;
    cairo = info.context->platformContext()->cr();
    ASSERT(cairo);
    cairo_get_matrix(cairo, &matrix);

    FloatSize scaledEntrySize(rect.size());
    scaledEntrySize.scale(matrix.xx, matrix.yy);

    IntRect scaleRect(static_cast<int>(rect.x()) * matrix.xx,
          static_cast<int>(rect.y()) * matrix.yy,
          scaledEntrySize.width(),
          scaledEntrySize.height());

    return scaleRect;
}
#endif

#endif
}
