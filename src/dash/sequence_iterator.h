/*!
 * sequence_iterator.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 */

#ifndef SRC_PLAYER_ES_DASH_PLAYER_DASH_SEQUENCE_ITERATOR_H_
#define SRC_PLAYER_ES_DASH_PLAYER_DASH_SEQUENCE_ITERATOR_H_

#include <memory>

#include "libdash/libdash.h"

#include "dash/media_segment_sequence.h"

class SegmentBaseIterator;
class SegmentTemplateIterator;
class SegmentListIterator;

class SequenceIterator {
 public:
  virtual ~SequenceIterator() = default;

  virtual std::unique_ptr<SequenceIterator> Clone() const = 0;
  virtual void NextSegment() = 0;
  virtual void PrevSegment() = 0;
  virtual std::unique_ptr<dash::mpd::ISegment> Get() const = 0;
  virtual bool Equals(const SequenceIterator&) const = 0;

  /// Checks given segment duration
  /// returns value < 0 in case of error (like invalid iterator).
  virtual double SegmentDuration(const MediaSegmentSequence*) const = 0;

  /// Checks given segment duration
  /// returns value < 0 in case of error (like invalid iterator).
  virtual double SegmentTimestamp(const MediaSegmentSequence*) const = 0;

  // Simple double dispatch for Equals as *Iterator class hierarchy will be
  // changed rarely as it's bounded with DASH spec.
  virtual bool EqualsTo(const SegmentBaseIterator&) const;
  virtual bool EqualsTo(const SegmentTemplateIterator&) const;
  virtual bool EqualsTo(const SegmentListIterator&) const;

 protected:
  SequenceIterator() = default;
  SequenceIterator(const SequenceIterator&) = default;
  SequenceIterator(SequenceIterator&&) = default;

  SequenceIterator& operator=(const SequenceIterator&) = default;
  SequenceIterator& operator=(SequenceIterator&&) = default;
};

#endif  // SRC_PLAYER_ES_DASH_PLAYER_DASH_SEQUENCE_ITERATOR_H_
