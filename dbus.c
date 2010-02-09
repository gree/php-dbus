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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <poll.h>
#include "dbus/dbus.h"
#include "php.h"
#include "php_ini.h"
#include "php_dbus.h"

ZEND_DECLARE_MODULE_GLOBALS(dbus)

typedef struct _dbus_connection_watch_list {
	DBusWatch **watch_list;
	int watch_count;
} dbus_connection_watch_list;


/* {{{ dbus_functions[] */
static function_entry dbus_functions[] = {
	PHP_FE(dbus_bus_get, NULL)

	PHP_FE(dbus_connection_ctor, NULL)
	PHP_FE(dbus_connection_send, NULL)
	PHP_FE(dbus_connection_send_with_reply_and_block, NULL)
	PHP_FE(dbus_connection_register_object_path, NULL)
	PHP_FE(dbus_connection_poll, NULL)

	PHP_FE(dbus_message_ctor, NULL)
	PHP_FE(dbus_message_set_destination, NULL)
	PHP_FE(dbus_message_set_path, NULL)
	PHP_FE(dbus_message_set_interface, NULL)
	PHP_FE(dbus_message_set_member, NULL)
	PHP_FE(dbus_message_set_auto_start, NULL)
	PHP_FE(dbus_message_append_args, NULL)
	PHP_FE(dbus_message_get_args, NULL)
    {NULL, NULL, NULL}
};
/* }}} */

/* {{{ dbus_connection_functions[] */
static zend_function_entry dbus_connection_functions[] = {
	PHP_FALIAS(dbusconnection,			dbus_connection_ctor,						NULL)
	PHP_FALIAS(send,					dbus_connection_send,	NULL)
	PHP_FALIAS(sendwithreplyandblock,	dbus_connection_send_with_reply_and_block,	NULL)
	PHP_FALIAS(registerobjectpath,		dbus_connection_register_object_path,		NULL)
	PHP_FALIAS(poll,					dbus_connection_poll,						NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ dbus_message_functions[] */
static zend_function_entry dbus_message_functions[] = {
	PHP_FALIAS(dbusmessage,			dbus_message_ctor,					NULL)
	PHP_FALIAS(setdestination,		dbus_message_set_destination,		NULL)
	PHP_FALIAS(setpath,				dbus_message_set_path,				NULL)
	PHP_FALIAS(setinterface,		dbus_message_set_interface,			NULL)
	PHP_FALIAS(setmember,			dbus_message_set_member,			NULL)
	PHP_FALIAS(setautostart,		dbus_message_set_auto_start,		NULL)
	PHP_FALIAS(appendargs,			dbus_message_append_args,			NULL)
	PHP_FALIAS(getargs,				dbus_message_get_args,				NULL)
	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ dbus_module_entry */
zend_module_entry dbus_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
    STANDARD_MODULE_HEADER,
#endif
    PHP_DBUS_EXTNAME,
    dbus_functions,
	PHP_MINIT(dbus),
	PHP_MSHUTDOWN(dbus),
	PHP_RINIT(dbus),					/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(dbus),				/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(dbus),
#if ZEND_MODULE_API_NO >= 20010901
    PHP_DBUS_VERSION,
#endif
    STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_DBUS
ZEND_GET_MODULE(dbus)
#endif

/* {{{ static function prototype */
static void _dbus_connection_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC);
static void _dbus_connection_watch_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC);
static void _dbus_connection_callback_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC);
static void _dbus_message_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC);
static int _dbus_connection_ctor(zval *obj, DBusConnection *c TSRMLS_DC);
static int _dbus_message_ctor(zval *obj, DBusMessage *m TSRMLS_DC);
static DBusConnection* _dbus_connection_resource(zval *obj TSRMLS_DC);
static DBusMessage* _dbus_message_resource(zval *obj TSRMLS_DC);

static int _dbus_connection_add_watch_list(dbus_connection_watch_list *watch_list, DBusWatch *watch);
static int _dbus_connection_remove_watch_list(dbus_connection_watch_list *watch_list, DBusWatch *watch);
static dbus_bool_t _dbus_connection_add_watch(DBusWatch *watch, void *data);
static void _dbus_connection_remove_watch(DBusWatch *watch, void *data);
static void _dbus_connection_toggle_watch(DBusWatch *watch, void *data);
static void _dbus_connection_free_watch(void *data);

static DBusHandlerResult _dbus_connection_callback_object_path(DBusConnection *c, DBusMessage *m, void *data);
/* }}} */

/* {{{ static variables */
static int le_dbus_connection;
static int le_dbus_connection_watch;
static int le_dbus_connection_callback;
static int le_dbus_message;
static zend_class_entry *dbus_connection_entry_ptr = NULL;
static zend_class_entry *dbus_message_entry_ptr = NULL;
/* }}} */

/* {{{ module interface */
/* {{{ php_dbus_init_globals */
static void php_dbus_init_globals(zend_dbus_globals *dbus_globals TSRMLS_DC) {   
}
/* }}} */

/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(dbus) {
	ZEND_INIT_MODULE_GLOBALS(dbus, php_dbus_init_globals, NULL);

	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/

	le_dbus_connection = zend_register_list_destructors_ex(_dbus_connection_resource_dtor, NULL, "dbus connection internal object", module_number);
	le_dbus_connection_watch = zend_register_list_destructors_ex(_dbus_connection_watch_resource_dtor, NULL, "dbus connection watch list", module_number);
	le_dbus_connection_callback = zend_register_list_destructors_ex(_dbus_connection_callback_resource_dtor, NULL, "dbus connection callback list", module_number);
	le_dbus_message = zend_register_list_destructors_ex(_dbus_message_resource_dtor, NULL, "dbus message internal object", module_number);

	zend_class_entry dbus_connection_entry;
	INIT_CLASS_ENTRY(dbus_connection_entry, "dbusconnection", dbus_connection_functions);
	dbus_connection_entry_ptr = zend_register_internal_class(&dbus_connection_entry TSRMLS_CC);

	zend_class_entry dbus_message_entry;
	INIT_CLASS_ENTRY(dbus_message_entry, "dbusmessage", dbus_message_functions);
	dbus_message_entry_ptr = zend_register_internal_class(&dbus_message_entry TSRMLS_CC);

	REGISTER_LONG_CONSTANT("DBUS_BUS_SESSION", DBUS_BUS_SESSION, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("DBUS_BUS_SYSTEM", DBUS_BUS_SYSTEM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("DBUS_BUS_STARTER", DBUS_BUS_STARTER, CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("DBUS_MESSAGE_TYPE_METHOD_CALL", DBUS_MESSAGE_TYPE_METHOD_CALL, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("DBUS_MESSAGE_TYPE_SIGNAL", DBUS_MESSAGE_TYPE_SIGNAL, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(dbus) {
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(dbus) {
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(dbus) {
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(dbus) {
	php_info_print_table_start();
	php_info_print_table_row(2, "DBus", "enabled");
	php_info_print_table_row(2, "Revision", "$Revision: 142 $");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */
/* }}} */

/* {{{ static functions */
/* {{{ _dbus_connection_resource_dtor */
static void _dbus_connection_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
	dbus_connection_unref((DBusConnection*)rsrc->ptr);
}
/* }}} */

/* {{{ _dbus_connection_watch_resource_dtor */
static void _dbus_connection_watch_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
	dbus_connection_watch_list *watch_list = (dbus_connection_watch_list*)rsrc->ptr;
	if (watch_list->watch_list) {
		efree(watch_list->watch_list);
	}
	efree(watch_list);
}
/* }}} */

/* {{{ _dbus_connection_callback_resource_dtor */
static void _dbus_connection_callback_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
	HashTable *ct = (HashTable*)rsrc->ptr;
	zend_hash_destroy(ct);
	efree(ct);
}
/* }}} */

/* {{{ _dbus_message_resource_dtor */
static void _dbus_message_resource_dtor(zend_rsrc_list_entry *rsrc TSRMLS_DC) {
	dbus_message_unref((DBusMessage*)rsrc->ptr);
}
/* }}} */

/* {{{ _dbus_connection_ctor */
static int _dbus_connection_ctor(zval *obj, DBusConnection *c TSRMLS_DC) {
	if (!obj) {
		return -1;
	}

	int list_id;
	list_id = zend_list_insert(c, le_dbus_connection);
	add_property_resource(obj, "connection", list_id);

	dbus_connection_watch_list *watch_list = emalloc(sizeof(dbus_connection_watch_list));
	watch_list->watch_list = emalloc(0);
	watch_list->watch_count = 0;
	list_id = zend_list_insert(watch_list, le_dbus_connection_watch);
	add_property_resource(obj, "connection_watch", list_id);

	HashTable *ct = (HashTable*)emalloc(sizeof(HashTable));;
	zend_hash_init(ct, 0, NULL, ZVAL_PTR_DTOR, 1);
	list_id = zend_list_insert(ct, le_dbus_connection_callback);
	add_property_resource(obj, "connection_callback", list_id);

	return 0;
}
/* }}} */

/* {{{ _dbus_message_ctor */
static int _dbus_message_ctor(zval *obj, DBusMessage *m TSRMLS_DC) {
	if (!obj) {
		return -1;
	}

	int list_id = zend_list_insert(m, le_dbus_message);
	add_property_resource(obj, "message", list_id);

	return 0;
}
/* }}} */

/* {{{ _dbus_connection_resource */
static DBusConnection* _dbus_connection_resource(zval *obj TSRMLS_DC) {
	zval **tmp;
	int resource_type;
	dbus_objprop_get_p(obj, "connection", tmp, 0);
	if (tmp == NULL) {
		return NULL;
	}
	void *connection = (void*)zend_list_find(Z_LVAL_PP(tmp), &resource_type);
	if (connection == NULL || resource_type != le_dbus_connection) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "connection identifier not found");
		return NULL;
	}

	return connection;
}
/* }}} */

/* {{{ _dbus_message_resource */
static DBusMessage* _dbus_message_resource(zval *obj TSRMLS_DC) {
	zval **tmp;
	int resource_type;
	dbus_objprop_get_p(obj, "message", tmp, 0);
	if (tmp == NULL) {
		return NULL;
	}
	void *message = (void*)zend_list_find(Z_LVAL_PP(tmp), &resource_type);
	if (message == NULL || resource_type != le_dbus_message) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "message identifier not found");
		return NULL;
	}

	return message;
}
/* }}} */

/* {{{ _dbus_connection_add_watch_list */
static int _dbus_connection_add_watch_list(dbus_connection_watch_list *watch_list, DBusWatch *watch) {
	DBusWatch **p = watch_list->watch_list;
	DBusWatch **q = emalloc(sizeof(DBusWatch*) * (watch_list->watch_count+1));
	int i;
	for (i = 0; i < watch_list->watch_count; i++) {
		if (*p == watch) {
			efree(q);
			return TRUE;
		}
		*q = *p;
		p++;
		q++;
	}
	*q = watch;
	efree(p);
	watch_list->watch_list = q;
	watch_list->watch_count++;

	return 0;
}
/* }}} */

/* {{{ _dbus_connection_remove_watch_list */
static int _dbus_connection_remove_watch_list(dbus_connection_watch_list *watch_list, DBusWatch *watch) {
	DBusWatch **p = watch_list->watch_list;
	DBusWatch **q = emalloc(sizeof(DBusWatch*) * watch_list->watch_count);
	int i, j = 0;
	for (i = 0; i < watch_list->watch_count; i++) {
		if (*p == watch) {
			j = 1;
			p++;
			continue;
		}
		*q = *p;
		p++;
		q++;
	}
	efree(p);
	watch_list->watch_list = q;
	if (j) {
		watch_list->watch_count--;
	}
}
/* }}} */

/* {{{ _dbus_connection_add_watch */
static dbus_bool_t _dbus_connection_add_watch(DBusWatch *watch, void *data) {
	if (!dbus_watch_get_enabled(watch)) {
		return TRUE;
	}

	dbus_connection_watch_list *watch_list = (dbus_connection_watch_list*)data;
	_dbus_connection_add_watch_list(watch_list, watch);

	return TRUE;
}
/* }}} */

/* {{{ _dbus_connection_remove_watch */
static void _dbus_connection_remove_watch(DBusWatch *watch, void *data) {
	dbus_connection_watch_list *watch_list = (dbus_connection_watch_list*)data;
	_dbus_connection_remove_watch_list(watch_list, watch);
}
/* }}} */

/* {{{ _dbus_connection_toggle_watch */
static void _dbus_connection_toggle_watch(DBusWatch *watch, void *data) {
	dbus_connection_watch_list *watch_list = (dbus_connection_watch_list*)data;
	if (dbus_watch_get_enabled(watch)) {
		_dbus_connection_add_watch_list(watch_list, watch);
	} else {
		_dbus_connection_remove_watch_list(watch_list, watch);
	}
}
/* }}} */

/* {{{ _dbus_connection_free_watch */
static void _dbus_connection_free_watch(void *data) {
}
/* }}} */

/* {{{ _dbus_connection_callback_object_path */
static DBusHandlerResult _dbus_connection_callback_object_path(DBusConnection *c, DBusMessage *m, void *data) {
	TSRMLS_FETCH();
	HashTable *ct = (HashTable*)data;

	const char *path = dbus_message_get_path(m);
	if (!path) {
		return DBUS_HANDLER_RESULT_HANDLED;
	}

	// ...
	char *path_tmp = emalloc(strlen(path)+1);
	strcpy(path_tmp, path);

	zval **tmp;
	if (zend_hash_find(ct, path_tmp, strlen(path_tmp)+1, (void**)&tmp) != SUCCESS) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "no callback found for [%s]", path);
		efree(path_tmp);
		return DBUS_HANDLER_RESULT_HANDLED;
	}
	efree(path_tmp);

	dbus_message_ref(m);

	zval *r = NULL;
	zval ***arg_list = emalloc(sizeof(zval**) * 1);
	*arg_list = emalloc(sizeof(zval*));

	MAKE_STD_ZVAL(**arg_list);
	object_init_ex(**arg_list, dbus_message_entry_ptr);
	_dbus_message_ctor(**arg_list, m TSRMLS_CC);

	call_user_function_ex(EG(function_table), NULL, *tmp, &r, 1, arg_list, 0, NULL TSRMLS_CC);

	if (r) {
		zval_ptr_dtor(&r);
	}
	zval_ptr_dtor(*arg_list);
	efree(*arg_list);
	efree(arg_list);

	return DBUS_HANDLER_RESULT_HANDLED;
}
/* }}} */
/* }}} */

/* {{{ proto dbus_bus_get() */
PHP_FUNCTION(dbus_bus_get) {
	int type;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &type) == FAILURE) {
		return;
	}

	DBusError e;
	dbus_error_init(&e);

	DBusConnection *c = dbus_bus_get(type, &e);
	if (!c) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "failed to create dbus connection object [%s]", e.message);
		dbus_error_free(&e);
		RETURN_FALSE;
	}

	/* create DBusConnection object and set resource */
	object_init_ex(return_value, dbus_connection_entry_ptr);
	_dbus_connection_ctor(return_value, c TSRMLS_CC);

	/* setup watch function */
	zval **tmp;
	int resource_type;
	dbus_objprop_get_p(return_value, "connection_watch", tmp, 0);
	dbus_connection_watch_list *p = (dbus_connection_watch_list*)zend_list_find(Z_LVAL_PP(tmp), &resource_type);
	dbus_connection_set_watch_functions(
			c,
			_dbus_connection_add_watch,
			_dbus_connection_remove_watch,
			_dbus_connection_toggle_watch,
			p,
			_dbus_connection_free_watch);
}
/* }}} */

/* {{{ proto dbus_connection_ctor() */
PHP_FUNCTION(dbus_connection_ctor) {
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "cannot create DBusConnection object directly (use dbus_bus_get())");
}
/* }}} */

/* {{{ proto dbus_connection_send() */
PHP_FUNCTION(dbus_connection_send) {
	zval *message;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O", &message, dbus_message_entry_ptr) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_connection_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusConnection *c = _dbus_connection_resource(obj TSRMLS_CC);
	if (!c) {
		RETURN_FALSE;
	}

	DBusMessage *m = _dbus_message_resource(message TSRMLS_CC);
	if (!m) {
		RETURN_FALSE;
	}

	dbus_message_set_no_reply(m, TRUE);

	if (!dbus_connection_send(c, m, NULL)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "dbus_connection_send failed");
		RETURN_FALSE;
	}

	RETURN_TRUE;
}

/* {{{ proto dbus_connection_send_with_reply_and_block() */
PHP_FUNCTION(dbus_connection_send_with_reply_and_block) {
	zval *message;
	int timeout = -1;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "O|l", &message, dbus_message_entry_ptr, &timeout) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_connection_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusConnection *c = _dbus_connection_resource(obj TSRMLS_CC);
	if (!c) {
		RETURN_FALSE;
	}

	DBusMessage *m = _dbus_message_resource(message TSRMLS_CC);
	if (!m) {
		RETURN_FALSE;
	}

	DBusError e;
	dbus_error_init(&e);
	DBusMessage *r = dbus_connection_send_with_reply_and_block(c, m, timeout > 0 ?  timeout * 1000 : timeout, &e);
	if (!r) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "dbus_connection_send_with_reply_and_block() failed (%s)", e.message);
		dbus_error_free(&e);
		RETURN_FALSE;
	}

	object_init_ex(return_value, dbus_message_entry_ptr);
	_dbus_message_ctor(return_value, r TSRMLS_CC);
}
/* }}} */

/* {{{ proto dbus_connection_register_object_path() */
PHP_FUNCTION(dbus_connection_register_object_path) {
	char *path;
	int path_len;
	zval *callback;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sz", &path, &path_len, &callback) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_connection_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	if (Z_TYPE_P(callback) != IS_STRING && Z_TYPE_P(callback) != IS_ARRAY) {
		convert_to_string(callback);
	}
	char *name;
	if (!zend_is_callable(callback, 0, &name)) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "invalid callback");
		efree(name);
		RETURN_FALSE;
	}
	efree(name);
	
	DBusConnection *c = _dbus_connection_resource(obj TSRMLS_CC);
	if (!c) {
		RETURN_FALSE;
	}

	zval **tmp;
	int resource_type;
	dbus_objprop_get_p(obj, "connection_callback", tmp, 0);
	HashTable *ct = (HashTable*)zend_list_find(Z_LVAL_PP(tmp), &resource_type);
	zval_add_ref(&callback);
	zend_hash_update(ct, path, path_len+1, &callback, sizeof(zval*), NULL);

	DBusObjectPathVTable vt;
	vt.message_function = _dbus_connection_callback_object_path;
	vt.unregister_function = NULL;
	if (!dbus_connection_register_object_path(c, path, &vt, ct)) {
		RETURN_FALSE;
	}
	
	RETURN_TRUE;
}
/* }}} */

/* {{{ proto dbus_connection_poll() */
PHP_FUNCTION(dbus_connection_poll) {
	int timeout;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &timeout) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_connection_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusConnection *c = _dbus_connection_resource(obj TSRMLS_CC);
	if (!c) {
		RETURN_FALSE;
	}

	zval **tmp;
	int resource_type;
	dbus_objprop_get_p(obj, "connection_watch", tmp, 0);
	dbus_connection_watch_list *watch_list = (dbus_connection_watch_list*)zend_list_find(Z_LVAL_PP(tmp), &resource_type);

	if (watch_list->watch_count <= 0) {
		php_error_docref(NULL TSRMLS_CC, E_WARNING, "nothing to watch");
		RETURN_FALSE;
	}

	int i;
	struct pollfd *pfd = emalloc(sizeof(struct pollfd) * watch_list->watch_count);
	for (i = 0; i < watch_list->watch_count; i++) {
		int fd = dbus_watch_get_fd(watch_list->watch_list[i]);
		pfd[i].fd = fd;
		pfd[i].events = POLLIN;
	}
	int r = poll(pfd, watch_list->watch_count, timeout > 0 ? timeout * 1000 : timeout);
	efree(pfd);

	for (i = 0; i < watch_list->watch_count; i++) {
		dbus_watch_handle(watch_list->watch_list[i], DBUS_WATCH_READABLE);
	}

	dbus_connection_ref(c);
	while (dbus_connection_dispatch(c) == DBUS_DISPATCH_DATA_REMAINS) {
		;
	}
	dbus_connection_unref(c);
	dbus_connection_flush(c);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto dbus_message_ctor() */
PHP_FUNCTION(dbus_message_ctor) {
	int type;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &type) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_message_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusMessage *m = dbus_message_new(type);
	if (!m) {
		RETURN_FALSE;
	}

	_dbus_message_ctor(obj, m TSRMLS_CC);
}
/* }}} */

/* {{{ proto dbus_message_set_destination() */
PHP_FUNCTION(dbus_message_set_destination) {
	char *destination;
	int destination_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &destination, &destination_len) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_message_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusMessage *m = _dbus_message_resource(obj TSRMLS_CC);
	if (!m) {
		RETURN_FALSE;
	}

	if (dbus_message_set_destination(m, destination_len > 0 ? destination : NULL)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto dbus_message_set_path() */
PHP_FUNCTION(dbus_message_set_path) {
	char *path;
	int path_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &path, &path_len) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_message_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusMessage *m = _dbus_message_resource(obj TSRMLS_CC);
	if (!m) {
		RETURN_FALSE;
	}

	if (dbus_message_set_path(m, path_len > 0 ? path : NULL)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto dbus_message_set_interface() */
PHP_FUNCTION(dbus_message_set_interface) {
	char *interface;
	int interface_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &interface, &interface_len) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_message_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusMessage *m = _dbus_message_resource(obj TSRMLS_CC);
	if (!m) {
		RETURN_FALSE;
	}

	if (dbus_message_set_interface(m, interface_len > 0 ? interface : NULL)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto dbus_message_set_member() */
PHP_FUNCTION(dbus_message_set_member) {
	char *member;
	int member_len;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &member, &member_len) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_message_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusMessage *m = _dbus_message_resource(obj TSRMLS_CC);
	if (!m) {
		RETURN_FALSE;
	}

	if (dbus_message_set_member(m, member_len > 0 ? member : NULL)) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */

/* {{{ proto dbus_message_set_auto_start() */
PHP_FUNCTION(dbus_message_set_auto_start) {
	zend_bool flag;
    if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "b", &flag) == FAILURE) {
		return;
	}

	zval *obj = DBUS_GET_THIS(dbus_message_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusMessage *m = _dbus_message_resource(obj TSRMLS_CC);
	if (!m) {
		RETURN_FALSE;
	}

	dbus_message_set_auto_start(m, flag ? TRUE : FALSE);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto dbus_message_append_args() */
PHP_FUNCTION(dbus_message_append_args) {
	zval *obj = DBUS_GET_THIS(dbus_message_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusMessage *m = _dbus_message_resource(obj TSRMLS_CC);
	if (!m) {
		RETURN_FALSE;
	}

	int argc = ZEND_NUM_ARGS();
	zval ***args;
	args = (zval***)safe_emalloc(argc, sizeof(zval**), 0);
	if (argc == 0 || zend_get_parameters_array_ex(argc, args) == FAILURE) {
		efree(args);
		WRONG_PARAM_COUNT;
	}

	// we use iterator here...
	int i;
	for (i = 0; i < argc; i++) {
		zval **arg = args[i];

		DBusMessageIter iter;
		dbus_message_iter_init_append(m, &iter);

		switch (Z_TYPE_PP(arg)) {
		case IS_STRING:
			dbus_message_iter_append_basic(&iter, DBUS_TYPE_STRING, &(Z_STRVAL_PP(arg)));
			break;
		case IS_LONG:
			dbus_message_iter_append_basic(&iter, DBUS_TYPE_INT32, &(Z_STRVAL_PP(arg)));
			break;
		case IS_DOUBLE:
			dbus_message_iter_append_basic(&iter, DBUS_TYPE_DOUBLE, &(Z_STRVAL_PP(arg)));
			break;
		case IS_BOOL:
			dbus_message_iter_append_basic(&iter, DBUS_TYPE_BOOLEAN, &(Z_STRVAL_PP(arg)));
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "unsupported type of argument -> skipping (type:%d, index:%d)", Z_TYPE_PP(arg), i);
			break;
		}
	}

	efree(args);

	RETURN_TRUE;
}
/* }}} */

/* {{{ proto dbus_message_get_args() */
PHP_FUNCTION(dbus_message_get_args) {
	zval *obj = DBUS_GET_THIS(dbus_message_entry_ptr);
	if (!obj) {
		RETURN_FALSE;
	}

	DBusMessage *m = _dbus_message_resource(obj TSRMLS_CC);
	if (!m) {
		RETURN_FALSE;
	}

	DBusMessageIter iter;
	dbus_message_iter_init(m, &iter);
	int type;
	int i = 0;
	array_init(return_value);
	while ((type = dbus_message_iter_get_arg_type(&iter)) != DBUS_TYPE_INVALID) {
		switch (type) {
		case DBUS_TYPE_STRING:
			{
				char *arg;
				dbus_message_iter_get_basic(&iter, &arg);
				add_index_string(return_value, i, arg, 1);
			}
			break;
		case DBUS_TYPE_INT32:
			{
				dbus_int32_t arg;
				dbus_message_iter_get_basic(&iter, &arg);
				add_index_long(return_value, i, arg);
			}
			break;
		case DBUS_TYPE_BOOLEAN:
			{
				dbus_bool_t arg;
				dbus_message_iter_get_basic(&iter, &arg);
				add_index_bool(return_value, i, arg);
			}
			break;
		case DBUS_TYPE_DOUBLE:
			{
				double arg;
				dbus_message_iter_get_basic(&iter, &arg);
				add_index_double(return_value, i, arg);
			}
			break;
		default:
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "unsupported type of argument -> skipping (type:%d, index:%d)", type, i);
			break;
		}
		dbus_message_iter_next(&iter);
		i++;
	}
}
/* }}} */

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
