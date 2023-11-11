#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <DigitalPin.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include "ScaledKnob.h"
#include "PSXPad.h"
#include "\Projects\Rovio\RovioMotor\include\Packet.h"
#include "VirtualMotor.h"
#include "Flogger.h"

#define plot(name1, name2, v) do {} while(0)
//#define plot(name1, name2, v) Serial.printf(">%s %s:%i\r\n", name1, name2, (int)(v))

// E8:9F:6D:22:02:EC

uint8_t broadcastAddress[] = {0xE8, 0x9F, 0x6D, 0x32, 0xD7, 0xF8};

VirtualMotor Motors[3];
const char* MotorPropertiesName[] =
{
    "Goal",
    "RPM",
    "DirectDrive",
};

esp_now_peer_info_t peerInfo;

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

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    if (status != ESP_NOW_SEND_SUCCESS)
        flogw("Delivery Fail");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *pData, int len)
{
    if (len == 0 || (len & 1) != 0)
    {
        floge("invalid data packet length");
        return;
    }
    while (len > 0)
    {
        int8_t entity = (*pData & 0xF0) >> 4;
        int8_t property = *pData++ & 0x0F;
        int8_t value = *pData++;
        len -= 2;
        switch (entity)
        {
        case Entities_LeftMotor:
        case Entities_RightMotor:
        case Entities_RearMotor:
            Motors[entity-Entities_LeftMotor].SetProperty((MotorProperties)property, value);
            break;

        case Entities_AllMotors:
            for (int8_t e = Entities_LeftMotor; e <= Entities_RearMotor; e++)
                Motors[e-Entities_LeftMotor].SetProperty((MotorProperties)property, value);
            break;
        
        default:    // invalid enity
            break;
        }
    }
}

extern void DBinit();
extern void DBButtonPress(PadKeys btn, bool pressed);
extern void DBAnalog(PadKeys btn, Point p);

bool PSXinit()
{
    if (!psx.begin())
    {
        floge("Controller not found");
        return false;
    }
    flogi("Controller found");
    delay(300);
    if (!psx.enterConfigMode())
        floge("Cannot enter config mode");
    if (!psx.enableAnalogSticks())
        floge("Cannot enable analog sticks");
    if (!psx.enableAnalogButtons())
        floge("Cannot enable analog buttons");
    if (!psx.exitConfigMode())
        floge("Cannot exit config mode");
    return true;
}

bool haveController = false;
Point lastLeftStick;
Point lastRightStick;
uint16_t lastBtns;

bool ProcessStickXY(Point &p, byte x, byte y)
{
    // UNDONE: map values around the missing deadzone?
    // convert [0..255] to [-127..127] (byte to signed byte)
    int16_t xx = (int16_t)x - 128;
    if (xx == -128)
        xx = -127;
    int16_t yy = (int16_t)y - 128;      
    if (yy == -128)
        yy = -127;
    // flip the Y axes so that positive is up
    yy = -yy;
    // enforce a stick deadzone
    if (abs(xx) <= deadzone)
        xx = 0;
    if (abs(yy) <= deadzone)
        yy = 0;
    if (xx == p.x && yy == p.y)
        return false;
    p.x = xx;
    p.y = yy;
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
            int8_t packet[2];
            packet[0] = (Entities_LeftMotor + id) << 4 | MotorProperties_Goal;
            packet[1] = (int8_t)value;
            esp_err_t result = esp_now_send(broadcastAddress, (uint8_t*)&packet, 2);
            if (result != ESP_OK)
                floge("Error sending the data");
            Motors[id].Goal = (int8_t)value;
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

    Motors[0].Name = "Left Motor";
    Motors[1].Name = "Right Motor";
    Motors[2].Name = "Rear Motor";

    tft.begin(); // Initialize screen
    tft.setRotation(1);
    tft.fillScreen(HX8357_BLACK);
    tft.setTextSize(2);
    flogi("TFT init");

    flogi("Touchscreen init");
    if (!ts.begin())
        flogf("%s FAILED", "Touchscreen init");

    haveController = PSXinit();

    // ESP32 requires 25 MHz limit
    flogi("SD init");
    if (!SD.begin(SD_CS, SD_SCK_MHZ(25)))
        flogf("%s FAILED", "SD init");

    flogi("WIFI init");
    if (!WiFi.mode(WIFI_STA))
        flogf("%s FAILED", "WIFI init");

    flogi("ESP_NOW init");
    if (esp_now_init() != ESP_OK)
        flogf("%s FAILED", "ESP_NOW init");

    esp_now_register_send_cb(OnDataSent);

    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
        flogf("%s FAILED", "ESP_NOW peer add");
    
    esp_now_register_recv_cb(OnDataRecv);
    flogi("SeeSaw init");
    if (!SeeSaw.begin(0x49) || !SSPixel.begin(0x49))
        flogf("%s FAILED", "SeeSaw init");

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
    delay(2000);
    tft.fillScreen(HX8357_BLUE);
    tft.setCursor(0, 280);
    DBinit();
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
        else
        {
            PsxButtons btns = psx.getButtonWord();

            PsxButtons chg = btns ^ lastBtns;
            if (chg != 0)
            {
                PsxButtons msk = 1;
                for (PadKeys i = PadKeys_select; i <= PadKeys_square; ++i)
                {
                    if (chg & msk)
                    {
                        bool pressed = (btns & msk) != 0;
                        DBButtonPress(i, pressed);
                        if (i == PadKeys_start && pressed)
                        {
                            flogi("MAC addr: %s", WiFi.macAddress());
                        }
                    }
                    msk <<= 1;
                }
            }
            lastBtns = btns;

            byte x, y;
            psx.getLeftAnalog(x, y);
            if (ProcessStickXY(lastLeftStick, x, y))
                DBAnalog(PadKeys_leftStick, lastLeftStick);
            psx.getRightAnalog(x, y);
            if (ProcessStickXY(lastRightStick, x, y))
                DBAnalog(PadKeys_rightStick, lastRightStick);
        }
        processKnob(Knob0, 0, valKnob0);
        processKnob(Knob1, 1, valKnob1);
        processKnob(Knob2, 2, valKnob2);
        processKnob(Knob3, 3, valKnob3);
    }
    dmsec = msec - timePlotLast;
    if (dmsec >= 1000)
    {
        timePlotLast = msec;
        for (int8_t e = Entities_LeftMotor; e <= Entities_RearMotor; e++)
        {
            plot(Motors[e-Entities_LeftMotor].Name, MotorPropertiesName[MotorProperties_Goal], Motors[e-Entities_LeftMotor].Goal);
            plot(Motors[e-Entities_LeftMotor].Name, MotorPropertiesName[MotorProperties_RPM], Motors[e-Entities_LeftMotor].RPM);
        }
    }
}
