/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
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

/* $Id: php_content_types.h 321634 2012-01-01 13:15:04Z felipe $ */

#ifndef PHP_CONTENT_TYPES_H
#define PHP_CONTENT_TYPES_H

#define DEFAULT_POST_CONTENT_TYPE "application/x-www-form-urlencoded"

SAPI_API SAPI_POST_READER_FUNC(php_default_post_reader);
SAPI_API SAPI_POST_HANDLER_FUNC(php_std_post_handler);
int php_startup_sapi_content_types(TSRMLS_D);
int php_setup_sapi_content_types(TSRMLS_D);

#endif /* PHP_CONTENT_TYPES_H */
