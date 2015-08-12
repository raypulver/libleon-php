/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2015 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_leon.h"
#include "zend_smart_str.h"

#include "types.h"
#include "endianness.h"
#include "encoder.h"
#include "parser.h"
#include "channel.h"
#include "classes.h"
#include "string_buffer.h"

/* If you declare any globals in php_leon.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(leon)
*/

/* True global resources - no need for thread safety here */
static int le_leon;
zend_class_entry *channel_ce, *date_ce, *undefined_ce, *nan_ce, *regexp_ce, *infinity_ce, *minus_infinity_ce;
ZEND_BEGIN_ARG_INFO_EX(arginfo_void, 0, 0, 0)
ZEND_END_ARG_INFO();

zend_function_entry date_functions[] = {
        PHP_ME(Date, __construct, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_FE_END
};

zend_function_entry regexp_functions[] = {
        PHP_ME(RegExp, __construct, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_FE_END
};

zend_function_entry no_functions[] = {
        PHP_FE_END
};

zend_function_entry string_buffer_functions[] = {
        PHP_ME(StringBuffer, __construct, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, readUInt8, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, writeUInt8, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, readUInt16LE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, readUInt16BE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, writeUInt16LE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, writeUInt16BE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, readUInt32LE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, readUInt32BE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, writeUInt32LE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, writeUInt32BE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, readFloatLE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, readFloatBE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, writeFloatLE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, writeFloatBE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, readDoubleLE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, readDoubleBE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, writeDoubleLE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, writeDoubleBE, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, toString, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(StringBuffer, length, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_FE_END
};

zend_function_entry channel_functions[] = {
        PHP_ME(Channel, __construct, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(Channel, encode, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_ME(Channel, decode, arginfo_void, ZEND_ACC_PUBLIC)
        PHP_FE_END
};

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("leon.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_leon_globals, leon_globals)
    STD_PHP_INI_ENTRY("leon.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_leon_globals, leon_globals)
PHP_INI_END()
*/
/* }}} */

/* Remove the following function when you have successfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_leon_compiled(string arg)
   Return a string to confirm that the module is compiled in */

PHP_LEON_API void php_leon_encode(zval *return_value, zval *payload) {
  leon_encoder_t *encoder = encoder_ctor();
  smart_str buf = {0};
  smart_str_free(&buf);
  smart_str_0(&buf);
  encoder->buffer = &buf;
  write_string_index(encoder, payload, 0);
  write_object_layout_index(encoder, payload, 0);
  write_data(encoder, payload, 0, 0xFF);
  ZVAL_STRINGL(return_value, encoder->buffer->s->val, encoder->buffer->s->len);
  encoder_dtor(encoder);
}
PHP_LEON_API void php_leon_decode(zval *return_value, char *payload, size_t len)
{
        leon_parser_t *parser = parser_ctor(payload, len);
        parse_string_index(parser);
        parse_object_layout_index(parser);
        parse_value(parser, read_uint8(parser), return_value);
        parser_dtor(parser);
} 

PHP_FUNCTION(leon_encode)
{
        zval *payload;
        if (zend_parse_parameters(ZEND_NUM_ARGS(), "z", &payload) == FAILURE) return;
        php_leon_encode(return_value, payload);
}
PHP_FUNCTION(leon_decode)
{
        char *payload;
        size_t len;
        if (zend_parse_parameters(ZEND_NUM_ARGS(), "s", &payload, &len) == FAILURE) return;
        php_leon_decode(return_value, payload, len);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and
   unfold functions in source code. See the corresponding marks just before
   function definition, where the functions purpose is also documented. Please
   follow this convention for the convenience of others editing your code.
*/


/* {{{ php_leon_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_leon_init_globals(zend_leon_globals *leon_globals)
{
	leon_globals->global_value = 0;
	leon_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(leon)
{
	/* If you have INI entries, uncomment these lines
	REGISTER_INI_ENTRIES();
	*/
        REGISTER_LONG_CONSTANT("LEON_UNSIGNED_CHAR", LEON_UNSIGNED_CHAR, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_CHAR", LEON_CHAR, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_UNSIGNED_SHORT", LEON_UNSIGNED_SHORT, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_SHORT", LEON_SHORT, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_UNSIGNED_INT", LEON_UNSIGNED_INT, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_INT", LEON_INT, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_FLOAT", LEON_FLOAT, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_DOUBLE", LEON_DOUBLE, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_STRING", LEON_STRING, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_BOOLEAN", LEON_BOOLEAN, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_NULL", LEON_NULL, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_UNDEFINED", LEON_UNDEFINED, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_DATE", LEON_DATE, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_BUFFER", LEON_BUFFER, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_REGEXP", LEON_REGEXP, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_NAN", LEON_NAN, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_INFINITY", LEON_INFINITY, CONST_CS | CONST_PERSISTENT);
        REGISTER_LONG_CONSTANT("LEON_MINUS_INFINITY", LEON_MINUS_INFINITY, CONST_CS | CONST_PERSISTENT);
        zend_class_entry tmp_channel;
        zend_class_entry tmp_string_buffer;
        zend_class_entry tmp_date;
        zend_class_entry tmp_regexp;
        zend_class_entry tmp_undefined;
        zend_class_entry tmp_nan;
        zend_class_entry tmp_infinity;
        zend_class_entry tmp_minus_infinity;
        INIT_CLASS_ENTRY(tmp_string_buffer, "LEON\\StringBuffer", string_buffer_functions);
        INIT_CLASS_ENTRY(tmp_channel, "LEON\\Channel", channel_functions);
        INIT_CLASS_ENTRY(tmp_date, "LEON\\Date", date_functions);
        INIT_CLASS_ENTRY(tmp_regexp, "LEON\\RegExp", regexp_functions);
        INIT_CLASS_ENTRY(tmp_undefined, "LEON\\Undefined", no_functions);
        INIT_CLASS_ENTRY(tmp_nan, "LEON\\NaN", no_functions);
        INIT_CLASS_ENTRY(tmp_infinity, "LEON\\Infinity", no_functions);
        INIT_CLASS_ENTRY(tmp_minus_infinity, "LEON\\MinusInfinity", no_functions);
        string_buffer_ce = zend_register_internal_class(&tmp_string_buffer TSRMLS_CC);
        string_buffer_ce->create_object = string_buffer_create;
        memcpy(&string_buffer_object_handlers, zend_get_std_object_handlers(), sizeof(zend_object_handlers));
        date_ce = zend_register_internal_class(&tmp_date TSRMLS_CC);
        regexp_ce = zend_register_internal_class(&tmp_regexp TSRMLS_CC);
        undefined_ce = zend_register_internal_class(&tmp_undefined TSRMLS_CC);
        nan_ce = zend_register_internal_class(&tmp_nan TSRMLS_CC);
        infinity_ce = zend_register_internal_class(&tmp_infinity TSRMLS_CC);
        minus_infinity_ce = zend_register_internal_class(&tmp_minus_infinity TSRMLS_CC);
        channel_ce = zend_register_internal_class(&tmp_channel TSRMLS_CC);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(leon)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(leon)
{
#if defined(COMPILE_DL_LEON) && defined(ZTS)
	ZEND_TSRMLS_CACHE_UPDATE();
#endif
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(leon)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(leon)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "leon support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */

/* {{{ leon_functions[]
 *
 * Every user visible function must have an entry in leon_functions[].
 */

zend_function_entry leon_functions[] = {
        PHP_FE(leon_encode, NULL)
        PHP_FE(leon_decode, NULL)
	PHP_FE_END	/* Must be the last line in leon_functions[] */
};
/* }}} */

/* {{{ leon_module_entry
 */
zend_module_entry leon_module_entry = {
	STANDARD_MODULE_HEADER,
	"leon",
	leon_functions,
	PHP_MINIT(leon),
	PHP_MSHUTDOWN(leon),
	PHP_RINIT(leon),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(leon),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(leon),
	PHP_LEON_VERSION,
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_LEON
#ifdef ZTS
ZEND_TSRMLS_CACHE_DEFINE();
#endif
ZEND_GET_MODULE(leon)
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
