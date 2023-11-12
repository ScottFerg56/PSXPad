#ifndef _PAD_H
#define _PAD_H

#include "PSXPad.h"

typedef void (*pad_cb)(PadKeys btn, int16_t x, int16_t y);

namespace Pad
{
void init();
void loop(pad_cb func);
void setKnobValue(PadKeys btn, int16_t v);
};

#endif // _PAD_H
