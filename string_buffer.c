#include "php.h"
#include "string_buffer.h"
#include "endianness.h"
#include "swizzle.h"
#include "zend_smart_str.h"
#include "types.h"
#include "conversions.h"

zend_class_entry *string_buffer_ce;
zend_object_handlers string_buffer_object_handlers;

static smart_str *smart_str_ctor() {
  smart_str *ret = (smart_str *) ecalloc(1, sizeof(smart_str));
  ret->a = 0;
  ret->s = zend_string_init("", 0, 0);
  return ret;
}

void string_buffer_free(string_buffer_t *intern TSRMLS_DC) {
  zend_object_std_dtor(&intern->std TSRMLS_CC);
  efree(intern->buffer);
  efree(intern);
}

string_buffer_t *get_string_buffer(zend_object *obj) {
  return (string_buffer_t *)((char *)(obj) - XtOffsetOf(string_buffer_t, std));
}

zend_object *string_buffer_create(zend_class_entry *ce TSRMLS_DC) {
  string_buffer_t *intern = (string_buffer_t *) emalloc(sizeof(string_buffer_t));
  memset(intern, 0, sizeof(string_buffer_t));
  smart_str *buf = smart_str_ctor();
  intern->buffer = buf;
  intern->endianness = is_little_endian();
  zend_object_std_init(&intern->std, ce TSRMLS_CC);
  object_properties_init(&intern->std, ce);
  intern->std.handlers = &string_buffer_object_handlers;
  return &intern->std;
}

void write_bytes(string_buffer_t *sb, unsigned char *bytes, size_t len, long offset, int endianness) {
  if (sb->endianness != endianness) swizzle(bytes, len); 
  for (size_t i = 0; i < len; ++i) {
    if (offset + i == sb->buffer->s->len) {
      smart_str_appendc(sb->buffer, bytes[i]);
    } else {
      sb->buffer->s->val[offset + i] = bytes[i];
    }
  }
}

void stringbuffer_write_long(string_buffer_t *sb, long v, long offset, unsigned char type, int endianness) {
  offset = normalize(sb, offset, 0);
  union char_to_unsigned c_to_u;
  union unsigned_short_to_bytes u_s_to_b;
  union short_to_bytes s_to_b;
  union unsigned_int_to_bytes u_i_to_b;
  union int_to_bytes i_to_b;
  unsigned char uc;
  char c;
  unsigned short us;
  short s;
  unsigned int ui;
  int i;
  switch (type) {
    case LEON_UNSIGNED_CHAR:
      uc = (unsigned char) v;
      write_bytes(sb, &uc, 1, offset, endianness);
      break;
    case LEON_CHAR:
      c = (char) v;
      write_bytes(sb, &c, 1, offset, endianness);
      break;
    case LEON_UNSIGNED_SHORT:
      us = (unsigned short) v;
      u_s_to_b.value = us;
      write_bytes(sb, u_s_to_b.bytes, 2, offset, endianness);
      break;
    case LEON_SHORT:
      s = (short) v;
      s_to_b.value = s;
      write_bytes(sb, s_to_b.bytes, 2, offset, endianness);
      break;
    case LEON_UNSIGNED_INT:
      ui = (unsigned int) v;
      u_i_to_b.value = ui;
      write_bytes(sb, u_i_to_b.bytes, 4, offset, endianness);
      break;
    case LEON_INT:
      i = (int) v;
      i_to_b.value = i;
      write_bytes(sb, i_to_b.bytes, 4, offset, endianness);
      break;
  }
}

void stringbuffer_write_double(string_buffer_t *sb, double d, long offset, unsigned char type, int endianness) {
  offset = normalize(sb, offset, 0);
  float f;
  union to_float f_to_b;
  union to_double d_to_b;
  switch (type) {
    case LEON_FLOAT:
      f = (float) d;
      f_to_b.val = f;
      write_bytes(sb, f_to_b.bytes, 4, offset, endianness);
      break;
    case LEON_DOUBLE:
      d_to_b.val = d;
      write_bytes(sb, d_to_b.bytes, 8, offset, endianness);
      break;
  }
}

double stringbuffer_read_double(string_buffer_t *sb, long offset, unsigned char type, int endianness) {
  offset = normalize(sb, offset, 1);
  union to_float t_f;
  union to_double t_d;
  switch (type) {
    case LEON_FLOAT:
      memcpy(t_f.bytes, sb->buffer->s->val + offset, 4);
      if (endianness != sb->endianness) swizzle(t_f.bytes, 4);
      return (double) t_f.val;
      break;
    case LEON_DOUBLE:
      memcpy(t_d.bytes, sb->buffer->s->val + offset, 8);
      if (endianness != sb->endianness) swizzle(t_d.bytes, 8);
      return t_d.val;
  }
}

long stringbuffer_read_long(string_buffer_t *sb, long offset, unsigned char type, int endianness) {
  offset = normalize(sb, offset, 1);
  union to_unsigned_short t_u_s;
  union to_short t_s;
  union to_unsigned_int t_u_i;
  union to_int t_i;
  switch (type) {
    case LEON_UNSIGNED_CHAR:
      return (long) ((unsigned char) sb->buffer->s->val[offset]);
      break;
    case LEON_CHAR:
      return (long) sb->buffer->s->val[offset];
      break;
    case LEON_UNSIGNED_SHORT:
      memcpy(t_u_s.bytes, sb->buffer->s->val + offset, 2);
      if (endianness != sb->endianness) swizzle(t_u_s.bytes, 2);
      return (long) t_u_s.val;
      break;
    case LEON_SHORT:
      memcpy(t_s.bytes, sb->buffer->s->val + offset, 2);
      if (endianness != sb->endianness) swizzle(t_s.bytes, 2);
      return (long) t_s.val;
      break;
    case LEON_UNSIGNED_INT:
      memcpy(t_u_i.bytes, sb->buffer->s->val + offset, 4);
      if (endianness != sb->endianness) swizzle(t_u_i.bytes, 4);
      return (long) t_u_i.val;
      break;
    case LEON_INT:
      memcpy(t_i.bytes, sb->buffer->s->val + offset, 4);
      if (endianness != sb->endianness) swizzle(t_i.bytes, 4);
      return (long) t_i.val;
      break;
  }
}
long normalize(string_buffer_t *sb, long i, int is_read) {
  if (i < 0 && !sb->buffer->s->len) return 0;
  if (i < 0) {
    i = sb->buffer->s->len + (is_read ? 0: 1) + i;
  }
  return i;
}
PHP_METHOD(StringBuffer, __construct) {}
PHP_METHOD(StringBuffer, readUInt8) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_UNSIGNED_CHAR, 1));
}
PHP_METHOD(StringBuffer, writeUInt8) {
  long v, offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &v, &offset) == FAILURE) return;
  stringbuffer_write_long(sb, v, offset, LEON_UNSIGNED_CHAR, 1);
}
PHP_METHOD(StringBuffer, readInt8) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_CHAR, 1));
}
PHP_METHOD(StringBuffer, writeInt8) {
  long v, offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &v, &offset) == FAILURE) return;
  stringbuffer_write_long(sb, v, offset, LEON_CHAR, 1);
}
PHP_METHOD(StringBuffer, readUInt16LE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_UNSIGNED_SHORT, 1));
}
PHP_METHOD(StringBuffer, readUInt16BE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_UNSIGNED_SHORT, 0));
}
PHP_METHOD(StringBuffer, writeUInt16LE) {
  long v, offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &v, &offset) == FAILURE) return;
  stringbuffer_write_long(sb, v, offset, LEON_UNSIGNED_SHORT, 1);
}
PHP_METHOD(StringBuffer, writeUInt16BE) {
  long v, offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &v, &offset) == FAILURE) return;
  stringbuffer_write_long(sb, v, offset, LEON_UNSIGNED_SHORT, 0);
}
PHP_METHOD(StringBuffer, readInt16LE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_SHORT, 1));
}
PHP_METHOD(StringBuffer, readInt16BE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_SHORT, 0));
}
PHP_METHOD(StringBuffer, readUInt32LE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_UNSIGNED_INT, 1));
}
PHP_METHOD(StringBuffer, readUInt32BE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_UNSIGNED_INT, 0));
}
PHP_METHOD(StringBuffer, writeUInt32LE) {
  long v, offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &v, &offset) == FAILURE) return;
  stringbuffer_write_long(sb, v, offset, LEON_UNSIGNED_INT, 1);
}
PHP_METHOD(StringBuffer, writeUInt32BE) {
  long v, offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &v, &offset) == FAILURE) return;
  stringbuffer_write_long(sb, v, offset, LEON_UNSIGNED_INT, 0);
}
PHP_METHOD(StringBuffer, readInt32LE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_INT, 1));
}
PHP_METHOD(StringBuffer, readInt32BE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_LONG(return_value, stringbuffer_read_long(sb, offset, LEON_INT, 0));
}
PHP_METHOD(StringBuffer, writeInt32LE) {
  long v, offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &v, &offset) == FAILURE) return;
  stringbuffer_write_long(sb, v, offset, LEON_INT, 1);
}
PHP_METHOD(StringBuffer, writeInt32BE) {
  long v, offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "ll", &v, &offset) == FAILURE) return;
  stringbuffer_write_long(sb, v, offset, LEON_INT, 0);
}
PHP_METHOD(StringBuffer, readFloatLE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_DOUBLE(return_value, stringbuffer_read_double(sb, offset, LEON_FLOAT, 1));
}
PHP_METHOD(StringBuffer, readFloatBE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_DOUBLE(return_value, stringbuffer_read_double(sb, offset, LEON_FLOAT, 0));
}
PHP_METHOD(StringBuffer, writeFloatLE) {
  long offset;
  double d;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "fl", &d, &offset) == FAILURE) return;
  stringbuffer_write_double(sb, d, offset, LEON_FLOAT, 1);
}
PHP_METHOD(StringBuffer, writeFloatBE) {
  long offset;
  double d;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "fl", &d, &offset) == FAILURE) return;
  stringbuffer_write_double(sb, d, offset, LEON_FLOAT, 0);
}
PHP_METHOD(StringBuffer, readDoubleLE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_DOUBLE(return_value, stringbuffer_read_double(sb, offset, LEON_DOUBLE, 1));
}
PHP_METHOD(StringBuffer, readDoubleBE) {
  long offset;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &offset) == FAILURE) { return; }
  ZVAL_DOUBLE(return_value, stringbuffer_read_double(sb, offset, LEON_DOUBLE, 0));
}
PHP_METHOD(StringBuffer, writeDoubleLE) {
  long offset;
  double d;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "dl", &d, &offset) == FAILURE) return;
  stringbuffer_write_double(sb, d, offset, LEON_DOUBLE, 1);
}
PHP_METHOD(StringBuffer, writeDoubleBE) {
  long offset;
  double d;
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "dl", &d, &offset) == FAILURE) return;
  stringbuffer_write_double(sb, d, offset, LEON_DOUBLE, 0);
}
PHP_METHOD(StringBuffer, toString) {
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  ZVAL_STRINGL(return_value, sb->buffer->s->val, sb->buffer->s->len);
}
PHP_METHOD(StringBuffer, length) {
  zval *this = getThis();
  string_buffer_t *sb = get_string_buffer(Z_OBJ_P(this) TSRMLS_CC);
  ZVAL_LONG(return_value, sb->buffer->s->len);
}
