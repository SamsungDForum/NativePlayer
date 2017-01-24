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
  CalculateSegmentStartTimes();
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
  if (!segment_template_->GetSegmentTimeline()) {
    if (time < 0 || segment_duration_ <= std::numeric_limits<double>::epsilon())
      return End();

    auto index = static_cast<uint32_t>(floor(time / segment_duration_));
    index += start_index_;
    if (index >= end_index_) return End();

    return MakeIterator<SegmentTemplateIterator>(this, index);
  }
  for (uint32_t index = 0; index < segment_start_times_.size(); ++index) {
    auto start_time = double(segment_start_times_.at(index).start_time_
        / (segment_template_->GetTimescale() > 0
            ? segment_template_->GetTimescale()
            : 1));

    auto duration = double(segment_start_times_.at(index).duration_
        / (segment_template_->GetTimescale() > 0
            ? segment_template_->GetTimescale()
            : 1));
    if (start_time <= time && start_time + duration >= time) {
      index += start_index_;
      return MakeIterator<SegmentTemplateIterator>(this, index);
    }
  }
  return End();
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
  if (number < start_index_ || number > end_index_)
    return nullptr;

  dash::mpd::ISegment *segment = nullptr;
  if (segment_template_->GetSegmentTimeline()) {
    number -= start_index_;  // passed index is not always zero-based

    if (number < segment_start_times_.size()) {
      auto start_time = segment_start_times_.at(number).start_time_;
      segment = segment_template_->GetMediaSegmentFromTime(
          base_urls_, rep_id_, bandwidth_, start_time);
    }
  } else {
    segment = segment_template_->GetMediaSegmentFromNumber(
        base_urls_, rep_id_, bandwidth_, number);
  }
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
    if (index < segment_start_times_.size()) {
      auto start_time = segment_start_times_.at(index).start_time_;
      return segment_template_->GetTimescale() == 0
          ? start_time
          : start_time / segment_template_->GetTimescale();
    }
    return kInvalidSegmentTimestamp;
  }
  return segment_duration_ * index;
}

double SegmentTemplateSequence::Duration(uint32_t index) const {
  if (!segment_template_->GetSegmentTimeline())
    return segment_duration_;

  if (index < start_index_ || index > end_index_)
    return kInvalidSegmentDuration;

  index -= start_index_;  // passed index is not always zero-based

  // invalid duration for last segment
  if (index < segment_start_times_.size()) {
    auto duration = segment_start_times_.at(index).duration_;
    return segment_template_->GetTimescale() == 0
        ? duration
        : duration / segment_template_->GetTimescale();
  }
  return kInvalidSegmentDuration;
}

void SegmentTemplateSequence::CalculateSegmentStartTimes() {
  if (!segment_template_->GetSegmentTimeline())
    return;

  uint64_t end_time = 0;
  auto& timelines = segment_template_->GetSegmentTimeline()->GetTimelines();
  for (const auto &timeline : timelines) {
    uint64_t start_time = timeline->GetStartTime();
    uint64_t duration = timeline->GetDuration();
    uint64_t repeat = timeline->GetRepeatCount();

    if (start_time == 0)
      start_time = end_time;

    for (uint64_t j = 0; j <= repeat; ++j)
      segment_start_times_.emplace_back(SegmentTimes(start_time + duration * j,
                                        duration));
    end_time = segment_start_times_.back().start_time_ + duration;
  }
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
