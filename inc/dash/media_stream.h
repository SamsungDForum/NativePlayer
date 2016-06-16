/*!
 * media_stream.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DASH_MEDIA_STREAM_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DASH_MEDIA_STREAM_H_

#include <string>
#include <memory>

#include "dash/content_protection_visitor.h"

/// @file
/// @brief This file defines the <code>MediaStreamType</code> enum and
/// <code>CommonStreamDescription</code>, <code>AudioStream</code>,
/// <code>VideoStream</code> structs.

/// @enum MediaStreamType
/// @brief Describes available stream types supported by the DASH parser.
enum class MediaStreamType : int32_t {
  Unknown = -1,
  Video = 0,
  Audio = 1,
  MaxTypes
};

/// @struct CommonStreamDescription
/// @brief Describes a media stream common part for the audio and video streams
///   used in their description.
/// @note Fields which aren't specified in the DASH manifest should be 0 or
///   empty.
/// @see AudioStream
/// @see VideoStream
struct CommonStreamDescription {
  /// Describes id of a media stream. It is counted from 0 and is increased
  /// when next audio or video stream is added.
  /// @note AudioStream and VideoStream objects are identified and
  /// counted independently.
  uint32_t id;

  /// Describes bitrate of the media stream. Can be useful while choosing stream
  /// to play basing on network connection.
  uint32_t bitrate;

  /// Describes content protection information. Such as DRM type and server
  /// address.
  std::shared_ptr<ContentProtectionDescriptor> content_protection;

  /// Constructs a <code>CommonStreamDescription</code> with 0 values.
  CommonStreamDescription() : id(0), bitrate(0) {}

  /// Constructs a copy of <code>other</code>.
  CommonStreamDescription(const CommonStreamDescription& other) = default;

  /// Move-constructs a <code>CommonStreamDescription</code> object, making it
  /// point at the same object that <code>other</code> was pointing to.
  CommonStreamDescription(CommonStreamDescription&& other) = default;

  /// Assigns <code>other</code> to this <code>CommonStreamDescription</code>
  CommonStreamDescription& operator=(const CommonStreamDescription& other) =
      default;

  /// Move-assigns <code>other</code> to this
  /// <code>CommonStreamDescription</code> object.
  CommonStreamDescription& operator=(CommonStreamDescription&& other) = default;

  /// Destroys <code>CommonStreamDescription</code> object.
  ~CommonStreamDescription() = default;
};

/// @struct AudioStream
/// @brief Describes the audio stream.
///
/// Consists of CommonStreamDescription and audio detailed description.\n
/// This struct is used to build audio elementary stream basing on information
/// parsed from the DASH manifest.
/// @note Fields which aren't specified in the DASH manifest should be 0 or
///   empty.
struct AudioStream {
  CommonStreamDescription description;
  std::string language;
};

/// @struct VideoStream
/// @brief Describes the video stream.
///
/// Consists of CommonStreamDescription and video detailed description.\n
/// This struct is used to build video elementary stream basing on information
/// parsed from the DASH manifest.
/// @note Fields which aren't specified in the DASH manifest should be 0 or
///   empty.
struct VideoStream {
  CommonStreamDescription description;
  uint32_t width;
  uint32_t height;

  /// Constructs a <code>VideoStream</code> with 0 values.
  VideoStream() : width(0), height(0) {}

  /// Constructs a copy of <code>other</code>.
  VideoStream(const VideoStream& other) = default;

  /// Move-constructs a <code>VideoStream</code> object, making it
  /// point at the same object that <code>other</code> was pointing to.
  VideoStream(VideoStream&& other) = default;

  /// Assigns <code>other</code> to this <code>VideoStream</code>
  VideoStream& operator=(const VideoStream& other) = default;

  /// Move-assigns <code>other</code> to this <code>VideoStream</code> object.
  VideoStream& operator=(VideoStream&& other) = default;

  /// Destroys <code>VideoStream</code> object.
  ~VideoStream() = default;
};

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DASH_MEDIA_STREAM_H_
