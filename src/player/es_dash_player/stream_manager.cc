/*!
 * url_player_controller.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Jacob Tarasiewicz
 */

#include "player/es_dash_player/stream_manager.h"

#include <stdlib.h>
#include <functional>
#include <memory>

#include "ppapi/utility/threading/lock.h"

#include "common.h"
#include "demuxer/elementary_stream_packet.h"
#include "demuxer/stream_demuxer.h"

#include "async_data_provider.h"
#include "media_segment.h"

using pp::AutoLock;
using Samsung::NaClPlayer::AudioElementaryStream;
using Samsung::NaClPlayer::DRMType;
using Samsung::NaClPlayer::DRMType_Unknown;
using Samsung::NaClPlayer::DRMType_Playready;
using Samsung::NaClPlayer::ElementaryStream;
using Samsung::NaClPlayer::ElementaryStreamListener;
using Samsung::NaClPlayer::ErrorCodes;
using Samsung::NaClPlayer::ESDataSource;
using Samsung::NaClPlayer::ESPacket;
using Samsung::NaClPlayer::MediaPlayer;
using Samsung::NaClPlayer::TimeTicks;
using Samsung::NaClPlayer::VideoElementaryStream;

using std::placeholders::_1;
using std::placeholders::_2;
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

const TimeTicks kNextSegmentTimeThreshold = 7.0f;    // in seconds
const TimeTicks kAppendPacketsMinTimeBuffer = 4.0f;  // in seconds

class StreamManager::Impl :
    public std::enable_shared_from_this<StreamManager::Impl> {
 public:
  explicit Impl(pp::InstanceHandle instance, StreamType type);
  ~Impl();
  bool Initialize(
       std::unique_ptr<MediaSegmentSequence> segment_sequence,
       std::shared_ptr<Samsung::NaClPlayer::ESDataSource> es_data_source,
       std::function<void(StreamType)> stream_configured_callback,
       Samsung::NaClPlayer::DRMType drm_type,
       std::shared_ptr<ElementaryStreamListener> listener);

  void SetMediaSegmentSequence(
       std::unique_ptr<MediaSegmentSequence> segment_sequence);

  Samsung::NaClPlayer::TimeTicks UpdateBuffer(
      Samsung::NaClPlayer::TimeTicks playback_time);

  bool IsInitialized() { return initialized_; }

  void OnNeedData(int32_t bytes_max);
  void OnEnoughData();
  void OnSeekData(Samsung::NaClPlayer::TimeTicks new_position);

 private:
  bool InitParser();
  bool ParseInitSegment();
  void GotSegment(std::unique_ptr<MediaSegment> segment);

  void OnAudioConfig(const AudioConfig& audio_config);
  void OnVideoConfig(const VideoConfig& video_config);
  void OnDRMInitData(const std::string& type,
                     const std::vector<uint8_t>& init_data);

  void OnESPacket(StreamDemuxer::Message msg,
                  std::unique_ptr<ElementaryStreamPacket> es_pkt);

  pp::InstanceHandle instance_handle_;
  StreamType stream_type_;

  std::unique_ptr<StreamDemuxer> demuxer_;
  std::unique_ptr<AsyncDataProvider> data_provider_;

  pp::CompletionCallbackFactory<Impl> callback_factory_;
  pp::Lock packets_lock_;

  std::list<std::unique_ptr<ElementaryStreamPacket>> packets_;
  std::shared_ptr<Samsung::NaClPlayer::ElementaryStream> elementary_stream_;
  std::function<void(StreamType)> stream_configured_callback_;

  bool drm_initialized_;
  bool end_of_stream_;
  bool exited_;
  bool init_seek_;
  bool initialized_;
  bool seeking_;
  bool segment_pending_;
  bool drop_segment_;

  AudioConfig audio_config_;
  VideoConfig video_config_;
  Samsung::NaClPlayer::DRMType drm_type_;

  Samsung::NaClPlayer::TimeTicks buffered_segments_time_;
  Samsung::NaClPlayer::TimeTicks buffered_packets_time_;
  Samsung::NaClPlayer::TimeTicks need_time_;
  Samsung::NaClPlayer::TimeTicks new_position_;
  int32_t bytes_max_;
};  // class StreamManager::Impl

StreamManager::Impl::Impl(pp::InstanceHandle instance, StreamType type)
    : instance_handle_(instance),
      stream_type_(type),
      data_provider_(),
      callback_factory_(this),
      packets_lock_(),
      drm_initialized_(false),
      end_of_stream_(false),
      exited_(false),
      init_seek_(false),
      initialized_(false),
      seeking_(false),
      segment_pending_(false),
      drop_segment_(false),
      drm_type_(Samsung::NaClPlayer::DRMType_Unknown),
      buffered_segments_time_(0.0f),
      buffered_packets_time_(0.0f),
      need_time_(0.0f),
      new_position_(0.0f),
      bytes_max_(0) {}

StreamManager::Impl::~Impl() {
  LOG_DEBUG("");
  exited_ = true;
}

void StreamManager::Impl::OnNeedData(int32_t bytes_max) {
  LOG_DEBUG("Type: %s size: %d",
            stream_type_ == StreamType::Video ? "VIDEO" : "AUDIO", bytes_max);
  {
    AutoLock critical_section(packets_lock_);
    bytes_max_ = bytes_max;
  }
}

void StreamManager::Impl::OnEnoughData() {
  LOG_DEBUG("Type: %s", stream_type_ == StreamType::Video ? "VIDEO" : "AUDIO");
  {
    AutoLock critical_section(packets_lock_);
    bytes_max_ = 0;
  }
}

void StreamManager::Impl::OnSeekData(TimeTicks new_position) {
  LOG_INFO("Type: %s position:: %f",
      stream_type_ == StreamType::Video ? "VIDEO" : "AUDIO", new_position);
  if (!init_seek_) {
    init_seek_ = true;
    return;
  }
  {
    AutoLock critical_section(packets_lock_);
    buffered_packets_time_ = 0.0;
    buffered_segments_time_ = 0.0;
    packets_.clear();
    seeking_ = true;
    need_time_ = new_position - data_provider_->AverageSegmentDuration();
    new_position_ = new_position;
    if (need_time_ < 0.0) need_time_ = 0.0;
    packets_.clear();
  }
  demuxer_.reset();
  LOG_INFO("Parser reset");
  if (InitParser()) {
    ParseInitSegment();
    data_provider_->SetNextSegmentToTime(need_time_);
  }
}

bool StreamManager::Impl::Initialize(
    unique_ptr<MediaSegmentSequence> segment_sequence,
    shared_ptr<ESDataSource> es_data_source,
    std::function<void(StreamType)> stream_configured_callback,
    DRMType drm_type,
    std::shared_ptr<ElementaryStreamListener> listener) {
  LOG_DEBUG("");
  if (!stream_configured_callback) {
    LOG_ERROR(" callback is null!");
    return false;
  }
  stream_configured_callback_ = stream_configured_callback;
  drm_type_ = drm_type;
  auto callback = WeakBind(&StreamManager::Impl::GotSegment,
      shared_from_this(), _1);
  data_provider_ = MakeUnique<AsyncDataProvider>(
      instance_handle_, callback);
  data_provider_->SetMediaSegmentSequence(std::move(segment_sequence));

  int32_t result = ErrorCodes::BadArgument;
  // Add a stream to ESDataSource
  if (stream_type_ == StreamType::Video) {
    auto video_stream = make_shared<VideoElementaryStream>();
    result = es_data_source->AddStream(*video_stream, listener);
    elementary_stream_ =
        std::static_pointer_cast<ElementaryStream>(video_stream);
  } else if (stream_type_ == StreamType::Audio) {
    auto audio_stream = make_shared<AudioElementaryStream>();
    result = es_data_source->AddStream(*audio_stream, listener);
    elementary_stream_ =
        std::static_pointer_cast<ElementaryStream>(audio_stream);
  }

  if (result != ErrorCodes::Success) {
    LOG_ERROR("Failed to AddStream, type: %d, result: %d", stream_type_,
              result);
    return false;
  }

  if (!elementary_stream_) {
    return false;
  }

  // Initialize stream parser
  if (!InitParser()) {
    LOG_ERROR("Failed to initialize parser or config listeners");
    return false;
  }

  return ParseInitSegment();
}

bool StreamManager::Impl::InitParser() {
  StreamDemuxer::Type demuxer_type;
  switch (stream_type_) {
    case StreamType::Video:
      demuxer_type = StreamDemuxer::kVideo;
      break;
    case StreamType::Audio:
      demuxer_type = StreamDemuxer::kAudio;
      break;
    default:
      demuxer_type = StreamDemuxer::kUnknown;
  }

  demuxer_ = StreamDemuxer::Create(instance_handle_, demuxer_type);

  if (!demuxer_) {
    LOG_ERROR("Failed to construct a FFMpegStreamParser");
  }

  if (!demuxer_->Init(WeakBind(&StreamManager::Impl::OnESPacket,
      shared_from_this(), _1, _2), pp::MessageLoop::GetCurrent()))
    return false;

  bool ok = demuxer_->SetAudioConfigListener(
                WeakBind(&StreamManager::Impl::OnAudioConfig,
                          shared_from_this(), _1));

  ok = ok &&
       demuxer_->SetVideoConfigListener(
           WeakBind(&StreamManager::Impl::OnVideoConfig,
                     shared_from_this(), _1));

  if (drm_type_ != DRMType_Unknown) {
    ok = ok &&
         demuxer_->SetDRMInitDataListener(
             WeakBind(&StreamManager::Impl::OnDRMInitData,
                       shared_from_this(), _1, _2));
  }
  return ok;
}

bool StreamManager::Impl::ParseInitSegment() {
  // Download initialization segment.
  vector<uint8_t> init_segment;
  if (!data_provider_->GetInitSegment(&init_segment)) {
    LOG_ERROR("Failed to download initialization segment!");
    return false;
  }

  if (init_segment.empty()) {
    LOG_ERROR("Initialization segment is empty!");
    return false;
  }

  demuxer_->Parse(init_segment);
  return true;
}

TimeTicks StreamManager::Impl::UpdateBuffer(TimeTicks playback_time) {
  LOG_DEBUG("stream manager: %p, playback_time: %f, buffered time: %f", this,
            playback_time, buffered_packets_time_);

  if (!elementary_stream_) {
    LOG_DEBUG("elementary stream is not initialized!");
    return buffered_packets_time_;
  }

  // Check if we need to request next segment download.
  if (!segment_pending_) {
    if (buffered_segments_time_ - playback_time < kNextSegmentTimeThreshold) {
      LOG_DEBUG("Requesting next segment");
      bool has_more_segments = data_provider_->RequestNextDataSegment();
      if (has_more_segments) {
        segment_pending_ = true;
      } else {
        LOG_DEBUG("There are no more segments to load");
      }
    }
  }

  // Check appended packets state.
  int32_t appended_bytes = 0;
  while (buffered_packets_time_ - playback_time < kAppendPacketsMinTimeBuffer) {
    AutoLock critical_section(packets_lock_);
    if (end_of_stream_ && packets_.empty()) {
      buffered_packets_time_ = kEndOfStream;
      break;
    }

    while (!packets_.empty() &&
           (packets_.front()->GetPts() < std::min(playback_time, new_position_))
               ) {
      LOG_DEBUG("dropping packets %f", packets_.front()->GetPts());
      packets_.pop_front();
    }

    if (packets_.empty()) {
      LOG_DEBUG("no packets to append");
      break;
    } else if (appended_bytes >= bytes_max_) {
      LOG_DEBUG("bytes limit reached! sent: %d, max: %d", appended_bytes,
                bytes_max_);
      break;
    }

    const ElementaryStreamPacket* packet = packets_.front().get();
    int32_t ret = ErrorCodes::Success;
    const char* fname = "";
    if (!packet->IsEncrypted()) {
      fname = "AppendPacket";
      ret = elementary_stream_->AppendPacket(packet->GetESPacket());
    } else {
      fname = "AppendEncryptedPacket";
      ret = elementary_stream_->AppendEncryptedPacket(
          packet->GetESPacket(), packet->GetEncryptionInfo());
    }

    LOG_DEBUG("stream: %s , %p, %s ret: %d, packet pts: %f",
              stream_type_ == StreamType::Video ? "VIDEO" : "AUDIO", this,
              fname, ret, packet->GetPts());
    if (ret == ErrorCodes::Success) {
      appended_bytes += packet->GetDataSize();
      buffered_packets_time_ = packet->GetPts();
      packets_.pop_front();
    } else {
      LOG_DEBUG("Failed to AppendPacket! Error code: %d", ret);
      break;
    }
  }

  LOG_DEBUG("Finishing");

  return buffered_packets_time_;
}

void StreamManager::Impl::SetMediaSegmentSequence(
    std::unique_ptr<MediaSegmentSequence> segment_sequence) {
  LOG_DEBUG("");
  if (segment_pending_) drop_segment_ = true;
  data_provider_->SetMediaSegmentSequence(std::move(segment_sequence));

  // if content is protected we need to initialize new parser
  if (drm_initialized_) {
    seeking_ = true;
    drop_segment_ = false;
    need_time_ = buffered_segments_time_;
    demuxer_.reset();
    drm_initialized_ = false;
    LOG_INFO("Parser reset");
    if (InitParser()) ParseInitSegment();
  }
  data_provider_->SetNextSegmentToTime(
      buffered_segments_time_ + data_provider_->AverageSegmentDuration());

  LOG_DEBUG("SetMediaSegmentSequence changed segments in data provider");
}

void StreamManager::Impl::GotSegment(std::unique_ptr<MediaSegment> segment) {
  LOG_INFO("Got segment: duration: %f, data.size(): %d, timestamp: %f",
      segment->duration_, segment->data_.size(), segment->timestamp_);
  segment_pending_ = false;
  if (seeking_ && (need_time_ < segment->duration_ + segment->timestamp_) &&
      (need_time_ >= segment->timestamp_)) {
    seeking_ = false;
    demuxer_->SetTimestamp(segment->timestamp_);
  } else if (seeking_) {
    return;
  } else if (drop_segment_) {
    LOG_DEBUG("Dropped after setting new segment sequence");
    drop_segment_ = false;
    return;
  }

  buffered_segments_time_ =
      static_cast<TimeTicks>(segment->duration_ + segment->timestamp_);
  demuxer_->Parse(segment->data_);
}

void StreamManager::Impl::OnESPacket(StreamDemuxer::Message msg,
                               unique_ptr<ElementaryStreamPacket> es_pkt) {
  LOG_DEBUG("Message from StreamParser - type: %d", msg);
  switch (msg) {
    case StreamDemuxer::kEndOfStream: {
      AutoLock critical_section(packets_lock_);
      end_of_stream_ = true;
      break;
    }
    case StreamDemuxer::kAudioPkt:
    case StreamDemuxer::kVideoPkt:
      LOG_DEBUG("%s , es_pkt pts %f, dts %f, dur %f, key %d, size %d",
                stream_type_ == StreamType::Video ? "VIDEO" : "AUDIO",
                es_pkt->GetESPacket().pts, es_pkt->GetESPacket().dts,
                es_pkt->GetESPacket().duration,
                es_pkt->GetESPacket().is_key_frame, es_pkt->GetESPacket().size);
      {
        AutoLock critical_section(packets_lock_);
        if (seeking_) return;
        packets_.push_back(std::move(es_pkt));
      }
      break;
    default:
      LOG_DEBUG("Not supported message type received!");
  }
}

void StreamManager::Impl::OnAudioConfig(const AudioConfig& audio_config) {
  LOG_INFO("OnAudioConfig codec_type: %d!", audio_config.codec_type);
  LOG_DEBUG(
      "audio configuration - codec: %d, profile: %d, sample_format: %d,"
      " bits_per_channel: %d, channel_layout: %d, samples_per_second: %d",
      audio_config.codec_type, audio_config.codec_profile,
      audio_config.sample_format, audio_config.bits_per_channel,
      audio_config.channel_layout, audio_config.samples_per_second);

  if (audio_config_ == audio_config) {
    LOG_INFO("The same config as before");
    return;
  }

  audio_config_ = audio_config;
  if (stream_type_ == StreamType::Audio) {
    auto audio_stream =
        std::static_pointer_cast<AudioElementaryStream>(elementary_stream_);
    audio_stream->SetAudioCodecType(audio_config.codec_type);
    audio_stream->SetAudioCodecProfile(audio_config.codec_profile);
    audio_stream->SetSampleFormat(audio_config.sample_format);
    audio_stream->SetChannelLayout(audio_config.channel_layout);
    audio_stream->SetBitsPerChannel(audio_config.bits_per_channel);
    audio_stream->SetSamplesPerSecond(audio_config.samples_per_second);
    audio_stream->SetCodecExtraData(audio_config.extra_data.size(),
                                    &audio_config.extra_data.front());

    int32_t ret = audio_stream->InitializeDone();
    LOG_DEBUG("audio - InitializeDone: %d", ret);

    if (ret == ErrorCodes::Success && !initialized_) {
      initialized_ = true;
      stream_configured_callback_(stream_type_);
    }
  } else {
    LOG_DEBUG("Check this - there should be no other than audio stream");
  }
}

void StreamManager::Impl::OnVideoConfig(const VideoConfig& video_config) {
  LOG_INFO("OnVideoConfig codec_type: %d!", video_config.codec_type);
  LOG_DEBUG(
      "video configuration - codec: %d, profile: %d, frame: %d "
      "visible_rect: %d %d ",
      video_config.codec_type, video_config.codec_profile,
      video_config.frame_format, video_config.size.width,
      video_config.size.height);
  if (video_config_ == video_config) {
    LOG_INFO("The same config as before");
    return;
  }

  video_config_ = video_config;
  if (stream_type_ == StreamType::Video) {
    auto video_stream =
        std::static_pointer_cast<VideoElementaryStream>(elementary_stream_);
    video_stream->SetVideoCodecType(video_config.codec_type);
    video_stream->SetVideoCodecProfile(video_config.codec_profile);
    video_stream->SetVideoFrameFormat(video_config.frame_format);
    video_stream->SetVideoFrameSize(video_config.size);
    video_stream->SetFrameRate(video_config.frame_rate);
    video_stream->SetCodecExtraData(video_config.extra_data.size(),
                                    &video_config.extra_data.front());

    int32_t ret = video_stream->InitializeDone();
    LOG_DEBUG("video - InitializeDone: %d", ret);

    if (ret == ErrorCodes::Success && !initialized_) {
      initialized_ = true;
      stream_configured_callback_(stream_type_);
    }
  } else {
    LOG_DEBUG("Check this - there should be no other than video stream");
  }
}

void StreamManager::Impl::OnDRMInitData(const std::string& type,
                                  const std::vector<uint8_t>& init_data) {
  LOG_DEBUG("stream type: %d, init data type: %s, init_data.size(): %d",
            stream_type_, type.c_str(), init_data.size());
  if (drm_initialized_) {
    LOG_INFO("DRM initialized already");
    return;
  }
  LOG_DEBUG("init_data hex str: [[%s]]",
            ToHexString(init_data.size(), init_data.data()).c_str());

  int32_t ret = elementary_stream_->SetDRMInitData(
      type, init_data.size(), static_cast<const void*>(init_data.data()));
  if (ret == ErrorCodes::Success) drm_initialized_ = true;
  LOG_DEBUG("SetDRMInitData returned: %d", ret);
}

// end of PIMPL implementation

StreamManager::StreamManager(pp::InstanceHandle instance, StreamType type)
  :pimpl_(MakeUnique<Impl>(instance, type)) {
}
StreamManager::~StreamManager() {
}
void StreamManager::OnNeedData(int32_t bytes_max) {
  pimpl_->OnNeedData(bytes_max);
}
void StreamManager::OnEnoughData() {
  pimpl_->OnEnoughData();
}
void StreamManager::OnSeekData(TimeTicks new_position) {
  pimpl_->OnSeekData(new_position);
}
bool StreamManager::IsInitialized() {
  return pimpl_->IsInitialized();
}
bool StreamManager::Initialize(
    unique_ptr<MediaSegmentSequence> segment_sequence,
    shared_ptr<ESDataSource> es_data_source,
    std::function<void(StreamType)> stream_configured_callback,
    DRMType drm_type) {

  return pimpl_->Initialize(std::move(segment_sequence), es_data_source,
                     stream_configured_callback, drm_type, shared_from_this());
}
TimeTicks StreamManager::UpdateBuffer(TimeTicks playback_time) {
  return pimpl_->UpdateBuffer(playback_time);
}

void StreamManager::SetMediaSegmentSequence(
    std::unique_ptr<MediaSegmentSequence> segment_sequence) {
  pimpl_->SetMediaSegmentSequence(std::move(segment_sequence));
}


