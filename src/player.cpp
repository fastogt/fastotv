#include "player.h"

#include <SDL2/SDL_audio.h>  // for SDL_MIX_MAXVOLUME, etc
#include <SDL2/SDL_hints.h>
#include <SDL2/SDL_events.h>  // for SDL_Event, SDL_PushEvent, etc

extern "C" {
#include <libavutil/time.h>
}

#include <common/file_system.h>

#include "video_state.h"

#include "core/app_options.h"

/* Step size for volume control */
#define SDL_VOLUME_STEP (SDL_MIX_MAXVOLUME / 50)
#define CURSOR_HIDE_DELAY 1000000

namespace {
typedef common::file_system::ascii_string_path file_path;
bool ReadPlaylistFromFile(const file_path& location,
                          std::vector<Url>* urls = nullptr) {
  if (!location.isValid()) {
    return false;
  }

  common::file_system::File pl(location);
  if (!pl.open("r")) {
    return false;
  }

  std::vector<Url> lurls;
  std::string path;
  while (!pl.isEof() && pl.readLine(&path)) {
    Url ur(path);
    if (ur.IsValid()) {
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

PlayerOptions::PlayerOptions()
    : exit_on_keydown(false),
      exit_on_mousedown(false),
      is_full_screen(false),
      seek_by_bytes(-1),
      window_title() {}

Player::Player(const PlayerOptions& options, core::AppOptions* opt, core::ComplexOptions* copt)
    : options_(options),
      opt_(opt),
      copt_(copt),
      play_list_(),
      stream_(),
      renderer_(NULL),
      window_(NULL),
      cursor_hidden_(false),
      cursor_last_shown_(0),
      last_mouse_left_click_(0) {
  ChangePlayListLocation(options.play_list_location);

  // stable options
  if (opt->startup_volume < 0) {
    WARNING_LOG() << "-volume=" << opt->startup_volume << " < 0, setting to 0";
  }
  if (opt->startup_volume > 100) {
    WARNING_LOG() << "-volume=" << opt->startup_volume << " > 100, setting to 100";
  }
  opt->startup_volume = av_clip(opt->startup_volume, 0, 100);
  opt->startup_volume =
      av_clip(SDL_MIX_MAXVOLUME * opt->startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
}

int Player::Exec() {
  if (play_list_.empty()) {
    return EXIT_FAILURE;
  }

  stream_ = common::make_scoped<VideoState>(play_list_[0].url(), opt_, copt_, this);
  int res = stream_->Exec();
  if (res == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  SDL_Event event;
  while (true) {
    {
      double remaining_time = 0.0;
      SDL_PumpEvents();
      while (!SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
        if (!cursor_hidden_ && av_gettime_relative() - cursor_last_shown_ > CURSOR_HIDE_DELAY) {
          SDL_ShowCursor(0);
          cursor_hidden_ = true;
        }
        if (remaining_time > 0.0) {
          const unsigned sleep_time = static_cast<unsigned>(remaining_time * 1000000.0);
          av_usleep(sleep_time);
        }
        remaining_time = REFRESH_RATE;
        stream_->TryRefreshVideo(&remaining_time);
        SDL_PumpEvents();
      }
    }
    switch (event.type) {
      case SDL_KEYDOWN: {
        if (options_.exit_on_keydown) {
          return EXIT_SUCCESS;
        }
        double incr = 0;
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
          case SDLK_q:
            return EXIT_SUCCESS;
          case SDLK_f: {
            bool full_screen = options_.is_full_screen = !options_.is_full_screen;
            stream_->SetFullScreen(full_screen);
            break;
          }
          case SDLK_p:
          case SDLK_SPACE:
            stream_->TogglePause();
            break;
          case SDLK_m:
            stream_->ToggleMute();
            break;
          case SDLK_KP_MULTIPLY:
          case SDLK_0:
            stream_->UpdateVolume(1, SDL_VOLUME_STEP);
            break;
          case SDLK_KP_DIVIDE:
          case SDLK_9:
            stream_->UpdateVolume(-1, SDL_VOLUME_STEP);
            break;
          case SDLK_s:  // S: Step to next frame
            stream_->StepToNextFrame();
            break;
          case SDLK_a:
            stream_->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
            break;
          case SDLK_v:
            stream_->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
            break;
          case SDLK_c:
            stream_->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
            stream_->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
            break;
          case SDLK_t:
            // StreamCycleChannel(AVMEDIA_TYPE_SUBTITLE);
            break;
          case SDLK_w: {
            stream_->ToggleWaveDisplay();
            break;
          }
          case SDLK_PAGEUP:
            stream_->MoveToNextFragment(0, options_.seek_by_bytes);
            break;
          case SDLK_PAGEDOWN:
            stream_->MoveToPreviousFragment(0, options_.seek_by_bytes);
            break;
          case SDLK_LEFT:
            incr = -10.0;
            goto do_seek;
          case SDLK_RIGHT:
            incr = 10.0;
            goto do_seek;
          case SDLK_UP:
            incr = 60.0;
            goto do_seek;
          case SDLK_DOWN:
            incr = -60.0;
          do_seek:
            stream_->StreemSeek(incr, options_.seek_by_bytes);
            break;
          default:
            break;
        }
        break;
      }
      case SDL_MOUSEBUTTONDOWN: {
        if (options_.exit_on_mousedown) {
          return EXIT_SUCCESS;
        }
        if (event.button.button == SDL_BUTTON_LEFT) {
          if (av_gettime_relative() - last_mouse_left_click_ <= 500000) {
            bool full_screen = options_.is_full_screen = !options_.is_full_screen;
            stream_->SetFullScreen(full_screen);
            last_mouse_left_click_ = 0;
          } else {
            last_mouse_left_click_ = av_gettime_relative();
          }
        }
      }
      case SDL_MOUSEMOTION: {
        if (cursor_hidden_) {
          SDL_ShowCursor(1);
          cursor_hidden_ = false;
        }
        double x;
        cursor_last_shown_ = av_gettime_relative();
        if (event.type == SDL_MOUSEBUTTONDOWN) {
          if (event.button.button != SDL_BUTTON_RIGHT) {
            break;
          }
          x = event.button.x;
        } else {
          if (!(event.motion.state & SDL_BUTTON_RMASK)) {
            break;
          }
          x = event.motion.x;
        }
        stream_->StreamSeekPos(x, options_.seek_by_bytes);
        break;
      }
      case SDL_WINDOWEVENT: {
        stream_->HandleWindowEvent(&event.window);
        break;
      }
      case SDL_QUIT:
      case FF_QUIT_EVENT: {
        return EXIT_SUCCESS;
      }
      case FF_ALLOC_EVENT: {
        int res = static_cast<VideoState*>(event.user.data1)->HandleAllocPictureEvent();
        if (res == ERROR_RESULT_VALUE) {
          return EXIT_FAILURE;
        }
        break;
      }
      default:
        break;
    }
  }
  return EXIT_SUCCESS;
}

Player::~Player() {
  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
    renderer_ = NULL;
  }
  if (window_) {
    SDL_DestroyWindow(window_);
    window_ = NULL;
  }
}

bool Player::RequestWindow(VideoState* stream,
                           int width,
                           int height,
                           SDL_Renderer** renderer,
                           SDL_Window** window) {
  if (!stream || !renderer || !window) {  // invalid input
    return false;
  }

  if (!window_) {
    Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    if (options_.is_full_screen) {
      flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    window_ = SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width,
                               height, flags);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    if (window_) {
      SDL_RendererInfo info;
      renderer_ =
          SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
      if (!renderer_) {
        WARNING_LOG() << "Failed to initialize a hardware accelerated renderer: " << SDL_GetError();
        renderer_ = SDL_CreateRenderer(window_, -1, 0);
      }
      if (renderer_) {
        if (!SDL_GetRendererInfo(renderer_, &info)) {
          DEBUG_LOG() << "Initialized " << info.name << " renderer.";
        }
      }
    }
  } else {
    SDL_SetWindowSize(window_, width, height);
  }

  if (!window_ || !renderer_) {
    ERROR_LOG() << "SDL: could not set video mode - exiting";
    return false;
  }

  if (options_.window_title.empty()) {
    const std::string uri_str = stream->uri().url();
    options_.window_title = uri_str;
  }

  SDL_SetWindowTitle(window_, options_.window_title.c_str());
  *renderer = renderer_;
  *window = window_;
  return true;
}

void Player::OnDiscoveryStream(VideoState* stream, AVFormatContext* context) {
  if (!stream || !context) {
    return;
  }

  if (options_.seek_by_bytes < 0) {
    options_.seek_by_bytes =
        !!(context->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", context->iformat->name);
  }
  AVDictionaryEntry* t = NULL;
  if (options_.window_title.empty() && (t = av_dict_get(context->metadata, "title", NULL, 0))) {
    options_.window_title = common::MemSPrintf("%s - %s", t->value, stream->uri().url());
  }
}

bool Player::ChangePlayListLocation(const common::uri::Uri& location) {
  if (location.scheme() == common::uri::Uri::file) {
    common::uri::Upath up = location.path();
    file_path apath(up.path());
    return ReadPlaylistFromFile(apath, &play_list_);
  }

  return false;
}
