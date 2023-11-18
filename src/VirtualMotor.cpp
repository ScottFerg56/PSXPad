#include "VirtualMotor.h"
#include "FLogger.h"

bool VirtualMotor::DirectDrive = false;
bool VirtualMotor::DirectDriveChanged = false;

void VirtualMotor::setProperty(int8_t property, int8_t value)
{
    switch (property)
    {
    case MotorProperties_Goal:
        setGoal(value);
        break;

    case MotorProperties_RPM:
        setRPM(value);
        break;
    
    case MotorProperties_Power:
        setPower(value);
        break;
    
    case MotorProperties_DirectDrive:
        setDirectDrive(value != 0);
        break;

    default:                    // invalid property
        // UNDONE: error reporting
        break;
    }
}

int8_t VirtualMotor::getProperty(int8_t property)
{
    switch (property)
    {
    case MotorProperties_Goal:
        return Goal;

    case MotorProperties_RPM:
        return RPM;
    
    case MotorProperties_Power:
        return Power;
    
    case MotorProperties_DirectDrive:
        return (int8_t)DirectDrive;

    default:                    // invalid property
        // UNDONE: error reporting
        return -1;
    }
}

bool VirtualMotor::getPropertyChanged(int8_t property)
{
    bool changed = false;
    switch (property)
    {
    case MotorProperties_Goal:
        changed = GoalChanged;
        GoalChanged = false;
        break;

    case MotorProperties_RPM:
        changed = RPMChanged;
        RPMChanged = false;
        break;
    
    case MotorProperties_Power:
        changed = PowerChanged;
        PowerChanged = false;
        break;
    
    case MotorProperties_DirectDrive:
        changed = DirectDriveChanged;
        DirectDriveChanged = false;
        break;

    default:                    // invalid property
        break;
    }
    return changed;
}
