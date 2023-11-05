#include <arduino.h>
#include <DigitalPin.h>
#include "PSXPad.h"

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
#if false
		if (stat == IMAGE_SUCCESS)
		{
			Serial.print(name);
			Serial.println(F(" BMP load succeeded"));
		}
		else
		{
			Serial.print(name);
			Serial.print(F(" BMP load failed: "));
			reader.printStatus(stat);
			Serial.println();
		}
#endif
	}
	return img;
}
 
Adafruit_Image imgPSXPad;

void DBButtonPress(PadKeys btn, bool pressed)
{
    Point pt = KeyImageMap[(int)btn];
    Adafruit_Image& img = GetKeyImage(btn, pressed);
    if (img.getFormat() != IMAGE_NONE)
        img.draw(tft, pt.x, pt.y);
}

void DBAnalog(PadKeys btn, Point p)
{
    tft.setCursor(0, 280);
    tft.setTextSize(2);
    tft.fillRect(0, 280, 24 * 12, 280+16, HX8357_BLUE);
    TFTpreMsg();
    tft.printf("%10s x:%4i y:%4i\r\n", psxButtonNames[(int)btn], p.x, p.y);
    DBButtonPress(btn, p.x != 0 || p.y != 0);
}

void DBinit()
{
	if (imgPSXPad.getFormat() == IMAGE_NONE)
	{
		if (reader.loadBMP("/psxpad.bmp", imgPSXPad) == IMAGE_SUCCESS)
    		imgPSXPad.draw(tft, 0, 0);
	}
}
