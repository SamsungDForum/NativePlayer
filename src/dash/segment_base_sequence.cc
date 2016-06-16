/*!
 * segment_base_sequence.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 */

#include <cassert>
#include <cstdlib>
#include <vector>
#include <sstream>
#include <string>

#include "segment_base_sequence.h"
#include "util.h"

namespace {

SegmentIndexEntry MakeEntry(double timestamp, double duration, uint64_t offset,
                            uint64_t size) {
  return {timestamp, duration, offset, size};
}

double ToSeconds(uint64_t pts, uint32_t timescale) {
  return static_cast<double>(pts) / static_cast<double>(timescale);
}

// TODO(samsung) raw pointer aritmethic grr....
template <typename T>
T NextUnsigned(const uint8_t*& stream) {
  T out = 0;
  for (size_t i = 0; i < sizeof(T); ++i) {
    out <<= 8;
    out |= *stream;
    ++stream;
  }

  return out;
}

template <>
uint8_t NextUnsigned<uint8_t>(const uint8_t*& stream) {
  return *stream++;
}

}  // namespace

SegmentBaseSequence::SegmentBaseSequence(const RepresentationDescription& desc,
                                         uint32_t)
    : base_urls_(desc.base_urls),
      segment_base_(desc.segment_base),
      average_segment_duration_(0.0) {
  LoadIndexSegment();
}

SegmentBaseSequence::~SegmentBaseSequence() {}

MediaSegmentSequence::Iterator SegmentBaseSequence::Begin() const {
  return MakeIterator<SegmentBaseIterator>(this, 0);
}

MediaSegmentSequence::Iterator SegmentBaseSequence::End() const {
  return MakeIterator<SegmentBaseIterator>(this, segment_index_.size());
}

MediaSegmentSequence::Iterator SegmentBaseSequence::MediaSegmentForTime(
    double time) const {
  for (uint32_t i = 0; i < segment_index_.size(); ++i) {
    const SegmentIndexEntry& e = segment_index_[i];
    if (e.timestamp <= time && time < e.timestamp + e.duration)
      return MakeIterator<SegmentBaseIterator>(this, i);
  }

  return End();
}

std::unique_ptr<dash::mpd::ISegment> SegmentBaseSequence::GetInitSegment()
    const {
  const dash::mpd::IURLType* url = segment_base_->GetInitialization();
  if (!url) return {};

  return AdoptUnique(url->ToSegment(base_urls_));
}

std::unique_ptr<dash::mpd::ISegment>
SegmentBaseSequence::GetBitstreamSwitchingSegment() const {
  return {};
}

std::unique_ptr<dash::mpd::ISegment>
SegmentBaseSequence::GetRepresentationIndexSegment() const {
  const dash::mpd::IURLType* url = segment_base_->GetRepresentationIndex();
  if (!url) return {};

  return AdoptUnique(url->ToSegment(base_urls_));
}

std::unique_ptr<dash::mpd::ISegment> SegmentBaseSequence::GetIndexSegment()
    const {
  if (segment_base_->GetIndexRange().empty()) return {};

  auto segment = GetInitSegment();
  if (!segment) return {};

  segment->Range(segment_base_->GetIndexRange());
  segment->HasByteRange(true);
  return segment;
}

double SegmentBaseSequence::AverageSegmentDuration() const {
  return average_segment_duration_;
}

void SegmentBaseSequence::ParseSidx(const std::vector<uint8_t>& sidx,
                                    uint64_t sidx_begin, uint64_t sidx_end) {
  // TODO(samsung) raw pointer aritmethic grr....
  const uint8_t* data = sidx.data();
  uint32_t sidx_size = NextUnsigned<uint32_t>(data);
  static_cast<void>(NextUnsigned<uint32_t>(data));  // FourCC
  uint8_t version = NextUnsigned<uint8_t>(data);
  for (int i = 0; i < 3; ++i)  // flags
    static_cast<void>(NextUnsigned<uint8_t>(data));
  static_cast<void>(NextUnsigned<uint32_t>(data));  // reference_id
  assert(sidx_end == sidx_begin + sidx_size - 1);

  uint32_t timescale = NextUnsigned<uint32_t>(data);
  uint64_t pts = 0;
  uint64_t offset = sidx_end + 1;

  if (version == 0) {
    pts += NextUnsigned<uint32_t>(data);
    offset += NextUnsigned<uint32_t>(data);
  } else {
    pts += NextUnsigned<uint64_t>(data);
    offset += NextUnsigned<uint64_t>(data);
  }

  segment_index_.clear();
  static_cast<void>(NextUnsigned<uint16_t>(data));  // reserved
  uint16_t reference_count = NextUnsigned<uint16_t>(data);

  average_segment_duration_ = 0.0;
  for (uint16_t i = 0; i < reference_count; ++i) {
    // TODO(samsung) include refereces to another sidx box
    uint32_t ref_size = NextUnsigned<uint32_t>(data);
    ref_size &= 0x7FFFFFFFu;

    uint32_t duration = NextUnsigned<uint32_t>(data);

    // TODO(samsung) Additional flags - currently igrnored
    static_cast<void>(NextUnsigned<uint32_t>(data));

    double segment_duration = ToSeconds(duration, timescale);
    average_segment_duration_ +=
        (segment_duration - average_segment_duration_) / (i + 1.0);

    SegmentIndexEntry entry = MakeEntry(ToSeconds(pts, timescale),
                                        segment_duration, offset, ref_size);
    segment_index_.push_back(entry);

    pts += duration;
    offset += ref_size;
  }
}

void SegmentBaseSequence::LoadIndexSegment() {
  using dash::mpd::ISegment;
  using dash::network::IChunk;
  auto segment = GetRepresentationIndexSegment();
  if (!segment) segment = std::move(GetIndexSegment());

  if (!segment) return;

  std::vector<uint8_t> data;
  DownloadSegment(segment.get(), &data);
  if (data.empty()) return;

  auto chunk = static_cast<IChunk*>(segment.get());
  std::string range = chunk->Range();
  size_t pos = range.find("-");
  if (pos == std::string::npos) return;

  uint32_t sidx_beg = atoi(range.substr(0, pos).c_str());
  uint32_t sidx_end = atoi(range.substr(pos + 1).c_str());
  ParseSidx(data, sidx_beg, sidx_end);
}

double SegmentBaseSequence::Duration(uint32_t segment) const {
  if (segment >= segment_index_.size())
    return MediaSegmentSequence::kInvalidSegmentDuration;

  return segment_index_[segment].duration;
}

double SegmentBaseSequence::Timestamp(uint32_t segment) const {
  if (segment >= segment_index_.size())
    return MediaSegmentSequence::kInvalidSegmentTimestamp;

  return segment_index_[segment].timestamp;
}

SegmentBaseIterator::SegmentBaseIterator()
    : sequence_(nullptr), current_index_(0) {}

SegmentBaseIterator::SegmentBaseIterator(const SegmentBaseSequence* seq,
                                         uint32_t current_index)
    : sequence_(seq), current_index_(current_index) {}

std::unique_ptr<SequenceIterator> SegmentBaseIterator::Clone() const {
  return MakeUnique<SegmentBaseIterator>(*this);
}

void SegmentBaseIterator::NextSegment() { ++current_index_; }

void SegmentBaseIterator::PrevSegment() { --current_index_; }

std::unique_ptr<dash::mpd::ISegment> SegmentBaseIterator::Get() const {
  if (current_index_ >= sequence_->segment_index_.size()) return {};

  auto& ent = sequence_->segment_index_[current_index_];
  std::ostringstream oss;
  oss << ent.byte_offset << "-" << (ent.byte_offset + ent.byte_size - 1);

  auto segment = sequence_->GetInitSegment();
  if (!segment) return {};

  segment->Range(oss.str());
  segment->HasByteRange(true);
  return segment;
}

bool SegmentBaseIterator::Equals(const SequenceIterator& it) const {
  return it.EqualsTo(*this);
}

double SegmentBaseIterator::SegmentDuration(
    const MediaSegmentSequence* sequence) const {
  if (!sequence_ || sequence_ != sequence)
    return MediaSegmentSequence::kInvalidSegmentDuration;

  return sequence_->Duration(current_index_);
}

double SegmentBaseIterator::SegmentTimestamp(
    const MediaSegmentSequence* sequence) const {
  if (!sequence_ || sequence_ != sequence)
    return MediaSegmentSequence::kInvalidSegmentTimestamp;

  return sequence_->Timestamp(current_index_);
}

bool SegmentBaseIterator::operator==(const SegmentBaseIterator& rhs) const {
  return sequence_ == rhs.sequence_ && current_index_ == rhs.current_index_;
}

bool SegmentBaseIterator::EqualsTo(const SegmentBaseIterator& it) const {
  return *this == it;
}
