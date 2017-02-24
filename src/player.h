#pragma once

#include <SDL2/SDL_events.h>  // for SDL_Event, SDL_PushEvent, etc
#include <SDL2/SDL_render.h>

#include "video_state_handler.h"

#include "url.h"

#include <common/smart_ptr.h>
#include <common/url.h>

namespace core {
struct AppOptions;
}
namespace core {
struct ComplexOptions;
}

class VideoState;

struct PlayerOptions {
  enum { width = 640, height = 480 };
  PlayerOptions();

  common::uri::Uri play_list_location;
  bool exit_on_keydown;
  bool exit_on_mousedown;
  bool is_full_screen;
  int default_width;
  int default_height;
  int screen_width;
  int screen_height;
};

class Player : public VideoStateHandler {
 public:
  Player(const PlayerOptions& options, core::AppOptions* opt, core::ComplexOptions* copt);
  int Exec() WARN_UNUSED_RESULT;
  void Stop();
  void SetFullScreen(bool full_screen);
  ~Player();

 protected:
  virtual bool HandleRequestAudio(VideoState* stream,
                                  int64_t wanted_channel_layout,
                                  int wanted_nb_channels,
                                  int wanted_sample_rate,
                                  core::AudioParams* audio_hw_params) override;

  virtual bool HandleRealocFrame(VideoState* stream, core::VideoFrame* frame) override;
  virtual void HanleDisplayFrame(VideoState* stream, const core::VideoFrame* frame) override;
  virtual bool HandleRequestWindow(VideoState* stream) override;
  virtual void HandleDefaultWindowSize(int width, int height, AVRational sar) override;

  virtual void HandleKeyPressEvent(SDL_KeyboardEvent* event);
  virtual void HandleWindowEvent(SDL_WindowEvent* event);
  virtual void HandleMousePressEvent(SDL_MouseButtonEvent* event);
  virtual void HandleMouseMoveEvent(SDL_MouseMotionEvent* event);

 private:
  /* prepare a new audio buffer */
  static void sdl_audio_callback(void* opaque, uint8_t* stream, int len);

  int ReallocTexture(SDL_Texture** texture,
                     Uint32 new_format,
                     int new_width,
                     int new_height,
                     SDL_BlendMode blendmode,
                     bool init_texture);
  Url CurrentUrl() const;
  void SwitchToErrorMode();
  void CalculateDispalySize();

  bool ChangePlayListLocation(const common::uri::Uri& location);
  VideoState* CreateCurrentStream();
  VideoState* CreateNextStream();
  VideoState* CreatePrevStream();
  VideoState* CreateStreamInner();

  PlayerOptions options_;
  core::AppOptions* opt_;
  core::ComplexOptions* copt_;
  std::vector<Url> play_list_;

  core::AudioParams* audio_params_;

  SDL_Renderer* renderer_;
  SDL_Window* window_;
  bool cursor_hidden_;
  int64_t cursor_last_shown_;
  int64_t last_mouse_left_click_;
  size_t curent_stream_pos_;

  bool stop_;
  VideoState* stream_;

  int width_;
  int height_;
  const int xleft_;
  const int ytop_;
};
