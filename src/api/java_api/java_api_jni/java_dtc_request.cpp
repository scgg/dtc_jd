/*
 * java_dtc_request.cpp
 *
 *  Created on: Jun 13, 2014
 *      Author: seanzheng
 */
#include <jni.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "com_jd_dtc_api_Request.h"
#include "include/ttcapi.h"
#include "java_dtc_errcode.h"
#include "java_dtc_comm.h"
#include <sys/time.h>


/*
 * Class:     com_jd_dtc_api_Request
 * Method:    InitRequest
 * Signature: (Lcom/jd/dtc/api/Server;I)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Request_InitRequest
  (JNIEnv *env, jobject objRequest, jobject objServer, jint op){
	TTC::Server *java_dtc_server = (TTC::Server*)getServerObject(env,objServer);
	if( java_dtc_server == NULL )
	{
		std::cout<<"InitRequest getServerObject false"<<std::endl;
		return;
	}

	TTC::Request *java_dtc_request = new TTC::Request(java_dtc_server,op);
	if( java_dtc_request == NULL )
	{
		std::cout<<"InitRequest new request instance false"<<std::endl;
		return;
	}

	jclass cls = env->GetObjectClass(objRequest);
	jmethodID callbackID = env->GetMethodID(cls, "setRequest", "(J)V");
	if( callbackID == NULL )
	{
		std::cout<<"InitRequest get methodID false"<<std::endl;
		return;
	}

	jlong jrequest =0;
	memcpy( &jrequest, &java_dtc_request, sizeof(TTC::Request*));
	//std::cout<<"Request object jlong:"<<std::hex<<jrequest<<",sizeof,jlong:"<<sizeof(jlong)<<std::endl;

	env->CallVoidMethod(objRequest, callbackID, jrequest);

	//std::cout<<"Request object:"<<java_dtc_request<<std::endl;

	return;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    DestroyRequest
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Request_DestroyRequest
  (JNIEnv *env, jobject obj){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request != NULL )
	{
		//std::cout<<"Request DestroyRequest:"<<java_dtc_request<<std::endl;
		delete java_dtc_request;
		java_dtc_request = NULL;
	}
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Reset
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Request_Reset__
  (JNIEnv *env, jobject obj){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request != NULL )
	{
		java_dtc_request->Reset();
	}

}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Reset
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Request_Reset__I
  (JNIEnv *env, jobject obj, jint op){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request != NULL )
		java_dtc_request->Reset(op);
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    AttachServer
 * Signature: (Lcom/jd/dtc/api/Server;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_AttachServer
  (JNIEnv *env, jobject objRequest, jobject objServer){
	TTC::Server *java_dtc_server = (TTC::Server*)getServerObject(env, objServer);
	if( java_dtc_server == NULL )
		return -EC_SERVER_OBJECT_NULL;
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, objRequest);
	if( java_dtc_request == NULL )
		return -EC_REQUEST_OBJECT_NULL;

	return java_dtc_request->AttachServer(java_dtc_server);
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    NoCache
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Request_NoCache
  (JNIEnv *env, jobject obj){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request != NULL )
	{
		java_dtc_request->NoCache();
	}
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    NoNextServer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Request_NoNextServer
  (JNIEnv *env, jobject obj){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request != NULL )
	{
		java_dtc_request->NoNextServer();
	}
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Need
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Need__Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring name){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request == NULL )
		return EC_REQUEST_OBJECT_NULL;

	const char *c_name = env->GetStringUTFChars(name, NULL);
	int ret = java_dtc_request->Need(c_name);
	env->ReleaseStringUTFChars(name, c_name);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Need
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Need__Ljava_lang_String_2I
  (JNIEnv *env, jobject obj, jstring name, jint vid){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request == NULL )
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_name = env->GetStringUTFChars(name, NULL);
	int ret = java_dtc_request->Need(c_name, vid);
	env->ReleaseStringUTFChars(name, c_name);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    FieldType
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_FieldType
  (JNIEnv *env, jobject obj, jstring name){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request == NULL )
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_name = env->GetStringUTFChars(name, NULL);
	int ret = java_dtc_request->FieldType(c_name);
	env->ReleaseStringUTFChars(name, c_name);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Limit
 * Signature: (II)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Request_Limit
  (JNIEnv *env, jobject obj, jint st, jint cnt){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request != NULL )
		java_dtc_request->Limit(st, cnt);
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    UnsetKey
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Request_UnsetKey
  (JNIEnv *env, jobject obj){
	TTC::Request *java_dtc_request = (TTC::Request*)getRequestObject(env, obj);
	if( java_dtc_request != NULL )
		java_dtc_request->UnsetKey();
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    SetKey
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_SetKey__J
  (JNIEnv *env, jobject obj, jlong key){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	return java_dtc_request->SetKey(key);

}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    SetKey
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_SetKey__Ljava_lang_String_2I
  (JNIEnv *env, jobject obj, jstring key, jint len){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_key = env->GetStringUTFChars(key, NULL);
	int ret = java_dtc_request->SetKey(c_key,len);
	env->ReleaseStringUTFChars(key, c_key);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    AddKeyValue
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_AddKeyValue__Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring name, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_name = env->GetStringUTFChars(name, NULL);
	int ret = java_dtc_request->AddKeyValue(c_name, v);
	env->ReleaseStringUTFChars(name, c_name);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    AddKeyValue
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_AddKeyValue__Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring name, jstring v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_name = env->GetStringUTFChars(name, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->AddKeyValue(c_name, c_v);
	env->ReleaseStringUTFChars(name, c_name);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    AddKeyValue
 * Signature: (Ljava/lang/String;Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_AddKeyValue__Ljava_lang_String_2Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring name , jstring v, jlong len){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_name = env->GetStringUTFChars(name, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->AddKeyValue(c_name, c_v, len);
	env->ReleaseStringUTFChars(name, c_name);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;

}


/*
 * Class:     com_jd_dtc_api_Request
 * Method:    SetCacheID
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_SetCacheID
  (JNIEnv *env, jobject obj, jlong id){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	return java_dtc_request->SetCacheID(id);
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    JExecute
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Request_JExecute__
  (JNIEnv *env, jobject obj){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	return (jlong)java_dtc_request->Execute();

}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    JExecute
 * Signature: (J)J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Request_JExecute__J
  (JNIEnv *env, jobject obj, jlong k){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	return (long)java_dtc_request->Execute(k);
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    JExecute
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Request_JExecute__Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring k){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_k = env->GetStringUTFChars(k, NULL);
	TTC::Result *java_dtc_result = java_dtc_request->Execute(c_k);
	env->ReleaseStringUTFChars(k, c_k);

	jlong jresult = 0;
	memcpy(&jresult,&java_dtc_result,sizeof(TTC::Result*));

	return jresult;

}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    JExecute
 * Signature: (Ljava/lang/String;I)J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Request_JExecute__Ljava_lang_String_2I
  (JNIEnv *env, jobject obj, jstring k, jint l){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_k = env->GetStringUTFChars(k, NULL);
	TTC::Result *java_dtc_result = java_dtc_request->Execute(c_k, l);
	env->ReleaseStringUTFChars(k, c_k);

	jlong jresult = 0;
	memcpy(&jresult,&java_dtc_result,sizeof(TTC::Result*));

	return jresult;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Execute
 * Signature: (Lcom/jd/dtc/api/Result;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Execute__Lcom_jd_dtc_api_Result_2
  (JNIEnv *env, jobject objReq, jobject objRes){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, objReq);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;
	jclass cls = env->GetObjectClass(objRes);
	jmethodID callbackID = env->GetMethodID(cls, "setResult", "(J)V");
	if( callbackID == NULL )
	{
		std::cout<<"Request::Execute,Get Result methodid false"<<std::endl;
		return -EC_GET_RESULT_METHODID;
	}

	TTC::Result *java_dtc_result = new TTC::Result();
	int ret = java_dtc_request->Execute(*java_dtc_result);

	jlong jresult = 0;
	memcpy(&jresult,&java_dtc_result,sizeof(TTC::Result*));
	env->CallLongMethod(objRes, callbackID, jresult );

	//std::cout<<"Result object:"<<java_dtc_result<<std::endl;

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Execute
 * Signature: (Lcom/jd/dtc/api/Result;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Execute__Lcom_jd_dtc_api_Result_2J
  (JNIEnv *env, jobject objReq, jobject objRes, jlong k){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, objReq);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;
	jclass cls = env->GetObjectClass(objRes);
	jmethodID callbackID = env->GetMethodID(cls, "setResult", "(J)V");
	if (callbackID == NULL) {
		std::cout << "Request::Execute,Get Result methodid false" << std::endl;
		return -EC_GET_RESULT_METHODID;
	}

	TTC::Result *java_dtc_result = new TTC::Result();
	int ret = java_dtc_request->Execute(*java_dtc_result, k);

	jlong jresult = 0;
	memcpy(&jresult,&java_dtc_result,sizeof(TTC::Result*));
	env->CallLongMethod(objRes, callbackID, jresult );

	//std::cout << "Result object:" << java_dtc_result << std::endl;

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Execute
 * Signature: (Lcom/jd/dtc/api/Result;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Execute__Lcom_jd_dtc_api_Result_2Ljava_lang_String_2
  (JNIEnv *env, jobject objReq, jobject objRes, jstring k ){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, objReq);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;
	jclass cls = env->GetObjectClass(objRes);
	jmethodID callbackID = env->GetMethodID(cls, "setResult", "(J)V");
	if (callbackID == NULL) {
		std::cout << "Request::Execute,Get Result methodid false" << std::endl;
		return -EC_GET_RESULT_METHODID;
	}

	const char *c_k = env->GetStringUTFChars(k, NULL);
	TTC::Result *java_dtc_result = new TTC::Result();
	int ret = java_dtc_request->Execute(*java_dtc_result, c_k);
	env->ReleaseStringUTFChars(k, c_k);

	jlong jresult = 0;
	memcpy(&jresult,&java_dtc_result,sizeof(TTC::Result*));
	env->CallLongMethod(objRes, callbackID, jresult );

	//std::cout << "Result object:" << java_dtc_result << std::endl;

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Execute
 * Signature: (Lcom/jd/dtc/api/Result;Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Execute__Lcom_jd_dtc_api_Result_2Ljava_lang_String_2I
  (JNIEnv *env, jobject objReq, jobject objRes, jstring k, jint l){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, objReq);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;
	jclass cls = env->GetObjectClass(objRes);
	jmethodID callbackID = env->GetMethodID(cls, "setResult", "(J)V");
	if (callbackID == NULL) {
		std::cout << "Request::Execute,Get Result methodid false" << std::endl;
		return -EC_GET_RESULT_METHODID;
	}

	const char *c_k = env->GetStringUTFChars(k, NULL);
	TTC::Result *java_dtc_result = new TTC::Result();
	int ret = java_dtc_request->Execute(*java_dtc_result, c_k, l);
	env->ReleaseStringUTFChars(k, c_k);

	jlong jresult = 0;
	memcpy(&jresult,&java_dtc_result,sizeof(TTC::Result*));
	env->CallLongMethod(objRes, callbackID, jresult );

	//std::cout << "Result object:" << java_dtc_result << std::endl;

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    ErrorMessage
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Request_ErrorMessage
  (JNIEnv *env, jobject obj){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return NULL;

	return env->NewStringUTF(java_dtc_request->ErrorMessage());
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    EQ
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_EQ__Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->EQ(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    EQ
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_EQ__Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring n, jstring v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->EQ(c_n, c_v);
	env->ReleaseStringUTFChars(n, c_n);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    EQ
 * Signature: (Ljava/lang/String;Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_EQ__Ljava_lang_String_2Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring n, jstring v, jlong l){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->EQ(c_n, c_v, l);
	env->ReleaseStringUTFChars(n, c_n);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    NE
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_NE__Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->NE(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    NE
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_NE__Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring n, jstring v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->NE(c_n, c_v);
	env->ReleaseStringUTFChars(n, c_n);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    NE
 * Signature: (Ljava/lang/String;Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_NE__Ljava_lang_String_2Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring n, jstring v, jlong l){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->NE(c_n, c_v, l);
	env->ReleaseStringUTFChars(n, c_n);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    GT
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_GT
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->GT(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    GE
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_GE
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->GE(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}
/*
 * Class:     com_jd_dtc_api_Request
 * Method:    LE
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_LE
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->LE(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    LT
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_LT
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->LT(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    OR
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_OR
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->OR(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Add
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Add__Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->Add(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Add
 * Signature: (Ljava/lang/String;D)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Add__Ljava_lang_String_2D
  (JNIEnv *env, jobject obj, jstring n, jdouble v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->Add(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Sub
 * Signature: (Ljava/lang/String;D)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Sub__Ljava_lang_String_2D
  (JNIEnv *env, jobject obj, jstring n, jdouble v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->Sub(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Sub
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Sub__Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->Sub(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Set
 * Signature: (Ljava/lang/String;D)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Set__Ljava_lang_String_2D
  (JNIEnv *env, jobject obj, jstring n, jdouble v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->Set(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Set
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Set__Ljava_lang_String_2Ljava_lang_String_2
  (JNIEnv *env, jobject obj, jstring n, jstring v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->Set(c_n, c_v);
	env->ReleaseStringUTFChars(n, c_n);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Set
 * Signature: (Ljava/lang/String;Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Set__Ljava_lang_String_2Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring n, jstring v, jlong l){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->Set(c_n, c_v, l);
	env->ReleaseStringUTFChars(n, c_n);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Set
 * Signature: (Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Set__Ljava_lang_String_2J
  (JNIEnv *env, jobject obj, jstring n, jlong v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->Set(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    Set
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_Set__Ljava_lang_String_2I
  (JNIEnv *env, jobject obj, jstring n, jint v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->Set(c_n, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;

}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    CompressSet
 * Signature: (Ljava/lang/String;Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_CompressSet
  (JNIEnv *env, jobject obj, jstring n, jstring v, jlong l){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->CompressSet(c_n, c_v, l);
	env->ReleaseStringUTFChars(n, c_n);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    CompressSetForce
 * Signature: (Ljava/lang/String;Ljava/lang/String;J)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_CompressSetForce
  (JNIEnv *env, jobject obj, jstring n, jstring v, jlong l){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	const char *c_v = env->GetStringUTFChars(v, NULL);
	int ret = java_dtc_request->CompressSetForce(c_n, c_v, l);
	env->ReleaseStringUTFChars(n, c_n);
	env->ReleaseStringUTFChars(v, c_v);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Request
 * Method:    SetMultiBits
 * Signature: (Ljava/lang/String;III)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Request_SetMultiBits
  (JNIEnv *env, jobject obj, jstring n, jint o, jint s, jint v){
	TTC::Request *java_dtc_request = (TTC::Request*) getRequestObject(env, obj);
	if (java_dtc_request == NULL)
		return -EC_REQUEST_OBJECT_NULL;

	const char *c_n = env->GetStringUTFChars(n, NULL);
	int ret = java_dtc_request->SetMultiBits(c_n, o, s, v);
	env->ReleaseStringUTFChars(n, c_n);

	return ret;
}


/*
 * Class:     com_jd_dtc_api_Request
 * Method:    GetCurrentUSeconds
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Request_GetCurrentUSeconds
  (JNIEnv *env, jobject obj){
	struct timeval now;
	gettimeofday(&now,0);

	return now.tv_sec*1000000+now.tv_usec;
}

