#ifndef LEON_TYPE_CHECK_H
#define LEON_TYPE_CHECK_H

#include <zend.h>
#include "types.h"

unsigned char integer_type_check(long l);
unsigned char fp_type_check(double d);

unsigned char type_check(zval *val);

unsigned char determine_array_type(HashTable *ht);

#endif
