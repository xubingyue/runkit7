/*
  +----------------------------------------------------------------------+
  | PHP Version 7                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2006 The PHP Group, (c) 2008-2015 Dmitry Zenovich |
  | "runkit7" patches (c) 2015-2016 Tyson Andre                          |
  +----------------------------------------------------------------------+
  | This source file is subject to the new BSD license,                  |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.opensource.org/licenses/BSD-3-Clause                      |
  | If you did not receive a copy of the license and are unable to       |
  | obtain it through the world-wide-web, please send a note to          |
  | dzenovich@gmail.com so we can mail you a copy immediately.           |
  +----------------------------------------------------------------------+
  | Author: Sara Golemon <pollita@php.net>                               |
  | Modified by Dmitry Zenovich <dzenovich@gmail.com>                    |
  | Modified for php7 by Tyson Andre <tysonandre775@hotmail.com>         |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#include "php_runkit.h"
#include "SAPI.h"

ZEND_DECLARE_MODULE_GLOBALS(runkit)

#ifdef PHP_RUNKIT_SUPERGLOBALS
/* {{{ proto array runkit_superglobals(void)
	Return numericly indexed array of registered superglobals */
PHP_FUNCTION(runkit_superglobals)
{
	zend_string* key;

	array_init(return_value);
	ZEND_HASH_FOREACH_STR_KEY(CG(auto_globals), key) {
		if (key != NULL) {
			add_next_index_str(return_value, key);
		}
	} ZEND_HASH_FOREACH_END();
}
/* }}} */
#endif /* PHP_RUNKIT_SUPERGLOBALS */

/* {{{ proto array runkit_zval_inspect(mized var)
 */
PHP_FUNCTION(runkit_zval_inspect)
{
	// TODO: Specify what this should do for php7 (e.g. primitives are no longer refcounted)
	zval *value;
	char *addr;
	int addr_len;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &value) == FAILURE) {
		return;
	}

	array_init(return_value);

	addr_len = spprintf(&addr, 0, "0x%0lx", (long)value);
	add_assoc_stringl(return_value, "address", addr, addr_len);
	efree(addr);
	addr = NULL;

	if (Z_REFCOUNTED_P(value)) {
		add_assoc_long(return_value, "refcount", Z_REFCOUNT_P(value));
		add_assoc_bool(return_value, "is_ref", Z_ISREF_P(value));
	}

	add_assoc_long(return_value, "type", Z_TYPE_P(value));
}
/* }}} */

/* {{{ runkit_functions[]
 */
zend_function_entry runkit_functions[] = {

	PHP_FE(runkit_zval_inspect,										NULL)
	PHP_FE(runkit_object_id,										NULL)
	PHP_FE(runkit_return_value_used,								NULL)

#ifdef PHP_RUNKIT_SUPERGLOBALS
	PHP_FE(runkit_superglobals,										NULL)
#endif

#ifdef PHP_RUNKIT_MANIPULATION
#ifdef PHP_RUNKIT_MANIPULATION_CLASSES
	// PHP_FE(runkit_class_emancipate,									NULL)
	// PHP_FE(runkit_class_adopt,										NULL)
#endif
	// PHP_FE(runkit_import,											NULL)

	PHP_FE(runkit_function_add,										NULL)
	PHP_FE(runkit_function_remove,									NULL)
	PHP_FE(runkit_function_rename,									NULL)
	PHP_FE(runkit_function_redefine,								NULL)
	PHP_FE(runkit_function_copy,									NULL)

	PHP_FE(runkit_method_add,										NULL)
	PHP_FE(runkit_method_redefine,									NULL)
	PHP_FE(runkit_method_remove,									NULL)
	PHP_FE(runkit_method_rename,									NULL)
	PHP_FE(runkit_method_copy,										NULL)

#ifdef PHP_RUNKIT_CLASSKIT_COMPAT
	PHP_FALIAS(classkit_method_add,			runkit_method_add,		NULL)
	PHP_FALIAS(classkit_method_redefine,	runkit_method_redefine,	NULL)
	PHP_FALIAS(classkit_method_remove,		runkit_method_remove,	NULL)
	PHP_FALIAS(classkit_method_rename,		runkit_method_rename,	NULL)
	PHP_FALIAS(classkit_method_copy,		runkit_method_copy,		NULL)
#endif

	PHP_FE(runkit_constant_redefine,								NULL)
	PHP_FE(runkit_constant_remove,									NULL)
	PHP_FE(runkit_constant_add,										NULL)

	// PHP_FE(runkit_default_property_add,								NULL)
	// PHP_FE(runkit_default_property_remove,							NULL)
#endif /* PHP_RUNKIT_MANIPULATION */

	{NULL, NULL, NULL}
};
/* }}} */

/* {{{ runkit_module_entry
 */
zend_module_entry runkit_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"runkit",
	runkit_functions,
	PHP_MINIT(runkit),
	PHP_MSHUTDOWN(runkit),
	PHP_RINIT(runkit),
	PHP_RSHUTDOWN(runkit),
	PHP_MINFO(runkit),
#if ZEND_MODULE_API_NO >= 20010901
	PHP_RUNKIT_VERSION,
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#if defined(PHP_RUNKIT_SUPERGLOBALS) || defined(PHP_RUNKIT_MANIPULATION)
PHP_INI_BEGIN()
#ifdef PHP_RUNKIT_SUPERGLOBALS
	PHP_INI_ENTRY("runkit.superglobal", "", PHP_INI_SYSTEM|PHP_INI_PERDIR, NULL)
#endif
#ifdef PHP_RUNKIT_MANIPULATION
	STD_PHP_INI_BOOLEAN("runkit.internal_override", "0", PHP_INI_SYSTEM, OnUpdateBool, internal_override, zend_runkit_globals, runkit_globals)
#endif
PHP_INI_END()
#endif

#ifdef COMPILE_DL_RUNKIT
ZEND_GET_MODULE(runkit)
#endif

#ifdef PHP_RUNKIT_MANIPULATION
ZEND_FUNCTION(_php_runkit_removed_function) {
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "A function removed by runkit was somehow invoked");
}
ZEND_FUNCTION(_php_runkit_removed_method) {
	php_error_docref(NULL TSRMLS_CC, E_ERROR, "A method removed by runkit was somehow invoked");
}

static inline void _php_runkit_init_stub_function(const char *name, void (*handler)(INTERNAL_FUNCTION_PARAMETERS), zend_function **result) {
	*result = pemalloc(sizeof(zend_function), 1);
	(*result)->common.function_name = zend_string_init(name, strlen(name), 1);  // TODO: Can this be persistent?
	(*result)->common.scope = NULL;
	(*result)->common.arg_info = NULL;
	(*result)->common.num_args = 0;
	(*result)->common.type = ZEND_INTERNAL_FUNCTION;
	(*result)->common.fn_flags = ZEND_ACC_PUBLIC | ZEND_ACC_STATIC;
	(*result)->common.arg_info = NULL;
	(*result)->internal_function.handler = handler;
	(*result)->internal_function.module = &runkit_module_entry;
}
#endif

#if defined(PHP_RUNKIT_MANIPULATION)
static void php_runkit_globals_ctor(void *pDest TSRMLS_DC)
{
	zend_runkit_globals *runkit_global = (zend_runkit_globals *) pDest;
#ifdef PHP_RUNKIT_MANIPULATION
	runkit_global->replaced_internal_functions = NULL;
	runkit_global->misplaced_internal_functions = NULL;
	// runkit_global->removed_default_class_members = NULL;
	runkit_global->name_str = "name";
	runkit_global->removed_method_str = "__method_removed_by_runkit__";
	runkit_global->removed_function_str = "__function_removed_by_runkit__";
	runkit_global->removed_parameter_str = "__parameter_removed_by_runkit__";

	_php_runkit_init_stub_function("__function_removed_by_runkit__", ZEND_FN(_php_runkit_removed_function), &runkit_global->removed_function);
	_php_runkit_init_stub_function("__method_removed_by_runkit__", ZEND_FN(_php_runkit_removed_method), &runkit_global->removed_method);
#endif
}
#endif

#define php_runkit_feature_constant(feature, enabled) \
		_php_runkit_feature_constant("RUNKIT_FEATURE_" #feature, sizeof("RUNKIT_FEATURE_" #feature), (enabled), \
									CONST_CS | CONST_PERSISTENT, module_number TSRMLS_CC)
static void _php_runkit_feature_constant(const char *name, size_t name_len, zend_bool enabled,
									int flags, int module_number TSRMLS_DC)
{
	zend_constant c;

	ZVAL_BOOL(&(c.value), enabled);
	c.flags = flags;
	c.name = zend_string_init(name, name_len - 1, 1);  // TODO: can this be persistent?
	c.module_number = module_number;
	zend_register_constant(&c TSRMLS_CC);
}


/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(runkit)
{
#ifdef ZTS
#if defined(PHP_RUNKIT_MANIPULATION)
	ts_allocate_id(&runkit_globals_id, sizeof(zend_runkit_globals), php_runkit_globals_ctor, NULL);
#endif
#else
	php_runkit_globals_ctor(&runkit_globals);
#endif

#if defined(PHP_RUNKIT_SUPERGLOBALS) || defined(PHP_RUNKIT_MANIPULATION)
	REGISTER_INI_ENTRIES();
#endif

	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_FUNCTIONS",          PHP_RUNKIT_IMPORT_FUNCTIONS,             CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_CLASS_METHODS",      PHP_RUNKIT_IMPORT_CLASS_METHODS,         CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_CLASS_CONSTS",       PHP_RUNKIT_IMPORT_CLASS_CONSTS,          CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_CLASS_PROPS",        PHP_RUNKIT_IMPORT_CLASS_PROPS,           CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_CLASS_STATIC_PROPS", PHP_RUNKIT_IMPORT_CLASS_STATIC_PROPS,    CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_CLASSES",            PHP_RUNKIT_IMPORT_CLASSES,               CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_IMPORT_OVERRIDE",           PHP_RUNKIT_IMPORT_OVERRIDE,              CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("RUNKIT_VERSION",                 PHP_RUNKIT_VERSION,                      CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("RUNKIT_ACC_RETURN_REFERENCE",      PHP_RUNKIT_ACC_RETURN_REFERENCE,         CONST_CS | CONST_PERSISTENT);

	REGISTER_LONG_CONSTANT("RUNKIT_ACC_PUBLIC",                ZEND_ACC_PUBLIC,                         CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_ACC_PROTECTED",             ZEND_ACC_PROTECTED,                      CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_ACC_PRIVATE",               ZEND_ACC_PRIVATE,                        CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_ACC_STATIC",                ZEND_ACC_STATIC,                         CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("RUNKIT_OVERRIDE_OBJECTS",          PHP_RUNKIT_OVERRIDE_OBJECTS,             CONST_CS | CONST_PERSISTENT);

#ifdef PHP_RUNKIT_CLASSKIT_COMPAT
	REGISTER_LONG_CONSTANT("CLASSKIT_ACC_PUBLIC",              ZEND_ACC_PUBLIC,                         CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("CLASSKIT_ACC_PROTECTED",           ZEND_ACC_PROTECTED,                      CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("CLASSKIT_ACC_PRIVATE",             ZEND_ACC_PRIVATE,                        CONST_CS | CONST_PERSISTENT);
	REGISTER_STRING_CONSTANT("CLASSKIT_VERSION",               PHP_RUNKIT_VERSION,                      CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("CLASSKIT_AGGREGATE_OVERRIDE",      PHP_RUNKIT_IMPORT_OVERRIDE,              CONST_CS | CONST_PERSISTENT);
#endif

	/* Feature Identifying constants */
#ifdef PHP_RUNKIT_MANIPULATION
	php_runkit_feature_constant(MANIPULATION, 1);
#else
	php_runkit_feature_constant(MANIPULATION, 0);
#endif
#ifdef PHP_RUNKIT_SUPERGLOBALS
	php_runkit_feature_constant(SUPERGLOBALS, 1);
#else
	php_runkit_feature_constant(SUPERGLOBALS, 0);
#endif

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(runkit)
{
#ifdef PHP_RUNKIT_MANIPULATION
	pefree(RUNKIT_G(removed_function), 1);
	pefree(RUNKIT_G(removed_method), 1);
#endif
#if defined(PHP_RUNKIT_SUPERGLOBALS) || defined(PHP_RUNKIT_MANIPULATION)
	UNREGISTER_INI_ENTRIES();
#endif

	return SUCCESS;
}
/* }}} */

#ifdef PHP_RUNKIT_SUPERGLOBALS
/* {{{ php_runkit_register_auto_global
	Register an autoglobal only if it's not already registered */
static void php_runkit_register_auto_global(char *s, int len TSRMLS_DC)
{
	zend_auto_global *auto_global;
	zend_string* globalName;
	zval z;

	if (zend_hash_str_exists(CG(auto_globals), s, len + 1)) {
		/* Registered already */
		return;
	}

	globalName = zend_string_init(s, len, 0);
	if (zend_register_auto_global(globalName,
			0,
		    NULL TSRMLS_CC) == SUCCESS) {
		// FIXME: This is broken.

		// TODO: How do I get an auto global out of a zend_hash????
		if ((auto_global = zend_hash_find_ptr(CG(auto_globals), globalName)) == NULL) {
			php_error_docref(NULL TSRMLS_CC, E_ERROR, "Cannot locate the newly created autoglobal");
			return;
		}
		auto_global->armed = 0;

		if (!RUNKIT_G(superglobals)) {
			ALLOC_HASHTABLE(RUNKIT_G(superglobals));
			zend_hash_init(RUNKIT_G(superglobals), 2, NULL, NULL, 0);
		}
		// TODO: destructor calls?
		ZVAL_NEW_STR(&z, globalName);
		// This seems like it was a bug in the original function? They tried to insert a raw string?
		zend_hash_next_index_insert(RUNKIT_G(superglobals), &z);
	}
}
/* }}} */

/* {{{ php_runkit_rinit_register_superglobals */
static void php_runkit_rinit_register_superglobals(const char *ini_s) {
	char *s;
	char *p;
	char *dup;
	int len;

	RUNKIT_G(superglobals) = NULL;
	if (!ini_s || !strlen(ini_s)) {
		return;
	}

	s = dup = estrdup(ini_s);
	p = strchr(s, ',');
	while(p) {
		if (p - s) {
			*p = '\0';
			php_runkit_register_auto_global(s, p - s TSRMLS_CC);
		}
		s = ++p;
		p = strchr(p, ',');
	}

	len = strlen(s);
	if (len) {
		php_runkit_register_auto_global(s, len TSRMLS_CC);
	}
	efree(dup);
}
/* }}} */
#endif /* PHP_RUNKIT_SUPERGLOBALS */

/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(runkit)
{
#ifdef PHP_RUNKIT_SUPERGLOBALS
	php_runkit_rinit_register_superglobals(INI_STR("runkit.superglobal"));
#endif /* PHP_RUNKIT_SUPERGLOBALS */

#ifdef PHP_RUNKIT_MANIPULATION
	RUNKIT_G(replaced_internal_functions) = NULL;
	RUNKIT_G(misplaced_internal_functions) = NULL;
	// RUNKIT_G(removed_default_class_members) = NULL;
#endif

	return SUCCESS;
}
/* }}} */

#ifdef PHP_RUNKIT_SUPERGLOBALS
/* {{{ php_runkit_superglobal_dtor */
static int php_runkit_superglobal_dtor(zval *zv TSRMLS_DC)
{
	ZEND_ASSERT(Z_TYPE_P(zv) == IS_STRING);
	zend_hash_del(CG(auto_globals), Z_STR_P(zv));

	return ZEND_HASH_APPLY_REMOVE;
}
/* }}} */
#endif /* PHP_RUNKIT_SUPERGLOBALS */

/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(runkit)
{
#ifdef PHP_RUNKIT_MANIPULATION
	// php_runkit_default_class_members_list_element *el;
#endif

#ifdef PHP_RUNKIT_SUPERGLOBALS
	if (RUNKIT_G(superglobals)) {
		zend_hash_apply(RUNKIT_G(superglobals), php_runkit_superglobal_dtor TSRMLS_CC);

		zend_hash_destroy(RUNKIT_G(superglobals));
		FREE_HASHTABLE(RUNKIT_G(superglobals));
	}
#endif /* PHP_RUNKIT_SUPERGLOBALS */

#ifdef PHP_RUNKIT_MANIPULATION
	if (RUNKIT_G(misplaced_internal_functions)) {
		/* Just wipe out rename-to targets before restoring originals */
		zend_hash_apply(RUNKIT_G(misplaced_internal_functions), php_runkit_destroy_misplaced_functions TSRMLS_CC);
		zend_hash_destroy(RUNKIT_G(misplaced_internal_functions));
		FREE_HASHTABLE(RUNKIT_G(misplaced_internal_functions));
		RUNKIT_G(misplaced_internal_functions) = NULL;
	}

	if (RUNKIT_G(replaced_internal_functions)) {
		/* Restore internal functions */
		zend_function *f;
		zend_string *key;
		ZEND_HASH_FOREACH_STR_KEY_PTR(RUNKIT_G(replaced_internal_functions), key, f) {
			if (key != NULL) {
				ZEND_ASSERT(f->type == ZEND_INTERNAL_FUNCTION || f->type == ZEND_USER_FUNCTION);
				php_runkit_restore_internal_function(key, f);
			}
		} ZEND_HASH_FOREACH_END();
		zend_hash_destroy(RUNKIT_G(replaced_internal_functions));
		FREE_HASHTABLE(RUNKIT_G(replaced_internal_functions));
		RUNKIT_G(replaced_internal_functions) = NULL;
	}

	// This would restore default properties that were removed by runkit_default_property_remove(...)

	/*
	el = RUNKIT_G(removed_default_class_members);
	// TODO: I have no idea what this is trying to do.
	while (el) {
		php_runkit_default_class_members_list_element *tmp;
		// TODO: Some sort of cleanup?
		zval **table = el->is_static ? el->ce->default_static_members_table : el->ce->default_properties_table;
		zval **table_el = &table[el->offset];
		if ( *table_el == NULL ) {
			ALLOC_ZVAL(*table_el);
			Z_TYPE_PP(table_el) = IS_NULL;
			Z_SET_REFCOUNT_PP(table_el, 1);
		}
		tmp = el;
		el = el->next;
		efree(tmp);
	}
	*/
#endif /* PHP_RUNKIT_MANIPULATION */

	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(runkit)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "runkit support", "enabled");
	php_info_print_table_header(2, "version", PHP_RUNKIT_VERSION);

	if (sapi_module.phpinfo_as_text) {
	    php_info_print_table_row(2, "Donate Runkit", "https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=P2WY8LBB2YGMQ");
	} else {
		php_printf("<tr>"
		              "<td colspan=\"2\" class=\"v\">"
		                  "<strong style=\"font-size:300%%; color:#8822ff; font-style:italic\">&iexcl;Runkit needs your help!</strong>"
		                  "<a href=\"https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&amp;hosted_button_id=P2WY8LBB2YGMQ\" target=\"_blank\">"
		                      "<img src=\"https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif\" alt=\"Donate Runkit via PayPal\" style=\"max-width:100%%;\">"
		                  "</a>"
		              "</td>"
		           "</tr>");
	}

#ifdef PHP_RUNKIT_CLASSKIT_COMPAT
	php_info_print_table_header(2, "classkit compatability", "enabled");
#endif

#ifdef PHP_RUNKIT_SUPERGLOBALS
	php_info_print_table_header(2, "Custom Superglobal support", "enabled");
#else
	php_info_print_table_header(2, "Custom Superglobal support", "disabled or unavailable");
#endif /* PHP_RUNKIT_SUPERGLOBALS */

	php_info_print_table_header(2, "Sandbox Support", "disabled for php7");

#ifdef PHP_RUNKIT_MANIPULATION
	php_info_print_table_header(2, "Runtime Manipulation", "enabled");
#else
	php_info_print_table_header(2, "Runtime Manipulation", "disabled or unavailable");
#endif /* PHP_RUNKIT_MANIPULATION */

	php_info_print_table_end();

#if defined(PHP_RUNKIT_SUPERGLOBALS) || defined(PHP_RUNKIT_MANIPULATION)
	DISPLAY_INI_ENTRIES();
#endif

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
