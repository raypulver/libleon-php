#ifndef LEON_STRINGBUFFER_H
#define LEON_STRINGBUFFER_H

#include "php.h"
#include "zend.h"
#include "zend_API.h"
#include "zend_smart_str.h"
#include "zend_objects.h"

extern zend_class_entry *string_buffer_ce;

extern zend_object_handlers string_buffer_object_handlers;

typedef struct _string_buffer_t {
  smart_str *buffer;
  int endianness;
  zend_object std;
} string_buffer_t;

void string_buffer_free(string_buffer_t *intern TSRMLS_DC);
zend_object *string_buffer_create(zend_class_entry *ce TSRMLS_DC);
void write_bytes(string_buffer_t *, unsigned char *, size_t, long, int);
long normalize(string_buffer_t *, long, int);
void stringbuffer_write_long(string_buffer_t *, long, long, unsigned char, int);
void stringbuffer_write_double(string_buffer_t *, double, long, unsigned char, int);
long stringbuffer_read_long(string_buffer_t *, long, unsigned char, int);
double stringbuffer_read_double(string_buffer_t *, long, unsigned char, int);
string_buffer_t *get_string_buffer(zend_object *);

PHP_METHOD(StringBuffer, __construct);
PHP_METHOD(StringBuffer, readUInt8);
PHP_METHOD(StringBuffer, writeUInt8);
PHP_METHOD(StringBuffer, readUInt16LE);
PHP_METHOD(StringBuffer, readUInt16BE);
PHP_METHOD(StringBuffer, writeUInt16LE);
PHP_METHOD(StringBuffer, writeUInt16BE);
PHP_METHOD(StringBuffer, readUInt32LE);
PHP_METHOD(StringBuffer, readUInt32BE);
PHP_METHOD(StringBuffer, writeUInt32LE);
PHP_METHOD(StringBuffer, writeUInt32BE);
PHP_METHOD(StringBuffer, readFloatLE);
PHP_METHOD(StringBuffer, readFloatBE);
PHP_METHOD(StringBuffer, writeFloatLE);
PHP_METHOD(StringBuffer, writeFloatBE);
PHP_METHOD(StringBuffer, readDoubleLE);
PHP_METHOD(StringBuffer, readDoubleBE);
PHP_METHOD(StringBuffer, writeDoubleLE);
PHP_METHOD(StringBuffer, writeDoubleBE);
PHP_METHOD(StringBuffer, toString);
PHP_METHOD(StringBuffer, length);

#endif
