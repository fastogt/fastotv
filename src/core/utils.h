#pragma once

#include <SDL2/SDL_audio.h>   // for SDL_AudioCallback
#include <SDL2/SDL_rect.h>    // for SDL_Rect
#include <SDL2/SDL_render.h>  // for SDL_Renderer, SDL_Texture

#include <stdint.h>  // for int64_t
#include <string>    // for string

#include "ffmpeg_config.h"  // for CONFIG_AVFILTER

extern "C" {
#include <libavcodec/avcodec.h>  // for AVCodec, AVCodecID
#if CONFIG_AVFILTER
#include <libavfilter/avfilter.h>
#endif
#include <libavformat/avformat.h>  // for AVFormatContext, AVStream
#include <libavutil/attributes.h>  // for av_noreturn
#include <libavutil/dict.h>        // for AVDictionary
#include <libavutil/frame.h>       // for AVFrame
#include <libavutil/rational.h>    // for AVRational
#include <libavutil/samplefmt.h>   // for AVSampleFormat
}

/* Minimum SDL audio buffer size, in samples. */
#define AUDIO_MIN_BUFFER_SIZE 512

struct SwsContext;

namespace fasto {
namespace fastotv {
namespace core {

double q2d_diff(AVRational a);

/**
 * Filter out options for given codec.
 *
 * Create a new options dictionary containing only the options from
 * opts which apply to the codec with ID codec_id.
 *
 * @param opts     dictionary to place options in
 * @param codec_id ID of the codec that should be filtered for
 * @param s Corresponding format context.
 * @param st A stream from s for which the options should be filtered.
 * @param codec The particular codec for which the options should be filtered.
 *              If null, the default one is looked up according to the codec id.
 * @return a pointer to the created dictionary
 */
AVDictionary* filter_codec_opts(AVDictionary* opts,
                                enum AVCodecID codec_id,
                                AVFormatContext* s,
                                AVStream* st,
                                AVCodec* codec);

/**
 * Setup AVCodecContext options for avformat_find_stream_info().
 *
 * Create an array of dictionaries, one dictionary for each stream
 * contained in s.
 * Each dictionary will contain the options from codec_opts which can
 * be applied to the corresponding stream codec context.
 *
 * @return pointer to the created array of dictionaries, NULL if it
 * cannot be created
 */
AVDictionary** setup_find_stream_info_opts(AVFormatContext* s, AVDictionary* codec_opts);

/**
 * Check if the given stream matches a stream specifier.
 *
 * @param s  Corresponding format context.
 * @param st Stream from s to be checked.
 * @param spec A stream specifier of the [v|a|s|d]:[\<stream index\>] form.
 *
 * @return 1 if the stream matches, 0 if it doesn't, <0 on error
 */
int check_stream_specifier(AVFormatContext* s, AVStream* st, const char* spec);

double get_rotation(AVStream* st);

/**
 * Wraps exit with a program-specific cleanup routine.
 */
void exit_program(int ret) av_noreturn;

#if CONFIG_AVFILTER
int configure_filtergraph(AVFilterGraph* graph,
                          const std::string& filtergraph,
                          AVFilterContext* source_ctx,
                          AVFilterContext* sink_ctx);
#endif

void calculate_display_rect(SDL_Rect* rect,
                            int scr_xleft,
                            int scr_ytop,
                            int scr_width,
                            int scr_height,
                            int pic_width,
                            int pic_height,
                            AVRational pic_sar);

int upload_texture(SDL_Texture* tex, const AVFrame* frame);
int audio_open(void* opaque,
               int64_t wanted_channel_layout,
               int wanted_nb_channels,
               int wanted_sample_rate,
               struct AudioParams* audio_hw_params,
               SDL_AudioCallback cb);

bool is_realtime(AVFormatContext* s);

int cmp_audio_fmts(enum AVSampleFormat fmt1,
                   int64_t channel_count1,
                   enum AVSampleFormat fmt2,
                   int64_t channel_count2);

}  // namespace core
}}
