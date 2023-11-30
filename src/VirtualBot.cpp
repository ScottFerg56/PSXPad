#include "VirtualBot.h"
#include "FLogger.h"
#include <esp_now.h>
#include <WiFi.h>
#include "PSXPad.h"
#include "Pad.h"

namespace VirtualBot
{
VirtualMotor Motors[3];
int16_t HeadGoal = 0;
bool HeadGoalChanged = false;
int16_t HeadPower = 0;
bool HeadPowerChanged = false;

enum ControlModes
{
    Control_Disabled,
    Control_Unlimited,
    Control_Limited,
};

ControlModes ControlMode = Control_Limited;

float factorMode = 0.5f;

//
// from "Motion Planning for Omnidirectional Wheeled Mobile Robot by Potential Field Method" equation (7)
// as computed by independent program 'OmniCtrl'
// the matrix, from columns A1, A2, and A3
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

int8_t rpm0 = 0, rpm1 = 0, rpm2 = 0;

// the following have been designed using the external OmniCtrl program to limit motor RPMs to 100:
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

void Guidance()
{
    //velW = M * velO;
    velW0 = M00 * velOx + M01 * velOy + M02 * velOw;
    velW1 = M10 * velOx + M11 * velOy + M12 * velOw;
    velW2 = M20 * velOx + M21 * velOy + M22 * velOw;
    //Serial.printf("velO: %f %f %f\r\n", velOx, velOy, velOw);
    //Serial.printf("velW: %f %f %f\r\n", velW0, velW1, velW2);
    // Given our stick inputs, used for the input values, are arbitrary
    // and max out at 100, that makes our rotational radians rather arbitrary
    // and max out around 2.4.
    // So use an arbitrary multiplier designed to max out our desired RPMs around 100...
    rpm0 = (int8_t)roundf(velW0 * rot2RPM);
    rpm1 = (int8_t)roundf(velW1 * rot2RPM);
    rpm2 = (int8_t)roundf(velW2 * rot2RPM);
    if (abs(rpm0) > 100 || abs(rpm1) > 100 || abs(rpm2) > 100)
        return;
    VirtualBot::setEntityProperty(Entities_LeftMotor, Properties_Goal, -rpm0);
    VirtualBot::setEntityProperty(Entities_RightMotor, Properties_Goal, rpm1);
    VirtualBot::setEntityProperty(Entities_RearMotor, Properties_Goal, rpm2);
}

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    //if (status != ESP_NOW_SEND_SUCCESS)
    //    flogw("Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *pData, int len)
{
    Packet packet;
    int cnt = len / sizeof(Packet);
    while (cnt > 0)
    {
        memcpy(&packet, pData, sizeof(Packet));
        pData += sizeof(Packet);
        cnt--;
        setEntityProperty(packet.entity, packet.property, packet.value);
    }
}

void init(uint8_t addr[])
{
    flogi("WIFI init");
    if (!WiFi.mode(WIFI_STA))
        flogf("%s FAILED", "WIFI init");

    flogi("MAC addr: %s", WiFi.macAddress().c_str());

    flogi("ESP_NOW init");
    if (esp_now_init() != ESP_OK)
        flogf("%s FAILED", "ESP_NOW init");

    esp_now_register_send_cb(OnDataSent);

    memcpy(peerInfo.peer_addr, addr, sizeof(peerInfo.peer_addr));
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
        flogf("%s FAILED", "ESP_NOW peer add");
    
    esp_now_register_recv_cb(OnDataRecv);
}

const char* getEntityName(Entities entity)
{
    switch (entity)
    {
    case Entities_LeftMotor:
        return "Left Motor";
    case Entities_RightMotor:
        return "Right Motor";
    case Entities_RearMotor:
        return "Rear Motor";
    case Entities_Head:
        return "Head";
    default:
        return "***";
    }
}

const char* getPropertyName(Properties property)
{
    switch (property)
    {
    case Properties_Goal:
        return "Goal";
    case Properties_RPM:
        return "RPM";
    case Properties_Power:
        return "Powr";
    case Properties_DirectDrive:
        return "DD";
    default:
        return "***";
    }
}

VirtualMotor& getMotor(Entities entity)
{
    return Motors[entity - Entities_LeftMotor];
}

void setEntityProperty(Entities entity, Properties property, int16_t value)
{
    switch (entity)
    {
    case Entities_LeftMotor:
    case Entities_RightMotor:
    case Entities_RearMotor:
        getMotor(entity).setProperty(property, value);
        break;

    case Entities_Head:
        switch (property)
        {
        case Properties_Goal:  // TODO: for now just power
            HeadGoalChanged |= HeadGoal != value;
            HeadGoal = value;
            break;
        
        case Properties_Power:  // TODO: for now just power
            HeadPowerChanged |= HeadPower != value;
            HeadPower = value;
            break;
        
        default:
            break;
        }

    default:    // invalid enity
        break;
    }
}

int16_t getEntityProperty(Entities entity, Properties property)
{
    switch (entity)
    {
    case Entities_LeftMotor:
    case Entities_RightMotor:
    case Entities_RearMotor:
        return getMotor(entity).getProperty(property);

    case Entities_Head:
        switch (property)
        {
        case Properties_Goal:  // TODO: for now just power
            return HeadGoal;
        
        case Properties_Power:  // TODO: for now just power
            return HeadPower;
        
        default:
            break;
        }

    default:    // invalid enity
        break;
    }
    return -1;
}

bool getEntityPropertyChanged(Entities entity, Properties property)
{
    switch (entity)
    {
    case Entities_LeftMotor:
    case Entities_RightMotor:
    case Entities_RearMotor:
        return getMotor(entity).getPropertyChanged(property);

    case Entities_Head:
        switch (property)
        {
        case Properties_Goal:  // TODO: for now just power
            {
                bool changed = HeadGoalChanged;
                HeadGoalChanged = false;
                return changed;
            }
        
        case Properties_Power:  // TODO: for now just power
            {
                bool changed = HeadPowerChanged;
                HeadPowerChanged = false;
                return changed;
            }
        
        default:
            break;
        }
    }
    return false;
}

bool active = true;

struct Ctrl
{
    Point location;
    uint8_t width;
    uint16_t textColor;
    Entities entity;
    Properties property;
    const char* label;
};

Ctrl ctrls[] =
{
    { { 144,  32 }, 4, HX8357_WHITE, Entities_LeftMotor,  Properties_Goal,  },
    { { 288,  32 }, 4, HX8357_WHITE, Entities_RightMotor, Properties_Goal,  },
    { { 216, 120 }, 4, HX8357_WHITE, Entities_RearMotor,  Properties_Goal,  },
    { { 144,  59 }, 4, HX8357_WHITE, Entities_LeftMotor,  Properties_RPM,   },
    { { 288,  59 }, 4, HX8357_WHITE, Entities_RightMotor, Properties_RPM,   },
    { { 216, 147 }, 4, HX8357_WHITE, Entities_RearMotor,  Properties_RPM,   },
    { { 144,  85 }, 4, HX8357_WHITE, Entities_LeftMotor,  Properties_Power, },
    { { 288,  85 }, 4, HX8357_WHITE, Entities_RightMotor, Properties_Power, },
    { { 216, 174 }, 4, HX8357_WHITE, Entities_RearMotor,  Properties_Power, },
    { { 216,  32 }, 4, HX8357_WHITE, Entities_None,       Properties_Goal,  },
    { { 216,  59 }, 4, HX8357_WHITE, Entities_None,       Properties_RPM,   },
    { { 216,  85 }, 4, HX8357_WHITE, Entities_None,       Properties_Power, },
    { {  12,  32 }, 9, HX8357_GREEN, Entities_None,       Properties_ControlMode },
    { {  12,  59 }, 4, HX8357_WHITE, Entities_Head,       Properties_Goal, },
    { {  12,  85 }, 4, HX8357_WHITE, Entities_Head,       Properties_Power, },
};

Ctrl& getCtrl(Entities entity, Properties property)
{
    for (int i = 0; i < sizeof(ctrls) / sizeof(Ctrl); i++)
    {
        if (ctrls[i].entity == entity && ctrls[i].property == property)
            return ctrls[i];
    }
    return ctrls[0];
}

const int8_t telMargin = 3;
void drawCtrl(Ctrl& ctrl)
{
    int16_t x = ctrl.location.x;
    int16_t y = ctrl.location.y;
    tft.setCursor(x, y + 1);
    tft.fillRect(x-telMargin, y-telMargin, ctrl.width * charWidth + telMargin*2, charHeight + telMargin*2, HX8357_BLACK);
    tft.setTextColor(ctrl.textColor);
    if (ctrl.entity != Entities_None)
    {
        tft.printf("%*i", ctrl.width, getEntityProperty(ctrl.entity, ctrl.property));
    }
    else
    {
        switch (ctrl.property)
        {
        case Properties_ControlMode:
            switch (ControlMode)
            {
            case Control_Disabled:
                tft.print("Disabled");
                break;
            case Control_Unlimited:
                tft.print("Unlimited");
                break;
            case Control_Limited:
                tft.print("Limited");
                break;
            }
            break;
        
        default:
            tft.print(getPropertyName(ctrl.property));
            break;
        }
    }
    tft.setTextColor(HX8357_WHITE);
}

void drawTelemetry()
{
    for (int i = 0; i < sizeof(ctrls) / sizeof(Ctrl); i++)
    {
        drawCtrl(ctrls[i]);
    }
}

void drawCtrl(Entities entity, Properties property)
{
    Ctrl& ctrl = getCtrl(entity, property);
    drawCtrl(ctrl);
}

void activate()
{
    active = true;
    tft.fillRect(0, 0, tftWidth, menuY, RGBto565(54,54,54));
    drawTelemetry();
}

void deactivate()
{
    active = false;
}

void processKey(PadKeys btn, int16_t x, int16_t y)
{
    
    if (btn == PadKeys_start)
    {
        if (x != 0)
        {
            Ctrl& ctrl = getCtrl(Entities_None, Properties_ControlMode);
            switch (ControlMode)
            {
            case Control_Disabled:
                ControlMode = Control_Unlimited;
                ctrl.textColor = HX8357_YELLOW;
                factorMode = 1;
                break;
            case Control_Unlimited:
                ControlMode = Control_Limited;
                ctrl.textColor = HX8357_GREEN;
                factorMode = 0.5f;
                break;
            case Control_Limited:
                ControlMode = Control_Disabled;
                ctrl.textColor = HX8357_RED;
                factorMode = 0;
                break;
            }
            drawCtrl(ctrl);
        }
        return;
    }

    if (ControlMode != Control_Disabled)
    {
        switch (btn)
        {
        case PadKeys_cross:
            // kill all motor movement
            for (Entities m = Entities_LeftMotor; m <= Entities_RearMotor; m++)
                getMotor(m).setGoal(0);
            for (PadKeys k = PadKeys_knob0; k <= PadKeys_knob3; k++)
                Pad::setKnobValue(k, 0);
            break;
        case PadKeys_leftStick:
            // 'translate'
            velOx = x * factorTrans * factorMode;
            velOy = y * factorTrans * factorMode;
            Guidance();
            break;
        case PadKeys_rightStick:
            // 'tractor'
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
        case PadKeys_knob0Btn:
        case PadKeys_knob1Btn:
        case PadKeys_knob2Btn:
            if (x != 0)
            {
                int ix = btn - PadKeys_knob0Btn;
                VirtualBot::setEntityProperty((Entities)(Entities_LeftMotor + ix), Properties_Goal, 0);
                Pad::setKnobValue((PadKeys)(PadKeys_knob0 + ix), 0);
            }
            break;
        case PadKeys_knob0:
        case PadKeys_knob1:
        case PadKeys_knob2:
            {
                Entities entity = (Entities)(Entities_LeftMotor + (btn - PadKeys_knob0));
                VirtualBot::setEntityProperty(entity, Properties_Goal, (int8_t)x);
            }
            break;    
        case PadKeys_knob3Btn:
            if (x != 0)
            {
                VirtualBot::setEntityProperty(Entities_Head, Properties_Goal, 0);
                Pad::setKnobValue(PadKeys_knob3, 0);
            }
            break;
        case PadKeys_knob3:
            VirtualBot::setEntityProperty(Entities_Head, Properties_Goal, (int8_t)x);
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
                Entities entity = (Entities)(Entities_LeftMotor + btn - PadKeys_knob0);
                int8_t goal = VirtualBot::getEntityProperty(entity, Properties_Goal);
                Pad::setKnobValue(btn, goal);
            }
            break;    
        }
    }
}

void flush()
{
    const int len = 10;
    Packet packets[len];     // enough for a few properties
    uint8_t cnt = 0;
    for (Entities entity = firstEntity; entity <= lastEntity; entity++)
    {
        for (Properties prop = firstProperty; prop <= lastProperty; prop++)
        {
            if (getEntityPropertyChanged(entity, prop))
            {
                if (cnt >= len)
                {
                    floge("packet buffer too small");
                    break;
                }
                if (prop != Properties_RPM && prop != Properties_Power)   // skip readonly
                {
                    packets[cnt].entity = entity;
                    packets[cnt].property = prop;
                    packets[cnt].value = getEntityProperty(entity, prop);
                    cnt++;
                }
                if (active)
                    drawCtrl(entity, prop);
            }
        }
    }
    if (cnt > 0)
    {
        esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)&packets, cnt * sizeof(Packet));
        if (result != ESP_OK)
            floge("Error sending data");
    }
}

void plotEntityProperty(Entities entity, Properties property)
{
    plot(getEntityName(entity), getPropertyName(entity, property), getEntityProperty(entity, property));
}

void doplot()
{
    for (Entities entity = Entities_LeftMotor; entity <= Entities_RearMotor; entity++)
    {
        plotEntityProperty(entity, Properties_Goal);
        plotEntityProperty(entity, Properties_RPM);
    }
}
};
