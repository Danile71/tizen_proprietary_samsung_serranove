//#include "config.h"

#include <Ecore.h>
#include <gtest/gtest.h>
#include "EFLTestLauncher.h"
#include "EFLTestView.h"
#include "EWebKit.h"
#include "ewk_tiled_model.h"

//#if ENABLE(TIZEN_TILED_GLOBAL_CACHE_MANAGMENT)

using namespace EFLUnitTests;

TEST(EwkBackingstoreCacheGlobalManagment, CacheReuseDiffrentEvasTest)
{
    ewk_tile_show(NULL);
    EFL_INIT();

    EFLTestEcoreEvas evas1;
    EFLTestEcoreEvas evas2;
    EFLTestEcoreEvas evas3;

    char *url = "http://www.webkit.org";
    EFLTestView view1(evas1.getEvas(), url);
    EFLTestView view2(evas2.getEvas(), url);
    EFLTestView view3(evas3.getEvas(), url);

    Ewk_Tile_Unused_Cache *cache1 = ewk_view_tiled_unused_cache_get(view1.getWebView());
    if (!cache1) {
        EXPECT_EQ(true, false);
        return;
    }

    Ewk_Tile_Unused_Cache *cache2 = ewk_view_tiled_unused_cache_get(view2.getWebView());
    if (!cache2) {
        EXPECT_EQ(true, false);
        return;
    }

    Ewk_Tile_Unused_Cache *cache3 = ewk_view_tiled_unused_cache_get(view3.getWebView());
    if (!cache3) {
        EXPECT_EQ(true, false);
        return;
    }

    // each cache should be diffrent if not return FALSE
    if (cache1 == cache2 || cache1 == cache3 || cache2 == cache3) {
        EXPECT_EQ(true, false);
        return;
    }
}

TEST(EwkBackingstoreCacheGlobalManagment, CacheReuseOneEvasTest)
{
    EFL_INIT();

    EFLTestEcoreEvas evas;

    char *url = "http://www.webkit.org";
    EFLTestView view1(evas.getEvas(), url);
    EFLTestView view2(evas.getEvas(), url);
    EFLTestView view3(evas.getEvas(), url);

    bool result = true;
    Ewk_Tile_Unused_Cache *cache1 = ewk_view_tiled_unused_cache_get(view1.getWebView());
    if (!cache1) {
        EXPECT_EQ(true, false);
        return;
    }

    Ewk_Tile_Unused_Cache *cache2 = ewk_view_tiled_unused_cache_get(view2.getWebView());
    if (!cache2) {
        EXPECT_EQ(true, false);
        return;
    }

    Ewk_Tile_Unused_Cache *cache3 = ewk_view_tiled_unused_cache_get(view3.getWebView());
    if (!cache3)  {
        EXPECT_EQ(true, false);
        return;
    }

    // each cache should be equal if not return FALSE
    if (cache1 != cache2 || cache1 != cache3 || cache2 != cache3) {
        EXPECT_EQ(true, false);
        return;
    }
}

const int count = 5;
int loaded = 0;

static Eina_Bool onClose(void *data)
{
    loaded = 0;
    END_TEST();
    return EINA_FALSE;
}

static void onLoadedWithScrollCb(void* data, Evas_Object* webView, void* event_info)
{
    loaded++;

    int w, h;
    ewk_frame_contents_size_get(ewk_view_frame_main_get(webView), &w, &h);
    ewk_frame_scroll_add(ewk_view_frame_main_get(webView), w, h);

    if (loaded == count) {
        ecore_timer_add(3, onClose, 0);
    }
}

TEST(EwkBackingstoreCacheGlobalManagment, OneCacheMaxAllocTest)
{
    EFLTestView *views[count];

    EFL_INIT();
    EFLTestEcoreEvas evas;
    evas.show();

    char *url = "http://www.gmail.pl";
    for (int i = 0; i < count; ++i) {
        views[i] = new EFLTestView(evas.getEvas(), url);
        views[i]->bindEvents(onLoadedWithScrollCb, "load,finished", NULL);
    }

    START_TEST();

    Ewk_Tile_Unused_Cache *cache = ewk_view_tiled_unused_cache_get(views[0]->getWebView());

    int max = ewk_tile_unused_cache_max_get(cache);
    int used = ewk_tile_unused_cache_used_get(cache);

    if (used > max)
        EXPECT_EQ(false, true);

    for (int i = 0; i < count; ++i)
        delete views[i];
}

TEST(EwkBackingstoreCacheGlobalManagment, TwoCachesNotReusable)
{
    EFL_INIT();
    EFLTestEcoreEvas evas;
    evas.show();

    Ewk_Tile_Unused_Cache *cache1 = ewk_tile_unused_cache_new(evas.getEvas(), EINA_FALSE, 1024);
    Ewk_Tile_Unused_Cache *cache2 = ewk_tile_unused_cache_new(evas.getEvas(), EINA_FALSE, 1024);

    if (cache1 == cache2)
        EXPECT_EQ(true, false);
}

TEST(EwkBackingstoreCacheGlobalManagment, TwoCachesReusable)
{
    EFL_INIT();
    EFLTestEcoreEvas evas;
    evas.show();

    Ewk_Tile_Unused_Cache *cache1 = ewk_tile_unused_cache_new(evas.getEvas(), EINA_TRUE, 1024);
    Ewk_Tile_Unused_Cache *cache2 = ewk_tile_unused_cache_new(evas.getEvas(), EINA_TRUE, 1024);

    if (cache1 != cache2)
        EXPECT_EQ(true, false);
}

TEST(EwkBackingstoreCacheGlobalManagment, FewCachesMaxAllocTest)
{
    EFLTestView *views[count];

    EFL_INIT();
    EFLTestEcoreEvas evas;
    evas.show();

    int MAX = 4 * 1024 * 1024;
    char *url = "http://www.gmail.pl";
    for (int i = 0; i < count; ++i) {
        views[i] = new EFLTestView(evas.getEvas(), url);

        views[i]->bindEvents(onLoadedWithScrollCb, "load,finished", NULL);

        EAPI Ewk_Tile_Unused_Cache *cache = ewk_tile_unused_cache_new(evas.getEvas(), EINA_FALSE, MAX);
        ewk_view_tiled_unused_cache_set(views[i]->getWebView(), cache);
    }

    START_TEST();

    int max = 0;
    for (int i = 0; i < count; ++i) {
        Ewk_Tile_Unused_Cache *cache = ewk_view_tiled_unused_cache_get(views[i]->getWebView());
        max += ewk_tile_unused_cache_used_get(cache);
        fprintf(stderr, "* cache: %p used: %d total:%d\n", cache, ewk_tile_unused_cache_used_get(cache), max);
    }

    if (max > MAX)
        EXPECT_EQ(true, false);

    for (int i = 0; i < count; ++i)
        delete views[i];
}

/*TEST(EwkBackingstoreCacheGlobalManagment, FewCachesMaxAllocTestWithChangedMax)
{
    UTEwkView *views[count];

    UTLauncher::eflInit();
    UTEcoreEvas evas;
    evas.show();

    char *url = "http://www.gmail.pl";
    int LOW_MAX = 2 * 1024 * 1024;
    int HIG_MAX = 4 * 1024 * 1024;

    for (int i = 0; i < count; ++i) {
        views[i] = new UTEwkView(evas.getEvas(), url);

        views[i]->bindEvents(onLoadedWithScrollCb, "load,finished", NULL);

        Ewk_Tile2_Unused_Cache *cache = ewk_tile2_unused_cache_new(evas.getEvas(), EINA_FALSE, LOW_MAX);
        ewk_view_tiled2_unused_cache_set(views[i]->getWebView(), cache);

        if (i == 0)
            ewk_tile2_unused_cache_max_set(cache, HIG_MAX);
    }

    UTLauncher::eflBeginLoop();

    int max = 0;
    for (int i = 0; i < count; ++i) {
        Ewk_Tile2_Unused_Cache *cache = ewk_view_tiled2_unused_cache_get(views[i]->getWebView());
        max += ewk_tile2_unused_cache_used_get(cache);
        fprintf(stderr, "* cache: %p used: %d total:%d\n", cache, ewk_tile2_unused_cache_used_get(cache), max);
    }

    if (max > HIG_MAX)
        EXPECT_EQ(true, false);

    for (int i = 0; i < count; ++i)
        delete views[i];
}*/

//#endif
