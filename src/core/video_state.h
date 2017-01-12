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
}

#include <common/macros.h>

#include "core/clock.h"
#include "core/frame_queue.h"
#include "core/decoder.h"
#include "core/app_options.h"

#define SAMPLE_ARRAY_SIZE (8 * 65536)
/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

struct AudioParams {
  int freq;
  int channels;
  int64_t channel_layout;
  enum AVSampleFormat fmt;
  int frame_size;
  int bytes_per_sec;
};

struct VideoState {
  VideoState(AVInputFormat* ifo, AppOptions* opt, ComplexOptions* copt);
  int exec();

  ~VideoState();

  AppOptions* const opt;
  ComplexOptions* const copt;
  int64_t audio_callback_time;

  SDL_Thread* read_tid;
  AVInputFormat* iformat;
  int abort_request;
  int force_refresh;
  int paused;
  int last_paused;
  int queue_attachments_req;
  int seek_req;
  int seek_flags;
  int64_t seek_pos;
  int64_t seek_rel;
  int read_pause_return;
  AVFormatContext* ic;
  int realtime;

  Clock* audclk;
  Clock* vidclk;
  Clock* extclk;

  FrameQueue pictq;
  FrameQueue subpq;
  FrameQueue sampq;

  Decoder* auddec;
  VideoDecoder* viddec;
  SubDecoder* subdec;

  int audio_stream;

  double audio_clock;
  int audio_clock_serial;
  double audio_diff_cum; /* used for AV difference average computation */
  double audio_diff_avg_coef;
  double audio_diff_threshold;
  int audio_diff_avg_count;
  AVStream* audio_st;
  PacketQueue audioq;
  int audio_hw_buf_size;
  uint8_t* audio_buf;
  uint8_t* audio_buf1;
  unsigned int audio_buf_size; /* in bytes */
  unsigned int audio_buf1_size;
  int audio_buf_index; /* in bytes */
  int audio_write_buf_size;
  int audio_volume;
  int muted;
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

  int subtitle_stream;
  AVStream* subtitle_st;
  PacketQueue subtitleq;

  double frame_timer;
  double frame_last_returned_time;
  double frame_last_filter_delay;
  int video_stream;
  AVStream* video_st;
  PacketQueue videoq;
  double max_frame_duration;  // maximum duration of a frame - above this, we consider the jump a
                              // timestamp discontinuity
  struct SwsContext* img_convert_ctx;
  struct SwsContext* sub_convert_ctx;
  int eof;

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

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoState);

  void refresh_loop_wait_event(SDL_Event* event);
  int video_open(Frame* vp);
  /* allocate a picture (needs to do that in main thread to avoid
     potential locking problems */
  int alloc_picture();
  void video_display();
  /* called to display each frame */
  void video_refresh(double* remaining_time);
  void toggle_full_screen();
  int realloc_texture(SDL_Texture** texture,
                      Uint32 new_format,
                      int new_width,
                      int new_height,
                      SDL_BlendMode blendmode,
                      int init_texture);
  void video_audio_display();
  void video_image_display();

  bool cursor_hidden_;
  int64_t cursor_last_shown_;
  SDL_Renderer* renderer;
  SDL_Window* window;
};
