#include "core/video_state.h"

#include <common/macros.h>

#include "core/cmd_utils.h"
#include "core/threads.h"
#include "core/types.h"

#define CURSOR_HIDE_DELAY 1000000
#define USE_ONEPASS_SUBTITLE_RENDER 1
/* Step size for volume control */
#define SDL_VOLUME_STEP (SDL_MIX_MAXVOLUME / 50)
#define EXTERNAL_CLOCK_MIN_FRAMES 2
#define EXTERNAL_CLOCK_MAX_FRAMES 10
/* Minimum SDL audio buffer size, in samples. */
#define SDL_AUDIO_MIN_BUFFER_SIZE 512
/* Calculate actual buffer size keeping in mind not cause too frequent audio callbacks */
#define SDL_AUDIO_MAX_CALLBACKS_PER_SEC 30
/* maximum audio speed change to get correct sync */
#define SAMPLE_CORRECTION_PERCENT_MAX 10
/* external clock speed adjustment constants for realtime sources based on buffer fullness */
#define EXTERNAL_CLOCK_SPEED_MIN 0.900
#define EXTERNAL_CLOCK_SPEED_MAX 1.010
#define EXTERNAL_CLOCK_SPEED_STEP 0.001
/* we use about AUDIO_DIFF_AVG_NB A-V differences to make the average */
#define AUDIO_DIFF_AVG_NB 20
/* polls for possible required screen refresh at least this often, should be less than 1/fps */
#define REFRESH_RATE 0.01
/* NOTE: the size must be big enough to compensate the hardware audio buffersize size */
/* TODO: We assume that a decoded and resampled frame fits into this buffer */

static unsigned sws_flags = SWS_BICUBIC;

VideoState::VideoState(const char* filename, AVInputFormat* iformat, AppOptions opt) : opt(opt) {
  VideoState* is = this;
  is->filename = av_strdup(filename);
  if (!is->filename) {
    return;
  }

  is->iformat = iformat;
  is->ytop = 0;
  is->xleft = 0;

  is->opt = opt;

  /* start video display */
  if (frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0) {
    return;
  }
  if (frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0) {
    return;
  }
  if (frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0) {
    return;
  }

  if (packet_queue_init(&is->videoq) < 0 || packet_queue_init(&is->audioq) < 0 ||
      packet_queue_init(&is->subtitleq) < 0) {
    return;
  }

  if (!(is->continue_read_thread = SDL_CreateCond())) {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
    return;
  }

  init_clock(&is->vidclk, &is->videoq.serial);
  init_clock(&is->audclk, &is->audioq.serial);
  init_clock(&is->extclk, &is->extclk.serial);
  is->audio_clock_serial = -1;
  if (startup_volume < 0)
    av_log(NULL, AV_LOG_WARNING, "-volume=%d < 0, setting to 0\n", startup_volume);
  if (startup_volume > 100)
    av_log(NULL, AV_LOG_WARNING, "-volume=%d > 100, setting to 100\n", startup_volume);
  startup_volume = av_clip(startup_volume, 0, 100);
  startup_volume = av_clip(SDL_MIX_MAXVOLUME * startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
  is->audio_volume = startup_volume;
  is->muted = 0;
  is->av_sync_type = av_sync_type;
}

VideoState::~VideoState() {
  /* XXX: use a special url_shutdown call to abort parse cleanly */
  abort_request = 1;
  SDL_WaitThread(read_tid, NULL);

  /* close each stream */
  if (audio_stream >= 0)
    stream_component_close(audio_stream);
  if (video_stream >= 0)
    stream_component_close(video_stream);
  if (subtitle_stream >= 0)
    stream_component_close(subtitle_stream);

  avformat_close_input(&ic);

  packet_queue_destroy(&videoq);
  packet_queue_destroy(&audioq);
  packet_queue_destroy(&subtitleq);

  /* free all pictures */
  frame_queue_destory(&pictq);
  frame_queue_destory(&sampq);
  frame_queue_destory(&subpq);
  SDL_DestroyCond(continue_read_thread);
  sws_freeContext(img_convert_ctx);
  sws_freeContext(sub_convert_ctx);
  av_free(filename);
  if (vis_texture)
    SDL_DestroyTexture(vis_texture);
  if (sub_texture)
    SDL_DestroyTexture(sub_texture);
}

static void toggle_full_screen(VideoState* is) {
  is_full_screen = !is_full_screen;
  SDL_SetWindowFullscreen(window, is_full_screen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}

static void check_external_clock_speed(VideoState* is) {
  if (is->video_stream >= 0 && is->videoq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES ||
      is->audio_stream >= 0 && is->audioq.nb_packets <= EXTERNAL_CLOCK_MIN_FRAMES) {
    set_clock_speed(&is->extclk,
                    FFMAX(EXTERNAL_CLOCK_SPEED_MIN, is->extclk.speed - EXTERNAL_CLOCK_SPEED_STEP));
  } else if ((is->video_stream < 0 || is->videoq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES) &&
             (is->audio_stream < 0 || is->audioq.nb_packets > EXTERNAL_CLOCK_MAX_FRAMES)) {
    set_clock_speed(&is->extclk,
                    FFMIN(EXTERNAL_CLOCK_SPEED_MAX, is->extclk.speed + EXTERNAL_CLOCK_SPEED_STEP));
  } else {
    double speed = is->extclk.speed;
    if (speed != 1.0)
      set_clock_speed(&is->extclk,
                      speed + EXTERNAL_CLOCK_SPEED_STEP * (1.0 - speed) / fabs(1.0 - speed));
  }
}

static inline int compute_mod(int a, int b) {
  return a < 0 ? a % b + b : a % b;
}

static inline void fill_rectangle(int x, int y, int w, int h) {
  SDL_Rect rect;
  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = h;
  if (w && h)
    SDL_RenderFillRect(renderer, &rect);
}

static int realloc_texture(SDL_Texture** texture,
                           Uint32 new_format,
                           int new_width,
                           int new_height,
                           SDL_BlendMode blendmode,
                           int init_texture) {
  Uint32 format;
  int access, w, h;
  if (SDL_QueryTexture(*texture, &format, &access, &w, &h) < 0 || new_width != w ||
      new_height != h || new_format != format) {
    void* pixels;
    int pitch;
    SDL_DestroyTexture(*texture);
    if (!(*texture = SDL_CreateTexture(renderer, new_format, SDL_TEXTUREACCESS_STREAMING, new_width,
                                       new_height)))
      return -1;
    if (SDL_SetTextureBlendMode(*texture, blendmode) < 0)
      return -1;
    if (init_texture) {
      if (SDL_LockTexture(*texture, NULL, &pixels, &pitch) < 0)
        return -1;
      memset(pixels, 0, pitch * new_height);
      SDL_UnlockTexture(*texture);
    }
  }
  return 0;
}

static void video_audio_display(VideoState* s) {
  int i, i_start, x, y1, y, ys, delay, n, nb_display_channels;
  int ch, channels, h, h2;
  int64_t time_diff;
  int rdft_bits, nb_freq;

  for (rdft_bits = 1; (1 << rdft_bits) < 2 * s->height; rdft_bits++)
    ;
  nb_freq = 1 << (rdft_bits - 1);

  /* compute display index : center on currently output samples */
  channels = s->audio_tgt.channels;
  nb_display_channels = channels;
  if (!s->paused) {
    int data_used = s->show_mode == SHOW_MODE_WAVES ? s->width : (2 * nb_freq);
    n = 2 * channels;
    delay = s->audio_write_buf_size;
    delay /= n;

    /* to be more precise, we take into account the time spent since
       the last buffer computation */
    if (audio_callback_time) {
      time_diff = av_gettime_relative() - audio_callback_time;
      delay -= (time_diff * s->audio_tgt.freq) / 1000000;
    }

    delay += 2 * data_used;
    if (delay < data_used)
      delay = data_used;

    i_start = x = compute_mod(s->sample_array_index - delay * channels, SAMPLE_ARRAY_SIZE);
    if (s->show_mode == SHOW_MODE_WAVES) {
      h = INT_MIN;
      for (i = 0; i < 1000; i += channels) {
        int idx = (SAMPLE_ARRAY_SIZE + x - i) % SAMPLE_ARRAY_SIZE;
        int a = s->sample_array[idx];
        int b = s->sample_array[(idx + 4 * channels) % SAMPLE_ARRAY_SIZE];
        int c = s->sample_array[(idx + 5 * channels) % SAMPLE_ARRAY_SIZE];
        int d = s->sample_array[(idx + 9 * channels) % SAMPLE_ARRAY_SIZE];
        int score = a - d;
        if (h < score && (b ^ c) < 0) {
          h = score;
          i_start = idx;
        }
      }
    }

    s->last_i_start = i_start;
  } else {
    i_start = s->last_i_start;
  }

  if (s->show_mode == SHOW_MODE_WAVES) {
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    /* total height for one channel */
    h = s->height / nb_display_channels;
    /* graph height / 2 */
    h2 = (h * 9) / 20;
    for (ch = 0; ch < nb_display_channels; ch++) {
      i = i_start + ch;
      y1 = s->ytop + ch * h + (h / 2); /* position of center line */
      for (x = 0; x < s->width; x++) {
        y = (s->sample_array[i] * h2) >> 15;
        if (y < 0) {
          y = -y;
          ys = y1 - y;
        } else {
          ys = y1;
        }
        fill_rectangle(s->xleft + x, ys, 1, y);
        i += channels;
        if (i >= SAMPLE_ARRAY_SIZE)
          i -= SAMPLE_ARRAY_SIZE;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);

    for (ch = 1; ch < nb_display_channels; ch++) {
      y = s->ytop + ch * h;
      fill_rectangle(s->xleft, y, s->width, 1);
    }
  } else {
    if (realloc_texture(&s->vis_texture, SDL_PIXELFORMAT_ARGB8888, s->width, s->height,
                        SDL_BLENDMODE_NONE, 1) < 0)
      return;

    nb_display_channels = FFMIN(nb_display_channels, 2);
    if (rdft_bits != s->rdft_bits) {
      av_rdft_end(s->rdft);
      av_free(s->rdft_data);
      s->rdft = av_rdft_init(rdft_bits, DFT_R2C);
      s->rdft_bits = rdft_bits;
      s->rdft_data = static_cast<FFTSample*>(av_malloc_array(nb_freq, 4 * sizeof(*s->rdft_data)));
    }
    if (!s->rdft || !s->rdft_data) {
      av_log(NULL, AV_LOG_ERROR,
             "Failed to allocate buffers for RDFT, switching to waves display\n");
      s->show_mode = SHOW_MODE_WAVES;
    } else {
      FFTSample* data[2];
      SDL_Rect rect = {.x = s->xpos, .y = 0, .w = 1, .h = s->height};
      uint32_t* pixels;
      int pitch;
      for (ch = 0; ch < nb_display_channels; ch++) {
        data[ch] = s->rdft_data + 2 * nb_freq * ch;
        i = i_start + ch;
        for (x = 0; x < 2 * nb_freq; x++) {
          double w = (x - nb_freq) * (1.0 / nb_freq);
          data[ch][x] = s->sample_array[i] * (1.0 - w * w);
          i += channels;
          if (i >= SAMPLE_ARRAY_SIZE)
            i -= SAMPLE_ARRAY_SIZE;
        }
        av_rdft_calc(s->rdft, data[ch]);
      }
      /* Least efficient way to do this, we should of course
       * directly access it but it is more than fast enough. */
      if (!SDL_LockTexture(s->vis_texture, &rect, (void**)&pixels, &pitch)) {
        pitch >>= 2;
        pixels += pitch * s->height;
        for (y = 0; y < s->height; y++) {
          double w = 1 / sqrt(nb_freq);
          int a = sqrt(w * sqrt(data[0][2 * y + 0] * data[0][2 * y + 0] +
                                data[0][2 * y + 1] * data[0][2 * y + 1]));
          int b = (nb_display_channels == 2)
                      ? sqrt(w * hypot(data[1][2 * y + 0], data[1][2 * y + 1]))
                      : a;
          a = FFMIN(a, 255);
          b = FFMIN(b, 255);
          pixels -= pitch;
          *pixels = (a << 16) + (b << 8) + ((a + b) >> 1);
        }
        SDL_UnlockTexture(s->vis_texture);
      }
      SDL_RenderCopy(renderer, s->vis_texture, NULL, NULL);
    }
    if (!s->paused)
      s->xpos++;
    if (s->xpos >= s->width)
      s->xpos = s->xleft;
  }
}

static int video_open(VideoState* is, Frame* vp) {
  int w, h;

  if (vp && vp->width)
    set_default_window_size(vp->width, vp->height, vp->sar);

  if (screen_width) {
    w = screen_width;
    h = screen_height;
  } else {
    w = default_width;
    h = default_height;
  }

  if (!window) {
    int flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
    if (!window_title)
      window_title = input_filename;
    if (is_full_screen)
      flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    window = SDL_CreateWindow(window_title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h,
                              flags);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    if (window) {
      SDL_RendererInfo info;
      renderer =
          SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
      if (!renderer) {
        av_log(NULL, AV_LOG_WARNING, "Failed to initialize a hardware accelerated renderer: %s\n",
               SDL_GetError());
        renderer = SDL_CreateRenderer(window, -1, 0);
      }
      if (renderer) {
        if (!SDL_GetRendererInfo(renderer, &info))
          av_log(NULL, AV_LOG_VERBOSE, "Initialized %s renderer.\n", info.name);
      }
    }
  } else {
    SDL_SetWindowSize(window, w, h);
  }

  if (!window || !renderer) {
    av_log(NULL, AV_LOG_FATAL, "SDL: could not set video mode - exiting\n");
    return ERROR_RESULT_VALUE;
  }

  is->width = w;
  is->height = h;

  return 0;
}

/* allocate a picture (needs to do that in main thread to avoid
   potential locking problems */
static int alloc_picture(VideoState* is) {
  Frame* vp = &is->pictq.queue[is->pictq.windex];

  int res = video_open(is, vp);
  if (res == ERROR_RESULT_VALUE) {
    return ERROR_RESULT_VALUE;
  }

  int sdl_format;
  if (vp->format == AV_PIX_FMT_YUV420P) {
    sdl_format = SDL_PIXELFORMAT_YV12;
  } else {
    sdl_format = SDL_PIXELFORMAT_ARGB8888;
  }

  if (realloc_texture(&vp->bmp, sdl_format, vp->width, vp->height, SDL_BLENDMODE_NONE, 0) < 0) {
    /* SDL allocates a buffer smaller than requested if the video
     * overlay hardware is unable to support the requested size. */
    av_log(NULL, AV_LOG_FATAL,
           "Error: the video system does not support an image\n"
           "size of %dx%d pixels. Try using -lowres or -vf \"scale=w:h\"\n"
           "to reduce the image size.\n",
           vp->width, vp->height);
    return ERROR_RESULT_VALUE;
  }

  SDL_LockMutex(is->pictq.mutex);
  vp->allocated = 1;
  SDL_CondSignal(is->pictq.cond);
  SDL_UnlockMutex(is->pictq.mutex);
  return SUCCESS_RESULT_VALUE;
}

static int upload_texture(SDL_Texture* tex, AVFrame* frame, struct SwsContext** img_convert_ctx) {
  int ret = 0;
  switch (frame->format) {
    case AV_PIX_FMT_YUV420P:
      if (frame->linesize[0] < 0 || frame->linesize[1] < 0 || frame->linesize[2] < 0) {
        av_log(NULL, AV_LOG_ERROR, "Negative linesize is not supported for YUV.\n");
        return -1;
      }
      ret = SDL_UpdateYUVTexture(tex, NULL, frame->data[0], frame->linesize[0], frame->data[1],
                                 frame->linesize[1], frame->data[2], frame->linesize[2]);
      break;
    case AV_PIX_FMT_BGRA:
      if (frame->linesize[0] < 0) {
        ret =
            SDL_UpdateTexture(tex, NULL, frame->data[0] + frame->linesize[0] * (frame->height - 1),
                              -frame->linesize[0]);
      } else {
        ret = SDL_UpdateTexture(tex, NULL, frame->data[0], frame->linesize[0]);
      }
      break;
    default:
      /* This should only happen if we are not using avfilter... */
      *img_convert_ctx = sws_getCachedContext(
          *img_convert_ctx, frame->width, frame->height, (AVPixelFormat)frame->format, frame->width,
          frame->height, AV_PIX_FMT_BGRA, sws_flags, NULL, NULL, NULL);
      if (*img_convert_ctx != NULL) {
        uint8_t* pixels[4];
        int pitch[4];
        if (!SDL_LockTexture(tex, NULL, (void**)pixels, pitch)) {
          sws_scale(*img_convert_ctx, (const uint8_t* const*)frame->data, frame->linesize, 0,
                    frame->height, pixels, pitch);
          SDL_UnlockTexture(tex);
        }
      } else {
        av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
        ret = -1;
      }
      break;
  }
  return ret;
}

static void video_image_display(VideoState* is) {
  Frame* vp;
  Frame* sp = NULL;
  SDL_Rect rect;

  vp = frame_queue_peek_last(&is->pictq);
  if (vp->bmp) {
    if (is->subtitle_st) {
      if (frame_queue_nb_remaining(&is->subpq) > 0) {
        sp = frame_queue_peek(&is->subpq);

        if (vp->pts >= sp->pts + ((float)sp->sub.start_display_time / 1000)) {
          if (!sp->uploaded) {
            uint8_t* pixels[4];
            int pitch[4];
            int i;
            if (!sp->width || !sp->height) {
              sp->width = vp->width;
              sp->height = vp->height;
            }
            if (realloc_texture(&is->sub_texture, SDL_PIXELFORMAT_ARGB8888, sp->width, sp->height,
                                SDL_BLENDMODE_BLEND, 1) < 0)
              return;

            for (i = 0; i < sp->sub.num_rects; i++) {
              AVSubtitleRect* sub_rect = sp->sub.rects[i];

              sub_rect->x = av_clip(sub_rect->x, 0, sp->width);
              sub_rect->y = av_clip(sub_rect->y, 0, sp->height);
              sub_rect->w = av_clip(sub_rect->w, 0, sp->width - sub_rect->x);
              sub_rect->h = av_clip(sub_rect->h, 0, sp->height - sub_rect->y);

              is->sub_convert_ctx = sws_getCachedContext(
                  is->sub_convert_ctx, sub_rect->w, sub_rect->h, AV_PIX_FMT_PAL8, sub_rect->w,
                  sub_rect->h, AV_PIX_FMT_BGRA, 0, NULL, NULL, NULL);
              if (!is->sub_convert_ctx) {
                av_log(NULL, AV_LOG_FATAL, "Cannot initialize the conversion context\n");
                return;
              }
              if (!SDL_LockTexture(is->sub_texture, (SDL_Rect*)sub_rect, (void**)pixels, pitch)) {
                sws_scale(is->sub_convert_ctx, (const uint8_t* const*)sub_rect->data,
                          sub_rect->linesize, 0, sub_rect->h, pixels, pitch);
                SDL_UnlockTexture(is->sub_texture);
              }
            }
            sp->uploaded = 1;
          }
        } else
          sp = NULL;
      }
    }

    calculate_display_rect(&rect, is->xleft, is->ytop, is->width, is->height, vp->width, vp->height,
                           vp->sar);

    if (!vp->uploaded) {
      if (upload_texture(vp->bmp, vp->frame, &is->img_convert_ctx) < 0)
        return;
      vp->uploaded = 1;
      vp->flip_v = vp->frame->linesize[0] < 0;
    }

    SDL_RenderCopyEx(renderer, vp->bmp, NULL, &rect, 0, NULL,
                     vp->flip_v ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE);
    if (sp) {
#if USE_ONEPASS_SUBTITLE_RENDER
      SDL_RenderCopy(renderer, is->sub_texture, NULL, &rect);
#else
      int i;
      double xratio = (double)rect.w / (double)sp->width;
      double yratio = (double)rect.h / (double)sp->height;
      for (i = 0; i < sp->sub.num_rects; i++) {
        SDL_Rect* sub_rect = (SDL_Rect*)sp->sub.rects[i];
        SDL_Rect target = {.x = rect.x + sub_rect->x * xratio,
                           .y = rect.y + sub_rect->y * yratio,
                           .w = sub_rect->w * xratio,
                           .h = sub_rect->h * yratio};
        SDL_RenderCopy(renderer, is->sub_texture, sub_rect, &target);
      }
#endif
    }
  }
}

/* display the current picture, if any */
static void video_display(VideoState* is) {
  if (!window)
    video_open(is, NULL);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  if (is->audio_st && is->show_mode != SHOW_MODE_VIDEO)
    video_audio_display(is);
  else if (is->video_st)
    video_image_display(is);
  SDL_RenderPresent(renderer);
}

static double vp_duration(VideoState* is, Frame* vp, Frame* nextvp) {
  if (vp->serial == nextvp->serial) {
    double duration = nextvp->pts - vp->pts;
    if (isnan(duration) || duration <= 0 || duration > is->max_frame_duration)
      return vp->duration;
    else
      return duration;
  } else {
    return 0.0;
  }
}

static void update_video_pts(VideoState* is, double pts, int64_t pos, int serial) {
  /* update current video pts */
  set_clock(&is->vidclk, pts, serial);
  sync_clock_to_slave(&is->extclk, &is->vidclk);
}

/* pause or resume the video */
static void stream_toggle_pause(VideoState* is) {
  if (is->paused) {
    is->frame_timer += av_gettime_relative() / 1000000.0 - is->vidclk.last_updated;
    if (is->read_pause_return != AVERROR(ENOSYS)) {
      is->vidclk.paused = 0;
    }
    set_clock(&is->vidclk, get_clock(&is->vidclk), is->vidclk.serial);
  }
  set_clock(&is->extclk, get_clock(&is->extclk), is->extclk.serial);
  is->paused = is->audclk.paused = is->vidclk.paused = is->extclk.paused = !is->paused;
}

static void toggle_pause(VideoState* is) {
  stream_toggle_pause(is);
  is->step = 0;
}

static void toggle_mute(VideoState* is) {
  is->muted = !is->muted;
}

static void update_volume(VideoState* is, int sign, int step) {
  is->audio_volume = av_clip(is->audio_volume + sign * step, 0, SDL_MIX_MAXVOLUME);
}

/* called to display each frame */
static void video_refresh(void* opaque, double* remaining_time) {
  VideoState* is = static_cast<VideoState*>(opaque);
  double time;

  Frame *sp, *sp2;

  if (!is->paused && is->get_master_sync_type() == AV_SYNC_EXTERNAL_CLOCK && is->realtime)
    check_external_clock_speed(is);

  if (!display_disable && is->show_mode != SHOW_MODE_VIDEO && is->audio_st) {
    time = av_gettime_relative() / 1000000.0;
    if (is->force_refresh || is->last_vis_time + rdftspeed < time) {
      video_display(is);
      is->last_vis_time = time;
    }
    *remaining_time = FFMIN(*remaining_time, is->last_vis_time + rdftspeed - time);
  }

  if (is->video_st) {
  retry:
    if (frame_queue_nb_remaining(&is->pictq) == 0) {
      // nothing to do, no picture to display in the queue
    } else {
      double last_duration, duration, delay;
      Frame *vp, *lastvp;

      /* dequeue the picture */
      lastvp = frame_queue_peek_last(&is->pictq);
      vp = frame_queue_peek(&is->pictq);

      if (vp->serial != is->videoq.serial) {
        frame_queue_next(&is->pictq);
        goto retry;
      }

      if (lastvp->serial != vp->serial)
        is->frame_timer = av_gettime_relative() / 1000000.0;

      if (is->paused)
        goto display;

      /* compute nominal last_duration */
      last_duration = vp_duration(is, lastvp, vp);
      delay = is->compute_target_delay(last_duration);

      time = av_gettime_relative() / 1000000.0;
      if (time < is->frame_timer + delay) {
        *remaining_time = FFMIN(is->frame_timer + delay - time, *remaining_time);
        goto display;
      }

      is->frame_timer += delay;
      if (delay > 0 && time - is->frame_timer > AV_SYNC_THRESHOLD_MAX)
        is->frame_timer = time;

      SDL_LockMutex(is->pictq.mutex);
      if (!isnan(vp->pts))
        update_video_pts(is, vp->pts, vp->pos, vp->serial);
      SDL_UnlockMutex(is->pictq.mutex);

      if (frame_queue_nb_remaining(&is->pictq) > 1) {
        Frame* nextvp = frame_queue_peek_next(&is->pictq);
        duration = vp_duration(is, vp, nextvp);
        if (!is->step &&
            (framedrop > 0 || (framedrop && is->get_master_sync_type() != AV_SYNC_VIDEO_MASTER)) &&
            time > is->frame_timer + duration) {
          is->frame_drops_late++;
          frame_queue_next(&is->pictq);
          goto retry;
        }
      }

      if (is->subtitle_st) {
        while (frame_queue_nb_remaining(&is->subpq) > 0) {
          sp = frame_queue_peek(&is->subpq);

          if (frame_queue_nb_remaining(&is->subpq) > 1)
            sp2 = frame_queue_peek_next(&is->subpq);
          else
            sp2 = NULL;

          if (sp->serial != is->subtitleq.serial ||
              (is->vidclk.pts > (sp->pts + ((float)sp->sub.end_display_time / 1000))) ||
              (sp2 && is->vidclk.pts > (sp2->pts + ((float)sp2->sub.start_display_time / 1000)))) {
            if (sp->uploaded) {
              int i;
              for (i = 0; i < sp->sub.num_rects; i++) {
                AVSubtitleRect* sub_rect = sp->sub.rects[i];
                uint8_t* pixels;
                int pitch, j;

                if (!SDL_LockTexture(is->sub_texture, (SDL_Rect*)sub_rect, (void**)&pixels,
                                     &pitch)) {
                  for (j = 0; j < sub_rect->h; j++, pixels += pitch)
                    memset(pixels, 0, sub_rect->w << 2);
                  SDL_UnlockTexture(is->sub_texture);
                }
              }
            }
            frame_queue_next(&is->subpq);
          } else {
            break;
          }
        }
      }

      frame_queue_next(&is->pictq);
      is->force_refresh = 1;

      if (is->step && !is->paused)
        stream_toggle_pause(is);
    }
  display:
    /* display picture */
    if (!display_disable && is->force_refresh && is->show_mode == SHOW_MODE_VIDEO &&
        is->pictq.rindex_shown)
      video_display(is);
  }
  is->force_refresh = 0;
  if (show_status) {
    static int64_t last_time;
    int64_t cur_time;
    int aqsize, vqsize, sqsize;
    double av_diff;

    cur_time = av_gettime_relative();
    if (!last_time || (cur_time - last_time) >= 30000) {
      aqsize = 0;
      vqsize = 0;
      sqsize = 0;
      if (is->audio_st)
        aqsize = is->audioq.size;
      if (is->video_st)
        vqsize = is->videoq.size;
      if (is->subtitle_st)
        sqsize = is->subtitleq.size;
      av_diff = 0;
      if (is->audio_st && is->video_st)
        av_diff = get_clock(&is->audclk) - get_clock(&is->vidclk);
      else if (is->video_st)
        av_diff = is->get_master_clock() - get_clock(&is->vidclk);
      else if (is->audio_st)
        av_diff = is->get_master_clock() - get_clock(&is->audclk);
      av_log(NULL, AV_LOG_INFO,
             "%7.2f %s:%7.3f fd=%4d aq=%5dKB vq=%5dKB sq=%5dB f=%" PRId64 "/%" PRId64 "   \r",
             is->get_master_clock(), (is->audio_st && is->video_st)
                                         ? "A-V"
                                         : (is->video_st ? "M-V" : (is->audio_st ? "M-A" : "   ")),
             av_diff, is->frame_drops_early + is->frame_drops_late, aqsize / 1024, vqsize / 1024,
             sqsize, is->video_st ? is->viddec.avctx->pts_correction_num_faulty_dts : 0,
             is->video_st ? is->viddec.avctx->pts_correction_num_faulty_pts : 0);
      fflush(stdout);
      last_time = cur_time;
    }
  }
}

static void refresh_loop_wait_event(VideoState* is, SDL_Event* event) {
  double remaining_time = 0.0;
  SDL_PumpEvents();
  while (!SDL_PeepEvents(event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT)) {
    if (!cursor_hidden && av_gettime_relative() - cursor_last_shown > CURSOR_HIDE_DELAY) {
      SDL_ShowCursor(0);
      cursor_hidden = 1;
    }
    if (remaining_time > 0.0)
      av_usleep((int64_t)(remaining_time * 1000000.0));
    remaining_time = REFRESH_RATE;
    if (is->show_mode != SHOW_MODE_NONE && (!is->paused || is->force_refresh))
      video_refresh(is, &remaining_time);
    SDL_PumpEvents();
  }
}

/* copy samples for viewing in editor window */
static void update_sample_display(VideoState* is, short* samples, int samples_size) {
  int size, len;

  size = samples_size / sizeof(short);
  while (size > 0) {
    len = SAMPLE_ARRAY_SIZE - is->sample_array_index;
    if (len > size)
      len = size;
    memcpy(is->sample_array + is->sample_array_index, samples, len * sizeof(short));
    samples += len;
    is->sample_array_index += len;
    if (is->sample_array_index >= SAMPLE_ARRAY_SIZE)
      is->sample_array_index = 0;
    size -= len;
  }
}

/* return the wanted number of samples to get better sync if sync_type is video
 * or external master clock */
static int synchronize_audio(VideoState* is, int nb_samples) {
  int wanted_nb_samples = nb_samples;

  /* if not master, then we try to remove or add samples to correct the clock */
  if (is->get_master_sync_type() != AV_SYNC_AUDIO_MASTER) {
    double diff, avg_diff;
    int min_nb_samples, max_nb_samples;

    diff = get_clock(&is->audclk) - is->get_master_clock();

    if (!isnan(diff) && fabs(diff) < AV_NOSYNC_THRESHOLD) {
      is->audio_diff_cum = diff + is->audio_diff_avg_coef * is->audio_diff_cum;
      if (is->audio_diff_avg_count < AUDIO_DIFF_AVG_NB) {
        /* not enough measures to have a correct estimate */
        is->audio_diff_avg_count++;
      } else {
        /* estimate the A-V difference */
        avg_diff = is->audio_diff_cum * (1.0 - is->audio_diff_avg_coef);

        if (fabs(avg_diff) >= is->audio_diff_threshold) {
          wanted_nb_samples = nb_samples + (int)(diff * is->audio_src.freq);
          min_nb_samples = ((nb_samples * (100 - SAMPLE_CORRECTION_PERCENT_MAX) / 100));
          max_nb_samples = ((nb_samples * (100 + SAMPLE_CORRECTION_PERCENT_MAX) / 100));
          wanted_nb_samples = av_clip(wanted_nb_samples, min_nb_samples, max_nb_samples);
        }
        av_log(NULL, AV_LOG_TRACE, "diff=%f adiff=%f sample_diff=%d apts=%0.3f %f\n", diff,
               avg_diff, wanted_nb_samples - nb_samples, is->audio_clock, is->audio_diff_threshold);
      }
    } else {
      /* too big difference : may be initial PTS errors, so
         reset A-V filter */
      is->audio_diff_avg_count = 0;
      is->audio_diff_cum = 0;
    }
  }

  return wanted_nb_samples;
}

/**
 * Decode one audio frame and return its uncompressed size.
 *
 * The processed audio frame is decoded, converted if required, and
 * stored in is->audio_buf, with size in bytes given by the return
 * value.
 */
static int audio_decode_frame(VideoState* is) {
  int data_size, resampled_data_size;
  int64_t dec_channel_layout;
  av_unused double audio_clock0;
  int wanted_nb_samples;
  Frame* af;

  if (is->paused)
    return -1;

  do {
#if defined(_WIN32)
    while (frame_queue_nb_remaining(&is->sampq) == 0) {
      if ((av_gettime_relative() - audio_callback_time) >
          1000000LL * is->audio_hw_buf_size / is->audio_tgt.bytes_per_sec / 2)
        return -1;
      av_usleep(1000);
    }
#endif
    if (!(af = frame_queue_peek_readable(&is->sampq)))
      return -1;
    frame_queue_next(&is->sampq);
  } while (af->serial != is->audioq.serial);

  data_size =
      av_samples_get_buffer_size(NULL, av_frame_get_channels(af->frame), af->frame->nb_samples,
                                 (AVSampleFormat)af->frame->format, 1);

  dec_channel_layout = (af->frame->channel_layout &&
                        av_frame_get_channels(af->frame) ==
                            av_get_channel_layout_nb_channels(af->frame->channel_layout))
                           ? af->frame->channel_layout
                           : av_get_default_channel_layout(av_frame_get_channels(af->frame));
  wanted_nb_samples = synchronize_audio(is, af->frame->nb_samples);

  if (af->frame->format != is->audio_src.fmt ||
      dec_channel_layout != is->audio_src.channel_layout ||
      af->frame->sample_rate != is->audio_src.freq ||
      (wanted_nb_samples != af->frame->nb_samples && !is->swr_ctx)) {
    swr_free(&is->swr_ctx);
    is->swr_ctx = swr_alloc_set_opts(
        NULL, is->audio_tgt.channel_layout, is->audio_tgt.fmt, is->audio_tgt.freq,
        dec_channel_layout, (AVSampleFormat)af->frame->format, af->frame->sample_rate, 0, NULL);
    if (!is->swr_ctx || swr_init(is->swr_ctx) < 0) {
      av_log(NULL, AV_LOG_ERROR,
             "Cannot create sample rate converter for conversion of %d Hz %s %d channels to %d Hz "
             "%s %d channels!\n",
             af->frame->sample_rate, av_get_sample_fmt_name((AVSampleFormat)af->frame->format),
             av_frame_get_channels(af->frame), is->audio_tgt.freq,
             av_get_sample_fmt_name(is->audio_tgt.fmt), is->audio_tgt.channels);
      swr_free(&is->swr_ctx);
      return -1;
    }
    is->audio_src.channel_layout = dec_channel_layout;
    is->audio_src.channels = av_frame_get_channels(af->frame);
    is->audio_src.freq = af->frame->sample_rate;
    is->audio_src.fmt = (AVSampleFormat)af->frame->format;
  }

  if (is->swr_ctx) {
    const uint8_t** in = (const uint8_t**)af->frame->extended_data;
    uint8_t** out = &is->audio_buf1;
    int out_count = (int64_t)wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate + 256;
    int out_size =
        av_samples_get_buffer_size(NULL, is->audio_tgt.channels, out_count, is->audio_tgt.fmt, 0);
    int len2;
    if (out_size < 0) {
      av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size() failed\n");
      return -1;
    }
    if (wanted_nb_samples != af->frame->nb_samples) {
      if (swr_set_compensation(is->swr_ctx, (wanted_nb_samples - af->frame->nb_samples) *
                                                is->audio_tgt.freq / af->frame->sample_rate,
                               wanted_nb_samples * is->audio_tgt.freq / af->frame->sample_rate) <
          0) {
        av_log(NULL, AV_LOG_ERROR, "swr_set_compensation() failed\n");
        return -1;
      }
    }
    av_fast_malloc(&is->audio_buf1, &is->audio_buf1_size, out_size);
    if (!is->audio_buf1)
      return AVERROR(ENOMEM);
    len2 = swr_convert(is->swr_ctx, out, out_count, in, af->frame->nb_samples);
    if (len2 < 0) {
      av_log(NULL, AV_LOG_ERROR, "swr_convert() failed\n");
      return -1;
    }
    if (len2 == out_count) {
      av_log(NULL, AV_LOG_WARNING, "audio buffer is probably too small\n");
      if (swr_init(is->swr_ctx) < 0)
        swr_free(&is->swr_ctx);
    }
    is->audio_buf = is->audio_buf1;
    resampled_data_size =
        len2 * is->audio_tgt.channels * av_get_bytes_per_sample(is->audio_tgt.fmt);
  } else {
    is->audio_buf = af->frame->data[0];
    resampled_data_size = data_size;
  }

  audio_clock0 = is->audio_clock;
  /* update the audio clock with the pts */
  if (!isnan(af->pts))
    is->audio_clock = af->pts + (double)af->frame->nb_samples / af->frame->sample_rate;
  else
    is->audio_clock = NAN;
  is->audio_clock_serial = af->serial;
#ifdef DEBUG
  {
    static double last_clock;
    printf("audio: delay=%0.3f clock=%0.3f clock0=%0.3f\n", is->audio_clock - last_clock,
           is->audio_clock, audio_clock0);
    last_clock = is->audio_clock;
  }
#endif
  return resampled_data_size;
}

/* prepare a new audio buffer */
static void sdl_audio_callback(void* opaque, Uint8* stream, int len) {
  VideoState* is = static_cast<VideoState*>(opaque);
  int audio_size, len1;

  audio_callback_time = av_gettime_relative();

  while (len > 0) {
    if (is->audio_buf_index >= is->audio_buf_size) {
      audio_size = audio_decode_frame(is);
      if (audio_size < 0) {
        /* if error, just output silence */
        is->audio_buf = NULL;
        is->audio_buf_size =
            SDL_AUDIO_MIN_BUFFER_SIZE / is->audio_tgt.frame_size * is->audio_tgt.frame_size;
      } else {
        if (is->show_mode != SHOW_MODE_VIDEO)
          update_sample_display(is, (int16_t*)is->audio_buf, audio_size);
        is->audio_buf_size = audio_size;
      }
      is->audio_buf_index = 0;
    }
    len1 = is->audio_buf_size - is->audio_buf_index;
    if (len1 > len)
      len1 = len;
    if (!is->muted && is->audio_buf && is->audio_volume == SDL_MIX_MAXVOLUME)
      memcpy(stream, (uint8_t*)is->audio_buf + is->audio_buf_index, len1);
    else {
      memset(stream, 0, len1);
      if (!is->muted && is->audio_buf)
        SDL_MixAudio(stream, (uint8_t*)is->audio_buf + is->audio_buf_index, len1, is->audio_volume);
    }
    len -= len1;
    stream += len1;
    is->audio_buf_index += len1;
  }
  is->audio_write_buf_size = is->audio_buf_size - is->audio_buf_index;
  /* Let's assume the audio driver that is used by SDL has two periods. */
  if (!isnan(is->audio_clock)) {
    set_clock_at(&is->audclk, is->audio_clock -
                                  (double)(2 * is->audio_hw_buf_size + is->audio_write_buf_size) /
                                      is->audio_tgt.bytes_per_sec,
                 is->audio_clock_serial, audio_callback_time / 1000000.0);
    sync_clock_to_slave(&is->extclk, &is->audclk);
  }
}

static int audio_open(void* opaque,
                      int64_t wanted_channel_layout,
                      int wanted_nb_channels,
                      int wanted_sample_rate,
                      struct AudioParams* audio_hw_params) {
  SDL_AudioSpec wanted_spec, spec;
  const char* env;
  static const int next_nb_channels[] = {0, 0, 1, 6, 2, 6, 4, 6};
  static const int next_sample_rates[] = {0, 44100, 48000, 96000, 192000};
  int next_sample_rate_idx = FF_ARRAY_ELEMS(next_sample_rates) - 1;

  env = SDL_getenv("SDL_AUDIO_CHANNELS");
  if (env) {
    wanted_nb_channels = atoi(env);
    wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
  }
  if (!wanted_channel_layout ||
      wanted_nb_channels != av_get_channel_layout_nb_channels(wanted_channel_layout)) {
    wanted_channel_layout = av_get_default_channel_layout(wanted_nb_channels);
    wanted_channel_layout &= ~AV_CH_LAYOUT_STEREO_DOWNMIX;
  }
  wanted_nb_channels = av_get_channel_layout_nb_channels(wanted_channel_layout);
  wanted_spec.channels = wanted_nb_channels;
  wanted_spec.freq = wanted_sample_rate;
  if (wanted_spec.freq <= 0 || wanted_spec.channels <= 0) {
    av_log(NULL, AV_LOG_ERROR, "Invalid sample rate or channel count!\n");
    return -1;
  }
  while (next_sample_rate_idx && next_sample_rates[next_sample_rate_idx] >= wanted_spec.freq)
    next_sample_rate_idx--;
  wanted_spec.format = AUDIO_S16SYS;
  wanted_spec.silence = 0;
  wanted_spec.samples = FFMAX(SDL_AUDIO_MIN_BUFFER_SIZE,
                              2 << av_log2(wanted_spec.freq / SDL_AUDIO_MAX_CALLBACKS_PER_SEC));
  wanted_spec.callback = sdl_audio_callback;
  wanted_spec.userdata = opaque;
  while (SDL_OpenAudio(&wanted_spec, &spec) < 0) {
    av_log(NULL, AV_LOG_WARNING, "SDL_OpenAudio (%d channels, %d Hz): %s\n", wanted_spec.channels,
           wanted_spec.freq, SDL_GetError());
    wanted_spec.channels = next_nb_channels[FFMIN(7, wanted_spec.channels)];
    if (!wanted_spec.channels) {
      wanted_spec.freq = next_sample_rates[next_sample_rate_idx--];
      wanted_spec.channels = wanted_nb_channels;
      if (!wanted_spec.freq) {
        av_log(NULL, AV_LOG_ERROR, "No more combinations to try, audio open failed\n");
        return -1;
      }
    }
    wanted_channel_layout = av_get_default_channel_layout(wanted_spec.channels);
  }
  if (spec.format != AUDIO_S16SYS) {
    av_log(NULL, AV_LOG_ERROR, "SDL advised audio format %d is not supported!\n", spec.format);
    return -1;
  }
  if (spec.channels != wanted_spec.channels) {
    wanted_channel_layout = av_get_default_channel_layout(spec.channels);
    if (!wanted_channel_layout) {
      av_log(NULL, AV_LOG_ERROR, "SDL advised channel count %d is not supported!\n", spec.channels);
      return -1;
    }
  }

  audio_hw_params->fmt = AV_SAMPLE_FMT_S16;
  audio_hw_params->freq = spec.freq;
  audio_hw_params->channel_layout = wanted_channel_layout;
  audio_hw_params->channels = spec.channels;
  audio_hw_params->frame_size =
      av_samples_get_buffer_size(NULL, audio_hw_params->channels, 1, audio_hw_params->fmt, 1);
  audio_hw_params->bytes_per_sec = av_samples_get_buffer_size(
      NULL, audio_hw_params->channels, audio_hw_params->freq, audio_hw_params->fmt, 1);
  if (audio_hw_params->bytes_per_sec <= 0 || audio_hw_params->frame_size <= 0) {
    av_log(NULL, AV_LOG_ERROR, "av_samples_get_buffer_size failed\n");
    return -1;
  }
  return spec.size;
}

static void stream_cycle_channel(VideoState* is, int codec_type) {
  AVFormatContext* ic = is->ic;
  int start_index, stream_index;
  int old_index;
  AVStream* st;
  AVProgram* p = NULL;
  int nb_streams = is->ic->nb_streams;

  if (codec_type == AVMEDIA_TYPE_VIDEO) {
    start_index = is->last_video_stream;
    old_index = is->video_stream;
  } else if (codec_type == AVMEDIA_TYPE_AUDIO) {
    start_index = is->last_audio_stream;
    old_index = is->audio_stream;
  } else {
    start_index = is->last_subtitle_stream;
    old_index = is->subtitle_stream;
  }
  stream_index = start_index;

  if (codec_type != AVMEDIA_TYPE_VIDEO && is->video_stream != -1) {
    p = av_find_program_from_stream(ic, NULL, is->video_stream);
    if (p) {
      nb_streams = p->nb_stream_indexes;
      for (start_index = 0; start_index < nb_streams; start_index++)
        if (p->stream_index[start_index] == stream_index)
          break;
      if (start_index == nb_streams)
        start_index = -1;
      stream_index = start_index;
    }
  }

  for (;;) {
    if (++stream_index >= nb_streams) {
      if (codec_type == AVMEDIA_TYPE_SUBTITLE) {
        stream_index = -1;
        is->last_subtitle_stream = -1;
        goto the_end;
      }
      if (start_index == -1)
        return;
      stream_index = 0;
    }
    if (stream_index == start_index)
      return;
    st = is->ic->streams[p ? p->stream_index[stream_index] : stream_index];
    if (st->codecpar->codec_type == codec_type) {
      /* check that parameters are OK */
      switch (codec_type) {
        case AVMEDIA_TYPE_AUDIO:
          if (st->codecpar->sample_rate != 0 && st->codecpar->channels != 0)
            goto the_end;
          break;
        case AVMEDIA_TYPE_VIDEO:
        case AVMEDIA_TYPE_SUBTITLE:
          goto the_end;
        default:
          break;
      }
    }
  }
the_end:
  if (p && stream_index != -1)
    stream_index = p->stream_index[stream_index];
  av_log(NULL, AV_LOG_INFO, "Switch %s stream from #%d to #%d\n",
         av_get_media_type_string((AVMediaType)codec_type), old_index, stream_index);

  is->stream_component_close(old_index);
  is->stream_component_open(stream_index);
}

static void toggle_audio_display(VideoState* is) {
  int next = is->show_mode;
  do {
    next = (next + 1) % SHOW_MODE_NB;
  } while (next != is->show_mode &&
           (next == SHOW_MODE_VIDEO && !is->video_st || next != SHOW_MODE_VIDEO && !is->audio_st));
  if (is->show_mode != next) {
    is->force_refresh = 1;
    is->show_mode = (ShowMode)next;
  }
}

static void seek_chapter(VideoState* is, int incr) {
  int64_t pos = is->get_master_clock() * AV_TIME_BASE;
  int i;

  if (!is->ic->nb_chapters)
    return;

  /* find the current chapter */
  for (i = 0; i < is->ic->nb_chapters; i++) {
    AVChapter* ch = is->ic->chapters[i];
    if (av_compare_ts(pos, AV_TIME_BASE_Q, ch->start, ch->time_base) < 0) {
      i--;
      break;
    }
  }

  i += incr;
  i = FFMAX(i, 0);
  if (i >= is->ic->nb_chapters)
    return;

  av_log(NULL, AV_LOG_VERBOSE, "Seeking to chapter %d.\n", i);
  is->stream_seek(
      av_rescale_q(is->ic->chapters[i]->start, is->ic->chapters[i]->time_base, AV_TIME_BASE_Q), 0,
      0);
}

#if CONFIG_AVFILTER
int configure_filtergraph(AVFilterGraph* graph,
                          const char* filtergraph,
                          AVFilterContext* source_ctx,
                          AVFilterContext* sink_ctx) {
  int ret, i;
  int nb_filters = graph->nb_filters;
  AVFilterInOut *outputs = NULL, *inputs = NULL;

  if (filtergraph) {
    outputs = avfilter_inout_alloc();
    inputs = avfilter_inout_alloc();
    if (!outputs || !inputs) {
      ret = AVERROR(ENOMEM);
      goto fail;
    }

    outputs->name = av_strdup("in");
    outputs->filter_ctx = source_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = sink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((ret = avfilter_graph_parse_ptr(graph, filtergraph, &inputs, &outputs, NULL)) < 0)
      goto fail;
  } else {
    if ((ret = avfilter_link(source_ctx, 0, sink_ctx, 0)) < 0)
      goto fail;
  }

  /* Reorder the filters to ensure that inputs of the custom filters are merged first */
  for (i = 0; i < graph->nb_filters - nb_filters; i++)
    FFSWAP(AVFilterContext*, graph->filters[i], graph->filters[i + nb_filters]);

  ret = avfilter_graph_config(graph, NULL);
fail:
  avfilter_inout_free(&outputs);
  avfilter_inout_free(&inputs);
  return ret;
}

int configure_video_filters(AVFilterGraph* graph,
                            VideoState* is,
                            const char* vfilters,
                            AVFrame* frame) {
  AVDictionary* sws_dict = is->opt.sws_dict;
  static const enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_BGRA,
                                                AV_PIX_FMT_NONE};
  char sws_flags_str[512] = "";
  char buffersrc_args[256];
  int ret;
  AVFilterContext *filt_src = NULL, *filt_out = NULL, *last_filter = NULL;
  AVCodecParameters* codecpar = is->video_st->codecpar;
  AVRational fr = av_guess_frame_rate(is->ic, is->video_st, NULL);
  AVDictionaryEntry* e = NULL;

  while ((e = av_dict_get(sws_dict, "", e, AV_DICT_IGNORE_SUFFIX))) {
    if (!strcmp(e->key, "sws_flags")) {
      av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", "flags", e->value);
    } else
      av_strlcatf(sws_flags_str, sizeof(sws_flags_str), "%s=%s:", e->key, e->value);
  }
  if (strlen(sws_flags_str))
    sws_flags_str[strlen(sws_flags_str) - 1] = '\0';

  graph->scale_sws_opts = av_strdup(sws_flags_str);

  snprintf(buffersrc_args, sizeof(buffersrc_args),
           "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d", frame->width,
           frame->height, frame->format, is->video_st->time_base.num, is->video_st->time_base.den,
           codecpar->sample_aspect_ratio.num, FFMAX(codecpar->sample_aspect_ratio.den, 1));
  if (fr.num && fr.den)
    av_strlcatf(buffersrc_args, sizeof(buffersrc_args), ":frame_rate=%d/%d", fr.num, fr.den);

  if ((ret = avfilter_graph_create_filter(&filt_src, avfilter_get_by_name("buffer"),
                                          "ffplay_buffer", buffersrc_args, NULL, graph)) < 0)
    goto fail;

  ret = avfilter_graph_create_filter(&filt_out, avfilter_get_by_name("buffersink"),
                                     "ffplay_buffersink", NULL, NULL, graph);
  if (ret < 0)
    goto fail;

  if ((ret = av_opt_set_int_list(filt_out, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE,
                                 AV_OPT_SEARCH_CHILDREN)) < 0)
    goto fail;

  last_filter = filt_out;

/* Note: this macro adds a filter before the lastly added filter, so the
 * processing order of the filters is in reverse */
#define INSERT_FILT(name, arg)                                                                     \
  do {                                                                                             \
    AVFilterContext* filt_ctx;                                                                     \
                                                                                                   \
    ret = avfilter_graph_create_filter(&filt_ctx, avfilter_get_by_name(name), "ffplay_" name, arg, \
                                       NULL, graph);                                               \
    if (ret < 0)                                                                                   \
      goto fail;                                                                                   \
                                                                                                   \
    ret = avfilter_link(filt_ctx, 0, last_filter, 0);                                              \
    if (ret < 0)                                                                                   \
      goto fail;                                                                                   \
                                                                                                   \
    last_filter = filt_ctx;                                                                        \
  } while (0)

  if (is->opt.autorotate) {
    double theta = get_rotation(is->video_st);

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

  if ((ret = configure_filtergraph(graph, vfilters, filt_src, last_filter)) < 0)
    goto fail;

  is->in_video_filter = filt_src;
  is->out_video_filter = filt_out;

fail:
  return ret;
}

int configure_audio_filters(VideoState* is, const char* afilters, int force_output_format) {
  AVDictionary* swr_opts = is->opt.swr_opts;
  static const enum AVSampleFormat sample_fmts[] = {AV_SAMPLE_FMT_S16, AV_SAMPLE_FMT_NONE};
  int sample_rates[2] = {0, -1};
  int64_t channel_layouts[2] = {0, -1};
  int channels[2] = {0, -1};
  AVFilterContext *filt_asrc = NULL, *filt_asink = NULL;
  char aresample_swr_opts[512] = "";
  AVDictionaryEntry* e = NULL;
  char asrc_args[256];
  int ret;

  avfilter_graph_free(&is->agraph);
  if (!(is->agraph = avfilter_graph_alloc()))
    return AVERROR(ENOMEM);

  while ((e = av_dict_get(swr_opts, "", e, AV_DICT_IGNORE_SUFFIX)))
    av_strlcatf(aresample_swr_opts, sizeof(aresample_swr_opts), "%s=%s:", e->key, e->value);
  if (strlen(aresample_swr_opts))
    aresample_swr_opts[strlen(aresample_swr_opts) - 1] = '\0';
  av_opt_set(is->agraph, "aresample_swr_opts", aresample_swr_opts, 0);

  ret = snprintf(asrc_args, sizeof(asrc_args),
                 "sample_rate=%d:sample_fmt=%s:channels=%d:time_base=%d/%d",
                 is->audio_filter_src.freq, av_get_sample_fmt_name(is->audio_filter_src.fmt),
                 is->audio_filter_src.channels, 1, is->audio_filter_src.freq);
  if (is->audio_filter_src.channel_layout)
    snprintf(asrc_args + ret, sizeof(asrc_args) - ret, ":channel_layout=0x%" PRIx64,
             is->audio_filter_src.channel_layout);

  ret = avfilter_graph_create_filter(&filt_asrc, avfilter_get_by_name("abuffer"), "ffplay_abuffer",
                                     asrc_args, NULL, is->agraph);
  if (ret < 0)
    goto end;

  ret = avfilter_graph_create_filter(&filt_asink, avfilter_get_by_name("abuffersink"),
                                     "ffplay_abuffersink", NULL, NULL, is->agraph);
  if (ret < 0)
    goto end;

  if ((ret = av_opt_set_int_list(filt_asink, "sample_fmts", sample_fmts, AV_SAMPLE_FMT_NONE,
                                 AV_OPT_SEARCH_CHILDREN)) < 0)
    goto end;
  if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 1, AV_OPT_SEARCH_CHILDREN)) < 0)
    goto end;

  if (force_output_format) {
    channel_layouts[0] = is->audio_tgt.channel_layout;
    channels[0] = is->audio_tgt.channels;
    sample_rates[0] = is->audio_tgt.freq;
    if ((ret = av_opt_set_int(filt_asink, "all_channel_counts", 0, AV_OPT_SEARCH_CHILDREN)) < 0)
      goto end;
    if ((ret = av_opt_set_int_list(filt_asink, "channel_layouts", channel_layouts, -1,
                                   AV_OPT_SEARCH_CHILDREN)) < 0)
      goto end;
    if ((ret = av_opt_set_int_list(filt_asink, "channel_counts", channels, -1,
                                   AV_OPT_SEARCH_CHILDREN)) < 0)
      goto end;
    if ((ret = av_opt_set_int_list(filt_asink, "sample_rates", sample_rates, -1,
                                   AV_OPT_SEARCH_CHILDREN)) < 0)
      goto end;
  }

  if ((ret = configure_filtergraph(is->agraph, afilters, filt_asrc, filt_asink)) < 0)
    goto end;

  is->in_audio_filter = filt_asrc;
  is->out_audio_filter = filt_asink;

end:
  if (ret < 0)
    avfilter_graph_free(&is->agraph);
  return ret;
}
#endif /* CONFIG_AVFILTER */

int VideoState::stream_component_open(int stream_index) {
  AVCodecContext* avctx;
  AVCodec* codec;
  const char* forced_codec_name = NULL;
  AVDictionary* opts = NULL;
  AVDictionaryEntry* t = NULL;
  int sample_rate, nb_channels;
  int64_t channel_layout;
  int ret = 0;
  int stream_lowres = lowres;

  if (stream_index < 0 || stream_index >= ic->nb_streams) {
    return -1;
  }

  avctx = avcodec_alloc_context3(NULL);
  if (!avctx)
    return AVERROR(ENOMEM);

  ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
  if (ret < 0)
    goto fail;
  av_codec_set_pkt_timebase(avctx, ic->streams[stream_index]->time_base);

  codec = avcodec_find_decoder(avctx->codec_id);

  switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
      last_audio_stream = stream_index;
      forced_codec_name = audio_codec_name;
      break;
    case AVMEDIA_TYPE_SUBTITLE:
      last_subtitle_stream = stream_index;
      forced_codec_name = subtitle_codec_name;
      break;
    case AVMEDIA_TYPE_VIDEO:
      last_video_stream = stream_index;
      forced_codec_name = video_codec_name;
      break;
  }
  if (forced_codec_name)
    codec = avcodec_find_decoder_by_name(forced_codec_name);
  if (!codec) {
    if (forced_codec_name)
      av_log(NULL, AV_LOG_WARNING, "No codec could be found with name '%s'\n", forced_codec_name);
    else
      av_log(NULL, AV_LOG_WARNING, "No codec could be found with id %d\n", avctx->codec_id);
    ret = AVERROR(EINVAL);
    goto fail;
  }

  avctx->codec_id = codec->id;
  if (stream_lowres > av_codec_get_max_lowres(codec)) {
    av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
           av_codec_get_max_lowres(codec));
    stream_lowres = av_codec_get_max_lowres(codec);
  }
  av_codec_set_lowres(avctx, stream_lowres);

#if FF_API_EMU_EDGE
  if (stream_lowres)
    avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif
  if (fast)
    avctx->flags2 |= AV_CODEC_FLAG2_FAST;
#if FF_API_EMU_EDGE
  if (codec->capabilities & AV_CODEC_CAP_DR1)
    avctx->flags |= CODEC_FLAG_EMU_EDGE;
#endif

  opts = filter_codec_opts(opt.codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);
  if (!av_dict_get(opts, "threads", NULL, 0))
    av_dict_set(&opts, "threads", "auto", 0);
  if (stream_lowres)
    av_dict_set_int(&opts, "lowres", stream_lowres, 0);
  if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
    av_dict_set(&opts, "refcounted_frames", "1", 0);
  if ((ret = avcodec_open2(avctx, codec, &opts)) < 0) {
    goto fail;
  }
  if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
    av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
    ret = AVERROR_OPTION_NOT_FOUND;
    goto fail;
  }

  eof = 0;
  ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;
  switch (avctx->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
#if CONFIG_AVFILTER
    {
      AVFilterLink* link;

      audio_filter_src.freq = avctx->sample_rate;
      audio_filter_src.channels = avctx->channels;
      audio_filter_src.channel_layout =
          get_valid_channel_layout(avctx->channel_layout, avctx->channels);
      audio_filter_src.fmt = avctx->sample_fmt;
      if ((ret = configure_audio_filters(this, afilters, 0)) < 0)
        goto fail;
      link = out_audio_filter->inputs[0];
      sample_rate = link->sample_rate;
      nb_channels = avfilter_link_get_channels(link);
      channel_layout = link->channel_layout;
    }
#else
      sample_rate = avctx->sample_rate;
      nb_channels = avctx->channels;
      channel_layout = avctx->channel_layout;
#endif

      /* prepare audio output */
      if ((ret = audio_open(this, channel_layout, nb_channels, sample_rate, &audio_tgt)) < 0) {
        goto fail;
      }

      audio_hw_buf_size = ret;
      audio_src = audio_tgt;
      audio_buf_size = 0;
      audio_buf_index = 0;

      /* init averaging filter */
      audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB);
      audio_diff_avg_count = 0;
      /* since we do not have a precise anough audio FIFO fullness,
         we correct audio sync only if larger than this threshold */
      audio_diff_threshold = (double)(audio_hw_buf_size) / audio_tgt.bytes_per_sec;

      audio_stream = stream_index;
      audio_st = ic->streams[stream_index];

      decoder_init(&auddec, avctx, &audioq, continue_read_thread);
      if ((ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) &&
          !ic->iformat->read_seek) {
        auddec.start_pts = audio_st->start_time;
        auddec.start_pts_tb = audio_st->time_base;
      }
      if ((ret = decoder_start(&auddec, audio_thread, this)) < 0) {
        goto out;
      }
      SDL_PauseAudio(0);
      break;
    case AVMEDIA_TYPE_VIDEO:
      video_stream = stream_index;
      video_st = ic->streams[stream_index];

      decoder_init(&viddec, avctx, &videoq, continue_read_thread);
      if ((ret = decoder_start(&viddec, video_thread, this)) < 0) {
        goto out;
      }
      queue_attachments_req = 1;
      break;
    case AVMEDIA_TYPE_SUBTITLE:
      subtitle_stream = stream_index;
      subtitle_st = ic->streams[stream_index];

      decoder_init(&subdec, avctx, &subtitleq, continue_read_thread);
      if ((ret = decoder_start(&subdec, subtitle_thread, this)) < 0) {
        goto out;
      }
      break;
    default:
      break;
  }
  goto out;

fail:
  avcodec_free_context(&avctx);
out:
  av_dict_free(&opts);

  return ret;
}

void VideoState::stream_component_close(int stream_index) {
  if (stream_index < 0 || stream_index >= ic->nb_streams) {
    return;
  }

  AVCodecParameters* codecpar = ic->streams[stream_index]->codecpar;
  switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
      decoder_abort(&auddec, &sampq);
      SDL_CloseAudio();
      decoder_destroy(&auddec);
      swr_free(&swr_ctx);
      av_freep(&audio_buf1);
      audio_buf1_size = 0;
      audio_buf = NULL;

      if (rdft) {
        av_rdft_end(rdft);
        av_freep(&rdft_data);
        rdft = NULL;
        rdft_bits = 0;
      }
      break;
    case AVMEDIA_TYPE_VIDEO:
      decoder_abort(&viddec, &pictq);
      decoder_destroy(&viddec);
      break;
    case AVMEDIA_TYPE_SUBTITLE:
      decoder_abort(&subdec, &subpq);
      decoder_destroy(&subdec);
      break;
    default:
      break;
  }

  ic->streams[stream_index]->discard = AVDISCARD_ALL;
  switch (codecpar->codec_type) {
    case AVMEDIA_TYPE_AUDIO:
      audio_st = NULL;
      audio_stream = -1;
      break;
    case AVMEDIA_TYPE_VIDEO:
      video_st = NULL;
      video_stream = -1;
      break;
    case AVMEDIA_TYPE_SUBTITLE:
      subtitle_st = NULL;
      subtitle_stream = -1;
      break;
    default:
      break;
  }
}

void VideoState::stream_seek(int64_t pos, int64_t rel, int seek_by_bytes) {
  if (!seek_req) {
    seek_pos = pos;
    seek_rel = rel;
    seek_flags &= ~AVSEEK_FLAG_BYTE;
    if (seek_by_bytes) {
      seek_flags |= AVSEEK_FLAG_BYTE;
    }
    seek_req = 1;
    SDL_CondSignal(continue_read_thread);
  }
}

void VideoState::step_to_next_frame() {
  /* if the stream is paused unpause it, then step */
  if (paused) {
    stream_toggle_pause(this);
  }
  step = 1;
}

int VideoState::get_master_sync_type() {
  if (av_sync_type == AV_SYNC_VIDEO_MASTER) {
    if (video_st) {
      return AV_SYNC_VIDEO_MASTER;
    } else {
      return AV_SYNC_AUDIO_MASTER;
    }
  } else if (av_sync_type == AV_SYNC_AUDIO_MASTER) {
    if (audio_st) {
      return AV_SYNC_AUDIO_MASTER;
    } else {
      return AV_SYNC_EXTERNAL_CLOCK;
    }
  }

  return AV_SYNC_EXTERNAL_CLOCK;
}

double VideoState::compute_target_delay(double delay) {
  double sync_threshold, diff = 0;

  /* update delay to follow master synchronisation source */
  if (get_master_sync_type() != AV_SYNC_VIDEO_MASTER) {
    /* if video is slave, we try to correct big delays by
       duplicating or deleting a frame */
    diff = get_clock(&vidclk) - get_master_clock();

    /* skip or repeat frame. We take into account the
       delay to compute the threshold. I still don't know
       if it is the best guess */
    sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
    if (!isnan(diff) && fabs(diff) < max_frame_duration) {
      if (diff <= -sync_threshold)
        delay = FFMAX(0, delay + diff);
      else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)
        delay = delay + diff;
      else if (diff >= sync_threshold)
        delay = 2 * delay;
    }
  }

  av_log(NULL, AV_LOG_TRACE, "video: delay=%0.3f A-V=%f\n", delay, -diff);

  return delay;
}

/* get the current master clock value */
double VideoState::get_master_clock() {
  double val;

  switch (get_master_sync_type()) {
    case AV_SYNC_VIDEO_MASTER:
      val = get_clock(&vidclk);
      break;
    case AV_SYNC_AUDIO_MASTER:
      val = get_clock(&audclk);
      break;
    default:
      val = get_clock(&extclk);
      break;
  }
  return val;
}

int VideoState::exec() {
  read_tid = SDL_CreateThread(read_thread, "read_thread", this);
  if (!read_tid) {
    av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());
    return EXIT_FAILURE;
  }

  SDL_Event event;
  double incr, pos, frac;
  VideoState* cur_stream = this;
  while (true) {
    double x;
    refresh_loop_wait_event(cur_stream, &event);
    switch (event.type) {
      case SDL_KEYDOWN:
        if (opt.exit_on_keydown) {
          return EXIT_SUCCESS;
        }
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
          case SDLK_q:
            return EXIT_SUCCESS;
          case SDLK_f:
            toggle_full_screen(cur_stream);
            cur_stream->force_refresh = 1;
            break;
          case SDLK_p:
          case SDLK_SPACE:
            toggle_pause(cur_stream);
            break;
          case SDLK_m:
            toggle_mute(cur_stream);
            break;
          case SDLK_KP_MULTIPLY:
          case SDLK_0:
            update_volume(cur_stream, 1, SDL_VOLUME_STEP);
            break;
          case SDLK_KP_DIVIDE:
          case SDLK_9:
            update_volume(cur_stream, -1, SDL_VOLUME_STEP);
            break;
          case SDLK_s:  // S: Step to next frame
            step_to_next_frame();
            break;
          case SDLK_a:
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
            break;
          case SDLK_v:
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
            break;
          case SDLK_c:
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_VIDEO);
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_AUDIO);
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
            break;
          case SDLK_t:
            stream_cycle_channel(cur_stream, AVMEDIA_TYPE_SUBTITLE);
            break;
          case SDLK_w:
#if CONFIG_AVFILTER
            if (cur_stream->show_mode == SHOW_MODE_VIDEO &&
                cur_stream->vfilter_idx < nb_vfilters - 1) {
              if (++cur_stream->vfilter_idx >= nb_vfilters)
                cur_stream->vfilter_idx = 0;
            } else {
              cur_stream->vfilter_idx = 0;
              toggle_audio_display(cur_stream);
            }
#else
            toggle_audio_display(cur_stream);
#endif
            break;
          case SDLK_PAGEUP:
            if (cur_stream->ic->nb_chapters <= 1) {
              incr = 600.0;
              goto do_seek;
            }
            seek_chapter(cur_stream, 1);
            break;
          case SDLK_PAGEDOWN:
            if (cur_stream->ic->nb_chapters <= 1) {
              incr = -600.0;
              goto do_seek;
            }
            seek_chapter(cur_stream, -1);
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
            if (seek_by_bytes) {
              pos = -1;
              if (pos < 0 && cur_stream->video_stream >= 0)
                pos = frame_queue_last_pos(&cur_stream->pictq);
              if (pos < 0 && cur_stream->audio_stream >= 0)
                pos = frame_queue_last_pos(&cur_stream->sampq);
              if (pos < 0)
                pos = avio_tell(cur_stream->ic->pb);
              if (cur_stream->ic->bit_rate)
                incr *= cur_stream->ic->bit_rate / 8.0;
              else
                incr *= 180000.0;
              pos += incr;
              cur_stream->stream_seek(pos, incr, 1);
            } else {
              pos = get_master_clock();
              if (isnan(pos))
                pos = (double)cur_stream->seek_pos / AV_TIME_BASE;
              pos += incr;
              if (cur_stream->ic->start_time != AV_NOPTS_VALUE &&
                  pos < cur_stream->ic->start_time / (double)AV_TIME_BASE)
                pos = cur_stream->ic->start_time / (double)AV_TIME_BASE;
              cur_stream->stream_seek((int64_t)(pos * AV_TIME_BASE), (int64_t)(incr * AV_TIME_BASE),
                                      0);
            }
            break;
          default:
            break;
        }
        break;
      case SDL_MOUSEBUTTONDOWN:
        if (opt.exit_on_mousedown) {
          return EXIT_SUCCESS;
        }
        if (event.button.button == SDL_BUTTON_LEFT) {
          static int64_t last_mouse_left_click = 0;
          if (av_gettime_relative() - last_mouse_left_click <= 500000) {
            toggle_full_screen(cur_stream);
            cur_stream->force_refresh = 1;
            last_mouse_left_click = 0;
          } else {
            last_mouse_left_click = av_gettime_relative();
          }
        }
      case SDL_MOUSEMOTION:
        if (cursor_hidden) {
          SDL_ShowCursor(1);
          cursor_hidden = 0;
        }
        cursor_last_shown = av_gettime_relative();
        if (event.type == SDL_MOUSEBUTTONDOWN) {
          if (event.button.button != SDL_BUTTON_RIGHT)
            break;
          x = event.button.x;
        } else {
          if (!(event.motion.state & SDL_BUTTON_RMASK))
            break;
          x = event.motion.x;
        }
        if (seek_by_bytes || cur_stream->ic->duration <= 0) {
          uint64_t size = avio_size(cur_stream->ic->pb);
          cur_stream->stream_seek(size * x / cur_stream->width, 0, 1);
        } else {
          int64_t ts;
          int ns, hh, mm, ss;
          int tns, thh, tmm, tss;
          tns = cur_stream->ic->duration / 1000000LL;
          thh = tns / 3600;
          tmm = (tns % 3600) / 60;
          tss = (tns % 60);
          frac = x / cur_stream->width;
          ns = frac * tns;
          hh = ns / 3600;
          mm = (ns % 3600) / 60;
          ss = (ns % 60);
          av_log(NULL, AV_LOG_INFO,
                 "Seek to %2.0f%% (%2d:%02d:%02d) of total duration (%2d:%02d:%02d)       \n",
                 frac * 100, hh, mm, ss, thh, tmm, tss);
          ts = frac * cur_stream->ic->duration;
          if (cur_stream->ic->start_time != AV_NOPTS_VALUE)
            ts += cur_stream->ic->start_time;
          cur_stream->stream_seek(ts, 0, 0);
        }
        break;
      case SDL_WINDOWEVENT:
        switch (event.window.event) {
          case SDL_WINDOWEVENT_RESIZED:
            screen_width = cur_stream->width = event.window.data1;
            screen_height = cur_stream->height = event.window.data2;
            if (cur_stream->vis_texture) {
              SDL_DestroyTexture(cur_stream->vis_texture);
              cur_stream->vis_texture = NULL;
            }
          case SDL_WINDOWEVENT_EXPOSED:
            cur_stream->force_refresh = 1;
        }
        break;
      case SDL_QUIT:
      case FF_QUIT_EVENT:
        return EXIT_SUCCESS;
      case FF_ALLOC_EVENT: {
        int res = alloc_picture(static_cast<VideoState*>(event.user.data1));
        if (res == ERROR_RESULT_VALUE) {
          return EXIT_FAILURE;
        }
      }
      default:
        break;
    }
  }
  return EXIT_SUCCESS;
}
