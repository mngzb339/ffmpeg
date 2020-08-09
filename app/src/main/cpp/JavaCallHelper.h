//
// Created by liuyang on 2020/8/2.
//

#ifndef MY_APPLICATION_JAVACALLHELPER_H
#define MY_APPLICATION_JAVACALLHELPER_H

#include "macro.h"

#include "../../../../../../../Library/Android/sdk/ndk/android-ndk-r17c/sysroot/usr/include/jni.h"

class JavaCallHelper {
public:

    JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instance);

    ~JavaCallHelper();
    void onError(int thread,int errorCode);

    void onPrepare(int i);

private:
    //JNIEnv 不能跨线程 所以反射java代码需要JavaVM
    JavaVM *javaVm;
    JNIEnv *env;
    jobject instance;
    jmethodID onErrorId;
    jmethodID onPrepareID;

};


#endif //MY_APPLICATION_JAVACALLHELPER_H
