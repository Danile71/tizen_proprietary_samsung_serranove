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
#ifndef EFLTestLauncher_h
#define EFLTestLauncher_h

#include <Evas.h>

#ifdef GTEST_TEST_FRAMEWORK
#include <gtest/gtest.h>
#endif

#ifdef GTEST_TEST_FRAMEWORK
    #define RUN_TEST(args...)   \
        do {    \
            ASSERT_EQ(EINA_TRUE, EFLTestLauncher::runTest(args)); \
        } while (0)
#else
    #define RUN_TEST(args...)   \
        do {    \
            EFLTestLauncher::runTest(args); \
        } while (0)
#endif

#define START_TEST()    \
    do {    \
        EFLTestLauncher::startTest();   \
    } while (0)

#define END_TEST()    \
    do {    \
        EFLTestLauncher::endTest(); \
    } while (0)

#define EFL_INIT()  \
    do {    \
        EFLTestLauncher::init();    \
    } while (0)

#define EFL_INIT_RET()  \
    do {    \
        if (!EFLTestLauncher::init())   \
            return EINA_FALSE;  \
    } while(0)

namespace EFLUnitTests {

class EFLTestLauncher {
    static Eina_Bool createTest(const char* url, void (*event_callback)(void*, Evas_Object*, void*), const char* event_name, void* event_data);
public:
    static Eina_Bool init();
    static void startTest();
    static void endTest();

    static Eina_Bool runTest(const char* url, void (*event_callback)(void*, Evas_Object*, void*), const char* event_name, void* event_data);
    static Eina_Bool runTest(void (*event_callback)(void*, Evas_Object*, void*), const char* event_name, void* event_data);
};

}

#endif
