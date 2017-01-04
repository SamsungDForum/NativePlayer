/*!
 * stream_manager.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Piotr Bałut
 */

#ifndef NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_PACKETS_MANAGER_H_
#define NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_PACKETS_MANAGER_H_

#include <common.h>

#include <array>
#include <queue>

#include "demuxer/elementary_stream_packet.h"
#include "demuxer/stream_demuxer.h"
#include "nacl_player/media_common.h"
#include "player/es_dash_player/stream_listener.h"
#include "player/es_dash_player/stream_manager.h"
#include "ppapi/utility/threading/lock.h"

class StreamManager;

/// @file
/// @brief This file defines the <code>PacketsManager</code> class.

/// @class StreamManager
/// This class synchronizes and feeds NaCl Player with
/// <code>ElementaryStreamPacket</code>s from multiple elementary streams
/// (represented by <code>StreamManager</code>s).
///
/// @see class <code>ElementaryStreamPacket</code>
/// @see class <code>StreamManager</code>

class PacketsManager : public StreamListener {
 public:
  PacketsManager();
  ~PacketsManager() override;

  void PrepareForSeek(Samsung::NaClPlayer::TimeTicks to_time);
  void OnEsPacket(StreamDemuxer::Message,
                  std::unique_ptr<ElementaryStreamPacket>);
  bool UpdateBuffer(Samsung::NaClPlayer::TimeTicks playback_time);
  void SetStream(StreamType type, std::shared_ptr<StreamManager> manager);

  void OnStreamConfig(const AudioConfig&) override;
  void OnStreamConfig(const VideoConfig&) override;
  void OnNeedData(StreamType type, int32_t bytes_max) override;
  void OnEnoughData(StreamType type) override;
  void OnSeekData(StreamType type,
                  Samsung::NaClPlayer::TimeTicks new_position)  override;

  bool IsEosReached() const;

   // This class encapsulates a stream object that is appendable to a stream in
   // a timely manner. Usually this means an ES packet, but a stream
   // configuration changed outside seek (i.e. during representation change)
   // also falls into this category.
  class BufferedStreamObject {
   public:
    BufferedStreamObject(StreamType type,
                         Samsung::NaClPlayer::TimeTicks time)
        : type_(type),
          time_(time) {}
    virtual ~BufferedStreamObject();
    virtual bool Append(StreamManager*) = 0;
    virtual bool IsKeyFrame() const = 0;
    StreamType type() const {
      return type_;
    }
    Samsung::NaClPlayer::TimeTicks time() const {
      return time_;
    }
    bool operator<(const BufferedStreamObject& another) const {
      return time_ < another.time_;
    }
    bool operator==(const BufferedStreamObject& another) const {
      // Stream objects are equal if they are literally equal, hence no kEps
      // and it's intentional.
      return time_ == another.time_;
    }
    bool operator!=(const BufferedStreamObject& another) const {
      return !(*this == another);
    }
    bool operator>(const BufferedStreamObject& another) const {
      return another < *this;
    }
    bool operator<=(const BufferedStreamObject& another) const {
      return !(another < *this);
    }
    bool operator>=(const BufferedStreamObject& another) const {
      return !(*this < another);
    }
    struct IsGreater {
      // for packets_ priority queue: lower timestamps go first
      bool operator()(
          const std::unique_ptr<BufferedStreamObject>& p1,
          const std::unique_ptr<BufferedStreamObject>& p2) const {
        return *p1 > *p2;
      }
    };
   private:
    StreamType type_;
    Samsung::NaClPlayer::TimeTicks time_;
  };  // class BufferedStreamObject
 private:
  /// This method assures that <code>packets_</code> buffer top packet can be
  /// safely used to start a playback after a seek operation. A good starting
  /// packet is a video keyframe, so this method essentially drops any audio or
  /// non-keyframe video packet until a video keyframe is encountered. If this
  /// happens, a seek operation is finished (i.e. <code>seeking_</code> flag is
  /// set to <code>false</code>).
  ///
  /// \pre Seeking operation must be in progress (i.e. <code>seeking_</code> is
  ///      set to <code>true</code>).
  /// \pre <code>packets_lock_</code> must be locked.
  ///
  /// @param[in] buffered_time A time for which both audio and video packets
  ///   are buffered. Any Packets in <code>packets_</code> buffer with a
  ///   <code>dts</code> value higher than <code>buffered_time</code> will not
  ///   be considered.
  void CheckSeekEndConditions(Samsung::NaClPlayer::TimeTicks buffered_time);

  /// Appends <code>ElementaryStreamPacket</code>s buffered in
  /// <code>packets_</code> buffer to Player for a playback. Only a number of
  /// packets with a <code>dts</code> value higher than
  /// <code>playback_time</code> will be sent. Packets in <code>packets_</code>
  /// buffer that have <code>dts</code> value higher than
  /// <code>buffered_time</code> are not considered for appending.
  ///
  /// \pre Seeking operation is NOT in progress (i.e. <code>seeking_</code> is
  ///      set to <code>false</code>).
  /// \pre <code>packets_lock_</code> must be locked.
  ///
  /// @param[in] playback_time A current playback position.
  /// @param[in] buffered_time A time for which both audio and video packets
  ///   are buffered.
  void AppendPackets(Samsung::NaClPlayer::TimeTicks playback_time,
                     Samsung::NaClPlayer::TimeTicks buffered_time);

  /// Checks if EOS is signalled on all streams by stream demuxers. Please note
  /// it might not be reached on packets manager side yet, i.e. there can still
  /// be packets that are needed to be sent before EOS can be sent to NaCl
  /// Player.
  bool IsEosSignalled() const;

  std::unique_ptr<BufferedStreamObject> CreateBufferedConfig(
      const AudioConfig&);

  std::unique_ptr<BufferedStreamObject> CreateBufferedConfig(
      const VideoConfig&);

  template <typename ConfigT>
  void HandleStreamConfig(StreamType stream, const ConfigT& config) {
    assert(stream < StreamType::MaxStreamTypes);
    auto stream_index = static_cast<uint32_t>(stream);
    if (!streams_[stream_index]) {
      LOG_ERROR("Received a configuration for a non-existing stream (%s).",
                stream_index == static_cast<int32_t>(StreamType::Video) ?
                    "VIDEO" : "AUDIO");
      return;
    }
    if (streams_[stream_index]->IsSeeking() ||
        !streams_[stream_index]->IsInitialized()) {
      // If stream is seeking or uninitialized, apply configuration
      // immediately:
      streams_[stream_index]->SetConfig(config);
    } else {
      // Otherwise enqueue configuration appliance after all packets from a
      // previous config are sent:
      packets_.push(CreateBufferedConfig(config));
    }

  }

  pp::Lock packets_lock_;
  typedef std::unique_ptr<BufferedStreamObject> BufferedStreamObjectPtr;
  std::priority_queue<BufferedStreamObjectPtr,
                      std::vector<BufferedStreamObjectPtr>,
                      typename BufferedStreamObject::IsGreater> packets_;

  /// If <code>true</code>, we are during a seek operation and cannot append
  /// any packets until we fill a buffer with a number of approperiate packets.
  ///
  /// @see method <code>PacketsManager::CheckSeekEndConditions()</code>
  bool seeking_;

  /// EOS is in effect when EOS count reaches number of streams.
  int eos_count_;

  std::array<bool,
            static_cast<int32_t>(StreamType::MaxStreamTypes)> seek_segment_set_;
  Samsung::NaClPlayer::TimeTicks seek_segment_video_time_;

  std::array<Samsung::NaClPlayer::TimeTicks,
             static_cast<int32_t>(StreamType::MaxStreamTypes)>
                 buffered_packets_timestamp_;

  std::array<std::shared_ptr<StreamManager>,
             static_cast<int32_t>(StreamType::MaxStreamTypes)> streams_;
};

#endif  // NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_PACKETS_MANAGER_H_
