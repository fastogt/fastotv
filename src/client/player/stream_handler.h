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

#include "client/player/core/video_state_handler.h"

namespace fastotv {
namespace client {
namespace player {

class StreamHandler : public core::VideoStateHandler {
 public:
  virtual ~StreamHandler();

  virtual common::Error HandleRequestAudio(core::VideoState* stream,
                                           int64_t wanted_channel_layout,
                                           int wanted_nb_channels,
                                           int wanted_sample_rate,
                                           core::AudioParams* audio_hw_params,
                                           int* audio_buff_size) override = 0;
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr, const uint8_t* src, uint32_t len, int volume) override = 0;

  virtual common::Error HandleRequestVideo(core::VideoState* stream,
                                           int width,
                                           int height,
                                           int av_pixel_format,
                                           AVRational aspect_ratio) override = 0;

 private:
  virtual void HandleFrameResize(core::VideoState* stream,
                                 int width,
                                 int height,
                                 int av_pixel_format,
                                 AVRational aspect_ratio) override final;
  virtual void HandleQuitStream(core::VideoState* stream, int exit_code, common::Error err) override final;
};

}  // namespace player
}  // namespace client
}  // namespace fastotv
