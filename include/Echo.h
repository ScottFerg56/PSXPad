#ifndef _ECHO_H
#define _ECHO_H

#include "PSXPad.h"

namespace Echo
{
void activate();
void deactivate();
void processKey(PadKeys btn, int16_t x, int16_t y);
};

#endif // _ECHO_H
