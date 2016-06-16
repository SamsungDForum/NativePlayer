/*!
 * message_receiver.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGE_RECEIVER_H_
#define NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGE_RECEIVER_H_

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/instance_handle.h"
#include "ppapi/cpp/message_handler.h"

#include "player/player_controller.h"
#include "player/player_provider.h"

/// @file
/// @brief This file defines <code>MessageReceiver</code> class.

namespace Communication {

/// @class MessageReceiver
/// @brief Main class responsible for handling messages from the
/// communication channel and controlling player life cycle.
///
/// <code>MessageReceiver</code> is designed to receive messages from
/// the communication channel and perform proper actions. It accepts
/// only <code>VarDictionary</code> messages, which must contain a key named
/// <code>kKeyMessageToPlayer</code> and have a value defined by the
/// <code>enum MessageToPlayer</code>. Basing on this value
/// <code>MessageReceiver</code> performs a proper action on the player.
///
/// @see Communication Description of the <code>Communication</code> namespace
///   provides a brief of the communication mechanism.
/// @see kKeyMessageToPlayer
/// @see MessageToPlayer

class MessageReceiver : public pp::MessageHandler {
 public:
  /// Creates a <code>MessageReceiver</code> object instance.
  ///
  /// @param[in] player_provider A factory object which is used to get player
  ///   controller which fits the needs.
  /// @see PlayerProvider
  explicit MessageReceiver(std::shared_ptr<PlayerProvider> player_provider)
      : player_provider_(std::move(player_provider)) {}

  /// Destroys the <code>MessageReceiver</code> object and frees all allocated
  /// resources.
  ~MessageReceiver() {}

  /// Handles asynchronous messages from the communication
  /// channel. This method performs an action selected basing on a value mapped
  /// by the <code>kKeyMessageToPlayer</code> key contained within message_data
  /// dictionary.
  ///
  /// @param[in] instance A handle to a plugin instance to which a message was
  ///   addressed. This value is provided automatically by the NaCl engine and
  ///   is not used in this application.
  /// @param[in] message_data A container with a message, it is expected that
  ///   the message is <code>VarDictionary</code>.
  /// @see kKeyMessageToPlayer
  /// @see MessageToPlayer
  void HandleMessage(pp::InstanceHandle instance,
                     const pp::Var& message_data) override;

  /// Handles synchronous messages. Such way of communication
  /// is not used in this application. Method sends a received message as a
  /// replay message.
  ///
  /// @param[in] instance A handle to a plugin instance to which the message
  ///   was addressed. The Value is provided automatically by NaCl engine.
  /// @param[in] message_data A container with message.
  /// @return Var object which is send back as a replay message.
  pp::Var HandleBlockingMessage(pp::InstanceHandle instance,
                                const pp::Var& message_data) override;

  /// Informs <code>MessageReceiver</code> object that it was unregistered from
  /// receiving messages. After this, no more calls will be made to this object.
  ///
  /// @param[in] instance A handle to a plugin instance from which receiving
  ///   list object has been removed.
  void WasUnregistered(pp::InstanceHandle) override {}

 private:
  /// @public
  /// Validates a <code>kChangeRepresentation</code> message, decodes provided
  /// parameters and requests the player to change the defined representation
  /// from the current one to the specified one. The request will be ignored if
  /// the content is not loaded.
  ///
  /// @param[in] type A type of the stream which is requested to be changed,
  ///   <code>Video = 0</code> or <code>Audio = 1</code>. This
  ///   <code>Var</code> is cast to a <code>StreamType</code> and it has to be
  ///   an integer value.
  /// @param[in] id An id of a representation which should be used. This
  ///   <code>Var</code> has to be an integer value.
  /// @see kChangeRepresentation.
  /// @see StreamType
  void ChangeRepresentation(const pp::Var& type, const pp::Var& id);

  /// @public
  /// Handles a <code>kClosePlayer</code> message, closes the player
  /// and frees resources.
  ///
  /// @see kClosePlayer
  void ClosePlayer();

  /// @public
  /// Validates a <code>kLoadMedia</code> message, decodes provided
  /// parameters and creates a player that can handle a given type of the
  /// multimedia content.
  ///
  /// @param[in] type A type of a content which needs to be loaded. This
  ///   <code>Var</code> is casted to <code>ClipTypeEnum</code> and it has to
  ///   be an integer value.
  /// @param[in] url An URL to a content container, it could point to
  ///   different types of files depending on the player type. This
  ///   <code>Var</code> has to be a <code>string</code> type value.
  /// @param[in] subtitle An URL to the file with external subtitles. It is an
  ///   optional parameter, but if provided it has to be a <code>string</code>
  ///   type value.
  /// @param[in] encoding A subtitle encoding code. It is an optional
  ///   parameter, which has to be a <code>string</code> type value.
  /// @see kLoadMedia
  /// @see ClipTypeEnum
  void LoadMedia(const pp::Var& type, const pp::Var& url,
                   const pp::Var& subtitle, const pp::Var& encoding);

  /// @public
  /// Handles a <code>kPause</code> message, and requests the player
  /// to pause. The request will be ignored if the content is not loaded.
  ///
  /// @see kPause
  void Pause();

  /// @public
  /// Handles a <code>kPlay</code> message, and requests the player to
  /// start play. The request will be ignored if the content is not loaded.
  ///
  /// @see kPlay
  void Play();

  /// @public
  /// Handles a <code>kSeek</code> message, validates a provided
  /// parameter and requests the player to change current playback time to the
  /// defined one. The request will be ignored if the content is not loaded.
  ///
  /// @param[in] time Time from which playback should continue when seek
  ///   operation completes. This value is cast to <code>TimeTick</code> and
  ///   it has to be a floating point value.
  /// @see kSeek
  /// @see TimeTick
  void Seek(const pp::Var& time);

  /// @public
  /// Handles a <code>kChangeViewRect</code> message, validates
  /// provided parameters and informs the player about new position and
  /// resolution of plugin window. If the player is not initialized then
  /// parameters will be provided during initialization.
  ///
  /// @param[in] x_position An x position of the plugin left upper corner;
  ///   should be expressed by an integer value.
  /// @param[in] y_position A y position of the plugin left upper corner;
  ///   should be expressed by an integer value.
  /// @param[in] width A width of the plugin window; should be expressed
  ///   by an integer value.
  /// @param[in] height A height of the plugin window; should be expressed
  ///   by an integer value.
  /// @see kChangeViewRect
  void ChangeViewRect(const pp::Var& x_position, const pp::Var& y_position,
                        const pp::Var& width, const pp::Var& height);

  /// @public
  /// Handles a <code>kChangeSubtitlesRepresentation</code> message,
  /// validates a provided parameter and requests the player to change
  /// subtitle strack to a defined one. The request will be ignored if the
  /// content is not loaded.
  ///
  /// @param[in] id An ID of subtitles which need to be used. This
  ///   <code>Var</code> has to be an <code>int</code> type.
  /// @see kChangeSubtitlesRepresentation
  void ChangeSubtitlesRepresentation(const pp::Var& id);

  /// @public
  /// Method which handles a <code>kChangeSubtitlesVisibility</code> message
  /// and disables or enables generating subtitle events, depending on previous
  /// state. The request will be ignored if the content is not loaded.
  ///
  /// @see kChangeSubtitlesVisibility
  void ChangeSubtitlesVisibility();

  std::shared_ptr<PlayerController> player_controller_;
  std::shared_ptr<PlayerProvider> player_provider_;
  Samsung::NaClPlayer::Rect view_rect_;
};

}  // namespace Communication

#endif  // NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGE_RECEIVER_H_
