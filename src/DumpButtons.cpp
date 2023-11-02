/*******************************************************************************
 * This file is part of PsxNewLib.                                             *
 *                                                                             *
 * Copyright (C) 2019-2020 by SukkoPera <software@sukkology.net>               *
 *                                                                             *
 * PsxNewLib is free software: you can redistribute it and/or                  *
 * modify it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation, either version 3 of the License, or           *
 * (at your option) any later version.                                         *
 *                                                                             *
 * PsxNewLib is distributed in the hope that it will be useful,                *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with PsxNewLib. If not, see http://www.gnu.org/licenses.              *
 *******************************************************************************
 *
 * This sketch will dump to serial whatever is done on a PSX controller. It is
 * an excellent way to test that all buttons/sticks are read correctly.
 *
 * It's missing support for analog buttons, that will come in the future.
 *
 * This example drives the controller through the hardware SPI port, so pins are
 * fixed and depend on the board/microcontroller being used. For instance, on an
 * Arduino Uno connections must be as follows:
 *
 * CMD: Pin 11
 * DATA: Pin 12
 * CLK: Pin 13
 *
 * Any pin can be used for ATTN, but please note that most 8-bit AVRs require
 * the HW SPI SS pin to be kept as an output for HW SPI to be in master mode, so
 * using that pin for ATTN is a natural choice. On the Uno this would be pin 10.
 *
 * It also works perfectly on OpenPSX2AmigaPadAdapter boards (as it's basically
 * a modified Uno).
 *
 * There is another similar one using a bitbanged protocol implementation that
 * can be used on any pins/board.
 */

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
    tft.printf("%10s x:%4i y:%4i", psxButtonNames[(int)btn], p.x, p.y);
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
