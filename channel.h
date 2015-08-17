#ifndef LEON_CHANNEL_H
#define LEON_CHANNEL_H

#include "php.h"
#include "ext/standard/php_smart_str.h"

PHP_METHOD(Channel, __construct);
PHP_METHOD(Channel, encode);
PHP_METHOD(Channel, decode);

void php_leon_channel_decode(zval *ret, zval *this, char *s, size_t len);
void php_leon_channel_encode(smart_str *, zval *this, zval *payload);

#endif
