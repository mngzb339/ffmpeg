//
// Created by liuyang on 2020/8/2.
//
#include "AudioChannel.h"
#include "macro.h"

void *audio_decode(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return 0;
}

void *audio_play(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->playAudio();
    return 0;
}

AudioChannel::AudioChannel(int id, AVCodecContext *avCodecContext, AVRational time_base) : BaseChannel(id,
                                                                                 avCodecContext,time_base) {
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    out_samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    out_sample_rate = 44100;
    //44100个16位 *2 表示双声道
    data = static_cast<uint8_t *>(malloc(out_sample_rate * out_samplesize * out_channels));
    memset(data, 0, out_sample_rate * out_samplesize * out_channels);
}

AudioChannel::~AudioChannel() {
    if (data) {
        free(data);
        data = 0;
    }

}


void AudioChannel::play() {
    //1。解码
    //2。播放
    pakets.setWork(1);
    frames.setWork(1);
    //0+ 输出声道 输出采样位 输出采样率 输入的3哥参数
    swrContext = swr_alloc_set_opts(0, AV_CH_LAYOUT_STEREO,
                                    AV_SAMPLE_FMT_S16, 44100, avCodecContext->channel_layout,
                                    avCodecContext->sample_fmt, avCodecContext->sample_rate, 0, 0);
     swr_init(swrContext);
    //设置播放状态
    isPlaying = 1;

    pthread_create(&pid_audio_decode, 0, audio_decode, this);
    pthread_create(&pid_audio_paly, 0, audio_play, this);

}

void AudioChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        //  取出一个数据包
        int ret = pakets.pop(packet);

        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }

        ret = avcodec_send_packet(avCodecContext, packet);

        releasePacket(&packet);

        if (ret != 0) {
            break;
        }
        //拿到一帧数据（先降数据输出出来）

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

}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *context) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(context);
//    //获得pcm 数据 多少个字节 data
    int dataSize = audioChannel->getPcm();
    if (dataSize > 0) {
        // 接收16位数据
        (*bq)->Enqueue(bq, audioChannel->data, dataSize);
    }
}

/**
 * 真正开始处理视频的地方 OpenSles
 */
void AudioChannel::playAudio() {
    LOGE("播放没问题把 %d", 103);

    SLresult result;
    // 创建引擎 SLObjectItf engineObject
    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 初始化引擎
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 获取引擎接口SLEngineItf engineInterface
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    LOGE("播放没问题把 %d", 122);

    //设置混音器

    // 创建混音器SLObjectItf outputMixObject
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0,
                                                 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    // 初始化混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    LOGE("播放没问题把 %d", 137);

    // 创建播放器
    /**
     * 配置输入声音信息
     */
    //创建buffer缓冲类型的队列 2个队列
    SLDataLocator_AndroidSimpleBufferQueue android_queue = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,
                                                            2};
    //pcm数据格式
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};

    //数据源 将上述配置信息放到这个数据源中
    SLDataSource slDataSource = {&android_queue, &pcm};

    //配置音轨（输出）
    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    //音轨
    SLDataSink audioSnk = {&outputMix, NULL};
    //需要的接口
    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    LOGE("播放没问题把 %d", 162);

    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    //创建播放器
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &slDataSource,
                                          &audioSnk, 1,
                                          ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //得到接口后调用  获取Player接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);


    //4.获取播放器队列接口
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE,
                                    &bqPlayerBufferQueueInterface);
    //设置回调
    (*bqPlayerBufferQueueInterface)->RegisterCallback(bqPlayerBufferQueueInterface,
                                                      bqPlayerCallback, this);

    // 5.设置播放状态
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);
    //6 手动激活一下
    bqPlayerCallback(bqPlayerBufferQueueInterface, this);
    LOGE("播放没问题把 %d", 111);

}

/**
 * 获取pcm
 */
int AudioChannel::getPcm() {
    int data_size = 0;
    AVFrame *frame;
    int ret = frames.pop(frame);
    if (!isPlaying) {
        if (ret) {
            releaseAVFrame(&frame);
        }
        return data_size;
    }
    // 重采样成44100
    //delays 加入我们输入了10哥数据 swrContext 转码器 这一次处理了8个 下次处理时候需要把上次没处理完的2个也要加入继续处理
    // 如果不出来 会造成积压 数据越积压越多 会导致堆崩掉
    int64_t delays = swr_get_delay(swrContext, frame->sample_rate);
    //将nb_samples 由采样率 sample_rate 转化成 44100 后返回多少个数据
    int64_t maxSample = av_rescale_rnd(delays + frame->nb_samples, out_sample_rate, frame->sample_rate, AV_ROUND_UP);
    // 上下文+输出反冲区+输出反冲区+ 能接受的最大数据量+ 输入数据 +输入数据个数
    int samples = swr_convert(swrContext, &data, maxSample, (const uint8_t **)frame->data, frame->nb_samples);
    // 真正转出的数据samples（单位是 44100*2(声道2个)）
    // 获取自己大小
    data_size = samples* out_samplesize * out_channels;//得到了多少个字节
    //获取音视频相对播放时间
    //获取相对播放这一段数据的的秒数
    clock = frame->pts*av_q2d(time_base);
    //LOGE("视频快了AudioChannel clock：%d",);

    return data_size;
}
void AudioChannel::stop() {
    isPlaying = 0;
    frames.setWork(0);
    pakets.setWork(0);

    pthread_join(pid_audio_decode, 0);
    pthread_join(pid_audio_paly, 0);
    if(swrContext){
        swr_free(&swrContext);
        swrContext=0;
    }
    //释放播放器
    if(bqPlayerObject){
        (*bqPlayerObject)->Destroy(bqPlayerObject);
        bqPlayerObject = 0;
        bqPlayerInterface = 0;
        bqPlayerBufferQueueInterface = 0;
    }

    //释放混音器
    if(outputMixObject){
        (*outputMixObject)->Destroy(outputMixObject);
        outputMixObject = 0;
    }

    //释放引擎
    if(engineObject){
        (*engineObject)->Destroy(engineObject);
        engineObject = 0;
        engineInterface = 0;
    }


}
