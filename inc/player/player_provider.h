/*!
 * player_provider.h (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Michal Murgrabia
 */

#ifndef NATIVE_PLAYER_INC_PLAYER_PLAYER_PROVIDER_H_
#define NATIVE_PLAYER_INC_PLAYER_PLAYER_PROVIDER_H_

#include <unordered_map>
#include <string>

#include "nacl_player/common.h"
#include "ppapi/cpp/instance.h"

#include "common.h"
#include "player/player_controller.h"
#include "communicator/message_sender.h"

/// @file
/// @brief This file defines <code>PlayerProvider</code> class.

/// @class PlayerProvider
/// @brief A factory of <code>PlayerController</code> objects.
///
/// API for controlling the player is specified by
/// <code>PlayerController</code>, but different implementations of it could
/// have a different initialization procedure. This class encapsulates it,
/// provided player controller is already initialized and ready to play the
/// content.

class PlayerProvider {
 public:
  /// @enum PlayerType
  /// This enum defines supported player types.
  enum PlayerType {
    /// This value is not related to any player type.
    kUnknown,

    /// Value which corresponds to a URL player controller. To create this
    /// type of player only a URL to a content is required. A Format of
    /// provided content will be detected automatically.
    /// @see UrlPlayerController
    kUrl,

    /// Value which corresponds to an elementary stream player controller with
    /// DASH. To create this type of player url to a DASH manifest file is
    /// required.
    /// @see EsDashPlayerController
    kEsDash,
  };

  /// Creates a <code>PlayerProvider</code> object. Gathers objects which
  /// are provided to the <code>PlayerController</code> implementation if it
  /// is needed.
  ///
  /// @param[in] instance A handle to main plugin instance.
  /// @param[in] message_sender A class which will be used by the player to
  ///   post messages through the communication channel.
  /// @see pp::Instance
  /// @see Communication::MessageSender
  explicit PlayerProvider(const pp::InstanceHandle& instance,
      std::shared_ptr<Communication::MessageSender> message_sender)
      : instance_(instance), message_sender_((std::move(message_sender))) {}

  /// Destroys a <code>PlayerProvider</code> object. Created
  /// <code>PlayerController</code> objects will not be destroyed.
  ~PlayerProvider() {}

  /// Provides an initialized <code>PlayerController</code> of a given type.
  /// The <code>PlayerController</code> object is ready to use. This is the
  /// main method of this class.
  ///
  /// @param[in] type A type of the player controller which is needed.
  /// @param[in] url A URL address to a file which the player controller
  ///   should use for getting the content. This parameter could point to
  ///   different file types depending on the <code>type</code> parameter.
  ///   Check <code>PlayerType</code> for more information about supported
  ///   formats.
  /// @param[in] view_rect A position and size of the player window.
  /// @param[in] subtitle A URL address to an external subtitles file. It is
  ///   an optional parameter, if is not provided then external subtitles
  ///   are not be available.
  /// @param[in] encoding A code of subtitles formating. It is an optional
  ///   parameter, if is not specified then UTF-8 is used.
  /// @param[in] drm_license_url URL of license server used to acquire content
  ///   decryption license.
  /// @param[in] drm_key_request_properties HTTP/HTTPS request header elements
  ///   used when requesting license from license server
  ///   (see <code>drm_license_url</code>).
  ///
  /// @return A configured and initialized <code>PlayerController<code>.
  std::shared_ptr<PlayerController> CreatePlayer(PlayerType type,
      const std::string& url,
      const Samsung::NaClPlayer::Rect view_rect,
      const std::string& subtitle = {},
      const std::string& encoding = {},
      const std::string& drm_license_url = {},
      const std::unordered_map<std::string, std::string>&
            drm_key_request_properties = {});

 private:
  pp::InstanceHandle instance_;
  std::shared_ptr<Communication::MessageSender> message_sender_;
};

#endif  // NATIVE_PLAYER_INC_PLAYER_PLAYER_PROVIDER_H_
