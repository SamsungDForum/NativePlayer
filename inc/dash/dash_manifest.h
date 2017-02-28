/*!
 * dash_manifest.h (https://github.com/SamsungDForum/NativePlayer)
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

#ifndef NATIVE_PLAYER_INC_PLAYER_ES_DASH_PLAYER_DASH_DASH_MANIFEST_H_
#define NATIVE_PLAYER_INC_PLAYER_ES_DASH_PLAYER_DASH_DASH_MANIFEST_H_

#include <memory>
#include <string>
#include <vector>

#include "dash/content_protection_visitor.h"
#include "dash/media_segment_sequence.h"
#include "dash/media_stream.h"

namespace dash {

class IDASHManager;

namespace mpd {

class IMPD;

}
}

/// @file
/// @brief This file defines <code>DashManifest</code> class.

/// @class DashManifest
/// @brief This class is responsible for parsing DASH manifest file from the
/// given URL.
///
/// This class provides information about the media streams available
/// in the DASH manifest.
/// This class allows to:
///   - Parse the DASH manifest from the given URL
///   - List available information about tracks (audio/video)
///   - Set/switch representation of the media stream
///   - Download an init segment of representation - needed to initialize
///     demuxer
///   - Download a next segment - next chunks of media container for defined
///     period of time parsed from the DASH manifest
///   - Seek - setting timestamp for a next segment to be downloaded
///   - Download segment - download a segment for the given timestamp or any
///     other special segment, e.g. IndexSegment
///
/// For more details please refer to:
/// @see MediaSegmentSequence
/// @see AudioStream
/// @see VideoStream
class DashManifest {
  template <typename T, class... Args>
  friend std::unique_ptr<T> MakeUnique(Args&&... args);

 public:
  /// Destroys <code>DashManifest</code> object.
  ~DashManifest();

  /// Parses DASH manifest and creates DashManifest object from given URL.
  ///
  /// @param[in] url A localization of the DASH manifest file
  /// @param[in] visitor A <code>ContentProtectionVisitor</code> which is used
  ///   to extract content protection information from the DASH manifest
  /// @return A DashManifest object created from the DASH manifest.\n An
  ///   <code>empty unique_ptr</code> in case of an error e.g. wrong URL.
  static std::unique_ptr<DashManifest> ParseMPD(
      const std::string& url, ContentProtectionVisitor* visitor = nullptr);

  /// Provides information about available <code>AudioStream</code>
  /// representations.
  /// @return A vector of <code>AudioStream</code> representations parsed
  /// from DASH manifest.
  std::vector<AudioStream> GetAudioStreams() const;

  /// Provides information about available <code>VideoStream</code>
  /// representations.
  /// @return A vector of <code>VideoStream</code> representations parsed
  /// from DASH manifest.
  std::vector<VideoStream> GetVideoStreams() const;

  /// Provides a segment sequence for the given parameters.
  /// This method calls DashManifest::GetAudioSequence or
  /// DashManifest::GetVideoSequence depending of passed <code>type</code>.
  ///
  /// @param[in] type An information about <code>MediaStreamType</code> for
  /// which <code>MediaSegmentSequence</code> will be returned.
  /// @param[in] id An information about which media stream representation
  /// <code>MediaSegmentSequence</code> is demanded.
  /// @return A <code>MediaSegmentSequence</code> object for the given
  /// <code>type</code> and <code>id</code>.
  std::unique_ptr<MediaSegmentSequence> GetSequence(MediaStreamType type,
                                                    uint32_t id);

  /// Provides a segment sequence for the given parameter among audio stream
  /// representations.
  ///
  /// @param[in] id An information about which audio stream representation
  /// <code>MediaSegmentSequence</code> is demanded.
  /// @return A <code>MediaSegmentSequence</code> object for the given
  /// <code>id</code>.
  std::unique_ptr<MediaSegmentSequence> GetAudioSequence(uint32_t id);

  /// Provides a segment sequence for the given parameter among video stream
  /// representations.
  ///
  /// @param[in] id An information about which video stream representation
  /// <code>MediaSegmentSequence</code> is demanded.
  /// @return A <code>MediaSegmentSequence</code> object for the given
  /// <code>id</code>.
  std::unique_ptr<MediaSegmentSequence> GetVideoSequence(uint32_t id);

  /// Provides duration of media content in text format parsed from DASH
  /// manifest.
  /// @note Format of duration is <code>xs:duration</code> which is in the
  /// following form "PnYnMnDTnHnMnS" where:
  ///   -  P indicates the period (required)
  ///   -  nY indicates the number of years
  ///   -  nM indicates the number of months
  ///   -  nD indicates the number of days
  ///   -  T indicates the start of a time section
  ///   (required if you are going to specify hours, minutes, or seconds)
  ///   -  nH indicates the number of hours
  ///   -  nM indicates the number of minutes
  ///   -  nS indicates the number of seconds
  ///
  /// @return A duration of the media content in string format provided in the
  ///   DASH manifest.
  const std::string& GetDuration() const;

 private:
  DashManifest(std::unique_ptr<dash::IDASHManager> manager,
               std::unique_ptr<dash::mpd::IMPD> mpd,
               ContentProtectionVisitor* visitor);

  DashManifest(const DashManifest&) = delete;
  DashManifest(DashManifest&&) = delete;

  DashManifest operator=(const DashManifest&) = delete;
  DashManifest operator=(DashManifest&&) = delete;

  class Impl;
  std::unique_ptr<Impl> pimpl_;
};

#endif  // NATIVE_PLAYER_INC_PLAYER_ES_DASH_PLAYER_DASH_DASH_MANIFEST_H_
