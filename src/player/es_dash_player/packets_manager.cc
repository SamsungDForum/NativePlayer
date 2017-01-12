/*!
 * stream_manager.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Piotr Bałut
 */

#include "player/es_dash_player/packets_manager.h"

#include <limits>

using Samsung::NaClPlayer::TimeTicks;

namespace {

constexpr int kAudioStreamId = static_cast<int>(StreamType::Audio);
constexpr int kVideoStreamId = static_cast<int>(StreamType::Video);
// Determines how many seconds worth of packets should be appended to NaCl
// Player in advance. All available packets in a range:
// (last appended packet; current_playback_time + kAppendPacketsThreshold]
// will be appended upon every UpdateBuffer().
constexpr TimeTicks kAppendPacketsThreshold = 4.0f;  // seconds

class BufferedPacket : public PacketsManager::BufferedStreamObject {
 public:
  BufferedPacket(StreamType type,
                 std::unique_ptr<ElementaryStreamPacket> packet)
      : BufferedStreamObject(type, packet->GetDts()),
        packet_(std::move(packet)) {}
  ~BufferedPacket() override = default;
  bool Append(StreamManager* stream_manager) override {
    stream_manager->AppendPacket(std::move(packet_));
    return false;
  }
  bool IsKeyFrame() const override {
    return packet_->IsKeyFrame();
  }
  bool IsConfig() const override {
    return false;
  }
 private:
   std::unique_ptr<ElementaryStreamPacket> packet_;
};

template <typename ConfigT, StreamType stream_type>
class BufferedConfig : public PacketsManager::BufferedStreamObject {
 public:
  BufferedConfig(TimeTicks time,
                 const ConfigT& config)
      : BufferedStreamObject(stream_type, time),
        config_(config) {}
  ~BufferedConfig() override = default;
  bool Append(StreamManager* stream_manager) override {
    return stream_manager->SetConfig(config_);
  }
  bool IsKeyFrame() const override {
    return false;
  }
  bool IsConfig() const override {
      return true;
  }
 private:
   ConfigT config_;
};

typedef BufferedConfig<AudioConfig, StreamType::Audio> BufferedAudioConfig;
typedef BufferedConfig<VideoConfig, StreamType::Video> BufferedVideoConfig;

} // anonymous namespace

PacketsManager::BufferedStreamObject::~BufferedStreamObject() = default;

PacketsManager::PacketsManager()
    : seeking_(false),
      eos_count_(0),
      seek_segment_set_{ {false, false} },
      seek_segment_video_time_(0),
      buffered_packets_timestamp_{ {0, 0} } {
}

PacketsManager::~PacketsManager() = default;

void PacketsManager::PrepareForSeek(Samsung::NaClPlayer::TimeTicks to_time) {
  pp::AutoLock critical_section(packets_lock_);

  // Append pending representation changes
  BufferedStreamObjectPtr last_config;
  while (!packets_.empty()) {
    const auto& packet = packets_.top();
    if (packet->IsConfig())
      const_cast<decltype(last_config)&>(packets_.top()).swap(last_config);
    packets_.pop();
  }
  if (last_config) {
    auto stream_id = static_cast<int>(last_config->type());
    if (streams_[stream_id]) {
      last_config->Append(streams_[stream_id].get());
    }
  }

  // Stream managers will not send packets while they are seeking streams.
  // If streamManager sends packet, it means stream is at a new position. This
  // manager seek ends when it receives a keyframe packet for each stream.
  seeking_ = true;
  seek_segment_set_[kAudioStreamId] = false;
  seek_segment_set_[kVideoStreamId] = false;
  seek_segment_video_time_ = 0;
  eos_count_ = 0;
  buffered_packets_timestamp_[kAudioStreamId] = 0;
  buffered_packets_timestamp_[kVideoStreamId] = 0;
}

void PacketsManager::OnEsPacket(
    StreamDemuxer::Message message,
    std::unique_ptr<ElementaryStreamPacket> packet) {
  switch (message) {
  case StreamDemuxer::kEndOfStream:
    ++eos_count_;
    break;
  case StreamDemuxer::kAudioPkt:
  case StreamDemuxer::kVideoPkt:
  {
    auto type = (message == StreamDemuxer::kAudioPkt ? StreamType::Audio :
                            StreamType::Video);
    auto stream_index = static_cast<int32_t>(type);
    if (!streams_[stream_index]) {
      LOG_ERROR("Received a packet for a non-existing stream (%s).",
                type == StreamType::Video ? "VIDEO" : "AUDIO");
      break;
    }
    if (streams_[stream_index]->IsSeeking())
      break;
    pp::AutoLock critical_section(packets_lock_);
    buffered_packets_timestamp_[stream_index] = packet->GetDts();
    packets_.emplace(MakeUnique<BufferedPacket>(type, std::move(packet)));
    break;
  };
  default:
    LOG_ERROR("Received an unsupported message type!");
  }
}

void PacketsManager::OnStreamConfig(const AudioConfig& config) {
    HandleStreamConfig(StreamType::Audio, config);
}

std::unique_ptr<PacketsManager::BufferedStreamObject>
PacketsManager::CreateBufferedConfig(const AudioConfig& config) {
  return MakeUnique<BufferedAudioConfig>(
      buffered_packets_timestamp_[kAudioStreamId] + kEps, config);
}

void PacketsManager::OnStreamConfig(const VideoConfig& config) {
    HandleStreamConfig(StreamType::Video, config);
}

std::unique_ptr<PacketsManager::BufferedStreamObject>
PacketsManager::CreateBufferedConfig(const VideoConfig& config) {
  return MakeUnique<BufferedVideoConfig>(
      buffered_packets_timestamp_[kVideoStreamId] + kEps, config);
}

void PacketsManager::OnNeedData(StreamType type, int32_t bytes_max) {
}

void PacketsManager::OnEnoughData(StreamType type) {
}

void PacketsManager::OnSeekData(StreamType type,
                                TimeTicks new_time) {
  if (streams_[kAudioStreamId] && type == StreamType::Audio) {
    seek_segment_set_[kAudioStreamId] = true;
  } else if (streams_[kVideoStreamId] && type == StreamType::Video) {
    // If video track is present, we want to align seek to video keyframe
    // (which is at the beginning start of a segment).
    seek_segment_set_[kVideoStreamId] = true;
    TimeTicks video_segment_start;
    TimeTicks video_segment_duration;
    streams_[kVideoStreamId]->SetSegmentToTime(new_time,
        &video_segment_start, &video_segment_duration);
    seek_segment_video_time_ = video_segment_start;
    LOG_DEBUG("Seek to video segment: %f [s] ... %f [s]", video_segment_start,
        video_segment_start + video_segment_duration);
  } else {
    LOG_ERROR("Received an OnSeekData event for a non-existing stream (%s).",
              type == StreamType::Video ? "VIDEO" : "AUDIO");
    return;
  }

  // If there is no video track, then just continue with seeking audio.
  // Otherwise allow seeking audio only after seeking video (i.e. when video
  // seek time is determined).
  auto video_segment_set = !streams_[kVideoStreamId] ||
                           seek_segment_set_[kVideoStreamId];
  auto audio_segment_set = seek_segment_set_[kAudioStreamId];
  if (streams_[kAudioStreamId] && audio_segment_set && video_segment_set) {
    // Align audio seek time to video seek time if video track is present.
    auto seek_audio_to_time = streams_[kVideoStreamId] ?
                               seek_segment_video_time_ : new_time;
    TimeTicks audio_segment_start;
    TimeTicks audio_segment_duration;
    streams_[kAudioStreamId]->SetSegmentToTime(seek_audio_to_time,
                                               &audio_segment_start,
                                               &audio_segment_duration);
    LOG_DEBUG("Seek to audio segment: %f [s] ... %f [s]", audio_segment_start,
        audio_segment_start + audio_segment_duration);
  }
}

void PacketsManager::CheckSeekEndConditions(
    Samsung::NaClPlayer::TimeTicks buffered_time) {
  // Seeks ends when:
  // - a video keyframe is received (if video stream is present)
  // - an audio keyframe is received (otherwise)
  // All packets before the one that ends seek must be dropped. It's worth
  // noting that all audio frames are keyframes.
  assert(seeking_);
  BufferedStreamObjectPtr last_config;
  while (!packets_.empty()) {
    const auto& packet = packets_.top();
    auto packet_playback_position = packet->time();
    if (buffered_time < packet_playback_position)
      break;
    if (((streams_[kVideoStreamId] && packet->type() == StreamType::Video) ||
         (!streams_[kVideoStreamId] && streams_[kAudioStreamId] &&
         packet->type() == StreamType::Audio)) && packet->IsKeyFrame()) {
      seeking_ = false;
      LOG_DEBUG("Seek finishing at %f [s] %s packet... buffered packets: %u",
          packet->time(),
          packet->type() == StreamType::Video ? "VIDEO" : "AUDIO",
          packets_.size());
      break;
    } else {
      if (packet->IsConfig())
        const_cast<decltype(last_config)&>(packets_.top()).swap(last_config);
      packets_.pop();
    }
  }
  if (last_config)
    packets_.push(std::move(last_config));
}

void PacketsManager::AppendPackets(TimeTicks playback_time,
                                   TimeTicks buffered_time) {
  assert(!seeking_);
  if (packets_.empty())
    return;
  // Append packets to respective streams:
  auto packet_playback_position = packets_.top()->time();
  while (packet_playback_position - playback_time < kAppendPacketsThreshold &&
         packet_playback_position < buffered_time) {
    auto stream_id = static_cast<int>(packets_.top()->type());
    if (streams_[stream_id]) {
      std::unique_ptr<BufferedStreamObject> stream_object;
      // Moving object out of priority queue breaks it ordering, hence
      // const_cast immediately followed by a pop to simulate atomic
      // "pop top element from queue to local variable".
      const_cast<decltype(stream_object)&>(packets_.top()).swap(stream_object);
      packets_.pop();
      // True means that we should break the loop and try again eg. audio/video
      // config has change and we need some time to finish initialization
      if (stream_object->Append(streams_[stream_id].get()))
        break;
    } else {
      LOG_ERROR("Invalid stream index: %d", stream_id);
    }
    if (packets_.empty())
      break;
  }
}

bool PacketsManager::IsEosSignalled() const {
  int stream_count = 0;
  for (const auto& stream : streams_) {
    if (stream)
      ++stream_count;
  }
  return eos_count_ == stream_count;
}

bool PacketsManager::IsEosReached() const {
  return packets_.empty() && IsEosSignalled();
}

bool PacketsManager::UpdateBuffer(
    Samsung::NaClPlayer::TimeTicks playback_time) {
  pp::AutoLock critical_section(packets_lock_);

  // Determine max time we have packets for:
  auto buffered_time = std::numeric_limits<TimeTicks>::max();

  // Upon EOS all packets needs to be flushed, so checking max time should be
  // skipped.
  if (!IsEosSignalled()) {
    if (streams_[kVideoStreamId] && buffered_time >
        buffered_packets_timestamp_[kVideoStreamId])
      buffered_time = buffered_packets_timestamp_[kVideoStreamId];

    if (streams_[kAudioStreamId] && buffered_time >
        buffered_packets_timestamp_[kAudioStreamId])
      buffered_time = buffered_packets_timestamp_[kAudioStreamId];
  }

  if (seeking_)
    CheckSeekEndConditions(buffered_time);

  if (!seeking_)
    AppendPackets(playback_time, buffered_time);

  return !packets_.empty();
}

void PacketsManager::SetStream(StreamType type,
                               std::shared_ptr<StreamManager> manager) {
  assert(type < StreamType::MaxStreamTypes);
  streams_[static_cast<int32_t>(type)] = std::move(manager);
}
