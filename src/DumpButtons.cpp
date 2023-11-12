#include <arduino.h>
#include <DigitalPin.h>
#include "PSXPad.h"
#include "FLogger.h"

int16_t padX = 0;
int16_t padY = 0;
uint16_t msgWidth = 13 * charWidth;
uint16_t msgHeight = 2 * charHeight;
uint16_t msgX = (tftWidth - msgWidth) / 2;
uint16_t msgY = menuY - msgHeight;

void msgPrep()
{
    tft.setCursor(msgX, msgY);
    tft.setTextSize(2);
    tft.setTextColor(HX8357_WHITE);
    tft.fillRect(msgX, msgY, msgWidth, msgHeight, HX8357_BLUE);
}

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

Adafruit_Image KeyImages[19];
Adafruit_Image KeyImagesRed[19];

Point KeyImageMap[] =
{
	{123,141},	// select,
	{ 89,171},	// l3,
	{184,171},	// r3,
	{180,140},	// start,
	{ 55,119},	// up,
	{ 70,136},	// right,
	{ 55,151},	// down,
	{ 38,136},	// left,
	{ 47,  0},	// l2,
	{235,  0},	// r2,
	{ 47, 51},	// l1,
	{235, 51},	// r1,
	{243,109},	// triangle
	{270,134},	// circle,
	{243,159},	// cross,
	{216,134},	// square,
	{152,170},	// analog,
	{ 89,171},	// leftStick,
	{184,171},	// rightStick,
};

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
 
Adafruit_Image imgPSXPad;

void drawButton(PadKeys btn, bool red)
{
    Point pt = KeyImageMap[(int)btn];
    Adafruit_Image& img = GetKeyImage(btn, red);
    if (img.getFormat() != IMAGE_NONE)
        img.draw(tft, pt.x + padX, pt.y + padY);
}

void DBButtonPress(PadKeys btn, int8_t v)
{
    drawButton(btn, v != 0);
    msgPrep();
    if (v != 0)
        tft.printf("    %4u    ", (uint8_t)v);
}

void DBAnalog(PadKeys btn, Point p)
{
    bool zeroed = p.x == 0 && p.y == 0;
    drawButton(btn, !zeroed);
    msgPrep();
    if (!zeroed)
        tft.printf("%4i    %4i", p.x, p.y);
}

void DBinit()
{
    tft.fillRect(0, 0, tftWidth, menuY, HX8357_BLUE);
	if (imgPSXPad.getFormat() == IMAGE_NONE)
	{
		ImageReturnCode stat = reader.loadBMP("/psxpad.bmp", imgPSXPad);
        if (stat != IMAGE_SUCCESS)
        {
            floge("image load failed: %i", stat);
            return;
        }
	}
    padX = (tftWidth - imgPSXPad.width()) / 2;
    padY = menuY - imgPSXPad.height();
    imgPSXPad.draw(tft, padX, padY);
}
