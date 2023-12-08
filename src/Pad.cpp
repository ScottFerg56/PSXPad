#include "Pad.h"
#include <PsxControllerBitBang.h>
#include "FLogger.h"
#include "ScaledKnob.h"

namespace Pad
{
// the PSX controller interfaced using bitbang (brute force IO pin method)
PsxControllerBitBang<PSX_ATT, PSX_CMD, PSX_DAT, PSX_CLK> psx;

// PadKeys_select thru PadKeys_rightStick are all buttons we can detect
// this maps them to their PsxAnalogButton values used by the PsxController library
// A value of -1 indicates buttons that have no analog mode
// indexes matching, in order, with their PadKeys values
int8_t analogBtnMap[] =
{
    -1,             // PadKeys_select,
    -1,             // PadKeys_l3,
    -1,             // PadKeys_r3,
    -1,             // PadKeys_start,
    PSAB_PAD_UP,    // PadKeys_up,
    PSAB_PAD_RIGHT, // PadKeys_right,
    PSAB_PAD_DOWN,  // PadKeys_down,
    PSAB_PAD_LEFT,  // PadKeys_left,
    PSAB_L2,        // PadKeys_l2,
    PSAB_R2,        // PadKeys_r2,
    PSAB_L1,        // PadKeys_l1,
    PSAB_R1,        // PadKeys_r1,
    PSAB_TRIANGLE,  // PadKeys_triangle,
    PSAB_CIRCLE,    // PadKeys_circle,
    PSAB_CROSS,     // PadKeys_cross,
    PSAB_SQUARE,    // PadKeys_square,
    -1,             // PadKeys_analog,
    -1,             // PadKeys_leftStick,
    -1,             // PadKeys_rightStick,
};

// interface to the neopixels in the knobs
Adafruit_seesaw SeeSaw;
seesaw_NeoPixel SSPixel = seesaw_NeoPixel(4, 18, NEO_GRB + NEO_KHZ800);

// interface to the knobs with value ranges we want
// UNDONE: these might want to change in the future based on the active UI page?
ScaledKnob Knob0(0, 12, -100, 100, 5);
ScaledKnob Knob1(1, 14, -100, 100, 5);
ScaledKnob Knob2(2, 17, -100, 100, 5);
ScaledKnob Knob3(3,  9, -255, 255, 5);  // currently unused

// flag when controller has been found
// initialization is deferred and recovery indicated when lost
// a future feature could allow for a touchscreen robot control interface!
bool haveController = false;

/**
 * @brief Set analog mode for buttons and joysticks and lock to disable non-analog mode
 */
void EnableAnalog()
{
    if (!psx.enterConfigMode())
        floge("Cannot enter config mode");
    if (!psx.enableAnalogSticks(true, true))
        floge("Cannot enable analog sticks");
    if (!psx.enableAnalogButtons())
        floge("Cannot enable analog buttons");
    if (!psx.exitConfigMode())
        floge("Cannot exit config mode");
}

/**
 * @brief (re)Initialize the PSX controller
 */
void InitPSX()
{
    haveController = psx.begin();
    if (!haveController)
        floge("Controller not found");
    else
        flogi("Controller found");
    delay(300);
    EnableAnalog();
}
    
void Init()
{
    flogi("SeeSaw init");
    if (!SeeSaw.begin(0x49) || !SSPixel.begin(0x49))
        flogf("%s FAILED", "SeeSaw init");

    SSPixel.setBrightness(50);

    Knob0.Init(&SeeSaw, &SSPixel, 0);
    Knob1.Init(&SeeSaw, &SSPixel, 0);
    Knob2.Init(&SeeSaw, &SSPixel, 0);
    Knob3.Init(&SeeSaw, &SSPixel, 0);

    Knob0.SetColor(128,   0,   0);
    Knob1.SetColor(  0, 128,   0);
    Knob2.SetColor(  0,   0, 128);
    Knob3.SetColor(128,   0, 128);

    InitPSX();
}

// currrent values for the buttons saved for detecting changes
// matching, in order, with their PadKeys values
Point currBtns[PadKeys_knob3-PadKeys_select+1];

// joystick deadzone
// this value is specifically chosen to make it easy to convert the
// normal joystick range of [-128..127] to [-100..100] by just clipping out the deadzone
const int16_t deadzone = 27;

/**
 * @brief Process a pair of new joystick values and compare against the saved values to detect a change
 *
 * @param p A Point value pair representing the previous x,y values
 * @param x A new x value
 * @param y A new y value
 * @return true if the values have changed
 * @return false if the values have not changed
 * @remarks Any value changes are saved back to the reference Point input
 */
bool ProcessStickXY(Point &p, int16_t x, int16_t y)
{
    // convert [0..255]
    // convert to [-127..127]
    int16_t xx = x - 128;
    if (xx == -128)
        xx = -127;
    int16_t yy = y - 128;      
    if (yy == -128)
        yy = -127;
    // flip the Y axes so that positive is up
    yy = -yy;
    // enforce a stick deadzone (27)
    // subtracting it out, leaving values [-100..100]
    // CONSIDER: just using map and constrain?
    if (abs(xx) <= deadzone)
        xx = 0;
    else if (xx > 0)
        xx -= deadzone;
    else
        xx += deadzone;
    if (abs(yy) <= deadzone)
        yy = 0;
    else if (yy > 0)
        yy -= deadzone;
    else
        yy += deadzone;
    if (xx == p.x && yy == p.y)
        return false;
    p.x = xx;
    p.y = yy;
    return true;
}

/**
 * @brief Process a new button value and compare against the saved value to detect a change
 *
 * @param p A Point value pair representing the previous x value (y value ignored)
 * @param x A new x value
 * @return true if the value has changed
 * @return false if the value has not changed
 * @remarks Any value changes are saved back to the reference Point input
 */
bool ProcessBtn(Point &p, byte x)
{
    if (x == p.x)
        return false;
    p.x = x;
    return true;
}

/**
 * @brief Detect a knob button being pressed and compare against the saved value to detect a change
 *
 * @param knob The knob to check
 * @param p A Point value pair representing the previous x value (y value ignored)
 * @return true if the value has changed
 * @return false if the value has not changed
 * @remarks Any value changes are saved back to the reference Point input
 */
bool processKnobBtn(ScaledKnob& knob, Point &p)
{
    ScaledKnob::Presses pressed = knob.Pressed();
    int16_t v = (pressed == ScaledKnob::Presses::Press || pressed == ScaledKnob::Presses::Hold) ? 0xFF : 0;
    if (v == p.x)
        return false;
    p.x = v;
    return true;
}

/**
 * @brief Read a knob (rotary encoder) value and compare against the saved value to detect a change
 *
 * @param knob The knob to check
 * @param p A Point value pair representing the previous x value (y value ignored)
 * @return true if the value has changed
 * @return false if the value has not changed
 * @remarks Any value changes are saved back to the reference Point input
 */
bool processKnob(ScaledKnob& knob, Point &p)
{
    knob.Sample();
    int16_t v = (int)roundf(knob.GetValue());
    if (v == p.x)
        return false;
    p.x = v;
    return true;
}

/**
 * @brief Set the value in the Knob
 * 
 * @param btn The PadKeys ID of the knob to set
 * @param v The value to set
 */
void SetKnobValue(PadKeys btn, int16_t v)
{
    switch (btn)
    {
    case PadKeys_knob0:
        Knob0.SetValue(v);
        break;
    case PadKeys_knob1:
        Knob1.SetValue(v);
        break;
    case PadKeys_knob2:
        Knob2.SetValue(v);
        break;
    case PadKeys_knob3:
        Knob3.SetValue(v);
        break;
    }
}

void Loop(pad_cb func)
{
    // check/restore the PSX connection
    if (!haveController)
    {
        InitPSX();
        if (!haveController)
            return;
    }
    // poll the PSX to read its current button/joystick states
    if (!psx.read())
    {
        floge("Controller lost");
        haveController = false;
    }
    else if (!psx.getAnalogButtonDataValid() || !psx.getAnalogSticksValid())
    {
        // make sure we're in analog mode!
        //flogi("anlog mode reenabled");
        EnableAnalog();
    }
    else
    {
        // process the new state of the PSX joysticks agains their last known state
        byte x, y;
        psx.getLeftAnalog(x, y);
        if (ProcessStickXY(currBtns[PadKeys_leftStick], x, y))
            (*func)(PadKeys_leftStick, currBtns[PadKeys_leftStick].x, currBtns[PadKeys_leftStick].y);
        psx.getRightAnalog(x, y);
        if (ProcessStickXY(currBtns[PadKeys_rightStick], x, y))
            (*func)(PadKeys_rightStick, currBtns[PadKeys_rightStick].x, currBtns[PadKeys_rightStick].y);

        // process the new state of the PSX buttons agains their last known state
        PsxButtons btns = psx.getButtonWord();

        PsxButtons msk = 1;
        for (PadKeys i = PadKeys_select; i <= PadKeys_square; ++i)
        {
            byte v = 0;
            int16_t abtn = analogBtnMap[i];
            if (abtn != -1)
            {
                // analog buttons range [0..255]
                v = psx.getAnalogButton((PsxAnalogButton)abtn);
            }
            else
            {
                // not an anolog-capable button: map to just 0 or 255
                v = (btns & msk) != 0 ? 0xff : 0;
            }
            if (ProcessBtn(currBtns[i], v))
                (*func)(i, currBtns[i].x, 0);
            msk <<= 1;
        }
    }
    // process the new state of the knobs and knob buttons agains their last known state
    if (processKnobBtn(Knob0, currBtns[PadKeys_knob0Btn]))
        (*func)(PadKeys_knob0Btn, currBtns[PadKeys_knob0Btn].x, 0);
    if (processKnobBtn(Knob1, currBtns[PadKeys_knob1Btn]))
        (*func)(PadKeys_knob1Btn, currBtns[PadKeys_knob1Btn].x, 0);
    if (processKnobBtn(Knob2, currBtns[PadKeys_knob2Btn]))
        (*func)(PadKeys_knob2Btn, currBtns[PadKeys_knob2Btn].x, 0);
    if (processKnobBtn(Knob3, currBtns[PadKeys_knob3Btn]))
        (*func)(PadKeys_knob3Btn, currBtns[PadKeys_knob3Btn].x, 0);
    if (processKnob(Knob0, currBtns[PadKeys_knob0]))
        (*func)(PadKeys_knob0, currBtns[PadKeys_knob0].x, 0);
    if (processKnob(Knob1, currBtns[PadKeys_knob1]))
        (*func)(PadKeys_knob1, currBtns[PadKeys_knob1].x, 0);
    if (processKnob(Knob2, currBtns[PadKeys_knob2]))
        (*func)(PadKeys_knob2, currBtns[PadKeys_knob2].x, 0);
    if (processKnob(Knob3, currBtns[PadKeys_knob3]))
        (*func)(PadKeys_knob3, currBtns[PadKeys_knob3].x, 0);
}

};
