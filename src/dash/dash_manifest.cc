/*!
 * dash_manifest.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 */

#include "dash/dash_manifest.h"

#include <vector>
#include <string>
#include <cassert>

#include "libdash/libdash.h"

#include "dash/media_stream.h"
#include "dash/media_segment_sequence.h"

#include "representation_builder.h"

// From DASH spec:
//
// A Media Presentation as described in the MPD consists of
// - A sequence of one or more Periods as described in 5.3.2.
//
// - Each Period contains one or more Adaptation Sets as described in 5.3.3.
//   In case an Adaptation Set contains multiple media content components,
//   then each media content component is described individually
//   as defined in 5.3.4.
//
// - Each Adaptation Set contains one or more Representations as described
//   in 5.3.5.
//
// - A Representation may contain one or more Sub-Representations as described
//   in 5.3.6.
//
// - Adaptation Sets, Representations and Sub-Representations share common
//   attributes and elements that are described in 5.3.7.
//
// - Each Period may contain one or more Subsets that restrict combination
//   of Adaptation Sets for presentation. Subsets are described in 5.3.8.
//
// - Each Representation consists of one or more Segments described in 6.
//   Segment Information is introduced in 5.3.9. Segments contain media data
//   and/or metadata to access, decode and present the included media content.
//   Representations may also include Sub-Representations as defined in 5.3.6
//   to describe and extract partial information from a Representation.
//
// - Each Segment consists of one or more Subsegments.
//   Subsegments are described in 6.2.3.2.


class DashManifest::Impl {
 public:
  Impl(std::unique_ptr<dash::IDASHManager> manager,
       std::unique_ptr<dash::mpd::IMPD> mpd,
       ContentProtectionVisitor* visitor);
  ~Impl() = default;

  Impl(const Impl&) = delete;
  Impl(Impl&&) = delete;
  Impl& operator=(const Impl&) = delete;
  Impl& operator=(Impl&&) = delete;

  // Assumptions for {Audio|Video}Stream:
  // Id of the stream (field) represenation.description.id
  // equals to index of stream!
  std::vector<AudioStream> GetAudioStreams() const;
  std::vector<VideoStream> GetVideoStreams() const;

  // Assumptions for {Audio|Video}Stream:
  // Id of the representation (field) represenation.description.id
  // equals to index of stream!
  std::unique_ptr<MediaSegmentSequence> GetAudioSequence(uint32_t id);
  std::unique_ptr<MediaSegmentSequence> GetVideoSequence(uint32_t id);

  const std::string& GetDuration() const;

 private:
  void ProcessMPD(ContentProtectionVisitor* visitor);
  void ProcessPeriod(dash::mpd::IPeriod* period,
                     const RepresentationBuilder& builder);
  void ProcessAdaptationSet(dash::mpd::IAdaptationSet* adaptation_set,
                            const RepresentationBuilder& builder);
  void ProcessRepresentation(dash::mpd::IRepresentation* representation,
                             const RepresentationBuilder& builder);

  std::unique_ptr<dash::IDASHManager> manager_;
  std::unique_ptr<dash::mpd::IMPD> mpd_;

  std::vector<VideoRepresentation> video_;
  std::vector<AudioRepresentation> audio_;

  dash::mpd::IPeriod* curr_period_;
};

template <typename T, typename U>
inline std::vector<T> ExtractStreamInfo(const std::vector<U>& representations) {
  std::vector<T> streams;

  for (const auto& rep : representations) streams.push_back(rep.stream);

  return streams;
}

DashManifest::Impl::Impl(std::unique_ptr<dash::IDASHManager> manager,
                         std::unique_ptr<dash::mpd::IMPD> mpd,
                         ContentProtectionVisitor* visitor)
    : manager_(std::move(manager)),
      mpd_(std::move(mpd)),
      curr_period_(nullptr) {
  ProcessMPD(visitor);
}

inline void DashManifest::Impl::ProcessMPD(ContentProtectionVisitor* visitor) {
  // Right now we support only one period in a mpd file.
  curr_period_ = mpd_->GetPeriods()[0];

  RepresentationBuilder builder(mpd_.get(), visitor);
  ProcessPeriod(curr_period_, builder);
}

inline std::vector<AudioStream> DashManifest::Impl::GetAudioStreams() const {
  return ExtractStreamInfo<AudioStream, AudioRepresentation>(audio_);
}

inline std::vector<VideoStream> DashManifest::Impl::GetVideoStreams() const {
  return ExtractStreamInfo<VideoStream, VideoRepresentation>(video_);
}

inline std::unique_ptr<MediaSegmentSequence>
DashManifest::Impl::GetAudioSequence(uint32_t id) {
  if (id >= audio_.size()) return {};

  return CreateSequence(audio_[id].representation,
                        audio_[id].stream.description.bitrate);
}

inline std::unique_ptr<MediaSegmentSequence>
DashManifest::Impl::GetVideoSequence(uint32_t id) {
  if (id >= video_.size()) return {};

  return CreateSequence(video_[id].representation,
                        video_[id].stream.description.bitrate);
}

const std::string& DashManifest::Impl::GetDuration() const {
  return mpd_->GetMediaPresentationDuration();
}

inline void DashManifest::Impl::ProcessPeriod(
    dash::mpd::IPeriod* period, const RepresentationBuilder& parent_builder) {
  RepresentationBuilder builder = parent_builder.Visit(period);

  for (auto as : period->GetAdaptationSets()) ProcessAdaptationSet(as, builder);
}

inline void DashManifest::Impl::ProcessAdaptationSet(
    dash::mpd::IAdaptationSet* adaptation_set,
    const RepresentationBuilder& parent_builder) {
  RepresentationBuilder builder = parent_builder.Visit(adaptation_set);

  for (auto rep : adaptation_set->GetRepresentation())
    ProcessRepresentation(rep, builder);
}

inline void DashManifest::Impl::ProcessRepresentation(
    dash::mpd::IRepresentation* representation,
    const RepresentationBuilder& parent_builder) {
  RepresentationBuilder builder = parent_builder.Visit(representation);
  builder.EmitRepresentation(video_, audio_);
}

std::unique_ptr<DashManifest> DashManifest::ParseMPD(
    const std::string& url, ContentProtectionVisitor* visitor) {
  std::unique_ptr<dash::IDASHManager> manager{CreateDashManager()};
  if (!manager) return {};

  // Copy url to vector and add '\0' to avoid const cast on url.c_str(),
  // as IDASHManager::Open(char*) expects non const pointer.
  std::vector<char> cfilename(url.begin(), url.end());
  cfilename.push_back('\0');
  std::unique_ptr<dash::mpd::IMPD> mpd{manager->Open(&(cfilename[0]))};
  if (!mpd) return {};

  // According to DASH spec must be at least one more Period
  assert(mpd->GetPeriods().size() > 0);
  if (mpd->GetPeriods().empty()) return {};

  auto manifest = MakeUnique<DashManifest>(
      std::move(manager), std::move(mpd), visitor);

  if (!manifest || !manifest->pimpl_) return {};

  return manifest;
}

std::vector<AudioStream> DashManifest::GetAudioStreams() const {
  return pimpl_->GetAudioStreams();
}
std::vector<VideoStream> DashManifest::GetVideoStreams() const {
  return pimpl_->GetVideoStreams();
}

std::unique_ptr<MediaSegmentSequence> DashManifest::GetSequence(
    MediaStreamType type, uint32_t id) {
  if (type == MediaStreamType::Audio) return GetAudioSequence(id);

  if (type == MediaStreamType::Video) return GetVideoSequence(id);

  return {};
}

std::unique_ptr<MediaSegmentSequence> DashManifest::GetAudioSequence(
    uint32_t id) {
  return pimpl_->GetAudioSequence(id);
}

std::unique_ptr<MediaSegmentSequence> DashManifest::GetVideoSequence(
    uint32_t id) {
  return pimpl_->GetVideoSequence(id);
}

const std::string& DashManifest::GetDuration() const {
  return pimpl_->GetDuration();
}

DashManifest::DashManifest(std::unique_ptr<dash::IDASHManager> manager,
                           std::unique_ptr<dash::mpd::IMPD> mpd,
                           ContentProtectionVisitor* visitor)
    : pimpl_(MakeUnique<DashManifest::Impl>(
        std::move(manager), std::move(mpd), visitor)) {}

DashManifest::~DashManifest() {}
