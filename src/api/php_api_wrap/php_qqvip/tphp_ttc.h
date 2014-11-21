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
 * @version 1.9.0
 * @author  fishchen
 * @date    2006/07/19
 * @brief   Tencent PHP Library define.
 */


#ifndef _TPHP_TTC_H_
#define _TPHP_TTC_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* include standard header */
#ifdef __cplusplus
extern "C" {
#endif
# include "php.h"
# include "php_ini.h"
# include "ext/standard/info.h"
#ifdef __cplusplus
}
#endif


/* Version */
#define TPHP_TTC_VERSTR         "3.3.0"
#define TPHP_TTC_VERNUM         330     ///< mean version number.


#define TPHP_TTC_STRLEN_SHOR    32
#define TPHP_TTC_STRLEN_NORM    256
#define TPHP_TTC_STRLEN_LONG    1024
#define TPHP_TTC_STRLEN_TEXT    10240


/* info funciotn */
ZEND_MINFO_FUNCTION( tphp_ttc );
ZEND_MINIT_FUNCTION( tphp_ttc );


/* declaration of functions to be exported */
// tphp_ttc_gobal function
ZEND_FUNCTION( tphp_ttc_init_log );
ZEND_FUNCTION( tphp_ttc_set_log_level );
ZEND_FUNCTION( tphp_ttc_set_key_value_max );

/// tphp_ttc_server
ZEND_FUNCTION( tphp_ttc_server_new );
ZEND_FUNCTION( tphp_ttc_server_set_address );
ZEND_FUNCTION( tphp_ttc_server_set_tablename );
ZEND_FUNCTION( tphp_ttc_server_set_accesskey );
ZEND_FUNCTION( tphp_ttc_server_intkey );
ZEND_FUNCTION( tphp_ttc_server_stringkey );
ZEND_FUNCTION( tphp_ttc_server_binarykey );
ZEND_FUNCTION( tphp_ttc_server_errormessage );
ZEND_FUNCTION( tphp_ttc_server_set_timeout );
ZEND_FUNCTION( tphp_ttc_server_connect );
ZEND_FUNCTION( tphp_ttc_server_close );
ZEND_FUNCTION( tphp_ttc_server_ping );
ZEND_FUNCTION( tphp_ttc_server_autoping );
ZEND_FUNCTION( tphp_ttc_server_set_mtimeout );
ZEND_FUNCTION( tphp_ttc_server_add_key ); 

/// tphp_ttc_request
ZEND_FUNCTION( tphp_ttc_request_new );
ZEND_FUNCTION( tphp_ttc_request_reset );
ZEND_FUNCTION( tphp_ttc_request_need );

ZEND_FUNCTION( tphp_ttc_request_eq );
ZEND_FUNCTION( tphp_ttc_request_ne );
ZEND_FUNCTION( tphp_ttc_request_lt );
ZEND_FUNCTION( tphp_ttc_request_le );
ZEND_FUNCTION( tphp_ttc_request_gt );
ZEND_FUNCTION( tphp_ttc_request_ge );
ZEND_FUNCTION( tphp_ttc_request_eq_str );
ZEND_FUNCTION( tphp_ttc_request_ne_str );

ZEND_FUNCTION( tphp_ttc_request_set );
ZEND_FUNCTION( tphp_ttc_request_add );
ZEND_FUNCTION( tphp_ttc_request_sub );
ZEND_FUNCTION( tphp_ttc_request_or );
ZEND_FUNCTION( tphp_ttc_request_set_flo );
ZEND_FUNCTION( tphp_ttc_request_add_flo );
ZEND_FUNCTION( tphp_ttc_request_sub_flo );
ZEND_FUNCTION( tphp_ttc_request_set_str );
ZEND_FUNCTION( tphp_ttc_request_set_bin );
ZEND_FUNCTION( tphp_ttc_request_limit );

// for bitmap
ZEND_FUNCTION( tphp_ttc_request_set_multi_bits ); 
ZEND_FUNCTION( tphp_ttc_request_set_bit ); 
ZEND_FUNCTION( tphp_ttc_request_clear_bit ); 

ZEND_FUNCTION( tphp_ttc_request_setkey );
ZEND_FUNCTION( tphp_ttc_request_setkey_str );
ZEND_FUNCTION( tphp_ttc_request_execute );
ZEND_FUNCTION( tphp_ttc_request_nocache ); 
ZEND_FUNCTION( tphp_ttc_request_addkey_value ); 
ZEND_FUNCTION( tphp_ttc_request_addkey_value_str ); 
ZEND_FUNCTION( tphp_ttc_request_addkey_value_bin ); 

// tphp_ttc_result
ZEND_FUNCTION( tphp_ttc_result_new );
ZEND_FUNCTION( tphp_ttc_result_reset );
ZEND_FUNCTION( tphp_ttc_result_resultcode );
ZEND_FUNCTION( tphp_ttc_result_errormessage );
ZEND_FUNCTION( tphp_ttc_result_errorfrom );
ZEND_FUNCTION( tphp_ttc_result_numrows );
ZEND_FUNCTION( tphp_ttc_result_totalrows );
ZEND_FUNCTION( tphp_ttc_result_affectedrows );
ZEND_FUNCTION( tphp_ttc_result_numfields );
ZEND_FUNCTION( tphp_ttc_result_insertid );
ZEND_FUNCTION( tphp_ttc_result_fieldname ); //
ZEND_FUNCTION( tphp_ttc_result_fieldpresent ); //

ZEND_FUNCTION( tphp_ttc_result_intkey );
ZEND_FUNCTION( tphp_ttc_result_stringkey );
ZEND_FUNCTION( tphp_ttc_result_binarykey );
ZEND_FUNCTION( tphp_ttc_result_intvalue );
ZEND_FUNCTION( tphp_ttc_result_floatvalue );
ZEND_FUNCTION( tphp_ttc_result_stringvalue );
ZEND_FUNCTION( tphp_ttc_result_binaryvalue );

ZEND_FUNCTION( tphp_ttc_result_fetchrow );
ZEND_FUNCTION( tphp_ttc_result_rewind );
ZEND_FUNCTION( tphp_ttc_result_fetcharray );

#endif



