/*!
 * url_player_controller.cc (https://github.com/SamsungDForum/NativePlayer)
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

#include "player/url_player/url_player_controller.h"

#include <functional>
#include <limits>
#include <utility>

#include "nacl_player/error_codes.h"
#include "nacl_player/url_data_source.h"
#include "ppapi/cpp/var_dictionary.h"

using Samsung::NaClPlayer::ErrorCodes;
using Samsung::NaClPlayer::URLDataSource;
using Samsung::NaClPlayer::MediaDataSource;
using Samsung::NaClPlayer::MediaPlayer;
using Samsung::NaClPlayer::MediaPlayerState;
using Samsung::NaClPlayer::Rect;
using Samsung::NaClPlayer::TextTrackInfo;
using Samsung::NaClPlayer::TimeTicks;
using std::make_shared;
using std::placeholders::_1;

void UrlPlayerController::InitPlayer(const std::string& url,
                                     const std::string& subtitle,
                                     const std::string& encoding) {
  LOG_INFO("Loading media from : [%s]", url.c_str());
  CleanPlayer();

  player_thread_ = MakeUnique<pp::SimpleThread>(instance_);
  player_thread_->Start();

  player_ = make_shared<MediaPlayer>();

  // initialize a listeners and register them in player
  listeners_.player_listener =
      make_shared<MediaPlayerListener>(message_sender_);
  listeners_.buffering_listener =
      make_shared<MediaBufferingListener>(message_sender_);
  listeners_.subtitle_listener =
      make_shared<SubtitleListener>(message_sender_);

  player_->SetMediaEventsListener(listeners_.player_listener);
  player_->SetBufferingListener(listeners_.buffering_listener);
  player_->SetSubtitleListener(listeners_.subtitle_listener);

  // register subtitles source if defined
  if (!subtitle.empty()) {
    text_track_ = MakeUnique<TextTrackInfo>();
    int32_t ret = player_->AddExternalSubtitles(subtitle,
                                                encoding,
                                                *text_track_);
    if (ret != ErrorCodes::Success) {
      LOG_ERROR("Failed to initialize subtitles, code: "
          "%d path: %s, encoding: %s", ret, subtitle.c_str(), encoding.c_str());
    }
  }

  int32_t ret = player_->SetDisplayRect(view_rect_);

  if (ret != ErrorCodes::Success) {
    LOG_ERROR("Failed to set display rect [(%d - %d) (%d - %d)], code: %d",
       view_rect_.x(), view_rect_.y(), view_rect_.width(), view_rect_.height(),
       ret);
  }

  InitializeUrlPlayer(url);
}

void UrlPlayerController::InitializeUrlPlayer(
    const std::string& content_container_url) {
  LOG_INFO("Play content directly from URL = %s ", content_container_url.c_str());
  data_source_ = make_shared<URLDataSource>(content_container_url);
  player_->AttachDataSource(*data_source_);
  TimeTicks duration;
  if (player_->GetDuration(duration) == ErrorCodes::Success) {
    message_sender_->SetMediaDuration(duration);
    video_duration_ = duration;
    LOG_INFO("Got duration: %f [s].", duration);
  } else {
    LOG_INFO("Failed to retreive duration!");
  }
  PostTextTrackInfo();
}

void UrlPlayerController::Play() {
  if (!player_) {
    LOG_INFO("Play. player is not initialized, cannot play");
    return;
  }

  int32_t ret = player_->Play();
  if (ret == ErrorCodes::Success) {
    LOG_INFO("Play called successfully");
  } else {
    LOG_ERROR("Play call failed, code: %d", ret);
  }
}

void UrlPlayerController::Pause() {
  if (!player_) {
    LOG_INFO("Pause. player is not initialized");
    return;
  }

  int32_t ret = player_->Pause();
  if (ret == ErrorCodes::Success) {
    LOG_INFO("Pause called successfully");
  } else {
    LOG_ERROR("Pause call failed, code: %d", ret);
  }
}

void UrlPlayerController::Seek(TimeTicks to_time) {
  LOG_INFO("Seek to %f", to_time);
  // video_duration_ equal to 0 means we failed to retrieve the video duration,
  // we still should try to seek, but it is possiable that seek will fail.
  if (video_duration_ && to_time > video_duration_ - kEps) {
    to_time = video_duration_ - kEps;
  } else if (to_time < kEps) {
    to_time = 0 + kEps;
  }
  int32_t ret =
      player_->Seek(to_time, std::bind(&UrlPlayerController::OnSeek, this, _1));
  if (ret < ErrorCodes::CompletionPending) {
    LOG_ERROR("Seek call failed, code: %d", ret);
  }
}

void UrlPlayerController::ChangeRepresentation(StreamType /*stream_type*/,
                                               int32_t /*id*/) {
  LOG_INFO("URLplayer doesnt support changing representation");
}

void UrlPlayerController::SetViewRect(const Rect& view_rect) {
  view_rect_ = view_rect;
  if (!player_) return;

  LOG_DEBUG("Set view rect to %d, %d", view_rect_.width(), view_rect_.height());
  int32_t ret = player_->SetDisplayRect(
      view_rect_, std::bind(&UrlPlayerController::OnSetDisplayRect, this, _1));
  if (ret < ErrorCodes::CompletionPending)
    LOG_ERROR("SetDisplayRect result: %d", ret);
}

void UrlPlayerController::PostTextTrackInfo() {
  int32_t ret = player_->GetTextTracksList(text_track_list_);
  if (ret == ErrorCodes::Success) {
    LOG_INFO("GetTextTrackInfo called successfully");
    message_sender_->SetTextTracks(text_track_list_);
  } else {
    LOG_ERROR("GetTextTrackInfo call failed, code: %d", ret);
  }
}

void UrlPlayerController::ChangeSubtitles(int32_t id) {
  LOG_INFO("Change subtitle to %d", id);
  player_thread_->message_loop().PostWork(
      cc_factory_.NewCallback(
          &UrlPlayerController::OnChangeSubtitles, id));
}

void UrlPlayerController::ChangeSubtitleVisibility() {
  subtitles_visible_ = !subtitles_visible_;
  LOG_INFO("Change subtitle visibility to %d", subtitles_visible_);
  player_thread_->message_loop().PostWork(
      cc_factory_.NewCallback(
          &UrlPlayerController::OnChangeSubVisibility,
          subtitles_visible_));
}

PlayerController::PlayerState UrlPlayerController::GetState() {
  return state_;
}

void UrlPlayerController::OnSetDisplayRect(int32_t ret) {
  LOG_DEBUG("SetDisplayRect result: %d", ret);
}

void UrlPlayerController::OnSeek(int32_t ret) {
  TimeTicks current_playback_time = 0.0;
  player_->GetCurrentTime(current_playback_time);
  // Java script UI will wait for buffering complet after seek.
  // If seek files it will block seeking forever.
  if (ret != ErrorCodes::Success)
    message_sender_->BufferingCompleted();
  LOG_INFO("After seek time: %f, result: %d", current_playback_time, ret);
}

void UrlPlayerController::OnChangeSubtitles(int32_t, int32_t id) {
  int32_t ret =
      player_->SelectTrack(Samsung::NaClPlayer::ElementaryStreamType_Text, id);
  if (ret == ErrorCodes::Success) {
    LOG_INFO("SelectTrack called successfully");
  } else {
    LOG_ERROR("SelectTrack call failed, code: %d", ret);
  }
}

void UrlPlayerController::OnChangeSubVisibility(int32_t, bool show) {
  if (show)
    player_->SetSubtitleListener(listeners_.subtitle_listener);
  else
    player_->SetSubtitleListener(nullptr);
}

void UrlPlayerController::CleanPlayer() {
  LOG_INFO("Cleaning player.");
  if (!player_) return;
  player_->SetMediaEventsListener(nullptr);
  player_->SetSubtitleListener(nullptr);
  player_->SetBufferingListener(nullptr);
  player_->SetDRMListener(nullptr);
  data_source_.reset();
  state_ = PlayerState::kUnitialized;
}
