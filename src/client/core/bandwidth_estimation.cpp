#include "client/core/bandwidth_estimation.h"

#include <common/logger.h>  // for COMPACT_LOG_FILE_CRIT
#include <common/macros.h>  // for DNOTREACHED

#include "client/types.h"  // for Size

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

DesireBytesPerSec CalculateDesireMP2BandwidthBytesPerSec(int channels) {
  if (channels == 1) {
    return DesireBytesPerSec(KBITS_TO_BYTES(72), KBITS_TO_BYTES(128));
  } else if (channels == 2) {
    return DesireBytesPerSec(KBITS_TO_BYTES(144), KBITS_TO_BYTES(256));
  } else if (channels == 4) {
    return DesireBytesPerSec(KBITS_TO_BYTES(288), KBITS_TO_BYTES(512));
  } else if (channels == 6) {
    return DesireBytesPerSec(KBITS_TO_BYTES(432), KBITS_TO_BYTES(768));
  }

  DNOTREACHED();
  return DesireBytesPerSec(KBITS_TO_BYTES(0), KBITS_TO_BYTES(0));
}

DesireBytesPerSec AudioBitrateAverage(bandwidth_t bytes_per_sec) {
  return DesireBytesPerSec(bytes_per_sec * 3 / 4, bytes_per_sec * 4 / 3);
}

DesireBytesPerSec CalculateDesireAACBandwidthBytesPerSec(int channels) {
  // https://toolstud.io/video/audiosize.php
  if (channels == 1) {
    return DesireBytesPerSec(KBITS_TO_BYTES(32), KBITS_TO_BYTES(96));
  } else if (channels == 2) {
    return DesireBytesPerSec(KBITS_TO_BYTES(64), KBITS_TO_BYTES(192));
  } else if (channels == 4) {
    return DesireBytesPerSec(KBITS_TO_BYTES(128), KBITS_TO_BYTES(384));
  } else if (channels == 6) {
    return DesireBytesPerSec(KBITS_TO_BYTES(256), KBITS_TO_BYTES(512));
  }

  DNOTREACHED();
  return DesireBytesPerSec(KBITS_TO_BYTES(0), KBITS_TO_BYTES(0));
}

DesireBytesPerSec VideoBitrateAverage(bandwidth_t bytes_per_sec) {
  return DesireBytesPerSec(bytes_per_sec * 0.5, bytes_per_sec * 1.5);
}

DesireBytesPerSec CalculateDesireH264BandwidthBytesPerSec(Size encoded_frame_size, double framerate, int profile) {
  // https://support.google.com/youtube/answer/2853702?hl=en
  // https://support.ustream.tv/hc/en-us/articles/207852117-Internet-connection-and-recommended-encoding-settings
  // https://support.google.com/youtube/answer/1722171?hl=en
  int stabled_profile = std::max(PROFILE_H264_BASELINE, std::min(PROFILE_H264_HIGH, profile));
  if (stabled_profile < PROFILE_H264_MAIN) {
    if (framerate <= 30.0) {
      if (encoded_frame_size.width >= 3840 && encoded_frame_size.height >= 2160) {  // 2160p (4k) 40000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(13000), KBITS_TO_BYTES(40000));
      } else if (encoded_frame_size.width >= 2560 && encoded_frame_size.height >= 1440) {  // 1440p (2k) 16000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(6000), KBITS_TO_BYTES(16000));
      } else if (encoded_frame_size.width >= 1920 && encoded_frame_size.height >= 1080) {  // 1080p 8000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(3000), KBITS_TO_BYTES(8000));
      } else if (encoded_frame_size.width >= 1280 && encoded_frame_size.height >= 720) {  // 720p 5000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(1500), KBITS_TO_BYTES(5000));
      } else if (encoded_frame_size.width >= 854 && encoded_frame_size.height >= 480) {  // 480p 2500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(500), KBITS_TO_BYTES(2500));
      } else if (encoded_frame_size.width >= 640 && encoded_frame_size.height >= 360) {  // 360p 1000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(400), KBITS_TO_BYTES(1000));
      } else if (encoded_frame_size.width >= 426 && encoded_frame_size.height >= 240) {  // 240p 600 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(300), KBITS_TO_BYTES(700));
      }
    } else {
      if (encoded_frame_size.width >= 3840 && encoded_frame_size.height >= 2160) {  // 2160p 60000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(20000), KBITS_TO_BYTES(60000));
      } else if (encoded_frame_size.width >= 2560 && encoded_frame_size.height >= 1440) {  // 1440p 24000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(9000), KBITS_TO_BYTES(24000));
      } else if (encoded_frame_size.width >= 1920 && encoded_frame_size.height >= 1080) {  // 1080p 12000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(4500), KBITS_TO_BYTES(12000));
      } else if (encoded_frame_size.width >= 1280 && encoded_frame_size.height >= 720) {  // 720p 7500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(2250), KBITS_TO_BYTES(7500));
      } else if (encoded_frame_size.width >= 854 && encoded_frame_size.height >= 480) {  // 480p 4000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(1000), KBITS_TO_BYTES(4000));
      } else if (encoded_frame_size.width >= 640 && encoded_frame_size.height >= 360) {  // 360p 1500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(600), KBITS_TO_BYTES(1500));
      } else if (encoded_frame_size.width >= 426 && encoded_frame_size.height >= 240) {  // 240p 900 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(400), KBITS_TO_BYTES(900));
      }
    }
  } else if (stabled_profile <= PROFILE_H264_HIGH) {
    if (framerate <= 30.0) {
      if (encoded_frame_size.width >= 3840 && encoded_frame_size.height >= 2160) {  // 2160p (4k) 40000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(13000) * KOEF_BASE_TO_HIGHT,
                                 KBITS_TO_BYTES(40000) * KOEF_BASE_TO_HIGHT);
      } else if (encoded_frame_size.width >= 2560 && encoded_frame_size.height >= 1440) {  // 1440p (2k) 16000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(6000) * KOEF_BASE_TO_HIGHT, KBITS_TO_BYTES(16000) * KOEF_BASE_TO_HIGHT);
      } else if (encoded_frame_size.width >= 1920 && encoded_frame_size.height >= 1080) {  // 1080p 12000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(3000), KBITS_TO_BYTES(12000));
      } else if (encoded_frame_size.width >= 1280 && encoded_frame_size.height >= 720) {  // 720p 6000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(1000), KBITS_TO_BYTES(6000));
      } else if (encoded_frame_size.width >= 854 && encoded_frame_size.height >= 480) {  // 480p 2500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(500) * KOEF_BASE_TO_HIGHT, KBITS_TO_BYTES(2500) * KOEF_BASE_TO_HIGHT);
      } else if (encoded_frame_size.width >= 640 && encoded_frame_size.height >= 360) {  // 360p 2500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(600), KBITS_TO_BYTES(2500));
      } else if (encoded_frame_size.width >= 426 && encoded_frame_size.height >= 240) {  // 240p 600 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(300) * KOEF_BASE_TO_HIGHT, KBITS_TO_BYTES(700) * KOEF_BASE_TO_HIGHT);
      }
    } else {
      if (encoded_frame_size.width >= 3840 && encoded_frame_size.height >= 2160) {  // 2160p 60000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(20000) * KOEF_BASE_TO_HIGHT,
                                 KBITS_TO_BYTES(60000) * KOEF_BASE_TO_HIGHT);
      } else if (encoded_frame_size.width >= 2560 && encoded_frame_size.height >= 1440) {  // 1440p 24000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(9000) * KOEF_BASE_TO_HIGHT, KBITS_TO_BYTES(24000) * KOEF_BASE_TO_HIGHT);
      } else if (encoded_frame_size.width >= 1920 && encoded_frame_size.height >= 1080) {  // 1080p 12000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(4500) * KOEF_BASE_TO_HIGHT, KBITS_TO_BYTES(12000) * KOEF_BASE_TO_HIGHT);
      } else if (encoded_frame_size.width >= 1280 && encoded_frame_size.height >= 720) {  // 720p 7500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(2250) * KOEF_BASE_TO_HIGHT, KBITS_TO_BYTES(7500) * KOEF_BASE_TO_HIGHT);
      } else if (encoded_frame_size.width >= 854 && encoded_frame_size.height >= 480) {  // 480p 4000 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(1000) * KOEF_BASE_TO_HIGHT, KBITS_TO_BYTES(4000) * KOEF_BASE_TO_HIGHT);
      } else if (encoded_frame_size.width >= 640 && encoded_frame_size.height >= 360) {  // 360p 1500 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(600) * KOEF_BASE_TO_HIGHT, KBITS_TO_BYTES(1500) * KOEF_BASE_TO_HIGHT);
      } else if (encoded_frame_size.width >= 426 && encoded_frame_size.height >= 240) {  // 240p 900 Kbps
        return DesireBytesPerSec(KBITS_TO_BYTES(400) * KOEF_BASE_TO_HIGHT, KBITS_TO_BYTES(900) * KOEF_BASE_TO_HIGHT);
      }
    }
  }

  WARNING_LOG() << "Size: " << encoded_frame_size.width << "x" << encoded_frame_size.height
                << ", framerate: " << framerate << ", profile: " << profile;
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

  DNOTREACHED() << "Size: " << encoded_frame_size.width << "x" << encoded_frame_size.height;
  return DesireBytesPerSec(KBITS_TO_BYTES(0), KBITS_TO_BYTES(0));
}

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
