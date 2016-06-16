/*!
 * sequence_iterator.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 */

#include "sequence_iterator.h"

bool SequenceIterator::EqualsTo(const SegmentBaseIterator&) const {
  return false;
}

bool SequenceIterator::EqualsTo(const SegmentTemplateIterator&) const {
  return false;
}

bool SequenceIterator::EqualsTo(const SegmentListIterator&) const {
  return false;
}
