/*!
 * util.h (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Tomasz Borkowski
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DASH_UTIL_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DASH_UTIL_H_

#include <memory>
#include <string>
#include <vector>

#include "libdash/libdash.h"

#include "common.h"
#include "dash/media_segment_sequence.h"
#include "dash/media_stream.h"

class MediaSegmentSequence;
class ContentProtectionDescriptor;

constexpr double kInvalidDuration = -1.0;

struct RepresentationDescription {
  // Vector of non owning pointers.
  std::vector<dash::mpd::IBaseUrl*> base_urls;
  std::string representation_id;

  // Non owning pointers.
  dash::mpd::ISegmentBase* segment_base;
  dash::mpd::ISegmentList* segment_list;
  dash::mpd::ISegmentTemplate* segment_template;
};

struct VideoRepresentation {
  VideoStream stream;
  RepresentationDescription representation;
};

struct AudioRepresentation {
  AudioStream stream;
  RepresentationDescription representation;
};

RepresentationDescription MakeEmptyRepresentation();

template <typename T, class... Args>
MediaSegmentSequence::Iterator MakeIterator(Args&&... args) {
  return MediaSegmentSequence::Iterator(
      std::move(MakeUnique<T>(std::forward<Args>(args)...)));
}

std::unique_ptr<MediaSegmentSequence> CreateSequence(
    const RepresentationDescription& representation, uint32_t bandwidth);

/// Parses an xs:duration format to a floating-point value in seconds.
/// Returns -1.0 (kInvalidDuration) if parsing failes.
double ParseDurationToSeconds(const std::string& duration_str);

template <typename T>
T GetHighestBitrateStream(const std::vector<T>& representations) {
  if (representations.empty()) return T();
  uint32_t highest_bitrate = 0;
  uint32_t index = 0;
  for (uint32_t i = 0; i < representations.size(); ++i) {
    const T& representation = representations[i];
    if (representation.description.bitrate > highest_bitrate) {
      highest_bitrate = representation.description.bitrate;
      index = i;
    }
  }

  return representations[index];
}

/// Returns a (Video/Audio)Stream with given id. If the passed id is not found
/// in 'representations' vector, the first element is returned.
template <typename T>
T GetStreamFromId(const std::vector<T>& representations, uint32_t id) {
  if (representations.empty()) return T();
  uint32_t index = 0;
  for (uint32_t i = 0; i < representations.size(); ++i) {
    const T& representation = representations[i];
    if (representation.description.id == id) {
      index = i;
      break;
    }
  }

  return representations[index];
}

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DASH_UTIL_H_
