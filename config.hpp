#ifndef CONFIG_HPP
#define CONFIG_HPP

// For led chips like Neopixels, which have a data line, ground, and power, you
// just need to define DATA_PIN.  For led chipsets that are SPI based (four
// wires - data, clock, ground, and power), like the LPD8806 define both
// DATA_PIN and CLOCK_PIN
#define DATA_PIN 2

// Which pin controls the backlight state (off/on)?
#define BACKLIGHT_PIN 3

// Which pin controls SD card chip select?
#define SD_CS 4

// How many leds in your strip?
#define NUM_LEDS 288

// Which analog pin monitors battery voltage?
// Undefine if no voltage monitoring is desired.
// #define BATTERY_VOLTAGE_PIN A0

// We assume below 4.0 V is critical and assume a resolution of 10 bits.
#define BATTERY_VOLTAGE_THRESHOLD ((4 * 1024) / 5)

// Define this to flip the display.
// Comment out to keep default orientation.
#define FLIP_DISPLAY

// Language to use
#define LANGUAGE_GERMAN

// Enable or disable some timing debug information on serial output
// #define ENABLE_TIMING

#endif

// vim: et ts=2
