#include "EFLTestLauncher.h"
#include "EWebKit.h"
#include <gtest/gtest.h>
#include <Ecore.h>

using namespace EFLUnitTests;

/**
* Callback function to a text search unit test.
*
* Function performs a few text searches and checks it results.
*
* @param eventInfo unused
* @param o pointer to a view object created for test purposes
* @param unused
*
* @see ewk_view_text_search()
*/

void textSearchTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    Eina_Bool searchResult;

    //three Eina_Bool parameters of ewk_view_test_search() function stand accordingly for: case sensitiveness, forward search, wrap at the end
    searchResult = ewk_view_text_search(o, "demos", EINA_FALSE, EINA_FALSE, EINA_FALSE);
    EXPECT_EQ(EINA_TRUE, searchResult);
    searchResult = ewk_view_text_search(o, "demos", EINA_FALSE, EINA_FALSE, EINA_TRUE);
    EXPECT_EQ(EINA_TRUE, searchResult);
    searchResult = ewk_view_text_search(o, "demos", EINA_FALSE, EINA_TRUE, EINA_FALSE);
    EXPECT_EQ(EINA_FALSE, searchResult); //search does not wrap at the end thus string cannot be found
    searchResult = ewk_view_text_search(o, "demos", EINA_FALSE, EINA_TRUE, EINA_TRUE);
    EXPECT_EQ(EINA_TRUE, searchResult);
    searchResult = ewk_view_text_search(o, "demos", EINA_TRUE, EINA_TRUE, EINA_TRUE);
    EXPECT_EQ(EINA_FALSE, searchResult);
    searchResult = ewk_view_text_search(o, "demos", EINA_TRUE, EINA_TRUE, EINA_FALSE);
    EXPECT_EQ(EINA_FALSE, searchResult);
    searchResult = ewk_view_text_search(o, "demos", EINA_TRUE, EINA_FALSE, EINA_TRUE);
    EXPECT_EQ(EINA_FALSE, searchResult);
    searchResult = ewk_view_text_search(o, "demos", EINA_TRUE, EINA_FALSE, EINA_FALSE);
    EXPECT_EQ(EINA_FALSE, searchResult);

    searchResult = ewk_view_text_search(o, "More information", EINA_FALSE, EINA_FALSE, EINA_TRUE);
    EXPECT_EQ(EINA_TRUE, searchResult);
    END_TEST();
}

/**
* Callback function to a strange text search unit test.
*
* Function performs a few text searches and checks it results.
*
* @param eventInfo unused
* @param o pointer to a view object created for test purposes
* @param unused
*
* @see ewk_view_text_search()
*/
void strangeTextSearchTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    Eina_Bool searchResult;

    searchResult = ewk_view_text_search(o, "somethingthatisveryunlikelytobefoundonanywebsite", EINA_FALSE, EINA_TRUE, EINA_FALSE);
    EXPECT_EQ(EINA_FALSE, searchResult);
    searchResult = ewk_view_text_search(o, "somethingthatisveryunlikelytobefoundonanywebsite", EINA_TRUE, EINA_TRUE, EINA_FALSE);
    EXPECT_EQ(EINA_FALSE, searchResult);

    searchResult = ewk_view_text_search(o, "92%", EINA_FALSE, EINA_FALSE, EINA_TRUE);
    EXPECT_EQ(EINA_TRUE, searchResult);
    searchResult = ewk_view_text_search(o, "@", EINA_FALSE, EINA_FALSE, EINA_TRUE);
    EXPECT_EQ(EINA_TRUE, searchResult);
    END_TEST();
}

/**
* Callback function to a text highlight set unit test.
*
* Function checks if matches highlight set and get works properly.
*
* @param eventInfo unused
* @param o pointer to a view object created for test purposes
* @param unused
*
* @see ewk_view_text_search()
*/

void textHighlightSetTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    EXPECT_EQ(EINA_TRUE, ewk_view_text_matches_highlight_set(o, EINA_TRUE));
    EXPECT_EQ(EINA_TRUE, ewk_view_text_matches_highlight_get(o));
    END_TEST();
}

/**
* Callback function to a text highlight unset unit test.
*
* Function checks if matches highlight unset and get works properly.
*
* @param eventInfo unused
* @param o pointer to a view object created for test purposes
* @param unused
*
* @see ewk_view_text_search()
*/

void textHighlightUnsetTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    EXPECT_EQ(EINA_TRUE, ewk_view_text_matches_highlight_set(o, EINA_FALSE));
    EXPECT_EQ(EINA_FALSE, ewk_view_text_matches_highlight_get(o));
    END_TEST();
}

/**
* Callback function to a text mark unit test.
*
* Function checks if text marking is working properly
*
* @param eventInfo unused
* @param o pointer to a view object created for test purposes
* @param unused
*/

void markTextSearchTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    //ewk_view_text_matches_mark() function parameters stand accordingly for: view object, searched string, case sensitiveness, highlight matches, maximum number of matches
    EXPECT_NE(0, ewk_view_text_matches_mark(o, "demos", EINA_FALSE, EINA_TRUE, 0));
    EXPECT_EQ(0, ewk_view_text_matches_mark(o, "demos", EINA_TRUE, EINA_TRUE, 0));
    EXPECT_NE(0, ewk_view_text_matches_mark(o, "demos", EINA_FALSE, EINA_FALSE, 0));
    EXPECT_EQ(0, ewk_view_text_matches_mark(o, "demos", EINA_TRUE, EINA_FALSE, 0));

    EXPECT_EQ(0, ewk_view_text_matches_mark(o, "somethingveryunlikelytobefoundonanywebsiteespeciallyonwebkitorg", EINA_FALSE, EINA_TRUE, 10));
    EXPECT_EQ(0, ewk_view_text_matches_mark(o, "somethingveryunlikelytobefoundonanywebsiteespeciallyonwebkitorg", EINA_TRUE, EINA_TRUE, 10));
    EXPECT_EQ(0, ewk_view_text_matches_mark(o, "somethingveryunlikelytobefoundonanywebsiteespeciallyonwebkitorg", EINA_TRUE, EINA_FALSE, 10));
    EXPECT_EQ(0, ewk_view_text_matches_mark(o, "somethingveryunlikelytobefoundonanywebsiteespeciallyonwebkitorg", EINA_FALSE, EINA_FALSE, 10));

    EXPECT_EQ(EINA_TRUE, ewk_view_text_matches_unmark_all(o));
    END_TEST();
}

/**
* Text search unit test.
*
* Function prepares environments and starts a unit test.
* It tests ewk_view_text_search()
*
* @see textSearchTestCallback()
*/

TEST(EwkSearchTests, TextSearchTest)
{
    RUN_TEST("http://www.webkit.org/", textSearchTestCallback, "load,finished", 0);
}

/**
* Strange text search unit test.
*
* Function prepares environments and starts a unit test.
* It tests ewk_view_text_search()
*
* @see strangeTextSearchTestCallback()
*/

TEST(EwkSearchTests, StrangeTextSearchTest)
{
    RUN_TEST("http://planet.webkit.org", strangeTextSearchTestCallback, "load,finished", 0);
}

/**
* Text highlight set unit test.
*
* Function prepares environments and starts a unit test.
* It tests ewk_view_text_matches_highlight_set() and
* ewk_view_text_matches_highlight_get()
*
* @see textHighlightTestCallback()
*/

TEST(EwkSearchTests, HighlightSetTextSearchTest)
{
    RUN_TEST("http://www.webkit.org/", textHighlightSetTestCallback, "load,finished", 0);
}

/**
* Text highlight unset unit test.
*
* Function prepares environments and starts a unit test.
* It tests ewk_view_text_matches_highlight_set() and
* ewk_view_text_matches_highlight_get()
*
* @see textHighlightTestCallback()
*/

TEST(EwkSearchTests, HighlightUnsetTextSearchTest)
{
    RUN_TEST("http://www.webkit.org/", textHighlightUnsetTestCallback, "load,finished", 0);
}

/**
* Text mark unit test.
*
* Function prepares environments and starts a unit test.
* It tests ewk_view_text_matches_mark() and
* ewk_view_text_matches_unmark_all()
*
* @see markTextSearchTestCallbackg()
*/

TEST(EwkSearchTests, MarkTextSearchTest)
{
    RUN_TEST("http://www.webkit.org/", markTextSearchTestCallback, "load,finished", 0);
}

