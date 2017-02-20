#include "player.h"

#include <common/file_system.h>

#include "video_state.h"

namespace {
typedef common::file_system::ascii_string_path file_path;
bool ReadPlaylistFromFile(const file_path& location,
                          std::vector<common::uri::Uri>* urls = nullptr) {
  if (!location.isValid()) {
    return false;
  }

  common::file_system::File pl(location);
  if (!pl.open("r")) {
    return false;
  }

  std::vector<common::uri::Uri> lurls;
  std::string path;
  while (!pl.isEof() && pl.readLine(&path)) {
    common::uri::Uri ur(path);
    if (ur.isValid()) {
      lurls.push_back(ur);
    }
  }

  if (urls) {
    *urls = lurls;
  }
  pl.close();
  return true;
}
}

Player::Player(const common::uri::Uri& play_list_location,
               core::AppOptions* opt,
               core::ComplexOptions* copt)
    : opt_(opt), copt_(copt), play_list_(), stream_() {
  ChangePlayListLocation(play_list_location);
}

int Player::Exec() {
  if (play_list_.empty()) {
    return EXIT_FAILURE;
  }

  stream_ = common::make_scoped<VideoState>(play_list_[0], opt_, copt_);
  return stream_->Exec();
}

Player::~Player() {}

bool Player::ChangePlayListLocation(const common::uri::Uri& location) {
  if (location.scheme() == common::uri::Uri::file) {
    common::uri::Upath up = location.path();
    file_path apath(up.path());
    return ReadPlaylistFromFile(apath, &play_list_);
  }

  return false;
}
