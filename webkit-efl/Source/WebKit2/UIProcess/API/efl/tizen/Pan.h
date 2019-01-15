/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef Pan_h
#define Pan_h

#include <Ecore.h>
#include <Eina.h>
#include <Evas.h>
#include <WebCore/IntPoint.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

#if ENABLE(TIZEN_GESTURE_FEATURE)
#include "SmoothAlgorithm.h"
#endif

class EwkViewImpl;

class Pan {
public:
    static PassOwnPtr<Pan> create(EwkViewImpl* ewkImpl)
    {
        return adoptPtr(new Pan(ewkImpl));
    }
    ~Pan();

    void start(const WebCore::IntPoint&);
    void update(const WebCore::IntPoint&);
    void stop(bool doFlick = true);
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    void resetUrlBarTriggerPoint();
#endif

    bool isWorking();
    void invokeFlick();

private:
    static Eina_Bool panAnimatorCallback(void*);
    static void viewRenderPreCallback(void*, Evas*, void*);
    static void onMouseDown(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void onMouseMove(void* data, Evas*, Evas_Object*, void* eventInfo);
    static void onMultiDown(void* data, Evas*, Evas_Object*, void* eventInfo);
    void storeDataToHistory(int x, int y, double timestamp);
    void getVelocity(int* xVelocity, int* yVelocity);
    void startFlick(float multiplier = 1.0);
#if ENABLE(TIZEN_ENABLE_MINIMUM_2_CPU_ON_MOUSE_DOWN)
    static void enable2CpuJobCallback(void*);
#endif
#if ENABLE(TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN)
    static void gpuWakeUpJobCallback(void*);
#endif
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    static void urlBarShowCallback(void *data, Evas_Object *, void *);
    static void urlBarHideCallback(void *data, Evas_Object *, void *);
    static void urlBarScrollOffsetCallback(void *data, Evas_Object *, void *eventInfo);
#endif

private:
    explicit Pan(EwkViewImpl*);
    void process();
    void initializeHistory();

    typedef struct {
        int x;
        int y;
        double time;
    } HistoryData;

    static const int s_maxSizeOfHistory = 10;
    static constexpr double s_propagationTime = 0.011; // seconds.
    static const int s_flickThreshold = 500; // pixels per second.
    static const int s_flickMaxVelocity = 9000; // pixels per second.

    EwkViewImpl* m_viewImpl;
    Ecore_Animator* m_panAnimator;
    WebCore::IntPoint m_lastPoint;
    WebCore::IntPoint m_currentPoint;
    bool m_isScrollPositionChanged;
    int m_lastHistoryIndex;
    int m_historyCount;
    HistoryData m_history[s_maxSizeOfHistory];
#if ENABLE(TIZEN_GESTURE_FEATURE)
    OwnPtr<SmoothAlgorithm> m_smoothAlgorithm;
#endif
    bool m_isTheFirstPanAnimatorProcessed;
    bool m_dataStoreTrigger;
#if ENABLE(TIZEN_ENABLE_MINIMUM_2_CPU_ON_MOUSE_DOWN)
    Ecore_Job* m_enable2CpuJob;
#endif
#if ENABLE(TIZEN_WAKEUP_GPU_ON_MOUSE_DOWN)
    Ecore_Job* m_gpuWakeUpJob;
#endif
#if ENABLE(TIZEN_WEBKIT2_HIDE_URL_BAR)
    bool m_urlBarStatus;
    bool m_isUrlBarMoving;
    int m_urlBarOffeset;
    int m_urlBarTriggerPoint;
#endif
#if ENABLE(TIZEN_BROWSER_DASH_MODE)
    void startPanDashMode();
    void stopPanDashMode();
    void panDashModeFired(WebCore::Timer<Pan>*);
    bool m_isDashMode;
    bool m_enableCpuDashMode;
    WebCore::Timer<Pan> m_panDashModeTimer;
#endif
};

#endif // Pan_h
