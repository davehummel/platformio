#include <Arduino.h>
#include "SPI.h"
#include "ILI9341_t3.h"
#if defined(CORE_TEENSY)
#include <ADC.h> /* https://github.com/pedvide/ADC */
#else
#include <elapsedMillis.h>
#endif

#include "c12880.h"

#define SPEC_TRG 6
#define SPEC_ST 7
#define SPEC_CLK 8
#define SPEC_VIDEO A0
#define MIN_SPECT_VLT 28000

#define BTN_PIN 0
#define KNOB_PIN A9

#define DSP_SCK 13
#define DSP_MISO 12
#define DSP_MOSI 11
#define DSP_CS 10
#define DSP_DC 9
#define DSP_RST 5

ILI9341_t3 tft = ILI9341_t3(DSP_CS, DSP_DC, DSP_RST);

uint16_t data[C12880_NUM_CHANNELS];

C12880_Class spec(SPEC_TRG, SPEC_ST, SPEC_CLK, SPEC_VIDEO);

uint16_t raw[288] = {0};
uint16_t minSample = 0;
uint8_t samples[288] = {255};
uint8_t rSamples[288] = {255};
uint8_t gSamples[288] = {255};
uint8_t bSamples[288] = {255};

/******************************************************************************/
void setup()
{
    pinMode(KNOB_PIN, INPUT);
    pinMode(BTN_PIN, INPUT_PULLDOWN);

    Serial.begin(115200); // Baud Rate set to 115200
                          // Setup callbacks for SerialCommand commands

    delay(500);
    Serial.println("Starting...");
    tft.begin();

    tft.setRotation(1);
    tft.fillScreen(ILI9341_BLACK);
    tft.setTextColor(ILI9341_BLUE);
    tft.setTextSize(1);
    tft.println("Display Enabled ...");

    tft.println("Starting Spec...");
    spec.begin();
    tft.println("Ready...");
    delay(500);
}
/******************************************************************************/
double sampleInterval = 0.01;
void checkKnob()
{
    uint16_t knobInput = max((analogRead(KNOB_PIN) - 5500), 0) / 1000;

    while (knobInput < 20 || knobInput > 30)
    {
        if (knobInput < 20)
        {
            if (knobInput >= 16)
                sampleInterval -= .0001;
            else if (knobInput >= 12)
                sampleInterval -= .001;
            else if (knobInput >= 8)
                sampleInterval -= .01;
            else
                sampleInterval -= .1;
        }
        else
        {
            if (knobInput <= 35)
                sampleInterval += .0001;
            else if (knobInput <= 40)
                sampleInterval += .001;
            else if (knobInput <= 45)
                sampleInterval += .01;
            else
                sampleInterval += .1;
        }
        if (sampleInterval < 0)
            sampleInterval = 0;
        if (sampleInterval > 2)
            sampleInterval = 2;
        tft.fillRect(260, 0, 320, 10, ILI9341_DARKCYAN);
        tft.setTextSize(1);
        tft.setTextColor(ILI9341_RED);
        tft.setCursor(260, 0);
        tft.printf("%1.4f", sampleInterval);
  
        tft.print('s');
        delay(200);
        knobInput = max((analogRead(KNOB_PIN) - 5500), 0) / 1000;
    }
}
void loop()
{
    if (digitalRead(BTN_PIN))
    {
        tft.fillScreen(ILI9341_PINK);
        for (uint16_t x = 0; x < 288; x++)
        {
            bSamples[x] = gSamples[x];
            gSamples[x] = rSamples[x];
            rSamples[x] = samples[x];
        }
        delay(500);
        while (digitalRead(BTN_PIN))
        {
            delay(10); // wait for release
        }
    }
    tft.fillScreen(ILI9341_BLACK);
    for (uint16_t x = 0; x < 288; x++)
    {
        bool skipGreen = false, skipBlue = false;
        if (rSamples[x] != samples[x] && rSamples[x] < 240)
        {
            if (rSamples[x] == gSamples[x])
            {
                skipGreen = true;
            }
            if (rSamples[x] == bSamples[x])
            {
                skipBlue = true;
            }

            if (skipBlue && skipGreen)
            {
                tft.drawPixel(x, rSamples[x], ILI9341_DARKGREY);
            }
            else if (skipBlue)
            {
                tft.drawPixel(x, rSamples[x], ILI9341_MAGENTA);
            }
            else if (skipGreen)
            {
                tft.drawPixel(x, rSamples[x], ILI9341_YELLOW);
            }
            else
            {
                tft.drawPixel(x, rSamples[x], ILI9341_RED);
            }
        }
        if (gSamples[x] != samples[x] && !skipGreen && gSamples[x] < 240)
        {
            if (gSamples[x] == bSamples[x])
            {
                skipBlue = true;
            }
            if (skipBlue)
            {
                tft.drawPixel(x, gSamples[x], ILI9341_CYAN);
            }
            else
            {
                tft.drawPixel(x, gSamples[x], ILI9341_GREEN);
            }
        }

        if (bSamples[x] != samples[x] && !skipBlue && bSamples[x] < 240)
        {
            tft.drawPixel(x, bSamples[x], ILI9341_BLUE);
        }

        if (samples[x] < 240)
        {
            tft.drawPixel(x, samples[x], ILI9341_WHITE);
        }
    }

    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(260, 0);
    tft.printf("%1.4f", sampleInterval);
    tft.print('s');

    spec.set_integration_time(sampleInterval);
    spec.read_into(raw);

    minSample = MIN_SPECT_VLT;

    for (uint16_t x = 0; x < 288; x++)
    {
        if (raw[x] < minSample)
        {
            minSample = raw[x];
        }
    }

    int16_t divisor = (65535 - minSample) / 240;

    for (uint16_t x = 0; x < 288; x++)
    {
        samples[x] = 239 - min((int)((raw[x] - minSample) / divisor), 239);
    }

    for (uint8_t i = 0; i < 100; i++)
    {
        delay(10);
        checkKnob();
    }
}
