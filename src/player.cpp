#include "player.h"

#include <SDL2/SDL_audio.h>  // for SDL_MIX_MAXVOLUME, etc
#include <SDL2/SDL_hints.h>

extern "C" {
#include <libavutil/time.h>
#include <png.h>
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
SDL_Surface* IMG_LoadPNG(const char* path) {
  unsigned char header[8];  // 8 is the maximum size that can be checked
  FILE* fp = fopen(path, "rb");
  if (!fp) {
    WARNING_LOG() << "Couldn't open png file path: " << path;
    return NULL;
  }

  size_t res = fread(header, sizeof(unsigned char), SIZEOFMASS(header), fp);
  if (res != SIZEOFMASS(header) || png_sig_cmp(header, 0, SIZEOFMASS(header))) {
    WARNING_LOG() << "Invalid png header path: " << path;
    fclose(fp);
    return NULL;
  }

  /* Create the PNG loading context structure */
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (png_ptr == NULL) {
    WARNING_LOG() << "Couldn't allocate memory for PNG file or incompatible PNG dll";
    fclose(fp);
    return NULL;
  }

  /* Allocate/initialize the memory for image information.  REQUIRED. */
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if (info_ptr == NULL) {
    WARNING_LOG() << "Couldn't create image information for PNG file";
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return NULL;
  }

/* Set error handling if you are using setjmp/longjmp method (this is
 * the normal method of doing things with libpng).  REQUIRED unless you
 * set up your own error handlers in png_create_read_struct() earlier.
 */
#ifndef LIBPNG_VERSION_12
  if (setjmp(*png_set_longjmp_fn(png_ptr, longjmp, sizeof(jmp_buf))))
#else
  if (setjmp(png_ptr->jmpbuf))
#endif
  {
    WARNING_LOG() << "Error reading the PNG file.";
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
    fclose(fp);
    return NULL;
  }

  /* Set up the input control */
  png_init_io(png_ptr, fp);
  png_set_sig_bytes(png_ptr, 8);

  png_uint_32 width, height;
  int bit_depth, color_type, interlace_type;
  SDL_Palette* palette;
  int ckey = -1;
  png_color_16* transv;
  /* Read PNG header info */
  png_read_info(png_ptr, info_ptr);
  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL,
               NULL);

  /* tell libpng to strip 16 bit/color files down to 8 bits/color */
  png_set_strip_16(png_ptr);

  /* Extract multiple pixels with bit depths of 1, 2, and 4 from a single
   * byte into separate bytes (useful for paletted and grayscale images).
   */
  png_set_packing(png_ptr);

  /* scale greyscale values to the range 0..255 */
  if (color_type == PNG_COLOR_TYPE_GRAY)
    png_set_expand(png_ptr);

  /* For images with a single "transparent colour", set colour key;
     if more than one index has transparency, or if partially transparent
     entries exist, use full alpha channel */
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
    int num_trans;
    Uint8* trans;
    png_get_tRNS(png_ptr, info_ptr, &trans, &num_trans, &transv);
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
      /* Check if all tRNS entries are opaque except one */
      int j, t = -1;
      for (j = 0; j < num_trans; j++) {
        if (trans[j] == 0) {
          if (t >= 0) {
            break;
          }
          t = j;
        } else if (trans[j] != 255) {
          break;
        }
      }
      if (j == num_trans) {
        /* exactly one transparent index */
        ckey = t;
      } else {
        /* more than one transparent index, or translucency */
        png_set_expand(png_ptr);
      }
    } else {
      ckey = 0; /* actual value will be set later */
    }
  }

  if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA) {
    png_set_gray_to_rgb(png_ptr);
  }

  png_read_update_info(png_ptr, info_ptr);

  png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, &interlace_type, NULL,
               NULL);

  /* Allocate the SDL surface to hold the image */
  Uint32 Rmask = 0;
  Uint32 Gmask = 0;
  Uint32 Bmask = 0;
  Uint32 Amask = 0;
  int num_channels = png_get_channels(png_ptr, info_ptr);
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
  SDL_Surface* volatile surface = SDL_CreateRGBSurface(
      SDL_SWSURFACE, width, height, bit_depth * num_channels, Rmask, Gmask, Bmask, Amask);
  if (surface == NULL) {
    WARNING_LOG() << SDL_GetError();
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return NULL;
  }

  if (ckey != -1) {
    if (color_type != PNG_COLOR_TYPE_PALETTE) {
      /* FIXME: Should these be truncated or shifted down? */
      ckey = SDL_MapRGB(surface->format, (Uint8)transv->red, (Uint8)transv->green,
                        (Uint8)transv->blue);
    }
    SDL_SetColorKey(surface, SDL_TRUE, ckey);
  }

  /* Create the array of pointers to image data */
  png_bytep* volatile row_pointers =
      static_cast<png_bytep*>(SDL_malloc(sizeof(png_bytep) * height));
  if (!row_pointers) {
    WARNING_LOG() << "Out of memory";
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    fclose(fp);
    return NULL;
  }
  for (png_uint_32 row = 0; row < height; row++) {
    row_pointers[row] = static_cast<png_bytep>(surface->pixels) + row * surface->pitch;
  }

  /* Read the entire image in one go */
  png_read_image(png_ptr, row_pointers);

  /* and we're done!  (png_read_end() can be omitted if no processing of
   * post-IDAT text/time/etc. is desired)
   * In some cases it can't read PNG's created by some popular programs (ACDSEE),
   * we do not want to process comments, so we omit png_read_end

  lib.png_read_end(png_ptr, info_ptr);
  */

  /* Load the palette, if any */
  palette = surface->format->palette;
  if (palette) {
    int png_num_palette;
    png_colorp png_palette;
    png_get_PLTE(png_ptr, info_ptr, &png_palette, &png_num_palette);
    if (color_type == PNG_COLOR_TYPE_GRAY) {
      palette->ncolors = 256;
      for (int i = 0; i < 256; i++) {
        palette->colors[i].r = i;
        palette->colors[i].g = i;
        palette->colors[i].b = i;
      }
    } else if (png_num_palette > 0) {
      palette->ncolors = png_num_palette;
      for (int i = 0; i < png_num_palette; ++i) {
        palette->colors[i].b = png_palette[i].blue;
        palette->colors[i].g = png_palette[i].green;
        palette->colors[i].r = png_palette[i].red;
      }
    }
  }

  SDL_free(row_pointers);
  png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
  fclose(fp);
  return (surface);
}

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
  stream_ = CreateCurrentStream();
  if (!stream_) {
    SwitchToErrorMode();
  }

  SDL_Surface* surface = IMG_LoadPNG(IMG_PATH);
  while (!stop_) {
    SDL_Event event;
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
          SDL_Texture* img = SDL_CreateTextureFromSurface(renderer_, surface);
          if (img) {
            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            SDL_RenderCopy(renderer_, img, NULL, NULL);
            SDL_RenderPresent(renderer_);
            SDL_DestroyTexture(img);
          }
        }
        SDL_PumpEvents();
      }
    }
    // handle event
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
        SDL_FreeSurface(surface);
        return EXIT_SUCCESS;
      }
      case FF_QUIT_EVENT: {  // stream want to quit
        // VideoState* stream = static_cast<VideoState*>(event.user.data1);
        destroy(&stream_);
        SwitchToErrorMode();
        break;
      }
      case FF_NEXT_STREAM: {
        stream_ = CreateNextStream();
        if (!stream_) {
          SwitchToErrorMode();
        }
        break;
      }
      case FF_PREV_STREAM: {
        stream_ = CreatePrevStream();
        if (!stream_) {
          SwitchToErrorMode();
        }
        break;
      }
      case FF_ALLOC_EVENT: {
        int res = static_cast<VideoState*>(event.user.data1)->HandleAllocPictureEvent();
        if (res == ERROR_RESULT_VALUE) {
          SDL_FreeSurface(surface);
          return EXIT_FAILURE;
        }
        break;
      }
      default:
        break;
    }
  }

  SDL_FreeSurface(surface);
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
      if (stream_) {
        stream_->TogglePause();
      }
      break;
    case SDLK_m:
      if (stream_) {
        stream_->ToggleMute();
      }
      break;
    case SDLK_KP_MULTIPLY:
    case SDLK_0:
      if (stream_) {
        stream_->UpdateVolume(1, SDL_VOLUME_STEP);
        opt_->startup_volume = stream_->Volume();
      }
      break;
    case SDLK_KP_DIVIDE:
    case SDLK_9:
      if (stream_) {
        stream_->UpdateVolume(-1, SDL_VOLUME_STEP);
        opt_->startup_volume = stream_->Volume();
      }
      break;
    case SDLK_s:  // S: Step to next frame
      if (stream_) {
        stream_->StepToNextFrame();
      }
      break;
    case SDLK_a:
      if (stream_) {
        stream_->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
      }
      break;
    case SDLK_v:
      if (stream_) {
        stream_->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
      }
      break;
    case SDLK_c:
      if (stream_) {
        stream_->StreamCycleChannel(AVMEDIA_TYPE_VIDEO);
        stream_->StreamCycleChannel(AVMEDIA_TYPE_AUDIO);
      }
      break;
    case SDLK_t:
      // StreamCycleChannel(AVMEDIA_TYPE_SUBTITLE);
      break;
    case SDLK_w: {
      if (stream_) {
        stream_->ToggleWaveDisplay();
      }
      break;
    }
    case SDLK_PAGEUP:
      if (stream_) {
        stream_->MoveToNextFragment(0);
      }
      break;
    case SDLK_PAGEDOWN:
      if (stream_) {
        stream_->MoveToPreviousFragment(0);
      }
      break;
    case SDLK_LEFTBRACKET: {  // change channel
      if (stream_) {
        stream_->Abort();
      }
      SDL_Event event;
      event.type = FF_PREV_STREAM;
      SDL_PushEvent(&event);
      break;
    }
    case SDLK_RIGHTBRACKET: {  // change channel
      if (stream_) {
        stream_->Abort();
      }
      SDL_Event event;
      event.type = FF_NEXT_STREAM;
      SDL_PushEvent(&event);
      break;
    }
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
      if (stream_) {
        stream_->StreemSeek(incr);
      }
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
  const std::string name_str = url.Name();
  int w, h;
  if (opt_->screen_width && opt_->screen_height) {
    w = opt_->screen_width;
    h = opt_->screen_height;
  } else {
    w = opt_->default_width;
    h = opt_->default_height;
  }

  if (!window_) {
    CreateWindow(w, h, options_.is_full_screen, name_str.c_str(), &renderer_, &window_);
  } else {
    SDL_SetWindowTitle(window_, name_str.c_str());
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

VideoState* Player::CreateCurrentStream() {
  Url url = play_list_[curent_stream_pos_];
  VideoState* stream = new VideoState(url.Id(), url.GetUrl(), *opt_, copt_, this);

  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    delete stream;
    return nullptr;
  }

  return stream;
}

VideoState* Player::CreateNextStream() {
  // check is executed in main thread?
  if (play_list_.empty()) {
    return nullptr;
  }

  if (curent_stream_pos_ + 1 == play_list_.size()) {
    curent_stream_pos_ = 0;
  } else {
    curent_stream_pos_++;
  }
  Url url = play_list_[curent_stream_pos_];
  VideoState* stream = new VideoState(url.Id(), url.GetUrl(), *opt_, copt_, this);

  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    delete stream;
    return nullptr;
  }

  return stream;
}

VideoState* Player::CreatePrevStream() {
  // check is executed in main thread?
  if (play_list_.empty()) {
    return nullptr;
  }

  if (curent_stream_pos_ == 0) {
    curent_stream_pos_ = play_list_.size() - 1;
  } else {
    --curent_stream_pos_;
  }
  Url url = play_list_[curent_stream_pos_];
  VideoState* stream = new VideoState(url.Id(), url.GetUrl(), *opt_, copt_, this);

  int res = stream->Exec();
  if (res == EXIT_FAILURE) {
    delete stream;
    return nullptr;
  }

  return stream;
}
