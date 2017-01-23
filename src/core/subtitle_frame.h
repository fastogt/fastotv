#pragma once

#include "ffmpeg_config.h"

extern "C" {
#include <libavcodec/avcodec.h>
}

#include <common/macros.h>

namespace core {

struct SubtitleFrame {
  SubtitleFrame();
  ~SubtitleFrame();

  AVSubtitle sub;
  int serial;
  double pts; /* presentation timestamp for the frame */
  int width;
  int height;
  int uploaded;

  void ClearFrame();

 private:
  DISALLOW_COPY_AND_ASSIGN(SubtitleFrame);
};

}  // namespace core
