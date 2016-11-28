/*!
 * stream_listener.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Piotr Bałut
 */

#ifndef NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_STREAM_LISTENER_H_
#define NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_STREAM_LISTENER_H_

#include "common.h"

/// @file
/// @brief This file defines the <code>StreamListener</code> interface.

/// @class StreamListener
/// This interface mimics <code>Samsung::NaClPlayer::ElementaryStreamListener
/// </code> interface, adding stream type to methods for usage in a class that
/// aggregates multiple streams.
///
/// @see class <code>PacketsManager</code>
/// @see class <code>Samsung::NaClPlayer::ElementaryStreamListener</code>

class StreamListener {
 public:
  virtual ~StreamListener();
  virtual void OnNeedData(StreamType type, int32_t bytes_max) = 0;
  virtual void OnEnoughData(StreamType type) = 0;
  virtual void OnSeekData(StreamType type,
                          Samsung::NaClPlayer::TimeTicks new_position) = 0;
};

#endif  // NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_STREAM_LISTENER_H_
