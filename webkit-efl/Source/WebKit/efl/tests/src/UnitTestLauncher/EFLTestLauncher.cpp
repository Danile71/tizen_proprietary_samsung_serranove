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
#include "EFLTestLauncher.h"

#include "EFLTestView.h"
#include "EFLTestUtility.h"
#include "EWebKit.h"

#include <Ecore.h>
#include <Edje.h>
#include <Eina.h>
#include <Elementary.h>

#define TEMPORARY_FIXING_UNIT_TESTS 0

namespace EFLUnitTests {

Eina_Bool EFLTestLauncher::init()
{
    if (!ecore_evas_init())
        return EINA_FALSE;

    if (!edje_init()) {
        ecore_evas_shutdown();
        return EINA_FALSE;
    }
    ewk_init();
    return EINA_TRUE;
}

void EFLTestLauncher::startTest()
{
    ecore_main_loop_begin();
}

void EFLTestLauncher::endTest()
{
    elm_shutdown();
    ecore_main_loop_quit();
}

Eina_Bool EFLTestLauncher::createTest(const char* url, void (*event_callback)(void*, Evas_Object*, void*), const char* event_name, void* event_data)
{
    EFL_INIT_RET();

#if TEMPORARY_FIXING_UNIT_TESTS

    elm_init(0, 0);

    EFLTestEcoreEvas* evas = new EFLTestEcoreEvas;
    if (!evas->getEvas())
        return EINA_FALSE;
    evas->show();

    EFLTestView* view;
    if (!url)
        view = new EFLTestView(evas->getEvas());
    else
        view = new EFLTestView(evas->getEvas(), url);

    if (!view->init())
        return EINA_FALSE;

    view->bindEvents(event_callback, event_name, event_data);
    view->show();
#else

    elm_init(0, 0);

    EFLTestEcoreEvas evas;
    if (!evas.getEvas())
        return EINA_FALSE;
    evas.show();

    EFLTestView view(evas.getEvas(), url);
    if (!view.init())
        return EINA_FALSE;
    view.bindEvents(event_callback, event_name, event_data);
    view.show();
#endif

    START_TEST();

    return EINA_TRUE;
}

Eina_Bool EFLTestLauncher::runTest(void (*event_callback)(void*, Evas_Object*, void*), const char* event_name, void* event_data)
{
    return createTest(0, event_callback, event_name, event_data);
}

Eina_Bool EFLTestLauncher::runTest(const char* url, void (*event_callback)(void*, Evas_Object*, void*), const char* event_name, void* event_data)
{
    return createTest(url, event_callback, event_name, event_data);
}

}
