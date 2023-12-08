#ifndef _CONTROLLER_H
#define _CONTROLLER_H

#include <Arduino.h>
#include "Domain.h"
#include "PSXPad.h"

namespace Controller
{
    /**
     * @brief Serial print interesting values for plotting using the Teleplot VS Code extension
     */
    void DoPlot();

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

    /**
     * @brief     Process Property changes (from another remote Domain)
     * @param     pe pointer to the Entity
     * @param     pp pointer to the changed Property
     */
    void ProcessChange(Entity* pe, Property* pp);
};

#endif // _CONTROLLER_H
