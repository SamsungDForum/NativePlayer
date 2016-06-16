/*!
 * segment_list_sequence.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
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
