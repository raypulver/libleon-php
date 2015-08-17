#include "php.h"
#include "classes.h"
PHP_METHOD(Date, __construct) {
  long timestamp;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "l", &timestamp) == FAILURE) {
    return;
  }
  zval *this = getThis();
  zval key;
  zval *value = (zval *) emalloc(sizeof(zval));
#if PHP_API_VERSION <= 20131106
  add_property_long_ex(this, "timestamp", sizeof("timestamp"), timestamp);
#else
  ZVAL_STRINGL(&key, "timestamp", sizeof("timestamp") - 1);
  ZVAL_LONG(value, timestamp);
  Z_OBJ_HT_P(this)->write_property(this, &key, value, NULL);
  zval_ptr_dtor(&key);
#endif
}

PHP_METHOD(RegExp, __construct) {
  char *pattern;
  char modifier[] = {0};
  size_t pattern_len, modifier_len = 0;
  if (zend_parse_parameters(ZEND_NUM_ARGS(), "s|s", &pattern, &pattern_len, &modifier, &modifier_len) == FAILURE) {
    return;
  }
  zval *this = getThis();
#if PHP_API_VERSION <= 20131106
  add_property_stringl(this, "pattern", pattern, pattern_len, 1);
  add_property_stringl(this, "modifier", modifier, modifier_len, 1);
#else
  zval key;
  zval *value = (zval *) emalloc(sizeof(zval));
  ZVAL_STRINGL(&key, "pattern", sizeof("pattern") - 1);
  ZVAL_STRINGL(value, pattern, pattern_len);
  Z_OBJ_HT_P(this)->write_property(this, &key, value, NULL);
  value = (zval *) emalloc(sizeof(zval));
  ZVAL_STRINGL(&key, "modifier", sizeof("modifier") - 1);
  ZVAL_STRINGL(value, modifier, modifier_len);
  Z_OBJ_HT_P(this)->write_property(this, &key, value, NULL);
  zval_ptr_dtor(&key);
#endif
}
