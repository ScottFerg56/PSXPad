#ifndef VirtualMotor_h
#define VirtualMotor_h

#include "Packet.h"

class VirtualMotor : Entity
{
public:
    float SpeedGoal = 0;
    bool DirectDrive = false;
    float RPM = 0;
    const char* Name;

    void SetProperty(int8_t property, int8_t value);
};

#endif