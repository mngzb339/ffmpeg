#include <jni.h>
#include <string>
#include <android/native_window_jni.h>
#include "safe_queue.h"
#include "BaseFFmpeg.h"

BaseFFmpeg *ffmpeg = 0;
JavaVM *javaVm = 0;
ANativeWindow *window = 0;
JavaCallHelper *callHelper=0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
int JNI_OnLoad(JavaVM *vm, void *unused) {
    javaVm = vm;
    return JNI_VERSION_1_6;
}

void render(uint8_t *data, int lineszie, int w, int h) {
    pthread_mutex_lock(&mutex);
    if (!window) {
        pthread_mutex_unlock(&mutex);
        return;
    }
    ANativeWindow_setBuffersGeometry(window, w,
                                     h,
                                     WINDOW_FORMAT_RGBA_8888);
    ANativeWindow_Buffer window_buffer;
    if (ANativeWindow_lock(window, &window_buffer, 0)) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&mutex);
        return;
    }
    //??rgb???dst_data
    uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
    // stride?????????RGBA? *4
    int dst_linesize = window_buffer.stride * 4;
    //???????
    for (int i = 0; i < window_buffer.height; ++i) {
        //memcpy(dst_data , data, dst_linesize);
        memcpy(dst_data + i * dst_linesize, data + i * lineszie, dst_linesize);
    }
    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&mutex);

}

extern "C"
JNIEXPORT void JNICALL
Java_com_luban_myapplication_LubanPlayer_native_1prepare(JNIEnv *env, jobject instance,
                                                         jstring date_source) {
    const char *dateSource = env->GetStringUTFChars(date_source, 0);
    callHelper= new JavaCallHelper(javaVm, env, instance);
    ffmpeg = new BaseFFmpeg(dateSource, callHelper);
    ffmpeg->setRenderFrameCallback(render);
    ffmpeg->prepare();

    env->ReleaseStringUTFChars(date_source, dateSource);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_luban_myapplication_LubanPlayer_native_1start(JNIEnv *env, jobject instance) {
    if(ffmpeg){
        ffmpeg->startPlay();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_luban_myapplication_LubanPlayer_native_1SetSurface(JNIEnv *env, jobject thiz,
                                                            jobject surface) {
    //native_Windows
    pthread_mutex_lock(&mutex);
    if (window) {
        //?????
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_luban_myapplication_LubanPlayer_native_1stop(JNIEnv *env, jobject instance) {
    if (ffmpeg) {
        ffmpeg->stop();
    }
    if(callHelper){
        delete (callHelper);
        callHelper = 0;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_luban_myapplication_LubanPlayer_native_1release(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&mutex);
    if (window) {
        //把老的释放
        ANativeWindow_release(window);
        window = 0;
    }
    pthread_mutex_unlock(&mutex);
}