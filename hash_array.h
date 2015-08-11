#ifndef ZVAL_ARRAY_H
#define ZVAL_ARRAY_H

#include "php.h"

typedef struct {
  zend_string *key;
  zval *value;
} hash_entry;

typedef struct {
  hash_entry **index;
  size_t len;
} hash_array_t;

hash_array_t *hash_array_ctor();
void hash_array_dtor(hash_array_t *);
void hash_array_push(hash_array_t *, hash_entry *);
void hash_array_sort(hash_array_t *);

#endif
