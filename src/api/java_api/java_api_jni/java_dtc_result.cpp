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
#include "include/ttcapi.h"
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
 * Method:    SetError
 * Signature: (ILjava/lang/String;Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Result_SetError
  (JNIEnv *env, jobject obj, jint err, jstring via , jstring msg){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result != NULL){
		const char *c_via = env->GetStringUTFChars(via, NULL);
		const char *c_msg = env->GetStringUTFChars(msg, NULL);
		java_dtc_result->SetError(err, c_via, c_msg);
		env->ReleaseStringUTFChars(via, c_via);
		env->ReleaseStringUTFChars(msg, c_msg);
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
 * Method:    Rewind
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Result_Rewind
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;
	return java_dtc_result->Rewind();
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
 * Method:    ErrorMessage
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_ErrorMessage
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return NULL;
	return env->NewStringUTF(java_dtc_result->ErrorMessage());
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    ErrorFrom
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_ErrorFrom
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return NULL;

	return env->NewStringUTF(java_dtc_result->ErrorFrom());
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
 * Method:    BinlogID
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Result_BinlogID
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->BinlogID();
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    BinlogOffset
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Result_BinlogOffset
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->BinlogOffset();
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    MemSize
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Result_MemSize
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->MemSize();
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    DataSize
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Result_DataSize
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->DataSize();
}

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
 * Method:    TotalRows
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Result_TotalRows
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->TotalRows();
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    InsertID
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Result_InsertID
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->InsertID();
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    NumFields
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Result_NumFields
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->NumFields();
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    FieldName
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_FieldName
  (JNIEnv *env, jobject obj, jint n){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return NULL;

	return env->NewStringUTF(java_dtc_result->FieldName(n));
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    FieldPresent
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Result_FieldPresent
  (JNIEnv *env, jobject obj, jstring name){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	const char *c_name = env->GetStringUTFChars(name, NULL);
	int ret = java_dtc_result->FieldPresent(c_name);
	env->ReleaseStringUTFChars(name, c_name);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    FieldType
 * Signature: (I)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Result_FieldType
  (JNIEnv *env, jobject obj, jint n){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->FieldType(n);
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
 * Method:    IntKey
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Result_IntKey
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->IntKey();
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    StringKey
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_StringKey__
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return NULL;

	return env->NewStringUTF(java_dtc_result->StringKey());
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    BinaryKey
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_BinaryKey__
  (JNIEnv *env, jobject obj){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return NULL;

	return env->NewStringUTF(java_dtc_result->BinaryKey());
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
 * Method:    IntValue
 * Signature: (I)J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Result_IntValue__I
  (JNIEnv *env, jobject obj, jint id){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->IntValue(id);
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    FloatValue
 * Signature: (Ljava/lang/String;)D
 */
JNIEXPORT jdouble JNICALL Java_com_jd_dtc_api_Result_FloatValue__Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring name){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;
	const char *c_name = env->GetStringUTFChars(name, NULL);
	double ret = java_dtc_result->FloatValue(c_name);
	env->ReleaseStringUTFChars(name,c_name);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    FloatValue
 * Signature: (I)D
 */
JNIEXPORT jdouble JNICALL Java_com_jd_dtc_api_Result_FloatValue__I
  (JNIEnv *env, jobject obj, jint id){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return EC_REQUEST_OBJECT_NULL;

	return java_dtc_result->FloatValue(id);
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

/*
 * Class:     com_jd_dtc_api_Result
 * Method:    BinaryValue
 * Signature: (I)Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Result_BinaryValue__I
  (JNIEnv *env, jobject obj, jint id){
	TTC::Result *java_dtc_result = (TTC::Result*) getResultObject(env, obj);
	if (java_dtc_result == NULL)
		return NULL;

	return env->NewStringUTF(java_dtc_result->BinaryValue(id));
}

