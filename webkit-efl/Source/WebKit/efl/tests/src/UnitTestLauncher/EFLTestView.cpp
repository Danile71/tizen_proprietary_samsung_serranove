/*
    Copyright (C) 2011 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
*/
#include "EFLTestView.h"

#include "EFLTestConfig.h"
#include "EFLTestUtility.h"
#include "EWebKit.h"

#include <Ecore.h>
#include <Ecore_File.h>
#include <Edje.h>
#include <Eina.h>

namespace EFLUnitTests {

EFLTestEcoreEvas::EFLTestEcoreEvas()
    : m_ecore_evas(0)
{
    m_ecore_evas = ecore_evas_new(0, 0, 0, EFLUnitTests::Config::defaultViewWidth, EFLUnitTests::Config::defaultViewHeight, 0);
}

EFLTestEcoreEvas::EFLTestEcoreEvas(const char *engine_name, int viewport_x, int viewport_y, int viewport_w, int viewport_h, const char *extra_options)
    : m_ecore_evas(0)
{
    m_ecore_evas = ecore_evas_new(engine_name, viewport_x, viewport_y, viewport_w, viewport_h, extra_options);
}

EFLTestEcoreEvas::~EFLTestEcoreEvas()
{
    if (m_ecore_evas)
        ecore_evas_free(m_ecore_evas);
}

Evas* EFLTestEcoreEvas::getEvas()
{
    if (m_ecore_evas)
        return ecore_evas_get(m_ecore_evas);
    return 0;
}

void EFLTestEcoreEvas::show()
{
    if (m_ecore_evas)
        ecore_evas_show(m_ecore_evas);
}

EFLTestView::EFLTestView(Evas* evas)
    : m_webView(0)
    , m_evas(evas)
    , m_url(0)
    , m_defaultViewType(TiledView)
    , m_width(EFLUnitTests::Config::defaultViewWidth)
    , m_height(EFLUnitTests::Config::defaultViewHeight)
{
    m_url = EFLTestUtility::createDefaultUrlPath(EFLTestUtility::DefaultTestPage);
}

EFLTestView::EFLTestView(Evas *evas, const char* url)
    : m_webView(0)
    , m_evas(evas)
    , m_url(0)
    , m_defaultViewType(TiledView)
    , m_width(EFLUnitTests::Config::defaultViewWidth)
    , m_height(EFLUnitTests::Config::defaultViewHeight)
{
    m_url = createTestUrl(url);
}

EFLTestView::EFLTestView(Evas *evas, EwkViewType type, const char* url)
    : m_webView(0)
    , m_evas(evas)
    , m_url(0)
    , m_defaultViewType(type)
    , m_width(EFLUnitTests::Config::defaultViewWidth)
    , m_height(EFLUnitTests::Config::defaultViewHeight)
{
    m_url = createTestUrl(url);
}

EFLTestView::EFLTestView(Evas *evas, EwkViewType type, const char* url, int width, int height)
    : m_webView(0)
    , m_evas(evas)
    , m_url(0)
    , m_defaultViewType(type)
    , m_width(width)
    , m_height(height)
{
    m_url = createTestUrl(url);
}

EFLTestView::~EFLTestView()
{
    if (m_webView)
        evas_object_del(m_webView);

    free(m_url);
}

char* EFLTestView::createTestUrl(const char* url)
{
    if (url)
        return strdup(url);
    return 0;
}

Eina_Bool EFLTestView::init()
{
    if (!m_evas || !m_url)
        return EINA_FALSE;

    switch (m_defaultViewType) {
    case SingleView:
        m_webView = ewk_view_single_add(m_evas);
        break;

    case TiledView:
        m_webView = ewk_view_tiled_add(m_evas);
        break;
    }

    if (!m_webView)
        return EINA_FALSE;

    ewk_view_theme_set(m_webView, DEFAULT_WEBKIT_THEME);
    ewk_view_uri_set(m_webView, m_url);
}

void EFLTestView::show()
{
    if (!m_webView)
        return;

    evas_object_resize(m_webView, m_width, m_height);
    evas_object_show(m_webView);
    evas_object_focus_set(m_webView, EINA_TRUE);
}

Evas_Object* EFLTestView::getMainFrame()
{
    if (m_webView)
        return ewk_view_frame_main_get(m_webView);
    return 0;
}

Evas* EFLTestView::getEvas()
{
    if (m_webView)
        return evas_object_evas_get(m_webView);
    return 0;
}

void EFLTestView::bindEvents(void (*callback)(void*, Evas_Object*, void*), const char* eventName, void* ptr)
{
    if (!m_webView)
        return;

    evas_object_smart_callback_del(m_webView, eventName, callback);
    evas_object_smart_callback_add(m_webView, eventName, callback, ptr);
}

}
