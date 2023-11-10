#ifndef VirtualMotor_h
#define VirtualMotor_h

#include "\Projects\Rovio\RovioMotor\include\Packet.h"

class VirtualMotor : Entity
{
public:
    int8_t Goal = 0;
    bool DirectDrive = false;
    int8_t RPM = 0;
    const char* Name;

    void SetProperty(int8_t property, int8_t value);
    int8_t GetProperty(int8_t property);
};

#endif