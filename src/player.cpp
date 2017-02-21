#include "player.h"

#include <SDL2/SDL_audio.h>  // for SDL_MIX_MAXVOLUME, etc
#include <SDL2/SDL_hints.h>
#include <SDL2/SDL_events.h>  // for SDL_Event, SDL_PushEvent, etc

extern "C" {
#include <libavutil/time.h>
}

#include "third-party/json-c/json-c/json.h"  // for json_object_...

#include <common/file_system.h>

#include "video_state.h"

#include "core/app_options.h"

/* Step size for volume control */
#define SDL_VOLUME_STEP (SDL_MIX_MAXVOLUME / 50)
#define CURSOR_HIDE_DELAY 1000000

#define USER_FIELD "user"
#define URLS_FIELD "urls"

namespace {
typedef common::file_system::ascii_string_path file_path;
bool ReadPlaylistFromFile(const file_path& location, std::vector<Url>* urls = nullptr) {
  if (!location.isValid()) {
    return false;
  }

  common::file_system::File pl(location);
  if (!pl.open("r")) {
    return false;
  }

  std::string line;
  std::string full_config;
  while (!pl.isEof() && pl.readLine(&line)) {
    full_config += line;
  }

  json_object* obj = json_tokener_parse(full_config.c_str());
  if (!obj) {
    ERROR_LOG() << "Invalid config file: " << location.path();
    pl.close();
    return false;
  }

  json_object* juser = NULL;
  json_bool juser_exists = json_object_object_get_ex(obj, USER_FIELD, &juser);
  if (!juser_exists) {
    ERROR_LOG() << "Invalid config(user field) file: " << location.path();
    json_object_put(obj);
    pl.close();
    return false;
  }

  json_object* jurls = NULL;
  json_bool jurls_exists = json_object_object_get_ex(obj, URLS_FIELD, &jurls);
  if (!jurls_exists) {
    ERROR_LOG() << "Invalid config(urls field) file: " << location.path();
    json_object_put(obj);
    pl.close();
    return false;
  }

  int array_len = json_object_array_length(jurls);
  std::vector<Url> lurls;
  for (int i = 0; i < array_len; ++i) {
    json_object* jpath = json_object_array_get_idx(jurls, i);
    const std::string path = json_object_get_string(jpath);
    Url ur(path);
    if (ur.IsValid()) {
      lurls.push_back(ur);
    }
  }

  if (urls) {
    *urls = lurls;
  }
  json_object_put(obj);
  pl.close();
  return true;
}
}

PlayerOptions::PlayerOptions()
    : exit_on_keydown(false), exit_on_mousedown(false), is_full_screen(false) {}

Player::Player(const PlayerOptions& options, core::AppOptions* opt, core::ComplexOptions* copt)
    : options_(options),
      opt_(opt),
      copt_(copt),
      play_list_(),
      renderer_(NULL),
      window_(NULL),
      cursor_hidden_(false),
      cursor_last_shown_(0),
      last_mouse_left_click_(0),
      curent_stream_pos_(0) {
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
  common::scoped_ptr<VideoState> stream = CreateNextStream();
  if (!stream) {
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
        stream->TryRefreshVideo(&remaining_time);
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
            stream->SetFullScreen(full_screen);
            break;
          }
          case SDLK_p:
          case SDLK_SPACE:
            stream->TogglePause();
            break;
          case SDLK_m:
            stream->ToggleMute();
            break;
          case SDLK_KP_MULTIPLY:
          case SDLK_0:
            stream->UpdateVolume(1, SDL_VOLUME_STEP);
            opt_->startup_volume = stream->Volume();
            break;
          case SDLK_KP_DIVIDE:
          case SDLK_9:
            stream->UpdateVolume(-1, SDL_VOLUME_STEP);
            opt_->startup_volume = stream->Volume();
            break;
          case SDLK_s:  // S: Step to next frame
            stream->StepToNextFrame();
            break;
          case SDLK_a:
            stream->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
            break;
          case SDLK_v:
            stream->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
            break;
          case SDLK_c:
            stream->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
            stream->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
            break;
          case SDLK_t:
            // StreamCycleChannel(AVMEDIA_TYPE_SUBTITLE);
            break;
          case SDLK_w: {
            stream->ToggleWaveDisplay();
            break;
          }
          case SDLK_PAGEUP:
            stream->MoveToNextFragment(0);
            break;
          case SDLK_PAGEDOWN:
            stream->MoveToPreviousFragment(0);
            break;
          case SDLK_LEFTBRACKET:  // change channel
            stream = CreatePrevStream();
            if (!stream) {
              return EXIT_FAILURE;
            }
            break;
          case SDLK_RIGHTBRACKET:  // change channel
            stream = CreateNextStream();
            if (!stream) {
              return EXIT_FAILURE;
            }
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
            stream->StreemSeek(incr);
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
            stream->SetFullScreen(full_screen);
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
        stream->StreamSeekPos(x);
        break;
      }
      case SDL_WINDOWEVENT: {
        if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
          opt_->screen_width = event.window.data1;
          opt_->screen_height = event.window.data2;
        }
        stream->HandleWindowEvent(&event.window);
        break;
      }
      case SDL_QUIT: {
        return EXIT_SUCCESS;
      }
      case FF_QUIT_EVENT: {  // stream want to quit
        /*stream = CreateNextStream();
        if (!stream) {
          return EXIT_FAILURE;
        }*/
        break;
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

  std::string name;
  for (size_t i = 0; i < play_list_.size(); ++i) {
    if (play_list_[i].Id() == stream->Id()) {
      name = play_list_[i].Name();
      break;
    }
  }

  SDL_SetWindowTitle(window_, name.c_str());
  *renderer = renderer_;
  *window = window_;
  return true;
}

bool Player::ChangePlayListLocation(const common::uri::Uri& location) {
  if (location.scheme() == common::uri::Uri::file) {
    common::uri::Upath up = location.path();
    file_path apath(up.path());
    return ReadPlaylistFromFile(apath, &play_list_);
  }

  return false;
}

common::scoped_ptr<VideoState> Player::CreateNextStream() {
  if (play_list_.empty()) {
    return common::scoped_ptr<VideoState>();
  }

  if (curent_stream_pos_ == play_list_.size()) {
    curent_stream_pos_ = 0;
  }
  Url url = play_list_[curent_stream_pos_++];
  common::scoped_ptr<VideoState> stream =
      common::make_scoped<VideoState>(url.Id(), url.GetUrl(), *opt_, copt_, this);

  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    return common::scoped_ptr<VideoState>();
  }

  return stream;
}

common::scoped_ptr<VideoState> Player::CreatePrevStream() {
  if (play_list_.empty()) {
    return common::scoped_ptr<VideoState>();
  }

  if (curent_stream_pos_ == 0) {
    curent_stream_pos_ = play_list_.size();
  }
  Url url = play_list_[--curent_stream_pos_];
  common::scoped_ptr<VideoState> stream =
      common::make_scoped<VideoState>(url.Id(), url.GetUrl(), *opt_, copt_, this);

  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    return common::scoped_ptr<VideoState>();
  }

  return stream;
}
