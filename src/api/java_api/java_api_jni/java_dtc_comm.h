/*
 * java_dtc_comm.h
 *
 *  Created on: Jun 13, 2014
 *      Author: seanzheng
 */
#include <jni.h>
#ifndef JAVA_DTC_COMM_H_
#define JAVA_DTC_COMM_H_
#ifdef __cplusplus
extern "C" {
#endif
jlong getServerObject(JNIEnv *, jobject);
jlong getRequestObject(JNIEnv *, jobject);
jlong getResultObject(JNIEnv *, jobject);


#ifdef __cplusplus
}
#endif
#endif /* JAVA_DTC_COMM_H_ */
