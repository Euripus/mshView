#include "Controller.h"
#include  <cmath>

Controller::Controller(RepeatType tp, double mind, double maxd)
    :
    repeat(tp),
    minTime(mind),
    maxTime(maxd),
    active(true)
{
}

Controller::Controller()
    :
    repeat(RepeatType::RT_CLAMP),
    minTime(0.0),
    maxTime(0.0),
    active(false)
{
}

Controller::~Controller()
{
}

double Controller::GetControlTime (double applicationTime) const
{
    if (repeat == RepeatType::RT_CLAMP)
    {
        // Clamp the time to the [min,max] interval.
        if (applicationTime < minTime)
        {
            return minTime;
        }
        if (applicationTime > maxTime)
        {
            return maxTime;    
        }
        return applicationTime;
    }

    double timeRange = maxTime - minTime;
    if (timeRange > 0.0)
    {
        double multiples = (applicationTime - minTime)/timeRange;
        double integerTime = std::floor(multiples);
        double fractionTime = multiples - integerTime;
        if (repeat == RepeatType::RT_WRAP)
        {
            return minTime + fractionTime*timeRange;
        }

        // repeat == RepeatType::RT_CYCLE
        if (((int)integerTime) & 1)
        {
            // Go backward in time.
            return maxTime - fractionTime*timeRange;
        }
        else
        {
            // Go forward in time.
            return minTime + fractionTime*timeRange;
        }
    }

    // The minimum and maximum times are the same, so return the minimum.
    return minTime;
}

