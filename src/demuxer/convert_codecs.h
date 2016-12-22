/*!
 * convert_codecs.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Jacob Tarasiewicz
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_CONVERT_CODECS_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_CONVERT_CODECS_H_

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/dict.h>
}

#include "common.h"

Samsung::NaClPlayer::AudioCodec_Type ConvertAudioCodec(AVCodecID codec) {
  switch (codec) {
    case AV_CODEC_ID_AAC:
    case AV_CODEC_ID_AAC_LATM:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_AAC;
    case AV_CODEC_ID_AC3:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_AC3;
    case AV_CODEC_ID_EAC3:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_EAC3;
    case AV_CODEC_ID_DTS:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_DTS;
    case AV_CODEC_ID_MP2:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_MP2;
    case AV_CODEC_ID_MP3:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_MP3;
    case AV_CODEC_ID_WMAV1:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_WMAV1;
    case AV_CODEC_ID_WMAV2:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_WMAV2;
    case AV_CODEC_ID_PCM_U8:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_PCM;
    case AV_CODEC_ID_PCM_MULAW:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_PCM_MULAW;
    case AV_CODEC_ID_PCM_S16BE:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_PCM_S16BE;
    case AV_CODEC_ID_PCM_S24BE:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_PCM_S24BE;
    case AV_CODEC_ID_VORBIS:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_VORBIS;
    case AV_CODEC_ID_FLAC:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_FLAC;
    case AV_CODEC_ID_AMR_NB:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_AMR_NB;
    case AV_CODEC_ID_AMR_WB:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_AMR_WB;
    case AV_CODEC_ID_GSM_MS:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_GSM_MS;
    case AV_CODEC_ID_OPUS:
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_OPUS;
    default:
      LOG_ERROR("unknown codec %d", codec);
      return Samsung::NaClPlayer::AUDIOCODEC_TYPE_UNKNOWN;
  }
}

Samsung::NaClPlayer::SampleFormat ConvertSampleFormat(AVSampleFormat format) {
  switch (format) {
    case AV_SAMPLE_FMT_U8:
      return Samsung::NaClPlayer::SAMPLEFORMAT_U8;
    case AV_SAMPLE_FMT_S16:
      return Samsung::NaClPlayer::SAMPLEFORMAT_S16;
    case AV_SAMPLE_FMT_S32:
      return Samsung::NaClPlayer::SAMPLEFORMAT_S32;
    case AV_SAMPLE_FMT_FLT:
      return Samsung::NaClPlayer::SAMPLEFORMAT_F32;
    case AV_SAMPLE_FMT_S16P:
      return Samsung::NaClPlayer::SAMPLEFORMAT_PLANARS16;
    case AV_SAMPLE_FMT_FLTP:
      return Samsung::NaClPlayer::SAMPLEFORMAT_PLANARF32;
    default:
      LOG_ERROR("unknown sample format %d", format);
      return Samsung::NaClPlayer::SAMPLEFORMAT_UNKNOWN;
  }
}

Samsung::NaClPlayer::ChannelLayout ChannelLayoutFromChannelCount(int channels) {
  switch (channels) {
    case 1:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_MONO;
    case 2:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_STEREO;
    case 3:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_SURROUND;
    case 4:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_QUAD;
    case 5:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_5_0;
    case 6:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_5_1;
    case 7:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_6_1;
    case 8:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_7_1;
    default:
      LOG_ERROR("layout %d", channels);
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_UNSUPPORTED;
  }
}

Samsung::NaClPlayer::ChannelLayout ConvertChannelLayout(uint64_t layout,
                                                        int channels) {
  switch (layout) {
    case AV_CH_LAYOUT_MONO:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_MONO;
    case AV_CH_LAYOUT_STEREO:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_STEREO;
    case AV_CH_LAYOUT_2_1:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_2_1;
    case AV_CH_LAYOUT_SURROUND:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_SURROUND;
    case AV_CH_LAYOUT_4POINT0:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_4_0;
    case AV_CH_LAYOUT_2_2:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_2_2;
    case AV_CH_LAYOUT_QUAD:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_QUAD;
    case AV_CH_LAYOUT_5POINT0:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_5_0;
    case AV_CH_LAYOUT_5POINT1:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_5_1;
    case AV_CH_LAYOUT_5POINT0_BACK:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_5_0_BACK;
    case AV_CH_LAYOUT_5POINT1_BACK:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_5_1_BACK;
    case AV_CH_LAYOUT_7POINT0:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_7_0;
    case AV_CH_LAYOUT_7POINT1:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_7_1;
    case AV_CH_LAYOUT_7POINT1_WIDE:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_7_1_WIDE;
    case AV_CH_LAYOUT_STEREO_DOWNMIX:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_STEREO_DOWNMIX;
    case AV_CH_LAYOUT_2POINT1:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_2POINT1;
    case AV_CH_LAYOUT_3POINT1:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_3_1;
    case AV_CH_LAYOUT_4POINT1:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_4_1;
    case AV_CH_LAYOUT_6POINT0:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_6_0;
    case AV_CH_LAYOUT_6POINT0_FRONT:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_6_0_FRONT;
    case AV_CH_LAYOUT_HEXAGONAL:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_HEXAGONAL;
    case AV_CH_LAYOUT_6POINT1:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_6_1;
    case AV_CH_LAYOUT_6POINT1_BACK:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_6_1_BACK;
    case AV_CH_LAYOUT_6POINT1_FRONT:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_6_1_FRONT;
    case AV_CH_LAYOUT_7POINT0_FRONT:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_7_0_FRONT;
    case AV_CH_LAYOUT_7POINT1_WIDE_BACK:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_7_1_WIDE_BACK;
    case AV_CH_LAYOUT_OCTAGONAL:
      return Samsung::NaClPlayer::CHANNEL_LAYOUT_OCTAGONAL;
    default:
      LOG_ERROR(
          "channel layout %llu unknown, getting layout from channel "
          "count %d",
          layout, channels);
      return ChannelLayoutFromChannelCount(channels);
  }
}

Samsung::NaClPlayer::AudioCodec_Profile ConvertAACAudioCodecProfile(
    int profile) {
  switch (profile) {
    case FF_PROFILE_AAC_MAIN:
      return Samsung::NaClPlayer::AUDIOCODEC_PROFILE_AAC_MAIN;
    case FF_PROFILE_AAC_LOW:
      return Samsung::NaClPlayer::AUDIOCODEC_PROFILE_AAC_LOW;
    case FF_PROFILE_AAC_SSR:
      return Samsung::NaClPlayer::AUDIOCODEC_PROFILE_AAC_SSR;
    case FF_PROFILE_AAC_LTP:
      return Samsung::NaClPlayer::AUDIOCODEC_PROFILE_AAC_LTP;
    case FF_PROFILE_AAC_HE:
      return Samsung::NaClPlayer::AUDIOCODEC_PROFILE_AAC_HE;
    case FF_PROFILE_AAC_HE_V2:
      return Samsung::NaClPlayer::AUDIOCODEC_PROFILE_AAC_HE_V2;
    case FF_PROFILE_AAC_LD:
      return Samsung::NaClPlayer::AUDIOCODEC_PROFILE_AAC_LD;
    case FF_PROFILE_AAC_ELD:
      return Samsung::NaClPlayer::AUDIOCODEC_PROFILE_AAC_ELD;
    default:
      LOG_ERROR("unknown profile %d", profile);
      return Samsung::NaClPlayer::AUDIOCODEC_PROFILE_UNKNOWN;
  }
}

Samsung::NaClPlayer::VideoCodec_Type ConvertVideoCodec(AVCodecID codec) {
  switch (codec) {
    case AV_CODEC_ID_H264:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_H264;
    case AV_CODEC_ID_THEORA:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_THEORA;
    case AV_CODEC_ID_MPEG4:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_MPEG4;
    case AV_CODEC_ID_VP8:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_VP8;
    case AV_CODEC_ID_VP9:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_VP9;
    case AV_CODEC_ID_MPEG2VIDEO:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_MPEG2;
    case AV_CODEC_ID_VC1:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_VC1;
    case AV_CODEC_ID_WMV1:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_WMV1;
    case AV_CODEC_ID_WMV2:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_WMV2;
    case AV_CODEC_ID_WMV3:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_WMV3;
    case AV_CODEC_ID_H263:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_H263;
    case AV_CODEC_ID_INDEO3:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_INDEO3;
    case AV_CODEC_ID_H265:
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_H265;
    default:
      LOG_ERROR("unknown codec %d", codec);
      return Samsung::NaClPlayer::VIDEOCODEC_TYPE_UNKNOWN;
  }
}

Samsung::NaClPlayer::VideoCodec_Profile ConvertH264VideoCodecProfile(
    int profile) {
  profile &= ~FF_PROFILE_H264_CONSTRAINED;
  profile &= ~FF_PROFILE_H264_INTRA;
  switch (profile) {
    case FF_PROFILE_H264_BASELINE:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_H264_BASELINE;
    case FF_PROFILE_H264_MAIN:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_H264_MAIN;
    case FF_PROFILE_H264_EXTENDED:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_H264_EXTENDED;
    case FF_PROFILE_H264_HIGH:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_H264_HIGH;
    case FF_PROFILE_H264_HIGH_10:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_H264_HIGH10;
    case FF_PROFILE_H264_HIGH_422:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_H264_HIGH422;
    case FF_PROFILE_H264_HIGH_444_PREDICTIVE:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_H264_HIGH444PREDICTIVE;
    default:
      LOG_ERROR("unknown profile %d", profile);
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_UNKNOWN;
  }
}

Samsung::NaClPlayer::VideoCodec_Profile ConvertMPEG2VideoCodecProfile(
    int profile) {
  switch (profile) {
    case FF_PROFILE_MPEG2_422:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_MPEG2_422;
    case FF_PROFILE_MPEG2_HIGH:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_MPEG2_HIGH;
    case FF_PROFILE_MPEG2_SS:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_MPEG2_SS;
    case FF_PROFILE_MPEG2_SNR_SCALABLE:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_MPEG2_SNR_SCALABLE;
    case FF_PROFILE_MPEG2_MAIN:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_MPEG2_MAIN;
    case FF_PROFILE_MPEG2_SIMPLE:
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_MPEG2_SIMPLE;
    default:
      LOG_ERROR("unknown profile %d", profile);
      return Samsung::NaClPlayer::VIDEOCODEC_PROFILE_UNKNOWN;
  }
}

Samsung::NaClPlayer::VideoFrame_Format ConvertVideoFrameFormat(int format) {
  switch (format) {
    case AV_PIX_FMT_YUV422P:
      return Samsung::NaClPlayer::VIDEOFRAME_FORMAT_YV16;
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
      return Samsung::NaClPlayer::VIDEOFRAME_FORMAT_YV12;
    case AV_PIX_FMT_YUVA420P:
      return Samsung::NaClPlayer::VIDEOFRAME_FORMAT_YV12A;
    default:
      LOG_ERROR("unknown format %d", format);
      return Samsung::NaClPlayer::VIDEOFRAME_FORMAT_INVALID;
  }
}

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_CONVERT_CODECS_H_
