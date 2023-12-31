#include "Echo.h"
#include "FLogger.h"

namespace Echo
{
// location for the PSX controller image
int16_t padX = 0;
int16_t padY = 0;
// metrics for the location of a message area on the image between the controller handles
uint16_t msgWidth = 13 * charWidth;
uint16_t msgHeight = 2 * charHeight;
uint16_t msgX = (tftWidth - msgWidth) / 2;
uint16_t msgY = menuY - msgHeight;

// names for all the PSX buttons
// matching, in order, with their PadKeys values
const char *const psxButtonNames[] =
{
    "Select",
    "L3",
    "R3",
    "Start",
    "Up",
    "Right",
    "Down",
    "Left",
    "L2",
    "R2",
    "L1",
    "R1",
    "Triangle",
    "Circle",
    "Cross",
    "Square",
    "Analog",
    "LeftStick",
    "RightStick",
};

// inactive (untinted) button images
// matching, in order, with their PadKeys values
Adafruit_Image KeyImages[19];
// active (red-tinted) button images
// matching, in order, with their PadKeys values
Adafruit_Image KeyImagesRed[19];

// locations, relative to the PSX image, for all the PSX buttons
// matching, in order, with their PadKeys values
Point KeyImageMap[] =
{
	{123,125},	// select,
	{ 89,155},	// l3,
	{184,155},	// r3,
	{180,124},	// start,
	{ 55,103},	// up,
	{ 70,120},	// right,
	{ 55,135},	// down,
	{ 38,120},	// left,
	{ 47,  0},	// l2,
	{235,  0},	// r2,
	{ 47, 38},	// l1,
	{235, 38},	// r1,
	{243, 93},	// triangle
	{270,118},	// circle,
	{243,143},	// cross,
	{216,118},	// square,
	{152,154},	// analog,
	{ 89,155},	// leftStick,
	{184,155},	// rightStick,
};

// since some keys share images, this mapping points to the primary holder of the image
// matching, in order, with their PadKeys values
PadKeys ImageShareMap[] =
{
    PadKeys_select,
    PadKeys_l3,
    PadKeys_l3,         // PadKeys_r3
    PadKeys_start,
    PadKeys_up,
    PadKeys_right,
    PadKeys_down,
    PadKeys_left,
    PadKeys_l2,
    PadKeys_l2,         // PadKeys_r2
    PadKeys_l1,
    PadKeys_l1,         // PadKeys_r1
    PadKeys_triangle,
    PadKeys_circle,
    PadKeys_cross,
    PadKeys_square,
    PadKeys_analog,
    PadKeys_l3,         // PadKeys_leftStick
    PadKeys_l3,         // PadKeys_rightStick
};

/**
 * @brief Get the Key's Image
 * 
 * @param btn The PadKeys key
 * @param red true to return the red-tinted (active) image
 * @return Adafruit_Image& The Key's image
 * @remarks Defers loading of the images until requested
 */
Adafruit_Image& GetKeyImage(PadKeys btn, bool red)
{
    // several images are identical and are shared to save memory and SD space
    btn = ImageShareMap[(int)btn];
	Adafruit_Image& img = red ? KeyImagesRed[btn] : KeyImages[btn];
	if (img.getFormat() == IMAGE_NONE)
	{
		char name[20];
		strcpy(name, "/");
		strcat(name, psxButtonNames[btn]);
        if (red)
			strcat(name, "r");
		strcat(name, ".bmp");
		ImageReturnCode stat = reader.loadBMP(name, img);
        if (stat != IMAGE_SUCCESS)
            floge("image load failed: %i", stat);
	}
	return img;
}
 
Adafruit_Image imgPSXPad;   // the image of the PSX controller

/**
 * @brief Draw the image of a button
 * 
 * @param btn The PadKeys button/joystick to draw
 * @param red true to draw the red-tinted (active) image
 */
void DrawButton(PadKeys btn, bool red)
{
    if (btn >= PadKeys_knob0Btn)
        return;
    Point pt = KeyImageMap[(int)btn];
    Adafruit_Image& img = GetKeyImage(btn, red);
    if (img.getFormat() != IMAGE_NONE)
        img.draw(tft, pt.x + padX, pt.y + padY);
}

void ProcessKey(PadKeys btn, int16_t x, int16_t y)
{
    bool zeroed = x == 0 && y == 0;
    DrawButton(btn, !zeroed);
    switch (btn)
    {
    case PadKeys_knob0:
    case PadKeys_knob1:
    case PadKeys_knob2:
    case PadKeys_knob3:
        {
            // echo the knob values on the top line below them
            int16_t x = (btn - PadKeys_knob0) * menuItemWidth;
            tft.fillRect(x, 1, menuItemWidth, charHeight, HX8357_BLUE);
            tft.setCursor(x, 1);
        }
        break;
    default:
        // echo button/joystick values in the 'msg' space
        tft.fillRect(msgX, msgY, msgWidth, msgHeight, HX8357_BLUE);
        tft.setCursor(msgX, msgY);
        break;
    }
    tft.setTextSize(2);
    tft.setTextColor(HX8357_WHITE);
    // leave zeroed values blank
    if (!zeroed)
    {
        switch (btn)
        {
        case PadKeys_leftStick:
        case PadKeys_rightStick:
            tft.printf("%4i    %4i", x, y);
            break;
        default:
            tft.printf("    %4i    ", x);
            break;
        }
    }
}

void Activate()
{
    // clear the screen, preserving the menu buttons
    tft.fillRect(0, 0, tftWidth, menuY, HX8357_BLUE);
    // load the PSX image
	if (imgPSXPad.getFormat() == IMAGE_NONE)
	{
		ImageReturnCode stat = reader.loadBMP("/psxpad.bmp", imgPSXPad);
        if (stat != IMAGE_SUCCESS)
        {
            floge("image load failed: %i", stat);
            return;
        }
	}
    // center the image in the area above the menu buttons
    padX = (tftWidth - imgPSXPad.width()) / 2;
    padY = menuY - imgPSXPad.height() - 4;
    // draw it
    imgPSXPad.draw(tft, padX, padY);
}

void Deactivate()
{
}

};
