// vim: set expandtab tabstop=4 shiftwidth=4 fdm=marker:
// +----------------------------------------------------------------------+
// | Tencent PHP Library.                                                 |
// +----------------------------------------------------------------------+
// | Copyright (c) 2004-2006 Tencent Inc. All Rights Reserved.            |
// +----------------------------------------------------------------------+
// | Authors: The Club Dev Team, ISRD, Tencent Inc.                       |
// |          fishchen <fishchen@tencent.com>                             |
// +----------------------------------------------------------------------+
// $Id$

/**
 * @version $Revision$
 * @author  $Author: fishchen $
 * @date    $Date$
 * @brief   Tencent PHP Library define.
 */


#include "tphp_ttc.h"
#include "php5obj.h"
#include "ttcapi.h"

extern int le_tphp_ttc_server;


/* {{{ ZEND_FUNCTION( tphp_ttc_server_new ) */
ZEND_FUNCTION( tphp_ttc_server_new )
{
    TTC::Server *tphp_ttc_server_instance = NULL;

    tphp_ttc_server_instance = new TTC::Server();
    if ( tphp_ttc_server_instance != NULL ) {
        PHP5CPP_REGISTER_RESOURCE( tphp_ttc_server, return_value, tphp_ttc_server_instance, le_tphp_ttc_server );
    } // if
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_set_address ) */
ZEND_FUNCTION( tphp_ttc_server_set_address )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    char *ps_host = NULL;
    char *ps_port = NULL;
    long i_hostlen = 0;
    long i_portlen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                                    &ps_host, &i_hostlen,
                                    &ps_port, &i_portlen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
        i_ret = tphp_ttc_server_instance->SetAddress( ps_host, ps_port );

    } else if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                                    &ps_host, &i_hostlen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
        i_ret = tphp_ttc_server_instance->SetAddress( ps_host );

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_set_tablename ) */
ZEND_FUNCTION( tphp_ttc_server_set_tablename )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    char *ps_table  = NULL;
    long i_tablelen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                                    &ps_table, &i_tablelen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_server_instance->SetTableName( ps_table );
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_server_set_accesskey ) */
ZEND_FUNCTION( tphp_ttc_server_set_accesskey )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    char *ps_accesskey  = NULL;
    long i_accesskeylen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                                    &ps_accesskey, &i_accesskeylen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    tphp_ttc_server_instance->SetAccessKey(ps_accesskey);
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_intkey ) */
ZEND_FUNCTION( tphp_ttc_server_intkey )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_server_instance->IntKey();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_stringkey ) */
ZEND_FUNCTION( tphp_ttc_server_stringkey )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_server_instance->StringKey();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_binarykey ) */
ZEND_FUNCTION( tphp_ttc_server_binarykey )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_server_instance->BinaryKey();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_errormessage ) */
ZEND_FUNCTION( tphp_ttc_server_errormessage )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    //char s_buf[TPHP_TTC_STRLEN_LONG] = { 0 };
    const char *ps_msg = NULL;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    ps_msg = tphp_ttc_server_instance->ErrorMessage();
    //snprintf( s_buf,  sizeof(s_buf),  "%s", ps_msg );
    RETURN_STRING( (char *)ps_msg, 1 );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_set_timeout ) */
ZEND_FUNCTION( tphp_ttc_server_set_timeout )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    long i_timeout = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                                    &i_timeout) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    tphp_ttc_server_instance->SetTimeout( i_timeout );
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_server_set_mtimeout ) */
ZEND_FUNCTION( tphp_ttc_server_set_mtimeout )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    long i_timeout = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                                    &i_timeout) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    tphp_ttc_server_instance->SetMTimeout( i_timeout );
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_server_connect ) */
ZEND_FUNCTION( tphp_ttc_server_connect )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_server_instance->Connect();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_close ) */
ZEND_FUNCTION( tphp_ttc_server_close )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    tphp_ttc_server_instance->Close();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_ping ) */
ZEND_FUNCTION( tphp_ttc_server_ping )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_server_instance->Ping();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_server_autoping ) */
ZEND_FUNCTION( tphp_ttc_server_autoping )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    tphp_ttc_server_instance->AutoPing();
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_server_add_key ) */
ZEND_FUNCTION( tphp_ttc_server_add_key )
{
    TTC::Server *tphp_ttc_server_instance = NULL;
    char *key_name = NULL;
    long i_name_len = 0;
    long i_key_type = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &key_name, &i_name_len,
                                    &i_key_type) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
        i_ret = tphp_ttc_server_instance->AddKey( key_name, i_key_type );

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    RETURN_LONG( i_ret );
}
/* }}} */


