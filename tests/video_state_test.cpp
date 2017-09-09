#include "client/player/media/video_state.h"

int main(int argc, char** argv) {
  using namespace fastotv::client::player;
  media::VideoState* vs =
      new media::VideoState(fastotv::stream_id(), common::uri::Url(), media::AppOptions(), media::ComplexOptions());
  delete vs;
  return 0;
}
