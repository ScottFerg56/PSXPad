#ifndef _VIRTUALBOT_H
#define _VIRTUALBOT_H

#include "\Projects\Rovio\RovioMotor\include\Packet.h"
#include "VirtualMotor.h"
#include "PSXPad.h"

namespace VirtualBot
{
    void init(uint8_t addr[]);
    void setEntityProperty(Entities entity, Properties property, int16_t value);
    int16_t getEntityProperty(Entities entity, Properties property);
    const char* getEntityName(Entities entity);
    const char* getPropertyName(Properties property);
    void flush();
    void doplot();
    void activate();
    void deactivate();
    void processKey(PadKeys btn, int16_t x, int16_t y);
};

#endif // _VIRTUALBOT_H