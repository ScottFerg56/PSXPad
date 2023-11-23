#ifndef _VIRTUALBOT_H
#define _VIRTUALBOT_H

#include "\Projects\Rovio\RovioMotor\include\Packet.h"
#include "VirtualMotor.h"
#include "PSXPad.h"

namespace VirtualBot
{
    void init(uint8_t addr[]);
    void setEntityProperty(uint8_t entity, uint8_t property, int16_t value);
    int16_t getEntityProperty(uint8_t entity, uint8_t property);
    const char* getEntityName(uint8_t entity);
    const char* getEntityPropertyName(uint8_t entity, uint8_t property);
    void flush();
    void doplot();
    void activate();
    void deactivate();
    void processKey(PadKeys btn, int16_t x, int16_t y);
};

#endif // _VIRTUALBOT_H