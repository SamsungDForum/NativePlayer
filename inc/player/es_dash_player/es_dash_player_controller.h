/*!
 * es_dash_player_controller.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_PLAYER_ES_DASH_PLAYER_ES_DASH_PLAYER_CONTROLLER_H_
#define NATIVE_PLAYER_INC_PLAYER_ES_DASH_PLAYER_ES_DASH_PLAYER_CONTROLLER_H_

#include <array>
#include <memory>
#include <string>
#include <vector>

#include "nacl_player/es_data_source.h"
#include "nacl_player/media_data_source.h"
#include "nacl_player/media_player.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/utility/completion_callback_factory.h"
#include "ppapi/utility/threading/simple_thread.h"

#include "common.h"
#include "dash/dash_manifest.h"
#include "player/es_dash_player/packets_manager.h"
#include "player/es_dash_player/stream_manager.h"
#include "player/player_controller.h"
#include "player/player_listeners.h"
#include "communicator/message_sender.h"

/// @file
/// @brief This file defines the <code>EsDashPlayerController</code> class.

/// @class EsDashPlayerController
/// @brief This class controls NaCl Player in the elementary stream playback
///   scenario with DASH support.
///
/// This class provides an implementation of the <code>PlayerController</code>
/// interface. It controls a life cycle and manages a <code>MediaPlayer</code>
/// object. <code>MediaPlayer</code> is the main class of NaCl Player.
/// <code>EsDashPlayerController</code> allows a playback of a DASH content
/// using elementary streams.
///
/// <code>EsDashPlayerController</code> uses NaCl Player in an elementary
/// stream playback mode, which relies on the <code>ESDataSource</code> class.
/// In this scenario it is an application responsibility to deliver elementary
/// stream packets to NaCl Player. For a DASH content, application is also
/// responsible for parsing a DASH manifest file, as well as managing and
/// demuxing media streams into a series of <code>ElementaryStreamPacket</code>
/// objects, which are delivered to NaCl Player for a playback.
///
/// The application needs to implement a basic player control operations,
/// elementary stream packets provision to NaCl Player and DASH handling.
///
/// <code>EsDashPlayerController</code> supports playback from a DASH manifest
/// file.
///
/// @see class <code>ElementaryStreamPacket</code>
/// @see class <code>PlayerController</code>
/// @see class <code>Samsung::NaClPlayer::ElementaryStream</code>
/// @see class <code>Samsung::NaClPlayer::ESDataSource</code>
/// @see class <code>Samsung::NaClPlayer::MediaPlayer</code>

class EsDashPlayerController : public PlayerController,
    public std::enable_shared_from_this<PlayerController> {
 public:
  /// Creates an <code>EsDashPlayerController</code> object. NaCl Player must
  /// be initialized using the <code>InitPlayer()</code> method before a
  /// playback can be started.
  ///
  /// <code>EsDashPlayerController</code> is created by a
  /// <code>PlayerProvider</code> factory.
  ///
  /// @param[in] instance An <code>InstanceHandle</code> identifying Native
  ///   Player object.
  /// @param[in] message_sender A <code>MessageSender</code> object pointer
  ///   which will be used to send messages through the communication channel.
  ///
  /// @see EsDashPlayerController::InitPlayer()
  EsDashPlayerController(const pp::InstanceHandle& instance,
                   std::shared_ptr<Communication::MessageSender> message_sender)
      : PlayerController(),
        instance_(instance),
        cc_factory_(this),
        subtitles_visible_(true),
        media_duration_(0.),
        message_sender_(message_sender),
        state_(PlayerState::kUnitialized) {}

  /// Destroys an <code>EsDashPlayerController</code> object. This also
  /// destroys a <code>MediaPlayer</code> object and thus a player pipeline.
  ~EsDashPlayerController() override {};

  /// Initializes NaCl Player and prepares it to play a given content.
  /// Subtitles information may also be passed to this function to get ready
  /// for a playback with the subtitles.
  ///
  /// This method can be called several times to change a multimedia content
  /// that should be played with NaCl Player.
  ///
  /// @param[in] url An address of a DASH manifest file to be prepared for a
  ///   playback in NaCl Player.
  /// @param[in] subtitle A URL of the subtitles file to be loaded along the
  ///   multimedia DASH clip pointed by the <code>url</code> parameter.
  ///   Subtitle text updates will be sent to the UI module using
  ///   <code>MessageSender</code> given during construction of this object.
  ///   May be an empty string if there are no external subtitles to be loaded.
  /// @param[in] encoding Encoding of the subtitles file pointed by the
  ///   <code>subtitle</code> parameter. UTF-8 encoding will be uesd as a
  ///   default if an empty string is given.
  ///
  /// @see EsDashPlayerController::EsDashPlayerController()
  /// @see MessageSender::ShowSubtitles()
  void InitPlayer(const std::string& url, const std::string& subtitle = {},
                  const std::string& encoding = {});

  // Overloaded methods defined by PlayerController, don't have to be commented
  void Play() override;
  void Pause() override;
  void Seek(Samsung::NaClPlayer::TimeTicks to_time) override;
  void ChangeRepresentation(StreamType stream_type, int32_t id) override;
  void SetViewRect(const Samsung::NaClPlayer::Rect& view_rect) override;
  void PostTextTrackInfo() override;
  void ChangeSubtitles(int32_t id) override;
  void ChangeSubtitleVisibility() override;
  PlayerState GetState() override;

 private:
  /// @public
  /// Checks every stream if there is enough data buffered. If not, initiates
  /// data download. This method reschedules a call to itself on the side
  /// player thread.
  ///
  /// @param[in] result A PPAPI error code, required in PP_MessageLoop tasks.
  ///   PP_OK is an expected value .
  /// @see StreamManager::UpdateBuffer()
  void UpdateStreamsBuffer(int32_t /*result*/);

  /// @public
  /// An event handler method that should be called both when configuration for
  /// the stream is set for the first time and when configuration is changed
  /// during playback (e.g. a representation is changed).
  ///
  /// @param[in] type Indicates a stream type for which configuration has been
  ///   completed.
  void OnStreamConfigured(StreamType type);

  /// @public
  /// Marks end of configuration of all media streams.
  void FinishStreamConfiguration();

  void OnChangeRepresentation(int32_t /*result*/, StreamType type, int32_t id);

  /// @public
  /// Loads a subtitles file. This will enable subtitle text updates to be sent
  /// to the UI module using the <code>MessageSender</code> class during
  /// playback. This method must be called before the
  /// <code>FinishStreamConfiguration()</code>.
  ///
  /// @param[in] subtitle A URL of the subtitles files.
  /// @param[in] encoding An encoding of the subtitles file.
  void InitializeSubtitles(const std::string& subtitle,
                          const std::string& encoding);

  /// @public
  /// Loads and parses a DASH manifest file. This method loads available
  /// stream representations, begins stream configuration procedure and starts
  /// a side worker thread.
  ///
  /// @param[in] mpd_file_path A URL of the DASH manifest file.
  void InitializeDash(const std::string& mpd_file_path);

  /// @public
  /// Initializes audio and video streams. This method choses initial
  /// representations for each available stream and initializes DRM if it is
  /// present.
  void InitializeStreams(int32_t);

  void InitializeVideoStream(Samsung::NaClPlayer::DRMType /*drm_type*/);

  void InitializeAudioStream(Samsung::NaClPlayer::DRMType /*drm_type*/);

  void OnSetDisplayRect(int32_t /*result*/);

  void OnSeek(int32_t /*result*/);

  void OnChangeSubtitles(int32_t /*result*/, int32_t id);

  void OnChangeSubVisibility(int32_t /*result*/, bool show);

  void CleanPlayer();

  pp::InstanceHandle instance_;
  std::unique_ptr<pp::SimpleThread> player_thread_;
  pp::CompletionCallbackFactory<EsDashPlayerController> cc_factory_;

  PlayerListeners listeners_;

  std::shared_ptr<Samsung::NaClPlayer::ESDataSource> data_source_;
  std::shared_ptr<Samsung::NaClPlayer::MediaPlayer> player_;
  std::unique_ptr<Samsung::NaClPlayer::TextTrackInfo> text_track_;
  std::vector<Samsung::NaClPlayer::TextTrackInfo> text_track_list_;
  bool subtitles_visible_;
  Samsung::NaClPlayer::TimeTicks media_duration_;

  std::shared_ptr<Communication::MessageSender> message_sender_;

  PlayerState state_;
  Samsung::NaClPlayer::Rect view_rect_;

  PacketsManager packets_manager_;
  std::unique_ptr<DashManifest> dash_parser_;
  std::array<std::shared_ptr<StreamManager>,
             static_cast<int32_t>(StreamType::MaxStreamTypes)> streams_;
  std::vector<VideoStream> video_representations_;
  std::vector<AudioStream> audio_representations_;
};

#endif  // NATIVE_PLAYER_INC_PLAYER_ES_DASH_PLAYER_ES_DASH_PLAYER_CONTROLLER_H_
