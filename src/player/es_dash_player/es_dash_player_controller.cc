/*!
 * es_dash_player_controller.cc (https://github.com/SamsungDForum/NativePlayer)
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

#include <functional>
#include <limits>
#include <utility>

#include "ppapi/cpp/var_dictionary.h"
#include "nacl_player/error_codes.h"
#include "nacl_player/es_data_source.h"
#include "nacl_player/url_data_source.h"

#include "player/es_dash_player/es_dash_player_controller.h"

#include "demuxer/elementary_stream_packet.h"
#include "dash/dash_manifest.h"
#include "dash/util.h"

#include "drm_play_ready.h"

using Samsung::NaClPlayer::DRMType;
using Samsung::NaClPlayer::DRMType_Playready;
using Samsung::NaClPlayer::DRMType_Unknown;
using Samsung::NaClPlayer::ErrorCodes;
using Samsung::NaClPlayer::ESDataSource;
using Samsung::NaClPlayer::URLDataSource;
using Samsung::NaClPlayer::MediaDataSource;
using Samsung::NaClPlayer::MediaPlayer;
using Samsung::NaClPlayer::MediaPlayerState;
using Samsung::NaClPlayer::Rect;
using Samsung::NaClPlayer::TextTrackInfo;
using Samsung::NaClPlayer::TimeTicks;
using std::make_shared;
using std::placeholders::_1;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;

const int64_t kMainLoopDelay = 50;  // in milliseconds

namespace {

template<typename RepType>
void PrintChosenRepresentation(const RepType& s);

template<>
void PrintChosenRepresentation(const VideoStream& s) {
  LOG_INFO("Chosen video rep is: %d x %d, bitrate: %u, id: %d",
            s.width, s.height, s.description.bitrate, s.description.id);
}

template<>
void PrintChosenRepresentation(const AudioStream& s) {
  LOG_INFO("Chosen audio rep is: %s, bitrate: %u, id: %d",
            s.language.c_str(), s.description.bitrate, s.description.id);
}
}

class EsDashPlayerController::Impl {
 public:
  template<typename RepType>
  static void InitializeStream(EsDashPlayerController* thiz,
                               StreamType type,
                               Samsung::NaClPlayer::DRMType drm_type,
                               const std::vector<RepType>& representations) {
    if (representations.empty()) return;

    RepType s = GetHighestBitrateStream(representations);
    thiz->message_sender_->SetRepresentations(representations);
    thiz->message_sender_->ChangeRepresentation(type, s.description.id);
    PrintChosenRepresentation(s);

    if (s.description.content_protection) {  // DRM content detected
      LOG_INFO("DRM content detected.");
      auto playready_descriptor =
          std::static_pointer_cast<DrmPlayReadyContentProtectionDescriptor>(
              s.description.content_protection);
      if (!thiz->drm_license_url_.empty())
        playready_descriptor->system_url_ = thiz->drm_license_url_;
      if (!thiz->drm_key_request_properties_.empty()) {
        playready_descriptor->key_request_properties_ =
            thiz->drm_key_request_properties_;
      }

      thiz->drm_listener_ = make_shared<DrmPlayReadyListener>(
          thiz->instance_, thiz->player_);

      thiz->drm_listener_->SetContentProtectionDescriptor(
          playready_descriptor);

      thiz->player_->SetDRMListener(thiz->drm_listener_);
    }

    auto& stream_manager = thiz->streams_[static_cast<int32_t>(type)];
    stream_manager = MakeUnique<StreamManager>(thiz->instance_, type);
    auto configured_callback = WeakBind(
        &EsDashPlayerController::OnStreamConfigured,
        std::static_pointer_cast<EsDashPlayerController>(
            thiz->shared_from_this()), _1);
    // We bravely capture this because we are bound to outlive stream_manager.
    auto es_packet_callback = [thiz](StreamDemuxer::Message message,
        std::unique_ptr<ElementaryStreamPacket> packet) {
      thiz->packets_manager_.OnEsPacket(message, std::move(packet));
    };

    bool success = stream_manager->Initialize(
        thiz->dash_parser_->GetSequence(
            static_cast<MediaStreamType>(type), s.description.id),
        thiz->data_source_.get(), configured_callback, es_packet_callback,
        &thiz->packets_manager_, drm_type);
    thiz->packets_manager_.SetStream(type, stream_manager.get());

    if (s.description.content_protection) {
      auto play_ready_desc =
          static_cast<DrmPlayReadyContentProtectionDescriptor*>(
              s.description.content_protection.get());
      if (!play_ready_desc->init_data_type_.empty()) {
        stream_manager->SetDrmInitData(play_ready_desc->init_data_type_,
                                       play_ready_desc->init_data_);
      }
    }

    if (!success) {
      LOG_ERROR("Failed to initialize video stream manager");
      thiz->state_ = PlayerState::kError;
    }
  }
};

void EsDashPlayerController::InitPlayer(const std::string& mpd_file_path,
    const std::string& subtitle,
    const std::string& encoding,
    const std::string& drm_license_url,
    const std::unordered_map<std::string, std::string>&
        drm_key_request_properties) {
  LOG_INFO("Loading media from : [%s]", mpd_file_path.c_str());
  CleanPlayer();

  drm_license_url_ = drm_license_url;
  drm_key_request_properties_ = drm_key_request_properties;
  player_ = make_shared<MediaPlayer>();
  listeners_.player_listener =
      make_shared<MediaPlayerListener>(message_sender_);
  listeners_.buffering_listener =
      make_shared<MediaBufferingListener>(message_sender_,
                                          shared_from_this());

  player_->SetMediaEventsListener(listeners_.player_listener);
  player_->SetBufferingListener(listeners_.buffering_listener);
  int32_t ret = player_->SetDisplayRect(view_rect_);

  if (ret != ErrorCodes::Success) {
    LOG_ERROR("Failed to set display rect [(%d - %d) (%d - %d)], code: %d",
       view_rect_.x(), view_rect_.y(), view_rect_.width(), view_rect_.height(),
       ret);
  }

  InitializeSubtitles(subtitle, encoding);

  player_thread_ = MakeUnique<pp::SimpleThread>(instance_);
  player_thread_->Start();
  player_thread_->message_loop().PostWork(
      cc_factory_.NewCallback(&EsDashPlayerController::InitializeDash,
                              mpd_file_path));
}

void EsDashPlayerController::InitializeSubtitles(const std::string& subtitle,
                                                 const std::string& encoding) {
  if (subtitle.empty()) return;

  text_track_ = MakeUnique<TextTrackInfo>();
  int32_t ret = player_->AddExternalSubtitles(subtitle, encoding, *text_track_);
  listeners_.subtitle_listener =
      make_shared<SubtitleListener>(message_sender_);

  player_->SetSubtitleListener(listeners_.subtitle_listener);
  LOG_INFO("Result of adding subtitles: %d path: %s, encoding: %s", ret,
      subtitle.c_str(), encoding.c_str());
}

void EsDashPlayerController::InitializeDash(int32_t,
    const std::string& mpd_file_path) {
  // we support only PlayReady right now
  unique_ptr<DrmPlayReadyContentProtectionVisitor> visitor =
      MakeUnique<DrmPlayReadyContentProtectionVisitor>();
  dash_parser_ = DashManifest::ParseMPD(mpd_file_path, visitor.get());
  if (!dash_parser_) {
    LOG_ERROR("Failed to load/parse MPD manifest file!");
    return;
  }

  auto es_data_source = std::make_shared<ESDataSource>();
  TimeTicks duration = ParseDurationToSeconds(dash_parser_->GetDuration());
  LOG_INFO("Duration from the manifest file: '%s', parsed: %f [s]",
      dash_parser_->GetDuration().c_str(), duration);
  if (duration != kInvalidDuration) {
    es_data_source->SetDuration(duration);
    message_sender_->SetMediaDuration(duration);
  } else {
    LOG_ERROR("Invalid media duration!");
  }
  data_source_ = es_data_source;
  media_duration_ = duration;
  for (auto& stream : streams_)
    stream.reset();
  video_representations_ = dash_parser_->GetVideoStreams();
  audio_representations_ = dash_parser_->GetAudioStreams();

  player_thread_->message_loop().PostWork(
      cc_factory_.NewCallback(&EsDashPlayerController::InitializeStreams));
  player_thread_->message_loop().PostWork(
      cc_factory_.NewCallback(&EsDashPlayerController::UpdateStreamsBuffer));
}

void EsDashPlayerController::InitializeStreams(int32_t) {
  // Currently only Playready is supported
  DRMType drm_type = DRMType_Playready;

  InitializeVideoStream(drm_type);
  InitializeAudioStream(drm_type);
}

void EsDashPlayerController::InitializeVideoStream(
    Samsung::NaClPlayer::DRMType drm_type) {
  Impl::InitializeStream(this, StreamType::Video, drm_type,
                         video_representations_);
}

void EsDashPlayerController::InitializeAudioStream(
    Samsung::NaClPlayer::DRMType drm_type) {
  Impl::InitializeStream(this, StreamType::Audio, drm_type,
                         audio_representations_);
}

void EsDashPlayerController::Play() {
  if (!player_) {
    LOG_INFO("Play. player_ is null");
    return;
  }

  int32_t ret = player_->Play();
  if (ret == ErrorCodes::Success) {
    LOG_INFO("Play called successfully");
    state_ = PlayerState::kPlaying;
  } else {
    LOG_ERROR("Play call failed, code: %d", ret);
  }
}

void EsDashPlayerController::Pause() {
  if (!player_) {
    LOG_INFO("Pause. player_ is null");
    return;
  }

  int32_t ret = player_->Pause();
  if (ret == ErrorCodes::Success) {
    LOG_INFO("Pause called successfully");
    state_ = PlayerState::kPaused;
  } else {
    LOG_ERROR("Pause call failed, code: %d", ret);
  }
}

void EsDashPlayerController::CleanPlayer() {
  LOG_INFO("Cleaning player.");
  if (!player_) return;
  player_thread_.reset();
  data_source_.reset();
  dash_parser_.reset();
  text_track_.reset();
  player_.reset();
  packets_manager_.SetStream(StreamType::Audio, nullptr);
  packets_manager_.SetStream(StreamType::Video, nullptr);
  for (auto& stream : streams_)
    stream.reset();
  state_ = PlayerState::kUnitialized;
  video_representations_.clear();
  audio_representations_.clear();
  LOG_INFO("Finished closing.");
}

void EsDashPlayerController::Seek(TimeTicks original_time) {
  if (state_ == PlayerState::kFinished) {
    LOG_INFO("Playback ended. Dropping seek to %f", original_time);
    return;
  }
  if (seeking_) {
    waiting_seek_ = MakeUnique<TimeTicks>(original_time);
    return;
  }
  seeking_ = true;
  // Seeking very close to media_duration_ will most likely place us after
  // last segment. kSegmentMargin is substracted  from media_duration_, which
  // will pretty reliable place us within the segment.
  constexpr Samsung::NaClPlayer::TimeTicks kSegmentMargin = 0.25;
  if (original_time > media_duration_ - kSegmentMargin) {
    original_time = media_duration_ - kSegmentMargin;
  } else if (original_time < kEps) {
    original_time = 0.;
  }
  auto to_time = streams_[static_cast<int>(StreamType::Video)]
      ->GetClosestKeyframeTime(original_time);
  LOG_INFO("Requested seek to %f [s], adjusted time to keyframe at %f [s]",
           original_time, to_time);

  if (drm_listener_)
    drm_listener_->Reset();

  for (const auto& stream : streams_) {
    if (stream)
      stream->PrepareForSeek(to_time);
  }

  packets_manager_.PrepareForSeek(to_time);

  auto callback = WeakBind(&EsDashPlayerController::OnSeek,
      std::static_pointer_cast<EsDashPlayerController>(
          shared_from_this()), _1);

  int32_t ret = player_->Seek(to_time, callback);
  if (ret < ErrorCodes::CompletionPending) {
    LOG_ERROR("Seek call failed, code: %d", ret);
  }
}

void EsDashPlayerController::OnSeek(int32_t ret) {
  if (ret == PP_OK) {
    seeking_ = false;
    if (waiting_seek_) {
      auto waiting_seek = *waiting_seek_;
      waiting_seek_.reset();
      Seek(waiting_seek);
      return;
    }
    TimeTicks current_playback_time = 0.0;
    player_->GetCurrentTime(current_playback_time);
    LOG_INFO("After seek, time: %f, result: %d", current_playback_time, ret);
  } else {
    LOG_ERROR("Seek failed with code: %d", ret);
  }
  message_sender_->BufferingCompleted();
  PerformWaitingOperations();
}

void EsDashPlayerController::ChangeRepresentation(StreamType stream_type,
                                                  int32_t id) {
  LOG_INFO("Changing rep type: %d -> %d", stream_type, id);
  player_thread_->message_loop().PostWork(cc_factory_.NewCallback(
      &EsDashPlayerController::OnChangeRepresentation, stream_type, id));
}

void EsDashPlayerController::OnChangeRepresentation(int32_t, StreamType type,
                                                     int32_t id) {
  if (seeking_) {
    waiting_representation_changes_[static_cast<size_t>(type)]
        = MakeUnique<int32_t>(id);
    return;
  }

  if (drm_listener_)
    drm_listener_->Reset();

  const auto& stream_manager =
      streams_[static_cast<int32_t>(type)];
  stream_manager->SetMediaSegmentSequence(
      dash_parser_->GetSequence(static_cast<MediaStreamType>(type), id));
}

void EsDashPlayerController::UpdateStreamsBuffer(int32_t) {
  TimeTicks current_playback_time = 0.0;

  if (!player_) {
    LOG_DEBUG("player_ is null!, quit function");
    return;
  }

  if (static_cast<int>(state_) > static_cast<int>(PlayerState::kReady)) {
    player_->GetCurrentTime(current_playback_time);
  } else {
    current_playback_time = 0.;
  }
  LOG_DEBUG("Current time: %f [s]", current_playback_time);

  bool segments_pending = false;

  for (const auto& stream : streams_) {
    if (stream) {
        segments_pending |= stream->UpdateBuffer(current_playback_time);
    }
  }

  if (static_cast<int>(state_) >= static_cast<int>(PlayerState::kReady) &&
      (!drm_listener_ || drm_listener_->IsInitialized())) {
    bool has_buffered_packets = packets_manager_.UpdateBuffer(
        current_playback_time);

    // All streams reached EOS:
    if (!waiting_seek_ && !segments_pending && !has_buffered_packets &&
        packets_manager_.IsEosReached()) {
      int32_t ret = data_source_->SetEndOfStream();
      if (ret == ErrorCodes::Success) {
        state_ = PlayerState::kFinished;
        LOG_INFO("End of stream signalized from all streams, set EOS - OK");
      } else {
        LOG_ERROR("Failed to signalize end of stream to ESDataSource");
      }
      return;
    }
  }
  if (player_thread_) {
    player_thread_->message_loop().PostWork(
        cc_factory_.NewCallback(&EsDashPlayerController::UpdateStreamsBuffer),
        kMainLoopDelay);
  }
  LOG_DEBUG("Finished");
}

void EsDashPlayerController::SetViewRect(const Rect& view_rect) {
  view_rect_ = view_rect;
  if (!player_) return;

  LOG_DEBUG("Set view rect to %d, %d", view_rect_.width(), view_rect_.height());
  auto callback = WeakBind(&EsDashPlayerController::OnSetDisplayRect,
      std::static_pointer_cast<EsDashPlayerController>(
          shared_from_this()), _1);
  int32_t ret = player_->SetDisplayRect(view_rect_, callback);
  if (ret < ErrorCodes::CompletionPending)
    LOG_ERROR("SetDisplayRect result: %d", ret);
}

void EsDashPlayerController::PostTextTrackInfo() {
  int32_t ret = player_->GetTextTracksList(text_track_list_);
  if (ret == ErrorCodes::Success) {
    LOG_INFO("GetTextTrackInfo called successfully");
    message_sender_->SetTextTracks(text_track_list_);
  } else {
    LOG_ERROR("GetTextTrackInfo call failed, code: %d", ret);
  }
}

void EsDashPlayerController::ChangeSubtitles(int32_t id) {
  LOG_INFO("Change subtitle to %d", id);
  player_thread_->message_loop().PostWork(
      cc_factory_.NewCallback(
          &EsDashPlayerController::OnChangeSubtitles, id));
}

void EsDashPlayerController::ChangeSubtitleVisibility() {
  subtitles_visible_ = !subtitles_visible_;
  LOG_INFO("Change subtitle visibility to %d", subtitles_visible_);
  player_thread_->message_loop().PostWork(
      cc_factory_.NewCallback(
          &EsDashPlayerController::OnChangeSubVisibility,
          subtitles_visible_));
}

PlayerController::PlayerState EsDashPlayerController::GetState() {
  return state_;
}

void EsDashPlayerController::OnStreamConfigured(StreamType type) {
  (void)type;  // suppress warning
  LOG_DEBUG("type: %d", static_cast<int32_t>(type));
  bool all_initialized = true;
  for (const auto& stream : streams_) {
    if (stream && !(stream->IsInitialized())) {
      LOG_DEBUG("some stream is not yet initialized.");
      all_initialized = false;
    }
  }

  if (all_initialized) {
    FinishStreamConfiguration();
  }
}

void EsDashPlayerController::FinishStreamConfiguration() {
  LOG_INFO("All streams configured, attaching data source.");
  // Audio and video stream should be initialized already.
  if (!player_) {
    LOG_DEBUG("player_ is null!, quit function");
    return;
  }
  int32_t result = player_->AttachDataSource(*data_source_);

  if (result == ErrorCodes::Success && state_ != PlayerState::kError) {
    if (state_ == PlayerState::kUnitialized)
      state_ = PlayerState::kReady;
    LOG_INFO("Data Source attached");
  } else {
    state_ = PlayerState::kError;
    LOG_ERROR("Failed to AttachDataSource!");
  }
}

void EsDashPlayerController::OnSetDisplayRect(int32_t ret) {
  LOG_DEBUG("SetDisplayRect result: %d", ret);
}

void EsDashPlayerController::OnChangeSubtitles(int32_t, int32_t id) {
  int32_t ret = player_->SelectTrack(
      Samsung::NaClPlayer::ElementaryStreamType_Text, id);
  if (ret == ErrorCodes::Success) {
    LOG_INFO("SelectTrack called successfully");
  } else {
    LOG_ERROR("SelectTrack call failed, code: %d", ret);
  }
}

void EsDashPlayerController::OnChangeSubVisibility(int32_t, bool show) {
  if (show)
    player_->SetSubtitleListener(listeners_.subtitle_listener);
  else
    player_->SetSubtitleListener(nullptr);
}

void EsDashPlayerController::PerformWaitingOperations() {
  for (size_t i = 0; i < waiting_representation_changes_.size(); ++i) {
    if (waiting_representation_changes_[i]) {
      OnChangeRepresentation(PP_OK, static_cast<StreamType>(i),
                             *waiting_representation_changes_[i]);
      waiting_representation_changes_[i].reset(nullptr);
    }
  }
}
