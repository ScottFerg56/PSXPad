#include "VirtualBot.h"
#include "FLogger.h"
#include <esp_now.h>
#include <WiFi.h>
#include "PSXPad.h"
#include "Pad.h"

namespace VirtualBot
{
VirtualMotor Motors[3];
Point EntityScreenMap[] =
{
	{ 144,  32},  // left motor
    { 288,  32},  // right motor
    { 216, 120},  // rear motor
    { 216,  32},  // all motors (labels)
};

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
    rpm0 = (int8_t)roundf(velW0 * 40);
    rpm1 = (int8_t)roundf(velW1 * 40);
    rpm2 = (int8_t)roundf(velW2 * 40);
    if (abs(rpm0) > 100 || abs(rpm1) > 100 || abs(rpm2) > 100)
        return;
    VirtualBot::setEntityProperty(Entities_LeftMotor, MotorProperties_Goal, -rpm0);
    VirtualBot::setEntityProperty(Entities_RightMotor, MotorProperties_Goal, rpm1);
    VirtualBot::setEntityProperty(Entities_RearMotor, MotorProperties_Goal, rpm2);
}

esp_now_peer_info_t peerInfo;

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    //if (status != ESP_NOW_SEND_SUCCESS)
    //    flogw("Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *pData, int len)
{
    if (len == 0 || (len & 1) != 0)
    {
        floge("invalid data packet length");
        return;
    }
    while (len > 0)
    {
        int8_t entity = (*pData & 0xF0) >> 4;
        int8_t property = *pData++ & 0x0F;
        int8_t value = *pData++;
        len -= 2;
        setEntityProperty(entity, property, value);
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

const char* getEntityName(int8_t entity)
{
    switch (entity)
    {
    case Entities_LeftMotor:
        return "Left Motor";
    case Entities_RightMotor:
        return "Right Motor";
    case Entities_RearMotor:
        return "Rear Motor";
    default:
        return "***";
    }
}

const char* getEntityPropertyName(int8_t entity, int8_t property)
{
    switch (entity)
    {
    case Entities_LeftMotor:
    case Entities_RightMotor:
    case Entities_RearMotor:
    case Entities_AllMotors:
        switch (property)
        {
        case MotorProperties_Goal:
            return "Goal";
        case MotorProperties_RPM:
            return "RPM";
        case MotorProperties_Power:
            return "Powr";
        case MotorProperties_DirectDrive:
            return "DD";
        default:
            return "***";
        }
    default:
        return "***";
    }
}

VirtualMotor& getMotor(int8_t entity)
{
    return Motors[entity - Entities_LeftMotor];
}

void setEntityProperty(int8_t entity, int8_t property, int8_t value)
{
    switch (entity)
    {
    case Entities_LeftMotor:
    case Entities_RightMotor:
    case Entities_RearMotor:
        getMotor(entity).setProperty(property, value);
        break;

    case Entities_AllMotors:
        for (int8_t m = Entities_LeftMotor; m <= Entities_RearMotor; m++)
            getMotor(m).setProperty(property, value);
        break;
    
    default:    // invalid enity
        break;
    }
}

int8_t getEntityProperty(int8_t entity, int8_t property)
{
    switch (entity)
    {
    case Entities_LeftMotor:
    case Entities_RightMotor:
    case Entities_RearMotor:
        return getMotor(entity).getProperty(property);

    case Entities_AllMotors:    // invalid
    default:    // invalid enity
        return -1;
    }
}

bool getEntityPropertyChanged(int8_t entity, int8_t property)
{
    switch (entity)
    {
    case Entities_LeftMotor:
    case Entities_RightMotor:
    case Entities_RearMotor:
        return getMotor(entity).getPropertyChanged(property);

    case Entities_AllMotors:    // invalid
    default:    // invalid enity
        return false;
    }
}

bool active = true;
bool enabled = true;

const int8_t telMargin = 3;
const int8_t telSpacing = 5;
void drawTelemetry(int8_t entity, int8_t prop, int8_t value)
{
    int16_t x = EntityScreenMap[entity].x;
    int16_t y = EntityScreenMap[entity].y;

    switch (prop)
    {
    case MotorProperties_Goal:
        break;
    case MotorProperties_RPM:
        y += charHeight + telMargin * 2 + telSpacing;
        break;
    case MotorProperties_Power:
        y += 2 * (charHeight + telMargin * 2 + telSpacing);
        break;
    }
    // no descenders in our text, so a slight Y offset to better center vertically
    tft.setCursor(x, y+1);
    tft.fillRect(x-telMargin, y-telMargin, 4 * charWidth + telMargin*2, charHeight + telMargin*2, HX8357_BLACK);
    if (entity == Entities_AllMotors)
    {
        tft.print(getEntityPropertyName(entity, prop));
    }
    else
    {
        switch (prop)
        {
        case MotorProperties_Goal:
        case MotorProperties_RPM:
        case MotorProperties_Power:
            tft.printf("%4i", value);
            break;
        }
    }
}

void activate()
{
    active = true;
    tft.fillRect(0, 0, tftWidth, menuY, RGBto565(54,54,54));
    // fill in labels and cells for all motors and properties
    for (int8_t entity = Entities_LeftMotor; entity <= Entities_AllMotors; entity++)
    {
        for (int8_t prop = MotorProperties_Goal; prop <= MotorProperties_Power; prop++)
        {
            drawTelemetry(entity, prop, 0);
        }
    }
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
            enabled = !enabled;
        return;
    }

    if (enabled)
    {
        switch (btn)
        {
        case PadKeys_cross:
            // kill all motor movement
            VirtualBot::setEntityProperty(Entities_AllMotors, MotorProperties_Goal, 0);
            for (PadKeys k = PadKeys_knob0; k <= PadKeys_knob3; k++)
                Pad::setKnobValue(k, 0);
            break;
        case PadKeys_leftStick:
            velOx = x;
            velOy = y;
            Guidance();
            break;
        case PadKeys_rightStick:
            velOw = -x / 140.0f;
            velOy = y;
            Guidance();
            break;
        case PadKeys_left:
            velOw = x / 350.0f;     // TODO: tune
            Guidance();
            break;
        case PadKeys_right:
            velOw = -x / 350.0f;    // TODO: tune
            Guidance();
            break;
        case PadKeys_knob0Btn:
        case PadKeys_knob1Btn:
        case PadKeys_knob2Btn:
            if (x != 0)
            {
                int ix = btn - PadKeys_knob0Btn;
                VirtualBot::setEntityProperty(Entities_LeftMotor + ix, MotorProperties_Goal, 0);
                Pad::setKnobValue((PadKeys)(PadKeys_knob0 + ix), 0);
            }
            break;
        case PadKeys_knob0:
        case PadKeys_knob1:
        case PadKeys_knob2:
            {
                Entities entity = (Entities)(Entities_LeftMotor + (btn - PadKeys_knob0));
                VirtualBot::setEntityProperty(entity, MotorProperties_Goal, (int8_t)x);
            }
            break;    
        case PadKeys_knob3Btn:
        case PadKeys_knob3:
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
            {
                // while disabled, force knob to stay at current goals
                Entities entity = (Entities)(Entities_LeftMotor + btn - PadKeys_knob0);
                int8_t goal = VirtualBot::getEntityProperty(entity, MotorProperties_Goal);
                Pad::setKnobValue(btn, goal);
            }
            break;    
        }
    }
}

void flush()
{
    uint8_t packet[12];
    uint8_t *p = packet;
    uint8_t len = 0;
    for (int8_t entity = Entities_LeftMotor; entity <= Entities_RearMotor; entity++)
    {
        for (int8_t prop = MotorProperties_Goal; prop <= MotorProperties_DirectDrive; prop++)
        {
            if (getEntityPropertyChanged(entity, prop))
            {
                int8_t value = getEntityProperty(entity, prop);
                if (prop != MotorProperties_RPM && prop != MotorProperties_Power)   // skip readonly
                {
                    *p++ = entity << 4 | prop;
                    *p++ = (int8_t)value;
                    len += 2;
                    if (len >= sizeof(packet))
                    {
                        floge("packet buffer too small");
                        break;
                    }
                }
                if (active)
                    drawTelemetry(entity, prop, value);
            }
        }
    }
    if (len > 0)
    {
        esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)&packet, len);
        if (result != ESP_OK)
            floge("Error sending the data");
    }
}

void plotEntityProperty(int8_t entity, int8_t property)
{
    plot(getEntityName(entity), getEntityPropertyName(entity, property), getEntityProperty(entity, property));
}

void doplot()
{
    for (int8_t entity = Entities_LeftMotor; entity <= Entities_RearMotor; entity++)
    {
        plotEntityProperty(entity, MotorProperties_Goal);
        plotEntityProperty(entity, MotorProperties_RPM);
    }
}
};
