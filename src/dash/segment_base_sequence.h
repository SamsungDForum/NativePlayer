/*!
 * segment_base_sequence.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DASH_SEGMENT_BASE_SEQUENCE_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DASH_SEGMENT_BASE_SEQUENCE_H_

#include "dash/media_segment_sequence.h"

#include <memory>
#include <vector>
#include <cstdlib>
#include <sstream>

#include "sequence_iterator.h"
#include "util.h"

class SegmentBaseIterator;
struct RepresentationDescription;

struct SegmentIndexEntry {
  double timestamp;
  double duration;
  uint64_t byte_offset;
  uint64_t byte_size;
};

class SegmentBaseSequence : public MediaSegmentSequence {
 public:
  SegmentBaseSequence(const RepresentationDescription& desc,
                      uint32_t bandwidth);
  virtual ~SegmentBaseSequence();

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
  void ParseSidx(const std::vector<uint8_t>& sidx, uint64_t sidx_begin,
                 uint64_t sidx_end);
  std::unique_ptr<dash::mpd::ISegment> FindIndexSegmentInMp4();
  void LoadIndexSegment();
  double Duration(uint32_t segment) const;
  double Timestamp(uint32_t segment) const;
  std::unique_ptr<dash::mpd::ISegment> GetBaseSegment() const;

  std::vector<dash::mpd::IBaseUrl*> base_urls_;
  dash::mpd::ISegmentBase* segment_base_;
  std::vector<SegmentIndexEntry> segment_index_;
  double average_segment_duration_;

  friend class SegmentBaseIterator;
};

class SegmentBaseIterator : public SequenceIterator {
 public:
  SegmentBaseIterator();
  SegmentBaseIterator(const SegmentBaseSequence*, uint32_t current_index);
  virtual ~SegmentBaseIterator() = default;

  std::unique_ptr<SequenceIterator> Clone() const override;
  void NextSegment() override;
  void PrevSegment() override;
  std::unique_ptr<dash::mpd::ISegment> Get() const override;
  bool Equals(const SequenceIterator&) const override;

  double SegmentDuration(const MediaSegmentSequence*) const override;
  double SegmentTimestamp(const MediaSegmentSequence*) const override;

  bool operator==(const SegmentBaseIterator&) const;

  SegmentBaseIterator(SegmentBaseIterator&&) = delete;
  SegmentBaseIterator& operator=(const SegmentBaseIterator&) = delete;
  SegmentBaseIterator& operator=(SegmentBaseIterator&&) = delete;
  SegmentBaseIterator(const SegmentBaseIterator&) = default;

 protected:
  bool EqualsTo(const SegmentBaseIterator&) const override;

 private:

  const SegmentBaseSequence* sequence_;
  uint32_t current_index_;
};

inline bool operator!=(const SegmentBaseIterator& lhs,
                       const SegmentBaseIterator& rhs) {
  return !(lhs == rhs);
}

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DASH_SEGMENT_BASE_SEQUENCE_H_
