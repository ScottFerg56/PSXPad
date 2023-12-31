#ifndef _ECHO_H
#define _ECHO_H

#include "PSXPad.h"

namespace Echo
{
    /**
     * @brief Activate the UI page
     */
    void Activate();
    
    /**
     * @brief Deactivate the UI page
     */
    void Deactivate();

    /**
     * @brief Process PSX key activity for the UI page
     * 
     * @param btn the PSX button, joystick or knob changed
     * @param x the x (or only) value
     * @param y the (optional) y value
     * @remark only joysticks will have a y value
     */
    void ProcessKey(PadKeys btn, int16_t x, int16_t y);
};

#endif // _ECHO_H
