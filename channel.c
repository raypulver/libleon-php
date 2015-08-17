#include "php.h"
#if PHP_API_VERSION <= 20131106
#include "ext/standard/php_smart_str.h"
#else
#include "zend_smart_str.h"
#endif
#include "channel.h"
#include "encoder.h"
#include "parser.h"
#include "types.h"
#include "php_leon.h"

PHP_METHOD(Channel, __construct) {
  zval *spec;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &spec) == FAILURE) {
    return;
  }
  zval *this = getThis();
#if PHP_API_VERSION <= 20131106
  add_property_zval_ex(this, "spec", sizeof("spec"), spec);
#else
  zval key;
  ZVAL_STRINGL(&key, "spec", sizeof("spec") - 1);
  Z_OBJ_HT_P(this)->write_property(this, &key, spec, NULL);
  zval_ptr_dtor(&key);
#endif
}
PHP_METHOD(Channel, encode) {
  zval *payload;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &payload) == FAILURE) {
    return;
  }
  zval *this = getThis();
#if PHP_API_VERSION <= 20131106
  HashTable *myht = Z_OBJPROP_P(this);
#else
  HashTable *myht = Z_OBJ_HT_P(this)->get_properties(this);
#endif
  zval *spec;
#if PHP_API_VERSION <= 20131106
  zval **data;
  zend_hash_find(myht, "spec", sizeof("spec"), (void **) &data);
  spec = *data;
#else
  zend_string *sp = zend_string_init("spec", sizeof("spec") - 1, 0);
  spec = zend_hash_find(myht, sp);
  zend_string_release(sp);
#endif
  smart_str buf = {0};
  smart_str_free(&buf);
  smart_str_0(&buf);
  php_leon_channel_encode(&buf, spec, payload);
#if PHP_API_VERSION <= 20131106
  ZVAL_STRINGL(return_value, buf.c, buf.len, 0);
#else
  ZVAL_STRINGL(return_value, buf.s->val, buf.s->len)
#endif
}
PHP_METHOD(Channel, decode) {
  char *payload;
  size_t len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &payload, &len) == FAILURE) {
    return;
  }
  zval *this = getThis();
#if PHP_API_VERSION <= 20131106
  HashTable *myht = Z_OBJPROP_P(this);
#else
  HashTable *myht = Z_OBJ_HT_P(this)->get_properties(this);
#endif
  zval *spec;
#if PHP_API_VERSION <= 20131106
  zval **data;
  zend_hash_find(myht, "spec", sizeof("spec"), (void **) &data);
  spec = *data;
#else
  zend_string *sp = zend_string_init("spec", sizeof("spec") - 1, 0);
  zval *spec = zend_hash_find(myht, sp);
  zend_string_release(sp);
#endif
  php_leon_channel_decode(return_value, spec, payload, len);
}
void php_leon_channel_encode(smart_str *str, zval *spec, zval *payload) {
  leon_encoder_t *encoder = encoder_ctor();
  encoder->buffer = str;
  encoder->string_index_type = LEON_EMPTY;
  write_data_with_spec(encoder, spec, payload);
  // encoder_dtor(encoder);
}

PHP_LEON_API void php_leon_channel_decode(zval *return_value, zval *spec, char *payload, size_t len) {
  leon_parser_t *parser = parser_ctor(payload, len);
  parser->string_index_type = LEON_EMPTY;
  zval ret;
  parse_value_with_spec(parser, spec, &ret);
  *return_value = ret;
  parser_dtor(parser);
}
