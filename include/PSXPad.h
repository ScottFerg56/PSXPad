#ifndef _PSXPAD_H
#define _PSXPAD_H

#include <Adafruit_GFX.h>         // Core graphics library
#include <Adafruit_HX8357.h>      // Hardware-specific library
#include <Adafruit_STMPE610.h>
#include <Adafruit_ImageReader.h> // Image-reading functions
#include "Domain.h"

/**
 * @brief Convert color RGB byte values to an integer 565 format value for the HX8357 screen
 * 
 * @param r Red value [0..255]
 * @param g Green value [0..255]
 * @param b Blue value [0..255]
 * @return uint16_t 16-bit value with RGB packed as 5 bits red, 6 bits green and 5 bits blue
 */
inline uint16_t RGBto565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

// serial print a value and its name(s) for the VS Code Teleplot extension to recognize and plot
#define plot(name1, name2, v) do {} while(0)
//#define plot(name1, name2, v) Serial.printf(">%s %s:%i\r\n", name1, name2, (int)(v))

const byte STMPE_CS = 32;   // touchscreen I2C chip select pin
const byte TFT_CS = 15;     // TFT screen I2C chip select pin
const byte TFT_DC = 33;     // TFT screen data control pin
const byte SD_CS = 14;      // SD card I2C chip select pin

const byte PSX_DAT = 37;    // PSX data SPI data input pin (input only pin!)
const byte PSX_CMD = 27;    // PSX data SPI command output pin
const byte PSX_CLK = 13;    // PSX data SPI clock output pin
const byte PSX_ATT = 12;    // PSX data SPI attention (CS) output pin

extern Adafruit_ImageReader reader;
extern Adafruit_HX8357 tft;
extern Adafruit_STMPE610 ts;

// display characteristics
const int16_t tftWidth = 480;
const int16_t tftHeight = 320;
const uint8_t charWidth = 12;
const uint8_t charHeight = 16;

// menu bar characteristics
const int8_t menuItems = 4;
const uint16_t menuItemWidth = tftWidth / menuItems;
const uint16_t menuItemHeight = charHeight + 12;
const int16_t menuY = tftHeight - menuItemHeight;

/**
 * @brief Structure for an x,y data point
 */
struct Point
{
    int16_t x;
    int16_t y;
};

/**
 * @brief IDs for each of the buttons, joysticks present on the PSX controller
 *      and the knobs mounted above it
 * @remarks The number and order of these is highly depended upon by data structures throughout!!
 */
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

/**
 * @brief postfix increment operator for PadKeys
 * 
 * @param k The PadKeys reference to increment
 * @return PadKeys reference after increment
 */
inline PadKeys& operator++(PadKeys& k)
{
    k = (PadKeys)((int)k + 1);
    return k;
}

/**
 * @brief postfix increment operator for PadKeys
 * 
 * @param k The PadKeys reference to increment
 * @return PadKeys reference before increment
 */
inline PadKeys operator++(PadKeys& k, int)
{
    PadKeys o = k;
    k = (PadKeys)((int)k + 1);
    return o;
}

/**
 * @brief The Domain of Rovio Entities and Properties to control
 */
extern Domain VirtualBot;

#endif // _PSXPAD_H
