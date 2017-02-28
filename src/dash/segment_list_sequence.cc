/*!
 * segment_list_sequence.cc (https://github.com/SamsungDForum/NativePlayer)
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

#include "segment_list_sequence.h"

#include <cmath>
#include <limits>
#include <vector>

#include "util.h"

SegmentListSequence::SegmentListSequence(const RepresentationDescription& desc,
                                         uint32_t)
    : base_urls_(desc.base_urls),
      segment_list_(desc.segment_list),
      segment_duration_(0.0) {
  ExtractSegmentDuration();
}

SegmentListSequence::~SegmentListSequence() {}

MediaSegmentSequence::Iterator SegmentListSequence::Begin() const {
  return MakeIterator<SegmentListIterator>(this, 0);
}

MediaSegmentSequence::Iterator SegmentListSequence::End() const {
  auto& urls = segment_list_->GetSegmentURLs();
  return MakeIterator<SegmentListIterator>(this, urls.size());
}

MediaSegmentSequence::Iterator SegmentListSequence::MediaSegmentForTime(
    double time) const {
  if (time < 0 || segment_duration_ <= std::numeric_limits<double>::epsilon())
    return End();

  uint32_t index = static_cast<uint32_t>(floor(time / segment_duration_));
  if (index >= segment_list_->GetSegmentURLs().size()) return End();

  return MakeIterator<SegmentListIterator>(this, index);
}

std::unique_ptr<dash::mpd::ISegment> SegmentListSequence::GetInitSegment()
    const {
  const dash::mpd::IURLType* url = segment_list_->GetInitialization();
  if (!url) return {};

  return AdoptUnique(url->ToSegment(base_urls_));
}

std::unique_ptr<dash::mpd::ISegment>
SegmentListSequence::GetBitstreamSwitchingSegment() const {
  return {};
}

std::unique_ptr<dash::mpd::ISegment>
SegmentListSequence::GetRepresentationIndexSegment() const {
  return {};
}

std::unique_ptr<dash::mpd::ISegment> SegmentListSequence::GetIndexSegment()
    const {
  return {};
}

void SegmentListSequence::ExtractSegmentDuration() {
  if (!segment_list_) return;

  segment_duration_ = segment_list_->GetDuration();
  if (segment_list_->GetTimescale() > 0)
    segment_duration_ /= segment_list_->GetTimescale();
}

double SegmentListSequence::AverageSegmentDuration() const {
  return segment_duration_;
}

double SegmentListSequence::Timestamp(uint32_t index) const {
  if (segment_list_->GetSegmentTimeline()) {
    auto& timelines = segment_list_->GetSegmentTimeline()->GetTimelines();

    if (index < timelines.size()) {
      double timestamp = static_cast<double>(timelines[index]->GetStartTime());
      return segment_list_->GetTimescale() == 0
                 ? timestamp
                 : timestamp / segment_list_->GetTimescale();
    }
  }
  return segment_duration_ * index;
}

SegmentListIterator::SegmentListIterator()
    : sequence_(NULL), current_index_(0) {}

SegmentListIterator::SegmentListIterator(const SegmentListSequence* seq,
                                         uint32_t current_index)
    : sequence_(seq), current_index_(current_index) {}

std::unique_ptr<SequenceIterator> SegmentListIterator::Clone() const {
  return MakeUnique<SegmentListIterator>(*this);
}

void SegmentListIterator::NextSegment() { ++current_index_; }

void SegmentListIterator::PrevSegment() { --current_index_; }

std::unique_ptr<dash::mpd::ISegment> SegmentListIterator::Get() const {
  auto& urls = sequence_->segment_list_->GetSegmentURLs();
  if (current_index_ >= urls.size()) return {};

  return AdoptUnique(
      urls[current_index_]->ToMediaSegment(sequence_->base_urls_));
}

bool SegmentListIterator::Equals(const SequenceIterator& it) const {
  return it.EqualsTo(*this);
}

double SegmentListIterator::SegmentDuration(
    const MediaSegmentSequence* sequence) const {
  if (!sequence_ || sequence_ != sequence)
    return MediaSegmentSequence::kInvalidSegmentDuration;

  return sequence_->Duration(current_index_);
}

double SegmentListIterator::SegmentTimestamp(
    const MediaSegmentSequence* sequence) const {
  if (!sequence_ || sequence_ != sequence)
    return MediaSegmentSequence::kInvalidSegmentDuration;

  return sequence_->Timestamp(current_index_);
}

bool SegmentListIterator::operator==(const SegmentListIterator& rhs) const {
  return sequence_ == rhs.sequence_ && current_index_ == rhs.current_index_;
}

bool SegmentListIterator::EqualsTo(const SegmentListIterator& it) const {
  return *this == it;
}
