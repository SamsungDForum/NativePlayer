/*!
 * player_controller.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_PLAYER_PLAYER_CONROLLER_H_
#define NATIVE_PLAYER_INC_PLAYER_PLAYER_CONROLLER_H_

#include <vector>

#include "common.h"
#include "nacl_player/media_common.h"

/// @file
/// @brief This file defines the <code>PlayerController</code> class.

/// @class PlayerController
/// @brief It is a definition of the player controlling interface.

class PlayerController {
 public:
  /// @enum PlayerState
  /// Enum which defines <code>PlayerController</code> possible states.
  enum class PlayerState {
    /// In this state <code>PlayerController</code> is not ready to use. This
    /// state is set right after creation.
    kUnitialized,

    /// In this state <code>PlayerController</code> is ready to use.
    kReady,

    /// In this state <code>PlayerController</code> pause playing the content
    /// and is waiting for resume or close order.
    kPaused,

    /// In this state <code>PlayerController</code> plays the content.
    kPlaying,

    /// This state means that <code>PlayerController</code> has caught an error
    /// and playback is not possible.
    kError
  };

  /// Creates a <code>PlayerController</code> object.
  PlayerController() {}

  /// Destroys the <code>PlayerController</code> object.
  virtual ~PlayerController() {}

  /// Orders the player to start/resume playback of the loaded content.
  virtual void Play() = 0;

  /// Orders the player to pause playback of the content.
  virtual void Pause() = 0;

  /// Orders the player to change the current playback time to the defined one.
  ///
  /// param[in] to_time Time from which playback should continue when seek
  ///   operation completes.
  virtual void Seek(Samsung::NaClPlayer::TimeTicks to_time) = 0;

  /// Orders the player to change a stream representation to a defined one.
  ///
  /// @param[in] stream_type A definition which stream representation should be
  ///   changed.
  /// @param[in] id An id of the new stream representation which should be
  ///   used.
  virtual void ChangeRepresentation(StreamType stream_type, int32_t id) = 0;

  /// Sets a player display area.
  ///
  /// @param[in] view_rect A size and position of a player display area.
  virtual void SetViewRect(const Samsung::NaClPlayer::Rect& view_rect) = 0;

  /// Orders the player to send a message about all available subtitles text
  /// track's information.
  virtual void PostTextTrackInfo() = 0;

  /// Orders the player to change a subtitles set from current to the
  /// specified one.
  ///
  /// @param[in] id An id of a subtitles set which should be used.
  virtual void ChangeSubtitles(int32_t id) = 0;

  /// Orders the player to start or stop generating events related to subtitles.
  virtual void ChangeSubtitleVisibility() = 0;

  /// Provides information about <code>PlayerController</code> state.
  /// @return A current state of the player.
  virtual PlayerState GetState() = 0;
};

#endif  // NATIVE_PLAYER_INC_PLAYER_PLAYER_CONROLLER_H_
