#include "endianness.h"

int is_little_endian() {
  union no_alias {
    char bytes[4];
    unsigned int i;
  } na;
  na.i = 1;
  if (na.bytes[0]) return 1;
  else return 0;
}
