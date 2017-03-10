/*!
 * representation_builder.h (https://github.com/SamsungDForum/NativePlayer)
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

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DASH_REPRESENTATION_BUILDER_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DASH_REPRESENTATION_BUILDER_H_

#include <vector>
#include <string>

#include "libdash/libdash.h"

#include "dash/content_protection_visitor.h"

#include "util.h"


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


class RepresentationBuilder {
 public:
  RepresentationBuilder(dash::mpd::IMPD*, ContentProtectionVisitor*);
  RepresentationBuilder(const RepresentationBuilder&) = default;
  RepresentationBuilder(RepresentationBuilder&&) = default;
  RepresentationBuilder& operator=(const RepresentationBuilder&) = default;
  RepresentationBuilder& operator=(RepresentationBuilder&&) = default;
  ~RepresentationBuilder() = default;

  RepresentationBuilder Visit(dash::mpd::IPeriod*) const;
  RepresentationBuilder Visit(dash::mpd::IAdaptationSet*) const;
  RepresentationBuilder Visit(dash::mpd::IRepresentation*) const;

  void EmitRepresentation(std::vector<VideoRepresentation>& video,
                          std::vector<AudioRepresentation>& audio) const;

 private:
  void ExtractAudioInfo(dash::mpd::IRepresentationBase*);
  void ExtractVideoInfo(dash::mpd::IRepresentationBase*);
  void ExtractContentProtection(dash::mpd::IRepresentationBase*);
  void ExtractInfo(dash::mpd::IRepresentationBase*);
  void ExtractRepresentationType(dash::mpd::IAdaptationSet*);
  void ExtractRepresentationType(dash::mpd::IRepresentation*);

  void ProcessNode(dash::mpd::IPeriod*);
  void ProcessNode(dash::mpd::IAdaptationSet*);
  void ProcessNode(dash::mpd::IRepresentation*);

  void EmitAudioRepresentation(std::vector<AudioRepresentation>& audio) const;
  void EmitVideoRepresentation(std::vector<VideoRepresentation>& video) const;

  RepresentationDescription representation_;

  MediaStreamType type_;
  AudioStream audio_;
  VideoStream video_;

  std::shared_ptr<ContentProtectionDescriptor> drm_descriptor_;

  // Non owning pointer
  ContentProtectionVisitor* visitor_;
};

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DASH_REPRESENTATION_BUILDER_H_
