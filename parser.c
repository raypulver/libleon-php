#include <zend.h>
#include <zend_API.h>
#include "endianness.h"
#include "string_index.h"
#include "object_layout_index.h"
#include "swizzle.h"
#include "parser.h"
#include "types.h"
#include "hash_array.h"
#include "type_check.h"
#include "conversions.h"
#include "classes.h"
#include "string_buffer.h"

leon_parser_t *parser_ctor(char *payload, size_t len) {
  leon_parser_t *ret = emalloc(sizeof(leon_parser_t));
  ret->endianness = is_little_endian();
  ret->string_index = string_index_ctor();
  ret->object_layout_index = object_layout_index_ctor();
  ret->state = 0;
  ret->payload = payload;
  ret->payload_len = len;
  ret->i = 0;
  return ret;
}

void parser_dtor(leon_parser_t *p) {
  string_index_dtor(p->string_index);
  object_layout_index_dtor(p->object_layout_index);
  efree(p);
}

unsigned char read_uint8(leon_parser_t *p) {
  p->i++;
  return (unsigned char) p->payload[p->i - 1];
}

int type_to_bytes(unsigned char type) {
  switch (type) {
    case LEON_UNSIGNED_CHAR:
    case LEON_CHAR:
      return 1;
      break;
    case LEON_UNSIGNED_SHORT:
    case LEON_SHORT:
      return 2;
      break;
    case LEON_UNSIGNED_INT:
    case LEON_INT:
    case LEON_FLOAT:
      return 4;
      break;
    case LEON_DOUBLE:
      return 8;
      break;
  }
}

long read_varint(leon_parser_t *p, unsigned char type) {
  union to_unsigned_short u_s;
  union to_short s;
  union to_unsigned_int u_i;
  union to_int i;
  unsigned int j;
  switch (type) {
    case LEON_UNSIGNED_CHAR:
      return (long) read_uint8(p);
      break;
    case LEON_CHAR:
      p->i++;
      return (long) p->payload[p->i - 1];
      break;
    case LEON_UNSIGNED_SHORT:
      p->i += 2;
      u_s.bytes[0] = (unsigned char) p->payload[p->i - 2];
      u_s.bytes[1] = (unsigned char) p->payload[p->i - 1];
      if (!p->endianness) swizzle(u_s.bytes, 2);
      return (long) u_s.val;
      break;
    case LEON_SHORT:
      p->i += 2;
      s.bytes[0] = (unsigned char) p->payload[p->i - 2];
      s.bytes[1] = (unsigned char) p->payload[p->i - 1];
      if (!p->endianness) swizzle(s.bytes, 2);
      return (long) s.val;
      break;
    case LEON_UNSIGNED_INT: 
      p->i += 4;
      for (j = 0; j < 4; ++j) {
        u_i.bytes[j] = (unsigned char) p->payload[p->i - (4 - j)];
      }
      if (!p->endianness) swizzle(u_i.bytes, 4);
      return (long) u_i.val;
      break;
    case LEON_INT:
      p->i += 4;
      for (j = 0; j < 4; ++j) {
        i.bytes[j] = (unsigned char) p->payload[p->i - (4 - j)];
      }
      if (!p->endianness) swizzle(i.bytes, 4);
      return (long) i.val;
      break;
  }
}
double read_double(leon_parser_t *p, unsigned char t) {
  union to_float f;
  union to_double td;
  unsigned int i;
  switch (t) {
    case LEON_FLOAT:
      p->i += 4;
      for (i = 0; i < 4; ++i) {
        f.bytes[i] = (unsigned char) p->payload[p->i - (4 - i)];
      }
      if (!p->endianness) swizzle(f.bytes, 4);
      return (double) f.val;
      break;
    case LEON_DOUBLE:
      p->i += 8;
      for (i = 0; i < 8; ++i) {
        td.bytes[i] = (unsigned char) p->payload[p->i - (8 - i)];
      }
      if (!p->endianness) swizzle(td.bytes, 8);
      return td.val;
      break;
  }
}
zend_string *read_string(leon_parser_t *p) {
  long len = read_varint(p, read_uint8(p));
  char *buf = (char *) emalloc(sizeof(char)*len);
  memcpy(buf, p->payload + p->i, sizeof(char)*len);
  zend_string *val = zend_string_init(buf, len, 0);
  p->i += len;
  efree(buf);
  return val;
}
void read_string_as_zval(leon_parser_t *p, zval *val) {
  long len = read_varint(p, read_uint8(p));
  char *buf = (char *) emalloc(sizeof(char)*len);
  memcpy(buf, p->payload + p->i, len);
  ZVAL_STRINGL(val, buf, len);
  p->i += len;
  efree(buf);
} 
void parse_string_index(leon_parser_t *p) {
  p->string_index_type = read_uint8(p);
  if (p->string_index_type == 0xFF) return;
  long string_count = read_varint(p, p->string_index_type);
  long i;
  for (i = 0; i < string_count; ++i) {
    string_index_push(p->string_index, read_string(p));
  }
}
void parse_object_layout_index(leon_parser_t *p) {
  if (p->string_index_type == 0xFF) return;
  p->object_layout_type = read_uint8(p);
  if (p->object_layout_type == 0xFF) return;
  long layout_count = read_varint(p, p->object_layout_type), i, j;
  for (i = 0; i < layout_count; ++i) {
    oli_entry *entry = oli_entry_ctor();
    long prop_count = read_varint(p, read_uint8(p));
    for (j = 0; j < prop_count; ++j) {
      oli_entry_push(entry, read_varint(p, p->string_index_type));;
    }
    object_layout_index_push(p->object_layout_index, entry);
  }
}
void parse_value_with_spec(leon_parser_t *p, zval *spec, zval *output) {
  unsigned char type = type_check(spec);
  long i;
  long len;
  HashTable *ht;
  hash_entry *he;
  zend_string *key;
  hash_array_t *ha;
  zval *data;
  switch (type) {
    case LEON_CONSTANT:
      return parse_value_with_spec(p, &((zend_constant *) Z_PTR_P(spec))->value, output);
      break;
    case LEON_UNSIGNED_CHAR:
      switch (Z_LVAL_P(spec)) {
        case LEON_BOOLEAN:
        case LEON_UNDEFINED:
        case LEON_NULL:
        case LEON_NAN:
        case LEON_INFINITY:
        case LEON_MINUS_INFINITY:
          parse_value(p, read_uint8(p), output);
          break;
        default:
          return parse_value(p, (unsigned char) Z_LVAL_P(spec), output);
          break;
      }
      break;
    case LEON_ARRAY:
      ht = Z_ARRVAL_P(spec);
      spec = zend_hash_index_find(ht, 0);
      array_init(output);
      type = read_uint8(p);
      len = read_varint(p, type);
      for (i = 0; i < len; ++i) {
        zval element;
        parse_value_with_spec(p, spec, &element);
        add_index_zval(output, (unsigned int) i, &element);
      }
      break;
    case LEON_OBJECT:
      array_init(output);
      ht = Z_ARRVAL_P(spec);
      ha = hash_array_ctor();
      ZEND_HASH_FOREACH_STR_KEY_VAL_IND(ht, key, data) {
        he = (hash_entry *) emalloc(sizeof(hash_entry));
        he->key = key;
        he->value = data;
        hash_array_push(ha, he);
      } ZEND_HASH_FOREACH_END();
      hash_array_sort(ha);
      for (i = 0; i < ha->len; ++i) {
        zval element;
        parse_value_with_spec(p, ha->index[i]->value, &element);
        zend_hash_update(Z_ARRVAL_P(output), ha->index[i]->key, &element);
      }
      hash_array_dtor(ha);
      break;
  }
}
void parse_value(leon_parser_t *p, unsigned char type, zval *output) {
  zend_string *buf;
  string_buffer_t *sb;
  zval *data;
  long l, i, len;
  double d;
  switch (type) {
    case LEON_UNSIGNED_CHAR:
    case LEON_CHAR:
    case LEON_UNSIGNED_SHORT:
    case LEON_SHORT:
    case LEON_UNSIGNED_INT:
    case LEON_INT:
      l = read_varint(p, type);
      ZVAL_LONG(output, l);
      break;
    case LEON_FLOAT:
    case LEON_DOUBLE:
      d = read_double(p, type);
      ZVAL_DOUBLE(output, d);
      break;
    case LEON_STRING:
      if (p->string_index_type == LEON_EMPTY) {
        read_string_as_zval(p, output);
      } else {
        zend_string *val = p->string_index->index[read_varint(p, p->string_index_type)];
        ZVAL_NEW_STR(output, val);
      }
      break;
    case LEON_FALSE:
      ZVAL_FALSE(output);
      break;
    case LEON_TRUE:
      ZVAL_TRUE(output);
      break;
    case LEON_NULL:
      ZVAL_NULL(output);
      break;
    case LEON_INFINITY:
      object_init_ex(output, infinity_ce);
      break;
    case LEON_MINUS_INFINITY:
      object_init_ex(output, minus_infinity_ce);
      break;
    case LEON_UNDEFINED:
      object_init_ex(output, undefined_ce);
      break;
    case LEON_NAN:
      object_init_ex(output, nan_ce);
      break;
    case LEON_ARRAY:
      array_init(output);
      len = read_varint(p, read_uint8(p));
      for (i = 0; i < len; ++i) {
        zval element;
        parse_value(p, read_uint8(p), &element);
        add_index_zval(output, (unsigned int) i, &element);
      }
      break;
    case LEON_OBJECT:
      array_init(output);
      long layout_idx = read_varint(p, p->object_layout_type);
      oli_entry *entry = p->object_layout_index->index[layout_idx];
      for (i = 0; i < entry->len; ++i) {
        zval element;
        parse_value(p, read_uint8(p), &element);
        add_assoc_zval(output, p->string_index->index[entry->entries[i]]->val, &element);
      }
      break;
    case LEON_DATE:
      object_init_ex(output, date_ce);
      d = read_double(p, LEON_DOUBLE);
      add_property_long_ex(output, "timestamp", sizeof("timestamp") - 1, (long) d);
      break;
    case LEON_REGEXP:
      object_init_ex(output, regexp_ce);
      zval str;
      read_string_as_zval(p, &str);
      add_property_zval_ex(output, "pattern", sizeof("pattern") - 1, &str);
      read_string_as_zval(p, &str);
      add_property_zval_ex(output, "modifier", sizeof("modifier") - 1, &str);
      break;
    case LEON_BUFFER:
      object_init_ex(output, string_buffer_ce);
      buf = read_string(p);
      sb = get_string_buffer(Z_OBJ_P(output));
      sb->buffer->s = buf;
      sb->buffer->a = buf->len;
      break;
  }
}
