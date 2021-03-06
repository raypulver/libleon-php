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

#ifndef PHP_LEON_H
#define PHP_LEON_H
#if PHP_API_VERSION <= 20131106
#include "ext/standard/php_smart_str.h"
#else
#include "zend_smart_str.h"
#endif

extern zend_module_entry leon_module_entry;
#define phpext_leon_ptr &leon_module_entry

#define PHP_LEON_VERSION "0.1.0" /* Replace with version number for your extension */

#ifdef PHP_WIN32
#	define PHP_LEON_API __declspec(dllexport)
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define PHP_LEON_API __attribute__ ((visibility("default")))
#else
#	define PHP_LEON_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/*
  	Declare any global variables you may need between the BEGIN
	and END macros here:

ZEND_BEGIN_MODULE_GLOBALS(leon)
	zend_long  global_value;
	char *global_string;
ZEND_END_MODULE_GLOBALS(leon)
*/

/* Always refer to the globals in your function as LEON_G(variable).
   You are encouraged to rename these macros something shorter, see
   examples in any other php module directory.
*/
#define LEON_G(v) ZEND_MODULE_GLOBALS_ACCESSOR(leon, v)

#if defined(ZTS) && defined(COMPILE_DL_LEON)
ZEND_TSRMLS_CACHE_EXTERN();
#endif

#endif	/* PHP_LEON_H */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
#if PHP_API_VERSION <= 20131106
PHP_LEON_API void php_leon_encode(smart_str *buf, zval *payload, zend_long64 options);
PHP_LEON_API void php_leon_decode(zval *return_value, char *payload, size_t len, zend_long64 options);
#else
PHP_LEON_API void php_leon_encode(smart_str *buf, zval *payload, zend_long options);
PHP_LEON_API void php_leon_decode(zval *return_value, char *payload, size_t len, zend_long options);
#endif

extern zend_function_entry channel_functions[];
