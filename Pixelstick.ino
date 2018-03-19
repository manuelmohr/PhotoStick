#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include <SD.h>
#include "FastLED.h"

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.
// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// SD card chip select
#define SD_CS 4

// Size of the color selection boxes and the paintbrush size
#define COLORWIDTH 120
int oldcolor, currentcolor;

// How many leds in your strip?
#define NUM_LEDS 20

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

  Serial.print("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    Serial.println("failed!");
    while (1);
  }
  Serial.println("OK!");

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

  bmpDraw("purple.bmp", (tft.width()  / 2), (tft.height() / 2));

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

// This function opens a Windows Bitmap (BMP) file and
// displays it at the given coordinates.  It's sped up
// by reading many pixels worth of data at a time
// (rather than pixel by pixel).  Increasing the buffer
// size takes more of the Arduino's precious RAM but
// makes loading a little faster.  20 pixels seems a
// good balance.

#define BUFFPIXEL 20

void bmpDraw(char *filename, int16_t x, int16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col, x2, y2, bx1, by1;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.print(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        x2 = x + bmpWidth  - 1; // Lower-right corner
        y2 = y + bmpHeight - 1;
        if((x2 >= 0) && (y2 >= 0)) { // On screen?
          w = bmpWidth; // Width/height of section to load/display
          h = bmpHeight;
          bx1 = by1 = 0; // UL coordinate in BMP file
          if(x < 0) { // Clip left
            bx1 = -x;
            x   = 0;
            w   = x2 + 1;
          }
          if(y < 0) { // Clip top
            by1 = -y;
            y   = 0;
            h   = y2 + 1;
          }
          if(x2 >= tft.width())  w = tft.width()  - x; // Clip right
          if(y2 >= tft.height()) h = tft.height() - y; // Clip bottom

          // Set TFT address window to clipped image bounds
          tft.startWrite(); // Requires start/end transaction now
          tft.setAddrWindow(x, y, w, h);

          for (row=0; row<h; row++) { // For each scanline...

            // Seek to start of scan line.  It might seem labor-
            // intensive to be doing this on every line, but this
            // method covers a lot of gritty details like cropping
            // and scanline padding.  Also, the seek only takes
            // place if the file position actually needs to change
            // (avoids a lot of cluster math in SD library).
            if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
              pos = bmpImageoffset + (bmpHeight - 1 - (row + by1)) * rowSize;
            else     // Bitmap is stored top-to-bottom
              pos = bmpImageoffset + (row + by1) * rowSize;
            pos += bx1 * 3; // Factor in starting column (bx1)
            if(bmpFile.position() != pos) { // Need seek?
              tft.endWrite(); // End TFT transaction
              bmpFile.seek(pos);
              buffidx = sizeof(sdbuffer); // Force buffer reload
              tft.startWrite(); // Start new TFT transaction
            }
            for (col=0; col<w; col++) { // For each pixel...
              // Time to read more pixel data?
              if (buffidx >= sizeof(sdbuffer)) { // Indeed
                tft.endWrite(); // End TFT transaction
                bmpFile.read(sdbuffer, sizeof(sdbuffer));
                buffidx = 0; // Set index to beginning
                tft.startWrite(); // Start new TFT transaction
              }
              // Convert pixel from BMP to TFT format, push to display
              b = sdbuffer[buffidx++];
              g = sdbuffer[buffidx++];
              r = sdbuffer[buffidx++];
              tft.writePixel(tft.color565(r,g,b));
            } // end pixel
          } // end scanline
          tft.endWrite(); // End last TFT transaction
        } // end onscreen
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

// vim: et ts=2
