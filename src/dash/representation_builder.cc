/*!
 * representation_builder.cc (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Adam Bujalski
 */

#include "representation_builder.h"

#include <vector>
#include <string>

#include "dash/content_protection_visitor.h"
#include "dash/dash_manifest.h"

const char kAudioTypeString[] = "audio/";
const char kVideoTypeString[] = "video/";

template <typename T>
void UpdateIfNotNull(T*& val, T* new_val) {
  if (new_val) val = new_val;
}

template <typename T>
void AppendIfNotNull(std::vector<T*>& dest, T* val) {
  if (val) dest.push_back(val);
}

template <typename T>
void AppendIfNotEmpty(std::vector<T>& dest, const std::vector<T>& src) {
  if (!src.empty())
    dest.push_back(src[0]);  // TODO(samsung) what if ther will be more URLs?
}

template <typename T>
void UpdateRepresentation(RepresentationDescription& rep, T* mpd_element) {
  // TODO(samsung) add support for multiple URLs...
  AppendIfNotEmpty(rep.base_urls, mpd_element->GetBaseURLs());
  UpdateIfNotNull(rep.segment_base, mpd_element->GetSegmentBase());
  UpdateIfNotNull(rep.segment_list, mpd_element->GetSegmentList());
  UpdateIfNotNull(rep.segment_template, mpd_element->GetSegmentTemplate());
}

MediaStreamType ParseContentType(const std::string& type) {
  if (type == kAudioTypeString) return MediaStreamType::Audio;

  if (type == kVideoTypeString) return MediaStreamType::Video;

  return MediaStreamType::Unknown;
}

MediaStreamType ParseTypeFromMimeType(const std::string& mime_type) {
  if (mime_type.empty()) return MediaStreamType::Unknown;

  if (mime_type.compare(0, sizeof(kAudioTypeString) - 1, kAudioTypeString) == 0)
    return MediaStreamType::Audio;

  if (mime_type.compare(0, sizeof(kVideoTypeString) - 1, kVideoTypeString) == 0)
    return MediaStreamType::Video;

  return MediaStreamType::Unknown;
}

RepresentationBuilder::RepresentationBuilder(dash::mpd::IMPD* mpd,
                                             ContentProtectionVisitor* visitor)
    : representation_(MakeEmptyRepresentation()),
      type_(MediaStreamType::Unknown),
      visitor_(visitor) {
  AppendIfNotNull(representation_.base_urls, mpd->GetMPDPathBaseUrl());
  AppendIfNotEmpty(representation_.base_urls, mpd->GetBaseUrls());
}

RepresentationBuilder RepresentationBuilder::Visit(
    dash::mpd::IPeriod* period) const {
  RepresentationBuilder builder = *this;
  builder.ProcessNode(period);
  return builder;
}

RepresentationBuilder RepresentationBuilder::Visit(
    dash::mpd::IAdaptationSet* adaptation_set) const {
  RepresentationBuilder builder = *this;
  builder.ProcessNode(adaptation_set);
  return builder;
}

RepresentationBuilder RepresentationBuilder::Visit(
    dash::mpd::IRepresentation* representation) const {
  RepresentationBuilder builder = *this;
  builder.ProcessNode(representation);
  return builder;
}

void RepresentationBuilder::EmitRepresentation(
    std::vector<VideoRepresentation>& video,
    std::vector<AudioRepresentation>& audio) const {
  if (type_ == MediaStreamType::Audio)
    EmitAudioRepresentation(audio);
  else if (type_ == MediaStreamType::Video)
    EmitVideoRepresentation(video);
}

void RepresentationBuilder::ExtractAudioInfo(dash::mpd::IRepresentationBase*) {
  // Currently nothing to extract for audio...
}

void RepresentationBuilder::ExtractVideoInfo(
    dash::mpd::IRepresentationBase* rb) {
  uint32_t width = rb->GetWidth();
  uint32_t height = rb->GetHeight();

  if (width > 0) video_.width = width;

  if (height > 0) video_.height = height;
}

void RepresentationBuilder::ExtractContentProtection(
    dash::mpd::IRepresentationBase* rb) {
  if (!visitor_) return;

  auto descriptor = visitor_->Visit(rb->GetContentProtection());
  if (!descriptor && !drm_descriptor_) return;

  if (!descriptor)
    descriptor = drm_descriptor_;

  if (type_ == MediaStreamType::Audio)
    audio_.description.content_protection = descriptor;
  else if (type_ == MediaStreamType::Video)
    video_.description.content_protection = descriptor;
  else
    drm_descriptor_ = descriptor;
}

void RepresentationBuilder::ExtractInfo(dash::mpd::IRepresentationBase* rb) {
  if (type_ == MediaStreamType::Audio)
    ExtractAudioInfo(rb);
  else if (type_ == MediaStreamType::Video)
    ExtractVideoInfo(rb);

  ExtractContentProtection(rb);
}

void RepresentationBuilder::ExtractRepresentationType(
    dash::mpd::IAdaptationSet* adaptation_set) {
  const dash::mpd::IContentComponent* content_component = nullptr;

  // TODO(samsung) handle situation when GetContentComponent provides more than
  // one IContentComponent
  if (!adaptation_set->GetContentComponent().empty())
    content_component = adaptation_set->GetContentComponent()[0];

  type_ =
      ParseContentType(content_component ? content_component->GetContentType()
                                         : adaptation_set->GetContentType());
  if (type_ == MediaStreamType::Unknown)
    type_ = ParseTypeFromMimeType(adaptation_set->GetMimeType());

  if (type_ == MediaStreamType::Audio) {
    audio_.language = (content_component ? content_component->GetLang()
                                         : adaptation_set->GetLang());
  }
}

void RepresentationBuilder::ProcessNode(dash::mpd::IPeriod* period) {
  UpdateRepresentation(representation_, period);
}

void RepresentationBuilder::ProcessNode(
    dash::mpd::IAdaptationSet* adaptation_set) {
  drm_descriptor_ = nullptr;
  UpdateRepresentation(representation_, adaptation_set);
  ExtractRepresentationType(adaptation_set);
  // ExtractInfo rely on determined representation type
  ExtractInfo(adaptation_set);
}

void RepresentationBuilder::ProcessNode(
    dash::mpd::IRepresentation* representation) {
  UpdateRepresentation(representation_, representation);
  representation_.representation_id = representation->GetId();

  // In some cases representation type is determined on IRepresentation level
  ExtractRepresentationType(representation);
  // ExtractInfo rely on determined representation type
  ExtractInfo(representation);

  uint32_t bandwidth = representation->GetBandwidth();
  if (bandwidth > 0 && type_ == MediaStreamType::Audio)
    audio_.description.bitrate = bandwidth;
  else if (bandwidth > 0 && type_ == MediaStreamType::Video)
    video_.description.bitrate = bandwidth;
}

void RepresentationBuilder::ExtractRepresentationType(
    dash::mpd::IRepresentation* representation) {
  if (type_ == MediaStreamType::Unknown)
    type_ = ParseTypeFromMimeType(representation->GetMimeType());
}

void RepresentationBuilder::EmitAudioRepresentation(
    std::vector<AudioRepresentation>& audio) const {
  AudioRepresentation rep = {audio_, representation_};
  rep.stream.description.id = audio.size();
  audio.push_back(rep);
}

void RepresentationBuilder::EmitVideoRepresentation(
    std::vector<VideoRepresentation>& video) const {
  VideoRepresentation rep = {video_, representation_};
  rep.stream.description.id = video.size();
  video.push_back(rep);
}
