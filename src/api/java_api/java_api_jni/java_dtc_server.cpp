#include <jni.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include <sys/time.h>
#include "com_jd_dtc_api_Server.h"
#include "include/ttcapi.h"
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
 * Method:    CloneTabDef
 * Signature: (Lcom/jd/dtc/api/Server;)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_CloneTabDef
  (JNIEnv *env, jobject dstobj, jobject srcobj ){
	TTC::Server *src_java_dtc_server = (TTC::Server*)getServerObject(env, srcobj);
	TTC::Server *dst_java_dtc_server = (TTC::Server*)getServerObject(env, dstobj);
	if (dst_java_dtc_server == NULL || src_java_dtc_server == NULL)
		return;

	dst_java_dtc_server->CloneTabDef(*src_java_dtc_server);
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
 * Method:    SetCompressLevel
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_SetCompressLevel
  (JNIEnv *env, jobject obj, jint level){

	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server != NULL)
		java_dtc_server->SetCompressLevel(level);
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    GetAddress
 * Signature: ()Ljava/lang/String;
*/
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Server_GetAddress(JNIEnv *env, jobject obj)
{
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL)
		return NULL;
	const char *c_addr = java_dtc_server->GetAddress();

	return env->NewStringUTF(c_addr);
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    GetTableName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Server_GetTableName
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
			return NULL;
	}
	const char *c_name = java_dtc_server->GetTableName();
	return env->NewStringUTF(c_name);
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    GetServerAddress
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Server_GetServerAddress
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return NULL;
	}
	const char *c_addr = java_dtc_server->GetServerAddress();

	return env->NewStringUTF(c_addr);
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    GetServerTableName
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Server_GetServerTableName
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return NULL;
	}
	const char *c_name = java_dtc_server->GetServerTableName();

	return env->NewStringUTF(c_name);
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
 * Method:    AddKey
 * Signature: (Ljava/lang/String;I)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_AddKey
  (JNIEnv *env, jobject obj, jstring name, jint type ){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return -EC_SERVER_OBJECT_NULL;
	}

	const char *c_name = env->GetStringUTFChars(name, NULL);
	int ret = java_dtc_server->AddKey(c_name, type);
	env->ReleaseStringUTFChars(name, c_name);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    FieldType
 * Signature: (Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_FieldType
  (JNIEnv *env, jobject obj, jstring name){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return -EC_SERVER_OBJECT_NULL;
	}
	const char *c_name = env->GetStringUTFChars(name, NULL);
	int ret = java_dtc_server->FieldType(c_name);
	env->ReleaseStringUTFChars(name, c_name);

	return ret;
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    ErrorMessage
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_com_jd_dtc_api_Server_ErrorMessage
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return NULL;
	}
	const char* c_msg = java_dtc_server->ErrorMessage();

	return env->NewStringUTF(c_msg);
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
 * Method:    SetMTimeout
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_SetMTimeout
  (JNIEnv *env, jobject obj, jint m_timeout){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server != NULL)
		java_dtc_server->SetMTimeout(m_timeout);
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    Connect
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_Connect
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL) {
		return -EC_SERVER_OBJECT_NULL;
	}
	return java_dtc_server->Connect();
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    Close
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_Close
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server != NULL)
		java_dtc_server->Close();
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    Ping
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_jd_dtc_api_Server_Ping
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL)
		return -EC_SERVER_OBJECT_NULL;

	return java_dtc_server->Ping();
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    AutoPing
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_AutoPing
  (JNIEnv *env, jobject obj){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server != NULL)
		java_dtc_server->AutoPing();
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    SetFD
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_SetFD
  (JNIEnv *env, jobject obj, jint fd){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server != NULL)
		java_dtc_server->SetFD(fd);
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    SetAutoUpdateTab
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_SetAutoUpdateTab
  (JNIEnv *env, jobject obj, jboolean auto_update){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server != NULL)
		java_dtc_server->SetAutoUpdateTab(auto_update);
}

/*
 * Class:     com_jd_dtc_api_Server
 * Method:    SetAutoReconnect
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_jd_dtc_api_Server_SetAutoReconnect
  (JNIEnv *env, jobject obj, jint auto_connect){
	TTC::Server *java_dtc_server = (TTC::Server*) getServerObject(env, obj);
	if (java_dtc_server == NULL)
		java_dtc_server->SetAutoReconnect(auto_connect);
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
	struct timeval now;
	gettimeofday(&now,0);

	return now.tv_sec*1000000+now.tv_usec;
}

