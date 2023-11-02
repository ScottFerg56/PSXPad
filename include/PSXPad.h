#ifndef _PSXPAD_H
#define _PSXPAD_H

#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_HX8357.h>      // Hardware-specific library
#include <Adafruit_STMPE610.h>
#include <Adafruit_ImageReader.h> // Image-reading functions
#include <PsxControllerBitBang.h>

const byte STMPE_CS = 32;
const byte TFT_CS = 15;
const byte TFT_DC = 33;
const byte SD_CS = 14;

const byte PSX_DAT = 37;    // input only
const byte PSX_CMD = 27;
const byte PSX_CLK = 13;
const byte PSX_ATT = 12;

extern Adafruit_ImageReader reader;
extern Adafruit_HX8357 tft;
extern Adafruit_STMPE610 ts;
extern PsxControllerBitBang<PSX_ATT, PSX_CMD, PSX_DAT, PSX_CLK> psx;

struct Point
{
    int16_t x;
    int16_t y;
};

const int16_t deadzone = 20;

void msg(const char *msg);
void msgln(const char *msg);
void err(const char *msg);

enum PadKeys
{
    PadKeys_select,
    PadKeys_l3,
    PadKeys_r3,
    PadKeys_start,
    PadKeys_up,
    PadKeys_right,
    PadKeys_down,
    PadKeys_left,
    PadKeys_l2,
    PadKeys_r2,
    PadKeys_l1,
    PadKeys_r1,
    PadKeys_triangle,
    PadKeys_circle,
    PadKeys_cross,
    PadKeys_square,
    PadKeys_analog,
    PadKeys_leftStick,
    PadKeys_rightStick,
};

inline PadKeys& operator++(PadKeys& k)
{
    k = (PadKeys)((int)k + 1);
    return k;
}

#endif // _PSXPAD_H
