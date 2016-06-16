/*!
 * player_listeners.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_PLAYER_PLAYER_LISTENERS_H_
#define NATIVE_PLAYER_INC_PLAYER_PLAYER_LISTENERS_H_

#include <memory>

#include "nacl_player/buffering_listener.h"
#include "nacl_player/media_events_listener.h"
#include "nacl_player/subtitle_listener.h"
#include "ppapi/cpp/var.h"

#include "player/player_controller.h"
#include "communicator/message_sender.h"
#include "common.h"

/// @fie
/// @brief This file defines <code>SubtitleListener</code>,
/// <code>MediaPlayerListener</code>, <code>MediaBufferingListener</code>
/// classes and <code>PlayerListeners</code> structure.

/// @class SubtitleListener
/// @brief Listener class designed to handle subtitle related events.
/// <code>SubtitleListener</code> receives event's information and passes it
/// through the communication channel using <code>MessageSender</code>.
class SubtitleListener : public Samsung::NaClPlayer::SubtitleListener {
 public:
  /// Creates a <code>SubtitleListener</code> object.
  ///
  /// @param[in] message_sender An object which will be used to send messages
  ///   based on received subtitle events through the communication channel
  explicit SubtitleListener(
      std::weak_ptr<Communication::MessageSender> message_sender)
      : message_sender_(std::move(message_sender)) {}

  /// An event handler method, called by the internal platform subtitle parser
  /// when a new subtitle should be presented. <code>SubtitleListener</code>
  /// passes delivered data through the communication channel using
  /// <code>MessageSender</code>.
  ///
  /// @param[in] time Duration for which the text should be displayed.
  /// @param[in] text The UTF-8 encoded string that contains a subtitle text
  ///   that should be displayed. Please note text encoding will be UTF-8
  ///   independently from the source subtitle file encoding.
  void OnShowSubtitle(Samsung::NaClPlayer::TimeTicks time, const char* text)
    override;

 private:
  std::weak_ptr<Communication::MessageSender> message_sender_;
};

/// @class MediaPlayerListener
/// @brief Listener class designed to handle player and playback
/// related events from the platform. <code>MediaPlayerListener</code>
/// receives event's information and passes it through the communication
/// channel using <code>MessageSender</code>.
class MediaPlayerListener : public Samsung::NaClPlayer::MediaEventsListener {
 public:
  /// Creates a <code>MediaPlayerListener</code> object.
  ///
  /// @param[in] message_sender An object which will be used to send messages
  ///   based on received subtitle events through the communication channel
  explicit MediaPlayerListener(
      std::weak_ptr<Communication::MessageSender> message_sender)
      : message_sender_(std::move(message_sender)) {}

  /// An event handler method, called periodically during clip playback and
  /// indicates a playback progress. <code>MediaPlayerListener</code> passes
  /// information about about playback progress through the communication
  /// channel using <code>MessageSender</code>.
  ///
  /// @param[in] time Current playback time.
  void OnTimeUpdate(Samsung::NaClPlayer::TimeTicks time) override;

  /// An event handler method, called when the player reaches the end of the
  /// stream. <code>MediaPlayerListener</code> passes information about EOS
  /// event through the communication channel using <code>MessageSender</code>.
  void OnEnded() override;

  /// An event handler method, called when any player-related error occurs.
  /// <code>MediaPlayerListener</code> passes information about
  /// it through the communication channel using <code>MessageSender</code>.
  ///
  /// @param[in] error Caught error code.
  void OnError(Samsung::NaClPlayer::MediaPlayerError error) override;

 private:
  std::weak_ptr<Communication::MessageSender> message_sender_;
};

/// @class MediaBufferingListener
/// @brief Listener class designed to handle buffering related
/// events. <code>MediaBufferingListener</code>
/// receives event's information and passes it through the communication
/// channel using <code>MessageSender</code>.
class MediaBufferingListener : public Samsung::NaClPlayer::BufferingListener {
 public:
  /// Creates a <code>MediaBufferingListener</code> object.
  ///
  /// @param[in] message_sender An object which will be used to send messages
  ///   based on received subtitle events through the communication channel
  /// @param[in] player_controller A handler to a controller, which will be
  ///   requested to send all text track information through the communication
  ///   channel when buffering finished.
  explicit MediaBufferingListener(
      std::weak_ptr<Communication::MessageSender> message_sender,
      std::weak_ptr<PlayerController> player_controller = {})
      : message_sender_(std::move(message_sender)),
        player_controller_(std::move(player_controller)) {}

  /// An event handler method, called when buffering has been started by the
  /// player. <code>MediaBufferingListener</code> passes this information
  /// through the communication channel using <code>MessageSender</code>.
  void OnBufferingStart() override;

  /// An event handler method, called periodically with information about
  /// buffering progress. <code>MediaBufferingListener</code> passes it
  /// through the communication channel using <code>MessageSender</code>.
  ///
  /// @param[in] percent Indicates how much of the initial data has been
  ///   buffered by the player.
  /// @note This event is generated only in URL data source playback scenario.
  void OnBufferingProgress(uint32_t percent) override;

  /// An event handler method, called when a buffering has been completed by the
  /// player. <code>MediaBufferingListener</code> passes this information
  /// through the communication channel using <code>MessageSender</code>.
  /// @note If <code>PlayerController</code> object was registered to this
  /// object, then in this method will use it to send information about
  /// all text track information.
  /// @note Elementary stream player type should send elementary stream packets
  ///   until this event occurs.
  /// @note Generating this event means that playback can be started.
  void OnBufferingComplete() override;

 private:
  std::weak_ptr<Communication::MessageSender> message_sender_;
  std::weak_ptr<PlayerController> player_controller_;
};

/// @struct PlayerListeners
/// @brief It aggregates implementation of all basic listeners required by
/// NaClPlayer.
struct PlayerListeners {
  std::shared_ptr<MediaBufferingListener> buffering_listener;
  std::shared_ptr<MediaPlayerListener> player_listener;
  std::shared_ptr<SubtitleListener> subtitle_listener;
};

#endif  // NATIVE_PLAYER_INC_PLAYER_PLAYER_LISTENERS_H_
