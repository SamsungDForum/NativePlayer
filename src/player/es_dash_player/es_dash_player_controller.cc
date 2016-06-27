/*!
 * es_dash_player_controller.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
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

void EsDashPlayerController::InitPlayer(const std::string& mpd_file_path,
                                        const std::string& subtitle,
                                        const std::string& encoding) {
  LOG("Loading media from : [%s]", mpd_file_path.c_str());
  CleanPlayer();

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
  InitializeDash(mpd_file_path);
}

void EsDashPlayerController::InitializeSubtitles(const std::string& subtitle,
                                                 const std::string& encoding) {
  if (subtitle.empty()) return;

  text_track_ = MakeUnique<TextTrackInfo>();
  int32_t ret = player_->AddExternalSubtitles(subtitle, encoding, *text_track_);
  listeners_.subtitle_listener =
      make_shared<SubtitleListener>(message_sender_);

  player_->SetSubtitleListener(listeners_.subtitle_listener);
  LOG("Result of adding subtitles: %d path: %s, encoding: %s", ret,
      subtitle.c_str(), encoding.c_str());
}

void EsDashPlayerController::InitializeDash(
    const std::string& mpd_file_path) {
  // we support only PlayReady right now
  unique_ptr<DrmPlayReadyContentProtectionVisitor> visitor =
      MakeUnique<DrmPlayReadyContentProtectionVisitor>();
  dash_parser_ = DashManifest::ParseMPD(mpd_file_path, visitor.get());
  if (!dash_parser_) {
    LOG_ERROR("Failed to load/parse MPD manifest file!");
    return;
  }

  player_thread_ = MakeUnique<pp::SimpleThread>(instance_);
  player_thread_->Start();

  auto es_data_source = std::make_shared<ESDataSource>();
  TimeTicks duration = ParseDurationToSeconds(dash_parser_->GetDuration());
  LOG("Duration from the manifest file: '%s', parsed: %f [s]",
      dash_parser_->GetDuration().c_str(), duration);
  if (duration != kInvalidDuration) {
    es_data_source->SetDuration(duration);
    message_sender_->SetMediaDuration(duration);
  } else {
    LOG_DEBUG("Invalid media duration!");
  }
  data_source_ = es_data_source;
  streams_.fill({});
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
  if (video_representations_.empty()) return;

  LOG("Video reps count: %d", video_representations_.size());
  VideoStream s = GetHighestBitrateStream(video_representations_);
  message_sender_->SetRepresentations(video_representations_);
  message_sender_->ChangeRepresentation(StreamType::Video,
                                            s.description.id);
  LOG_DEBUG("Chosen video rep is: %d x %d, bitrate: %u, id: %d", s.width,
            s.height, s.description.bitrate, s.description.id);

  if (s.description.content_protection) {  // DRM content detected
    LOG("DRM content detected.");
    auto drm_listener = make_shared<DrmPlayReadyListener>(instance_, player_);

    drm_listener->SetContentProtectionDescriptor(
        std::static_pointer_cast<DrmPlayReadyContentProtectionDescriptor>(
            s.description.content_protection));

    player_->SetDRMListener(drm_listener);
  }

  auto& stream_manager = streams_[static_cast<int32_t>(StreamType::Video)];
  stream_manager = make_shared<StreamManager>(instance_, StreamType::Video);
  auto callback = WeakBind(&EsDashPlayerController::OnStreamConfigured,
      std::static_pointer_cast<EsDashPlayerController>(
          shared_from_this()), _1);

  bool success = stream_manager->Initialize(
      dash_parser_->GetVideoSequence(s.description.id),
      data_source_, callback, drm_type);

  if (!success) {
    LOG_ERROR("Failed to initialize video stream manager");
    state_ = PlayerState::kError;
  }
}

void EsDashPlayerController::InitializeAudioStream(
    Samsung::NaClPlayer::DRMType drm_type) {
  if (audio_representations_.empty()) return;

  LOG("Audio reps count: %d", audio_representations_.size());
  AudioStream s = GetHighestBitrateStream(audio_representations_);
  message_sender_->SetRepresentations(audio_representations_);
  message_sender_->ChangeRepresentation(StreamType::Audio,
                                            s.description.id);
  LOG_DEBUG("Chosen audio rep is: %s, bitrate: %u, id: %d", s.language.c_str(),
            s.description.bitrate, s.description.id);

  if (s.description.content_protection) {  // DRM content detected
    LOG("DRM content detected.");
    auto drm_listener = make_shared<DrmPlayReadyListener>(instance_, player_);

    drm_listener->SetContentProtectionDescriptor(
        std::static_pointer_cast<DrmPlayReadyContentProtectionDescriptor>(
            s.description.content_protection));

    player_->SetDRMListener(drm_listener);
  }

  auto& stream_manager = streams_[static_cast<int32_t>(StreamType::Audio)];
  stream_manager = make_shared<StreamManager>(instance_, StreamType::Audio);
  auto callback = WeakBind(&EsDashPlayerController::OnStreamConfigured,
      std::static_pointer_cast<EsDashPlayerController>(
          shared_from_this()), _1);

  bool success = stream_manager->Initialize(
      dash_parser_->GetAudioSequence(s.description.id),
      data_source_, callback, drm_type);

  if (!success) {
    LOG_ERROR("Failed to initialize audio stream manager");
    state_ = PlayerState::kError;
  }
}

void EsDashPlayerController::Play() {
  if (!player_) {
    LOG("Play. player_ is null");
    return;
  }

  int32_t ret = player_->Play();
  if (ret == ErrorCodes::Success) {
    LOG("Play called successfully");
  } else {
    LOG_ERROR("Play call failed, code: %d", ret);
  }
}

void EsDashPlayerController::Pause() {
  if (!player_) {
    LOG("Pause. player_ is null");
    return;
  }

  int32_t ret = player_->Pause();
  if (ret == ErrorCodes::Success) {
    LOG("Pause called successfully");
  } else {
    LOG_ERROR("Pause call failed, code: %d", ret);
  }
}

void EsDashPlayerController::CleanPlayer() {
  LOG("Cleaning player.");
  if (player_) return;
  player_thread_.reset();
  data_source_.reset();
  dash_parser_.reset();
  streams_.fill({});
  state_ = PlayerState::kUnitialized;
  video_representations_.clear();
  audio_representations_.clear();
  LOG("Finished closing.");
}

void EsDashPlayerController::Seek(TimeTicks to_time) {
  LOG("Seek to %f", to_time);
  auto callback = WeakBind(&EsDashPlayerController::OnSeek,
      std::static_pointer_cast<EsDashPlayerController>(
          shared_from_this()), _1);
  int32_t ret = player_->Seek(to_time, callback);
  if (ret < ErrorCodes::CompletionPending) {
    LOG_ERROR("Seek call failed, code: %d", ret);
  }
}

void EsDashPlayerController::OnSeek(int32_t ret) {
  TimeTicks current_playback_time = 0.0;
  player_->GetCurrentTime(current_playback_time);
  LOG("After seek time: %f, result: %d", current_playback_time, ret);
  message_sender_->BufferingCompleted();
}

void EsDashPlayerController::ChangeRepresentation(StreamType stream_type,
                                                  int32_t id) {
  LOG("Changing rep type: %d to id: %d", stream_type, id);
  player_thread_->message_loop().PostWork(cc_factory_.NewCallback(
      &EsDashPlayerController::OnChangeRepresentation, stream_type, id));
}

void EsDashPlayerController::OnChangeRepresentation(int32_t, StreamType type,
                                                     int32_t id) {
  shared_ptr<StreamManager>& stream_manager =
      streams_[static_cast<int32_t>(type)];
  stream_manager->SetMediaSegmentSequence(
      dash_parser_->GetSequence(static_cast<MediaStreamType>(type), id));
}

void EsDashPlayerController::UpdateStreamsBuffer(int32_t) {
  LOG_DEBUG("");
  TimeTicks current_playback_time = 0.0;
  TimeTicks buffered_time = kEndOfStream;  // a lower value of audio buffered
  // time and video buffered time

  if (!player_) {
    LOG_DEBUG("player_ is null!, quit function");
    return;
  }

  player_->GetCurrentTime(current_playback_time);
  LOG_DEBUG("Current time: %f[s]", current_playback_time);
  for (auto stream : streams_) {
    if (stream) {
      TimeTicks stream_buffered_time =
          stream->UpdateBuffer(current_playback_time);
      if (stream_buffered_time < buffered_time) {
        buffered_time = stream_buffered_time;
      }
    }
  }

  if (buffered_time == kEndOfStream) {  // all streams reached end of stream.
    int32_t ret = data_source_->SetEndOfStream();
    if (ret == ErrorCodes::Success)
      LOG("End of stream signalized from all streams, set EOS - OK");
    else
      LOG_ERROR("Failed to signalize end of stream to ESDataSource");
    return;
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
    LOG("GetTextTrackInfo called successfully");
    message_sender_->SetTextTracks(text_track_list_);
  } else {
    LOG_ERROR("GetTextTrackInfo call failed, code: %d", ret);
  }
}

void EsDashPlayerController::ChangeSubtitles(int32_t id) {
  LOG("Change subtitle to %d", id);
  player_thread_->message_loop().PostWork(
      cc_factory_.NewCallback(
          &EsDashPlayerController::OnChangeSubtitles, id));
}

void EsDashPlayerController::ChangeSubtitleVisibility() {
  subtitles_visible_ = !subtitles_visible_;
  LOG("Change subtitle visibility to %d", subtitles_visible_);
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
  for (auto stream : streams_) {
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
  LOG("All streams configured, attaching data source.");
  // Audio and video stream should be initialized already.
  if (!player_) {
    LOG_DEBUG("player_ is null!, quit function");
    return;
  }
  int32_t result = player_->AttachDataSource(*data_source_);

  if (result == ErrorCodes::Success && state_ != PlayerState::kError) {
    state_ = PlayerState::kReady;
    LOG("Data Source attached");
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
    LOG("SelectTrack called successfully");
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
