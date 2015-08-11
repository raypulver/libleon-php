#ifndef LEON_CLASSES_H
#define LEON_CLASSES_H

#include "php.h"

extern zend_class_entry *channel_ce, *date_ce, *undefined_ce, *nan_ce, *regexp_ce, *infinity_ce, *minus_infinity_ce;

PHP_METHOD(Date, __construct);
PHP_METHOD(RegExp, __construct);

#endif
