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


#ifndef _PHP5OBJ_H_
#define _PHP5OBJ_H_

//#include "clib_mss.h"
//#include "clib_sock.h"
#include "ttcapi.h"

//using namespace TencentTableCache;

typedef enum {
//    is_tphp_mss,
//    is_tphp_sock,
    is_tphp_ttc_server = 2,
    is_tphp_ttc_request,
    is_tphp_ttc_result
} php5cpp_obj_type;

struct php5cpp_obj {
    zend_object std;
    int         rsrc_id;
    php5cpp_obj_type type;
    union {
//        clib_mss    *tphp_mss;
//        clib_sock   *tphp_sock;
        TTC::Server     *tphp_ttc_server;
        TTC::Request    *tphp_ttc_request;
        TTC::Result     *tphp_ttc_result;
    } u;
};


// Register the class entry..
#define PHP5CPP_REGISTER_CLASS(name, obj_name) \
    { \
        zend_class_entry ce; \
        INIT_CLASS_ENTRY(ce, obj_name, php5cpp_ ## name ##_methods); \
        ce.create_object = php5cpp_object_new_ ## name; \
        php5cpp_ce_ ## name = zend_register_internal_class_ex(&ce, NULL, NULL TSRMLS_CC);  \
        memcpy( &php5cpp_object_handlers_ ## name, \
                zend_get_std_object_handlers(), \
                sizeof(zend_object_handlers) ); \
        php5cpp_object_handlers_ ## name.clone_obj = NULL; \
        php5cpp_ce_ ## name->ce_flags |= ZEND_ACC_FINAL_CLASS; \
    }


#define PHP5CPP_GET_THIS() \
    zval* object = getThis();


// Register resources. If we're using an object, put it into the object store.
#define PHP5CPP_REGISTER_RESOURCE(obj_type, return_value, res, le) \
    { \
        PHP5CPP_GET_THIS(); \
        int rsrc_id = ZEND_REGISTER_RESOURCE(object ? NULL : return_value, res, le); \
        if (object) { \
            php5cpp_obj *obj = (php5cpp_obj *)zend_object_store_get_object(object TSRMLS_CC); \
            obj->u.obj_type  = res; \
            obj->rsrc_id     = rsrc_id; \
            obj->type        = is_ ## obj_type; \
        } \
    }


// These are for parsing parameters and getting the actual object from the store.
#define PHP5CPP_SET_OBJ(type) \
    { \
        php5cpp_obj *obj  = (php5cpp_obj *)zend_object_store_get_object( object TSRMLS_CC ); \
        type ## _instance = obj->u.type; \
    }

// Deprecated
#define PHP5CPP_OBJ_PARAMS(type, params) \
    { \
        PHP5CPP_GET_THIS(); \
        if (object) { \
            if (params == FAILURE) { \
                RETURN_FALSE; \
            } \
            PHP5CPP_SET_OBJ(type) \
        } \
    }

// Deprecated
#define PHP5CPP_OBJ_NO_PARAMS(type) \
    { \
        PHP5CPP_GET_THIS(); \
        if (object) { \
            if (ZEND_NUM_ARGS() != 0) { \
                php_error(E_WARNING, "didn't expect any arguments in %s()", get_active_function_name(TSRMLS_C)); \
            } \
            PHP5CPP_SET_OBJ(type) \
        } \
    }

#define PHP5CPP_GET_OBJ(type) \
    { \
        PHP5CPP_GET_THIS(); \
        if (object) { \
            PHP5CPP_SET_OBJ(type) \
        } \
        if (type ## _instance == NULL) { \
            php_error(E_WARNING, "Get object fall in %s()", get_active_function_name(TSRMLS_C)); \
        } \
    }

#define PHP5CPP_GET_OBJ_ZVAL(type, obj) \
    { \
        zval* object = obj; \
        if (object) { \
            PHP5CPP_SET_OBJ(type) \
        } \
        if (type ## _instance == NULL) { \
            php_error(E_WARNING, "Get object fall in %s()", get_active_function_name(TSRMLS_C)); \
        } \
    }

    //static void php5cpp_object_free_storage_ ## type(zend_object *object TSRMLS_DC) 
#define PHP5CPP_OBJ_FREE_FUNCTION(type) \
    static void php5cpp_object_free_storage_ ## type(void *object TSRMLS_DC) \
    { \
        php5cpp_obj *obj = (php5cpp_obj *) object; \
        zend_hash_destroy(obj->std.properties); \
        FREE_HASHTABLE(obj->std.properties); \
        if ( obj->u.type ) { \
            zend_list_delete(obj->rsrc_id); \
        } \
        efree(obj); \
    }


#define PHP5CPP_OBJ_NEW_FUNCTION(type) \
    static zend_object_value php5cpp_object_new_ ## type(zend_class_entry *class_type TSRMLS_DC) \
    { \
        zend_object_value retval; \
        zval *tmp; \
        php5cpp_obj *obj = (php5cpp_obj *)emalloc( sizeof(php5cpp_obj) ); \
        memset( obj, 0, sizeof(php5cpp_obj) ); \
        obj->std.ce = class_type; \
        ALLOC_HASHTABLE( obj->std.properties ); \
        zend_hash_init( obj->std.properties, 0, NULL, ZVAL_PTR_DTOR, 0 ); \
        zend_hash_copy( obj->std.properties, \
                        &class_type->properties_info, \
                        (copy_ctor_func_t) zval_add_ref, \
                        (void *) &tmp, \
                        sizeof(zval *) ); \
        retval.handle = zend_objects_store_put( obj, \
                                                NULL, \
                                                (zend_objects_free_object_storage_t)php5cpp_object_free_storage_ ## type, \
                                                NULL TSRMLS_CC ); \
        retval.handlers = &php5cpp_object_handlers_ ## type; \
        return retval; \
    }

#endif


