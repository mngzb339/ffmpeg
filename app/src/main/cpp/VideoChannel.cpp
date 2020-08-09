//
// Created by liuyang on 2020/8/2.
//

#include "VideoChannel.h"
#include "macro.h"

extern "C" {
#include <libavutil/imgutils.h>
}

void *decode_task(void *args) {
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->decode();
    return 0;
}

/**
 * 渲染线程
 */
void *render_task(void *args) {
    VideoChannel *channel = static_cast<VideoChannel *>(args);
    channel->render();
    return 0;
}

/**
 * 1 解码
 * 2.播放
 */
VideoChannel::VideoChannel(int id, AVCodecContext *avCodecContext) : BaseChannel(id,
                                                                                 avCodecContext) {
}


VideoChannel::~VideoChannel() {
}

void VideoChannel::play() {
    isPlaying = 1;
    frames.setWork(1);
    pakets.setWork(1);
    //开线程 防止阻塞线程 先解码后播放
    pthread_create(&pid_decode, 0, decode_task, this);

    pthread_create(&pid_play, 0, render_task, this);

}

//解码
void VideoChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        //  取出一个数据包
        int ret = pakets.pop(packet);

        if (!isPlaying) {
            break;
        }
        LOGE("没有音视频startPlaydecode %d", ret);

        if (!ret) {

            continue;
        }

        ret = avcodec_send_packet(avCodecContext, packet);

        releasePacket(&packet);

        if (ret != 0) {
            break;
        }
        //拿到一帧数据（先降数据输出出来）
        LOGE("没有音视频startPlaydecode %d", ret);

        AVFrame *frame = av_frame_alloc();
        //从解码器中读取解码后的数据包
        ret = avcodec_receive_frame(avCodecContext, frame);
        //需要更多的数据才能进行解码
        if (ret == AVERROR(EAGAIN)) {
            continue;
        } else if (ret != 0) {//先统一处理为失败
            break;
        }
        //再开一个线程 来播放 方便音视频同步
        frames.push(frame);

    }
    releasePacket(&packet);
}

void VideoChannel::render() {
// 开始播放啦
    swsContext = sws_getContext(avCodecContext->width, avCodecContext->height,
                                avCodecContext->pix_fmt, avCodecContext->width,
                                avCodecContext->height, AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL,
                                NULL);
    AVFrame *frame = 0;
    uint8_t *dst_data[4];
    int dst_lineSize[4];
    av_image_alloc(dst_data, dst_lineSize,
                   avCodecContext->width, avCodecContext->height, AV_PIX_FMT_RGBA, 1);
    while (isPlaying) {
        int ret = frames.pop(frame);
        if (!isPlaying) {
            break;
        }
        //src_lineSize 每一行 存放的数据长度
        sws_scale(swsContext, reinterpret_cast<const uint8_t *const *>(frame->data),
                  frame->linesize, 0,
                  avCodecContext->height,
                  dst_data,
                  dst_lineSize);
        //回调出去进行播放
        callBack(dst_data[0], dst_lineSize[0], avCodecContext->width, avCodecContext->height);
        releaseAVFrame(&frame);
    }
    av_freep(&dst_data[0]);
    releaseAVFrame(&frame);
}

void VideoChannel::setRenderFrame(RenderFrameCallBack callBack) {
    this->callBack = callBack;
}
