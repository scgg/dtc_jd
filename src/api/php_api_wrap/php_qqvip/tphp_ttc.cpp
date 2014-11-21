// vim: set expandtab tabstop=4 shiftwidth=4 fdm=marker:
// +----------------------------------------------------------------------+
// | Tencent PHP Library.                                                 |
// +----------------------------------------------------------------------+
// | Copyright (c) 2004-2007 Tencent Inc. All Rights Reserved.            |
// +----------------------------------------------------------------------+
// | Authors: The Club Dev Team, ISRD, Tencent Inc.                       |
// |          fishchen <fishchen@tencent.com>                             |
// +----------------------------------------------------------------------+
// $Id$

/**
 * @version $Revision$
 * @author  $Author: fishchen $
 * @date    $Date$
 * @brief   Tencent PHP Library.
 */


#include "tphp_ttc.h"
#include "php5obj.h"
#include "ttcapi.h"

/* compiled function list so Zend knows what's in this module */
zend_function_entry tphp_ttc_functions[] =
{
    //ZEND_FE( tphp_msglog,               NULL )
    //ZEND_FE( tphp_ns_qry_name,          NULL )
	ZEND_FE( tphp_ttc_init_log, NULL)
	ZEND_FE( tphp_ttc_set_log_level, NULL)
	ZEND_FE( tphp_ttc_set_key_value_max, NULL)
    { NULL, NULL, NULL }
};


/* compiled module information */
zend_module_entry tphp_ttc_module_entry =
{
    STANDARD_MODULE_HEADER,
    "tphp_dtc",
    tphp_ttc_functions,
    ZEND_MINIT( tphp_ttc ),
    NULL, 
    NULL, 
    NULL, 
    ZEND_MINFO( tphp_ttc ),
    TPHP_TTC_VERSTR,
    STANDARD_MODULE_PROPERTIES
};

/* implement standard "stub" routine to introduce ourselves to Zend */
#ifdef __cplusplus
BEGIN_EXTERN_C()
#endif
ZEND_GET_MODULE( tphp_ttc )
#ifdef __cplusplus
END_EXTERN_C()
#endif


/* {{{ ZEND_MINFO_FUNCTION( tphp_ttc ) */
/**
 * Set print info where use phpinfo().
 */
ZEND_MINFO_FUNCTION( tphp_ttc )
{
    php_info_print_table_start();
    php_info_print_table_colspan_header( 2,     "Tencent PHP Library - ttc" );
    php_info_print_table_row( 2, "Version",     TPHP_TTC_VERSTR );
    php_info_print_table_row( 2, "Anthor",      "The Club Dev Team, ISRD, Tencent Inc." );
    php_info_print_table_row( 2, "Copyright",   "Copyright (c) 2007 Tencent Inc. All Rights Reserved." );
    php_info_print_table_end();

    php_info_print_table_start();
    php_info_print_table_header( 2, "Library",  "Version" );
    php_info_print_table_row( 2, "ttc_api",     TPHP_TTC_VERSTR );
    php_info_print_table_end();

    php_info_print_table_start();
    php_info_print_table_colspan_header( 2,     "Build Environment" );
    php_info_print_table_row( 2, "PHP",         PHP_VERSION );
    php_info_print_table_end();
}
/* }}} */


/* {{{ tphp_ttc_server */
static ZEND_RSRC_DTOR_FUNC( destroy_tphp_ttc_server )
{
    if ( rsrc->ptr ) {
        delete (TTC::Server *)rsrc->ptr;
        rsrc->ptr = NULL;
    }
}

ZEND_FUNCTION( tphp_ttc_init_log )
{
	int ret;
	char* app;
	int app_len;
	char* dir;
	int dir_len;

	if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss", &app, &app_len, &dir, &dir_len) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
		TTC::init_log(app, dir);
		printf("init_log(%s,%s)\n", app, dir);
		ret = 0;
    } else if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &app, &app_len) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
		TTC::init_log(app);
		ret = 0;
    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */
	
	RETURN_LONG( ret );
}

ZEND_FUNCTION( tphp_ttc_set_log_level )
{
	long level;
	
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &level) == FAILURE){
		WRONG_PARAM_COUNT;
        RETURN_LONG(-1);
    }
	TTC::set_log_level((int)level);
	printf("set_log_level(%ld)\n", level);
	RETURN_LONG( 0 );
}

ZEND_FUNCTION( tphp_ttc_set_key_value_max )
{
	int key_max;
	
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "d", &key_max) == FAILURE){
		WRONG_PARAM_COUNT;
        RETURN_LONG(-1);
    }
	
	RETURN_LONG( TTC::set_key_value_max(key_max) );
}


zend_function_entry php5cpp_tphp_ttc_server_methods[] = {
#if PHP_VERSION_ID >= 50200
    ZEND_ME_MAPPING( tphp_ttc_server,   tphp_ttc_server_new,            NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set_address,       tphp_ttc_server_set_address,    NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set_tablename,     tphp_ttc_server_set_tablename,  NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set_accesskey,     tphp_ttc_server_set_accesskey,  NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( int_key,           tphp_ttc_server_intkey,         NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( string_key,        tphp_ttc_server_stringkey,      NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( binary_key,        tphp_ttc_server_binarykey,      NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( error_message,     tphp_ttc_server_errormessage,   NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set_timeout,       tphp_ttc_server_set_timeout,    NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( connect,           tphp_ttc_server_connect,        NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( close,             tphp_ttc_server_close,          NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( ping,              tphp_ttc_server_ping,           NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( autoping,          tphp_ttc_server_autoping,       NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( set_mtimeout,      tphp_ttc_server_set_mtimeout,   NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( add_key,           tphp_ttc_server_add_key,        NULL, ZEND_ACC_PUBLIC )
    { NULL, NULL, NULL, 0 }
#else
    ZEND_ME_MAPPING( tphp_ttc_server,   tphp_ttc_server_new,            NULL )
    ZEND_ME_MAPPING( set_address,       tphp_ttc_server_set_address,    NULL )
    ZEND_ME_MAPPING( set_tablename,     tphp_ttc_server_set_tablename,  NULL )
    ZEND_ME_MAPPING( set_accesskey,     tphp_ttc_server_set_accesskey,  NULL )
    ZEND_ME_MAPPING( int_key,           tphp_ttc_server_intkey,         NULL )
    ZEND_ME_MAPPING( string_key,        tphp_ttc_server_stringkey,      NULL )
    ZEND_ME_MAPPING( binary_key,        tphp_ttc_server_binarykey,      NULL )
    ZEND_ME_MAPPING( error_message,     tphp_ttc_server_errormessage,   NULL )
    ZEND_ME_MAPPING( set_timeout,       tphp_ttc_server_set_timeout,    NULL )
    ZEND_ME_MAPPING( connect,           tphp_ttc_server_connect,        NULL )
    ZEND_ME_MAPPING( close,             tphp_ttc_server_close,          NULL )
    ZEND_ME_MAPPING( ping,              tphp_ttc_server_ping,           NULL )
    ZEND_ME_MAPPING( autoping,          tphp_ttc_server_autoping,       NULL )
	ZEND_ME_MAPPING( set_mtimeout,      tphp_ttc_server_set_mtimeout,   NULL )
	ZEND_ME_MAPPING( add_key,           tphp_ttc_server_add_key,        NULL )
    { NULL, NULL, NULL }
#endif
};


int le_tphp_ttc_server;
zend_class_entry *php5cpp_ce_tphp_ttc_server;
static zend_object_handlers php5cpp_object_handlers_tphp_ttc_server;


PHP5CPP_OBJ_FREE_FUNCTION( tphp_ttc_server )
PHP5CPP_OBJ_NEW_FUNCTION(  tphp_ttc_server )
/* }}} */


/* {{{ tphp_ttc_request */
static ZEND_RSRC_DTOR_FUNC( destroy_tphp_ttc_request )
{
    if ( rsrc->ptr ) {
        delete (TTC::Request *)rsrc->ptr;
        rsrc->ptr = NULL;
    }
}


zend_function_entry php5cpp_tphp_ttc_request_methods[] = {
#if PHP_VERSION_ID >= 50200
    ZEND_ME_MAPPING( tphp_ttc_request,  tphp_ttc_request_new,                NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( reset,             tphp_ttc_request_reset,              NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( need,              tphp_ttc_request_need,               NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( eq,                tphp_ttc_request_eq,                 NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( ne,                tphp_ttc_request_ne,                 NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( lt,                tphp_ttc_request_lt,                 NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( le,                tphp_ttc_request_le,                 NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( gt,                tphp_ttc_request_gt,                 NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( ge,                tphp_ttc_request_ge,                 NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( eq_str,            tphp_ttc_request_eq_str,             NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( ne_str,            tphp_ttc_request_ne_str,             NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set,               tphp_ttc_request_set,                NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( add,               tphp_ttc_request_add,                NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( sub,               tphp_ttc_request_sub,                NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( or,                tphp_ttc_request_or,                 NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set_flo,           tphp_ttc_request_set_flo,            NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( add_flo,           tphp_ttc_request_add_flo,            NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( sub_flo,           tphp_ttc_request_sub_flo,            NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set_str,           tphp_ttc_request_set_str,            NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set_bin,           tphp_ttc_request_set_bin,            NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( limit,             tphp_ttc_request_limit,              NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set_key,           tphp_ttc_request_setkey,             NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( set_key_str,       tphp_ttc_request_setkey_str,         NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( execute,           tphp_ttc_request_execute,            NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( set_multi_bits,    tphp_ttc_request_set_multi_bits,     NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( set_bit,           tphp_ttc_request_set_bit,            NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( clear_bit,         tphp_ttc_request_clear_bit,          NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( nocache,           tphp_ttc_request_nocache,            NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( addkey_value,      tphp_ttc_request_addkey_value,       NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( addkey_value_str,  tphp_ttc_request_addkey_value_str,   NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( addkey_value_bin,  tphp_ttc_request_addkey_value_bin,   NULL, ZEND_ACC_PUBLIC )
    { NULL, NULL, NULL, 0 }
#else
    ZEND_ME_MAPPING( tphp_ttc_request,  tphp_ttc_request_new,                NULL )
    ZEND_ME_MAPPING( reset,             tphp_ttc_request_reset,              NULL )
    ZEND_ME_MAPPING( need,              tphp_ttc_request_need,               NULL )
    ZEND_ME_MAPPING( eq,                tphp_ttc_request_eq,                 NULL )
    ZEND_ME_MAPPING( ne,                tphp_ttc_request_ne,                 NULL )
    ZEND_ME_MAPPING( lt,                tphp_ttc_request_lt,                 NULL )
    ZEND_ME_MAPPING( le,                tphp_ttc_request_le,                 NULL )
    ZEND_ME_MAPPING( gt,                tphp_ttc_request_gt,                 NULL )
    ZEND_ME_MAPPING( ge,                tphp_ttc_request_ge,                 NULL )
    ZEND_ME_MAPPING( eq_str,            tphp_ttc_request_eq_str,             NULL )
    ZEND_ME_MAPPING( ne_str,            tphp_ttc_request_ne_str,             NULL )
    ZEND_ME_MAPPING( set,               tphp_ttc_request_set,                NULL )
    ZEND_ME_MAPPING( add,               tphp_ttc_request_add,                NULL )
    ZEND_ME_MAPPING( sub,               tphp_ttc_request_sub,                NULL )
    ZEND_ME_MAPPING( or,                tphp_ttc_request_or,                 NULL )
    ZEND_ME_MAPPING( set_flo,           tphp_ttc_request_set_flo,            NULL )
    ZEND_ME_MAPPING( add_flo,           tphp_ttc_request_add_flo,            NULL )
    ZEND_ME_MAPPING( sub_flo,           tphp_ttc_request_sub_flo,            NULL )
    ZEND_ME_MAPPING( set_str,           tphp_ttc_request_set_str,            NULL )
    ZEND_ME_MAPPING( set_bin,           tphp_ttc_request_set_bin,            NULL )
    ZEND_ME_MAPPING( limit,             tphp_ttc_request_limit,              NULL )
    ZEND_ME_MAPPING( set_key,           tphp_ttc_request_setkey,             NULL )
    ZEND_ME_MAPPING( set_key_str,       tphp_ttc_request_setkey_str,         NULL )
    ZEND_ME_MAPPING( execute,           tphp_ttc_request_execute,            NULL )
	ZEND_ME_MAPPING( set_multi_bits,    tphp_ttc_request_set_multi_bits,     NULL )
	ZEND_ME_MAPPING( set_bit,           tphp_ttc_request_set_bit,            NULL )
	ZEND_ME_MAPPING( clear_bit,         tphp_ttc_request_clear_bit,          NULL )
	ZEND_ME_MAPPING( nocache,           tphp_ttc_request_nocache,            NULL )
	ZEND_ME_MAPPING( addkey_value,      tphp_ttc_request_addkey_value,       NULL )
	ZEND_ME_MAPPING( addkey_value_str,  tphp_ttc_request_addkey_value_str,   NULL )
	ZEND_ME_MAPPING( addkey_value_bin,  tphp_ttc_request_addkey_value_bin,   NULL )
    { NULL, NULL, NULL }
#endif
};


int le_tphp_ttc_request;
zend_class_entry *php5cpp_ce_tphp_ttc_request;
static zend_object_handlers php5cpp_object_handlers_tphp_ttc_request;


PHP5CPP_OBJ_FREE_FUNCTION( tphp_ttc_request )
PHP5CPP_OBJ_NEW_FUNCTION(  tphp_ttc_request )
/* }}} */


/* {{{ tphp_ttc_result */
static ZEND_RSRC_DTOR_FUNC( destroy_tphp_ttc_result )
{
    if ( rsrc->ptr ) {
        delete (TTC::Result *)rsrc->ptr;
        rsrc->ptr = NULL;
    }
}


zend_function_entry php5cpp_tphp_ttc_result_methods[] = {
#if PHP_VERSION_ID >= 50200
    ZEND_ME_MAPPING( tphp_ttc_result,   tphp_ttc_result_new,            NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( reset,             tphp_ttc_result_reset,          NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( result_code,       tphp_ttc_result_resultcode,     NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( error_message,     tphp_ttc_result_errormessage,   NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( error_from,        tphp_ttc_result_errorfrom,      NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( num_rows,          tphp_ttc_result_numrows,        NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( affected_rows,     tphp_ttc_result_affectedrows,   NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( total_rows,        tphp_ttc_result_totalrows,      NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( num_fields,        tphp_ttc_result_numfields,      NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( insert_id,         tphp_ttc_result_insertid,       NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( int_key,           tphp_ttc_result_intkey,         NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( string_key,        tphp_ttc_result_stringkey,      NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( binary_key,        tphp_ttc_result_binarykey,      NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( int_value,         tphp_ttc_result_intvalue,       NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( float_value,       tphp_ttc_result_floatvalue,     NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( string_value,      tphp_ttc_result_stringvalue,    NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( binary_value,      tphp_ttc_result_binaryvalue,    NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( fetch_row,         tphp_ttc_result_fetchrow,       NULL, ZEND_ACC_PUBLIC )
    ZEND_ME_MAPPING( rewind,            tphp_ttc_result_rewind,         NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( field_name,        tphp_ttc_result_fieldname,      NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( field_present,     tphp_ttc_result_fieldpresent,   NULL, ZEND_ACC_PUBLIC )
	ZEND_ME_MAPPING( fetch_array,       tphp_ttc_result_fetcharray,     NULL, ZEND_ACC_PUBLIC )
    { NULL, NULL, NULL, 0 }
#else
    ZEND_ME_MAPPING( tphp_ttc_result,   tphp_ttc_result_new,            NULL )
    ZEND_ME_MAPPING( reset,             tphp_ttc_result_reset,          NULL )
    ZEND_ME_MAPPING( result_code,       tphp_ttc_result_resultcode,     NULL )
    ZEND_ME_MAPPING( error_message,     tphp_ttc_result_errormessage,   NULL )
    ZEND_ME_MAPPING( error_from,        tphp_ttc_result_errorfrom,      NULL )
    ZEND_ME_MAPPING( num_rows,          tphp_ttc_result_numrows,        NULL )
    ZEND_ME_MAPPING( affected_rows,     tphp_ttc_result_affectedrows,   NULL )
    ZEND_ME_MAPPING( total_rows,        tphp_ttc_result_totalrows,      NULL )
    ZEND_ME_MAPPING( num_fields,        tphp_ttc_result_numfields,      NULL )
    ZEND_ME_MAPPING( insert_id,         tphp_ttc_result_insertid,       NULL )
    ZEND_ME_MAPPING( int_key,           tphp_ttc_result_intkey,         NULL )
    ZEND_ME_MAPPING( string_key,        tphp_ttc_result_stringkey,      NULL )
    ZEND_ME_MAPPING( binary_key,        tphp_ttc_result_binarykey,      NULL )
    ZEND_ME_MAPPING( int_value,         tphp_ttc_result_intvalue,       NULL )
    ZEND_ME_MAPPING( float_value,       tphp_ttc_result_floatvalue,     NULL )
    ZEND_ME_MAPPING( string_value,      tphp_ttc_result_stringvalue,    NULL )
    ZEND_ME_MAPPING( binary_value,      tphp_ttc_result_binaryvalue,    NULL )
    ZEND_ME_MAPPING( fetch_row,         tphp_ttc_result_fetchrow,       NULL )
    ZEND_ME_MAPPING( rewind,            tphp_ttc_result_rewind,         NULL )
	ZEND_ME_MAPPING( field_name,        tphp_ttc_result_fieldname,      NULL )
	ZEND_ME_MAPPING( field_present,     tphp_ttc_result_fieldpresent,   NULL )
	ZEND_ME_MAPPING( fetch_array,       tphp_ttc_result_fetcharray,     NULL )
    { NULL, NULL, NULL }
#endif
};


int le_tphp_ttc_result;
zend_class_entry *php5cpp_ce_tphp_ttc_result;
static zend_object_handlers php5cpp_object_handlers_tphp_ttc_result;


PHP5CPP_OBJ_FREE_FUNCTION( tphp_ttc_result )
PHP5CPP_OBJ_NEW_FUNCTION(  tphp_ttc_result )
/* }}} */


/* {{{ ZEND_MINIT_FUNCTION */
ZEND_MINIT_FUNCTION( tphp_ttc )
{
    PHP5CPP_REGISTER_CLASS( tphp_ttc_server,  "tphp_ttc_server" );
    le_tphp_ttc_server  = zend_register_list_destructors_ex( destroy_tphp_ttc_server,  NULL, "tphp_ttc_server",  module_number );
    PHP5CPP_REGISTER_CLASS( tphp_ttc_request, "tphp_ttc_request" );
    le_tphp_ttc_request = zend_register_list_destructors_ex( destroy_tphp_ttc_request, NULL, "tphp_ttc_request", module_number );
    PHP5CPP_REGISTER_CLASS( tphp_ttc_result,  "tphp_ttc_result" );
    le_tphp_ttc_result  = zend_register_list_destructors_ex( destroy_tphp_ttc_result,  NULL, "tphp_ttc_result",  module_number );
    return( SUCCESS );
}
/* }}} */



