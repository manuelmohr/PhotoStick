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
#define COLORWIDTH 240
int oldcolor, currentcolor;

// How many leds in your strip?
#define NUM_LEDS 288

// For led chips like Neopixels, which have a data line, ground, and power, you just
// need to define DATA_PIN.  For led chipsets that are SPI based (four wires - data, clock,
// ground, and power), like the LPD8806 define both DATA_PIN and CLOCK_PIN
#define DATA_PIN 2

// Define the array of leds
CRGB leds[NUM_LEDS];

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

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
}

int ledWidth = 80;
int ledPos = ledWidth / 2 + 1;
CRGB ledColor = CRGB::White;

#define WRAP(x) ((x + NUM_LEDS) % NUM_LEDS)

void loop()
{
  leds[WRAP(ledPos - ledWidth / 2)] = CRGB::Black;
  ++ledPos;
  if (ledPos == NUM_LEDS) {
    ledPos = 0;
  }
  for (int i = -ledWidth / 2; i <= ledWidth / 2; ++i) {
    leds[WRAP(ledPos + i)] = ledColor;
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

  unsigned hue = (float(p.y) / tft.height()) * 255;
  ledColor = CRGB(CHSV(hue, 255, 255));
}

// vim: et ts=2
