/*
 * Copyright (C) 2012 Samsung Electronics
 */

#include "config.h"
#include "SmoothAlgorithm.h"

#include <Ecore.h>
#include <WebCore/IntPoint.h>

using namespace WebCore;

static inline int intRound(double value)
{
    return (value >= 0 ? static_cast<int>(value + 0.5) : static_cast<int>(value - 0.5));
}

SmoothAlgorithm::SmoothAlgorithm()
    : m_lastXVelocity(0)
    , m_lastYVelocity(0)
{
}

void SmoothAlgorithm::initialize()
{
    m_lastPoint = IntPoint::zero();
    m_lastXVelocity = 0;
    m_lastYVelocity = 0;
}

// Raw data's updating term can be short and long(it is not smooth),
// so this code makes smooth virtual data when user want to get current data.
// This code is based on Tizen elementary's elm_interface_scrollable.c's _elm_scroll_hold_animator().
void SmoothAlgorithm::correctPoint(IntPoint& point, int x1, int y1, double timestamp1, int x2, int y2, double timestamp2)
{
    double xVelocity = 0;
    double yVelocity = 0;
    double currentTime = ecore_loop_time_get();
    double timeDifference = currentTime - timestamp1;

    // Initialize m_lastPoint with latest position.
    if (!m_lastPoint.x() && !m_lastPoint.y()) {
        m_lastPoint.setX(x1);
        m_lastPoint.setY(y1);
    }

    if (fabs(timeDifference) > s_pauseTimeThreshold)
        point = m_lastPoint;
    else {
        xVelocity = (x1 - x2) * s_moveEventPerSecond;
        yVelocity = (y1 - y2) * s_moveEventPerSecond;
        int x = intRound(x1 + (timeDifference + s_extraPredictionTime) * xVelocity);
        int y = intRound(y1 + (timeDifference + s_extraPredictionTime) * yVelocity);

        // Prevent that point goes back even though direction of velocity is not changed.
        if ((m_lastXVelocity * xVelocity >= 0)
            && (xVelocity == 0 || (xVelocity < 0 && x > m_lastPoint.x()) || (xVelocity > 0 && x < m_lastPoint.x())))
            x = m_lastPoint.x();
        if ((m_lastYVelocity * yVelocity >= 0)
            && (yVelocity == 0 || (yVelocity < 0 && y > m_lastPoint.y()) || (yVelocity > 0 && y < m_lastPoint.y())))
            y = m_lastPoint.y();

        point = IntPoint(x, y);
    }

    m_lastPoint = point;
    m_lastXVelocity = xVelocity;
    m_lastYVelocity = yVelocity;
}
