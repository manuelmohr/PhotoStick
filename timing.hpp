#ifndef TIMING_HPP
#define TIMING_HPP

#include "config.hpp"

#ifdef ENABLE_TIMING
namespace Timing
{
struct Stats
{
  unsigned long minMs;
  unsigned long maxMs;
  unsigned long sumMs;
  unsigned long num;

  Stats()
  {
    minMs = ULONG_MAX;
    maxMs = 0;
    sumMs = 0;
    num   = 0;
  }

  void update(unsigned long tookMs)
  {
    if (tookMs < minMs)
      minMs = tookMs;
    if (tookMs > maxMs)
      maxMs = tookMs;
    sumMs += tookMs;

    ++num;
  }

  double getAverage() const
  {
    return ((double)sumMs) / num;
  }

  void println() const
  {
    Serial.print(F("min="));
    Serial.print(minMs);
    Serial.print(F(" max="));
    Serial.print(maxMs);
    Serial.print(F(" avg="));
    Serial.println(getAverage());
  }
};
}

#define TIME(statPtr, op)                   \
  do {                                      \
    const unsigned long startMs = millis(); \
    (op);                                   \
    const unsigned long stopMs = millis();  \
    (statPtr)->update(stopMs - startMs);    \
  } while (false)
#else
#define TIME(statPtr, op) (op)
#endif

#endif
