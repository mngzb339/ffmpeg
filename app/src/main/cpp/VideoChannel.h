//
// Created by liuyang on 2020/8/2.
//

#ifndef MY_APPLICATION_VIDEOCHANNEL_H
#define MY_APPLICATION_VIDEOCHANNEL_H


#include "BaseChannel.h"
#include "AudioChannel.h"

extern "C" {
#include <libswscale/swscale.h>
};
typedef void (*RenderFrameCallBack)(uint8_t *,int,int,int);
class VideoChannel : public BaseChannel {
public:
    VideoChannel(int id, AVCodecContext *avCodecContext, int fps,AVRational time_base);

    ~VideoChannel();

    void setAudioChannel(AudioChannel *audioChannel);
    //播放
    void play();

    //解码
    void decode();

    //渲染
    void render();
    void setRenderFrame(RenderFrameCallBack callBack);

private:
    pthread_t pid_decode;
    pthread_t pid_play;
    SwsContext *swsContext=0;
    RenderFrameCallBack callBack;
    int fps;
    AudioChannel *audioChannel=0;
};


#endif //MY_APPLICATION_VIDEOCHANNEL_H
