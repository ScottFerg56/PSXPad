#include "VirtualMotor.h"
#include "FLogger.h"

bool VirtualMotor::DirectDrive = false;
bool VirtualMotor::DirectDriveChanged = false;

void VirtualMotor::setProperty(Properties property, int16_t value)
{
    switch (property)
    {
    case Properties_Goal:
        setGoal(value);
        break;

    case Properties_RPM:
        setRPM(value);
        break;
    
    case Properties_Power:
        setPower(value);
        break;
    
    case Properties_DirectDrive:
        setDirectDrive(value != 0);
        break;

    default:                    // invalid property
        // UNDONE: error reporting
        break;
    }
}

int16_t VirtualMotor::getProperty(Properties property)
{
    switch (property)
    {
    case Properties_Goal:
        return Goal;

    case Properties_RPM:
        return RPM;
    
    case Properties_Power:
        return Power;
    
    case Properties_DirectDrive:
        return (int16_t)DirectDrive;

    default:                    // invalid property
        // UNDONE: error reporting
        return -1;
    }
}

bool VirtualMotor::getPropertyChanged(Properties property)
{
    bool changed = false;
    switch (property)
    {
    case Properties_Goal:
        changed = GoalChanged;
        GoalChanged = false;
        break;

    case Properties_RPM:
        changed = RPMChanged;
        RPMChanged = false;
        break;
    
    case Properties_Power:
        changed = PowerChanged;
        PowerChanged = false;
        break;
    
    case Properties_DirectDrive:
        changed = DirectDriveChanged;
        DirectDriveChanged = false;
        break;

    default:                    // invalid property
        break;
    }
    return changed;
}
