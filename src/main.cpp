#include <SdFat.h>                // SD card & FAT filesystem library
#include <Adafruit_SPIFlash.h>    // SPI / QSPI flash library
#include <DigitalPin.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include "ScaledKnob.h"
#include "PSXPad.h"

// E8:9F:6D:22:02:EC

uint8_t broadcastAddress[]
     = {0xE8, 0x9F, 0x6D, 0x32, 0xD7, 0xF8};
    // {0xE8, 0x9F, 0x6D, 0x32, 0xDE, 0x2C};

bool btnPressed;

// Variable to store if sending data was successful
String success;

typedef struct struct_message
{
    bool btnPressed;
} struct_message;

struct_message packetOut;
struct_message packetIn;

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

ScaledKnob Knob0(0, 12, 0, 100, 1);
ScaledKnob Knob1(1, 14, 0, 100, 1);
ScaledKnob Knob2(2, 17, 0, 100, 1);
ScaledKnob Knob3(3,  9, 0, 100, 1);

float valKnob0;
float valKnob1;
float valKnob2;
float valKnob3;

void msg(const char *msg)
{
    Serial.print(msg);
    tft.print(msg);
}

void msgln(const char *msg)
{
    Serial.println(msg);
    tft.println(msg);
}

void err(const char *msg)
{
    Serial.println(msg);
    tft.println(msg);
    while (1)
        ;
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
    Serial.print("\r\nLast packetOut Send Status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    if (status == 0)
        success = "Delivery Success";
    else
        success = "Delivery Fail";
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len)
{
    memcpy(&packetIn, incomingData, sizeof(packetIn));
    TFTpreMsg();
    tft.printf("Bytes received: %i\r\n", len);
}

extern void DBinit();
extern void DBButtonPress(PadKeys btn, bool pressed);
extern void DBAnalog(PadKeys btn, Point p);

bool PSXinit()
{
    if (psx.begin())
    {
        msgln("Controller found!");
        delay(300);
        if (!psx.enterConfigMode())
        {
            msgln("Cannot enter config mode");
            return true;
        }
        if (!psx.enableAnalogSticks())
            msgln("Cannot enable analog sticks");

        if (!psx.enableAnalogButtons())
            msgln("Cannot enable analog buttons");

        if (!psx.exitConfigMode())
            Serial.println("Cannot exit config mode");
        return true;
    }
    return false;
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
    knob.Sample();
    if (knob.Pressed() == ScaledKnob::Presses::Press)
        knob.SetValue(0);
    float v = knob.GetValue();
    if (v != value)
    {
        value = v;
        tft.setCursor(0, 280);
        tft.setTextSize(2);
        tft.fillRect(0, 280, 24 * 12, 280+16, HX8357_BLUE);
        TFTpreMsg();
        tft.printf("knob %i v:%4.0f\r\n", id, v);
    }
}

void setup(void)
{
    led.config(OUTPUT, HIGH);
    Serial.begin(115200);
    delay(2000);
    tft.begin(); // Initialize screen
    tft.setRotation(1);
    tft.fillScreen(HX8357_BLACK);
    msgln("TFT init");

    msgln("Touchscreen init");
    if (!ts.begin())
        err("FAILED");

    haveController = PSXinit();

    // ESP32 requires 25 MHz limit
    msgln("SD init");
    if (!SD.begin(SD_CS, SD_SCK_MHZ(25)))
        err("FAILED");

    msgln("WIFI init");
    if (!WiFi.mode(WIFI_STA))
        err("FAILED");

    msgln("ESP_NOW init");
    if (esp_now_init() != ESP_OK)
        err("FAILED");

    esp_now_register_send_cb(OnDataSent);

    memcpy(peerInfo.peer_addr, broadcastAddress, 6);
    peerInfo.channel = 0;  
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
        err("Failed to add peer");
    
    esp_now_register_recv_cb(OnDataRecv);
    msgln("SeeSaw init");
    if (!SeeSaw.begin(0x49) || !SSPixel.begin(0x49))
        err("FAILED");

    SSPixel.setBrightness(50);

    Knob0.Init(&SeeSaw, &SSPixel, 0);
    Knob1.Init(&SeeSaw, &SSPixel, 0);
    Knob2.Init(&SeeSaw, &SSPixel, 0);
    Knob3.Init(&SeeSaw, &SSPixel, 0);

    Knob0.SetColor(128,   0,   0);
    Knob1.SetColor(  0, 128,   0);
    Knob2.SetColor(  0,   0, 128);
    Knob3.SetColor(128,   0, 128);

    msgln("OK");
    tft.fillScreen(HX8357_BLUE);
    tft.setCursor(0, 280);
    DBinit();
}

void loop()
{
    // Retrieve a point  
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
        msgln("Controller lost");
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
                    if (i == PadKeys_select)
                    {
                        packetOut.btnPressed = pressed;
                        esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &packetOut, sizeof(packetOut));
                        TFTpreMsg();
                        if (result == ESP_OK)
                            tft.println("Sent with success");
                        else
                            tft.println("Error sending the data");
                    }
                    if (i == PadKeys_start && pressed)
                    {
                        Serial.println(WiFi.macAddress());
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

    delay(1000 / 60);
}
