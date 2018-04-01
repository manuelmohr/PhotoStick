#ifndef UTIL_HPP
#define UTIL_HPP

__attribute__((noreturn)) void panic(const __FlashStringHelper *pgstr)
{
  Serial.println(pgstr);
  while (1) {
  }
}

#endif
