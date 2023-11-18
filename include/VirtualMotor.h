#ifndef _VIRTUALMOTOR_H
#define _VIRTUALMOTOR_H

#include "\Projects\Rovio\RovioMotor\include\Packet.h"

class VirtualMotor : Entity
{
protected:
    int8_t Goal = 0;
    bool GoalChanged = false;
    static bool DirectDrive;
    static bool DirectDriveChanged;
    int8_t RPM = 0;
    bool RPMChanged = false;
    int8_t Power = 0;
    bool PowerChanged = false;

public:
    inline int8_t getGoal() { return Goal; }
    inline void setGoal(int8_t value) { Goal = value; GoalChanged = true; }
    inline int8_t getRPM() { return RPM; }
    inline void setRPM(int8_t value) { RPMChanged = RPM != value; RPM = value; }
    inline int8_t getPower() { return Power; }
    inline void setPower(int8_t value) { PowerChanged = Power != value; Power = value; }
    inline int8_t getDirectDrive() { return DirectDrive; }
    inline void setDirectDrive(int8_t value) { DirectDrive = value; DirectDriveChanged = true; }
    void setProperty(int8_t property, int8_t value);
    int8_t getProperty(int8_t property);
    bool getPropertyChanged(int8_t property);
};

#endif // _VIRTUALMOTOR_H
