/*
 * java_dtc_errcode.h
 *
 *  Created on: Jun 10, 2014
 *      Author: parallels
 */

#ifndef JAVA_DTC_ERRCODE_H_
#define JAVA_DTC_ERRCODE_H_

enum{
	EC_ERROR_BASE = 3000,
	EC_GET_SERVER_METHODID,
	EC_GET_SERVER_OBJECT,
	EC_SERVER_OBJECT_NULL,
	EC_REQUEST_OBJECT_NULL,
	EC_GET_REQUEST_METHODID,
	EC_GET_REQUEST_OBJECT,
	EC_GET_RESULT_METHODID,
	EC_GET_RESULT_OBJECT,
	EC_RESULT_OBJECT_NULL,
};


#endif /* JAVA_DTC_ERRCODE_H_ */