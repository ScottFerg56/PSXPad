#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include "Flogger.h"
#include "PSXPad.h"
#include "Controller.h"
#include "Pad.h"
#include "Echo.h"
#include "MotorBase.h"
#include "HeadBase.h"

//
// NOTE: These Entities might best be declared within the Domain subclass,
// as is done with the Properties in the Entity class.
// But that is causing unexplained catastrophic failure that is possibly
// due to C++ random order of object construction??
// Fortunately the domain and these entities ony need one instance,
// so placing them outside the class works well enough locically.
//

MotorBase LeftMotor =  MotorBase(EntityID_LeftMotor, "Left Motor");
MotorBase RightMotor = MotorBase(EntityID_RightMotor, "Right Motor");
MotorBase RearMotor =  MotorBase(EntityID_RearMotor, "Rear Motor");
HeadBase HeadX = HeadBase(EntityID_Head, "Head");
Entity* entities[5] = { &LeftMotor, &RightMotor, &RearMotor, &HeadX, nullptr };

//class Bot : public Domain
//{
//public:
//    Bot() : Domain(false, ea) {}
//};

Domain VirtualBot = Domain(false, entities);

// E8:9F:6D:22:02:EC

uint8_t botMacAddress[] = {0xE8, 0x9F, 0x6D, 0x32, 0xD7, 0xF8};

/**
 * @brief each menu item activates a different UI page
 * 
 */
enum MenuItems
{
    Menu_Telemetry, // show robot telemetry
    Menu_Echo,      // show PSX activity
    Menu_Log,       // show flog messages
    Menu_TBD
};

Adafruit_GFX_Button menu[4];    // menu buttons for the MenuItems
MenuItems menuItem = Menu_Log;  // active UI page

/**
 * @brief Select a UI menu item and activate its UI page
 * 
 * @param newItem The new item to select
 */
void SelectMenuItem(MenuItems newItem)
{
    if (newItem != menuItem)
    {
        // deactivate the old item
        menu[menuItem].drawButton(false);
        switch (menuItem)
        {
        case Menu_Echo:
            Echo::Deactivate();
            break;
        case Menu_Telemetry:
            Controller::Deactivate();
            break;
        case Menu_Log:
        case Menu_TBD:
            break;
        }
        // activate the new item
        menuItem = newItem;
        menu[menuItem].drawButton(true);
        tft.setTextColor(HX8357_WHITE);
        tft.setCursor(0,0);
        switch (menuItem)
        {
        case Menu_Echo:
            Echo::Activate();
            break;
        case Menu_Telemetry:
            Controller::Activate();
            break;
        case Menu_Log:
        case Menu_TBD:
            tft.fillRect(0, 0, tftWidth, menuY, HX8357_BLACK);
            break;
        }
    }
}

/**
 * @brief Advance to the next UI page
 */
void SelectNextMenuItem()
{
    switch (menuItem)
    {
    case Menu_Telemetry:
        SelectMenuItem(Menu_Echo);
        break;
    case Menu_Echo:
        SelectMenuItem(Menu_Log);
        break;
    case Menu_Log:
    default:
        SelectMenuItem(Menu_Telemetry);
        break;
    }
}

SdFat SD;   // images for the Echo UI page are loaded from the SD card
Adafruit_ImageReader reader(SD);

// Init screen on hardware SPI, HX8357D type:
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

int16_t calib[4] = {154, 3812, 276, 3808};  // touch screen calibration
TS_Point lastTSpt;  // the last screen point touched

/**
 * @brief Replacement print function for flog
 * 
 * @param s The string to print
 * @return int The number of characters printed
 */
int flog_printer(const char* s)
{
    int len = Serial.print(s);
    // echo the message only if the log UI is active
    if (menuItem != Menu_Log)
        return len;
    // 12x16 characters
    int16_t y = tft.getCursorY();
    // cycle back to the top once the menu buttons are reached
    if (y >= menuY - charHeight * 2)
    {
        tft.setCursor(0, 0);
        y = 0;
    }
    // clear this line and the next for clarity
    tft.fillRect(0, y, tftWidth, 2*16, HX8357_BLACK);
    return tft.print(s);
}

/**
 * @brief Draw the button menu bar for selecting a UI screen
 */
void DrawMenuButtons()
{
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
}

void setup(void)
{
    FLogger::setPrinter(&flog_printer);
    FLogger::setLogLevel(FLOG_DEBUG);
    Serial.begin(115200);
    delay(2000);    // wait a bit for the computer connection

    tft.begin();    // initialize screen
    tft.setRotation(1);
    tft.fillScreen(HX8357_BLACK);
    tft.setTextSize(2);
    flogi("TFT init");

    flogi("Touchscreen init");
    if (!ts.begin())
        flogf("%s FAILED", "Touchscreen init");

    flogi("SD init");
    // ESP32 requires 25 MHz limit
    if (!SD.begin(SD_CS, SD_SCK_MHZ(25)))
        flogf("%s FAILED", "SD init");

    Pad::Init();

    VirtualBot.Init(botMacAddress);     // starts WiFi and ESP_NOW

    DrawMenuButtons();
    delay(1000);

    SelectMenuItem(Menu_Telemetry);

    flogi("completed");
}

unsigned long timeInputLast = 0;
unsigned long timePlotLast = 0;

void PadCallback(PadKeys btn, int16_t x, int16_t y)
{
    if (menuItem == Menu_Echo)
        Echo::ProcessKey(btn, x, y);
    switch (btn)
    {
    case PadKeys_select:
        if (x != 0)
            SelectNextMenuItem();
        break;
    }
    Controller::ProcessKey(btn, x, y);
}

void ChgCallback(Entity* pe, Property* pp)
{
    if (menuItem == Menu_Telemetry)
        Controller::ProcessChange(pe, pp);
}

void loop()
{
    unsigned long msec = millis();
    // time slice for processing UI inputs and changes from the robot
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
                // Scale to tftWidth using the calibration #'s
                x = map(x, calib[0], calib[1], 0, tftWidth);
                y = map(y, calib[2], calib[3], 0, tftHeight);
                // invert Y axis
                y = tftHeight - y;
                tft.fillCircle(x, y, 3, HX8357_MAGENTA);
                //Serial.print("("); Serial.print(p.x); Serial.print(","); Serial.print(p.y); Serial.println(")");
                MenuItems newItem = menuItem;
                for (int i = Menu_Telemetry; i <= Menu_TBD; i++)
                {
                    if (menu[i].contains(x, y))
                    {
                        SelectMenuItem((MenuItems)i);
                        break;
                    }
                }
            }
        }
        Pad::Loop(&PadCallback);
        VirtualBot.ProcessChanges(&ChgCallback);
    }
    // time slice for processing debug data plots
    dmsec = msec - timePlotLast;
    if (dmsec >= 1000)
    {
        timePlotLast = msec;
        Controller::DoPlot();
    }
}
