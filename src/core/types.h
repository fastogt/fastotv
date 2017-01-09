#pragma once

#include <stdint.h>

enum ShowMode {
  SHOW_MODE_NONE = -1,
  SHOW_MODE_VIDEO = 0,
  SHOW_MODE_WAVES,
  SHOW_MODE_RDFT,
  SHOW_MODE_NB
};

int64_t get_valid_channel_layout(int64_t channel_layout, int channels);
