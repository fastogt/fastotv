#pragma once

#include <stddef.h>  // for size_t
#include <stdint.h>  // for int64_t, uint8_t, int16_t
#include <memory>    // for shared_ptr
#include <string>    // for string

#include <SDL2/SDL_blendmode.h>  // for SDL_BlendMode
#include <SDL2/SDL_render.h>     // for SDL_Renderer, SDL_Texture
#include <SDL2/SDL_stdinc.h>     // for Uint32, Uint8
#include <SDL2/SDL_video.h>      // for SDL_Window
#include <SDL2/SDL_events.h>

#include "ffmpeg_config.h"  // for CONFIG_AVFILTER

extern "C" {
#include <libavfilter/avfilter.h>  // for AVFilterContext (ptr only), etc
#include <libavformat/avformat.h>  // for AVInputFormat, etc
#include <libavutil/frame.h>       // for AVFrame
#include <libavutil/rational.h>    // for AVRational
}

#include <common/macros.h>  // for DISALLOW_COPY_AND_ASSIGN, etc
#include <common/smart_ptr.h>
#include <common/url.h>

#include "core/audio_params.h"  // for AudioParams

namespace common {
namespace threads {
template <typename RT>
class Thread;
}
}
namespace core {
class AudioDecoder;
}
namespace core {
class AudioStream;
}
namespace core {
class VideoDecoder;
}
namespace core {
class VideoStream;
}
namespace core {
struct AppOptions;
}
namespace core {
struct ComplexOptions;
}
namespace core {
struct VideoFrame;
}
namespace core {
template <size_t buffer_size>
class AudioFrameQueue;
}
namespace core {
template <size_t buffer_size>
class VideoFrameQueue;
}

#define SAMPLE_ARRAY_SIZE (8 * 65536)
/* no AV correction is done if too big error */
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9

/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01

#define FF_ALLOC_EVENT (SDL_USEREVENT)
#define FF_QUIT_EVENT (SDL_USEREVENT + 2)

struct Stats {
  Stats() : frame_drops_early(0), frame_drops_late(0) {}

  int FrameDrops() const { return frame_drops_early + frame_drops_late; }

  int frame_drops_early;
  int frame_drops_late;
};

class VideoStateHandler;
class VideoState {
 public:
  enum { invalid_stream_index = -1 };
  VideoState(const common::uri::Uri& uri,
             core::AppOptions* opt,
             core::ComplexOptions* copt,
             VideoStateHandler* handler);
  int Exec() WARN_UNUSED_RESULT;
  void Abort();
  bool IsAborted() const;
  const common::uri::Uri& uri() const;
  virtual ~VideoState();

  void SetFullScreen(bool full_screen);
  /* pause or resume the video */
  void TogglePause();
  void ToggleMute();
  void ToggleWaveDisplay();
  void ToggleAudioDisplay();
  void TryRefreshVideo(double* remaining_time);

  void UpdateVolume(int sign, int step);

  void StepToNextFrame();
  void StreamCycleChannel(AVMediaType codec_type);
  void StreamSeekPos(double x, int seek_by_bytes);
  void StreemSeek(double incr, int seek_by_bytes);

  void MoveToNextFragment(double incr, int seek_by_bytes);
  void MoveToPreviousFragment(double incr, int seek_by_bytes);

  virtual void HandleWindowEvent(SDL_WindowEvent* event);
  virtual int HandleAllocPictureEvent() WARN_UNUSED_RESULT;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoState);

  /* open a given stream. Return 0 if OK */
  int StreamComponentOpen(int stream_index);
  void StreamComponentClose(int stream_index);
  void StreamTogglePause();

  /* seek in the stream */
  void StreamSeek(int64_t pos, int64_t rel, int seek_by_bytes);

  int GetMasterSyncType() const;
  double ComputeTargetDelay(double delay) const;
  double GetMasterClock() const;
  void SetDefaultWindowSize(int width, int height, AVRational sar);
#if CONFIG_AVFILTER
  int ConfigureVideoFilters(AVFilterGraph* graph, const std::string& vfilters, AVFrame* frame);
  int ConfigureAudioFilters(const std::string& afilters, int force_output_format);
#endif

  int VideoOpen(core::VideoFrame* vp);
  /* allocate a picture (needs to do that in main thread to avoid
     potential locking problems */
  int AllocPicture();
  void VideoDisplay();
  /* called to display each frame */
  void VideoRefresh(double* remaining_time);
  int ReallocTexture(SDL_Texture** texture,
                     Uint32 new_format,
                     int new_width,
                     int new_height,
                     SDL_BlendMode blendmode,
                     bool init_texture);
  void VideoAudioDisplay();
  void VideoImageDisplay();

  void SeekChapter(int incr);
  /* copy samples for viewing in editor window */
  void UpdateSampleDisplay(short* samples, int samples_size);
  /* return the wanted number of samples to get better sync if sync_type is video
   * or external master clock */
  int SynchronizeAudio(int nb_samples);
  /**
   * Decode one audio frame and return its uncompressed size.
   *
   * The processed audio frame is decoded, converted if required, and
   * stored in is->audio_buf, with size in bytes given by the return
   * value.
   */
  int AudioDecodeFrame();
  int GetVideoFrame(AVFrame* frame);
  int QueuePicture(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial);

  /* prepare a new audio buffer */
  static void sdl_audio_callback(void* opaque, Uint8* stream, int len);

  int ReadThread();
  int VideoThread();
  int AudioThread();

  const common::uri::Uri uri_;
  core::AppOptions* const opt_;
  core::ComplexOptions* const copt_;
  int64_t audio_callback_time_;

  common::shared_ptr<common::threads::Thread<int>> read_tid_;
  bool force_refresh_;
  bool queue_attachments_req_;
  bool seek_req_;
  int seek_flags_;
  int64_t seek_pos_;
  int64_t seek_rel_;
  int read_pause_return_;
  AVFormatContext* ic_;
  bool realtime_;

  core::VideoStream* vstream_;
  core::AudioStream* astream_;

  core::VideoDecoder* viddec_;
  core::AudioDecoder* auddec_;

  core::VideoFrameQueue<VIDEO_PICTURE_QUEUE_SIZE>* video_frame_queue_;
  core::AudioFrameQueue<SAMPLE_QUEUE_SIZE>* audio_frame_queue_;

  double audio_clock_;
  int audio_clock_serial_;
  double audio_diff_cum_; /* used for AV difference average computation */
  double audio_diff_avg_coef_;
  double audio_diff_threshold_;
  int audio_diff_avg_count_;
  int audio_hw_buf_size_;
  uint8_t* audio_buf_;
  uint8_t* audio_buf1_;
  unsigned int audio_buf_size_; /* in bytes */
  unsigned int audio_buf1_size_;
  int audio_buf_index_; /* in bytes */
  int audio_write_buf_size_;
  int audio_volume_;
  struct core::AudioParams audio_src_;
#if CONFIG_AVFILTER
  struct core::AudioParams audio_filter_src_;
#endif
  struct core::AudioParams audio_tgt_;
  struct SwrContext* swr_ctx_;

  int16_t sample_array_[SAMPLE_ARRAY_SIZE];
  int sample_array_index_;
  int last_i_start_;

  double last_vis_time_;

  double frame_timer_;
  double frame_last_returned_time_;
  double frame_last_filter_delay_;
  double max_frame_duration_;  // maximum duration of a frame - above this, we consider the jump a
                               // timestamp discontinuity
  struct SwsContext* img_convert_ctx_;
  struct SwsContext* sub_convert_ctx_;

  int width_;
  int height_;
  const int xleft_;
  const int ytop_;
  bool step_;

#if CONFIG_AVFILTER
  size_t vfilter_idx_;
  AVFilterContext* in_video_filter_;   // the first filter in the video chain
  AVFilterContext* out_video_filter_;  // the last filter in the video chain
  AVFilterContext* in_audio_filter_;   // the first filter in the audio chain
  AVFilterContext* out_audio_filter_;  // the last filter in the audio chain
  AVFilterGraph* agraph_;              // audio filter graph
#endif

  int last_video_stream_;
  int last_audio_stream_;

  common::shared_ptr<common::threads::Thread<int>> vdecoder_tid_;
  common::shared_ptr<common::threads::Thread<int>> adecoder_tid_;

  bool paused_;
  bool last_paused_;
  bool muted_;
  bool eof_;
  bool abort_request_;

  SDL_Renderer* renderer_;
  SDL_Window* window_;
  Stats stats_;
  VideoStateHandler* handler_;
};
