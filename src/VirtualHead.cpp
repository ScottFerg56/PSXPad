#include "VirtualHead.h"
#include "FLogger.h"

void VirtualHead::setProperty(Properties property, int16_t value)
{
    switch (property)
    {
    case Properties_Power:
        setPower(value);
        break;
    case Properties_Position:
        setPosition(value);
        break;
    }
}

int16_t VirtualHead::getProperty(Properties property)
{
    switch (property)
    {
    case Properties_Power:
        return Power;
    case Properties_Position:
        return Position;
    }
    return -1;
}

bool VirtualHead::getPropertyChanged(Properties property)
{
    bool changed = false;
    switch (property)
    {
    case Properties_Power:
        changed = PowerChanged;
        PowerChanged = false;
        break;
    case Properties_Position:
        changed = PositionChanged;
        PositionChanged = false;
        break;
    }
    return changed;
}

bool VirtualHead::propertyToBot(Properties property)
{
    switch (property)
    {
    case Properties_Power:
        return true;
    default:
        return false;
    }
}

bool VirtualHead::propertyFromBot(Properties property)
{
    switch (property)
    {
    case Properties_Position:
    case Properties_Power:
        return true;
    default:
        return false;
    }
}
