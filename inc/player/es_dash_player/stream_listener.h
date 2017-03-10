/*!
 * stream_listener.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @author Piotr Bałut
 */

#ifndef NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_STREAM_LISTENER_H_
#define NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_STREAM_LISTENER_H_

#include "common.h"
#include "demuxer/stream_demuxer.h"

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
  virtual void OnStreamConfig(const AudioConfig&) = 0;
  virtual void OnStreamConfig(const VideoConfig&) = 0;
  virtual void OnNeedData(StreamType type, int32_t bytes_max) = 0;
  virtual void OnEnoughData(StreamType type) = 0;
  virtual void OnSeekData(StreamType type,
                          Samsung::NaClPlayer::TimeTicks new_position) = 0;
};

#endif  // NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_STREAM_LISTENER_H_
