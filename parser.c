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
      for (unsigned int j = 0; j < 4; ++j) {
        u_i.bytes[j] = (unsigned char) p->payload[p->i - (4 - j)];
      }
      if (!p->endianness) swizzle(u_i.bytes, 4);
      return (long) u_i.val;
      break;
    case LEON_INT:
      p->i += 4;
      for (unsigned int j = 0; j < 4; ++j) {
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
  switch (t) {
    case LEON_FLOAT:
      p->i += 4;
      for (unsigned int i = 0; i < 4; ++i) {
        f.bytes[i] = (unsigned char) p->payload[p->i - (4 - i)];
      }
      if (!p->endianness) swizzle(f.bytes, 4);
      return (double) f.val;
      break;
    case LEON_DOUBLE:
      p->i += 8;
      for (unsigned int i = 0; i < 8; ++i) {
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
zval read_string_as_zval(leon_parser_t *p) {
  long len = read_varint(p, read_uint8(p));
  char *buf = (char *) emalloc(sizeof(char)*len);
  memcpy(buf, p->payload + p->i, len);
  zval val;
  ZVAL_STRINGL(&val, buf, len);
  p->i += len;
  efree(buf);
  return val;
} 
void parse_string_index(leon_parser_t *p) {
  p->string_index_type = read_uint8(p);
  if (p->string_index_type == 0xFF) return;
  long string_count = read_varint(p, p->string_index_type);
  for (long i = 0; i < string_count; ++i) {
    string_index_push(p->string_index, read_string(p));
  }
}
void parse_object_layout_index(leon_parser_t *p) {
  if (p->string_index_type == 0xFF) return;
  p->object_layout_type = read_uint8(p);
  if (p->object_layout_type == 0xFF) return;
  long layout_count = read_varint(p, p->object_layout_type);
  for (long i = 0; i < layout_count; ++i) {
    oli_entry *entry = oli_entry_ctor();
    long prop_count = read_varint(p, read_uint8(p));
    for (long j = 0; j < prop_count; ++j) {
      oli_entry_push(entry, read_varint(p, p->string_index_type));;
    }
    object_layout_index_push(p->object_layout_index, entry);
  }
}
zval parse_value_with_spec(leon_parser_t *p, zval *spec) {
  unsigned char type = type_check(spec);
  zval ret;
  int i;
  long len;
  HashTable *ht;
  hash_entry *he;
  zend_string *key;
  hash_array_t *ha;
  zval *data;
  switch (type) {
    case LEON_CONSTANT:
      return parse_value_with_spec(p, &((zend_constant *) Z_PTR_P(spec))->value);
      break;
    case LEON_UNSIGNED_CHAR:
      switch (Z_LVAL_P(spec)) {
        case LEON_BOOLEAN:
        case LEON_UNDEFINED:
        case LEON_NULL:
        case LEON_NAN:
        case LEON_INFINITY:
        case LEON_MINUS_INFINITY:
          return parse_value(p, read_uint8(p));
          break;
        default:
          return parse_value(p, (unsigned char) Z_LVAL_P(spec));
          break;
      }
      break;
    case LEON_ARRAY:
      ht = Z_ARRVAL_P(spec);
      spec = zend_hash_index_find(ht, 0);
      array_init(&ret);
      type = read_uint8(p);
      len = read_varint(p, type);
      for (long i = 0; i < len; ++i) {
        zval element = parse_value_with_spec(p, spec);
        add_index_zval(&ret, (unsigned int) i, &element);
      }
      break;
    case LEON_OBJECT:
      array_init(&ret);
      ht = Z_ARRVAL_P(spec);
      ha = hash_array_ctor();
      ZEND_HASH_FOREACH_STR_KEY_VAL_IND(ht, key, data) {
        he = (hash_entry *) emalloc(sizeof(hash_entry));
        he->key = key;
        he->value = data;
        hash_array_push(ha, he);
      } ZEND_HASH_FOREACH_END();
      hash_array_sort(ha);
      for (long i = 0; i < ha->len; ++i) {
        zval element = parse_value_with_spec(p, ha->index[i]->value);
        add_assoc_zval(&ret, ha->index[i]->key->val, &element);
      }
      hash_array_dtor(ha);
      break;
  }
  return ret;
}
zval parse_value(leon_parser_t *p, unsigned char type) {
  zval ret;
  zend_string *buf;
  string_buffer_t *sb;
  zval *data;
  long l;
  double d;
  switch (type) {
    case LEON_UNSIGNED_CHAR:
    case LEON_CHAR:
    case LEON_UNSIGNED_SHORT:
    case LEON_SHORT:
    case LEON_UNSIGNED_INT:
    case LEON_INT:
      l = read_varint(p, type);
      ZVAL_LONG(&ret, l);
      break;
    case LEON_FLOAT:
    case LEON_DOUBLE:
      d = read_double(p, type);
      ZVAL_DOUBLE(&ret, d);
      break;
    case LEON_STRING:
      if (p->string_index_type == LEON_EMPTY) {
        zend_string *val = read_string(p);
        ZVAL_NEW_STR(&ret, val);
      } else {
        zend_string *val = p->string_index->index[read_varint(p, p->string_index_type)];
        ZVAL_NEW_STR(&ret, val);
      }
      break;
    case LEON_FALSE:
      ZVAL_FALSE(&ret);
      break;
    case LEON_TRUE:
      ZVAL_TRUE(&ret);
      break;
    case LEON_NULL:
      ZVAL_NULL(&ret);
      break;
    case LEON_INFINITY:
      object_init_ex(&ret, infinity_ce);
      break;
    case LEON_MINUS_INFINITY:
      object_init_ex(&ret, minus_infinity_ce);
      break;
    case LEON_UNDEFINED:
      object_init_ex(&ret, undefined_ce);
      break;
    case LEON_NAN:
      object_init_ex(&ret, nan_ce);
      break;
    case LEON_ARRAY:
      array_init(&ret);
      long len = read_varint(p, read_uint8(p));
      for (long i = 0; i < len; ++i) {
        zval element = parse_value(p, read_uint8(p));
        add_index_zval(&ret, (unsigned int) i, &element);
      }
      break;
    case LEON_OBJECT:
      array_init(&ret);
      long layout_idx = read_varint(p, p->object_layout_type);
      oli_entry *entry = p->object_layout_index->index[layout_idx];
      for (long i = 0; i < entry->len; ++i) {
        zval element = parse_value(p, read_uint8(p));
        add_assoc_zval(&ret, p->string_index->index[entry->entries[i]]->val, &element);
      }
      break;
    case LEON_DATE:
      object_init_ex(&ret, date_ce);
      d = read_double(p, LEON_DOUBLE);
      add_property_long_ex(&ret, "timestamp", sizeof("timestamp") - 1, (long) d);
      break;
    case LEON_REGEXP:
      object_init_ex(&ret, regexp_ce);
      zval str = read_string_as_zval(p);
      add_property_zval_ex(&ret, "pattern", sizeof("pattern") - 1, &str);
      str = read_string_as_zval(p);
      add_property_zval_ex(&ret, "modifier", sizeof("modifier") - 1, &str);
      break;
    case LEON_BUFFER:
      object_init_ex(&ret, string_buffer_ce);
      buf = read_string(p);
      sb = get_string_buffer(Z_OBJ(ret));
      sb->buffer->s = buf;
      sb->buffer->a = buf->len;
      break;
  }
  return ret;
}
