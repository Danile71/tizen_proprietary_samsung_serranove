/*
 * Copyright (C) 2012 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "EflScreenUtilities.h"

#ifdef HAVE_ECORE_X
#include <Ecore_Evas.h>
#include <Ecore_X.h>
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>
#endif

namespace WebCore {

#ifdef HAVE_ECORE_X
class CursorMap {
private:
    HashMap<String, unsigned short> m_cursorStringMap;

public:
    CursorMap();
    int cursor(const String&);
};

int CursorMap::cursor(const String& cursorGroup)
{
    int ret = m_cursorStringMap.get(cursorGroup);

    if (ret < ECORE_X_CURSOR_X || ret > ECORE_X_CURSOR_XTERM)
        ret = ECORE_X_CURSOR_LEFT_PTR;

    return ret;
}

CursorMap::CursorMap()
{
    m_cursorStringMap.set("cursor/pointer", ECORE_X_CURSOR_LEFT_PTR);
    m_cursorStringMap.set("cursor/move", ECORE_X_CURSOR_FLEUR);
    m_cursorStringMap.set("cursor/cross", ECORE_X_CURSOR_CROSS);
    m_cursorStringMap.set("cursor/hand", ECORE_X_CURSOR_HAND2);
    m_cursorStringMap.set("cursor/i_beam", ECORE_X_CURSOR_XTERM);
    m_cursorStringMap.set("cursor/wait", ECORE_X_CURSOR_WATCH);
    m_cursorStringMap.set("cursor/help", ECORE_X_CURSOR_QUESTION_ARROW);
    m_cursorStringMap.set("cursor/east_resize", ECORE_X_CURSOR_RIGHT_SIDE);
    m_cursorStringMap.set("cursor/north_resize", ECORE_X_CURSOR_TOP_SIDE);
    m_cursorStringMap.set("cursor/north_east_resize", ECORE_X_CURSOR_TOP_RIGHT_CORNER);
    m_cursorStringMap.set("cursor/north_west_resize", ECORE_X_CURSOR_TOP_LEFT_CORNER);
    m_cursorStringMap.set("cursor/south_resize", ECORE_X_CURSOR_BOTTOM_SIDE);
    m_cursorStringMap.set("cursor/south_east_resize", ECORE_X_CURSOR_BOTTOM_RIGHT_CORNER);
    m_cursorStringMap.set("cursor/south_west_resize", ECORE_X_CURSOR_BOTTOM_LEFT_CORNER);
    m_cursorStringMap.set("cursor/west_resize", ECORE_X_CURSOR_LEFT_SIDE);
    m_cursorStringMap.set("cursor/north_south_resize", ECORE_X_CURSOR_SB_H_DOUBLE_ARROW);
    m_cursorStringMap.set("cursor/east_west_resize", ECORE_X_CURSOR_SB_V_DOUBLE_ARROW);
    m_cursorStringMap.set("cursor/north_east_south_west_resize", ECORE_X_CURSOR_SIZING);
    m_cursorStringMap.set("cursor/north_west_south_east_resize", ECORE_X_CURSOR_SIZING);
    m_cursorStringMap.set("cursor/column_resize", ECORE_X_CURSOR_SB_V_DOUBLE_ARROW);
    m_cursorStringMap.set("cursor/row_resize", ECORE_X_CURSOR_SB_H_DOUBLE_ARROW);
    m_cursorStringMap.set("cursor/middle_panning",  ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set("cursor/east_panning", ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set("cursor/north_panning", ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set("cursor/north_east_panning", ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set("cursor/north_west_panning", ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set("cursor/south_panning", ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set("cursor/south_east_panning", ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set("cursor/south_west_panning", ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set("cursor/west_panning", ECORE_X_CURSOR_CROSS_REVERSE);
    m_cursorStringMap.set("cursor/vertical_text", ECORE_X_CURSOR_SB_DOWN_ARROW);
    m_cursorStringMap.set("cursor/cell", ECORE_X_CURSOR_ICON);
    m_cursorStringMap.set("cursor/context_menu", ECORE_X_CURSOR_HAND2);
    m_cursorStringMap.set("cursor/no_drop", ECORE_X_CURSOR_DOT_BOX_MASK);
    m_cursorStringMap.set("cursor/copy", ECORE_X_CURSOR_ICON);
    m_cursorStringMap.set("cursor/progress", ECORE_X_CURSOR_WATCH);
    m_cursorStringMap.set("cursor/alias", ECORE_X_CURSOR_MAN);
    m_cursorStringMap.set("cursor/none", ECORE_X_CURSOR_X);
    m_cursorStringMap.set("cursor/not_allowed", ECORE_X_CURSOR_X);
    m_cursorStringMap.set("cursor/zoom_in", ECORE_X_CURSOR_DIAMOND_CROSS);
    m_cursorStringMap.set("cursor/zoom_out", ECORE_X_CURSOR_DIAMOND_CROSS);
    m_cursorStringMap.set("cursor/grab", ECORE_X_CURSOR_HAND2);
    m_cursorStringMap.set("cursor/grabbing", ECORE_X_CURSOR_HAND2);
}

int getEcoreCursor(const String& cursorString)
{
    DEFINE_STATIC_LOCAL(CursorMap, cursorStringMap, ());

    return cursorStringMap.cursor(cursorString);
}
#endif

void applyFallbackCursor(Ecore_Evas* ecoreEvas, const char* cursorString)
{
#ifdef HAVE_ECORE_X
    int shape = getEcoreCursor(cursorString);
    if (shape < ECORE_X_CURSOR_X || shape > ECORE_X_CURSOR_XTERM) {
        LOG_ERROR("cannot map an equivalent X cursor for"
                  " c ursor group %s", cursorString);
        shape = ECORE_X_CURSOR_LEFT_PTR;
    }

    Ecore_X_Window window = 0;
    const char* engine = ecore_evas_engine_name_get(ecoreEvas);
#if USE(ACCELERATED_COMPOSITING)
    if (engine && !strcmp(engine, "opengl_x11"))
        window = ecore_evas_gl_x11_window_get(ecoreEvas);
    // Fallback to software mode if necessary.
    if (!window)
#endif
    {
        if (engine && !strcmp(engine, "software_x11"))
            window = ecore_evas_software_x11_window_get(ecoreEvas);
    }

    Ecore_X_Cursor cursor = ecore_x_cursor_shape_get(shape);
    ecore_x_window_cursor_set(window, cursor);
#endif
}

int getDPI()
{
#ifdef HAVE_ECORE_X
    return ecore_x_dpi_get();
#else
    return 160;
#endif
}

int getPixelDepth(const Evas* evas)
{
#ifdef HAVE_ECORE_X
    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(evas);
    // FIXME: ecore_evas_software_x11_window_get() can't get Ecore_X_Window during the layout test.
    // Because, EFL DumpRenderTree doesn't use X11 window by default.
    // See also, http://trac.webkit.org/browser/trunk/Tools/DumpRenderTree/efl/DumpRenderTree.cpp#L69
    Ecore_X_Window window = 0;
    const char* engine = ecore_evas_engine_name_get(ecoreEvas);
    if (engine) {
        if (!strcmp(engine, "opengl_x11"))
            window = ecore_evas_gl_x11_window_get(ecoreEvas);
        else if (!strcmp(engine, "software_x11"))
            window = ecore_evas_software_x11_window_get(ecoreEvas);
    }

    if (!window)
        return 8;

    return ecore_x_window_depth_get(window);
#else
    return 8;
#endif
}

#if ENABLE(TIZEN_VIEWPORT_META_TAG)

static const int defaultLDPI = 120;
static const int defaultMDPI = 160;
static const int defaultHDPI = 240;
static const int defaultXHDPI = 320;
static const int defaultXXHDPI = 480;
static const int defaultXXXHDPI = 640;

static const float minimumTabletScreenWidthInInch = 3.3;

IntSize getDefaultScreenResolution()
{
    int width = 0;
    int height = 0;
#ifdef HAVE_ECORE_X
    ecore_x_screen_size_get(ecore_x_default_screen_get(), &width, &height);
#endif
    return IntSize(width, height);
}

int getMobileDPI()
{
    IntSize resolution = getDefaultScreenResolution();
    if (resolution.isEmpty())
        return defaultMDPI;

    int screenDPI = getDPI();
    int resolutionWidth = (resolution.width() < resolution.height()) ? resolution.width() : resolution.height();
    float screenWidthInInch = static_cast<float>(resolution.width()) / screenDPI;

    // Assume that phone screen width is under 3.1 inch(Galaxy Mega 6.3)
    if (screenWidthInInch < minimumTabletScreenWidthInInch) {
        if (resolutionWidth < 300)
            return defaultLDPI;
        if (resolutionWidth < 400)
            return defaultMDPI;
        if (resolutionWidth < 600)
            return defaultHDPI;
        if (resolutionWidth < 800)
            return defaultXHDPI;
        if (resolutionWidth < 1200)
            return defaultXXHDPI;
        return defaultXXXHDPI;
    }

    // For tablet screen, currently support only MDPI
    return defaultMDPI;
}
#endif

bool isUsingEcoreX(const Evas* evas)
{
#ifdef HAVE_ECORE_X
    Ecore_Evas* ecoreEvas = ecore_evas_ecore_evas_get(evas);
    const char* engine = ecore_evas_engine_name_get(ecoreEvas);
    return !strcmp(engine, "opengl_x11")
        || !strcmp(engine, "software_x11")
        || !strcmp(engine, "software_xcb")
        || !strcmp(engine, "software_16_x11")
        || !strncmp(engine, "xrender", sizeof("xrender") - 1);
#else
    return false;
#endif
}

#if ENABLE(TIZEN_WEBKIT2_CONTEXT_X_WINDOW)
static Ecore_X_Window s_xWindow;

void setXWindow(Ecore_X_Window xWindow)
{
    s_xWindow = xWindow;
}

Ecore_X_Window getXWindow()
{
    return s_xWindow;
}

bool isMiniWindowMode(Ecore_X_Window xWindow)
{
    if (!xWindow)
        xWindow = getXWindow();

    if (xWindow && ecore_x_e_illume_window_state_get(xWindow) == ECORE_X_ILLUME_WINDOW_STATE_FLOATING)
        return true;

    return false;
}

bool isMultiWindowMode(Ecore_X_Window xWindow)
{
    unsigned int id = 0;
    if (!xWindow)
        xWindow = getXWindow();

    if (xWindow && ecore_x_window_prop_card32_get(xWindow, 0/*ECORE_X_ATOM_E_WINDOW_DESKTOP_LAYOUT*/, &id, 1) != -1 )
        if (id == 0 || id == 1)
            return true;

    return false;
}
#endif

} // namespace WebCore
