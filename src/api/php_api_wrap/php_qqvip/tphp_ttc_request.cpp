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

extern int le_tphp_ttc_request;


/* {{{ ZEND_FUNCTION( tphp_ttc_request_new ) */
ZEND_FUNCTION( tphp_ttc_request_new )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    TTC::Server  *tphp_ttc_server_instance  = NULL;
    zval *po_server;
    long i_op = TTC::RequestGet;

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ol",
                                    &po_server,
                                    &i_op) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_NULL();
        } /* if */
    } else {
        WRONG_PARAM_COUNT;
        RETURN_NULL();
    } /* if ( ZEND_NUM_ARGS() == x ) */

    PHP5CPP_GET_OBJ_ZVAL( tphp_ttc_server, po_server )
    if ( tphp_ttc_server_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    tphp_ttc_request_instance = new TTC::Request( tphp_ttc_server_instance, i_op );
    if ( tphp_ttc_request_instance != NULL ) {
        PHP5CPP_REGISTER_RESOURCE( tphp_ttc_request, return_value, tphp_ttc_request_instance, le_tphp_ttc_request );
    } // if
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_reset ) */
ZEND_FUNCTION( tphp_ttc_request_reset )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    tphp_ttc_request_instance->Reset();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_need ) */
ZEND_FUNCTION( tphp_ttc_request_need )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                                    &ps_field, &i_fieldlen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->Need( ps_field );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_eq ) */
ZEND_FUNCTION( tphp_ttc_request_eq )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->EQ( ps_field, i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_ne ) */
ZEND_FUNCTION( tphp_ttc_request_ne )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->NE( ps_field, i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_lt ) */
ZEND_FUNCTION( tphp_ttc_request_lt )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->LT( ps_field, i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_le ) */
ZEND_FUNCTION( tphp_ttc_request_le )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->LE( ps_field, i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_gt ) */
ZEND_FUNCTION( tphp_ttc_request_gt )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->GT( ps_field, i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_ge ) */
ZEND_FUNCTION( tphp_ttc_request_ge )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->GE( ps_field, i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_eqi_str ) */
ZEND_FUNCTION( tphp_ttc_request_eq_str )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    char *ps_value  = NULL;
    long i_fieldlen = 0;
    long i_valuelen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                                    &ps_field, &i_fieldlen,
                                    &ps_value, &i_valuelen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->EQ( ps_field, ps_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_ne_str ) */
ZEND_FUNCTION( tphp_ttc_request_ne_str )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    char *ps_value  = NULL;
    long i_fieldlen = 0;
    long i_valuelen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                                    &ps_field, &i_fieldlen,
                                    &ps_value, &i_valuelen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->NE( ps_field, ps_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_set ) */
ZEND_FUNCTION( tphp_ttc_request_set )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->Set( ps_field, (int64_t)i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_add ) */
ZEND_FUNCTION( tphp_ttc_request_add )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->Add( ps_field, (int64_t)i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_sub ) */
ZEND_FUNCTION( tphp_ttc_request_sub )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->Sub( ps_field, (int64_t)i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_sub ) */
ZEND_FUNCTION( tphp_ttc_request_or )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_field, &i_fieldlen,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->OR( ps_field, (int64_t)i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_set_flo ) */
ZEND_FUNCTION( tphp_ttc_request_set_flo )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    double d_value  = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sd",
                                    &ps_field, &i_fieldlen,
                                    &d_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->Set( ps_field, d_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_add_flo ) */
ZEND_FUNCTION( tphp_ttc_request_add_flo )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    double d_value  = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sd",
                                    &ps_field, &i_fieldlen,
                                    &d_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->Add( ps_field, d_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_sub_flo ) */
ZEND_FUNCTION( tphp_ttc_request_sub_flo )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    double d_value  = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sd",
                                    &ps_field, &i_fieldlen,
                                    &d_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->Sub( ps_field, d_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_set_str ) */
ZEND_FUNCTION( tphp_ttc_request_set_str )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    char *ps_value  = NULL;
    long i_fieldlen = 0;
    long i_valuelen = 0;
    long i_ret = 0;
    //char *ps_buf = NULL;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                                    &ps_field, &i_fieldlen,
                                    &ps_value, &i_valuelen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    //snprintf( s_buf1, sizeof(s_buf1), "%s", ps_field );
    //ps_buf = (char *)malloc( i_valuelen + 1 );
    //snprintf( ps_buf, i_valuelen+1, "%s", ps_value );
    //i_ret = tphp_ttc_request_instance->Set( ps_field, ps_buf );
    i_ret = tphp_ttc_request_instance->Set( ps_field, (const char *)(ps_value) );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_set_bin ) */
ZEND_FUNCTION( tphp_ttc_request_set_bin )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_field  = NULL;
    char *ps_value  = NULL;
    long i_fieldlen = 0;
    long i_valuelen = 0;
    long i_ret = 0;
    //char *ps_buf = NULL;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                                    &ps_field, &i_fieldlen,
                                    &ps_value, &i_valuelen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    //snprintf( s_buf1, sizeof(s_buf1), "%s", ps_field );
    //ps_buf = (char *)malloc( i_valuelen + 1 );
    //snprintf( ps_buf, i_valuelen+1, "%s", ps_value );
    //i_ret = tphp_ttc_request_instance->Set( ps_field, ps_buf );
    i_ret = tphp_ttc_request_instance->Set( ps_field, (const char *)(ps_value), i_valuelen );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_limit ) */
ZEND_FUNCTION( tphp_ttc_request_limit )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    long i_from  = 0;
    long i_count = 0;
    long i_ret   = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ll",
                                    &i_from,
                                    &i_count) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    tphp_ttc_request_instance->Limit( i_from, i_count );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_setkey ) */
ZEND_FUNCTION( tphp_ttc_request_setkey )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    long i_value    = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l",
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->SetKey( (int64_t)i_value );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_setkey_str ) */
ZEND_FUNCTION( tphp_ttc_request_setkey_str )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_value  = NULL;
    long i_valuelen = 0;
    long i_len      = 0;
    long i_ret      = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s",
                                    &ps_value, &i_valuelen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
        i_ret = tphp_ttc_request_instance->SetKey( ps_value );

    } else if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_value, &i_valuelen,
                                    &i_len) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
        i_ret = tphp_ttc_request_instance->SetKey( ps_value, i_len );

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_request_execute ) */
ZEND_FUNCTION( tphp_ttc_request_execute )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    TTC::Result  *tphp_ttc_result_instance  = NULL;
    zval *po_result;
    long i_value = 0;
    long i_ret   = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "o",
                                    &po_result) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ol",
                                    &po_result,
                                    &i_value) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    PHP5CPP_GET_OBJ_ZVAL( tphp_ttc_result, po_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        i_ret = tphp_ttc_request_instance->Execute( (*tphp_ttc_result_instance) );
    } else {
        i_ret = tphp_ttc_request_instance->Execute( (*tphp_ttc_result_instance), (int64_t)i_value );
    } // if
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_request_set_multi_bits ) */
ZEND_FUNCTION( tphp_ttc_request_set_multi_bits )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_name  = NULL;
	long i_op = 0;
	long i_s = 0;
	long i_v = 0;
    long i_namelen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 4 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "slll",
                                    &ps_name, &i_namelen,
									&i_op, &i_s, &i_v) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->SetMultiBits( ps_name, (int)i_op, (int)i_s, (unsigned int)i_v );
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_request_set_bit ) */
ZEND_FUNCTION( tphp_ttc_request_set_bit )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_name  = NULL;
	long i_op = 0;
    long i_namelen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_name, &i_namelen,
									&i_op) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->SetBit( ps_name, (int)i_op );
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_request_clear_bit ) */
ZEND_FUNCTION( tphp_ttc_request_clear_bit )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_name  = NULL;
	long i_op = 0;
    long i_namelen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_name, &i_namelen,
									&i_op) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->ClearBit( ps_name, (int)i_op );
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_request_nocache ) */
ZEND_FUNCTION( tphp_ttc_request_nocache )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    tphp_ttc_request_instance->NoCache();
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_request_addkey_value ) */
ZEND_FUNCTION( tphp_ttc_request_addkey_value )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_name  = NULL;
	long i_v = 0;
    long i_namelen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "sl",
                                    &ps_name, &i_namelen,
									&i_v) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->AddKeyValue( ps_name, i_v );
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_request_addkey_value_str ) */
ZEND_FUNCTION( tphp_ttc_request_addkey_value_str )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_name  = NULL;
	char *ps_v;
    long i_namelen = 0;
	long i_vlen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                                    &ps_name, &i_namelen,
									&ps_v, &i_vlen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->AddKeyValue( ps_name, ps_v );
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_request_addkey_value_bin ) */
ZEND_FUNCTION( tphp_ttc_request_addkey_value_bin )
{
    TTC::Request *tphp_ttc_request_instance = NULL;
    char *ps_name  = NULL;
	char *ps_v;
    long i_namelen = 0;
	long i_vlen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_request )
    if ( tphp_ttc_request_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 2 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "ss",
                                    &ps_name, &i_namelen,
									&ps_v, &i_vlen) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    i_ret = tphp_ttc_request_instance->AddKeyValue( ps_name, ps_v, (int)i_vlen );
    RETURN_LONG( i_ret );
}
/* }}} */

