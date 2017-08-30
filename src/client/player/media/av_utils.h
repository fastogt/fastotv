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

#pragma once

#include "ffmpeg_config.h"  // for CONFIG_AVFILTER

extern "C" {
#if CONFIG_AVFILTER
#include <libavfilter/avfilter.h>
#endif
#include <libavformat/avformat.h>  // for AVFormatContext, AVStream
#include <libavutil/frame.h>       // for AVFrame
#include <libavutil/rational.h>    // for AVRational
}

namespace fastotv {
namespace client {
namespace player {
namespace media {

double q2d_diff(AVRational a);

AVRational guess_sample_aspect_ratio(AVStream* stream, AVFrame* frame);

double get_rotation(AVStream* st);

bool is_realtime(AVFormatContext* s);

#if CONFIG_AVFILTER
int configure_filtergraph(AVFilterGraph* graph,
                          const char* filtergraph,
                          AVFilterContext* source_ctx,
                          AVFilterContext* sink_ctx);
#endif

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

}  // namespace media
}  // namespace player
}  // namespace client
}  // namespace fastotv
