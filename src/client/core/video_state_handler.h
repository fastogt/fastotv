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

extern "C" {
#include <libavutil/rational.h>
}

#include "client/core/events/events_base.h"
#include "client/types.h"

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

struct AudioParams;
class VideoState;
struct VideoFrame;

class VideoStateHandler : public core::events::EventListener {
 public:
  VideoStateHandler();
  virtual void HandleEvent(event_t* event) override = 0;
  virtual void HandleExceptionEvent(event_t* event, common::Error err) override = 0;

  virtual ~VideoStateHandler();

  // audio
  virtual bool HandleRequestAudio(VideoState* stream,
                                  int64_t wanted_channel_layout,
                                  int wanted_nb_channels,
                                  int wanted_sample_rate,
                                  core::AudioParams* audio_hw_params,
                                  int* audio_buff_size) = 0;
  virtual void HanleAudioMix(uint8_t* audio_stream_ptr,
                             const uint8_t* src,
                             uint32_t len,
                             int volume) = 0;

  // video
  virtual bool HandleRequestVideo(VideoState* stream) = 0;
  virtual bool HandleRealocFrame(VideoState* stream, core::VideoFrame* frame) = 0;
  virtual void HanleDisplayFrame(VideoState* stream, const core::VideoFrame* frame) = 0;
  virtual void HandleDefaultWindowSize(Size frame_size, AVRational sar) = 0;
};
}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
