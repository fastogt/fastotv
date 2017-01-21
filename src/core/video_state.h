#pragma once

#include "ffmpeg_config.h"
extern "C" {
#include <libavutil/avstring.h>
#include <libavutil/eval.h>
#include <libavutil/mathematics.h>
#include <libavutil/pixdesc.h>
#include <libavutil/imgutils.h>
#include <libavutil/dict.h>
#include <libavutil/parseutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/avassert.h>
#include <libavutil/time.h>
#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavcodec/avfft.h>
#include <libswresample/swresample.h>

#if CONFIG_AVFILTER
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#endif

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_mutex.h>
}

#include <common/macros.h>
#include <common/thread/thread_manager.h>

#include "core/frame_queue.h"
#include "core/decoder.h"
#include "core/app_options.h"
#include "core/audio_params.h"
#include "core/stream.h"

#define SAMPLE_ARRAY_SIZE (8 * 65536)
/* no AV correction is done if too big error */
#define SUBPICTURE_QUEUE_SIZE 16
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9

class VideoState : public Decoder::DecoderClient {
 public:
  VideoState(AVInputFormat* ifo, AppOptions* opt, ComplexOptions* copt);
  int Exec() WARN_UNUSED_RESULT;
  ~VideoState();

  void ToggleFullScreen();
  void StreamTogglePause();
  void TogglePause();
  void ToggleMute();
  void ToggleAudioDisplay();

 protected:
  virtual void HandleEmptyQueue(Decoder* dec);

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoState);

  /* open a given stream. Return 0 if OK */
  int stream_component_open(int stream_index);
  void stream_component_close(int stream_index);

  /* seek in the stream */
  void stream_seek(int64_t pos, int64_t rel, int seek_by_bytes);

  void step_to_next_frame();
  int get_master_sync_type() const;
  double compute_target_delay(double delay);
  double get_master_clock();
  void set_default_window_size(int width, int height, AVRational sar);
#if CONFIG_AVFILTER
  int configure_video_filters(AVFilterGraph* graph, const char* vfilters, AVFrame* frame);
  int configure_audio_filters(const char* afilters, int force_output_format);
#endif

  void refresh_loop_wait_event(SDL_Event* event);
  int video_open(VideoFrame* vp);
  /* allocate a picture (needs to do that in main thread to avoid
     potential locking problems */
  int alloc_picture();
  void video_display();
  /* called to display each frame */
  void video_refresh(double* remaining_time);
  int realloc_texture(SDL_Texture** texture,
                      Uint32 new_format,
                      int new_width,
                      int new_height,
                      SDL_BlendMode blendmode,
                      int init_texture);
  void video_audio_display();
  void video_image_display();
  void check_external_clock_speed();
  double vp_duration(VideoFrame* vp, VideoFrame* nextvp);
  /* pause or resume the video */
  void update_volume(int sign, int step);
  void seek_chapter(int incr);
  /* copy samples for viewing in editor window */
  void update_sample_display(short* samples, int samples_size);
  void stream_cycle_channel(int codec_type);
  /* return the wanted number of samples to get better sync if sync_type is video
   * or external master clock */
  int synchronize_audio(int nb_samples);
  /**
   * Decode one audio frame and return its uncompressed size.
   *
   * The processed audio frame is decoded, converted if required, and
   * stored in is->audio_buf, with size in bytes given by the return
   * value.
   */
  int audio_decode_frame();
  int get_video_frame(AVFrame* frame);
  int queue_picture(AVFrame* src_frame, double pts, double duration, int64_t pos, int serial);

  /* prepare a new audio buffer */
  static void sdl_audio_callback(void* opaque, Uint8* stream, int len);

  int read_thread();
  int video_thread();
  int audio_thread();
  int subtitle_thread();
  static int decode_interrupt_cb(void* user_data);

  AppOptions* const opt;
  ComplexOptions* const copt;
  int64_t audio_callback_time;

  common::shared_ptr<common::thread::Thread<int>> read_tid_;
  AVInputFormat* iformat;
  int force_refresh;
  int queue_attachments_req;
  int seek_req;
  int seek_flags;
  int64_t seek_pos;
  int64_t seek_rel;
  int read_pause_return;
  AVFormatContext* ic;
  int realtime;

  VideoStream* vstream_;
  AudioStream* astream_;
  SubtitleStream* sstream_;

  VideoDecoder* viddec;
  AudioDecoder* auddec;
  SubDecoder* subdec;

  VideoFrameQueueEx<VIDEO_PICTURE_QUEUE_SIZE>* video_frame_queue_;
  AudioFrameQueue<SAMPLE_QUEUE_SIZE>* audio_frame_queue_;
  SubTitleQueue<SUBPICTURE_QUEUE_SIZE>* subtitle_frame_queue_;

  double audio_clock;
  int audio_clock_serial;
  double audio_diff_cum; /* used for AV difference average computation */
  double audio_diff_avg_coef;
  double audio_diff_threshold;
  int audio_diff_avg_count;
  int audio_hw_buf_size;
  uint8_t* audio_buf;
  uint8_t* audio_buf1;
  unsigned int audio_buf_size; /* in bytes */
  unsigned int audio_buf1_size;
  int audio_buf_index; /* in bytes */
  int audio_write_buf_size;
  int audio_volume;
  struct AudioParams audio_src;
#if CONFIG_AVFILTER
  struct AudioParams audio_filter_src;
#endif
  struct AudioParams audio_tgt;
  struct SwrContext* swr_ctx;
  int frame_drops_early;
  int frame_drops_late;

  int16_t sample_array[SAMPLE_ARRAY_SIZE];
  int sample_array_index;
  int last_i_start;
  RDFTContext* rdft;
  int rdft_bits_;
  FFTSample* rdft_data;
  int xpos;
  double last_vis_time;
  SDL_Texture* vis_texture;
  SDL_Texture* sub_texture;

  double frame_timer;
  double frame_last_returned_time;
  double frame_last_filter_delay;
  double max_frame_duration;  // maximum duration of a frame - above this, we consider the jump a
                              // timestamp discontinuity
  struct SwsContext* img_convert_ctx;
  struct SwsContext* sub_convert_ctx;

  int width, height, xleft, ytop;
  int step;

#if CONFIG_AVFILTER
  int vfilter_idx;
  AVFilterContext* in_video_filter;   // the first filter in the video chain
  AVFilterContext* out_video_filter;  // the last filter in the video chain
  AVFilterContext* in_audio_filter;   // the first filter in the audio chain
  AVFilterContext* out_audio_filter;  // the last filter in the audio chain
  AVFilterGraph* agraph;              // audio filter graph
#endif

  int last_video_stream, last_audio_stream, last_subtitle_stream;

  SDL_cond* continue_read_thread;
  common::shared_ptr<common::thread::Thread<int>> vdecoder_tid_;
  common::shared_ptr<common::thread::Thread<int>> adecoder_tid_;
  common::shared_ptr<common::thread::Thread<int>> sdecoder_tid_;

  bool paused_;
  bool last_paused_;
  bool cursor_hidden_;
  bool muted_;
  int64_t cursor_last_shown_;
  bool eof_;
  bool abort_request_;

  SDL_Renderer* renderer;
  SDL_Window* window;
};
