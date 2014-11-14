/*
 * java_dtc_result.cpp
 *
 *  Created on: Jun 16, 2014
 *      Author: seanzheng
 */
#include <jni.h>
#include <stdlib.h>
#include <iostream>
#include "com_jd_dtc_api_Result.h"
#include "ttcapi.h"
#include "java_dtc_errcode.h"
#include "java_dtc_comm.h"


/*
 * Class:     com_jd_dtc_api_Result
 * Method:    Reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Result_Reset
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*)getResultObject(env, obj);
	if( java_dtc_result != NULL )
		java_dtc_result->Reset();
}


/*
 * Class:     com_jd_dtc_api_Result
 * Method:    DestroyResult
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Result_DestroyResult
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*)getResultObject(env, obj);
		if( java_dtc_result != NULL )
		{
			//std::cout<<"Result DestroyRequest:"<<java_dtc_result<<std::endl;
			delete java_dtc_result;
			java_dtc_result = NULL;
		}
}


/*
 * Class:     com_jd_dtc_api_Result
 * Method:    FetchRow
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Result_FetchRow
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;
	return java_dtc_result->FetchRow();
}



/*
 * Class:     com_jd_dtc_api_Result
 * Method:    ResultCode
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Result_ResultCode
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;
	return java_dtc_result->ResultCode();
}



/*
 * Class:     com_jd_dtc_api_Result
 * Method:    ServerInfo
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_ServerInfo
  (JNIEnv *env, jobject obj);//{
//	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
//	if (java_dtc_result == NULL)
//		return NULL;
//
//	return env->NewStringUTF(java_dtc_result->ServerInfo());
//}




/*
 * Class:     com_jd_dtc_api_Result
 * Method:    NumRows
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Result_NumRows
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->NumRows();
}






/*
 * Class:     com_jd_dtc_api_Result
 * Method:    AffectedRows
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Result_AffectedRows
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->AffectedRows();
}


/*
 * Class:     com_jd_dtc_api_Result
 * Method:    IntValue
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Result_IntValue__Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring name){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;
	const char *c_name = env->GetStringUTFChars(name, NULL);
	int ret = java_dtc_result->IntValue(c_name);
	env->ReleaseStringUTFChars(name, c_name);

	return ret;
}





/*
 * Class:     com_jd_dtc_api_Result
 * Method:    StringValue
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_StringValue__Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring name){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return NULL;
	const char *c_name = env->GetStringUTFChars(name, NULL);
	const char *ret = java_dtc_result->StringValue(c_name);
	env->ReleaseStringUTFChars(name, c_name);

	return env->NewStringUTF(ret);
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    StringValue
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_StringValue__I
  (JNIEnv *env, jobject obj, jint id){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return NULL;

	return env->NewStringUTF(java_dtc_result->StringValue(id));
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    BinaryValue
 * Signature: (Ljava/lang/String;)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_BinaryValue__Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring name){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return NULL;
	const char *c_name = env->GetStringUTFChars(name, NULL);
	const char *res = java_dtc_result->BinaryValue(c_name);
	env->ReleaseStringUTFChars(name, c_name);
	return env->NewStringUTF(res);
}

