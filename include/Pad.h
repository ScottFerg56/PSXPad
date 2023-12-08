#ifndef _PAD_H
#define _PAD_H

#include "PSXPad.h"

/**
  * @brief     Callback function for processing button/joystick/knob changes
  * @param     pe pointer to the Entity
  * @param     pp pointer to the changed Property
  */
typedef void (*pad_cb)(PadKeys btn, int16_t x, int16_t y);

/**
 * @brief Interface to the PSX game pad
 */
namespace Pad
{
    /**
     * @brief Initialize the PSX game pad controller
     */
    void Init();
    /**
     * @brief Perform periodic loop() processing to detect PSX activity
     * 
     * @param func A callback function to notify the caller of PSX activity
     */
    void Loop(pad_cb func);
    /**
     * @brief Set the value of a controller Knob
     * 
     * @param btn The PadKeys ID for a knob
     * @param v The value to set
     */
    void SetKnobValue(PadKeys btn, int16_t v);
};

#endif // _PAD_H
