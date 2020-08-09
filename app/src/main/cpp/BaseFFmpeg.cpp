//
// Created by liuyang on 2020/8/1.
//
#include <cstring>
#include "BaseFFmpeg.h"
#include "JavaCallHelper.h"
#include "include/libavutil/avutil.h"

void *task_prepare(void *args) {

    BaseFFmpeg *fFmpeg = static_cast<BaseFFmpeg *>(args);
    fFmpeg->_prepare();
    // 线程函数一定要return 不然会出错 你也会查不到错误在哪里
    return 0;

}

BaseFFmpeg::BaseFFmpeg(const char *datasource, JavaCallHelper *callHelper) {
    //防止datasource 指向的内存被释放
    // strlen 获取字符串不包含\0
    this->datasource = new char[strlen(datasource) + 1];
    this->callHelper = callHelper;
    strcpy(this->datasource, datasource);
}

BaseFFmpeg::~BaseFFmpeg() {
    //释放
    delete datasource;
    datasource = 0;
    DELETE(datasource);
    DELETE(callHelper);
}

/**
 * 调用ffmpeg 搞事情
 */
void BaseFFmpeg::prepare() {
    //创建一个线程

    pthread_create(&pid, 0, task_prepare, this);

}


void BaseFFmpeg::_prepare() {

    //初始化ffmpeg 使用网络
    avformat_network_init();
    //结构体包含了视频的信息宽高等信息 1.拿到formatContext
    formatContext = 0;
    //文件路径不对 手机没网都有可能导致失败
    int ret = avformat_open_input(&formatContext, datasource, 0, 0);

    if (ret != 0) {//ret ==0表示成功
        LOGE("打开媒体失败:%s", av_err2str(ret));
        callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_OPEN_URL);
        return;
    }
    //查找媒体中的音视频流
    ret = avformat_find_stream_info(formatContext, 0);
    //小于0 则失败

    if (ret < 0) {
        LOGE("查找流失败:%s", av_err2str(ret));

        callHelper->onError(THREAD_CHILD, FFMPEG_CAN_NOT_FIND_STREAMS);
        return;

    }
    //av_Streams 数量正常的视频最少有两个 av_Streams
    //经过avformat_find_stream_info formatContext->streams[也就有值了

    for (int i = 0; i < formatContext->nb_streams; ++i) {
        AVStream *stream = formatContext->streams[i];
        //包含了解码 这段流的各种信息
        AVCodecParameters *codecPar = stream->codecpar;
        //通用处理
        //1。获取解码器（查找当前流的编码方式） 2。
        AVCodec *dec = avcodec_find_decoder(codecPar->codec_id);
        if (dec == NULL) {
            LOGE("查找解码器失败:%s", av_err2str(ret));

            callHelper->onError(THREAD_CHILD, FFMPEG_FIND_DECODER_FAIL);
            return;
        }
        //2.获得解码器上下文
        AVCodecContext *context = avcodec_alloc_context3(dec);
        if (context == NULL) {
            LOGE("创建解码上下文失败:%s", av_err2str(ret));

            callHelper->onError(THREAD_CHILD, FFMPEG_ALLOC_CODEC_CONTEXT_FAIL);
            return;
        }
        //3. 设置上下文一些参数
        ret = avcodec_parameters_to_context(context, codecPar);
        if (ret < 0) {
            LOGE("设置解码上下文参数失败:%s", av_err2str(ret));

            callHelper->onError(THREAD_CHILD, FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL);
            return;
        }
        //4. 打开解码器
        ret = avcodec_open2(context, dec, 0);
        if (ret != 0) {
            LOGE("打开解码器失败:%s", av_err2str(ret));
            callHelper->onError(THREAD_CHILD, FFMPEG_OPEN_DECODER_FAIL);
            return;
        }
        //单位This is the fundamental unit of time (in seconds) in terms
        AVRational time_base = stream->time_base;
        if (codecPar->codec_type == AVMEDIA_TYPE_AUDIO) {//音频
            audioChannel = new AudioChannel(i, context, time_base);
        } else if (codecPar->codec_type == AVMEDIA_TYPE_VIDEO) {//视频
            //帧率：单位时间内 显示多少个图像
            AVRational frame_rate = stream->avg_frame_rate;
            int fps = av_q2d(frame_rate);
            videoChannel = new VideoChannel(i, context, fps, time_base);
            videoChannel->setRenderFrame(callback);

        }

    }
    //没有音视频 很少见

    if (audioChannel != 0 && !videoChannel != 0) {
        LOGE("没有音视频");

        callHelper->onError(THREAD_CHILD, FFMPEG_NOMEDIA);
        return;
    }
    callHelper->onPrepare(THREAD_CHILD);
}

void *task_play(void *args) {
    BaseFFmpeg *fFmpeg = static_cast<BaseFFmpeg *>(args);
    fFmpeg->_startPlay();
    return 0;

}

void BaseFFmpeg::startPlay() {

    // 正在播放
    isPlaying = true;
    //设置伟工作状态
    if (audioChannel) {
        audioChannel->play();
    }
    if (videoChannel) {
        videoChannel->setAudioChannel(audioChannel);
        videoChannel->play();
    }

    pthread_create(&playerPid, 0, task_play, this);
}

/**
 *
 * 专门读取数据包
 */
void BaseFFmpeg::_startPlay() {
    // 1.读取音视频数据包
    int ret;
    while (isPlaying) {
        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(formatContext, packet);
        if (ret == 0) {//ret==0 表示成功
            //stream_index 流的序号
            if (audioChannel && packet->stream_index == audioChannel->id) {
                audioChannel->pakets.push(packet);
            } else if (videoChannel && packet->stream_index == videoChannel->id) {
                videoChannel->pakets.push(packet);

            }
        } else if (ret == AVERROR_EOF) {
            //读取完成 但是可能没有播放完成

        } else {

        }
    }
    // 2.解码

}

void BaseFFmpeg::setRenderFrameCallback(RenderFrameCallBack callBack1) {
    this->callback = callBack1;

}
