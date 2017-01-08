#include "core/types.h"

extern "C" {
#include <libavutil/channel_layout.h>
}

int64_t get_valid_channel_layout(int64_t channel_layout, int channels) {
  if (channel_layout && av_get_channel_layout_nb_channels(channel_layout) == channels)
    return channel_layout;
  else
    return 0;
}
