//
// Created by liuyang on 2020/8/1.
//

#ifndef MY_APPLICATION_BASEFFMPEG_H
#define MY_APPLICATION_BASEFFMPEG_H
#include <pthread.h>
#include "JavaCallHelper.h"
#include "AudioChannel.h"
#include "VideoChannel.h"

extern "C"{
#include "include/libavformat/avformat.h"
#include "include/libavcodec/avcodec.h"
#
}

class BaseFFmpeg {
public:
    BaseFFmpeg(const char* datasource,JavaCallHelper* callHelper);
    ~BaseFFmpeg();
    //准备1部
    void prepare();
    //准备2部 真正的核心实现
    void _prepare();
    // 开始播放
    void startPlay();
    void _startPlay();

    void setRenderFrameCallback(RenderFrameCallBack callback);
    // 暂停
    void stop();

public:
    char * datasource;
    pthread_t pid;
    pthread_t playerPid;
    pthread_t stop_pid;

    AVFormatContext *formatContext=0;
    //初始化 给予默认值
    AudioChannel *audioChannel=0;
    VideoChannel *videoChannel=0;
    RenderFrameCallBack callback;

    bool isPlaying;
public:
    JavaCallHelper *callHelper;

};


#endif //MY_APPLICATION_BASEFFMPEG_H
