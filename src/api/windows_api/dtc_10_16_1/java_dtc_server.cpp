#include <jni.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>

#include "com_jd_dtc_api_Server.h"
#include "ttcapi.h"
#include "java_dtc_errcode.h"
#include "java_dtc_comm.h"


/*
 * Class:     com_jd_dtc_api_Server
 * Method:    initServer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_InitServer(JNIEnv *env, jobject obj)
{
	TTC::Server *ttc_server_instance = new TTC::Server();
	jclass cls = env->GetObjectClass(obj);
	jmethodID callbackID = env->GetMethodID(cls, "setServer", "(J)V");
	if( callbackID == NULL )
	{
		std::cout<<"get server tag methodid false"<<std::endl;
		return;
	}

	jlong jserver = 0;
	memcpy(&jserver,&ttc_server_instance,sizeof(TTC::Server*));
	env->CallLongMethod(obj,callbackID, jserver);
	//ttc_server_instance->SetTableName("test");

	//std::cout<<"server:"<<ttc_server_instance<<std::endl;
	return;

}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    destroyServer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_DestroyServer(JNIEnv *env, jobject obj)
{
	TTC::Server *java_dtc_server = (TTC::Server*)getServerObject(env, obj);
	if ( java_dtc_server == NULL )
		return;
	delete java_dtc_server;
	java_dtc_server = NULL;
	//std::cout<<"server destroy:"<<java_dtc_server<<std::endl;
}


/*
 * Class:     com_jd_dtc_api_Server
 * Method:    SetTableName
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_SetTableName(JNIEnv *env, jobject obj, jstring name)
{
	TTC::Server *java_dtc_server = (TTC::Server*)getServerObject(env, obj);
	if (java_dtc_server == NULL)
	{
		return -EC_SERVER_OBJECT_NULL;
	}

	const char *c_table_name = env->GetStringUTFChars(name, NULL);
	int ret = java_dtc_server->SetTableName(c_table_name);
    env->ReleaseStringUTFChars(name, c_table_name);
    //std::cout<<"set table name ok"<<std::endl;

    return ret;
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    SetAddress
 * Signature: (Ljava/lang/String;Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_SetAddress(
	JNIEnv *env,jobject obj, jstring host, jstring port) {

	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return -EC_SERVER_OBJECT_NULL;
	}
	const char *c_host = env->GetStringUTFChars(host, NULL);
	const char *c_port = env->GetStringUTFChars(port, NULL);
	int ret = java_dtc_server->SetAddress(c_host, c_port);
	env->ReleaseStringUTFChars(host, c_host);
	env->ReleaseStringUTFChars(port, c_port);

	return ret;
}



/*
 * Class:     com_jd_dtc_api_Server
 * Method:    CloneServer
 * Signature: (Lcom/jd/dtc/api/Server;)V
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Server_CloneServer
  (JNIEnv *env, jobject dstobj, jobject srcobj ){
	TTC::Server *src_java_dtc_server = (TTC::Server*) getServerObject(env, srcobj);
	if (src_java_dtc_server == NULL)
		return -EC_SERVER_OBJECT_NULL;

	return (jlong)new TTC::Server(*src_java_dtc_server);

}








/*
 * Class:     com_jd_dtc_api_Server
 * Method:    IntKey
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_IntKey
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return -EC_SERVER_OBJECT_NULL;
	}
	return java_dtc_server->IntKey();
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    BinaryKey
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_BinaryKey
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return -EC_SERVER_OBJECT_NULL;
	}

	return java_dtc_server->BinaryKey();
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    StringKey
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_StringKey
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return -EC_SERVER_OBJECT_NULL;
	}

	return java_dtc_server->StringKey();
}






/*
 * Class:     com_jd_dtc_api_Server
 * Method:    SetTimeout
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_SetTimeout
  (JNIEnv *env, jobject obj, jint timeout){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server != NULL)
		java_dtc_server->SetTimeout(timeout);
}









/*
 * Class:     com_jd_dtc_api_Server
 * Method:    CheckPacketSize
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_CheckPacketSize
  (JNIEnv *, jobject, jstring, jint);

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    SetAccessKey
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_SetAccessKey
  (JNIEnv *env, jobject obj, jstring accessToken){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server != NULL) {
		const char *c_token = env->GetStringUTFChars(accessToken, NULL);
		java_dtc_server->SetAccessKey(c_token);
		env->ReleaseStringUTFChars(accessToken, c_token);
	}

}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    GetCurrentUSeconds
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_com_jd_dtc_api_Server_GetCurrentUSeconds
  (JNIEnv *env, jobject obj){
/*
	  win_time_val_t wintv;  
	  win_time_t wintime;  
	  win_gettimeofday(&wintv); 
	  return wintv.sec*1000000+wintv.msec;
*/
	  return 0;
}

