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
#define VIDEO_PICTURE_QUEUE_SIZE 3
#define SAMPLE_QUEUE_SIZE 9

class VideoState : public core::Decoder::DecoderClient {
 public:
  VideoState(AVInputFormat* ifo, core::AppOptions* opt, core::ComplexOptions* copt);
  int Exec() WARN_UNUSED_RESULT;
  ~VideoState();

  void ToggleFullScreen();
  void StreamTogglePause();
  void TogglePause();
  void ToggleMute();
  void ToggleAudioDisplay();

 protected:
  virtual void HandleEmptyQueue(core::Decoder* dec) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(VideoState);

  /* open a given stream. Return 0 if OK */
  int StreamComponentOpen(int stream_index);
  void StreamComponentClose(int stream_index);

  /* seek in the stream */
  void StreamSeek(int64_t pos, int64_t rel, int seek_by_bytes);

  void StepToNextFrame();
  int GetMasterSyncType() const;
  double ComputeTargetDelay(double delay);
  double GetMasterClock();
  void SetDefaultWindowSize(int width, int height, AVRational sar);
#if CONFIG_AVFILTER
  int ConfigureVideoFilters(AVFilterGraph* graph, const char* vfilters, AVFrame* frame);
  int ConfigureAudioFilters(const char* afilters, int force_output_format);
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
                     int init_texture);
  void VideoAudioDisplay();
  void VideoImageDisplay();
  double VpDuration(core::VideoFrame* vp, core::VideoFrame* nextvp);
  /* pause or resume the video */
  void UpdateVolume(int sign, int step);
  void SeekChapter(int incr);
  /* copy samples for viewing in editor window */
  void UpdateSampleDisplay(short* samples, int samples_size);
  void StreamCycleChannel(int codec_type);
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
  static int decode_interrupt_callback(void* user_data);

  core::AppOptions* const opt_;
  core::ComplexOptions* const copt_;
  int64_t audio_callback_time_;

  common::shared_ptr<common::thread::Thread<int>> read_tid_;
  AVInputFormat* iformat_;
  bool force_refresh_;
  int queue_attachments_req_;
  int seek_req_;
  int seek_flags_;
  int64_t seek_pos_;
  int64_t seek_rel_;
  int read_pause_return_;
  AVFormatContext* ic_;
  int realtime_;

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
  int frame_drops_early_;
  int frame_drops_late_;

  int16_t sample_array_[SAMPLE_ARRAY_SIZE];
  int sample_array_index_;
  int last_i_start_;

  RDFTContext* rdft_;
  int rdft_bits_;
  FFTSample* rdft_data_;

  int xpos_;
  double last_vis_time_;
  SDL_Texture* vis_texture_;
  SDL_Texture* sub_texture_;

  double frame_timer_;
  double frame_last_returned_time_;
  double frame_last_filter_delay_;
  double max_frame_duration_;  // maximum duration of a frame - above this, we consider the jump a
                               // timestamp discontinuity
  struct SwsContext* img_convert_ctx_;
  struct SwsContext* sub_convert_ctx_;

  int width_;
  int height_;
  int xleft_;
  int ytop_;
  int step_;

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

  common::thread::condition_variable continue_read_thread_;
  common::thread::mutex wait_mutex_;
  common::shared_ptr<common::thread::Thread<int>> vdecoder_tid_;
  common::shared_ptr<common::thread::Thread<int>> adecoder_tid_;

  bool paused_;
  bool last_paused_;
  bool cursor_hidden_;
  bool muted_;
  int64_t cursor_last_shown_;
  bool eof_;
  bool abort_request_;

  SDL_Renderer* renderer_;
  SDL_Window* window_;
};
