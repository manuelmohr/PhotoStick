#include <Adafruit_GFX.h>    // Core graphics library
#include <SPI.h>
#include <Wire.h>      // this is needed even tho we aren't using it
#include <Adafruit_ILI9341.h>
#include <Adafruit_STMPE610.h>
#include "SD.h"
#include "FastLED.h"

// This is calibration data for the raw touch data to the screen coordinates
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.
// The display also uses hardware SPI, plus #9 & #10
#define TFT_CS 10
#define TFT_DC 9
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// The STMPE610 uses hardware SPI on the shield, and #8
#define STMPE_CS 8
Adafruit_STMPE610 ts = Adafruit_STMPE610(STMPE_CS);

// SD card chip select
#define SD_CS 4

// How many leds in your strip?
#define NUM_LEDS 2

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
    Serial.println(F("Couldn't start touchscreen controller"));
    while (1);
  }
  Serial.println(F("Touchscreen started"));

  Serial.print(F("Initializing SD card..."));
  if (!SD.begin(SD_CS)) {
    Serial.println(F("failed!"));
    while (1);
  }
  Serial.println(F("OK!"));

  tft.fillScreen(ILI9341_BLACK);

  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(1);
}

void clearLeds()
{
  for (unsigned i = 0; i < NUM_LEDS; ++i) {
    leds[i] = CRGB::Black;
  }
}

uint16_t showRow = 0;
void loop()
{
  if (showRow < NUM_LEDS) {
    bmpShow("Test1.bmp", (showRow++) % NUM_LEDS);
    delay(5);
  } else {
    clearLeds();
  }
  FastLED.show();
}

#define BUFFPIXEL 20

void bmpShow(const char *filename, uint16_t showRow)
{
  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col, x2, y2, bx1, by1;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

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
  if(read16(bmpFile) != 0x4D42) {
    Serial.print(F("Invalid header"));
    return;
  }

  Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
  (void)read32(bmpFile); // Read & ignore creator bytes
  bmpImageoffset = read32(bmpFile); // Start of image data
  Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
  // Read DIB header
  Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
  bmpWidth  = read32(bmpFile);
  bmpHeight = read32(bmpFile);
  if(read16(bmpFile) != 1) { // # planes -- must be '1'
    Serial.print(F("#planes must be 1"));
    return;
  }

  bmpDepth = read16(bmpFile); // bits per pixel
  Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
  if ((bmpDepth != 24) || (read32(bmpFile) != 0)) { // 0 = uncompressed
    Serial.print(F("Depth wrong or compressed file"));
    return;
  }

  Serial.print(F("Image size: "));
  Serial.print(bmpWidth);
  Serial.print('x');
  Serial.println(bmpHeight);
  if (bmpWidth != NUM_LEDS) {
    Serial.print(F("Wrong width: "));
    Serial.println(bmpWidth);
  }

  // BMP rows are padded (if needed) to 4-byte boundary
  rowSize = (bmpWidth * 3 + 3) & ~3;

  // If bmpHeight is negative, image is in top-down order.
  // This is not canon but has been observed in the wild.
  if(bmpHeight < 0) {
    bmpHeight = -bmpHeight;
    flip      = false;
  }

  row = showRow;

  // Seek to start of scan line.  It might seem labor-
  // intensive to be doing this on every line, but this
  // method covers a lot of gritty details like cropping
  // and scanline padding.  Also, the seek only takes
  // place if the file position actually needs to change
  // (avoids a lot of cluster math in SD library).
  if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
    pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
  else     // Bitmap is stored top-to-bottom
    pos = bmpImageoffset + row * rowSize;
  if(bmpFile.position() != pos) { // Need seek?
    bmpFile.seek(pos);
    buffidx = sizeof(sdbuffer); // Force buffer reload
  }
  for (col=0; col<NUM_LEDS; col++) { // For each pixel...
    // Time to read more pixel data?
    if (buffidx >= sizeof(sdbuffer)) { // Indeed
      bmpFile.read(sdbuffer, sizeof(sdbuffer));
      buffidx = 0; // Set index to beginning
    }
    // Convert pixel from BMP to TFT format, push to display
    b = sdbuffer[buffidx++];
    g = sdbuffer[buffidx++];
    r = sdbuffer[buffidx++];
    leds[col] = CRGB(r, g, b);
  }

  Serial.print(F("Loaded in "));
  Serial.print(millis() - startTime);
  Serial.println(F(" ms"));

  bmpFile.close();
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
