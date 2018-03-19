#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include "FastLED.h"

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Size of the color selection boxes and the paintbrush size
#define COLORWIDTH 120
int oldcolor, currentcolor;

// How many leds in your strip?
#define NUM_LEDS 288

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 2

// Define the array of leds
CRGB leds[NUM_LEDS];

#define BUTTON_SIZE 60

unsigned nextButtonX, nextButtonY;
unsigned prevButtonX, prevButtonY;

void setup(void)
{
  Serial.begin(9600);
  Serial.println(F("Pixelstick"));

  tft.begin();
  if (!ts.begin()) {
    Serial.println("Couldn't start touchscreen controller");
    while (1);
  }
  Serial.println("Touchscreen started");

  tft.fillScreen(ILI9341_BLACK);

  // make the color selection
  const unsigned boxWidth = tft.width();
  unsigned pos = 0;
  for (unsigned hue = 0; hue < 256; ++hue) {
    const CRGB c(CHSV(hue, 255, 255));
    const uint16_t c565 = tft.color565(c.r, c.g, c.b);
    const unsigned boxHeight = 1 + unsigned(hue % 5 == 0);
    tft.fillRect(0, pos, COLORWIDTH, boxHeight, c565);
    pos += boxHeight;
  }

  nextButtonX = prevButtonX = tft.width() / 2 + BUTTON_SIZE / 2;
  nextButtonY = tft.height() / 2 - (BUTTON_SIZE + BUTTON_SIZE / 2);
  prevButtonY = tft.height() / 2 + BUTTON_SIZE;
  tft.fillRect(nextButtonX, nextButtonY, BUTTON_SIZE, BUTTON_SIZE, ILI9341_WHITE);
  tft.fillRect(prevButtonX, prevButtonY, BUTTON_SIZE, BUTTON_SIZE, ILI9341_WHITE);

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
}

CRGB ledColor = CRGB::White;

#define WRAP(x) ((x + NUM_LEDS) % NUM_LEDS)

bool insideButton(const TS_Point& p, unsigned buttonX, unsigned buttonY)
{
  return    p.x >= buttonX && p.x < buttonX + BUTTON_SIZE
         && p.y >= buttonY && p.y < buttonY + BUTTON_SIZE;
}

#define NUM_ANIMS 2
unsigned ledAnimNum = 0;

void animBlink(unsigned call)
{
  const unsigned freq = 2;
  const CRGB c = (call % freq < (freq / 2)) ? ledColor : CRGB::Black;
  for (unsigned i = 0; i < NUM_LEDS; ++i) {
    leds[i] = c;
  }
}


void animAlternate(unsigned call)
{
  const unsigned freq = 100;
  const bool odd = (call % freq < (freq / 2));
  for (unsigned i = 0; i < NUM_LEDS; ++i) {
    if (i % 2 == odd) {
      leds[i] = ledColor;
    } else {
      leds[i] = CRGB::Black;
    }
  }
}

void animShift(unsigned ledPos)
{
  const unsigned ledWidth = 80;

  leds[WRAP(ledPos - ledWidth / 2)] = CRGB::Black;
  for (int i = -ledWidth / 2; i <= ledWidth / 2; ++i) {
    leds[WRAP(ledPos + i)] = ledColor;
  }
}

void clearLeds()
{
  for (unsigned i = 0; i < NUM_LEDS; ++i) {
    leds[i] = CRGB::Black;
  }
}

unsigned numCall = 0;
void loop()
{
  ++numCall;
  switch (ledAnimNum) {
  case 1:
    animBlink(numCall);
    break;
  case 0:
    animAlternate(numCall);
    break;
  }
  FastLED.show();

  // See if there's any touch data for us
  if (ts.bufferEmpty()) {
    return;
  }

  // Retrieve a point
  TS_Point p = ts.getPoint();

  // Scale from ~0->4000 to tft.width using the calibration #'s
  p.x = map(p.x, TS_MINX, TS_MAXX, 0, tft.width());
  p.y = map(p.y, TS_MINY, TS_MAXY, 0, tft.height());

  if (p.x < 120) {
    unsigned hue = (float(p.y) / tft.height()) * 255;
    ledColor = CRGB(CHSV(hue, 255, 255));
  } else if (insideButton(p, nextButtonX, nextButtonY)) {
    clearLeds();
    ledAnimNum = (ledAnimNum + 1) % NUM_ANIMS;
  } else if (insideButton(p, prevButtonX, prevButtonY)) {
    clearLeds();
    ledAnimNum = (ledAnimNum - 1) % NUM_ANIMS;
  }
}

// vim: et ts=2
