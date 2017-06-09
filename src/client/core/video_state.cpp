/*  Copyright (C) 2014-2017 FastoGT. All right reserved.

    This file is part of FastoTV.

    FastoTV is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FastoTV is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FastoTV. If not, see <http://www.gnu.org/licenses/>.
*/

#include "client/core/video_state.h"

#include <errno.h>             // for ENOMEM, EINVAL, EAGAIN
#include <inttypes.h>          // for PRIx64
#include <math.h>              // for fabs, exp, log
#include <stdio.h>             // for snprintf
#include <stdlib.h>            // for NULL, abs, calloc, free
#include <string.h>            // for memset, strcmp, strlen
#include <condition_variable>  // for cv_status, cv_status::...

extern "C" {
#include <libavcodec/avcodec.h>        // for AVCodecContext, AVCode...
#include <libavcodec/version.h>        // for FF_API_EMU_EDGE
#include <libavformat/avio.h>          // for AVIOContext, AVIOInter...
#include <libavutil/avstring.h>        // for av_strlcatf
#include <libavutil/avutil.h>          // for AVMediaType::AVMEDIA_T...
#include <libavutil/buffer.h>          // for av_buffer_ref
#include <libavutil/channel_layout.h>  // for av_get_channel_layout_...
#include <libavutil/dict.h>            // for av_dict_free, av_dict_get
#include <libavutil/error.h>           // for AVERROR, AVERROR_EOF
#include <libavutil/mathematics.h>     // for av_compare_ts, av_resc...
#include <libavutil/mem.h>             // for av_freep, av_fast_malloc
#include <libavutil/opt.h>             // for AV_OPT_SEARCH_CHILDREN
#include <libavutil/pixdesc.h>         // for AVPixFmtDescriptor
#include <libavutil/pixfmt.h>          // for AVPixelFormat, AVPixel...
#include <libavutil/rational.h>        // for AVRational
#include <libavutil/samplefmt.h>       // for AVSampleFormat, av_sam...
#include <libswresample/swresample.h>  // for swr_free, swr_init

#if CONFIG_AVFILTER
#include <libavfilter/avfilter.h>    // for avfilter_graph_free
#include <libavfilter/buffersink.h>  // for av_buffersink_get_fram...
#include <libavfilter/buffersrc.h>   // for av_buffersrc_add_frame
#endif
}

#include <common/application/application.h>  // for fApp
#include <common/error.h>                    // for Error, make_error_valu...
#include <common/logger.h>                   // for COMPACT_LOG_WARNING
#include <common/macros.h>                   // for ERROR_RESULT_VALUE
#include <common/threads/thread_manager.h>   // for THREAD_MANAGER
#include <common/utils.h>                    // for freeifnotnull
#include <common/value.h>                    // for Value, Value::ErrorsTy...

#include "ffmpeg_internal.h"

#include "client/core/app_options.h"           // for ComplexOptions, AppOpt...
#include "client/core/audio_frame.h"           // for AudioFrame
#include "client/core/bandwidth_estimation.h"  // for DesireBytesPerSec
#include "client/core/decoder.h"               // for VideoDecoder, AudioDec...
#include "client/core/events/stream_events.h"  // for QuitStreamEvent, Alloc...
#include "client/core/frame_queue.h"           // for VideoDecoder, AudioDec...
#include "client/core/packet_queue.h"          // for PacketQueue
#include "client/core/stream.h"                // for AudioStream, VideoStream
#include "client/core/types.h"                 // for clock_t, IsValidClock
#include "client/core/utils.h"                 // for q2d_diff, configure_fi...
#include "client/core/video_frame.h"           // for VideoFrame
#include "client/core/video_state_handler.h"
#include "client/types.h"  // for Size

#undef ERROR

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN_MSEC 40
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX_MSEC 100
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD_MSEC 100

#define AV_NOSYNC_THRESHOLD_MSEC 10000

/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10
/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB 20

/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */
#define MAX_QUEUE_SIZE (15 * 1024 * 1024)

namespace {
std::string ffmpeg_errno_to_string(int err) {
  char errbuf[128];
  if (av_strerror(err, errbuf, sizeof(errbuf)) < 0) {
    return strerror(AVUNERROR(err));
  }
  return errbuf;
}
}  // namespace

namespace fasto {
namespace fastotv {
namespace client {
namespace core {

namespace {

int decode_interrupt_callback(void* user_data) {
  VideoState* is = static_cast<VideoState*>(user_data);
  return is->IsAborted();
}

enum AVPixelFormat get_format(AVCodecContext* s, const enum AVPixelFormat* pix_fmts) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);
  const enum AVPixelFormat* p;
  for (p = pix_fmts; *p != -1; p++) {
    const AVPixFmtDescriptor* desc = av_pix_fmt_desc_get(*p);

    if (!(desc->flags & AV_PIX_FMT_FLAG_HWACCEL)) {
      break;
    }

    const HWAccel* hwaccel = get_hwaccel(*p);
    if (!hwaccel || (ist->active_hwaccel_id && ist->active_hwaccel_id != hwaccel->id) ||
        (ist->hwaccel_id != HWACCEL_AUTO && ist->hwaccel_id != hwaccel->id)) {
      continue;
    }

    int ret = hwaccel->init(s);
    if (ret < 0) {
      if (ist->hwaccel_id == hwaccel->id) {
        return AV_PIX_FMT_NONE;
      }
      continue;
    }

    if (ist->hw_frames_ctx) {
      s->hw_frames_ctx = av_buffer_ref(ist->hw_frames_ctx);
      if (!s->hw_frames_ctx) {
        return AV_PIX_FMT_NONE;
      }
    }

    ist->active_hwaccel_id = hwaccel->id;
    ist->hwaccel_pix_fmt = *p;
    break;
  }

  return *p;
}

int get_buffer(AVCodecContext* s, AVFrame* frame, int flags) {
  InputStream* ist = static_cast<InputStream*>(s->opaque);

  if (ist->hwaccel_get_buffer && frame->format == ist->hwaccel_pix_fmt) {
    return ist->hwaccel_get_buffer(s, frame, flags);
  }

  return avcodec_default_get_buffer2(s, frame, flags);
}
}  // namespace

VideoState::VideoState(stream_id id,
                       const common::uri::Uri& uri,
                       const AppOptions& opt,
                       const ComplexOptions& copt,
                       VideoStateHandler* handler)
    : id_(id),
      uri_(uri),
      opt_(opt),
      copt_(copt),
      read_tid_(THREAD_MANAGER()->CreateThread(&VideoState::ReadThread, this)),
      force_refresh_(false),
      read_pause_return_(0),
      ic_(NULL),
      realtime_(false),
      vstream_(new VideoStream),
      astream_(new AudioStream),
      viddec_(nullptr),
      auddec_(nullptr),
      video_frame_queue_(nullptr),
      audio_frame_queue_(nullptr),
      audio_clock_(0),
      audio_diff_cum_(0),
      audio_diff_avg_coef_(0),
      audio_diff_threshold_(0),
      audio_diff_avg_count_(0),
      audio_hw_buf_size_(0),
      audio_buf_(NULL),
      audio_buf1_(NULL),
      audio_buf_size_(0),
      audio_buf1_size_(0),
      audio_buf_index_(0),
      audio_write_buf_size_(0),
      audio_src_(),
#if CONFIG_AVFILTER
      audio_filter_src_(),
#endif
      audio_tgt_(),
      swr_ctx_(NULL),
      frame_timer_(0),
      frame_last_returned_time_(0),
      frame_last_filter_delay_(0),
      max_frame_duration_(0),
      step_(false),
#if CONFIG_AVFILTER
      in_video_filter_(NULL),
      out_video_filter_(NULL),
      in_audio_filter_(NULL),
      out_audio_filter_(NULL),
      agraph_(NULL),
#endif
      last_video_stream_(invalid_stream_index),
      last_audio_stream_(invalid_stream_index),
      vdecoder_tid_(THREAD_MANAGER()->CreateThread(&VideoState::VideoThread, this)),
      adecoder_tid_(THREAD_MANAGER()->CreateThread(&VideoState::AudioThread, this)),
      paused_(false),
      last_paused_(false),
      eof_(false),
      abort_request_(false),
      stats_(new Stats),
      handler_(handler),
      input_st_(static_cast<InputStream*>(calloc(1, sizeof(InputStream)))),
      seek_req_(false),
      seek_pos_(0),
      seek_rel_(0),
      seek_flags_(0),
      read_thread_cond_(),
      read_thread_mutex_() {
  CHECK(handler_);
  CHECK(id_ != invalid_stream_id);

  input_st_->hwaccel_id = opt_.hwaccel_id;
  input_st_->hwaccel_device = common::utils::strdupornull(opt_.hwaccel_device);
  const char* hwaccel_output_format = common::utils::c_strornull(opt_.hwaccel_output_format);
  if (hwaccel_output_format) {
    input_st_->hwaccel_output_format = av_get_pix_fmt(hwaccel_output_format);
    if (input_st_->hwaccel_output_format == AV_PIX_FMT_NONE) {
      CRITICAL_LOG() << "Unrecognised hwaccel output format: " << hwaccel_output_format;
    }
  } else {
    input_st_->hwaccel_output_format = AV_PIX_FMT_NONE;
  }
}

VideoState::~VideoState() {
  destroy(&astream_);
  destroy(&vstream_);

  common::utils::freeifnotnull(input_st_->hwaccel_device);
  free(input_st_);
  input_st_ = NULL;
}

int VideoState::StreamComponentOpen(int stream_index) {
  if (stream_index == invalid_stream_index || static_cast<unsigned int>(stream_index) >= ic_->nb_streams) {
    return AVERROR(EINVAL);
  }

  AVCodecContext* avctx = avcodec_alloc_context3(NULL);
  if (!avctx) {
    return AVERROR(ENOMEM);
  }

  AVStream* stream = ic_->streams[stream_index];
  int ret = avcodec_parameters_to_context(avctx, stream->codecpar);
  if (ret < 0) {
    avcodec_free_context(&avctx);
    return ret;
  }

  const char* forced_codec_name = NULL;

  AVRational tb = stream->time_base;
  av_codec_set_pkt_timebase(avctx, tb);
  AVCodec* codec = avcodec_find_decoder(avctx->codec_id);

  if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
    last_video_stream_ = stream_index;
    forced_codec_name = common::utils::c_strornull(opt_.video_codec_name);
  } else if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
    last_audio_stream_ = stream_index;
    forced_codec_name = common::utils::c_strornull(opt_.audio_codec_name);
  }

  if (forced_codec_name) {
    codec = avcodec_find_decoder_by_name(forced_codec_name);
  }
  if (!codec) {
    if (forced_codec_name) {
      WARNING_LOG() << "No codec could be found with name '" << forced_codec_name << "'";
    } else {
      WARNING_LOG() << "No codec could be found with id " << avctx->codec_id;
    }
    ret = AVERROR(EINVAL);
    avcodec_free_context(&avctx);
    return ret;
  }

  int stream_lowres = opt_.lowres;
  avctx->codec_id = codec->id;
  if (stream_lowres > av_codec_get_max_lowres(codec)) {
    WARNING_LOG() << "The maximum value for lowres supported by the decoder is " << av_codec_get_max_lowres(codec);
    stream_lowres = av_codec_get_max_lowres(codec);
  }
  av_codec_set_lowres(avctx, stream_lowres);

#if FF_API_EMU_EDGE
  if (stream_lowres) {
    avctx->flags |= CODEC_FLAG_EMU_EDGE;
  }
#endif
  if (opt_.fast) {
    avctx->flags2 |= AV_CODEC_FLAG2_FAST;
  }
#if FF_API_EMU_EDGE
  if (codec->capabilities & AV_CODEC_CAP_DR1) {
    avctx->flags |= CODEC_FLAG_EMU_EDGE;
  }
#endif

  if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
    avctx->opaque = input_st_;
    avctx->get_format = get_format;
    avctx->get_buffer2 = get_buffer;
    avctx->thread_safe_callbacks = 1;
  }

  AVDictionary* opts = filter_codec_opts(copt_.codec_opts, avctx->codec_id, ic_, stream, codec);
  if (!av_dict_get(opts, "threads", NULL, 0)) {
    av_dict_set(&opts, "threads", "auto", 0);
  }
  if (stream_lowres) {
    av_dict_set_int(&opts, "lowres", stream_lowres, 0);
  }
  if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
    av_dict_set(&opts, "refcounted_frames", "1", 0);
  }
  ret = avcodec_open2(avctx, codec, &opts);
  if (ret < 0) {
    avcodec_free_context(&avctx);
    av_dict_free(&opts);
    return ret;
  }

  AVDictionaryEntry* t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX);
  if (t) {
    ERROR_LOG() << "Option " << t->key << " not found.";
    avcodec_free_context(&avctx);
    av_dict_free(&opts);
    return AVERROR_OPTION_NOT_FOUND;
  }

  int sample_rate, nb_channels;
  int64_t channel_layout = 0;
  eof_ = false;
  stream->discard = AVDISCARD_DEFAULT;
  if (avctx->codec_type == AVMEDIA_TYPE_VIDEO) {
    AVRational frame_rate = av_guess_frame_rate(ic_, stream, NULL);
    bool opened = vstream_->Open(stream_index, stream, frame_rate);
    UNUSED(opened);
    PacketQueue* packet_queue = vstream_->Queue();
    video_frame_queue_ = new VideoFrameQueue<VIDEO_PICTURE_QUEUE_SIZE>(true);
    viddec_ = new VideoDecoder(avctx, packet_queue);
    viddec_->Start();
    if (!vdecoder_tid_->Start()) {
      destroy(&viddec_);
      goto out;
    }
  } else if (avctx->codec_type == AVMEDIA_TYPE_AUDIO) {
#if CONFIG_AVFILTER
    {
      audio_filter_src_.freq = avctx->sample_rate;
      audio_filter_src_.channels = avctx->channels;
      audio_filter_src_.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
      audio_filter_src_.fmt = avctx->sample_fmt;
      ret = ConfigureAudioFilters(opt_.afilters, 0);
      if (ret < 0) {
        avcodec_free_context(&avctx);
        av_dict_free(&opts);
        return ret;
      }
      AVFilterLink* link = out_audio_filter_->inputs[0];
      sample_rate = link->sample_rate;
      nb_channels = avfilter_link_get_channels(link);
      channel_layout = link->channel_layout;
    }
#else
    sample_rate = avctx->sample_rate;
    nb_channels = avctx->channels;
    channel_layout = avctx->channel_layout;
#endif

    int audio_buff_size = 0;
    bool audio_opened =
        handler_->HandleRequestAudio(this, channel_layout, nb_channels, sample_rate, &audio_tgt_, &audio_buff_size);
    if (!audio_opened) {
      avcodec_free_context(&avctx);
      av_dict_free(&opts);
      return ret;
    }

    audio_hw_buf_size_ = audio_buff_size;
    audio_src_ = audio_tgt_;
    audio_buf_size_ = 0;
    audio_buf_index_ = 0;

    /* init averaging filter */
    audio_diff_avg_coef_ = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
    DCHECK(0 <= audio_diff_avg_coef_ && audio_diff_avg_coef_ <= 1);
    audio_diff_avg_count_ = 0;
    /* since we do not have a precise anough audio FIFO fullness,
       we correct audio sync only if larger than this threshold */
    audio_diff_threshold_ =
        static_cast<double>(audio_hw_buf_size_) / static_cast<double>(audio_tgt_.bytes_per_sec) * 1000;
    bool opened = astream_->Open(stream_index, stream);
    UNUSED(opened);
    PacketQueue* packet_queue = astream_->Queue();
    audio_frame_queue_ = new AudioFrameQueue<SAMPLE_QUEUE_SIZE>(true);
    auddec_ = new AudioDecoder(avctx, packet_queue);
    if ((ic_->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) &&
        !ic_->iformat->read_seek) {
      auddec_->SetStartPts(stream->start_time, stream->time_base);
    }
    auddec_->Start();
    if (!adecoder_tid_->Start()) {
      destroy(&auddec_);
      goto out;
    }
  }
out:
  av_dict_free(&opts);
  return ret;
}

void VideoState::StreamComponentClose(int stream_index) {
  if (stream_index < 0 || static_cast<unsigned int>(stream_index) >= ic_->nb_streams) {
    return;
  }

  AVStream* avs = ic_->streams[stream_index];
  AVCodecParameters* codecpar = avs->codecpar;
  if (codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
    if (video_frame_queue_) {
      video_frame_queue_->Stop();
    }
    if (viddec_) {
      viddec_->Abort();
    }
    if (vdecoder_tid_) {
      vdecoder_tid_->Join();
      vdecoder_tid_ = NULL;
    }
    if (input_st_->hwaccel_uninit) {
      input_st_->hwaccel_uninit(viddec_->GetAvCtx());
      input_st_->hwaccel_uninit = NULL;
    }
    destroy(&viddec_);
    destroy(&video_frame_queue_);
  } else if (codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
    if (audio_frame_queue_) {
      audio_frame_queue_->Stop();
    }
    auddec_->Abort();
    if (adecoder_tid_) {
      adecoder_tid_->Join();
      adecoder_tid_ = NULL;
    }
    destroy(&auddec_);
    destroy(&audio_frame_queue_);
    swr_free(&swr_ctx_);
    av_freep(&audio_buf1_);
    audio_buf1_size_ = 0;
    audio_buf_ = NULL;
  }
  avs->discard = AVDISCARD_ALL;
}

void VideoState::StepToNextFrame() {
  /* if the stream is paused unpause it, then step */
  if (paused_) {
    StreamTogglePause();
  }
  step_ = true;
}

void VideoState::SeekNextChunk() {
  clock_t incr = 0;
  if (ic_->nb_chapters <= 1) {
    incr = 60000;
  }
  SeekChapter(1);
  Seek(incr);
}

void VideoState::SeekPrevChunk() {
  clock_t incr = 0;
  if (ic_->nb_chapters <= 1) {
    incr = -60000;
  }
  SeekChapter(-1);
  Seek(incr);
}

void VideoState::SeekChapter(int incr) {
  if (!ic_->nb_chapters) {
    return;
  }

  const AVRational tb = {1, AV_TIME_BASE};  // AV_TIME_BASE_Q
  const clock_t pos = GetMasterClock() * AV_TIME_BASE * 1000;
  int i;
  /* find the current chapter */
  for (i = 0; i < ic_->nb_chapters; i++) {
    AVChapter* ch = ic_->chapters[i];
    if (av_compare_ts(pos, tb, ch->start, ch->time_base) < 0) {
      i--;
      break;
    }
  }

  i += incr;
  unsigned int chapter = FFMAX(i, 0);
  if (chapter >= ic_->nb_chapters) {
    return;
  }

  DEBUG_LOG() << "Seeking to chapter " << chapter << ".";
  int64_t poss = av_rescale_q(ic_->chapters[chapter]->start, ic_->chapters[chapter]->time_base, tb);
  StreamSeek(poss, 0, false);
}

void VideoState::StreamSeek(int64_t pos, int64_t rel, bool seek_by_bytes) {
  if (seek_req_) {
    return;
  }

  seek_pos_ = pos;
  seek_rel_ = rel;
  seek_flags_ &= ~AVSEEK_FLAG_BYTE;
  if (seek_by_bytes) {
    seek_flags_ |= AVSEEK_FLAG_BYTE;
  }
  seek_req_ = true;
  read_thread_cond_.notify_one();
}

void VideoState::Seek(clock_t msec) {
  if (opt_.seek_by_bytes == SEEK_BY_BYTES_ON) {
    int64_t pos = -1;
    if (pos < 0 && vstream_->IsOpened()) {
      pos = video_frame_queue_->GetLastPos();
    }
    if (pos < 0 && astream_->IsOpened()) {
      pos = audio_frame_queue_->GetLastPos();
    }
    if (pos < 0) {
      pos = avio_tell(ic_->pb);
    }

    int64_t incr_in_bytes = 0;
    if (ic_->bit_rate) {
      incr_in_bytes = (msec / 1000) * ic_->bit_rate / 8.0;
    } else {
      incr_in_bytes = (msec / 1000) * 180000.0;
    }
    pos += incr_in_bytes;
    StreamSeek(pos, incr_in_bytes, true);
    return;
  }

  SeekMsec(msec);
}

void VideoState::SeekMsec(clock_t clock) {
  clock_t pos = GetMasterClock();
  if (!IsValidClock(pos)) {
    pos = seek_pos_ / AV_TIME_BASE * 1000;
  }
  pos += clock;
  if (ic_->start_time != AV_NOPTS_VALUE) {  // if selected out of range move to start
    clock_t st = ic_->start_time / AV_TIME_BASE * 1000;
    if (pos < st) {
      pos = st;
    }
  }

  int64_t pos_seek = pos * AV_TIME_BASE / 1000;
  int64_t incr_seek = clock * AV_TIME_BASE / 1000;
  StreamSeek(pos_seek, incr_seek, false);
}

AvSyncType VideoState::GetMasterSyncType() const {
  return opt_.av_sync_type;
}

clock_t VideoState::ComputeTargetDelay(clock_t delay) const {
  clock_t diff = 0;

  /* update delay to follow master synchronisation source */
  if (GetMasterSyncType() != AV_SYNC_VIDEO_MASTER) {
    /* if video is slave, we try to correct big delays by
       duplicating or deleting a frame */
    diff = vstream_->GetClock() - GetMasterClock();

    /* skip or repeat frame. We take into account the
       delay to compute the threshold. I still don't know
       if it is the best guess */
    clock_t sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN_MSEC, FFMIN(AV_SYNC_THRESHOLD_MAX_MSEC, delay));
    if (IsValidClock(diff) && std::abs(diff) < max_frame_duration_) {
      if (diff <= -sync_threshold) {
        delay = FFMAX(0, delay + diff);
      } else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD_MSEC) {
        delay = delay + diff;
      } else if (diff >= sync_threshold) {
        delay = 2 * delay;
      }
    }
  }
  DEBUG_LOG() << "video: delay=" << delay << " A-V=" << -diff;
  return delay;
}

/* get the current master clock value */
clock_t VideoState::GetMasterClock() const {
  if (GetMasterSyncType() == AV_SYNC_VIDEO_MASTER) {
    return vstream_->GetClock();
  }

  return astream_->GetClock();
}

int VideoState::VideoOpen(VideoFrame* vp) {
  if (vp && vp->width && vp->height) {
    handler_->HandleDefaultWindowSize(Size(vp->width, vp->height), vp->sar);
  }

  bool res = handler_->HandleRequestVideo(this);
  if (!res) {
    return ERROR_RESULT_VALUE;
  }

  return 0;
}

int VideoState::AllocPicture() {
  VideoFrame* vp = video_frame_queue_->Windex();

  int res = VideoOpen(vp);
  if (res == ERROR_RESULT_VALUE) {
    return ERROR_RESULT_VALUE;
  }

  if (!handler_->HandleReallocFrame(this, vp)) {
    return ERROR_RESULT_VALUE;
  }

  video_frame_queue_->ChangeSafeAndNotify([](VideoFrame* fr) { fr->allocated = true; }, vp);
  return SUCCESS_RESULT_VALUE;
}

/* display the current picture, if any */
void VideoState::VideoDisplay() {
  if (!vstream_->IsOpened()) {
    return;
  }

  if (!video_frame_queue_) {
    return;
  }

  VideoFrame* vp = video_frame_queue_->PeekLast();
  if (vp->bmp) {
    if (!vp->uploaded) {
      if (upload_texture(vp->bmp, vp->frame) < 0) {
        return;
      }
      vp->uploaded = true;
      vp->flip_v = vp->frame->linesize[0] < 0;
    }

    handler_->HanleDisplayFrame(this, vp);
    stats_->frame_processed++;
  }
}

int VideoState::Exec() {
  bool started = read_tid_->Start();
  if (!started) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

void VideoState::Abort() {
  abort_request_ = true;
  read_tid_->Join();
  Close();
  avformat_close_input(&ic_);
}

bool VideoState::IsReadThread() const {
  return common::threads::IsCurrentThread(read_tid_.get());
}

bool VideoState::IsVideoThread() const {
  return common::threads::IsCurrentThread(vdecoder_tid_.get());
}

bool VideoState::IsAudioThread() const {
  return common::threads::IsCurrentThread(adecoder_tid_.get());
}

bool VideoState::IsAborted() const {
  return abort_request_;
}

bool VideoState::IsStreamReady() const {
  if (!astream_ && !vstream_) {
    return false;
  }

  return IsAudioReady() && IsVideoReady() && !IsAborted();
}

stream_id VideoState::GetId() const {
  return id_;
}

const common::uri::Uri& VideoState::GetUri() const {
  return uri_;
}

void VideoState::StreamTogglePause() {
  if (paused_) {
    frame_timer_ += GetRealClockTime() - vstream_->LastUpdatedClock();
    if (read_pause_return_ != AVERROR(ENOSYS)) {
      vstream_->SetPaused(false);
    }
    vstream_->SyncSerialClock();
  }
  paused_ = !paused_;
  vstream_->SetPaused(paused_);
  astream_->SetPaused(paused_);
}

void VideoState::RefreshRequest() {
  force_refresh_ = true;
}

void VideoState::TogglePause() {
  StreamTogglePause();
  step_ = false;
}

void VideoState::Close() {
  /* close each stream */
  if (vstream_->IsOpened()) {
    StreamComponentClose(vstream_->Index());
    vstream_->Close();
  }
  if (astream_->IsOpened()) {
    StreamComponentClose(astream_->Index());
    astream_->Close();
  }
}

bool VideoState::IsAudioReady() const {
  return astream_ && (astream_->IsOpened() || opt_.disable_audio);
}

bool VideoState::IsVideoReady() const {
  return vstream_ && (vstream_->IsOpened() || opt_.disable_video);
}

void VideoState::StreamCycleChannel(AVMediaType codec_type) {
  int start_index = invalid_stream_index;
  int old_index = invalid_stream_index;
  if (codec_type == AVMEDIA_TYPE_VIDEO) {
    start_index = last_video_stream_;
    old_index = vstream_->Index();
  } else if (codec_type == AVMEDIA_TYPE_AUDIO) {
    start_index = last_audio_stream_;
    old_index = astream_->Index();
  } else {
    DNOTREACHED();
  }
  int stream_index = start_index;

  AVProgram* p = NULL;
  int lnb_streams = ic_->nb_streams;
  if (codec_type != AVMEDIA_TYPE_VIDEO && vstream_->IsOpened()) {
    p = av_find_program_from_stream(ic_, NULL, old_index);
    if (p) {
      lnb_streams = p->nb_stream_indexes;
      for (start_index = 0; start_index < lnb_streams; start_index++) {
        if (p->stream_index[start_index] == stream_index) {
          break;
        }
      }
      if (start_index == lnb_streams) {
        start_index = invalid_stream_index;
      }
      stream_index = start_index;
    }
  }

  while (true) {
    if (++stream_index >= lnb_streams) {
      if (start_index == invalid_stream_index) {
        return;
      }
      stream_index = 0;
    }
    if (stream_index == start_index) {
      return;
    }
    AVStream* st = ic_->streams[p ? p->stream_index[stream_index] : stream_index];
    if (st->codecpar->codec_type == codec_type) {
      if (codec_type == AVMEDIA_TYPE_AUDIO) {
        if (st->codecpar->sample_rate != 0 && st->codecpar->channels != 0) {
          goto the_end;
        }
      } else if (codec_type == AVMEDIA_TYPE_VIDEO || codec_type == AVMEDIA_TYPE_SUBTITLE) {
        goto the_end;
      }
    }
  }
the_end:
  if (p && stream_index != invalid_stream_index) {
    stream_index = p->stream_index[stream_index];
  }
  INFO_LOG() << "Switch " << av_get_media_type_string(static_cast<AVMediaType>(codec_type)) << "  stream from #"
             << old_index << " to #" << stream_index;
  StreamComponentClose(old_index);
  StreamComponentOpen(stream_index);
}

int VideoState::HandleAllocPictureEvent() {
  return AllocPicture();
}

int VideoState::SynchronizeAudio(int nb_samples) {
  int wanted_nb_samples = nb_samples;

  /* if not master, then we try to remove or add samples to correct the clock */
  if (GetMasterSyncType() != AV_SYNC_AUDIO_MASTER) {
    clock_t diff = astream_->GetClock() - GetMasterClock();
    if (IsValidClock(diff) && std::abs(diff) < AV_NOSYNC_THRESHOLD_MSEC) {
      audio_diff_cum_ = diff + audio_diff_avg_coef_ * audio_diff_cum_;
      if (audio_diff_avg_count_ < AUDIO_DIFF_AVG_NB) {
        /* not enough measures to have a correct estimate */
        audio_diff_avg_count_++;
      } else {
        /* estimate the A-V difference */
        double avg_diff = audio_diff_cum_ * (1.0 - audio_diff_avg_coef_);
        if (fabs(avg_diff) >= audio_diff_threshold_) {
          wanted_nb_samples = nb_samples + static_cast<int>(diff * audio_src_.freq);
          int min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
          int max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
          wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
        }
        DEBUG_LOG() << "diff=" << diff << " adiff=" << avg_diff << " sample_diff=" << wanted_nb_samples - nb_samples
                    << " apts=" << audio_clock_ << " " << audio_diff_threshold_;
      }
    } else {
      /* too big difference : may be initial PTS errors, so
         reset A-V filter */
      audio_diff_avg_count_ = 0;
      audio_diff_cum_ = 0;
    }
  }

  return wanted_nb_samples;
}

int VideoState::AudioDecodeFrame() {
  if (paused_) {
    return ERROR_RESULT_VALUE;
  }

  if (!IsAudioReady()) {
    return ERROR_RESULT_VALUE;
  }

  AudioFrame* af = audio_frame_queue_->GetPeekReadable();
  if (!af) {
    return ERROR_RESULT_VALUE;
  }
  audio_frame_queue_->MoveToNext();

  const AVSampleFormat sample_fmt = static_cast<AVSampleFormat>(af->frame->format);
  int data_size =
      av_samples_get_buffer_size(NULL, av_frame_get_channels(af->frame), af->frame->nb_samples, sample_fmt, 1);
  int64_t dec_channel_layout =
      (af->frame->channel_layout &&
       av_frame_get_channels(af->frame) == av_get_channel_layout_nb_channels(af->frame->channel_layout))
          ? af->frame->channel_layout
          : av_get_default_channel_layout(av_frame_get_channels(af->frame));
  int wanted_nb_samples = SynchronizeAudio(af->frame->nb_samples);

  if (af->frame->format != audio_src_.fmt || dec_channel_layout != audio_src_.channel_layout ||
      af->frame->sample_rate != audio_src_.freq || (wanted_nb_samples != af->frame->nb_samples && !swr_ctx_)) {
    swr_free(&swr_ctx_);
    swr_ctx_ = swr_alloc_set_opts(NULL, audio_tgt_.channel_layout, audio_tgt_.fmt, audio_tgt_.freq, dec_channel_layout,
                                  sample_fmt, af->frame->sample_rate, 0, NULL);
    if (!swr_ctx_ || swr_init(swr_ctx_) < 0) {
      ERROR_LOG() << "Cannot create sample rate converter for conversion of " << af->frame->sample_rate << " Hz "
                  << av_get_sample_fmt_name(sample_fmt) << " " << av_frame_get_channels(af->frame) << " channels to "
                  << audio_tgt_.freq << " Hz " << av_get_sample_fmt_name(audio_tgt_.fmt) << " " << audio_tgt_.channels
                  << " channels!";
      swr_free(&swr_ctx_);
      return ERROR_RESULT_VALUE;
    }
    audio_src_.channel_layout = dec_channel_layout;
    audio_src_.channels = av_frame_get_channels(af->frame);
    audio_src_.freq = af->frame->sample_rate;
    audio_src_.fmt = sample_fmt;
  }

  int resampled_data_size = 0;
  if (swr_ctx_) {
    const uint8_t** in = const_cast<const uint8_t**>(af->frame->extended_data);
    uint8_t** out = &audio_buf1_;
    int out_count = wanted_nb_samples * audio_tgt_.freq / af->frame->sample_rate + 256;
    int out_size = av_samples_get_buffer_size(NULL, audio_tgt_.channels, out_count, audio_tgt_.fmt, 0);
    if (out_size < 0) {
      ERROR_LOG() << "av_samples_get_buffer_size() failed";
      return ERROR_RESULT_VALUE;
    }
    if (wanted_nb_samples != af->frame->nb_samples) {
      if (swr_set_compensation(swr_ctx_,
                               (wanted_nb_samples - af->frame->nb_samples) * audio_tgt_.freq / af->frame->sample_rate,
                               wanted_nb_samples * audio_tgt_.freq / af->frame->sample_rate) < 0) {
        ERROR_LOG() << "swr_set_compensation() failed";
        return ERROR_RESULT_VALUE;
      }
    }
    av_fast_malloc(&audio_buf1_, &audio_buf1_size_, out_size);
    if (!audio_buf1_) {
      return AVERROR(ENOMEM);
    }
    int len2 = swr_convert(swr_ctx_, out, out_count, in, af->frame->nb_samples);
    if (len2 < 0) {
      ERROR_LOG() << "swr_convert() failed";
      return -1;
    }
    if (len2 == out_count) {
      WARNING_LOG() << "audio buffer is probably too small";
      if (swr_init(swr_ctx_) < 0) {
        swr_free(&swr_ctx_);
      }
    }
    audio_buf_ = audio_buf1_;
    resampled_data_size = len2 * audio_tgt_.channels * av_get_bytes_per_sample(audio_tgt_.fmt);
  } else {
    audio_buf_ = af->frame->data[0];
    resampled_data_size = data_size;
  }

  /* update the audio clock with the pts */
  if (IsValidClock(af->pts)) {
    const double div = static_cast<double>(af->frame->nb_samples) / af->frame->sample_rate;
    const clock_t dur = div * 1000;
    audio_clock_ = af->pts + dur;
  } else {
    audio_clock_ = invalid_clock();
  }
  return resampled_data_size;
}

void VideoState::RefreshVideo() {
retry:
  if (video_frame_queue_->IsEmpty()) {
    // nothing to do, no picture to display in the queue
  } else {
    /* dequeue the picture */
    VideoFrame* lastvp = video_frame_queue_->PeekLast();
    VideoFrame* vp = video_frame_queue_->Peek();

    if (frame_timer_ == 0) {
      frame_timer_ = GetRealClockTime();
    }

    if (paused_) {
      goto display;
    }

    /* compute nominal last_duration */
    clock_t last_duration = CalcDurationBetweenVideoFrames(lastvp, vp, max_frame_duration_);
    clock_t delay = ComputeTargetDelay(last_duration);
    clock_t time = GetRealClockTime();
    clock_t next_frame_ts = frame_timer_ + delay;
    if (time < next_frame_ts) {
      goto display;
    }

    frame_timer_ = next_frame_ts;
    if (delay > 0) {
      if (time - frame_timer_ > AV_SYNC_THRESHOLD_MAX_MSEC) {
        frame_timer_ = time;
      }
    }

    const clock_t pts = vp->pts;
    if (IsValidClock(pts)) {
      /* update current video pts */
      vstream_->SetClockAt(pts, time);
    }

    VideoFrame* nextvp = video_frame_queue_->PeekNextOrNull();
    if (nextvp) {
      clock_t duration = CalcDurationBetweenVideoFrames(vp, nextvp, max_frame_duration_);
      if (!step_ && (opt_.framedrop == FRAME_DROP_AUTO ||
                     (opt_.framedrop == FRAME_DROP_ON || (GetMasterSyncType() != AV_SYNC_VIDEO_MASTER)))) {
        clock_t next_next_frame_ts = frame_timer_ + duration;
        clock_t diff_drop = time - next_next_frame_ts;
        if (diff_drop > 0) {
          stats_->frame_drops_late++;
          video_frame_queue_->MoveToNext();
          goto retry;
        }
      }
    }

    video_frame_queue_->MoveToNext();
    force_refresh_ = true;

    if (step_ && !paused_) {
      StreamTogglePause();
    }
  }
display:
  /* display picture */
  if (force_refresh_ && video_frame_queue_->RindexShown()) {
    VideoDisplay();
  }
}

void VideoState::TryRefreshVideo() {
  if (!paused_ || force_refresh_) {
    AVStream* video_st = vstream_->IsOpened() ? vstream_->AvStream() : NULL;
    AVStream* audio_st = astream_->IsOpened() ? astream_->AvStream() : NULL;
    PacketQueue* video_packet_queue = vstream_->Queue();
    PacketQueue* audio_packet_queue = astream_->Queue();

    if (video_st && video_frame_queue_) {
      RefreshVideo();
    }
    force_refresh_ = false;

    int aqsize = 0, vqsize = 0;
    bandwidth_t video_bandwidth = 0, audio_bandwidth = 0;
    if (video_st) {
      vqsize = video_packet_queue->Size();
      video_bandwidth = vstream_->Bandwidth();
    }
    if (audio_st) {
      aqsize = audio_packet_queue->Size();
      audio_bandwidth = astream_->Bandwidth();
    }

    stats_->master_clock = GetMasterClock();
    stats_->video_clock = vstream_->GetClock();
    stats_->audio_clock = astream_->GetClock();
    stats_->fmt = (audio_st && video_st)
                      ? (HAVE_VIDEO_STREAM | HAVE_AUDIO_STREAM)
                      : (video_st ? HAVE_VIDEO_STREAM : (audio_st ? HAVE_AUDIO_STREAM : UNKNOWN_STREAM));
    stats_->audio_queue_size = aqsize;
    stats_->video_queue_size = vqsize;
    stats_->audio_bandwidth = audio_bandwidth;
    stats_->video_bandwidth = video_bandwidth;
  }
}

VideoState::stats_t VideoState::GetStatistic() const {
  return stats_;
}

void VideoState::UpdateAudioBuffer(uint8_t* stream, int len, int audio_volume) {
  if (!IsStreamReady()) {
    return;
  }

  const clock_t audio_callback_time = GetRealClockTime();
  while (len > 0) {
    if (audio_buf_index_ >= audio_buf_size_) {
      int audio_size = AudioDecodeFrame();
      if (audio_size < 0) {
        /* if error, just output silence */
        audio_buf_ = NULL;
        audio_buf_size_ = AUDIO_MIN_BUFFER_SIZE / audio_tgt_.frame_size * audio_tgt_.frame_size;
      } else {
        audio_buf_size_ = audio_size;
      }
      audio_buf_index_ = 0;
    }
    int len1 = audio_buf_size_ - audio_buf_index_;
    if (len1 > len) {
      len1 = len;
    }
    if (audio_buf_ && audio_volume == 100) {
      memcpy(stream, audio_buf_ + audio_buf_index_, len1);
    } else {
      memset(stream, 0, len1);
      if (audio_buf_) {
        handler_->HanleAudioMix(stream, audio_buf_ + audio_buf_index_, len1, audio_volume);
      }
    }
    len -= len1;
    stream += len1;
    audio_buf_index_ += len1;
  }
  audio_write_buf_size_ = audio_buf_size_ - audio_buf_index_;
  /* Let's assume the audio driver that is used by SDL has two periods. */
  if (IsValidClock(audio_clock_)) {
    double clc = static_cast<double>(2 * audio_hw_buf_size_ + audio_write_buf_size_) /
                 static_cast<double>(audio_tgt_.bytes_per_sec) * 1000;
    const clock_t pts = audio_clock_ - clc;
    astream_->SetClockAt(pts, audio_callback_time);
  }
}

int VideoState::QueuePicture(AVFrame* src_frame, clock_t pts, clock_t duration, int64_t pos) {
  PacketQueue* video_packet_queue = vstream_->Queue();
  VideoFrame* vp = video_frame_queue_->GetPeekWritable();
  if (!vp) {
    return ERROR_RESULT_VALUE;
  }

  vp->sar = src_frame->sample_aspect_ratio;
  vp->uploaded = false;

  /* alloc or resize hardware picture buffer */
  if (!vp->bmp || !vp->allocated || vp->width != src_frame->width || vp->height != src_frame->height ||
      vp->format != src_frame->format) {
    vp->allocated = false;
    vp->width = src_frame->width;
    vp->height = src_frame->height;
    vp->format = src_frame->format;

    /* the allocation must be done in the main thread to avoid
       locking problems. */
    events::AllocFrameEvent* event = new events::AllocFrameEvent(this, events::FrameInfo(this, vp));
    fApp->PostEvent(event);

    video_frame_queue_->WaitSafeAndNotify(
        [video_packet_queue, vp]() -> bool { return !vp->allocated && !video_packet_queue->IsAborted(); });

    if (video_packet_queue->IsAborted()) {
      return ERROR_RESULT_VALUE;
    }
  }

  /* if the frame is not skipped, then display it */
  if (vp->bmp) {
    vp->pts = pts;
    vp->duration = duration;
    vp->pos = pos;

    av_frame_move_ref(vp->frame, src_frame);
    video_frame_queue_->Push();
  }
  return SUCCESS_RESULT_VALUE;
}

int VideoState::GetVideoFrame(AVFrame* frame) {
  int got_picture = viddec_->DecodeFrame(frame);
  if (got_picture < 0) {
    return ERROR_RESULT_VALUE;
  }

  if (got_picture) {
    frame->sample_aspect_ratio = av_guess_sample_aspect_ratio(ic_, vstream_->AvStream(), frame);

    if (opt_.framedrop == FRAME_DROP_AUTO || (opt_.framedrop || GetMasterSyncType() != AV_SYNC_VIDEO_MASTER)) {
      if (IsValidPts(frame->pts)) {
        clock_t dpts = vstream_->q2d() * frame->pts;
        clock_t diff = dpts - GetMasterClock();
        PacketQueue* video_packet_queue = vstream_->Queue();
        if (IsValidClock(diff) && std::abs(diff) < AV_NOSYNC_THRESHOLD_MSEC && diff - frame_last_filter_delay_ < 0 &&
            video_packet_queue->NbPackets()) {
          stats_->frame_drops_early++;
          av_frame_unref(frame);
          got_picture = 0;
        }
      }
    }
  }

  return got_picture;
}

/* this thread gets the stream from the disk or the network */
int VideoState::ReadThread() {
  AVFormatContext* ic = avformat_alloc_context();
  if (!ic) {
    const int av_errno = AVERROR(ENOMEM);
    common::Error err = common::make_error_value_errno(av_errno, common::Value::E_ERROR);
    events::QuitStreamEvent* qevent = new events::QuitStreamEvent(this, events::QuitStreamInfo(this, av_errno, err));
    fApp->PostEvent(qevent);
    return ERROR_RESULT_VALUE;
  }

  bool scan_all_pmts_set = false;
  std::string uri_str;
  if (uri_.Scheme() == common::uri::Uri::file) {
    common::uri::Upath upath = uri_.Path();
    uri_str = upath.Path();
  } else {
    uri_str = uri_.Url();
  }
  const char* in_filename = common::utils::c_strornull(uri_str);
  ic->interrupt_callback.callback = decode_interrupt_callback;
  ic->interrupt_callback.opaque = this;
  if (!av_dict_get(copt_.format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE)) {
    av_dict_set(&copt_.format_opts, "scan_all_pmts", "1", AV_DICT_DONT_OVERWRITE);
    scan_all_pmts_set = true;
  }

  int open_result = avformat_open_input(&ic, in_filename, NULL, &copt_.format_opts);  // autodetect format
  if (open_result < 0) {
    std::string err_str = ffmpeg_errno_to_string(open_result);
    common::Error err = common::make_error_value(err_str, common::Value::E_ERROR);
    avformat_close_input(&ic);
    events::QuitStreamEvent* qevent = new events::QuitStreamEvent(this, events::QuitStreamInfo(this, open_result, err));
    fApp->PostEvent(qevent);
    return ERROR_RESULT_VALUE;
  }
  if (scan_all_pmts_set) {
    av_dict_set(&copt_.format_opts, "scan_all_pmts", NULL, AV_DICT_MATCH_CASE);
  }

  /*AVDictionaryEntry* t = av_dict_get(copt_->format_opts, "", NULL, AV_DICT_IGNORE_SUFFIX);
  if (t) {
    ERROR_LOG() << "Option " << t->key << " not found.";
    avformat_close_input(&ic);
    SDL_Event event;
    event.type = FF_QUIT_EVENT;
    event.user.data1 = this;
    event.user.code = AVERROR_OPTION_NOT_FOUND;
    SDL_PushEvent(&event);
    return AVERROR_OPTION_NOT_FOUND;
  }*/
  ic_ = ic;

  VideoStream* video_stream = vstream_;
  AudioStream* audio_stream = astream_;
  PacketQueue* video_packet_queue = video_stream->Queue();
  PacketQueue* audio_packet_queue = audio_stream->Queue();
  int st_index[AVMEDIA_TYPE_NB];
  memset(st_index, -1, sizeof(st_index));

  if (opt_.genpts) {
    ic->flags |= AVFMT_FLAG_GENPTS;
  }

  av_format_inject_global_side_data(ic);

  AVDictionary** opts = setup_find_stream_info_opts(ic, copt_.codec_opts);
  unsigned int orig_nb_streams = ic->nb_streams;

  int find_stream_info_result = avformat_find_stream_info(ic, opts);

  for (unsigned int i = 0; i < orig_nb_streams; i++) {
    av_dict_free(&opts[i]);
  }
  av_freep(&opts);

  AVPacket pkt1, *pkt = &pkt1;
  if (find_stream_info_result < 0) {
    std::string err_str = ffmpeg_errno_to_string(find_stream_info_result);
    common::Error err = common::make_error_value(err_str, common::Value::E_ERROR);
    events::QuitStreamEvent* qevent = new events::QuitStreamEvent(this, events::QuitStreamInfo(this, -1, err));
    fApp->PostEvent(qevent);
    return ERROR_RESULT_VALUE;
  }

  if (ic->pb) {
    ic->pb->eof_reached = 0;  // FIXME hack, ffplay maybe should not use avio_feof() to test for the end
  }

  max_frame_duration_ = (ic->iformat->flags & AVFMT_TS_DISCONT) ? 10000 : 3600000;

  if (opt_.seek_by_bytes == SEEK_AUTO) {
    bool seek = (ic->iformat->flags & AVFMT_TS_DISCONT) && strcmp("ogg", ic->iformat->name);
    opt_.seek_by_bytes = seek ? SEEK_BY_BYTES_ON : SEEK_BY_BYTES_OFF;
  }

  realtime_ = is_realtime(ic);

  // av_dump_format(ic, 0, id_.c_str(), 0);

  for (int i = 0; i < static_cast<int>(ic->nb_streams); i++) {
    AVStream* st = ic->streams[i];
    enum AVMediaType type = st->codecpar->codec_type;
    st->discard = AVDISCARD_ALL;
    const char* want_spec = common::utils::c_strornull(opt_.wanted_stream_spec[type]);
    if (type >= 0 && want_spec && st_index[type] == -1) {
      if (avformat_match_stream_specifier(ic, st, want_spec) > 0) {
        st_index[type] = i;
      }
    }
  }
  for (int i = 0; i < static_cast<int>(AVMEDIA_TYPE_NB); i++) {
    const char* want_spec = common::utils::c_strornull(opt_.wanted_stream_spec[i]);
    if (want_spec && st_index[i] == -1) {
      ERROR_LOG() << "Stream specifier " << want_spec << " does not match any "
                  << av_get_media_type_string(static_cast<AVMediaType>(i)) << " stream";
      st_index[i] = INT_MAX;
    }
  }

  st_index[AVMEDIA_TYPE_VIDEO] = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, st_index[AVMEDIA_TYPE_VIDEO], -1, NULL, 0);

  st_index[AVMEDIA_TYPE_AUDIO] =
      av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, st_index[AVMEDIA_TYPE_AUDIO], st_index[AVMEDIA_TYPE_VIDEO], NULL, 0);

  if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
    AVStream* st = ic->streams[st_index[AVMEDIA_TYPE_VIDEO]];
    AVCodecParameters* codecpar = st->codecpar;
    AVRational sar = av_guess_sample_aspect_ratio(ic, st, NULL);
    if (codecpar->width && codecpar->height) {
      handler_->HandleDefaultWindowSize(Size(codecpar->width, codecpar->height), sar);
    }
  }

  /* open the streams */
  if (st_index[AVMEDIA_TYPE_AUDIO] >= 0) {
    int res_audio = StreamComponentOpen(st_index[AVMEDIA_TYPE_AUDIO]);
    if (res_audio < 0) {
      WARNING_LOG() << "Failed to open audio stream";
    }
  }

  if (st_index[AVMEDIA_TYPE_VIDEO] >= 0) {
    int res_video = StreamComponentOpen(st_index[AVMEDIA_TYPE_VIDEO]);
    if (res_video < 0) {
      WARNING_LOG() << "Failed to open video stream";
    }
  }

  /*if (!IsStreamReady()) {
    common::Error err = common::make_error_value("Failed to open stream, or configure filtergraph.",
                                                 common::Value::E_ERROR);
    events::QuitStreamEvent* qevent =
        new events::QuitStreamEvent(this, events::QuitStreamInfo(this, -1, err));
    fApp->PostEvent(qevent);
    return ERROR_RESULT_VALUE;
  }*/

  DesireBytesPerSec video_bandwidth_calc = video_stream->DesireBandwith();
  DCHECK(video_bandwidth_calc.IsValid());
  DesireBytesPerSec audio_bandwidth_calc = audio_stream->DesireBandwith();
  DCHECK(audio_bandwidth_calc.IsValid());
  const DesireBytesPerSec band = video_bandwidth_calc + audio_bandwidth_calc;
  if (ic_->bit_rate) {
    bandwidth_t byte_per_sec = ic_->bit_rate / 8;
    DCHECK(band.InRange(byte_per_sec));
  }
  if (opt_.infinite_buffer < 0 && realtime_) {
    opt_.infinite_buffer = 1;
  }

  AVStream* const video_st = video_stream->AvStream();
  AVStream* const audio_st = audio_stream->AvStream();
  UNUSED(audio_st);
  stats_.reset(new Stats);

  while (!IsAborted()) {
    if (paused_ != last_paused_) {
      last_paused_ = paused_;
      if (paused_) {
        read_pause_return_ = av_read_pause(ic);
      } else {
        av_read_play(ic);
      }
    }

    if (seek_req_) {
      int64_t seek_target = seek_pos_;
      int64_t seek_min = seek_rel_ > 0 ? seek_target - seek_rel_ + 2 : INT64_MIN;
      int64_t seek_max = seek_rel_ < 0 ? seek_target - seek_rel_ - 2 : INT64_MAX;
      // FIXME the +-2 is due to rounding being not done in the correct direction in generation
      //      of the seek_pos/seek_rel variables

      int ret = avformat_seek_file(ic, -1, seek_min, seek_target, seek_max, seek_flags_);
      if (ret < 0) {
        ERROR_LOG() << "Seeking " << id_ << "failed error: " << ffmpeg_errno_to_string(ret);
      } else {
        if (video_stream->IsOpened()) {
          video_packet_queue->PutNullpacket(video_stream->Index());
        }
        if (audio_stream->IsOpened()) {
          audio_packet_queue->PutNullpacket(audio_stream->Index());
        }
      }
      seek_req_ = false;
      eof_ = false;
      if (paused_) {
        StepToNextFrame();
      }
    }

    /* if the queue are full, no need to read more */
    if (opt_.infinite_buffer < 1 && (video_packet_queue->Size() + audio_packet_queue->Size() > MAX_QUEUE_SIZE ||
                                     (astream_->HasEnoughPackets() && vstream_->HasEnoughPackets()))) {
      common::unique_lock<common::mutex> lock(read_thread_mutex_);
      std::cv_status interrupt_status = read_thread_cond_.wait_for(lock, std::chrono::milliseconds(10));
      if (interrupt_status == std::cv_status::no_timeout) {  // if notify
      }
      continue;
    }
    if (!paused_ && eof_) {
      bool is_audio_dec_ready = auddec_ && audio_frame_queue_;
      bool is_audio_not_finished_but_empty = false;
      if (is_audio_dec_ready) {
        is_audio_not_finished_but_empty = !auddec_->IsFinished() && audio_frame_queue_->IsEmpty();
      }
      bool is_video_dec_ready = viddec_ && video_frame_queue_;
      bool is_video_not_finished_but_empty = false;
      if (is_video_dec_ready) {
        is_video_not_finished_but_empty = !viddec_->IsFinished() && video_frame_queue_->IsEmpty();
      }
      if ((!is_audio_dec_ready || (is_audio_dec_ready && is_audio_not_finished_but_empty)) &&
          (!is_video_dec_ready || (is_video_dec_ready && is_video_not_finished_but_empty)) && opt_.auto_exit) {
        int errn = AVERROR_EOF;
        std::string err_str = ffmpeg_errno_to_string(errn);
        common::Error err = common::make_error_value(err_str, common::Value::E_ERROR);
        events::QuitStreamEvent* qevent = new events::QuitStreamEvent(this, events::QuitStreamInfo(this, errn, err));
        fApp->PostEvent(qevent);
        return ERROR_RESULT_VALUE;
      }
    }
    int ret = av_read_frame(ic, pkt);
    if (ret < 0) {
      WARNING_LOG() << "Read input stream error: " << ffmpeg_errno_to_string(ret);
      bool is_eof = ret == AVERROR_EOF;
      bool is_feof = avio_feof(ic->pb);
      if ((is_eof || is_feof) && !eof_) {
        if (video_stream->IsOpened()) {
          video_packet_queue->PutNullpacket(video_stream->Index());
        }
        if (audio_stream->IsOpened()) {
          audio_packet_queue->PutNullpacket(audio_stream->Index());
        }
        eof_ = true;
      }
      if (ic->pb && ic->pb->error) {
        break;
      }
      continue;
    } else {
      eof_ = false;
    }

    if (pkt->stream_index == audio_stream->Index()) {
      audio_stream->RegisterPacket(pkt);
      audio_packet_queue->Put(pkt);
    } else if (pkt->stream_index == video_stream->Index()) {
      if (video_st->disposition & AV_DISPOSITION_ATTACHED_PIC) {
        av_packet_unref(pkt);
      } else {
        video_stream->RegisterPacket(pkt);
        video_packet_queue->Put(pkt);
      }
    } else {
      av_packet_unref(pkt);
    }
  }

  events::QuitStreamEvent* qevent = new events::QuitStreamEvent(this, events::QuitStreamInfo(this, 0, common::Error()));
  fApp->PostEvent(qevent);
  return SUCCESS_RESULT_VALUE;
}

int VideoState::AudioThread() {
  AudioFrame* af = nullptr;
#if CONFIG_AVFILTER
  int64_t dec_channel_layout;
  int reconfigure;
#endif
  int ret = 0;

  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    return AVERROR(ENOMEM);
  }

  do {
    int got_frame = auddec_->DecodeFrame(frame);
    if (got_frame < 0) {
      break;
    }

    if (got_frame) {
      AVRational tb = {1, frame->sample_rate};

#if CONFIG_AVFILTER
      dec_channel_layout = get_valid_channel_layout(frame->channel_layout, av_frame_get_channels(frame));

      reconfigure = cmp_audio_fmts(audio_filter_src_.fmt, audio_filter_src_.channels,
                                   static_cast<AVSampleFormat>(frame->format), av_frame_get_channels(frame)) ||
                    audio_filter_src_.channel_layout != dec_channel_layout ||
                    audio_filter_src_.freq != frame->sample_rate;

      if (reconfigure) {
        char buf1[1024], buf2[1024];
        av_get_channel_layout_string(buf1, sizeof(buf1), -1, audio_filter_src_.channel_layout);
        av_get_channel_layout_string(buf2, sizeof(buf2), -1, dec_channel_layout);
        const std::string mess = common::MemSPrintf(
            "Audio frame changed from rate:%d ch:%d fmt:%s layout:%s serial:%d to rate:%d ch:%d "
            "fmt:%s layout:%s serial:%d\n",
            audio_filter_src_.freq, audio_filter_src_.channels, av_get_sample_fmt_name(audio_filter_src_.fmt), buf1, 0,
            frame->sample_rate, av_frame_get_channels(frame),
            av_get_sample_fmt_name(static_cast<AVSampleFormat>(frame->format)), buf2, 0);
        DEBUG_LOG() << mess;

        audio_filter_src_.fmt = static_cast<AVSampleFormat>(frame->format);
        audio_filter_src_.channels = av_frame_get_channels(frame);
        audio_filter_src_.channel_layout = dec_channel_layout;
        audio_filter_src_.freq = frame->sample_rate;

        if ((ret = ConfigureAudioFilters(opt_.afilters, 1)) < 0) {
          break;
        }
      }

      ret = av_buffersrc_add_frame(in_audio_filter_, frame);
      if (ret < 0) {
        break;
      }

      while ((ret = av_buffersink_get_frame_flags(out_audio_filter_, frame, 0)) >= 0) {
        tb = out_audio_filter_->inputs[0]->time_base;
#endif
        af = audio_frame_queue_->GetPeekWritable();
        if (!af) {  // if stoped
#if CONFIG_AVFILTER
          avfilter_graph_free(&agraph_);
#endif
          av_frame_free(&frame);
          return ret;
        }

        af->pts = IsValidPts(frame->pts) ? frame->pts * q2d_diff(tb) : invalid_clock();
        af->pos = av_frame_get_pkt_pos(frame);
        AVRational tmp = {frame->nb_samples, frame->sample_rate};
        af->duration = q2d_diff(tmp);

        av_frame_move_ref(af->frame, frame);
        audio_frame_queue_->Push();

#if CONFIG_AVFILTER
      }
      if (ret == AVERROR_EOF) {
        auddec_->SetFinished(true);
      }
#endif
    }
  } while (ret >= 0 || ret == AVERROR(EAGAIN) || ret == AVERROR_EOF);

#if CONFIG_AVFILTER
  avfilter_graph_free(&agraph_);
#endif
  av_frame_free(&frame);
  return ret;
}

int VideoState::VideoThread() {
  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    return AVERROR(ENOMEM);
  }

#if CONFIG_AVFILTER
  AVFilterGraph* graph = avfilter_graph_alloc();
  AVFilterContext *filt_out = NULL, *filt_in = NULL;
  int last_w = 0;
  int last_h = 0;
  enum AVPixelFormat last_format = AV_PIX_FMT_NONE;  //-2
  if (!graph) {
    av_frame_free(&frame);
    return AVERROR(ENOMEM);
  }
#endif

  AVStream* video_st = vstream_->AvStream();
  AVRational tb = video_st->time_base;
  AVRational frame_rate = av_guess_frame_rate(ic_, video_st, NULL);
  while (true) {
    int ret = GetVideoFrame(frame);
    if (ret < 0) {
      goto the_end;
    }
    if (!ret) {
      continue;
    }

    if (input_st_->hwaccel_retrieve_data && frame->format == input_st_->hwaccel_pix_fmt) {
      int err = input_st_->hwaccel_retrieve_data(viddec_->GetAvCtx(), frame);
      if (err < 0) {
        continue;
      }
    }
    input_st_->hwaccel_retrieved_pix_fmt = static_cast<AVPixelFormat>(frame->format);

#if CONFIG_AVFILTER
    if (last_w != frame->width || last_h != frame->height || last_format != frame->format) {  // -vf option
      const std::string mess = common::MemSPrintf(
          "Video frame changed from size:%dx%d format:%s serial:%d to size:%dx%d format:%s "
          "serial:%d",
          last_w, last_h, static_cast<const char*>(av_x_if_null(av_get_pix_fmt_name(last_format), "none")), 0,
          frame->width, frame->height,
          static_cast<const char*>(
              av_x_if_null(av_get_pix_fmt_name(static_cast<AVPixelFormat>(frame->format)), "none")),
          0);
      DEBUG_LOG() << mess;
      avfilter_graph_free(&graph);
      graph = avfilter_graph_alloc();
      const std::string vfilters = opt_.vfilters;
      int ret = ConfigureVideoFilters(graph, vfilters, frame);
      if (ret < 0) {
        ERROR_LOG() << "Internal video error!";
        goto the_end;
      }
      filt_in = in_video_filter_;
      filt_out = out_video_filter_;
      last_w = frame->width;
      last_h = frame->height;
      last_format = static_cast<AVPixelFormat>(frame->format);
      frame_rate = filt_out->inputs[0]->frame_rate;
    }

    ret = av_buffersrc_add_frame(filt_in, frame);
    if (ret < 0) {
      goto the_end;
    }

    while (ret >= 0) {
      frame_last_returned_time_ = GetRealClockTime();

      ret = av_buffersink_get_frame_flags(filt_out, frame, 0);
      if (ret < 0) {
        if (ret == AVERROR_EOF) {
          viddec_->SetFinished(true);
        }
        ret = 0;
        break;
      }

      frame_last_filter_delay_ = GetRealClockTime() - frame_last_returned_time_;
      if (std::abs(frame_last_filter_delay_) > AV_NOSYNC_THRESHOLD_MSEC) {
        frame_last_filter_delay_ = 0;
      }
      if (filt_out) {
        tb = filt_out->inputs[0]->time_base;
      }
#endif
      AVRational fr = {frame_rate.den, frame_rate.num};
      clock_t duration = (frame_rate.num && frame_rate.den ? q2d_diff(fr) : 0);
      clock_t pts = IsValidPts(frame->pts) ? frame->pts * q2d_diff(tb) : invalid_clock();
      ret = QueuePicture(frame, pts, duration, av_frame_get_pkt_pos(frame));
      av_frame_unref(frame);
#if CONFIG_AVFILTER
    }
#endif

    if (ret < 0) {
      goto the_end;
    }
  }
the_end:
#if CONFIG_AVFILTER
  avfilter_graph_free(&graph);
#endif
  av_frame_free(&frame);
  return 0;
}

#if CONFIG_AVFILTER
int VideoState::ConfigureVideoFilters(AVFilterGraph* graph, const std::string& vfilters, AVFrame* frame) {
  static const enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_BGRA, AV_PIX_FMT_NONE};
  AVDictionary* sws_dict = copt_.sws_dict;
  AVDictionaryEntry* e = NULL;
  char sws_flags_str[512] = {0};
  while ((e = av_dict_get(sws_dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
    if (strcmp(e->key, "sws_flags") == 0) {
      av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", e->value);
    } else {
      av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", e->key, e->value);
    }
  }
  const size_t len_sws_flags = strlen(sws_flags_str);
  if (len_sws_flags) {
    sws_flags_str[len_sws_flags - 1] = 0;
  }

  AVStream* video_st = vstream_->AvStream();
  if (!video_st) {
    DNOTREACHED();
    return ERROR_RESULT_VALUE;
  }
  AVCodecParameters* codecpar = video_st->codecpar;

  char buffersrc_args[256];
  AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
  graph->scale_sws_opts = av_strdup(sws_flags_str);
  snprintf(buffersrc_args, sizeof(buffersrc_args), "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
           frame->width, frame->height, frame->format, video_st->time_base.num, video_st->time_base.den,
           codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));
  AVRational fr = av_guess_frame_rate(ic_, video_st, NULL);
  if (fr.num && fr.den) {
    av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);
  }

  int ret = avfilter_graph_create_filter(&filt_src, avfilter_get_by_name("buffer"), "ffplay_buffer", buffersrc_args,
                                         NULL, graph);
  if (ret < 0) {
    return ret;
  }

  ret = avfilter_graph_create_filter(&filt_out, avfilter_get_by_name("buffersink"), "ffplay_buffersink", NULL, NULL,
                                     graph);
  if (ret < 0) {
    return ret;
  }
  ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
  if (ret < 0) {
    WARNING_LOG() << "Failed to set pix_fmts ret: " << ret;
    return ret;
  }

  last_filter = filt_out;

/* Note: this macro adds a filter before the lastly added filter, so the
 * processing order of the filters is in reverse */
#define INSERT_FILT(name, arg)                                                                                   \
  do {                                                                                                           \
    AVFilterContext* filt_ctx;                                                                                   \
                                                                                                                 \
    ret = avfilter_graph_create_filter(&filt_ctx, avfilter_get_by_name(name), "ffplay_" name, arg, NULL, graph); \
    if (ret < 0)                                                                                                 \
      return ret;                                                                                                \
                                                                                                                 \
    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                                                            \
    if (ret < 0)                                                                                                 \
      return ret;                                                                                                \
                                                                                                                 \
    last_filter = filt_ctx;                                                                                      \
  } while (0)

  if (opt_.autorotate) {
    double theta = get_rotation(video_st);

    if (fabs(theta - 90) < 1.0) {
      INSERT_FILT("transpose", "clock");
    } else if (fabs(theta - 180) < 1.0) {
      INSERT_FILT("hflip", NULL);
      INSERT_FILT("vflip", NULL);
    } else if (fabs(theta - 270) < 1.0) {
      INSERT_FILT("transpose", "cclock");
    } else if (fabs(theta) > 1.0) {
      char rotate_buf[64];
      snprintf(rotate_buf, sizeof(rotate_buf), "%f*PI/180", theta);
      INSERT_FILT("rotate", rotate_buf);
    }
  }

  if ((ret = configure_filtergraph(graph, vfilters, filt_src, last_filter)) < 0) {
    WARNING_LOG() << "Failed to configure_filtergraph ret: " << ret;
    return ret;
  }

  in_video_filter_ = filt_src;
  out_video_filter_ = filt_out;
  return ret;
}

int VideoState::ConfigureAudioFilters(const std::string& afilters, int force_output_format) {
  static const enum AVSampleFormat sample_fmts[] = {AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE};
  avfilter_graph_free(&agraph_);
  agraph_ = avfilter_graph_alloc();
  if (!agraph_) {
    return AVERROR(ENOMEM);
  }

  AVDictionaryEntry* e = NULL;
  char aresample_swr_opts[512] = "";
  AVDictionary* swr_opts = copt_.swr_opts;
  while ((e = av_dict_get(swr_opts, "", e, AV_DICT_IGNORE_SUFFIX))) {
    av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts), "%s=%s:", e->key, e->value);
  }
  if (strlen(aresample_swr_opts)) {
    aresample_swr_opts[strlen(aresample_swr_opts) - 1] = '\0';
  }
  av_opt_set(agraph_, "aresample_swr_opts", aresample_swr_opts, 0);

  char asrc_args[256];
  int ret = snprintf(asrc_args, sizeof(asrc_args), "sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
                     audio_filter_src_.freq, av_get_sample_fmt_name(audio_filter_src_.fmt), audio_filter_src_.channels,
                     1, audio_filter_src_.freq);
  if (audio_filter_src_.channel_layout) {
    snprintf(asrc_args + ret, sizeof(asrc_args) - ret, ":channel_layout=0x%" PRIx64, audio_filter_src_.channel_layout);
  }

  AVFilterContext *filt_asrc = NULL, *filt_asink = NULL;
  ret = avfilter_graph_create_filter(&filt_asrc, avfilter_get_by_name("abuffer"), "ffplay_abuffer", asrc_args, NULL,
                                     agraph_);
  if (ret < 0) {
    avfilter_graph_free(&agraph_);
    return ret;
  }

  ret = avfilter_graph_create_filter(&filt_asink, avfilter_get_by_name("abuffersink"), "ffplay_abuffersink", NULL, NULL,
                                     agraph_);
  if (ret < 0) {
    avfilter_graph_free(&agraph_);
    return ret;
  }

  if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE, AV_OPT_SEARCH_CHILDREN)) <
      0) {
    avfilter_graph_free(&agraph_);
    return ret;
  }
  if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0) {
    avfilter_graph_free(&agraph_);
    return ret;
  }

  if (force_output_format) {
    int channels[2] = {0, -1};
    int64_t channel_layouts[2] = {0, -1};
    int sample_rates[2] = {0, -1};
    channel_layouts[0] = audio_tgt_.channel_layout;
    channels[0] = audio_tgt_.channels;
    sample_rates[0] = audio_tgt_.freq;
    if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0) {
      avfilter_graph_free(&agraph_);
      return ret;
    }
    ret = av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts, -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
      avfilter_graph_free(&agraph_);
      return ret;
    }
    ret = av_opt_set_int_list(filt_asink, "channel_counts", channels, -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
      avfilter_graph_free(&agraph_);
      return ret;
    }
    ret = av_opt_set_int_list(filt_asink, "sample_rates", sample_rates, -1, AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
      avfilter_graph_free(&agraph_);
      return ret;
    }
  }

  ret = configure_filtergraph(agraph_, afilters, filt_asrc, filt_asink);
  if (ret < 0) {
    avfilter_graph_free(&agraph_);
    return ret;
  }

  in_audio_filter_ = filt_asrc;
  out_audio_filter_ = filt_asink;
  return ret;
}
#endif /* CONFIG_AVFILTER */

}  // namespace core
}  // namespace client
}  // namespace fastotv
}  // namespace fasto
