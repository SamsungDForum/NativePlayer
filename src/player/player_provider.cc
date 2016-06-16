/*!
 * player_provider.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
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
                    const std::string& subtitle, const std::string& encoding) {
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
      controller->InitPlayer(url, subtitle, encoding);
      return controller;
    }
    default:
      Logger::Error("Not known type of player %d", type);
  }

  return 0;
}
