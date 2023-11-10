#include "VirtualMotor.h"
#include "PSXPad.h"

void VirtualMotor::SetProperty(int8_t property, int8_t value)
{
    switch (property)
    {
    case MotorProperties_Goal:
        Goal = value;
        break;

    case MotorProperties_RPM:
        RPM = value;
        break;
    
    case MotorProperties_DirectDrive:
        DirectDrive = value != 0;
        break;

    default:                    // invalid property
        // UNDONE: error reporting
        break;
    }
}

int8_t VirtualMotor::GetProperty(int8_t property)
{
    switch (property)
    {
    case MotorProperties_Goal:
        return (int)Goal;

    case MotorProperties_RPM:
        return (int)RPM;
    
    case MotorProperties_DirectDrive:
        return (int)DirectDrive;

    default:                    // invalid property
        // UNDONE: error reporting
        return -1;
    }
}
