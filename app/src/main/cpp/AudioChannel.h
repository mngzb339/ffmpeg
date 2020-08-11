//
// Created by liuyang on 2020/8/2.
//

#ifndef MY_APPLICATION_AUDIOCHANNEL_H
#define MY_APPLICATION_AUDIOCHANNEL_H

#include <SLES/OpenSLES.h>
#include <stdint.h>
#include "BaseChannel.h"
#include <SLES/OpenSLES_Android.h>

extern "C" {
//采样率相关头文件
#include <libswresample/swresample.h>
};


class AudioChannel : public BaseChannel {
public:
    AudioChannel(int id, AVCodecContext *avCodecContext, AVRational time_base);

    ~AudioChannel();

    void play();

    void stop();

    // 解码
    void decode();

    void playAudio();

    //获取PCM
    int getPcm();

public:
    //采集数据内存地址
    uint8_t *data = 0;
    int out_channels;
    int out_samplesize;
    int out_sample_rate;
private:
    pthread_t pid_audio_decode;
    pthread_t pid_audio_paly;
    //引擎对象
    SLObjectItf engineObject = 0;
    //引擎接口
    SLEngineItf engineInterface = 0;
    //混音器
    SLObjectItf outputMixObject = 0;
    //播放器
    SLObjectItf bqPlayerObject = 0;
    //播放器接口
    SLPlayItf bqPlayerInterface = 0;

    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueueInterface = 0;
    // 重新采样
    SwrContext *swrContext = 0;

};


#endif //MY_APPLICATION_AUDIOCHANNEL_H
