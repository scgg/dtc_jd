/*
 * java_dtc_comm.cc
 *
 *  Created on: Jun 13, 2014
 *      Author: seanzheng
 */
#include <stdlib.h>
#include <iostream>
#include "java_dtc_errcode.h"
#include "java_dtc_comm.h"


jlong getServerObject(JNIEnv *env, jobject obj){

	jclass cls = env->GetObjectClass(obj);
	jmethodID callbackID = env->GetMethodID(cls, "getServer", "()J");
	if (callbackID == NULL) {
		std::cout << "get server tag methodid false" << std::endl;
		return NULL;
	}

	jlong serverObject = env->CallLongMethod(obj, callbackID);
	if (serverObject <= 0) {
		std::cout << "get request tag false" << std::endl;
		return NULL;
	}

	return serverObject;

}
jlong getRequestObject(JNIEnv *env, jobject obj){
	jclass cls = env->GetObjectClass(obj);
	jmethodID callbackID = env->GetMethodID(cls, "getRequest", "()J");
	if (callbackID == NULL) {
		std::cout << "get request tag methodid false" << std::endl;
		return NULL;
	}

	jlong requestObject = env->CallLongMethod(obj, callbackID);
	if (requestObject <= 0) {
		std::cout << "get request tag false" << std::endl;
		return NULL;
	}

	return requestObject;
}

jlong getResultObject(JNIEnv *env, jobject obj){
	jclass cls = env->GetObjectClass(obj);
	jmethodID callbackID = env->GetMethodID(cls, "getResult", "()J");
	if (callbackID == NULL) {
		std::cout << "get result tag methodid false" << std::endl;
		return NULL;
	}

	jlong objRes = env->CallLongMethod(obj, callbackID);
	if ( objRes <= 0) {
		std::cout << "get result tag false" << std::endl;
		return NULL;
	}

	return objRes;
}

