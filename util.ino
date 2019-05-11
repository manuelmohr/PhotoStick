#include "util.hpp"
#include "Arduino.h"

__attribute__((noreturn)) void panic(const __FlashStringHelper *pgstr)
{
  Serial.println(pgstr);
  while (1) {
  }
}
