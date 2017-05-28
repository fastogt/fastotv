#include "client/core/bandwidth_estimation.h"

#define KBITS_TO_BYTES(X) (X * 1024 / 8)

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

DesireBytesPerSec::DesireBytesPerSec() : min(0), max(0) {}
DesireBytesPerSec::DesireBytesPerSec(bandwidth_t min, bandwidth_t max) : min(min), max(max) {}

bool DesireBytesPerSec::IsValid() const {
  if (min == 0 && max == 0) {
    return false;
  }
  return min <= max;
}

bool DesireBytesPerSec::InRange(bandwidth_t val) const {
  if (!IsValid()) {
    return false;
  }

  return min <= val && val <= max;
}

DesireBytesPerSec& DesireBytesPerSec::operator+=(const DesireBytesPerSec& other) {
  min += other.min;
  max += other.max;
  return *this;
}

DesireBytesPerSec CalculateDesireAudioBandwidthBytesPerSec(int rate, int channels) {
  int min_bitrate_kbps = rate * channels / 1000;
  return DesireBytesPerSec(KBITS_TO_BYTES(min_bitrate_kbps), KBITS_TO_BYTES(min_bitrate_kbps * 2));
}

DesireBytesPerSec CalculateDesireX264BandwidthBytesPerSec(Size encoded_frame_size, int framerate) {
  // https://support.google.com/youtube/answer/2853702?hl=en
  // https://support.ustream.tv/hc/en-us/articles/207852117-Internet-connection-and-recommended-encoding-settings
  if (framerate <= 30) {
    if (encoded_frame_size.width >= 3840 && encoded_frame_size.height >= 2160) {
      return DesireBytesPerSec(KBITS_TO_BYTES(13000), KBITS_TO_BYTES(34000));
    } else if (encoded_frame_size.width >= 2560 && encoded_frame_size.height >= 1440) {
      return DesireBytesPerSec(KBITS_TO_BYTES(6000), KBITS_TO_BYTES(13000));
    } else if (encoded_frame_size.width >= 1920 && encoded_frame_size.height >= 1080) {
      return DesireBytesPerSec(KBITS_TO_BYTES(3000), KBITS_TO_BYTES(6000));
    } else if (encoded_frame_size.width >= 1280 && encoded_frame_size.height >= 720) {
      return DesireBytesPerSec(KBITS_TO_BYTES(1200), KBITS_TO_BYTES(4000));
    } else if (encoded_frame_size.width >= 854 && encoded_frame_size.height >= 480) {
      return DesireBytesPerSec(KBITS_TO_BYTES(500), KBITS_TO_BYTES(2000));
    } else if (encoded_frame_size.width >= 640 && encoded_frame_size.height >= 360) {
      return DesireBytesPerSec(KBITS_TO_BYTES(400), KBITS_TO_BYTES(1200));
    } else if (encoded_frame_size.width >= 426 && encoded_frame_size.height >= 240) {
      return DesireBytesPerSec(KBITS_TO_BYTES(300), KBITS_TO_BYTES(700));
    }
  } else {
    if (encoded_frame_size.width >= 3840 && encoded_frame_size.height >= 2160) {
      return DesireBytesPerSec(KBITS_TO_BYTES(20000), KBITS_TO_BYTES(51000));
    } else if (encoded_frame_size.width >= 2560 && encoded_frame_size.height >= 1440) {
      return DesireBytesPerSec(KBITS_TO_BYTES(9000), KBITS_TO_BYTES(18000));
    } else if (encoded_frame_size.width >= 1920 && encoded_frame_size.height >= 1080) {
      return DesireBytesPerSec(KBITS_TO_BYTES(4500), KBITS_TO_BYTES(9000));
    } else if (encoded_frame_size.width >= 1280 && encoded_frame_size.height >= 720) {
      return DesireBytesPerSec(KBITS_TO_BYTES(2250), KBITS_TO_BYTES(6000));
    } else if (encoded_frame_size.width >= 854 && encoded_frame_size.height >= 480) {
      return DesireBytesPerSec(KBITS_TO_BYTES(750), KBITS_TO_BYTES(3000));
    } else if (encoded_frame_size.width >= 640 && encoded_frame_size.height >= 360) {
      return DesireBytesPerSec(KBITS_TO_BYTES(600), KBITS_TO_BYTES(1500));
    } else if (encoded_frame_size.width >= 426 && encoded_frame_size.height >= 240) {
      return DesireBytesPerSec(KBITS_TO_BYTES(450), KBITS_TO_BYTES(1050));
    }
  }

  return DesireBytesPerSec(KBITS_TO_BYTES(0), KBITS_TO_BYTES(0));
}

DesireBytesPerSec CalculateDesireMPEGBandwidthBytesPerSec(Size encoded_frame_size) {
  // https://en.wikipedia.org/wiki/H.262/MPEG-2_Part_2#Video_profiles_and_levels
  // http://www.iem.thm.de/telekom-labor/zinke/mk/mpeg2beg/beginnzi.htm#Profiles and Levels
  if (encoded_frame_size.width <= 352 && encoded_frame_size.height <= 288) {
    return DesireBytesPerSec(KBITS_TO_BYTES(2000), KBITS_TO_BYTES(4000));
  } else if (encoded_frame_size.width <= 720 && encoded_frame_size.height <= 576) {
    return DesireBytesPerSec(KBITS_TO_BYTES(7500), KBITS_TO_BYTES(15000));
  } else if (encoded_frame_size.width <= 1440 && encoded_frame_size.height <= 1152) {
    return DesireBytesPerSec(KBITS_TO_BYTES(30000), KBITS_TO_BYTES(60000));
  } else if (encoded_frame_size.width <= 1920 && encoded_frame_size.height <= 1152) {
    return DesireBytesPerSec(KBITS_TO_BYTES(40000), KBITS_TO_BYTES(80000));
  }

  return DesireBytesPerSec(KBITS_TO_BYTES(0), KBITS_TO_BYTES(0));
}

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
