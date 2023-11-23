#ifndef _VIRTUALMOTOR_H
#define _VIRTUALMOTOR_H

#include "\Projects\Rovio\RovioMotor\include\Packet.h"

class VirtualMotor : Entity
{
protected:
    int16_t Goal = 0;
    bool GoalChanged = false;
    static bool DirectDrive;
    static bool DirectDriveChanged;
    int16_t RPM = 0;
    bool RPMChanged = false;
    int16_t Power = 0;
    bool PowerChanged = false;

public:
    inline int16_t getGoal() { return Goal; }
    inline void setGoal(int16_t value) { Goal = value; GoalChanged = true; }
    inline int16_t getRPM() { return RPM; }
    inline void setRPM(int16_t value) { RPMChanged = RPM != value; RPM = value; }
    inline int16_t getPower() { return Power; }
    inline void setPower(int16_t value) { PowerChanged = Power != value; Power = value; }
    inline int16_t getDirectDrive() { return DirectDrive; }
    inline void setDirectDrive(int16_t value) { DirectDrive = value; DirectDriveChanged = true; }
    void setProperty(int8_t property, int16_t value);
    int16_t getProperty(int8_t property);
    bool getPropertyChanged(int8_t property);
};

#endif // _VIRTUALMOTOR_H
