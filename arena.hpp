#ifndef ARENA_HPP
#define ARENA_HPP

#include "util.hpp"

template<size_t SIZE> class Arena
{
private:
  uint8_t memory[SIZE];
  size_t  current;

public:
  Arena() : current(0) {}
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

  void *allocate(size_t size)
  {
    if (current + size > SIZE) {
      panic(F("Out of memory"));
    }
    void *ret = (void*)(memory + current);
    current += size;
    return ret;
  }

  template<typename T> void destroy(T *p)
  {
    if (p != nullptr) {
      p->~T();
    }
  }

  size_t reset()
  {
    size_t old = current;
    current = 0;
    return old;
  }

  size_t set(size_t pos)
  {
    size_t old = current;
    if (pos < SIZE) {
      current = pos;
    }
    return old;
  }
};

#endif
