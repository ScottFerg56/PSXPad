#ifndef _ECHO_H
#define _ECHO_H

#include "PSXPad.h"

typedef void (*pad_cb)(PadKeys btn, int16_t x, int16_t y);

namespace Echo
{
void activate();
void processKey(PadKeys btn, int16_t x, int16_t y);
};

#endif // _ECHO_H
