#include "EFLTestLauncher.h"
#include "EWebKit.h"
#include <gtest/gtest.h>
#include <iostream>
#include <Ecore.h>
#include <time.h>

using namespace EFLUnitTests;

/**
* Callback for standard zoom unit test.
*
* Function is triggered by "load,finished" signal. It checks if
* ewk_view_zoom_get() and ewk_view_zoom_set() functions are working
* properly. It also measures the time of standard zoom.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void nonAnimatedZoomTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    Eina_Bool zoomStarted;
    double startTimeNonAnimatedZoom(0.0), endTimeNonAnimatedZoom(0.0);
    double nonAnimatedZoomTime(0.0);
    EXPECT_TRUE(abs(ewk_view_zoom_get(o) - 1) < 0.1);
    startTimeNonAnimatedZoom = ecore_time_get();
    zoomStarted = ewk_view_zoom_set(o, 2, 10, 10);
    endTimeNonAnimatedZoom = ecore_time_get();
    nonAnimatedZoomTime = endTimeNonAnimatedZoom - startTimeNonAnimatedZoom;
    EXPECT_GT(200, nonAnimatedZoomTime*1000);
    EXPECT_EQ(zoomStarted, EINA_TRUE);
    EXPECT_TRUE(abs(ewk_view_zoom_get(o) - 2) < 0.1);
    zoomStarted = ewk_view_zoom_set(o, 1, 50, 50);
    EXPECT_EQ(zoomStarted, EINA_TRUE);
    EXPECT_TRUE(abs(ewk_view_zoom_get(o) - 1) < 0.1);
    END_TEST();
}

/**
* Callback for smooth scale zoom unit test.
*
* Function is triggered by "load,finished" signal. It checks if
* ewk_view_zoom_weak_smooth_scale_get() and
* ewk_view_zoom_weak_smooth_scale_set() functions are working
* properly.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void smoothScaleZoomTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    EXPECT_EQ(EINA_FALSE, ewk_view_zoom_weak_smooth_scale_get(o));
    ewk_view_zoom_weak_smooth_scale_set(o, EINA_TRUE);
    EXPECT_EQ(EINA_TRUE, ewk_view_zoom_weak_smooth_scale_get(o));
    END_TEST();
}

/**
* Callback for weak zoom unit test.
*
* Function is triggered by "load,finished" signal. It checks if
* ewk_view_zoom_weak_set() function is working
* properly. It also measures the time of weak zoom.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void weakZoomTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    Eina_Bool zoomDone;
    double weakZoomStartTime(0.0), weakZoomEndTime(0.0);
    EXPECT_TRUE(abs(ewk_view_zoom_get(o) - 1) < 0.1);
    weakZoomStartTime = ecore_time_get();
    zoomDone = ewk_view_zoom_weak_set(o, 2, 10, 10);
    weakZoomEndTime = ecore_time_get();
    EXPECT_EQ(EINA_TRUE, zoomDone);
    END_TEST();
}

/**
* Callback function to animated zoom value test.
*
* Function is triggered by "zoom,animated,end" signal. It checks if zoom is
* set in proper value after "zoom,animated,end" signal.
*
* @param eventInfo unused.
* @param o pointer to a view object created for test purposes.
* @param data unused.
*/

void onEndAnimatedZoomCheckCallback(void* eventInfo, Evas_Object* o,
                                    void* data)
{
    evas_object_smart_callback_del(o, "zoom,animated,end",
                                   onEndAnimatedZoomCheckCallback);
    EXPECT_TRUE(abs(ewk_view_zoom_get(o) - 2) < 0.1);
    END_TEST();
}

/**
* Callback function to animated zoom time test.
*
* Function is triggered by "zoom,animated,end" signal. It checks if zoom was
* performed in specific time andstarts another zoom to check if demanded
* value is reached.
*
* @param eventInfo pointer to a double value which holds time when animated
*        zoom was started.
* @param o pointer to a view created for test purposes.
* @param data unused.
*
* @see onEndAnimatedZoomCheckCallback()
*/

void onEndAnimatedZoomTimeCheckCallback(void* eventInfo, Evas_Object* o,
                                        void* data)
{
    double endTimeAnimatedZoom = ecore_time_get();
    Eina_Bool animatedZoomStarted;
    evas_object_smart_callback_del(o, "zoom,animated,end",
                                   onEndAnimatedZoomTimeCheckCallback);
    EXPECT_TRUE((endTimeAnimatedZoom - *(double*)eventInfo > 3
                && (endTimeAnimatedZoom - *(double*)eventInfo) < 3.1));
    animatedZoomStarted = ewk_view_zoom_animated_set(o, 2, 3, 10, 10);
    delete (double*)eventInfo;
    evas_object_smart_callback_add(o, "zoom,animated,end",
                                   onEndAnimatedZoomCheckCallback, 0);
}

/**
* Callback to an animated zoom unit test.
*
* Function is triggered by "load,finished" signal. It starts animated zoom,
* catch starting time and create new callback when zoom is end. Also checks if
* zoom was started.
*
* @param eventInfo unused.
* @param o pointer to a view object created for test purposes.
* @param data unused.
*
* @see onEndAnimatedZoomTimeCheckCallback()
*/

void animatedZoomTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    Eina_Bool animatedZoomStarted;
    double* startTimeAnimatedZoom;
    startTimeAnimatedZoom = new double;
    animatedZoomStarted = ewk_view_zoom_animated_set(o, 1.5, 3, 0, 0);
    *startTimeAnimatedZoom = ecore_time_get();
    EXPECT_EQ(EINA_TRUE, animatedZoomStarted);
    if (animatedZoomStarted) {
        evas_object_smart_callback_add(o, "zoom,animated,end",
                                       onEndAnimatedZoomTimeCheckCallback,
                                       startTimeAnimatedZoom);
    } else {
        END_TEST();
    }
}

/**
* Callback for text-only zoom unit test.
*
* Function is triggered by "load,finished" signal. It checks if
* ewk_view_zoom_text_only_get() and ewk_view_zoom_text_only_set()
* functions are working properly.
*
* @param eventInfo unused.
* @param o pointer to a view created for testing purposes.
* @param data unused.
*/

void zoomTextOnlyTestCallback(void* eventInfo, Evas_Object* o, void* data)
{
    Eina_Bool zoomTextStatus;
    //zoomTextStatus = ewk_view_zoom_text_only_get(o);
    EXPECT_EQ(EINA_FALSE, zoomTextStatus);
    //ewk_view_zoom_text_only_set(o, EINA_TRUE);
    //zoomTextStatus = ewk_view_zoom_text_only_get(o);
    EXPECT_EQ(EINA_TRUE, zoomTextStatus);
    END_TEST();
}

/**
* ewk_view_zoom_get() and ewk_view_zoom_set() unit test.
*
* Function prepares environment and starts a unit test.
*
* @see nonAnimatedZoomTestCallback()
*/

TEST(EwkZoomTests, NonAnimatedZoomTest)
{
    RUN_TEST("http://www.webkit.org/", nonAnimatedZoomTestCallback,
             "load,finished", 0);
}

/**
* Weak smooth scale zoom unit test.
*
* Function prepares environment and starts a unit test.
* Tests ewk_view_weak_smooth_scale_zoom_get() and
* ewk_view_weak_smooth_scale_zoom_set() functions.
*
* @see smoothScaleZoomTestCallback()
*/

TEST(EwkZoomTests, SmoothScaleZoomTest)
{
    RUN_TEST("http://www.webkit.org/", smoothScaleZoomTestCallback,
             "load,finished", 0);
}

/**
* Weak zoom unit test.
*
* Function prepares environment and starts a unit test.
* Tests ewk_view_weak_zoom_get() function.
*
* @see weakZoomTestCallback()
*/

TEST(EwkZoomTests, WeakZoomTest)
{
    RUN_TEST("http://www.webkit.org/", weakZoomTestCallback,
             "load,finished", 0);
}

/**
* Animated zoom unit test
*
* Function that prepares environment and starts an animated zoom unit test.
* Tests ewk_view_animated_zoom_set().
*
* @see animatedZoomTestCallback()
*/

TEST(EwkZoomTests, AnimatedZoomTest)
{
    RUN_TEST("http://www.webkit.org/", animatedZoomTestCallback,
             "load,finished", 0);
}

/**
* Text-only zoom unit test.
*
* Function prepares environment and starts a unit test.
* Tests ewk_view_text_only_zoom_get() and
* ewk_view_text_only_zoom_set() functions.
*
* @see zoomTextOnlyTestCallback()
*/

TEST(EwkZoomTests, TextOnlyZoomTest)
{
    RUN_TEST("http://www.webkit.org/", zoomTextOnlyTestCallback,
             "load,finished", 0);
}
