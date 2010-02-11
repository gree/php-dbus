/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2007 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.0 of the PHP license,       |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_0.txt.                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author: Masaki Fujimoto <fujimoto@php.net>                           |
  +----------------------------------------------------------------------+
*/
#ifndef PHP_DBUS_H
#define PHP_DBUS_H

#define PHP_DBUS_VERSION "0.1.1"
#define PHP_DBUS_EXTNAME "dbus"

#ifdef PHP_WIN32
#define PHP_DBUS_API __declspec(dllexport)
#else
#define PHP_DBUS_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

#define	DBUS_GET_THIS(ce)			(getThis() ? (Z_OBJCE_P(getThis()) == ce ? getThis() : NULL) : NULL)

PHP_MINIT_FUNCTION(dbus);
PHP_MSHUTDOWN_FUNCTION(dbus);
PHP_RINIT_FUNCTION(dbus);
PHP_RSHUTDOWN_FUNCTION(dbus);
PHP_MINFO_FUNCTION(dbus);

/* {{{ function prototype */
PHP_FUNCTION(dbus_bus_get);

PHP_FUNCTION(dbus_connection_ctor);
PHP_FUNCTION(dbus_connection_send);
PHP_FUNCTION(dbus_connection_send_with_reply_and_block);
PHP_FUNCTION(dbus_connection_register_object_path);
PHP_FUNCTION(dbus_connection_poll);

PHP_FUNCTION(dbus_message_ctor);
PHP_FUNCTION(dbus_message_set_destination);
PHP_FUNCTION(dbus_message_set_path);
PHP_FUNCTION(dbus_message_set_interface);
PHP_FUNCTION(dbus_message_set_member);
PHP_FUNCTION(dbus_message_set_auto_start);
PHP_FUNCTION(dbus_message_append_args);
PHP_FUNCTION(dbus_message_append_arg1);
PHP_FUNCTION(dbus_message_get_args);
/* }}} */

extern zend_module_entry dbus_module_entry;
#define phpext_dbus_ptr &dbus_module_entry

/* {{{ module global */
ZEND_BEGIN_MODULE_GLOBALS(dbus)
ZEND_END_MODULE_GLOBALS(dbus)

#ifdef ZTS
#define DBUS_G(v) TSRMG(dbus_globals_id, zend_dbus_globals *, v)
#else
#define DBUS_G(v) (dbus_globals.v)
#endif

ZEND_EXTERN_MODULE_GLOBALS(dbus)
/* }}} */

/* {{{ macro */
#define	dbus_objprop_get(zv, key, element, on_error) { \
	if (zend_hash_find(Z_OBJPROP(zv), key, strlen(key)+1, (void**)&element) != SUCCESS) { \
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "property [%s] is not set", key); \
		element = NULL; \
		on_error; \
	} \
}
#define dbus_objprop_get_p(zv_p, key, element, on_error)	dbus_objprop_get(*zv_p, key, element, on_error)
#define dbus_objprop_get_pp(zv_pp, key, element, on_error)	dbus_objprop_get(**zv_pp, key, element, on_error)
/* }}} */
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * indent-tabs-mode: t
 * End:
 */
