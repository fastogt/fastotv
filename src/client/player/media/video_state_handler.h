/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <stdint.h>  // for uint8_t, int64_t, uint32_t

extern "C" {
#include <libavutil/rational.h>  // for AVRational
}

#include <common/error.h>  // for Error

namespace fastotv {
namespace client {
namespace player {
namespace media {

class VideoState;
struct AudioParams;
struct VideoFrame;

class VideoStateHandler {
 public:
  VideoStateHandler();
  virtual ~VideoStateHandler();

  // audio
  virtual common::Error HandleRequestAudio(VideoState* stream,
                                           int64_t wanted_channel_layout,
                                           int wanted_nb_channels,
                                           int wanted_sample_rate,
                                           AudioParams* audio_hw_params,
                                           int* audio_buff_size) WARN_UNUSED_RESULT = 0;  // init audio
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr,
                             const uint8_t* src,
                             uint32_t len,
                             int volume) = 0;  // change volume

  // video
  virtual common::Error HandleRequestVideo(VideoState* stream,
                                           int width,
                                           int height,
                                           int av_pixel_format,
                                           AVRational aspect_ratio) WARN_UNUSED_RESULT = 0;  // init video

  virtual void HandleFrameResize(VideoState* stream, int width, int height, int av_pixel_format, AVRational sar) = 0;
  virtual void HandleQuitStream(VideoState* stream, int exit_code, common::Error err) = 0;
};

}  // namespace media
}  // namespace player
}  // namespace client
}  // namespace fastotv
