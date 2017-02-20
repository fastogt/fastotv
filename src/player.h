#pragma once

#include <common/smart_ptr.h>
#include <common/url.h>

namespace core {
struct AppOptions;
}
namespace core {
struct ComplexOptions;
}

class VideoState;

class Player {
 public:
  Player(const common::uri::Uri& play_list_location,
         core::AppOptions* opt,
         core::ComplexOptions* copt);
  int Exec() WARN_UNUSED_RESULT;
  ~Player();

 private:
  bool ChangePlayListLocation(const common::uri::Uri& location);

  core::AppOptions* opt_;
  core::ComplexOptions* copt_;
  std::vector<common::uri::Uri> play_list_;
  common::scoped_ptr<VideoState> stream_;
};
