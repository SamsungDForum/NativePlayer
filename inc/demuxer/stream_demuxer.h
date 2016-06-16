/*!
 * stream_demuxer.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Jacob Tarasiewicz
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_STREAM_DEMUXER_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DEMUXER_STREAM_DEMUXER_H_

#include <functional>
#include <string>
#include <vector>

#include "ppapi/c/pp_stdint.h"
#include "ppapi/cpp/instance.h"
#include "nacl_player/common.h"
#include "nacl_player/media_common.h"
#include "nacl_player/media_codecs.h"

#include "demuxer/elementary_stream_packet.h"
/// @file
/// @brief This file defines the <code>StreamDemuxer</code>,
/// <code>AudioConfig</code> and <code>VideoConfig</code>.

/// @struct AudioConfig
/// @brief Structure describing Elementary Audio Stream configuration.
/// @note Not all fields are required to properly configure audio elementary
///   stream. Fields not needed can be ignored.
struct AudioConfig {
  /// Describes audio codec type like AAC, MP3, etc.
  /// @see enum <code>Samsung::NaClPlayer::AudioCodec_Type</code>
  Samsung::NaClPlayer::AudioCodec_Type codec_type;

  /// Describes audio codec profile like AAC Main, AAC Low, etc.
  /// @see enum <code>Samsung::NaClPlayer::AudioCodec_Profile</code>
  /// @note Only some codecs require codec profile like AAC, DTS, etc.
  Samsung::NaClPlayer::AudioCodec_Profile codec_profile;

  /// Describes audio sample format like signed 16-bit, Float 32-bit, etc.
  /// @see enum <code>Samsung::NaClPlayer::SampleFormat</code>
  Samsung::NaClPlayer::SampleFormat sample_format;

  /// Describes audio channel layout like mono, stereo, etc.
  /// @see enum <code>Samsung::NaClPlayer::ChannelLayout</code>
  Samsung::NaClPlayer::ChannelLayout channel_layout;

  /// Describes audio bits per channel like 16, 32 etc.
  /// @note Please note that used unit is [bit] not [byte].
  /// E.g. 16 [bits] = 2 [bytes]
  int32_t bits_per_channel;

  /// Describes audio samples per second in [Hz] like 44100, 22050, etc.
  int32_t samples_per_second;

  /// Describes audio extra data. Some codecs need more data to encode
  /// stream. Usually it's in header of the file.
  std::vector<uint8_t> extra_data;

  /// Checks if compared object <code>config</code> is equal to this
  /// <code>AudioConfig</code>.
  /// @return True if objects are equal.\n False otherwise.
  bool operator==(const AudioConfig& config) const {
    return ((bits_per_channel == config.bits_per_channel) &&
            (channel_layout == config.channel_layout) &&
            (codec_profile == config.codec_profile) &&
            (codec_type == config.codec_type) &&
            (extra_data == config.extra_data) &&
            (sample_format == config.sample_format) &&
            (samples_per_second == config.samples_per_second));
  }
};

/// @struct VideoConfig
/// @brief Structure describing Elementary Video Stream configuration.
/// @note Not all fields are required to properly configure audio elementary
///   stream. Fields not needed can be ignored.
struct VideoConfig {
  /// Describes video codec type like H264, MPEG2, etc.
  /// @see enum <code>Samsung::NaClPlayer::VideoCodec_Type</code>
  Samsung::NaClPlayer::VideoCodec_Type codec_type;

  /// Describes video codec profile like H264 Main, VP9 Max, etc.
  /// @see enum <code>Samsung::NaClPlayer::VideoCodec_Profile</code>
  /// @note Only some codecs require codec profile like H264, MPEG4, etc.
  Samsung::NaClPlayer::VideoCodec_Profile codec_profile;

  /// Describes video frame format like 32bpp RGB, 20bpp YUVA, etc.
  /// @see enum <code>Samsung::NaClPlayer::VideoFrame_Format</code>
  Samsung::NaClPlayer::VideoFrame_Format frame_format;

  /// Describes video frame size. It's expressed by width[px] and
  /// height [px] like 1920 1080, 640 480, etc.
  /// @see class <code>Samsung::NaClPlayer::Size</code>
  Samsung::NaClPlayer::Size size;

  /// Describes video frame rate. It's represented as rational number
  /// like 45000 1877 gives around 24 frames per second, etc.
  /// @see class <code>Samsung::NaClPlayer::Rational</code>
  Samsung::NaClPlayer::Rational frame_rate;

  /// Describes video extra data. Some codecs need more data to encode
  /// stream. Usually it's in header of the file.
  std::vector<uint8_t> extra_data;

  /// Checks if compared object <code>config</code> is equal to this
  /// <code>VideoConfig</code>.
  /// @return True if objects are equal.\n False otherwise.
  bool operator==(const VideoConfig& config) const {
    return ((codec_profile == config.codec_profile) &&
            (codec_type == config.codec_type) &&
            (extra_data == config.extra_data) &&
            (frame_format == config.frame_format));
  }
};

/// @class StreamDemuxer
/// @brief An interface for demuxing modules.
/// This interface provides methods used to parse data of media container and
/// perform basic operation on <code>StreamDemuxer</code>.
/// This interface allows to set listeners for data needed by
/// <code>NaCl Player</code> - stream configurations and DRM data.
/// @note This interface is needed to be implemented by demuxer module to be
/// consistent with other <code>Native Player</code> modules.
class StreamDemuxer {
 public:
  /// @enum Type
  /// Describes type of demuxer. Each demuxer can be used to parse one
  /// type of container: audio or video.
  enum Type { kUnknown = -1, kAudio = 0, kVideo = 1 };

  /// @enum Message
  /// Describes message types that <code>StreamDemuxer</code> can post
  /// via callback function passed by StreamDemuxer::Init method. This Message
  /// can be useful when running demuxer methods asynchronously to signal
  /// certain events or completion of actions.
  enum Message {
    kError = -1,
    kInitialized = 0,
    kFlushed = 1,
    kClosed = 2,
    kEndOfStream = 3,
    kAudioPkt = 4,
    kVideoPkt = 5,
  };

  /// Creates <code>StreamDemuxer</code> for given StreamDemuxer::Type.
  /// @param[in] instance An <code>InstanceHandle</code> identifying
  /// Native Player object.
  /// @param[in] type A <code>StreamDemuxer::Type</code> identifying a type of
  /// requested to create <code>StreamDemuxer</code> to create.
  ///
  /// @return StreamDemuxer constructed with given params.
  static std::unique_ptr<StreamDemuxer> Create(
      const pp::InstanceHandle& instance, Type type);

  /// Constructs an empty <code>StreamDemuxer</code>.
  StreamDemuxer() {}

  /// Destroys <code>StreamDemuxer</code> object.
  virtual ~StreamDemuxer() {}

  /// Initialize StreamDemuxer, if finished successfully then data from the
  /// requested stream can be parsed.
  ///
  /// @param[in] callback A function which is registered.
  /// This function is used to communicate with use of StreamDemuxer::Message.
  /// It should be called on callback_dispatcher MessageLoop\n
  /// In particular is used to pass ElementaryStreamPacket.
  /// @param[in] callback_dispatcher A <code>pp::MessageLoop</code> on which
  /// communication should be performed.
  ///
  /// @return True on success, false otherwise.
  virtual bool Init(
      const std::function<
          void(Message, std::unique_ptr<ElementaryStreamPacket>)>& callback,
      pp::MessageLoop callback_dispatcher) = 0;

  /// Performs flush operation on StreamDemuxer. All demuxed elementary stream
  /// packets have to be erased, in order to prepare <code>StreamDemuxer</code>
  /// to receive data from container.
  virtual void Flush() = 0;

  /// Performs parse operation. Passed data should be parsed or added to
  /// queue data to be parsed. Parsed data - <code>ElementaryStreamPacket</code>
  /// should be passed to the callback function registered in
  /// StreamDemuxer::Init. To recognize type of demuxed packet
  /// StreamDemuxer::Message::kAudioPkt or StreamDemuxer::Message::kVideoPkt
  /// can be used.
  ///
  /// @param[in] data An byte array which helds data of media container.
  virtual void Parse(const std::vector<uint8_t>& data) = 0;

  /// Registers a callback function which is called every time audio
  /// configuration has changed and pass new configuration.
  ///
  /// @param[in] callback A function which is registered in StreamDemuxer.
  /// It should be called each time <code>AudioConfig</code> had changed or
  /// on the first time configuration is available.\n Callback should be called
  /// on callback_dispatcher MessageLoop registered in StreamDemuxer::Init.
  /// @return True on success, false otherwise.
  virtual bool SetAudioConfigListener(
      const std::function<void(const AudioConfig&)>& callback) = 0;

  /// Registers a callback function which is called every time video
  /// configuration has changed and pass new configuration.
  ///
  /// @param[in] callback A function which is registered in
  /// <code>StreamDemuxer</code>.
  /// It should be called each time <code>VideoConfig</code> had changed or
  /// on the first time configuration is available.\n Callback should be called
  /// on callback_dispatcher MessageLoop registered in StreamDemuxer::Init.
  /// @return True on success, false otherwise.
  virtual bool SetVideoConfigListener(
      const std::function<void(const VideoConfig&)>& callback) = 0;

  /// Registers a callback function which is called every time DRM init data is
  /// discovered by StreamDemuxer and pass DRM init data.
  ///
  /// @param[in] callback A function which is registered in StreamDemuxer.
  /// It should be called DRM data is available.\n Callback should be called
  /// on callback_dispatcher MessageLoop registered in StreamDemuxer::Init.
  /// @return True on success, false otherwise.
  virtual bool SetDRMInitDataListener(const std::function<
      void(const std::string& type, const std::vector<uint8_t>& init_data)>&
                                          callback) = 0;

  /// Sets time stamp to demuxed packets. Can be use after performing
  /// seek operation on elementary stream to set proper timestamps of packets.
  virtual void SetTimestamp(Samsung::NaClPlayer::TimeTicks) = 0;

  /// Closes StreamDemuxer. Clear all data, stream configurations.
  /// StreamDemuxer::Init should be called, before using it again.
  virtual void Close() = 0;
};

#endif  // NATIVE_PLAYER_SRC_DEMUXER_STREAM_DEMUXER_H_
