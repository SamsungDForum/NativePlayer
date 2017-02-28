/*!
 * messages.h (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Tomasz Borkowski
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGES_H_
#define NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGES_H_

#include <string>

/// @namespace Communication
/// In this namespace you can find a class designed for receiving messages from
/// UI, a class designed for sending messages to UI and key values used for a
/// message building. \n
/// It is assumed that UI is implemented in the separate module. In this
/// application JavaScript is used for this purpose. For communication purposes
/// the "post message" - "handle message" mechanism, characteristic for
/// JavaScript and supported in NaCl, is used.
/// All messages are <code>VarDictionary</code> objects and their values
/// are held in a "key" - "value" map. It is mandatory for the message to
/// contain a <code>kKeyAction</code> entry which defines message type.
namespace Communication {

/// @enum MessageToPlayer
/// Values defined in this enum are used to recognize the message type sent to
/// the player. The value of this type should be send in the
/// <code>VarDictionary</code> object with the <code>kKeyMessageToPlayer</code>
/// key. When an action requires additional parameters these values should be
/// also included in the same<code>VarDictionary</code> object.
/// Parameters can be found in the values description, each one's
/// type key identifier and description is provided.
/// @see kKeyMessageToPlayer
enum MessageToPlayer {
  /// A request to close the player; no additional parameters.
  kClosePlayer = 0,

  /// A request to load content specified in additional fields and
  /// prepare the player to play it.
  /// @param (int)kKeyType A specification of what kind of content have
  ///   to be loaded. The only values accepted for  this parameter are
  ///   the ones defined by <code>ClipEnumType</code>
  /// @param (string)kKeyUrl An URL to the content container. This points
  ///   to different file types depending on values of
  ///   <code>kKeyType</code>.
  /// @param (string)kKeySubtitle [optional] A path to the
  ///   file with external subtitles. If an application wants to play
  ///   content with external subtitles this field must be filled.
  /// @param (string)kKeyEncoding [optional] A subtitles encoding code.
  ///   If this parameter is not specified then UTF-8 will be used .
  /// @see Communication::ClipTypeEnum
  kLoadMedia = 1,

  /// A request to start playing; no additional parameters.
  kPlay = 2,

  /// A request to pause playing; no additional parameters.
  kPause = 3,

  /// A request to set a playback current position to a defined one.
  /// @param (double)kKeyTime A new current playback position.
  kSeek = 4,

  /// A request to change stream representation to a defined one.
  /// @param (int)kKeyType An information whether a
  ///   <code>Video = 0</code> or an <code>Audio = 1</code> stream needs
  ///   to be changed.
  /// @param (int)kKeyId An index of a stream representation which should
  ///   be used.
  kChangeRepresentation = 5,

  /// A request to change subtitles representation to a defined one.
  /// @param (int)kKeyId An index of a subtitles representation which
  ///   should be used.
  kChangeSubtitlesRepresentation = 7,

  /// A request to enable or disable subtitle events, depending on the
  /// previous state. Initially subtitles events are enabled. No
  /// additional parameters.
  kChangeSubtitlesVisibility = 8,

  /// An information about the players position and size.
  /// @param (int)XCoordination An x position of the players left upper
  ///   corner.
  /// @param (int)YCoordination A y position of the players left upper
  ///   corner.
  /// @param (int)kKeyWidth A width of the players window.
  /// @param (int)kKeyHeight A height of the players window.
  kChangeViewRect = 9,

  /// Set a log level.
  /// @param (int)New log level. A value from the LogLevel enum.
  /// @see logger.h
  /// @see enum class LogLevel
  kSetLogLevel = 90,
};

/// @enum MessageFromPlayer
/// Values defined in this enum are used to recognize the message type sent
/// from the player. The value of this type should be send in the <code>
/// VarDictionary</code> object with the <code>kKeyMessageFromPlayer</code>
/// key. When an action requires additional parameters these values should be
/// also included in the same<code>VarDictionary</code> object.
/// Parameters can be found in the values description, each one's
/// type key identifier and description is provided.
/// @note Information about all tracks, representations and content duration is
///   send right after content loading is finished. Other messages are send
///   when accurate event occurs, or related operation have been completed.
/// @see kKeyMessageToPlayer
enum MessageFromPlayer {
  /// An information from the player about current playback position.
  /// @param (double)kKeyTime A value which holds current playback
  ///   position in seconds.
  kTimeUpdate = 100,

  /// An information from the player about the duration of the loaded
  /// content.
  /// @param (double)kKeyTime A value which holds the content duration
  ///   in seconds.
  kSetDuration = 101,

  /// An information from the player that buffering have been finished;
  /// no additional parameters.
  kBufferingCompleted = 102,

  /// An information from the player about an Audio representation.
  /// @param (int)kKeyId An index of the Audio representation.
  /// @param (int)kKeyBitrate A bitrate of the Audio representation.
  /// @param (string)kKeyLanguage A language code of the Audio
  ///   representation.
  /// @note When the player finishes loading the content, messages
  ///   about all available Audio representation are sent.
  kAudioRepresentation = 103,

  /// An information from the player about a Video representation.
  /// @param (int)kKeyId An index of the Video representation.
  /// @param (int)kKeyBitrate A bitrate of the Video representation.
  /// @param (int)kKeyHeight A height of the Video representation
  ///   resolution.
  /// @param (int)kKeyWidth A width of the Video representation
  ///   resolution.
  /// @note When the player finishes loading the content, messages
  ///   about all available Video representation are sent.
  kVideoRepresentation = 104,

  /// An information from the player about subtitles representation.
  /// @param (int)kKeyId An index of the subtitles representation.
  /// @param (string)kKeyLanguage A language of the subtitle
  ///   representation.
  /// @note When the player finishes loading the content, messages
  ///   about all available subtitles representation are sent.
  kSubtitlesRepresentation = 105,

  /// An information that a defined representation has been changed.
  /// @param (int)kKeyType A definition whether the message is about
  ///   a <code>Video = 0</code> or an <code>Audio = 1</code> stream.
  /// @param (int)kKeyId An index of the currently used representation.
  kRepresentationChanged = 106,

  /// An information from the player that a subtitle sent in a message
  /// should be shown.
  /// @param (double)kKeyDuration An information (in seconds) for how
  ///   long from the current moment, a sent subtitle should be visible.
  /// @param (string)kKeySubtitle A string which includes a subtitle
  ///   which should be presented.
  kSubtitles = 107,

  /// An information from the player that stream has finished;
  /// no additional parameters.
  kStreamEnded = 108,
};

/// @enum ClipTypeEnum
/// This enum is used to define type of clip which is requested. Basing on
/// this information the player can prepare accurate playback pipeline. \n
/// It is used in a <code>MessageToPlayer::kLoadMedia</code> message.
enum class ClipTypeEnum {
  /// @private this type enum value is not supported.
  kUnknown = 0,

  /// Requested content is a media container.
  kUrl = 1,

  /// Requested content is a dash manifest.
  kDash = 2
};

/// A string value used in messages as a <code>VarDictionary</code> key.
/// <code>kKeyMessageToPlayer</code> has to be used in all messages addressed
/// to the player. The value sent in a field with this key is used to define
/// what kind of message it is. The only values accepted for this field are the
/// ones defined by the <code>enum MessageToPlayer</code>.
/// @see Communication::MessageToPlayer
const std::string kKeyMessageToPlayer = "messageToPlayer";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// <code>kKeyMessageFromPlayer</code> is used in all messages sent by the
/// player. The value in a field with this key is used to define what
/// kind of message it is. The only values the player can use for this field
/// are values defined by <code>enum MessageFromPlayer</code>
/// @see Communication::MessageFromPlayer
const std::string kKeyMessageFromPlayer = "messageFromPlayer";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyBitrate = "bitrate";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>double</code> type value.
const std::string kKeyDuration = "duration";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>string</code> type value.
const std::string kKeyEncoding = "encoding";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyId = "id";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>string</code> type value.
const std::string kKeyLanguage = "language";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>string</code> type value.
const std::string kKeySubtitle = "subtitle";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>double</code> type value.
const std::string kKeyTime = "time";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyType = "type";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to a <code>string</code> type value.
const std::string kKeyUrl = "url";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyWidth = "width";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyHeight = "height";

const std::string kDrmLicenseUrl = "drm_license_url";
const std::string kDrmKeyRequestProperties = "drm_key_request_properties";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code> type value.
const std::string kKeyXCoordination = "x_coordinate";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code>type value.
const std::string kKeyYCoordination = "y_coordinate";

/// A string value used in messages as a <code>VarDictionary</code> key.
/// This key maps to an <code>int</code>type value corresponding to a LogLevel
/// enum value.
const std::string kKeyLogLevel = "level";

}  // namespace Communication

#endif  // NATIVE_PLAYER_INC_COMMUNICATOR_MESSAGES_H_
