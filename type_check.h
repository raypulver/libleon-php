#ifndef LEON_TYPE_CHECK_H
#define LEON_TYPE_CHECK_H

#include <zend.h>
#include "types.h"

zend_always_inline unsigned char integer_type_check(long l) {
  if (( (long) ((unsigned char) l) ) == l) return LEON_UNSIGNED_CHAR;
  if (( (long) ((char) l) ) == l) return LEON_CHAR;
  if (( (long) ((unsigned short) l) ) == l) return LEON_UNSIGNED_SHORT;
  if (( (long) ((short) l) ) == l) return LEON_SHORT;
  if (( (long) ((unsigned int) l) ) == l) return LEON_UNSIGNED_INT;
  if (( (long) ((int) l) ) == l) return LEON_INT;
  return LEON_DOUBLE;
}

zend_always_inline unsigned char fp_type_check(double d) {
  if (( (double) ((float) d) ) == d) return LEON_FLOAT;
  return LEON_DOUBLE;
}

unsigned char type_check(zval *val);

unsigned char determine_array_type(HashTable *ht);

#endif
