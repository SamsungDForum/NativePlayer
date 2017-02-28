/*!
 * segment_template_iterator.h (https://github.com/SamsungDForum/NativePlayer)
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

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DASH_SEGMENT_TEMPLATE_SEQUENCE_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DASH_SEGMENT_TEMPLATE_SEQUENCE_H_

#include "dash/media_segment_sequence.h"

#include <string>
#include <vector>

#include "sequence_iterator.h"

class SegmentTemplateIterator;
struct RepresentationDescription;

// FIXME missing support for <SegmentTimeline> attributes
class SegmentTemplateSequence : public MediaSegmentSequence {
 public:
  SegmentTemplateSequence(const RepresentationDescription& desc,
                          uint32_t bandwidth);
  virtual ~SegmentTemplateSequence();

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
  struct SegmentTimes {
      SegmentTimes(uint64_t start_time, uint32_t duration)
          : start_time_(start_time), duration_(duration){}
      uint64_t start_time_;
      uint32_t duration_;
  };

  void ExtractSegmentDuration();
  void ExtractStartIndex();
  void CalculateSegmentStartTimes();

  double Duration(uint32_t) const;
  double Timestamp(uint32_t index) const;

  // For use by iterators
  std::unique_ptr<dash::mpd::ISegment> GetMediaSegmentFromNumber(
      uint32_t number) const;

  std::vector<dash::mpd::IBaseUrl*> base_urls_;
  std::string rep_id_;
  dash::mpd::ISegmentTemplate* segment_template_;
  uint32_t bandwidth_;
  uint32_t start_index_;
  uint32_t end_index_;
  double segment_duration_;
  std::vector<SegmentTimes> segment_start_times_;

  friend class SegmentTemplateIterator;
};

class SegmentTemplateIterator : public SequenceIterator {
 public:
  SegmentTemplateIterator();
  SegmentTemplateIterator(const SegmentTemplateSequence*,
                          uint32_t current_index);
  virtual ~SegmentTemplateIterator() = default;

  std::unique_ptr<SequenceIterator> Clone() const override;
  void NextSegment() override;
  void PrevSegment() override;
  std::unique_ptr<dash::mpd::ISegment> Get() const override;
  bool Equals(const SequenceIterator&) const override;

  double SegmentDuration(const MediaSegmentSequence*) const override;
  double SegmentTimestamp(const MediaSegmentSequence*) const override;

  bool operator==(const SegmentTemplateIterator&) const;

  SegmentTemplateIterator(SegmentTemplateIterator&&) = delete;
  SegmentTemplateIterator& operator=(const SegmentTemplateIterator&) = delete;
  SegmentTemplateIterator& operator=(SegmentTemplateIterator&&) = delete;
  SegmentTemplateIterator(const SegmentTemplateIterator&) = default;

 protected:
  bool EqualsTo(const SegmentTemplateIterator&) const override;

 private:
  const SegmentTemplateSequence* sequence_;
  uint32_t current_index_;
};

inline bool operator!=(const SegmentTemplateIterator& lhs,
                       const SegmentTemplateIterator& rhs) {
  return !(lhs == rhs);
}

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DASH_SEGMENT_TEMPLATE_SEQUENCE_H_
