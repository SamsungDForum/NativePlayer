/*!
 * async_data_provider.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 */

#include "async_data_provider.h"

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
    // Pass an empty MediaSegment as an end of stream signal.
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

  return true;
}

void AsyncDataProvider::SetMediaSegmentSequence(
    std::unique_ptr<MediaSegmentSequence> sequence) {
  AutoLock lock(iterator_lock_);
  sequence_ = std::move(sequence);
  next_segment_iterator_ = sequence_->Begin();
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
  LOG_DEBUG("");
  auto seg = MakeUnique<MediaSegment>();

  // arbitrary additional buffer space if segments size varies a little
  seg->data_.reserve(last_segment_size_ + last_segment_size_ / 32);
  seg->duration_ = sequence_->SegmentDuration(segment_iterator);
  seg->timestamp_ = sequence_->SegmentTimestamp(segment_iterator);

  DownloadSegment(*segment_iterator, &(seg->data_));
  last_segment_size_ = seg->data_.size();

  destination_message_loop.PostWork(cc_factory_.NewCallback(
      &AsyncDataProvider::PassResultOnCallerThread, seg.release()));
  LOG_DEBUG("Finishing");
}

void AsyncDataProvider::PassResultOnCallerThread(int32_t,
                                                 MediaSegment* segment) {
  LOG_DEBUG("");
  data_segment_callback_(AdoptUnique(segment));
  LOG_DEBUG("Finishing");
}

