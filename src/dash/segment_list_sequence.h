/*!
 * segment_list_sequence.h (https://github.com/SamsungDForum/NativePlayer)
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

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DASH_SEGMENT_LIST_SEQUENCE_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DASH_SEGMENT_LIST_SEQUENCE_H_

#include "dash/media_segment_sequence.h"

#include <vector>

#include "sequence_iterator.h"

struct RepresentationDescription;

class SegmentListSequence : public MediaSegmentSequence {
 public:
  SegmentListSequence(const RepresentationDescription& desc,
                      uint32_t bandwidth);
  virtual ~SegmentListSequence();

  Iterator Begin() const override;
  Iterator End() const override;

  Iterator MediaSegmentForTime(double time) const override;

  std::unique_ptr<dash::mpd::ISegment> GetInitSegment() const override;

  std::unique_ptr<dash::mpd::ISegment> GetBitstreamSwitchingSegment()
      const override;

  std::unique_ptr<dash::mpd::ISegment> GetRepresentationIndexSegment()
      const override;

  std::unique_ptr<dash::mpd::ISegment> GetIndexSegment() const override;

  double AverageSegmentDuration() const override;

 private:
  void ExtractSegmentDuration();
  double Duration(uint32_t) const { return segment_duration_; }
  double Timestamp(uint32_t index) const;

  std::vector<dash::mpd::IBaseUrl*> base_urls_;
  dash::mpd::ISegmentList* segment_list_;
  double segment_duration_;

  friend class SegmentListIterator;
};

class SegmentListIterator : public SequenceIterator {
 public:
  SegmentListIterator();
  SegmentListIterator(const SegmentListSequence*, uint32_t current_index);
  virtual ~SegmentListIterator() = default;

  std::unique_ptr<SequenceIterator> Clone() const override;
  void NextSegment() override;
  void PrevSegment() override;
  std::unique_ptr<dash::mpd::ISegment> Get() const override;
  bool Equals(const SequenceIterator&) const override;

  double SegmentDuration(const MediaSegmentSequence*) const override;
  double SegmentTimestamp(const MediaSegmentSequence*) const override;

  bool operator==(const SegmentListIterator&) const;

  SegmentListIterator(SegmentListIterator&&) = delete;
  SegmentListIterator& operator=(const SegmentListIterator&) = delete;
  SegmentListIterator& operator=(SegmentListIterator&&) = delete;
  SegmentListIterator(const SegmentListIterator&) = default;

 protected:
  bool EqualsTo(const SegmentListIterator&) const override;

 private:
  const SegmentListSequence* sequence_;
  uint32_t current_index_;
};

inline bool operator!=(const SegmentListIterator& lhs,
                       const SegmentListIterator& rhs) {
  return !(lhs == rhs);
}

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DASH_SEGMENT_LIST_SEQUENCE_H_
