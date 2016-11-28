/*!
 * ffmpeg_demuxer.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Jacob Tarasiewicz
 * @author Tomasz Borkowski
 */

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/dict.h"
}

#include "ffmpeg_demuxer.h"
#include "common.h"

#include "convert_codecs.h"

using pp::AutoLock;
using pp::MessageLoop;
using std::unique_ptr;
using Samsung::NaClPlayer::ESPacket;
using Samsung::NaClPlayer::Rational;
using Samsung::NaClPlayer::Size;
using Samsung::NaClPlayer::TimeTicks;

const uint8_t kPlayReadySystemId[] = {
    // "9a04f079-9840-4286-ab92-e65be0885f95";
    0x9a, 0x04, 0xf0, 0x79, 0x98, 0x40, 0x42, 0x86,
    0xab, 0x92, 0xe6, 0x5b, 0xe0, 0x88, 0x5f, 0x95,
};
const std::string kDRMInitDataType("cenc:pssh");

static const int kKidLength = 16;
static const size_t kErrorBufferSize = 1024;
static const uint32_t kBufferSize = 32 * 1024;
static const uint32_t kMicrosecondsPerSecond = 1000000;
static const TimeTicks kOneMicrosecond = 1.0 / kMicrosecondsPerSecond;
static const AVRational kMicrosBase = {1, kMicrosecondsPerSecond};

static const uint32_t kAnalyzeDuration = 10 * kMicrosecondsPerSecond;
static const uint32_t kAudioStreamProbeSize = 32 * 1024;
static const uint32_t kVideoStreamProbeSize = 128 * 1024;

static TimeTicks ToTimeTicks(int64_t time_ticks, AVRational time_base) {
  int64_t us = av_rescale_q(time_ticks, time_base, kMicrosBase);
  return us * kOneMicrosecond;
}

static int AVIOReadOperation(void* opaque, uint8_t* buf, int buf_size) {
  FFMpegDemuxer* parser = reinterpret_cast<FFMpegDemuxer*>(opaque);
  int result = parser->Read(buf, buf_size);
  if (result < 0) {
    result = AVERROR(EIO);
  }
  return result;
}

template <size_t N, size_t M>
static bool SystemIdEqual(const uint8_t(&s0)[N], const uint8_t(&s1)[M]) {
  if (N != M) return false;
  for (size_t i = 0; i < N; ++i) {
    if (s0[i] != s1[i]) return false;
  }
  return true;
}

unique_ptr<StreamDemuxer> StreamDemuxer::Create(
    const pp::InstanceHandle& instance, Type type) {
  switch (type) {
    case kAudio:
      return MakeUnique<FFMpegDemuxer>(instance, kAudioStreamProbeSize, type);
    case kVideo:
      return MakeUnique<FFMpegDemuxer>(instance, kVideoStreamProbeSize, type);
    default:
      LOG_ERROR("ERROR - not supported type of stream");
  }

  return nullptr;
}

FFMpegDemuxer::FFMpegDemuxer(const pp::InstanceHandle& instance,
                             uint32_t probe_size, Type type)
    : stream_type_(type),
      audio_stream_idx_(-1),
      video_stream_idx_(-1),
      parser_thread_(instance),
      callback_factory_(this),
      format_context_(nullptr),
      io_context_(nullptr),
      buffer_lock_(),
      context_opened_(false),
      streams_initialized_(false),
      end_of_file_(false),
      exited_(false),
      probe_size_(probe_size),
      timestamp_(0.0) {
  LOG_DEBUG("parser: %p", this);
}

FFMpegDemuxer::~FFMpegDemuxer() {
  LOG_DEBUG("");
  exited_ = true;
  parser_thread_.Join();
  av_freep(io_context_);
  avformat_free_context(format_context_);
  LOG_DEBUG("");
}

bool FFMpegDemuxer::Init(const InitCallback& callback,
    MessageLoop callback_dispatcher) {
  LOG_DEBUG("Start, parser: %p", this);
  if (callback_dispatcher.is_null() || !callback) {
    LOG_ERROR("ERROR: callback is null or callback_dispatcher is invalid!");
    return false;
  }

  es_pkt_callback_ = callback;
  callback_dispatcher_ = callback_dispatcher;

  InitFFmpeg();

  format_context_ = avformat_alloc_context();
  io_context_ = avio_alloc_context(
      reinterpret_cast<unsigned char*>(av_malloc(kBufferSize)), kBufferSize, 0,
      this, AVIOReadOperation, NULL, NULL);

  if (format_context_ == NULL || io_context_ == NULL) {
    LOG_ERROR("ERROR: failed to allocate avformat or avio context!");
    return false;
  }

  io_context_->seekable = 0;
  io_context_->write_flag = 0;

  // Change this value in case when clip is not well recognized by ffmpeg
  format_context_->probesize = probe_size_;
  format_context_->max_analyze_duration = kAnalyzeDuration;
  format_context_->flags |= AVFMT_FLAG_CUSTOM_IO;
  format_context_->pb = io_context_;

  LOG_INFO("ffmpeg probe size: %u", probe_size_);
  LOG_INFO("ffmpeg analyze duration: %d",
           format_context_->max_analyze_duration);
  LOG_INFO("done, format_context: %p, io_context: %p",
           format_context_, io_context_);

  LOG_INFO("Initialized");
  parser_thread_.Start();
  DispatchCallback(kInitialized);

  return true;
}

void FFMpegDemuxer::Flush() {
  LOG_DEBUG("");
  DispatchCallback(kFlushed);
}

void FFMpegDemuxer::Parse(const std::vector<uint8_t>& data) {
  LOG_DEBUG("parser: %p, data size: %d", this, data.size());
  if (data.empty()) {
    LOG_DEBUG("Signal EOF");
    end_of_file_ = true;
  }

  pp::AutoLock lock(buffer_lock_);
  buffer_.insert(buffer_.end(), data.begin(), data.end());
  if (streams_initialized_ || buffer_.size() >= probe_size_) {
    parser_thread_.message_loop().PostWork(
        callback_factory_.NewCallback(&FFMpegDemuxer::StartParsing));
  } else {
    LOG_DEBUG("buffer size is smaller than %d, wait for next segment",
              probe_size_);
    return;
  }

  LOG_DEBUG("parser: %p, Added buffer to parser.", this);
}

bool FFMpegDemuxer::SetAudioConfigListener(
    const std::function<void(const AudioConfig&)>& callback) {
  LOG_DEBUG("");
  if (callback) {
    audio_config_callback_ = callback;
    return true;
  } else {
    LOG_DEBUG("callback is null!");
    return false;
  }
}

bool FFMpegDemuxer::SetVideoConfigListener(
    const std::function<void(const VideoConfig&)>& callback) {
  LOG_DEBUG("");
  if (callback) {
    video_config_callback_ = callback;
    return true;
  } else {
    LOG_DEBUG("callback is null!");
    return false;
  }
}

bool FFMpegDemuxer::SetDRMInitDataListener(const DrmInitCallback& callback) {
  if (callback) {
    drm_init_data_callback_ = callback;
    return true;
  } else {
    LOG_DEBUG("callback is null!");
    return false;
  }
}

void FFMpegDemuxer::SetTimestamp(TimeTicks timestamp) {
  LOG_INFO("current timestamp: %f, new: %f", timestamp_, timestamp);
  timestamp_ = timestamp;
}

void FFMpegDemuxer::Close() {
  DispatchCallback(kClosed);
  LOG_DEBUG("");
}

void FFMpegDemuxer::StartParsing(int32_t) {
  LOG_DEBUG("parser: %p, parser buffer size: %d", this, buffer_.size());
  if (!streams_initialized_ && !InitStreamInfo()) {
    LOG_ERROR("Can't initialize demuxer");
    return;
  }

  AVPacket pkt;

  while (!exited_) {
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    {
      AutoLock critical_section(buffer_lock_);
      // ensure, that very last packets will be read.
      if (!end_of_file_ && buffer_.empty()) {
        LOG_DEBUG(
            "buffer is empty and it's not the end of file "
            "- don't call av_read_frame");
        break;
      }
    }

    Message packet_msg;
    unique_ptr<ElementaryStreamPacket> es_pkt;
    int32_t ret = av_read_frame(format_context_, &pkt);
    if (ret < 0) {
      if (ret == AVERROR_EOF) {
        exited_ = true;
        packet_msg = kEndOfStream;
      } else {  // Not handled error.
        char errbuff[kErrorBufferSize];
        int32_t strerror_ret = av_strerror(ret, errbuff, kErrorBufferSize);
        LOG_ERROR("av_read_frame error: %d [%s], av_strerror ret: %d", ret,
                  errbuff, strerror_ret);
        break;
      }
    } else {
      LOG_DEBUG("parser: %p, got packet with size: %d", this, pkt.size);
      if (pkt.stream_index == audio_stream_idx_) {
        packet_msg = kAudioPkt;
      } else if (pkt.stream_index == video_stream_idx_) {
        packet_msg = kVideoPkt;
      } else {
        packet_msg = kError;
        LOG_ERROR("Error! Packet stream index (%d) not recognized!",
                  pkt.stream_index);
      }
      es_pkt = MakeESPacketFromAVPacket(&pkt);
    }

    auto es_pkt_callback = std::make_shared<EsPktCallbackData>(packet_msg,
        std::move(es_pkt));
    callback_dispatcher_.PostWork(callback_factory_.NewCallback(
        &FFMpegDemuxer::EsPktCallbackInDispatcherThread, es_pkt_callback));

    av_free_packet(&pkt);
  }

  LOG_DEBUG("Finished parsing data. buffer left: %d, parser: %p",
            buffer_.size(), this);
}

void FFMpegDemuxer::EsPktCallbackInDispatcherThread(int32_t,
    const std::shared_ptr<EsPktCallbackData>& data) {
  if (es_pkt_callback_) {
    es_pkt_callback_(
        std::get<kEsPktCallbackDataMessage>(*data),
        std::move(std::get<kEsPktCallbackDataPacket>(*data)));
  } else {
    LOG_ERROR("ERROR: es_pkt_callback_ is not initialized");
  }
}

void FFMpegDemuxer::CallbackInDispatcherThread(int32_t, Message msg) {
  (void)msg;  // suppress warning
  LOG_DEBUG("msg: %d", static_cast<int32_t>(msg));
}

void FFMpegDemuxer::DispatchCallback(Message msg) {
  LOG_DEBUG("");
  callback_dispatcher_.PostWork(callback_factory_.NewCallback(
      &FFMpegDemuxer::CallbackInDispatcherThread, msg));
}

void FFMpegDemuxer::CallbackConfigInDispatcherThread(int32_t, Type type) {
  LOG_DEBUG("type: %d", static_cast<int32_t>(type));
  switch (type) {
    case kAudio:
      if (audio_config_callback_) audio_config_callback_(audio_config_);
      break;
    case kVideo:
      if (video_config_callback_) {
        video_config_callback_(video_config_);
      }
      break;
    default:
      LOG_DEBUG("Unsupported type!");
  }
  LOG_DEBUG("");
}

int FFMpegDemuxer::Read(uint8_t* data, int size) {
  pp::AutoLock lock(buffer_lock_);
  LOG_DEBUG("Want to read %d bytes from parser buffer (size: %d)", size,
            buffer_.size());

  if (buffer_.empty()) {
    LOG_DEBUG("parser: %p, Parser buffer is empty", this);
    return AVERROR(EIO);
  }

  if (size > 0) {
    size_t read_bytes = std::min(size, static_cast<int>(buffer_.size()));
    memcpy(data, buffer_.data(), read_bytes);
    buffer_.erase(buffer_.begin(), buffer_.begin() + read_bytes);
    return read_bytes;
  } else {
    return 0;
  }
}

void FFMpegDemuxer::InitFFmpeg() {
  static bool is_initialized = false;
  if (!is_initialized) {
    LOG_INFO("avcodec_register_all() - start");
    av_register_all();
    is_initialized = true;
  }
}

bool FFMpegDemuxer::InitStreamInfo() {
  LOG_DEBUG("FFmpegStreamParser::InitStreamInfo");
  int ret;

  if (!context_opened_) {
    LOG_DEBUG("opening context = %p", format_context_);
    ret = avformat_open_input(&format_context_, NULL, NULL, NULL);
    if (ret < 0) {
      LOG_DEBUG(
          "failed to open context"
          " ret %d %s",
          ret, av_err2str(ret));
      return false;
    }

    LOG_DEBUG("context opened");
    context_opened_ = true;
    streams_initialized_ = false;
  }

  if (!streams_initialized_) {
    LOG_DEBUG("parsing stream info ctx = %p", format_context_);
    ret = avformat_find_stream_info(format_context_, NULL);
    LOG_DEBUG("find stream info ret %d", ret);
    if (ret < 0) {
      LOG_DEBUG("ERROR - find stream info error, ret: %d", ret);
    }
    av_dump_format(format_context_, NULL, NULL, NULL);
  }
  UpdateContentProtectionConfig();

  audio_stream_idx_ =
      av_find_best_stream(format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
  if (audio_stream_idx_ >= 0) {
    UpdateAudioConfig();
  }

  video_stream_idx_ =
      av_find_best_stream(format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (video_stream_idx_ >= 0) {
    UpdateVideoConfig();
  }

  LOG_DEBUG("Configs updated");
  if (!streams_initialized_) {
    streams_initialized_ = (audio_stream_idx_ >= 0 || video_stream_idx_ >= 0);
  }

  LOG_DEBUG("DONE, parser: %p, initialized: %d, audio: %d, video: %d", this,
            streams_initialized_, audio_stream_idx_ >= 0,
            video_stream_idx_ >= 0);

  return streams_initialized_;
}

void FFMpegDemuxer::UpdateAudioConfig() {
  LOG_DEBUG("audio index: %d", audio_stream_idx_);

  int channel_no;
  AVStream* s = format_context_->streams[audio_stream_idx_];
  LOG_DEBUG("audio ffmpeg duration: %lld %s", s->duration,
            s->duration == AV_NOPTS_VALUE ? "(AV_NOPTS_VALUE)" : "");

  audio_config_.codec_type = ConvertAudioCodec(s->codec->codec_id);
  audio_config_.sample_format = ConvertSampleFormat(s->codec->sample_fmt);
  if (s->codec->bits_per_coded_sample > 0) {
    audio_config_.bits_per_channel = s->codec->bits_per_coded_sample;
  } else {
    audio_config_.bits_per_channel =
        av_get_bytes_per_sample(s->codec->sample_fmt) * 8 / s->codec->channels;
  }
  audio_config_.channel_layout =
      ConvertChannelLayout(s->codec->channel_layout, s->codec->channels);
  audio_config_.samples_per_second = s->codec->sample_rate;
  if (audio_config_.codec_type == Samsung::NaClPlayer::AUDIOCODEC_TYPE_AAC) {
    audio_config_.codec_profile =
        ConvertAACAudioCodecProfile(s->codec->profile);
    // this method read channel_no, and modify
    // audio_config_.samples_per_second too
    channel_no =
        PrepareAACHeader(s->codec->extradata, s->codec->extradata_size);
    // if we read channel no change channel_layout,
    // (else we sould break for AAC ;( )
    if (channel_no >= 0)
      audio_config_.channel_layout =
          ConvertChannelLayout(s->codec->channel_layout, channel_no);
  }

  if (s->codec->extradata_size > 0) {
    audio_config_.extra_data.assign(
        s->codec->extradata, s->codec->extradata + s->codec->extradata_size);
  }
  char fourcc[20];
  av_get_codec_tag_string(fourcc, sizeof(fourcc), s->codec->codec_tag);
  LOG_DEBUG(
      "audio configuration - codec: %d, profile: %d, codec_tag: (%s), "
      "sample_format: %d, bits_per_channel: %d, channel_layout: %d, "
      "samples_per_second: %d",
      audio_config_.codec_type, audio_config_.codec_profile, fourcc,
      audio_config_.sample_format, audio_config_.bits_per_channel,
      audio_config_.channel_layout, audio_config_.samples_per_second);

  callback_dispatcher_.PostWork(callback_factory_.NewCallback(
      &FFMpegDemuxer::CallbackConfigInDispatcherThread, kAudio));
  LOG_DEBUG("audio configuration updated");
}

void FFMpegDemuxer::UpdateVideoConfig() {
  LOG_DEBUG("video index: %d", video_stream_idx_);

  AVStream* s = format_context_->streams[video_stream_idx_];
  LOG_DEBUG("video ffmpeg duration: %lld %s", s->duration,
            s->duration == AV_NOPTS_VALUE ? "(AV_NOPTS_VALUE)" : "");

  video_config_.codec_type = ConvertVideoCodec(s->codec->codec_id);
  switch (video_config_.codec_type) {
    case Samsung::NaClPlayer::VIDEOCODEC_TYPE_VP8:
      video_config_.codec_profile =
          Samsung::NaClPlayer::VIDEOCODEC_PROFILE_VP8_MAIN;
      break;
    case Samsung::NaClPlayer::VIDEOCODEC_TYPE_VP9:
      video_config_.codec_profile =
          Samsung::NaClPlayer::VIDEOCODEC_PROFILE_VP9_MAIN;
      break;
    case Samsung::NaClPlayer::VIDEOCODEC_TYPE_H264:
      video_config_.codec_profile =
          ConvertH264VideoCodecProfile(s->codec->profile);
      break;
    case Samsung::NaClPlayer::VIDEOCODEC_TYPE_MPEG2:
      video_config_.codec_profile =
          ConvertMPEG2VideoCodecProfile(s->codec->profile);
      break;
    default:
      video_config_.codec_profile =
          Samsung::NaClPlayer::VIDEOCODEC_PROFILE_UNKNOWN;
  }

  video_config_.frame_format = ConvertVideoFrameFormat(s->codec->pix_fmt);

  AVDictionaryEntry* webm_alpha =
      av_dict_get(s->metadata, "alpha_mode", NULL, 0);
  if (webm_alpha && !strcmp(webm_alpha->value, "1"))
    video_config_.frame_format = Samsung::NaClPlayer::VIDEOFRAME_FORMAT_YV12A;

  video_config_.size = Size(s->codec->coded_width, s->codec->coded_height);

  LOG_DEBUG("r_frame_rate %d. %d#", s->r_frame_rate.num, s->r_frame_rate.den);
  video_config_.frame_rate = Rational(s->r_frame_rate.num, s->r_frame_rate.den);

  if (s->codec->extradata_size > 0) {
    video_config_.extra_data.assign(
        s->codec->extradata, s->codec->extradata + s->codec->extradata_size);
  }

  char fourcc[20];
  av_get_codec_tag_string(fourcc, sizeof(fourcc), s->codec->codec_tag);
  LOG_DEBUG(
      "video configuration - codec: %d, profile: %d, codec_tag: (%s), "
      "frame: %d, visible_rect: %d %d ",
      video_config_.codec_type, video_config_.codec_profile, fourcc,
      video_config_.frame_format, video_config_.size.width,
      video_config_.size.height);

  callback_dispatcher_.PostWork(callback_factory_.NewCallback(
      &FFMpegDemuxer::CallbackConfigInDispatcherThread, kVideo));
  LOG_DEBUG("video configuration updated");
}

int FFMpegDemuxer::PrepareAACHeader(const uint8_t* data, size_t length) {
  LOG_DEBUG("");
  if (!data || length < 2) {
    LOG_DEBUG("empty extra data, it's needed to read aac config");
    return -1;
  }

  int32_t samples_rate_index;
  samples_rate_index = (((*data) & 0x3) << 1) | (*(data + 1)) >> 7;

  uint8_t channel_config;
  if (samples_rate_index != 15) {
    channel_config = (*(data + 1) & 0x78) >> 3;
  } else {
    return -1;  // we don't support custom frequencies
  }

  // Extern from ffmpeg file: mpeg4audio.c
  extern const int avpriv_mpeg4audio_sample_rates[];
  audio_config_.samples_per_second =
      avpriv_mpeg4audio_sample_rates[samples_rate_index];

  LOG_DEBUG("prepared AAC (ADTS) header template");
  return channel_config;
}

void FFMpegDemuxer::UpdateContentProtectionConfig() {
  LOG_DEBUG("protection data count: %u",
            format_context_->protection_system_data_count);
  if (format_context_->protection_system_data_count <= 0) return;

  for (uint32_t i = 0; i < format_context_->protection_system_data_count; ++i) {
    AVProtectionSystemSpecificData& system_data =
        format_context_->protection_system_data[i];

    if (SystemIdEqual(system_data.system_id, kPlayReadySystemId)) {
      std::vector<uint8_t> init_data;
      init_data.insert(init_data.begin(), system_data.pssh_box,
                       system_data.pssh_box + system_data.pssh_box_size);
      LOG_DEBUG("Found PlayReady init data (pssh box)");

      callback_dispatcher_.PostWork(callback_factory_.NewCallback(
          &FFMpegDemuxer::DrmInitCallbackInDispatcherThread, kDRMInitDataType,
          init_data));
      return;
    }
  }

  LOG_DEBUG("Couldn't find PlayReady init data! App supports only PlayReady");
  return;
}

void FFMpegDemuxer::DrmInitCallbackInDispatcherThread(int32_t,
    const std::string& type, const std::vector<uint8_t>& init_data) {
  if (drm_init_data_callback_)
    drm_init_data_callback_(type, init_data);
  else
    LOG_ERROR("ERROR: drm_init_data_callback_ is not initialized!");
}

unique_ptr<ElementaryStreamPacket> FFMpegDemuxer::MakeESPacketFromAVPacket(
    AVPacket* pkt) {
  auto es_packet = MakeUnique<ElementaryStreamPacket>(pkt->data, pkt->size);

  AVStream* s = format_context_->streams[pkt->stream_index];

  es_packet->SetPts(ToTimeTicks(pkt->pts, s->time_base) + timestamp_);
  es_packet->SetDts(ToTimeTicks(pkt->dts, s->time_base) + timestamp_);
  es_packet->SetDuration(ToTimeTicks(pkt->duration, s->time_base));
  es_packet->SetKeyFrame(pkt->flags == 1);

  AVEncInfo* enc_info = reinterpret_cast<AVEncInfo*>(
      av_packet_get_side_data(pkt, AV_PKT_DATA_ENCRYPT_INFO, NULL));

  if (!enc_info) return es_packet;

  es_packet->SetKeyId(enc_info->kid, kKidLength);
  es_packet->SetIv(enc_info->iv, enc_info->iv_size);
  for (uint32_t i = 0; i < enc_info->subsample_count; ++i) {
    es_packet->AddSubsample(enc_info->subsamples[i].bytes_of_clear_data,
                            enc_info->subsamples[i].bytes_of_enc_data);
  }

  return es_packet;
}
