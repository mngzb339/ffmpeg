//
// Created by liuyang on 2020/8/2.
//

#ifndef MY_APPLICATION_BASECHANNEL_H
#define MY_APPLICATION_BASECHANNEL_H


#include "safe_queue.h"

extern "C" {
#include <libavcodec/avcodec.h>
};

class BaseChannel {
public:
    BaseChannel(int id, AVCodecContext *context, AVRational time_base) : id(id),
                                                                         avCodecContext(context),
                                                                         time_base(time_base) {
        frames.setReleaseCallback(releaseAVFrame);
        pakets.setReleaseCallback(BaseChannel::releasePacket);


    }

    //西沟方法 设置伟虚函数
    virtual ~BaseChannel() {
        pakets.clear();
        frames.clear();
        if (avCodecContext) {
            avcodec_close(avCodecContext);
            avcodec_free_context(&avCodecContext);
            avCodecContext = 0;
        }
    }

    //相当于抽象方法
    virtual void play() = 0;

    /**
     *
     * 释放av_packet
     */
    static void releasePacket(AVPacket **packet) {
        if (packet) {
            av_packet_free(packet);
            //修改指针的指向，
            *packet = 0;
        }
    }

    static void releaseAVFrame(AVFrame **frame) {
        if (frame) {
            av_frame_free(frame);
            //修改指针的指向，
            *frame = 0;
        }
    }

    int id;
    //编码数据包队列
    SafeQueue<AVPacket *> pakets;
    //解码数据包队列
    SafeQueue<AVFrame *> frames;

    bool isPlaying;
    AVCodecContext *avCodecContext;
    AVRational time_base;
    //时间戳
public:
    double clock;

};


#endif
