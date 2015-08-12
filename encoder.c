#include <zend.h>
#include "zend_smart_str.h"
#include "encoder.h"
#include "string_index.h"
#include "object_layout_index.h"
#include "type_check.h"
#include "endianness.h"
#include "swizzle.h"
#include "types.h"
#include "hash_array.h"
#include "conversions.h"
#include "classes.h"
#include "string_buffer.h"

leon_encoder_t *encoder_ctor() {
  leon_encoder_t *ret = (leon_encoder_t *) emalloc(sizeof(leon_encoder_t));
  ret->endianness = is_little_endian();
  ret->string_index = string_index_ctor();
  ret->object_layout_index = object_layout_index_ctor();
  ret->state = 0;
  return ret;
}

void encoder_dtor(leon_encoder_t *p) {
  string_index_dtor(p->string_index);
  object_layout_index_dtor(p->object_layout_index);
  efree(p);
}

void write_string_index(leon_encoder_t *encoder, zval *payload, int depth) {
  zend_string *key;
  zval *data;
  zend_ulong index;
  HashTable *ht;
  zend_object *zobj;
  zend_string *prop;
  switch (type_check(payload)) {
    case LEON_STRING:
      if (string_index_find(encoder->string_index, Z_STR_P(payload)) == -1) {
        string_index_push(encoder->string_index, Z_STR_P(payload));
      }
      break;
    case LEON_ARRAY:
      ht = Z_ARRVAL_P(payload);
      ZEND_HASH_FOREACH_KEY_VAL_IND(ht, index, key, data) {
        write_string_index(encoder, data, depth + 1);
      } ZEND_HASH_FOREACH_END();
      break;
    case LEON_OBJECT:
      ht = Z_ARRVAL_P(payload);
      ZEND_HASH_FOREACH_KEY_VAL_IND(ht, index, key, data) {
        if (string_index_find(encoder->string_index, key) == -1) {
          string_index_push(encoder->string_index, key);
        }
      } ZEND_HASH_FOREACH_END();
      ZEND_HASH_FOREACH_KEY_VAL_IND(ht, index, key, data) {
        write_string_index(encoder, data, depth + 1);
      } ZEND_HASH_FOREACH_END();
      break;
    case LEON_NATIVE_OBJECT:
      ht = Z_OBJ_HT_P(payload)->get_properties(payload);
      zobj = Z_OBJ_P(payload);
      ZEND_HASH_FOREACH_STR_KEY_VAL_IND(ht, key, data) {
        if (key) {
          if (zend_check_property_access(zobj, key) == SUCCESS) {
            if (ZSTR_VAL(key)[0] == 0) {
              const char *prop_name, *class_name;
              size_t prop_len;
              zend_unmangle_property_name_ex(key, &class_name, &prop_name, &prop_len);
              prop = zend_string_init(prop_name, prop_len, 0); 
              string_index_set_push(encoder->string_index, prop);
            } else {
              string_index_set_push(encoder->string_index, key);
            }
          }
        }
      } ZEND_HASH_FOREACH_END();
      ZEND_HASH_FOREACH_STR_KEY_VAL_IND(ht, key, data) {
        if (key) {
          if (zend_check_property_access(zobj, key) == SUCCESS) {
            write_string_index(encoder, data, depth + 1);
          }
        }
      } ZEND_HASH_FOREACH_END();
      break;
    case LEON_INDIRECT:
      write_string_index(encoder, Z_INDIRECT_P(payload), depth + 1);
      break;
  }
  if (!depth) {
    if (!encoder->string_index->len) {
      smart_str_appendc(encoder->buffer, LEON_EMPTY);
      return;
    }
    encoder->string_index_type = integer_type_check((long) encoder->string_index->len);
    smart_str_appendc(encoder->buffer, encoder->string_index_type);
    write_long(encoder, (long) encoder->string_index->len, encoder->string_index_type);
    for (unsigned int i = 0; i < encoder->string_index->len; ++i) {
      write_zstr(encoder, encoder->string_index->index[i]);
    }
  }
}


void write_object_layout_index(leon_encoder_t *encoder, zval *payload, int depth) {
  if (!encoder->string_index->len) return;
  unsigned char type = type_check(payload);
  zend_string *key;
  zval *data;
  zend_ulong index;
  HashTable *ht;
  oli_entry *entry;
  zend_object *zobj;
  zend_string *prop;
  switch (type) {
    case LEON_INDIRECT:
      write_object_layout_index(encoder, Z_INDIRECT_P(payload), depth + 1);
      break;
    case LEON_NATIVE_OBJECT:
      entry = oli_entry_ctor();
      ht = Z_OBJ_HT_P(payload)->get_properties(payload);
      if (ht == NULL) {
        oli_entry_dtor(entry);
        return;
      }
      zobj = Z_OBJ_P(payload);
      ZEND_HASH_FOREACH_STR_KEY_VAL_IND(ht, key, data) {
        if (key) {
          if (zend_check_property_access(zobj, key) == SUCCESS) {
            if (ZSTR_VAL(key)[0] == 0) {
              const char *prop_name, *class_name;
              size_t prop_len;
              zend_unmangle_property_name_ex(key, &class_name, &prop_name, &prop_len);
              prop = zend_string_init(prop_name, prop_len, 0); 
              oli_entry_push(entry, string_index_find(encoder->string_index, prop));
            } else {
              oli_entry_push(entry, string_index_find(encoder->string_index, key));
            }
          }
        }
      } ZEND_HASH_FOREACH_END();
      oli_entry_sort(entry);
      if (object_layout_index_find(encoder->object_layout_index, entry) == -1) {
        object_layout_index_push(encoder->object_layout_index, entry);
      } else {
        oli_entry_dtor(entry);
      }  
      ZEND_HASH_FOREACH_STR_KEY_VAL_IND(ht, key, data) {
        if (key) {
          if (zend_check_property_access(zobj, key) == SUCCESS) {
            write_object_layout_index(encoder, data, depth + 1);
          }
        }
      } ZEND_HASH_FOREACH_END();
      break;
    case LEON_OBJECT:
      entry = oli_entry_ctor();
      ht = Z_ARRVAL_P(payload);
      ZEND_HASH_FOREACH_KEY_VAL_IND(ht, index, key, data) {
        oli_entry_push(entry, string_index_find(encoder->string_index, key)); 
      } ZEND_HASH_FOREACH_END();
      oli_entry_sort(entry);
      if (object_layout_index_find(encoder->object_layout_index, entry) == -1) {
        object_layout_index_push(encoder->object_layout_index, entry);
      } else {
        oli_entry_dtor(entry);
      }
      ZEND_HASH_FOREACH_KEY_VAL_IND(ht, index, key, data) {
        write_object_layout_index(encoder, data, depth + 1);
      } ZEND_HASH_FOREACH_END();
      break;
    case LEON_ARRAY:
      ht = Z_ARRVAL_P(payload);
      ZEND_HASH_FOREACH_KEY_VAL_IND(ht, index, key, data) {
        write_object_layout_index(encoder, data, depth + 1);
      } ZEND_HASH_FOREACH_END();
      break;
    default:
      break;
  }
  if (!depth) {
    if (!encoder->object_layout_index->len) {
      smart_str_appendc(encoder->buffer, LEON_EMPTY);
      return;
    }
    unsigned char type = integer_type_check(encoder->object_layout_index->len);
    encoder->object_layout_type = type;
    smart_str_appendc(encoder->buffer, type);
    write_long(encoder, (long) encoder->object_layout_index->len, type);
    for (size_t i = 0; i < encoder->object_layout_index->len; ++i) {
      type = integer_type_check(encoder->object_layout_index->index[i]->len);
      smart_str_appendc(encoder->buffer, type);
      write_long(encoder, (long) encoder->object_layout_index->index[i]->len, type);
      for (size_t j = 0; j < encoder->object_layout_index->index[i]->len; ++j) {
        write_long(encoder, encoder->object_layout_index->index[i]->entries[j], encoder->string_index_type);
      }
    }
  }
  return;
}

void write_zstr_index(leon_encoder_t *encoder, zend_string *str) {
  int idx = string_index_find(encoder->string_index, str);
  write_long(encoder, (long) idx, encoder->string_index_type);
}

void write_zstr(leon_encoder_t *encoder, zend_string *str) {
  unsigned char len_type = integer_type_check((long) ZSTR_LEN(str));
  smart_str_appendc(encoder->buffer, len_type);
  write_long(encoder, (long) ZSTR_LEN(str), len_type);
  for (unsigned int i = 0; i < ZSTR_LEN(str); ++i) {
    smart_str_appendc(encoder->buffer, ZSTR_VAL(str)[i]);
  }
}

void write_long(leon_encoder_t *encoder, long value, unsigned char type) {
  union char_to_unsigned c_to_u;
  union unsigned_short_to_bytes u_s_to_b;
  union short_to_bytes s_to_b;
  union unsigned_int_to_bytes u_i_to_b;
  union int_to_bytes i_to_b;
  switch (type) {
    case LEON_UNSIGNED_CHAR:
      smart_str_appendc(encoder->buffer, (unsigned char) value);
      break;
    case LEON_CHAR:
      c_to_u.signed_byte = (char) value;
      smart_str_appendc(encoder->buffer, c_to_u.byte);
      break;
    case LEON_UNSIGNED_SHORT:
      u_s_to_b.value = (unsigned short) value;
      if (!encoder->endianness) swizzle(u_s_to_b.bytes, 2);
      for (unsigned int i = 0; i < 2; ++i) {
        smart_str_appendc(encoder->buffer, u_s_to_b.bytes[i]);
      }
      break;
    case LEON_SHORT:
      s_to_b.value = (short) value;
      if (!encoder->endianness) swizzle(s_to_b.bytes, 2);
      for (unsigned int i = 0; i < 2; ++i) {
        smart_str_appendc(encoder->buffer, s_to_b.bytes[i]);
      }
      break;
    case LEON_UNSIGNED_INT:
      u_i_to_b.value = (unsigned int) value;
      if (!encoder->endianness) swizzle(u_i_to_b.bytes, 4);
      for (unsigned int i = 0; i < 4; ++i) {
        smart_str_appendc(encoder->buffer, u_i_to_b.bytes[i]);
      }
      break;
    case LEON_INT:
      i_to_b.value = (int) value;
      if (!encoder->endianness) swizzle(i_to_b.bytes, 4);
      for (unsigned int i = 0; i < 4; ++i) {
        smart_str_appendc(encoder->buffer, i_to_b.bytes[i]);
      }
      break;
  }
}
void write_double(leon_encoder_t *encoder, double d, unsigned char type) {
  union to_float f_to_b;
  union to_double d_to_b;
  if (type == LEON_FLOAT) {
    f_to_b.val = (float) d;
    if (!encoder->endianness) swizzle(f_to_b.bytes, 4);
    for (unsigned int i = 0; i < 4; ++i) {
      smart_str_appendc(encoder->buffer, f_to_b.bytes[i]);
    }
  } else {
    d_to_b.val = d;
    if (!encoder->endianness) swizzle(d_to_b.bytes, 8);
    for (unsigned int i = 0; i < 8; ++i) {
      smart_str_appendc(encoder->buffer, d_to_b.bytes[i]);
    }
  }
}
void write_data_with_spec(leon_encoder_t *encoder, zval *spec, zval *payload) {
  unsigned char type = type_check(spec);
  unsigned char payload_type = type_check(payload);
  zend_string *key;
  zend_ulong index;
  zval *data;
  HashTable *spec_ht;
  HashTable *ht;
  zval *zv_dest;
  oli_entry *entry;
  zend_object *zobj;
  zend_string *prop;
  hash_array_t *ha;
  hash_entry *he;
  int i;
  if (payload_type == LEON_INDIRECT) {
    write_data_with_spec(encoder, spec, Z_INDIRECT_P(payload));
    return;
  }
  switch (type) {
    case LEON_CONSTANT:
      write_data_with_spec(encoder, &((zend_constant *) Z_PTR_P(spec))->value, payload);
      break;
    case LEON_INDIRECT:
      write_data_with_spec(encoder, Z_INDIRECT_P(spec), payload);
      break;
    case LEON_UNSIGNED_CHAR:
      write_data(encoder, payload, 1, (unsigned char) Z_LVAL_P(spec));
      break;
    case LEON_ARRAY:
      spec_ht = Z_ARRVAL_P(spec);
      spec = zend_hash_index_find(spec_ht, 0);
      ht = Z_ARRVAL_P(payload);
      i = zend_hash_num_elements(ht);
      unsigned char type = integer_type_check((long) i);
      smart_str_appendc(encoder->buffer, type);
      write_long(encoder, (long) i, type);
      if (i > 0) {
      do {
        Bucket *_p = (ht)->arData;
        if (!_p) break;
        Bucket *_end = _p + i;
        for (; _p != _end; _p++) {
          zval *_z = &_p->val;
          if (Z_TYPE_P(_z) == IS_INDIRECT) {
            _z = Z_INDIRECT_P(_z);
          }
          if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;
          index = _p->h;
          data = _z;
          write_data_with_spec(encoder, spec, data);
      ZEND_HASH_FOREACH_END();
      }
      break;
    case LEON_OBJECT:
      if (payload_type == LEON_OBJECT) {
        ht = Z_ARRVAL_P(payload);
        spec_ht = Z_ARRVAL_P(spec);
        ha = hash_array_ctor();
        ZEND_HASH_FOREACH_KEY_VAL_IND(spec_ht, index, key, data) {
          he = (hash_entry *) emalloc(sizeof(hash_entry));
          he->key = key;
          he->value = data;
          hash_array_push(ha, he);
        } ZEND_HASH_FOREACH_END();
        hash_array_sort(ha);
        for (int i = 0; i < ha->len; ++i) {
          zv_dest = zend_hash_find(ht, ha->index[i]->key);
          write_data_with_spec(encoder, ha->index[i]->value, zv_dest);
        }
        hash_array_dtor(ha);
      } else {
        spec_ht = Z_ARRVAL_P(spec);
        ht = Z_OBJ_HT_P(payload)->get_properties(payload);
        zobj = Z_OBJ_P(payload);
        if (ht == NULL) {
          return;
        }
        ha = hash_array_ctor();
        ZEND_HASH_FOREACH_KEY_VAL_IND(spec_ht, index, key, data) {
          he = (hash_entry *) emalloc(sizeof(hash_entry));
          he->key = key;
          he->value = data;
          hash_array_push(ha, he);
        } ZEND_HASH_FOREACH_END();
        hash_array_sort(ha);
        for (int i = 0; i < ha->len; ++i) {
          zv_dest = zend_hash_find(ht, ha->index[i]->key);
          write_data_with_spec(encoder, ha->index[i]->value, zv_dest);
        }
        hash_array_dtor(ha);
      }
      break;
  }
}
static void write_string(leon_encoder_t *encoder, char *str, size_t len) {
  unsigned char type = integer_type_check((long) len);
  smart_str_appendc(encoder->buffer, type);
  write_long(encoder, (long) len, type);
  for (size_t i = 0; i < len; ++i) {
    smart_str_appendc(encoder->buffer, str[i]);
  }
}
void write_data(leon_encoder_t *encoder, zval *payload, int implicit, unsigned char t) {
  unsigned char type;
  if (t == LEON_EMPTY || t == LEON_BOOLEAN) type = type_check(payload);
  else type = t;
  if (!implicit && type != LEON_INDIRECT) {
    if (type == LEON_NATIVE_OBJECT) smart_str_appendc(encoder->buffer, LEON_OBJECT);
    else smart_str_appendc(encoder->buffer, type);
  }
  zend_string *key;
  zend_ulong index;
  zval *data;
  HashTable *ht;
  zval *zv_dest;
  oli_entry *entry;
  zend_object *zobj;
  zend_string *prop;
  string_buffer_t *sb;
  int i, layout_idx;
  long l;
  double d;
  switch (type) {
    case LEON_CONSTANT:
      write_data(encoder, &((zend_constant *) Z_PTR_P(payload))->value, implicit, LEON_EMPTY);
      break;
    case LEON_INDIRECT:
      write_data(encoder, Z_INDIRECT_P(payload), implicit, LEON_EMPTY);
      break;
    case LEON_TRUE:
    case LEON_NULL:
    case LEON_UNDEFINED:
    case LEON_FALSE:
    case LEON_NAN:
    case LEON_INFINITY:
    case LEON_MINUS_INFINITY:
      if (implicit) smart_str_appendc(encoder->buffer, type);
      break;
    case LEON_UNSIGNED_CHAR:
    case LEON_CHAR:
    case LEON_UNSIGNED_SHORT:
    case LEON_SHORT:
    case LEON_UNSIGNED_INT:
    case LEON_INT:
      write_long(encoder, Z_LVAL_P(payload), type);
      break;
    case LEON_FLOAT:
    case LEON_DOUBLE:
      write_double(encoder, Z_DVAL_P(payload), type);
      break;
    case LEON_STRING:
      if (encoder->string_index->len) write_zstr_index(encoder, Z_STR_P(payload));
      else write_zstr(encoder, Z_STR_P(payload));
      break;
    case LEON_ARRAY:
      ht = Z_ARRVAL_P(payload);
      i = zend_hash_num_elements(ht);
      unsigned char type = integer_type_check((long) i);
      smart_str_appendc(encoder->buffer, type);
      write_long(encoder, (long) i, type);
      if (i > 0) {
      do {
        Bucket *_p = (ht)->arData;
        if (!_p) break;
        Bucket *_end = _p + i;
        for (; _p != _end; _p++) {
          zval *_z = &_p->val;
          if (Z_TYPE_P(_z) == IS_INDIRECT) {
            _z = Z_INDIRECT_P(_z);
          }
          if (UNEXPECTED(Z_TYPE_P(_z) == IS_UNDEF)) continue;
          index = _p->h;
          data = _z;
          write_data(encoder, data, implicit, LEON_EMPTY);
      ZEND_HASH_FOREACH_END();
      }
      break;
    case LEON_OBJECT:
      ht = Z_ARRVAL_P(payload);
      entry = oli_entry_ctor();
      ZEND_HASH_FOREACH_KEY_VAL_IND(ht, index, key, data) {
        oli_entry_push(entry, string_index_find(encoder->string_index, key));
      } ZEND_HASH_FOREACH_END();
      oli_entry_sort(entry);
      layout_idx = object_layout_index_find(encoder->object_layout_index, entry);
      if (!implicit) write_long(encoder, (long) layout_idx, encoder->object_layout_type);
      for (int i = 0; i < entry->len; ++i) {
        zv_dest = zend_hash_find(ht, encoder->string_index->index[entry->entries[i]]);
        write_data(encoder, zv_dest, implicit, LEON_EMPTY);
      }
      oli_entry_dtor(entry);
      break;
    case LEON_NATIVE_OBJECT:
      entry = oli_entry_ctor();
      ht = Z_OBJ_HT_P(payload)->get_properties(payload);
      zobj = Z_OBJ_P(payload);
      if (ht == NULL) {
        oli_entry_dtor(entry);
        return;
      }
      ZEND_HASH_FOREACH_STR_KEY_VAL_IND(ht, key, data) {
        if (key) {
          if (zend_check_property_access(zobj, key) == SUCCESS) {
            if (ZSTR_VAL(key)[0] == 0) {
              const char *prop_name, *class_name;
              size_t prop_len;
              zend_unmangle_property_name_ex(key, &class_name, &prop_name, &prop_len);
              prop = zend_string_init(prop_name, prop_len, 0);  
              oli_entry_push(entry, string_index_find(encoder->string_index, prop));
            } else {
              oli_entry_push(entry, string_index_find(encoder->string_index, key));
            }
          }
        }
      } ZEND_HASH_FOREACH_END();
      oli_entry_sort(entry);
      layout_idx = object_layout_index_find(encoder->object_layout_index, entry);
      if (!implicit) write_long(encoder, (long) layout_idx, encoder->object_layout_type);
      for (int i = 0; i < entry->len; ++i) {
        zv_dest = zend_hash_find(ht, encoder->string_index->index[entry->entries[i]]);
        write_data(encoder, zv_dest, implicit, LEON_EMPTY);
      }
      oli_entry_dtor(entry);
      break;
    case LEON_REGEXP:
      ht = Z_OBJ_HT_P(payload)->get_properties(payload);
      key = zend_string_init("pattern", sizeof("pattern") - 1, 0);
      data = zend_hash_find(ht, key);
      write_string(encoder, ZSTR_VAL(Z_STR_P(data)), ZSTR_LEN(Z_STR_P(data)));
      key = zend_string_init("modifier", sizeof("modifier") - 1, 0);
      data = zend_hash_find(ht, key);
      write_string(encoder, ZSTR_VAL(Z_STR_P(data)), ZSTR_LEN(Z_STR_P(data)));
      break;
    case LEON_DATE:
      ht = Z_OBJ_HT_P(payload)->get_properties(payload);
      key = zend_string_init("timestamp", sizeof("timestamp") - 1, 0);
      data = zend_hash_find(ht, key);
      l = Z_LVAL_P(data);
      d = (double) l;
      write_double(encoder, d, LEON_DOUBLE);
      break;
    case LEON_BUFFER:
      sb = get_string_buffer(Z_OBJ_P(payload));
      write_string(encoder, sb->buffer->s->val, sb->buffer->s->len);
      break;
  }
}
