#include "php.h"
#include "zend_smart_str.h"
#include "channel.h"
#include "encoder.h"
#include "parser.h"
#include "types.h"

PHP_METHOD(Channel, __construct) {
  zval *spec;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &spec) == FAILURE) {
    return;
  }
  zval *this = getThis();
  zval key;
  ZVAL_STRINGL(&key, "spec", sizeof("spec") - 1);
  Z_OBJ_HT_P(this)->write_property(this, &key, spec, NULL);
  zval_ptr_dtor(&key);
}

PHP_METHOD(Channel, encode) {
  zval *payload;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &payload) == FAILURE) {
    return;
  }
  zval *this = getThis();
  HashTable *ht = Z_OBJ_HT_P(this)->get_properties(this);
  zend_string *sp = zend_string_init("spec", sizeof("spec") - 1, 0);
  zval *spec = zend_hash_find(ht, sp);
  php_leon_channel_encode(return_value, spec, payload);
}
PHP_METHOD(Channel, decode) {
  char *payload;
  size_t len;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &payload, &len) == FAILURE) {
    return;
  }
  zval *this = getThis();
  HashTable *ht = Z_OBJ_HT_P(this)->get_properties(this);
  zend_string *sp = zend_string_init("spec", sizeof("spec") - 1, 0);
  zval *spec = zend_hash_find(ht, sp);
  php_leon_channel_decode(return_value, spec, payload, len);
}

void php_leon_channel_encode(zval *return_value, zval *spec, zval *payload) {
  leon_encoder_t *encoder = encoder_ctor();
  smart_str buf = {0};
  smart_str_free(&buf);
  smart_str_0(&buf);
  encoder->buffer = &buf;
  encoder->string_index_type = LEON_EMPTY;
  write_data_with_spec(encoder, spec, payload);
  ZVAL_STRINGL(return_value, encoder->buffer->s->val, encoder->buffer->s->len);
  encoder_dtor(encoder);
}

void php_leon_channel_decode(zval *return_value, zval *spec, char *payload, size_t len) {
  leon_parser_t *parser = parser_ctor(payload, len);
  parser->string_index_type = LEON_EMPTY;
  *return_value = *parse_value_with_spec(parser, spec);
  parser_dtor(parser);
}
