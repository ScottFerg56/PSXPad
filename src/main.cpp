#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <DigitalPin.h>
#include <Wire.h>
#include "Flogger.h"
#include "PSXPad.h"
#include "VirtualBot.h"
#include "Pad.h"

// E8:9F:6D:22:02:EC

uint8_t botMacAddress[] = {0xE8, 0x9F, 0x6D, 0x32, 0xD7, 0xF8};

uint16_t RGBto565(uint8_t r, uint8_t g, uint8_t b)
{
    return ((r / 8) << 11) | ((g / 4) << 5) | (b / 8);
}

enum MenuItems
{
    Menu_Telemetry,
    Menu_Echo,
    Menu_Log,
    Menu_TBD
};

Adafruit_GFX_Button menu[4];
int menuItem = Menu_TBD;

extern void DBinit();
extern void DBEcho(PadKeys btn, int16_t x, int16_t y);

void selectMenuItem(int newItem)
{
    if (newItem != menuItem)
    {
        menu[menuItem].drawButton(false);
        menuItem = newItem;
        menu[menuItem].drawButton(true);
        tft.setTextColor(HX8357_WHITE);
        switch (menuItem)
        {
        case Menu_Echo:
            tft.setCursor(0,0);
            DBinit();
            break;
        case Menu_Telemetry:
        case Menu_Log:
        case Menu_TBD:
            tft.fillRect(0, 0, tftWidth, menuY, HX8357_BLACK);
            tft.setCursor(0,0);
            break;
        
        default:
            break;
        }
    }
}

void selectNextMenuItem()
{
    int newItem = menuItem + 1;
    if (newItem > Menu_TBD)
        newItem = Menu_Telemetry;
    selectMenuItem(newItem);
}

DigitalPin<LED_BUILTIN> led;

SdFat SD;
Adafruit_ImageReader reader(SD);

// Init screen on hardware SPI, HX8357D type:
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

int16_t calib[4] = {154, 3812, 276, 3808};
TS_Point lastTSpt;

int flog_printer(const char* s)
{
    Serial.print(s);
    // 12x16 characters
    int16_t y = tft.getCursorY();
    if (y >= tft.height())
    {
        tft.setCursor(0, 0);
        y = 0;
    }
    // clear this line and the next for clarity
    tft.fillRect(0, y, tft.width(), 2*16, HX8357_BLUE);
    return tft.print(s);
}

void TFTpreMsg()
{
    // psxPad BMP is 318x271 pixels (26.5 12-pixel characters wide)
    int16_t x = tft.getCursorX();
    int16_t y = tft.getCursorY();
    if (y >= 280 + 2 * 16)
    {
        tft.setCursor(0, 280);
        x = 0;
        y = 280;
    }
    if (x == 0)
    {
        tft.setTextSize(2); // 12x16 characters
        tft.fillRect(0, y, 26 * 12, 280+16, HX8357_BLUE);
    }
}

void setup(void)
{
    FLogger::setPrinter(&flog_printer);
    FLogger::setLogLevel(FLOG_DEBUG);
    led.config(OUTPUT, HIGH);
    Serial.begin(115200);
    delay(2000);

    tft.begin(); // Initialize screen
    tft.setRotation(1);
    tft.fillScreen(HX8357_BLACK);
    tft.setTextSize(2);
    flogi("TFT init");

    flogi("Touchscreen init");
    if (!ts.begin())
        flogf("%s FAILED", "Touchscreen init");

    // ESP32 requires 25 MHz limit
    flogi("SD init");
    if (!SD.begin(SD_CS, SD_SCK_MHZ(25)))
        flogf("%s FAILED", "SD init");

    VirtualBot::init(botMacAddress);

    Pad::init();

    flogi("completed");

    delay(1000);

    for (int i = Menu_Telemetry; i <= Menu_TBD; i++)
    {
        int16_t x = i * menuItemWidth;
        const char* label;
        switch (i)
        {
        case Menu_Telemetry: label = "TELEM"; break;
        case Menu_Echo:      label = "ECHO";  break;
        case Menu_Log:       label = "LOG";   break;
        case Menu_TBD:       label = "TBD";   break;
        }
        menu[i].initButtonUL(&tft, x, menuY, menuItemWidth, menuItemHeight, HX8357_WHITE, RGBto565(0x50,0x50,0x50), RGBto565(0xB0,0xB0,0xB0), (char*)label, 2);
        menu[i].drawButton();
    }
    selectMenuItem(Menu_Echo);
}

unsigned long timeInputLast = 0;
unsigned long timePlotLast = 0;

void padCallback(PadKeys btn, int16_t x, int16_t y)
{
    if (menuItem == Menu_Echo)
        DBEcho(btn, x, y);
    switch (btn)
    {
    case PadKeys_select:
        if (x != 0)
            selectNextMenuItem();
        break;
    case PadKeys_knob0Btn:
    case PadKeys_knob1Btn:
    case PadKeys_knob2Btn:
    case PadKeys_knob3Btn:
        if (x != 0)
            Pad::setKnobValue((PadKeys)(btn+4), 0);
        break;
    case PadKeys_knob0:
    case PadKeys_knob1:
    case PadKeys_knob2:
        {
            Entities entity = (Entities)(Entities_LeftMotor + (btn - PadKeys_knob0));
            VirtualBot::setEntityProperty(entity, MotorProperties_Goal, (int8_t)x);
        }
        break;    
    case PadKeys_knob3:
        break;
    default:
        break;
    }
}

void loop()
{
    unsigned long msec = millis();
    unsigned long dmsec = msec - timeInputLast;
    if (dmsec >= 20)
    {
        timeInputLast = msec;
        if (!ts.bufferEmpty())
        {
            TS_Point p = ts.getPoint();
            if (p != lastTSpt)
            {
                lastTSpt = p;
                // flip X and Y for the current rotation
                int16_t x = p.y;
                int16_t y = p.x;
                // Scale to tft.width using the calibration #'s
                x = map(x, calib[0], calib[1], 0, tft.width());
                y = map(y, calib[2], calib[3], 0, tft.height());
                // invert Y axis
                y = tft.height() - y;
                tft.fillCircle(x, y, 3, HX8357_MAGENTA);
                //Serial.print("("); Serial.print(p.x); Serial.print(","); Serial.print(p.y); Serial.println(")");
                int newItem = -1;
                for (int i = Menu_Telemetry; i <= Menu_TBD; i++)
                {
                    if (menu[i].contains(x, y))
                    {
                        newItem = i;
                        break;
                    }
                }
                if (newItem != -1)
                {
                    selectMenuItem(newItem);
                }
            }
        }
        Pad::loop(&padCallback);
        VirtualBot::flush();
    }
    dmsec = msec - timePlotLast;
    if (dmsec >= 1000)
    {
        timePlotLast = msec;
        VirtualBot::doplot();
    }
}
