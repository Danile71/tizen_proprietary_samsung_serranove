#include "EFLTestLauncher.h"
#include "EWebKit.h"
#include <gtest/gtest.h>
#include <iostream>

using namespace EFLUnitTests;

/**
* Callback function set by pageLoadedCallback().
*
* Function checks if basic history operations are working properly when
* there are three items in history. Then it ask a view to load next page
* from history.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void historyForwardCallback(void* eventInfo, Evas_Object* o, void* data)
{
    evas_object_smart_callback_del(o, "load,finished", historyForwardCallback);
    EXPECT_EQ(EINA_TRUE, ewk_view_back_possible(o));
    EXPECT_EQ(EINA_TRUE, ewk_view_forward_possible(o));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate_possible(o, 2));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate(o, -2));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate_possible(o, -2));
    END_TEST();
}

/**
* Callback function set by pageLoadedCallback().
*
* Function checks if basic history operations are working properly when
* there are three items in history. Then it ask a view to load next page
* from history.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void historyBackCallback(void* eventInfo, Evas_Object* o, void* data)
{
    evas_object_smart_callback_del(o, "load,finished", historyBackCallback);
    EXPECT_EQ(EINA_FALSE, ewk_view_back(o));
    EXPECT_EQ(EINA_FALSE, ewk_view_back_possible(o));
    EXPECT_EQ(EINA_TRUE, ewk_view_forward_possible(o));
    EXPECT_EQ(EINA_TRUE, ewk_view_navigate_possible(o, 2));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate(o, -2));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate_possible(o, -2));
    EXPECT_EQ(EINA_TRUE, ewk_view_forward(o));
    evas_object_smart_callback_add(o, "load,finished",
                                   historyForwardCallback, 0);
}

/**
* Callback function set by loadNextPageCallback().
*
* Function checks if basic history operations are working properly when
* there are three items in history. Then it ask a view to load -2 page
* from history.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void pageLoadedCallback(void* eventInfo, Evas_Object* o, void* data)
{
    evas_object_smart_callback_del(o, "load,finished", pageLoadedCallback);
    EXPECT_EQ(EINA_TRUE, ewk_view_back_possible(o));
    EXPECT_EQ(EINA_FALSE, ewk_view_forward(o));
    EXPECT_EQ(EINA_FALSE, ewk_view_forward_possible(o));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate(o, 2));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate_possible(o, 2));
    EXPECT_EQ(EINA_TRUE, ewk_view_navigate_possible(o, -2));
    EXPECT_EQ(EINA_TRUE, ewk_view_navigate(o, -2));
    evas_object_smart_callback_add(o, "load,finished", historyBackCallback, 0);
}

/**
* Callback function set by historyBackForwardTestCallback().
*
* Function load another page in order to create larger history.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void loadNextPageCallback(void* eventInfo, Evas_Object* o, void* data)
{
    evas_object_smart_callback_del(o, "load,finished", loadNextPageCallback);
    ewk_view_uri_set(o, "http://www.webkit.org/");
    evas_object_smart_callback_add(o, "load,finished", pageLoadedCallback, 0);
}

/**
* Callback to basic history operations unit test.
*
* Function checks basic operations on empty history. Then asks a view to load
* new page (which will be added to history) and sets a callback when page is
* loaded.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void historyBackForwardTestCallback(void* eventInfo, Evas_Object* o,
                                    void* data)
{
    evas_object_smart_callback_del(o, "load,finished",
                                   historyBackForwardTestCallback);
    EXPECT_EQ(EINA_FALSE, ewk_view_back(o));
    EXPECT_EQ(EINA_FALSE, ewk_view_back_possible(o));
    EXPECT_EQ(EINA_FALSE, ewk_view_forward(o));
    EXPECT_EQ(EINA_FALSE, ewk_view_forward_possible(o));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate(o, 2));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate_possible(o, 2));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate(o, -2));
    EXPECT_EQ(EINA_FALSE, ewk_view_navigate_possible(o, -2));
    ewk_view_uri_set(o, "http://www.google.pl/");
    evas_object_smart_callback_add(o, "load,finished",
                                   loadNextPageCallback, 0);
}

/**
* Callback to disabled history unit test.
*
* Function checks if navigation is possible when history is disabled.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*
* @see historyEnableTestCallback()
*/

void historyDisabledCallback(void* eventInfo, Evas_Object* o, void* data)
{
    evas_object_smart_callback_del(o, "load,finished",
                                   historyDisabledCallback);
    EXPECT_EQ(EINA_FALSE, ewk_view_back_possible(o));
    END_TEST();
}

/**
* Callback to history enable and disable unit test.
*
* Function checks if ewk_view_history_enable_set() and
* ewk_view_history_enable_get() work properly. Then asks a view to load
* next page and adds new callback when demanded website is fully loaded.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void historyEnableTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    evas_object_smart_callback_del(o, "load,finished",
                                   historyEnableTestCallback);
    EXPECT_EQ(EINA_TRUE, ewk_view_history_enable_get(o));
    EXPECT_EQ(EINA_TRUE, ewk_view_history_enable_set(o, EINA_FALSE));
    EXPECT_EQ(EINA_FALSE, ewk_view_history_enable_get(o));
    ewk_view_uri_set(o, "http://www.google.pl/");
    evas_object_smart_callback_add(o, "load,finished",
                                   historyDisabledCallback, 0);
}

/**
* Callback to history get unit test.
*
* Function checks if ewk_view_history_get() returns proper values.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void historyGetTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    EXPECT_NE((Ewk_History *) 0, ewk_view_history_get(o));
    END_TEST();
}

/**
* Unit test for basic history operations.
*
* Function prepares environment and starts unit test.
* It tests ewk_view_history_back_possible(),
* ewk_view_history_forward_possible(), ewk_view_navigate_possible(),
* ewk_view_history_navigate().
*/

TEST(DISABLED_EwkHistoryTests, HistoryBackForwardTest)
{
    RUN_TEST("http://www.webkit.org/", historyBackForwardTestCallback,
             "load,finished", 0);
}

/**
* Unit test for enabling and disabling history.
*
* Function prepares environment and start unit test. It tests
* ewk_view_history_enable_get() and ewk_view_history_enable_set().
*/

TEST(EwkHistoryTests, HistoryEnableTest)
{
    RUN_TEST("http://www.webkit.org/", historyEnableTestCallback,
             "load,finished", 0);
}

/**
* Unit test for getting history.
*
* Function prepares environment and start unit test. It tests
* ewk_view_history_get().
*/

TEST(EwkHistoryTests, HistoryGetTest)
{
    RUN_TEST("http://www.webkit.org/", historyGetTestCallback,
             "load,finished", 0);
}
