#ifndef _VIRTUALHEAD_H
#define _VIRTUALHEAD_H

#include "\Projects\Rovio\RovioMotor\include\Packet.h"

class VirtualHead : Entity
{
protected:
    int16_t Power = 0;
    bool PowerChanged = false;
    int16_t Position = 0;
    bool PositionChanged = false;

public:
    inline int16_t getPower() { return Power; }
    inline void setPower(int16_t value) { PowerChanged |= Power != value; Power = value; }
    inline int16_t getPosition() { return Position; }
    inline void setPosition(int16_t value) { PositionChanged |= Position != value; Position = value; }
    void setProperty(Properties property, int16_t value);
    int16_t getProperty(Properties property);
    bool getPropertyChanged(Properties property);
    bool propertyToBot(Properties property);
    bool propertyFromBot(Properties property);
};

#endif // _VIRTUALHEAD_H
