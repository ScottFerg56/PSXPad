#include "VirtualMotor.h"
#include "PSXPad.h"

void VirtualMotor::SetProperty(int8_t property, int8_t value)
{
    switch (property)
    {
    case MotorProperties_Goal:
        SpeedGoal = value;
        TFTpreMsg();
        tft.printf("%s Goal: %4i\r\n", Name, (int)SpeedGoal);
        break;

    case MotorProperties_RPM:
        RPM = value;
        TFTpreMsg();
        tft.printf("%s RPM: %4i\r\n", Name, (int)RPM);
        break;
    
    case MotorProperties_DirectDrive:
        DirectDrive = value != 0;
        TFTpreMsg();
        tft.printf("%s DirectDrive: %s\r\n", Name, DirectDrive ? "ON" : "OFF");
        break;

    default:                    // invalid property
        // UNDONE: error reporting
        break;
    }
}
