/*!
 * player_provider.cc (https://github.com/SamsungDForum/NativePlayer)
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

#include "player/player_provider.h"

#include "player/es_dash_player/es_dash_player_controller.h"
#include "player/url_player/url_player_controller.h"
#include "logger.h"

using Samsung::NaClPlayer::Rect;

std::shared_ptr<PlayerController> PlayerProvider::CreatePlayer(
    PlayerType type, const std::string& url,
    const Samsung::NaClPlayer::Rect view_rect,
    const std::string& subtitle, const std::string& encoding,
    const std::string& drm_license_url,
    const std::unordered_map<std::string, std::string>&
        drm_key_request_properties) {
  switch (type) {
    case kUrl: {
      std::shared_ptr<UrlPlayerController> controller =
          std::make_shared<UrlPlayerController>(instance_, message_sender_);
      controller->SetViewRect(view_rect);
      controller->InitPlayer(url, subtitle, encoding);
      return controller;
    }
    case kEsDash: {
      std::shared_ptr<EsDashPlayerController> controller =
          std::make_shared<EsDashPlayerController>(instance_, message_sender_);
      controller->SetViewRect(view_rect);
      controller->InitPlayer(url, subtitle, encoding,
                             drm_license_url, drm_key_request_properties);
      return controller;
    }
    default:
      Logger::Error("Not known type of player %d", type);
  }

  return 0;
}
