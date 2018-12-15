//
//  ffmpeg.h
//  
//
//  Created by mac on 2018/11/27.
//

#ifndef ffmpeg_h
#define ffmpeg_h

#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>

/**
 解封装
 
 @param infilename 输入多媒体文件名
 @param[out] out_input_format_context 输入文件的多媒体格式上下文
 @param[out] out_audio_stream_idx 音频流索引
 @param[out] out_video_stream_idx 视频流索引
 @param[out] out_m_audiomuxtimebasetrue 音频的mux层timebse是否正确 -1 不正确 0 正确
 @return 0:成功
 */
int init_demux(const char *infilename,AVFormatContext **out_input_format_context,int *out_audio_stream_idx,int *out_video_stream_idx,int *out_m_audiomuxtimebasetrue);

/**
 释放格式上下文
 
 @param formatContext formatContext description
 @return return value description
 */
int uinit_demux(AVFormatContext *input_format_context);

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
int init_mux(const char *outfilename,AVFormatContext *input_format_context,int audio_stream_idx,int video_stream_idx,AVCodecContext* audio_encode_codec_ctx,AVCodecContext *video_encode_codec_ctx,AVOutputFormat **out_output_format,AVFormatContext **out_output_format_context);

/**
 封装
 
 @param outfilename 输出文件名
 @param input_format_context 输入文件格式上下文
 @param[out] out_output_format 输出格式
 @param[out] out_output_format_context 输出格式上下文
 @return 0:成功
 */
int uinit_mux(AVFormatContext *output_format_context);

/**
 初始化音频解码器
 
 @param input_format_context 输入文件格式上下文
 @param audio_stream_idx 音频流索引
 @param[out] out_audio_decode_codec_ctx 音频解码器上下文
 @param[out] out_audio_decode_codec 音频解码器
 @return 0: success
 */
int init_audio_decode(AVFormatContext *input_format_context,int audio_stream_idx,AVCodecContext **out_audio_decode_codec_ctx,AVCodec **out_audio_decode_codec);

/**
 音频解码
 @param audio_avcodec_ctx 音频解码器
 @param audio_pkt 等待解码的AVPacket
 @param[out] out_audio_frame 解码器后的AVFrame
 @return 0 成功
 */
int perform_audio_decode(AVCodecContext *audio_decode_codec_ctx,AVPacket audio_pkt,AVFrame *out_audio_frame);

/**
 释放音频解码相关环境
 
 @param audio_decode_codec_ctx 音频解码器上下文
 @return 0:成功
 */
int uinit_audio_decode(AVCodecContext* audio_decode_codec_ctx);

/**
 重采样初始化
 
 @param audio_decode_codec_ctx 音频解码器
 @param out_sample_fmt 目标采样格式
 @param out_channels 目标声道数
 @param out_sample_rate 目标采样率
 @return NULL失败
 */
SwrContext *init_pcm_resample(AVCodecContext *audio_decode_codec_ctx,
                              int dst_sample_fmt,int dst_channels ,int dst_sample_rate);

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
                         enum AVSampleFormat dst_sample_fmt,int dst_channels,AVFrame *resample_audio_frame,AVAudioFifo *audiofifo);

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
int init_audio_encode(enum AVCodecID audio_codec_id,int dst_sample_rate,int dst_channels,int dst_audio_frame_size,enum AVSampleFormat dst_sample_fmt,AVCodecContext **out_audio_encode_codec_ctx,AVCodec **out_audio_encode_codec);

/**
 音频编码
 @param audio_encode_codec_ctx 音频编码上下文
 @param audio_frame 需要编码的AVFrame
 @param out_audio_pkt 已经编码的AVPacket
 @return 0 成功
 */
int perform_audio_encode(AVCodecContext *audio_encode_codec_ctx,AVFrame* audio_frame,AVPacket *out_audio_pkt);

/**
 向封装格式中写入音频
 
 @param pkt 要写入的AVPacket
 @param output_format_context 输出格式上下文
 @param audio_stream_idx 音频流索引
 @param in_audio_stream 输入音频流
 @param dst_audio_stream 输出音频流
 @return 0: success
 */
int write_audio_frame(AVPacket *pkt,AVFormatContext *output_format_context,int audio_stream_idx,AVStream *in_audio_stream,AVStream *dst_audio_stream);

/**
 释放音频编码器相关资源
 
 @param audio_encode_codec_ctx 音频编码器上下文
 @return 0:成功
 */
int uinit_audio_encode(AVCodecContext* audio_encode_codec_ctx);

/**
 初始化视频解码器
 
 @param input_format_context 输入文件格式上下文
 @param video_stream_idx 视频流索引
 @param[out] out_video_decode_codec_ctx 视频解码器上下文
 @param[out] out_video_decode_codec 视频解码器
 @return 0:success
 */
int init_video_decode(AVFormatContext *input_format_context,int video_stream_idx,AVCodecContext **out_video_decode_codec_ctx,AVCodec **out_video_decode_codec);

/**
 执行视频解码
 
 @param video_decode_codec_ctx 视频解码器上下文
 @param pkt 解码之前的AVPacket
 @param[out] out_video_frame 解码之后存放数据的AVFrame
 @return 0 success
 */
int perform_video_decode(AVCodecContext *video_decode_codec_ctx,AVPacket pkt,AVFrame *out_video_frame);

/**
 释放视频解码器相关资源
 
 @param video_decode_codec_ctx 视频解码器上下文
 @return 0:成功
 */
int uinit_video_decode(AVCodecContext* video_decode_codec_ctx);

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
int init_video_encode(enum AVCodecID dst_video_codecID,int dst_video_width, int dst_video_height,enum AVPixelFormat dst_video_pixelformat,double dst_frameRate,AVCodecContext **out_video_encode_codec_ctx,AVCodec** out_video_encode_codec);

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
int perform_yuv_conversion(AVCodecContext *video_decode_codec_ctx,AVFrame *in_video_frame,int dst_video_width,int dst_video_height,enum AVPixelFormat dst_video_pixelformat,AVFrame *out_video_frame);

/**
 视频编码
 
 @param video_encode_codec_ctx 编码器上下文
 @param video_deocde_codec_ctx 解码器上下文
 @param picture 要编码的AVFrame
 @param[out] out_packet 编码后的AVPacket
 @return 0:成功
 */
int perform_video_encode(AVCodecContext *video_encode_codec_ctx,AVCodecContext *video_deocde_codec_ctx,AVFrame *picture,AVPacket **out_packet);

/**
 释放视频编码器相关资源
 
 @param video_encode_codec_ctx 视频编码器上下文
 @return 0:成功
 */
int uinit_video_encode(AVCodecContext* video_encode_codec_ctx);

/**
 向封装格式中写入视频
 
 @param pkt 要写入的AVPacket
 @param output_format_context 输出格式上下文
 @param video_stream_idx 视频流索引
 @param in_video_stream 视频输入流
 @param dst_video_stream 视频输出流
 
 @return 0:成功
 */
int write_video_frame(AVPacket *pkt,AVFormatContext *output_format_context,int video_stream_idx,AVStream *in_video_stream,AVStream *dst_video_stream);
#endif /* ffmpeg_h */
