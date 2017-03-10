/*!
 * media_segment_sequence.h (https://github.com/SamsungDForum/NativePlayer)
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

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DASH_MEDIA_SEGMENT_SEQUENCE_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DASH_MEDIA_SEGMENT_SEQUENCE_H_

#include <memory>
#include <vector>

/// @file
/// @brief This file defines <code>MediaSegmentSequence</code> class.

namespace dash {
namespace mpd {
class ISegment;
}
}

class SequenceIterator;

/// @class MediaSegmentSequence
/// @brief Container of possibly infinite (in case of live streams) sequence
/// of  MediaSegments for particular representation described
/// in the DASH manifest. This class is used by <code>DashManifest</code>
///
/// @note Depending on type of the content this sequence might be mutable or
/// not!\n For live streaming this sequence might be mutable.
class MediaSegmentSequence {
 public:
  static constexpr double kInvalidSegmentDuration = -1.0;
  static constexpr double kInvalidSegmentTimestamp = -1.0;

  virtual ~MediaSegmentSequence();

  /// @class Iterator
  /// @brief Constant bidirectional iterator through the sequence.
  ///
  /// For live streams incrementation and decrementation operations might
  /// return past-the-end iterator when a segment is either not yet available
  /// or no more segments are available.
  class Iterator {
   public:
    /// Constructs an empty <code>Iterator</code> object.
    Iterator();

    /// Constructs an <code>Iterator</code> object
    /// from <code>SequenceIterator</code>.
    explicit Iterator(std::unique_ptr<SequenceIterator>&&);

    /// Constructs a copy of <code>other</code>.
    Iterator(const Iterator& other);

    /// Move-constructs a <code>Iterator</code> object, making it point at the
    /// same object that <code>other</code> was pointing to.
    Iterator(Iterator&& other);

    /// Destroys <code>Iterator</code> object.
    ~Iterator();

    /// Assigns <code>other</code> to this <code>Iterator</code>
    Iterator& operator=(const Iterator& other);

    /// Move-assigns <code>other</code> to this
    /// <code>Iterator</code> object.
    Iterator& operator=(Iterator&& other);

    /// Advances the iterator by one position.
    /// @return *this.
    Iterator& operator++();

    /// Advances the iterator by one position.
    /// @return The value *this had before the call.
    Iterator operator++(int);

    /// Decreases the iterator by one position.
    /// @return *this.
    Iterator& operator--();

    /// Decreases the iterator by one position.
    /// @return The value *this had before the call.
    Iterator operator--(int);

    /// Checks if compared object is equal to current object.
    /// @return True if objects are equal.\n False otherwise
    bool operator==(const Iterator&) const;

    /// Checks if compared object is different from current object.
    /// @return True if objects are different.\n True otherwise
    bool operator!=(const Iterator&) const;

    /// @note Iteration happens over ISegment*, however a call to
    /// this method creates a new ISegment* object!

    /// Returns a smart pointer to the segment which is a copy of the
    /// ISegment pointed by the iterator.
    /// @return A smart pointer to an ISegment instance returned by value.
    std::unique_ptr<dash::mpd::ISegment> operator*() const;

    /// Provides a segment duration for the given MediaSegmentSequence.
    /// @return Segment duration in seconds.\n Value < 0 in case of error
    /// (like invalid MediaSegmentSequence).
    double SegmentDuration(const MediaSegmentSequence*) const;

    /// Provides a segment timestamp for given the MediaSegmentSequence.
    /// @return Segment timestamp in seconds.\n Value < 0 in case of error
    /// (like invalid MediaSegmentSequence).
    double SegmentTimestamp(const MediaSegmentSequence*) const;

   private:
    std::unique_ptr<SequenceIterator> pimpl_;
  };

  /// @return Iterator to first element.
  virtual Iterator Begin() const = 0;

  /// @return Past-the-end iterator.
  virtual Iterator End() const = 0;

  // TODO(samsung) use units from nacl-player
  /// Searches for a segment which has frames for given timestamp.
  /// @return Iterator for given timestamp.
  virtual Iterator MediaSegmentForTime(double time) const = 0;

  /// Provides an initialization segment of media stream needed to configure
  /// demuxer.
  /// @return New Initialization Segment object.
  virtual std::unique_ptr<dash::mpd::ISegment> GetInitSegment() const = 0;

  /// Provides a bitstream switching segment of media stream needed in live
  /// profile.
  /// @return New Bitstream Switching Segment object.
  virtual std::unique_ptr<dash::mpd::ISegment> GetBitstreamSwitchingSegment()
      const = 0;

  /// Provides a representation index segment of media stream.
  /// @return New Representation Index Segment object.
  virtual std::unique_ptr<dash::mpd::ISegment> GetRepresentationIndexSegment()
      const = 0;

  /// Provides a index segment of media stream.
  /// @return New Index Segment object.
  virtual std::unique_ptr<dash::mpd::ISegment> GetIndexSegment() const = 0;

  /// Provides an average segment duration of media stream. Can be useful when
  /// configuring media content data loader to run in a background thread or
  /// other <code>MediaSegmentSequance</code> operations.
  /// @return Average segment duration in seconds.
  virtual double AverageSegmentDuration() const = 0;

  /// Provides a segment duration for the given Iterator.
  /// @return Segment duration in seconds.\n Value < 0 in case of error
  /// (like invalid iterator, passed iterator doesn't points to current
  /// sequence).
  virtual double SegmentDuration(const Iterator& it) const;

  /// Provides a segment timestamp for the given Iterator.
  /// @return Segment timestamp in seconds.\n Value < 0 in case of error
  /// (like invalid iterator, passed iterator doesn't points to current
  /// sequence).
  virtual double SegmentTimestamp(const Iterator& it) const;
};

/// Downloads the whole segment to the vector pointed by data for the given
/// segment.
///
/// @param[in] seg An ISegment for which data will be downloaded.
/// @param[out] data An array container to which data will be downloaded.
/// @return True if download succeed.\n False if download fails (e.g. wrong
/// ISegment).
bool DownloadSegment(dash::mpd::ISegment* seg, std::vector<uint8_t>* data);

/// Downloads whole segment to vector pointed by data for given segment.
/// @note This method calls  <code>DownloadSegment(dash::mpd::ISegment* seg,
/// std::vector<uint8_t>* data)</code>
///
/// @param[in] seg An ISegment for which data will be downloaded.
/// @param[out] data An array container to which data will be downloaded.
/// @return True if download succeed.\n False if download fails (e.g. wrong
/// ISegment).
inline
bool DownloadSegment(std::unique_ptr<dash::mpd::ISegment> seg,
    std::vector<uint8_t>* data) {
  return DownloadSegment(seg.get(), data);
}

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DASH_MEDIA_SEGMENT_SEQUENCE_H_
