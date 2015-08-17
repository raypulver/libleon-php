#ifndef ZVAL_ARRAY_H
#define ZVAL_ARRAY_H

#include "php.h"

#if PHP_API_VERSION <= 20131106
typedef struct {
  char *key;
  uint len;
  zend_ulong hash;
  zval *value;
} hash_entry;
#else
typedef struct {
  zend_string *key;
  zend_ulong hash;
  zval *value;
} hash_entry;
#endif

typedef struct {
  hash_entry **index;
  size_t len;
  size_t a;
} hash_array_t;

hash_array_t *hash_array_ctor();
void hash_array_dtor(hash_array_t *);
void hash_array_push(hash_array_t *, hash_entry *);
void hash_array_sort(hash_array_t *);

#endif
