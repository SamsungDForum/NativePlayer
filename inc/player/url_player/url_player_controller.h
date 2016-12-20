/*!
 * url_player_controller.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_PLAYER_URL_PLAYER_URL_PLAYER_CONTROLLER_H_
#define NATIVE_PLAYER_INC_PLAYER_URL_PLAYER_URL_PLAYER_CONTROLLER_H_

#include <array>
#include <string>
#include <memory>
#include <vector>

#include "nacl_player/media_player.h"
#include "nacl_player/media_data_source.h"
#include "ppapi/cpp/instance.h"
#include "ppapi/utility/threading/simple_thread.h"
#include "ppapi/utility/completion_callback_factory.h"

#include "player/player_controller.h"
#include "player/player_listeners.h"
#include "communicator/message_sender.h"

#include "common.h"

/// @file This file defines the <code>UrlPlayerController</code> class.

/// @class UrlPlayerController
/// @brief This class controls NaCl Player in the playback from the URL
///   scenario.
///
/// This class provides an implementation of the <code>PlayerController</code>
/// interface. It controls a life cycle and manages a <code>MediaPlayer</code>
/// object. <code>MediaPlayer</code> is the main class of NaCl Player.\n
/// This particular <code>PlayerController</code> uses
/// <code>URLDataSource</code> as a media data source. Using
/// <code>UrlPlayerController</code> means
/// that downloading and demuxing are made by <code>NaCl Player</code>.\n
/// The application needs to control only basic player operations:
///   - Play
///   - Pause
///   - Seek
///   - Post subtitles (optional)
///
/// <code>UrlPlayerController</code> supports playback from multimedia files.
/// @note For supported formats please check
/// http://www.samsungdforum.com/Tizen/Spec
/// @note Factory <code>PlayerProvider</code> is responsible for creation of
/// <code>UrlPlayerController</code>.
///
/// @see Samsung::NaClPlayer::URLDataSource
/// @see PlayerProvider

class UrlPlayerController : public PlayerController {
 public:
  /// Constructs a <code>UrlPlayerController</code> object with the given
  /// parameters. NaCl Player must be initialized using the
  /// <code>InitPlayer()</code> method before a
  /// playback can be started.\n
  /// <code>UrlPlayerController</code> is created by a
  /// <code>PlayerProvider</code> factory.
  ///
  /// @param[in] instance An <code>InstanceHandle</code> identifying Native
  ///   Player object.
  /// @param[in] message_sender A <code>MessageSender</code> object pointer
  ///   which will be used to send messages through the communication channel.
  ///
  /// @see Communication::MessageSender
  /// @see UrlPlayerController::InitPlayer()
  explicit UrlPlayerController(const pp::InstanceHandle& instance,
      std::shared_ptr<Communication::MessageSender> message_sender)
      : PlayerController(),
        instance_(instance),
        cc_factory_(this),
        subtitles_visible_(true),
        message_sender_(std::move(message_sender)),
        state_(PlayerState::kUnitialized),
        video_duration_(0.) {}

  /// Destroys <code>UrlPlayerController</code> object. This also
  /// destroys a <code>MediaPlayer</code> object and thus a player pipeline.
  ~UrlPlayerController() {}

  /// This method initializes <code>UrlPlayerController</code> with the given
  /// parameters. In this method <code>NaCl Player</code> is created.
  /// Listeners are registered and if the subtitle path is present external
  /// subtitles are added.\n
  /// This method is called by a <code>PlayerProvider</code> factory.
  ///
  /// @param[in] url A URL address of the file with a media content
  /// @param[in] subtitle A URL address to an external subtitles file. It is
  ///   an optional parameter, if an empty string is provided then external
  ///   subtitles are not available.
  /// @param[in] encoding Encoding of the subtitles file pointed by the
  ///   <code>subtitle</code> parameter. UTF-8 encoding will be used as a
  ///   default if an empty string is given.
  void InitPlayer(const std::string& url, const std::string& subtitle = {},
                  const std::string& encoding = {});

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
  /// This method initializes a media data source with the given URL. After a
  /// successful initialization the media data source is attached to the player.
  /// This method is called after <code>UrlPlayerController::InitPlayer</code>.
  /// @param[in] content_container_url A URL address of a file with a media
  /// content.
  void InitializeUrlPlayer(const std::string& content_container_url);

  void OnSetDisplayRect(int32_t /*result*/);

  void OnSeek(int32_t /*result*/);

  void OnChangeSubtitles(int32_t /*result*/, int32_t id);

  void OnChangeSubVisibility(int32_t /*result*/, bool show);

  void CleanPlayer();

  pp::InstanceHandle instance_;
  std::unique_ptr<pp::SimpleThread> player_thread_;
  pp::CompletionCallbackFactory<UrlPlayerController> cc_factory_;

  PlayerListeners listeners_;

  std::shared_ptr<Samsung::NaClPlayer::MediaDataSource> data_source_;
  std::shared_ptr<Samsung::NaClPlayer::MediaPlayer> player_;
  std::unique_ptr<Samsung::NaClPlayer::TextTrackInfo> text_track_;
  std::vector<Samsung::NaClPlayer::TextTrackInfo> text_track_list_;
  bool subtitles_visible_;

  std::shared_ptr<Communication::MessageSender> message_sender_;

  PlayerState state_;
  Samsung::NaClPlayer::Rect view_rect_;
  Samsung::NaClPlayer::TimeTicks video_duration_;
};

#endif  // NATIVE_PLAYER_INC_PLAYER_URL_PLAYER_URL_PLAYER_CONTROLLER_H_
