/*!
 * media_segment.h (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Tomasz Borkowski
 */

#ifndef NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_MEDIA_SEGMENT_H_
#define NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_MEDIA_SEGMENT_H_

#include <vector>

struct MediaSegment {
  std::vector<uint8_t> data_;
  double duration_;
  double timestamp_;

  MediaSegment() : data_(), duration_(0.0), timestamp_(0.0) {}
};

#endif  // NATIVE_PLAYER_SRC_PLAYER_ES_DASH_PLAYER_MEDIA_SEGMENT_H_
