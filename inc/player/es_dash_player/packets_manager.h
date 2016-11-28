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

  void OnNeedData(StreamType type, int32_t bytes_max) override;
  void OnEnoughData(StreamType type) override;
  void OnSeekData(StreamType type,
                  Samsung::NaClPlayer::TimeTicks new_position)  override;

 private:
  struct BufferedPacket {
    StreamType type;
    std::unique_ptr<ElementaryStreamPacket> packet;
    BufferedPacket(StreamType t, std::unique_ptr<ElementaryStreamPacket> p)
      : type(t), packet(std::move(p)) {}
    bool operator>(const BufferedPacket& another) const {
      // for packets_ priority queue: lower timestamps go first
      return packet->GetDts() > another.packet->GetDts();
    }
  };

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

  pp::Lock packets_lock_;
  std::priority_queue<BufferedPacket, std::vector<BufferedPacket>,
                      std::greater<BufferedPacket>> packets_;

  /// If <code>true</code>, we are during a seek operation and cannot append
  /// any packets until we fill a buffer with a number of approperiate packets.
  ///
  /// @see method <code>PacketsManager::CheckSeekEndConditions()</code>
  bool seeking_;

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
