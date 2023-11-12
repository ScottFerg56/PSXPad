#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <DigitalPin.h>
#include <Wire.h>
#include "ScaledKnob.h"
#include "PSXPad.h"
#include "VirtualBot.h"
#include "Flogger.h"

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
extern void DBButtonPress(PadKeys btn, int8_t v);
extern void DBAnalog(PadKeys btn, Point p);

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

int8_t analogBtnMap[] =
{
    -1,             // PadKeys_select,
    -1,             // PadKeys_l3,
    -1,             // PadKeys_r3,
    -1,             // PadKeys_start,
    PSAB_PAD_UP,    // PadKeys_up,
    PSAB_PAD_RIGHT, // PadKeys_right,
    PSAB_PAD_DOWN,  // PadKeys_down,
    PSAB_PAD_LEFT,  // PadKeys_left,
    PSAB_L2,        // PadKeys_l2,
    PSAB_R2,        // PadKeys_r2,
    PSAB_L1,        // PadKeys_l1,
    PSAB_R1,        // PadKeys_r1,
    PSAB_TRIANGLE,  // PadKeys_triangle,
    PSAB_CIRCLE,    // PadKeys_circle,
    PSAB_CROSS,     // PadKeys_cross,
    PSAB_SQUARE,    // PadKeys_square,
    -1,             // PadKeys_analog,
    -1,             // PadKeys_leftStick,
    -1,             // PadKeys_rightStick,
};

DigitalPin<LED_BUILTIN> led;

SdFat SD;
Adafruit_ImageReader reader(SD);

// Init screen on hardware SPI, HX8357D type:
Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC);
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

PsxControllerBitBang<PSX_ATT, PSX_CMD, PSX_DAT, PSX_CLK> psx;

int16_t calib[4] = {154, 3812, 276, 3808};
TS_Point lastTSpt;

Adafruit_seesaw SeeSaw;
seesaw_NeoPixel SSPixel = seesaw_NeoPixel(4, 18, NEO_GRB + NEO_KHZ800);

ScaledKnob Knob0(0, 12, -100, 100, 5);
ScaledKnob Knob1(1, 14, -100, 100, 5);
ScaledKnob Knob2(2, 17, -100, 100, 5);
ScaledKnob Knob3(3,  9, -100, 100, 5);

float valKnob0;
float valKnob1;
float valKnob2;
float valKnob3;

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

void enableAnalog()
{
    if (!psx.enterConfigMode())
        floge("Cannot enter config mode");
    if (!psx.enableAnalogSticks(true, true))
        floge("Cannot enable analog sticks");
    if (!psx.enableAnalogButtons())
        floge("Cannot enable analog buttons");
    if (!psx.exitConfigMode())
        floge("Cannot exit config mode");
}

bool PSXinit()
{
    if (!psx.begin())
    {
        floge("Controller not found");
        return false;
    }
    flogi("Controller found");
    delay(300);
    enableAnalog();
    return true;
}

bool haveController = false;
Point currBtns[PadKeys_rightStick-PadKeys_select+1];

const int16_t deadzone = 27;

bool ProcessStickXY(Point &p, byte x, byte y)
{
    // convert [0..255]
    // convert to [-127..127]
    int16_t xx = (int16_t)x - 128;
    if (xx == -128)
        xx = -127;
    int16_t yy = (int16_t)y - 128;      
    if (yy == -128)
        yy = -127;
    // flip the Y axes so that positive is up
    yy = -yy;
    // enforce a stick deadzone (27)
    // subtracting it out, leaving values [-100..100]
    if (abs(xx) <= deadzone)
        xx = 0;
    else if (xx > 0)
        xx -= deadzone;
    else
        xx += deadzone;
    if (abs(yy) <= deadzone)
        yy = 0;
    else if (yy > 0)
        yy -= deadzone;
    else
        yy += deadzone;
    if (xx == p.x && yy == p.y)
        return false;
    p.x = xx;
    p.y = yy;
    return true;
}

bool ProcessBtn(Point &p, byte x)
{
    if (x == p.x)
        return false;
    p.x = x;
    return true;
}

void processKnob(ScaledKnob& knob, int id, float& value)
{
    float v = 0;
    if (knob.Pressed() == ScaledKnob::Presses::Press)
    {
        knob.SetValue(0);
    }
    else
    {
        knob.Sample();
        v = knob.GetValue();
    }
    if (v != value)
    {
        value = v;
        if (id < 3)
        {
            VirtualBot::setEntityProperty(Entities_LeftMotor + id, MotorProperties_Goal, (int8_t)value);
        }
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

    flogi("SeeSaw init");
    if (!SeeSaw.begin(0x49) || !SSPixel.begin(0x49))
        flogf("%s FAILED", "SeeSaw init");

    VirtualBot::init(botMacAddress);

    SSPixel.setBrightness(50);

    Knob0.Init(&SeeSaw, &SSPixel, 0);
    Knob1.Init(&SeeSaw, &SSPixel, 0);
    Knob2.Init(&SeeSaw, &SSPixel, 0);
    Knob3.Init(&SeeSaw, &SSPixel, 0);

    Knob0.SetColor(128,   0,   0);
    Knob1.SetColor(  0, 128,   0);
    Knob2.SetColor(  0,   0, 128);
    Knob3.SetColor(128,   0, 128);

    //ESP_LOGD("main", "setup completed");
    flogi("completed");

    haveController = PSXinit();

    delay(2000);

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
        if (!haveController)
        {
            haveController = PSXinit();
            if (!haveController)
                return;
        }
        if (!psx.read())
        {
            floge("Controller lost");
            haveController = false;
        }
        else if (!psx.getAnalogButtonDataValid() || !psx.getAnalogSticksValid())
        {
            //flogi("anlog mode reenabled");
            enableAnalog();
        }
        else
        {
            byte x, y;
            psx.getLeftAnalog(x, y);
            if (ProcessStickXY(currBtns[PadKeys_leftStick], x, y))
                DBAnalog(PadKeys_leftStick, currBtns[PadKeys_leftStick]);
            psx.getRightAnalog(x, y);
            if (ProcessStickXY(currBtns[PadKeys_rightStick], x, y))
                DBAnalog(PadKeys_rightStick, currBtns[PadKeys_rightStick]);

            PsxButtons btns = psx.getButtonWord();

            PsxButtons msk = 1;
            for (PadKeys i = PadKeys_select; i <= PadKeys_square; ++i)
            {
                byte v = 0;
                int8_t abtn = analogBtnMap[i];
                if (abtn != -1)
                {
                    v = psx.getAnalogButton((PsxAnalogButton)abtn);
                }
                else
                {
                    v = (btns & msk) != 0 ? 0xff : 0;
                }
                if (ProcessBtn(currBtns[i], v))
                {
                    DBButtonPress(i, v);
                    if (i == PadKeys_select && v != 0)
                        selectNextMenuItem();
                }
                msk <<= 1;
            }
        }
        processKnob(Knob0, 0, valKnob0);
        processKnob(Knob1, 1, valKnob1);
        processKnob(Knob2, 2, valKnob2);
        processKnob(Knob3, 3, valKnob3);

        VirtualBot::flush();
    }
    dmsec = msec - timePlotLast;
    if (dmsec >= 1000)
    {
        timePlotLast = msec;
        VirtualBot::doplot();
    }
}
