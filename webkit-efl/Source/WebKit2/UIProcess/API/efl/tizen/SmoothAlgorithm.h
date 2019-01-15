/*
 * Copyright (C) 2012 Samsung Electronics
 */

#ifndef SmoothAlgorithm_h
#define SmoothAlgorithm_h

#include <wtf/PassOwnPtr.h>

namespace WebCore {
    class IntPoint;
}

class SmoothAlgorithm {
public:
    static PassOwnPtr<SmoothAlgorithm> create()
    {
        return adoptPtr(new SmoothAlgorithm());
    }

    void initialize();
    void correctPoint(WebCore::IntPoint& point, int x1, int y1, double timestamp1, int x2, int y2, double timestamp2);

private:
    static constexpr double s_moveEventPerSecond = 89.95;
    static constexpr double s_pauseTimeThreshold = 0.05; // seconds
    static constexpr double s_extraPredictionTime = -0.005; // seconds

    SmoothAlgorithm();

    WebCore::IntPoint m_lastPoint;
    double m_lastXVelocity;
    double m_lastYVelocity;
};

#endif // SmoothAlgorithm_h
