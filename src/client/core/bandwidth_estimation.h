#pragma once

#include "client/types.h"
#include "client_server_types.h"  // for bandwidth_t

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
DesireBytesPerSec AudioBitrateAverage(bandwidth_t bytes_per_sec);
DesireBytesPerSec CalculateDesireAACBandwidthBytesPerSec(int channels);
DesireBytesPerSec CalculateDesireMP2BandwidthBytesPerSec(int channels);

DesireBytesPerSec VideoBitrateAverage(bandwidth_t bytes_per_sec);
DesireBytesPerSec CalculateDesireH264BandwidthBytesPerSec(Size encoded_frame_size, double framerate, int profile);
DesireBytesPerSec CalculateDesireMPEGBandwidthBytesPerSec(Size encoded_frame_size);

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
