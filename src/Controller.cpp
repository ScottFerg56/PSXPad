#include "Controller.h"
#include "FLogger.h"
#include "PSXPad.h"
#include "Pad.h"

namespace Controller
{
// modes for processing control inputs
enum ControlModes
{
    Control_Disabled,   // control inputs disabled
    Control_Unlimited,  // control inputs reduced from their full range
    Control_Limited,    // control inputs with their full range
};

ControlModes ControlMode = Control_Limited;     // the current control mode

float factorMode = 0.5f;    // the current ControlMode range factor

//
// from "Motion Planning for Omnidirectional Wheeled Mobile Robot by Potential Field Method" equation (7)
// as computed by the custom program 'OmniCtrl'
// the matrix 'M', from columns A1, A2, and A3
// which will be multiplied by the robot velocity and spin vector to produce the wheel rotation vector
//
float M00 = -0.0110, M01 = -0.0235, M02 = 2.7273;
float M10 =	-0.0110, M11 =  0.0235, M12 = 2.7273;
float M20 =  0.0260, M21 =  0.0000, M22 = 3.7662;

//
// the desired robot velocity x and y components and the desired rotational velocity in radians
//
float velOx = 0, velOy = 0, velOw = 0;

//
// the output wheel rotational velocities in radians
//
float velW0 = 0, velW1 = 0, velW2 = 0;

//
// the resulting wheel RPMs
//
int8_t rpm0 = 0, rpm1 = 0, rpm2 = 0;

// the following have been designed using the custom OmniCtrl program to limit motor RPMs to 100:
// 'translate' mode multiplier for arbitrary X & Y stick values [-100..100],
//      converting to actual bot X & Y translation speeds (mm/sec)
const float factorTrans = 4.031f;

// 'tractor' mode multiplier for arbitrary X stick values [-100..100] controlling spin,
//      converting to actual rotation speed (radians/sec)
// Y stick deflection controls forward translation as in translate mode
const float factorSpinT = 0.027f;

// 'spin' mode multiplier for arbitrary button values [0..255],
//      converting to actual rotation speed (radians/sec)
const float factorSpin = (0.027f * 100.0f) / 255.0f;

// convert rotation in radians/sec to RPM
const float rot2RPM = 60.0f / (2 * PI);

/**
 * @brief Calculate the 3 omni-wheel RPMs (rpm0..rpm2) required to produce the desired
 *      robot velocity x and y and rotation (velOx, velOy, velOw)
 *      and set the RPM goals respectively to the left, right and rear motors
 */
void Guidance()
{
    //velW = M * velO;
    velW0 = M00 * velOx + M01 * velOy + M02 * velOw;
    velW1 = M10 * velOx + M11 * velOy + M12 * velOw;
    velW2 = M20 * velOx + M21 * velOy + M22 * velOw;
    //Serial.printf("velO: %f %f %f\r\n", velOx, velOy, velOw);
    //Serial.printf("velW: %f %f %f\r\n", velW0, velW1, velW2);
    // convert wheel rotations in radians/sec to RPM
    rpm0 = (int8_t)roundf(velW0 * rot2RPM);
    rpm1 = (int8_t)roundf(velW1 * rot2RPM);
    rpm2 = (int8_t)roundf(velW2 * rot2RPM);
    // if any of the RPM goals are above the maximum, ignore the whole set, leaving it as previously set
    if (abs(rpm0) > 100 || abs(rpm1) > 100 || abs(rpm2) > 100)
        return;
    // set the motor goals, which will be sent to the robot
    VirtualBot.SetEntityPropertyValue(EntityID_LeftMotor, PropertyID_Goal, -rpm0);
    VirtualBot.SetEntityPropertyValue(EntityID_RightMotor, PropertyID_Goal, rpm1);
    VirtualBot.SetEntityPropertyValue(EntityID_RearMotor, PropertyID_Goal, rpm2);
}

bool active = true;

/**
 * @brief A windowing control struct identifying a screen region and information to display there
 */
struct Ctrl
{
    /**
     * @brief A Point containing the x,y position on the screen
     */
    Point location;
    /**
     * @brief The width on the screen, in characters (as standardized across the system)
     */
    uint8_t width;
    /**
     * @brief The color of the text
     */
    uint16_t textColor;
    /**
     * @brief The ID of an Entity to display (unused if label is present)
     */
    EntityID entity;
    /**
     * @brief The ID of a Property on the Entity to display (unused if label is present)
     */
    PropertyID property;
    /**
     * @brief The text to display, if present.
     */
    const char* label;
};

/**
 * @brief The controls to display on the screen
 */
Ctrl ctrls[] =
{
    { { 144,  32 }, 4, HX8357_WHITE, EntityID_LeftMotor,  PropertyID_Goal,  nullptr },
    { { 288,  32 }, 4, HX8357_WHITE, EntityID_RightMotor, PropertyID_Goal,  nullptr },
    { { 216, 120 }, 4, HX8357_WHITE, EntityID_RearMotor,  PropertyID_Goal,  nullptr },
    { { 144,  59 }, 4, HX8357_WHITE, EntityID_LeftMotor,  PropertyID_RPM,   nullptr },
    { { 288,  59 }, 4, HX8357_WHITE, EntityID_RightMotor, PropertyID_RPM,   nullptr },
    { { 216, 147 }, 4, HX8357_WHITE, EntityID_RearMotor,  PropertyID_RPM,   nullptr },
    { { 144,  85 }, 4, HX8357_WHITE, EntityID_LeftMotor,  PropertyID_Power, nullptr },
    { { 288,  85 }, 4, HX8357_WHITE, EntityID_RightMotor, PropertyID_Power, nullptr },
    { { 216, 174 }, 4, HX8357_WHITE, EntityID_RearMotor,  PropertyID_Power, nullptr },
    { { 216,  32 }, 4, HX8357_WHITE, EntityID_None,       PropertyID_None,  "Goal"  },
    { { 216,  59 }, 4, HX8357_WHITE, EntityID_None,       PropertyID_None,  "RPM"   },
    { { 216,  85 }, 4, HX8357_WHITE, EntityID_None,       PropertyID_None,  "Power"  },
    { {  12,  32 }, 9, HX8357_GREEN, EntityID_None,       PropertyID_ControlMode, "Limited" },
    { {  12,  59 }, 4, HX8357_WHITE, EntityID_Head,       PropertyID_Power, nullptr },
    { {  12,  85 }, 4, HX8357_WHITE, EntityID_Head,       PropertyID_Position, nullptr },
};

/**
 * @brief Get the Ctrl from the list for the specified Entity and Property
 * 
 * @param entity The ID of the Entity to fetch
 * @param property The ID of the Property to fetch
 * @return Ctrl& A reference to the found Ctrl
 * @remarks Not finding the Ctrl is reported as an error and the first Ctrl in the list is returned
 */
Ctrl& GetCtrl(EntityID entity, PropertyID property)
{
    for (int i = 0; i < sizeof(ctrls) / sizeof(Ctrl); i++)
    {
        if (ctrls[i].entity == entity && ctrls[i].property == property)
            return ctrls[i];
    }
    // no match is an error! not fatal, but will cause problems
    floge("Ctrl not found");
    return ctrls[0];
}

const int8_t telMargin = 3; // the region margin used to compute the location values in the Ctrl list

/**
 * @brief Draw a given Ctrl
 * 
 * @param ctrl A reference to the Ctrl to draw
 */
void DrawCtrl(Ctrl& ctrl)
{
    int16_t x = ctrl.location.x;
    int16_t y = ctrl.location.y;
    tft.setCursor(x, y + 1);
    // clear the region
    tft.fillRect(x-telMargin, y-telMargin, ctrl.width * charWidth + telMargin*2, charHeight + telMargin*2, RGBto565(54,54,54));
    tft.setTextColor(ctrl.textColor);
    if (ctrl.label != nullptr)
    {
        // print any supplied label text
        tft.printf("%-.*s", ctrl.width, ctrl.label);
    }
    else
    {
        // print an Entity Property value
        tft.print(VirtualBot.GetEntityProperty(ctrl.entity, ctrl.property)->Get());
    }
    // restore the default text color to be nice to other UI pages
    tft.setTextColor(HX8357_WHITE);
}

/**
 * @brief Draw all the controls
 */
void DrawTelemetry()
{
    for (int i = 0; i < sizeof(ctrls) / sizeof(Ctrl); i++)
    {
        DrawCtrl(ctrls[i]);
    }
}

/**
 * @brief Draw a Ctrl cpecified by Entity and Property IDs
 * 
 * @param entity The Entity ID of the Ctrl
 * @param property The Property ID of the Ctrl
 */
void DrawCtrl(EntityID entity, PropertyID property)
{
    Ctrl& ctrl = GetCtrl(entity, property);
    DrawCtrl(ctrl);
}

void Activate()
{
    active = true;
    tft.fillRect(0, 0, tftWidth, menuY, HX8357_BLACK);
    DrawTelemetry();
}

void Deactivate()
{
    active = false;
}

void ProcessKey(PadKeys btn, int16_t x, int16_t y)
{
    if (btn == PadKeys_start)
    {
        // the start button cycles through the ControlModes (when pressed)
        if (x != 0) // ignore the release
        {
            Ctrl& ctrl = GetCtrl(EntityID_None, PropertyID_ControlMode);
            switch (ControlMode)
            {
            case Control_Disabled:
                ControlMode = Control_Unlimited;
                ctrl.textColor = HX8357_YELLOW;
                ctrl.label = "Unlimited";
                factorMode = 1;
                break;
            case Control_Unlimited:
                ControlMode = Control_Limited;
                ctrl.textColor = HX8357_GREEN;
                ctrl.label = "Limited";
                factorMode = 0.5f;
                break;
            case Control_Limited:
                ControlMode = Control_Disabled;
                ctrl.textColor = HX8357_RED;
                ctrl.label = "Disabled";
                factorMode = 0;
                break;
            }
            if (active)
                DrawCtrl(ctrl);
        }
        return;
    }

    if (ControlMode != Control_Disabled)
    {
        // robot control is enabled
        switch (btn)
        {
        case PadKeys_cross:
            // kill all motor movement
            for (EntityID m = EntityID_LeftMotor; m <= EntityID_RearMotor; m++)
                VirtualBot.SetEntityPropertyValue(m, PropertyID_Goal, 0);
            for (PadKeys k = PadKeys_knob0; k <= PadKeys_knob3; k++)
                Pad::SetKnobValue(k, 0);
            break;
        case PadKeys_leftStick:
            // 'translate' in a vector corresponding to the left stick position
            velOx = x * factorTrans * factorMode;
            velOy = y * factorTrans * factorMode;
            Guidance();
            break;
        case PadKeys_rightStick:
            // 'tractor' x controls spin and y controls y movement (much like normal car driving)
            velOw = -x * factorSpinT * factorMode;
            velOy = y * factorTrans * factorMode;
            Guidance();
            break;
        case PadKeys_left:
            // 'spin' left
            velOw = x * factorSpin * factorMode;
            Guidance();
            break;
        case PadKeys_right:
            // 'spin' right
            velOw = -x * factorSpin * factorMode;
            Guidance();
            break;
        case PadKeys_up:
            // head up while pressed
            VirtualBot.SetEntityPropertyValue(EntityID_Head, PropertyID_Power, (int16_t)roundf(x * 100.0f) / 255.0f);
            break;
        case PadKeys_down:
            // head down while pressed
            VirtualBot.SetEntityPropertyValue(EntityID_Head, PropertyID_Power, -(int16_t)roundf(x * 100.0f) / 255.0f);
            break;
        case PadKeys_knob0Btn:
        case PadKeys_knob1Btn:
        case PadKeys_knob2Btn:
            if (x != 0)
            {
                // when pressed, zeroes the corresponding motor goal
                int ix = btn - PadKeys_knob0Btn;
                VirtualBot.SetEntityPropertyValue((EntityID)(EntityID_LeftMotor + ix), PropertyID_Goal, 0);
                Pad::SetKnobValue((PadKeys)(PadKeys_knob0 + ix), 0);
            }
            break;
        case PadKeys_knob0:
        case PadKeys_knob1:
        case PadKeys_knob2:
            {
                // when set, controls the corresponding motor goal
                // UNDONE: these knob controls can interfere or confuse the stick/button controls
                //      Need a way to disconnect the two, but the knobs are mostly for testing
                EntityID entity = (EntityID)(EntityID_LeftMotor + (btn - PadKeys_knob0));
                VirtualBot.SetEntityPropertyValue(entity, PropertyID_Goal, (int8_t)x);
            }
            break;    
        }
    }
    else
    {
        switch (btn)
        {
        case PadKeys_knob0:
        case PadKeys_knob1:
        case PadKeys_knob2:
        case PadKeys_knob3:
            {
                // while disabled, force knob to stay at current goals
                EntityID entity = (EntityID)(EntityID_LeftMotor + btn - PadKeys_knob0);
                int8_t goal = VirtualBot.GetEntityPropertyValue(entity, PropertyID_Goal);
                Pad::SetKnobValue(btn, goal);
            }
            break;    
        }
    }
}

void ProcessChange(Entity* pe, Property* pp)
{
    //flogd("%s.%s -> %i", pe->GetName(), pp->GetName(), pp->Get());
    DrawCtrl(pe->GetID(), pp->GetID());
}

/**
 * @brief Plot a value for a given Entity and Property
 * 
 * @param entity The ID for the Entity
 * @param property The ID for the Property
 * @remarks Serial prints the name and value for use by the Teleplot VS Code extension
 */
void PlotEntityProperty(EntityID entity, PropertyID property)
{
    Property* pp = VirtualBot.GetEntityProperty(entity, property);
    plot(VirtualBot.GetEntity(entity)->GetName(), pp->GetName(), pp->Get());
}

void DoPlot()
{
    for (EntityID entity = EntityID_LeftMotor; entity <= EntityID_RearMotor; entity++)
    {
        PlotEntityProperty(entity, PropertyID_Goal);
        PlotEntityProperty(entity, PropertyID_RPM);
    }
}
};
