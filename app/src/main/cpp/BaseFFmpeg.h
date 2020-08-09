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
private:
    char * datasource;
    pthread_t pid;
     pthread_t playerPid;

    AVFormatContext *formatContext=0;
    JavaCallHelper *callHelper;
    //初始化 给予默认值
    AudioChannel *audioChannel=0;
    VideoChannel *videoChannel=0;
    RenderFrameCallBack callback;

    bool isPlaying;
};


#endif //MY_APPLICATION_BASEFFMPEG_H
