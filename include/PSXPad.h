#ifndef _PSXPAD_H
#define _PSXPAD_H

#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_HX8357.h>      // Hardware-specific library
#include <Adafruit_STMPE610.h>
#include <Adafruit_ImageReader.h> // Image-reading functions

inline uint16_t RGBto565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}


#define plot(name1, name2, v) do {} while(0)
//#define plot(name1, name2, v) Serial.printf(">%s %s:%i\r\n", name1, name2, (int)(v))

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

const int16_t tftWidth = 480;
const int16_t tftHeight = 320;
const uint8_t charWidth = 12;
const uint8_t charHeight = 16;

const int8_t menuItems = 4;
const uint16_t menuItemWidth = tftWidth / menuItems;
const uint16_t menuItemHeight = charHeight + 12;
const int16_t menuY = tftHeight - menuItemHeight;

struct Point
{
    int16_t x;
    int16_t y;
};

void TFTpreMsg();

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
    PadKeys_knob0Btn,
    PadKeys_knob1Btn,
    PadKeys_knob2Btn,
    PadKeys_knob3Btn,
    PadKeys_knob0,
    PadKeys_knob1,
    PadKeys_knob2,
    PadKeys_knob3,
};

// prefix ++
inline PadKeys& operator++(PadKeys& k)
{
    k = (PadKeys)((int)k + 1);
    return k;
}

// postfix ++
inline PadKeys operator++(PadKeys& k, int)
{
    PadKeys o = k;
    k = (PadKeys)((int)k + 1);
    return o;
}

#endif // _PSXPAD_H
