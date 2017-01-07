#ifndef CORE_TYPES_H
#define CORE_TYPES_H

#include "ffmpeg_config.h"

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

#include "core/frame_queue.h"
#include "core/decoder.h"

#define SAMPLE_ARRAY_SIZE (8 * 65536)
/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN 0.04
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX 0.1
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD 0.1

static unsigned sws_flags = SWS_BICUBIC;
static int autorotate = 1;

enum {
  AV_SYNC_AUDIO_MASTER, /* default choice */
  AV_SYNC_VIDEO_MASTER,
  AV_SYNC_EXTERNAL_CLOCK, /* synchronize to an external clock */
};

typedef struct AppOptions {
  AVDictionary* sws_dict;
  AVDictionary* swr_opts;
  AVDictionary* format_opts;
  AVDictionary* codec_opts;
} AppOptions;

typedef struct VideoState {
  AppOptions opt;

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

  Clock audclk;
  Clock vidclk;
  Clock extclk;

  FrameQueue pictq;
  FrameQueue subpq;
  FrameQueue sampq;

  Decoder auddec;
  Decoder viddec;
  Decoder subdec;

  int audio_stream;

  int av_sync_type;

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

  enum ShowMode {
    SHOW_MODE_NONE = -1,
    SHOW_MODE_VIDEO = 0,
    SHOW_MODE_WAVES,
    SHOW_MODE_RDFT,
    SHOW_MODE_NB
  } show_mode;
  int16_t sample_array[SAMPLE_ARRAY_SIZE];
  int sample_array_index;
  int last_i_start;
  RDFTContext* rdft;
  int rdft_bits;
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

  char* filename;
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
} VideoState;

#if CONFIG_AVFILTER
int configure_filtergraph(AVFilterGraph* graph,
                          const char* filtergraph,
                          AVFilterContext* source_ctx,
                          AVFilterContext* sink_ctx);
int configure_video_filters(AVFilterGraph* graph,
                            VideoState* is,
                            const char* vfilters,
                            AVFrame* frame);
int configure_audio_filters(VideoState* is, const char* afilters, int force_output_format);
#endif

int64_t get_valid_channel_layout(int64_t channel_layout, int channels);

#endif
