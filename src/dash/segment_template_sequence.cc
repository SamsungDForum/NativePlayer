/*!
 * segment_template_sequence.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 */

#include "segment_template_sequence.h"

#include <cmath>
#include <limits>

#include "util.h"

SegmentTemplateSequence::SegmentTemplateSequence(
    const RepresentationDescription& desc, uint32_t bandwidth)
    : base_urls_(desc.base_urls),
      rep_id_(desc.representation_id),
      segment_template_(desc.segment_template),
      bandwidth_(bandwidth),
      start_index_(0),
      end_index_(std::numeric_limits<uint32_t>::max()),
      segment_duration_(MediaSegmentSequence::kInvalidSegmentDuration) {
  ExtractSegmentDuration();
  ExtractStartIndex();
  // FIXME compute end_index_, but it might require access to period!!!
}

SegmentTemplateSequence::~SegmentTemplateSequence() {}

MediaSegmentSequence::Iterator SegmentTemplateSequence::Begin() const {
  return MakeIterator<SegmentTemplateIterator>(this, start_index_);
}

MediaSegmentSequence::Iterator SegmentTemplateSequence::End() const {
  return MakeIterator<SegmentTemplateIterator>(this, end_index_);
}

MediaSegmentSequence::Iterator SegmentTemplateSequence::MediaSegmentForTime(
    double time) const {
  if (time < 0 || segment_duration_ <= std::numeric_limits<double>::epsilon())
    return End();

  uint32_t index = static_cast<uint32_t>(floor(time / segment_duration_));
  index += start_index_;
  if (index >= end_index_) return End();

  return MakeIterator<SegmentTemplateIterator>(this, index);
}

std::unique_ptr<dash::mpd::ISegment> SegmentTemplateSequence::GetInitSegment()
    const {
  auto segment = segment_template_->ToInitializationSegment(base_urls_, rep_id_,
                                                            bandwidth_);
  return AdoptUnique(segment);
}

std::unique_ptr<dash::mpd::ISegment>
SegmentTemplateSequence::GetBitstreamSwitchingSegment() const {
  auto segment = segment_template_->ToBitstreamSwitchingSegment(
      base_urls_, rep_id_, bandwidth_);
  return AdoptUnique(segment);
}

std::unique_ptr<dash::mpd::ISegment>
SegmentTemplateSequence::GetRepresentationIndexSegment() const {
  return {};
}

std::unique_ptr<dash::mpd::ISegment> SegmentTemplateSequence::GetIndexSegment()
    const {
  return {};
}

double SegmentTemplateSequence::AverageSegmentDuration() const {
  return segment_duration_;
}

std::unique_ptr<dash::mpd::ISegment>
SegmentTemplateSequence::GetMediaSegmentFromNumber(uint32_t number) const {
  auto segment = segment_template_->GetMediaSegmentFromNumber(
      base_urls_, rep_id_, bandwidth_, number);
  return AdoptUnique(segment);
}

void SegmentTemplateSequence::ExtractSegmentDuration() {
  if (!segment_template_) return;

  segment_duration_ = segment_template_->GetDuration();
  if (segment_template_->GetTimescale() > 0)
    segment_duration_ /= segment_template_->GetTimescale();
}

void SegmentTemplateSequence::ExtractStartIndex() {
  if (!segment_template_) return;

  start_index_ = segment_template_->GetStartNumber();
}

double SegmentTemplateSequence::Timestamp(uint32_t index) const {
  if (index < start_index_ || index > end_index_)
    return kInvalidSegmentTimestamp;

  index -= start_index_;  // passed index is not always zero-based

  if (segment_template_->GetSegmentTimeline()) {
    std::vector<dash::mpd::ITimeline*>& timelines =
        segment_template_->GetSegmentTimeline()->GetTimelines();

    if (index < timelines.size()) {
      double timestamp = static_cast<double>(timelines[index]->GetStartTime());
      return segment_template_->GetTimescale() == 0
                 ? timestamp
                 : timestamp / segment_template_->GetTimescale();
    }
  }
  return segment_duration_ * index;
}

SegmentTemplateIterator::SegmentTemplateIterator()
    : sequence_(nullptr), current_index_(0) {}

SegmentTemplateIterator::SegmentTemplateIterator(
    const SegmentTemplateSequence* seq, uint32_t current_index)
    : sequence_(seq), current_index_(current_index) {}

std::unique_ptr<SequenceIterator> SegmentTemplateIterator::Clone() const {
  return MakeUnique<SegmentTemplateIterator>(*this);
}

void SegmentTemplateIterator::NextSegment() { ++current_index_; }

void SegmentTemplateIterator::PrevSegment() { --current_index_; }

std::unique_ptr<dash::mpd::ISegment> SegmentTemplateIterator::Get() const {
  return sequence_->GetMediaSegmentFromNumber(current_index_);
}

bool SegmentTemplateIterator::Equals(const SequenceIterator& it) const {
  return it.EqualsTo(*this);
}

double SegmentTemplateIterator::SegmentDuration(
    const MediaSegmentSequence* sequence) const {
  if (!sequence_ || sequence_ != sequence)
    return MediaSegmentSequence::kInvalidSegmentDuration;

  return sequence_->Duration(current_index_);
}

double SegmentTemplateIterator::SegmentTimestamp(
    const MediaSegmentSequence* sequence) const {
  if (!sequence_ || sequence_ != sequence)
    return MediaSegmentSequence::kInvalidSegmentTimestamp;

  return sequence_->Timestamp(current_index_);
}

bool SegmentTemplateIterator::operator==(
    const SegmentTemplateIterator& rhs) const {
  return sequence_ == rhs.sequence_ && current_index_ == rhs.current_index_;
}

bool SegmentTemplateIterator::EqualsTo(
    const SegmentTemplateIterator& it) const {
  return *this == it;
}
