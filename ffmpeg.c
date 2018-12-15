//
//  ffmpeg.c
//  
//
//  Created by mac on 2018/11/27.
//

#include "ffmpeg.h"
/**
 解封装

 @param infilename 输入多媒体文件名
 @param[out] out_input_format_context 输入文件的多媒体格式上下文
 @param[out] out_audio_stream_idx 音频流索引
 @param[out] out_video_stream_idx 视频流索引
 @param[out] out_m_audiomuxtimebasetrue 音频的mux层timebse是否正确 -1 不正确 0 正确
 @return 0:成功
 */
int init_demux(const char *infilename,AVFormatContext **out_input_format_context,int *out_audio_stream_idx,int *out_video_stream_idx,int *out_m_audiomuxtimebasetrue) {
    int i = 0;
    int audio_stream_idx = -1;
    AVFormatContext *input_format_context = NULL;
    int ret = 0;
    if(infilename == NULL || out_audio_stream_idx == NULL || out_video_stream_idx == NULL || out_m_audiomuxtimebasetrue == NULL){
        ret = -1;
        printf("init_demux 参数不能为NULL!\n");
        goto end;
    }
 
    ret = avformat_open_input(out_input_format_context,infilename,NULL, NULL);
    if (ret < 0)
    {
        printf("Call avformat_open_input function failed!\n");
        goto end;
    }
    input_format_context = *out_input_format_context;
    ret = avformat_find_stream_info(input_format_context,NULL);
    if (ret < 0)
    {
        printf("Call av_find_stream_info function failed!\n");
        goto end;
    }

    //输出视频信息
    av_dump_format(input_format_context,-1, infilename, 0);

    //添加音频信息到输出context
    for (i = 0; i < input_format_context->nb_streams; i++)
    {
        if (input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            *out_video_stream_idx = i;
        }
        else if (input_format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            audio_stream_idx = i;
            *out_audio_stream_idx = audio_stream_idx;
            if(input_format_context->streams[audio_stream_idx]->time_base.den == 0 ||
               input_format_context->streams[audio_stream_idx]->time_base.num == 0 ||
               input_format_context->streams[audio_stream_idx]->codecpar->sample_rate == 0)
            {
                *out_m_audiomuxtimebasetrue = -1;
            }
        }
    }

end:
    return ret;
}

/**
 释放格式上下文

 @param formatContext formatContext description
 @return return value description
 */
int uinit_demux(AVFormatContext *input_format_context)
{
    int ret = 0;
    if(input_format_context == NULL){
        ret = -1;
        printf("uinit_demux 参数不能为空!\n");
        goto end;
    }
    /* free the stream */
    av_free(input_format_context);
end:
    return ret;
}


/**
 初始化音频解码器

 @param input_format_context 输入文件格式上下文
 @param audio_stream_idx 音频流索引
 @param[out] out_audio_decode_codec_ctx 音频解码器上下文
 @param[out] out_audio_decode_codec 音频解码器
 @return 0: success
 */
int init_audio_decode(AVFormatContext *input_format_context,int audio_stream_idx,AVCodecContext **out_audio_decode_codec_ctx,AVCodec **out_audio_decode_codec){
    int ret = 0;
    AVCodecContext* audio_decode_codec_ctx = NULL;
    AVCodec *audio_decode_codec = NULL;
    AVStream *audio_stream = NULL;
    if(input_format_context == NULL){
        ret = -1;
        printf("init_audio_decode 参数不能为空！\n");
        goto end;
    }
    
    audio_stream = input_format_context->streams[audio_stream_idx];
    audio_decode_codec = avcodec_find_decoder(audio_stream->codecpar->codec_id);
    if (audio_decode_codec == NULL)
    {
        printf("没有找到对应的音频解码器\n");
        ret = -1;
        goto end;
    }
    *out_audio_decode_codec = audio_decode_codec;
    audio_decode_codec_ctx = avcodec_alloc_context3(audio_decode_codec);
    if(audio_decode_codec_ctx == NULL){
        printf("avcodec_alloc_context3 failed!\n");
        ret = -1;
        goto end;
    }
    *out_audio_decode_codec_ctx = audio_decode_codec_ctx;
    //设置音频解码器的相关参数
    avcodec_parameters_to_context(audio_decode_codec_ctx,audio_stream->codecpar);
    //设置音频解码器的时间基
    audio_decode_codec_ctx->time_base = (AVRational){1,audio_stream->codecpar->sample_rate};
    //打开音频解码器
    ret = avcodec_open2(audio_decode_codec_ctx, audio_decode_codec, NULL);
    if (ret < 0)
    {
        printf("Could not open audio decoder\n");
        goto end;
    }
end:
    return ret;
}

/**
 音频解码
 @param audio_avcodec_ctx 音频解码器
 @param audio_pkt 等待解码的AVPacket
 @param[out] out_audio_frame 解码器后的AVFrame
 @return 0 成功
 */
int perform_audio_decode(AVCodecContext *audio_decode_codec_ctx,AVPacket audio_pkt,AVFrame *out_audio_frame) {
    int ret = 0;
    ret = avcodec_send_packet(audio_decode_codec_ctx,&audio_pkt);
    if(audio_decode_codec_ctx == NULL || out_audio_frame == NULL){
        ret = -1;
        printf("perform_audio_decode 参数不能为空!\n");
        goto end;
    }
    if(ret == AVERROR(EAGAIN)){
        
    }
    else if (ret < 0) {
        printf("avcodec_send_packet failed!\n");
        goto end;
    }
    ret = avcodec_receive_frame(audio_decode_codec_ctx,out_audio_frame);
    if (ret == AVERROR(EAGAIN)){
        goto end;
    }
    if(ret < 0)
    {
        printf("avcodec_receive_frame failed!\n");
        goto end;
    }
end:
    return ret;
}


/**
 释放音频解码相关环境

 @param audio_decode_codec_ctx 音频解码器上下文
 @return 0:成功
 */
int uinit_audio_decode(AVCodecContext* audio_decode_codec_ctx)
{
    int ret = 0;
    if(audio_decode_codec_ctx == NULL){
        printf("uinit_audio_decode 参数不能为空!\n");
        ret = -1;
        goto end;
    }
    avcodec_close(audio_decode_codec_ctx);
end:
    return ret;
}


/**
 重采样初始化

 @param audio_decode_codec_ctx 音频解码器
 @param out_sample_fmt 目标采样格式
 @param out_channels 目标声道数
 @param out_sample_rate 目标采样率
 @return NULL失败
 */
SwrContext *init_pcm_resample(AVCodecContext *audio_decode_codec_ctx,
                              int dst_sample_fmt,int dst_channels ,int dst_sample_rate) {
    SwrContext *swr_ctx = NULL;
    enum AVSampleFormat sample_fmt;
    int out_channel_layout;
    if(audio_decode_codec_ctx == NULL){
        printf("init_pcm_resample:参数不能为空!\n");
        goto end;
    }
    
    swr_ctx = swr_alloc();
    if (swr_ctx == NULL)
    {
        printf("swr_alloc error \n");
        goto end;
    }
    sample_fmt = dst_sample_fmt; //样本
    out_channel_layout = av_get_default_channel_layout(dst_channels);
    if (audio_decode_codec_ctx->channel_layout == 0)
    {
        audio_decode_codec_ctx->channel_layout = av_get_default_channel_layout(2);
    }

    av_opt_set_int(swr_ctx, "in_channel_layout",    audio_decode_codec_ctx->channel_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate",       audio_decode_codec_ctx->sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", audio_decode_codec_ctx->sample_fmt, 0);
    av_opt_set_int(swr_ctx, "out_channel_layout",   out_channel_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate",       dst_sample_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);
    swr_init(swr_ctx);

end:
    return swr_ctx;
}


/**
 音频重采样

 @param swr_ctx 重采样上下文
 @param audio_decode_codec_ctx 音频解码器上下文
 @param audio_decode_frame 解码后的AVFrame
 @param dst_sample_fmt 目标采样格式
 @param dst_channels 目标声道数
 @param resample_audio_frame 重采样之后的frame
 @param audiofifo 重采样之后存储数据的audiofifo
 @return 大于等于0 表示成功
 */
int perform_pcm_resample(SwrContext *swr_ctx,AVCodecContext *audio_decode_codec_ctx,AVFrame *audio_decode_frame,
                         enum AVSampleFormat dst_sample_fmt,int dst_channels,AVFrame *resample_audio_frame,AVAudioFifo *audiofifo) {
    
    int ret = 0;
    int samples_out_per_size = 0; //转换之后的samples大小
    if(swr_ctx == NULL || audio_decode_codec_ctx == NULL || audio_decode_frame == NULL || resample_audio_frame == NULL || audiofifo == NULL){
        ret = -1;
        printf("perform_pcm_resample 参数不能为空!\n");
        goto end;
    }
    //这里注意下samples_out_per_size这个值和 out_frame->nb_samples这个值有时候不一样，ffmpeg里面做了策略不是问题。
    //进行重采样
    samples_out_per_size = swr_convert(swr_ctx, resample_audio_frame->data, resample_audio_frame->nb_samples,
                                       (const uint8_t**)audio_decode_frame->data, audio_decode_frame->nb_samples);
    if (samples_out_per_size < 0)
    {
        ret = -1;
        printf("swr_convert error!\n");
        goto end;
    }
    
    int buffersize_in = av_samples_get_buffer_size(&audio_decode_frame->linesize[0],audio_decode_codec_ctx->channels,
                                                   audio_decode_frame->nb_samples, audio_decode_codec_ctx->sample_fmt, 1);
    if(buffersize_in < 0){
        ret = -1;
        printf("perform_pcm_resample av_samples_get_buffer_size error!\n");
        goto end;
    }
    
    //修改分包内存
    int buffersize_out = av_samples_get_buffer_size(&resample_audio_frame->linesize[0], dst_channels,
                                                    samples_out_per_size, dst_sample_fmt, 1);
    if(buffersize_out < 0){
        ret = -1;
        printf("perform_pcm_resample av_samples_get_buffer_size error!\n");
        goto end;
    }
    
    int fifo_size = av_audio_fifo_size(audiofifo);
    fifo_size = av_audio_fifo_realloc(audiofifo, av_audio_fifo_size(audiofifo) + resample_audio_frame->nb_samples);
    if(fifo_size < 0){
        ret = -1;
        printf("av_audio_fifo_realloc failed!\n");
        goto end;
    }
    ret = av_audio_fifo_write(audiofifo,(void **)resample_audio_frame->data,samples_out_per_size);
    if(ret < 0){
        printf("av_audio_fifo_write failed!\n");
        goto end;
    }
    
    resample_audio_frame->pkt_pts = audio_decode_frame->pkt_pts;
    resample_audio_frame->pkt_dts = audio_decode_frame->pkt_dts;
    //有时pkt_pts和pkt_dts不同，并且pkt_pts是编码前的dts,这里要给avframe传入pkt_dts而不能用pkt_pts
    //out_frame->pts = out_frame->pkt_pts;
    resample_audio_frame->pts = audio_decode_frame->pkt_dts;
    
end:
    return ret;
}


/**
 初始化音频编码器

 @param audio_codec_id 编码器ID
 @param dst_sample_rate 目标采样率
 @param dst_channels 目标声道数
 @param dst_audio_frame_size 目标frame_size aac是1024 mp3是1152
 @param dst_sample_fmt 目标采样格式
 @param[out] out_audio_encode_codec_ctx 音频编码器上下文
 @param[out] out_audio_encode_codec 音频编码器
 @return 0:成功
 */
int init_audio_encode(enum AVCodecID audio_codec_id,int dst_sample_rate,int dst_channels,int dst_audio_frame_size,enum AVSampleFormat dst_sample_fmt,AVCodecContext **out_audio_encode_codec_ctx,AVCodec **out_audio_encode_codec){
    int ret = 0;
    AVCodec* audio_encode_codec = NULL;
    AVCodecContext *audio_encode_codec_ctx = NULL;
    
    //新版ffmpeg不支持AV_SAMPLE_FMT_S16，所以使用libfdk_aac，需要先把libfdk_aac这个库集成进来
    if(dst_sample_fmt == AV_SAMPLE_FMT_S16 && audio_codec_id == AV_CODEC_ID_AAC){
        audio_encode_codec = avcodec_find_encoder_by_name("libfdk_aac");
    }else{
        audio_encode_codec = avcodec_find_encoder(audio_codec_id);
    }
   
    if(audio_encode_codec == NULL){
        printf("avcodec_find_encoder failed!\n");
        ret = -1;
        goto end;
    }
    audio_encode_codec_ctx = avcodec_alloc_context3(audio_encode_codec);
    if(audio_encode_codec_ctx == NULL){
        printf("audio_encode_codec_ctx failed!\n");
        ret = -1;
        goto end;
    }
    if(dst_sample_fmt == AV_SAMPLE_FMT_S16 && audio_codec_id == AV_CODEC_ID_AAC){
         audio_encode_codec_ctx->codec_id = AV_CODEC_ID_NONE;
    }else{
         audio_encode_codec_ctx->codec_id = audio_codec_id;
    }
    
    //参数配置
    audio_encode_codec_ctx->codec_type = AVMEDIA_TYPE_AUDIO;
    audio_encode_codec_ctx->sample_rate = dst_sample_rate;
    audio_encode_codec_ctx->channels = dst_channels;
    audio_encode_codec_ctx->channel_layout = av_get_default_channel_layout(dst_channels);
    //这个码率有些编码器不支持特别大，例如wav的码率是1411200 比aac大了10倍多
    audio_encode_codec_ctx->bit_rate = 128000;
    audio_encode_codec_ctx->frame_size = dst_audio_frame_size;
    audio_encode_codec_ctx->sample_fmt  = dst_sample_fmt; //样本
    audio_encode_codec_ctx->time_base =(AVRational){1,dst_sample_rate}; //设置编码器的时间基
//    audio_encode_codec_ctx->block_align = 0;
    //打开编码器
    ret = avcodec_open2(audio_encode_codec_ctx, audio_encode_codec, NULL);
    if (ret < 0)
    {
        printf("Could not open encoder\n");
        goto end;
    }
    
    *out_audio_encode_codec_ctx = audio_encode_codec_ctx;
    *out_audio_encode_codec = audio_encode_codec;

end:
    return ret;
}


/**
 音频编码
 @param audio_encode_codec_ctx 音频编码上下文
 @param audio_frame 需要编码的AVFrame
 @param out_audio_pkt 已经编码的AVPacket
 @return 0 成功
 */
int perform_audio_encode(AVCodecContext *audio_encode_codec_ctx,AVFrame* audio_frame,AVPacket *out_audio_pkt) {
    int ret = 0;
    if(audio_encode_codec_ctx == NULL || audio_frame == NULL || out_audio_pkt == NULL){
        printf("perform_audio_encode 参数不能为空!\n");
        ret = -1;
        goto end;
    }
    /* send the frame for encoding */
    ret = avcodec_send_frame(audio_encode_codec_ctx, audio_frame);
    if(ret == AVERROR(EAGAIN)){

    }
    if (ret < 0) {
        printf("Error sending the frame to the encoder\n");
        goto end;
    }
    /* read all the available output packets (in general there may be any
     * number of them */
    ret = avcodec_receive_packet(audio_encode_codec_ctx, out_audio_pkt);
    if (ret == AVERROR(EAGAIN)){
        goto end;
    }
    else if (ret < 0) {
        printf("Error encoding audio frame\n");
        goto end;
    }

end:
    return ret;
}

/**
 向封装格式中写入音频
 
 @param pkt 要写入的AVPacket
 @param output_format_context 输出格式上下文
 @param audio_stream_idx 音频流索引
 @param in_audio_stream 输入音频流
 @param dst_audio_stream 输出音频流
 @return 0: success
 */
int write_audio_frame(AVPacket *pkt,AVFormatContext *output_format_context,int audio_stream_idx,AVStream *in_audio_stream,AVStream *dst_audio_stream) {
    
    int ret = 0;
    if(in_audio_stream == NULL || dst_audio_stream == NULL || output_format_context == NULL || pkt == NULL){
        ret = -1;
        printf("write_audio_frame failed!\n");
        goto end;
    }
    pkt->pts = av_rescale_q_rnd(pkt->pts,in_audio_stream->time_base,dst_audio_stream->time_base, AV_ROUND_NEAR_INF);
    //dts用于解码
    pkt->dts = av_rescale_q_rnd(pkt->dts,in_audio_stream->time_base,dst_audio_stream->time_base, AV_ROUND_NEAR_INF);
    pkt->duration = av_rescale_q(pkt->duration,in_audio_stream->time_base,dst_audio_stream->time_base);
    
    pkt->pos = -1;
    pkt->stream_index = audio_stream_idx;
    ret = av_interleaved_write_frame(output_format_context, pkt);
    if (ret < 0) {
        printf("Error muxing packet!\n");
        goto end;
    }
    
end:
    return ret;
}


/**
 释放音频编码器相关资源

 @param audio_encode_codec_ctx 音频编码器上下文
 @return 0:成功
 */
int uinit_audio_encode(AVCodecContext* audio_encode_codec_ctx)
{
    int ret = 0;
    if(audio_encode_codec_ctx == NULL){
        printf("uinit_audio_encode 参数不能为空!\n");
        ret = -1;
        goto end;
    }
    avcodec_close(audio_encode_codec_ctx);
end:
    return ret;
}


/**
 初始化视频解码器

 @param input_format_context 输入文件格式上下文
 @param video_stream_idx 视频流索引
 @param[out] out_video_decode_codec_ctx 视频解码器上下文
 @param[out] out_video_decode_codec 视频解码器
 @return 0:success
 */
int init_video_decode(AVFormatContext *input_format_context,int video_stream_idx,AVCodecContext **out_video_decode_codec_ctx,AVCodec **out_video_decode_codec) {
    int ret = 0;
    int frame_rate = 0;
    AVCodecContext* video_deocde_codec_ctx = NULL;
    AVCodec *video_decode_codec = NULL;
    if(input_format_context == NULL){
        ret = -1;
        printf("init_video_decode: 参数不能为空!\n");
        goto end;
    }
    AVStream *video_stream = input_format_context->streams[video_stream_idx];
    //查找视频解码器
    video_decode_codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
    if (video_decode_codec == NULL)
    {
        printf("init_video_decode avcodec_find_decoder error!\n");
        ret = -1;
        goto end;
    }
    *out_video_decode_codec = video_decode_codec;
    video_deocde_codec_ctx = avcodec_alloc_context3(video_decode_codec);
    if(video_deocde_codec_ctx == NULL){
        printf("init_video_decode avcodec_alloc_context3 error!\n");
        ret = -1;
        goto end;
    }
    *out_video_decode_codec_ctx = video_deocde_codec_ctx;
    avcodec_parameters_to_context(video_deocde_codec_ctx,video_stream->codecpar);
    //计算视频帧率
    frame_rate = (int)(video_stream->r_frame_rate.num/(double)video_stream->r_frame_rate.den);
    video_deocde_codec_ctx->framerate = (AVRational){1,frame_rate}; //设置时间基
    //打开解码器
    ret = avcodec_open2(video_deocde_codec_ctx, video_decode_codec, NULL);
    if (ret < 0)
    {
        printf("init_video_decode Could not open decoder\n");
        goto end;
    }
end:
    return ret;
}


/**
 执行视频解码
 
 @param video_decode_codec_ctx 视频解码器上下文
 @param pkt 解码之前的AVPacket
 @param[out] out_video_frame 解码之后存放数据的AVFrame
 @return 0 success
 */
int perform_video_decode(AVCodecContext *video_decode_codec_ctx,AVPacket pkt,AVFrame *out_video_frame) {
    int ret = 0;
    if(video_decode_codec_ctx == NULL || out_video_frame == NULL){
        ret = -1;
        printf("参数不能为空!\n");
        goto end;
    }
    ret = avcodec_send_packet(video_decode_codec_ctx,&pkt);
    if(ret == AVERROR(EAGAIN)){
        
    }
    else if (ret < 0) {
        printf("avcodec_send_packet failed!\n");
        goto end;
    }
    ret = avcodec_receive_frame(video_decode_codec_ctx,out_video_frame);
    if (ret == AVERROR(EAGAIN)){
        goto end;
    }
    if(ret < 0)
    {
        printf("avcodec_receive_frame failed!\n");
        goto end;
    }
end:
    return ret;
}


/**
 释放视频解码器相关资源

 @param video_decode_codec_ctx 视频解码器上下文
 @return 0:成功
 */
int uinit_video_decode(AVCodecContext* video_decode_codec_ctx)
{
    int ret = 0;
    if(video_decode_codec_ctx == NULL){
        printf("uinit_video_decode 参数不能为空!\n");
        ret = -1;
        goto end;
    }
    avcodec_close(video_decode_codec_ctx);
end:
    return ret;
}

/**
 初始化视频编码器

 @param video_codecID 目标视频编码器ID
 @param dst_video_width 目标视频宽度
 @param dst_video_height 目标视频高度
 @param dst_video_pixelformat 目标是像素格式
 @param dst_frameRate 目标视频帧率
 @param[out] out_video_encode_codec_ctx 视频编码器上下文
 @param[out] out_video_encode_codec 视频编码器
 @return 0: success
 */
int init_video_encode(enum AVCodecID dst_video_codecID,int dst_video_width, int dst_video_height,enum AVPixelFormat dst_video_pixelformat,double dst_frameRate,AVCodecContext **out_video_encode_codec_ctx,AVCodec** out_video_encode_codec)
{
    int ret = 0;
    AVCodec* video_encode_codec = NULL;
    AVCodecContext *video_encode_codec_ctx = NULL;
    
    video_encode_codec = avcodec_find_encoder(dst_video_codecID);
    if(video_encode_codec == NULL){
        printf("avcodec_find_encoder:failed\n");
        ret = -1;
        goto end;
    }
    video_encode_codec_ctx = avcodec_alloc_context3(video_encode_codec);
    video_encode_codec_ctx->codec_id = dst_video_codecID;
    video_encode_codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
    //设置读取像素数据格式->编码的是像素数据格式->视频像素数据格式->YUV420P(YUV422P、YUV444P等等...)
    video_encode_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    video_encode_codec_ctx->width = dst_video_width;
    video_encode_codec_ctx->height = dst_video_height;
    //设置帧率:每秒25帧
    //dst_frameRate
    video_encode_codec_ctx->time_base.num = 1;
    video_encode_codec_ctx->time_base.den = dst_frameRate;
    //根据源文件设置正确的码率
    //    avcodec_context->bit_rate = 468000;
    video_encode_codec_ctx->bit_rate = 1573000;
    //设置GOP:影响到视频质量问题->画面组->一组连续画面
    video_encode_codec_ctx->gop_size = 250;
    //设置量化参数
    //一般情况下都是默认值，最小量化系数默认值是10，最大量化系数默认值是51
    video_encode_codec_ctx->qmin = 10;
    video_encode_codec_ctx->qmax = 51;
    //设置b帧最大值->设置不需要B帧
    video_encode_codec_ctx->max_b_frames = 0;
    
    AVDictionary *param = NULL;
    if(video_encode_codec_ctx->codec_id == AV_CODEC_ID_H264){
        /*
         需要查看x264源码->x264.c文件
         第一个值：预备参数
         key: preset
         value: slow->慢 superfast->超快
         */
        av_dict_set(&param,"preset","slow",0);
        av_dict_set(&param,"tune","zerolatency",0);
    }
    //H.265
    if(video_encode_codec_ctx->codec_id == AV_CODEC_ID_H265){
        av_dict_set(&param, "preset", "ultrafast", 0);
        av_dict_set(&param, "tune", "zero-latency", 0);
    }
    
    //打开编码器
    ret = avcodec_open2(video_encode_codec_ctx, video_encode_codec, &param);
    if (ret < 0)
    {
        printf("Could not open encoder\n");
        goto end;
    }
    *out_video_encode_codec_ctx = video_encode_codec_ctx;
    *out_video_encode_codec = video_encode_codec;

end:
    return ret;
}

/**
 视频格式转换
 
 @param video_decode_codec_ctx 视频解码器上下文
 @param in_video_frame 等待格式转换的AVFrame
 @param dst_video_width 目标视频宽度
 @param dst_video_height 目标视频高度
 @param dst_video_pixelformat 目标视频像素格式
 @param[out] out_video_frame 转换后的AVFrame
 @return 大于或者等于0: 表示成功
 */
int perform_yuv_conversion(AVCodecContext *video_decode_codec_ctx,AVFrame *in_video_frame,int dst_video_width,int dst_video_height,enum AVPixelFormat dst_video_pixelformat,AVFrame *out_video_frame)
{
    struct SwsContext* img_convert_ctx_video = NULL;
    int src_w = video_decode_codec_ctx->width;
    int src_h = video_decode_codec_ctx->height;
    enum AVPixelFormat src_pixfmt = video_decode_codec_ctx->pix_fmt;
    int ret = 0;
    if(video_decode_codec_ctx == NULL || in_video_frame == NULL){
        ret = -1;
        printf("perform_yuv_conversion 参数不能为空!\n");
        goto end;
    }
    //设置转换context
    if (img_convert_ctx_video == NULL)
    {
        //dst_video_width dst_video_height
        img_convert_ctx_video = sws_getContext(src_w, src_h,
                                               src_pixfmt,
                                               dst_video_width, dst_video_height,
                                               dst_video_pixelformat,
                                               SWS_BICUBIC, NULL, NULL, NULL);
        if (img_convert_ctx_video == NULL)
        {
            printf("Cannot initialize the conversion context\n");
            ret = -1;
            goto end;
        }
    }
    //开始转换
    ret = sws_scale(img_convert_ctx_video, in_video_frame->data, in_video_frame->linesize,
              0, src_h, out_video_frame->data, out_video_frame->linesize);
    if(ret < 0){
        printf("sws_scale failed!\n");
        goto end;
    }
    out_video_frame->pts = in_video_frame->pts;
    out_video_frame->pkt_dts = in_video_frame->pkt_dts;
    //有时pkt_pts和pkt_dts不同，并且pkt_pts是编码前的dts,这里要给avframe传入pkt_dts而不能用pkt_pts
    //poutframe->pts = poutframe->pkt_pts;
    out_video_frame->pts = in_video_frame->pkt_dts;
    
    out_video_frame->width = dst_video_width;
    out_video_frame->height = dst_video_height;
    out_video_frame->format = dst_video_pixelformat;
    sws_freeContext(img_convert_ctx_video);
    
end:
    return ret;
}

/**
 视频编码
 
 @param video_encode_codec_ctx 编码器上下文
 @param video_deocde_codec_ctx 解码器上下文
 @param picture 要编码的AVFrame
 @param[out] out_packet 编码后的AVPacket
 @return 0:成功
 */
int perform_video_encode(AVCodecContext *video_encode_codec_ctx,AVCodecContext *video_deocde_codec_ctx,AVFrame *picture,AVPacket **out_packet) {
    AVPacket *pkt_t = NULL;
    int ret = 0;
    if(video_encode_codec_ctx == NULL || video_deocde_codec_ctx == NULL || picture == NULL){
        ret = -1;
        printf("perform_video_encode 参数不能为空!\n");
        goto end;
    }
    pkt_t = av_packet_alloc();
    av_init_packet(pkt_t);
    pkt_t->data = NULL; // packet data will be allocated by the encoder
    pkt_t->size = 0;
    
    //旧版API
    //avcodec_encode_video2(video_encode_avcodec_ctx,pkt_t,picture,&frameFinished);
    ret = avcodec_send_frame(video_encode_codec_ctx, picture);
    if(ret == AVERROR(EAGAIN)){
        
    }else if(ret < 0){
        printf("avcodec_send_frame failed!\n");
        goto end;
    }
    ret = avcodec_receive_packet(video_encode_codec_ctx, pkt_t);
    if (ret == AVERROR(EAGAIN)){
        goto end;
    }
    if(ret < 0){
        printf("avcodec_receive_packet failed!\n");
        goto end;
    }
    if (ret == 0) //编码成功
    {
        *out_packet = pkt_t;
    }
end:
    return ret;
}


/**
 释放视频编码器相关资源

 @param video_encode_codec_ctx 视频编码器上下文
 @return 0:成功
 */
int uinit_video_encode(AVCodecContext* video_encode_codec_ctx)
{
    int ret = 0;
    if(video_encode_codec_ctx == NULL){
        printf("uinit_video_encode 参数不能为空!\n");
        ret = -1;
        goto end;
    }
    avcodec_close(video_encode_codec_ctx);
end:
    return ret;
}


/**
 向封装格式中写入视频

 @param pkt 要写入的AVPacket
 @param output_format_context 输出格式上下文
 @param video_stream_idx 视频流索引
 @param in_video_stream 视频输入流
 @param dst_video_stream 视频输出流

 @return 0:成功
 */
int write_video_frame(AVPacket *pkt,AVFormatContext *output_format_context,int video_stream_idx,AVStream *in_video_stream,AVStream *dst_video_stream){
    int ret = 0;
    if(in_video_stream == NULL || dst_video_stream == NULL || output_format_context == NULL || pkt == NULL){
        ret = -1;
        printf("write_video_frame 参数不能为空!\n");
        goto end;
        
    }
    pkt->stream_index = video_stream_idx;
    pkt->pts = av_rescale_q_rnd(pkt->pts,in_video_stream->time_base,dst_video_stream->time_base, AV_ROUND_NEAR_INF);
    //dts用于解码
    pkt->dts = av_rescale_q_rnd(pkt->dts,in_video_stream->time_base,dst_video_stream->time_base, AV_ROUND_NEAR_INF);
    pkt->duration = av_rescale_q(pkt->duration,in_video_stream->time_base,dst_video_stream->time_base);
    pkt->pos = -1;
    ret = av_interleaved_write_frame(output_format_context, pkt);
    if (ret < 0) {
        printf("Error muxing packet!\n");
        goto end;
    }
    
end:
    return ret;
}

/**
  封装

 @param outfilename 输出文件名
 @param input_format_context 输入文件格式上下文
 @param audio_stream_idx 输出音频流索引
 @param video_stream_idx 输出视频流索引
 @param audio_encode_codec_ctx 音频编码器上下文
 @param video_encode_codec_ctx 视频编码器上下文
 @param out_output_format 输出格式
 @param out_output_format_context 输出格式上下文
 @return 0:成功
 */
int init_mux(const char *outfilename,AVFormatContext *input_format_context,int audio_stream_idx,int video_stream_idx,AVCodecContext* audio_encode_codec_ctx,AVCodecContext *video_encode_codec_ctx,AVOutputFormat **out_output_format,AVFormatContext **out_output_format_context) {
    int ret = 0;
    AVFormatContext *output_format_context = NULL;
    AVOutputFormat *output_format = NULL;
    if(outfilename == NULL || input_format_context == NULL || audio_encode_codec_ctx == NULL || video_encode_codec_ctx == NULL){
        ret = -1;
        printf("init_mux参数不能为空!\n");
        goto end;
    }
    //生成默认输出上下文
    avformat_alloc_output_context2(out_output_format_context, NULL, NULL, outfilename);
    if (out_output_format_context == NULL) {
        printf("Could not create output context\n");
        ret = -1;
        goto end;
    }
    output_format_context = *out_output_format_context;
    output_format = output_format_context->oformat;
    *out_output_format = output_format;
    
    for (int i = 0; i < input_format_context->nb_streams; i++) {
        AVStream *out_stream;
        AVStream *in_stream = input_format_context->streams[i];
        AVCodecParameters *in_codecpar = in_stream->codecpar;
        
        if (in_codecpar->codec_type != AVMEDIA_TYPE_AUDIO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_VIDEO &&
            in_codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            continue;
        }
        out_stream = avformat_new_stream(output_format_context, NULL);
        if (out_stream == NULL) {
            printf("Failed allocating output stream\n");
            ret = -1;
            goto end;
        }
        ret = avcodec_parameters_copy(out_stream->codecpar, in_codecpar);
        if (ret < 0) {
            printf("Failed to copy codec parameters\n");
            goto end;
        }
        out_stream->codecpar->codec_tag = 0;
    }
    
    if (!(output_format->flags & AVFMT_NOFILE)) {
        ret = avio_open(&output_format_context->pb, outfilename, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("Could not open output file '%s'", outfilename);
            ret = -1;
            goto end;
        }
    }
    avcodec_parameters_from_context(output_format_context->streams[audio_stream_idx]->codecpar,
                                    audio_encode_codec_ctx);
    avcodec_parameters_from_context(output_format_context->streams[video_stream_idx]->codecpar,
                                    video_encode_codec_ctx);
end:
    return ret;
}

/**
 封装
 
 @param outfilename 输出文件名
 @param input_format_context 输入文件格式上下文
 @param[out] out_output_format 输出格式
 @param[out] out_output_format_context 输出格式上下文
 @return 0:成功
 */
int uinit_mux(AVFormatContext *output_format_context){
    int ret = 0;
    if(output_format_context == NULL){
        ret = -1;
        printf("uinit_mux: 参数不能为空!\n");
        goto end;
    }
    /* free the stream */
    av_free(output_format_context);
end:
    return ret;
}


int main(int argc,const char* argv[]){
    const char *infilename = NULL; //输入文件
    const char *outfilename = NULL; //输出文件
    AVFormatContext *input_format_context = NULL; //输入文件的格式上下文
    AVFormatContext *output_format_context = NULL; //输出文件的格式上下文
    AVOutputFormat *output_format = NULL;
    
    AVCodecContext *audio_decode_codec_ctx = NULL; //音频解码器上下文
    AVCodec *audio_decode_codec = NULL; //音频解码
    AVCodecContext *audio_encode_codec_ctx = NULL; //音频编码器上下文
    AVCodec *audio_encode_codec = NULL; //音频编码器
  
    AVCodecContext *video_decode_codec_ctx = NULL; //视频解码器上下文
    AVCodec *video_decode_codec = NULL; //视频解码器
    AVCodecContext *video_encode_codec_ctx = NULL; //视频编码器上下文
    AVCodec *video_encode_codec = NULL; //视频编码器
    
    AVFrame *pout_video_frame = NULL;
    AVFrame *pConvertframe = NULL;
    AVFrame *video_decode_frame = NULL;
    AVAudioFifo* audiofifo = NULL;
    
    AVPacket pkt;
    AVPacket* out_pkt;
    AVFrame *audio_decode_frame = NULL;
    AVFrame *resample_audio_frame = NULL;
    SwrContext *swr_ctx = NULL;
    int out_buffer_size = 0; //重采样之后数据所需要的缓存大小
 
    int audio_stream_idx = -1; //音频流索引
    int video_stream_idx = -1; //视频流索引
    int ret = 0;
    int size = 0;
    uint8_t* frame_buf;
    
    //视频参数
    enum AVPixelFormat dst_video_pixelformat = AV_PIX_FMT_YUV420P; //转换后的像素格式
    int dst_video_width = 720; //转换后的视频宽度
    int dst_video_height = 480; //转换后的视频高度
    double dst_frameRate = 25; //视频帧率
    enum AVCodecID dst_video_codecID = AV_CODEC_ID_H264;
    
    //音频参数
    int out_channels = 2;//声道
    int out_sample_rate = 96000; //采样率 AV_SAMPLE_FMT_S16 AV_SAMPLE_FMT_FLTP
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_FLTP; //采样格式
    int m_audiomuxtimebasetrue = 0; //音频的mux层timebse是否正确
    //aac
    enum AVCodecID out_audio_codecID = AV_CODEC_ID_AAC;
    int out_audio_frame_size  = 1024;
    int kMAX_AUDIO_FRAME_SIZE = 192000;
    
    
    int m_is_first_audio_pts = 0;
    int64_t m_first_audio_pts = 0;
    
    if(argc <=2){
        printf("parameters: srcfilename dstfilename \n");
        return -1;
    }
    av_log_set_level(AV_LOG_INFO);
    infilename = argv[1];
    outfilename = argv[2];
    //解封装
    ret = init_demux(infilename,&input_format_context,&audio_stream_idx,&video_stream_idx,&m_audiomuxtimebasetrue);
    if (ret < 0){
        printf("解封装失败");
        return -1;
    }
  
    //初始化音频解码
    ret = init_audio_decode(input_format_context,audio_stream_idx,&audio_decode_codec_ctx,&audio_decode_codec);
    if (ret < 0){
        printf("初始化解码器失败");
        return -1;
    }
    //初始化音频编码器
    ret  = init_audio_encode(out_audio_codecID,out_sample_rate,out_channels,out_audio_frame_size,out_sample_fmt,&audio_encode_codec_ctx,&audio_encode_codec);
    if(ret < 0){
        printf("init_audio_encode failed!\n");
    }
    
    uint8_t *out_buffer=(uint8_t *)av_malloc(kMAX_AUDIO_FRAME_SIZE*2);
    
    swr_ctx = init_pcm_resample(audio_decode_codec_ctx,
                               audio_encode_codec_ctx->sample_fmt,audio_encode_codec_ctx->channels ,audio_encode_codec_ctx->sample_rate);
 

    resample_audio_frame = av_frame_alloc();
    if (!resample_audio_frame) {
        fprintf(stderr, "Could not allocate audio frame\n");
        exit(1);
    }
    //打开输出文件
    FILE* outfile = fopen(outfilename, "wb");
    
    //初始化视频解码器
    ret = init_video_decode(input_format_context,video_stream_idx,&video_decode_codec_ctx,&video_decode_codec);
    if(ret < 0){
        fprintf(stderr, "Could not init_video_decode\n");
        return -1;
    }
    //初始化视频编码器
    ret = init_video_encode(dst_video_codecID,dst_video_width,dst_video_height,dst_video_pixelformat,dst_frameRate,&video_encode_codec_ctx,&video_encode_codec);
    if(ret < 0){
        fprintf(stderr, "Could not init_video_encode\n");
        return -1;
    }
    //封装
    ret = init_mux(outfilename,input_format_context,audio_stream_idx,video_stream_idx,audio_encode_codec_ctx,video_encode_codec_ctx,&output_format,&output_format_context);
 
    
    //写入头
    ret = avformat_write_header(output_format_context, NULL);
    if (ret < 0) {
        fprintf(stderr, "Error occurred when opening output file:%d\n",ret);
        return -1;
    }
    
    pout_video_frame = av_frame_alloc();
    if(pout_video_frame == NULL){
        printf("av_frame_alloc pout_video_frame error\n");
        return -1;
    }

    //旧版API
    //int out_size = avpicture_get_size(video_pixelfromat, video_width,video_height);
    int out_size = av_image_get_buffer_size(dst_video_pixelformat,dst_video_width,dst_video_height,1);
    uint8_t * pOutput_buf =( uint8_t *)malloc(out_size * 3 * sizeof(char)); //最大分配的空间，能满足yuv的各种格式
    //旧版API
    //avpicture_fill((AVPicture *)pout_video_frame, (unsigned char *)pOutput_buf, video_pixelfromat,video_width, video_height);
    ret = av_image_fill_arrays(pout_video_frame->data,pout_video_frame->linesize,pOutput_buf,dst_video_pixelformat,dst_video_width,dst_video_height,1); //内存关联
    
    while (1) {
        AVStream *in_stream, *out_stream;
        av_init_packet(&pkt);
        out_pkt = av_packet_alloc();
        av_init_packet(out_pkt);
        if(!audio_decode_frame) {
            audio_decode_frame = av_frame_alloc();
            if(audio_decode_frame == NULL){
                fprintf(stderr, "Could not allocate audio frame\n");
                break;
            }
        }
        if(!video_decode_frame) {
            video_decode_frame = av_frame_alloc();
            if(video_decode_frame == NULL){
                fprintf(stderr, "Could not allocate audio frame\n");
                break;
            }
        }
        //获取audio_pkt
        if(av_read_frame(input_format_context,&pkt) < 0) {
            break;
        }
    
        in_stream  = input_format_context->streams[pkt.stream_index];
        out_stream = output_format_context->streams[pkt.stream_index];
        
        //如果是音频
        if(pkt.stream_index == audio_stream_idx){
            //进行音频解码
            ret = perform_audio_decode(audio_decode_codec_ctx,pkt,audio_decode_frame);
            if (ret == 0){
                if(audiofifo == NULL){
                  
                    int64_t src_nb_samples = audio_decode_frame->nb_samples;
                    //计算输出的samples 和采样率有关 例如：48000转44100，samples则是从1152转为1059，除法
                    resample_audio_frame->nb_samples = av_rescale_rnd(src_nb_samples, out_sample_rate, audio_decode_codec_ctx->sample_rate, AV_ROUND_UP);
                    int ret = av_samples_alloc(resample_audio_frame->data, &resample_audio_frame->linesize[0],
                                               out_channels, resample_audio_frame->nb_samples,out_sample_fmt,1);
                    if (ret < 0)
                    {
                        return -1;
                    }
                    
                    audiofifo  = av_audio_fifo_alloc(out_sample_fmt, out_channels,
                                                                        resample_audio_frame->nb_samples);
                }
                //进行重采样
                out_buffer_size = perform_pcm_resample(swr_ctx,audio_decode_codec_ctx,
                                                       audio_decode_frame,
                                                       audio_encode_codec_ctx->sample_fmt,
                                                       audio_encode_codec_ctx->channels,resample_audio_frame,audiofifo);
              
                int64_t pts_t = audio_decode_frame->pts;
                int duration_t = 0;
                if(m_audiomuxtimebasetrue == -1)
                {
                    duration_t = (double)audio_encode_codec_ctx->frame_size * (audio_decode_codec_ctx->time_base.den/audio_decode_codec_ctx->time_base.num)/
                    audio_decode_codec_ctx->sample_rate;
                }
                else
                {
                    duration_t = (double)audio_encode_codec_ctx->frame_size * (input_format_context->streams[audio_stream_idx]->time_base.den /input_format_context->streams[audio_stream_idx]->time_base.num)/
                    audio_decode_codec_ctx->sample_rate;
                }
                int out_framesize = audio_encode_codec_ctx->frame_size;
                AVFrame * tempFrame = av_frame_alloc();
                tempFrame->nb_samples = out_framesize;
                tempFrame->channel_layout = audio_encode_codec_ctx->channel_layout;
                tempFrame->channels = audio_encode_codec_ctx->channels;
                tempFrame->format         = audio_encode_codec_ctx->sample_fmt;
                tempFrame->sample_rate    = audio_encode_codec_ctx->sample_rate;
                int error = 0;
                if ((error = av_frame_get_buffer(tempFrame, 0)) < 0)
                {
                    av_frame_free(&tempFrame);
                    return error;
                }
                while (av_audio_fifo_size(audiofifo) >= tempFrame->nb_samples) {
                    av_audio_fifo_read(audiofifo,(void **)tempFrame->data,tempFrame->nb_samples);
                    //重新计算音频的pts
                    if(m_is_first_audio_pts == 0)
                    {
                        m_first_audio_pts = pts_t;
                        m_is_first_audio_pts = 1;
                    }
                    tempFrame->pts = m_first_audio_pts;
                    m_first_audio_pts += duration_t;
                    
                    tempFrame->pts = av_rescale_q_rnd(tempFrame->pts, audio_encode_codec_ctx->time_base, audio_decode_codec_ctx->time_base, AV_ROUND_NEAR_INF);
                    ret = perform_audio_encode(audio_encode_codec_ctx,tempFrame,out_pkt);
                    if (ret == AVERROR(EAGAIN))
                        continue;
                    if (ret < 0) {
                        fprintf(stderr, "Error perform_audio_encode\n");
                        exit(1);
                    }
                    ret = write_audio_frame(out_pkt,output_format_context,audio_stream_idx,in_stream,out_stream);
                }
         
            }else if (ret == AVERROR(EAGAIN)){
                // output is not available in this state - user must try to send new input
                //AVERROR(EAGAIN)(-35) is not an error, it just means output is not yet available and you need to call _send_packet() a few more times before the first output will be available from _receive_frame().
                av_log(NULL,AV_LOG_WARNING,"%s\n",av_err2str(ret));
                continue;
            }else{
                av_log(NULL,AV_LOG_ERROR,"%s\n",av_err2str(ret));
                break;
            }
        }else if(pkt.stream_index == video_stream_idx){
            ret = perform_video_decode(video_decode_codec_ctx,pkt,video_decode_frame);
           
            if(ret == AVERROR(EAGAIN)){
                continue;
            }
            if(ret == 0){
                ret = perform_yuv_conversion(video_decode_codec_ctx,video_decode_frame,dst_video_width,dst_video_height, dst_video_pixelformat,pout_video_frame);
                if(ret >= 0){
                    
                    ret = perform_video_encode(video_encode_codec_ctx,video_decode_codec_ctx,pout_video_frame, &out_pkt);
                    if(ret == AVERROR(EAGAIN)){
                        continue;
                    }
                    
                    if (ret == 0) {
                        ret = write_video_frame(out_pkt,output_format_context,video_stream_idx,in_stream,out_stream);
                    }
                }
                
            }
         
        }
     
    }
    
    av_write_trailer(output_format_context);
    
    uinit_audio_decode(audio_decode_codec_ctx);
    uinit_demux(input_format_context);
    av_frame_free(&audio_decode_frame);
}

