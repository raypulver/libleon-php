#ifndef LEON_STRING_INDEX_H
#define LEON_STRING_INDEX_H

#include <zend.h>
#include <zend_string.h>
#include <php.h>

typedef struct {
  zend_string **index;
  size_t len;
  size_t a;
} string_index_t;

string_index_t *string_index_ctor();
void string_index_dtor(string_index_t* p_string_index);
void string_index_push(string_index_t* p_string_index, zend_string* str);
void string_index_set_push(string_index_t* p_string_index, zend_string* str);
long string_index_find(string_index_t* p_string_index, zend_string* needle);
zend_string* string_index_get(string_index_t* p_string_index, int i);

#endif
