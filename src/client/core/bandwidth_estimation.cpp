#include "client/core/bandwidth_estimation.h"

#define KBITS_TO_BYTES(X) (X * 1024 / 8)

#define PROFILE_H264_BASELINE 66                                           // cbr
#define PROFILE_H264_CONSTRAINED_BASELINE (66 | PROFILE_H264_CONSTRAINED)  // cbr
#define PROFILE_H264_MAIN 77
#define PROFILE_H264_EXTENDED 88
#define PROFILE_H264_HIGH 100

#define PROFILE_AAC_MAIN 0  // cbr
#define PROFILE_AAC_LOW 1
#define PROFILE_AAC_SSR 2
#define PROFILE_AAC_LTP 3
#define PROFILE_AAC_HE 4

#define KOEF_BASE_TO_HIGHT 4 / 3

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

DesireBytesPerSec CalculateDesireAACBandwidthBytesPerSec(int channels) {
  // https://toolstud.io/video/audiosize.php
  if (channels == 1) {
    return DesireBytesPerSec(KBITS_TO_BYTES(64), KBITS_TO_BYTES(128));
  } else if (channels == 2) {
    return DesireBytesPerSec(KBITS_TO_BYTES(128), KBITS_TO_BYTES(256));
  } else if (channels == 4) {
    return DesireBytesPerSec(KBITS_TO_BYTES(256), KBITS_TO_BYTES(384));
  } else if (channels == 6) {
    return DesireBytesPerSec(KBITS_TO_BYTES(384), KBITS_TO_BYTES(512));
  }

  DNOTREACHED();
  return DesireBytesPerSec(KBITS_TO_BYTES(0), KBITS_TO_BYTES(0));
}

DesireBytesPerSec CalculateDesireH264BandwidthBytesPerSec(Size encoded_frame_size,
                                                          double framerate,
                                                          int profile) {
  // https://support.google.com/youtube/answer/2853702?hl=en
  // https://support.ustream.tv/hc/en-us/articles/207852117-Internet-connection-and-recommended-encoding-settings
  // https://support.google.com/youtube/answer/1722171?hl=en
  if (profile < PROFILE_H264_MAIN) {
    if (framerate <= 30.0) {
      if (encoded_frame_size.width >= 3840 &&
          encoded_frame_size.height >= 2160) {  // 2160p (4k) 40000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(30000), KBITS_TO_BYTES(50000));
      } else if (encoded_frame_size.width >= 2560 &&
                 encoded_frame_size.height >= 1440) {  // 1440p (2k) 16000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(12000), KBITS_TO_BYTES(20000));
      } else if (encoded_frame_size.width >= 1920 &&
                 encoded_frame_size.height >= 1080) {  // 1080p 8000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(6000), KBITS_TO_BYTES(10000));
      } else if (encoded_frame_size.width >= 1280 &&
                 encoded_frame_size.height >= 720) {  // 720p 5000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(3000), KBITS_TO_BYTES(7000));
      } else if (encoded_frame_size.width >= 854 &&
                 encoded_frame_size.height >= 480) {  // 480p 2500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(2000), KBITS_TO_BYTES(3000));
      } else if (encoded_frame_size.width >= 640 &&
                 encoded_frame_size.height >= 360) {  // 360p 1000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(650), KBITS_TO_BYTES(1200));
      } else if (encoded_frame_size.width >= 426 &&
                 encoded_frame_size.height >= 240) {  // 240p 600 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(450), KBITS_TO_BYTES(800));
      }
    } else {
      if (encoded_frame_size.width >= 3840 &&
          encoded_frame_size.height >= 2160) {  // 2160p 60000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(45000), KBITS_TO_BYTES(75000));
      } else if (encoded_frame_size.width >= 2560 &&
                 encoded_frame_size.height >= 1440) {  // 1440p 24000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(18000), KBITS_TO_BYTES(30000));
      } else if (encoded_frame_size.width >= 1920 &&
                 encoded_frame_size.height >= 1080) {  // 1080p 12000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(9000), KBITS_TO_BYTES(15000));
      } else if (encoded_frame_size.width >= 1280 &&
                 encoded_frame_size.height >= 720) {  // 720p 7500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(6000), KBITS_TO_BYTES(9000));
      } else if (encoded_frame_size.width >= 854 &&
                 encoded_frame_size.height >= 480) {  // 480p 4000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(3000), KBITS_TO_BYTES(5000));
      } else if (encoded_frame_size.width >= 640 &&
                 encoded_frame_size.height >= 360) {  // 360p 1500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(1200), KBITS_TO_BYTES(1800));
      } else if (encoded_frame_size.width >= 426 &&
                 encoded_frame_size.height >= 240) {  // 240p 900 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(700), KBITS_TO_BYTES(1100));
      }
    }
  } else if (profile <= PROFILE_H264_HIGH) {
    if (framerate <= 30.0) {
      if (encoded_frame_size.width >= 3840 &&
          encoded_frame_size.height >= 2160) {  // 2160p (4k) 55000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(44000), KBITS_TO_BYTES(56000));
      } else if (encoded_frame_size.width >= 2560 &&
                 encoded_frame_size.height >= 1440) {  // 1440p (2k) 20000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(15000), KBITS_TO_BYTES(25000));
      } else if (encoded_frame_size.width >= 1920 &&
                 encoded_frame_size.height >= 1080) {  // 1080p 10000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(8000), KBITS_TO_BYTES(12000));
      } else if (encoded_frame_size.width >= 1280 &&
                 encoded_frame_size.height >= 720) {  // 720p 6500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(5000), KBITS_TO_BYTES(8000));
      } else if (encoded_frame_size.width >= 854 &&
                 encoded_frame_size.height >= 480) {  // 480p 4500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(3300), KBITS_TO_BYTES(5700));
      } else if (encoded_frame_size.width >= 640 &&
                 encoded_frame_size.height >= 360) {  // 360p 2700 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(1900), KBITS_TO_BYTES(3500));
      } else if (encoded_frame_size.width >= 426 &&
                 encoded_frame_size.height >= 240) {  // 240p 1500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(1000), KBITS_TO_BYTES(2000));
      }
    } else {
      if (encoded_frame_size.width >= 3840 &&
          encoded_frame_size.height >= 2160) {  // 2160p 75500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(66000), KBITS_TO_BYTES(85000));
      } else if (encoded_frame_size.width >= 2560 &&
                 encoded_frame_size.height >= 1440) {  // 1440p 30000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(23000), KBITS_TO_BYTES(37000));
      } else if (encoded_frame_size.width >= 1920 &&
                 encoded_frame_size.height >= 1080) {  // 1080p 15000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(12000), KBITS_TO_BYTES(18000));
      } else if (encoded_frame_size.width >= 1280 &&
                 encoded_frame_size.height >= 720) {  // 720p 9500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(7000), KBITS_TO_BYTES(12000));
      } else if (encoded_frame_size.width >= 854 &&
                 encoded_frame_size.height >= 480) {  // 480p 6000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(5000), KBITS_TO_BYTES(7000));
      } else if (encoded_frame_size.width >= 640 &&
                 encoded_frame_size.height >= 360) {  // 360p 3200 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(2400), KBITS_TO_BYTES(3800));
      } else if (encoded_frame_size.width >= 426 &&
                 encoded_frame_size.height >= 240) {  // 240p 2000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(1500), KBITS_TO_BYTES(2500));
      }
    }
  }

  DNOTREACHED();
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

  DNOTREACHED();
  return DesireBytesPerSec(KBITS_TO_BYTES(0), KBITS_TO_BYTES(0));
}

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
