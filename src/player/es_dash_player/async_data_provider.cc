/*!
 * async_data_provider.cc (https://github.com/SamsungDForum/NativePlayer)
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
 * @author Tomasz Borkowski
 */

#include "async_data_provider.h"

#include <cmath>
#include <chrono>
#include <string>

#include "libdash/libdash.h"

#include "common.h"
#include "dash/media_segment_sequence.h"

#include "media_segment.h"

using pp::AutoLock;
using pp::MessageLoop;
using std::unique_ptr;
using std::vector;

namespace {
const uint32_t kDefaultSegmentSize = 32 * 1024;
}

AsyncDataProvider::AsyncDataProvider(
    const pp::InstanceHandle& instance,
    std::function<void(std::unique_ptr<MediaSegment>)> callback)
    : own_thread_(instance),
      next_segment_iterator_(),
      iterator_lock_(),
      cc_factory_(this),
      last_segment_size_(kDefaultSegmentSize),
      data_segment_callback_(callback) {
  own_thread_.Start();
}

bool AsyncDataProvider::RequestNextDataSegment() {
  LOG_DEBUG("Requesting next data segment");
  AutoLock lock(iterator_lock_);
  if (next_segment_iterator_ == sequence_->End()) {
    LOG_DEBUG("Pass an empty MediaSegment as an end of stream signal.");
    data_segment_callback_(MakeUnique<MediaSegment>());
    return false;
  }

  MessageLoop destination_message_loop = MessageLoop::GetCurrent();
  if (destination_message_loop.is_null()) {
    LOG_ERROR("Unable to dispatch next data segment on current MessageLoop!");
    return false;
  }

  int32_t result = own_thread_.message_loop().PostWork(cc_factory_.NewCallback(
      &AsyncDataProvider::DownloadNextSegmentOnOwnThread,
      next_segment_iterator_++, destination_message_loop));

  if (result != PP_OK) {
    return false;
  }

  LOG_DEBUG("Finishing");
  return true;
}

bool AsyncDataProvider::SetNextSegmentToTime(double time) {
  AutoLock lock(iterator_lock_);
  if (!sequence_) {
    return false;
  }
  next_segment_iterator_ = sequence_->MediaSegmentForTime(time);
  if (next_segment_iterator_ == sequence_->End()) {
    LOG_ERROR("Can't find segment for time: %f", time);
    return false;
  }

  return true;
}

Samsung::NaClPlayer::TimeTicks AsyncDataProvider::GetClosestKeyframeTime(
    Samsung::NaClPlayer::TimeTicks time) {
  constexpr Samsung::NaClPlayer::TimeTicks kSeekMargin = 0.1;
  auto segment = sequence_->MediaSegmentForTime(time);
  if (segment == sequence_->End())
    return 0.;
  // Calculates a next segment:
  auto next_segment = segment;
  ++next_segment;
  bool has_next_segment = (next_segment != sequence_->End());
  // Calculates a closest keyframe time:
  auto segment_start = segment.SegmentTimestamp(sequence_.get());
  if (!has_next_segment)
    return segment_start;
  auto next_segment_start = next_segment.SegmentTimestamp(sequence_.get());
  if (time - segment_start < next_segment_start - time)
    return segment_start + kSeekMargin;
  return next_segment_start + kSeekMargin;
}

void AsyncDataProvider::SetMediaSegmentSequence(
    std::unique_ptr<MediaSegmentSequence> sequence, double time) {
  AutoLock lock(iterator_lock_);
  sequence_ = std::move(sequence);
  if (fabs(time) < kEps) {
    next_segment_iterator_ = sequence_->Begin();
  } else {
    next_segment_iterator_ = sequence_->MediaSegmentForTime(time);
  }
}

double AsyncDataProvider::AverageSegmentDuration() {
  if (!sequence_) {
    return 0.0;
  }
  return sequence_->AverageSegmentDuration();
}

bool AsyncDataProvider::GetInitSegment(std::vector<uint8_t>* buffer) {
  return DownloadSegment(sequence_->GetInitSegment(), buffer);
}

void AsyncDataProvider::DownloadNextSegmentOnOwnThread(
    int32_t, MediaSegmentSequence::Iterator segment_iterator,
    MessageLoop destination_message_loop) {
  using std::chrono::steady_clock;
  using std::chrono::duration_cast;
  using std::chrono::duration;

  auto segment_duration = sequence_->SegmentDuration(segment_iterator);
  auto segment_timestamp = sequence_->SegmentTimestamp(segment_iterator);
  auto st = steady_clock::now();
  LOG_DEBUG("Starting download for a segment: %f [s] ... %f [s]",
      segment_timestamp, segment_timestamp + segment_duration);
  auto seg = MakeUnique<MediaSegment>();

  // arbitrary additional buffer space if segments size varies a little
  seg->data_.reserve(last_segment_size_ + last_segment_size_ / 32);
  seg->duration_ = segment_duration;
  seg->timestamp_ = segment_timestamp;

  auto segment = *segment_iterator;
  dash::network::IChunk* chunk =
      static_cast<dash::network::IChunk*>(segment.get());
  std::string url = chunk->AbsoluteURI();
  if (chunk->HasByteRange())
    url += " Range: " + chunk->Range();
  if (!DownloadSegment(std::move(segment), &(seg->data_))) {
    LOG_DEBUG("Download of a segment: %f [s] ... %f [s] was interrupted.",
        segment_timestamp, segment_timestamp + segment_duration);
    return;
  }
  last_segment_size_ = seg->data_.size();

  size_t seg_data_size = seg->data_.size();
  destination_message_loop.PostWork(cc_factory_.NewCallback(
      &AsyncDataProvider::PassResultOnCallerThread, seg.release()));

  auto et = steady_clock::now();
  duration<double> d = et - st;
  LOG_DEBUG("Finished download of a segment: %f [s] ... %f [s]",
      segment_timestamp, segment_timestamp + segment_duration);

  LOG_DEBUG("download time: %.4f segment duration: %.4f data size: %u url: %s",
        d.count(), segment_duration, seg_data_size,
        url.c_str());
}

void AsyncDataProvider::PassResultOnCallerThread(int32_t,
                                                 MediaSegment* segment) {
  LOG_DEBUG("");
  data_segment_callback_(AdoptUnique(segment));
  LOG_DEBUG("Finishing");
}

