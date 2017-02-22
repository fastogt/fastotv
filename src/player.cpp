#include "player.h"

#include <SDL2/SDL_audio.h>  // for SDL_MIX_MAXVOLUME, etc
#include <SDL2/SDL_hints.h>

extern "C" {
#include <libavutil/time.h>
#include "png/png_reader.h"
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

#define IMG_PATH "offline.png"

namespace {
typedef common::file_system::ascii_string_path file_path;
bool ReadPlaylistFromFile(const file_path& location, std::vector<Url>* urls = nullptr) {
  if (!location.isValid()) {
    ERROR_LOG() << "Invalid config file: empty location!";
    return false;
  }

  common::file_system::File pl(location);
  if (!pl.open("r")) {
    ERROR_LOG() << "Can't open config file: " << location.path();
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

bool CreateWindow(int width,
                  int height,
                  bool is_full_screen,
                  const std::string& title,
                  SDL_Renderer** renderer,
                  SDL_Window** window) {
  if (!renderer || !window) {  // invalid input
    return false;
  }

  Uint32 flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
  if (is_full_screen) {
    flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
  }
  SDL_Window* lwindow = SDL_CreateWindow(NULL, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                         width, height, flags);
  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
  SDL_Renderer* lrenderer = NULL;
  if (lwindow) {
    SDL_RendererInfo info;
    lrenderer =
        SDL_CreateRenderer(lwindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!lrenderer) {
      WARNING_LOG() << "Failed to initialize a hardware accelerated renderer: " << SDL_GetError();
      lrenderer = SDL_CreateRenderer(lwindow, -1, 0);
    }
    if (lrenderer) {
      if (!SDL_GetRendererInfo(lrenderer, &info)) {
        DEBUG_LOG() << "Initialized " << info.name << " renderer.";
      }
    }
  }

  if (!lwindow || !lrenderer) {
    ERROR_LOG() << "SDL: could not set video mode - exiting";
    return false;
  }

  SDL_SetWindowSize(lwindow, width, height);
  SDL_SetWindowTitle(lwindow, title.c_str());

  *window = lwindow;
  *renderer = lrenderer;
  return true;
}
}  // namespace

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
      curent_stream_pos_(0),
      stop_(false),
      stream_() {
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
  struct PngReader* png_reader = alloc_png_reader();
  if (!png_reader) {
    WARNING_LOG() << "Can't alloc png reader!";
    return EXIT_FAILURE;
  }

  int res = read_png_file(png_reader, IMG_PATH);
  if (res == ERROR_RESULT_VALUE) {
    free_png_reader(&png_reader);
    WARNING_LOG() << "Can't load stub image, path: " << IMG_PATH;
    return EXIT_FAILURE;
  }

  int img_width = png_img_width(png_reader);
  int img_height = png_img_height(png_reader);
  int num_channels = png_img_channels(png_reader);
  int bit_depth = png_img_bit_depth(png_reader);
  int pitch = png_img_row_bytes(png_reader);
  // free_png_reader(&png_reader);

  stream_ = CreateNextStream();
  if (!stream_) {
    SwitchToErrorMode();
  }

  SDL_Event event;
  while (!stop_) {
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
        if (stream_) {
          stream_->TryRefreshVideo(&remaining_time);
        } else {
          Uint32 Rmask = 0;
          Uint32 Gmask = 0;
          Uint32 Bmask = 0;
          Uint32 Amask = 0;
          if (num_channels >= 3) {
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
            Rmask = 0x000000FF;
            Gmask = 0x0000FF00;
            Bmask = 0x00FF0000;
            Amask = (num_channels == 4) ? 0xFF000000 : 0;
#else
            int s = (num_channels == 4) ? 0 : 8;
            Rmask = 0xFF000000 >> s;
            Gmask = 0x00FF0000 >> s;
            Bmask = 0x0000FF00 >> s;
            Amask = 0x000000FF >> s;
#endif
          }

          SDL_Surface* surface =
              SDL_CreateRGBSurfaceFrom(png_reader->row_pointers, img_width, img_height,
                                       bit_depth * num_channels, pitch, Rmask, Gmask, Bmask, Amask);
          if (surface) {
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            {
              SDL_Texture* img = SDL_CreateTextureFromSurface(renderer_, surface);

              res = SDL_RenderCopy(renderer_, img, NULL, NULL);
              SDL_FreeSurface(surface);
              SDL_DestroyTexture(img);
            }
            SDL_RenderPresent(renderer_);
          }
        }
        SDL_PumpEvents();
      }
    }
    switch (event.type) {
      case SDL_KEYDOWN: {
        HandleKeyPressEvent(&event.key);
        break;
      }
      case SDL_MOUSEBUTTONDOWN: {
        HandleMousePressEvent(&event.button);
        break;
      }
      case SDL_MOUSEMOTION: {
        HandleMouseMoveEvent(&event.motion);
        break;
      }
      case SDL_WINDOWEVENT: {
        HandleWindowEvent(&event.window);
        break;
      }
      case SDL_QUIT: {
        return EXIT_SUCCESS;
      }
      case FF_QUIT_EVENT: {  // stream want to quit
        VideoState* stream = static_cast<VideoState*>(event.user.data1);
        if (stream == stream_.get()) {
          stream_.release();
          SwitchToErrorMode();
        }
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

void Player::Stop() {
  stop_ = true;
}

void Player::SetFullScreen(bool full_screen) {
  SDL_SetWindowFullscreen(window_, full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  if (stream_) {
    stream_->RefreshRequest();
  }
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

  std::string name;
  for (size_t i = 0; i < play_list_.size(); ++i) {
    if (play_list_[i].Id() == stream->Id()) {
      name = play_list_[i].Name();
      break;
    }
  }

  if (!window_) {
    bool created = CreateWindow(width, height, options_.is_full_screen, name, &renderer_, &window_);
    if (!created) {
      return false;
    }
  } else {
    SDL_SetWindowSize(window_, width, height);
    SDL_SetWindowTitle(window_, name.c_str());
  }

  *renderer = renderer_;
  *window = window_;
  return true;
}

void Player::HandleKeyPressEvent(SDL_KeyboardEvent* event) {
  if (options_.exit_on_keydown) {
    Stop();
    return;
  }

  double incr = 0;
  switch (event->keysym.sym) {
    case SDLK_ESCAPE:
    case SDLK_q: {
      Stop();
      return;
    }
    case SDLK_f: {
      bool full_screen = options_.is_full_screen = !options_.is_full_screen;
      SetFullScreen(full_screen);
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
      opt_->startup_volume = stream_->Volume();
      break;
    case SDLK_KP_DIVIDE:
    case SDLK_9:
      stream_->UpdateVolume(-1, SDL_VOLUME_STEP);
      opt_->startup_volume = stream_->Volume();
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
      stream_->MoveToNextFragment(0);
      break;
    case SDLK_PAGEDOWN:
      stream_->MoveToPreviousFragment(0);
      break;
    case SDLK_LEFTBRACKET:  // change channel
      stream_ = CreatePrevStream();
      if (!stream_) {
        SwitchToErrorMode();
        return;
      }
      break;
    case SDLK_RIGHTBRACKET:  // change channel
      stream_ = CreateNextStream();
      if (!stream_) {
        SwitchToErrorMode();
        return;
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
      stream_->StreemSeek(incr);
      break;
    default:
      break;
  }
}

void Player::HandleMousePressEvent(SDL_MouseButtonEvent* event) {
  if (options_.exit_on_mousedown) {
    Stop();
    return;
  }
  if (event->button == SDL_BUTTON_LEFT) {
    if (av_gettime_relative() - last_mouse_left_click_ <= 500000) {  // double click
      bool full_screen = options_.is_full_screen = !options_.is_full_screen;
      SetFullScreen(full_screen);
      last_mouse_left_click_ = 0;
    } else {
      last_mouse_left_click_ = av_gettime_relative();
    }
  }

  if (cursor_hidden_) {
    SDL_ShowCursor(1);
    cursor_hidden_ = false;
  }
  cursor_last_shown_ = av_gettime_relative();

  /*double x;
  if (event->type == SDL_MOUSEBUTTONDOWN) {
    if (event->button.button != SDL_BUTTON_RIGHT) {
      return;
    }
    x = event->button.x;
  } else {
    if (!(event->motion.state & SDL_BUTTON_RMASK)) {
      return;
    }
    x = event->motion.x;
  }
  stream_->StreamSeekPos(x);*/
}

void Player::HandleMouseMoveEvent(SDL_MouseMotionEvent* event) {
  UNUSED(event);
  if (cursor_hidden_) {
    SDL_ShowCursor(1);
    cursor_hidden_ = false;
  }
  cursor_last_shown_ = av_gettime_relative();
}

void Player::HandleWindowEvent(SDL_WindowEvent* event) {
  if (event->event == SDL_WINDOWEVENT_RESIZED) {
    opt_->screen_width = event->data1;
    opt_->screen_height = event->data2;
  }
  if (stream_) {
    stream_->HandleWindowEvent(event);
  }
}

Url Player::CurrentUrl() const {
  size_t pos = curent_stream_pos_;
  if (pos == play_list_.size()) {
    pos = 0;
  }
  return play_list_[pos];
}

void Player::SwitchToErrorMode() {
  Url url = CurrentUrl();
  if (!window_) {
    int w, h;
    if (opt_->screen_width && opt_->screen_height) {
      w = opt_->screen_width;
      h = opt_->screen_height;
    } else {
      w = opt_->default_width;
      h = opt_->default_height;
    }
    CreateWindow(w, h, options_.is_full_screen, url.Name(), &renderer_, &window_);
  } else {
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

common::scoped_ptr<VideoState> Player::CreateNextStream() {
  // check is executed in main thread?
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
  // check is executed in main thread?
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
