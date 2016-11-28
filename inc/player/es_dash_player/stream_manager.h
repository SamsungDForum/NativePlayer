/*!
 * stream_manager.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Jacob Tarasiewicz
 */

#ifndef NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_STREAM_MANAGER_H_
#define NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_STREAM_MANAGER_H_

#include <common.h>
#include <list>
#include <memory>
#include <string>
#include <vector>

#include "nacl_player/elementary_stream_listener.h"
#include "nacl_player/es_data_source.h"
#include "nacl_player/media_common.h"
#include "nacl_player/media_player.h"
#include "ppapi/cpp/instance.h"

#include "dash/media_segment_sequence.h"
#include "player/es_dash_player/packets_manager.h"
#include "demuxer/stream_demuxer.h"

class ElementaryStreamPacket;
class StreamListener;

/// @file
/// @brief This file defines the <code>StreamManager</code> class.

/// @class StreamManager
/// This class manages a single NaCl Player elementary stream.
///
/// <code>StreamManager</code> responsibility is to manage a single NaCl Player
/// elementary stream (i.e. either an audio or a video stream, each stream
/// managed by a separate <code>StreamManager</code> instance). The elementary
/// stream is represented by a <code>MediaSegmentSequence</code> object
/// associated with an instance of this class. Media segments are acquired by
/// the <code>StreamManager</code> object during playback and then demuxed into
/// a series of <code>ElementaryStreamPacket</code> objects which are delivered
/// to NaCl Player. Demuxing is done by using <code>StreamDemuxer</code>.
///
/// This class implements the <code>ElementaryStreamListener</code> interface,
/// which allows to receive notifications regarding an internal NaCl Player
/// elementary stream packet buffer's state. This information is used to
/// control a speed at which elementary stream packets are sent to NaCl Player.
///
/// It is <code>StreamManager</code> responsibility to configure an elementary
/// stream it manages. When the <code>StreamDemuxer</code> object discovers a
/// configuration of the associated <code>MediaSegmentSequence</code>, this
/// class properly configures NaCl Player using the discovered configuration.
///
/// At any given time, an initialized <code>StreamManager</code> manages only
/// one particular representation of the stream, which is associated with the
/// used <code>MediaSegmentSequence</code> object. However, the representation
/// (media segment sequence) can be changed.
///
/// Sending <code>ElementaryStreamPacket</code>s to NaCl Player should be
/// synchronized when packets originate from multiple strams.
/// <code>StreamManager</code> does not send packets directly to NaCl Player,
/// but sends them to <code>EsPacketManager</code> which assures streams
/// synchronization.
///
/// @see class <code>MediaSegmentSequence</code>
/// @see class <code>StreamDemuxer</code>
/// @see class <code>Samsung::NaClPlayer::ElementaryStreamListener</code>

class StreamManager : public Samsung::NaClPlayer::ElementaryStreamListener,
                      public std::enable_shared_from_this<StreamManager> {
 public:
  /// Creates a <code>StreamManager</code> object and opens a stream of a given
  /// type in a give player. Newly created object must be initialized using the
  /// <code>Initialize()</code> method before use.
  ///
  /// @note Please note at most one <code>StreamManager</code> object per stream
  ///   type should exist for any given player at any given time.
  ///
  /// @param[in] instance An <code>InstanceHandle</code> identifying a Native
  ///   Player object.
  /// @param[in] type A stream type for which the <code>StreamManager</code>
  ///   object is constructed.
  explicit StreamManager(pp::InstanceHandle instance, StreamType type);

  /// Destroys a <code>StreamManager</code> object and closes a stream it
  /// manages.
  ~StreamManager();

  /// Initializes a <code>StreamManager</code> object, associating it with a
  /// <code>segment_sequence</code> and enabling it to deliver elementary
  /// stream packets from that sequence to NaCl Player.
  ///
  /// This method must be called before a given <code>es_data_source</code> is
  /// attached to NaCl Player (i.e. before media stream configuration is
  /// completed and thus before
  /// <code>EsDashPlayerController::OnStreamConfigured()</code> for the managed
  /// stream is called).
  ///
  /// @param[in] segment_sequence A source of elementary stream packets to be
  ///   used in this stream.
  /// @param[in] es_data_source NaCl Player data source which will consume
  ///    elementary stream packets.
  /// @param[in] stream_configured_callback A callback which will be called
  ///   whenever a new stream configuration is discovered and successfully
  ///   applied to NaCl Player.
  /// @param[in] packets_manager A class that will perform synchronization of
  ///   <code>ElementaryStreamPacket</code>s outputted from this stream.
  /// @param[in] drm_type A DRM scheme used by the managed stream. If no DRM
  ///   is in use, use <code>Samsung::NaClPlayer::DRMType_Unknown</code>.
  ///
  /// @return <code>true</code> if an initialization was successfull, or
  ///   <code>false</code> otherwise.
  bool Initialize(
      std::unique_ptr<MediaSegmentSequence> segment_sequence,
      std::shared_ptr<Samsung::NaClPlayer::ESDataSource> es_data_source,
      std::function<void(StreamType)> stream_configured_callback,
      std::function<void(StreamDemuxer::Message, std::unique_ptr<
          ElementaryStreamPacket>)> es_packet_callback,
      StreamListener* stream_listener,
      Samsung::NaClPlayer::DRMType drm_type =
          Samsung::NaClPlayer::DRMType_Unknown);

  /// Changes a <code>MediaSegmentSequence</code> object associated with this
  /// stream. This method resets internal demuxer to parse a given new media
  /// segment, usually causing a change in a stream configuration.
  ///
  /// This method allows to change a representation of this media stream by
  /// providing a <code>MediaSegmentSequence</code> of the new representation.
  ///
  /// @param[in] segment_sequence A new source of elementary stream packets to
  ///   be used in this stream.
  void SetMediaSegmentSequence(
      std::unique_ptr<MediaSegmentSequence> segment_sequence);

  /// Checks if there is enough data buffered for this stream and initiates
  /// data download and parsing if there is not enough buffered elementary
  /// stream packets.
  ///
  /// UpdateBuffer() should be called periodically to keep playback going.
  ///
  /// @param[in] playback_time A current playback time.
  ///
  /// @return Indicates whether there are more segments to download or not.
  bool UpdateBuffer(Samsung::NaClPlayer::TimeTicks playback_time);

  /// Checks if this <code>StreamManager</code> was initialized, i.e.
  /// <code>Initialize()</code> was successfully called on this object before
  /// and thus internal demuxer is properly initialized.
  ///
  /// @return A <code>true</code> value if this <code>StreamManager</code> is
  ///   in a proper and useable state, or a <code>false</code> otherwise.
  bool IsInitialized();

  bool IsSeeking() const;

  /// Prepares this <code>StreamManager</code> for a seek operation. This
  /// stops the manager from downloading media segments from an old playback
  /// position and initiates download of media segments starting at a
  /// <code>new_position</code>. Additionally appending elementary stream
  /// packets to an underlying NaCl Player will be stopped until
  /// <code>OnSeekData()</code> event occurs.
  ///
  /// @param[in] new_position A new playback position.
  void PrepareForSeek(Samsung::NaClPlayer::TimeTicks new_position);

  bool AppendPacket(std::unique_ptr<ElementaryStreamPacket>);

  void SetSegmentToTime(Samsung::NaClPlayer::TimeTicks time,
      Samsung::NaClPlayer::TimeTicks* timestamp,
      Samsung::NaClPlayer::TimeTicks* duration);

  Samsung::NaClPlayer::TimeTicks GetClosestKeyframeTime(
      Samsung::NaClPlayer::TimeTicks);

 private:
  void OnNeedData(int32_t bytes_max) override;
  void OnEnoughData() override;
  void OnSeekData(Samsung::NaClPlayer::TimeTicks new_position) override;

  class Impl;
  std::shared_ptr<Impl> pimpl_;
};

#endif  // NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_STREAM_MANAGER_H_
