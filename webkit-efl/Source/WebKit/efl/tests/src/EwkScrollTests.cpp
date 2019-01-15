#include "EFLTestLauncher.h"
#include "EWebKit.h"
#include <gtest/gtest.h>
#include <iostream>
#include <Ecore.h>
#include <time.h>

using namespace EFLUnitTests;

typedef struct _ScrollData {
int scrollPosX, scrollPosY, scrollDX, scrollDY;
Ecore_Timer* scrollTimer;
Evas_Object* obj;
double scrollTimeout;
} ScrollData;

/**
* Callback function for get scroll unit test.
*
* Function is triggered by "load,finished" signal and tests if
* ewk_frame_scroll_pos_get() function returns proper value.
*
* @param eventInfo pointer to data passed by RUN_TEST(), unused.
* @param o pointer to a view created for testing purposes.
* @param data pointer to data passed by evas_object_smart_callback_add(),
*        unused.
*/

void scrollGetCallback(void* eventInfo, Evas_Object* o, void* data)
{
    int scrollPosX, scrollPosY;
    ewk_frame_scroll_pos_get(ewk_view_frame_main_get(o),
                             &scrollPosX,
                             &scrollPosY);
    EXPECT_EQ(0, scrollPosX);
    EXPECT_EQ(0, scrollPosY);
    END_TEST();
}

/**
* Callback function for set scroll unit test.
*
* Function is triggered by "load,finished" signal and tests if
* ewk_frame_scroll_pos_set() function sets proper value.
*
* @param eventInfo pointer to data passed by RUN_TEST(), unused.
* @param o pointer to a view created for testing purposes.
* @param data pointer to data passed by evas_object_smart_callback_add(),
*        unused.
*/

void scrollSetCallback(void* eventInfo, Evas_Object* o, void* data)
{
    int scrollPosX, scrollPosY;
    ewk_frame_scroll_pos_get(ewk_view_frame_main_get(o),
                             &scrollPosX,
                             &scrollPosY);
    EXPECT_EQ(0, scrollPosX);
    EXPECT_EQ(0, scrollPosY);
    ewk_frame_scroll_set(ewk_view_frame_main_get(o), 10, 10);
    ewk_frame_scroll_pos_get(ewk_view_frame_main_get(o),
                             &scrollPosX,
                             &scrollPosY);
    EXPECT_EQ(10, scrollPosX);
    EXPECT_EQ(10, scrollPosY);
    END_TEST();
}

/**
* Callback function for timer set by scrollAddCallback() function.
*
* Function is triggerd when specified time has elapsed. It checks if
* scroll has changed.
*
* @param data structiure that includes: previous X and Y scrolls positions,
*        scroll timer pointer.
*
* @see scrollAddCallback()
*/

Eina_Bool onCloseTimerScrollAddCallback(void* data)
{
    int newScrollPosX(0), newScrollPosY(0);
    ecore_timer_del(((ScrollData*)data)->scrollTimer);
    ((ScrollData*)data)->scrollTimer = NULL;
    ewk_frame_scroll_pos_get(ewk_view_frame_main_get(((ScrollData*)data)->obj),
                             &newScrollPosX,
                             &newScrollPosY);
    EXPECT_EQ(((ScrollData*)data)->scrollPosX + ((ScrollData*)data)->scrollDX,
              newScrollPosX);
    EXPECT_EQ(((ScrollData*)data)->scrollPosY + ((ScrollData*)data)->scrollDY,
              newScrollPosY);
    END_TEST();
}
/**
* Callback function for add scroll unit test.
*
* Function is triggerd by "load,finished" signal. It starts
* ewk_frame_scroll_add() function and sets timer that will set off
* after 3 seconds when scrolling is probably over.
*
* @param eventInfo pointer to a structure that includes: pointer to a view
*        object, current X and Y scrolls positions, scroll timer pointer,
*        values that will be added to X and Y scrolls.
* @param o pointer to a view created for test purposes
* @param data unused.
*
* @see onCloseTimerScrollAddCallback()
*/
void scrollAddCallback(void* eventInfo, Evas_Object* o, void* data)
{
    ((ScrollData*)eventInfo)->obj = o;
    ewk_frame_scroll_pos_get(ewk_view_frame_main_get(o),
                             &(((ScrollData*)eventInfo)->scrollPosX),
                             &(((ScrollData*)eventInfo)->scrollPosY));
    ewk_frame_scroll_add(ewk_view_frame_main_get(o),
                         ((ScrollData*)eventInfo)->scrollDX,
                         ((ScrollData*)eventInfo)->scrollDY);
    ((ScrollData*)eventInfo)->scrollTimer = ecore_timer_add(3,
        onCloseTimerScrollAddCallback, eventInfo);
}

/**
* Callback function for scroll time unit test.
*
* Function that is triggered by "load,finished" signal. It measures time
* between ewk_frame_scroll_add() function call and actual scroll change.
*
* @param eventInfo pointer to a structure that includes: pointer to a view
*        object, current X and Y scrolls positions, scroll timer pointer,
*        values that will be added to X and Y scrolls.
* @param o pointer to a view created for test purposes
* @param data unused.
*/

void scrollTimeCallback(void* eventInfo, Evas_Object* o, void* data)
{
    int newScrollPosX, newScrollPosY;
    Evas_Object* main_frame;
    main_frame = ewk_view_frame_main_get(o);
    ewk_frame_scroll_pos_get(main_frame,
                             &(((ScrollData*)eventInfo)->scrollPosX),
                             &(((ScrollData*)eventInfo)->scrollPosY));
    newScrollPosX = ((ScrollData*)eventInfo)->scrollPosX;
    newScrollPosY = ((ScrollData*)eventInfo)->scrollPosY;
    double startTimeScroll = ecore_time_get();
    ewk_frame_scroll_add(main_frame,
                         ((ScrollData*)eventInfo)->scrollDX,
                         ((ScrollData*)eventInfo)->scrollDY);
    while (((((ScrollData*)eventInfo)->scrollPosX
           + ((ScrollData*)eventInfo)->scrollDX) != newScrollPosX)
           && ((((ScrollData*)eventInfo)->scrollPosY
           + ((ScrollData*)eventInfo)->scrollDY) != newScrollPosY))
    {
        ewk_frame_scroll_pos_get(main_frame,
                                 &newScrollPosX,
                                 &newScrollPosY);
        if (((ScrollData*)eventInfo)->scrollTimeout < (ecore_time_get()
            - startTimeScroll))
        {
            EXPECT_FALSE(((ScrollData*)eventInfo)->scrollTimeout
                         < (ecore_time_get() - startTimeScroll));
            break;
        }
    }
    double endTimeScroll = ecore_time_get();
    EXPECT_LT((endTimeScroll - startTimeScroll)*1000, 1);
    END_TEST();
}

/**
* Callback function for timer set by scrollMaxRightCallback()
*
* Function checks if previous scroll was done properly. Checks if horizontal
* scroll returned to starting position (0) and if vertical scroll is 0.
* Then it deletes created data and ends the test.
*
* @param data pointer to a ScrollData type variable that holds values needed
*        for proper scroll check and change. Fields scrollDX and scroll DY
*        contain maximum scroll values read in fullScrollCallback().
*
* @see scrollMaxRightCallback()
*/

Eina_Bool scrollMaxLeftCallback(void* data)
{
    ecore_timer_del(((ScrollData*)data)->scrollTimer);
    ((ScrollData*)data)->scrollTimer = NULL;
    ewk_frame_scroll_pos_get(ewk_view_frame_main_get(((ScrollData*)data)->obj),
                             &(((ScrollData*)data)->scrollPosX),
                             &(((ScrollData*)data)->scrollPosY));
    EXPECT_EQ(0, ((ScrollData*)data)->scrollPosX);
    EXPECT_EQ(0, ((ScrollData*)data)->scrollPosY);
    delete (ScrollData*)data;

    END_TEST();
}

/**
* Callback function for timer set by scrollMaxUpCallback()
*
* Function checks if previous scroll was done properly. Checks if horizontal
* scroll is maximum and if vertical scroll is 0.
* Then it adds negative maximum horizontal scroll to current scroll and sets timer
* that will trigger scrollMaxLeftCallback() function when scroll is probably
* finished.
*
* @param data pointer to a ScrollData type variable that holds values needed
*        for proper scroll check and change. Fields scrollDX and scroll DY
*        contain maximum scroll values read in fullScrollCallback().
*
* @see scrollMaxUpCallback()
* @see scrollMaxLeftCallback()
*/

Eina_Bool scrollMaxRightCallback(void* data)
{
    ecore_timer_del(((ScrollData*)data)->scrollTimer);
    ((ScrollData*)data)->scrollTimer = NULL;
    ewk_frame_scroll_pos_get(ewk_view_frame_main_get(((ScrollData*)data)->obj),
                             &(((ScrollData*)data)->scrollPosX),
                             &(((ScrollData*)data)->scrollPosY));
    EXPECT_EQ(((ScrollData*)data)->scrollDX, ((ScrollData*)data)->scrollPosX);
    EXPECT_EQ(0, ((ScrollData*)data)->scrollPosY);
    ewk_frame_scroll_add(ewk_view_frame_main_get(((ScrollData*)data)->obj),
                         -(((ScrollData*)data)->scrollDX),
                         0);
    ((ScrollData*)data)->scrollTimer = ecore_timer_add(3, scrollMaxLeftCallback,
                                                       data);
}

/**
* Callback function for timer set by scrollMaxDownCallback()
*
* Function checks if previous scroll was done properly. Checks if horizontal
* scroll is unchanged (0) and if vertical scroll returned to starting point 0.
* Then it adds maximum horizontal scroll to current scroll and sets timer
* that will trigger scrollMaxRightCallback() function when scroll is probably
* finished.
*
* @param data pointer to a ScrollData type variable that holds values needed
*        for proper scroll check and change. Fields scrollDX and scroll DY
*        contain maximum scroll values read in fullScrollCallback().
*
* @see scrollMaxDownCallback()
* @see scrollMaxRightCallback()
*/

Eina_Bool scrollMaxUpCallback(void* data)
{
    ecore_timer_del(((ScrollData*)data)->scrollTimer);
    ((ScrollData*)data)->scrollTimer = NULL;
    ewk_frame_scroll_pos_get(ewk_view_frame_main_get(((ScrollData*)data)->obj),
                             &(((ScrollData*)data)->scrollPosX),
                             &(((ScrollData*)data)->scrollPosY));
    EXPECT_EQ(0, ((ScrollData*)data)->scrollPosX);
    EXPECT_EQ(0, ((ScrollData*)data)->scrollPosY);
    ewk_frame_scroll_add(ewk_view_frame_main_get(((ScrollData*)data)->obj),
                         (((ScrollData*)data)->scrollDX),
                         0);
    ((ScrollData*)data)->scrollTimer = ecore_timer_add(
            3, scrollMaxRightCallback, data);
}

/**
* Callback function for timer set by fullScrollCallback()
*
* Function checks if previous scroll was done properly. Checks if horizontal
* scroll is unchanged (0) and if vertical scroll is maximum. Then it adds
* negative value of maximum vertical scroll to current scroll and sets timer
* that will trigger scrollMaxUpCallback() function when scroll is probably
* finished.
*
* @param data pointer to a ScrollData type variable that holds values needed
*        for proper scroll check and change. Fields scrollDX and scroll DY
*        contain maximum scroll values read in fullScrollCallback().
*
* @see fullScrollCallback()
* @see scrollMaxUpCallback()
*/

Eina_Bool scrollMaxDownCallback(void* data)
{
    ecore_timer_del(((ScrollData*)data)->scrollTimer);
    ((ScrollData*)data)->scrollTimer = NULL;
    ewk_frame_scroll_pos_get(ewk_view_frame_main_get(((ScrollData*)data)->obj),
                             &(((ScrollData*)data)->scrollPosX),
                             &(((ScrollData*)data)->scrollPosY));
    EXPECT_EQ(0, ((ScrollData*)data)->scrollPosX);
    EXPECT_EQ(((ScrollData*)data)->scrollDY, ((ScrollData*)data)->scrollPosY);
    ewk_frame_scroll_add(ewk_view_frame_main_get(((ScrollData*)data)->obj),
                         0,
                         -(((ScrollData*)data)->scrollDY));
    ((ScrollData*)data)->scrollTimer = ecore_timer_add(3, scrollMaxUpCallback,
                                                       data);
}
/**
* Callback function for full scroll unit test.
*
* Function creates necessary variables and reads maximum scroll values.
* After that it scrolls webpage down by maximum value and sets a timer for
* another callback function that would check if scroll was performed properly.
*
* @param eventInfo unused
* @param o pointer to view object created for test purposes
* @param data unused
*
* @see scrollMaxDownCallback()
*/
void fullScrollCallback(void* eventInfo, Evas_Object* o, void* data)
{
    ScrollData* scrollData;
    scrollData = new ScrollData;
    scrollData->obj = o;
    ewk_frame_scroll_size_get(ewk_view_frame_main_get(o),
                              &(scrollData->scrollDX),
                              &(scrollData->scrollDY));
    ewk_frame_scroll_add(ewk_view_frame_main_get(o),
                         0,
                         scrollData->scrollDY);
    scrollData->scrollTimer = ecore_timer_add(3, scrollMaxDownCallback,
                                              scrollData);
}

/**
* ewk_frame_scroll_pos_get() unit test.
*
* Function prepares environment and starts
* ewk_frame_scroll_pos_get() unit test.
*
* @see scrollGetCallback()
*/

TEST(EwkScrollTests, scrollGetTest)
{
    RUN_TEST("http://www.webkit.org", scrollGetCallback, "load,finished", 0);
}

/**
* ewk_frame_scroll_pos_set() unit test.
*
* Function prepares environment and starts
* ewk_frame_scroll_pos_set() unit test.
*
* @see scrollSetCallback()
*/

TEST(EwkScrollTests, scrollSetTest)
{
    RUN_TEST("http://www.webkit.org", scrollSetCallback, "load,finished", 0);
}

/**
* ewk_frame_scroll_add() unit test.
*
* Function prepares environment and starts
* ewk_frame_scroll_add() unit test.
*
* @see scrollAddCallback()
*/

TEST(EwkScrollTests, scrollAddTest)
{
    ScrollData scrollData;
    //These are example values, feel free to modify them in any way necessary.
    scrollData.scrollDX = 50;
    scrollData.scrollDY = 50;
    RUN_TEST("http://www.webkit.org", scrollAddCallback, "load,finished",
             &scrollData);
}

/**
* Scroll time unit test.
*
* Function prepares environment and starts scroll time unit test.
*
* @see scrollTimeCallback()
*/

TEST(EwkScrollTests, scrollTimeTest)
{
    ScrollData scrollData;
    //These are example values, feel free to modify them in any way necessary.
    scrollData.scrollDX = 50;
    scrollData.scrollDY = 50;
    //Set scroll timeout. This is number of seconds that the test will wait
    //for scroll change. If after this amount of seconds scroll is unchanged,
    //it is considered that scroll is not working and test is failed.
    scrollData.scrollTimeout = 5;
    RUN_TEST("http://www.webkit.org", scrollTimeCallback, "load,finished",
             &scrollData);
}

/**
* Full scroll unit test.
*
* Function prepares environment and starts full scroll
* unit test. In this test webpage is scrolled maximum down,
* returns up (scrolled minus maximum down), scrolled maximum right,
* returns left (scrolled minus maximum right).
*
* @see fullScrollCallback()
* @see scrollMaxDownCallback()
* @see scrollMaxUpCallback()
* @see scrollMaxRightCallback()
* @see scrollMaxLeftCallback()
*/

TEST(EwkScrollTests, fullScrollTest)
{
    RUN_TEST("http://www.webkit.org", fullScrollCallback, "load,finished", 0);
}

TEST(EwkScrollTests, fullScrollTest2)
{
    RUN_TEST("http://www.naver.com", fullScrollCallback, "load,finished", 0);
}

