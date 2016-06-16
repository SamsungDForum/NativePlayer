/*!
 * util.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * @author Adam Bujalski
 * @author Tomasz Borkowski
 */

#include <string>
#include <vector>

#include "util.h"
#include "segment_base_sequence.h"
#include "segment_list_sequence.h"
#include "segment_template_sequence.h"

namespace {

template <typename T>
std::unique_ptr<MediaSegmentSequence> MakeSequence(
    const RepresentationDescription& representation, uint32_t bandwidth) {
  return std::unique_ptr<MediaSegmentSequence>{
      new T(representation, bandwidth)};
}
}

RepresentationDescription MakeEmptyRepresentation() {
  RepresentationDescription representation;

  representation.segment_base = nullptr;
  representation.segment_list = nullptr;
  representation.segment_template = nullptr;

  return representation;
}

std::unique_ptr<MediaSegmentSequence> CreateSequence(
    const RepresentationDescription& representation, uint32_t bandwidth) {
  if (representation.segment_base)
    return MakeSequence<SegmentBaseSequence>(representation, bandwidth);

  if (representation.segment_list)
    return MakeSequence<SegmentListSequence>(representation, bandwidth);

  if (representation.segment_template)
    return MakeSequence<SegmentTemplateSequence>(representation, bandwidth);

  return {};
}

double ParseDurationToSeconds(const std::string& duration_str) {
  // We don't support negative duration, years and months.
  if (duration_str.empty() || duration_str[0] != 'P')  // 'P' is obligatory.
    return kInvalidDuration;

  double duration_in_seconds = 0;
  uint32_t digits_in_a_row = 0;
  const double kSecondsInMinute = 60;
  const double kSecondsInHour = 60 * kSecondsInMinute;
  const double kSecondsInDay = 24 * kSecondsInHour;

  bool time_flag_occured = false;

  // begin from 1, because first character has been checked already
  for (uint32_t i = 1; i < duration_str.length(); ++i) {
    if (isdigit(duration_str[i]) || duration_str[i] == '.') {
      ++digits_in_a_row;
      continue;
    }

    double multiplier = 0;
    switch (duration_str[i]) {
      // we don't support years and months
      case 'D':
        multiplier = kSecondsInDay;
        break;
      case 'T':
        time_flag_occured = true;
        break;
      case 'H':
        multiplier = kSecondsInHour;
        break;
      case 'M':  // 'M' means months or if there was a 'T' flag - minutes
        if (time_flag_occured)
          multiplier = kSecondsInMinute;
        else
          return kInvalidDuration;
        break;
      case 'S':
        multiplier = 1;
        break;
      default:
        return kInvalidDuration;
    }

    if (digits_in_a_row > 0) {
      double value = std::atof(
          duration_str.substr(i - digits_in_a_row, digits_in_a_row).c_str());

      duration_in_seconds += value * multiplier;
      digits_in_a_row = 0;
    }
  }

  return duration_in_seconds;
}
