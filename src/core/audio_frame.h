#pragma once

extern "C" {
#include <libavutil/frame.h>
}

#include <common/macros.h>

struct AudioFrame {
  AudioFrame();
  ~AudioFrame();

  AVFrame* frame;
  int serial;
  double pts;      /* presentation timestamp for the frame */
  double duration; /* estimated duration of the frame */
  int64_t pos;     /* byte position of the frame in the input file */

  void ClearFrame();

 private:
  DISALLOW_COPY_AND_ASSIGN(AudioFrame);
};
