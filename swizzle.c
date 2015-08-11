#include <stddef.h>

void swizzle(unsigned char *start, size_t len) {
  unsigned char *end = start + len - 1;
  while (start < end) {
    char tmp = *start;
    *start++ = *end;
    *end-- = tmp;
  }
}
