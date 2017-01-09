/*!
 * media_segment_sequence.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 */

#include <sstream>

#include "ppapi/c/pp_errors.h"

#include "dash/media_segment_sequence.h"

#include "segment_base_sequence.h"
#include "segment_list_sequence.h"
#include "segment_template_sequence.h"
#include "sequence_iterator.h"
#include "util.h"

MediaSegmentSequence::~MediaSegmentSequence() {}

double MediaSegmentSequence::SegmentDuration(const Iterator& it) const {
  return it.SegmentDuration(this);
}

double MediaSegmentSequence::SegmentTimestamp(const Iterator& it) const {
  return it.SegmentTimestamp(this);
}

MediaSegmentSequence::Iterator::Iterator() : pimpl_() {}

MediaSegmentSequence::Iterator::Iterator(
    std::unique_ptr<SequenceIterator>&& sequence_iterator)
    : pimpl_(std::move(sequence_iterator)) {}

MediaSegmentSequence::Iterator::Iterator(
    const MediaSegmentSequence::Iterator& rhs)
    : pimpl_(rhs.pimpl_ ? rhs.pimpl_->Clone() : nullptr) {}

MediaSegmentSequence::Iterator::~Iterator() {}

MediaSegmentSequence::Iterator& MediaSegmentSequence::Iterator::operator=(
    const MediaSegmentSequence::Iterator& rhs) {
  if (this == &rhs) return *this;

  if (rhs.pimpl_)
    pimpl_ = std::move(rhs.pimpl_->Clone());
  else
    pimpl_.reset();

  return *this;
}

MediaSegmentSequence::Iterator& MediaSegmentSequence::Iterator::operator=(
    MediaSegmentSequence::Iterator&& other) {
  pimpl_ = std::move(other.pimpl_);

  return *this;
}

MediaSegmentSequence::Iterator& MediaSegmentSequence::Iterator::operator++() {
  if (pimpl_) pimpl_->NextSegment();

  return *this;
}

MediaSegmentSequence::Iterator MediaSegmentSequence::Iterator::operator++(
    int /* dummy */) {
  MediaSegmentSequence::Iterator old(*this);
  ++(*this);
  return old;
}

MediaSegmentSequence::Iterator& MediaSegmentSequence::Iterator::operator--() {
  if (pimpl_) pimpl_->PrevSegment();

  return *this;
}

MediaSegmentSequence::Iterator MediaSegmentSequence::Iterator::operator--(
    int /* dummy */) {
  MediaSegmentSequence::Iterator old(*this);
  --(*this);
  return old;
}

bool MediaSegmentSequence::Iterator::operator==(
    const MediaSegmentSequence::Iterator& rhs) const {
  if (!pimpl_ || !rhs.pimpl_) return false;

  return pimpl_->Equals(*(rhs.pimpl_));
}

bool MediaSegmentSequence::Iterator::operator!=(
    const MediaSegmentSequence::Iterator& rhs) const {
  if (!pimpl_ || !rhs.pimpl_) return true;

  return !pimpl_->Equals(*(rhs.pimpl_));
}

std::unique_ptr<dash::mpd::ISegment> MediaSegmentSequence::Iterator::operator*()
    const {
  return pimpl_->Get();
}

double MediaSegmentSequence::Iterator::SegmentDuration(
    const MediaSegmentSequence* ptr) const {
  if (!pimpl_)
    return kInvalidSegmentDuration;
  return pimpl_->SegmentDuration(ptr);
}

double MediaSegmentSequence::Iterator::SegmentTimestamp(
    const MediaSegmentSequence* ptr) const {
  if (!pimpl_)
    return kInvalidSegmentTimestamp;
  return pimpl_->SegmentTimestamp(ptr);
}


bool DownloadSegment(dash::mpd::ISegment* seg, std::vector<uint8_t>* data) {
  if (!seg || !data) return false;

  dash::network::IChunk* chunk = static_cast<dash::network::IChunk*>(seg);
  // Quick fix for wrongly parsed MPDs
  // Got url in following form:
  //    "http://dash.akamaized.net/dash264/TestCasesMCA/dolby/1/1/"
  //    "http://dash.akamaized.net/dash264/TestCasesMCA/dolby/1/1/"
  //    "ChID_voices_51_256_ddp_A.mp4"
  // and thus got 404 error.
  std::string url = chunk->AbsoluteURI();
  auto first_match = url.find("://");
  auto last_match = url.rfind("://");
  if (first_match != last_match)
    url.erase(url.begin() + first_match, url.begin() + last_match);
  LOG_INFO("Downloading segment: %s%s%s", url.c_str(),
           chunk->HasByteRange() ? " Range: " : "",
           chunk->HasByteRange() ? chunk->Range().c_str() : "");

  auto request = GetRequestForURL(url);
  if (chunk->HasByteRange()) {
    std::ostringstream oss;
    oss << "Range: bytes=" << chunk->Range();
    request.SetProperty(PP_URLREQUESTPROPERTY_HEADERS, oss.str());
  }

  int32_t error_code = ProcessURLRequestOnSideThread(request, data);
  if (error_code != PP_OK) {
    LOG_ERROR("Segment download failed: %d", error_code);
    return false;
  }

  return true;
}
