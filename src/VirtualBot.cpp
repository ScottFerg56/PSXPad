#include "VirtualBot.h"
#include "FLogger.h"
#include <esp_now.h>
#include <WiFi.h>
#include "PSXPad.h"

namespace VirtualBot
{
VirtualMotor Motors[3];

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
        switch (property)
        {
        case MotorProperties_Goal:
            return "Goal";
        case MotorProperties_RPM:
            return "RPM";
        case MotorProperties_DirectDrive:
            return "DirectDrive";
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

void flush()
{
    uint8_t packet[12];
    uint8_t *p = packet;
    uint8_t len = 0;
    for (int8_t entity = Entities_LeftMotor; entity <= Entities_RearMotor; entity++)
    {
        for (int8_t prop = MotorProperties_Goal; entity <= MotorProperties_DirectDrive; entity++)
        {
            if (prop == MotorProperties_RPM)   // skip readonly
                continue;
            if (getEntityPropertyChanged(entity, prop))
            {
                int8_t value = getEntityProperty(entity, prop);
                *p++ = entity << 4 | prop;
                *p++ = (int8_t)value;
                len += 2;
                if (len >= sizeof(packet))
                {
                    floge("packet buffer too small");
                    break;
                }
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
