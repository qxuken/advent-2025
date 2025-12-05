#ifndef NUMBERS_UTILS_H
#define NUMBERS_UTILS_H

#include <ctype.h>
#include <stdint.h>

int parse_next_number(unsigned char **p, unsigned char *end, uint64_t *n) {
  while (*p < end && !isdigit(**p))
    (*p)++;
  if (*p >= end)
    return 0;
  unsigned char *q = *p;
  while (q < end && isdigit(*q))
    q++;
  *n = 0;
  for (unsigned char *r = *p; r < q; r++)
    *n = *n * 10 + (*r - '0');
  *p = q;
  return 1;
}

int u64_to_str(uint64_t x, char *buf) {
  char tmp[21];
  int len = 0;

  do {
    tmp[len++] = (char)('0' + (x % 10));
    x /= 10;
  } while (x != 0);

  // reverse into buf
  for (int i = 0; i < len; ++i) {
    buf[i] = tmp[len - 1 - i];
  }
  return len;
}
int int_len(uint64_t n) {
  if (n >= 10000000000000000000ULL) return 20;
  if (n >= 1000000000000000000ULL)  return 19;
  if (n >= 100000000000000000ULL)   return 18;
  if (n >= 10000000000000000ULL)    return 17;
  if (n >= 1000000000000000ULL)     return 16;
  if (n >= 100000000000000ULL)      return 15;
  if (n >= 10000000000000ULL)       return 14;
  if (n >= 1000000000000ULL)        return 13;
  if (n >= 100000000000ULL)         return 12;
  if (n >= 10000000000ULL)          return 11;
  if (n >= 1000000000ULL)           return 10;
  if (n >= 100000000ULL)            return 9;
  if (n >= 10000000ULL)             return 8;
  if (n >= 1000000ULL)              return 7;
  if (n >= 100000ULL)               return 6;
  if (n >= 10000ULL)                return 5;
  if (n >= 1000ULL)                 return 4;
  if (n >= 100ULL)                  return 3;
  if (n >= 10ULL)                   return 2;
  return 1;
}

int u64_compare(const void* a, const void* b) {
   return (int)(*(uint64_t*)a - *(uint64_t*)b);
}

#endif /* ifndef NUMBERS_UTILS_H */
