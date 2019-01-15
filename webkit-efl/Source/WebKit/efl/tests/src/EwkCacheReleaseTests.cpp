#include "EWebKit.h"
#include "EFLTestLauncher.h"
#include <gtest/gtest.h>

using namespace EFLUnitTests;

static void onEwkFontReleaseTest(void* data, Evas_Object* webView, void* event_info)
{
    EXPECT_EQ(true, ewk_cache_font_invalidate());
    END_TEST();
}

static void onEwkPageReleaseTest(void* data, Evas_Object* webView, void* event_info)
{
    EXPECT_EQ(true, ewk_cache_page_release(1));
    END_TEST();
}

static void onEwkComponentReleaseSetDisabled(void* data, Evas_Object* webView, void* event_info)
{
    EXPECT_EQ(true, ewk_cache_component_reset());
    END_TEST();
}

static void onEwkComponentReleasePrune(void* data, Evas_Object* webView, void* event_info)
{
    EXPECT_EQ(true, ewk_cache_component_prune());
    END_TEST();
}

static void onEwkPageCacheSetGetCapacity(void* data, Evas_Object* webView, void* event_info)
{
    ewk_cache_page_capacity_set(1);

    int cacheCapacity = ewk_cache_page_capacity_get();
    EXPECT_EQ(1, cacheCapacity);

    END_TEST();
}

TEST(EwkCacheReleaseTests, EwkFontReleaseTest)
{
    RUN_TEST("http://www.webkit.org", onEwkFontReleaseTest, "load,finished", NULL);
}

TEST(EwkCacheReleaseTests, EwkPageReleaseTest)
{
    RUN_TEST("http://www.webkit.org", onEwkPageReleaseTest, "load,finished", NULL);
}

TEST(EwkCacheReleaseTests, EwkComponentReleaseTest_1)
{
    RUN_TEST("http://www.webkit.org", onEwkComponentReleaseSetDisabled, "load,finished", NULL);
}

TEST(EwkCacheReleaseTests, EwkComponentReleaseTest_2)
{
    RUN_TEST("http://www.webkit.org", onEwkComponentReleasePrune, "load,finished", NULL);
}

TEST(EwkCaseReleaseTests, EwkPageCacheSetCapacity)
{
    RUN_TEST("http://www.webkit.org", onEwkPageCacheSetGetCapacity, "load,finished", NULL);
}
