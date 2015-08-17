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
#if PHP_API_VERSION <= 20131106
void *read_string(leon_parser_t *p, char **str, size_t *length, int terminate) {
  long len = read_varint(p, read_uint8(p));
  char *buf;
  if (terminate) {
    buf = (char *) emalloc(sizeof(char)*len + 1);
    memset(buf, 0, sizeof(char)*len + 1);
  } else {
    buf = (char *) emalloc(sizeof(char)*len);
    memset(buf, 0, sizeof(char)*len);
  }
  memcpy(buf, p->payload + p->i, sizeof(char)*len);
  *str = buf;
  *length = (size_t) len;
  p->i += len;
}
#else
zend_string *read_string(leon_parser_t *p) {
  long len = read_varint(p, read_uint8(p));
  char *buf = (char *) emalloc(sizeof(char)*len);
  memcpy(buf, p->payload + p->i, sizeof(char)*len);
  zend_string *val = zend_string_init(buf, len, 0);
  p->i += len;
  efree(buf);
  return val;
}
#endif
void read_string_as_zval(leon_parser_t *p, zval *val) {
  long len = read_varint(p, read_uint8(p));
#if PHP_API_VERSION <= 20131106
  ZVAL_STRINGL(val, p->payload + p->i, len, 1);
#else
  ZVAL_STRINGL(val, p->payload + p->i, len);
#endif
  p->i += len;
} 
void parse_string_index(leon_parser_t *p) {
  p->string_index_type = read_uint8(p);
  if (p->string_index_type == 0xFF) {
    p->state |= 0x1;
    return;
  }
  long string_count = read_varint(p, p->string_index_type);
  long i;
  for (i = 0; i < string_count; ++i) {
#if PHP_API_VERSION <= 20131106
    zval str;
    read_string_as_zval(p, &str);
    string_index_push(p->string_index, &str);
#else
    string_index_push(p->string_index, read_string(p));
#endif
  }
  p->state |= 0x1;
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
  hash_array_t *ha;
#if PHP_API_VERSION <= 20131106
  HashPosition pos;
  int j;
  uint key_len;
  char *key;
  zval **data;
  ulong index;
  zval **zv_dest, *tmp;
#else
  zend_string *key;
  zval *data;
#endif
  switch (type) {
#if PHP_API_VERSION > 20131106
    case LEON_CONSTANT:
      parse_value_with_spec(p, &((zend_constant *) Z_PTR_P(spec))->value, output);
      return;
      break;
#endif
    case LEON_UNSIGNED_CHAR:
      switch (Z_LVAL_P(spec)) {
        case LEON_BOOLEAN:
        case LEON_UNDEFINED:
        case LEON_NULL:
        case LEON_NAN:
        case LEON_INFINITY:
        case LEON_MINUS_INFINITY:
        case LEON_DYNAMIC:
          parse_value(p, read_uint8(p), output);
          break;
        default:
          return parse_value(p, (unsigned char) Z_LVAL_P(spec), output);
          break;
      }
      break;
    case LEON_ARRAY:
      ht = Z_ARRVAL_P(spec);
#if PHP_API_VERSION <= 20131106
      zend_hash_index_find(ht, 0, (void **) &data);
      spec = *data;
#else
      spec = zend_hash_index_find(ht, 0);
#endif
      array_init(output);
      type = read_uint8(p);
      len = read_varint(p, type);
      for (i = 0; i < len; ++i) {
#if PHP_API_VERSION <= 20131106
        zval *element;
        MAKE_STD_ZVAL(element);
        parse_value_with_spec(p, spec, element);
        zend_hash_index_update(Z_ARRVAL_P(output), (unsigned int) i, &element, sizeof(zval *), (void **) &zv_dest);
#else
        zval element;
        parse_value_with_spec(p, spec, &element);
        add_index_zval(output, (unsigned int) i, &element);
#endif
      }
      break;
    case LEON_OBJECT:
      array_init(output);
      ht = Z_ARRVAL_P(spec);
      ha = hash_array_ctor();
#if PHP_API_VERSION <= 20131106
      zend_hash_internal_pointer_reset_ex(ht, &pos);
      for (;; zend_hash_move_forward_ex(ht, &pos)) {
        j = zend_hash_get_current_key_ex(ht, &key, &key_len, &index, 0, &pos);
        if (j == HASH_KEY_NON_EXISTENT) break;
        if (zend_hash_get_current_data_ex(ht, (void **) &data, &pos) == SUCCESS) {
          he = (hash_entry *) emalloc(sizeof(hash_entry));
          he->key = key;
          he->len = key_len;
          he->hash = index;
          he->value = *data;
          hash_array_push(ha, he);
        }
      }
      hash_array_sort(ha);
      for (i = 0; i < ha->len; ++i) {
        zval *element;
        MAKE_STD_ZVAL(element);
        parse_value_with_spec(p, ha->index[i]->value, element);
        zend_hash_update(Z_ARRVAL_P(output), ha->index[i]->key, ha->index[i]->len, &element, sizeof(zval *), NULL);
      }
      hash_array_dtor(ha);
#else
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
#endif
      break;
  }
}
void parse_value(leon_parser_t *p, unsigned char type, zval *output) {
#if PHP_API_VERSION > 20131106
  zend_string *buf;
#endif
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
      if (!(p->state & 0x1) || p->string_index_type == LEON_EMPTY) {
        read_string_as_zval(p, output);
      } else {
#if PHP_API_VERSION <= 20131106
        *output = *p->string_index->index[read_varint(p, p->string_index_type)];
#else
        zend_string *val = p->string_index->index[read_varint(p, p->string_index_type)];
        ZVAL_NEW_STR(output, val);
#endif
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
#if PHP_API_VERSION <= 20131106
        zval *element;
        MAKE_STD_ZVAL(element);
        parse_value(p, read_uint8(p), element);
        zend_hash_index_update(Z_ARRVAL_P(output), (unsigned int) i, &element, sizeof(zval *), NULL);
#else
        zval element;
        parse_value(p, read_uint8(p), &element);
        add_index_zval(output, (unsigned int) i, &element);
#endif
      }
      break;
    case LEON_OBJECT:
      array_init(output);
#if PHP_API_VERSION <= 20131106
      if (p->state & 0x1) {
        long layout_idx = read_varint(p, p->object_layout_type);
        oli_entry *entry = p->object_layout_index->index[layout_idx];
        for (i = 0; i < entry->len; ++i) {
          zval *element;
          MAKE_STD_ZVAL(element);
          parse_value(p, read_uint8(p), element);
          add_assoc_zval_ex(output, p->string_index->index[entry->entries[i]]->value.str.val, p->string_index->index[entry->entries[i]]->value.str.len, element);
        }
      } else {
        long keys = read_varint(p, read_uint8(p));
        for (i = 0; i < keys; ++i) {
          char *key_name;
          size_t key_len;
          read_string(p, &key_name, &key_len, 1);
          zval *element;
          MAKE_STD_ZVAL(element);
          parse_value(p, read_uint8(p), element);
          add_assoc_zval_ex(output, key_name, key_len + 1, element);
          efree(key_name);
        }
      } 
#else
      if (p->state & 0x1) {
        long layout_idx = read_varint(p, p->object_layout_type);
        oli_entry *entry = p->object_layout_index->index[layout_idx];
        for (i = 0; i < entry->len; ++i) {
          zval element;
          parse_value(p, read_uint8(p), &element);
          add_assoc_zval(output, p->string_index->index[entry->entries[i]]->val, &element);
        }
      } else {
        long keys = read_varint(p, read_uint8(p));
        for (i = 0; i < keys; ++i) {
          zend_string *key = read_string(p);
          zval element;
          parse_value(p, read_uint8(p), &element);
          add_assoc_zval_ex(output, ZSTR_VAL(key), ZSTR_LEN(key), &element);
          zend_string_release(key);
        }
      } 
#endif
      break;
    case LEON_DATE:
      object_init_ex(output, date_ce);
      d = read_double(p, LEON_DOUBLE);
      add_property_long(output, "timestamp", (long) d);
      break;
    case LEON_REGEXP:
      object_init_ex(output, regexp_ce);
#if PHP_API_VERSION <= 20131106
      zval *pattern;
      MAKE_STD_ZVAL(pattern);
      read_string_as_zval(p, pattern);
      add_property_zval(output, "pattern", pattern);
      zval *modifier;
      MAKE_STD_ZVAL(modifier);
      read_string_as_zval(p, modifier);
      add_property_zval(output, "modifier", modifier);
#else
      zval str;
      read_string_as_zval(p, &str);
      add_property_zval_ex(output, "pattern", sizeof("pattern") - 1, &str);
      read_string_as_zval(p, &str);
      add_property_zval_ex(output, "modifier", sizeof("modifier") - 1, &str);
#endif
      break;
    case LEON_BUFFER:
      object_init_ex(output, string_buffer_ce);
#if PHP_API_VERSION <= 20131106
      sb = get_string_buffer(zend_objects_get_address(output));
      char *bufstr;
      size_t buflen;
      read_string(p, &bufstr, &buflen, 0);
      sb->buffer->c = bufstr;
      sb->buffer->a = buflen;
      sb->buffer->len = buflen;
#else
      buf = read_string(p);
      sb = get_string_buffer(Z_OBJ_P(output));
      sb->buffer->s = buf;
      sb->buffer->a = buf->len;
#endif
      break;
  }
}
