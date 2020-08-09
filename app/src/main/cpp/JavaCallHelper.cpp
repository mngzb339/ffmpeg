//
// Created by liuyang on 2020/8/2.
//
/**
 * Java反射帮助类
 */
#include "JavaCallHelper.h"

JavaCallHelper::JavaCallHelper(JavaVM *vm, JNIEnv *env, jobject instance) {
    this->javaVm = vm;
    //如果在主线程
    this->env = env;
    //一旦涉及搭配jobject 跨方法和线程就需要全局引用
    this->instance = env->NewGlobalRef(instance);
    jclass jclass1 = env->GetObjectClass(instance);
    onErrorId = env->GetMethodID(jclass1, "onError", "(I)V");
    onPrepareID = env->GetMethodID(jclass1, "onPrepare", "()V");

}

JavaCallHelper::~JavaCallHelper() {
    env->DeleteGlobalRef(instance);
}

void JavaCallHelper::onError(int thread, int errorCode) {
    if (thread == THREAD_MAIN) {//如果是主线程
        env->CallVoidMethod(instance, onErrorId);
    } else {
        //子线程
        JNIEnv * threadEnv;
        //获取属于当前线程的jnienv;
        javaVm->AttachCurrentThread(&threadEnv,0);
        threadEnv->CallVoidMethod(instance, onErrorId);
        javaVm->DetachCurrentThread();
    }

}


/**
 * 回调java  准备好了
 */
void JavaCallHelper::onPrepare(int thread) {

    if (thread == THREAD_MAIN) {//如果是主线程
        env->CallVoidMethod(instance, onPrepareID);
    } else {
        //子线程
        JNIEnv * threadEnv;
        //获取属于当前线程的jnienv;
        javaVm->AttachCurrentThread(&threadEnv,0);
        threadEnv->CallVoidMethod(instance, onPrepareID);
        javaVm->DetachCurrentThread();
    }
}


