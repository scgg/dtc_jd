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
 * @brief   Tencent PHP Library define.
 */


#include "tphp_ttc.h"
#include "php5obj.h"
#include "ttcapi.h"

extern int le_tphp_ttc_result;

#define TTC_ASSOC		0x1
#define TTC_NUM		    0x2
#define TTC_BOTH		(TTC_ASSOC|TTC_NUM)

/* {{{ ZEND_FUNCTION( tphp_ttc_result_new ) */
ZEND_FUNCTION( tphp_ttc_result_new )
{
    TTC::Result *tphp_ttc_result_instance = NULL;

    tphp_ttc_result_instance = new TTC::Result();
    if ( tphp_ttc_result_instance != NULL ) {
        PHP5CPP_REGISTER_RESOURCE( tphp_ttc_result, return_value, tphp_ttc_result_instance, le_tphp_ttc_result );
    } // if
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_reset ) */
ZEND_FUNCTION( tphp_ttc_result_reset )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    tphp_ttc_result_instance->Reset();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_resultcode ) */
ZEND_FUNCTION( tphp_ttc_result_resultcode )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_result_instance->ResultCode();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_errormessage ) */
ZEND_FUNCTION( tphp_ttc_result_errormessage )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    //char s_buf[TPHP_TTC_STRLEN_LONG] = { 0 };
    const char *ps_msg = NULL;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    ps_msg = tphp_ttc_result_instance->ErrorMessage();
    //snprintf( s_buf,  sizeof(s_buf),  "%s", ps_msg );
    if ( ps_msg == NULL ) {
    	RETURN_STRING("no error", 1);
    }
    RETURN_STRING( (char *)ps_msg, 1 );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_errorfrom ) */
ZEND_FUNCTION( tphp_ttc_result_errorfrom )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    //char s_buf[TPHP_TTC_STRLEN_LONG] = { 0 };
    const char *ps_msg = NULL;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    ps_msg = tphp_ttc_result_instance->ErrorFrom();
    //snprintf( s_buf,  sizeof(s_buf),  "%s", ps_msg );
    if ( ps_msg == NULL ) {
    	RETURN_STRING("no error", 1);
    }
    RETURN_STRING( (char *)ps_msg, 1 );

}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_numrows ) */
ZEND_FUNCTION( tphp_ttc_result_numrows )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_result_instance->NumRows();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_totalrows ) */
ZEND_FUNCTION( tphp_ttc_result_totalrows )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_result_instance->TotalRows();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_affectedrows ) */
ZEND_FUNCTION( tphp_ttc_result_affectedrows )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_result_instance->AffectedRows();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_numfields ) */
ZEND_FUNCTION( tphp_ttc_result_numfields )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_result_instance->NumFields();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_intkey ) */
ZEND_FUNCTION( tphp_ttc_result_insertid )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_result_instance->InsertID();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_intkey ) */
ZEND_FUNCTION( tphp_ttc_result_intkey )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_result_instance->IntKey();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_stringkey ) */
ZEND_FUNCTION( tphp_ttc_result_stringkey )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    //char s_buf[TPHP_TTC_STRLEN_TEXT] = { 0 };
    const char *ps_key = NULL;
    int  i_len = 0;
    zval *z_len;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_len) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
        /* check for parameter being passed by reference */
        if (!PZVAL_IS_REF(z_len)) {
            zend_error(E_WARNING, "z_len wasn't passed by reference");
            //RETURN_LONG( -1 );
        }
        ps_key = tphp_ttc_result_instance->StringKey( i_len );
        ZVAL_LONG(z_len, i_len);

    } else if ( ZEND_NUM_ARGS() == 0 ) {
        ps_key = tphp_ttc_result_instance->StringKey( );

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    //snprintf( s_buf,  sizeof(s_buf),  "%s", ps_key );
    if ( i_len != 0 ) {
        RETURN_STRINGL( (char *)ps_key, i_len, 1 );
    } else {
        RETURN_STRING( (char *)ps_key, 1 );
    }
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_binarykey ) */
ZEND_FUNCTION( tphp_ttc_result_binarykey )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    //char s_buf[TPHP_TTC_STRLEN_TEXT] = { 0 };
    const char *ps_key = NULL;
    int  i_len = 0;
    zval *z_len;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &z_len) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
        /* check for parameter being passed by reference */
        if (!PZVAL_IS_REF(z_len)) {
            zend_error(E_WARNING, "z_len wasn't passed by reference");
            //RETURN_LONG( -1 );
        }
        ps_key = tphp_ttc_result_instance->BinaryKey( i_len );
        ZVAL_LONG(z_len, i_len);

    } else if ( ZEND_NUM_ARGS() == 0 ) {
        ps_key = tphp_ttc_result_instance->BinaryKey( );

    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    //snprintf( s_buf,  sizeof(s_buf),  "%s", ps_key );
    if ( i_len != 0 ) {
        RETURN_STRINGL( (char *)ps_key, i_len, 1 );
    } else {
        RETURN_STRING( (char *)ps_key, 1 );
    }
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_intvalue ) */
ZEND_FUNCTION( tphp_ttc_result_intvalue )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
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

    i_ret = tphp_ttc_result_instance->IntValue( ps_field );
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_floatvalue ) */
ZEND_FUNCTION( tphp_ttc_result_floatvalue )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    double i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
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

    i_ret = tphp_ttc_result_instance->FloatValue( ps_field );
    RETURN_DOUBLE( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_stringvalue ) */
ZEND_FUNCTION( tphp_ttc_result_stringvalue )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    int  i_len = 0;
    //char s_buf[TPHP_TTC_STRLEN_TEXT] = { 0 };
    const char *ps_val = NULL;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
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

    ps_val = tphp_ttc_result_instance->StringValue( ps_field, i_len );
    //snprintf( s_buf,  sizeof(s_buf),  "%s", ps_val );
    RETURN_STRINGL( (char *)ps_val, i_len, 1 );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_binaryvalue ) */
ZEND_FUNCTION( tphp_ttc_result_binaryvalue )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    char *ps_field  = NULL;
    long i_fieldlen = 0;
    int  i_len = 0;
    //char s_buf[TPHP_TTC_STRLEN_TEXT] = { 0 };
    const char *ps_val = NULL;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
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

    ps_val = tphp_ttc_result_instance->BinaryValue( ps_field, i_len );
    //snprintf( s_buf,  sizeof(s_buf),  "%s", ps_val );
    RETURN_STRINGL( (char *)ps_val, i_len, 1 );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_fetchrow ) */
ZEND_FUNCTION( tphp_ttc_result_fetchrow )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_result_instance->FetchRow();
    RETURN_LONG( i_ret );
}
/* }}} */


/* {{{ ZEND_FUNCTION( tphp_ttc_result_rewind ) */
ZEND_FUNCTION( tphp_ttc_result_rewind )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    i_ret = tphp_ttc_result_instance->Rewind();
    RETURN_LONG( i_ret );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_result_fieldname ) */
ZEND_FUNCTION( tphp_ttc_result_fieldname )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    long i_fid;
    const char *ps_name = NULL;
    
    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &i_fid) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
       
        ps_name = tphp_ttc_result_instance->FieldName( i_fid );
        if(ps_name == NULL)
			RETURN_LONG( -1 );
    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

    RETURN_STRING( (char *)ps_name, 1 );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_result_fieldpresent ) */
ZEND_FUNCTION( tphp_ttc_result_fieldpresent )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
    const char *ps_name = NULL;
    int  i_len = 0;
   
    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_LONG( -1 );
    } // if

    if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &ps_name, &i_len) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_LONG( -1 );
        } /* if */
    } else {
        WRONG_PARAM_COUNT;
        RETURN_LONG( -1 );
    } /* if ( ZEND_NUM_ARGS() == x ) */

	RETURN_LONG( tphp_ttc_result_instance->FieldPresent( ps_name ) );
}
/* }}} */

/* {{{ ZEND_FUNCTION( tphp_ttc_result_fetcharray ) */
ZEND_FUNCTION( tphp_ttc_result_fetcharray )
{
    TTC::Result *tphp_ttc_result_instance = NULL;
	long i_res_type;
    long i_ret = 0;

    PHP5CPP_GET_OBJ( tphp_ttc_result )
    if ( tphp_ttc_result_instance == NULL ) {
        RETURN_FALSE;
    } // if

	if ( ZEND_NUM_ARGS() == 1 ) {
        if ( zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l", &i_res_type) == FAILURE ) {
            WRONG_PARAM_COUNT;
            RETURN_FALSE;
        } /* if */
		if((i_res_type & TTC_BOTH) == 0){
			php_error_docref(NULL TSRMLS_CC, E_WARNING, "The result type should be either TTC_NUM, TTC_ASSOC or TTC_BOTH.");
			RETURN_FALSE;
		}
    } else {
        i_res_type = TTC_BOTH;
    } /* if ( ZEND_NUM_ARGS() == x ) */
	
    i_ret = tphp_ttc_result_instance->FetchRow();
	if(i_ret != 0){
		RETURN_FALSE;
	}
	
	array_init(return_value);
	char buf[128];
	int len;
	for(int i=0; i < tphp_ttc_result_instance->NumFields(); i++){
		zval *data;
		MAKE_STD_ZVAL(data);
		
		switch(tphp_ttc_result_instance->FieldType(i)){
		/*
			case FieldTypeSigned:
				Z_TYPE_P(data) = IS_LONG;
				Z_LVAL_P(data) = (long)(tphp_ttc_result_instance->IntValue(i));
				break;
		*/	
			case TTC::FieldTypeSigned:
				len = snprintf(buf, sizeof(buf), "%lld", tphp_ttc_result_instance->IntValue(i));
				ZVAL_STRINGL(data, buf, len, 1);
				break;
				
			case TTC::FieldTypeUnsigned:
				len = snprintf(buf, sizeof(buf), "%llu", (unsigned long long)(tphp_ttc_result_instance->IntValue(i)));
				ZVAL_STRINGL(data, buf, len, 1);
				break;
			
			case TTC::FieldTypeFloat:
				ZVAL_DOUBLE(data, tphp_ttc_result_instance->FloatValue(i));
				break;
			
			case TTC::FieldTypeString:
				{
					const char* p = tphp_ttc_result_instance->StringValue(tphp_ttc_result_instance->FieldName(i), len);
					ZVAL_STRINGL(data, (char*)p, len, 1);
					break;
				}
			
			case  TTC::FieldTypeBinary:
				{
					const char* p = tphp_ttc_result_instance->BinaryValue(tphp_ttc_result_instance->FieldName(i), len);
					ZVAL_STRINGL(data, (char*)p, len, 1);
					break;
				}
			
			default:
				php_error_docref(NULL TSRMLS_CC, E_WARNING, "unknow field type in result-set.");
				RETURN_FALSE;
		}
		
		if (i_res_type & TTC_NUM) {
			add_index_zval(return_value, i, data);
		}
		if (i_res_type & TTC_ASSOC) {
			if (i_res_type & TTC_NUM) {
				Z_ADDREF_P(data);
			}
			add_assoc_zval(return_value, (char*)(tphp_ttc_result_instance->FieldName(i)), data);
		}
	}
	
}
/* }}} */


