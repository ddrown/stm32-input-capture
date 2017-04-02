#include <stdint.h>
#include <ieee754.h>

#include "float.h"

uint32_t htonf (float val)
{
  union ieee754_float u;
  uint32_t v;
  uint8_t *un = (uint8_t *) &v;

  u.f = val;
  un[0] = (u.ieee.negative << 7) + ((u.ieee.exponent & 0xfe) >> 1);
  un[1] = ((u.ieee.exponent & 0x01) << 7) + ((u.ieee.mantissa & 0x7f0000) >> 16);
  un[2] = (u.ieee.mantissa & 0xff00) >> 8;
  un[3] = (u.ieee.mantissa & 0xff);
  return v;
}

uint64_t htond(double val)
{
  union ieee754_double u;
  uint64_t v;
  uint8_t *un = (uint8_t *) &v;

  u.d = val;
  un[0] = (u.ieee.negative << 7) + ((u.ieee.exponent & 0b11111110000) >> 4);
  un[1] = ((u.ieee.exponent & 0b1111) << 4) + ((u.ieee.mantissa0 & 0b11110000000000000000) >> 16);
  un[2] = (u.ieee.mantissa0 & 0b1111111100000000) >> 8;
  un[3] = (u.ieee.mantissa0 & 0xff);
  un[4] = (u.ieee.mantissa1 & 0xff000000) >> 24;
  un[5] = (u.ieee.mantissa1 & 0xff0000) >> 16;
  un[6] = (u.ieee.mantissa1 & 0xff00) >> 8;
  un[7] = (u.ieee.mantissa1 & 0xff);
  return v;
}

float ntohf (uint32_t val)
{
  union ieee754_float u;
  uint8_t *un = (uint8_t *) &val;

  u.ieee.negative = (un[0] & 0x80) >> 7;
  u.ieee.exponent = (un[0] & 0x7f) << 1;
  u.ieee.exponent += (un[1] & 0x80) >> 7;
  u.ieee.mantissa = (un[1] & 0x7f) << 16;
  u.ieee.mantissa += un[2] << 8;
  u.ieee.mantissa += un[3];

  return u.f;
}

double ntohd (uint64_t val)
{
  union ieee754_double u;
  uint8_t *un = (uint8_t *) &val;

  u.ieee.negative = (un[0] & 0x80) >> 7;
  u.ieee.exponent = (un[0] & 0b1111111) << 4;
  u.ieee.exponent += (un[1] & 0b11110000) >> 4;
  u.ieee.mantissa0 = (un[1] & 0b1111) << 16;
  u.ieee.mantissa0 += un[2] << 8;
  u.ieee.mantissa0 += un[3];

  u.ieee.mantissa1 = un[4] << 24;
  u.ieee.mantissa1 += un[5] << 16;
  u.ieee.mantissa1 += un[6] << 8;
  u.ieee.mantissa1 += un[7];

  return u.d;
}
