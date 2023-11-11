#ifndef _VIRTUALBOT_H
#define _VIRTUALBOT_H

#include "\Projects\Rovio\RovioMotor\include\Packet.h"
#include "VirtualMotor.h"

namespace VirtualBot
{
    void init(uint8_t addr[]);
    VirtualMotor& getMotor(int8_t motor);
    void setEntityProperty(int8_t entity, int8_t property, int8_t value);
    int8_t getEntityProperty(int8_t entity, int8_t property);
    const char* getEntityName(int8_t entity);
    const char* getEntityPropertyName(int8_t entity, int8_t property);
    void flush();
    void doplot();
};

#endif // _VIRTUALBOT_H