//
// Created by liuyang on 2020/8/2.
//

#include "VideoChannel.h"
#include "macro.h"

extern "C" {
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
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
 * 解码前的队列
 * 丢包直到下一个关键帧
 * 至丢一组
 */
void dropAVPacket(queue<AVPacket *> &q) {
    while (!q.empty()) {
        AVPacket *packet = q.front();
        //如果不属于I帧 丢掉
        if (packet->flags != AV_PKT_FLAG_KEY) {
            BaseChannel::releasePacket(&packet);
            q.pop();
        } else {
            break;
        }
    }
}

/**
 * 一次丢一帧
 */
void dropAVFrames(queue<AVFrame *> &q) {
    if (!q.empty()) {
        AVFrame *frame = q.front();
        BaseChannel::releaseAVFrame(&frame);
        q.pop();
    }
}

/**
 * 1 解码
 * 2.播放
 */
VideoChannel::VideoChannel(int id, AVCodecContext *avCodecContext, int fps, AVRational time_base)
        : BaseChannel(id,
                      avCodecContext, time_base) {
    this->fps = fps;

    frames.setSyncHandle(dropAVFrames);
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
    //每个界面刷新的间隔
    double frame_delays = 1.0 / fps;//单位秒
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
        //获得 当前画面播放的相对时间 best_effort_timestamp更加精准 大多数和pts 值一样
        double clock = frame->best_effort_timestamp * av_q2d(time_base);
        //表示画面延迟多久显示出来（额外的延迟时间）
        double extra_delay = frame->repeat_pict / (2 * fps);
        //真实需要的间隔时间
        double delays = extra_delay + frame_delays;
        if (!audioChannel) {
            LOGE("视频快了video audioChannel：%lf", audioChannel->clock);
            av_usleep(delays * 1000000);
        } else {
            if (clock == 0) {
                LOGE("视频快了video audioChannel %lf", audioChannel->clock);
                av_usleep(delays * 1000000);
            } else {
                //比较音视频
                double audioClock = audioChannel->clock;
                if (audioChannel->clock < 0.0001) {
                    av_usleep(1 * 100000);
                    audioClock = audioChannel->clock;
                }
                LOGE("视频快了video audioChannel %d", &audioChannel);

                //音视频相差的间隔
                double diff = clock - audioClock;
                //大于0 表示视频比较快 小于0 表示音频比较快
                if (diff > 0) {
                    LOGE("视频快了：%lf", diff);
                    av_usleep((delays + diff) * 1000000);
                } else if (diff < 0) {
                    LOGE("音乐 频快了：%lf", diff);

                    //不睡了 直接往前赶；视频包有可能积压的太多了 可以考虑丢包
                    if (fabs(diff) >= 0.05) { //如果是音视频比视频快了0 .0 5秒 直接区丢包
                        releaseAVFrame(&frame);
                        // 开始丢包
                        frames.sync();
                        continue;
                    } else {
                        //表示在允许的范围内之内
                    }
                }
            }
        }
        //单位微妙
        //回调出去进行播放
        callBack(dst_data[0], dst_lineSize[0], avCodecContext->width, avCodecContext->height);
        releaseAVFrame(&frame);
    }
    av_freep(&dst_data[0]);
    releaseAVFrame(&frame);
    isPlaying=0;
    sws_freeContext(swsContext);
    swsContext=0;
}

void VideoChannel::setRenderFrame(RenderFrameCallBack callBack) {
    this->callBack = callBack;
}

void VideoChannel::setAudioChannel(AudioChannel *audioChannel1) {
    LOGE("视频快了video audioChannel 207：%d", &audioChannel1);
    audioChannel = audioChannel1;
    LOGE("视频快了video audioChannel 210：%d", &audioChannel);

}

void VideoChannel::stop() {
    isPlaying = 0;
    frames.setWork(0);
    pakets.setWork(0);
    pthread_join(pid_decode, 0);
    pthread_join(pid_play, 0);


}


