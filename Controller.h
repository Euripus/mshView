#ifndef CONTROLLER_H
#define CONTROLLER_H

class Controller
{
public:
    enum class RepeatType
    {
        RT_CLAMP,
        RT_WRAP,
        RT_CYCLE
    };
    
    Controller();
    Controller(RepeatType tp, double min, double max);
    virtual ~Controller();
    
    // Conversion from application time units to controller time units.
    // Derived classes may use this in their update routines.
    double GetControlTime (double applicationTime) const;
protected:
    RepeatType  repeat;
    double      minTime;
    double      maxTime;
    bool        active;
};

#endif // CONTROLLER_H
