#pragma once

#include <stddef.h>

#include "client_server_types.h"

#include "client/types.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

struct DesireBytesPerSec {
  DesireBytesPerSec();
  DesireBytesPerSec(bandwidth_t min, bandwidth_t max);

  bool IsValid() const;
  bool InRange(bandwidth_t val) const;

  DesireBytesPerSec& operator+=(const DesireBytesPerSec& other);

  bandwidth_t min;
  bandwidth_t max;
};

inline DesireBytesPerSec operator+(const DesireBytesPerSec& left, const DesireBytesPerSec& right) {
  DesireBytesPerSec tmp = left;
  tmp += right;
  return tmp;
}

DesireBytesPerSec CalculateDesireAudioBandwidthBytesPerSec(int rate, int channels);  // raw
DesireBytesPerSec CalculateDesireAACBandwidthBytesPerSec(int channels);

DesireBytesPerSec CalculateDesireH264BandwidthBytesPerSec(Size encoded_frame_size,
                                                          double framerate,
                                                          int profile);
DesireBytesPerSec CalculateDesireMPEGBandwidthBytesPerSec(Size encoded_frame_size);

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
